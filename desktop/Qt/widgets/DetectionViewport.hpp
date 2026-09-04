#pragma once

#include "odf/detection/detection.hpp"

#include <QImage>
#include <QWidget>

#include <string>
#include <vector>

namespace odf::desktop {

class DetectionViewport final : public QWidget {
    Q_OBJECT

public:
    explicit DetectionViewport(QWidget* parent = nullptr);

    void setFrame(QImage image, std::vector<detection::Detection> detections,
                  std::vector<std::string> labels);
    void clearFrame();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage image_;
    std::vector<detection::Detection> detections_;
    std::vector<std::string> labels_;
};

}  // namespace odf::desktop

