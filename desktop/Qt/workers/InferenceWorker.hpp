#pragma once

#include "odf/app/runtime_catalog.hpp"
#include "odf/detection/detection.hpp"
#include "odf/image/image.hpp"

#include <QObject>
#include <QString>
#include <QStringList>

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace odf::desktop {

struct DetectionPacket {
    image::ImageFrame frame;
    detection::DetectionResult result;
    std::vector<std::string> labels;
    std::uint64_t requestGeneration{0};
    std::uint64_t sourceGeneration{0};
    std::uint64_t droppedFrames{0};
};

using DetectionPacketPtr = std::shared_ptr<const DetectionPacket>;
using ImageFramePtr = std::shared_ptr<const image::ImageFrame>;

class InferenceWorker final : public QObject {
    Q_OBJECT

public:
    explicit InferenceWorker(const app::RuntimeCatalog& catalog, QObject* parent = nullptr);
    ~InferenceWorker() override;

    void requestLoad(QString modelId, QString backend, backend::BackendConfig config,
                     bool allowUnvalidatedModel);
    bool submitFrame(ImageFramePtr frame, std::uint64_t sourceGeneration);
    void invalidateSource(std::uint64_t sourceGeneration);
    void setOptions(detection::InferenceOptions options);
    void stop();

signals:
    void stateChanged(QString state, QString detail);
    void modelLoaded(quint64 generation, QString modelId, QStringList labels);
    void detectionReady(odf::desktop::DetectionPacketPtr packet);
    void failed(QString operation, QString detail);

private:
    struct LoadRequest {
        std::uint64_t generation{0};
        std::string modelId;
        std::string backend;
        backend::BackendConfig config;
        bool allowUnvalidatedModel{false};
    };

    struct PendingFrame {
        ImageFramePtr frame;
        std::uint64_t sourceGeneration{0};
    };

    void run();

    const app::RuntimeCatalog& catalog_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stopping_{false};
    std::uint64_t desiredGeneration_{0};
    std::uint64_t activeSourceGeneration_{0};
    std::uint64_t droppedFrames_{0};
    std::optional<LoadRequest> pendingLoad_;
    std::optional<PendingFrame> pendingFrame_;
    detection::InferenceOptions options_;
};

}  // namespace odf::desktop

Q_DECLARE_METATYPE(odf::desktop::DetectionPacketPtr)
Q_DECLARE_METATYPE(odf::desktop::ImageFramePtr)
