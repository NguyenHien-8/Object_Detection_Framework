#include "widgets/DetectionViewport.hpp"

#include <QPainter>

#include <algorithm>

namespace odf::desktop {
namespace {

QColor colorForClass(int classId) {
    const auto value = static_cast<unsigned int>(classId) * 2654435761U;
    return QColor(64 + static_cast<int>((value >> 16U) & 127U),
                  64 + static_cast<int>((value >> 8U) & 127U),
                  64 + static_cast<int>(value & 127U));
}

}  // namespace

DetectionViewport::DetectionViewport(QWidget* parent) : QWidget(parent) {
    setMinimumSize(640, 420);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(false);
}

void DetectionViewport::setFrame(QImage image,
                                 std::vector<detection::Detection> detections,
                                 std::vector<std::string> labels) {
    image_ = std::move(image);
    detections_ = std::move(detections);
    labels_ = std::move(labels);
    update();
}

void DetectionViewport::clearFrame() {
    image_ = {};
    detections_.clear();
    labels_.clear();
    update();
}

void DetectionViewport::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(24, 27, 32));
    if (image_.isNull()) {
        painter.setPen(QColor(150, 158, 170));
        painter.drawText(rect(), Qt::AlignCenter,
                         QStringLiteral("Open an image or start a camera"));
        return;
    }

    const QSize fitted = image_.size().scaled(size(), Qt::KeepAspectRatio);
    const QRect target((width() - fitted.width()) / 2, (height() - fitted.height()) / 2,
                       fitted.width(), fitted.height());
    painter.drawImage(target, image_);
    const double scaleX = static_cast<double>(target.width()) / image_.width();
    const double scaleY = static_cast<double>(target.height()) / image_.height();

    painter.setRenderHint(QPainter::Antialiasing, true);
    for (const auto& detection : detections_) {
        if (!detection.box.valid()) continue;
        const QColor color = colorForClass(detection.classId);
        const QRectF box(target.left() + detection.box.x1 * scaleX,
                         target.top() + detection.box.y1 * scaleY,
                         (detection.box.x2 - detection.box.x1) * scaleX,
                         (detection.box.y2 - detection.box.y1) * scaleY);
        painter.setPen(QPen(color, 2.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(box);
        QString label = QStringLiteral("class %1").arg(detection.classId);
        if (detection.classId >= 0 &&
            static_cast<std::size_t>(detection.classId) < labels_.size()) {
            label = QString::fromStdString(labels_[static_cast<std::size_t>(detection.classId)]);
        }
        label += QStringLiteral(" %1").arg(detection.confidence, 0, 'f', 2);
        const QRect textBounds = painter.fontMetrics().boundingRect(label).adjusted(-4, -2, 4, 2);
        QRectF background(box.left(), std::max(box.top(), target.top() + 1.0),
                          textBounds.width(), textBounds.height());
        if (background.right() > target.right()) background.moveRight(target.right());
        painter.fillRect(background, color);
        painter.setPen(Qt::white);
        painter.drawText(background, Qt::AlignCenter, label);
    }
}

}  // namespace odf::desktop

