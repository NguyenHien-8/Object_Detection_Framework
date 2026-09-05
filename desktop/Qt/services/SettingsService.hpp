#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace odf::desktop {

struct DesktopSettings {
    QString modelId{QStringLiteral("yolo26n")};
    QString backend{QStringLiteral("onnxruntime")};
    double confidence{0.25};
    double iou{0.45};
    int maxDetections{300};
    QString cameraDeviceId;
    int legacyCameraIndex{-1};
    QString lastImageDirectory;
    QString modelRootOverride;
    QList<int> selectedClasses;
    bool classSelectionStored{false};
    QByteArray windowGeometry;
};

class SettingsService final {
public:
    [[nodiscard]] DesktopSettings load() const;
    void save(const DesktopSettings& settings) const;
};

}  // namespace odf::desktop
