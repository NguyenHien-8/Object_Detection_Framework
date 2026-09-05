#include "bridge/QtImageBridge.hpp"
#include "services/CameraDeviceService.hpp"
#include "widgets/ClassFilterPanel.hpp"
#include "widgets/MainWindow.hpp"
#include "workers/InferenceWorker.hpp"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QPushButton>
#include <QSet>

#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testClassFilterAndImageBridge() {
    odf::desktop::ClassFilterPanel filter;
    filter.setClasses({QStringLiteral("person"), QStringLiteral("bus"),
                       QStringLiteral("car")}, true);
    require(filter.selectedClasses().size() == 3, "select-all class state is incorrect");
    filter.setSelectedClasses({});
    require(filter.selectedClasses().isEmpty(), "clear-all class state is incorrect");
    filter.setSelectedClasses(QSet<int>{1});
    require(filter.selectedClasses() == QSet<int>{1},
            "individual class state is incorrect");

    odf::image::Image bgr;
    bgr.width = 1;
    bgr.height = 1;
    bgr.format = odf::image::PixelFormat::Bgr8;
    bgr.pixels = {10, 20, 30};
    auto image = odf::desktop::toQImage(bgr);
    require(image.ok() && image.value().pixelColor(0, 0) == QColor(30, 20, 10),
            "ODF-to-QImage BGR conversion is incorrect");
}

std::vector<odf::desktop::CameraDeviceInfo> fakeDevices() {
    return {{QStringLiteral("device-a"), QStringLiteral("Integrated Camera"), 0, 700},
            {QStringLiteral("device-b"), QStringLiteral("HD USB Camera"), 1, 700}};
}

void testCameraSelectionPolicy() {
    const auto devices = fakeDevices();
    require(odf::desktop::CameraDeviceService::preferredDeviceIndex(
                devices, QStringLiteral("device-b")) == 1,
            "stable camera ID was not preferred");
    require(odf::desktop::CameraDeviceService::preferredDeviceIndex(
                devices, QStringLiteral("missing"), 0) == 0,
            "legacy camera index was not migrated");
    require(odf::desktop::CameraDeviceService::preferredDeviceIndex(
                devices, QStringLiteral("missing"), 99) == 0,
            "missing saved camera did not fall back to the first real device");
    require(odf::desktop::CameraDeviceService::preferredDeviceIndex(
                {}, QStringLiteral("missing"), 0) == -1,
            "empty camera list produced a fake selection");
}

void testCameraSessionState() {
    odf::desktop::CameraSessionState session;
    require(session.state() == odf::desktop::CameraState::Idle,
            "camera did not start idle");
    require(session.beginOpening(10), "Idle -> Opening was rejected");
    require(!session.beginOpening(11), "overlapping camera start was accepted");
    require(!session.acknowledgeStarted(9), "stale started acknowledgement was accepted");
    require(session.acknowledgeStarted(10), "Opening -> Running was rejected");
    require(session.beginStopping(), "Running -> Stopping was rejected");
    require(!session.acknowledgeStopped(9), "stale stopped acknowledgement was accepted");
    require(session.acknowledgeStopped(10), "Stopping -> Idle was rejected");
    require(session.beginOpening(11), "camera could not restart after stopped acknowledgement");
    require(session.fail(11) && session.state() == odf::desktop::CameraState::Error,
            "camera failure did not enter Error");
    require(session.beginOpening(12), "Error -> Opening recovery was rejected");
}

void testCameraControls() {
    odf::desktop::MainWindow window;
    auto* combo = window.findChild<QComboBox*>(QStringLiteral("cameraDeviceCombo"));
    auto* cameraButton = window.findChild<QPushButton*>(QStringLiteral("cameraButton"));
    auto* refreshButton = window.findChild<QPushButton*>(QStringLiteral("refreshButton"));
    require(combo && cameraButton && refreshButton, "camera toolbar controls were not found");

    window.setModelReady(true);
    window.setCameraDevices({});
    require(combo->count() == 1 && combo->currentData().toString().isEmpty(),
            "zero cameras did not produce a non-device placeholder");
    require(!cameraButton->isEnabled(), "Start was enabled without a real camera");
    require(refreshButton->isEnabled(), "Refresh was disabled with zero cameras");

    auto devices = fakeDevices();
    window.setCameraDevices(devices);
    require(combo->count() == 2, "camera list replacement produced the wrong count");
    require(combo->itemText(0) == QStringLiteral("Integrated Camera") &&
                combo->itemText(1) == QStringLiteral("HD USB Camera"),
            "camera selector did not show real supplied names");
    window.selectCameraById(QStringLiteral("device-b"));
    devices.erase(devices.begin());
    devices.push_back({QStringLiteral("device-c"), QStringLiteral("Capture Device"), 2, 700});
    window.setCameraDevices(devices);
    require(window.selectedCameraId() == QStringLiteral("device-b"),
            "stable selection was not preserved after refresh");

    window.setCameraState(odf::desktop::CameraState::Opening);
    require(cameraButton->text() == QStringLiteral("Opening...") &&
                !cameraButton->isEnabled(),
            "Opening controls are not authoritative");
    window.setCameraState(odf::desktop::CameraState::Stopping);
    require(cameraButton->text() == QStringLiteral("Stopping...") &&
                !cameraButton->isEnabled(),
            "Stopping controls are not authoritative");
    window.setCameraState(odf::desktop::CameraState::Running);
    require(cameraButton->text() == QStringLiteral("Stop camera") &&
                cameraButton->isEnabled(),
            "Running controls are incorrect");

    int refreshRequests = 0;
    QObject::connect(&window, &odf::desktop::MainWindow::refreshRequested,
                     [&refreshRequests] { ++refreshRequests; });
    window.setCameraState(odf::desktop::CameraState::Idle);
    refreshButton->click();
    window.setRefreshActive(true);
    refreshButton->click();
    require(refreshRequests == 1, "overlapping Refresh request was accepted");
    require(!cameraButton->isEnabled(), "camera control remained enabled during Refresh");
}

void testInferenceSourceGeneration() {
    odf::app::RuntimeCatalog catalog;
    odf::desktop::InferenceWorker worker(catalog);
    worker.invalidateSource(7);
    auto frame = std::make_shared<odf::image::ImageFrame>();
    require(!worker.submitFrame(frame, 6), "stale source generation was accepted");
    worker.stop();
}

void reportDiscoveredCameras() {
    odf::desktop::CameraDeviceService service;
    auto result = service.enumerate();
    require(result.ok(), "native camera enumeration failed");
    std::cout << "Media Foundation cameras discovered=" << result.value().size() << '\n';
    for (const auto& device : result.value()) {
        std::cout << "  name=" << device.displayName.toStdString()
                  << " index=" << device.backendIndex << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    QApplication application(argc, argv);
    try {
        testClassFilterAndImageBridge();
        testCameraSelectionPolicy();
        testCameraSessionState();
        testCameraControls();
        testInferenceSourceGeneration();
        reportDiscoveredCameras();
        std::cout << "desktop camera lifecycle, controls, and image tests passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "desktop tests failed: " << exception.what() << '\n';
        return 1;
    }
}
