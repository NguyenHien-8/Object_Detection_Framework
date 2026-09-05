#pragma once

#include "odf/app/cli_options.hpp"
#include "odf/app/runtime_catalog.hpp"
#include "services/CameraDeviceService.hpp"
#include "services/SettingsService.hpp"
#include "workers/InferenceWorker.hpp"

#include <QObject>
#include <QSet>
#include <QThread>
#include <QTimer>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace odf::desktop {

class CameraWorker;
class MainWindow;

enum class ModelState { NoModel, Loading, Ready, Error, Unloading };

class AppController final : public QObject {
    Q_OBJECT

public:
    AppController(MainWindow& window, std::filesystem::path modelRoot,
                  app::CommandLineOptions commandLine, QObject* parent = nullptr);
    ~AppController() override;

    Status initialize();

signals:
    void startCameraWorker(odf::desktop::CameraOpenRequest request);
    void stopCameraWorker(quint64 sessionId);

private slots:
    void loadModel(const QString& modelId, const QString& backend);
    void refreshSources();
    void openImage();
    void startCamera(const QString& stableId);
    void stopCamera();
    void updateThresholds(double confidence, double iou, int maxDetections);
    void updateClassSelection(const QSet<int>& selectedClasses);
    void onModelLoaded(quint64 generation, const QString& modelId,
                       const QStringList& labels);
    void onDetection(odf::desktop::DetectionPacketPtr packet);
    void onFailure(const QString& operation, const QString& detail);
    void onCameraOpening(quint64 sessionId, const QString& deviceId);
    void onCameraStarted(quint64 sessionId, const QString& deviceId);
    void onCameraStopped(quint64 sessionId);
    void onCameraFrame(quint64 sessionId, odf::desktop::ImageFramePtr frame);
    void onCameraFailed(quint64 sessionId, const QString& operation,
                        const QString& detail);
    void onCameraOpenTimeout();
    void onCameraStopTimeout();
    void shutdown();

private:
    enum class AfterCameraStop { None, Refresh, LoadModel };

    struct PendingModelLoad {
        QString modelId;
        QString backend;
    };

    void performModelLoad(const QString& modelId, const QString& backend);
    void loadImagePath(const std::filesystem::path& path);
    void beginCameraStart(const QString& stableId);
    void beginCameraStop(AfterCameraStop afterStop);
    void continueAfterCameraStop();
    void continueRefreshAfterStop();
    bool enumerateCameraDevices(const QString& preferredStableId,
                                int legacyBackendIndex = -1);
    const CameraDeviceInfo* findCameraDevice(const QString& stableId) const;
    QString displayNameForCamera(const QString& stableId) const;
    void completeRefresh(const QString& detail, bool error);
    void invalidateSource();
    void applyInferenceOptions();
    void submitCurrentImage();
    void displayPacket(const DetectionPacket& packet);
    void persistSettings();
    void markCameraWorkerResponsive();
    void handleCameraWatchdogTimeout(const QString& phase);

    MainWindow& window_;
    std::filesystem::path modelRoot_;
    app::CommandLineOptions commandLine_;
    app::RuntimeCatalog catalog_;
    SettingsService settingsService_;
    DesktopSettings settings_;
    CameraDeviceService cameraDeviceService_;
    std::vector<CameraDeviceInfo> cameraDevices_;
    std::unique_ptr<InferenceWorker> inference_;
    QThread cameraThread_;
    CameraWorker* camera_{nullptr};
    QTimer cameraOpenWatchdog_;
    QTimer cameraStopWatchdog_;
    ImageFramePtr currentImage_;
    DetectionPacketPtr currentPacket_;
    QSet<int> selectedClasses_;
    CameraSessionState cameraSession_;
    ModelState modelState_{ModelState::NoModel};
    std::optional<PendingModelLoad> pendingModelLoad_;
    AfterCameraStop afterCameraStop_{AfterCameraStop::None};
    std::uint64_t cameraSessionSequence_{0};
    std::uint64_t sourceGeneration_{0};
    std::uint64_t activeCameraSourceGeneration_{0};
    std::uint64_t refreshGeneration_{0};
    quint64 timedOutCameraSession_{0};
    QString startupCameraId_;
    QString refreshPreferredCameraId_;
    bool refreshRestartCamera_{false};
    bool refreshActive_{false};
    bool cameraReleasePending_{false};
    bool initialSourceStarted_{false};
    bool shuttingDown_{false};
};

}  // namespace odf::desktop
