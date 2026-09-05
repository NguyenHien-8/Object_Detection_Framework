#include "workers/InferenceWorker.hpp"

#include "odf/logging.hpp"

#include <utility>

namespace odf::desktop {

InferenceWorker::InferenceWorker(const app::RuntimeCatalog& catalog, QObject* parent)
    : QObject(parent), catalog_(catalog), thread_(&InferenceWorker::run, this) {}

InferenceWorker::~InferenceWorker() {
    stop();
}

void InferenceWorker::requestLoad(QString modelId, QString backendName,
                                  backend::BackendConfig config,
                                  bool allowUnvalidatedModel) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++desiredGeneration_;
        pendingLoad_ = LoadRequest{desiredGeneration_, modelId.toStdString(),
                                   backendName.toStdString(), std::move(config),
                                   allowUnvalidatedModel};
        pendingFrame_.reset();
        droppedFrames_ = 0;
    }
    condition_.notify_one();
}

bool InferenceWorker::submitFrame(ImageFramePtr frame, std::uint64_t sourceGeneration) {
    if (!frame) return false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_ || sourceGeneration != activeSourceGeneration_) {
            logging::log(logging::Level::Debug,
                         "stale source frame discarded before inference: generation=" +
                             std::to_string(sourceGeneration) + " active=" +
                             std::to_string(activeSourceGeneration_));
            return false;
        }
        if (pendingFrame_) ++droppedFrames_;
        pendingFrame_ = PendingFrame{std::move(frame), sourceGeneration};
    }
    condition_.notify_one();
    return true;
}

void InferenceWorker::invalidateSource(std::uint64_t sourceGeneration) {
    std::lock_guard<std::mutex> lock(mutex_);
    activeSourceGeneration_ = sourceGeneration;
    droppedFrames_ = 0;
    if (pendingFrame_) {
        logging::log(logging::Level::Debug,
                     "pending inference frame discarded: new source generation=" +
                         std::to_string(sourceGeneration));
        pendingFrame_.reset();
    }
}

void InferenceWorker::setOptions(detection::InferenceOptions options) {
    std::lock_guard<std::mutex> lock(mutex_);
    options_ = std::move(options);
}

void InferenceWorker::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) return;
        stopping_ = true;
        pendingFrame_.reset();
        pendingLoad_.reset();
    }
    condition_.notify_one();
    if (thread_.joinable()) thread_.join();
}

void InferenceWorker::run() {
    std::unique_ptr<pipeline::IDetector> detector;
    std::uint64_t loadedGeneration = 0;
    std::vector<std::string> labels;

    while (true) {
        std::optional<LoadRequest> load;
        PendingFrame pending;
        detection::InferenceOptions options;
        std::uint64_t dropped = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] {
                return stopping_ || pendingLoad_.has_value() || pendingFrame_.has_value();
            });
            if (stopping_) break;
            if (pendingLoad_) {
                load = std::move(pendingLoad_);
                pendingLoad_.reset();
                pendingFrame_.reset();
            } else {
                pending = std::move(*pendingFrame_);
                pendingFrame_.reset();
                options = options_;
                dropped = droppedFrames_;
            }
        }

        if (load) {
            detector.reset();
            labels.clear();
            emit stateChanged(QStringLiteral("Loading"),
                              QString::fromStdString(load->modelId));
            auto created = catalog_.createLoadedDetector(
                load->modelId, load->backend, load->config, load->allowUnvalidatedModel);
            if (!created.ok()) {
                emit failed(QStringLiteral("Model load"),
                            QString::fromStdString(created.status().message()));
                continue;
            }
            const auto info = created.value()->modelInfo();
            if (!info) {
                emit failed(QStringLiteral("Model load"),
                            QStringLiteral("Backend loaded without model metadata"));
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (load->generation != desiredGeneration_) continue;
            }
            labels = info->labels;
            detector = created.takeValue();
            loadedGeneration = load->generation;
            QStringList qtLabels;
            for (const auto& label : labels) qtLabels.push_back(QString::fromStdString(label));
            emit modelLoaded(loadedGeneration, QString::fromStdString(info->id), qtLabels);
            emit stateChanged(QStringLiteral("Ready"), QString::fromStdString(info->displayName));
            continue;
        }

        if (!pending.frame) continue;
        if (!detector) {
            emit failed(QStringLiteral("Inference"),
                        QStringLiteral("Load a model before starting inference"));
            continue;
        }
        auto result = detector->detect(*pending.frame, options);
        if (!result.ok()) {
            emit failed(QStringLiteral("Inference"),
                        QString::fromStdString(result.status().message()));
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (loadedGeneration != desiredGeneration_) continue;
            if (pending.sourceGeneration != activeSourceGeneration_) {
                logging::log(logging::Level::Debug,
                             "stale detection discarded after inference: generation=" +
                                 std::to_string(pending.sourceGeneration) + " active=" +
                                 std::to_string(activeSourceGeneration_));
                continue;
            }
        }
        auto packet = std::make_shared<DetectionPacket>();
        packet->frame = *pending.frame;
        packet->result = result.takeValue();
        packet->result.modelGeneration = loadedGeneration;
        packet->labels = labels;
        packet->requestGeneration = loadedGeneration;
        packet->sourceGeneration = pending.sourceGeneration;
        packet->droppedFrames = dropped;
        emit detectionReady(std::move(packet));
    }
    if (detector) detector->unload();
}

}  // namespace odf::desktop
