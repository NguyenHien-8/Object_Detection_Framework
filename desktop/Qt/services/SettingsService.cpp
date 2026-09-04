#include "services/SettingsService.hpp"

#include <QSettings>

namespace odf::desktop {

DesktopSettings SettingsService::load() const {
    QSettings store(QStringLiteral("ODF"), QStringLiteral("Object Detection Framework"));
    DesktopSettings result;
    result.modelId = store.value(QStringLiteral("model/id"), result.modelId).toString();
    result.backend = store.value(QStringLiteral("model/backend"), result.backend).toString();
    result.confidence = store.value(QStringLiteral("detection/confidence"), result.confidence).toDouble();
    result.iou = store.value(QStringLiteral("detection/iou"), result.iou).toDouble();
    result.maxDetections = store.value(QStringLiteral("detection/max"), result.maxDetections).toInt();
    result.cameraIndex = store.value(QStringLiteral("source/camera"), result.cameraIndex).toInt();
    result.lastImageDirectory = store.value(QStringLiteral("source/imageDirectory")).toString();
    result.modelRootOverride = store.value(QStringLiteral("model/rootOverride")).toString();
    result.classSelectionStored = store.contains(QStringLiteral("detection/selectedClasses"));
    for (const auto& value : store.value(QStringLiteral("detection/selectedClasses")).toList()) {
        result.selectedClasses.push_back(value.toInt());
    }
    result.windowGeometry = store.value(QStringLiteral("window/geometry")).toByteArray();
    return result;
}

void SettingsService::save(const DesktopSettings& settings) const {
    QSettings store(QStringLiteral("ODF"), QStringLiteral("Object Detection Framework"));
    store.setValue(QStringLiteral("model/id"), settings.modelId);
    store.setValue(QStringLiteral("model/backend"), settings.backend);
    store.setValue(QStringLiteral("detection/confidence"), settings.confidence);
    store.setValue(QStringLiteral("detection/iou"), settings.iou);
    store.setValue(QStringLiteral("detection/max"), settings.maxDetections);
    store.setValue(QStringLiteral("source/camera"), settings.cameraIndex);
    store.setValue(QStringLiteral("source/imageDirectory"), settings.lastImageDirectory);
    store.setValue(QStringLiteral("model/rootOverride"), settings.modelRootOverride);
    QVariantList selectedClasses;
    for (const int classId : settings.selectedClasses) selectedClasses.push_back(classId);
    store.setValue(QStringLiteral("detection/selectedClasses"), selectedClasses);
    store.setValue(QStringLiteral("window/geometry"), settings.windowGeometry);
    store.sync();
}

}  // namespace odf::desktop
