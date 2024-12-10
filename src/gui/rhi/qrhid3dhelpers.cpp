// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhid3dhelpers_p.h"
#include <QtCore/private/qsystemlibrary_p.h>
#include <QtCore/private/qsystemerror_p.h>

QT_BEGIN_NAMESPACE

namespace QRhiD3D {

pD3DCompile resolveD3DCompile()
{
    for (const wchar_t *libraryName : {L"D3DCompiler_47", L"D3DCompiler_43"}) {
        QSystemLibrary library(libraryName);
        if (library.load()) {
            if (auto symbol = library.resolve("D3DCompile"))
                return reinterpret_cast<pD3DCompile>(symbol);
        } else {
            qWarning("Failed to load D3DCompiler_47/43.dll");
        }
    }
    return nullptr;
}

IDCompositionDevice *createDirectCompositionDevice()
{
    QSystemLibrary dcomplib(QStringLiteral("dcomp"));
    typedef HRESULT (__stdcall *DCompositionCreateDeviceFuncPtr)(
        _In_opt_ IDXGIDevice *dxgiDevice,
        _In_ REFIID iid,
        _Outptr_ void **dcompositionDevice);
    DCompositionCreateDeviceFuncPtr func = reinterpret_cast<DCompositionCreateDeviceFuncPtr>(
        dcomplib.resolve("DCompositionCreateDevice"));
    if (!func) {
        qWarning("Unable to resolve DCompositionCreateDevice, perhaps dcomp.dll is missing?");
        return nullptr;
    }
    IDCompositionDevice *device = nullptr;
    HRESULT hr = func(nullptr, __uuidof(IDCompositionDevice), reinterpret_cast<void **>(&device));
    if (FAILED(hr)) {
        qWarning("Failed to create Direct Composition device: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return nullptr;
    }
    return device;
}

#ifdef QRHI_D3D12_HAS_DXC
std::pair<IDxcCompiler *, IDxcLibrary *> createDxcCompiler()
{
    QSystemLibrary dxclib(QStringLiteral("dxcompiler"));
    // this will not be in the system library location, hence onlySystemDirectory==false
    if (!dxclib.load(false)) {
        qWarning("Failed to load dxcompiler.dll");
        return {};
    }
    DxcCreateInstanceProc func = reinterpret_cast<DxcCreateInstanceProc>(dxclib.resolve("DxcCreateInstance"));
    if (!func) {
        qWarning("Unable to resolve DxcCreateInstance");
        return {};
    }
    IDxcCompiler *compiler = nullptr;
    HRESULT hr = func(CLSID_DxcCompiler, __uuidof(IDxcCompiler), reinterpret_cast<void**>(&compiler));
    if (FAILED(hr)) {
        qWarning("Failed to create dxc compiler instance: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return {};
    }
    IDxcLibrary *library = nullptr;
    hr = func(CLSID_DxcLibrary, __uuidof(IDxcLibrary), reinterpret_cast<void**>(&library));
    if (FAILED(hr)) {
        qWarning("Failed to create dxc library instance: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return {};
    }
    return { compiler, library };
}
#endif

void fillDriverInfo(QRhiDriverInfo *info, const DXGI_ADAPTER_DESC1 &desc)
{
    const QString name = QString::fromUtf16(reinterpret_cast<const char16_t *>(desc.Description));
    info->deviceName = name.toUtf8();
    info->deviceId = desc.DeviceId;
    info->vendorId = desc.VendorId;
    info->deviceType = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) ? QRhiDriverInfo::CpuDevice
                                                                 : QRhiDriverInfo::UnknownDevice;
}

} // namespace

QT_END_NAMESPACE
