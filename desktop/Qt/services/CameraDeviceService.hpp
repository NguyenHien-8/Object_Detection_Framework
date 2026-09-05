#pragma once

#include "odf/status.hpp"

#include <QString>
#include <QMetaType>
#include <QtGlobal>

#include <vector>

namespace odf::desktop {

struct CameraDeviceInfo {
    QString stableId;
    QString displayName;
    int backendIndex{-1};
    int backendApi{0};

    [[nodiscard]] bool valid() const noexcept {
        return !stableId.isEmpty() && !displayName.isEmpty() && backendIndex >= 0;
    }
};

struct CameraOpenRequest {
    quint64 sessionId{0};
    QString stableId;
    QString displayName;
    int backendIndex{-1};
    int backendApi{0};

    [[nodiscard]] bool valid() const noexcept {
        return sessionId != 0 && !stableId.isEmpty() && !displayName.isEmpty() &&
               backendIndex >= 0;
    }
};

enum class CameraState { Idle, Opening, Running, Stopping, Error };

// Small, deterministic state machine used by the controller and unit tests. Worker signals are
// accepted only when they belong to the active session.
class CameraSessionState final {
public:
    [[nodiscard]] CameraState state() const noexcept { return state_; }
    [[nodiscard]] quint64 activeSession() const noexcept { return activeSession_; }
    [[nodiscard]] bool accepts(quint64 sessionId) const noexcept {
        return sessionId != 0 && sessionId == activeSession_;
    }

    bool beginOpening(quint64 sessionId) noexcept;
    bool acknowledgeStarted(quint64 sessionId) noexcept;
    bool beginStopping() noexcept;
    bool acknowledgeStopped(quint64 sessionId) noexcept;
    bool fail(quint64 sessionId) noexcept;

private:
    CameraState state_{CameraState::Idle};
    quint64 activeSession_{0};
};

class CameraDeviceService final {
public:
    [[nodiscard]] Result<std::vector<CameraDeviceInfo>> enumerate() const;

    // Returns a vector position, not a backend index. Stable identity wins, then the legacy
    // backend index for one-time settings migration, then the first real device.
    [[nodiscard]] static int preferredDeviceIndex(
        const std::vector<CameraDeviceInfo>& devices, const QString& stableId,
        int legacyBackendIndex = -1) noexcept;
};

}  // namespace odf::desktop

Q_DECLARE_METATYPE(odf::desktop::CameraOpenRequest)
