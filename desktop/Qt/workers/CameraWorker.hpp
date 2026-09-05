#pragma once

#include "services/CameraDeviceService.hpp"
#include "workers/InferenceWorker.hpp"

#include <QObject>

#include <cstdint>
#include <memory>

namespace cv {
class VideoCapture;
}
class QTimer;

namespace odf::desktop {

class CameraWorker final : public QObject {
    Q_OBJECT

public:
    explicit CameraWorker(QObject* parent = nullptr);
    ~CameraWorker() override;

public slots:
    void start(odf::desktop::CameraOpenRequest request);
    void stop(quint64 sessionId);

signals:
    void opening(quint64 sessionId, QString deviceId);
    void started(quint64 sessionId, QString deviceId);
    void stopped(quint64 sessionId);
    void frameCaptured(quint64 sessionId, odf::desktop::ImageFramePtr frame);
    void failed(quint64 sessionId, QString operation, QString detail);

private slots:
    void captureNext();

private:
    void releaseActive() noexcept;
    void failActive(const QString& detail);

    std::unique_ptr<cv::VideoCapture> capture_;
    QTimer* timer_{nullptr};
    std::uint64_t frameId_{0};
    quint64 activeSession_{0};
    QString activeDeviceId_;
    QString activeDisplayName_;
    bool firstFrameSeen_{false};
};

}  // namespace odf::desktop
