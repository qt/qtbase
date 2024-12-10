// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdxgihdrinfo_p.h"
#include <QtCore/private/qsystemerror_p.h>

QT_BEGIN_NAMESPACE

QDxgiHdrInfo::QDxgiHdrInfo()
{
    HRESULT hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), reinterpret_cast<void **>(&m_factory));
    if (FAILED(hr)) {
        qWarning("QDxgiHdrInfo: CreateDXGIFactory2 failed: %s", qPrintable(QSystemError::windowsComString(hr)));
        return;
    }

    // Create the factory but leave m_adapter set to null, indicating that all
    // outputs of all adapters should be enumerated (if this is a good idea, not
    // sure, but there is no choice here, when someone wants to use this outside
    // of QRhi's scope and control)
}

QDxgiHdrInfo::QDxgiHdrInfo(LUID luid)
{
    HRESULT hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), reinterpret_cast<void **>(&m_factory));
    if (FAILED(hr)) {
        qWarning("QDxgiHdrInfo: CreateDXGIFactory2 failed: %s", qPrintable(QSystemError::windowsComString(hr)));
        return;
    }

    IDXGIAdapter1 *ad;
    for (int adapterIndex = 0; m_factory->EnumAdapters1(UINT(adapterIndex), &ad) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        ad->GetDesc1(&desc);
        if (desc.AdapterLuid.LowPart == luid.LowPart && desc.AdapterLuid.HighPart == luid.HighPart) {
            m_adapter = ad;
            break;
        }
        ad->Release();
    }

    if (!m_adapter)
        qWarning("QDxgiHdrInfo: No adapter found");
}

QDxgiHdrInfo::QDxgiHdrInfo(IDXGIAdapter1 *adapter)
    : m_adapter(adapter)
{
    m_adapter->AddRef(); // so that the dtor does not destroy it
}

QDxgiHdrInfo::~QDxgiHdrInfo()
{
    if (m_adapter)
        m_adapter->Release();

    if (m_factory)
        m_factory->Release();
}

bool QDxgiHdrInfo::isHdrCapable(QWindow *w)
{
    if (!m_adapter && m_factory) {
        IDXGIAdapter1 *adapter;
        for (int adapterIndex = 0; m_factory->EnumAdapters1(UINT(adapterIndex), &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
            DXGI_OUTPUT_DESC1 desc1;
            const bool ok = outputDesc1ForWindow(w, adapter,  &desc1) && desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
            adapter->Release();
            if (ok)
                return true;
        }
    } else if (m_adapter) {
        DXGI_OUTPUT_DESC1 desc1;
        if (outputDesc1ForWindow(w, m_adapter,  &desc1)) {
            // as per https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
            if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
                // "HDR display with all Advanced Color capabilities"
                return true;
            }
        }
    }

    return false;
}

QRhiSwapChainHdrInfo QDxgiHdrInfo::queryHdrInfo(QWindow *w)
{
    QRhiSwapChainHdrInfo info;
    info.limitsType = QRhiSwapChainHdrInfo::LuminanceInNits;
    info.limits.luminanceInNits.minLuminance = 0.0f;
    info.limits.luminanceInNits.maxLuminance = 1000.0f;
    info.luminanceBehavior = QRhiSwapChainHdrInfo::SceneReferred;
    info.sdrWhiteLevel = 200.0f;

    DXGI_OUTPUT_DESC1 hdrOutputDesc;
    bool ok = false;
    if (!m_adapter && m_factory) {
        IDXGIAdapter1 *adapter;
        for (int adapterIndex = 0; m_factory->EnumAdapters1(UINT(adapterIndex), &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
            ok = outputDesc1ForWindow(w, adapter, &hdrOutputDesc);
            adapter->Release();
            if (ok)
                break;
        }
    } else if (m_adapter) {
        ok = outputDesc1ForWindow(w, m_adapter, &hdrOutputDesc);
    }
    if (ok) {
        info.limitsType = QRhiSwapChainHdrInfo::LuminanceInNits;
        info.limits.luminanceInNits.minLuminance = hdrOutputDesc.MinLuminance;
        info.limits.luminanceInNits.maxLuminance = hdrOutputDesc.MaxLuminance;
        info.luminanceBehavior = QRhiSwapChainHdrInfo::SceneReferred; // 1.0 = 80 nits
        info.sdrWhiteLevel = sdrWhiteLevelInNits(hdrOutputDesc);
    }

    return info;
}

bool QDxgiHdrInfo::output6ForWindow(QWindow *w, IDXGIAdapter1 *adapter, IDXGIOutput6 **result)
{
    if (!adapter)
        return false;

    bool ok = false;
    QRect wr = w->geometry();
    wr = QRect(wr.topLeft() * w->devicePixelRatio(), wr.size() * w->devicePixelRatio());
    const QPoint center = wr.center();
    IDXGIOutput *currentOutput = nullptr;
    IDXGIOutput *output = nullptr;
    for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i) {
        if (!output)
            continue;
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        const RECT r = desc.DesktopCoordinates;
        const QRect dr(QPoint(r.left, r.top), QPoint(r.right - 1, r.bottom - 1));
        if (dr.contains(center)) {
            currentOutput = output;
            break;
        } else {
            output->Release();
        }
    }
    if (currentOutput) {
        ok = SUCCEEDED(currentOutput->QueryInterface(__uuidof(IDXGIOutput6), reinterpret_cast<void **>(result)));
        currentOutput->Release();
    }
    return ok;
}

bool QDxgiHdrInfo::outputDesc1ForWindow(QWindow *w, IDXGIAdapter1 *adapter, DXGI_OUTPUT_DESC1 *result)
{
    bool ok = false;
    IDXGIOutput6 *out6 = nullptr;
    if (output6ForWindow(w, adapter, &out6)) {
        ok = SUCCEEDED(out6->GetDesc1(result));
        out6->Release();
    }
    return ok;
}

float QDxgiHdrInfo::sdrWhiteLevelInNits(const DXGI_OUTPUT_DESC1 &outputDesc)
{
    QVector<DISPLAYCONFIG_PATH_INFO> pathInfos;
    uint32_t pathInfoCount, modeInfoCount;
    LONG result;
    do {
        if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathInfoCount, &modeInfoCount) == ERROR_SUCCESS) {
            pathInfos.resize(pathInfoCount);
            QVector<DISPLAYCONFIG_MODE_INFO> modeInfos(modeInfoCount);
            result = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathInfoCount, pathInfos.data(), &modeInfoCount, modeInfos.data(), nullptr);
        } else {
            return 200.0f;
        }
    } while (result == ERROR_INSUFFICIENT_BUFFER);

    MONITORINFOEX monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfo(outputDesc.Monitor, &monitorInfo);

    for (const DISPLAYCONFIG_PATH_INFO &info : pathInfos) {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME deviceName = {};
        deviceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        deviceName.header.size = sizeof(deviceName);
        deviceName.header.adapterId = info.sourceInfo.adapterId;
        deviceName.header.id = info.sourceInfo.id;
        if (DisplayConfigGetDeviceInfo(&deviceName.header) == ERROR_SUCCESS) {
            if (!wcscmp(monitorInfo.szDevice, deviceName.viewGdiDeviceName)) {
                DISPLAYCONFIG_SDR_WHITE_LEVEL whiteLevel = {};
                whiteLevel.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
                whiteLevel.header.size = sizeof(DISPLAYCONFIG_SDR_WHITE_LEVEL);
                whiteLevel.header.adapterId = info.targetInfo.adapterId;
                whiteLevel.header.id = info.targetInfo.id;
                if (DisplayConfigGetDeviceInfo(&whiteLevel.header) == ERROR_SUCCESS)
                    return whiteLevel.SDRWhiteLevel * 80 / 1000.0f;
            }
        }
    }

    return 200.0f;
}

QT_END_NAMESPACE
