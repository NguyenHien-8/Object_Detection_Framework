#include "controllers/AppController.hpp"

#include "bridge/QtImageBridge.hpp"
#include "odf/app/opencv_bridge.hpp"
#include "widgets/MainWindow.hpp"
#include "workers/CameraWorker.hpp"

#include <QCoreApplication>
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

AppController::AppController(MainWindow& window, std::filesystem::path modelRoot,
                             app::CommandLineOptions commandLine, QObject* parent)
    : QObject(parent), window_(window), modelRoot_(std::move(modelRoot)),
      commandLine_(std::move(commandLine)) {
    qRegisterMetaType<DetectionPacketPtr>();
    qRegisterMetaType<ImageFramePtr>();
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
    window_.selectCamera(commandLine_.camera.value_or(settings_.cameraIndex));
    if (!settings_.windowGeometry.isEmpty()) window_.restoreGeometry(settings_.windowGeometry);

    inference_ = std::make_unique<InferenceWorker>(catalog_);
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
    connect(camera_, &CameraWorker::frameCaptured, this, [this](ImageFramePtr frame) {
        sourceState_ = SourceState::CameraRunning;
        if (inference_) inference_->submitFrame(std::move(frame));
    });
    connect(camera_, &CameraWorker::stateChanged, &window_,
            [this](const QString& state, const QString& detail) {
                window_.setStatus(state, detail, false);
            });
    connect(camera_, &CameraWorker::failed, this, &AppController::onFailure);
    cameraThread_.setObjectName(QStringLiteral("ODF Camera Capture"));
    cameraThread_.start();

    connect(&window_, &MainWindow::loadModelRequested, this, &AppController::loadModel);
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
    stopCamera();
    modelState_ = ModelState::Loading;
    currentPacket_.reset();
    selectedClasses_.clear();
    window_.setModelReady(false);
    window_.setModelLoading(true);
    backend::BackendConfig config;
    config.device = "CPU";
    config.precision = model::Precision::Fp32;
    inference_->requestLoad(modelId, backendName, config,
                            commandLine_.allowUnvalidatedModel);
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
    auto frame = std::make_shared<image::ImageFrame>();
    frame->image = converted.takeValue();
    frame->timestamp = std::chrono::steady_clock::now();
    frame->frameId = static_cast<std::uint64_t>(frame->timestamp.time_since_epoch().count());
    frame->source = path.string();
    currentImage_ = frame;
    sourceState_ = SourceState::ImageLoaded;
    currentPacket_.reset();
    settings_.lastImageDirectory = QFileInfo(qtPath).absolutePath();
    auto display = toQImage(frame->image);
    if (display.ok()) window_.showSourceFrame(display.takeValue());
    submitCurrentImage();
}

void AppController::startCamera(int cameraIndex) {
    if (modelState_ != ModelState::Ready ||
        sourceState_ == SourceState::CameraStarting ||
        sourceState_ == SourceState::CameraRunning) return;
    currentImage_.reset();
    currentPacket_.reset();
    sourceState_ = SourceState::CameraStarting;
    settings_.cameraIndex = cameraIndex;
    window_.setCameraRunning(true);
    emit startCameraWorker(cameraIndex);
}

void AppController::stopCamera() {
    if (sourceState_ != SourceState::CameraStarting &&
        sourceState_ != SourceState::CameraRunning) return;
    sourceState_ = SourceState::CameraStopped;
    emit stopCameraWorker();
    window_.setCameraRunning(false);
}

void AppController::updateThresholds(double confidence, double iou, int maxDetections) {
    settings_.confidence = confidence;
    settings_.iou = iou;
    settings_.maxDetections = maxDetections;
    applyInferenceOptions();
    if (sourceState_ != SourceState::CameraStarting &&
        sourceState_ != SourceState::CameraRunning) submitCurrentImage();
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
        window_.setStatus(
            QStringLiteral("Ready"),
            loadedConfiguration,
            false);
    }
    submitCurrentImage();
    if (!initialSourceStarted_ && commandLine_.camera) {
        initialSourceStarted_ = true;
        startCamera(*commandLine_.camera);
    }
}

void AppController::onDetection(DetectionPacketPtr packet) {
    if (!packet) return;
    currentPacket_ = std::move(packet);
    displayPacket(*currentPacket_);
}

void AppController::onFailure(const QString& operation, const QString& detail) {
    if (operation == QStringLiteral("Model load")) {
        modelState_ = ModelState::Error;
        window_.setModelLoading(false);
        window_.setModelReady(false);
    }
    if (operation == QStringLiteral("Camera")) {
        sourceState_ = SourceState::CameraError;
        window_.setCameraRunning(false);
    }
    if (operation == QStringLiteral("Inference") &&
        (sourceState_ == SourceState::CameraStarting ||
         sourceState_ == SourceState::CameraRunning)) {
        stopCamera();
        sourceState_ = SourceState::CameraError;
    }
    window_.setStatus(operation + QStringLiteral(" failed"), detail, true);
}

void AppController::submitCurrentImage() {
    if (modelState_ == ModelState::Ready && currentImage_ && inference_) {
        inference_->submitFrame(currentImage_);
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

void AppController::persistSettings() {
    settings_.modelId = window_.selectedModel();
    settings_.backend = window_.selectedBackend();
    settings_.windowGeometry = window_.saveGeometry();
    settings_.selectedClasses.clear();
    for (const int classId : selectedClasses_) settings_.selectedClasses.push_back(classId);
    settings_.classSelectionStored = true;
    settingsService_.save(settings_);
}

void AppController::shutdown() {
    if (shuttingDown_) return;
    shuttingDown_ = true;
    modelState_ = ModelState::Unloading;
    persistSettings();
    if (cameraThread_.isRunning() && camera_) {
        QMetaObject::invokeMethod(camera_, "stop", Qt::BlockingQueuedConnection);
        cameraThread_.quit();
        cameraThread_.wait(3000);
    }
    camera_ = nullptr;
    if (inference_) {
        inference_->stop();
        inference_.reset();
    }
}

}  // namespace odf::desktop
