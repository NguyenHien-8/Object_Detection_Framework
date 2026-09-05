#include "services/CameraDeviceService.hpp"

#include "odf/logging.hpp"

#include <opencv2/videoio.hpp>

#include <string>
#include <unordered_set>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <wrl/client.h>
#endif

namespace odf::desktop {

bool CameraSessionState::beginOpening(quint64 sessionId) noexcept {
    if (sessionId == 0 || (state_ != CameraState::Idle && state_ != CameraState::Error)) {
        return false;
    }
    activeSession_ = sessionId;
    state_ = CameraState::Opening;
    return true;
}

bool CameraSessionState::acknowledgeStarted(quint64 sessionId) noexcept {
    if (!accepts(sessionId) || state_ != CameraState::Opening) return false;
    state_ = CameraState::Running;
    return true;
}

bool CameraSessionState::beginStopping() noexcept {
    if (state_ != CameraState::Opening && state_ != CameraState::Running) return false;
    state_ = CameraState::Stopping;
    return true;
}

bool CameraSessionState::acknowledgeStopped(quint64 sessionId) noexcept {
    if (!accepts(sessionId) || state_ != CameraState::Stopping) return false;
    activeSession_ = 0;
    state_ = CameraState::Idle;
    return true;
}

bool CameraSessionState::fail(quint64 sessionId) noexcept {
    if (!accepts(sessionId)) return false;
    activeSession_ = 0;
    state_ = CameraState::Error;
    return true;
}

int CameraDeviceService::preferredDeviceIndex(
    const std::vector<CameraDeviceInfo>& devices, const QString& stableId,
    int legacyBackendIndex) noexcept {
    if (!stableId.isEmpty()) {
        for (std::size_t index = 0; index < devices.size(); ++index) {
            if (devices[index].stableId == stableId) return static_cast<int>(index);
        }
    }
    if (legacyBackendIndex >= 0) {
        for (std::size_t index = 0; index < devices.size(); ++index) {
            if (devices[index].backendIndex == legacyBackendIndex) {
                return static_cast<int>(index);
            }
        }
    }
    return devices.empty() ? -1 : 0;
}

#ifdef _WIN32
namespace {
using Microsoft::WRL::ComPtr;

class ComApartment final {
public:
    ComApartment() : result_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {
        ownsInitialization_ = result_ == S_OK || result_ == S_FALSE;
    }
    ~ComApartment() {
        if (ownsInitialization_) CoUninitialize();
    }

    [[nodiscard]] bool usable() const noexcept {
        return SUCCEEDED(result_) || result_ == RPC_E_CHANGED_MODE;
    }
    [[nodiscard]] HRESULT result() const noexcept { return result_; }

private:
    HRESULT result_{E_FAIL};
    bool ownsInitialization_{false};
};

class MediaFoundation final {
public:
    MediaFoundation() : result_(MFStartup(MF_VERSION, MFSTARTUP_LITE)) {}
    ~MediaFoundation() {
        if (SUCCEEDED(result_)) MFShutdown();
    }

    [[nodiscard]] HRESULT result() const noexcept { return result_; }

private:
    HRESULT result_{E_FAIL};
};

class ActivationArray final {
public:
    ~ActivationArray() {
        if (values) {
            for (UINT32 index = 0; index < count; ++index) {
                if (values[index]) values[index]->Release();
            }
            CoTaskMemFree(values);
        }
    }

    IMFActivate** values{nullptr};
    UINT32 count{0};
};

QString readAllocatedString(IMFActivate* activation, REFGUID key) {
    if (!activation) return {};
    WCHAR* value = nullptr;
    UINT32 length = 0;
    if (FAILED(activation->GetAllocatedString(key, &value, &length)) || !value) return {};
    const QString result = QString::fromWCharArray(value, static_cast<qsizetype>(length)).trimmed();
    CoTaskMemFree(value);
    return result;
}

std::string hresultMessage(const char* operation, HRESULT result) {
    return std::string(operation) + " failed (HRESULT=" +
           std::to_string(static_cast<unsigned long>(result)) + ")";
}
}  // namespace
#endif

Result<std::vector<CameraDeviceInfo>> CameraDeviceService::enumerate() const {
    std::vector<CameraDeviceInfo> devices;
#ifdef _WIN32
    ComApartment apartment;
    if (!apartment.usable()) {
        return Status::error(ErrorCode::CameraFailure,
                             hresultMessage("CoInitializeEx", apartment.result()));
    }

    MediaFoundation mediaFoundation;
    if (FAILED(mediaFoundation.result())) {
        return Status::error(ErrorCode::CameraFailure,
                             hresultMessage("Media Foundation startup",
                                            mediaFoundation.result()));
    }

    ComPtr<IMFAttributes> attributes;
    HRESULT result = MFCreateAttributes(attributes.GetAddressOf(), 1);
    if (FAILED(result)) {
        return Status::error(ErrorCode::CameraFailure,
                             hresultMessage("Media Foundation attributes creation", result));
    }
    result = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                 MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(result)) {
        return Status::error(ErrorCode::CameraFailure,
                             hresultMessage("Media Foundation capture filter", result));
    }

    ActivationArray activations;
    result = MFEnumDeviceSources(attributes.Get(), &activations.values, &activations.count);
    if (FAILED(result)) {
        return Status::error(ErrorCode::CameraFailure,
                             hresultMessage("Media Foundation camera enumeration", result));
    }
    std::unordered_set<std::wstring> identities;
    for (UINT32 index = 0; index < activations.count; ++index) {
        const QString displayName = readAllocatedString(
            activations.values[index], MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
        const QString stableId = readAllocatedString(
            activations.values[index],
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK);
        if (displayName.isEmpty() || stableId.isEmpty()) continue;

        const std::wstring identity = stableId.toStdWString();
        if (!identities.insert(identity).second) continue;
        devices.push_back({stableId, displayName, static_cast<int>(index),
                           static_cast<int>(cv::CAP_MSMF)});
        logging::log(logging::Level::Debug,
                     "camera discovered: id='" + stableId.toStdString() + "' name='" +
                         displayName.toStdString() + "' msmf_index=" +
                         std::to_string(index));
    }
#endif
    return devices;
}

}  // namespace odf::desktop
