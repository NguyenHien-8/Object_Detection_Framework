#include "workers/CameraWorker.hpp"

#include "odf/app/opencv_bridge.hpp"

#include <QTimer>

#include <opencv2/videoio.hpp>

#include <chrono>
#include <utility>

namespace odf::desktop {

CameraWorker::CameraWorker(QObject* parent) : QObject(parent) {}

CameraWorker::~CameraWorker() {
    stop();
}

void CameraWorker::start(int cameraIndex) {
    stop();
    emit stateChanged(QStringLiteral("Opening camera"), QString::number(cameraIndex));
    capture_ = std::make_unique<cv::VideoCapture>();
#ifdef _WIN32
    if (!capture_->open(cameraIndex, cv::CAP_DSHOW))
#endif
    {
        if (!capture_->open(cameraIndex, cv::CAP_ANY)) {
            capture_.reset();
            emit failed(QStringLiteral("Camera"),
                        QStringLiteral("Could not open camera %1").arg(cameraIndex));
            return;
        }
    }
    cameraIndex_ = cameraIndex;
    frameId_ = 0;
    if (!timer_) {
        timer_ = new QTimer(this);
        timer_->setTimerType(Qt::PreciseTimer);
        connect(timer_, &QTimer::timeout, this, &CameraWorker::captureNext);
    }
    timer_->start(1);
    emit stateChanged(QStringLiteral("Camera running"), QString::number(cameraIndex));
}

void CameraWorker::stop() {
    if (timer_) timer_->stop();
    if (capture_) {
        capture_->release();
        capture_.reset();
    }
    if (cameraIndex_ >= 0) emit stateChanged(QStringLiteral("Camera stopped"), {});
    cameraIndex_ = -1;
}

void CameraWorker::captureNext() {
    if (!capture_) return;
    cv::Mat frame;
    if (!capture_->read(frame) || frame.empty()) {
        stop();
        emit failed(QStringLiteral("Camera"), QStringLiteral("Frame capture failed"));
        return;
    }
    auto converted = app::toOdfImage(frame);
    if (!converted.ok()) {
        const auto detail = QString::fromStdString(converted.status().message());
        stop();
        emit failed(QStringLiteral("Camera"), detail);
        return;
    }
    auto packet = std::make_shared<image::ImageFrame>();
    packet->image = converted.takeValue();
    packet->timestamp = std::chrono::steady_clock::now();
    packet->frameId = ++frameId_;
    packet->source = "camera:" + std::to_string(cameraIndex_);
    emit frameCaptured(std::move(packet));
}

}  // namespace odf::desktop
