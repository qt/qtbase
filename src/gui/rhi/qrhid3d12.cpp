// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhid3d12_p.h"
#include <qmath.h>
#include <QtCore/private/qsystemerror_p.h>
#include <comdef.h>
#include "qrhid3dhelpers_p.h"
#include "cs_mipmap_p.h"
#include "cs_mipmap_3d_p.h"

#if __has_include(<pix.h>)
#include <pix.h>
#define QRHI_D3D12_HAS_OLD_PIX
#endif

#ifdef __ID3D12Device2_INTERFACE_DEFINED__

QT_BEGIN_NAMESPACE

/*
  Direct 3D 12 backend.
*/

/*!
    \class QRhiD3D12InitParams
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \brief Direct3D 12 specific initialization parameters.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    A D3D12-based QRhi needs no special parameters for initialization. If
    desired, enableDebugLayer can be set to \c true to enable the Direct3D
    debug layer. This can be useful during development, but should be avoided
    in production builds.

    \badcode
        QRhiD3D12InitParams params;
        params.enableDebugLayer = true;
        rhi = QRhi::create(QRhi::D3D12, &params);
    \endcode

    \note QRhiSwapChain should only be used in combination with QWindow
    instances that have their surface type set to QSurface::Direct3DSurface.

    \section2 Working with existing Direct3D 12 devices

    When interoperating with another graphics engine, it may be necessary to
    get a QRhi instance that uses the same Direct3D device. This can be
    achieved by passing a pointer to a QRhiD3D12NativeHandles to
    QRhi::create(). QRhi does not take ownership of any of the external
    objects.

    Sometimes, for example when using QRhi in combination with OpenXR, one will
    want to specify which adapter to use, and optionally, which feature level
    to request on the device, while leaving the device creation to QRhi. This
    is achieved by leaving the device pointer set to null, while specifying the
    adapter LUID and feature level.

    Optionally the ID3D12CommandQueue can be specified as well, by setting \c
    commandQueue to a non-null value.
 */

/*!
    \variable QRhiD3D12InitParams::enableDebugLayer

    When set to true, the debug layer is enabled, if installed and available.
    The default value is false.
*/

/*!
    \class QRhiD3D12NativeHandles
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \brief Holds the D3D12 device used by the QRhi.

    \note The class uses \c{void *} as the type since including the COM-based
    \c{d3d12.h} headers is not acceptable here. The actual types are
    \c{ID3D12Device *} and \c{ID3D12CommandQueue *}.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiD3D12NativeHandles::dev

    Points to a
    \l{https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12device}{ID3D12Device}
    or left set to \nullptr if no existing device is to be imported.
*/

/*!
    \variable QRhiD3D12NativeHandles::minimumFeatureLevel

    Specifies the \b minimum feature level passed to
    \l{https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-d3d12createdevice}{D3D12CreateDevice()}.
    When not set, \c{D3D_FEATURE_LEVEL_11_0} is used. See
    \l{https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels}{this
    page} for details.

    Relevant only when QRhi creates the device, ignored when importing a device
    and device context.
*/

/*!
    \variable QRhiD3D12NativeHandles::adapterLuidLow

    The low part of the local identifier (LUID) of the DXGI adapter to use.
    Relevant only when QRhi creates the device, ignored when importing a device
    and device context.
*/

/*!
    \variable QRhiD3D12NativeHandles::adapterLuidHigh

    The high part of the local identifier (LUID) of the DXGI adapter to use.
    Relevant only when QRhi creates the device, ignored when importing a device
    and device context.
*/

/*!
    \variable QRhiD3D12NativeHandles::commandQueue

    When set, must point to a
    \l{https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12commandqueue}{ID3D12CommandQueue}.
    It allows to optionally import a command queue as well, in addition to a
    device.
*/

/*!
    \class QRhiD3D12CommandBufferNativeHandles
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \brief Holds the ID3D12GraphicsCommandList1 object that is backing a QRhiCommandBuffer.

    \note The command list object is only guaranteed to be valid, and
    in recording state, while recording a frame. That is, between a
    \l{QRhi::beginFrame()}{beginFrame()} - \l{QRhi::endFrame()}{endFrame()} or
    \l{QRhi::beginOffscreenFrame()}{beginOffscreenFrame()} -
    \l{QRhi::endOffscreenFrame()}{endOffscreenFrame()} pair.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiD3D12CommandBufferNativeHandles::commandList
*/

// https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels
static const D3D_FEATURE_LEVEL MIN_FEATURE_LEVEL = D3D_FEATURE_LEVEL_11_0;

QRhiD3D12::QRhiD3D12(QRhiD3D12InitParams *params, QRhiD3D12NativeHandles *importParams)
{
    debugLayer = params->enableDebugLayer;
    if (importParams) {
        if (importParams->dev) {
            ID3D12Device *d3d12Device = reinterpret_cast<ID3D12Device *>(importParams->dev);
            if (SUCCEEDED(d3d12Device->QueryInterface(__uuidof(ID3D12Device2), reinterpret_cast<void **>(&dev)))) {
                // get rid of the ref added by QueryInterface
                d3d12Device->Release();
                importedDevice = true;
            } else {
                qWarning("ID3D12Device2 not supported, cannot import device");
            }
        }
        if (importParams->commandQueue) {
            cmdQueue = reinterpret_cast<ID3D12CommandQueue *>(importParams->commandQueue);
            importedCommandQueue = true;
        }
        minimumFeatureLevel = D3D_FEATURE_LEVEL(importParams->minimumFeatureLevel);
        adapterLuid.LowPart = importParams->adapterLuidLow;
        adapterLuid.HighPart = importParams->adapterLuidHigh;
    }
}

template <class Int>
inline Int aligned(Int v, Int byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

static inline UINT calcSubresource(UINT mipSlice, UINT arraySlice, UINT mipLevels)
{
    return mipSlice + arraySlice * mipLevels;
}

static inline QD3D12RenderTargetData *rtData(QRhiRenderTarget *rt)
{
    switch (rt->resourceType()) {
    case QRhiResource::SwapChainRenderTarget:
        return &QRHI_RES(QD3D12SwapChainRenderTarget, rt)->d;
    case QRhiResource::TextureRenderTarget:
        return &QRHI_RES(QD3D12TextureRenderTarget, rt)->d;
        break;
    default:
        break;
    }
    Q_UNREACHABLE_RETURN(nullptr);
}

bool QRhiD3D12::create(QRhi::Flags flags)
{
    rhiFlags = flags;

    UINT factoryFlags = 0;
    if (debugLayer)
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    HRESULT hr = CreateDXGIFactory2(factoryFlags, __uuidof(IDXGIFactory2), reinterpret_cast<void **>(&dxgiFactory));
    if (FAILED(hr)) {
        // retry without debug, if it was requested (to match D3D11 backend behavior)
        if (debugLayer) {
            qCDebug(QRHI_LOG_INFO, "Debug layer was requested but is not available. "
                                   "Attempting to create DXGIFactory2 without it.");
            factoryFlags &= ~DXGI_CREATE_FACTORY_DEBUG;
            hr = CreateDXGIFactory2(factoryFlags, __uuidof(IDXGIFactory2), reinterpret_cast<void **>(&dxgiFactory));
        }
        if (SUCCEEDED(hr)) {
            debugLayer = false;
        } else {
            qWarning("CreateDXGIFactory2() failed to create DXGI factory: %s",
                     qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    }

    if (qEnvironmentVariableIsSet("QT_D3D_MAX_FRAME_LATENCY"))
        maxFrameLatency = UINT(qMax(0, qEnvironmentVariableIntValue("QT_D3D_MAX_FRAME_LATENCY")));
    if (maxFrameLatency != 0)
        qCDebug(QRHI_LOG_INFO, "Using frame latency waitable object with max frame latency %u", maxFrameLatency);

    supportsAllowTearing = false;
    IDXGIFactory5 *factory5 = nullptr;
    if (SUCCEEDED(dxgiFactory->QueryInterface(__uuidof(IDXGIFactory5), reinterpret_cast<void **>(&factory5)))) {
        BOOL allowTearing = false;
        if (SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            supportsAllowTearing = allowTearing;
        factory5->Release();
    }

    if (debugLayer) {
        ID3D12Debug1 *debug = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(ID3D12Debug1), reinterpret_cast<void **>(&debug)))) {
            qCDebug(QRHI_LOG_INFO, "Enabling D3D12 debug layer");
            debug->EnableDebugLayer();
            debug->Release();
        }
    }

    activeAdapter = nullptr;

    if (!importedDevice) {
        IDXGIAdapter1 *adapter;
        int requestedAdapterIndex = -1;
        if (qEnvironmentVariableIsSet("QT_D3D_ADAPTER_INDEX"))
            requestedAdapterIndex = qEnvironmentVariableIntValue("QT_D3D_ADAPTER_INDEX");

        if (requestedRhiAdapter)
            adapterLuid = static_cast<QD3D12Adapter *>(requestedRhiAdapter)->luid;

        // importParams or requestedRhiAdapter may specify an adapter by the luid, use that in the absence of an env.var. override.
        if (requestedAdapterIndex < 0 && (adapterLuid.LowPart || adapterLuid.HighPart)) {
            for (int adapterIndex = 0; dxgiFactory->EnumAdapters1(UINT(adapterIndex), &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);
                adapter->Release();
                if (desc.AdapterLuid.LowPart == adapterLuid.LowPart
                        && desc.AdapterLuid.HighPart == adapterLuid.HighPart)
                {
                    requestedAdapterIndex = adapterIndex;
                    break;
                }
            }
        }

        if (requestedAdapterIndex < 0 && flags.testFlag(QRhi::PreferSoftwareRenderer)) {
            for (int adapterIndex = 0; dxgiFactory->EnumAdapters1(UINT(adapterIndex), &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);
                adapter->Release();
                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                    requestedAdapterIndex = adapterIndex;
                    break;
                }
            }
        }

        for (int adapterIndex = 0; dxgiFactory->EnumAdapters1(UINT(adapterIndex), &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            const QString name = QString::fromUtf16(reinterpret_cast<char16_t *>(desc.Description));
            qCDebug(QRHI_LOG_INFO, "Adapter %d: '%s' (vendor 0x%X device 0x%X flags 0x%X)",
                    adapterIndex,
                    qPrintable(name),
                    desc.VendorId,
                    desc.DeviceId,
                    desc.Flags);
            if (!activeAdapter && (requestedAdapterIndex < 0 || requestedAdapterIndex == adapterIndex)) {
                activeAdapter = adapter;
                adapterLuid = desc.AdapterLuid;
                QRhiD3D::fillDriverInfo(&driverInfoStruct, desc);
                qCDebug(QRHI_LOG_INFO, "  using this adapter");
            } else {
                adapter->Release();
            }
        }
        if (!activeAdapter) {
            qWarning("No adapter");
            return false;
        }

        if (minimumFeatureLevel == 0)
            minimumFeatureLevel = MIN_FEATURE_LEVEL;

        hr = D3D12CreateDevice(activeAdapter,
                               minimumFeatureLevel,
                               __uuidof(ID3D12Device2),
                               reinterpret_cast<void **>(&dev));
        if (FAILED(hr)) {
            qWarning("Failed to create D3D12 device: %s", qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    } else {
        Q_ASSERT(dev);
        // cannot just get a IDXGIDevice from the ID3D12Device anymore, look up the adapter instead
        adapterLuid = dev->GetAdapterLuid();
        IDXGIAdapter1 *adapter;
        for (int adapterIndex = 0; dxgiFactory->EnumAdapters1(UINT(adapterIndex), &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (desc.AdapterLuid.LowPart == adapterLuid.LowPart
                    && desc.AdapterLuid.HighPart == adapterLuid.HighPart)
            {
                activeAdapter = adapter;
                QRhiD3D::fillDriverInfo(&driverInfoStruct, desc);
                break;
            } else {
                adapter->Release();
            }
        }
        if (!activeAdapter) {
            qWarning("No adapter");
            return false;
        }
        qCDebug(QRHI_LOG_INFO, "Using imported device %p", dev);
    }

    QDxgiVSyncService::instance()->refAdapter(adapterLuid);

    if (debugLayer) {
        ID3D12InfoQueue *infoQueue;
        if (SUCCEEDED(dev->QueryInterface(__uuidof(ID3D12InfoQueue), reinterpret_cast<void **>(&infoQueue)))) {
            if (qEnvironmentVariableIntValue("QT_D3D_DEBUG_BREAK")) {
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
            }
            D3D12_INFO_QUEUE_FILTER filter = {};
            D3D12_MESSAGE_ID suppressedMessages[2] = {
                // there is no way of knowing the clear color upfront
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                // we have no control over viewport and scissor rects
                D3D12_MESSAGE_ID_DRAW_EMPTY_SCISSOR_RECTANGLE
            };
            filter.DenyList.NumIDs = 2;
            filter.DenyList.pIDList = suppressedMessages;
            // Setting the filter would enable Info messages (e.g. about
            // resource creation) which we don't need.
            D3D12_MESSAGE_SEVERITY infoSev = D3D12_MESSAGE_SEVERITY_INFO;
            filter.DenyList.NumSeverities = 1;
            filter.DenyList.pSeverityList = &infoSev;
            infoQueue->PushStorageFilter(&filter);
            infoQueue->Release();
        }
    }

    if (!importedCommandQueue) {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        hr = dev->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void **>(&cmdQueue));
        if (FAILED(hr)) {
            qWarning("Failed to create command queue: %s", qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    }

    hr = dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), reinterpret_cast<void **>(&fullFence));
    if (FAILED(hr)) {
        qWarning("Failed to create fence: %s", qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }
    fullFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    fullFenceCounter = 0;

    for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
        hr = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                         __uuidof(ID3D12CommandAllocator),
                                         reinterpret_cast<void **>(&cmdAllocators[i]));
        if (FAILED(hr)) {
            qWarning("Failed to create command allocator: %s", qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    }

    if (!vma.create(dev, activeAdapter)) {
        qWarning("Failed to initialize graphics memory suballocator");
        return false;
    }

    if (!rtvPool.create(dev, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, "main RTV pool")) {
        qWarning("Could not create RTV pool");
        return false;
    }

    if (!dsvPool.create(dev, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, "main DSV pool")) {
        qWarning("Could not create DSV pool");
        return false;
    }

    if (!cbvSrvUavPool.create(dev, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "main CBV-SRV-UAV pool")) {
        qWarning("Could not create CBV-SRV-UAV pool");
        return false;
    }

    resourcePool.create("main resource pool");
    pipelinePool.create("main pipeline pool");
    rootSignaturePool.create("main root signature pool");
    releaseQueue.create(&resourcePool, &pipelinePool, &rootSignaturePool);
    barrierGen.create(&resourcePool);

    if (!samplerMgr.create(dev)) {
        qWarning("Could not create sampler pool and shader-visible sampler heap");
        return false;
    }

    if (!mipmapGen.create(this)) {
        qWarning("Could not initialize mipmap generator");
        return false;
    }

    if (!mipmapGen3D.create(this)) {
        qWarning("Could not initialize 3D texture mipmap generator");
        return false;
    }

    const qint32 smallStagingSize = aligned(SMALL_STAGING_AREA_BYTES_PER_FRAME, QD3D12StagingArea::ALIGNMENT);
    for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
        if (!smallStagingAreas[i].create(this, smallStagingSize, D3D12_HEAP_TYPE_UPLOAD)) {
            qWarning("Could not create host-visible staging area");
            return false;
        }
        QString decoratedName = QLatin1String("Small staging area buffer/");
        decoratedName += QString::number(i);
        smallStagingAreas[i].mem.buffer->SetName(reinterpret_cast<LPCWSTR>(decoratedName.utf16()));
    }

    if (!shaderVisibleCbvSrvUavHeap.create(dev,
                                           D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                           SHADER_VISIBLE_CBV_SRV_UAV_HEAP_PER_FRAME_START_SIZE))
    {
        qWarning("Could not create first shader-visible CBV/SRV/UAV heap");
        return false;
    }

    if (flags.testFlag(QRhi::EnableTimestamps)) {
        static bool wantsStablePowerState = qEnvironmentVariableIntValue("QT_D3D_STABLE_POWER_STATE");
        //
        // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-setstablepowerstate
        //
        // NB! This is a _global_ setting, affecting other processes (and 3D
        // APIs such as Vulkan), as long as this application is running. Hence
        // making it an env.var. for now. Never enable it in production. But
        // extremely useful for the GPU timings with NVIDIA at least; the
        // timestamps become stable and smooth, making the number readable and
        // actually useful e.g. in Quick 3D's DebugView when this is enabled.
        // (otherwise the number's all over the place)
        //
        // See also
        // https://developer.nvidia.com/blog/advanced-api-performance-setstablepowerstate/
        // for possible other approaches.
        //
        if (wantsStablePowerState)
            dev->SetStablePowerState(TRUE);

        hr = cmdQueue->GetTimestampFrequency(&timestampTicksPerSecond);
        if (FAILED(hr)) {
            qWarning("Failed to query timestamp frequency: %s",
                     qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
        if (!timestampQueryHeap.create(dev, QD3D12_FRAMES_IN_FLIGHT * 2, D3D12_QUERY_HEAP_TYPE_TIMESTAMP)) {
            qWarning("Failed to create timestamp query pool");
            return false;
        }
        const quint32 readbackBufSize = QD3D12_FRAMES_IN_FLIGHT * 2 * sizeof(quint64);
        if (!timestampReadbackArea.create(this, readbackBufSize, D3D12_HEAP_TYPE_READBACK)) {
            qWarning("Failed to create timestamp readback buffer");
            return false;
        }
        timestampReadbackArea.mem.buffer->SetName(L"Timestamp readback buffer");
        memset(timestampReadbackArea.mem.p, 0, readbackBufSize);
    }

    caps = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS3 options3 = {};
    if (SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &options3, sizeof(options3)))) {
        caps.multiView = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
        // https://microsoft.github.io/DirectX-Specs/d3d/RelaxedCasting.html
        caps.textureViewFormat = options3.CastingFullyTypedFormatSupported;
    }

#ifdef QRHI_D3D12_CL5_AVAILABLE
    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
    if (SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6)))) {
        caps.vrs = options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
        caps.vrsMap = options6.VariableShadingRateTier == D3D12_VARIABLE_SHADING_RATE_TIER_2;
        caps.vrsAdditionalRates = options6.AdditionalShadingRatesSupported;
        shadingRateImageTileSize = options6.ShadingRateImageTileSize;
    }
#else
    caps.vrs = false;
    caps.vrsMap = false;
    caps.vrsAdditionalRates = false;
#endif

    deviceLost = false;
    offscreenActive = false;

    nativeHandlesStruct.dev = dev;
    nativeHandlesStruct.minimumFeatureLevel = minimumFeatureLevel;
    nativeHandlesStruct.adapterLuidLow = adapterLuid.LowPart;
    nativeHandlesStruct.adapterLuidHigh = adapterLuid.HighPart;
    nativeHandlesStruct.commandQueue = cmdQueue;

    return true;
}

void QRhiD3D12::destroy()
{
    if (!deviceLost && fullFence && fullFenceEvent)
        waitGpu();

    releaseQueue.releaseAll();

    for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
        if (offscreenCb[i]) {
            if (offscreenCb[i]->cmdList)
                offscreenCb[i]->cmdList->Release();
            delete offscreenCb[i];
            offscreenCb[i] = nullptr;
        }
    }

    timestampQueryHeap.destroy();
    timestampReadbackArea.destroy();

    shaderVisibleCbvSrvUavHeap.destroy();

    for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i)
        smallStagingAreas[i].destroy();

    mipmapGen.destroy();
    mipmapGen3D.destroy();
    samplerMgr.destroy();
    resourcePool.destroy();
    pipelinePool.destroy();
    rootSignaturePool.destroy();
    rtvPool.destroy();
    dsvPool.destroy();
    cbvSrvUavPool.destroy();

    for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
        if (cmdAllocators[i]) {
            cmdAllocators[i]->Release();
            cmdAllocators[i] = nullptr;
        }
    }

    if (fullFenceEvent) {
        CloseHandle(fullFenceEvent);
        fullFenceEvent = nullptr;
    }

    if (fullFence) {
        fullFence->Release();
        fullFence = nullptr;
    }

    if (!importedCommandQueue) {
        if (cmdQueue) {
            cmdQueue->Release();
            cmdQueue = nullptr;
        }
    }

    vma.destroy();

    if (!importedDevice) {
        if (dev) {
            dev->Release();
            dev = nullptr;
        }
    }

    if (dcompDevice) {
        dcompDevice->Release();
        dcompDevice = nullptr;
    }

    if (activeAdapter) {
        activeAdapter->Release();
        activeAdapter = nullptr;
    }

    if (dxgiFactory) {
        dxgiFactory->Release();
        dxgiFactory = nullptr;
    }

    adapterLuid = {};
    importedDevice = false;
    importedCommandQueue = false;

    QDxgiVSyncService::instance()->derefAdapter(adapterLuid);
}

QRhi::AdapterList QRhiD3D12::enumerateAdaptersBeforeCreate(QRhiNativeHandles *nativeHandles) const
{
    LUID requestedLuid = {};
    if (nativeHandles) {
        QRhiD3D12NativeHandles *h = static_cast<QRhiD3D12NativeHandles *>(nativeHandles);
        const LUID adapterLuid = { h->adapterLuidLow, h->adapterLuidHigh };
        if (adapterLuid.LowPart || adapterLuid.HighPart)
            requestedLuid = adapterLuid;
    }

    IDXGIFactory2 *dxgi = nullptr;
    if (FAILED(CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), reinterpret_cast<void **>(&dxgi))))
        return {};

    QRhi::AdapterList list;
    IDXGIAdapter1 *adapter;
    for (int adapterIndex = 0; dxgi->EnumAdapters1(UINT(adapterIndex), &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        adapter->Release();
        if (requestedLuid.LowPart || requestedLuid.HighPart) {
            if (desc.AdapterLuid.LowPart != requestedLuid.LowPart
                || desc.AdapterLuid.HighPart != requestedLuid.HighPart)
            {
                continue;
            }
        }
        QD3D12Adapter *a = new QD3D12Adapter;
        a->luid = desc.AdapterLuid;
        QRhiD3D::fillDriverInfo(&a->adapterInfo, desc);
        list.append(a);
    }

    dxgi->Release();
    return list;
}

QRhiDriverInfo QD3D12Adapter::info() const
{
    return adapterInfo;
}

QList<int> QRhiD3D12::supportedSampleCounts() const
{
    return { 1, 2, 4, 8 };
}

QList<QSize> QRhiD3D12::supportedShadingRates(int sampleCount) const
{
    QList<QSize> sizes;
    switch (sampleCount) {
    case 0:
    case 1:
        if (caps.vrsAdditionalRates) {
            sizes.append(QSize(4, 4));
            sizes.append(QSize(4, 2));
            sizes.append(QSize(2, 4));
        }
        sizes.append(QSize(2, 2));
        sizes.append(QSize(2, 1));
        sizes.append(QSize(1, 2));
        break;
    case 2:
        if (caps.vrsAdditionalRates)
            sizes.append(QSize(2, 4));
        sizes.append(QSize(2, 2));
        sizes.append(QSize(2, 1));
        sizes.append(QSize(1, 2));
        break;
    case 4:
        sizes.append(QSize(2, 2));
        sizes.append(QSize(2, 1));
        sizes.append(QSize(1, 2));
        break;
    default:
        break;
    }
    sizes.append(QSize(1, 1));
    return sizes;
}

QRhiSwapChain *QRhiD3D12::createSwapChain()
{
    return new QD3D12SwapChain(this);
}

QRhiBuffer *QRhiD3D12::createBuffer(QRhiBuffer::Type type, QRhiBuffer::UsageFlags usage, quint32 size)
{
    return new QD3D12Buffer(this, type, usage, size);
}

int QRhiD3D12::ubufAlignment() const
{
    return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // 256
}

bool QRhiD3D12::isYUpInFramebuffer() const
{
    return false;
}

bool QRhiD3D12::isYUpInNDC() const
{
    return true;
}

bool QRhiD3D12::isClipDepthZeroToOne() const
{
    return true;
}

QMatrix4x4 QRhiD3D12::clipSpaceCorrMatrix() const
{
    // Like with Vulkan, but Y is already good.

    static QMatrix4x4 m;
    if (m.isIdentity()) {
        // NB the ctor takes row-major
        m = QMatrix4x4(1.0f, 0.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 0.5f, 0.5f,
                       0.0f, 0.0f, 0.0f, 1.0f);
    }
    return m;
}

bool QRhiD3D12::isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const
{
    Q_UNUSED(flags);

    if (format >= QRhiTexture::ETC2_RGB8 && format <= QRhiTexture::ASTC_12x12)
        return false;

    return true;
}

bool QRhiD3D12::isFeatureSupported(QRhi::Feature feature) const
{
    switch (feature) {
    case QRhi::MultisampleTexture:
        return true;
    case QRhi::MultisampleRenderBuffer:
        return true;
    case QRhi::DebugMarkers:
#ifdef QRHI_D3D12_HAS_OLD_PIX
        return true;
#else
        return false;
#endif
    case QRhi::Timestamps:
        return true;
    case QRhi::Instancing:
        return true;
    case QRhi::CustomInstanceStepRate:
        return true;
    case QRhi::PrimitiveRestart:
        return true;
    case QRhi::NonDynamicUniformBuffers:
        return false;
    case QRhi::NonFourAlignedEffectiveIndexBufferOffset:
        return true;
    case QRhi::NPOTTextureRepeat:
        return true;
    case QRhi::RedOrAlpha8IsRed:
        return true;
    case QRhi::ElementIndexUint:
        return true;
    case QRhi::Compute:
        return true;
    case QRhi::WideLines:
        return false;
    case QRhi::VertexShaderPointSize:
        return false;
    case QRhi::BaseVertex:
        return true;
    case QRhi::BaseInstance:
        return true;
    case QRhi::TriangleFanTopology:
        return false;
    case QRhi::ReadBackNonUniformBuffer:
        return true;
    case QRhi::ReadBackNonBaseMipLevel:
        return true;
    case QRhi::TexelFetch:
        return true;
    case QRhi::RenderToNonBaseMipLevel:
        return true;
    case QRhi::IntAttributes:
        return true;
    case QRhi::ScreenSpaceDerivatives:
        return true;
    case QRhi::ReadBackAnyTextureFormat:
        return true;
    case QRhi::PipelineCacheDataLoadSave:
        return false; // ###
    case QRhi::ImageDataStride:
        return true;
    case QRhi::RenderBufferImport:
        return false;
    case QRhi::ThreeDimensionalTextures:
        return true;
    case QRhi::RenderTo3DTextureSlice:
        return true;
    case QRhi::TextureArrays:
        return true;
    case QRhi::Tessellation:
        return true;
    case QRhi::GeometryShader:
        return true;
    case QRhi::TextureArrayRange:
        return true;
    case QRhi::NonFillPolygonMode:
        return true;
    case QRhi::OneDimensionalTextures:
        return true;
    case QRhi::OneDimensionalTextureMipmaps:
        return false; // we generate mipmaps ourselves with compute and this is not implemented
    case QRhi::HalfAttributes:
        return true;
    case QRhi::RenderToOneDimensionalTexture:
        return true;
    case QRhi::ThreeDimensionalTextureMipmaps:
        return true;
    case QRhi::MultiView:
        return caps.multiView;
    case QRhi::TextureViewFormat:
        return caps.textureViewFormat;
    case QRhi::ResolveDepthStencil:
        // there is no Multisample Resolve support for depth/stencil formats
        // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/hardware-support-for-direct3d-12-1-formats
        return false;
    case QRhi::VariableRateShading:
        return caps.vrs;
    case QRhi::VariableRateShadingMap:
    case QRhi::VariableRateShadingMapWithTexture:
        return caps.vrsMap;
    case QRhi::PerRenderTargetBlending:
    case QRhi::SampleVariables:
        return true;
    }
    return false;
}

int QRhiD3D12::resourceLimit(QRhi::ResourceLimit limit) const
{
    switch (limit) {
    case QRhi::TextureSizeMin:
        return 1;
    case QRhi::TextureSizeMax:
        return 16384;
    case QRhi::MaxColorAttachments:
        return 8;
    case QRhi::FramesInFlight:
        return QD3D12_FRAMES_IN_FLIGHT;
    case QRhi::MaxAsyncReadbackFrames:
        return QD3D12_FRAMES_IN_FLIGHT;
    case QRhi::MaxThreadGroupsPerDimension:
        return 65535;
    case QRhi::MaxThreadsPerThreadGroup:
        return 1024;
    case QRhi::MaxThreadGroupX:
        return 1024;
    case QRhi::MaxThreadGroupY:
        return 1024;
    case QRhi::MaxThreadGroupZ:
        return 1024;
    case QRhi::TextureArraySizeMax:
        return 2048;
    case QRhi::MaxUniformBufferRange:
        return 65536;
    case QRhi::MaxVertexInputs:
        return 32;
    case QRhi::MaxVertexOutputs:
        return 32;
    case QRhi::ShadingRateImageTileSize:
        return shadingRateImageTileSize;
    }
    return 0;
}

const QRhiNativeHandles *QRhiD3D12::nativeHandles()
{
    return &nativeHandlesStruct;
}

QRhiDriverInfo QRhiD3D12::driverInfo() const
{
    return driverInfoStruct;
}

QRhiStats QRhiD3D12::statistics()
{
    QRhiStats result;
    result.totalPipelineCreationTime = totalPipelineCreationTime();

    D3D12MA::Budget budgets[2]; // [gpu, system] with discreet GPU or [shared, nothing] with UMA
    vma.getBudget(&budgets[0], &budgets[1]);
    for (int i = 0; i < 2; ++i) {
        const D3D12MA::Statistics &stats(budgets[i].Stats);
        result.blockCount += stats.BlockCount;
        result.allocCount += stats.AllocationCount;
        result.usedBytes += stats.AllocationBytes;
        result.unusedBytes += stats.BlockBytes - stats.AllocationBytes;
        result.totalUsageBytes += budgets[i].UsageBytes;
    }

    return result;
}

bool QRhiD3D12::makeThreadLocalNativeContextCurrent()
{
    // not applicable
    return false;
}

void QRhiD3D12::setQueueSubmitParams(QRhiNativeHandles *)
{
    // not applicable
}

void QRhiD3D12::releaseCachedResources()
{
    shaderBytecodeCache.data.clear();
}

bool QRhiD3D12::isDeviceLost() const
{
    return deviceLost;
}

QByteArray QRhiD3D12::pipelineCacheData()
{
    return {};
}

void QRhiD3D12::setPipelineCacheData(const QByteArray &data)
{
    Q_UNUSED(data);
}

QRhiRenderBuffer *QRhiD3D12::createRenderBuffer(QRhiRenderBuffer::Type type, const QSize &pixelSize,
                                                int sampleCount, QRhiRenderBuffer::Flags flags,
                                                QRhiTexture::Format backingFormatHint)
{
    return new QD3D12RenderBuffer(this, type, pixelSize, sampleCount, flags, backingFormatHint);
}

QRhiTexture *QRhiD3D12::createTexture(QRhiTexture::Format format,
                                      const QSize &pixelSize, int depth, int arraySize,
                                      int sampleCount, QRhiTexture::Flags flags)
{
    return new QD3D12Texture(this, format, pixelSize, depth, arraySize, sampleCount, flags);
}

QRhiSampler *QRhiD3D12::createSampler(QRhiSampler::Filter magFilter, QRhiSampler::Filter minFilter,
                                      QRhiSampler::Filter mipmapMode,
                                      QRhiSampler::AddressMode u, QRhiSampler::AddressMode v, QRhiSampler::AddressMode w)
{
    return new QD3D12Sampler(this, magFilter, minFilter, mipmapMode, u, v, w);
}

QRhiTextureRenderTarget *QRhiD3D12::createTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                              QRhiTextureRenderTarget::Flags flags)
{
    return new QD3D12TextureRenderTarget(this, desc, flags);
}

QRhiShadingRateMap *QRhiD3D12::createShadingRateMap()
{
    return new QD3D12ShadingRateMap(this);
}

QRhiGraphicsPipeline *QRhiD3D12::createGraphicsPipeline()
{
    return new QD3D12GraphicsPipeline(this);
}

QRhiComputePipeline *QRhiD3D12::createComputePipeline()
{
    return new QD3D12ComputePipeline(this);
}

QRhiShaderResourceBindings *QRhiD3D12::createShaderResourceBindings()
{
    return new QD3D12ShaderResourceBindings(this);
}

void QRhiD3D12::setGraphicsPipeline(QRhiCommandBuffer *cb, QRhiGraphicsPipeline *ps)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::RenderPass);
    QD3D12GraphicsPipeline *psD = QRHI_RES(QD3D12GraphicsPipeline, ps);
    const bool pipelineChanged = cbD->currentGraphicsPipeline != psD || cbD->currentPipelineGeneration != psD->generation;

    if (pipelineChanged) {
        cbD->currentGraphicsPipeline = psD;
        cbD->currentComputePipeline = nullptr;
        cbD->currentPipelineGeneration = psD->generation;

        if (QD3D12Pipeline *pipeline = pipelinePool.lookupRef(psD->handle)) {
            Q_ASSERT(pipeline->type == QD3D12Pipeline::Graphics);
            cbD->cmdList->SetPipelineState(pipeline->pso);
            if (QD3D12RootSignature *rs = rootSignaturePool.lookupRef(psD->rootSigHandle))
                cbD->cmdList->SetGraphicsRootSignature(rs->rootSig);
        }

        cbD->cmdList->IASetPrimitiveTopology(psD->topology);

        if (psD->viewInstanceMask)
            cbD->cmdList->SetViewInstanceMask(psD->viewInstanceMask);
    }
}

void QD3D12CommandBuffer::visitUniformBuffer(QD3D12Stage s,
                                             const QRhiShaderResourceBinding::Data::UniformBufferData &d,
                                             int,
                                             int binding,
                                             int dynamicOffsetCount,
                                             const QRhiCommandBuffer::DynamicOffset *dynamicOffsets)
{
    QD3D12Buffer *bufD = QRHI_RES(QD3D12Buffer, d.buf);
    quint32 offset = d.offset;
    if (d.hasDynamicOffset) {
        for (int i = 0; i < dynamicOffsetCount; ++i) {
            const QRhiCommandBuffer::DynamicOffset &dynOfs(dynamicOffsets[i]);
            if (dynOfs.first == binding) {
                Q_ASSERT(aligned(dynOfs.second, 256u) == dynOfs.second);
                offset += dynOfs.second;
            }
        }
    }
    QRHI_RES_RHI(QRhiD3D12);
    visitorData.cbufs[s].append({ bufD->handles[rhiD->currentFrameSlot], offset });
}

void QD3D12CommandBuffer::visitTexture(QD3D12Stage s,
                                       const QRhiShaderResourceBinding::TextureAndSampler &d,
                                       int)
{
    QD3D12Texture *texD = QRHI_RES(QD3D12Texture, d.tex);
    visitorData.srvs[s].append(texD->srv);
}

void QD3D12CommandBuffer::visitSampler(QD3D12Stage s,
                                       const QRhiShaderResourceBinding::TextureAndSampler &d,
                                       int)
{
    QD3D12Sampler *samplerD = QRHI_RES(QD3D12Sampler, d.sampler);
    visitorData.samplers[s].append(samplerD->lookupOrCreateShaderVisibleDescriptor());
}

void QD3D12CommandBuffer::visitStorageBuffer(QD3D12Stage s,
                                             const QRhiShaderResourceBinding::Data::StorageBufferData &d,
                                             QD3D12ShaderResourceVisitor::StorageOp,
                                             int)
{
    QD3D12Buffer *bufD = QRHI_RES(QD3D12Buffer, d.buf);
    // SPIRV-Cross generated HLSL uses RWByteAddressBuffer
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = d.offset / 4;
    uavDesc.Buffer.NumElements = aligned(bufD->m_size - d.offset, 4u) / 4;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    visitorData.uavs[s].append({ bufD->handles[0], uavDesc });
}

void QD3D12CommandBuffer::visitStorageImage(QD3D12Stage s,
                                            const QRhiShaderResourceBinding::Data::StorageImageData &d,
                                            QD3D12ShaderResourceVisitor::StorageOp,
                                            int)
{
    QD3D12Texture *texD = QRHI_RES(QD3D12Texture, d.tex);
    const bool isCube = texD->m_flags.testFlag(QRhiTexture::CubeMap);
    const bool isArray = texD->m_flags.testFlag(QRhiTexture::TextureArray);
    const bool is3D = texD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = texD->rtFormat;
    if (isCube) {
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Texture2DArray.MipSlice = UINT(d.level);
        uavDesc.Texture2DArray.FirstArraySlice = 0;
        uavDesc.Texture2DArray.ArraySize = 6;
    } else if (isArray) {
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Texture2DArray.MipSlice = UINT(d.level);
        uavDesc.Texture2DArray.FirstArraySlice = 0;
        uavDesc.Texture2DArray.ArraySize = UINT(qMax(0, texD->m_arraySize));
    } else if (is3D) {
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.MipSlice = UINT(d.level);
        uavDesc.Texture3D.WSize = UINT(-1);
    } else {
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = UINT(d.level);
    }
    visitorData.uavs[s].append({ texD->handle, uavDesc });
}

void QRhiD3D12::setShaderResources(QRhiCommandBuffer *cb, QRhiShaderResourceBindings *srb,
                                   int dynamicOffsetCount,
                                   const QRhiCommandBuffer::DynamicOffset *dynamicOffsets)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass != QD3D12CommandBuffer::NoPass);
    QD3D12GraphicsPipeline *gfxPsD = QRHI_RES(QD3D12GraphicsPipeline, cbD->currentGraphicsPipeline);
    QD3D12ComputePipeline *compPsD = QRHI_RES(QD3D12ComputePipeline, cbD->currentComputePipeline);

    if (!srb) {
        if (gfxPsD)
            srb = gfxPsD->m_shaderResourceBindings;
        else
            srb = compPsD->m_shaderResourceBindings;
    }

    QD3D12ShaderResourceBindings *srbD = QRHI_RES(QD3D12ShaderResourceBindings, srb);

    for (int i = 0, ie = srbD->m_bindings.size(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = shaderResourceBindingData(srbD->m_bindings[i]);
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            QD3D12Buffer *bufD = QRHI_RES(QD3D12Buffer, b->u.ubuf.buf);
            Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::UniformBuffer));
            Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
            bufD->executeHostWritesForFrameSlot(currentFrameSlot);
        }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
        case QRhiShaderResourceBinding::Texture:
        case QRhiShaderResourceBinding::Sampler:
        {
            const QRhiShaderResourceBinding::Data::TextureAndOrSamplerData *data = &b->u.stex;
            for (int elem = 0; elem < data->count; ++elem) {
                QD3D12Texture *texD = QRHI_RES(QD3D12Texture, data->texSamplers[elem].tex);
                QD3D12Sampler *samplerD = QRHI_RES(QD3D12Sampler, data->texSamplers[elem].sampler);
                // We use the same code path for both combined and separate
                // images and samplers, so tex or sampler (but not both) can be
                // null here.
                Q_ASSERT(texD || samplerD);
                if (texD) {
                    UINT state = 0;
                    if (b->stage == QRhiShaderResourceBinding::FragmentStage) {
                        state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                    } else if (b->stage.testFlag(QRhiShaderResourceBinding::FragmentStage)) {
                        state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                    } else {
                        state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                    }
                    barrierGen.addTransitionBarrier(texD->handle, D3D12_RESOURCE_STATES(state));
                    barrierGen.enqueueBufferedTransitionBarriers(cbD);
                }
            }
        }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
        {
            QD3D12Texture *texD = QRHI_RES(QD3D12Texture, b->u.simage.tex);
            if (QD3D12Resource *res = resourcePool.lookupRef(texD->handle)) {
                if (res->uavUsage) {
                    if (res->uavUsage & QD3D12Resource::UavUsageWrite) {
                        // RaW or WaW
                        barrierGen.enqueueUavBarrier(cbD, texD->handle);
                    } else {
                        if (b->type == QRhiShaderResourceBinding::ImageStore
                                || b->type == QRhiShaderResourceBinding::ImageLoadStore)
                        {
                            // WaR or WaW
                            barrierGen.enqueueUavBarrier(cbD, texD->handle);
                        }
                    }
                }
                res->uavUsage = 0;
                if (b->type == QRhiShaderResourceBinding::ImageLoad || b->type == QRhiShaderResourceBinding::ImageLoadStore)
                    res->uavUsage |= QD3D12Resource::UavUsageRead;
                if (b->type == QRhiShaderResourceBinding::ImageStore || b->type == QRhiShaderResourceBinding::ImageLoadStore)
                    res->uavUsage |= QD3D12Resource::UavUsageWrite;
                barrierGen.addTransitionBarrier(texD->handle, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                barrierGen.enqueueBufferedTransitionBarriers(cbD);
            }
        }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
        case QRhiShaderResourceBinding::BufferStore:
        case QRhiShaderResourceBinding::BufferLoadStore:
        {
            QD3D12Buffer *bufD = QRHI_RES(QD3D12Buffer, b->u.sbuf.buf);
            Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::StorageBuffer));
            Q_ASSERT(bufD->m_type != QRhiBuffer::Dynamic);
            if (QD3D12Resource *res = resourcePool.lookupRef(bufD->handles[0])) {
                if (res->uavUsage) {
                    if (res->uavUsage & QD3D12Resource::UavUsageWrite) {
                        // RaW or WaW
                        barrierGen.enqueueUavBarrier(cbD, bufD->handles[0]);
                    } else {
                        if (b->type == QRhiShaderResourceBinding::BufferStore
                                || b->type == QRhiShaderResourceBinding::BufferLoadStore)
                        {
                            // WaR or WaW
                            barrierGen.enqueueUavBarrier(cbD, bufD->handles[0]);
                        }
                    }
                }
                res->uavUsage = 0;
                if (b->type == QRhiShaderResourceBinding::BufferLoad || b->type == QRhiShaderResourceBinding::BufferLoadStore)
                    res->uavUsage |= QD3D12Resource::UavUsageRead;
                if (b->type == QRhiShaderResourceBinding::BufferStore || b->type == QRhiShaderResourceBinding::BufferLoadStore)
                    res->uavUsage |= QD3D12Resource::UavUsageWrite;
                barrierGen.addTransitionBarrier(bufD->handles[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                barrierGen.enqueueBufferedTransitionBarriers(cbD);
            }
        }
            break;
        }
    }

    const bool srbChanged = gfxPsD ? (cbD->currentGraphicsSrb != srb) : (cbD->currentComputeSrb != srb);
    const bool srbRebuilt = cbD->currentSrbGeneration != srbD->generation;

    if (srbChanged || srbRebuilt || srbD->hasDynamicOffset) {
        const QD3D12ShaderStageData *stageData = gfxPsD ? gfxPsD->stageData.data() : &compPsD->stageData;

        // The order of root parameters must match
        // QD3D12ShaderResourceBindings::createRootSignature(), meaning the
        // logic below must mirror that function (uniform buffers first etc.)

        QD3D12ShaderResourceVisitor visitor(srbD, stageData, gfxPsD ? 5 : 1);

        QD3D12CommandBuffer::VisitorData &visitorData(cbD->visitorData);
        visitorData = {};

        using namespace std::placeholders;
        visitor.uniformBuffer = std::bind(&QD3D12CommandBuffer::visitUniformBuffer, cbD, _1, _2, _3, _4, dynamicOffsetCount, dynamicOffsets);
        visitor.texture = std::bind(&QD3D12CommandBuffer::visitTexture, cbD, _1, _2, _3);
        visitor.sampler = std::bind(&QD3D12CommandBuffer::visitSampler, cbD, _1, _2, _3);
        visitor.storageBuffer = std::bind(&QD3D12CommandBuffer::visitStorageBuffer, cbD, _1, _2, _3, _4);
        visitor.storageImage = std::bind(&QD3D12CommandBuffer::visitStorageImage, cbD, _1, _2, _3, _4);

        visitor.visit();

        quint32 cbvSrvUavCount = 0;
        for (int s = 0; s < 6; ++s) {
            // CBs use root constant buffer views, no need to count them here
            cbvSrvUavCount += visitorData.srvs[s].count();
            cbvSrvUavCount += visitorData.uavs[s].count();
        }

        bool gotNewHeap = false;
        if (!ensureShaderVisibleDescriptorHeapCapacity(&shaderVisibleCbvSrvUavHeap,
                                                       D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                       currentFrameSlot,
                                                       cbvSrvUavCount,
                                                       &gotNewHeap))
        {
            return;
        }
        if (gotNewHeap) {
            qCDebug(QRHI_LOG_INFO, "Created new shader-visible CBV/SRV/UAV descriptor heap,"
                    " per-frame slice size is now %u,"
                    " if this happens frequently then that's not great.",
                    shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[0].capacity);
            bindShaderVisibleHeaps(cbD);
        }

        int rootParamIndex = 0;
        for (int s = 0; s < 6; ++s) {
            if (!visitorData.cbufs[s].isEmpty()) {
                for (int i = 0, count = visitorData.cbufs[s].count(); i < count; ++i) {
                    const auto &cbuf(visitorData.cbufs[s][i]);
                    if (QD3D12Resource *res = resourcePool.lookupRef(cbuf.first)) {
                        quint32 offset = cbuf.second;
                        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = res->resource->GetGPUVirtualAddress() + offset;
                        if (cbD->currentGraphicsPipeline)
                            cbD->cmdList->SetGraphicsRootConstantBufferView(rootParamIndex, gpuAddr);
                        else
                            cbD->cmdList->SetComputeRootConstantBufferView(rootParamIndex, gpuAddr);
                    }
                    rootParamIndex += 1;
                }
            }
        }
        for (int s = 0; s < 6; ++s) {
            if (!visitorData.srvs[s].isEmpty()) {
                QD3D12DescriptorHeap &gpuSrvHeap(shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[currentFrameSlot]);
                QD3D12Descriptor startDesc = gpuSrvHeap.get(visitorData.srvs[s].count());
                for (int i = 0, count = visitorData.srvs[s].count(); i < count; ++i) {
                    const auto &srv(visitorData.srvs[s][i]);
                    dev->CopyDescriptorsSimple(1, gpuSrvHeap.incremented(startDesc, i).cpuHandle, srv.cpuHandle,
                                               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }

                if (cbD->currentGraphicsPipeline)
                    cbD->cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, startDesc.gpuHandle);
                else if (cbD->currentComputePipeline)
                    cbD->cmdList->SetComputeRootDescriptorTable(rootParamIndex, startDesc.gpuHandle);

                rootParamIndex += 1;
            }
        }
        for (int s = 0; s < 6; ++s) {
            // Samplers are one parameter / descriptor table each, and the
            // descriptor is from the shader visible sampler heap already.
            for (const QD3D12Descriptor &samplerDescriptor : visitorData.samplers[s]) {
                if (cbD->currentGraphicsPipeline)
                    cbD->cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, samplerDescriptor.gpuHandle);
                else if (cbD->currentComputePipeline)
                    cbD->cmdList->SetComputeRootDescriptorTable(rootParamIndex, samplerDescriptor.gpuHandle);

                rootParamIndex += 1;
            }
        }
        for (int s = 0; s < 6; ++s) {
            if (!visitorData.uavs[s].isEmpty()) {
                QD3D12DescriptorHeap &gpuUavHeap(shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[currentFrameSlot]);
                QD3D12Descriptor startDesc = gpuUavHeap.get(visitorData.uavs[s].count());
                for (int i = 0, count = visitorData.uavs[s].count(); i < count; ++i) {
                    const auto &uav(visitorData.uavs[s][i]);
                    if (QD3D12Resource *res = resourcePool.lookupRef(uav.first)) {
                        dev->CreateUnorderedAccessView(res->resource, nullptr, &uav.second,
                                                       gpuUavHeap.incremented(startDesc, i).cpuHandle);
                    } else {
                        dev->CreateUnorderedAccessView(nullptr, nullptr, nullptr,
                                                       gpuUavHeap.incremented(startDesc, i).cpuHandle);
                    }
                }

                if (cbD->currentGraphicsPipeline)
                    cbD->cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, startDesc.gpuHandle);
                else if (cbD->currentComputePipeline)
                    cbD->cmdList->SetComputeRootDescriptorTable(rootParamIndex, startDesc.gpuHandle);

                rootParamIndex += 1;
            }
        }

        if (gfxPsD) {
            cbD->currentGraphicsSrb = srb;
            cbD->currentComputeSrb = nullptr;
        } else {
            cbD->currentGraphicsSrb = nullptr;
            cbD->currentComputeSrb = srb;
        }
        cbD->currentSrbGeneration = srbD->generation;
    }
}

void QRhiD3D12::setVertexInput(QRhiCommandBuffer *cb,
                               int startBinding, int bindingCount, const QRhiCommandBuffer::VertexInput *bindings,
                               QRhiBuffer *indexBuf, quint32 indexOffset, QRhiCommandBuffer::IndexFormat indexFormat)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::RenderPass);

    bool needsBindVBuf = false;
    for (int i = 0; i < bindingCount; ++i) {
        const int inputSlot = startBinding + i;
        QD3D12Buffer *bufD = QRHI_RES(QD3D12Buffer, bindings[i].first);
        Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::VertexBuffer));
        const bool isDynamic = bufD->m_type == QRhiBuffer::Dynamic;
        if (isDynamic)
            bufD->executeHostWritesForFrameSlot(currentFrameSlot);

        if (cbD->currentVertexBuffers[inputSlot] != bufD->handles[isDynamic ? currentFrameSlot : 0]
                || cbD->currentVertexOffsets[inputSlot] != bindings[i].second)
        {
            needsBindVBuf = true;
            cbD->currentVertexBuffers[inputSlot] = bufD->handles[isDynamic ? currentFrameSlot : 0];
            cbD->currentVertexOffsets[inputSlot] = bindings[i].second;
        }
    }

    if (needsBindVBuf) {
        QVarLengthArray<D3D12_VERTEX_BUFFER_VIEW, 4> vbv;
        vbv.reserve(bindingCount);

        QD3D12GraphicsPipeline *psD = cbD->currentGraphicsPipeline;
        const QRhiVertexInputLayout &inputLayout(psD->m_vertexInputLayout);
        const int inputBindingCount = inputLayout.cendBindings() - inputLayout.cbeginBindings();

        for (int i = 0, ie = qMin(bindingCount, inputBindingCount); i != ie; ++i) {
            QD3D12Buffer *bufD = QRHI_RES(QD3D12Buffer, bindings[i].first);
            const QD3D12ObjectHandle handle = bufD->handles[bufD->m_type == QRhiBuffer::Dynamic ? currentFrameSlot : 0];
            const quint32 offset = bindings[i].second;
            const quint32 stride = inputLayout.bindingAt(i)->stride();

            if (bufD->m_type != QRhiBuffer::Dynamic) {
                barrierGen.addTransitionBarrier(handle, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                barrierGen.enqueueBufferedTransitionBarriers(cbD);
            }

            if (QD3D12Resource *res = resourcePool.lookupRef(handle)) {
                vbv.append({
                    res->resource->GetGPUVirtualAddress() + offset,
                    UINT(res->desc.Width - offset),
                    stride
                });
            }
        }

        cbD->cmdList->IASetVertexBuffers(UINT(startBinding), vbv.count(), vbv.constData());
    }

    if (indexBuf) {
        QD3D12Buffer *ibufD = QRHI_RES(QD3D12Buffer, indexBuf);
        Q_ASSERT(ibufD->m_usage.testFlag(QRhiBuffer::IndexBuffer));
        const bool isDynamic = ibufD->m_type == QRhiBuffer::Dynamic;
        if (isDynamic)
            ibufD->executeHostWritesForFrameSlot(currentFrameSlot);

        const DXGI_FORMAT dxgiFormat = indexFormat == QRhiCommandBuffer::IndexUInt16 ? DXGI_FORMAT_R16_UINT
                                                                                     : DXGI_FORMAT_R32_UINT;
        if (cbD->currentIndexBuffer != ibufD->handles[isDynamic ? currentFrameSlot : 0]
                || cbD->currentIndexOffset != indexOffset
                || cbD->currentIndexFormat != dxgiFormat)
        {
            cbD->currentIndexBuffer = ibufD->handles[isDynamic ? currentFrameSlot : 0];
            cbD->currentIndexOffset = indexOffset;
            cbD->currentIndexFormat = dxgiFormat;

            if (ibufD->m_type != QRhiBuffer::Dynamic) {
                barrierGen.addTransitionBarrier(cbD->currentIndexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);
                barrierGen.enqueueBufferedTransitionBarriers(cbD);
            }

            if (QD3D12Resource *res = resourcePool.lookupRef(cbD->currentIndexBuffer)) {
                const D3D12_INDEX_BUFFER_VIEW ibv = {
                    res->resource->GetGPUVirtualAddress() + indexOffset,
                    UINT(res->desc.Width - indexOffset),
                    dxgiFormat
                };
                cbD->cmdList->IASetIndexBuffer(&ibv);
            }
        }
    }
}

void QRhiD3D12::setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::RenderPass);
    Q_ASSERT(cbD->currentTarget);
    const QSize outputSize = cbD->currentTarget->pixelSize();

    // D3D expects top-left, QRhiViewport is bottom-left
    float x, y, w, h;
    if (!qrhi_toTopLeftRenderTargetRect<UnBounded>(outputSize, viewport.viewport(), &x, &y, &w, &h))
        return;

    D3D12_VIEWPORT v;
    v.TopLeftX = x;
    v.TopLeftY = y;
    v.Width = w;
    v.Height = h;
    v.MinDepth = viewport.minDepth();
    v.MaxDepth = viewport.maxDepth();
    cbD->cmdList->RSSetViewports(1, &v);

    if (cbD->currentGraphicsPipeline
            && !cbD->currentGraphicsPipeline->flags().testFlag(QRhiGraphicsPipeline::UsesScissor))
    {
        qrhi_toTopLeftRenderTargetRect<Bounded>(outputSize, viewport.viewport(), &x, &y, &w, &h);
        D3D12_RECT r;
        r.left = x;
        r.top = y;
        // right and bottom are exclusive
        r.right = x + w;
        r.bottom = y + h;
        cbD->cmdList->RSSetScissorRects(1, &r);
    }
}

void QRhiD3D12::setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::RenderPass);
    Q_ASSERT(cbD->currentTarget);
    const QSize outputSize = cbD->currentTarget->pixelSize();

    // D3D expects top-left, QRhiScissor is bottom-left
    int x, y, w, h;
    if (!qrhi_toTopLeftRenderTargetRect<Bounded>(outputSize, scissor.scissor(), &x, &y, &w, &h))
        return;

    D3D12_RECT r;
    r.left = x;
    r.top = y;
    // right and bottom are exclusive
    r.right = x + w;
    r.bottom = y + h;
    cbD->cmdList->RSSetScissorRects(1, &r);
}

void QRhiD3D12::setBlendConstants(QRhiCommandBuffer *cb, const QColor &c)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::RenderPass);
    float v[4] = { c.redF(), c.greenF(), c.blueF(), c.alphaF() };
    cbD->cmdList->OMSetBlendFactor(v);
}

void QRhiD3D12::setStencilRef(QRhiCommandBuffer *cb, quint32 refValue)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::RenderPass);
    cbD->cmdList->OMSetStencilRef(refValue);
}

static inline D3D12_SHADING_RATE toD3DShadingRate(const QSize &coarsePixelSize)
{
    if (coarsePixelSize == QSize(1, 2))
        return D3D12_SHADING_RATE_1X2;
    if (coarsePixelSize == QSize(2, 1))
        return D3D12_SHADING_RATE_2X1;
    if (coarsePixelSize == QSize(2, 2))
        return D3D12_SHADING_RATE_2X2;
    if (coarsePixelSize == QSize(2, 4))
        return D3D12_SHADING_RATE_2X4;
    if (coarsePixelSize == QSize(4, 2))
        return D3D12_SHADING_RATE_4X2;
    if (coarsePixelSize == QSize(4, 4))
        return D3D12_SHADING_RATE_4X4;
    return D3D12_SHADING_RATE_1X1;
}

void QRhiD3D12::setShadingRate(QRhiCommandBuffer *cb, const QSize &coarsePixelSize)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    cbD->hasShadingRateSet = false;

#ifdef QRHI_D3D12_CL5_AVAILABLE
    if (!caps.vrs)
        return;

    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::RenderPass);
    const D3D12_SHADING_RATE_COMBINER combiners[] = { D3D12_SHADING_RATE_COMBINER_MAX, D3D12_SHADING_RATE_COMBINER_MAX };
    cbD->cmdList->RSSetShadingRate(toD3DShadingRate(coarsePixelSize), combiners);
    if (coarsePixelSize.width() != 1 || coarsePixelSize.height() != 1)
        cbD->hasShadingRateSet = true;
#else
    Q_UNUSED(cb);
    Q_UNUSED(coarsePixelSize);
    qWarning("Attempted to set ShadingRate without building Qt against a sufficiently new Windows SDK and d3d12.h. This cannot work.");
#endif
}

void QRhiD3D12::draw(QRhiCommandBuffer *cb, quint32 vertexCount,
                     quint32 instanceCount, quint32 firstVertex, quint32 firstInstance)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::RenderPass);
    cbD->cmdList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void QRhiD3D12::drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                            quint32 instanceCount, quint32 firstIndex, qint32 vertexOffset, quint32 firstInstance)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::RenderPass);
    cbD->cmdList->DrawIndexedInstanced(indexCount, instanceCount,
                                       firstIndex, vertexOffset,
                                       firstInstance);
}

void QRhiD3D12::debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name)
{
    if (!debugMarkers)
        return;

    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
#ifdef QRHI_D3D12_HAS_OLD_PIX
    PIXBeginEvent(cbD->cmdList, PIX_COLOR_DEFAULT, reinterpret_cast<LPCWSTR>(QString::fromLatin1(name).utf16()));
#else
    Q_UNUSED(cbD);
    Q_UNUSED(name);
#endif
}

void QRhiD3D12::debugMarkEnd(QRhiCommandBuffer *cb)
{
    if (!debugMarkers)
        return;

    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
#ifdef QRHI_D3D12_HAS_OLD_PIX
    PIXEndEvent(cbD->cmdList);
#else
    Q_UNUSED(cbD);
#endif
}

void QRhiD3D12::debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg)
{
    if (!debugMarkers)
        return;

    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
#ifdef QRHI_D3D12_HAS_OLD_PIX
    PIXSetMarker(cbD->cmdList, PIX_COLOR_DEFAULT, reinterpret_cast<LPCWSTR>(QString::fromLatin1(msg).utf16()));
#else
    Q_UNUSED(cbD);
    Q_UNUSED(msg);
#endif
}

const QRhiNativeHandles *QRhiD3D12::nativeHandles(QRhiCommandBuffer *cb)
{
    return QRHI_RES(QD3D12CommandBuffer, cb)->nativeHandles();
}

void QRhiD3D12::beginExternal(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}

void QRhiD3D12::endExternal(QRhiCommandBuffer *cb)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    cbD->resetPerPassState();
    bindShaderVisibleHeaps(cbD);
    if (cbD->currentTarget) { // could be compute, no rendertarget then
        QD3D12RenderTargetData *rtD = rtData(cbD->currentTarget);
        cbD->cmdList->OMSetRenderTargets(UINT(rtD->colorAttCount),
                                         rtD->rtv,
                                         TRUE,
                                         rtD->dsAttCount ? &rtD->dsv : nullptr);
    }
}

double QRhiD3D12::lastCompletedGpuTime(QRhiCommandBuffer *cb)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    return cbD->lastGpuTime;
}

static void calculateGpuTime(QD3D12CommandBuffer *cbD,
                             int timestampPairStartIndex,
                             const quint8 *readbackBufPtr,
                             quint64 timestampTicksPerSecond)
{
    const size_t byteOffset = timestampPairStartIndex * sizeof(quint64);
    const quint64 *p = reinterpret_cast<const quint64 *>(readbackBufPtr + byteOffset);
    const quint64 startTime = *p++;
    const quint64 endTime = *p;
    if (startTime < endTime) {
        const quint64 ticks = endTime - startTime;
        const double timeSec = ticks / double(timestampTicksPerSecond);
        cbD->lastGpuTime = timeSec;
    }
}

QRhi::FrameOpResult QRhiD3D12::beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);

    QD3D12SwapChain *swapChainD = QRHI_RES(QD3D12SwapChain, swapChain);
    currentSwapChain = swapChainD;
    currentFrameSlot = swapChainD->currentFrameSlot;
    QD3D12SwapChain::FrameResources &fr(swapChainD->frameRes[currentFrameSlot]);

    // We could do smarter things but mirror the Vulkan backend for now: Make
    // sure the previous commands for this same frame slot have finished. Do
    // this also for any other swapchain's commands with the same frame slot.
    // While this reduces concurrency in render-to-swapchain-A,
    // render-to-swapchain-B, repeat kind of scenarios, it keeps resource usage
    // safe: swapchain A starting its frame 0, followed by swapchain B starting
    // its own frame 0 will make B wait for A's frame 0 commands. If a resource
    // is written in B's frame or when B checks for pending resource releases,
    // that won't mess up A's in-flight commands (as they are guaranteed not to
    // be in flight anymore). With Qt Quick this situation cannot happen anyway
    // by design (one QRhi per window).
    for (QD3D12SwapChain *sc : std::as_const(swapchains))
        sc->waitCommandCompletionForFrameSlot(currentFrameSlot); // note: swapChainD->currentFrameSlot, not sc's

    if (swapChainD->frameLatencyWaitableObject) {
        // only wait when endFrame() called Present(), otherwise this would become a 1 sec timeout
        if (swapChainD->lastFrameLatencyWaitSlot != currentFrameSlot) {
            WaitForSingleObjectEx(swapChainD->frameLatencyWaitableObject, 1000, true);
            swapChainD->lastFrameLatencyWaitSlot = currentFrameSlot;
        }
    }

    HRESULT hr = cmdAllocators[currentFrameSlot]->Reset();
    if (FAILED(hr)) {
        qWarning("Failed to reset command allocator: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return QRhi::FrameOpError;
    }

    if (!startCommandListForCurrentFrameSlot(&fr.cmdList))
        return QRhi::FrameOpError;

    QD3D12CommandBuffer *cbD = &swapChainD->cbWrapper;
    cbD->cmdList = fr.cmdList;

    swapChainD->rtWrapper.d.rtv[0] = swapChainD->sampleDesc.Count > 1
            ? swapChainD->msaaRtvs[swapChainD->currentBackBufferIndex].cpuHandle
            : swapChainD->rtvs[swapChainD->currentBackBufferIndex].cpuHandle;

    swapChainD->rtWrapper.d.dsv = swapChainD->ds ? swapChainD->ds->dsv.cpuHandle
                                                 : D3D12_CPU_DESCRIPTOR_HANDLE { 0 };

    if (swapChainD->stereo) {
        swapChainD->rtWrapperRight.d.rtv[0] = swapChainD->sampleDesc.Count > 1
                ? swapChainD->msaaRtvs[swapChainD->currentBackBufferIndex].cpuHandle
                : swapChainD->rtvsRight[swapChainD->currentBackBufferIndex].cpuHandle;

        swapChainD->rtWrapperRight.d.dsv =
                swapChainD->ds ? swapChainD->ds->dsv.cpuHandle : D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    }


    // Time to release things that are marked for currentFrameSlot since due to
    // the wait above we know that the previous commands on the GPU for this
    // slot must have finished already.
    releaseQueue.executeDeferredReleases(currentFrameSlot);

    // Full reset of the command buffer data.
    cbD->resetState();

    // Move the head back to zero for the per-frame shader-visible descriptor heap work areas.
    shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[currentFrameSlot].head = 0;
    // Same for the small staging area.
    smallStagingAreas[currentFrameSlot].head = 0;

    bindShaderVisibleHeaps(cbD);

    finishActiveReadbacks(); // last, in case the readback-completed callback issues rhi calls

    if (timestampQueryHeap.isValid() && timestampTicksPerSecond) {
        // Read the timestamps for the previous frame for this slot. (the
        // ResolveQuery() should have completed by now due to the wait above)
        const int timestampPairStartIndex = currentFrameSlot * QD3D12_FRAMES_IN_FLIGHT;
        calculateGpuTime(cbD,
                         timestampPairStartIndex,
                         timestampReadbackArea.mem.p,
                         timestampTicksPerSecond);
        // Write the start timestamp for this frame for this slot.
        cbD->cmdList->EndQuery(timestampQueryHeap.heap,
                               D3D12_QUERY_TYPE_TIMESTAMP,
                               timestampPairStartIndex);
    }

    QDxgiVSyncService::instance()->beginFrame(adapterLuid);

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiD3D12::endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags)
{
    QD3D12SwapChain *swapChainD = QRHI_RES(QD3D12SwapChain, swapChain);
    Q_ASSERT(currentSwapChain == swapChainD);
    QD3D12CommandBuffer *cbD = &swapChainD->cbWrapper;

    QD3D12ObjectHandle backBufferResourceHandle = swapChainD->colorBuffers[swapChainD->currentBackBufferIndex];
    if (swapChainD->sampleDesc.Count > 1) {
        QD3D12ObjectHandle msaaBackBufferResourceHandle = swapChainD->msaaBuffers[swapChainD->currentBackBufferIndex];
        barrierGen.addTransitionBarrier(msaaBackBufferResourceHandle, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
        barrierGen.addTransitionBarrier(backBufferResourceHandle, D3D12_RESOURCE_STATE_RESOLVE_DEST);
        barrierGen.enqueueBufferedTransitionBarriers(cbD);
        const QD3D12Resource *src = resourcePool.lookupRef(msaaBackBufferResourceHandle);
        const QD3D12Resource *dst = resourcePool.lookupRef(backBufferResourceHandle);
        if (src && dst)
            cbD->cmdList->ResolveSubresource(dst->resource, 0, src->resource, 0, swapChainD->colorFormat);
    }

    barrierGen.addTransitionBarrier(backBufferResourceHandle, D3D12_RESOURCE_STATE_PRESENT);
    barrierGen.enqueueBufferedTransitionBarriers(cbD);

    if (timestampQueryHeap.isValid()) {
        const int timestampPairStartIndex = currentFrameSlot * QD3D12_FRAMES_IN_FLIGHT;
        cbD->cmdList->EndQuery(timestampQueryHeap.heap,
                               D3D12_QUERY_TYPE_TIMESTAMP,
                               timestampPairStartIndex + 1);
        cbD->cmdList->ResolveQueryData(timestampQueryHeap.heap,
                                       D3D12_QUERY_TYPE_TIMESTAMP,
                                       timestampPairStartIndex,
                                       2,
                                       timestampReadbackArea.mem.buffer,
                                       timestampPairStartIndex * sizeof(quint64));
    }

    D3D12GraphicsCommandList *cmdList = cbD->cmdList;
    HRESULT hr = cmdList->Close();
    if (FAILED(hr)) {
        qWarning("Failed to close command list: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return QRhi::FrameOpError;
    }

    ID3D12CommandList *execList[] = { cmdList };
    cmdQueue->ExecuteCommandLists(1, execList);

    if (!flags.testFlag(QRhi::SkipPresent)) {
        UINT presentFlags = 0;
        if (swapChainD->swapInterval == 0
                && (swapChainD->swapChainFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING))
        {
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }
        if (!swapChainD->swapChain) {
            qWarning("Failed to present, no swapchain");
            return QRhi::FrameOpError;
        }
        HRESULT hr = swapChainD->swapChain->Present(swapChainD->swapInterval, presentFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            qWarning("Device loss detected in Present()");
            deviceLost = true;
            return QRhi::FrameOpDeviceLost;
        } else if (FAILED(hr)) {
            qWarning("Failed to present: %s", qPrintable(QSystemError::windowsComString(hr)));
            return QRhi::FrameOpError;
        }

        if (dcompDevice && swapChainD->dcompTarget && swapChainD->dcompVisual)
            dcompDevice->Commit();
    }

    swapChainD->addCommandCompletionSignalForCurrentFrameSlot();

    // NB! The deferred-release mechanism here differs from the older QRhi
    // backends. There is no lastActiveFrameSlot tracking. Instead,
    // currentFrameSlot is written to the registered entries now, and so the
    // resources will get released in the frames_in_flight'th beginFrame()
    // counting starting from now.
    releaseQueue.activatePendingDeferredReleaseRequests(currentFrameSlot);

    if (!flags.testFlag(QRhi::SkipPresent)) {
        // Only move to the next slot if we presented. Otherwise will block and
        // wait for completion in the next beginFrame already, but SkipPresent
        // should be infrequent anyway.
        swapChainD->currentFrameSlot = (swapChainD->currentFrameSlot + 1) % QD3D12_FRAMES_IN_FLIGHT;
        swapChainD->currentBackBufferIndex = swapChainD->swapChain->GetCurrentBackBufferIndex();
    }

    currentSwapChain = nullptr;
    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiD3D12::beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);

    // Switch to the next slot manually. Swapchains do not know about this
    // which is good. So for example an onscreen, onscreen, offscreen,
    // onscreen, onscreen, onscreen sequence of frames leads to 0, 1, 0, 0, 1,
    // 0. (no strict alternation anymore) But this is not different from what
    // happens when multiple swapchains are involved. Offscreen frames are
    // synchronous anyway in the sense that they wait for execution to complete
    // in endOffscreenFrame, so no resources used in that frame are busy
    // anymore in the next frame.

    currentFrameSlot = (currentFrameSlot + 1) % QD3D12_FRAMES_IN_FLIGHT;

    for (QD3D12SwapChain *sc : std::as_const(swapchains))
        sc->waitCommandCompletionForFrameSlot(currentFrameSlot); // note: not sc's currentFrameSlot

    HRESULT hr = cmdAllocators[currentFrameSlot]->Reset();
    if (FAILED(hr)) {
        qWarning("Failed to reset command allocator: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return QRhi::FrameOpError;
    }

    if (!offscreenCb[currentFrameSlot])
        offscreenCb[currentFrameSlot] = new QD3D12CommandBuffer(this);
    QD3D12CommandBuffer *cbD = offscreenCb[currentFrameSlot];
    if (!startCommandListForCurrentFrameSlot(&cbD->cmdList))
        return QRhi::FrameOpError;

    releaseQueue.executeDeferredReleases(currentFrameSlot);
    cbD->resetState();
    shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[currentFrameSlot].head = 0;
    smallStagingAreas[currentFrameSlot].head = 0;

    bindShaderVisibleHeaps(cbD);

    if (timestampQueryHeap.isValid() && timestampTicksPerSecond) {
        cbD->cmdList->EndQuery(timestampQueryHeap.heap,
                               D3D12_QUERY_TYPE_TIMESTAMP,
                               currentFrameSlot * QD3D12_FRAMES_IN_FLIGHT);
    }

    offscreenActive = true;
    *cb = cbD;

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiD3D12::endOffscreenFrame(QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    Q_ASSERT(offscreenActive);
    offscreenActive = false;

    QD3D12CommandBuffer *cbD = offscreenCb[currentFrameSlot];
    if (timestampQueryHeap.isValid()) {
        const int timestampPairStartIndex = currentFrameSlot * QD3D12_FRAMES_IN_FLIGHT;
        cbD->cmdList->EndQuery(timestampQueryHeap.heap,
                               D3D12_QUERY_TYPE_TIMESTAMP,
                               timestampPairStartIndex + 1);
        cbD->cmdList->ResolveQueryData(timestampQueryHeap.heap,
                                       D3D12_QUERY_TYPE_TIMESTAMP,
                                       timestampPairStartIndex,
                                       2,
                                       timestampReadbackArea.mem.buffer,
                                       timestampPairStartIndex * sizeof(quint64));
    }

    D3D12GraphicsCommandList *cmdList = cbD->cmdList;
    HRESULT hr = cmdList->Close();
    if (FAILED(hr)) {
        qWarning("Failed to close command list: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return QRhi::FrameOpError;
    }

    ID3D12CommandList *execList[] = { cmdList };
    cmdQueue->ExecuteCommandLists(1, execList);

    releaseQueue.activatePendingDeferredReleaseRequests(currentFrameSlot);

    // wait for completion
    waitGpu();

    // Here we know that executing the host-side reads for this (or any
    // previous) frame is safe since we waited for completion above.
    finishActiveReadbacks(true);

    // the timestamp query results should be available too, given the wait
    if (timestampQueryHeap.isValid()) {
        calculateGpuTime(cbD,
                         currentFrameSlot * QD3D12_FRAMES_IN_FLIGHT,
                         timestampReadbackArea.mem.p,
                         timestampTicksPerSecond);
    }

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiD3D12::finish()
{
    QD3D12CommandBuffer *cbD = nullptr;
    if (inFrame) {
        if (offscreenActive) {
            Q_ASSERT(!currentSwapChain);
            cbD = offscreenCb[currentFrameSlot];
        } else {
            Q_ASSERT(currentSwapChain);
            cbD = &currentSwapChain->cbWrapper;
        }
        if (!cbD)
            return QRhi::FrameOpError;

        Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::NoPass);

        D3D12GraphicsCommandList *cmdList = cbD->cmdList;
        HRESULT hr = cmdList->Close();
        if (FAILED(hr)) {
            qWarning("Failed to close command list: %s",
                    qPrintable(QSystemError::windowsComString(hr)));
            return QRhi::FrameOpError;
        }

        ID3D12CommandList *execList[] = { cmdList };
        cmdQueue->ExecuteCommandLists(1, execList);

        releaseQueue.activatePendingDeferredReleaseRequests(currentFrameSlot);
    }

    // full blocking wait for everything, frame slots do not matter now
    waitGpu();

    if (inFrame) {
        HRESULT hr = cmdAllocators[currentFrameSlot]->Reset();
        if (FAILED(hr)) {
            qWarning("Failed to reset command allocator: %s",
                    qPrintable(QSystemError::windowsComString(hr)));
            return QRhi::FrameOpError;
        }

        if (!startCommandListForCurrentFrameSlot(&cbD->cmdList))
            return QRhi::FrameOpError;

        cbD->resetState();

        shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[currentFrameSlot].head = 0;
        smallStagingAreas[currentFrameSlot].head = 0;

        bindShaderVisibleHeaps(cbD);
    }

    releaseQueue.releaseAll();
    finishActiveReadbacks(true);

    return QRhi::FrameOpSuccess;
}

void QRhiD3D12::resourceUpdate(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::NoPass);
    enqueueResourceUpdates(cbD, resourceUpdates);
}

void QRhiD3D12::beginPass(QRhiCommandBuffer *cb,
                          QRhiRenderTarget *rt,
                          const QColor &colorClearValue,
                          const QRhiDepthStencilClearValue &depthStencilClearValue,
                          QRhiResourceUpdateBatch *resourceUpdates,
                          QRhiCommandBuffer::BeginPassFlags)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cbD, resourceUpdates);

    QD3D12RenderTargetData *rtD = rtData(rt);
    bool wantsColorClear = true;
    bool wantsDsClear = true;
    if (rt->resourceType() == QRhiRenderTarget::TextureRenderTarget) {
        QD3D12TextureRenderTarget *rtTex = QRHI_RES(QD3D12TextureRenderTarget, rt);
        wantsColorClear = !rtTex->m_flags.testFlag(QRhiTextureRenderTarget::PreserveColorContents);
        wantsDsClear = !rtTex->m_flags.testFlag(QRhiTextureRenderTarget::PreserveDepthStencilContents);
        if (!QRhiRenderTargetAttachmentTracker::isUpToDate<QD3D12Texture, QD3D12RenderBuffer>(rtTex->description(), rtD->currentResIdList))
            rtTex->create();

        for (auto it = rtTex->m_desc.cbeginColorAttachments(), itEnd = rtTex->m_desc.cendColorAttachments(); it != itEnd; ++it) {
            QD3D12Texture *texD = QRHI_RES(QD3D12Texture, it->texture());
            QD3D12Texture *resolveTexD = QRHI_RES(QD3D12Texture, it->resolveTexture());
            QD3D12RenderBuffer *rbD = QRHI_RES(QD3D12RenderBuffer, it->renderBuffer());
            if (texD)
                barrierGen.addTransitionBarrier(texD->handle, D3D12_RESOURCE_STATE_RENDER_TARGET);
            else if (rbD)
                barrierGen.addTransitionBarrier(rbD->handle, D3D12_RESOURCE_STATE_RENDER_TARGET);
            if (resolveTexD)
                barrierGen.addTransitionBarrier(resolveTexD->handle, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
        if (rtTex->m_desc.depthStencilBuffer()) {
            QD3D12RenderBuffer *rbD = QRHI_RES(QD3D12RenderBuffer, rtTex->m_desc.depthStencilBuffer());
            Q_ASSERT(rbD->m_type == QRhiRenderBuffer::DepthStencil);
            barrierGen.addTransitionBarrier(rbD->handle, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        } else if (rtTex->m_desc.depthTexture()) {
            QD3D12Texture *depthTexD = QRHI_RES(QD3D12Texture, rtTex->m_desc.depthTexture());
            barrierGen.addTransitionBarrier(depthTexD->handle, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        }
        barrierGen.enqueueBufferedTransitionBarriers(cbD);
    } else {
        Q_ASSERT(currentSwapChain);
        barrierGen.addTransitionBarrier(currentSwapChain->sampleDesc.Count > 1
                                        ? currentSwapChain->msaaBuffers[currentSwapChain->currentBackBufferIndex]
                                        : currentSwapChain->colorBuffers[currentSwapChain->currentBackBufferIndex],
                                        D3D12_RESOURCE_STATE_RENDER_TARGET);
        barrierGen.enqueueBufferedTransitionBarriers(cbD);
    }

    cbD->cmdList->OMSetRenderTargets(UINT(rtD->colorAttCount),
                                     rtD->rtv,
                                     TRUE,
                                     rtD->dsAttCount ? &rtD->dsv : nullptr);

    if (rtD->colorAttCount && wantsColorClear) {
        float clearColor[4] = {
            colorClearValue.redF(),
            colorClearValue.greenF(),
            colorClearValue.blueF(),
            colorClearValue.alphaF()
        };
        for (int i = 0; i < rtD->colorAttCount; ++i)
            cbD->cmdList->ClearRenderTargetView(rtD->rtv[i], clearColor, 0, nullptr);
    }
    if (rtD->dsAttCount && wantsDsClear) {
        cbD->cmdList->ClearDepthStencilView(rtD->dsv,
                                            D3D12_CLEAR_FLAGS(D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL),
                                            depthStencilClearValue.depthClearValue(),
                                            UINT8(depthStencilClearValue.stencilClearValue()),
                                            0,
                                            nullptr);
    }

    cbD->recordingPass = QD3D12CommandBuffer::RenderPass;
    cbD->currentTarget = rt;

    bool hasShadingRateMapSet = false;
#ifdef QRHI_D3D12_CL5_AVAILABLE
    if (rtD->rp->hasShadingRateMap) {
        cbD->setShadingRate(QSize(1, 1));
        QD3D12ShadingRateMap *rateMapD = rt->resourceType() == QRhiRenderTarget::TextureRenderTarget
            ? QRHI_RES(QD3D12ShadingRateMap, QRHI_RES(QD3D12TextureRenderTarget, rt)->m_desc.shadingRateMap())
            : QRHI_RES(QD3D12ShadingRateMap, QRHI_RES(QD3D12SwapChainRenderTarget, rt)->swapChain()->shadingRateMap());
        if (QD3D12Resource *res = resourcePool.lookupRef(rateMapD->handle)) {
            barrierGen.addTransitionBarrier(rateMapD->handle, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
            barrierGen.enqueueBufferedTransitionBarriers(cbD);
            cbD->cmdList->RSSetShadingRateImage(res->resource);
            hasShadingRateMapSet = true;
        }
    } else if (cbD->hasShadingRateMapSet) {
        cbD->cmdList->RSSetShadingRateImage(nullptr);
        cbD->setShadingRate(QSize(1, 1));
    } else if (cbD->hasShadingRateSet) {
        cbD->setShadingRate(QSize(1, 1));
    }
#endif

    cbD->resetPerPassState();

    // shading rate tracking is reset in resetPerPassState(), sync what we did just above
    cbD->hasShadingRateMapSet = hasShadingRateMapSet;
}

void QRhiD3D12::endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::RenderPass);

    if (cbD->currentTarget->resourceType() == QRhiResource::TextureRenderTarget) {
        QD3D12TextureRenderTarget *rtTex = QRHI_RES(QD3D12TextureRenderTarget, cbD->currentTarget);
        for (auto it = rtTex->m_desc.cbeginColorAttachments(), itEnd = rtTex->m_desc.cendColorAttachments();
             it != itEnd; ++it)
        {
            const QRhiColorAttachment &colorAtt(*it);
            if (!colorAtt.resolveTexture())
                continue;

            QD3D12Texture *dstTexD = QRHI_RES(QD3D12Texture, colorAtt.resolveTexture());
            QD3D12Resource *dstRes = resourcePool.lookupRef(dstTexD->handle);
            if (!dstRes)
                continue;

            QD3D12Texture *srcTexD = QRHI_RES(QD3D12Texture, colorAtt.texture());
            QD3D12RenderBuffer *srcRbD = QRHI_RES(QD3D12RenderBuffer, colorAtt.renderBuffer());
            Q_ASSERT(srcTexD || srcRbD);
            QD3D12Resource *srcRes = resourcePool.lookupRef(srcTexD ? srcTexD->handle : srcRbD->handle);
            if (!srcRes)
                continue;

            if (srcTexD) {
                if (srcTexD->dxgiFormat != dstTexD->dxgiFormat) {
                    qWarning("Resolve source (%d) and destination (%d) formats do not match",
                             int(srcTexD->dxgiFormat), int(dstTexD->dxgiFormat));
                    continue;
                }
                if (srcTexD->sampleDesc.Count <= 1) {
                    qWarning("Cannot resolve a non-multisample texture");
                    continue;
                }
                if (srcTexD->m_pixelSize != dstTexD->m_pixelSize) {
                    qWarning("Resolve source and destination sizes do not match");
                    continue;
                }
            } else {
                if (srcRbD->dxgiFormat != dstTexD->dxgiFormat) {
                    qWarning("Resolve source (%d) and destination (%d) formats do not match",
                             int(srcRbD->dxgiFormat), int(dstTexD->dxgiFormat));
                    continue;
                }
                if (srcRbD->m_pixelSize != dstTexD->m_pixelSize) {
                    qWarning("Resolve source and destination sizes do not match");
                    continue;
                }
            }

            barrierGen.addTransitionBarrier(srcTexD ? srcTexD->handle : srcRbD->handle, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            barrierGen.addTransitionBarrier(dstTexD->handle, D3D12_RESOURCE_STATE_RESOLVE_DEST);
            barrierGen.enqueueBufferedTransitionBarriers(cbD);

            const UINT resolveCount = colorAtt.multiViewCount() >= 2 ? colorAtt.multiViewCount() : 1;
            for (UINT resolveIdx = 0; resolveIdx < resolveCount; ++resolveIdx) {
                const UINT srcSubresource = calcSubresource(0, UINT(colorAtt.layer()) + resolveIdx, 1);
                const UINT dstSubresource = calcSubresource(UINT(colorAtt.resolveLevel()),
                                                            UINT(colorAtt.resolveLayer()) + resolveIdx,
                                                            dstTexD->mipLevelCount);
                cbD->cmdList->ResolveSubresource(dstRes->resource, dstSubresource,
                                                 srcRes->resource, srcSubresource,
                                                 dstTexD->dxgiFormat);
            }
        }
        if (rtTex->m_desc.depthResolveTexture())
            qWarning("Resolving multisample depth-stencil buffers is not supported with D3D");
    }

    cbD->recordingPass = QD3D12CommandBuffer::NoPass;
    cbD->currentTarget = nullptr;

    if (resourceUpdates)
        enqueueResourceUpdates(cbD, resourceUpdates);
}

void QRhiD3D12::beginComputePass(QRhiCommandBuffer *cb,
                                 QRhiResourceUpdateBatch *resourceUpdates,
                                 QRhiCommandBuffer::BeginPassFlags)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cbD, resourceUpdates);

    cbD->recordingPass = QD3D12CommandBuffer::ComputePass;

    cbD->resetPerPassState();
}

void QRhiD3D12::endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::ComputePass);

    cbD->recordingPass = QD3D12CommandBuffer::NoPass;

    if (resourceUpdates)
        enqueueResourceUpdates(cbD, resourceUpdates);
}

void QRhiD3D12::setComputePipeline(QRhiCommandBuffer *cb, QRhiComputePipeline *ps)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::ComputePass);
    QD3D12ComputePipeline *psD = QRHI_RES(QD3D12ComputePipeline, ps);
    const bool pipelineChanged = cbD->currentComputePipeline != psD || cbD->currentPipelineGeneration != psD->generation;

    if (pipelineChanged) {
        cbD->currentGraphicsPipeline = nullptr;
        cbD->currentComputePipeline = psD;
        cbD->currentPipelineGeneration = psD->generation;

        if (QD3D12Pipeline *pipeline = pipelinePool.lookupRef(psD->handle)) {
            Q_ASSERT(pipeline->type == QD3D12Pipeline::Compute);
            cbD->cmdList->SetPipelineState(pipeline->pso);
            if (QD3D12RootSignature *rs = rootSignaturePool.lookupRef(psD->rootSigHandle))
                cbD->cmdList->SetComputeRootSignature(rs->rootSig);
        }
    }
}

void QRhiD3D12::dispatch(QRhiCommandBuffer *cb, int x, int y, int z)
{
    QD3D12CommandBuffer *cbD = QRHI_RES(QD3D12CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D12CommandBuffer::ComputePass);
    cbD->cmdList->Dispatch(UINT(x), UINT(y), UINT(z));
}

bool QD3D12DescriptorHeap::create(ID3D12Device *device,
                                  quint32 descriptorCount,
                                  D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                  D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags)
{
    head = 0;
    capacity = descriptorCount;
    this->heapType = heapType;
    this->heapFlags = heapFlags;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = heapType;
    heapDesc.NumDescriptors = capacity;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS(heapFlags);

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void **>(&heap));
    if (FAILED(hr)) {
        qWarning("Failed to create descriptor heap: %s", qPrintable(QSystemError::windowsComString(hr)));
        heap = nullptr;
        capacity = descriptorByteSize = 0;
        return false;
    }

    descriptorByteSize = device->GetDescriptorHandleIncrementSize(heapType);
    heapStart.cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
    if (heapFlags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
        heapStart.gpuHandle = heap->GetGPUDescriptorHandleForHeapStart();

    return true;
}

void QD3D12DescriptorHeap::createWithExisting(const QD3D12DescriptorHeap &other,
                                              quint32 offsetInDescriptors,
                                              quint32 descriptorCount)
{
    heap = nullptr;
    head = 0;
    capacity = descriptorCount;
    heapType = other.heapType;
    heapFlags = other.heapFlags;
    descriptorByteSize = other.descriptorByteSize;
    heapStart = incremented(other.heapStart, offsetInDescriptors);
}

void QD3D12DescriptorHeap::destroy()
{
    if (heap) {
        heap->Release();
        heap = nullptr;
    }
    capacity = 0;
}

void QD3D12DescriptorHeap::destroyWithDeferredRelease(QD3D12ReleaseQueue *releaseQueue)
{
    if (heap) {
        releaseQueue->deferredReleaseDescriptorHeap(heap);
        heap = nullptr;
    }
    capacity = 0;
}

QD3D12Descriptor QD3D12DescriptorHeap::get(quint32 count)
{
    Q_ASSERT(count > 0);
    if (head + count > capacity) {
        qWarning("Cannot get %u descriptors as that would exceed capacity %u", count, capacity);
        return {};
    }
    head += count;
    return at(head - count);
}

QD3D12Descriptor QD3D12DescriptorHeap::at(quint32 index) const
{
    const quint32 startOffset = index * descriptorByteSize;
    QD3D12Descriptor result;
    result.cpuHandle.ptr = heapStart.cpuHandle.ptr + startOffset;
    if (heapStart.gpuHandle.ptr != 0)
        result.gpuHandle.ptr = heapStart.gpuHandle.ptr + startOffset;
    return result;
}

bool QD3D12CpuDescriptorPool::create(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, const char *debugName)
{
    QD3D12DescriptorHeap firstHeap;
    if (!firstHeap.create(device, DESCRIPTORS_PER_HEAP, heapType, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
        return false;
    heaps.append(HeapWithMap::init(firstHeap, DESCRIPTORS_PER_HEAP));
    descriptorByteSize = heaps[0].heap.descriptorByteSize;
    this->device = device;
    this->debugName = debugName;
    return true;
}

void QD3D12CpuDescriptorPool::destroy()
{
#ifndef QT_NO_DEBUG
    // debug builds: just do it always
    static bool leakCheck = true;
#else
    // release builds: opt-in
    static bool leakCheck = qEnvironmentVariableIntValue("QT_RHI_LEAK_CHECK");
#endif
    if (leakCheck) {
        for (HeapWithMap &heap : heaps) {
            const int leakedDescriptorCount = heap.map.count(true);
            if (leakedDescriptorCount > 0) {
                qWarning("QD3D12CpuDescriptorPool::destroy(): "
                         "Heap %p for descriptor pool %p '%s' has %d unreleased descriptors",
                         &heap.heap, this, debugName, leakedDescriptorCount);
            }
        }
    }
    for (HeapWithMap &heap : heaps)
        heap.heap.destroy();
    heaps.clear();
}

QD3D12Descriptor QD3D12CpuDescriptorPool::allocate(quint32 count)
{
    Q_ASSERT(count > 0 && count <= DESCRIPTORS_PER_HEAP);

    HeapWithMap &last(heaps.last());
    if (last.heap.head + count <= last.heap.capacity) {
        quint32 firstIndex = last.heap.head;
        for (quint32 i = 0; i < count; ++i)
            last.map.setBit(firstIndex + i);
        return last.heap.get(count);
    }

    for (HeapWithMap &heap : heaps) {
        quint32 freeCount = 0;
        for (quint32 i = 0; i < DESCRIPTORS_PER_HEAP; ++i) {
            if (heap.map.testBit(i)) {
                freeCount = 0;
            } else {
                freeCount += 1;
                if (freeCount == count) {
                    quint32 firstIndex = i - (freeCount - 1);
                    for (quint32 j = 0; j < count; ++j) {
                        heap.map.setBit(firstIndex + j);
                        return heap.heap.at(firstIndex);
                    }
                }
            }
        }
    }

    QD3D12DescriptorHeap newHeap;
    if (!newHeap.create(device, DESCRIPTORS_PER_HEAP, last.heap.heapType, last.heap.heapFlags))
        return {};

    heaps.append(HeapWithMap::init(newHeap, DESCRIPTORS_PER_HEAP));

    for (quint32 i = 0; i < count; ++i)
        heaps.last().map.setBit(i);

    return heaps.last().heap.get(count);
}

void QD3D12CpuDescriptorPool::release(const QD3D12Descriptor &descriptor, quint32 count)
{
    Q_ASSERT(count > 0 && count <= DESCRIPTORS_PER_HEAP);
    if (!descriptor.isValid())
        return;

    const SIZE_T addr = descriptor.cpuHandle.ptr;
    for (HeapWithMap &heap : heaps) {
        const SIZE_T begin = heap.heap.heapStart.cpuHandle.ptr;
        const SIZE_T end = begin + heap.heap.descriptorByteSize * heap.heap.capacity;
        if (addr >= begin && addr < end) {
            quint32 firstIndex = (addr - begin) / heap.heap.descriptorByteSize;
            for (quint32 i = 0; i < count; ++i)
                heap.map.setBit(firstIndex + i, false);
            return;
        }
    }

    qWarning("QD3D12CpuDescriptorPool::release: Descriptor with address %llu is not in any heap",
             quint64(descriptor.cpuHandle.ptr));
}

bool QD3D12QueryHeap::create(ID3D12Device *device,
                             quint32 queryCount,
                             D3D12_QUERY_HEAP_TYPE heapType)
{
    capacity = queryCount;

    D3D12_QUERY_HEAP_DESC heapDesc = {};
    heapDesc.Type = heapType;
    heapDesc.Count = capacity;

    HRESULT hr = device->CreateQueryHeap(&heapDesc, __uuidof(ID3D12QueryHeap), reinterpret_cast<void **>(&heap));
    if (FAILED(hr)) {
        qWarning("Failed to create query heap: %s", qPrintable(QSystemError::windowsComString(hr)));
        heap = nullptr;
        capacity = 0;
        return false;
    }

    return true;
}

void QD3D12QueryHeap::destroy()
{
    if (heap) {
        heap->Release();
        heap = nullptr;
    }
    capacity = 0;
}

bool QD3D12StagingArea::create(QRhiD3D12 *rhi, quint32 capacity, D3D12_HEAP_TYPE heapType)
{
    Q_ASSERT(heapType == D3D12_HEAP_TYPE_UPLOAD || heapType == D3D12_HEAP_TYPE_READBACK);
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = capacity;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc = { 1, 0 };
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    UINT state = heapType == D3D12_HEAP_TYPE_UPLOAD ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;
    HRESULT hr = rhi->vma.createResource(heapType,
                                         &resourceDesc,
                                         D3D12_RESOURCE_STATES(state),
                                         nullptr,
                                         &allocation,
                                         __uuidof(ID3D12Resource),
                                         reinterpret_cast<void **>(&resource));
    if (FAILED(hr)) {
        qWarning("Failed to create buffer for staging area: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }
    void *p = nullptr;
    hr = resource->Map(0, nullptr, &p);
    if (FAILED(hr)) {
        qWarning("Failed to map buffer for staging area: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        destroy();
        return false;
    }

    mem.p = static_cast<quint8 *>(p);
    mem.gpuAddr = resource->GetGPUVirtualAddress();
    mem.buffer = resource;
    mem.bufferOffset = 0;

    this->capacity = capacity;
    head = 0;

    return true;
}

void QD3D12StagingArea::destroy()
{
    if (resource) {
        resource->Release();
        resource = nullptr;
    }
    if (allocation) {
        allocation->Release();
        allocation = nullptr;
    }
    mem = {};
}

void QD3D12StagingArea::destroyWithDeferredRelease(QD3D12ReleaseQueue *releaseQueue)
{
    if (resource)
        releaseQueue->deferredReleaseResourceAndAllocation(resource, allocation);
    mem = {};
}

QD3D12StagingArea::Allocation QD3D12StagingArea::get(quint32 byteSize)
{
    const quint32 allocSize = aligned(byteSize, ALIGNMENT);
    if (head + allocSize > capacity) {
        qWarning("Failed to allocate %u (%u) bytes from staging area of size %u with %u bytes left",
                 allocSize, byteSize, capacity, remainingCapacity());
        return {};
    }
    const quint32 offset = head;
    head += allocSize;
    return {
        mem.p + offset,
        mem.gpuAddr + offset,
        mem.buffer,
        offset
    };
}

// Can be called inside and outside of begin-endFrame. Removes from the pool
// and releases the underlying native resource only in the frames_in_flight'th
// beginFrame() counted starting from the next endFrame().
void QD3D12ReleaseQueue::deferredReleaseResource(const QD3D12ObjectHandle &handle)
{
    DeferredReleaseEntry e;
    e.handle = handle;
    queue.append(e);
}

void QD3D12ReleaseQueue::deferredReleaseResourceWithViews(const QD3D12ObjectHandle &handle,
                                                          QD3D12CpuDescriptorPool *pool,
                                                          const QD3D12Descriptor &viewsStart,
                                                          int viewCount)
{
    DeferredReleaseEntry e;
    e.type = DeferredReleaseEntry::Resource;
    e.handle = handle;
    e.poolForViews = pool;
    e.viewsStart = viewsStart;
    e.viewCount = viewCount;
    queue.append(e);
}

void QD3D12ReleaseQueue::deferredReleasePipeline(const QD3D12ObjectHandle &handle)
{
    DeferredReleaseEntry e;
    e.type = DeferredReleaseEntry::Pipeline;
    e.handle = handle;
    queue.append(e);
}

void QD3D12ReleaseQueue::deferredReleaseRootSignature(const QD3D12ObjectHandle &handle)
{
    DeferredReleaseEntry e;
    e.type = DeferredReleaseEntry::RootSignature;
    e.handle = handle;
    queue.append(e);
}

void QD3D12ReleaseQueue::deferredReleaseCallback(std::function<void(void*)> callback, void *userData)
{
    DeferredReleaseEntry e;
    e.type = DeferredReleaseEntry::Callback;
    e.callback = callback;
    e.callbackUserData = userData;
    queue.append(e);
}

void QD3D12ReleaseQueue::deferredReleaseResourceAndAllocation(ID3D12Resource *resource,
                                                              D3D12MA::Allocation *allocation)
{
    DeferredReleaseEntry e;
    e.type = DeferredReleaseEntry::ResourceAndAllocation;
    e.resourceAndAllocation = { resource, allocation };
    queue.append(e);
}

void QD3D12ReleaseQueue::deferredReleaseDescriptorHeap(ID3D12DescriptorHeap *heap)
{
    DeferredReleaseEntry e;
    e.type = DeferredReleaseEntry::DescriptorHeap;
    e.descriptorHeap = heap;
    queue.append(e);
}

void QD3D12ReleaseQueue::deferredReleaseViews(QD3D12CpuDescriptorPool *pool,
                                              const QD3D12Descriptor &viewsStart,
                                              int viewCount)
{
    DeferredReleaseEntry e;
    e.type = DeferredReleaseEntry::Views;
    e.poolForViews = pool;
    e.viewsStart = viewsStart;
    e.viewCount = viewCount;
    queue.append(e);
}

void QD3D12ReleaseQueue::activatePendingDeferredReleaseRequests(int frameSlot)
{
    for (DeferredReleaseEntry &e : queue) {
        if (!e.frameSlotToBeReleasedIn.has_value())
            e.frameSlotToBeReleasedIn = frameSlot;
    }
}

void QD3D12ReleaseQueue::executeDeferredReleases(int frameSlot, bool forced)
{
    for (int i = queue.count() - 1; i >= 0; --i) {
        const DeferredReleaseEntry &e(queue[i]);
        if (forced || (e.frameSlotToBeReleasedIn.has_value() && e.frameSlotToBeReleasedIn.value() == frameSlot)) {
            switch (e.type) {
            case DeferredReleaseEntry::Resource:
                resourcePool->remove(e.handle);
                if (e.poolForViews && e.viewsStart.isValid() && e.viewCount > 0)
                    e.poolForViews->release(e.viewsStart, e.viewCount);
                break;
            case DeferredReleaseEntry::Pipeline:
                pipelinePool->remove(e.handle);
                break;
            case DeferredReleaseEntry::RootSignature:
                rootSignaturePool->remove(e.handle);
                break;
            case DeferredReleaseEntry::Callback:
                e.callback(e.callbackUserData);
                break;
            case DeferredReleaseEntry::ResourceAndAllocation:
                // order matters: resource first, then the allocation (which
                // may be null)
                e.resourceAndAllocation.first->Release();
                if (e.resourceAndAllocation.second)
                    e.resourceAndAllocation.second->Release();
                break;
            case DeferredReleaseEntry::DescriptorHeap:
                e.descriptorHeap->Release();
                break;
            case DeferredReleaseEntry::Views:
                e.poolForViews->release(e.viewsStart, e.viewCount);
                break;
            }
            queue.removeAt(i);
        }
    }
}

void QD3D12ReleaseQueue::releaseAll()
{
    executeDeferredReleases(0, true);
}

void QD3D12ResourceBarrierGenerator::addTransitionBarrier(const QD3D12ObjectHandle &resourceHandle,
                                                          D3D12_RESOURCE_STATES stateAfter)
{
    if (QD3D12Resource *res = resourcePool->lookupRef(resourceHandle)) {
        if (stateAfter != res->state) {
            transitionResourceBarriers.append({ resourceHandle, res->state, stateAfter });
            res->state = stateAfter;
        }
    }
}

void QD3D12ResourceBarrierGenerator::enqueueBufferedTransitionBarriers(QD3D12CommandBuffer *cbD)
{
    QVarLengthArray<D3D12_RESOURCE_BARRIER, PREALLOC> barriers;
    for (const TransitionResourceBarrier &trb : transitionResourceBarriers) {
        if (QD3D12Resource *res = resourcePool->lookupRef(trb.resourceHandle)) {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = res->resource;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = trb.stateBefore;
            barrier.Transition.StateAfter = trb.stateAfter;
            barriers.append(barrier);
        }
    }
    transitionResourceBarriers.clear();
    if (!barriers.isEmpty())
        cbD->cmdList->ResourceBarrier(barriers.count(), barriers.constData());
}

void QD3D12ResourceBarrierGenerator::enqueueSubresourceTransitionBarrier(QD3D12CommandBuffer *cbD,
                                                                         const QD3D12ObjectHandle &resourceHandle,
                                                                         UINT subresource,
                                                                         D3D12_RESOURCE_STATES stateBefore,
                                                                         D3D12_RESOURCE_STATES stateAfter)
{
    if (QD3D12Resource *res = resourcePool->lookupRef(resourceHandle)) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = res->resource;
        barrier.Transition.Subresource = subresource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        cbD->cmdList->ResourceBarrier(1, &barrier);
    }
}

void QD3D12ResourceBarrierGenerator::enqueueUavBarrier(QD3D12CommandBuffer *cbD,
                                                       const QD3D12ObjectHandle &resourceHandle)
{
    if (QD3D12Resource *res = resourcePool->lookupRef(resourceHandle)) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = res->resource;
        cbD->cmdList->ResourceBarrier(1, &barrier);
    }
}

void QD3D12ShaderBytecodeCache::insertWithCapacityLimit(const QRhiShaderStage &key, const Shader &s)
{
    if (data.count() >= QRhiD3D12::MAX_SHADER_CACHE_ENTRIES)
        data.clear();
    data.insert(key, s);
}

bool QD3D12ShaderVisibleDescriptorHeap::create(ID3D12Device *device,
                                               D3D12_DESCRIPTOR_HEAP_TYPE type,
                                               quint32 perFrameDescriptorCount)
{
    Q_ASSERT(type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    quint32 size = perFrameDescriptorCount * QD3D12_FRAMES_IN_FLIGHT;

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
    const quint32 CBV_SRV_UAV_MAX = 1000000;
    const quint32 SAMPLER_MAX = 2048;
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        size = qMin(size, CBV_SRV_UAV_MAX);
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
        size = qMin(size, SAMPLER_MAX);

    if (!heap.create(device, size, type, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)) {
        qWarning("Failed to create shader-visible descriptor heap of size %u", size);
        return false;
    }

    perFrameDescriptorCount = size / QD3D12_FRAMES_IN_FLIGHT;
    quint32 currentOffsetInDescriptors = 0;
    for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
        perFrameHeapSlice[i].createWithExisting(heap, currentOffsetInDescriptors, perFrameDescriptorCount);
        currentOffsetInDescriptors += perFrameDescriptorCount;
    }

    return true;
}

void QD3D12ShaderVisibleDescriptorHeap::destroy()
{
    heap.destroy();
}

void QD3D12ShaderVisibleDescriptorHeap::destroyWithDeferredRelease(QD3D12ReleaseQueue *releaseQueue)
{
    heap.destroyWithDeferredRelease(releaseQueue);
}

static inline std::pair<int, int> mapBinding(int binding, const QShader::NativeResourceBindingMap &map)
{
    if (map.isEmpty())
        return { binding, binding }; // assume 1:1 mapping

    auto it = map.constFind(binding);
    if (it != map.cend())
        return *it;

    // Hitting this path is normal too. It is not given that the resource is
    // present in the shaders for all the stages specified by the visibility
    // mask in the QRhiShaderResourceBinding.
    return { -1, -1 };
}

void QD3D12ShaderResourceVisitor::visit()
{
    for (int bindingIdx = 0, bindingCount = srb->m_bindings.count(); bindingIdx != bindingCount; ++bindingIdx) {
        const QRhiShaderResourceBinding &b(srb->m_bindings[bindingIdx]);
        const QRhiShaderResourceBinding::Data *bd = QRhiImplementation::shaderResourceBindingData(b);

        for (int stageIdx = 0; stageIdx < stageCount; ++stageIdx) {
            const QD3D12ShaderStageData *sd = &stageData[stageIdx];
            if (!sd->valid)
                continue;

            if (!bd->stage.testFlag(qd3d12_stageToSrb(sd->stage)))
                continue;

            switch (bd->type) {
            case QRhiShaderResourceBinding::UniformBuffer:
            {
                const int shaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).first;
                if (shaderRegister >= 0 && uniformBuffer)
                    uniformBuffer(sd->stage, bd->u.ubuf, shaderRegister, bd->binding);
            }
                break;
            case QRhiShaderResourceBinding::SampledTexture:
            {
                Q_ASSERT(bd->u.stex.count > 0);
                const int textureBaseShaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).first;
                const int samplerBaseShaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).second;
                for (int i = 0; i < bd->u.stex.count; ++i) {
                    if (textureBaseShaderRegister >= 0 && texture)
                        texture(sd->stage, bd->u.stex.texSamplers[i], textureBaseShaderRegister + i);
                    if (samplerBaseShaderRegister >= 0 && sampler)
                        sampler(sd->stage, bd->u.stex.texSamplers[i], samplerBaseShaderRegister + i);
                }
            }
                break;
            case QRhiShaderResourceBinding::Texture:
            {
                Q_ASSERT(bd->u.stex.count > 0);
                const int baseShaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).first;
                if (baseShaderRegister >= 0 && texture) {
                    for (int i = 0; i < bd->u.stex.count; ++i)
                        texture(sd->stage, bd->u.stex.texSamplers[i], baseShaderRegister + i);
                }
            }
                break;
            case QRhiShaderResourceBinding::Sampler:
            {
                Q_ASSERT(bd->u.stex.count > 0);
                const int baseShaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).first;
                if (baseShaderRegister >= 0 && sampler) {
                    for (int i = 0; i < bd->u.stex.count; ++i)
                        sampler(sd->stage, bd->u.stex.texSamplers[i], baseShaderRegister + i);
                }
            }
                break;
            case QRhiShaderResourceBinding::ImageLoad:
            {
                const int shaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).first;
                if (shaderRegister >= 0 && storageImage)
                    storageImage(sd->stage, bd->u.simage, Load, shaderRegister);
            }
                break;
            case QRhiShaderResourceBinding::ImageStore:
            {
                const int shaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).first;
                if (shaderRegister >= 0 && storageImage)
                    storageImage(sd->stage, bd->u.simage, Store, shaderRegister);
            }
                break;
            case QRhiShaderResourceBinding::ImageLoadStore:
            {
                const int shaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).first;
                if (shaderRegister >= 0 && storageImage)
                    storageImage(sd->stage, bd->u.simage, LoadStore, shaderRegister);
            }
                break;
            case QRhiShaderResourceBinding::BufferLoad:
            {
                const int shaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).first;
                if (shaderRegister >= 0 && storageBuffer)
                    storageBuffer(sd->stage, bd->u.sbuf, Load, shaderRegister);
            }
                break;
            case QRhiShaderResourceBinding::BufferStore:
            {
                const int shaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).first;
                if (shaderRegister >= 0 && storageBuffer)
                    storageBuffer(sd->stage, bd->u.sbuf, Store, shaderRegister);
            }
                break;
            case QRhiShaderResourceBinding::BufferLoadStore:
            {
                const int shaderRegister = mapBinding(bd->binding, sd->nativeResourceBindingMap).first;
                if (shaderRegister >= 0 && storageBuffer)
                    storageBuffer(sd->stage, bd->u.sbuf, LoadStore, shaderRegister);
            }
                break;
            }
        }
    }
}

bool QD3D12SamplerManager::create(ID3D12Device *device)
{
    // This does not need to be per-frame slot, just grab space for MAX_SAMPLERS samplers.
    if (!shaderVisibleSamplerHeap.create(device,
                                         D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                                         MAX_SAMPLERS / QD3D12_FRAMES_IN_FLIGHT))
    {
        qWarning("Could not create shader-visible SAMPLER heap");
        return false;
    }

    this->device = device;
    return true;
}

void QD3D12SamplerManager::destroy()
{
    if (device) {
        shaderVisibleSamplerHeap.destroy();
        device = nullptr;
    }
}

QD3D12Descriptor QD3D12SamplerManager::getShaderVisibleDescriptor(const D3D12_SAMPLER_DESC &desc)
{
    auto it = gpuMap.constFind({desc});
    if (it != gpuMap.cend())
        return *it;

    QD3D12Descriptor descriptor = shaderVisibleSamplerHeap.heap.get(1);
    if (descriptor.isValid()) {
        device->CreateSampler(&desc, descriptor.cpuHandle);
        gpuMap.insert({desc}, descriptor);
    } else {
        qWarning("Out of shader-visible SAMPLER descriptor heap space,"
                 " this should not happen, maximum number of unique samplers is %u",
                 shaderVisibleSamplerHeap.heap.capacity);
    }

    return descriptor;
}

bool QD3D12MipmapGenerator::create(QRhiD3D12 *rhiD)
{
    this->rhiD = rhiD;

    D3D12_ROOT_PARAMETER1 rootParams[3] = {};
    D3D12_DESCRIPTOR_RANGE1 descriptorRanges[2] = {};

    // b0
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;

    // t0
    descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[0].NumDescriptors = 1;
    descriptorRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &descriptorRanges[0];

    // u0..3
    descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorRanges[1].NumDescriptors = 4;
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[2].DescriptorTable.pDescriptorRanges = &descriptorRanges[1];

    // s0
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rsDesc.Desc_1_1.NumParameters = 3;
    rsDesc.Desc_1_1.pParameters = rootParams;
    rsDesc.Desc_1_1.NumStaticSamplers = 1;
    rsDesc.Desc_1_1.pStaticSamplers = &samplerDesc;

    ID3DBlob *signature = nullptr;
    HRESULT hr = D3D12SerializeVersionedRootSignature(&rsDesc, &signature, nullptr);
    if (FAILED(hr)) {
        qWarning("Failed to serialize root signature: %s", qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }
    ID3D12RootSignature *rootSig = nullptr;
    hr = rhiD->dev->CreateRootSignature(0,
                                        signature->GetBufferPointer(),
                                        signature->GetBufferSize(),
                                        __uuidof(ID3D12RootSignature),
                                        reinterpret_cast<void **>(&rootSig));
    signature->Release();
    if (FAILED(hr)) {
        qWarning("Failed to create root signature: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }

    rootSigHandle = QD3D12RootSignature::addToPool(&rhiD->rootSignaturePool, rootSig);

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSig;
    psoDesc.CS.pShaderBytecode = g_csMipmap;
    psoDesc.CS.BytecodeLength = sizeof(g_csMipmap);
    ID3D12PipelineState *pso = nullptr;
    hr = rhiD->dev->CreateComputePipelineState(&psoDesc,
                                               __uuidof(ID3D12PipelineState),
                                               reinterpret_cast<void **>(&pso));
    if (FAILED(hr)) {
        qWarning("Failed to create compute pipeline state: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        rhiD->rootSignaturePool.remove(rootSigHandle);
        rootSigHandle = {};
        return false;
    }

    pipelineHandle = QD3D12Pipeline::addToPool(&rhiD->pipelinePool, QD3D12Pipeline::Compute, pso);

    return true;
}

void QD3D12MipmapGenerator::destroy()
{
    rhiD->pipelinePool.remove(pipelineHandle);
    pipelineHandle = {};
    rhiD->rootSignaturePool.remove(rootSigHandle);
    rootSigHandle = {};
}

void QD3D12MipmapGenerator::generate(QD3D12CommandBuffer *cbD, const QD3D12ObjectHandle &textureHandle)
{
    QD3D12Pipeline *pipeline = rhiD->pipelinePool.lookupRef(pipelineHandle);
    if (!pipeline)
        return;
    QD3D12RootSignature *rootSig = rhiD->rootSignaturePool.lookupRef(rootSigHandle);
    if (!rootSig)
        return;
    QD3D12Resource *res = rhiD->resourcePool.lookupRef(textureHandle);
    if (!res)
        return;

    const quint32 mipLevelCount = res->desc.MipLevels;
    if (mipLevelCount < 2)
        return;

    if (res->desc.SampleDesc.Count > 1) {
        qWarning("Cannot generate mipmaps for MSAA texture");
        return;
    }

    const bool is1D = res->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    if (is1D) {
        qWarning("Cannot generate mipmaps for 1D texture");
        return;
    }

    const bool is3D = res->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    const bool isCubeOrArray = res->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D
            && res->desc.DepthOrArraySize > 1;
    const quint32 layerCount = isCubeOrArray ? res->desc.DepthOrArraySize : 1;

    if (is3D) {
        qWarning("2D mipmap generator invoked for 3D texture, this should not happen");
        return;
    }

    rhiD->barrierGen.addTransitionBarrier(textureHandle, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    rhiD->barrierGen.enqueueBufferedTransitionBarriers(cbD);

    cbD->cmdList->SetPipelineState(pipeline->pso);
    cbD->cmdList->SetComputeRootSignature(rootSig->rootSig);

    const quint32 descriptorByteSize = rhiD->shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[rhiD->currentFrameSlot].descriptorByteSize;

    struct CBufData {
        quint32 srcMipLevel;
        quint32 numMipLevels;
        float texelWidth;
        float texelHeight;
    };

    const quint32 allocSize = QD3D12StagingArea::allocSizeForArray(sizeof(CBufData), mipLevelCount * layerCount);
    std::optional<QD3D12StagingArea> ownStagingArea;
    if (rhiD->smallStagingAreas[rhiD->currentFrameSlot].remainingCapacity() < allocSize) {
        ownStagingArea = QD3D12StagingArea();
        if (!ownStagingArea->create(rhiD, allocSize, D3D12_HEAP_TYPE_UPLOAD)) {
            qWarning("Could not create staging area for mipmap generation");
            return;
        }
    }
    QD3D12StagingArea *workArea = ownStagingArea.has_value()
            ? &ownStagingArea.value()
            : &rhiD->smallStagingAreas[rhiD->currentFrameSlot];

    bool gotNewHeap = false;
    if (!rhiD->ensureShaderVisibleDescriptorHeapCapacity(&rhiD->shaderVisibleCbvSrvUavHeap,
                                                         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                         rhiD->currentFrameSlot,
                                                         (1 + 4) * mipLevelCount * layerCount,
                                                         &gotNewHeap))
    {
        qWarning("Could not ensure enough space in descriptor heap for mipmap generation");
        return;
    }
    if (gotNewHeap)
        rhiD->bindShaderVisibleHeaps(cbD);

    for (quint32 layer = 0; layer < layerCount; ++layer) {
        for (quint32 level = 0; level < mipLevelCount ;) {
            UINT subresource = calcSubresource(level, layer, res->desc.MipLevels);
            rhiD->barrierGen.enqueueSubresourceTransitionBarrier(cbD, textureHandle, subresource,
                                                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            quint32 levelPlusOneMipWidth = res->desc.Width >> (level + 1);
            quint32 levelPlusOneMipHeight = res->desc.Height >> (level + 1);
            const quint32 dw = levelPlusOneMipWidth == 1 ? levelPlusOneMipHeight : levelPlusOneMipWidth;
            const quint32 dh = levelPlusOneMipHeight == 1 ? levelPlusOneMipWidth : levelPlusOneMipHeight;
            // number of times the size can be halved while still resulting in an even dimension
            const quint32 additionalMips = qCountTrailingZeroBits(dw | dh);
            const quint32 numGenMips = qMin(1u + qMin(3u, additionalMips), res->desc.MipLevels - level);
            levelPlusOneMipWidth = qMax(1u, levelPlusOneMipWidth);
            levelPlusOneMipHeight = qMax(1u, levelPlusOneMipHeight);

            CBufData cbufData = {
                level,
                numGenMips,
                1.0f / float(levelPlusOneMipWidth),
                1.0f / float(levelPlusOneMipHeight)
            };

            QD3D12StagingArea::Allocation cbuf = workArea->get(sizeof(cbufData));
            memcpy(cbuf.p, &cbufData, sizeof(cbufData));
            cbD->cmdList->SetComputeRootConstantBufferView(0, cbuf.gpuAddr);

            QD3D12Descriptor srv = rhiD->shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[rhiD->currentFrameSlot].get(1);
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = res->desc.Format;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            if (isCubeOrArray) {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MipLevels = res->desc.MipLevels;
                srvDesc.Texture2DArray.FirstArraySlice = layer;
                srvDesc.Texture2DArray.ArraySize = 1;
            } else {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = res->desc.MipLevels;
            }
            rhiD->dev->CreateShaderResourceView(res->resource, &srvDesc, srv.cpuHandle);
            cbD->cmdList->SetComputeRootDescriptorTable(1, srv.gpuHandle);

            QD3D12Descriptor uavStart = rhiD->shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[rhiD->currentFrameSlot].get(4);
            D3D12_CPU_DESCRIPTOR_HANDLE uavCpuHandle = uavStart.cpuHandle;
            // if level is N, then need UAVs for levels N+1, ..., N+4
            for (quint32 uavIdx = 0; uavIdx < 4; ++uavIdx) {
                const quint32 uavMipLevel = qMin(level + 1u + uavIdx, res->desc.MipLevels - 1u);
                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = res->desc.Format;
                if (isCubeOrArray) {
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = uavMipLevel;
                    uavDesc.Texture2DArray.FirstArraySlice = layer;
                    uavDesc.Texture2DArray.ArraySize = 1;
                } else {
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    uavDesc.Texture2D.MipSlice = uavMipLevel;
                }
                rhiD->dev->CreateUnorderedAccessView(res->resource, nullptr, &uavDesc, uavCpuHandle);
                uavCpuHandle.ptr += descriptorByteSize;
            }
            cbD->cmdList->SetComputeRootDescriptorTable(2, uavStart.gpuHandle);

            cbD->cmdList->Dispatch(levelPlusOneMipWidth, levelPlusOneMipHeight, 1);

            rhiD->barrierGen.enqueueUavBarrier(cbD, textureHandle);
            rhiD->barrierGen.enqueueSubresourceTransitionBarrier(cbD, textureHandle, subresource,
                                                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            level += numGenMips;
        }
    }

    if (ownStagingArea.has_value())
        ownStagingArea->destroyWithDeferredRelease(&rhiD->releaseQueue);
}

bool QD3D12MipmapGenerator3D::create(QRhiD3D12 *rhiD)
{
    this->rhiD = rhiD;

    D3D12_ROOT_PARAMETER1 rootParams[3] = {};
    D3D12_DESCRIPTOR_RANGE1 descriptorRanges[2] = {};

    // b0
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;

    // t0
    descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[0].NumDescriptors = 1;
    descriptorRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &descriptorRanges[0];

    // u0
    descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorRanges[1].NumDescriptors = 1;
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[2].DescriptorTable.pDescriptorRanges = &descriptorRanges[1];

    // s0
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rsDesc.Desc_1_1.NumParameters = 3;
    rsDesc.Desc_1_1.pParameters = rootParams;
    rsDesc.Desc_1_1.NumStaticSamplers = 1;
    rsDesc.Desc_1_1.pStaticSamplers = &samplerDesc;

    ID3DBlob *signature = nullptr;
    HRESULT hr = D3D12SerializeVersionedRootSignature(&rsDesc, &signature, nullptr);
    if (FAILED(hr)) {
        qWarning("Failed to serialize root signature: %s", qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }
    ID3D12RootSignature *rootSig = nullptr;
    hr = rhiD->dev->CreateRootSignature(0,
                                        signature->GetBufferPointer(),
                                        signature->GetBufferSize(),
                                        __uuidof(ID3D12RootSignature),
                                        reinterpret_cast<void **>(&rootSig));
    signature->Release();
    if (FAILED(hr)) {
        qWarning("Failed to create root signature: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }

    rootSigHandle = QD3D12RootSignature::addToPool(&rhiD->rootSignaturePool, rootSig);

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSig;
    psoDesc.CS.pShaderBytecode = g_csMipmap3D;
    psoDesc.CS.BytecodeLength = sizeof(g_csMipmap3D);
    ID3D12PipelineState *pso = nullptr;
    hr = rhiD->dev->CreateComputePipelineState(&psoDesc,
                                               __uuidof(ID3D12PipelineState),
                                               reinterpret_cast<void **>(&pso));
    if (FAILED(hr)) {
        qWarning("Failed to create compute pipeline state: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        rhiD->rootSignaturePool.remove(rootSigHandle);
        rootSigHandle = {};
        return false;
    }

    pipelineHandle = QD3D12Pipeline::addToPool(&rhiD->pipelinePool, QD3D12Pipeline::Compute, pso);

    return true;
}

void QD3D12MipmapGenerator3D::destroy()
{
    rhiD->pipelinePool.remove(pipelineHandle);
    pipelineHandle = {};
    rhiD->rootSignaturePool.remove(rootSigHandle);
    rootSigHandle = {};
}

void QD3D12MipmapGenerator3D::generate(QD3D12CommandBuffer *cbD, const QD3D12ObjectHandle &textureHandle)
{
    QD3D12Pipeline *pipeline = rhiD->pipelinePool.lookupRef(pipelineHandle);
    if (!pipeline)
        return;
    QD3D12RootSignature *rootSig = rhiD->rootSignaturePool.lookupRef(rootSigHandle);
    if (!rootSig)
        return;
    QD3D12Resource *res = rhiD->resourcePool.lookupRef(textureHandle);
    if (!res)
        return;

    const quint32 mipLevelCount = res->desc.MipLevels;
    if (mipLevelCount < 2)
        return;

    const bool is3D = res->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    if (!is3D) {
        qWarning("3D mipmap generator invoked for non-3D texture, this should not happen");
        return;
    }

    rhiD->barrierGen.addTransitionBarrier(textureHandle, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    rhiD->barrierGen.enqueueBufferedTransitionBarriers(cbD);

    cbD->cmdList->SetPipelineState(pipeline->pso);
    cbD->cmdList->SetComputeRootSignature(rootSig->rootSig);

    const quint32 descriptorByteSize = rhiD->shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[rhiD->currentFrameSlot].descriptorByteSize;

    struct CBufData {
        float texelWidth;
        float texelHeight;
        float texelDepth;
        quint32 srcMipLevel;
    };

    const quint32 allocSize = QD3D12StagingArea::allocSizeForArray(sizeof(CBufData), mipLevelCount);
    std::optional<QD3D12StagingArea> ownStagingArea;
    if (rhiD->smallStagingAreas[rhiD->currentFrameSlot].remainingCapacity() < allocSize) {
        ownStagingArea = QD3D12StagingArea();
        if (!ownStagingArea->create(rhiD, allocSize, D3D12_HEAP_TYPE_UPLOAD)) {
            qWarning("Could not create staging area for mipmap generation");
            return;
        }
    }
    QD3D12StagingArea *workArea = ownStagingArea.has_value()
            ? &ownStagingArea.value()
            : &rhiD->smallStagingAreas[rhiD->currentFrameSlot];

    bool gotNewHeap = false;
    if (!rhiD->ensureShaderVisibleDescriptorHeapCapacity(&rhiD->shaderVisibleCbvSrvUavHeap,
                                                         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                         rhiD->currentFrameSlot,
                                                         (1 + 1) * mipLevelCount, // 1 SRV + 1 UAV
                                                         &gotNewHeap))
    {
        qWarning("Could not ensure enough space in descriptor heap for mipmap generation");
        return;
    }
    if (gotNewHeap)
        rhiD->bindShaderVisibleHeaps(cbD);

    for (quint32 level = 0; level < mipLevelCount; ++level) {
        UINT subresource = calcSubresource(level, 0u, res->desc.MipLevels);
        rhiD->barrierGen.enqueueSubresourceTransitionBarrier(cbD, textureHandle, subresource,
                                                                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        quint32 levelPlusOneMipWidth = qMax<quint32>(1, res->desc.Width >> (level + 1));
        quint32 levelPlusOneMipHeight = qMax<quint32>(1, res->desc.Height >> (level + 1));
        quint32 levelPlusOneMipDepth = qMax<quint32>(1, res->desc.DepthOrArraySize >> (level + 1));

        CBufData cbufData = {
            1.0f / float(levelPlusOneMipWidth),
            1.0f / float(levelPlusOneMipHeight),
            1.0f / float(levelPlusOneMipDepth),
            quint32(level)
        };

        QD3D12StagingArea::Allocation cbuf = workArea->get(sizeof(cbufData));
        memcpy(cbuf.p, &cbufData, sizeof(cbufData));
        cbD->cmdList->SetComputeRootConstantBufferView(0, cbuf.gpuAddr);

        QD3D12Descriptor srv = rhiD->shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[rhiD->currentFrameSlot].get(1);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = res->desc.Format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MipLevels = res->desc.MipLevels;

        rhiD->dev->CreateShaderResourceView(res->resource, &srvDesc, srv.cpuHandle);
        cbD->cmdList->SetComputeRootDescriptorTable(1, srv.gpuHandle);

        QD3D12Descriptor uavStart = rhiD->shaderVisibleCbvSrvUavHeap.perFrameHeapSlice[rhiD->currentFrameSlot].get(1);
        D3D12_CPU_DESCRIPTOR_HANDLE uavCpuHandle = uavStart.cpuHandle;
        const quint32 uavMipLevel = qMin(level + 1u, res->desc.MipLevels - 1u);
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = res->desc.Format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.MipSlice = uavMipLevel;
        uavDesc.Texture3D.WSize = UINT(-1);
        rhiD->dev->CreateUnorderedAccessView(res->resource, nullptr, &uavDesc, uavCpuHandle);
        uavCpuHandle.ptr += descriptorByteSize;
        cbD->cmdList->SetComputeRootDescriptorTable(2, uavStart.gpuHandle);

        cbD->cmdList->Dispatch(levelPlusOneMipWidth, levelPlusOneMipHeight, levelPlusOneMipDepth);

        rhiD->barrierGen.enqueueUavBarrier(cbD, textureHandle);
        rhiD->barrierGen.enqueueSubresourceTransitionBarrier(cbD, textureHandle, subresource,
                                                                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    if (ownStagingArea.has_value())
        ownStagingArea->destroyWithDeferredRelease(&rhiD->releaseQueue);
}

bool QD3D12MemoryAllocator::create(ID3D12Device *device, IDXGIAdapter1 *adapter)
{
    this->device = device;

    // We can function with and without D3D12MA: CreateCommittedResource is
    // just fine for our purposes and not any complicated API-wise; the memory
    // allocator is interesting for efficiency mainly since it can suballocate
    // instead of making everything a committed resource allocation.

    static bool disableMA = qEnvironmentVariableIntValue("QT_D3D_NO_SUBALLOC");
    if (disableMA)
        return true;

    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        return true;

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = device;
    allocatorDesc.pAdapter = adapter;
    // A QRhi is supposed to be used from one single thread only. Disable
    // the allocator's own mutexes. This may give a performance boost.
    allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;
    HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocator);
    if (FAILED(hr)) {
        qWarning("Failed to initialize D3D12 Memory Allocator: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }
    return true;
}

void QD3D12MemoryAllocator::destroy()
{
    if (allocator) {
        allocator->Release();
        allocator = nullptr;
    }
}

HRESULT QD3D12MemoryAllocator::createResource(D3D12_HEAP_TYPE heapType,
                                              const D3D12_RESOURCE_DESC *resourceDesc,
                                              D3D12_RESOURCE_STATES initialState,
                                              const D3D12_CLEAR_VALUE *optimizedClearValue,
                                              D3D12MA::Allocation **maybeAllocation,
                                              REFIID riidResource,
                                              void **ppvResource)
{
    if (allocator) {
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = heapType;
        return allocator->CreateResource(&allocDesc,
                                         resourceDesc,
                                         initialState,
                                         optimizedClearValue,
                                         maybeAllocation,
                                         riidResource,
                                         ppvResource);
    } else {
        *maybeAllocation = nullptr;
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = heapType;
        return device->CreateCommittedResource(&heapProps,
                                               D3D12_HEAP_FLAG_NONE,
                                               resourceDesc,
                                               initialState,
                                               optimizedClearValue,
                                               riidResource,
                                               ppvResource);
    }
}

void QD3D12MemoryAllocator::getBudget(D3D12MA::Budget *localBudget, D3D12MA::Budget *nonLocalBudget)
{
    if (allocator) {
        allocator->GetBudget(localBudget, nonLocalBudget);
    } else {
        *localBudget = {};
        *nonLocalBudget = {};
    }
}

void QRhiD3D12::waitGpu()
{
    fullFenceCounter += 1u;
    if (SUCCEEDED(cmdQueue->Signal(fullFence, fullFenceCounter))) {
        if (SUCCEEDED(fullFence->SetEventOnCompletion(fullFenceCounter, fullFenceEvent)))
            WaitForSingleObject(fullFenceEvent, INFINITE);
    }
}

DXGI_SAMPLE_DESC QRhiD3D12::effectiveSampleDesc(int sampleCount, DXGI_FORMAT format) const
{
    DXGI_SAMPLE_DESC desc;
    desc.Count = 1;
    desc.Quality = 0;

    const int s = effectiveSampleCount(sampleCount);

    if (s > 1) {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaInfo = {};
        msaaInfo.Format = format;
        msaaInfo.SampleCount = UINT(s);
        if (SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaInfo, sizeof(msaaInfo)))) {
            if (msaaInfo.NumQualityLevels > 0) {
                desc.Count = UINT(s);
                desc.Quality = msaaInfo.NumQualityLevels - 1;
            } else {
                qWarning("No quality levels for multisampling with sample count %d", s);
            }
        }
    }

    return desc;
}

bool QRhiD3D12::startCommandListForCurrentFrameSlot(D3D12GraphicsCommandList **cmdList)
{
    ID3D12CommandAllocator *cmdAlloc = cmdAllocators[currentFrameSlot];
    if (!*cmdList) {
        HRESULT hr = dev->CreateCommandList(0,
                                            D3D12_COMMAND_LIST_TYPE_DIRECT,
                                            cmdAlloc,
                                            nullptr,
                                            __uuidof(D3D12GraphicsCommandList),
                                            reinterpret_cast<void **>(cmdList));
        if (FAILED(hr)) {
            qWarning("Failed to create command list: %s", qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    } else {
        HRESULT hr = (*cmdList)->Reset(cmdAlloc, nullptr);
        if (FAILED(hr)) {
            qWarning("Failed to reset command list: %s", qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    }
    return true;
}

static inline QRhiTexture::Format swapchainReadbackTextureFormat(DXGI_FORMAT format, QRhiTexture::Flags *flags)
{
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return QRhiTexture::RGBA8;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        if (flags)
            (*flags) |= QRhiTexture::sRGB;
        return QRhiTexture::RGBA8;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return QRhiTexture::BGRA8;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        if (flags)
            (*flags) |= QRhiTexture::sRGB;
        return QRhiTexture::BGRA8;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return QRhiTexture::RGBA16F;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return QRhiTexture::RGBA32F;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return QRhiTexture::RGB10A2;
    default:
        qWarning("DXGI_FORMAT %d cannot be read back", format);
        break;
    }
    return QRhiTexture::UnknownFormat;
}

void QRhiD3D12::enqueueResourceUpdates(QD3D12CommandBuffer *cbD, QRhiResourceUpdateBatch *resourceUpdates)
{
    QRhiResourceUpdateBatchPrivate *ud = QRhiResourceUpdateBatchPrivate::get(resourceUpdates);

    for (int opIdx = 0; opIdx < ud->activeBufferOpCount; ++opIdx) {
        const QRhiResourceUpdateBatchPrivate::BufferOp &u(ud->bufferOps[opIdx]);
        if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::DynamicUpdate) {
            QD3D12Buffer *bufD = QRHI_RES(QD3D12Buffer, u.buf);
            Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
            for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
                if (u.offset == 0 && u.data.size() == bufD->m_size)
                    bufD->pendingHostWrites[i].clear();
                bufD->pendingHostWrites[i].append({ u.offset, u.data });
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::StaticUpload) {
            QD3D12Buffer *bufD = QRHI_RES(QD3D12Buffer, u.buf);
            Q_ASSERT(bufD->m_type != QRhiBuffer::Dynamic);
            Q_ASSERT(u.offset + u.data.size() <= bufD->m_size);

            // The general approach to staging upload data is to first try
            // using the per-frame "small" staging area, which is a very simple
            // linear allocator; if that's not big enough then create a
            // dedicated StagingArea and then deferred-release it to make sure
            // if stays alive while the frame is possibly still in flight.

            QD3D12StagingArea::Allocation stagingAlloc;
            const quint32 allocSize = QD3D12StagingArea::allocSizeForArray(bufD->m_size, 1);
            if (smallStagingAreas[currentFrameSlot].remainingCapacity() >= allocSize)
                stagingAlloc = smallStagingAreas[currentFrameSlot].get(bufD->m_size);

            std::optional<QD3D12StagingArea> ownStagingArea;
            if (!stagingAlloc.isValid()) {
                ownStagingArea = QD3D12StagingArea();
                if (!ownStagingArea->create(this, allocSize, D3D12_HEAP_TYPE_UPLOAD))
                    continue;
                stagingAlloc = ownStagingArea->get(allocSize);
                if (!stagingAlloc.isValid()) {
                    ownStagingArea->destroy();
                    continue;
                }
            }

            memcpy(stagingAlloc.p + u.offset, u.data.constData(), u.data.size());

            barrierGen.addTransitionBarrier(bufD->handles[0], D3D12_RESOURCE_STATE_COPY_DEST);
            barrierGen.enqueueBufferedTransitionBarriers(cbD);

            if (QD3D12Resource *res = resourcePool.lookupRef(bufD->handles[0])) {
                cbD->cmdList->CopyBufferRegion(res->resource,
                                               u.offset,
                                               stagingAlloc.buffer,
                                               stagingAlloc.bufferOffset + u.offset,
                                               u.data.size());
            }

            if (ownStagingArea.has_value())
                ownStagingArea->destroyWithDeferredRelease(&releaseQueue);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::Read) {
            QD3D12Buffer *bufD = QRHI_RES(QD3D12Buffer, u.buf);
            if (bufD->m_type == QRhiBuffer::Dynamic) {
                bufD->executeHostWritesForFrameSlot(currentFrameSlot);
                if (QD3D12Resource *res = resourcePool.lookupRef(bufD->handles[currentFrameSlot])) {
                    Q_ASSERT(res->cpuMapPtr);
                    u.result->data.resize(u.readSize);
                    memcpy(u.result->data.data(), reinterpret_cast<char *>(res->cpuMapPtr) + u.offset, u.readSize);
                }
                if (u.result->completed)
                    u.result->completed();
            } else {
                QD3D12Readback readback;
                readback.frameSlot = currentFrameSlot;
                readback.result = u.result;
                readback.byteSize = u.readSize;
                const quint32 allocSize = aligned(u.readSize, QD3D12StagingArea::ALIGNMENT);
                if (!readback.staging.create(this, allocSize, D3D12_HEAP_TYPE_READBACK)) {
                    if (u.result->completed)
                        u.result->completed();
                    continue;
                }
                QD3D12StagingArea::Allocation stagingAlloc = readback.staging.get(u.readSize);
                if (!stagingAlloc.isValid()) {
                    readback.staging.destroy();
                    if (u.result->completed)
                        u.result->completed();
                    continue;
                }
                Q_ASSERT(stagingAlloc.bufferOffset == 0);
                barrierGen.addTransitionBarrier(bufD->handles[0], D3D12_RESOURCE_STATE_COPY_SOURCE);
                barrierGen.enqueueBufferedTransitionBarriers(cbD);
                if (QD3D12Resource *res = resourcePool.lookupRef(bufD->handles[0])) {
                    cbD->cmdList->CopyBufferRegion(stagingAlloc.buffer, 0, res->resource, u.offset, u.readSize);
                    activeReadbacks.append(readback);
                } else {
                    readback.staging.destroy();
                    if (u.result->completed)
                        u.result->completed();
                }
            }
        }
    }

    for (int opIdx = 0; opIdx < ud->activeTextureOpCount; ++opIdx) {
        const QRhiResourceUpdateBatchPrivate::TextureOp &u(ud->textureOps[opIdx]);
        if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Upload) {
            QD3D12Texture *texD = QRHI_RES(QD3D12Texture, u.dst);
            const bool is3D = texD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
            QD3D12Resource *res = resourcePool.lookupRef(texD->handle);
            if (!res)
                continue;
            barrierGen.addTransitionBarrier(texD->handle, D3D12_RESOURCE_STATE_COPY_DEST);
            barrierGen.enqueueBufferedTransitionBarriers(cbD);
            for (int layer = 0, maxLayer = u.subresDesc.size(); layer < maxLayer; ++layer) {
                for (int level = 0; level < QRhi::MAX_MIP_LEVELS; ++level) {
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : std::as_const(u.subresDesc[layer][level])) {
                        D3D12_SUBRESOURCE_FOOTPRINT footprint = {};
                        footprint.Format = res->desc.Format;
                        footprint.Depth = 1;
                        quint32 totalBytes = 0;

                        const QSize subresSize = subresDesc.sourceSize().isEmpty() ? q->sizeForMipLevel(level, texD->m_pixelSize)
                                                                                   : subresDesc.sourceSize();
                        const QPoint srcPos = subresDesc.sourceTopLeft();
                        QPoint dstPos = subresDesc.destinationTopLeft();

                        if (!subresDesc.image().isNull()) {
                            const QImage img = subresDesc.image();
                            const int bpl = img.bytesPerLine();
                            footprint.RowPitch = aligned<UINT>(bpl, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
                            totalBytes = footprint.RowPitch * img.height();
                        } else if (!subresDesc.data().isEmpty() && isCompressedFormat(texD->m_format)) {
                            QSize blockDim;
                            quint32 bpl = 0;
                            compressedFormatInfo(texD->m_format, subresSize, &bpl, nullptr, &blockDim);
                            footprint.RowPitch = aligned<UINT>(bpl, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
                            const int rowCount = aligned(subresSize.height(), blockDim.height()) / blockDim.height();
                            totalBytes = footprint.RowPitch * rowCount;
                        } else if (!subresDesc.data().isEmpty()) {
                            quint32 bpl = 0;
                            if (subresDesc.dataStride())
                                bpl = subresDesc.dataStride();
                            else
                                textureFormatInfo(texD->m_format, subresSize, &bpl, nullptr, nullptr);
                            footprint.RowPitch = aligned<UINT>(bpl, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
                            totalBytes = footprint.RowPitch * subresSize.height();
                        } else {
                            qWarning("Invalid texture upload for %p layer=%d mip=%d", texD, layer, level);
                            continue;
                        }

                        const quint32 allocSize = QD3D12StagingArea::allocSizeForArray(totalBytes, 1);
                        QD3D12StagingArea::Allocation stagingAlloc;
                        if (smallStagingAreas[currentFrameSlot].remainingCapacity() >= allocSize)
                            stagingAlloc = smallStagingAreas[currentFrameSlot].get(allocSize);

                        std::optional<QD3D12StagingArea> ownStagingArea;
                        if (!stagingAlloc.isValid()) {
                            ownStagingArea = QD3D12StagingArea();
                            if (!ownStagingArea->create(this, allocSize, D3D12_HEAP_TYPE_UPLOAD))
                                continue;
                            stagingAlloc = ownStagingArea->get(allocSize);
                            if (!stagingAlloc.isValid()) {
                                ownStagingArea->destroy();
                                continue;
                            }
                        }

                        D3D12_TEXTURE_COPY_LOCATION dst;
                        dst.pResource = res->resource;
                        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                        dst.SubresourceIndex = calcSubresource(UINT(level), is3D ? 0u : UINT(layer), texD->mipLevelCount);
                        D3D12_TEXTURE_COPY_LOCATION src;
                        src.pResource = stagingAlloc.buffer;
                        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                        src.PlacedFootprint.Offset = stagingAlloc.bufferOffset;

                        D3D12_BOX srcBox; // back, right, bottom are exclusive

                        if (!subresDesc.image().isNull()) {
                            const QImage img = subresDesc.image();
                            const int bpc = qMax(1, img.depth() / 8);
                            const int bpl = img.bytesPerLine();

                            QSize size = subresDesc.sourceSize().isEmpty() ? img.size() : subresDesc.sourceSize();
                            size.setWidth(qMin(size.width(), img.width() - srcPos.x()));
                            size.setHeight(qMin(size.height(), img.height() - srcPos.y()));

                            footprint.Width = size.width();
                            footprint.Height = size.height();

                            srcBox.left = 0;
                            srcBox.top = 0;
                            srcBox.right = UINT(size.width());
                            srcBox.bottom = UINT(size.height());
                            srcBox.front = 0;
                            srcBox.back = 1;

                            const uchar *imgPtr = img.constBits();
                            const quint32 lineBytes = size.width() * bpc;
                            for (int y = 0, h = size.height(); y < h; ++y) {
                                memcpy(stagingAlloc.p + y * footprint.RowPitch,
                                       imgPtr + srcPos.x() * bpc + (y + srcPos.y()) * bpl,
                                       lineBytes);
                            }
                        } else if (!subresDesc.data().isEmpty() && isCompressedFormat(texD->m_format)) {
                            QSize blockDim;
                            quint32 bpl = 0;
                            compressedFormatInfo(texD->m_format, subresSize, &bpl, nullptr, &blockDim);
                            // x and y must be multiples of the block width and height
                            dstPos.setX(aligned(dstPos.x(), blockDim.width()));
                            dstPos.setY(aligned(dstPos.y(), blockDim.height()));

                            srcBox.left = 0;
                            srcBox.top = 0;
                            // width and height must be multiples of the block width and height
                            srcBox.right = aligned(subresSize.width(), blockDim.width());
                            srcBox.bottom = aligned(subresSize.height(), blockDim.height());

                            srcBox.front = 0;
                            srcBox.back = 1;

                            footprint.Width = aligned(subresSize.width(), blockDim.width());
                            footprint.Height = aligned(subresSize.height(), blockDim.height());

                            const quint32 copyBytes = qMin(bpl, footprint.RowPitch);
                            const QByteArray imgData = subresDesc.data();
                            const char *imgPtr = imgData.constData();
                            const int rowCount = aligned(subresSize.height(), blockDim.height()) / blockDim.height();
                            for (int y = 0; y < rowCount; ++y)
                                memcpy(stagingAlloc.p + y * footprint.RowPitch, imgPtr + y * bpl, copyBytes);
                        } else if (!subresDesc.data().isEmpty()) {
                            srcBox.left = 0;
                            srcBox.top = 0;
                            srcBox.right = subresSize.width();
                            srcBox.bottom = subresSize.height();
                            srcBox.front = 0;
                            srcBox.back = 1;

                            footprint.Width = subresSize.width();
                            footprint.Height = subresSize.height();

                            quint32 bpl = 0;
                            if (subresDesc.dataStride())
                                bpl = subresDesc.dataStride();
                            else
                                textureFormatInfo(texD->m_format, subresSize, &bpl, nullptr, nullptr);

                            const quint32 copyBytes = qMin(bpl, footprint.RowPitch);
                            const QByteArray data = subresDesc.data();
                            const char *imgPtr = data.constData();
                            for (int y = 0, h = subresSize.height(); y < h; ++y)
                                memcpy(stagingAlloc.p + y * footprint.RowPitch, imgPtr + y * bpl, copyBytes);
                        }

                        src.PlacedFootprint.Footprint = footprint;

                        cbD->cmdList->CopyTextureRegion(&dst,
                                                        UINT(dstPos.x()),
                                                        UINT(dstPos.y()),
                                                        is3D ? UINT(layer) : 0u,
                                                        &src,
                                                        &srcBox);

                        if (ownStagingArea.has_value())
                            ownStagingArea->destroyWithDeferredRelease(&releaseQueue);
                    }
                }
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Copy) {
            Q_ASSERT(u.src && u.dst);
            QD3D12Texture *srcD = QRHI_RES(QD3D12Texture, u.src);
            QD3D12Texture *dstD = QRHI_RES(QD3D12Texture, u.dst);
            const bool srcIs3D = srcD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
            const bool dstIs3D = dstD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
            QD3D12Resource *srcRes = resourcePool.lookupRef(srcD->handle);
            QD3D12Resource *dstRes = resourcePool.lookupRef(dstD->handle);
            if (!srcRes || !dstRes)
                continue;

            barrierGen.addTransitionBarrier(srcD->handle, D3D12_RESOURCE_STATE_COPY_SOURCE);
            barrierGen.addTransitionBarrier(dstD->handle, D3D12_RESOURCE_STATE_COPY_DEST);
            barrierGen.enqueueBufferedTransitionBarriers(cbD);

            const UINT srcSubresource = calcSubresource(UINT(u.desc.sourceLevel()),
                                                        srcIs3D ? 0u : UINT(u.desc.sourceLayer()),
                                                        srcD->mipLevelCount);
            const UINT dstSubresource = calcSubresource(UINT(u.desc.destinationLevel()),
                                                        dstIs3D ? 0u : UINT(u.desc.destinationLayer()),
                                                        dstD->mipLevelCount);
            const QPoint dp = u.desc.destinationTopLeft();
            const QSize mipSize = q->sizeForMipLevel(u.desc.sourceLevel(), srcD->m_pixelSize);
            const QSize copySize = u.desc.pixelSize().isEmpty() ? mipSize : u.desc.pixelSize();
            const QPoint sp = u.desc.sourceTopLeft();

            D3D12_BOX srcBox;
            srcBox.left = UINT(sp.x());
            srcBox.top = UINT(sp.y());
            srcBox.front = srcIs3D ? UINT(u.desc.sourceLayer()) : 0u;
            // back, right, bottom are exclusive
            srcBox.right = srcBox.left + UINT(copySize.width());
            srcBox.bottom = srcBox.top + UINT(copySize.height());
            srcBox.back = srcBox.front + 1;

            D3D12_TEXTURE_COPY_LOCATION src;
            src.pResource = srcRes->resource;
            src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            src.SubresourceIndex = srcSubresource;
            D3D12_TEXTURE_COPY_LOCATION dst;
            dst.pResource = dstRes->resource;
            dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dst.SubresourceIndex = dstSubresource;

            cbD->cmdList->CopyTextureRegion(&dst,
                                            UINT(dp.x()),
                                            UINT(dp.y()),
                                            dstIs3D ? UINT(u.desc.destinationLayer()) : 0u,
                                            &src,
                                            &srcBox);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Read) {
            QD3D12Readback readback;
            readback.frameSlot = currentFrameSlot;
            readback.result = u.result;

            QD3D12ObjectHandle srcHandle;
            QRect rect;
            bool is3D = false;
            if (u.rb.texture()) {
                QD3D12Texture *texD = QRHI_RES(QD3D12Texture, u.rb.texture());
                if (texD->sampleDesc.Count > 1) {
                    qWarning("Multisample texture cannot be read back");
                    continue;
                }
                is3D = texD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
                if (u.rb.rect().isValid())
                    rect = u.rb.rect();
                else
                    rect = QRect({0, 0}, q->sizeForMipLevel(u.rb.level(), texD->m_pixelSize));
                readback.format = texD->m_format;
                srcHandle = texD->handle;
            } else {
                Q_ASSERT(currentSwapChain);
                if (u.rb.rect().isValid())
                    rect = u.rb.rect();
                else
                    rect = QRect({0, 0}, currentSwapChain->pixelSize);
                readback.format = swapchainReadbackTextureFormat(currentSwapChain->colorFormat, nullptr);
                if (readback.format == QRhiTexture::UnknownFormat)
                    continue;
                srcHandle = currentSwapChain->colorBuffers[currentSwapChain->currentBackBufferIndex];
            }
            readback.pixelSize = rect.size();

            textureFormatInfo(readback.format,
                              readback.pixelSize,
                              &readback.bytesPerLine,
                              &readback.byteSize,
                              nullptr);

            QD3D12Resource *srcRes = resourcePool.lookupRef(srcHandle);
            if (!srcRes)
                continue;

            const UINT subresource = calcSubresource(UINT(u.rb.level()),
                                                     is3D ? 0u : UINT(u.rb.layer()),
                                                     srcRes->desc.MipLevels);
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
            // totalBytes is what we get from D3D, with the 256 aligned stride,
            // readback.byteSize is the final result that's not relevant here yet
            UINT64 totalBytes = 0;
            dev->GetCopyableFootprints(&srcRes->desc, subresource, 1, 0,
                                       &layout, nullptr, nullptr, &totalBytes);
            readback.stagingRowPitch = layout.Footprint.RowPitch;

            const quint32 allocSize = aligned<quint32>(totalBytes, QD3D12StagingArea::ALIGNMENT);
            if (!readback.staging.create(this, allocSize, D3D12_HEAP_TYPE_READBACK)) {
                if (u.result->completed)
                    u.result->completed();
                continue;
            }
            QD3D12StagingArea::Allocation stagingAlloc = readback.staging.get(totalBytes);
            if (!stagingAlloc.isValid()) {
                readback.staging.destroy();
                if (u.result->completed)
                    u.result->completed();
                continue;
            }
            Q_ASSERT(stagingAlloc.bufferOffset == 0);

            barrierGen.addTransitionBarrier(srcHandle, D3D12_RESOURCE_STATE_COPY_SOURCE);
            barrierGen.enqueueBufferedTransitionBarriers(cbD);

            D3D12_TEXTURE_COPY_LOCATION dst;
            dst.pResource = stagingAlloc.buffer;
            dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            dst.PlacedFootprint.Offset = 0;
            dst.PlacedFootprint.Footprint = layout.Footprint;

            D3D12_TEXTURE_COPY_LOCATION src;
            src.pResource = srcRes->resource;
            src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            src.SubresourceIndex = subresource;

            D3D12_BOX srcBox = {};
            srcBox.left = UINT(rect.left());
            srcBox.top = UINT(rect.top());
            srcBox.front = is3D ? UINT(u.rb.layer()) : 0u;
            // back, right, bottom are exclusive
            srcBox.right = srcBox.left + UINT(rect.width());
            srcBox.bottom = srcBox.top + UINT(rect.height());
            srcBox.back = srcBox.front + 1;

            cbD->cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, &srcBox);
            activeReadbacks.append(readback);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::GenMips) {
            QD3D12Texture *texD = QRHI_RES(QD3D12Texture, u.dst);
            Q_ASSERT(texD->flags().testFlag(QRhiTexture::UsedWithGenerateMips));
            if (texD->flags().testFlag(QRhiTexture::ThreeDimensional))
                mipmapGen3D.generate(cbD, texD->handle);
            else
                mipmapGen.generate(cbD, texD->handle);
        }
    }

    ud->free();
}

void QRhiD3D12::finishActiveReadbacks(bool forced)
{
    QVarLengthArray<std::function<void()>, 4> completedCallbacks;

    for (int i = activeReadbacks.size() - 1; i >= 0; --i) {
        QD3D12Readback &readback(activeReadbacks[i]);
        if (forced || currentFrameSlot == readback.frameSlot || readback.frameSlot < 0) {
            readback.result->format = readback.format;
            readback.result->pixelSize = readback.pixelSize;
            readback.result->data.resize(int(readback.byteSize));

            if (readback.format != QRhiTexture::UnknownFormat) {
                quint8 *dstPtr = reinterpret_cast<quint8 *>(readback.result->data.data());
                const quint8 *srcPtr = readback.staging.mem.p;
                const quint32 lineSize = qMin(readback.bytesPerLine, readback.stagingRowPitch);
                for (int y = 0, h = readback.pixelSize.height(); y < h; ++y)
                    memcpy(dstPtr + y * readback.bytesPerLine, srcPtr + y * readback.stagingRowPitch, lineSize);
            } else {
                memcpy(readback.result->data.data(), readback.staging.mem.p, readback.byteSize);
            }

            readback.staging.destroy();

            if (readback.result->completed)
                completedCallbacks.append(readback.result->completed);

            activeReadbacks.remove(i);
        }
    }

    for (auto f : completedCallbacks)
        f();
}

bool QRhiD3D12::ensureShaderVisibleDescriptorHeapCapacity(QD3D12ShaderVisibleDescriptorHeap *h,
                                                          D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                          int frameSlot,
                                                          quint32 neededDescriptorCount,
                                                          bool *gotNew)
{
    // Gets a new heap if needed. Note that the capacity we get is clamped
    // automatically (e.g. to 1 million, or 2048 for samplers), so * 2 does not
    // mean we can grow indefinitely, then again even using the same size would
    // work (because we what we are after here is a new heap for the rest of
    // the commands, not affecting what's already recorded).
    if (h->perFrameHeapSlice[frameSlot].remainingCapacity() < neededDescriptorCount) {
        const quint32 newPerFrameSize = qMax(h->perFrameHeapSlice[frameSlot].capacity * 2,
                                             neededDescriptorCount);
        QD3D12ShaderVisibleDescriptorHeap newHeap;
        if (!newHeap.create(dev, type, newPerFrameSize)) {
            qWarning("Could not create new shader-visible descriptor heap");
            return false;
        }
        h->destroyWithDeferredRelease(&releaseQueue);
        *h = newHeap;
        *gotNew = true;
    }
    return true;
}

void QRhiD3D12::bindShaderVisibleHeaps(QD3D12CommandBuffer *cbD)
{
    ID3D12DescriptorHeap *heaps[] = {
        shaderVisibleCbvSrvUavHeap.heap.heap,
        samplerMgr.shaderVisibleSamplerHeap.heap.heap
    };
    cbD->cmdList->SetDescriptorHeaps(2, heaps);
}

QD3D12Buffer::QD3D12Buffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size)
    : QRhiBuffer(rhi, type, usage, size)
{
}

QD3D12Buffer::~QD3D12Buffer()
{
    destroy();
}

void QD3D12Buffer::destroy()
{
    if (handles[0].isNull())
        return;

    QRHI_RES_RHI(QRhiD3D12);

    // destroy() implementations, unlike other functions, are expected to test
    // for m_rhi (rhiD) being null, to allow surviving in case one attempts to
    // destroy a (leaked) resource after the QRhi.
    //
    // If there is no QRhi anymore, we do not deferred-release but that's fine
    // since the QRhi already released everything that was in the resourcePool.

    for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
        if (rhiD)
            rhiD->releaseQueue.deferredReleaseResource(handles[i]);
        handles[i] = {};
        pendingHostWrites[i].clear();
    }

    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QD3D12Buffer::create()
{
    if (!handles[0].isNull())
        destroy();

    if (m_usage.testFlag(QRhiBuffer::UniformBuffer) && m_type != Dynamic) {
        qWarning("UniformBuffer must always be Dynamic");
        return false;
    }

    if (m_usage.testFlag(QRhiBuffer::StorageBuffer) && m_type == Dynamic) {
        qWarning("StorageBuffer cannot be combined with Dynamic");
        return false;
    }

    const quint32 nonZeroSize = m_size <= 0 ? 256 : m_size;
    const quint32 roundedSize = aligned(nonZeroSize, m_usage.testFlag(QRhiBuffer::UniformBuffer) ? 256u : 4u);

    UINT resourceFlags = D3D12_RESOURCE_FLAG_NONE;
    if (m_usage.testFlag(QRhiBuffer::StorageBuffer))
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    QRHI_RES_RHI(QRhiD3D12);
    HRESULT hr = 0;
    for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
        if (i == 0 || m_type == Dynamic) {
            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Width = roundedSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc = { 1, 0 };
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAGS(resourceFlags);
            ID3D12Resource *resource = nullptr;
            D3D12MA::Allocation *allocation = nullptr;
            // Dynamic == host (CPU) visible
            D3D12_HEAP_TYPE heapType = m_type == Dynamic
                    ? D3D12_HEAP_TYPE_UPLOAD
                    : D3D12_HEAP_TYPE_DEFAULT;
            D3D12_RESOURCE_STATES resourceState = m_type == Dynamic
                    ? D3D12_RESOURCE_STATE_GENERIC_READ
                    : D3D12_RESOURCE_STATE_COMMON;
            hr = rhiD->vma.createResource(heapType,
                                          &resourceDesc,
                                          resourceState,
                                          nullptr,
                                          &allocation,
                                          __uuidof(resource),
                                          reinterpret_cast<void **>(&resource));
            if (FAILED(hr))
                break;
            if (!m_objectName.isEmpty()) {
                QString decoratedName = QString::fromUtf8(m_objectName);
                if (m_type == Dynamic) {
                    decoratedName += QLatin1Char('/');
                    decoratedName += QString::number(i);
                }
                resource->SetName(reinterpret_cast<LPCWSTR>(decoratedName.utf16()));
            }
            void *cpuMemPtr = nullptr;
            if (m_type == Dynamic) {
                // will be mapped for ever on the CPU, this makes future host write operations very simple
                hr = resource->Map(0, nullptr, &cpuMemPtr);
                if (FAILED(hr)) {
                    qWarning("Map() failed to dynamic buffer");
                    resource->Release();
                    if (allocation)
                        allocation->Release();
                    break;
                }
            }
            handles[i] = QD3D12Resource::addToPool(&rhiD->resourcePool,
                                                   resource,
                                                   resourceState,
                                                   allocation,
                                                   cpuMemPtr);
        }
    }
    if (FAILED(hr)) {
        qWarning("Failed to create buffer: '%s' Type was %d, size was %u, using D3D12MA was %d.",
                 qPrintable(QSystemError::windowsComString(hr)),
                 int(m_type),
                 roundedSize,
                 int(rhiD->vma.isUsingD3D12MA()));
        return false;
    }

    rhiD->registerResource(this);
    return true;
}

QRhiBuffer::NativeBuffer QD3D12Buffer::nativeBuffer()
{
    NativeBuffer b;
    Q_ASSERT(sizeof(b.objects) / sizeof(b.objects[0]) >= size_t(QD3D12_FRAMES_IN_FLIGHT));
    QRHI_RES_RHI(QRhiD3D12);
    if (m_type == Dynamic) {
        for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
            executeHostWritesForFrameSlot(i);
            if (QD3D12Resource *res = rhiD->resourcePool.lookupRef(handles[i]))
                b.objects[i] = res->resource;
            else
                b.objects[i] = nullptr;
        }
        b.slotCount = QD3D12_FRAMES_IN_FLIGHT;
        return b;
    }
    if (QD3D12Resource *res = rhiD->resourcePool.lookupRef(handles[0]))
        b.objects[0] = res->resource;
    else
        b.objects[0] = nullptr;
    b.slotCount = 1;
    return b;
}

char *QD3D12Buffer::beginFullDynamicBufferUpdateForCurrentFrame()
{
    // Shortcut the entire buffer update mechanism and allow the client to do
    // the host writes directly to the buffer. This will lead to unexpected
    // results when combined with QRhiResourceUpdateBatch-based updates for the
    // buffer, but provides a fast path for dynamic buffers that have all their
    // content changed in every frame.

    Q_ASSERT(m_type == Dynamic);
    QRHI_RES_RHI(QRhiD3D12);
    Q_ASSERT(rhiD->inFrame);
    if (QD3D12Resource *res = rhiD->resourcePool.lookupRef(handles[rhiD->currentFrameSlot]))
        return static_cast<char *>(res->cpuMapPtr);

    return nullptr;
}

void QD3D12Buffer::endFullDynamicBufferUpdateForCurrentFrame()
{
    // nothing to do here
}

void QD3D12Buffer::executeHostWritesForFrameSlot(int frameSlot)
{
    if (pendingHostWrites[frameSlot].isEmpty())
        return;

    Q_ASSERT(m_type == QRhiBuffer::Dynamic);
    QRHI_RES_RHI(QRhiD3D12);
    if (QD3D12Resource *res = rhiD->resourcePool.lookupRef(handles[frameSlot])) {
        Q_ASSERT(res->cpuMapPtr);
        for (const QD3D12Buffer::HostWrite &u : std::as_const(pendingHostWrites[frameSlot]))
            memcpy(static_cast<char *>(res->cpuMapPtr) + u.offset, u.data.constData(), u.data.size());
    }
    pendingHostWrites[frameSlot].clear();
}

static inline DXGI_FORMAT toD3DTextureFormat(QRhiTexture::Format format, QRhiTexture::Flags flags)
{
    const bool srgb = flags.testFlag(QRhiTexture::sRGB);
    switch (format) {
    case QRhiTexture::RGBA8:
        return srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    case QRhiTexture::BGRA8:
        return srgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
    case QRhiTexture::R8:
        return DXGI_FORMAT_R8_UNORM;
    case QRhiTexture::R8SI:
        return DXGI_FORMAT_R8_SINT;
    case QRhiTexture::R8UI:
        return DXGI_FORMAT_R8_UINT;
    case QRhiTexture::RG8:
        return DXGI_FORMAT_R8G8_UNORM;
    case QRhiTexture::R16:
        return DXGI_FORMAT_R16_UNORM;
    case QRhiTexture::RG16:
        return DXGI_FORMAT_R16G16_UNORM;
    case QRhiTexture::RED_OR_ALPHA8:
        return DXGI_FORMAT_R8_UNORM;

    case QRhiTexture::RGBA16F:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case QRhiTexture::RGBA32F:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case QRhiTexture::R16F:
        return DXGI_FORMAT_R16_FLOAT;
    case QRhiTexture::R32F:
        return DXGI_FORMAT_R32_FLOAT;

    case QRhiTexture::RGB10A2:
        return DXGI_FORMAT_R10G10B10A2_UNORM;

    case QRhiTexture::R32SI:
        return DXGI_FORMAT_R32_SINT;
    case QRhiTexture::R32UI:
        return DXGI_FORMAT_R32_UINT;
    case QRhiTexture::RG32SI:
        return DXGI_FORMAT_R32G32_SINT;
    case QRhiTexture::RG32UI:
        return DXGI_FORMAT_R32G32_UINT;
    case QRhiTexture::RGBA32SI:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case QRhiTexture::RGBA32UI:
        return DXGI_FORMAT_R32G32B32A32_UINT;

    case QRhiTexture::D16:
        return DXGI_FORMAT_R16_TYPELESS;
    case QRhiTexture::D24:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case QRhiTexture::D24S8:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case QRhiTexture::D32F:
        return DXGI_FORMAT_R32_TYPELESS;
    case QRhiTexture::Format::D32FS8:
        return DXGI_FORMAT_R32G8X24_TYPELESS;

    case QRhiTexture::BC1:
        return srgb ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
    case QRhiTexture::BC2:
        return srgb ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
    case QRhiTexture::BC3:
        return srgb ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
    case QRhiTexture::BC4:
        return DXGI_FORMAT_BC4_UNORM;
    case QRhiTexture::BC5:
        return DXGI_FORMAT_BC5_UNORM;
    case QRhiTexture::BC6H:
        return DXGI_FORMAT_BC6H_UF16;
    case QRhiTexture::BC7:
        return srgb ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;

    case QRhiTexture::ETC2_RGB8:
    case QRhiTexture::ETC2_RGB8A1:
    case QRhiTexture::ETC2_RGBA8:
        qWarning("QRhiD3D12 does not support ETC2 textures");
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case QRhiTexture::ASTC_4x4:
    case QRhiTexture::ASTC_5x4:
    case QRhiTexture::ASTC_5x5:
    case QRhiTexture::ASTC_6x5:
    case QRhiTexture::ASTC_6x6:
    case QRhiTexture::ASTC_8x5:
    case QRhiTexture::ASTC_8x6:
    case QRhiTexture::ASTC_8x8:
    case QRhiTexture::ASTC_10x5:
    case QRhiTexture::ASTC_10x6:
    case QRhiTexture::ASTC_10x8:
    case QRhiTexture::ASTC_10x10:
    case QRhiTexture::ASTC_12x10:
    case QRhiTexture::ASTC_12x12:
        qWarning("QRhiD3D12 does not support ASTC textures");
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    default:
        break;
    }
    return DXGI_FORMAT_R8G8B8A8_UNORM;
}

QD3D12RenderBuffer::QD3D12RenderBuffer(QRhiImplementation *rhi,
                                       Type type,
                                       const QSize &pixelSize,
                                       int sampleCount,
                                       Flags flags,
                                       QRhiTexture::Format backingFormatHint)
    : QRhiRenderBuffer(rhi, type, pixelSize, sampleCount, flags, backingFormatHint)
{
}

QD3D12RenderBuffer::~QD3D12RenderBuffer()
{
    destroy();
}

void QD3D12RenderBuffer::destroy()
{
    if (handle.isNull())
        return;

    QRHI_RES_RHI(QRhiD3D12);
    if (rhiD) {
        if (rtv.isValid())
            rhiD->releaseQueue.deferredReleaseResourceWithViews(handle, &rhiD->rtvPool, rtv, 1);
        else if (dsv.isValid())
            rhiD->releaseQueue.deferredReleaseResourceWithViews(handle, &rhiD->dsvPool, dsv, 1);
    }

    handle = {};
    rtv = {};
    dsv = {};

    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QD3D12RenderBuffer::create()
{
    if (!handle.isNull())
        destroy();

    if (m_pixelSize.isEmpty())
        return false;

    QRHI_RES_RHI(QRhiD3D12);

    switch (m_type) {
    case QRhiRenderBuffer::Color:
    {
        dxgiFormat = toD3DTextureFormat(backingFormat(), {});
        sampleDesc = rhiD->effectiveSampleDesc(m_sampleCount, dxgiFormat);
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Width = UINT64(m_pixelSize.width());
        resourceDesc.Height = UINT(m_pixelSize.height());
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = dxgiFormat;
        resourceDesc.SampleDesc = sampleDesc;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = dxgiFormat;
        // have a separate allocation and resource object (meaning both will need its own Release())
        ID3D12Resource *resource = nullptr;
        D3D12MA::Allocation *allocation = nullptr;
        HRESULT hr = rhiD->vma.createResource(D3D12_HEAP_TYPE_DEFAULT,
                                              &resourceDesc,
                                              D3D12_RESOURCE_STATE_RENDER_TARGET,
                                              &clearValue,
                                              &allocation,
                                              __uuidof(ID3D12Resource),
                                              reinterpret_cast<void **>(&resource));
        if (FAILED(hr)) {
            qWarning("Failed to create color buffer: %s", qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
        handle = QD3D12Resource::addToPool(&rhiD->resourcePool, resource, D3D12_RESOURCE_STATE_RENDER_TARGET, allocation);
        rtv = rhiD->rtvPool.allocate(1);
        if (!rtv.isValid())
            return false;
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = dxgiFormat;
        rtvDesc.ViewDimension = sampleDesc.Count > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DMS
                                                     : D3D12_RTV_DIMENSION_TEXTURE2D;
        rhiD->dev->CreateRenderTargetView(resource, &rtvDesc, rtv.cpuHandle);
    }
        break;
    case QRhiRenderBuffer::DepthStencil:
    {
        dxgiFormat = DS_FORMAT;
        sampleDesc = rhiD->effectiveSampleDesc(m_sampleCount, dxgiFormat);
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Width = UINT64(m_pixelSize.width());
        resourceDesc.Height = UINT(m_pixelSize.height());
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = dxgiFormat;
        resourceDesc.SampleDesc = sampleDesc;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if (m_flags.testFlag(UsedWithSwapChainOnly))
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = dxgiFormat;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        ID3D12Resource *resource = nullptr;
        D3D12MA::Allocation *allocation = nullptr;
        HRESULT hr = rhiD->vma.createResource(D3D12_HEAP_TYPE_DEFAULT,
                                              &resourceDesc,
                                              D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                              &clearValue,
                                              &allocation,
                                              __uuidof(ID3D12Resource),
                                              reinterpret_cast<void **>(&resource));
        if (FAILED(hr)) {
            qWarning("Failed to create depth-stencil buffer: %s", qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
        handle = QD3D12Resource::addToPool(&rhiD->resourcePool, resource, D3D12_RESOURCE_STATE_DEPTH_WRITE, allocation);
        dsv = rhiD->dsvPool.allocate(1);
        if (!dsv.isValid())
            return false;
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = dxgiFormat;
        dsvDesc.ViewDimension = sampleDesc.Count > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DMS
                                                     : D3D12_DSV_DIMENSION_TEXTURE2D;
        rhiD->dev->CreateDepthStencilView(resource, &dsvDesc, dsv.cpuHandle);
    }
        break;
    }

    if (!m_objectName.isEmpty()) {
        if (QD3D12Resource *res = rhiD->resourcePool.lookupRef(handle)) {
            const QString name = QString::fromUtf8(m_objectName);
            res->resource->SetName(reinterpret_cast<LPCWSTR>(name.utf16()));
        }
    }

    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::Format QD3D12RenderBuffer::backingFormat() const
{
    if (m_backingFormatHint != QRhiTexture::UnknownFormat)
        return m_backingFormatHint;
    else
        return m_type == Color ? QRhiTexture::RGBA8 : QRhiTexture::UnknownFormat;
}

QD3D12Texture::QD3D12Texture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
                             int arraySize, int sampleCount, Flags flags)
    : QRhiTexture(rhi, format, pixelSize, depth, arraySize, sampleCount, flags)
{
}

QD3D12Texture::~QD3D12Texture()
{
    destroy();
}

void QD3D12Texture::destroy()
{
    if (handle.isNull())
        return;

    QRHI_RES_RHI(QRhiD3D12);
    if (rhiD)
        rhiD->releaseQueue.deferredReleaseResourceWithViews(handle, &rhiD->cbvSrvUavPool, srv, 1);

    handle = {};
    srv = {};

    if (rhiD)
        rhiD->unregisterResource(this);
}

static inline DXGI_FORMAT toD3DDepthTextureSRVFormat(QRhiTexture::Format format)
{
    switch (format) {
    case QRhiTexture::Format::D16:
        return DXGI_FORMAT_R16_FLOAT;
    case QRhiTexture::Format::D24:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case QRhiTexture::Format::D24S8:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case QRhiTexture::Format::D32F:
        return DXGI_FORMAT_R32_FLOAT;
    case QRhiTexture::Format::D32FS8:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    default:
        break;
    }
    Q_UNREACHABLE_RETURN(DXGI_FORMAT_R32_FLOAT);
}

static inline DXGI_FORMAT toD3DDepthTextureDSVFormat(QRhiTexture::Format format)
{
    // here the result cannot be typeless
    switch (format) {
    case QRhiTexture::Format::D16:
        return DXGI_FORMAT_D16_UNORM;
    case QRhiTexture::Format::D24:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case QRhiTexture::Format::D24S8:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case QRhiTexture::Format::D32F:
        return DXGI_FORMAT_D32_FLOAT;
    case QRhiTexture::Format::D32FS8:
        return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    default:
        break;
    }
    Q_UNREACHABLE_RETURN(DXGI_FORMAT_D32_FLOAT);
}

static inline bool isDepthTextureFormat(QRhiTexture::Format format)
{
    switch (format) {
    case QRhiTexture::Format::D16:
    case QRhiTexture::Format::D24:
    case QRhiTexture::Format::D24S8:
    case QRhiTexture::Format::D32F:
    case QRhiTexture::Format::D32FS8:
        return true;
    default:
        return false;
    }
}

bool QD3D12Texture::prepareCreate(QSize *adjustedSize)
{
    if (!handle.isNull())
        destroy();

    QRHI_RES_RHI(QRhiD3D12);
    if (!rhiD->isTextureFormatSupported(m_format, m_flags))
        return false;

    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);
    const bool is1D = m_flags.testFlag(OneDimensional);

    const QSize size = is1D ? QSize(qMax(1, m_pixelSize.width()), 1)
                            : (m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize);

    dxgiFormat = toD3DTextureFormat(m_format, m_flags);
    if (isDepth) {
        srvFormat = toD3DDepthTextureSRVFormat(m_format);
        rtFormat = toD3DDepthTextureDSVFormat(m_format);
    } else {
        srvFormat = dxgiFormat;
        rtFormat = dxgiFormat;
    }
    if (m_writeViewFormat.format != UnknownFormat) {
        if (isDepth)
            rtFormat = toD3DDepthTextureDSVFormat(m_writeViewFormat.format);
        else
            rtFormat = toD3DTextureFormat(m_writeViewFormat.format, m_writeViewFormat.srgb ? sRGB : Flags());
    }
    if (m_readViewFormat.format != UnknownFormat) {
        if (isDepth)
            srvFormat = toD3DDepthTextureSRVFormat(m_readViewFormat.format);
        else
            srvFormat = toD3DTextureFormat(m_readViewFormat.format, m_readViewFormat.srgb ? sRGB : Flags());
    }

    mipLevelCount = uint(hasMipMaps ? rhiD->q->mipLevelsForSize(size) : 1);
    sampleDesc = rhiD->effectiveSampleDesc(m_sampleCount, dxgiFormat);
    if (sampleDesc.Count > 1) {
        if (isCube) {
            qWarning("Cubemap texture cannot be multisample");
            return false;
        }
        if (is3D) {
            qWarning("3D texture cannot be multisample");
            return false;
        }
        if (hasMipMaps) {
            qWarning("Multisample texture cannot have mipmaps");
            return false;
        }
    }
    if (isDepth && hasMipMaps) {
        qWarning("Depth texture cannot have mipmaps");
        return false;
    }
    if (isCube && is3D) {
        qWarning("Texture cannot be both cube and 3D");
        return false;
    }
    if (isArray && is3D) {
        qWarning("Texture cannot be both array and 3D");
        return false;
    }
    if (isCube && is1D) {
        qWarning("Texture cannot be both cube and 1D");
        return false;
    }
    if (is1D && is3D) {
        qWarning("Texture cannot be both 1D and 3D");
        return false;
    }
    if (m_depth > 1 && !is3D) {
        qWarning("Texture cannot have a depth of %d when it is not 3D", m_depth);
        return false;
    }
    if (m_arraySize > 0 && !isArray) {
        qWarning("Texture cannot have an array size of %d when it is not an array", m_arraySize);
        return false;
    }
    if (m_arraySize < 1 && isArray) {
        qWarning("Texture is an array but array size is %d", m_arraySize);
        return false;
    }

    if (adjustedSize)
        *adjustedSize = size;

    return true;
}

bool QD3D12Texture::finishCreate()
{
    QRHI_RES_RHI(QRhiD3D12);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool is1D = m_flags.testFlag(OneDimensional);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = srvFormat;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (isCube) {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = mipLevelCount;
    } else {
        if (is1D) {
            if (isArray) {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                srvDesc.Texture1DArray.MipLevels = mipLevelCount;
                if (m_arrayRangeStart >= 0 && m_arrayRangeLength >= 0) {
                    srvDesc.Texture1DArray.FirstArraySlice = UINT(m_arrayRangeStart);
                    srvDesc.Texture1DArray.ArraySize = UINT(m_arrayRangeLength);
                } else {
                    srvDesc.Texture1DArray.FirstArraySlice = 0;
                    srvDesc.Texture1DArray.ArraySize = UINT(qMax(0, m_arraySize));
                }
            } else {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                srvDesc.Texture1D.MipLevels = mipLevelCount;
            }
        } else if (isArray) {
            if (sampleDesc.Count > 1) {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                if (m_arrayRangeStart >= 0 && m_arrayRangeLength >= 0) {
                    srvDesc.Texture2DMSArray.FirstArraySlice = UINT(m_arrayRangeStart);
                    srvDesc.Texture2DMSArray.ArraySize = UINT(m_arrayRangeLength);
                } else {
                    srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                    srvDesc.Texture2DMSArray.ArraySize = UINT(qMax(0, m_arraySize));
                }
            } else {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MipLevels = mipLevelCount;
                if (m_arrayRangeStart >= 0 && m_arrayRangeLength >= 0) {
                    srvDesc.Texture2DArray.FirstArraySlice = UINT(m_arrayRangeStart);
                    srvDesc.Texture2DArray.ArraySize = UINT(m_arrayRangeLength);
                } else {
                    srvDesc.Texture2DArray.FirstArraySlice = 0;
                    srvDesc.Texture2DArray.ArraySize = UINT(qMax(0, m_arraySize));
                }
            }
        } else {
            if (sampleDesc.Count > 1) {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            } else if (is3D) {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MipLevels = mipLevelCount;
            } else {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = mipLevelCount;
            }
        }
    }

    srv = rhiD->cbvSrvUavPool.allocate(1);
    if (!srv.isValid())
        return false;

    if (QD3D12Resource *res = rhiD->resourcePool.lookupRef(handle)) {
        rhiD->dev->CreateShaderResourceView(res->resource, &srvDesc, srv.cpuHandle);
        if (!m_objectName.isEmpty()) {
            const QString name = QString::fromUtf8(m_objectName);
            res->resource->SetName(reinterpret_cast<LPCWSTR>(name.utf16()));
        }
    } else {
        return false;
    }

    generation += 1;
    return true;
}

bool QD3D12Texture::create()
{
    QSize size;
    if (!prepareCreate(&size))
        return false;

    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool is1D = m_flags.testFlag(OneDimensional);

    QRHI_RES_RHI(QRhiD3D12);

    bool needsOptimizedClearValueSpecified = false;
    UINT resourceFlags = 0;
    if (m_flags.testFlag(RenderTarget) || sampleDesc.Count > 1) {
        if (isDepth)
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        else
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        needsOptimizedClearValueSpecified = true;
    }
    if (m_flags.testFlag(UsedWithGenerateMips)) {
        if (isDepth) {
            qWarning("Depth texture cannot have mipmaps generated");
            return false;
        }
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (m_flags.testFlag(UsedWithLoadStore))
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = is1D ? D3D12_RESOURCE_DIMENSION_TEXTURE1D
                                  : (is3D ? D3D12_RESOURCE_DIMENSION_TEXTURE3D
                                          : D3D12_RESOURCE_DIMENSION_TEXTURE2D);
    resourceDesc.Width = UINT64(size.width());
    resourceDesc.Height = UINT(size.height());
    resourceDesc.DepthOrArraySize = isCube ? 6
                                           : (isArray ? UINT(qMax(0, m_arraySize))
                                                      : (is3D ? qMax(1, m_depth)
                                                              : 1));
    resourceDesc.MipLevels = mipLevelCount;
    resourceDesc.Format = dxgiFormat;
    resourceDesc.SampleDesc = sampleDesc;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAGS(resourceFlags);
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = dxgiFormat;
    if (isDepth) {
        clearValue.Format = toD3DDepthTextureDSVFormat(m_format);
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
    }
    ID3D12Resource *resource = nullptr;
    D3D12MA::Allocation *allocation = nullptr;
    HRESULT hr = rhiD->vma.createResource(D3D12_HEAP_TYPE_DEFAULT,
                                          &resourceDesc,
                                          D3D12_RESOURCE_STATE_COMMON,
                                          needsOptimizedClearValueSpecified ? &clearValue : nullptr,
                                          &allocation,
                                          __uuidof(ID3D12Resource),
                                          reinterpret_cast<void **>(&resource));
    if (FAILED(hr)) {
        qWarning("Failed to create texture: '%s'"
                 " Dim was %d Size was %ux%u Depth/ArraySize was %u MipLevels was %u Format was %d Sample count was %d",
                 qPrintable(QSystemError::windowsComString(hr)),
                 int(resourceDesc.Dimension),
                 uint(resourceDesc.Width),
                 uint(resourceDesc.Height),
                 uint(resourceDesc.DepthOrArraySize),
                 uint(resourceDesc.MipLevels),
                 int(resourceDesc.Format),
                 int(resourceDesc.SampleDesc.Count));
        return false;
    }

    handle = QD3D12Resource::addToPool(&rhiD->resourcePool, resource, D3D12_RESOURCE_STATE_COMMON, allocation);

    if (!finishCreate())
        return false;

    rhiD->registerResource(this);
    return true;
}

bool QD3D12Texture::createFrom(QRhiTexture::NativeTexture src)
{
    if (!src.object)
        return false;

    if (!prepareCreate())
        return false;

    ID3D12Resource *resource = reinterpret_cast<ID3D12Resource *>(src.object);
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATES(src.layout);

    QRHI_RES_RHI(QRhiD3D12);
    handle = QD3D12Resource::addNonOwningToPool(&rhiD->resourcePool, resource, state);

    if (!finishCreate())
        return false;

    rhiD->registerResource(this);
    return true;
}

QRhiTexture::NativeTexture QD3D12Texture::nativeTexture()
{
    QRHI_RES_RHI(QRhiD3D12);
    if (QD3D12Resource *res = rhiD->resourcePool.lookupRef(handle))
        return { quint64(res->resource), int(res->state) };

    return {};
}

void QD3D12Texture::setNativeLayout(int layout)
{
    QRHI_RES_RHI(QRhiD3D12);
    if (QD3D12Resource *res = rhiD->resourcePool.lookupRef(handle))
        res->state = D3D12_RESOURCE_STATES(layout);
}

QD3D12Sampler::QD3D12Sampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                             AddressMode u, AddressMode v, AddressMode w)
    : QRhiSampler(rhi, magFilter, minFilter, mipmapMode, u, v, w)
{
}

QD3D12Sampler::~QD3D12Sampler()
{
    destroy();
}

void QD3D12Sampler::destroy()
{
    shaderVisibleDescriptor = {};

    QRHI_RES_RHI(QRhiD3D12);
    if (rhiD)
        rhiD->unregisterResource(this);
}

static inline D3D12_FILTER toD3DFilter(QRhiSampler::Filter minFilter, QRhiSampler::Filter magFilter, QRhiSampler::Filter mipFilter)
{
    if (minFilter == QRhiSampler::Nearest) {
        if (magFilter == QRhiSampler::Nearest) {
            if (mipFilter == QRhiSampler::Linear)
                return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            else
                return D3D12_FILTER_MIN_MAG_MIP_POINT;
        } else {
            if (mipFilter == QRhiSampler::Linear)
                return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
            else
                return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        }
    } else {
        if (magFilter == QRhiSampler::Nearest) {
            if (mipFilter == QRhiSampler::Linear)
                return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            else
                return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        } else {
            if (mipFilter == QRhiSampler::Linear)
                return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            else
                return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        }
    }
    Q_UNREACHABLE_RETURN(D3D12_FILTER_MIN_MAG_MIP_LINEAR);
}

static inline D3D12_TEXTURE_ADDRESS_MODE toD3DAddressMode(QRhiSampler::AddressMode m)
{
    switch (m) {
    case QRhiSampler::Repeat:
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case QRhiSampler::ClampToEdge:
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case QRhiSampler::Mirror:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    }
    Q_UNREACHABLE_RETURN(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
}

static inline D3D12_COMPARISON_FUNC toD3DTextureComparisonFunc(QRhiSampler::CompareOp op)
{
    switch (op) {
    case QRhiSampler::Never:
        return D3D12_COMPARISON_FUNC_NEVER;
    case QRhiSampler::Less:
        return D3D12_COMPARISON_FUNC_LESS;
    case QRhiSampler::Equal:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case QRhiSampler::LessOrEqual:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case QRhiSampler::Greater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case QRhiSampler::NotEqual:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case QRhiSampler::GreaterOrEqual:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case QRhiSampler::Always:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    }
    Q_UNREACHABLE_RETURN(D3D12_COMPARISON_FUNC_NEVER);
}

bool QD3D12Sampler::create()
{
    desc = {};
    desc.Filter = toD3DFilter(m_minFilter, m_magFilter, m_mipmapMode);
    if (m_compareOp != Never)
        desc.Filter = D3D12_FILTER(desc.Filter | 0x80);
    desc.AddressU = toD3DAddressMode(m_addressU);
    desc.AddressV = toD3DAddressMode(m_addressV);
    desc.AddressW = toD3DAddressMode(m_addressW);
    desc.MaxAnisotropy = 1.0f;
    desc.ComparisonFunc = toD3DTextureComparisonFunc(m_compareOp);
    desc.MaxLOD = m_mipmapMode == None ? 0.0f : 10000.0f;

    QRHI_RES_RHI(QRhiD3D12);
    rhiD->registerResource(this, false);
    return true;
}

QD3D12Descriptor QD3D12Sampler::lookupOrCreateShaderVisibleDescriptor()
{
    if (!shaderVisibleDescriptor.isValid()) {
        QRHI_RES_RHI(QRhiD3D12);
        shaderVisibleDescriptor = rhiD->samplerMgr.getShaderVisibleDescriptor(desc);
    }
    return shaderVisibleDescriptor;
}

QD3D12ShadingRateMap::QD3D12ShadingRateMap(QRhiImplementation *rhi)
    : QRhiShadingRateMap(rhi)
{
}

QD3D12ShadingRateMap::~QD3D12ShadingRateMap()
{
    destroy();
}

void QD3D12ShadingRateMap::destroy()
{
    if (handle.isNull())
        return;

    handle = {};
}

bool QD3D12ShadingRateMap::createFrom(QRhiTexture *src)
{
    if (!handle.isNull())
        destroy();

    handle = QRHI_RES(QD3D12Texture, src)->handle;

    return true;
}

QD3D12TextureRenderTarget::QD3D12TextureRenderTarget(QRhiImplementation *rhi,
                                                     const QRhiTextureRenderTargetDescription &desc,
                                                     Flags flags)
    : QRhiTextureRenderTarget(rhi, desc, flags),
      d(rhi)
{
}

QD3D12TextureRenderTarget::~QD3D12TextureRenderTarget()
{
    destroy();
}

void QD3D12TextureRenderTarget::destroy()
{
    if (!rtv[0].isValid() && !dsv.isValid())
        return;

    QRHI_RES_RHI(QRhiD3D12);
    if (dsv.isValid()) {
        if (ownsDsv && rhiD)
            rhiD->releaseQueue.deferredReleaseViews(&rhiD->dsvPool, dsv, 1);
        dsv = {};
    }

    for (int i = 0; i < QD3D12RenderTargetData::MAX_COLOR_ATTACHMENTS; ++i) {
        if (rtv[i].isValid()) {
            if (ownsRtv[i] && rhiD)
                rhiD->releaseQueue.deferredReleaseViews(&rhiD->rtvPool, rtv[i], 1);
            rtv[i] = {};
        }
    }

    if (rhiD)
        rhiD->unregisterResource(this);
}

QRhiRenderPassDescriptor *QD3D12TextureRenderTarget::newCompatibleRenderPassDescriptor()
{
    // not yet built so cannot rely on data computed in create()

    QD3D12RenderPassDescriptor *rpD = new QD3D12RenderPassDescriptor(m_rhi);

    rpD->colorAttachmentCount = 0;
    for (auto it = m_desc.cbeginColorAttachments(), itEnd = m_desc.cendColorAttachments(); it != itEnd; ++it) {
        QD3D12Texture *texD = QRHI_RES(QD3D12Texture, it->texture());
        QD3D12RenderBuffer *rbD = QRHI_RES(QD3D12RenderBuffer, it->renderBuffer());
        if (texD)
            rpD->colorFormat[rpD->colorAttachmentCount] = texD->rtFormat;
        else if (rbD)
            rpD->colorFormat[rpD->colorAttachmentCount] = rbD->dxgiFormat;
        rpD->colorAttachmentCount += 1;
    }

    rpD->hasDepthStencil = false;
    if (m_desc.depthStencilBuffer()) {
        rpD->hasDepthStencil = true;
        rpD->dsFormat = QD3D12RenderBuffer::DS_FORMAT;
    } else if (m_desc.depthTexture()) {
        QD3D12Texture *depthTexD = QRHI_RES(QD3D12Texture, m_desc.depthTexture());
        rpD->hasDepthStencil = true;
        rpD->dsFormat = toD3DDepthTextureDSVFormat(depthTexD->format()); // cannot be a typeless format
    }

    rpD->hasShadingRateMap = m_desc.shadingRateMap() != nullptr;

    rpD->updateSerializedFormat();

    QRHI_RES_RHI(QRhiD3D12);
    rhiD->registerResource(rpD);
    return rpD;
}

bool QD3D12TextureRenderTarget::create()
{
    if (rtv[0].isValid() || dsv.isValid())
        destroy();

    QRHI_RES_RHI(QRhiD3D12);
    Q_ASSERT(m_desc.colorAttachmentCount() > 0 || m_desc.depthTexture());
    Q_ASSERT(!m_desc.depthStencilBuffer() || !m_desc.depthTexture());
    const bool hasDepthStencil = m_desc.depthStencilBuffer() || m_desc.depthTexture();
    d.colorAttCount = 0;
    int attIndex = 0;

    for (auto it = m_desc.cbeginColorAttachments(), itEnd = m_desc.cendColorAttachments(); it != itEnd; ++it, ++attIndex) {
        d.colorAttCount += 1;
        const QRhiColorAttachment &colorAtt(*it);
        QRhiTexture *texture = colorAtt.texture();
        QRhiRenderBuffer *rb = colorAtt.renderBuffer();
        Q_ASSERT(texture || rb);
        if (texture) {
            QD3D12Texture *texD = QRHI_RES(QD3D12Texture, texture);
            QD3D12Resource *res = rhiD->resourcePool.lookupRef(texD->handle);
            if (!res) {
                qWarning("Could not look up texture handle for render target");
                return false;
            }
            const bool isMultiView = it->multiViewCount() >= 2;
            UINT layerCount = isMultiView ? UINT(it->multiViewCount()) : 1;
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = texD->rtFormat;
            if (texD->flags().testFlag(QRhiTexture::CubeMap)) {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = UINT(colorAtt.level());
                rtvDesc.Texture2DArray.FirstArraySlice = UINT(colorAtt.layer());
                rtvDesc.Texture2DArray.ArraySize = layerCount;
            } else if (texD->flags().testFlag(QRhiTexture::OneDimensional)) {
                if (texD->flags().testFlag(QRhiTexture::TextureArray)) {
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                    rtvDesc.Texture1DArray.MipSlice = UINT(colorAtt.level());
                    rtvDesc.Texture1DArray.FirstArraySlice = UINT(colorAtt.layer());
                    rtvDesc.Texture1DArray.ArraySize = layerCount;
                } else {
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                    rtvDesc.Texture1D.MipSlice = UINT(colorAtt.level());
                }
            } else if (texD->flags().testFlag(QRhiTexture::TextureArray)) {
                if (texD->sampleDesc.Count > 1) {
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    rtvDesc.Texture2DMSArray.FirstArraySlice = UINT(colorAtt.layer());
                    rtvDesc.Texture2DMSArray.ArraySize = layerCount;
                } else {
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtvDesc.Texture2DArray.MipSlice = UINT(colorAtt.level());
                    rtvDesc.Texture2DArray.FirstArraySlice = UINT(colorAtt.layer());
                    rtvDesc.Texture2DArray.ArraySize = layerCount;
                }
            } else if (texD->flags().testFlag(QRhiTexture::ThreeDimensional)) {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
                rtvDesc.Texture3D.MipSlice = UINT(colorAtt.level());
                rtvDesc.Texture3D.FirstWSlice = UINT(colorAtt.layer());
                rtvDesc.Texture3D.WSize = layerCount;
            } else {
                if (texD->sampleDesc.Count > 1) {
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                } else {
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                    rtvDesc.Texture2D.MipSlice = UINT(colorAtt.level());
                }
            }
            rtv[attIndex] = rhiD->rtvPool.allocate(1);
            if (!rtv[attIndex].isValid()) {
                qWarning("Failed to allocate RTV for texture render target");
                return false;
            }
            rhiD->dev->CreateRenderTargetView(res->resource, &rtvDesc, rtv[attIndex].cpuHandle);
            ownsRtv[attIndex] = true;
            if (attIndex == 0) {
                d.pixelSize = rhiD->q->sizeForMipLevel(colorAtt.level(), texD->pixelSize());
                d.sampleCount = int(texD->sampleDesc.Count);
            }
        } else if (rb) {
            QD3D12RenderBuffer *rbD = QRHI_RES(QD3D12RenderBuffer, rb);
            ownsRtv[attIndex] = false;
            rtv[attIndex] = rbD->rtv;
            if (attIndex == 0) {
                d.pixelSize = rbD->pixelSize();
                d.sampleCount = int(rbD->sampleDesc.Count);
            }
        }
    }

    d.dpr = 1;

    if (hasDepthStencil) {
        if (m_desc.depthTexture()) {
            ownsDsv = true;
            QD3D12Texture *depthTexD = QRHI_RES(QD3D12Texture, m_desc.depthTexture());
            QD3D12Resource *res = rhiD->resourcePool.lookupRef(depthTexD->handle);
            if (!res) {
                qWarning("Could not look up depth texture handle");
                return false;
            }
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = depthTexD->rtFormat;
            dsvDesc.ViewDimension = depthTexD->sampleDesc.Count > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DMS
                                                                    : D3D12_DSV_DIMENSION_TEXTURE2D;
            if (depthTexD->flags().testFlag(QRhiTexture::TextureArray)) {
                if (depthTexD->sampleDesc.Count > 1) {
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    if (depthTexD->arrayRangeStart() >= 0 && depthTexD->arrayRangeLength() >= 0) {
                        dsvDesc.Texture2DMSArray.FirstArraySlice = UINT(depthTexD->arrayRangeStart());
                        dsvDesc.Texture2DMSArray.ArraySize = UINT(depthTexD->arrayRangeLength());
                    } else {
                        dsvDesc.Texture2DMSArray.FirstArraySlice = 0;
                        dsvDesc.Texture2DMSArray.ArraySize = UINT(qMax(0, depthTexD->arraySize()));
                    }
                } else {
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                    if (depthTexD->arrayRangeStart() >= 0 && depthTexD->arrayRangeLength() >= 0) {
                        dsvDesc.Texture2DArray.FirstArraySlice = UINT(depthTexD->arrayRangeStart());
                        dsvDesc.Texture2DArray.ArraySize = UINT(depthTexD->arrayRangeLength());
                    } else {
                        dsvDesc.Texture2DArray.FirstArraySlice = 0;
                        dsvDesc.Texture2DArray.ArraySize = UINT(qMax(0, depthTexD->arraySize()));
                    }
                }
            }
            dsv = rhiD->dsvPool.allocate(1);
            if (!dsv.isValid()) {
                qWarning("Failed to allocate DSV for texture render target");
                return false;
            }
            rhiD->dev->CreateDepthStencilView(res->resource, &dsvDesc, dsv.cpuHandle);
            if (d.colorAttCount == 0) {
                d.pixelSize = depthTexD->pixelSize();
                d.sampleCount = int(depthTexD->sampleDesc.Count);
            }
        } else {
            ownsDsv = false;
            QD3D12RenderBuffer *depthRbD = QRHI_RES(QD3D12RenderBuffer, m_desc.depthStencilBuffer());
            dsv = depthRbD->dsv;
            if (d.colorAttCount == 0) {
                d.pixelSize = m_desc.depthStencilBuffer()->pixelSize();
                d.sampleCount = int(depthRbD->sampleDesc.Count);
            }
        }
        d.dsAttCount = 1;
    } else {
        d.dsAttCount = 0;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE nullDescHandle = { 0 };
    for (int i = 0; i < QD3D12RenderTargetData::MAX_COLOR_ATTACHMENTS; ++i)
        d.rtv[i] = i < d.colorAttCount ? rtv[i].cpuHandle : nullDescHandle;
    d.dsv = dsv.cpuHandle;
    d.rp = QRHI_RES(QD3D12RenderPassDescriptor, m_renderPassDesc);

    QRhiRenderTargetAttachmentTracker::updateResIdList<QD3D12Texture, QD3D12RenderBuffer>(m_desc, &d.currentResIdList);

    rhiD->registerResource(this);
    return true;
}

QSize QD3D12TextureRenderTarget::pixelSize() const
{
    if (!QRhiRenderTargetAttachmentTracker::isUpToDate<QD3D12Texture, QD3D12RenderBuffer>(m_desc, d.currentResIdList))
        const_cast<QD3D12TextureRenderTarget *>(this)->create();

    return d.pixelSize;
}

float QD3D12TextureRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QD3D12TextureRenderTarget::sampleCount() const
{
    return d.sampleCount;
}

QD3D12ShaderResourceBindings::QD3D12ShaderResourceBindings(QRhiImplementation *rhi)
    : QRhiShaderResourceBindings(rhi)
{
}

QD3D12ShaderResourceBindings::~QD3D12ShaderResourceBindings()
{
    destroy();
}

void QD3D12ShaderResourceBindings::destroy()
{
    QRHI_RES_RHI(QRhiD3D12);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QD3D12ShaderResourceBindings::create()
{
    QRHI_RES_RHI(QRhiD3D12);
    if (!rhiD->sanityCheckShaderResourceBindings(this))
        return false;

    rhiD->updateLayoutDesc(this);

    hasDynamicOffset = false;
    for (const QRhiShaderResourceBinding &b : std::as_const(m_bindings)) {
        const QRhiShaderResourceBinding::Data *bd = QRhiImplementation::shaderResourceBindingData(b);
        if (bd->type == QRhiShaderResourceBinding::UniformBuffer && bd->u.ubuf.hasDynamicOffset) {
            hasDynamicOffset = true;
            break;
        }
    }

    // The root signature is not part of the srb. Unintuitive, but the shader
    // translation pipeline ties our hands: as long as the per-shader (so per
    // stage!) nativeResourceBindingMap exist, meaning f.ex. that a SPIR-V
    // combined image sampler binding X passed in here may map to the tY and sY
    // HLSL registers, where Y is known only once the mapping table from the
    // shader is looked up. Creating a root parameters at this stage is
    // therefore impossible.

    generation += 1;
    rhiD->registerResource(this, false);
    return true;
}

void QD3D12ShaderResourceBindings::updateResources(UpdateFlags flags)
{
    Q_UNUSED(flags);
    generation += 1;
}

// Accessing the QRhiBuffer/Texture/Sampler resources must be avoided in the
// callbacks; that would only be possible if the srb had those specified, and
// that's not required at the time of srb and pipeline create() time, and
// createRootSignature is called from the pipeline create().

void QD3D12ShaderResourceBindings::visitUniformBuffer(QD3D12Stage s,
                                                      const QRhiShaderResourceBinding::Data::UniformBufferData &,
                                                      int shaderRegister,
                                                      int)
{
    D3D12_ROOT_PARAMETER1 rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParam.ShaderVisibility = qd3d12_stageToVisibility(s);
    rootParam.Descriptor.ShaderRegister = shaderRegister;
    rootParam.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
    visitorData.cbParams[s].append(rootParam);
}

void QD3D12ShaderResourceBindings::visitTexture(QD3D12Stage s,
                                                const QRhiShaderResourceBinding::TextureAndSampler &,
                                                int shaderRegister)
{
    D3D12_DESCRIPTOR_RANGE1 range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = shaderRegister;
    range.OffsetInDescriptorsFromTableStart = visitorData.currentSrvRangeOffset[s];
    visitorData.currentSrvRangeOffset[s] += 1;
    visitorData.srvRanges[s].append(range);
    if (visitorData.srvRanges[s].count() == 1) {
        visitorData.srvTables[s].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        visitorData.srvTables[s].ShaderVisibility = qd3d12_stageToVisibility(s);
    }
}

void QD3D12ShaderResourceBindings::visitSampler(QD3D12Stage s,
                                                const QRhiShaderResourceBinding::TextureAndSampler &,
                                                int shaderRegister)
{
    // Unlike SRVs and UAVs, samplers are handled so that each sampler becomes
    // a root parameter with its own descriptor table.

    int &rangeStoreIdx(visitorData.samplerRangeHeads[s]);
    if (rangeStoreIdx == 16) {
        qWarning("Sampler count in QD3D12Stage %d exceeds the limit of 16, this is disallowed by QRhi", s);
        return;
    }
    D3D12_DESCRIPTOR_RANGE1 range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = shaderRegister;
    visitorData.samplerRanges[s][rangeStoreIdx] = range;
    D3D12_ROOT_PARAMETER1 param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.ShaderVisibility = qd3d12_stageToVisibility(s);
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = &visitorData.samplerRanges[s][rangeStoreIdx];
    rangeStoreIdx += 1;
    visitorData.samplerTables[s].append(param);
}

void QD3D12ShaderResourceBindings::visitStorageBuffer(QD3D12Stage s,
                                                      const QRhiShaderResourceBinding::Data::StorageBufferData &,
                                                      QD3D12ShaderResourceVisitor::StorageOp,
                                                      int shaderRegister)
{
    D3D12_DESCRIPTOR_RANGE1 range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = shaderRegister;
    range.OffsetInDescriptorsFromTableStart = visitorData.currentUavRangeOffset[s];
    visitorData.currentUavRangeOffset[s] += 1;
    visitorData.uavRanges[s].append(range);
    if (visitorData.uavRanges[s].count() == 1) {
        visitorData.uavTables[s].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        visitorData.uavTables[s].ShaderVisibility = qd3d12_stageToVisibility(s);
    }
}

void QD3D12ShaderResourceBindings::visitStorageImage(QD3D12Stage s,
                                                     const QRhiShaderResourceBinding::Data::StorageImageData &,
                                                     QD3D12ShaderResourceVisitor::StorageOp,
                                                     int shaderRegister)
{
    D3D12_DESCRIPTOR_RANGE1 range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = shaderRegister;
    range.OffsetInDescriptorsFromTableStart = visitorData.currentUavRangeOffset[s];
    visitorData.currentUavRangeOffset[s] += 1;
    visitorData.uavRanges[s].append(range);
    if (visitorData.uavRanges[s].count() == 1) {
        visitorData.uavTables[s].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        visitorData.uavTables[s].ShaderVisibility = qd3d12_stageToVisibility(s);
    }
}

QD3D12ObjectHandle QD3D12ShaderResourceBindings::createRootSignature(const QD3D12ShaderStageData *stageData,
                                                                     int stageCount)
{
    QRHI_RES_RHI(QRhiD3D12);

    // It's not just that the root signature has to be tied to the pipeline
    // (cannot just freely create it like e.g. with Vulkan where one just
    // creates a descriptor layout 1:1 with the QRhiShaderResourceBindings'
    // data), due to not knowing the shader-specific resource binding mapping
    // tables at the point of srb creation, but each shader stage may have a
    // different mapping table. (ugh!)
    //
    // Hence we set up everything per-stage, even if it means the root
    // signature gets unnecessarily big. (note that the magic is in the
    // ShaderVisibility: even though the register range is the same in the
    // descriptor tables, the visibility is different)

    QD3D12ShaderResourceVisitor visitor(this, stageData, stageCount);

    visitorData = {};

    using namespace std::placeholders;
    visitor.uniformBuffer = std::bind(&QD3D12ShaderResourceBindings::visitUniformBuffer, this, _1, _2, _3, _4);
    visitor.texture = std::bind(&QD3D12ShaderResourceBindings::visitTexture, this, _1, _2, _3);
    visitor.sampler = std::bind(&QD3D12ShaderResourceBindings::visitSampler, this, _1, _2, _3);
    visitor.storageBuffer = std::bind(&QD3D12ShaderResourceBindings::visitStorageBuffer, this, _1, _2, _3, _4);
    visitor.storageImage = std::bind(&QD3D12ShaderResourceBindings::visitStorageImage, this, _1, _2, _3, _4);

    visitor.visit();

    // The maximum size of a root signature is 256 bytes, where a descriptor
    // table is 4, a root descriptor (e.g. CBV) is 8. We have 5 stages at most
    // (or 1 with compute) and a separate descriptor table for SRVs (->
    // textures) and UAVs (-> storage buffers and images) per stage, plus each
    // uniform buffer counts as a CBV in the stages it is visible.
    //
    // Due to the limited maximum size of a shader-visible sampler heap (2048)
    // and the potential costly switching of descriptor heaps, each sampler is
    // declared as a separate root parameter / descriptor table (meaning that
    // two samplers in the same stage are two parameters and two tables, not
    // just one). QRhi documents a hard limit of 16 on texture/sampler bindings
    // in a shader (matching D3D11), so we can hopefully get away with this.
    //
    // This means that e.g. a vertex+fragment shader with a uniform buffer
    // visible in both and one texture+sampler in the fragment shader would
    // consume 2*8 + 4 + 4 = 24 bytes. This also implies that clients
    // specifying the minimal stage bit mask for each entry in
    // QRhiShaderResourceBindings are ideal for this backend since it helps
    // reducing the chance of hitting the size limit.

    QVarLengthArray<D3D12_ROOT_PARAMETER1, 4> rootParams;
    for (int s = 0; s < 6; ++s) {
        if (!visitorData.cbParams[s].isEmpty())
            rootParams.append(visitorData.cbParams[s].constData(), visitorData.cbParams[s].count());
    }
    for (int s = 0; s < 6; ++s) {
        if (!visitorData.srvRanges[s].isEmpty()) {
            visitorData.srvTables[s].DescriptorTable.NumDescriptorRanges = visitorData.srvRanges[s].count();
            visitorData.srvTables[s].DescriptorTable.pDescriptorRanges = visitorData.srvRanges[s].constData();
            rootParams.append(visitorData.srvTables[s]);
        }
    }
    for (int s = 0; s < 6; ++s) {
        if (!visitorData.samplerTables[s].isEmpty())
            rootParams.append(visitorData.samplerTables[s].constData(), visitorData.samplerTables[s].count());
    }
    for (int s = 0; s < 6; ++s) {
        if (!visitorData.uavRanges[s].isEmpty()) {
            visitorData.uavTables[s].DescriptorTable.NumDescriptorRanges = visitorData.uavRanges[s].count();
            visitorData.uavTables[s].DescriptorTable.pDescriptorRanges = visitorData.uavRanges[s].constData();
            rootParams.append(visitorData.uavTables[s]);
        }
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (!rootParams.isEmpty()) {
        rsDesc.Desc_1_1.NumParameters = rootParams.count();
        rsDesc.Desc_1_1.pParameters = rootParams.constData();
    }

    UINT rsFlags = 0;
    for (int stageIdx = 0; stageIdx < stageCount; ++stageIdx) {
        if (stageData[stageIdx].valid && stageData[stageIdx].stage == VS)
            rsFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }
    rsDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAGS(rsFlags);

    ID3DBlob *signature = nullptr;
    HRESULT hr = D3D12SerializeVersionedRootSignature(&rsDesc, &signature, nullptr);
    if (FAILED(hr)) {
        qWarning("Failed to serialize root signature: %s", qPrintable(QSystemError::windowsComString(hr)));
        return {};
    }
    ID3D12RootSignature *rootSig = nullptr;
    hr = rhiD->dev->CreateRootSignature(0,
                                        signature->GetBufferPointer(),
                                        signature->GetBufferSize(),
                                        __uuidof(ID3D12RootSignature),
                                        reinterpret_cast<void **>(&rootSig));
    signature->Release();
    if (FAILED(hr)) {
        qWarning("Failed to create root signature: %s", qPrintable(QSystemError::windowsComString(hr)));
        return {};
    }

    return QD3D12RootSignature::addToPool(&rhiD->rootSignaturePool, rootSig);
}

// For shader model < 6.0 we do the same as the D3D11 backend: use the old
// compiler (D3DCompile) to generate DXBC, just as qsb does (when -c is passed)
// by invoking fxc, not dxc. For SM >= 6.0 we have to use the new compiler and
// work with DXIL. And that involves IDxcCompiler and needs the presence of
// dxcompiler.dll and dxil.dll at runtime. Plus there's a chance we have
// ancient SDK headers when not using MSVC. So this is heavily optional,
// meaning support for dxc can be disabled both at build time (no dxcapi.h) and
// at run time (no DLLs).

static inline void makeHlslTargetString(char target[7], const char stage[3], int version)
{
    const int smMajor = version / 10;
    const int smMinor = version % 10;
    target[0] = stage[0];
    target[1] = stage[1];
    target[2] = '_';
    target[3] = '0' + smMajor;
    target[4] = '_';
    target[5] = '0' + smMinor;
    target[6] = '\0';
}

enum class HlslCompileFlag
{
    WithDebugInfo = 0x01
};

static QByteArray legacyCompile(const QShaderCode &hlslSource, const char *target, int flags, QString *error)
{
    static const pD3DCompile d3dCompile = QRhiD3D::resolveD3DCompile();
    if (!d3dCompile) {
        qWarning("Unable to resolve function D3DCompile()");
        return QByteArray();
    }

    ID3DBlob *bytecode = nullptr;
    ID3DBlob *errors = nullptr;
    UINT d3dCompileFlags = 0;
    if (flags & int(HlslCompileFlag::WithDebugInfo))
        d3dCompileFlags |= D3DCOMPILE_DEBUG;

    HRESULT hr = d3dCompile(hlslSource.shader().constData(), SIZE_T(hlslSource.shader().size()),
                            nullptr, nullptr, nullptr,
                            hlslSource.entryPoint().constData(), target, d3dCompileFlags, 0, &bytecode, &errors);
    if (FAILED(hr) || !bytecode) {
        qWarning("HLSL shader compilation failed: 0x%x", uint(hr));
        if (errors) {
            *error = QString::fromUtf8(static_cast<const char *>(errors->GetBufferPointer()),
                                       int(errors->GetBufferSize()));
            errors->Release();
        }
        return QByteArray();
    }

    QByteArray result;
    result.resize(int(bytecode->GetBufferSize()));
    memcpy(result.data(), bytecode->GetBufferPointer(), size_t(result.size()));
    bytecode->Release();
    return result;
}

#ifdef QRHI_D3D12_HAS_DXC

#ifndef DXC_CP_UTF8
#define DXC_CP_UTF8 65001
#endif

#ifndef DXC_ARG_DEBUG
#define DXC_ARG_DEBUG L"-Zi"
#endif

static QByteArray dxcCompile(const QShaderCode &hlslSource, const char *target, int flags, QString *error)
{
    static std::pair<IDxcCompiler *, IDxcLibrary *> dxc = QRhiD3D::createDxcCompiler();
    IDxcCompiler *compiler = dxc.first;
    if (!compiler) {
        qWarning("Unable to instantiate IDxcCompiler. Likely no dxcompiler.dll and dxil.dll present. "
                 "Use windeployqt or try https://github.com/microsoft/DirectXShaderCompiler/releases");
        return QByteArray();
    }
    IDxcLibrary *library = dxc.second;
    if (!library)
        return QByteArray();

    IDxcBlobEncoding *sourceBlob = nullptr;
    HRESULT hr = library->CreateBlobWithEncodingOnHeapCopy(hlslSource.shader().constData(),
                                                           UINT32(hlslSource.shader().size()),
                                                           DXC_CP_UTF8,
                                                           &sourceBlob);
    if (FAILED(hr)) {
        qWarning("Failed to create source blob for dxc: 0x%x (%s)",
                 uint(hr),
                 qPrintable(QSystemError::windowsComString(hr)));
        return QByteArray();
    }

    const QString entryPointStr = QString::fromLatin1(hlslSource.entryPoint());
    const QString targetStr = QString::fromLatin1(target);

    QVarLengthArray<LPCWSTR, 4> argPtrs;
    QString debugArg;
    if (flags & int(HlslCompileFlag::WithDebugInfo)) {
        debugArg = QString::fromUtf16(reinterpret_cast<const char16_t *>(DXC_ARG_DEBUG));
        argPtrs.append(reinterpret_cast<LPCWSTR>(debugArg.utf16()));
    }

    IDxcOperationResult *result = nullptr;
    hr = compiler->Compile(sourceBlob,
                           nullptr,
                           reinterpret_cast<LPCWSTR>(entryPointStr.utf16()),
                           reinterpret_cast<LPCWSTR>(targetStr.utf16()),
                           argPtrs.data(), argPtrs.count(),
                           nullptr, 0,
                           nullptr,
                           &result);
    sourceBlob->Release();
    if (SUCCEEDED(hr))
        result->GetStatus(&hr);
    if (FAILED(hr)) {
        qWarning("HLSL shader compilation failed: 0x%x (%s)",
                 uint(hr),
                 qPrintable(QSystemError::windowsComString(hr)));
        if (result) {
            IDxcBlobEncoding *errorsBlob = nullptr;
            if (SUCCEEDED(result->GetErrorBuffer(&errorsBlob))) {
                if (errorsBlob) {
                    *error = QString::fromUtf8(static_cast<const char *>(errorsBlob->GetBufferPointer()),
                                               int(errorsBlob->GetBufferSize()));
                    errorsBlob->Release();
                }
            }
        }
        return QByteArray();
    }

    IDxcBlob *bytecode = nullptr;
    if FAILED(result->GetResult(&bytecode)) {
        qWarning("No result from IDxcCompiler: 0x%x (%s)",
                 uint(hr),
                 qPrintable(QSystemError::windowsComString(hr)));
        return QByteArray();
    }

    QByteArray ba;
    ba.resize(int(bytecode->GetBufferSize()));
    memcpy(ba.data(), bytecode->GetBufferPointer(), size_t(ba.size()));
    bytecode->Release();
    return ba;
}

#endif // QRHI_D3D12_HAS_DXC

static QByteArray compileHlslShaderSource(const QShader &shader,
                                          QShader::Variant shaderVariant,
                                          int flags,
                                          QString *error,
                                          QShaderKey *usedShaderKey)
{
    // look for SM 6.7, 6.6, .., 5.0
    const int shaderModelMax = 67;
    for (int sm = shaderModelMax; sm >= 50; --sm) {
        for (QShader::Source type : { QShader::DxilShader, QShader::DxbcShader }) {
            QShaderKey key = { type, sm, shaderVariant };
            QShaderCode intermediateBytecodeShader = shader.shader(key);
            if (!intermediateBytecodeShader.shader().isEmpty()) {
                if (usedShaderKey)
                    *usedShaderKey = key;
                return intermediateBytecodeShader.shader();
            }
        }
    }

    QShaderCode hlslSource;
    QShaderKey key;
    for (int sm = shaderModelMax; sm >= 50; --sm) {
        key = { QShader::HlslShader, sm, shaderVariant };
        hlslSource = shader.shader(key);
        if (!hlslSource.shader().isEmpty())
            break;
    }

    if (hlslSource.shader().isEmpty()) {
        qWarning() << "No HLSL (shader model 6.7..5.0) code found in baked shader" << shader;
        return QByteArray();
    }

    if (usedShaderKey)
        *usedShaderKey = key;

    char target[7];
    switch (shader.stage()) {
    case QShader::VertexStage:
        makeHlslTargetString(target, "vs", key.sourceVersion().version());
        break;
    case QShader::TessellationControlStage:
        makeHlslTargetString(target, "hs", key.sourceVersion().version());
        break;
    case QShader::TessellationEvaluationStage:
        makeHlslTargetString(target, "ds", key.sourceVersion().version());
        break;
    case QShader::GeometryStage:
        makeHlslTargetString(target, "gs", key.sourceVersion().version());
        break;
    case QShader::FragmentStage:
        makeHlslTargetString(target, "ps", key.sourceVersion().version());
        break;
    case QShader::ComputeStage:
        makeHlslTargetString(target, "cs", key.sourceVersion().version());
        break;
    }

    if (key.sourceVersion().version() >= 60) {
#ifdef QRHI_D3D12_HAS_DXC
        return dxcCompile(hlslSource, target, flags, error);
#else
        qWarning("Attempted to runtime-compile HLSL source code for shader model >= 6.0 "
                 "but the Qt build has no support for DXC. "
                 "Rebuild Qt with a recent Windows SDK or switch to an MSVC build.");
#endif
    }

    return legacyCompile(hlslSource, target, flags, error);
}

static inline UINT8 toD3DColorWriteMask(QRhiGraphicsPipeline::ColorMask c)
{
    UINT8 f = 0;
    if (c.testFlag(QRhiGraphicsPipeline::R))
        f |= D3D12_COLOR_WRITE_ENABLE_RED;
    if (c.testFlag(QRhiGraphicsPipeline::G))
        f |= D3D12_COLOR_WRITE_ENABLE_GREEN;
    if (c.testFlag(QRhiGraphicsPipeline::B))
        f |= D3D12_COLOR_WRITE_ENABLE_BLUE;
    if (c.testFlag(QRhiGraphicsPipeline::A))
        f |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
    return f;
}

static inline D3D12_BLEND toD3DBlendFactor(QRhiGraphicsPipeline::BlendFactor f, bool rgb)
{
    // SrcBlendAlpha and DstBlendAlpha do not accept *_COLOR. With other APIs
    // this is handled internally (so that e.g. VK_BLEND_FACTOR_SRC_COLOR is
    // accepted and is in effect equivalent to VK_BLEND_FACTOR_SRC_ALPHA when
    // set as an alpha src/dest factor), but for D3D we have to take care of it
    // ourselves. Hence the rgb argument.

    switch (f) {
    case QRhiGraphicsPipeline::Zero:
        return D3D12_BLEND_ZERO;
    case QRhiGraphicsPipeline::One:
        return D3D12_BLEND_ONE;
    case QRhiGraphicsPipeline::SrcColor:
        return rgb ? D3D12_BLEND_SRC_COLOR : D3D12_BLEND_SRC_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrcColor:
        return rgb ? D3D12_BLEND_INV_SRC_COLOR : D3D12_BLEND_INV_SRC_ALPHA;
    case QRhiGraphicsPipeline::DstColor:
        return rgb ? D3D12_BLEND_DEST_COLOR : D3D12_BLEND_DEST_ALPHA;
    case QRhiGraphicsPipeline::OneMinusDstColor:
        return rgb ? D3D12_BLEND_INV_DEST_COLOR : D3D12_BLEND_INV_DEST_ALPHA;
    case QRhiGraphicsPipeline::SrcAlpha:
        return D3D12_BLEND_SRC_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrcAlpha:
        return D3D12_BLEND_INV_SRC_ALPHA;
    case QRhiGraphicsPipeline::DstAlpha:
        return D3D12_BLEND_DEST_ALPHA;
    case QRhiGraphicsPipeline::OneMinusDstAlpha:
        return D3D12_BLEND_INV_DEST_ALPHA;
    case QRhiGraphicsPipeline::ConstantColor:
    case QRhiGraphicsPipeline::ConstantAlpha:
        return D3D12_BLEND_BLEND_FACTOR;
    case QRhiGraphicsPipeline::OneMinusConstantColor:
    case QRhiGraphicsPipeline::OneMinusConstantAlpha:
        return D3D12_BLEND_INV_BLEND_FACTOR;
    case QRhiGraphicsPipeline::SrcAlphaSaturate:
        return D3D12_BLEND_SRC_ALPHA_SAT;
    case QRhiGraphicsPipeline::Src1Color:
        return rgb ? D3D12_BLEND_SRC1_COLOR : D3D12_BLEND_SRC1_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrc1Color:
        return rgb ? D3D12_BLEND_INV_SRC1_COLOR : D3D12_BLEND_INV_SRC1_ALPHA;
    case QRhiGraphicsPipeline::Src1Alpha:
        return D3D12_BLEND_SRC1_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrc1Alpha:
        return D3D12_BLEND_INV_SRC1_ALPHA;
    }
    Q_UNREACHABLE_RETURN(D3D12_BLEND_ZERO);
}

static inline D3D12_BLEND_OP toD3DBlendOp(QRhiGraphicsPipeline::BlendOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::Add:
        return D3D12_BLEND_OP_ADD;
    case QRhiGraphicsPipeline::Subtract:
        return D3D12_BLEND_OP_SUBTRACT;
    case QRhiGraphicsPipeline::ReverseSubtract:
        return D3D12_BLEND_OP_REV_SUBTRACT;
    case QRhiGraphicsPipeline::Min:
        return D3D12_BLEND_OP_MIN;
    case QRhiGraphicsPipeline::Max:
        return D3D12_BLEND_OP_MAX;
    }
    Q_UNREACHABLE_RETURN(D3D12_BLEND_OP_ADD);
}

static inline D3D12_CULL_MODE toD3DCullMode(QRhiGraphicsPipeline::CullMode c)
{
    switch (c) {
    case QRhiGraphicsPipeline::None:
        return D3D12_CULL_MODE_NONE;
    case QRhiGraphicsPipeline::Front:
        return D3D12_CULL_MODE_FRONT;
    case QRhiGraphicsPipeline::Back:
        return D3D12_CULL_MODE_BACK;
    }
    Q_UNREACHABLE_RETURN(D3D12_CULL_MODE_NONE);
}

static inline D3D12_FILL_MODE toD3DFillMode(QRhiGraphicsPipeline::PolygonMode mode)
{
    switch (mode) {
    case QRhiGraphicsPipeline::Fill:
        return D3D12_FILL_MODE_SOLID;
    case QRhiGraphicsPipeline::Line:
        return D3D12_FILL_MODE_WIREFRAME;
    }
    Q_UNREACHABLE_RETURN(D3D12_FILL_MODE_SOLID);
}

static inline D3D12_COMPARISON_FUNC toD3DCompareOp(QRhiGraphicsPipeline::CompareOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::Never:
        return D3D12_COMPARISON_FUNC_NEVER;
    case QRhiGraphicsPipeline::Less:
        return D3D12_COMPARISON_FUNC_LESS;
    case QRhiGraphicsPipeline::Equal:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case QRhiGraphicsPipeline::LessOrEqual:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case QRhiGraphicsPipeline::Greater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case QRhiGraphicsPipeline::NotEqual:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case QRhiGraphicsPipeline::GreaterOrEqual:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case QRhiGraphicsPipeline::Always:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    }
    Q_UNREACHABLE_RETURN(D3D12_COMPARISON_FUNC_ALWAYS);
}

static inline D3D12_STENCIL_OP toD3DStencilOp(QRhiGraphicsPipeline::StencilOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::StencilZero:
        return D3D12_STENCIL_OP_ZERO;
    case QRhiGraphicsPipeline::Keep:
        return D3D12_STENCIL_OP_KEEP;
    case QRhiGraphicsPipeline::Replace:
        return D3D12_STENCIL_OP_REPLACE;
    case QRhiGraphicsPipeline::IncrementAndClamp:
        return D3D12_STENCIL_OP_INCR_SAT;
    case QRhiGraphicsPipeline::DecrementAndClamp:
        return D3D12_STENCIL_OP_DECR_SAT;
    case QRhiGraphicsPipeline::Invert:
        return D3D12_STENCIL_OP_INVERT;
    case QRhiGraphicsPipeline::IncrementAndWrap:
        return D3D12_STENCIL_OP_INCR;
    case QRhiGraphicsPipeline::DecrementAndWrap:
        return D3D12_STENCIL_OP_DECR;
    }
    Q_UNREACHABLE_RETURN(D3D12_STENCIL_OP_KEEP);
}

static inline D3D12_PRIMITIVE_TOPOLOGY toD3DTopology(QRhiGraphicsPipeline::Topology t, int patchControlPointCount)
{
    switch (t) {
    case QRhiGraphicsPipeline::Triangles:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case QRhiGraphicsPipeline::TriangleStrip:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case QRhiGraphicsPipeline::TriangleFan:
        qWarning("Triangle fans are not supported with D3D");
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case QRhiGraphicsPipeline::Lines:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case QRhiGraphicsPipeline::LineStrip:
        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case QRhiGraphicsPipeline::Points:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case QRhiGraphicsPipeline::Patches:
        Q_ASSERT(patchControlPointCount >= 1 && patchControlPointCount <= 32);
        return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (patchControlPointCount - 1));
    }
    Q_UNREACHABLE_RETURN(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

static inline D3D12_PRIMITIVE_TOPOLOGY_TYPE toD3DTopologyType(QRhiGraphicsPipeline::Topology t)
{
    switch (t) {
    case QRhiGraphicsPipeline::Triangles:
    case QRhiGraphicsPipeline::TriangleStrip:
    case QRhiGraphicsPipeline::TriangleFan:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case QRhiGraphicsPipeline::Lines:
    case QRhiGraphicsPipeline::LineStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case QRhiGraphicsPipeline::Points:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case QRhiGraphicsPipeline::Patches:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }
    Q_UNREACHABLE_RETURN(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
}

static inline DXGI_FORMAT toD3DAttributeFormat(QRhiVertexInputAttribute::Format format)
{
    switch (format) {
    case QRhiVertexInputAttribute::Float4:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case QRhiVertexInputAttribute::Float3:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case QRhiVertexInputAttribute::Float2:
        return DXGI_FORMAT_R32G32_FLOAT;
    case QRhiVertexInputAttribute::Float:
        return DXGI_FORMAT_R32_FLOAT;
    case QRhiVertexInputAttribute::UNormByte4:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case QRhiVertexInputAttribute::UNormByte2:
        return DXGI_FORMAT_R8G8_UNORM;
    case QRhiVertexInputAttribute::UNormByte:
        return DXGI_FORMAT_R8_UNORM;
    case QRhiVertexInputAttribute::UInt4:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case QRhiVertexInputAttribute::UInt3:
        return DXGI_FORMAT_R32G32B32_UINT;
    case QRhiVertexInputAttribute::UInt2:
        return DXGI_FORMAT_R32G32_UINT;
    case QRhiVertexInputAttribute::UInt:
        return DXGI_FORMAT_R32_UINT;
     case QRhiVertexInputAttribute::SInt4:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case QRhiVertexInputAttribute::SInt3:
        return DXGI_FORMAT_R32G32B32_SINT;
    case QRhiVertexInputAttribute::SInt2:
        return DXGI_FORMAT_R32G32_SINT;
    case QRhiVertexInputAttribute::SInt:
        return DXGI_FORMAT_R32_SINT;
    case QRhiVertexInputAttribute::Half4:
    // Note: D3D does not support half3.  Pass through half3 as half4.
    case QRhiVertexInputAttribute::Half3:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case QRhiVertexInputAttribute::Half2:
        return DXGI_FORMAT_R16G16_FLOAT;
    case QRhiVertexInputAttribute::Half:
         return DXGI_FORMAT_R16_FLOAT;
    case QRhiVertexInputAttribute::UShort4:
    // Note: D3D does not support UShort3.  Pass through UShort3 as UShort4.
    case QRhiVertexInputAttribute::UShort3:
        return DXGI_FORMAT_R16G16B16A16_UINT;
    case QRhiVertexInputAttribute::UShort2:
        return DXGI_FORMAT_R16G16_UINT;
    case QRhiVertexInputAttribute::UShort:
        return DXGI_FORMAT_R16_UINT;
    case QRhiVertexInputAttribute::SShort4:
    // Note: D3D does not support SShort3.  Pass through SShort3 as SShort4.
    case QRhiVertexInputAttribute::SShort3:
        return DXGI_FORMAT_R16G16B16A16_SINT;
    case QRhiVertexInputAttribute::SShort2:
        return DXGI_FORMAT_R16G16_SINT;
    case QRhiVertexInputAttribute::SShort:
        return DXGI_FORMAT_R16_SINT;
    }
    Q_UNREACHABLE_RETURN(DXGI_FORMAT_R32G32B32A32_FLOAT);
}

QD3D12GraphicsPipeline::QD3D12GraphicsPipeline(QRhiImplementation *rhi)
    : QRhiGraphicsPipeline(rhi)
{
}

QD3D12GraphicsPipeline::~QD3D12GraphicsPipeline()
{
    destroy();
}

void QD3D12GraphicsPipeline::destroy()
{
    if (handle.isNull())
        return;

    QRHI_RES_RHI(QRhiD3D12);
    if (rhiD) {
        rhiD->releaseQueue.deferredReleasePipeline(handle);
        rhiD->releaseQueue.deferredReleaseRootSignature(rootSigHandle);
    }

    handle = {};
    stageData = {};

    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QD3D12GraphicsPipeline::create()
{
    if (!handle.isNull())
        destroy();

    QRHI_RES_RHI(QRhiD3D12);
    if (!rhiD->sanityCheckGraphicsPipeline(this))
        return false;

    rhiD->pipelineCreationStart();

    QByteArray shaderBytecode[5];
    for (const QRhiShaderStage &shaderStage : std::as_const(m_shaderStages)) {
        const QD3D12Stage d3dStage = qd3d12_stage(shaderStage.type());
        stageData[d3dStage].valid = true;
        stageData[d3dStage].stage = d3dStage;
        auto cacheIt = rhiD->shaderBytecodeCache.data.constFind(shaderStage);
        if (cacheIt != rhiD->shaderBytecodeCache.data.constEnd()) {
            shaderBytecode[d3dStage] = cacheIt->bytecode;
            stageData[d3dStage].nativeResourceBindingMap = cacheIt->nativeResourceBindingMap;
        } else {
            QString error;
            QShaderKey shaderKey;
            int compileFlags = 0;
            if (m_flags.testFlag(CompileShadersWithDebugInfo))
                compileFlags |= int(HlslCompileFlag::WithDebugInfo);
            const QByteArray bytecode = compileHlslShaderSource(shaderStage.shader(),
                                                                shaderStage.shaderVariant(),
                                                                compileFlags,
                                                                &error,
                                                                &shaderKey);
            if (bytecode.isEmpty()) {
                qWarning("HLSL graphics shader compilation failed: %s", qPrintable(error));
                return false;
            }

            shaderBytecode[d3dStage] = bytecode;
            stageData[d3dStage].nativeResourceBindingMap = shaderStage.shader().nativeResourceBindingMap(shaderKey);
            rhiD->shaderBytecodeCache.insertWithCapacityLimit(shaderStage,
                                                              { bytecode, stageData[d3dStage].nativeResourceBindingMap });
        }
    }

    QD3D12ShaderResourceBindings *srbD = QRHI_RES(QD3D12ShaderResourceBindings, m_shaderResourceBindings);
    if (srbD) {
        rootSigHandle = srbD->createRootSignature(stageData.data(), 5);
        if (rootSigHandle.isNull()) {
            qWarning("Failed to create root signature");
            return false;
        }
    }
    ID3D12RootSignature *rootSig = nullptr;
    if (QD3D12RootSignature *rs = rhiD->rootSignaturePool.lookupRef(rootSigHandle))
        rootSig = rs->rootSig;
    if (!rootSig) {
        qWarning("Cannot create graphics pipeline state without root signature");
        return false;
    }

    QD3D12RenderPassDescriptor *rpD = QRHI_RES(QD3D12RenderPassDescriptor, m_renderPassDesc);
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    if (rpD->colorAttachmentCount > 0) {
        format = DXGI_FORMAT(rpD->colorFormat[0]);
    } else if (rpD->hasDepthStencil) {
        format = DXGI_FORMAT(rpD->dsFormat);
    } else {
        qWarning("Cannot create graphics pipeline state without color or depthStencil format");
        return false;
    }
    const DXGI_SAMPLE_DESC sampleDesc = rhiD->effectiveSampleDesc(m_sampleCount, format);

    struct {
        QD3D12PipelineStateSubObject<ID3D12RootSignature *, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE> rootSig;
        QD3D12PipelineStateSubObject<D3D12_INPUT_LAYOUT_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT> inputLayout;
        QD3D12PipelineStateSubObject<D3D12_PRIMITIVE_TOPOLOGY_TYPE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY> primitiveTopology;
        QD3D12PipelineStateSubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS> VS;
        QD3D12PipelineStateSubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS> HS;
        QD3D12PipelineStateSubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS> DS;
        QD3D12PipelineStateSubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS> GS;
        QD3D12PipelineStateSubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS> PS;
        QD3D12PipelineStateSubObject<D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER> rasterizerState;
        QD3D12PipelineStateSubObject<D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL> depthStencilState;
        QD3D12PipelineStateSubObject<D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND> blendState;
        QD3D12PipelineStateSubObject<D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> rtFormats;
        QD3D12PipelineStateSubObject<DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT> dsFormat;
        QD3D12PipelineStateSubObject<DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC> sampleDesc;
        QD3D12PipelineStateSubObject<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK> sampleMask;
        QD3D12PipelineStateSubObject<D3D12_VIEW_INSTANCING_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING> viewInstancingDesc;
    } stream;

    stream.rootSig.object = rootSig;

    QVarLengthArray<D3D12_INPUT_ELEMENT_DESC, 4> inputDescs;
    QByteArrayList matrixSliceSemantics;
    if (!shaderBytecode[VS].isEmpty()) {
        for (auto it = m_vertexInputLayout.cbeginAttributes(), itEnd = m_vertexInputLayout.cendAttributes();
             it != itEnd; ++it)
        {
            D3D12_INPUT_ELEMENT_DESC desc = {};
            // The output from SPIRV-Cross uses TEXCOORD<location> as the
            // semantic, except for matrices that are unrolled into consecutive
            // vec2/3/4s attributes and need TEXCOORD<location>_ as
            // SemanticName and row/column index as SemanticIndex.
            const int matrixSlice = it->matrixSlice();
            if (matrixSlice < 0) {
                desc.SemanticName = "TEXCOORD";
                desc.SemanticIndex = UINT(it->location());
            } else {
                QByteArray sem;
                sem.resize(16);
                std::snprintf(sem.data(), sem.size(), "TEXCOORD%d_", it->location() - matrixSlice);
                matrixSliceSemantics.append(sem);
                desc.SemanticName = matrixSliceSemantics.last().constData();
                desc.SemanticIndex = UINT(matrixSlice);
            }
            desc.Format = toD3DAttributeFormat(it->format());
            desc.InputSlot = UINT(it->binding());
            desc.AlignedByteOffset = it->offset();
            const QRhiVertexInputBinding *inputBinding = m_vertexInputLayout.bindingAt(it->binding());
            if (inputBinding->classification() == QRhiVertexInputBinding::PerInstance) {
                desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                desc.InstanceDataStepRate = inputBinding->instanceStepRate();
            } else {
                desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            }
            inputDescs.append(desc);
        }
    }

    stream.inputLayout.object.NumElements = inputDescs.count();
    stream.inputLayout.object.pInputElementDescs = inputDescs.isEmpty() ? nullptr : inputDescs.constData();

    stream.primitiveTopology.object = toD3DTopologyType(m_topology);
    topology = toD3DTopology(m_topology, m_patchControlPointCount);

    for (const QRhiShaderStage &shaderStage : std::as_const(m_shaderStages)) {
        const int d3dStage = qd3d12_stage(shaderStage.type());
        switch (d3dStage) {
        case VS:
            stream.VS.object.pShaderBytecode = shaderBytecode[d3dStage].constData();
            stream.VS.object.BytecodeLength = shaderBytecode[d3dStage].size();
            break;
        case HS:
            stream.HS.object.pShaderBytecode = shaderBytecode[d3dStage].constData();
            stream.HS.object.BytecodeLength = shaderBytecode[d3dStage].size();
            break;
        case DS:
            stream.DS.object.pShaderBytecode = shaderBytecode[d3dStage].constData();
            stream.DS.object.BytecodeLength = shaderBytecode[d3dStage].size();
            break;
        case GS:
            stream.GS.object.pShaderBytecode = shaderBytecode[d3dStage].constData();
            stream.GS.object.BytecodeLength = shaderBytecode[d3dStage].size();
            break;
        case PS:
            stream.PS.object.pShaderBytecode = shaderBytecode[d3dStage].constData();
            stream.PS.object.BytecodeLength = shaderBytecode[d3dStage].size();
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    stream.rasterizerState.object.FillMode = toD3DFillMode(m_polygonMode);
    stream.rasterizerState.object.CullMode = toD3DCullMode(m_cullMode);
    stream.rasterizerState.object.FrontCounterClockwise = m_frontFace == CCW;
    stream.rasterizerState.object.DepthBias = m_depthBias;
    stream.rasterizerState.object.SlopeScaledDepthBias = m_slopeScaledDepthBias;
    stream.rasterizerState.object.DepthClipEnable = TRUE;
    stream.rasterizerState.object.MultisampleEnable = sampleDesc.Count > 1;

    stream.depthStencilState.object.DepthEnable = m_depthTest;
    stream.depthStencilState.object.DepthWriteMask = m_depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    stream.depthStencilState.object.DepthFunc = toD3DCompareOp(m_depthOp);
    stream.depthStencilState.object.StencilEnable = m_stencilTest;
    if (m_stencilTest) {
        stream.depthStencilState.object.StencilReadMask = UINT8(m_stencilReadMask);
        stream.depthStencilState.object.StencilWriteMask = UINT8(m_stencilWriteMask);
        stream.depthStencilState.object.FrontFace.StencilFailOp = toD3DStencilOp(m_stencilFront.failOp);
        stream.depthStencilState.object.FrontFace.StencilDepthFailOp = toD3DStencilOp(m_stencilFront.depthFailOp);
        stream.depthStencilState.object.FrontFace.StencilPassOp = toD3DStencilOp(m_stencilFront.passOp);
        stream.depthStencilState.object.FrontFace.StencilFunc = toD3DCompareOp(m_stencilFront.compareOp);
        stream.depthStencilState.object.BackFace.StencilFailOp = toD3DStencilOp(m_stencilBack.failOp);
        stream.depthStencilState.object.BackFace.StencilDepthFailOp = toD3DStencilOp(m_stencilBack.depthFailOp);
        stream.depthStencilState.object.BackFace.StencilPassOp = toD3DStencilOp(m_stencilBack.passOp);
        stream.depthStencilState.object.BackFace.StencilFunc = toD3DCompareOp(m_stencilBack.compareOp);
    }

    stream.blendState.object.IndependentBlendEnable = m_targetBlends.count() > 1;
    for (int i = 0, ie = m_targetBlends.count(); i != ie; ++i) {
        const QRhiGraphicsPipeline::TargetBlend &b(m_targetBlends[i]);
        D3D12_RENDER_TARGET_BLEND_DESC blend = {};
        blend.BlendEnable = b.enable;
        blend.SrcBlend = toD3DBlendFactor(b.srcColor, true);
        blend.DestBlend = toD3DBlendFactor(b.dstColor, true);
        blend.BlendOp = toD3DBlendOp(b.opColor);
        blend.SrcBlendAlpha = toD3DBlendFactor(b.srcAlpha, false);
        blend.DestBlendAlpha = toD3DBlendFactor(b.dstAlpha, false);
        blend.BlendOpAlpha = toD3DBlendOp(b.opAlpha);
        blend.RenderTargetWriteMask = toD3DColorWriteMask(b.colorWrite);
        stream.blendState.object.RenderTarget[i] = blend;
    }
    if (m_targetBlends.isEmpty()) {
        D3D12_RENDER_TARGET_BLEND_DESC blend = {};
        blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        stream.blendState.object.RenderTarget[0] = blend;
    }

    stream.rtFormats.object.NumRenderTargets = rpD->colorAttachmentCount;
    for (int i = 0; i < rpD->colorAttachmentCount; ++i)
        stream.rtFormats.object.RTFormats[i] = DXGI_FORMAT(rpD->colorFormat[i]);

    stream.dsFormat.object = rpD->hasDepthStencil ? DXGI_FORMAT(rpD->dsFormat) : DXGI_FORMAT_UNKNOWN;

    stream.sampleDesc.object = sampleDesc;

    stream.sampleMask.object = 0xFFFFFFFF;

    viewInstanceMask = 0;
    const bool isMultiView = m_multiViewCount >= 2;
    stream.viewInstancingDesc.object.ViewInstanceCount = isMultiView ? m_multiViewCount : 0;
    QVarLengthArray<D3D12_VIEW_INSTANCE_LOCATION, 4> viewInstanceLocations;
    if (isMultiView) {
        for (int i = 0; i < m_multiViewCount; ++i) {
            viewInstanceMask |= (1 << i);
            viewInstanceLocations.append({ 0, UINT(i) });
        }
        stream.viewInstancingDesc.object.pViewInstanceLocations = viewInstanceLocations.constData();
    }

    const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = { sizeof(stream), &stream };

    ID3D12PipelineState *pso = nullptr;
    HRESULT hr = rhiD->dev->CreatePipelineState(&streamDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pso));
    if (FAILED(hr)) {
        qWarning("Failed to create graphics pipeline state: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        rhiD->rootSignaturePool.remove(rootSigHandle);
        rootSigHandle = {};
        return false;
    }

    handle = QD3D12Pipeline::addToPool(&rhiD->pipelinePool, QD3D12Pipeline::Graphics, pso);

    rhiD->pipelineCreationEnd();
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QD3D12ComputePipeline::QD3D12ComputePipeline(QRhiImplementation *rhi)
    : QRhiComputePipeline(rhi)
{
}

QD3D12ComputePipeline::~QD3D12ComputePipeline()
{
    destroy();
}

void QD3D12ComputePipeline::destroy()
{
    if (handle.isNull())
        return;

    QRHI_RES_RHI(QRhiD3D12);
    if (rhiD) {
        rhiD->releaseQueue.deferredReleasePipeline(handle);
        rhiD->releaseQueue.deferredReleaseRootSignature(rootSigHandle);
    }

    handle = {};
    stageData = {};

    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QD3D12ComputePipeline::create()
{
    if (!handle.isNull())
        destroy();

    QRHI_RES_RHI(QRhiD3D12);
    rhiD->pipelineCreationStart();

    stageData.valid = true;
    stageData.stage = CS;

    QByteArray shaderBytecode;
    auto cacheIt = rhiD->shaderBytecodeCache.data.constFind(m_shaderStage);
    if (cacheIt != rhiD->shaderBytecodeCache.data.constEnd()) {
        shaderBytecode = cacheIt->bytecode;
        stageData.nativeResourceBindingMap = cacheIt->nativeResourceBindingMap;
    } else {
        QString error;
        QShaderKey shaderKey;
        int compileFlags = 0;
        if (m_flags.testFlag(CompileShadersWithDebugInfo))
            compileFlags |= int(HlslCompileFlag::WithDebugInfo);
        const QByteArray bytecode = compileHlslShaderSource(m_shaderStage.shader(),
                                                            m_shaderStage.shaderVariant(),
                                                            compileFlags,
                                                            &error,
                                                            &shaderKey);
        if (bytecode.isEmpty()) {
            qWarning("HLSL compute shader compilation failed: %s", qPrintable(error));
            return false;
        }

        shaderBytecode = bytecode;
        stageData.nativeResourceBindingMap = m_shaderStage.shader().nativeResourceBindingMap(shaderKey);
        rhiD->shaderBytecodeCache.insertWithCapacityLimit(m_shaderStage, { bytecode,
                                                                           stageData.nativeResourceBindingMap });
    }

    QD3D12ShaderResourceBindings *srbD = QRHI_RES(QD3D12ShaderResourceBindings, m_shaderResourceBindings);
    if (srbD) {
        rootSigHandle = srbD->createRootSignature(&stageData, 1);
        if (rootSigHandle.isNull()) {
            qWarning("Failed to create root signature");
            return false;
        }
    }
    ID3D12RootSignature *rootSig = nullptr;
    if (QD3D12RootSignature *rs = rhiD->rootSignaturePool.lookupRef(rootSigHandle))
        rootSig = rs->rootSig;
    if (!rootSig) {
        qWarning("Cannot create compute pipeline state without root signature");
        return false;
    }

    struct {
        QD3D12PipelineStateSubObject<ID3D12RootSignature *, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE> rootSig;
        QD3D12PipelineStateSubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS> CS;
    } stream;
    stream.rootSig.object = rootSig;
    stream.CS.object.pShaderBytecode = shaderBytecode.constData();
    stream.CS.object.BytecodeLength = shaderBytecode.size();
    const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = { sizeof(stream), &stream };
    ID3D12PipelineState *pso = nullptr;
    HRESULT hr = rhiD->dev->CreatePipelineState(&streamDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pso));
    if (FAILED(hr)) {
        qWarning("Failed to create compute pipeline state: %s",
                 qPrintable(QSystemError::windowsComString(hr)));
        rhiD->rootSignaturePool.remove(rootSigHandle);
        rootSigHandle = {};
        return false;
    }

    handle = QD3D12Pipeline::addToPool(&rhiD->pipelinePool, QD3D12Pipeline::Compute, pso);

    rhiD->pipelineCreationEnd();
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

// This is a lot like in the Metal backend: we need to now the rtv and dsv
// formats to create a graphics pipeline, and that's exactly what our
// "renderpass descriptor" is going to hold.
QD3D12RenderPassDescriptor::QD3D12RenderPassDescriptor(QRhiImplementation *rhi)
    : QRhiRenderPassDescriptor(rhi)
{
    serializedFormatData.reserve(16);
}

QD3D12RenderPassDescriptor::~QD3D12RenderPassDescriptor()
{
    destroy();
}

void QD3D12RenderPassDescriptor::destroy()
{
    QRHI_RES_RHI(QRhiD3D12);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QD3D12RenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const
{
    if (!other)
        return false;

    const QD3D12RenderPassDescriptor *o = QRHI_RES(const QD3D12RenderPassDescriptor, other);

    if (colorAttachmentCount != o->colorAttachmentCount)
        return false;

    if (hasDepthStencil != o->hasDepthStencil)
         return false;

    for (int i = 0; i < colorAttachmentCount; ++i) {
        if (colorFormat[i] != o->colorFormat[i])
            return false;
    }

    if (hasDepthStencil) {
        if (dsFormat != o->dsFormat)
            return false;
    }

    if (hasShadingRateMap != o->hasShadingRateMap)
        return false;

    return true;
}

void QD3D12RenderPassDescriptor::updateSerializedFormat()
{
    serializedFormatData.clear();
    auto p = std::back_inserter(serializedFormatData);

    *p++ = colorAttachmentCount;
    *p++ = hasDepthStencil;
    for (int i = 0; i < colorAttachmentCount; ++i)
      *p++ = colorFormat[i];
    *p++ = hasDepthStencil ? dsFormat : 0;
}

QRhiRenderPassDescriptor *QD3D12RenderPassDescriptor::newCompatibleRenderPassDescriptor() const
{
    QD3D12RenderPassDescriptor *rpD = new QD3D12RenderPassDescriptor(m_rhi);
    rpD->colorAttachmentCount = colorAttachmentCount;
    rpD->hasDepthStencil = hasDepthStencil;
    memcpy(rpD->colorFormat, colorFormat, sizeof(colorFormat));
    rpD->dsFormat = dsFormat;
    rpD->hasShadingRateMap = hasShadingRateMap;

    rpD->updateSerializedFormat();

    QRHI_RES_RHI(QRhiD3D12);
    rhiD->registerResource(rpD);
    return rpD;
}

QVector<quint32> QD3D12RenderPassDescriptor::serializedFormat() const
{
    return serializedFormatData;
}

QD3D12CommandBuffer::QD3D12CommandBuffer(QRhiImplementation *rhi)
    : QRhiCommandBuffer(rhi)
{
    resetState();
}

QD3D12CommandBuffer::~QD3D12CommandBuffer()
{
    destroy();
}

void QD3D12CommandBuffer::destroy()
{
    // nothing to do here, the command list is not owned by us
}

const QRhiNativeHandles *QD3D12CommandBuffer::nativeHandles()
{
    nativeHandlesStruct.commandList = cmdList;
    return &nativeHandlesStruct;
}

QD3D12SwapChainRenderTarget::QD3D12SwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain)
    : QRhiSwapChainRenderTarget(rhi, swapchain),
      d(rhi)
{
}

QD3D12SwapChainRenderTarget::~QD3D12SwapChainRenderTarget()
{
    destroy();
}

void QD3D12SwapChainRenderTarget::destroy()
{
    // nothing to do here
}

QSize QD3D12SwapChainRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QD3D12SwapChainRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QD3D12SwapChainRenderTarget::sampleCount() const
{
    return d.sampleCount;
}

QD3D12SwapChain::QD3D12SwapChain(QRhiImplementation *rhi)
    : QRhiSwapChain(rhi),
      rtWrapper(rhi, this),
      rtWrapperRight(rhi, this),
      cbWrapper(rhi)
{
}

QD3D12SwapChain::~QD3D12SwapChain()
{
    destroy();
}

void QD3D12SwapChain::destroy()
{
    if (!swapChain)
        return;

    releaseBuffers();

    swapChain->Release();
    swapChain = nullptr;
    sourceSwapChain1->Release();
    sourceSwapChain1 = nullptr;

    for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
        FrameResources &fr(frameRes[i]);
        if (fr.fence)
            fr.fence->Release();
        if (fr.fenceEvent)
            CloseHandle(fr.fenceEvent);
        if (fr.cmdList)
            fr.cmdList->Release();
        fr = {};
    }

    if (dcompVisual) {
        dcompVisual->Release();
        dcompVisual = nullptr;
    }

    if (dcompTarget) {
        dcompTarget->Release();
        dcompTarget = nullptr;
    }

    if (frameLatencyWaitableObject) {
        CloseHandle(frameLatencyWaitableObject);
        frameLatencyWaitableObject = nullptr;
    }

    QDxgiVSyncService::instance()->unregisterWindow(window);

    QRHI_RES_RHI(QRhiD3D12);
    if (rhiD) {
        rhiD->swapchains.remove(this);
        rhiD->unregisterResource(this);
    }
}

void QD3D12SwapChain::releaseBuffers()
{
    QRHI_RES_RHI(QRhiD3D12);
    rhiD->waitGpu();
    for (UINT i = 0; i < BUFFER_COUNT; ++i) {
        rhiD->resourcePool.remove(colorBuffers[i]);
        rhiD->rtvPool.release(rtvs[i], 1);
        if (stereo)
            rhiD->rtvPool.release(rtvsRight[i], 1);
        if (!msaaBuffers[i].isNull())
            rhiD->resourcePool.remove(msaaBuffers[i]);
        if (msaaRtvs[i].isValid())
            rhiD->rtvPool.release(msaaRtvs[i], 1);
    }
}

void QD3D12SwapChain::waitCommandCompletionForFrameSlot(int frameSlot)
{
    FrameResources &fr(frameRes[frameSlot]);
    if (fr.fence->GetCompletedValue() < fr.fenceCounter) {
        fr.fence->SetEventOnCompletion(fr.fenceCounter, fr.fenceEvent);
        WaitForSingleObject(fr.fenceEvent, INFINITE);
    }
}

void QD3D12SwapChain::addCommandCompletionSignalForCurrentFrameSlot()
{
    QRHI_RES_RHI(QRhiD3D12);
    FrameResources &fr(frameRes[currentFrameSlot]);
    fr.fenceCounter += 1u;
    rhiD->cmdQueue->Signal(fr.fence, fr.fenceCounter);
}

QRhiCommandBuffer *QD3D12SwapChain::currentFrameCommandBuffer()
{
    return &cbWrapper;
}

QRhiRenderTarget *QD3D12SwapChain::currentFrameRenderTarget()
{
    return &rtWrapper;
}

QRhiRenderTarget *QD3D12SwapChain::currentFrameRenderTarget(StereoTargetBuffer targetBuffer)
{
    return !stereo || targetBuffer == StereoTargetBuffer::LeftBuffer ? &rtWrapper : &rtWrapperRight;
}

QSize QD3D12SwapChain::surfacePixelSize()
{
    Q_ASSERT(m_window);
    return m_window->size() * m_window->devicePixelRatio();
}

bool QD3D12SwapChain::isFormatSupported(Format f)
{
    if (f == SDR)
        return true;

    if (!m_window) {
        qWarning("Attempted to call isFormatSupported() without a window set");
        return false;
    }

    QRHI_RES_RHI(QRhiD3D12);
    if (QDxgiHdrInfo(rhiD->activeAdapter).isHdrCapable(m_window))
        return f == QRhiSwapChain::HDRExtendedSrgbLinear || f == QRhiSwapChain::HDR10;

    return false;
}

QRhiSwapChainHdrInfo QD3D12SwapChain::hdrInfo()
{
    QRhiSwapChainHdrInfo info = QRhiSwapChain::hdrInfo();
    // Must use m_window, not window, given this may be called before createOrResize().
    if (m_window) {
        QRHI_RES_RHI(QRhiD3D12);
        info = QDxgiHdrInfo(rhiD->activeAdapter).queryHdrInfo(m_window);
    }
    return info;
}

QRhiRenderPassDescriptor *QD3D12SwapChain::newCompatibleRenderPassDescriptor()
{
    // not yet built so cannot rely on data computed in createOrResize()
    chooseFormats();

    QD3D12RenderPassDescriptor *rpD = new QD3D12RenderPassDescriptor(m_rhi);
    rpD->colorAttachmentCount = 1;
    rpD->hasDepthStencil = m_depthStencil != nullptr;
    rpD->colorFormat[0] = int(srgbAdjustedColorFormat);
    rpD->dsFormat = QD3D12RenderBuffer::DS_FORMAT;

    rpD->hasShadingRateMap = m_shadingRateMap != nullptr;

    rpD->updateSerializedFormat();

    QRHI_RES_RHI(QRhiD3D12);
    rhiD->registerResource(rpD);
    return rpD;
}

bool QRhiD3D12::ensureDirectCompositionDevice()
{
    if (dcompDevice)
        return true;

    qCDebug(QRHI_LOG_INFO, "Creating Direct Composition device (needed for semi-transparent windows)");
    dcompDevice = QRhiD3D::createDirectCompositionDevice();
    return dcompDevice ? true : false;
}

static const DXGI_FORMAT DEFAULT_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
static const DXGI_FORMAT DEFAULT_SRGB_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

void QD3D12SwapChain::chooseFormats()
{
    colorFormat = DEFAULT_FORMAT;
    srgbAdjustedColorFormat = m_flags.testFlag(sRGB) ? DEFAULT_SRGB_FORMAT : DEFAULT_FORMAT;
    hdrColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709; // SDR
    QRHI_RES_RHI(QRhiD3D12);
    if (m_format != SDR) {
        if (QDxgiHdrInfo(rhiD->activeAdapter).isHdrCapable(m_window)) {
            // https://docs.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
            switch (m_format) {
            case HDRExtendedSrgbLinear:
                colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
                hdrColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
                srgbAdjustedColorFormat = colorFormat;
                break;
            case HDR10:
                colorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
                hdrColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                srgbAdjustedColorFormat = colorFormat;
                break;
            default:
                break;
            }
        } else {
            // This happens also when Use HDR is set to Off in the Windows
            // Display settings. Show a helpful warning, but continue with the
            // default non-HDR format.
            qWarning("The output associated with the window is not HDR capable "
                     "(or Use HDR is Off in the Display Settings), ignoring HDR format request");
        }
    }
    sampleDesc = rhiD->effectiveSampleDesc(m_sampleCount, colorFormat);
}

bool QD3D12SwapChain::createOrResize()
{
    // Can be called multiple times due to window resizes - that is not the
    // same as a simple destroy+create (as with other resources). Just need to
    // resize the buffers then.

    const bool needsRegistration = !window || window != m_window;

    // except if the window actually changes
    if (window && window != m_window)
        destroy();

    window = m_window;
    m_currentPixelSize = surfacePixelSize();
    pixelSize = m_currentPixelSize;

    if (pixelSize.isEmpty())
        return false;

    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    HRESULT hr;
    QRHI_RES_RHI(QRhiD3D12);
    stereo = m_window->format().stereo() && rhiD->dxgiFactory->IsWindowedStereoEnabled();

    if (m_flags.testFlag(SurfaceHasPreMulAlpha) || m_flags.testFlag(SurfaceHasNonPreMulAlpha)) {
        if (rhiD->ensureDirectCompositionDevice()) {
            if (!dcompTarget) {
                hr = rhiD->dcompDevice->CreateTargetForHwnd(hwnd, false, &dcompTarget);
                if (FAILED(hr)) {
                    qWarning("Failed to create Direct Composition target for the window: %s",
                             qPrintable(QSystemError::windowsComString(hr)));
                }
            }
            if (dcompTarget && !dcompVisual) {
                hr = rhiD->dcompDevice->CreateVisual(&dcompVisual);
                if (FAILED(hr)) {
                    qWarning("Failed to create DirectComposition visual: %s",
                             qPrintable(QSystemError::windowsComString(hr)));
                }
            }
        }
        // simple consistency check
        if (window->requestedFormat().alphaBufferSize() <= 0)
            qWarning("Swapchain says surface has alpha but the window has no alphaBufferSize set. "
                     "This may lead to problems.");
    }

    swapInterval = m_flags.testFlag(QRhiSwapChain::NoVSync) ? 0 : 1;
    swapChainFlags = 0;
    if (swapInterval == 0 && rhiD->supportsAllowTearing)
        swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    // maxFrameLatency 0 means no waitable object usage.
    // Ignore it also when NoVSync is on, and when using WARP.
    const bool useFrameLatencyWaitableObject = rhiD->maxFrameLatency != 0
                                               && swapInterval != 0
                                               && rhiD->driverInfoStruct.deviceType != QRhiDriverInfo::CpuDevice;
    if (useFrameLatencyWaitableObject)
        swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    if (!swapChain) {
        chooseFormats();

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = UINT(pixelSize.width());
        desc.Height = UINT(pixelSize.height());
        desc.Format = colorFormat;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = BUFFER_COUNT;
        desc.Flags = swapChainFlags;
        desc.Scaling = DXGI_SCALING_NONE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Stereo = stereo;

        if (dcompVisual) {
            // With DirectComposition setting AlphaMode to STRAIGHT fails the
            // swapchain creation, whereas the result seems to be identical
            // with any of the other values, including IGNORE. (?)
            desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

            // DirectComposition has its own limitations, cannot use
            // SCALING_NONE. So with semi-transparency requested we are forced
            // to SCALING_STRETCH.
            desc.Scaling = DXGI_SCALING_STRETCH;
        }

        if (dcompVisual)
            hr = rhiD->dxgiFactory->CreateSwapChainForComposition(rhiD->cmdQueue, &desc, nullptr, &sourceSwapChain1);
        else
            hr = rhiD->dxgiFactory->CreateSwapChainForHwnd(rhiD->cmdQueue, hwnd, &desc, nullptr, nullptr, &sourceSwapChain1);

        // If failed and we tried a HDR format, then try with SDR. This
        // matches other backends, such as Vulkan where if the format is
        // not supported, the default one is used instead.
        if (FAILED(hr) && m_format != SDR) {
            colorFormat = DEFAULT_FORMAT;
            desc.Format = DEFAULT_FORMAT;
            if (dcompVisual)
                hr = rhiD->dxgiFactory->CreateSwapChainForComposition(rhiD->cmdQueue, &desc, nullptr, &sourceSwapChain1);
            else
                hr = rhiD->dxgiFactory->CreateSwapChainForHwnd(rhiD->cmdQueue, hwnd, &desc, nullptr, nullptr, &sourceSwapChain1);
        }

        if (SUCCEEDED(hr)) {
            if (FAILED(sourceSwapChain1->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void **>(&swapChain)))) {
                qWarning("IDXGISwapChain3 not available");
                return false;
            }
            if (m_format != SDR) {
                hr = swapChain->SetColorSpace1(hdrColorSpace);
                if (FAILED(hr)) {
                    qWarning("Failed to set color space on swapchain: %s",
                             qPrintable(QSystemError::windowsComString(hr)));
                }
            }
            if (useFrameLatencyWaitableObject) {
                swapChain->SetMaximumFrameLatency(rhiD->maxFrameLatency);
                frameLatencyWaitableObject = swapChain->GetFrameLatencyWaitableObject();
            }
            if (dcompVisual) {
                hr = dcompVisual->SetContent(swapChain);
                if (SUCCEEDED(hr)) {
                    hr = dcompTarget->SetRoot(dcompVisual);
                    if (FAILED(hr)) {
                        qWarning("Failed to associate Direct Composition visual with the target: %s",
                                 qPrintable(QSystemError::windowsComString(hr)));
                    }
                } else {
                    qWarning("Failed to set content for Direct Composition visual: %s",
                             qPrintable(QSystemError::windowsComString(hr)));
                }
            } else {
                // disable Alt+Enter; not relevant when using DirectComposition
                rhiD->dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES);
            }
        }
        if (FAILED(hr)) {
            qWarning("Failed to create D3D12 swapchain: %s"
                     " (Width=%u Height=%u Format=%u SampleCount=%u BufferCount=%u Scaling=%u SwapEffect=%u Stereo=%u)",
                     qPrintable(QSystemError::windowsComString(hr)),
                     desc.Width, desc.Height, UINT(desc.Format), desc.SampleDesc.Count,
                     desc.BufferCount, UINT(desc.Scaling), UINT(desc.SwapEffect), UINT(desc.Stereo));
            return false;
        }

        for (int i = 0; i < QD3D12_FRAMES_IN_FLIGHT; ++i) {
            hr = rhiD->dev->CreateFence(0,
                                        D3D12_FENCE_FLAG_NONE,
                                        __uuidof(ID3D12Fence),
                                        reinterpret_cast<void **>(&frameRes[i].fence));
            if (FAILED(hr)) {
                qWarning("Failed to create fence for swapchain: %s",
                         qPrintable(QSystemError::windowsComString(hr)));
                return false;
            }
            frameRes[i].fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

            frameRes[i].fenceCounter = 0;
        }
    } else {
        releaseBuffers();
        hr = swapChain->ResizeBuffers(BUFFER_COUNT,
                                      UINT(pixelSize.width()),
                                      UINT(pixelSize.height()),
                                      colorFormat,
                                      swapChainFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            qWarning("Device loss detected in ResizeBuffers()");
            rhiD->deviceLost = true;
            return false;
        } else if (FAILED(hr)) {
            qWarning("Failed to resize D3D12 swapchain: %s", qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    }

    for (UINT i = 0; i < BUFFER_COUNT; ++i) {
        ID3D12Resource *colorBuffer;
        hr = swapChain->GetBuffer(i, __uuidof(ID3D12Resource), reinterpret_cast<void **>(&colorBuffer));
        if (FAILED(hr)) {
            qWarning("Failed to get buffer %u for D3D12 swapchain: %s",
                     i, qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
        colorBuffers[i] = QD3D12Resource::addToPool(&rhiD->resourcePool, colorBuffer, D3D12_RESOURCE_STATE_PRESENT);
        rtvs[i] = rhiD->rtvPool.allocate(1);
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = srgbAdjustedColorFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rhiD->dev->CreateRenderTargetView(colorBuffer, &rtvDesc, rtvs[i].cpuHandle);

        if (stereo) {
            rtvsRight[i] = rhiD->rtvPool.allocate(1);
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = srgbAdjustedColorFormat;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.ArraySize = 1;
            rtvDesc.Texture2DArray.FirstArraySlice = 1;
            rhiD->dev->CreateRenderTargetView(colorBuffer, &rtvDesc, rtvsRight[i].cpuHandle);
        }
    }

    if (m_depthStencil && m_depthStencil->sampleCount() != m_sampleCount) {
        qWarning("Depth-stencil buffer's sampleCount (%d) does not match color buffers' sample count (%d). Expect problems.",
                 m_depthStencil->sampleCount(), m_sampleCount);
    }
    if (m_depthStencil && m_depthStencil->pixelSize() != pixelSize) {
        if (m_depthStencil->flags().testFlag(QRhiRenderBuffer::UsedWithSwapChainOnly)) {
            m_depthStencil->setPixelSize(pixelSize);
            if (!m_depthStencil->create())
                qWarning("Failed to rebuild swapchain's associated depth-stencil buffer for size %dx%d",
                         pixelSize.width(), pixelSize.height());
        } else {
            qWarning("Depth-stencil buffer's size (%dx%d) does not match the surface size (%dx%d). Expect problems.",
                     m_depthStencil->pixelSize().width(), m_depthStencil->pixelSize().height(),
                     pixelSize.width(), pixelSize.height());
        }
    }

    ds = m_depthStencil ? QRHI_RES(QD3D12RenderBuffer, m_depthStencil) : nullptr;

    if (sampleDesc.Count > 1) {
        for (UINT i = 0; i < BUFFER_COUNT; ++i) {
            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resourceDesc.Width = UINT64(pixelSize.width());
            resourceDesc.Height = UINT(pixelSize.height());
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = srgbAdjustedColorFormat;
            resourceDesc.SampleDesc = sampleDesc;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            D3D12_CLEAR_VALUE clearValue = {};
            clearValue.Format = colorFormat;
            ID3D12Resource *resource = nullptr;
            D3D12MA::Allocation *allocation = nullptr;
            HRESULT hr = rhiD->vma.createResource(D3D12_HEAP_TYPE_DEFAULT,
                                                  &resourceDesc,
                                                  D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                  &clearValue,
                                                  &allocation,
                                                  __uuidof(ID3D12Resource),
                                                  reinterpret_cast<void **>(&resource));
            if (FAILED(hr)) {
                qWarning("Failed to create MSAA color buffer: %s", qPrintable(QSystemError::windowsComString(hr)));
                return false;
            }
            msaaBuffers[i] = QD3D12Resource::addToPool(&rhiD->resourcePool, resource, D3D12_RESOURCE_STATE_RENDER_TARGET, allocation);
            msaaRtvs[i] = rhiD->rtvPool.allocate(1);
            if (!msaaRtvs[i].isValid())
                return false;
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = srgbAdjustedColorFormat;
            rtvDesc.ViewDimension = sampleDesc.Count > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DMS
                                                         : D3D12_RTV_DIMENSION_TEXTURE2D;
            rhiD->dev->CreateRenderTargetView(resource, &rtvDesc, msaaRtvs[i].cpuHandle);
        }
    }

    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
    currentFrameSlot = 0;
    lastFrameLatencyWaitSlot = -1; // wait already in the first frame, as instructed in the dxgi docs

    rtWrapper.setRenderPassDescriptor(m_renderPassDesc); // for the public getter in QRhiRenderTarget
    QD3D12SwapChainRenderTarget *rtD = QRHI_RES(QD3D12SwapChainRenderTarget, &rtWrapper);
    rtD->d.rp = QRHI_RES(QD3D12RenderPassDescriptor, m_renderPassDesc);
    rtD->d.pixelSize = pixelSize;
    rtD->d.dpr = float(window->devicePixelRatio());
    rtD->d.sampleCount = int(sampleDesc.Count);
    rtD->d.colorAttCount = 1;
    rtD->d.dsAttCount = m_depthStencil ? 1 : 0;

    rtWrapperRight.setRenderPassDescriptor(m_renderPassDesc);
    QD3D12SwapChainRenderTarget *rtDr = QRHI_RES(QD3D12SwapChainRenderTarget, &rtWrapperRight);
    rtDr->d.rp = QRHI_RES(QD3D12RenderPassDescriptor, m_renderPassDesc);
    rtDr->d.pixelSize = pixelSize;
    rtDr->d.dpr = float(window->devicePixelRatio());
    rtDr->d.sampleCount = int(sampleDesc.Count);
    rtDr->d.colorAttCount = 1;
    rtDr->d.dsAttCount = m_depthStencil ? 1 : 0;

    QDxgiVSyncService::instance()->registerWindow(window);

    if (needsRegistration || !rhiD->swapchains.contains(this))
        rhiD->swapchains.insert(this);

    rhiD->registerResource(this);

    return true;
}

QT_END_NAMESPACE

#endif // __ID3D12Device2_INTERFACE_DEFINED__
