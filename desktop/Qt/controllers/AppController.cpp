#include "controllers/AppController.hpp"

#include "bridge/QtImageBridge.hpp"
#include "odf/app/opencv_bridge.hpp"
#include "odf/logging.hpp"
#include "widgets/MainWindow.hpp"
#include "workers/CameraWorker.hpp"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMetaObject>

#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace odf::desktop {
namespace {
constexpr int kCameraOpenTimeoutMs = 5000;
constexpr int kCameraStopTimeoutMs = 3000;

std::string quoted(const QString& value) {
    return "'" + value.toStdString() + "'";
}
}  // namespace

AppController::AppController(MainWindow& window, std::filesystem::path modelRoot,
                             app::CommandLineOptions commandLine, QObject* parent)
    : QObject(parent), window_(window), modelRoot_(std::move(modelRoot)),
      commandLine_(std::move(commandLine)) {
    qRegisterMetaType<DetectionPacketPtr>();
    qRegisterMetaType<ImageFramePtr>();
    qRegisterMetaType<CameraOpenRequest>();

    cameraOpenWatchdog_.setSingleShot(true);
    cameraOpenWatchdog_.setInterval(kCameraOpenTimeoutMs);
    connect(&cameraOpenWatchdog_, &QTimer::timeout, this,
            &AppController::onCameraOpenTimeout);
    cameraStopWatchdog_.setSingleShot(true);
    cameraStopWatchdog_.setInterval(kCameraStopTimeoutMs);
    connect(&cameraStopWatchdog_, &QTimer::timeout, this,
            &AppController::onCameraStopTimeout);
}

AppController::~AppController() {
    shutdown();
}

Status AppController::initialize() {
    settings_ = settingsService_.load();
    settings_.modelRootOverride = QString::fromStdWString(modelRoot_.wstring());
    const auto registryStatus = catalog_.loadRegistry(modelRoot_);
    if (!registryStatus.ok()) return registryStatus;

    QSet<QString> availableBackends;
    for (const auto& backend : catalog_.backendInfos()) {
        if (backend.available) availableBackends.insert(QString::fromStdString(backend.name));
    }
    std::vector<ModelChoice> choices;
    for (const auto& model : catalog_.registry().models()) {
        ModelChoice choice;
        choice.id = QString::fromStdString(model.id);
        choice.displayName = QString::fromStdString(model.displayName);
        choice.framework = model.sourceFramework == model::SourceFramework::PyTorch
                               ? QStringLiteral("PyTorch")
                               : QStringLiteral("PaddlePaddle");
        choice.inputSize = QStringLiteral("%1×%2").arg(model.input.width).arg(model.input.height);
        choice.defaultBackend = QString::fromStdString(model.defaultBackend);
        choice.deploymentValidated = model.deploymentValidated;
        for (const auto& backend : model.supportedBackends) {
            choice.supportedBackends.push_back(QString::fromStdString(backend));
        }
        choices.push_back(std::move(choice));
    }
    window_.setCatalog(choices, availableBackends);
    window_.selectModel(commandLine_.modelSpecified
                            ? QString::fromStdString(commandLine_.model)
                            : settings_.modelId);
    window_.selectBackend(commandLine_.backendSpecified
                              ? QString::fromStdString(commandLine_.backend)
                              : settings_.backend);
    const double confidence = commandLine_.confidenceSpecified
                                  ? commandLine_.confidence
                                  : settings_.confidence;
    const double iou = commandLine_.iouSpecified ? commandLine_.iou : settings_.iou;
    const int maximum = commandLine_.maxDetectionsSpecified
                            ? static_cast<int>(commandLine_.maxDetections)
                            : settings_.maxDetections;
    window_.setThresholds(confidence, iou, maximum);

    const int requestedLegacyIndex = commandLine_.camera
                                         ? *commandLine_.camera
                                         : settings_.legacyCameraIndex;
    enumerateCameraDevices(settings_.cameraDeviceId, requestedLegacyIndex);
    if (commandLine_.camera) {
        const auto found = std::find_if(
            cameraDevices_.begin(), cameraDevices_.end(), [this](const CameraDeviceInfo& device) {
                return device.backendIndex == *commandLine_.camera;
            });
        if (found != cameraDevices_.end()) startupCameraId_ = found->stableId;
    }
    if (!settings_.windowGeometry.isEmpty()) window_.restoreGeometry(settings_.windowGeometry);

    inference_ = std::make_unique<InferenceWorker>(catalog_);
    inference_->invalidateSource(sourceGeneration_);
    connect(inference_.get(), &InferenceWorker::stateChanged, &window_,
            [this](const QString& state, const QString& detail) {
                window_.setStatus(state, detail, false);
            });
    connect(inference_.get(), &InferenceWorker::modelLoaded, this,
            &AppController::onModelLoaded);
    connect(inference_.get(), &InferenceWorker::detectionReady, this,
            &AppController::onDetection);
    connect(inference_.get(), &InferenceWorker::failed, this, &AppController::onFailure);

    camera_ = new CameraWorker();
    camera_->moveToThread(&cameraThread_);
    connect(&cameraThread_, &QThread::finished, camera_, &QObject::deleteLater);
    connect(this, &AppController::startCameraWorker, camera_, &CameraWorker::start,
            Qt::QueuedConnection);
    connect(this, &AppController::stopCameraWorker, camera_, &CameraWorker::stop,
            Qt::QueuedConnection);
    connect(camera_, &CameraWorker::opening, this, &AppController::onCameraOpening);
    connect(camera_, &CameraWorker::started, this, &AppController::onCameraStarted);
    connect(camera_, &CameraWorker::stopped, this, &AppController::onCameraStopped);
    connect(camera_, &CameraWorker::frameCaptured, this, &AppController::onCameraFrame);
    connect(camera_, &CameraWorker::failed, this, &AppController::onCameraFailed);
    cameraThread_.setObjectName(QStringLiteral("ODF Camera Capture"));
    cameraThread_.start();

    connect(&window_, &MainWindow::loadModelRequested, this, &AppController::loadModel);
    connect(&window_, &MainWindow::refreshRequested, this, &AppController::refreshSources);
    connect(&window_, &MainWindow::configurationEdited, this,
            [this](const QString&, const QString&) {
                if (modelState_ == ModelState::Ready) {
                    window_.setStatus(
                        QStringLiteral("Pending configuration"),
                        QStringLiteral("The loaded model remains active until Load model is clicked"),
                        false);
                }
            });
    connect(&window_, &MainWindow::openImageRequested, this, &AppController::openImage);
    connect(&window_, &MainWindow::startCameraRequested, this, &AppController::startCamera);
    connect(&window_, &MainWindow::stopCameraRequested, this, &AppController::stopCamera);
    connect(&window_, &MainWindow::thresholdsChanged, this, &AppController::updateThresholds);
    connect(&window_, &MainWindow::classSelectionChanged, this,
            &AppController::updateClassSelection);
    connect(&window_, &MainWindow::closing, this, &AppController::shutdown);

    updateThresholds(window_.confidence(), window_.iou(), window_.maxDetections());
    window_.setStatus(QStringLiteral("Registry ready"),
                      QString::fromStdString(modelRoot_.string()), false);
    loadModel(window_.selectedModel(), window_.selectedBackend());
    if (commandLine_.image) loadImagePath(*commandLine_.image);
    return Status::success();
}

void AppController::loadModel(const QString& modelId, const QString& backendName) {
    if (!inference_ || modelId.isEmpty() || backendName.isEmpty()) {
        onFailure(QStringLiteral("Model load"),
                  QStringLiteral("No compatible model/backend selection is available"));
        return;
    }
    if (cameraReleasePending_) {
        window_.setStatus(
            QStringLiteral("Model load blocked"),
            QStringLiteral("The camera driver has not acknowledged release; restart the application if it remains unresponsive"),
            true);
        return;
    }
    if (refreshActive_) return;

    if (cameraSession_.state() == CameraState::Opening ||
        cameraSession_.state() == CameraState::Running ||
        cameraSession_.state() == CameraState::Stopping) {
        pendingModelLoad_ = PendingModelLoad{modelId, backendName};
        window_.setModelLoading(true);
        beginCameraStop(AfterCameraStop::LoadModel);
        return;
    }
    performModelLoad(modelId, backendName);
}

void AppController::performModelLoad(const QString& modelId, const QString& backendName) {
    pendingModelLoad_.reset();
    afterCameraStop_ = AfterCameraStop::None;
    invalidateSource();
    modelState_ = ModelState::Loading;
    selectedClasses_.clear();
    window_.setModelReady(false);
    window_.setModelLoading(true);
    backend::BackendConfig config;
    config.device = "CPU";
    config.precision = model::Precision::Fp32;
    inference_->requestLoad(modelId, backendName, config,
                            commandLine_.allowUnvalidatedModel);
}

void AppController::refreshSources() {
    if (refreshActive_ || modelState_ == ModelState::Loading || shuttingDown_) return;

    refreshActive_ = true;
    ++refreshGeneration_;
    refreshPreferredCameraId_ = window_.selectedCameraId();
    refreshRestartCamera_ = cameraSession_.state() == CameraState::Opening ||
                            cameraSession_.state() == CameraState::Running;
    window_.setRefreshActive(true);
    logging::log(logging::Level::Debug,
                 "source refresh request: generation=" +
                     std::to_string(refreshGeneration_) + " camera=" +
                     quoted(refreshPreferredCameraId_));

    if (cameraReleasePending_) {
        enumerateCameraDevices(refreshPreferredCameraId_);
        completeRefresh(
            QStringLiteral("Device list refreshed, but the previous camera release is still pending; restart the application if the driver remains unresponsive"),
            true);
        return;
    }

    if (cameraSession_.state() == CameraState::Opening ||
        cameraSession_.state() == CameraState::Running) {
        beginCameraStop(AfterCameraStop::Refresh);
        return;
    }
    if (cameraSession_.state() == CameraState::Stopping) {
        afterCameraStop_ = AfterCameraStop::Refresh;
        return;
    }

    invalidateSource();
    if (!enumerateCameraDevices(refreshPreferredCameraId_)) {
        completeRefresh(QStringLiteral("Camera device enumeration failed"), true);
        return;
    }
    if (currentImage_ && modelState_ == ModelState::Ready) submitCurrentImage();
    completeRefresh(cameraDevices_.empty() ? QStringLiteral("No camera detected")
                                           : QStringLiteral("Camera devices refreshed"),
                    false);
}

void AppController::openImage() {
    const QString path = QFileDialog::getOpenFileName(
        &window_, QStringLiteral("Open image"), settings_.lastImageDirectory,
        QStringLiteral("Images (*.jpg *.jpeg *.png *.bmp *.webp);;All files (*)"));
    if (path.isEmpty()) return;
    loadImagePath(std::filesystem::path(path.toStdWString()));
}

void AppController::loadImagePath(const std::filesystem::path& path) {
    const QString qtPath = QString::fromStdWString(path.wstring());
    QFile file(qtPath);
    if (!file.open(QIODevice::ReadOnly)) {
        onFailure(QStringLiteral("Open image"),
                  QStringLiteral("Could not read image: %1").arg(qtPath));
        return;
    }
    const QByteArray encoded = file.readAll();
    const std::vector<unsigned char> bytes(encoded.begin(), encoded.end());
    const cv::Mat source = cv::imdecode(bytes, cv::IMREAD_COLOR);
    if (source.empty()) {
        onFailure(QStringLiteral("Open image"),
                  QStringLiteral("Unsupported or corrupt image: %1").arg(qtPath));
        return;
    }
    auto converted = app::toOdfImage(source);
    if (!converted.ok()) {
        onFailure(QStringLiteral("Open image"),
                  QString::fromStdString(converted.status().message()));
        return;
    }
    invalidateSource();
    auto frame = std::make_shared<image::ImageFrame>();
    frame->image = converted.takeValue();
    frame->timestamp = std::chrono::steady_clock::now();
    frame->frameId = static_cast<std::uint64_t>(frame->timestamp.time_since_epoch().count());
    frame->source = path.string();
    currentImage_ = frame;
    settings_.lastImageDirectory = QFileInfo(qtPath).absolutePath();
    auto display = toQImage(frame->image);
    if (display.ok()) window_.showSourceFrame(display.takeValue());
    submitCurrentImage();
}

void AppController::startCamera(const QString& stableId) {
    if (refreshActive_ || cameraReleasePending_) return;
    beginCameraStart(stableId);
}

void AppController::beginCameraStart(const QString& stableId) {
    if (modelState_ != ModelState::Ready ||
        (cameraSession_.state() != CameraState::Idle &&
         cameraSession_.state() != CameraState::Error)) {
        return;
    }
    const auto* device = findCameraDevice(stableId);
    if (!device) {
        window_.setStatus(QStringLiteral("Camera unavailable"),
                          QStringLiteral("Refresh the device list and select an available camera"),
                          true);
        return;
    }

    ++cameraSessionSequence_;
    if (cameraSessionSequence_ == 0) ++cameraSessionSequence_;
    if (!cameraSession_.beginOpening(cameraSessionSequence_)) return;

    invalidateSource();
    activeCameraSourceGeneration_ = sourceGeneration_;
    currentImage_.reset();
    settings_.cameraDeviceId = device->stableId;
    window_.selectCameraById(device->stableId);
    window_.setCameraRecoveryBlocked(false);
    window_.setCameraState(CameraState::Opening);
    window_.setStatus(QStringLiteral("Opening camera"), device->displayName, false);
    cameraOpenWatchdog_.start();

    CameraOpenRequest request;
    request.sessionId = cameraSession_.activeSession();
    request.stableId = device->stableId;
    request.displayName = device->displayName;
    request.backendIndex = device->backendIndex;
    request.backendApi = device->backendApi;
    emit startCameraWorker(std::move(request));
}

void AppController::stopCamera() {
    if (refreshActive_) return;
    beginCameraStop(AfterCameraStop::None);
}

void AppController::beginCameraStop(AfterCameraStop afterStop) {
    if (afterStop != AfterCameraStop::None) afterCameraStop_ = afterStop;
    if (cameraSession_.state() == CameraState::Stopping) return;
    if (!cameraSession_.beginStopping()) {
        continueAfterCameraStop();
        return;
    }

    cameraOpenWatchdog_.stop();
    invalidateSource();
    window_.setCameraState(CameraState::Stopping);
    window_.setStatus(QStringLiteral("Stopping camera"),
                      displayNameForCamera(settings_.cameraDeviceId), false);
    const quint64 sessionId = cameraSession_.activeSession();
    cameraStopWatchdog_.start();
    emit stopCameraWorker(sessionId);
}

void AppController::onCameraOpening(quint64 sessionId, const QString& deviceId) {
    if (!cameraSession_.accepts(sessionId) ||
        cameraSession_.state() != CameraState::Opening) {
        logging::log(logging::Level::Debug,
                     "stale camera opening acknowledgement ignored: session=" +
                         std::to_string(sessionId));
        return;
    }
    window_.setStatus(QStringLiteral("Opening camera"), displayNameForCamera(deviceId), false);
}

void AppController::onCameraStarted(quint64 sessionId, const QString& deviceId) {
    if (!cameraSession_.acknowledgeStarted(sessionId)) {
        logging::log(logging::Level::Debug,
                     "stale camera started acknowledgement ignored: session=" +
                         std::to_string(sessionId));
        return;
    }
    cameraOpenWatchdog_.stop();
    markCameraWorkerResponsive();
    window_.setCameraState(CameraState::Running);
    window_.setStatus(QStringLiteral("Camera running"), displayNameForCamera(deviceId), false);
    if (refreshActive_) {
        completeRefresh(QStringLiteral("Camera restarted: %1")
                            .arg(displayNameForCamera(deviceId)),
                        false);
    }
}

void AppController::onCameraStopped(quint64 sessionId) {
    if (cameraReleasePending_ && sessionId == timedOutCameraSession_) {
        markCameraWorkerResponsive();
        window_.setCameraState(CameraState::Idle);
        window_.setStatus(QStringLiteral("Camera recovered"),
                          QStringLiteral("The delayed release acknowledgement was received"),
                          false);
        return;
    }
    if (!cameraSession_.acknowledgeStopped(sessionId)) {
        logging::log(logging::Level::Debug,
                     "stale camera stopped acknowledgement ignored: session=" +
                         std::to_string(sessionId));
        return;
    }
    cameraStopWatchdog_.stop();
    markCameraWorkerResponsive();
    window_.setCameraState(CameraState::Idle);
    logging::log(logging::Level::Debug,
                 "camera stopped acknowledgement: session=" + std::to_string(sessionId));
    continueAfterCameraStop();
}

void AppController::onCameraFrame(quint64 sessionId, ImageFramePtr frame) {
    if (!frame || !cameraSession_.accepts(sessionId) ||
        cameraSession_.state() != CameraState::Running ||
        activeCameraSourceGeneration_ != sourceGeneration_) {
        logging::log(logging::Level::Debug,
                     "stale camera frame discarded: session=" + std::to_string(sessionId));
        return;
    }
    if (inference_) inference_->submitFrame(std::move(frame), activeCameraSourceGeneration_);
}

void AppController::onCameraFailed(quint64 sessionId, const QString& operation,
                                   const QString& detail) {
    if (cameraReleasePending_ && sessionId == timedOutCameraSession_) {
        markCameraWorkerResponsive();
        window_.setCameraState(CameraState::Error);
        window_.setStatus(operation + QStringLiteral(" failed"), detail, true);
        return;
    }
    if (!cameraSession_.fail(sessionId)) {
        logging::log(logging::Level::Debug,
                     "stale camera failure ignored: session=" + std::to_string(sessionId));
        return;
    }
    cameraOpenWatchdog_.stop();
    cameraStopWatchdog_.stop();
    markCameraWorkerResponsive();
    invalidateSource();
    window_.setCameraState(CameraState::Error);

    const AfterCameraStop continuation = afterCameraStop_;
    afterCameraStop_ = AfterCameraStop::None;
    if (continuation == AfterCameraStop::LoadModel && pendingModelLoad_) {
        const auto request = *pendingModelLoad_;
        performModelLoad(request.modelId, request.backend);
        return;
    }
    if (refreshActive_) {
        completeRefresh(detail, true);
        return;
    }
    window_.setStatus(operation + QStringLiteral(" failed"), detail, true);
}

void AppController::continueAfterCameraStop() {
    const AfterCameraStop continuation = afterCameraStop_;
    afterCameraStop_ = AfterCameraStop::None;
    if (continuation == AfterCameraStop::Refresh) {
        continueRefreshAfterStop();
        return;
    }
    if (continuation == AfterCameraStop::LoadModel && pendingModelLoad_) {
        const auto request = *pendingModelLoad_;
        performModelLoad(request.modelId, request.backend);
        return;
    }
    window_.setStatus(QStringLiteral("Camera stopped"),
                      displayNameForCamera(settings_.cameraDeviceId), false);
}

void AppController::continueRefreshAfterStop() {
    if (!refreshActive_) return;
    const QString previousId = refreshPreferredCameraId_;
    if (!enumerateCameraDevices(previousId)) {
        completeRefresh(QStringLiteral("Camera device enumeration failed"), true);
        return;
    }
    if (refreshRestartCamera_) {
        if (findCameraDevice(previousId)) {
            beginCameraStart(previousId);
            return;
        }
        completeRefresh(
            cameraDevices_.empty()
                ? QStringLiteral("The previous camera disappeared; no camera is currently detected")
                : QStringLiteral("The previous camera disappeared; select an available device to restart"),
            true);
        return;
    }
    if (currentImage_ && modelState_ == ModelState::Ready) submitCurrentImage();
    completeRefresh(cameraDevices_.empty() ? QStringLiteral("No camera detected")
                                           : QStringLiteral("Camera devices refreshed"),
                    false);
}

bool AppController::enumerateCameraDevices(const QString& preferredStableId,
                                           int legacyBackendIndex) {
    auto enumerated = cameraDeviceService_.enumerate();
    if (!enumerated.ok()) {
        cameraDevices_.clear();
        window_.setCameraDevices({});
        window_.setStatus(QStringLiteral("Camera discovery failed"),
                          QString::fromStdString(enumerated.status().message()), true);
        return false;
    }
    cameraDevices_ = enumerated.takeValue();
    const int preferred = CameraDeviceService::preferredDeviceIndex(
        cameraDevices_, preferredStableId, legacyBackendIndex);
    window_.setCameraDevices(cameraDevices_);
    if (preferred >= 0) {
        const auto& selected = cameraDevices_[static_cast<std::size_t>(preferred)];
        window_.selectCameraById(selected.stableId);
        settings_.cameraDeviceId = selected.stableId;
    } else {
        settings_.cameraDeviceId.clear();
    }
    return true;
}

const CameraDeviceInfo* AppController::findCameraDevice(const QString& stableId) const {
    const auto found = std::find_if(cameraDevices_.begin(), cameraDevices_.end(),
                                    [&stableId](const CameraDeviceInfo& device) {
                                        return device.stableId == stableId;
                                    });
    return found == cameraDevices_.end() ? nullptr : &*found;
}

QString AppController::displayNameForCamera(const QString& stableId) const {
    const auto* device = findCameraDevice(stableId);
    return device ? device->displayName : stableId;
}

void AppController::completeRefresh(const QString& detail, bool error) {
    refreshActive_ = false;
    refreshRestartCamera_ = false;
    refreshPreferredCameraId_.clear();
    window_.setRefreshActive(false);
    window_.setStatus(error ? QStringLiteral("Refresh failed")
                            : QStringLiteral("Refreshed"),
                      detail, error);
}

void AppController::invalidateSource() {
    ++sourceGeneration_;
    if (sourceGeneration_ == 0) ++sourceGeneration_;
    currentPacket_.reset();
    if (inference_) inference_->invalidateSource(sourceGeneration_);
}

void AppController::updateThresholds(double confidence, double iou, int maxDetections) {
    settings_.confidence = confidence;
    settings_.iou = iou;
    settings_.maxDetections = maxDetections;
    applyInferenceOptions();
    if (cameraSession_.state() != CameraState::Opening &&
        cameraSession_.state() != CameraState::Running &&
        cameraSession_.state() != CameraState::Stopping) {
        submitCurrentImage();
    }
}

void AppController::applyInferenceOptions() {
    if (!inference_) return;
    detection::InferenceOptions options;
    options.confidenceThreshold = static_cast<float>(settings_.confidence);
    options.iouThreshold = static_cast<float>(settings_.iou);
    options.maxDetections = static_cast<std::size_t>(settings_.maxDetections);
    for (const int classId : selectedClasses_) options.selectedClassIds.push_back(classId);
    std::sort(options.selectedClassIds.begin(), options.selectedClassIds.end());
    inference_->setOptions(std::move(options));
}

void AppController::updateClassSelection(const QSet<int>& selectedClasses) {
    selectedClasses_ = selectedClasses;
    applyInferenceOptions();
    if (currentPacket_) displayPacket(*currentPacket_);
}

void AppController::onModelLoaded(quint64, const QString& modelId,
                                  const QStringList& labels) {
    const bool restoreClasses = settings_.classSelectionStored && settings_.modelId == modelId;
    modelState_ = ModelState::Ready;
    window_.setModelLoading(false);
    window_.setModelReady(true);
    window_.setClasses(labels, !restoreClasses);
    if (restoreClasses) {
        QSet<int> restored;
        for (const int classId : settings_.selectedClasses) {
            if (classId >= 0 && classId < labels.size()) restored.insert(classId);
        }
        window_.setSelectedClasses(restored);
    }
    settings_.modelId = modelId;
    settings_.backend = window_.selectedBackend();
    const auto* spec = catalog_.registry().find(modelId.toStdString());
    if (spec) {
        const QString loadedConfiguration =
            QStringLiteral("%1 | %2 | CPU | FP32 | %3×%4")
                .arg(QString::fromStdString(spec->displayName), window_.selectedBackend())
                .arg(spec->input.width)
                .arg(spec->input.height);
        window_.setLoadedConfiguration(loadedConfiguration);
        window_.setStatus(QStringLiteral("Ready"), loadedConfiguration, false);
    }
    submitCurrentImage();
    if (!initialSourceStarted_ && commandLine_.camera) {
        initialSourceStarted_ = true;
        if (startupCameraId_.isEmpty()) {
            window_.setStatus(
                QStringLiteral("Camera unavailable"),
                QStringLiteral("Requested camera index %1 was not discovered")
                    .arg(*commandLine_.camera),
                true);
        } else {
            beginCameraStart(startupCameraId_);
        }
    }
}

void AppController::onDetection(DetectionPacketPtr packet) {
    if (!packet || packet->sourceGeneration != sourceGeneration_) {
        if (packet) {
            logging::log(logging::Level::Debug,
                         "stale detection discarded in controller: generation=" +
                             std::to_string(packet->sourceGeneration) + " active=" +
                             std::to_string(sourceGeneration_));
        }
        return;
    }
    currentPacket_ = std::move(packet);
    displayPacket(*currentPacket_);
}

void AppController::onFailure(const QString& operation, const QString& detail) {
    if (operation == QStringLiteral("Model load")) {
        modelState_ = ModelState::Error;
        window_.setModelLoading(false);
        window_.setModelReady(false);
    }
    if (operation == QStringLiteral("Inference") &&
        (cameraSession_.state() == CameraState::Opening ||
         cameraSession_.state() == CameraState::Running)) {
        beginCameraStop(AfterCameraStop::None);
    }
    window_.setStatus(operation + QStringLiteral(" failed"), detail, true);
}

void AppController::submitCurrentImage() {
    if (modelState_ == ModelState::Ready && currentImage_ && inference_) {
        inference_->submitFrame(currentImage_, sourceGeneration_);
    }
}

void AppController::displayPacket(const DetectionPacket& packet) {
    auto image = toQImage(packet.frame.image);
    if (!image.ok()) {
        onFailure(QStringLiteral("Display"), QString::fromStdString(image.status().message()));
        return;
    }
    std::vector<detection::Detection> filtered;
    if (!selectedClasses_.isEmpty()) {
        for (const auto& detection : packet.result.detections) {
            if (selectedClasses_.contains(detection.classId)) filtered.push_back(detection);
        }
    }
    const auto& timings = packet.result.timings;
    MetricsView metrics;
    metrics.preprocessMs = timings.preprocessMs;
    metrics.inferenceMs = timings.inferenceMs;
    metrics.postprocessMs = timings.postprocessMs;
    metrics.totalMs = timings.totalMs;
    metrics.fps = timings.totalMs > 0.0 ? 1000.0 / timings.totalMs : 0.0;
    metrics.detections = filtered.size();
    metrics.droppedFrames = packet.droppedFrames;
    window_.setFrame(image.takeValue(), std::move(filtered), packet.labels);
    window_.setMetrics(metrics);
}

void AppController::onCameraOpenTimeout() {
    handleCameraWatchdogTimeout(QStringLiteral("open/first-frame"));
}

void AppController::onCameraStopTimeout() {
    handleCameraWatchdogTimeout(QStringLiteral("stop/release"));
}

void AppController::handleCameraWatchdogTimeout(const QString& phase) {
    const quint64 sessionId = cameraSession_.activeSession();
    if (sessionId == 0 || !cameraSession_.fail(sessionId)) return;

    cameraOpenWatchdog_.stop();
    cameraStopWatchdog_.stop();
    invalidateSource();
    cameraReleasePending_ = true;
    timedOutCameraSession_ = sessionId;
    window_.setCameraRecoveryBlocked(true);
    window_.setCameraState(CameraState::Error);
    logging::log(logging::Level::Warning,
                 "camera watchdog timeout: phase=" + phase.toStdString() +
                     " session=" + std::to_string(sessionId));
    emit stopCameraWorker(sessionId);

    const bool modelLoadWasPending = pendingModelLoad_.has_value();
    pendingModelLoad_.reset();
    afterCameraStop_ = AfterCameraStop::None;
    if (modelLoadWasPending) window_.setModelLoading(false);
    const QString diagnostic =
        QStringLiteral("Camera %1 timed out. A safe release was requested; native driver calls cannot be force-killed. Restart the application if release is not acknowledged.")
            .arg(phase);
    if (refreshActive_) completeRefresh(diagnostic, true);
    else window_.setStatus(QStringLiteral("Camera timeout"), diagnostic, true);
}

void AppController::markCameraWorkerResponsive() {
    cameraReleasePending_ = false;
    timedOutCameraSession_ = 0;
    window_.setCameraRecoveryBlocked(false);
}

void AppController::persistSettings() {
    settings_.modelId = window_.selectedModel();
    settings_.backend = window_.selectedBackend();
    settings_.cameraDeviceId = window_.selectedCameraId();
    settings_.windowGeometry = window_.saveGeometry();
    settings_.selectedClasses.clear();
    for (const int classId : selectedClasses_) settings_.selectedClasses.push_back(classId);
    settings_.classSelectionStored = true;
    settingsService_.save(settings_);
}

void AppController::shutdown() {
    if (shuttingDown_) return;
    shuttingDown_ = true;
    cameraOpenWatchdog_.stop();
    cameraStopWatchdog_.stop();
    modelState_ = ModelState::Unloading;
    persistSettings();
    if (cameraThread_.isRunning() && camera_) {
        const quint64 sessionId = cameraSession_.activeSession() != 0
                                      ? cameraSession_.activeSession()
                                      : timedOutCameraSession_;
        if (sessionId != 0) {
            QMetaObject::invokeMethod(camera_, "stop", Qt::BlockingQueuedConnection,
                                      Q_ARG(quint64, sessionId));
        }
        cameraThread_.quit();
        if (!cameraThread_.wait(3000)) {
            logging::log(logging::Level::Error,
                         "camera thread did not stop within the shutdown timeout; waiting without force termination");
            cameraThread_.wait();
        }
    }
    camera_ = nullptr;
    if (inference_) {
        inference_->stop();
        inference_.reset();
    }
}

}  // namespace odf::desktop
