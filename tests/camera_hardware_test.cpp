#include "services/CameraDeviceService.hpp"
#include "workers/CameraWorker.hpp"

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <QTimer>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int kRequiredCycles = 20;
constexpr int kCycleTimeoutMs = 8000;

struct RunState {
    std::vector<odf::desktop::CameraDeviceInfo> devices;
    QString selectedStableId;
    int completedCycles{0};
    quint64 activeSession{0};
    bool receivedFrame{false};
    bool failed{false};
    QString failure;
};
}  // namespace

int main(int argc, char** argv) {
    QCoreApplication application(argc, argv);
    odf::desktop::CameraDeviceService service;
    auto discovered = service.enumerate();
    if (!discovered.ok()) {
        std::cerr << "camera discovery failed: " << discovered.status().message() << '\n';
        return 1;
    }
    if (discovered.value().empty()) {
        std::cerr << "camera hardware test requires at least one Media Foundation camera\n";
        return 2;
    }

    RunState state;
    state.devices = discovered.takeValue();
    int selectedPosition = 0;
    if (argc > 1) {
        try {
            selectedPosition = std::stoi(argv[1]);
        } catch (const std::exception&) {
            std::cerr << "usage: odf_camera_hardware_test.exe [enumerated-device-position]\n";
            return 2;
        }
    }
    if (selectedPosition < 0 ||
        selectedPosition >= static_cast<int>(state.devices.size())) {
        std::cerr << "selected camera position is outside the discovered device list\n";
        return 2;
    }
    state.selectedStableId = state.devices[static_cast<std::size_t>(selectedPosition)].stableId;
    std::cout << "real Media Foundation cameras=" << state.devices.size() << '\n';
    for (const auto& device : state.devices) {
        std::cout << "  " << device.displayName.toStdString()
                  << " [index=" << device.backendIndex << "]\n";
    }

    QThread cameraThread;
    auto* worker = new odf::desktop::CameraWorker();
    worker->moveToThread(&cameraThread);
    QObject::connect(&cameraThread, &QThread::finished, worker, &QObject::deleteLater);

    QTimer cycleTimeout;
    cycleTimeout.setSingleShot(true);
    cycleTimeout.setInterval(kCycleTimeoutMs);
    QObject::connect(&cycleTimeout, &QTimer::timeout, &application, [&] {
        state.failed = true;
        state.failure = QStringLiteral("camera lifecycle cycle timed out");
        application.quit();
    });

    std::function<void()> startNext;
    startNext = [&] {
        if (state.completedCycles >= kRequiredCycles) {
            application.quit();
            return;
        }
        ++state.activeSession;
        state.receivedFrame = false;
        const auto selected = std::find_if(
            state.devices.begin(), state.devices.end(), [&](const auto& device) {
                return device.stableId == state.selectedStableId;
            });
        if (selected == state.devices.end()) {
            state.failed = true;
            state.failure = QStringLiteral("selected stable camera ID is unavailable");
            application.quit();
            return;
        }
        const auto& device = *selected;
        odf::desktop::CameraOpenRequest request{state.activeSession, device.stableId,
                                                device.displayName, device.backendIndex,
                                                device.backendApi};
        cycleTimeout.start();
        QMetaObject::invokeMethod(
            worker, [worker, request] { worker->start(request); }, Qt::QueuedConnection);
    };

    QObject::connect(worker, &odf::desktop::CameraWorker::started, &application,
                     [&](quint64 sessionId, const QString&) {
        if (sessionId != state.activeSession) return;
        QMetaObject::invokeMethod(
            worker, [worker, sessionId] { worker->stop(sessionId); },
            Qt::QueuedConnection);
    });
    QObject::connect(worker, &odf::desktop::CameraWorker::frameCaptured, &application,
                     [&](quint64 sessionId, odf::desktop::ImageFramePtr frame) {
        if (sessionId == state.activeSession && frame && frame->frameId > 0) {
            state.receivedFrame = true;
        }
    });
    QObject::connect(worker, &odf::desktop::CameraWorker::stopped, &application,
                     [&](quint64 sessionId) {
        if (sessionId != state.activeSession) return;
        cycleTimeout.stop();
        if (!state.receivedFrame) {
            state.failed = true;
            state.failure = QStringLiteral("camera stopped without delivering its first frame");
            application.quit();
            return;
        }
        ++state.completedCycles;
        std::cout << "cycle " << state.completedCycles << '/' << kRequiredCycles
                  << " start+frame+release passed\n";

        if (state.completedCycles == kRequiredCycles / 2) {
            auto refreshed = service.enumerate();
            if (!refreshed.ok()) {
                state.failed = true;
                state.failure = QStringLiteral("device re-enumeration failed during restart test");
                application.quit();
                return;
            }
            const auto stillPresent = std::find_if(
                refreshed.value().begin(), refreshed.value().end(),
                [&state](const odf::desktop::CameraDeviceInfo& device) {
                    return device.stableId == state.selectedStableId;
                });
            if (stillPresent == refreshed.value().end()) {
                state.failed = true;
                state.failure = QStringLiteral("stable camera ID disappeared during Refresh");
                application.quit();
                return;
            }
            state.devices = refreshed.takeValue();
        }
        QTimer::singleShot(25, &application, startNext);
    });
    QObject::connect(worker, &odf::desktop::CameraWorker::failed, &application,
                     [&](quint64 sessionId, const QString&, const QString& detail) {
        if (sessionId != state.activeSession) return;
        cycleTimeout.stop();
        state.failed = true;
        state.failure = detail;
        application.quit();
    });

    cameraThread.start();
    QTimer::singleShot(0, &application, startNext);
    application.exec();

    cycleTimeout.stop();
    if (state.activeSession != 0) {
        QMetaObject::invokeMethod(worker, [worker, session = state.activeSession] {
            worker->stop(session);
        }, Qt::BlockingQueuedConnection);
    }
    cameraThread.quit();
    cameraThread.wait();

    if (state.failed) {
        std::cerr << "camera hardware test failed after " << state.completedCycles
                  << " cycles: " << state.failure.toStdString() << '\n';
        return 1;
    }
    std::cout << "20/20 repeated camera start/stop cycles passed with stable identity refresh\n";
    return 0;
}
