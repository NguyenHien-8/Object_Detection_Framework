#pragma once

#include "odf/detection/detection.hpp"
#include "services/CameraDeviceService.hpp"

#include <QMainWindow>
#include <QSet>

#include <map>
#include <cstdint>
#include <string>
#include <vector>

class QCloseEvent;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;

namespace odf::desktop {

class ClassFilterPanel;
class DetectionViewport;

struct ModelChoice {
    QString id;
    QString displayName;
    QString framework;
    QString inputSize;
    QString defaultBackend;
    QStringList supportedBackends;
    bool deploymentValidated{false};
};

struct MetricsView {
    double preprocessMs{0.0};
    double inferenceMs{0.0};
    double postprocessMs{0.0};
    double totalMs{0.0};
    double fps{0.0};
    std::size_t detections{0};
    std::uint64_t droppedFrames{0};
};

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    void setCatalog(const std::vector<ModelChoice>& models,
                    const QSet<QString>& availableBackends);
    void selectModel(const QString& modelId);
    void selectBackend(const QString& backend);
    [[nodiscard]] QString selectedModel() const;
    [[nodiscard]] QString selectedBackend() const;
    void setCameraDevices(const std::vector<CameraDeviceInfo>& devices);
    [[nodiscard]] QString selectedCameraId() const;
    void selectCameraById(const QString& stableId);
    void setThresholds(double confidence, double iou, int maxDetections);
    [[nodiscard]] double confidence() const;
    [[nodiscard]] double iou() const;
    [[nodiscard]] int maxDetections() const;
    void setClasses(const QStringList& labels, bool selectAll = true);
    void setSelectedClasses(const QSet<int>& selectedClasses);
    void setFrame(QImage image, std::vector<detection::Detection> detections,
                  std::vector<std::string> labels);
    void showSourceFrame(QImage image);
    void setMetrics(const MetricsView& metrics);
    void setStatus(const QString& state, const QString& detail, bool error = false);
    void setLoadedConfiguration(const QString& configuration);
    void setModelLoading(bool loading);
    void setModelReady(bool ready);
    void setCameraState(CameraState state);
    void setRefreshActive(bool active);
    void setCameraRecoveryBlocked(bool blocked);

signals:
    void configurationEdited(QString modelId, QString backend);
    void loadModelRequested(QString modelId, QString backend);
    void refreshRequested();
    void openImageRequested();
    void startCameraRequested(QString stableId);
    void stopCameraRequested();
    void thresholdsChanged(double confidence, double iou, int maxDetections);
    void classSelectionChanged(QSet<int> selectedClasses);
    void closing();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void rebuildBackends();
    void updateControls();

    QComboBox* modelCombo_{nullptr};
    QComboBox* frameworkCombo_{nullptr};
    QComboBox* backendCombo_{nullptr};
    QComboBox* deviceCombo_{nullptr};
    QComboBox* precisionCombo_{nullptr};
    QComboBox* cameraCombo_{nullptr};
    QPushButton* loadButton_{nullptr};
    QPushButton* refreshButton_{nullptr};
    QPushButton* openButton_{nullptr};
    QPushButton* cameraButton_{nullptr};
    QDoubleSpinBox* confidenceSpin_{nullptr};
    QDoubleSpinBox* iouSpin_{nullptr};
    QSpinBox* maxDetectionsSpin_{nullptr};
    QLabel* stateLabel_{nullptr};
    QLabel* detailLabel_{nullptr};
    QLabel* timingLabel_{nullptr};
    QLabel* countLabel_{nullptr};
    QLabel* loadedLabel_{nullptr};
    DetectionViewport* viewport_{nullptr};
    ClassFilterPanel* classFilter_{nullptr};
    std::vector<ModelChoice> models_;
    std::vector<CameraDeviceInfo> cameraDevices_;
    QSet<QString> availableBackends_;
    CameraState cameraState_{CameraState::Idle};
    bool modelReady_{false};
    bool modelLoading_{false};
    bool refreshActive_{false};
    bool cameraRecoveryBlocked_{false};
};

}  // namespace odf::desktop
