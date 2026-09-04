#pragma once

#include "odf/app/cli_options.hpp"
#include "odf/app/runtime_catalog.hpp"
#include "services/SettingsService.hpp"
#include "workers/InferenceWorker.hpp"

#include <QObject>
#include <QSet>
#include <QThread>

#include <filesystem>
#include <memory>

namespace odf::desktop {

class CameraWorker;
class MainWindow;

enum class ModelState { NoModel, Loading, Ready, Error, Unloading };
enum class SourceState { None, ImageLoaded, CameraStarting, CameraRunning, CameraStopped, CameraError };

class AppController final : public QObject {
    Q_OBJECT

public:
    AppController(MainWindow& window, std::filesystem::path modelRoot,
                  app::CommandLineOptions commandLine, QObject* parent = nullptr);
    ~AppController() override;

    Status initialize();

signals:
    void startCameraWorker(int cameraIndex);
    void stopCameraWorker();

private slots:
    void loadModel(const QString& modelId, const QString& backend);
    void openImage();
    void startCamera(int cameraIndex);
    void stopCamera();
    void updateThresholds(double confidence, double iou, int maxDetections);
    void updateClassSelection(const QSet<int>& selectedClasses);
    void onModelLoaded(quint64 generation, const QString& modelId,
                       const QStringList& labels);
    void onDetection(odf::desktop::DetectionPacketPtr packet);
    void onFailure(const QString& operation, const QString& detail);
    void shutdown();

private:
    void loadImagePath(const std::filesystem::path& path);
    void applyInferenceOptions();
    void submitCurrentImage();
    void displayPacket(const DetectionPacket& packet);
    void persistSettings();

    MainWindow& window_;
    std::filesystem::path modelRoot_;
    app::CommandLineOptions commandLine_;
    app::RuntimeCatalog catalog_;
    SettingsService settingsService_;
    DesktopSettings settings_;
    std::unique_ptr<InferenceWorker> inference_;
    QThread cameraThread_;
    CameraWorker* camera_{nullptr};
    ImageFramePtr currentImage_;
    DetectionPacketPtr currentPacket_;
    QSet<int> selectedClasses_;
    ModelState modelState_{ModelState::NoModel};
    SourceState sourceState_{SourceState::None};
    bool initialSourceStarted_{false};
    bool shuttingDown_{false};
};

}  // namespace odf::desktop
