#include "widgets/MainWindow.hpp"

#include "widgets/ClassFilterPanel.hpp"
#include "widgets/DetectionViewport.hpp"

#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>

#include <algorithm>

namespace odf::desktop {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("ODF Desktop — Object Detection Framework"));
    resize(1320, 820);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    auto* toolbar = new QHBoxLayout();
    frameworkCombo_ = new QComboBox(central);
    frameworkCombo_->setEnabled(false);
    frameworkCombo_->setMinimumWidth(115);
    modelCombo_ = new QComboBox(central);
    modelCombo_->setMinimumWidth(210);
    backendCombo_ = new QComboBox(central);
    backendCombo_->setMinimumWidth(150);
    deviceCombo_ = new QComboBox(central);
    deviceCombo_->addItem(QStringLiteral("CPU"));
    deviceCombo_->setEnabled(false);
    precisionCombo_ = new QComboBox(central);
    precisionCombo_->addItem(QStringLiteral("FP32"));
    precisionCombo_->setEnabled(false);
    loadButton_ = new QPushButton(QStringLiteral("Load model"), central);
    loadButton_->setObjectName(QStringLiteral("loadModelButton"));
    refreshButton_ = new QPushButton(QStringLiteral("Refresh"), central);
    refreshButton_->setObjectName(QStringLiteral("refreshButton"));
    refreshButton_->setToolTip(
        QStringLiteral("Refresh source state and camera devices without reloading the model"));
    openButton_ = new QPushButton(QStringLiteral("Open image…"), central);
    openButton_->setObjectName(QStringLiteral("openImageButton"));
    cameraCombo_ = new QComboBox(central);
    cameraCombo_->setObjectName(QStringLiteral("cameraDeviceCombo"));
    cameraButton_ = new QPushButton(QStringLiteral("Start camera"), central);
    cameraButton_->setObjectName(QStringLiteral("cameraButton"));
    toolbar->addWidget(new QLabel(QStringLiteral("Framework"), central));
    toolbar->addWidget(frameworkCombo_);
    toolbar->addWidget(new QLabel(QStringLiteral("Model"), central));
    toolbar->addWidget(modelCombo_);
    toolbar->addWidget(new QLabel(QStringLiteral("Backend"), central));
    toolbar->addWidget(backendCombo_);
    toolbar->addWidget(deviceCombo_);
    toolbar->addWidget(precisionCombo_);
    toolbar->addWidget(loadButton_);
    toolbar->addWidget(refreshButton_);
    toolbar->addWidget(openButton_);
    toolbar->addWidget(cameraCombo_);
    toolbar->addWidget(cameraButton_);
    root->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal, central);
    viewport_ = new DetectionViewport(splitter);
    auto* sidebar = new QWidget(splitter);
    sidebar->setMinimumWidth(285);
    sidebar->setMaximumWidth(390);
    auto* sidebarLayout = new QVBoxLayout(sidebar);

    auto* settingsBox = new QGroupBox(QStringLiteral("Detection settings"), sidebar);
    auto* settingsLayout = new QFormLayout(settingsBox);
    confidenceSpin_ = new QDoubleSpinBox(settingsBox);
    confidenceSpin_->setRange(0.0, 1.0);
    confidenceSpin_->setSingleStep(0.05);
    confidenceSpin_->setDecimals(2);
    iouSpin_ = new QDoubleSpinBox(settingsBox);
    iouSpin_->setRange(0.0, 1.0);
    iouSpin_->setSingleStep(0.05);
    iouSpin_->setDecimals(2);
    maxDetectionsSpin_ = new QSpinBox(settingsBox);
    maxDetectionsSpin_->setRange(1, 100000);
    settingsLayout->addRow(QStringLiteral("Confidence"), confidenceSpin_);
    settingsLayout->addRow(QStringLiteral("IoU"), iouSpin_);
    settingsLayout->addRow(QStringLiteral("Max detections"), maxDetectionsSpin_);
    sidebarLayout->addWidget(settingsBox);

    auto* metricsBox = new QGroupBox(QStringLiteral("Live metrics"), sidebar);
    auto* metricsLayout = new QVBoxLayout(metricsBox);
    timingLabel_ = new QLabel(QStringLiteral("No inference yet"), metricsBox);
    timingLabel_->setWordWrap(true);
    countLabel_ = new QLabel(QStringLiteral("Detections: 0"), metricsBox);
    countLabel_->setObjectName(QStringLiteral("metricCount"));
    metricsLayout->addWidget(timingLabel_);
    metricsLayout->addWidget(countLabel_);
    sidebarLayout->addWidget(metricsBox);

    classFilter_ = new ClassFilterPanel(sidebar);
    sidebarLayout->addWidget(classFilter_, 1);
    splitter->addWidget(viewport_);
    splitter->addWidget(sidebar);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    root->addWidget(splitter, 1);
    setCentralWidget(central);

    stateLabel_ = new QLabel(QStringLiteral("Starting"), this);
    detailLabel_ = new QLabel(this);
    loadedLabel_ = new QLabel(QStringLiteral("Loaded: none"), this);
    statusBar()->addWidget(stateLabel_);
    statusBar()->addWidget(detailLabel_, 1);
    statusBar()->addPermanentWidget(loadedLabel_);

    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget { background: #20242a; color: #e7ebf0; }
        QComboBox, QDoubleSpinBox, QSpinBox, QLineEdit {
            background: #2c323a; border: 1px solid #48515e; border-radius: 4px;
            padding: 5px; color: #f4f6f8;
        }
        QPushButton { background: #3568d4; border: 0; border-radius: 4px; padding: 7px 12px; }
        QPushButton:hover { background: #4279e8; }
        QPushButton:disabled { background: #454b54; color: #969da8; }
        QGroupBox { border: 1px solid #3b424c; border-radius: 5px; margin-top: 9px; padding-top: 8px; }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
        QScrollArea { border: 1px solid #3b424c; }
        QStatusBar { background: #171a1f; }
        QLabel#metricCount { font-size: 17px; font-weight: 600; color: #7ec8ff; }
        QLabel#sectionTitle { font-size: 15px; font-weight: 600; }
    )"));

    connect(modelCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this] {
                rebuildBackends();
                emit configurationEdited(selectedModel(), selectedBackend());
            });
    connect(backendCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this] { emit configurationEdited(selectedModel(), selectedBackend()); });
    connect(loadButton_, &QPushButton::clicked, this, [this] {
        emit loadModelRequested(selectedModel(), selectedBackend());
    });
    connect(refreshButton_, &QPushButton::clicked, this, &MainWindow::refreshRequested);
    connect(openButton_, &QPushButton::clicked, this, &MainWindow::openImageRequested);
    connect(cameraButton_, &QPushButton::clicked, this, [this] {
        if (cameraState_ == CameraState::Running) {
            emit stopCameraRequested();
        } else if (cameraState_ == CameraState::Idle || cameraState_ == CameraState::Error) {
            const QString stableId = selectedCameraId();
            if (!stableId.isEmpty()) emit startCameraRequested(stableId);
        }
    });
    const auto thresholdChanged = [this] {
        emit thresholdsChanged(confidence(), iou(), maxDetections());
    };
    connect(confidenceSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            thresholdChanged);
    connect(iouSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            thresholdChanged);
    connect(maxDetectionsSpin_, qOverload<int>(&QSpinBox::valueChanged), this,
            thresholdChanged);
    connect(classFilter_, &ClassFilterPanel::selectionChanged, this,
            &MainWindow::classSelectionChanged);
    setCameraDevices({});
    setModelReady(false);
}

void MainWindow::setCatalog(const std::vector<ModelChoice>& models,
                            const QSet<QString>& availableBackends) {
    models_ = models;
    availableBackends_ = availableBackends;
    modelCombo_->clear();
    for (const auto& model : models_) {
        const QString suffix = model.deploymentValidated ? QString() : QStringLiteral("  [template]");
        modelCombo_->addItem(model.displayName + suffix, model.id);
    }
    rebuildBackends();
}

void MainWindow::selectModel(const QString& modelId) {
    const int index = modelCombo_->findData(modelId);
    if (index >= 0) modelCombo_->setCurrentIndex(index);
}

void MainWindow::selectBackend(const QString& backend) {
    const int index = backendCombo_->findData(backend);
    if (index >= 0) backendCombo_->setCurrentIndex(index);
}

QString MainWindow::selectedModel() const {
    return modelCombo_->currentData().toString();
}

QString MainWindow::selectedBackend() const {
    return backendCombo_->currentData().toString();
}

void MainWindow::setCameraDevices(const std::vector<CameraDeviceInfo>& devices) {
    const QString previousId = selectedCameraId();
    const QSignalBlocker blocker(cameraCombo_);
    cameraDevices_ = devices;
    cameraCombo_->clear();
    for (const auto& device : cameraDevices_) {
        if (device.valid()) cameraCombo_->addItem(device.displayName, device.stableId);
    }
    if (cameraCombo_->count() == 0) {
        cameraCombo_->addItem(QStringLiteral("No camera detected"), QString());
    } else {
        selectCameraById(previousId);
    }
    updateControls();
}

QString MainWindow::selectedCameraId() const {
    return cameraCombo_->currentData().toString();
}

void MainWindow::selectCameraById(const QString& stableId) {
    int index = cameraCombo_->findData(stableId);
    if (index < 0 && !stableId.isEmpty()) return;
    if (index < 0 && !cameraDevices_.empty()) index = 0;
    if (index >= 0) cameraCombo_->setCurrentIndex(index);
}

void MainWindow::setThresholds(double confidenceValue, double iouValue,
                               int maxDetectionsValue) {
    const QSignalBlocker confidenceBlock(confidenceSpin_);
    const QSignalBlocker iouBlock(iouSpin_);
    const QSignalBlocker maximumBlock(maxDetectionsSpin_);
    confidenceSpin_->setValue(confidenceValue);
    iouSpin_->setValue(iouValue);
    maxDetectionsSpin_->setValue(maxDetectionsValue);
}

double MainWindow::confidence() const { return confidenceSpin_->value(); }
double MainWindow::iou() const { return iouSpin_->value(); }
int MainWindow::maxDetections() const { return maxDetectionsSpin_->value(); }

void MainWindow::setClasses(const QStringList& labels, bool selectAll) {
    classFilter_->setClasses(labels, selectAll);
}

void MainWindow::setSelectedClasses(const QSet<int>& selectedClasses) {
    classFilter_->setSelectedClasses(selectedClasses);
}

void MainWindow::setFrame(QImage image, std::vector<detection::Detection> detections,
                          std::vector<std::string> labels) {
    viewport_->setFrame(std::move(image), std::move(detections), std::move(labels));
}

void MainWindow::showSourceFrame(QImage image) {
    viewport_->setFrame(std::move(image), {}, {});
}

void MainWindow::setMetrics(const MetricsView& metrics) {
    timingLabel_->setText(
        QStringLiteral("Preprocess  %1 ms\nInference   %2 ms\nPostprocess %3 ms\nTotal       %4 ms\nThroughput  %5 FPS\nDropped     %6")
            .arg(metrics.preprocessMs, 0, 'f', 2)
            .arg(metrics.inferenceMs, 0, 'f', 2)
            .arg(metrics.postprocessMs, 0, 'f', 2)
            .arg(metrics.totalMs, 0, 'f', 2)
            .arg(metrics.fps, 0, 'f', 1)
            .arg(metrics.droppedFrames));
    countLabel_->setText(QStringLiteral("Detections: %1").arg(metrics.detections));
}

void MainWindow::setStatus(const QString& state, const QString& detail, bool error) {
    stateLabel_->setText(state);
    stateLabel_->setStyleSheet(error ? QStringLiteral("color:#ff7d87;font-weight:600")
                                    : QStringLiteral("color:#84d4a4;font-weight:600"));
    detailLabel_->setText(detail);
}

void MainWindow::setLoadedConfiguration(const QString& configuration) {
    loadedLabel_->setText(QStringLiteral("Loaded: ") + configuration);
}

void MainWindow::setModelLoading(bool loading) {
    modelLoading_ = loading;
    updateControls();
}

void MainWindow::setModelReady(bool ready) {
    modelReady_ = ready;
    updateControls();
}

void MainWindow::setCameraState(CameraState state) {
    cameraState_ = state;
    updateControls();
}

void MainWindow::setRefreshActive(bool active) {
    refreshActive_ = active;
    updateControls();
}

void MainWindow::setCameraRecoveryBlocked(bool blocked) {
    cameraRecoveryBlocked_ = blocked;
    updateControls();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    emit closing();
    QMainWindow::closeEvent(event);
}

void MainWindow::rebuildBackends() {
    const QString previous = selectedBackend();
    backendCombo_->clear();
    const auto found = std::find_if(models_.begin(), models_.end(), [this](const ModelChoice& model) {
        return model.id == selectedModel();
    });
    if (found == models_.end()) return;
    frameworkCombo_->clear();
    frameworkCombo_->addItem(found->framework);
    for (const auto& backend : found->supportedBackends) {
        if (availableBackends_.contains(backend)) backendCombo_->addItem(backend, backend);
    }
    int index = backendCombo_->findData(previous);
    if (index < 0) index = backendCombo_->findData(found->defaultBackend);
    if (index >= 0) backendCombo_->setCurrentIndex(index);
    updateControls();
}

void MainWindow::updateControls() {
    const bool hasCamera = !selectedCameraId().isEmpty();
    const bool cameraTransition = cameraState_ == CameraState::Opening ||
                                  cameraState_ == CameraState::Stopping;
    const bool cameraRunning = cameraState_ == CameraState::Running;
    const bool cameraCanStart = (cameraState_ == CameraState::Idle ||
                                 cameraState_ == CameraState::Error) &&
                                !cameraRecoveryBlocked_;

    modelCombo_->setEnabled(!modelLoading_ && !refreshActive_);
    backendCombo_->setEnabled(!modelLoading_ && !refreshActive_);
    loadButton_->setEnabled(!modelLoading_ && !refreshActive_ &&
                            backendCombo_->count() > 0 && !cameraRecoveryBlocked_);
    refreshButton_->setEnabled(!modelLoading_ && !refreshActive_);
    openButton_->setEnabled(modelReady_ && !refreshActive_ &&
                            !cameraRunning && !cameraTransition);
    cameraCombo_->setEnabled(hasCamera && !refreshActive_ && !cameraRunning &&
                             !cameraTransition && !cameraRecoveryBlocked_);

    if (cameraState_ == CameraState::Opening) {
        cameraButton_->setText(QStringLiteral("Opening..."));
    } else if (cameraState_ == CameraState::Stopping) {
        cameraButton_->setText(QStringLiteral("Stopping..."));
    } else if (cameraState_ == CameraState::Running) {
        cameraButton_->setText(QStringLiteral("Stop camera"));
    } else {
        cameraButton_->setText(QStringLiteral("Start camera"));
    }
    cameraButton_->setEnabled(modelReady_ && !refreshActive_ &&
                              ((cameraRunning && !cameraRecoveryBlocked_) ||
                               (cameraCanStart && hasCamera)));
}

}  // namespace odf::desktop
