#include "workers/CameraWorker.hpp"

#include "odf/app/opencv_bridge.hpp"
#include "odf/logging.hpp"

#include <QTimer>

#include <opencv2/videoio.hpp>

#include <chrono>
#include <utility>

namespace odf::desktop {

CameraWorker::CameraWorker(QObject* parent) : QObject(parent) {}

CameraWorker::~CameraWorker() {
    releaseActive();
}

void CameraWorker::start(CameraOpenRequest request) {
    if (!request.valid()) {
        emit failed(request.sessionId, QStringLiteral("Camera"),
                    QStringLiteral("Invalid camera open request"));
        return;
    }
    if (activeSession_ != 0 || capture_) {
        emit failed(request.sessionId, QStringLiteral("Camera"),
                    QStringLiteral("A previous camera session is still active"));
        return;
    }

    activeSession_ = request.sessionId;
    activeDeviceId_ = request.stableId;
    activeDisplayName_ = request.displayName;
    frameId_ = 0;
    firstFrameSeen_ = false;
    logging::log(logging::Level::Debug,
                 "camera start request: session=" + std::to_string(activeSession_) +
                     " name='" + activeDisplayName_.toStdString() + "' index=" +
                     std::to_string(request.backendIndex) + " api=" +
                     std::to_string(request.backendApi));
    emit opening(activeSession_, activeDeviceId_);

    capture_ = std::make_unique<cv::VideoCapture>();
    const bool opened = request.backendApi == cv::CAP_ANY
                            ? capture_->open(request.backendIndex)
                            : capture_->open(request.backendIndex, request.backendApi);
    if (!opened) {
        failActive(QStringLiteral("Could not open %1").arg(request.displayName));
        return;
    }
    logging::log(logging::Level::Debug,
                 "camera open succeeded: session=" + std::to_string(activeSession_));
    if (!timer_) {
        timer_ = new QTimer(this);
        timer_->setTimerType(Qt::PreciseTimer);
        timer_->setSingleShot(true);
        connect(timer_, &QTimer::timeout, this, &CameraWorker::captureNext);
    }
    timer_->start(15);
}

void CameraWorker::stop(quint64 sessionId) {
    logging::log(logging::Level::Debug,
                 "camera stop request: session=" + std::to_string(sessionId));
    if (activeSession_ == 0) {
        emit stopped(sessionId);
        return;
    }
    if (sessionId != activeSession_) {
        logging::log(logging::Level::Debug,
                     "stale camera stop ignored: session=" + std::to_string(sessionId) +
                         " active=" + std::to_string(activeSession_));
        emit stopped(sessionId);
        return;
    }
    const quint64 releasedSession = activeSession_;
    releaseActive();
    logging::log(logging::Level::Debug,
                 "camera release complete: session=" + std::to_string(releasedSession));
    emit stopped(releasedSession);
}

void CameraWorker::captureNext() {
    const quint64 sessionId = activeSession_;
    if (sessionId == 0 || !capture_ || !capture_->isOpened()) return;
    cv::Mat frame;
    if (!capture_->read(frame) || frame.empty()) {
        failActive(QStringLiteral("Frame capture failed for %1").arg(activeDisplayName_));
        return;
    }
    auto converted = app::toOdfImage(frame);
    if (!converted.ok()) {
        failActive(QString::fromStdString(converted.status().message()));
        return;
    }
    if (sessionId != activeSession_) return;
    auto packet = std::make_shared<image::ImageFrame>();
    packet->image = converted.takeValue();
    packet->timestamp = std::chrono::steady_clock::now();
    packet->frameId = ++frameId_;
    packet->source = "camera:" + activeDeviceId_.toStdString();
    if (!firstFrameSeen_) {
        firstFrameSeen_ = true;
        logging::log(logging::Level::Debug,
                     "camera first frame: session=" + std::to_string(sessionId));
        emit started(sessionId, activeDeviceId_);
    }
    emit frameCaptured(sessionId, std::move(packet));
    if (sessionId == activeSession_ && timer_) timer_->start(15);
}

void CameraWorker::releaseActive() noexcept {
    activeSession_ = 0;
    firstFrameSeen_ = false;
    if (timer_) timer_->stop();
    if (capture_) {
        capture_->release();
        capture_.reset();
    }
    activeDeviceId_.clear();
    activeDisplayName_.clear();
    frameId_ = 0;
}

void CameraWorker::failActive(const QString& detail) {
    const quint64 failedSession = activeSession_;
    releaseActive();
    logging::log(logging::Level::Warning,
                 "camera session failed after release: session=" +
                     std::to_string(failedSession) + " detail='" + detail.toStdString() + "'");
    emit failed(failedSession, QStringLiteral("Camera"), detail);
}

}  // namespace odf::desktop
