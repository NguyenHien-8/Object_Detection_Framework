#pragma once

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
    void start(int cameraIndex);
    void stop();

signals:
    void frameCaptured(odf::desktop::ImageFramePtr frame);
    void stateChanged(QString state, QString detail);
    void failed(QString operation, QString detail);

private slots:
    void captureNext();

private:
    std::unique_ptr<cv::VideoCapture> capture_;
    QTimer* timer_{nullptr};
    std::uint64_t frameId_{0};
    int cameraIndex_{-1};
};

}  // namespace odf::desktop

