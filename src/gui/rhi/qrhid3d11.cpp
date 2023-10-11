// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhid3d11_p.h"
#include "qshader.h"
#include "vs_test_p.h"
#include <QWindow>
#include <qmath.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/private/qsystemerror_p.h>
#include "qrhid3dhelpers_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*
  Direct3D 11 backend. Provides a double-buffered flip model swapchain.
  Textures and "static" buffers are USAGE_DEFAULT, leaving it to
  UpdateSubResource to upload the data in any way it sees fit. "Dynamic"
  buffers are USAGE_DYNAMIC and updating is done by mapping with WRITE_DISCARD.
  (so here QRhiBuffer keeps a copy of the buffer contents and all of it is
  memcpy'd every time, leaving the rest (juggling with the memory area Map
  returns) to the driver).
*/

/*!
    \class QRhiD3D11InitParams
    \inmodule QtGui
    \since 6.6
    \brief Direct3D 11 specific initialization parameters.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    A D3D11-based QRhi needs no special parameters for initialization. If
    desired, enableDebugLayer can be set to \c true to enable the Direct3D
    debug layer. This can be useful during development, but should be avoided
    in production builds.

    \badcode
        QRhiD3D11InitParams params;
        params.enableDebugLayer = true;
        rhi = QRhi::create(QRhi::D3D11, &params);
    \endcode

    \note QRhiSwapChain should only be used in combination with QWindow
    instances that have their surface type set to QSurface::Direct3DSurface.

    \section2 Working with existing Direct3D 11 devices

    When interoperating with another graphics engine, it may be necessary to
    get a QRhi instance that uses the same Direct3D device. This can be
    achieved by passing a pointer to a QRhiD3D11NativeHandles to
    QRhi::create(). When the device is set to a non-null value, the device
    context must be specified as well. QRhi does not take ownership of any of
    the external objects.

    Sometimes, for example when using QRhi in combination with OpenXR, one will
    want to specify which adapter to use, and optionally, which feature level
    to request on the device, while leaving the device creation to QRhi. This
    is achieved by leaving the device and context pointers set to null, while
    specifying the adapter LUID and feature level.

    \note QRhi works with immediate contexts only. Deferred contexts are not
    used in any way.

    \note Regardless of using an imported or a QRhi-created device context, the
    \c ID3D11DeviceContext1 interface (Direct3D 11.1) must be supported.
    Initialization will fail otherwise.
 */

/*!
    \variable QRhiD3D11InitParams::enableDebugLayer

    When set to true, a debug device is created, assuming the debug layer is
    available. The default value is false.
*/

/*!
    \class QRhiD3D11NativeHandles
    \inmodule QtGui
    \since 6.6
    \brief Holds the D3D device and device context used by the QRhi.

    \note The class uses \c{void *} as the type since including the COM-based
    \c{d3d11.h} headers is not acceptable here. The actual types are
    \c{ID3D11Device *} and \c{ID3D11DeviceContext *}.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiD3D11NativeHandles::dev
*/

/*!
    \variable QRhiD3D11NativeHandles::context
*/

/*!
    \variable QRhiD3D11NativeHandles::featureLevel
*/

/*!
    \variable QRhiD3D11NativeHandles::adapterLuidLow
*/

/*!
    \variable QRhiD3D11NativeHandles::adapterLuidHigh
*/

// help mingw with its ancient sdk headers
#ifndef DXGI_ADAPTER_FLAG_SOFTWARE
#define DXGI_ADAPTER_FLAG_SOFTWARE 2
#endif

#ifndef D3D11_1_UAV_SLOT_COUNT
#define D3D11_1_UAV_SLOT_COUNT 64
#endif

#ifndef D3D11_VS_INPUT_REGISTER_COUNT
#define D3D11_VS_INPUT_REGISTER_COUNT 32
#endif

QRhiD3D11::QRhiD3D11(QRhiD3D11InitParams *params, QRhiD3D11NativeHandles *importParams)
    : ofr(this)
{
    debugLayer = params->enableDebugLayer;

    if (importParams) {
        if (importParams->dev && importParams->context) {
            dev = reinterpret_cast<ID3D11Device *>(importParams->dev);
            ID3D11DeviceContext *ctx = reinterpret_cast<ID3D11DeviceContext *>(importParams->context);
            if (SUCCEEDED(ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void **>(&context)))) {
                // get rid of the ref added by QueryInterface
                ctx->Release();
                importedDeviceAndContext = true;
            } else {
                qWarning("ID3D11DeviceContext1 not supported by context, cannot import");
            }
        }
        featureLevel = D3D_FEATURE_LEVEL(importParams->featureLevel);
        adapterLuid.LowPart = importParams->adapterLuidLow;
        adapterLuid.HighPart = importParams->adapterLuidHigh;
    }
}

template <class Int>
inline Int aligned(Int v, Int byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

static IDXGIFactory1 *createDXGIFactory2()
{
    IDXGIFactory1 *result = nullptr;
    const HRESULT hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), reinterpret_cast<void **>(&result));
    if (FAILED(hr)) {
        qWarning("CreateDXGIFactory2() failed to create DXGI factory: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        result = nullptr;
    }
    return result;
}

bool QRhiD3D11::create(QRhi::Flags flags)
{
    rhiFlags = flags;

    uint devFlags = 0;
    if (debugLayer)
        devFlags |= D3D11_CREATE_DEVICE_DEBUG;

    dxgiFactory = createDXGIFactory2();
    if (!dxgiFactory)
        return false;

    // For a FLIP_* swapchain Present(0, 0) is not necessarily
    // sufficient to get non-blocking behavior, try using ALLOW_TEARING
    // when available.
    supportsAllowTearing = false;
    IDXGIFactory5 *factory5 = nullptr;
    if (SUCCEEDED(dxgiFactory->QueryInterface(__uuidof(IDXGIFactory5), reinterpret_cast<void **>(&factory5)))) {
        BOOL allowTearing = false;
        if (SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            supportsAllowTearing = allowTearing;
        factory5->Release();
    }

    if (qEnvironmentVariableIntValue("QT_D3D_FLIP_DISCARD"))
        qWarning("The default swap effect is FLIP_DISCARD, QT_D3D_FLIP_DISCARD is now ignored");

    // Support for flip model swapchains is required now (since we are
    // targeting Windows 10+), but the option for using the old model is still
    // there. (some features are not supported then, however)
    useLegacySwapchainModel = qEnvironmentVariableIntValue("QT_D3D_NO_FLIP");

    qCDebug(QRHI_LOG_INFO, "FLIP_* swapchain supported = true, ALLOW_TEARING supported = %s, use legacy (non-FLIP) model = %s",
            supportsAllowTearing ? "true" : "false",
            useLegacySwapchainModel ? "true" : "false");

    if (!importedDeviceAndContext) {
        IDXGIAdapter1 *adapter;
        int requestedAdapterIndex = -1;
        if (qEnvironmentVariableIsSet("QT_D3D_ADAPTER_INDEX"))
            requestedAdapterIndex = qEnvironmentVariableIntValue("QT_D3D_ADAPTER_INDEX");

        // The importParams may specify an adapter by the luid, take that into account.
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

        activeAdapter = nullptr;
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
                driverInfoStruct.deviceName = name.toUtf8();
                driverInfoStruct.deviceId = desc.DeviceId;
                driverInfoStruct.vendorId = desc.VendorId;
                qCDebug(QRHI_LOG_INFO, "  using this adapter");
            } else {
                adapter->Release();
            }
        }
        if (!activeAdapter) {
            qWarning("No adapter");
            return false;
        }

        // Normally we won't specify a requested feature level list,
        // except when a level was specified in importParams.
        QVarLengthArray<D3D_FEATURE_LEVEL, 4> requestedFeatureLevels;
        bool requestFeatureLevels = false;
        if (featureLevel) {
            requestFeatureLevels = true;
            requestedFeatureLevels.append(featureLevel);
        }

        ID3D11DeviceContext *ctx = nullptr;
        HRESULT hr = D3D11CreateDevice(activeAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, devFlags,
                                       requestFeatureLevels ? requestedFeatureLevels.constData() : nullptr,
                                       requestFeatureLevels ? requestedFeatureLevels.count() : 0,
                                       D3D11_SDK_VERSION,
                                       &dev, &featureLevel, &ctx);
        // We cannot assume that D3D11_CREATE_DEVICE_DEBUG is always available. Retry without it, if needed.
        if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING && debugLayer) {
            qCDebug(QRHI_LOG_INFO, "Debug layer was requested but is not available. "
                                   "Attempting to create D3D11 device without it.");
            devFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
            hr = D3D11CreateDevice(activeAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, devFlags,
                                   requestFeatureLevels ? requestedFeatureLevels.constData() : nullptr,
                                   requestFeatureLevels ? requestedFeatureLevels.count() : 0,
                                   D3D11_SDK_VERSION,
                                   &dev, &featureLevel, &ctx);
        }
        if (FAILED(hr)) {
            qWarning("Failed to create D3D11 device and context: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }

        // Test if creating a Shader Model 5.0 vertex shader works; we want to
        // fail already in create() if that's not the case.
        ID3D11VertexShader *testShader = nullptr;
        if (SUCCEEDED(dev->CreateVertexShader(g_testVertexShader, sizeof(g_testVertexShader), nullptr, &testShader))) {
            testShader->Release();
        } else {
            qWarning("D3D11 smoke test failed (failed to create vertex shader)");
            ctx->Release();
            return false;
        }

        const bool supports11_1 = SUCCEEDED(ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void **>(&context)));
        ctx->Release();
        if (!supports11_1) {
            qWarning("ID3D11DeviceContext1 not supported");
            return false;
        }

        D3D11_FEATURE_DATA_D3D11_OPTIONS features = {};
        if (SUCCEEDED(dev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &features, sizeof(features)))) {
            // The D3D _runtime_ may be 11.1, but the underlying _driver_ may
            // still not support this D3D_FEATURE_LEVEL_11_1 feature. (e.g.
            // because it only does 11_0)
            if (!features.ConstantBufferOffsetting) {
                qWarning("Constant buffer offsetting is not supported by the driver");
                return false;
            }
        } else {
            qWarning("Failed to query D3D11_FEATURE_D3D11_OPTIONS");
            return false;
        }
    } else {
        Q_ASSERT(dev && context);
        featureLevel = dev->GetFeatureLevel();
        IDXGIDevice *dxgiDev = nullptr;
        if (SUCCEEDED(dev->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(&dxgiDev)))) {
            IDXGIAdapter *adapter = nullptr;
            if (SUCCEEDED(dxgiDev->GetAdapter(&adapter))) {
                DXGI_ADAPTER_DESC desc;
                adapter->GetDesc(&desc);
                adapterLuid = desc.AdapterLuid;
                driverInfoStruct.deviceName = QString::fromUtf16(reinterpret_cast<char16_t *>(desc.Description)).toUtf8();
                driverInfoStruct.deviceId = desc.DeviceId;
                driverInfoStruct.vendorId = desc.VendorId;
                adapter->Release();
            }
            dxgiDev->Release();
        }
        qCDebug(QRHI_LOG_INFO, "Using imported device %p", dev);
    }

    if (FAILED(context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), reinterpret_cast<void **>(&annotations))))
        annotations = nullptr;

    if (flags.testFlag(QRhi::EnableTimestamps)) {
        ofr.timestamps.prepare(2, this);
        // timestamp queries are optional so we can go on even if they failed
    }

    deviceLost = false;

    nativeHandlesStruct.dev = dev;
    nativeHandlesStruct.context = context;
    nativeHandlesStruct.featureLevel = featureLevel;
    nativeHandlesStruct.adapterLuidLow = adapterLuid.LowPart;
    nativeHandlesStruct.adapterLuidHigh = adapterLuid.HighPart;

    return true;
}

void QRhiD3D11::clearShaderCache()
{
    for (Shader &s : m_shaderCache)
        s.s->Release();

    m_shaderCache.clear();
}

void QRhiD3D11::destroy()
{
    finishActiveReadbacks();

    clearShaderCache();

    ofr.timestamps.destroy();

    if (annotations) {
        annotations->Release();
        annotations = nullptr;
    }

    if (!importedDeviceAndContext) {
        if (context) {
            context->Release();
            context = nullptr;
        }
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
}

void QRhiD3D11::reportLiveObjects(ID3D11Device *device)
{
    // this works only when params.enableDebugLayer was true
    ID3D11Debug *debug;
    if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void **>(&debug)))) {
        debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        debug->Release();
    }
}

QList<int> QRhiD3D11::supportedSampleCounts() const
{
    return { 1, 2, 4, 8 };
}

DXGI_SAMPLE_DESC QRhiD3D11::effectiveSampleCount(int sampleCount) const
{
    DXGI_SAMPLE_DESC desc;
    desc.Count = 1;
    desc.Quality = 0;

    // Stay compatible with QSurfaceFormat and friends where samples == 0 means the same as 1.
    int s = qBound(1, sampleCount, 64);

    if (!supportedSampleCounts().contains(s)) {
        qWarning("Attempted to set unsupported sample count %d", sampleCount);
        return desc;
    }

    desc.Count = UINT(s);
    if (s > 1)
        desc.Quality = UINT(D3D11_STANDARD_MULTISAMPLE_PATTERN);
    else
        desc.Quality = 0;

    return desc;
}

QRhiSwapChain *QRhiD3D11::createSwapChain()
{
    return new QD3D11SwapChain(this);
}

QRhiBuffer *QRhiD3D11::createBuffer(QRhiBuffer::Type type, QRhiBuffer::UsageFlags usage, quint32 size)
{
    return new QD3D11Buffer(this, type, usage, size);
}

int QRhiD3D11::ubufAlignment() const
{
    return 256;
}

bool QRhiD3D11::isYUpInFramebuffer() const
{
    return false;
}

bool QRhiD3D11::isYUpInNDC() const
{
    return true;
}

bool QRhiD3D11::isClipDepthZeroToOne() const
{
    return true;
}

QMatrix4x4 QRhiD3D11::clipSpaceCorrMatrix() const
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

bool QRhiD3D11::isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const
{
    Q_UNUSED(flags);

    if (format >= QRhiTexture::ETC2_RGB8 && format <= QRhiTexture::ASTC_12x12)
        return false;

    return true;
}

bool QRhiD3D11::isFeatureSupported(QRhi::Feature feature) const
{
    switch (feature) {
    case QRhi::MultisampleTexture:
        return true;
    case QRhi::MultisampleRenderBuffer:
        return true;
    case QRhi::DebugMarkers:
        return annotations != nullptr;
    case QRhi::Timestamps:
        return true;
    case QRhi::Instancing:
        return true;
    case QRhi::CustomInstanceStepRate:
        return true;
    case QRhi::PrimitiveRestart:
        return true;
    case QRhi::NonDynamicUniformBuffers:
        return false; // because UpdateSubresource cannot deal with this
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
        return true;
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
        return true;
    case QRhi::HalfAttributes:
        return true;
    case QRhi::RenderToOneDimensionalTexture:
        return true;
    case QRhi::ThreeDimensionalTextureMipmaps:
        return true;
    default:
        Q_UNREACHABLE();
        return false;
    }
}

int QRhiD3D11::resourceLimit(QRhi::ResourceLimit limit) const
{
    switch (limit) {
    case QRhi::TextureSizeMin:
        return 1;
    case QRhi::TextureSizeMax:
        return D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    case QRhi::MaxColorAttachments:
        return 8;
    case QRhi::FramesInFlight:
        // From our perspective. What D3D does internally is another question
        // (there could be pipelining, helped f.ex. by our MAP_DISCARD based
        // uniform buffer update strategy), but that's out of our hands and
        // does not concern us here.
        return 1;
    case QRhi::MaxAsyncReadbackFrames:
        return 1;
    case QRhi::MaxThreadGroupsPerDimension:
        return D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    case QRhi::MaxThreadsPerThreadGroup:
        return D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
    case QRhi::MaxThreadGroupX:
        return D3D11_CS_THREAD_GROUP_MAX_X;
    case QRhi::MaxThreadGroupY:
        return D3D11_CS_THREAD_GROUP_MAX_Y;
    case QRhi::MaxThreadGroupZ:
        return D3D11_CS_THREAD_GROUP_MAX_Z;
    case QRhi::TextureArraySizeMax:
        return D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    case QRhi::MaxUniformBufferRange:
        return 65536;
    case QRhi::MaxVertexInputs:
        return D3D11_VS_INPUT_REGISTER_COUNT;
    case QRhi::MaxVertexOutputs:
        return D3D11_VS_OUTPUT_REGISTER_COUNT;
    default:
        Q_UNREACHABLE();
        return 0;
    }
}

const QRhiNativeHandles *QRhiD3D11::nativeHandles()
{
    return &nativeHandlesStruct;
}

QRhiDriverInfo QRhiD3D11::driverInfo() const
{
    return driverInfoStruct;
}

QRhiStats QRhiD3D11::statistics()
{
    QRhiStats result;
    result.totalPipelineCreationTime = totalPipelineCreationTime();
    return result;
}

bool QRhiD3D11::makeThreadLocalNativeContextCurrent()
{
    // not applicable
    return false;
}

void QRhiD3D11::releaseCachedResources()
{
    clearShaderCache();
    m_bytecodeCache.clear();
}

bool QRhiD3D11::isDeviceLost() const
{
    return deviceLost;
}

struct QD3D11PipelineCacheDataHeader
{
    quint32 rhiId;
    quint32 arch;
    // no need for driver specifics
    quint32 count;
    quint32 dataSize;
};

QByteArray QRhiD3D11::pipelineCacheData()
{
    QByteArray data;
    if (m_bytecodeCache.isEmpty())
        return data;

    QD3D11PipelineCacheDataHeader header;
    memset(&header, 0, sizeof(header));
    header.rhiId = pipelineCacheRhiId();
    header.arch = quint32(sizeof(void*));
    header.count = m_bytecodeCache.count();

    const size_t dataOffset = sizeof(header);
    size_t dataSize = 0;
    for (auto it = m_bytecodeCache.cbegin(), end = m_bytecodeCache.cend(); it != end; ++it) {
        BytecodeCacheKey key = it.key();
        QByteArray bytecode = it.value();
        dataSize +=
                  sizeof(quint32) + key.sourceHash.size()
                + sizeof(quint32) + key.target.size()
                + sizeof(quint32) + key.entryPoint.size()
                + sizeof(quint32) // compileFlags
                + sizeof(quint32) + bytecode.size();
    }

    QByteArray buf(dataOffset + dataSize, Qt::Uninitialized);
    char *p = buf.data() + dataOffset;
    for (auto it = m_bytecodeCache.cbegin(), end = m_bytecodeCache.cend(); it != end; ++it) {
        BytecodeCacheKey key = it.key();
        QByteArray bytecode = it.value();

        quint32 i = key.sourceHash.size();
        memcpy(p, &i, 4);
        p += 4;
        memcpy(p, key.sourceHash.constData(), key.sourceHash.size());
        p += key.sourceHash.size();

        i = key.target.size();
        memcpy(p, &i, 4);
        p += 4;
        memcpy(p, key.target.constData(), key.target.size());
        p += key.target.size();

        i = key.entryPoint.size();
        memcpy(p, &i, 4);
        p += 4;
        memcpy(p, key.entryPoint.constData(), key.entryPoint.size());
        p += key.entryPoint.size();

        quint32 f = key.compileFlags;
        memcpy(p, &f, 4);
        p += 4;

        i = bytecode.size();
        memcpy(p, &i, 4);
        p += 4;
        memcpy(p, bytecode.constData(), bytecode.size());
        p += bytecode.size();
    }
    Q_ASSERT(p == buf.data() + dataOffset + dataSize);

    header.dataSize = quint32(dataSize);
    memcpy(buf.data(), &header, sizeof(header));

    return buf;
}

void QRhiD3D11::setPipelineCacheData(const QByteArray &data)
{
    if (data.isEmpty())
        return;

    const size_t headerSize = sizeof(QD3D11PipelineCacheDataHeader);
    if (data.size() < qsizetype(headerSize)) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: Invalid blob size (header incomplete)");
        return;
    }
    const size_t dataOffset = headerSize;
    QD3D11PipelineCacheDataHeader header;
    memcpy(&header, data.constData(), headerSize);

    const quint32 rhiId = pipelineCacheRhiId();
    if (header.rhiId != rhiId) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: The data is for a different QRhi version or backend (%u, %u)",
                rhiId, header.rhiId);
        return;
    }
    const quint32 arch = quint32(sizeof(void*));
    if (header.arch != arch) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: Architecture does not match (%u, %u)",
                arch, header.arch);
        return;
    }
    if (header.count == 0)
        return;

    if (data.size() < qsizetype(dataOffset + header.dataSize)) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: Invalid blob size (data incomplete)");
        return;
    }

    m_bytecodeCache.clear();

    const char *p = data.constData() + dataOffset;
    for (quint32 i = 0; i < header.count; ++i) {
        quint32 len = 0;
        memcpy(&len, p, 4);
        p += 4;
        QByteArray sourceHash(len, Qt::Uninitialized);
        memcpy(sourceHash.data(), p, len);
        p += len;

        memcpy(&len, p, 4);
        p += 4;
        QByteArray target(len, Qt::Uninitialized);
        memcpy(target.data(), p, len);
        p += len;

        memcpy(&len, p, 4);
        p += 4;
        QByteArray entryPoint(len, Qt::Uninitialized);
        memcpy(entryPoint.data(), p, len);
        p += len;

        quint32 flags;
        memcpy(&flags, p, 4);
        p += 4;

        memcpy(&len, p, 4);
        p += 4;
        QByteArray bytecode(len, Qt::Uninitialized);
        memcpy(bytecode.data(), p, len);
        p += len;

        BytecodeCacheKey cacheKey;
        cacheKey.sourceHash = sourceHash;
        cacheKey.target = target;
        cacheKey.entryPoint = entryPoint;
        cacheKey.compileFlags = flags;

        m_bytecodeCache.insert(cacheKey, bytecode);
    }

    qCDebug(QRHI_LOG_INFO, "Seeded bytecode cache with %d shaders", int(m_bytecodeCache.count()));
}

QRhiRenderBuffer *QRhiD3D11::createRenderBuffer(QRhiRenderBuffer::Type type, const QSize &pixelSize,
                                                int sampleCount, QRhiRenderBuffer::Flags flags,
                                                QRhiTexture::Format backingFormatHint)
{
    return new QD3D11RenderBuffer(this, type, pixelSize, sampleCount, flags, backingFormatHint);
}

QRhiTexture *QRhiD3D11::createTexture(QRhiTexture::Format format,
                                      const QSize &pixelSize, int depth, int arraySize,
                                      int sampleCount, QRhiTexture::Flags flags)
{
    return new QD3D11Texture(this, format, pixelSize, depth, arraySize, sampleCount, flags);
}

QRhiSampler *QRhiD3D11::createSampler(QRhiSampler::Filter magFilter, QRhiSampler::Filter minFilter,
                                      QRhiSampler::Filter mipmapMode,
                                      QRhiSampler::AddressMode u, QRhiSampler::AddressMode v, QRhiSampler::AddressMode w)
{
    return new QD3D11Sampler(this, magFilter, minFilter, mipmapMode, u, v, w);
}

QRhiTextureRenderTarget *QRhiD3D11::createTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                              QRhiTextureRenderTarget::Flags flags)
{
    return new QD3D11TextureRenderTarget(this, desc, flags);
}

QRhiGraphicsPipeline *QRhiD3D11::createGraphicsPipeline()
{
    return new QD3D11GraphicsPipeline(this);
}

QRhiComputePipeline *QRhiD3D11::createComputePipeline()
{
    return new QD3D11ComputePipeline(this);
}

QRhiShaderResourceBindings *QRhiD3D11::createShaderResourceBindings()
{
    return new QD3D11ShaderResourceBindings(this);
}

void QRhiD3D11::setGraphicsPipeline(QRhiCommandBuffer *cb, QRhiGraphicsPipeline *ps)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);
    QD3D11GraphicsPipeline *psD = QRHI_RES(QD3D11GraphicsPipeline, ps);
    const bool pipelineChanged = cbD->currentGraphicsPipeline != ps || cbD->currentPipelineGeneration != psD->generation;

    if (pipelineChanged) {
        cbD->currentGraphicsPipeline = ps;
        cbD->currentComputePipeline = nullptr;
        cbD->currentPipelineGeneration = psD->generation;

        QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QD3D11CommandBuffer::Command::BindGraphicsPipeline;
        cmd.args.bindGraphicsPipeline.ps = psD;
    }
}

static const int RBM_SUPPORTED_STAGES = 6;
static const int RBM_VERTEX = 0;
static const int RBM_HULL = 1;
static const int RBM_DOMAIN = 2;
static const int RBM_GEOMETRY = 3;
static const int RBM_FRAGMENT = 4;
static const int RBM_COMPUTE = 5;

void QRhiD3D11::setShaderResources(QRhiCommandBuffer *cb, QRhiShaderResourceBindings *srb,
                                   int dynamicOffsetCount,
                                   const QRhiCommandBuffer::DynamicOffset *dynamicOffsets)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass != QD3D11CommandBuffer::NoPass);
    QD3D11GraphicsPipeline *gfxPsD = QRHI_RES(QD3D11GraphicsPipeline, cbD->currentGraphicsPipeline);
    QD3D11ComputePipeline *compPsD = QRHI_RES(QD3D11ComputePipeline, cbD->currentComputePipeline);

    if (!srb) {
        if (gfxPsD)
            srb = gfxPsD->m_shaderResourceBindings;
        else
            srb = compPsD->m_shaderResourceBindings;
    }

    QD3D11ShaderResourceBindings *srbD = QRHI_RES(QD3D11ShaderResourceBindings, srb);

    bool srbUpdate = false;
    for (int i = 0, ie = srbD->sortedBindings.count(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = shaderResourceBindingData(srbD->sortedBindings.at(i));
        QD3D11ShaderResourceBindings::BoundResourceData &bd(srbD->boundResourceData[i]);
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, b->u.ubuf.buf);
            // NonDynamicUniformBuffers is not supported by this backend
            Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic && bufD->m_usage.testFlag(QRhiBuffer::UniformBuffer));

            executeBufferHostWrites(bufD);

            if (bufD->generation != bd.ubuf.generation || bufD->m_id != bd.ubuf.id) {
                srbUpdate = true;
                bd.ubuf.id = bufD->m_id;
                bd.ubuf.generation = bufD->generation;
            }
        }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
        case QRhiShaderResourceBinding::Texture:
        case QRhiShaderResourceBinding::Sampler:
        {
            const QRhiShaderResourceBinding::Data::TextureAndOrSamplerData *data = &b->u.stex;
            if (bd.stex.count != data->count) {
                bd.stex.count = data->count;
                srbUpdate = true;
            }
            for (int elem = 0; elem < data->count; ++elem) {
                QD3D11Texture *texD = QRHI_RES(QD3D11Texture, data->texSamplers[elem].tex);
                QD3D11Sampler *samplerD = QRHI_RES(QD3D11Sampler, data->texSamplers[elem].sampler);
                // We use the same code path for both combined and separate
                // images and samplers, so tex or sampler (but not both) can be
                // null here.
                Q_ASSERT(texD || samplerD);
                const quint64 texId = texD ? texD->m_id : 0;
                const uint texGen = texD ? texD->generation : 0;
                const quint64 samplerId = samplerD ? samplerD->m_id : 0;
                const uint samplerGen = samplerD ? samplerD->generation : 0;
                if (texGen != bd.stex.d[elem].texGeneration
                        || texId != bd.stex.d[elem].texId
                        || samplerGen != bd.stex.d[elem].samplerGeneration
                        || samplerId != bd.stex.d[elem].samplerId)
                {
                    srbUpdate = true;
                    bd.stex.d[elem].texId = texId;
                    bd.stex.d[elem].texGeneration = texGen;
                    bd.stex.d[elem].samplerId = samplerId;
                    bd.stex.d[elem].samplerGeneration = samplerGen;
                }
            }
        }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
        {
            QD3D11Texture *texD = QRHI_RES(QD3D11Texture, b->u.simage.tex);
            if (texD->generation != bd.simage.generation || texD->m_id != bd.simage.id) {
                srbUpdate = true;
                bd.simage.id = texD->m_id;
                bd.simage.generation = texD->generation;
            }
        }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
        case QRhiShaderResourceBinding::BufferStore:
        case QRhiShaderResourceBinding::BufferLoadStore:
        {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, b->u.sbuf.buf);
            if (bufD->generation != bd.sbuf.generation || bufD->m_id != bd.sbuf.id) {
                srbUpdate = true;
                bd.sbuf.id = bufD->m_id;
                bd.sbuf.generation = bufD->generation;
            }
        }
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    if (srbUpdate) {
        const QShader::NativeResourceBindingMap *resBindMaps[RBM_SUPPORTED_STAGES];
        memset(resBindMaps, 0, sizeof(resBindMaps));
        if (gfxPsD) {
            resBindMaps[RBM_VERTEX] = &gfxPsD->vs.nativeResourceBindingMap;
            resBindMaps[RBM_HULL] = &gfxPsD->hs.nativeResourceBindingMap;
            resBindMaps[RBM_DOMAIN] = &gfxPsD->ds.nativeResourceBindingMap;
            resBindMaps[RBM_GEOMETRY] = &gfxPsD->gs.nativeResourceBindingMap;
            resBindMaps[RBM_FRAGMENT] = &gfxPsD->fs.nativeResourceBindingMap;
        } else {
            resBindMaps[RBM_COMPUTE] = &compPsD->cs.nativeResourceBindingMap;
        }
        updateShaderResourceBindings(srbD, resBindMaps);
    }

    const bool srbChanged = gfxPsD ? (cbD->currentGraphicsSrb != srb) : (cbD->currentComputeSrb != srb);
    const bool srbRebuilt = cbD->currentSrbGeneration != srbD->generation;

    if (srbChanged || srbRebuilt || srbUpdate || srbD->hasDynamicOffset) {
        if (gfxPsD) {
            cbD->currentGraphicsSrb = srb;
            cbD->currentComputeSrb = nullptr;
        } else {
            cbD->currentGraphicsSrb = nullptr;
            cbD->currentComputeSrb = srb;
        }
        cbD->currentSrbGeneration = srbD->generation;

        QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QD3D11CommandBuffer::Command::BindShaderResources;
        cmd.args.bindShaderResources.srb = srbD;
        // dynamic offsets have to be applied at the time of executing the bind
        // operations, not here
        cmd.args.bindShaderResources.offsetOnlyChange = !srbChanged && !srbRebuilt && !srbUpdate && srbD->hasDynamicOffset;
        cmd.args.bindShaderResources.dynamicOffsetCount = 0;
        if (srbD->hasDynamicOffset) {
            if (dynamicOffsetCount < QD3D11CommandBuffer::MAX_DYNAMIC_OFFSET_COUNT) {
                cmd.args.bindShaderResources.dynamicOffsetCount = dynamicOffsetCount;
                uint *p = cmd.args.bindShaderResources.dynamicOffsetPairs;
                for (int i = 0; i < dynamicOffsetCount; ++i) {
                    const QRhiCommandBuffer::DynamicOffset &dynOfs(dynamicOffsets[i]);
                    const uint binding = uint(dynOfs.first);
                    Q_ASSERT(aligned(dynOfs.second, 256u) == dynOfs.second);
                    const quint32 offsetInConstants = dynOfs.second / 16;
                    *p++ = binding;
                    *p++ = offsetInConstants;
                }
            } else {
                qWarning("Too many dynamic offsets (%d, max is %d)",
                         dynamicOffsetCount, QD3D11CommandBuffer::MAX_DYNAMIC_OFFSET_COUNT);
            }
        }
    }
}

void QRhiD3D11::setVertexInput(QRhiCommandBuffer *cb,
                               int startBinding, int bindingCount, const QRhiCommandBuffer::VertexInput *bindings,
                               QRhiBuffer *indexBuf, quint32 indexOffset, QRhiCommandBuffer::IndexFormat indexFormat)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);

    bool needsBindVBuf = false;
    for (int i = 0; i < bindingCount; ++i) {
        const int inputSlot = startBinding + i;
        QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, bindings[i].first);
        Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::VertexBuffer));
        if (bufD->m_type == QRhiBuffer::Dynamic)
            executeBufferHostWrites(bufD);

        if (cbD->currentVertexBuffers[inputSlot] != bufD->buffer
                || cbD->currentVertexOffsets[inputSlot] != bindings[i].second)
        {
            needsBindVBuf = true;
            cbD->currentVertexBuffers[inputSlot] = bufD->buffer;
            cbD->currentVertexOffsets[inputSlot] = bindings[i].second;
        }
    }

    if (needsBindVBuf) {
        QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QD3D11CommandBuffer::Command::BindVertexBuffers;
        cmd.args.bindVertexBuffers.startSlot = startBinding;
        if (bindingCount > QD3D11CommandBuffer::MAX_VERTEX_BUFFER_BINDING_COUNT) {
            qWarning("Too many vertex buffer bindings (%d, max is %d)",
                     bindingCount, QD3D11CommandBuffer::MAX_VERTEX_BUFFER_BINDING_COUNT);
            bindingCount = QD3D11CommandBuffer::MAX_VERTEX_BUFFER_BINDING_COUNT;
        }
        cmd.args.bindVertexBuffers.slotCount = bindingCount;
        QD3D11GraphicsPipeline *psD = QRHI_RES(QD3D11GraphicsPipeline, cbD->currentGraphicsPipeline);
        const QRhiVertexInputLayout &inputLayout(psD->m_vertexInputLayout);
        const int inputBindingCount = inputLayout.cendBindings() - inputLayout.cbeginBindings();
        for (int i = 0, ie = qMin(bindingCount, inputBindingCount); i != ie; ++i) {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, bindings[i].first);
            cmd.args.bindVertexBuffers.buffers[i] = bufD->buffer;
            cmd.args.bindVertexBuffers.offsets[i] = bindings[i].second;
            cmd.args.bindVertexBuffers.strides[i] = inputLayout.bindingAt(i)->stride();
        }
    }

    if (indexBuf) {
        QD3D11Buffer *ibufD = QRHI_RES(QD3D11Buffer, indexBuf);
        Q_ASSERT(ibufD->m_usage.testFlag(QRhiBuffer::IndexBuffer));
        if (ibufD->m_type == QRhiBuffer::Dynamic)
            executeBufferHostWrites(ibufD);

        const DXGI_FORMAT dxgiFormat = indexFormat == QRhiCommandBuffer::IndexUInt16 ? DXGI_FORMAT_R16_UINT
                                                                                     : DXGI_FORMAT_R32_UINT;
        if (cbD->currentIndexBuffer != ibufD->buffer
                || cbD->currentIndexOffset != indexOffset
                || cbD->currentIndexFormat != dxgiFormat)
        {
            cbD->currentIndexBuffer = ibufD->buffer;
            cbD->currentIndexOffset = indexOffset;
            cbD->currentIndexFormat = dxgiFormat;

            QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QD3D11CommandBuffer::Command::BindIndexBuffer;
            cmd.args.bindIndexBuffer.buffer = ibufD->buffer;
            cmd.args.bindIndexBuffer.offset = indexOffset;
            cmd.args.bindIndexBuffer.format = dxgiFormat;
        }
    }
}

void QRhiD3D11::setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);
    Q_ASSERT(cbD->currentTarget);
    const QSize outputSize = cbD->currentTarget->pixelSize();

    // d3d expects top-left, QRhiViewport is bottom-left
    float x, y, w, h;
    if (!qrhi_toTopLeftRenderTargetRect<UnBounded>(outputSize, viewport.viewport(), &x, &y, &w, &h))
        return;

    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::Viewport;
    cmd.args.viewport.x = x;
    cmd.args.viewport.y = y;
    cmd.args.viewport.w = w;
    cmd.args.viewport.h = h;
    cmd.args.viewport.d0 = viewport.minDepth();
    cmd.args.viewport.d1 = viewport.maxDepth();
}

void QRhiD3D11::setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);
    Q_ASSERT(cbD->currentTarget);
    const QSize outputSize = cbD->currentTarget->pixelSize();

    // d3d expects top-left, QRhiScissor is bottom-left
    int x, y, w, h;
    if (!qrhi_toTopLeftRenderTargetRect<Bounded>(outputSize, scissor.scissor(), &x, &y, &w, &h))
        return;

    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::Scissor;
    cmd.args.scissor.x = x;
    cmd.args.scissor.y = y;
    cmd.args.scissor.w = w;
    cmd.args.scissor.h = h;
}

void QRhiD3D11::setBlendConstants(QRhiCommandBuffer *cb, const QColor &c)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);

    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::BlendConstants;
    cmd.args.blendConstants.ps = QRHI_RES(QD3D11GraphicsPipeline, cbD->currentGraphicsPipeline);
    cmd.args.blendConstants.c[0] = float(c.redF());
    cmd.args.blendConstants.c[1] = float(c.greenF());
    cmd.args.blendConstants.c[2] = float(c.blueF());
    cmd.args.blendConstants.c[3] = float(c.alphaF());
}

void QRhiD3D11::setStencilRef(QRhiCommandBuffer *cb, quint32 refValue)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);

    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::StencilRef;
    cmd.args.stencilRef.ps = QRHI_RES(QD3D11GraphicsPipeline, cbD->currentGraphicsPipeline);
    cmd.args.stencilRef.ref = refValue;
}

void QRhiD3D11::draw(QRhiCommandBuffer *cb, quint32 vertexCount,
                     quint32 instanceCount, quint32 firstVertex, quint32 firstInstance)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);

    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::Draw;
    cmd.args.draw.ps = QRHI_RES(QD3D11GraphicsPipeline, cbD->currentGraphicsPipeline);
    cmd.args.draw.vertexCount = vertexCount;
    cmd.args.draw.instanceCount = instanceCount;
    cmd.args.draw.firstVertex = firstVertex;
    cmd.args.draw.firstInstance = firstInstance;
}

void QRhiD3D11::drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                            quint32 instanceCount, quint32 firstIndex, qint32 vertexOffset, quint32 firstInstance)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);

    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::DrawIndexed;
    cmd.args.drawIndexed.ps = QRHI_RES(QD3D11GraphicsPipeline, cbD->currentGraphicsPipeline);
    cmd.args.drawIndexed.indexCount = indexCount;
    cmd.args.drawIndexed.instanceCount = instanceCount;
    cmd.args.drawIndexed.firstIndex = firstIndex;
    cmd.args.drawIndexed.vertexOffset = vertexOffset;
    cmd.args.drawIndexed.firstInstance = firstInstance;
}

void QRhiD3D11::debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name)
{
    if (!debugMarkers || !annotations)
        return;

    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::DebugMarkBegin;
    qstrncpy(cmd.args.debugMark.s, name.constData(), sizeof(cmd.args.debugMark.s));
}

void QRhiD3D11::debugMarkEnd(QRhiCommandBuffer *cb)
{
    if (!debugMarkers || !annotations)
        return;

    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::DebugMarkEnd;
}

void QRhiD3D11::debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg)
{
    if (!debugMarkers || !annotations)
        return;

    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::DebugMarkMsg;
    qstrncpy(cmd.args.debugMark.s, msg.constData(), sizeof(cmd.args.debugMark.s));
}

const QRhiNativeHandles *QRhiD3D11::nativeHandles(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
    return nullptr;
}

void QRhiD3D11::beginExternal(QRhiCommandBuffer *cb)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    // no timestampSwapChain, in order to avoid timestamp mess
    executeCommandBuffer(cbD);
    cbD->resetCommands();
}

void QRhiD3D11::endExternal(QRhiCommandBuffer *cb)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->commands.isEmpty());
    cbD->resetCachedState();
    if (cbD->currentTarget) { // could be compute, no rendertarget then
        QD3D11CommandBuffer::Command &fbCmd(cbD->commands.get());
        fbCmd.cmd = QD3D11CommandBuffer::Command::SetRenderTarget;
        fbCmd.args.setRenderTarget.rt = cbD->currentTarget;
    }
}

double QRhiD3D11::lastCompletedGpuTime(QRhiCommandBuffer *cb)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    return cbD->lastGpuTime;
}

QRhi::FrameOpResult QRhiD3D11::beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);

    QD3D11SwapChain *swapChainD = QRHI_RES(QD3D11SwapChain, swapChain);
    contextState.currentSwapChain = swapChainD;
    const int currentFrameSlot = swapChainD->currentFrameSlot;

    swapChainD->cb.resetState();

    swapChainD->rt.d.rtv[0] = swapChainD->sampleDesc.Count > 1 ?
                swapChainD->msaaRtv[currentFrameSlot] : swapChainD->backBufferRtv;
    swapChainD->rt.d.dsv = swapChainD->ds ? swapChainD->ds->dsv : nullptr;

    finishActiveReadbacks();

    if (swapChainD->timestamps.active[currentFrameSlot]) {
        double elapsedSec = 0;
        if (swapChainD->timestamps.tryQueryTimestamps(currentFrameSlot, context, &elapsedSec))
            swapChainD->cb.lastGpuTime = elapsedSec;
    }

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiD3D11::endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags)
{
    QD3D11SwapChain *swapChainD = QRHI_RES(QD3D11SwapChain, swapChain);
    Q_ASSERT(contextState.currentSwapChain = swapChainD);
    const int currentFrameSlot = swapChainD->currentFrameSlot;

    ID3D11Query *tsDisjoint = swapChainD->timestamps.disjointQuery[currentFrameSlot];
    const int tsIdx = QD3D11SwapChain::BUFFER_COUNT * currentFrameSlot;
    ID3D11Query *tsStart = swapChainD->timestamps.query[tsIdx];
    ID3D11Query *tsEnd = swapChainD->timestamps.query[tsIdx + 1];
    const bool recordTimestamps = tsDisjoint && tsStart && tsEnd && !swapChainD->timestamps.active[currentFrameSlot];

    // send all commands to the context
    if (recordTimestamps)
        executeCommandBuffer(&swapChainD->cb, swapChainD);
    else
        executeCommandBuffer(&swapChainD->cb);

    if (swapChainD->sampleDesc.Count > 1) {
        context->ResolveSubresource(swapChainD->backBufferTex, 0,
                                    swapChainD->msaaTex[currentFrameSlot], 0,
                                    swapChainD->colorFormat);
    }

    // this is here because we want to include the time spent on the resolve as well
    if (recordTimestamps) {
        context->End(tsEnd);
        context->End(tsDisjoint);
        swapChainD->timestamps.active[currentFrameSlot] = true;
    }

    if (!flags.testFlag(QRhi::SkipPresent)) {
        UINT presentFlags = 0;
        if (swapChainD->swapInterval == 0 && (swapChainD->swapChainFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING))
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        if (!swapChainD->swapChain) {
            qWarning("Failed to present: IDXGISwapChain is unavailable");
            return QRhi::FrameOpError;
        }
        HRESULT hr = swapChainD->swapChain->Present(swapChainD->swapInterval, presentFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            qWarning("Device loss detected in Present()");
            deviceLost = true;
            return QRhi::FrameOpDeviceLost;
        } else if (FAILED(hr)) {
            qWarning("Failed to present: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return QRhi::FrameOpError;
        }

        if (dcompDevice && swapChainD->dcompTarget && swapChainD->dcompVisual)
            dcompDevice->Commit();

        // move on to the next buffer
        swapChainD->currentFrameSlot = (swapChainD->currentFrameSlot + 1) % QD3D11SwapChain::BUFFER_COUNT;
    } else {
        context->Flush();
    }

    swapChainD->frameCount += 1;
    contextState.currentSwapChain = nullptr;

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiD3D11::beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);
    ofr.active = true;

    ofr.cbWrapper.resetState();
    *cb = &ofr.cbWrapper;

    if (ofr.timestamps.active[ofr.timestampIdx]) {
        double elapsedSec = 0;
        if (ofr.timestamps.tryQueryTimestamps(ofr.timestampIdx, context, &elapsedSec))
            ofr.cbWrapper.lastGpuTime = elapsedSec;
    }

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiD3D11::endOffscreenFrame(QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    ofr.active = false;

    ID3D11Query *tsDisjoint = ofr.timestamps.disjointQuery[ofr.timestampIdx];
    ID3D11Query *tsStart = ofr.timestamps.query[ofr.timestampIdx * 2];
    ID3D11Query *tsEnd = ofr.timestamps.query[ofr.timestampIdx * 2 + 1];
    const bool recordTimestamps = tsDisjoint && tsStart && tsEnd && !ofr.timestamps.active[ofr.timestampIdx];
    if (recordTimestamps) {
        context->Begin(tsDisjoint);
        context->End(tsStart); // record timestamp; no Begin() for D3D11_QUERY_TIMESTAMP
    }

    executeCommandBuffer(&ofr.cbWrapper);
    context->Flush();

    finishActiveReadbacks();

    if (recordTimestamps) {
        context->End(tsEnd);
        context->End(tsDisjoint);
        ofr.timestamps.active[ofr.timestampIdx] = true;
        ofr.timestampIdx = (ofr.timestampIdx + 1) % 2;
    }

    return QRhi::FrameOpSuccess;
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

    case QRhiTexture::D16:
        return DXGI_FORMAT_R16_TYPELESS;
    case QRhiTexture::D24:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case QRhiTexture::D24S8:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case QRhiTexture::D32F:
        return DXGI_FORMAT_R32_TYPELESS;

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
        qWarning("QRhiD3D11 does not support ETC2 textures");
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
        qWarning("QRhiD3D11 does not support ASTC textures");
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    default:
        Q_UNREACHABLE();
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
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

static inline bool isDepthTextureFormat(QRhiTexture::Format format)
{
    switch (format) {
    case QRhiTexture::Format::D16:
    case QRhiTexture::Format::D24:
    case QRhiTexture::Format::D24S8:
    case QRhiTexture::Format::D32F:
        return true;

    default:
        return false;
    }
}

QRhi::FrameOpResult QRhiD3D11::finish()
{
    if (inFrame) {
        if (ofr.active) {
            Q_ASSERT(!contextState.currentSwapChain);
            Q_ASSERT(ofr.cbWrapper.recordingPass == QD3D11CommandBuffer::NoPass);
            executeCommandBuffer(&ofr.cbWrapper);
            ofr.cbWrapper.resetCommands();
        } else {
            Q_ASSERT(contextState.currentSwapChain);
            Q_ASSERT(contextState.currentSwapChain->cb.recordingPass == QD3D11CommandBuffer::NoPass);
            executeCommandBuffer(&contextState.currentSwapChain->cb); // no timestampSwapChain, in order to avoid timestamp mess
            contextState.currentSwapChain->cb.resetCommands();
        }
    }

    finishActiveReadbacks();

    return QRhi::FrameOpSuccess;
}

void QRhiD3D11::enqueueSubresUpload(QD3D11Texture *texD, QD3D11CommandBuffer *cbD,
                                    int layer, int level, const QRhiTextureSubresourceUploadDescription &subresDesc)
{
    const bool is3D = texD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
    UINT subres = D3D11CalcSubresource(UINT(level), is3D ? 0u : UINT(layer), texD->mipLevelCount);
    D3D11_BOX box;
    box.front = is3D ? UINT(layer) : 0u;
    // back, right, bottom are exclusive
    box.back = box.front + 1;
    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::UpdateSubRes;
    cmd.args.updateSubRes.dst = texD->textureResource();
    cmd.args.updateSubRes.dstSubRes = subres;

    const QPoint dp = subresDesc.destinationTopLeft();
    if (!subresDesc.image().isNull()) {
        QImage img = subresDesc.image();
        QSize size = img.size();
        int bpl = img.bytesPerLine();
        if (!subresDesc.sourceSize().isEmpty() || !subresDesc.sourceTopLeft().isNull()) {
            const QPoint sp = subresDesc.sourceTopLeft();
            if (!subresDesc.sourceSize().isEmpty())
                size = subresDesc.sourceSize();
            if (img.depth() == 32) {
                const int offset = sp.y() * img.bytesPerLine() + sp.x() * 4;
                cmd.args.updateSubRes.src = cbD->retainImage(img) + offset;
            } else {
                img = img.copy(sp.x(), sp.y(), size.width(), size.height());
                bpl = img.bytesPerLine();
                cmd.args.updateSubRes.src = cbD->retainImage(img);
            }
        } else {
            cmd.args.updateSubRes.src = cbD->retainImage(img);
        }
        box.left = UINT(dp.x());
        box.top = UINT(dp.y());
        box.right = UINT(dp.x() + size.width());
        box.bottom = UINT(dp.y() + size.height());
        cmd.args.updateSubRes.hasDstBox = true;
        cmd.args.updateSubRes.dstBox = box;
        cmd.args.updateSubRes.srcRowPitch = UINT(bpl);
    } else if (!subresDesc.data().isEmpty() && isCompressedFormat(texD->m_format)) {
        const QSize size = subresDesc.sourceSize().isEmpty() ? q->sizeForMipLevel(level, texD->m_pixelSize)
                                                             : subresDesc.sourceSize();
        quint32 bpl = 0;
        QSize blockDim;
        compressedFormatInfo(texD->m_format, size, &bpl, nullptr, &blockDim);
        // Everything must be a multiple of the block width and
        // height, so e.g. a mip level of size 2x2 will be 4x4 when it
        // comes to the actual data.
        box.left = UINT(aligned(dp.x(), blockDim.width()));
        box.top = UINT(aligned(dp.y(), blockDim.height()));
        box.right = UINT(aligned(dp.x() + size.width(), blockDim.width()));
        box.bottom = UINT(aligned(dp.y() + size.height(), blockDim.height()));
        cmd.args.updateSubRes.hasDstBox = true;
        cmd.args.updateSubRes.dstBox = box;
        cmd.args.updateSubRes.src = cbD->retainData(subresDesc.data());
        cmd.args.updateSubRes.srcRowPitch = bpl;
    } else if (!subresDesc.data().isEmpty()) {
        const QSize size = subresDesc.sourceSize().isEmpty() ? q->sizeForMipLevel(level, texD->m_pixelSize)
                                                             : subresDesc.sourceSize();
        quint32 bpl = 0;
        if (subresDesc.dataStride())
            bpl = subresDesc.dataStride();
        else
            textureFormatInfo(texD->m_format, size, &bpl, nullptr, nullptr);
        box.left = UINT(dp.x());
        box.top = UINT(dp.y());
        box.right = UINT(dp.x() + size.width());
        box.bottom = UINT(dp.y() + size.height());
        cmd.args.updateSubRes.hasDstBox = true;
        cmd.args.updateSubRes.dstBox = box;
        cmd.args.updateSubRes.src = cbD->retainData(subresDesc.data());
        cmd.args.updateSubRes.srcRowPitch = bpl;
    } else {
        qWarning("Invalid texture upload for %p layer=%d mip=%d", texD, layer, level);
        cbD->commands.unget();
    }
}

void QRhiD3D11::enqueueResourceUpdates(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    QRhiResourceUpdateBatchPrivate *ud = QRhiResourceUpdateBatchPrivate::get(resourceUpdates);

    for (int opIdx = 0; opIdx < ud->activeBufferOpCount; ++opIdx) {
        const QRhiResourceUpdateBatchPrivate::BufferOp &u(ud->bufferOps[opIdx]);
        if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::DynamicUpdate) {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, u.buf);
            Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
            memcpy(bufD->dynBuf + u.offset, u.data.constData(), size_t(u.data.size()));
            bufD->hasPendingDynamicUpdates = true;
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::StaticUpload) {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, u.buf);
            Q_ASSERT(bufD->m_type != QRhiBuffer::Dynamic);
            Q_ASSERT(u.offset + u.data.size() <= bufD->m_size);
            QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QD3D11CommandBuffer::Command::UpdateSubRes;
            cmd.args.updateSubRes.dst = bufD->buffer;
            cmd.args.updateSubRes.dstSubRes = 0;
            cmd.args.updateSubRes.src = cbD->retainBufferData(u.data);
            cmd.args.updateSubRes.srcRowPitch = 0;
            // Specify the region (even when offset is 0 and all data is provided)
            // since the ID3D11Buffer's size is rounded up to be a multiple of 256
            // while the data we have has the original size.
            D3D11_BOX box;
            box.left = u.offset;
            box.top = box.front = 0;
            box.back = box.bottom = 1;
            box.right = u.offset + u.data.size(); // no -1: right, bottom, back are exclusive, see D3D11_BOX doc
            cmd.args.updateSubRes.hasDstBox = true;
            cmd.args.updateSubRes.dstBox = box;
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::Read) {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, u.buf);
            if (bufD->m_type == QRhiBuffer::Dynamic) {
                u.result->data.resize(u.readSize);
                memcpy(u.result->data.data(), bufD->dynBuf + u.offset, size_t(u.readSize));
                if (u.result->completed)
                    u.result->completed();
            } else {
                BufferReadback readback;
                readback.result = u.result;
                readback.byteSize = u.readSize;

                D3D11_BUFFER_DESC desc = {};
                desc.ByteWidth = readback.byteSize;
                desc.Usage = D3D11_USAGE_STAGING;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                HRESULT hr = dev->CreateBuffer(&desc, nullptr, &readback.stagingBuf);
                if (FAILED(hr)) {
                    qWarning("Failed to create buffer: %s",
                        qPrintable(QSystemError::windowsComString(hr)));
                    continue;
                }

                QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
                cmd.cmd = QD3D11CommandBuffer::Command::CopySubRes;
                cmd.args.copySubRes.dst = readback.stagingBuf;
                cmd.args.copySubRes.dstSubRes = 0;
                cmd.args.copySubRes.dstX = 0;
                cmd.args.copySubRes.dstY = 0;
                cmd.args.copySubRes.dstZ = 0;
                cmd.args.copySubRes.src = bufD->buffer;
                cmd.args.copySubRes.srcSubRes = 0;
                cmd.args.copySubRes.hasSrcBox = true;
                D3D11_BOX box;
                box.left = u.offset;
                box.top = box.front = 0;
                box.back = box.bottom = 1;
                box.right = u.offset + u.readSize;
                cmd.args.copySubRes.srcBox = box;

                activeBufferReadbacks.append(readback);
            }
        }
    }
    for (int opIdx = 0; opIdx < ud->activeTextureOpCount; ++opIdx) {
        const QRhiResourceUpdateBatchPrivate::TextureOp &u(ud->textureOps[opIdx]);
        if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Upload) {
            QD3D11Texture *texD = QRHI_RES(QD3D11Texture, u.dst);
            for (int layer = 0, maxLayer = u.subresDesc.count(); layer < maxLayer; ++layer) {
                for (int level = 0; level < QRhi::MAX_MIP_LEVELS; ++level) {
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : std::as_const(u.subresDesc[layer][level]))
                        enqueueSubresUpload(texD, cbD, layer, level, subresDesc);
                }
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Copy) {
            Q_ASSERT(u.src && u.dst);
            QD3D11Texture *srcD = QRHI_RES(QD3D11Texture, u.src);
            QD3D11Texture *dstD = QRHI_RES(QD3D11Texture, u.dst);
            const bool srcIs3D = srcD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
            const bool dstIs3D = dstD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
            UINT srcSubRes = D3D11CalcSubresource(UINT(u.desc.sourceLevel()), srcIs3D ? 0u : UINT(u.desc.sourceLayer()), srcD->mipLevelCount);
            UINT dstSubRes = D3D11CalcSubresource(UINT(u.desc.destinationLevel()), dstIs3D ? 0u : UINT(u.desc.destinationLayer()), dstD->mipLevelCount);
            const QPoint dp = u.desc.destinationTopLeft();
            const QSize mipSize = q->sizeForMipLevel(u.desc.sourceLevel(), srcD->m_pixelSize);
            const QSize copySize = u.desc.pixelSize().isEmpty() ? mipSize : u.desc.pixelSize();
            const QPoint sp = u.desc.sourceTopLeft();
            D3D11_BOX srcBox;
            srcBox.left = UINT(sp.x());
            srcBox.top = UINT(sp.y());
            srcBox.front = srcIs3D ? UINT(u.desc.sourceLayer()) : 0u;
            // back, right, bottom are exclusive
            srcBox.right = srcBox.left + UINT(copySize.width());
            srcBox.bottom = srcBox.top + UINT(copySize.height());
            srcBox.back = srcBox.front + 1;
            QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QD3D11CommandBuffer::Command::CopySubRes;
            cmd.args.copySubRes.dst = dstD->textureResource();
            cmd.args.copySubRes.dstSubRes = dstSubRes;
            cmd.args.copySubRes.dstX = UINT(dp.x());
            cmd.args.copySubRes.dstY = UINT(dp.y());
            cmd.args.copySubRes.dstZ = dstIs3D ? UINT(u.desc.destinationLayer()) : 0u;
            cmd.args.copySubRes.src = srcD->textureResource();
            cmd.args.copySubRes.srcSubRes = srcSubRes;
            cmd.args.copySubRes.hasSrcBox = true;
            cmd.args.copySubRes.srcBox = srcBox;
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Read) {
            TextureReadback readback;
            readback.desc = u.rb;
            readback.result = u.result;

            ID3D11Resource *src;
            DXGI_FORMAT dxgiFormat;
            QSize pixelSize;
            QRhiTexture::Format format;
            UINT subres = 0;
            QD3D11Texture *texD = QRHI_RES(QD3D11Texture, u.rb.texture());
            QD3D11SwapChain *swapChainD = nullptr;
            bool is3D = false;

            if (texD) {
                if (texD->sampleDesc.Count > 1) {
                    qWarning("Multisample texture cannot be read back");
                    continue;
                }
                src = texD->textureResource();
                dxgiFormat = texD->dxgiFormat;
                pixelSize = q->sizeForMipLevel(u.rb.level(), texD->m_pixelSize);
                format = texD->m_format;
                is3D = texD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
                subres = D3D11CalcSubresource(UINT(u.rb.level()), UINT(is3D ? 0 : u.rb.layer()), texD->mipLevelCount);
            } else {
                Q_ASSERT(contextState.currentSwapChain);
                swapChainD = QRHI_RES(QD3D11SwapChain, contextState.currentSwapChain);
                if (swapChainD->sampleDesc.Count > 1) {
                    // Unlike with textures, reading back a multisample swapchain image
                    // has to be supported. Insert a resolve.
                    QD3D11CommandBuffer::Command &rcmd(cbD->commands.get());
                    rcmd.cmd = QD3D11CommandBuffer::Command::ResolveSubRes;
                    rcmd.args.resolveSubRes.dst = swapChainD->backBufferTex;
                    rcmd.args.resolveSubRes.dstSubRes = 0;
                    rcmd.args.resolveSubRes.src = swapChainD->msaaTex[swapChainD->currentFrameSlot];
                    rcmd.args.resolveSubRes.srcSubRes = 0;
                    rcmd.args.resolveSubRes.format = swapChainD->colorFormat;
                }
                src = swapChainD->backBufferTex;
                dxgiFormat = swapChainD->colorFormat;
                pixelSize = swapChainD->pixelSize;
                format = swapchainReadbackTextureFormat(dxgiFormat, nullptr);
                if (format == QRhiTexture::UnknownFormat)
                    continue;
            }
            quint32 byteSize = 0;
            quint32 bpl = 0;
            textureFormatInfo(format, pixelSize, &bpl, &byteSize, nullptr);

            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = UINT(pixelSize.width());
            desc.Height = UINT(pixelSize.height());
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = dxgiFormat;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            ID3D11Texture2D *stagingTex;
            HRESULT hr = dev->CreateTexture2D(&desc, nullptr, &stagingTex);
            if (FAILED(hr)) {
                qWarning("Failed to create readback staging texture: %s",
                    qPrintable(QSystemError::windowsComString(hr)));
                return;
            }

            QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QD3D11CommandBuffer::Command::CopySubRes;
            cmd.args.copySubRes.dst = stagingTex;
            cmd.args.copySubRes.dstSubRes = 0;
            cmd.args.copySubRes.dstX = 0;
            cmd.args.copySubRes.dstY = 0;
            cmd.args.copySubRes.dstZ = 0;
            cmd.args.copySubRes.src = src;
            cmd.args.copySubRes.srcSubRes = subres;
            if (is3D) {
                D3D11_BOX srcBox = {};
                srcBox.front = UINT(u.rb.layer());
                srcBox.right = desc.Width; // exclusive
                srcBox.bottom = desc.Height;
                srcBox.back = srcBox.front + 1;
                cmd.args.copySubRes.hasSrcBox = true;
                cmd.args.copySubRes.srcBox = srcBox;
            } else {
                cmd.args.copySubRes.hasSrcBox = false;
            }

            readback.stagingTex = stagingTex;
            readback.byteSize = byteSize;
            readback.bpl = bpl;
            readback.pixelSize = pixelSize;
            readback.format = format;

            activeTextureReadbacks.append(readback);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::GenMips) {
            Q_ASSERT(u.dst->flags().testFlag(QRhiTexture::UsedWithGenerateMips));
            QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QD3D11CommandBuffer::Command::GenMip;
            cmd.args.genMip.srv = QRHI_RES(QD3D11Texture, u.dst)->srv;
        }
    }

    ud->free();
}

void QRhiD3D11::finishActiveReadbacks()
{
    QVarLengthArray<std::function<void()>, 4> completedCallbacks;

    for (int i = activeTextureReadbacks.count() - 1; i >= 0; --i) {
        const QRhiD3D11::TextureReadback &readback(activeTextureReadbacks[i]);
        readback.result->format = readback.format;
        readback.result->pixelSize = readback.pixelSize;

        D3D11_MAPPED_SUBRESOURCE mp;
        HRESULT hr = context->Map(readback.stagingTex, 0, D3D11_MAP_READ, 0, &mp);
        if (SUCCEEDED(hr)) {
            readback.result->data.resize(int(readback.byteSize));
            // nothing says the rows are tightly packed in the texture, must take
            // the stride into account
            char *dst = readback.result->data.data();
            char *src = static_cast<char *>(mp.pData);
            for (int y = 0, h = readback.pixelSize.height(); y != h; ++y) {
                memcpy(dst, src, readback.bpl);
                dst += readback.bpl;
                src += mp.RowPitch;
            }
            context->Unmap(readback.stagingTex, 0);
        } else {
            qWarning("Failed to map readback staging texture: %s",
                qPrintable(QSystemError::windowsComString(hr)));
        }

        readback.stagingTex->Release();

        if (readback.result->completed)
            completedCallbacks.append(readback.result->completed);

        activeTextureReadbacks.removeLast();
    }

    for (int i = activeBufferReadbacks.count() - 1; i >= 0; --i) {
        const QRhiD3D11::BufferReadback &readback(activeBufferReadbacks[i]);

        D3D11_MAPPED_SUBRESOURCE mp;
        HRESULT hr = context->Map(readback.stagingBuf, 0, D3D11_MAP_READ, 0, &mp);
        if (SUCCEEDED(hr)) {
            readback.result->data.resize(int(readback.byteSize));
            memcpy(readback.result->data.data(), mp.pData, readback.byteSize);
            context->Unmap(readback.stagingBuf, 0);
        } else {
            qWarning("Failed to map readback staging texture: %s",
                qPrintable(QSystemError::windowsComString(hr)));
        }

        readback.stagingBuf->Release();

        if (readback.result->completed)
            completedCallbacks.append(readback.result->completed);

        activeBufferReadbacks.removeLast();
    }

    for (auto f : completedCallbacks)
        f();
}

static inline QD3D11RenderTargetData *rtData(QRhiRenderTarget *rt)
{
    switch (rt->resourceType()) {
    case QRhiResource::SwapChainRenderTarget:
        return &QRHI_RES(QD3D11SwapChainRenderTarget, rt)->d;
    case QRhiResource::TextureRenderTarget:
        return &QRHI_RES(QD3D11TextureRenderTarget, rt)->d;
    default:
        Q_UNREACHABLE();
        return nullptr;
    }
}

void QRhiD3D11::resourceUpdate(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_ASSERT(QRHI_RES(QD3D11CommandBuffer, cb)->recordingPass == QD3D11CommandBuffer::NoPass);

    enqueueResourceUpdates(cb, resourceUpdates);
}

void QRhiD3D11::beginPass(QRhiCommandBuffer *cb,
                          QRhiRenderTarget *rt,
                          const QColor &colorClearValue,
                          const QRhiDepthStencilClearValue &depthStencilClearValue,
                          QRhiResourceUpdateBatch *resourceUpdates,
                          QRhiCommandBuffer::BeginPassFlags)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);

    bool wantsColorClear = true;
    bool wantsDsClear = true;
    QD3D11RenderTargetData *rtD = rtData(rt);
    if (rt->resourceType() == QRhiRenderTarget::TextureRenderTarget) {
        QD3D11TextureRenderTarget *rtTex = QRHI_RES(QD3D11TextureRenderTarget, rt);
        wantsColorClear = !rtTex->m_flags.testFlag(QRhiTextureRenderTarget::PreserveColorContents);
        wantsDsClear = !rtTex->m_flags.testFlag(QRhiTextureRenderTarget::PreserveDepthStencilContents);
        if (!QRhiRenderTargetAttachmentTracker::isUpToDate<QD3D11Texture, QD3D11RenderBuffer>(rtTex->description(), rtD->currentResIdList))
            rtTex->create();
    }

    cbD->commands.get().cmd = QD3D11CommandBuffer::Command::ResetShaderResources;

    QD3D11CommandBuffer::Command &fbCmd(cbD->commands.get());
    fbCmd.cmd = QD3D11CommandBuffer::Command::SetRenderTarget;
    fbCmd.args.setRenderTarget.rt = rt;

    QD3D11CommandBuffer::Command &clearCmd(cbD->commands.get());
    clearCmd.cmd = QD3D11CommandBuffer::Command::Clear;
    clearCmd.args.clear.rt = rt;
    clearCmd.args.clear.mask = 0;
    if (rtD->colorAttCount && wantsColorClear)
        clearCmd.args.clear.mask |= QD3D11CommandBuffer::Command::Color;
    if (rtD->dsAttCount && wantsDsClear)
        clearCmd.args.clear.mask |= QD3D11CommandBuffer::Command::Depth | QD3D11CommandBuffer::Command::Stencil;

    clearCmd.args.clear.c[0] = float(colorClearValue.redF());
    clearCmd.args.clear.c[1] = float(colorClearValue.greenF());
    clearCmd.args.clear.c[2] = float(colorClearValue.blueF());
    clearCmd.args.clear.c[3] = float(colorClearValue.alphaF());
    clearCmd.args.clear.d = depthStencilClearValue.depthClearValue();
    clearCmd.args.clear.s = depthStencilClearValue.stencilClearValue();

    cbD->recordingPass = QD3D11CommandBuffer::RenderPass;
    cbD->currentTarget = rt;

    cbD->resetCachedState();
}

void QRhiD3D11::endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);

    if (cbD->currentTarget->resourceType() == QRhiResource::TextureRenderTarget) {
        QD3D11TextureRenderTarget *rtTex = QRHI_RES(QD3D11TextureRenderTarget, cbD->currentTarget);
        for (auto it = rtTex->m_desc.cbeginColorAttachments(), itEnd = rtTex->m_desc.cendColorAttachments();
             it != itEnd; ++it)
        {
            const QRhiColorAttachment &colorAtt(*it);
            if (!colorAtt.resolveTexture())
                continue;

            QD3D11Texture *dstTexD = QRHI_RES(QD3D11Texture, colorAtt.resolveTexture());
            QD3D11Texture *srcTexD = QRHI_RES(QD3D11Texture, colorAtt.texture());
            QD3D11RenderBuffer *srcRbD = QRHI_RES(QD3D11RenderBuffer, colorAtt.renderBuffer());
            Q_ASSERT(srcTexD || srcRbD);
            QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QD3D11CommandBuffer::Command::ResolveSubRes;
            cmd.args.resolveSubRes.dst = dstTexD->textureResource();
            cmd.args.resolveSubRes.dstSubRes = D3D11CalcSubresource(UINT(colorAtt.resolveLevel()),
                                                                    UINT(colorAtt.resolveLayer()),
                                                                    dstTexD->mipLevelCount);
            if (srcTexD) {
                cmd.args.resolveSubRes.src = srcTexD->textureResource();
                if (srcTexD->dxgiFormat != dstTexD->dxgiFormat) {
                    qWarning("Resolve source (%d) and destination (%d) formats do not match",
                             int(srcTexD->dxgiFormat), int(dstTexD->dxgiFormat));
                    cbD->commands.unget();
                    continue;
                }
                if (srcTexD->sampleDesc.Count <= 1) {
                    qWarning("Cannot resolve a non-multisample texture");
                    cbD->commands.unget();
                    continue;
                }
                if (srcTexD->m_pixelSize != dstTexD->m_pixelSize) {
                    qWarning("Resolve source and destination sizes do not match");
                    cbD->commands.unget();
                    continue;
                }
            } else {
                cmd.args.resolveSubRes.src = srcRbD->tex;
                if (srcRbD->dxgiFormat != dstTexD->dxgiFormat) {
                    qWarning("Resolve source (%d) and destination (%d) formats do not match",
                             int(srcRbD->dxgiFormat), int(dstTexD->dxgiFormat));
                    cbD->commands.unget();
                    continue;
                }
                if (srcRbD->m_pixelSize != dstTexD->m_pixelSize) {
                    qWarning("Resolve source and destination sizes do not match");
                    cbD->commands.unget();
                    continue;
                }
            }
            cmd.args.resolveSubRes.srcSubRes = D3D11CalcSubresource(0, UINT(colorAtt.layer()), 1);
            cmd.args.resolveSubRes.format = dstTexD->dxgiFormat;
        }
    }

    cbD->recordingPass = QD3D11CommandBuffer::NoPass;
    cbD->currentTarget = nullptr;

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);
}

void QRhiD3D11::beginComputePass(QRhiCommandBuffer *cb,
                                 QRhiResourceUpdateBatch *resourceUpdates,
                                 QRhiCommandBuffer::BeginPassFlags)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);

    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::ResetShaderResources;

    cbD->recordingPass = QD3D11CommandBuffer::ComputePass;

    cbD->resetCachedState();
}

void QRhiD3D11::endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::ComputePass);

    cbD->recordingPass = QD3D11CommandBuffer::NoPass;

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);
}

void QRhiD3D11::setComputePipeline(QRhiCommandBuffer *cb, QRhiComputePipeline *ps)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::ComputePass);
    QD3D11ComputePipeline *psD = QRHI_RES(QD3D11ComputePipeline, ps);
    const bool pipelineChanged = cbD->currentComputePipeline != ps || cbD->currentPipelineGeneration != psD->generation;

    if (pipelineChanged) {
        cbD->currentGraphicsPipeline = nullptr;
        cbD->currentComputePipeline = psD;
        cbD->currentPipelineGeneration = psD->generation;

        QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QD3D11CommandBuffer::Command::BindComputePipeline;
        cmd.args.bindComputePipeline.ps = psD;
    }
}

void QRhiD3D11::dispatch(QRhiCommandBuffer *cb, int x, int y, int z)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::ComputePass);

    QD3D11CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QD3D11CommandBuffer::Command::Dispatch;
    cmd.args.dispatch.x = UINT(x);
    cmd.args.dispatch.y = UINT(y);
    cmd.args.dispatch.z = UINT(z);
}

static inline QPair<int, int> mapBinding(int binding,
                                         int stageIndex,
                                         const QShader::NativeResourceBindingMap *nativeResourceBindingMaps[])
{
    const QShader::NativeResourceBindingMap *map = nativeResourceBindingMaps[stageIndex];
    if (!map || map->isEmpty())
        return { binding, binding }; // assume 1:1 mapping

    auto it = map->constFind(binding);
    if (it != map->cend())
        return *it;

    // Hitting this path is normal too. It is not given that the resource is
    // present in the shaders for all the stages specified by the visibility
    // mask in the QRhiShaderResourceBinding.
    return { -1, -1 };
}

void QRhiD3D11::updateShaderResourceBindings(QD3D11ShaderResourceBindings *srbD,
                                             const QShader::NativeResourceBindingMap *nativeResourceBindingMaps[])
{
    srbD->vsUniformBufferBatches.clear();
    srbD->hsUniformBufferBatches.clear();
    srbD->dsUniformBufferBatches.clear();
    srbD->gsUniformBufferBatches.clear();
    srbD->fsUniformBufferBatches.clear();
    srbD->csUniformBufferBatches.clear();

    srbD->vsSamplerBatches.clear();
    srbD->hsSamplerBatches.clear();
    srbD->dsSamplerBatches.clear();
    srbD->gsSamplerBatches.clear();
    srbD->fsSamplerBatches.clear();
    srbD->csSamplerBatches.clear();

    srbD->csUavBatches.clear();

    struct Stage {
        struct Buffer {
            int binding; // stored and sent along in XXorigbindings just for applyDynamicOffsets()
            int breg; // b0, b1, ...
            ID3D11Buffer *buffer;
            uint offsetInConstants;
            uint sizeInConstants;
        };
        struct Texture {
            int treg; // t0, t1, ...
            ID3D11ShaderResourceView *srv;
        };
        struct Sampler {
            int sreg; // s0, s1, ...
            ID3D11SamplerState *sampler;
        };
        struct Uav {
            int ureg;
            ID3D11UnorderedAccessView *uav;
        };
        QVarLengthArray<Buffer, 8> buffers;
        QVarLengthArray<Texture, 8> textures;
        QVarLengthArray<Sampler, 8> samplers;
        QVarLengthArray<Uav, 8> uavs;
        void buildBufferBatches(QD3D11ShaderResourceBindings::StageUniformBufferBatches &batches) const
        {
            for (const Buffer &buf : buffers) {
                batches.ubufs.feed(buf.breg, buf.buffer);
                batches.ubuforigbindings.feed(buf.breg, UINT(buf.binding));
                batches.ubufoffsets.feed(buf.breg, buf.offsetInConstants);
                batches.ubufsizes.feed(buf.breg, buf.sizeInConstants);
            }
            batches.finish();
        }
        void buildSamplerBatches(QD3D11ShaderResourceBindings::StageSamplerBatches &batches) const
        {
            for (const Texture &t : textures)
                batches.shaderresources.feed(t.treg, t.srv);
            for (const Sampler &s : samplers)
                batches.samplers.feed(s.sreg, s.sampler);
            batches.finish();
        }
        void buildUavBatches(QD3D11ShaderResourceBindings::StageUavBatches &batches) const
        {
            for (const Stage::Uav &u : uavs)
                batches.uavs.feed(u.ureg, u.uav);
            batches.finish();
        }
    } res[RBM_SUPPORTED_STAGES];

    for (int i = 0, ie = srbD->sortedBindings.count(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = shaderResourceBindingData(srbD->sortedBindings.at(i));
        QD3D11ShaderResourceBindings::BoundResourceData &bd(srbD->boundResourceData[i]);
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, b->u.ubuf.buf);
            Q_ASSERT(aligned(b->u.ubuf.offset, 256u) == b->u.ubuf.offset);
            bd.ubuf.id = bufD->m_id;
            bd.ubuf.generation = bufD->generation;
            // Dynamic ubuf offsets are not considered here, those are baked in
            // at a later stage, which is good as vsubufoffsets and friends are
            // per-srb, not per-setShaderResources call. Other backends (GL,
            // Metal) are different in this respect since those do not store
            // per-srb vsubufoffsets etc. data so life's a bit easier for them.
            // But here we have to defer baking in the dynamic offset.
            const quint32 offsetInConstants = b->u.ubuf.offset / 16;
            // size must be 16 mult. (in constants, i.e. multiple of 256 bytes).
            // We can round up if needed since the buffers's actual size
            // (ByteWidth) is always a multiple of 256.
            const quint32 sizeInConstants = aligned(b->u.ubuf.maybeSize ? b->u.ubuf.maybeSize : bufD->m_size, 256u) / 16;
            if (b->stage.testFlag(QRhiShaderResourceBinding::VertexStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_VERTEX, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0)
                    res[RBM_VERTEX].buffers.append({ b->binding, nativeBinding.first, bufD->buffer, offsetInConstants, sizeInConstants });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::TessellationControlStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_HULL, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0)
                    res[RBM_HULL].buffers.append({ b->binding, nativeBinding.first, bufD->buffer, offsetInConstants, sizeInConstants });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::TessellationEvaluationStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_DOMAIN, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0)
                    res[RBM_DOMAIN].buffers.append({ b->binding, nativeBinding.first, bufD->buffer, offsetInConstants, sizeInConstants });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::GeometryStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_GEOMETRY, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0)
                    res[RBM_GEOMETRY].buffers.append({ b->binding, nativeBinding.first, bufD->buffer, offsetInConstants, sizeInConstants });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::FragmentStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_FRAGMENT, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0)
                    res[RBM_FRAGMENT].buffers.append({ b->binding, nativeBinding.first, bufD->buffer, offsetInConstants, sizeInConstants });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::ComputeStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_COMPUTE, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0)
                    res[RBM_COMPUTE].buffers.append({ b->binding, nativeBinding.first, bufD->buffer, offsetInConstants, sizeInConstants });
            }
        }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
        case QRhiShaderResourceBinding::Texture:
        case QRhiShaderResourceBinding::Sampler:
        {
            const QRhiShaderResourceBinding::Data::TextureAndOrSamplerData *data = &b->u.stex;
            bd.stex.count = data->count;
            const QPair<int, int> nativeBindingVert = mapBinding(b->binding, RBM_VERTEX, nativeResourceBindingMaps);
            const QPair<int, int> nativeBindingHull = mapBinding(b->binding, RBM_HULL, nativeResourceBindingMaps);
            const QPair<int, int> nativeBindingDomain = mapBinding(b->binding, RBM_DOMAIN, nativeResourceBindingMaps);
            const QPair<int, int> nativeBindingGeom = mapBinding(b->binding, RBM_GEOMETRY, nativeResourceBindingMaps);
            const QPair<int, int> nativeBindingFrag = mapBinding(b->binding, RBM_FRAGMENT, nativeResourceBindingMaps);
            const QPair<int, int> nativeBindingComp = mapBinding(b->binding, RBM_COMPUTE, nativeResourceBindingMaps);
            // if SPIR-V binding b is mapped to tN and sN in HLSL, and it
            // is an array, then it will use tN, tN+1, tN+2, ..., and sN,
            // sN+1, sN+2, ...
            for (int elem = 0; elem < data->count; ++elem) {
                QD3D11Texture *texD = QRHI_RES(QD3D11Texture, data->texSamplers[elem].tex);
                QD3D11Sampler *samplerD = QRHI_RES(QD3D11Sampler, data->texSamplers[elem].sampler);
                bd.stex.d[elem].texId = texD ? texD->m_id : 0;
                bd.stex.d[elem].texGeneration = texD ? texD->generation : 0;
                bd.stex.d[elem].samplerId = samplerD ? samplerD->m_id : 0;
                bd.stex.d[elem].samplerGeneration = samplerD ? samplerD->generation : 0;
                // Must handle all three cases (combined, separate, separate):
                //   first = texture binding, second = sampler binding
                //   first = texture binding
                //   first = sampler binding
                if (b->stage.testFlag(QRhiShaderResourceBinding::VertexStage)) {
                    const int samplerBinding = texD && samplerD ? nativeBindingVert.second
                                                                : (samplerD ? nativeBindingVert.first : -1);
                    if (nativeBindingVert.first >= 0 && texD)
                        res[RBM_VERTEX].textures.append({ nativeBindingVert.first + elem, texD->srv });
                    if (samplerBinding >= 0)
                        res[RBM_VERTEX].samplers.append({ samplerBinding + elem, samplerD->samplerState });
                }
                if (b->stage.testFlag(QRhiShaderResourceBinding::TessellationControlStage)) {
                    const int samplerBinding = texD && samplerD ? nativeBindingHull.second
                                                                : (samplerD ? nativeBindingHull.first : -1);
                    if (nativeBindingHull.first >= 0 && texD)
                        res[RBM_HULL].textures.append({ nativeBindingHull.first + elem, texD->srv });
                    if (samplerBinding >= 0)
                        res[RBM_HULL].samplers.append({ samplerBinding + elem, samplerD->samplerState });
                }
                if (b->stage.testFlag(QRhiShaderResourceBinding::TessellationEvaluationStage)) {
                    const int samplerBinding = texD && samplerD ? nativeBindingDomain.second
                                                                : (samplerD ? nativeBindingDomain.first : -1);
                    if (nativeBindingDomain.first >= 0 && texD)
                        res[RBM_DOMAIN].textures.append({ nativeBindingDomain.first + elem, texD->srv });
                    if (samplerBinding >= 0)
                        res[RBM_DOMAIN].samplers.append({ samplerBinding + elem, samplerD->samplerState });
                }
                if (b->stage.testFlag(QRhiShaderResourceBinding::GeometryStage)) {
                    const int samplerBinding = texD && samplerD ? nativeBindingGeom.second
                                                                : (samplerD ? nativeBindingGeom.first : -1);
                    if (nativeBindingGeom.first >= 0 && texD)
                        res[RBM_GEOMETRY].textures.append({ nativeBindingGeom.first + elem, texD->srv });
                    if (samplerBinding >= 0)
                        res[RBM_GEOMETRY].samplers.append({ samplerBinding + elem, samplerD->samplerState });
                }
                if (b->stage.testFlag(QRhiShaderResourceBinding::FragmentStage)) {
                    const int samplerBinding = texD && samplerD ? nativeBindingFrag.second
                                                                : (samplerD ? nativeBindingFrag.first : -1);
                    if (nativeBindingFrag.first >= 0 && texD)
                        res[RBM_FRAGMENT].textures.append({ nativeBindingFrag.first + elem, texD->srv });
                    if (samplerBinding >= 0)
                        res[RBM_FRAGMENT].samplers.append({ samplerBinding + elem, samplerD->samplerState });
                }
                if (b->stage.testFlag(QRhiShaderResourceBinding::ComputeStage)) {
                    const int samplerBinding = texD && samplerD ? nativeBindingComp.second
                                                                : (samplerD ? nativeBindingComp.first : -1);
                    if (nativeBindingComp.first >= 0 && texD)
                        res[RBM_COMPUTE].textures.append({ nativeBindingComp.first + elem, texD->srv });
                    if (samplerBinding >= 0)
                        res[RBM_COMPUTE].samplers.append({ samplerBinding + elem, samplerD->samplerState });
                }
            }
        }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
        {
            QD3D11Texture *texD = QRHI_RES(QD3D11Texture, b->u.simage.tex);
            bd.simage.id = texD->m_id;
            bd.simage.generation = texD->generation;
            if (b->stage.testFlag(QRhiShaderResourceBinding::ComputeStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_COMPUTE, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0) {
                    ID3D11UnorderedAccessView *uav = texD->unorderedAccessViewForLevel(b->u.simage.level);
                    if (uav)
                        res[RBM_COMPUTE].uavs.append({ nativeBinding.first, uav });
                }
            } else {
                qWarning("Unordered access only supported at compute stage");
            }
        }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
        case QRhiShaderResourceBinding::BufferStore:
        case QRhiShaderResourceBinding::BufferLoadStore:
        {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, b->u.sbuf.buf);
            bd.sbuf.id = bufD->m_id;
            bd.sbuf.generation = bufD->generation;
            if (b->stage.testFlag(QRhiShaderResourceBinding::ComputeStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_COMPUTE, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0) {
                    ID3D11UnorderedAccessView *uav = bufD->unorderedAccessView(b->u.sbuf.offset);
                    if (uav)
                        res[RBM_COMPUTE].uavs.append({ nativeBinding.first, uav });
                }
            } else {
                qWarning("Unordered access only supported at compute stage");
            }
        }
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    // QRhiBatchedBindings works with the native bindings and expects
    // sorted input. The pre-sorted QRhiShaderResourceBinding list (based
    // on the QRhi (SPIR-V) binding) is not helpful in this regard, so we
    // have to sort here every time.
    for (int stage = 0; stage < RBM_SUPPORTED_STAGES; ++stage) {
        std::sort(res[stage].buffers.begin(), res[stage].buffers.end(), [](const Stage::Buffer &a, const Stage::Buffer &b) {
            return a.breg < b.breg;
        });
        std::sort(res[stage].textures.begin(), res[stage].textures.end(), [](const Stage::Texture &a, const Stage::Texture &b) {
            return a.treg < b.treg;
        });
        std::sort(res[stage].samplers.begin(), res[stage].samplers.end(), [](const Stage::Sampler &a, const Stage::Sampler &b) {
            return a.sreg < b.sreg;
        });
        std::sort(res[stage].uavs.begin(), res[stage].uavs.end(), [](const Stage::Uav &a, const Stage::Uav &b) {
            return a.ureg < b.ureg;
        });
    }

    res[RBM_VERTEX].buildBufferBatches(srbD->vsUniformBufferBatches);
    res[RBM_HULL].buildBufferBatches(srbD->hsUniformBufferBatches);
    res[RBM_DOMAIN].buildBufferBatches(srbD->dsUniformBufferBatches);
    res[RBM_GEOMETRY].buildBufferBatches(srbD->gsUniformBufferBatches);
    res[RBM_FRAGMENT].buildBufferBatches(srbD->fsUniformBufferBatches);
    res[RBM_COMPUTE].buildBufferBatches(srbD->csUniformBufferBatches);

    res[RBM_VERTEX].buildSamplerBatches(srbD->vsSamplerBatches);
    res[RBM_HULL].buildSamplerBatches(srbD->hsSamplerBatches);
    res[RBM_DOMAIN].buildSamplerBatches(srbD->dsSamplerBatches);
    res[RBM_GEOMETRY].buildSamplerBatches(srbD->gsSamplerBatches);
    res[RBM_FRAGMENT].buildSamplerBatches(srbD->fsSamplerBatches);
    res[RBM_COMPUTE].buildSamplerBatches(srbD->csSamplerBatches);

    res[RBM_COMPUTE].buildUavBatches(srbD->csUavBatches);
}

void QRhiD3D11::executeBufferHostWrites(QD3D11Buffer *bufD)
{
    if (!bufD->hasPendingDynamicUpdates || bufD->m_size < 1)
        return;

    Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
    bufD->hasPendingDynamicUpdates = false;
    D3D11_MAPPED_SUBRESOURCE mp;
    HRESULT hr = context->Map(bufD->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mp);
    if (SUCCEEDED(hr)) {
        memcpy(mp.pData, bufD->dynBuf, bufD->m_size);
        context->Unmap(bufD->buffer, 0);
    } else {
        qWarning("Failed to map buffer: %s",
            qPrintable(QSystemError::windowsComString(hr)));
    }
}

static void applyDynamicOffsets(UINT *offsets,
                                int batchIndex,
                                const QRhiBatchedBindings<UINT> *originalBindings,
                                const QRhiBatchedBindings<UINT> *staticOffsets,
                                const uint *dynOfsPairs, int dynOfsPairCount)
{
    const int count = staticOffsets->batches[batchIndex].resources.count();
    // Make a copy of the offset list, the entries that have no corresponding
    // dynamic offset will continue to use the existing offset value.
    for (int b = 0; b < count; ++b) {
        offsets[b] = staticOffsets->batches[batchIndex].resources[b];
        for (int di = 0; di < dynOfsPairCount; ++di) {
            const uint binding = dynOfsPairs[2 * di];
            // binding is the SPIR-V style binding point here, nothing to do
            // with the native one.
            if (binding == originalBindings->batches[batchIndex].resources[b]) {
                const uint offsetInConstants = dynOfsPairs[2 * di + 1];
                offsets[b] = offsetInConstants;
                break;
            }
        }
    }
}

static inline uint clampedResourceCount(uint startSlot, int countSlots, uint maxSlots, const char *resType)
{
    if (startSlot + countSlots > maxSlots) {
        qWarning("Not enough D3D11 %s slots to bind %d resources starting at slot %d, max slots is %d",
                 resType, countSlots, startSlot, maxSlots);
        countSlots = maxSlots > startSlot ? maxSlots - startSlot : 0;
    }
    return countSlots;
}

#define SETUBUFBATCH(stagePrefixL, stagePrefixU) \
    if (srbD->stagePrefixL##UniformBufferBatches.present) { \
        const QD3D11ShaderResourceBindings::StageUniformBufferBatches &batches(srbD->stagePrefixL##UniformBufferBatches); \
        for (int i = 0, ie = batches.ubufs.batches.count(); i != ie; ++i) { \
            const uint count = clampedResourceCount(batches.ubufs.batches[i].startBinding, \
                                                    batches.ubufs.batches[i].resources.count(), \
                                                    D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, \
                                                    #stagePrefixU " cbuf"); \
            if (count) { \
                if (!dynOfsPairCount) { \
                    context->stagePrefixU##SetConstantBuffers1(batches.ubufs.batches[i].startBinding, \
                                                   count, \
                                                   batches.ubufs.batches[i].resources.constData(), \
                                                   batches.ubufoffsets.batches[i].resources.constData(), \
                                                   batches.ubufsizes.batches[i].resources.constData()); \
                } else { \
                    applyDynamicOffsets(offsets, i, \
                                        &batches.ubuforigbindings, &batches.ubufoffsets, \
                                        dynOfsPairs, dynOfsPairCount); \
                    context->stagePrefixU##SetConstantBuffers1(batches.ubufs.batches[i].startBinding, \
                                                   count, \
                                                   batches.ubufs.batches[i].resources.constData(), \
                                                   offsets, \
                                                   batches.ubufsizes.batches[i].resources.constData()); \
                } \
            } \
        } \
    }

#define SETSAMPLERBATCH(stagePrefixL, stagePrefixU) \
    if (srbD->stagePrefixL##SamplerBatches.present) { \
        for (const auto &batch : srbD->stagePrefixL##SamplerBatches.samplers.batches) { \
            const uint count = clampedResourceCount(batch.startBinding, batch.resources.count(), \
                                                    D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, #stagePrefixU " sampler"); \
            if (count) \
                context->stagePrefixU##SetSamplers(batch.startBinding, count, batch.resources.constData()); \
        } \
        for (const auto &batch : srbD->stagePrefixL##SamplerBatches.shaderresources.batches) { \
            const uint count = clampedResourceCount(batch.startBinding, batch.resources.count(), \
                                                    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, #stagePrefixU " SRV"); \
            if (count) { \
                context->stagePrefixU##SetShaderResources(batch.startBinding, count, batch.resources.constData()); \
                contextState.stagePrefixL##HighestActiveSrvBinding = qMax(contextState.stagePrefixL##HighestActiveSrvBinding, \
                                                              int(batch.startBinding + count) - 1); \
            } \
        } \
    }

#define SETUAVBATCH(stagePrefixL, stagePrefixU) \
    if (srbD->stagePrefixL##UavBatches.present) { \
        for (const auto &batch : srbD->stagePrefixL##UavBatches.uavs.batches) { \
            const uint count = clampedResourceCount(batch.startBinding, batch.resources.count(), \
                                                    D3D11_1_UAV_SLOT_COUNT, #stagePrefixU " UAV"); \
            if (count) { \
                context->stagePrefixU##SetUnorderedAccessViews(batch.startBinding, \
                                                   count, \
                                                   batch.resources.constData(), \
                                                   nullptr); \
                contextState.stagePrefixL##HighestActiveUavBinding = qMax(contextState.stagePrefixL##HighestActiveUavBinding, \
                                                              int(batch.startBinding + count) - 1); \
            } \
        } \
    }

void QRhiD3D11::bindShaderResources(QD3D11ShaderResourceBindings *srbD,
                                    const uint *dynOfsPairs, int dynOfsPairCount,
                                    bool offsetOnlyChange)
{
    UINT offsets[QD3D11CommandBuffer::MAX_DYNAMIC_OFFSET_COUNT];

    SETUBUFBATCH(vs, VS)
    SETUBUFBATCH(hs, HS)
    SETUBUFBATCH(ds, DS)
    SETUBUFBATCH(gs, GS)
    SETUBUFBATCH(fs, PS)
    SETUBUFBATCH(cs, CS)

    if (!offsetOnlyChange) {
        SETSAMPLERBATCH(vs, VS)
        SETSAMPLERBATCH(hs, HS)
        SETSAMPLERBATCH(ds, DS)
        SETSAMPLERBATCH(gs, GS)
        SETSAMPLERBATCH(fs, PS)
        SETSAMPLERBATCH(cs, CS)

        SETUAVBATCH(cs, CS)
    }
}

void QRhiD3D11::resetShaderResources()
{
    // Output cannot be bound on input etc.

    if (contextState.vsHasIndexBufferBound) {
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
        contextState.vsHasIndexBufferBound = false;
    }

    if (contextState.vsHighestActiveVertexBufferBinding >= 0) {
        const int count = contextState.vsHighestActiveVertexBufferBinding + 1;
        QVarLengthArray<ID3D11Buffer *, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> nullbufs(count);
        for (int i = 0; i < count; ++i)
            nullbufs[i] = nullptr;
        QVarLengthArray<UINT, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> nullstrides(count);
        for (int i = 0; i < count; ++i)
            nullstrides[i] = 0;
        QVarLengthArray<UINT, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> nulloffsets(count);
        for (int i = 0; i < count; ++i)
            nulloffsets[i] = 0;
        context->IASetVertexBuffers(0, UINT(count), nullbufs.constData(), nullstrides.constData(), nulloffsets.constData());
        contextState.vsHighestActiveVertexBufferBinding = -1;
    }

    int nullsrvCount = qMax(contextState.vsHighestActiveSrvBinding, contextState.fsHighestActiveSrvBinding);
    nullsrvCount = qMax(nullsrvCount, contextState.hsHighestActiveSrvBinding);
    nullsrvCount = qMax(nullsrvCount, contextState.dsHighestActiveSrvBinding);
    nullsrvCount = qMax(nullsrvCount, contextState.gsHighestActiveSrvBinding);
    nullsrvCount = qMax(nullsrvCount, contextState.csHighestActiveSrvBinding);
    nullsrvCount += 1;
    if (nullsrvCount > 0) {
        QVarLengthArray<ID3D11ShaderResourceView *,
                D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> nullsrvs(nullsrvCount);
        for (int i = 0; i < nullsrvs.count(); ++i)
            nullsrvs[i] = nullptr;
        if (contextState.vsHighestActiveSrvBinding >= 0) {
            context->VSSetShaderResources(0, UINT(contextState.vsHighestActiveSrvBinding + 1), nullsrvs.constData());
            contextState.vsHighestActiveSrvBinding = -1;
        }
        if (contextState.hsHighestActiveSrvBinding >= 0) {
            context->HSSetShaderResources(0, UINT(contextState.hsHighestActiveSrvBinding + 1), nullsrvs.constData());
            contextState.hsHighestActiveSrvBinding = -1;
        }
        if (contextState.dsHighestActiveSrvBinding >= 0) {
            context->DSSetShaderResources(0, UINT(contextState.dsHighestActiveSrvBinding + 1), nullsrvs.constData());
            contextState.dsHighestActiveSrvBinding = -1;
        }
        if (contextState.gsHighestActiveSrvBinding >= 0) {
            context->GSSetShaderResources(0, UINT(contextState.gsHighestActiveSrvBinding + 1), nullsrvs.constData());
            contextState.gsHighestActiveSrvBinding = -1;
        }
        if (contextState.fsHighestActiveSrvBinding >= 0) {
            context->PSSetShaderResources(0, UINT(contextState.fsHighestActiveSrvBinding + 1), nullsrvs.constData());
            contextState.fsHighestActiveSrvBinding = -1;
        }
        if (contextState.csHighestActiveSrvBinding >= 0) {
            context->CSSetShaderResources(0, UINT(contextState.csHighestActiveSrvBinding + 1), nullsrvs.constData());
            contextState.csHighestActiveSrvBinding = -1;
        }
    }

    if (contextState.csHighestActiveUavBinding >= 0) {
        const int nulluavCount = contextState.csHighestActiveUavBinding + 1;
        QVarLengthArray<ID3D11UnorderedAccessView *,
                D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> nulluavs(nulluavCount);
        for (int i = 0; i < nulluavCount; ++i)
            nulluavs[i] = nullptr;
        context->CSSetUnorderedAccessViews(0, UINT(nulluavCount), nulluavs.constData(), nullptr);
        contextState.csHighestActiveUavBinding = -1;
    }
}

#define SETSHADER(StageL, StageU) \
    if (psD->StageL.shader) { \
        context->StageU##SetShader(psD->StageL.shader, nullptr, 0); \
        currentShaderMask |= StageU##MaskBit; \
    } else if (currentShaderMask & StageU##MaskBit) { \
        context->StageU##SetShader(nullptr, nullptr, 0); \
        currentShaderMask &= ~StageU##MaskBit; \
    }

void QRhiD3D11::executeCommandBuffer(QD3D11CommandBuffer *cbD, QD3D11SwapChain *timestampSwapChain)
{
    quint32 stencilRef = 0;
    float blendConstants[] = { 1, 1, 1, 1 };
    enum ActiveShaderMask {
        VSMaskBit = 0x01,
        HSMaskBit = 0x02,
        DSMaskBit = 0x04,
        GSMaskBit = 0x08,
        PSMaskBit = 0x10
    };
    int currentShaderMask = 0xFF;

    if (timestampSwapChain) {
        const int currentFrameSlot = timestampSwapChain->currentFrameSlot;
        ID3D11Query *tsDisjoint = timestampSwapChain->timestamps.disjointQuery[currentFrameSlot];
        const int tsIdx = QD3D11SwapChain::BUFFER_COUNT * currentFrameSlot;
        ID3D11Query *tsStart = timestampSwapChain->timestamps.query[tsIdx];
        if (tsDisjoint && tsStart && !timestampSwapChain->timestamps.active[currentFrameSlot]) {
            // The timestamps seem to include vsync time with Present(1), except
            // when running on a non-primary gpu. This is not ideal. So try working
            // it around by issuing a semi-fake OMSetRenderTargets early and
            // writing the first timestamp only afterwards.
            context->Begin(tsDisjoint);
            QD3D11RenderTargetData *rtD = rtData(&timestampSwapChain->rt);
            context->OMSetRenderTargets(UINT(rtD->colorAttCount), rtD->colorAttCount ? rtD->rtv : nullptr, rtD->dsv);
            context->End(tsStart); // just record a timestamp, no Begin needed
        }
    }

    for (auto it = cbD->commands.cbegin(), end = cbD->commands.cend(); it != end; ++it) {
        const QD3D11CommandBuffer::Command &cmd(*it);
        switch (cmd.cmd) {
        case QD3D11CommandBuffer::Command::ResetShaderResources:
            resetShaderResources();
            break;
        case QD3D11CommandBuffer::Command::SetRenderTarget:
        {
            QD3D11RenderTargetData *rtD = rtData(cmd.args.setRenderTarget.rt);
            context->OMSetRenderTargets(UINT(rtD->colorAttCount), rtD->colorAttCount ? rtD->rtv : nullptr, rtD->dsv);
        }
            break;
        case QD3D11CommandBuffer::Command::Clear:
        {
            QD3D11RenderTargetData *rtD = rtData(cmd.args.clear.rt);
            if (cmd.args.clear.mask & QD3D11CommandBuffer::Command::Color) {
                for (int i = 0; i < rtD->colorAttCount; ++i)
                    context->ClearRenderTargetView(rtD->rtv[i], cmd.args.clear.c);
            }
            uint ds = 0;
            if (cmd.args.clear.mask & QD3D11CommandBuffer::Command::Depth)
                ds |= D3D11_CLEAR_DEPTH;
            if (cmd.args.clear.mask & QD3D11CommandBuffer::Command::Stencil)
                ds |= D3D11_CLEAR_STENCIL;
            if (ds)
                context->ClearDepthStencilView(rtD->dsv, ds, cmd.args.clear.d, UINT8(cmd.args.clear.s));
        }
            break;
        case QD3D11CommandBuffer::Command::Viewport:
        {
            D3D11_VIEWPORT v;
            v.TopLeftX = cmd.args.viewport.x;
            v.TopLeftY = cmd.args.viewport.y;
            v.Width = cmd.args.viewport.w;
            v.Height = cmd.args.viewport.h;
            v.MinDepth = cmd.args.viewport.d0;
            v.MaxDepth = cmd.args.viewport.d1;
            context->RSSetViewports(1, &v);
        }
            break;
        case QD3D11CommandBuffer::Command::Scissor:
        {
            D3D11_RECT r;
            r.left = cmd.args.scissor.x;
            r.top = cmd.args.scissor.y;
            // right and bottom are exclusive
            r.right = cmd.args.scissor.x + cmd.args.scissor.w;
            r.bottom = cmd.args.scissor.y + cmd.args.scissor.h;
            context->RSSetScissorRects(1, &r);
        }
            break;
        case QD3D11CommandBuffer::Command::BindVertexBuffers:
            contextState.vsHighestActiveVertexBufferBinding = qMax<int>(
                        contextState.vsHighestActiveVertexBufferBinding,
                        cmd.args.bindVertexBuffers.startSlot + cmd.args.bindVertexBuffers.slotCount - 1);
            context->IASetVertexBuffers(UINT(cmd.args.bindVertexBuffers.startSlot),
                                        UINT(cmd.args.bindVertexBuffers.slotCount),
                                        cmd.args.bindVertexBuffers.buffers,
                                        cmd.args.bindVertexBuffers.strides,
                                        cmd.args.bindVertexBuffers.offsets);
            break;
        case QD3D11CommandBuffer::Command::BindIndexBuffer:
            contextState.vsHasIndexBufferBound = true;
            context->IASetIndexBuffer(cmd.args.bindIndexBuffer.buffer,
                                      cmd.args.bindIndexBuffer.format,
                                      cmd.args.bindIndexBuffer.offset);
            break;
        case QD3D11CommandBuffer::Command::BindGraphicsPipeline:
        {
            QD3D11GraphicsPipeline *psD = cmd.args.bindGraphicsPipeline.ps;
            SETSHADER(vs, VS)
            SETSHADER(hs, HS)
            SETSHADER(ds, DS)
            SETSHADER(gs, GS)
            SETSHADER(fs, PS)
            context->IASetPrimitiveTopology(psD->d3dTopology);
            context->IASetInputLayout(psD->inputLayout); // may be null, that's ok
            context->OMSetDepthStencilState(psD->dsState, stencilRef);
            context->OMSetBlendState(psD->blendState, blendConstants, 0xffffffff);
            context->RSSetState(psD->rastState);
        }
            break;
        case QD3D11CommandBuffer::Command::BindShaderResources:
            bindShaderResources(cmd.args.bindShaderResources.srb,
                                cmd.args.bindShaderResources.dynamicOffsetPairs,
                                cmd.args.bindShaderResources.dynamicOffsetCount,
                                cmd.args.bindShaderResources.offsetOnlyChange);
            break;
        case QD3D11CommandBuffer::Command::StencilRef:
            stencilRef = cmd.args.stencilRef.ref;
            context->OMSetDepthStencilState(cmd.args.stencilRef.ps->dsState, stencilRef);
            break;
        case QD3D11CommandBuffer::Command::BlendConstants:
            memcpy(blendConstants, cmd.args.blendConstants.c, 4 * sizeof(float));
            context->OMSetBlendState(cmd.args.blendConstants.ps->blendState, blendConstants, 0xffffffff);
            break;
        case QD3D11CommandBuffer::Command::Draw:
            if (cmd.args.draw.ps) {
                if (cmd.args.draw.instanceCount == 1)
                    context->Draw(cmd.args.draw.vertexCount, cmd.args.draw.firstVertex);
                else
                    context->DrawInstanced(cmd.args.draw.vertexCount, cmd.args.draw.instanceCount,
                                           cmd.args.draw.firstVertex, cmd.args.draw.firstInstance);
            } else {
                qWarning("No graphics pipeline active for draw; ignored");
            }
            break;
        case QD3D11CommandBuffer::Command::DrawIndexed:
            if (cmd.args.drawIndexed.ps) {
                if (cmd.args.drawIndexed.instanceCount == 1)
                    context->DrawIndexed(cmd.args.drawIndexed.indexCount, cmd.args.drawIndexed.firstIndex,
                                         cmd.args.drawIndexed.vertexOffset);
                else
                    context->DrawIndexedInstanced(cmd.args.drawIndexed.indexCount, cmd.args.drawIndexed.instanceCount,
                                                  cmd.args.drawIndexed.firstIndex, cmd.args.drawIndexed.vertexOffset,
                                                  cmd.args.drawIndexed.firstInstance);
            } else {
                qWarning("No graphics pipeline active for drawIndexed; ignored");
            }
            break;
        case QD3D11CommandBuffer::Command::UpdateSubRes:
            context->UpdateSubresource(cmd.args.updateSubRes.dst, cmd.args.updateSubRes.dstSubRes,
                                       cmd.args.updateSubRes.hasDstBox ? &cmd.args.updateSubRes.dstBox : nullptr,
                                       cmd.args.updateSubRes.src, cmd.args.updateSubRes.srcRowPitch, 0);
            break;
        case QD3D11CommandBuffer::Command::CopySubRes:
            context->CopySubresourceRegion(cmd.args.copySubRes.dst, cmd.args.copySubRes.dstSubRes,
                                           cmd.args.copySubRes.dstX, cmd.args.copySubRes.dstY, cmd.args.copySubRes.dstZ,
                                           cmd.args.copySubRes.src, cmd.args.copySubRes.srcSubRes,
                                           cmd.args.copySubRes.hasSrcBox ? &cmd.args.copySubRes.srcBox : nullptr);
            break;
        case QD3D11CommandBuffer::Command::ResolveSubRes:
            context->ResolveSubresource(cmd.args.resolveSubRes.dst, cmd.args.resolveSubRes.dstSubRes,
                                        cmd.args.resolveSubRes.src, cmd.args.resolveSubRes.srcSubRes,
                                        cmd.args.resolveSubRes.format);
            break;
        case QD3D11CommandBuffer::Command::GenMip:
            context->GenerateMips(cmd.args.genMip.srv);
            break;
        case QD3D11CommandBuffer::Command::DebugMarkBegin:
            annotations->BeginEvent(reinterpret_cast<LPCWSTR>(QString::fromLatin1(cmd.args.debugMark.s).utf16()));
            break;
        case QD3D11CommandBuffer::Command::DebugMarkEnd:
            annotations->EndEvent();
            break;
        case QD3D11CommandBuffer::Command::DebugMarkMsg:
            annotations->SetMarker(reinterpret_cast<LPCWSTR>(QString::fromLatin1(cmd.args.debugMark.s).utf16()));
            break;
        case QD3D11CommandBuffer::Command::BindComputePipeline:
            context->CSSetShader(cmd.args.bindComputePipeline.ps->cs.shader, nullptr, 0);
            break;
        case QD3D11CommandBuffer::Command::Dispatch:
            context->Dispatch(cmd.args.dispatch.x, cmd.args.dispatch.y, cmd.args.dispatch.z);
            break;
        default:
            break;
        }
    }
}

QD3D11Buffer::QD3D11Buffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size)
    : QRhiBuffer(rhi, type, usage, size)
{
}

QD3D11Buffer::~QD3D11Buffer()
{
    destroy();
}

void QD3D11Buffer::destroy()
{
    if (!buffer)
        return;

    buffer->Release();
    buffer = nullptr;

    delete[] dynBuf;
    dynBuf = nullptr;

    for (auto it = uavs.begin(), end = uavs.end(); it != end; ++it)
        it.value()->Release();
    uavs.clear();

    QRHI_RES_RHI(QRhiD3D11);
    if (rhiD)
        rhiD->unregisterResource(this);
}

static inline uint toD3DBufferUsage(QRhiBuffer::UsageFlags usage)
{
    int u = 0;
    if (usage.testFlag(QRhiBuffer::VertexBuffer))
        u |= D3D11_BIND_VERTEX_BUFFER;
    if (usage.testFlag(QRhiBuffer::IndexBuffer))
        u |= D3D11_BIND_INDEX_BUFFER;
    if (usage.testFlag(QRhiBuffer::UniformBuffer))
        u |= D3D11_BIND_CONSTANT_BUFFER;
    if (usage.testFlag(QRhiBuffer::StorageBuffer))
        u |= D3D11_BIND_UNORDERED_ACCESS;
    return uint(u);
}

bool QD3D11Buffer::create()
{
    if (buffer)
        destroy();

    if (m_usage.testFlag(QRhiBuffer::UniformBuffer) && m_type != Dynamic) {
        qWarning("UniformBuffer must always be combined with Dynamic on D3D11");
        return false;
    }

    if (m_usage.testFlag(QRhiBuffer::StorageBuffer) && m_type == Dynamic) {
        qWarning("StorageBuffer cannot be combined with Dynamic");
        return false;
    }

    const quint32 nonZeroSize = m_size <= 0 ? 256 : m_size;
    const quint32 roundedSize = aligned(nonZeroSize, m_usage.testFlag(QRhiBuffer::UniformBuffer) ? 256u : 4u);

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = roundedSize;
    desc.Usage = m_type == Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.BindFlags = toD3DBufferUsage(m_usage);
    desc.CPUAccessFlags = m_type == Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.MiscFlags = m_usage.testFlag(QRhiBuffer::StorageBuffer) ? D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS : 0;

    QRHI_RES_RHI(QRhiD3D11);
    HRESULT hr = rhiD->dev->CreateBuffer(&desc, nullptr, &buffer);
    if (FAILED(hr)) {
        qWarning("Failed to create buffer: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }

    if (m_type == Dynamic) {
        dynBuf = new char[nonZeroSize];
        hasPendingDynamicUpdates = false;
    }

    if (!m_objectName.isEmpty())
        buffer->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(m_objectName.size()), m_objectName.constData());

    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QRhiBuffer::NativeBuffer QD3D11Buffer::nativeBuffer()
{
    if (m_type == Dynamic) {
        QRHI_RES_RHI(QRhiD3D11);
        rhiD->executeBufferHostWrites(this);
    }
    return { { &buffer }, 1 };
}

char *QD3D11Buffer::beginFullDynamicBufferUpdateForCurrentFrame()
{
    // Shortcut the entire buffer update mechanism and allow the client to do
    // the host writes directly to the buffer. This will lead to unexpected
    // results when combined with QRhiResourceUpdateBatch-based updates for the
    // buffer, since dynBuf is left untouched and out of sync, but provides a
    // fast path for dynamic buffers that have all their content changed in
    // every frame.
    Q_ASSERT(m_type == Dynamic);
    D3D11_MAPPED_SUBRESOURCE mp;
    QRHI_RES_RHI(QRhiD3D11);
    HRESULT hr = rhiD->context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mp);
    if (FAILED(hr)) {
        qWarning("Failed to map buffer: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return nullptr;
    }
    return static_cast<char *>(mp.pData);
}

void QD3D11Buffer::endFullDynamicBufferUpdateForCurrentFrame()
{
    QRHI_RES_RHI(QRhiD3D11);
    rhiD->context->Unmap(buffer, 0);
}

ID3D11UnorderedAccessView *QD3D11Buffer::unorderedAccessView(quint32 offset)
{
    auto it = uavs.find(offset);
    if (it != uavs.end())
        return it.value();

    // SPIRV-Cross generated HLSL uses RWByteAddressBuffer
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = offset / 4u;
    desc.Buffer.NumElements = aligned(m_size - offset, 4u) / 4u;
    desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

    QRHI_RES_RHI(QRhiD3D11);
    ID3D11UnorderedAccessView *uav = nullptr;
    HRESULT hr = rhiD->dev->CreateUnorderedAccessView(buffer, &desc, &uav);
    if (FAILED(hr)) {
        qWarning("Failed to create UAV: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return nullptr;
    }

    uavs[offset] = uav;
    return uav;
}

QD3D11RenderBuffer::QD3D11RenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                                       int sampleCount, QRhiRenderBuffer::Flags flags,
                                       QRhiTexture::Format backingFormatHint)
    : QRhiRenderBuffer(rhi, type, pixelSize, sampleCount, flags, backingFormatHint)
{
}

QD3D11RenderBuffer::~QD3D11RenderBuffer()
{
    destroy();
}

void QD3D11RenderBuffer::destroy()
{
    if (!tex)
        return;

    if (dsv) {
        dsv->Release();
        dsv = nullptr;
    }

    if (rtv) {
        rtv->Release();
        rtv = nullptr;
    }

    tex->Release();
    tex = nullptr;

    QRHI_RES_RHI(QRhiD3D11);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QD3D11RenderBuffer::create()
{
    if (tex)
        destroy();

    if (m_pixelSize.isEmpty())
        return false;

    QRHI_RES_RHI(QRhiD3D11);
    sampleDesc = rhiD->effectiveSampleCount(m_sampleCount);

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = UINT(m_pixelSize.width());
    desc.Height = UINT(m_pixelSize.height());
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc = sampleDesc;
    desc.Usage = D3D11_USAGE_DEFAULT;

    if (m_type == Color) {
        dxgiFormat = m_backingFormatHint == QRhiTexture::UnknownFormat ? DXGI_FORMAT_R8G8B8A8_UNORM
                                                                       : toD3DTextureFormat(m_backingFormatHint, {});
        desc.Format = dxgiFormat;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        HRESULT hr = rhiD->dev->CreateTexture2D(&desc, nullptr, &tex);
        if (FAILED(hr)) {
            qWarning("Failed to create color renderbuffer: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = dxgiFormat;
        rtvDesc.ViewDimension = desc.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS
                                                          : D3D11_RTV_DIMENSION_TEXTURE2D;
        hr = rhiD->dev->CreateRenderTargetView(tex, &rtvDesc, &rtv);
        if (FAILED(hr)) {
            qWarning("Failed to create rtv: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    } else if (m_type == DepthStencil) {
        dxgiFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.Format = dxgiFormat;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        HRESULT hr = rhiD->dev->CreateTexture2D(&desc, nullptr, &tex);
        if (FAILED(hr)) {
            qWarning("Failed to create depth-stencil buffer: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = dxgiFormat;
        dsvDesc.ViewDimension = desc.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS
                                                          : D3D11_DSV_DIMENSION_TEXTURE2D;
        hr = rhiD->dev->CreateDepthStencilView(tex, &dsvDesc, &dsv);
        if (FAILED(hr)) {
            qWarning("Failed to create dsv: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    } else {
        return false;
    }

    if (!m_objectName.isEmpty())
        tex->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(m_objectName.size()), m_objectName.constData());

    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::Format QD3D11RenderBuffer::backingFormat() const
{
    if (m_backingFormatHint != QRhiTexture::UnknownFormat)
        return m_backingFormatHint;
    else
        return m_type == Color ? QRhiTexture::RGBA8 : QRhiTexture::UnknownFormat;
}

QD3D11Texture::QD3D11Texture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
                             int arraySize, int sampleCount, Flags flags)
    : QRhiTexture(rhi, format, pixelSize, depth, arraySize, sampleCount, flags)
{
    for (int i = 0; i < QRhi::MAX_MIP_LEVELS; ++i)
        perLevelViews[i] = nullptr;
}

QD3D11Texture::~QD3D11Texture()
{
    destroy();
}

void QD3D11Texture::destroy()
{
    if (!tex && !tex3D && !tex1D)
        return;

    if (srv) {
        srv->Release();
        srv = nullptr;
    }

    for (int i = 0; i < QRhi::MAX_MIP_LEVELS; ++i) {
        if (perLevelViews[i]) {
            perLevelViews[i]->Release();
            perLevelViews[i] = nullptr;
        }
    }

    if (owns) {
        if (tex)
            tex->Release();
        if (tex3D)
            tex3D->Release();
        if (tex1D)
            tex1D->Release();
    }

    tex = nullptr;
    tex3D = nullptr;
    tex1D = nullptr;

    QRHI_RES_RHI(QRhiD3D11);
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
    default:
        Q_UNREACHABLE();
        return DXGI_FORMAT_R32_FLOAT;
    }
}

static inline DXGI_FORMAT toD3DDepthTextureDSVFormat(QRhiTexture::Format format)
{
    switch (format) {
    case QRhiTexture::Format::D16:
        return DXGI_FORMAT_D16_UNORM;
    case QRhiTexture::Format::D24:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case QRhiTexture::Format::D24S8:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case QRhiTexture::Format::D32F:
        return DXGI_FORMAT_D32_FLOAT;
    default:
        Q_UNREACHABLE();
        return DXGI_FORMAT_D32_FLOAT;
    }
}

bool QD3D11Texture::prepareCreate(QSize *adjustedSize)
{
    if (tex || tex3D || tex1D)
        destroy();

    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);
    const bool is1D = m_flags.testFlag(OneDimensional);

    const QSize size = is1D ? QSize(qMax(1, m_pixelSize.width()), 1)
                            : (m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize);

    QRHI_RES_RHI(QRhiD3D11);
    dxgiFormat = toD3DTextureFormat(m_format, m_flags);
    mipLevelCount = uint(hasMipMaps ? rhiD->q->mipLevelsForSize(size) : 1);
    sampleDesc = rhiD->effectiveSampleCount(m_sampleCount);
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

bool QD3D11Texture::finishCreate()
{
    QRHI_RES_RHI(QRhiD3D11);
    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool is1D = m_flags.testFlag(OneDimensional);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = isDepth ? toD3DDepthTextureSRVFormat(m_format) : dxgiFormat;
    if (isCube) {
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = mipLevelCount;
    } else {
        if (is1D) {
            if (isArray) {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                srvDesc.Texture1DArray.MipLevels = mipLevelCount;
                if (m_arrayRangeStart >= 0 && m_arrayRangeLength >= 0) {
                    srvDesc.Texture1DArray.FirstArraySlice = UINT(m_arrayRangeStart);
                    srvDesc.Texture1DArray.ArraySize = UINT(m_arrayRangeLength);
                } else {
                    srvDesc.Texture1DArray.FirstArraySlice = 0;
                    srvDesc.Texture1DArray.ArraySize = UINT(qMax(0, m_arraySize));
                }
            } else {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                srvDesc.Texture1D.MipLevels = mipLevelCount;
            }
        } else if (isArray) {
            if (sampleDesc.Count > 1) {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                if (m_arrayRangeStart >= 0 && m_arrayRangeLength >= 0) {
                    srvDesc.Texture2DMSArray.FirstArraySlice = UINT(m_arrayRangeStart);
                    srvDesc.Texture2DMSArray.ArraySize = UINT(m_arrayRangeLength);
                } else {
                    srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                    srvDesc.Texture2DMSArray.ArraySize = UINT(qMax(0, m_arraySize));
                }
            } else {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
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
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            } else if (is3D) {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MipLevels = mipLevelCount;
            } else {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = mipLevelCount;
            }
        }
    }

    HRESULT hr = rhiD->dev->CreateShaderResourceView(textureResource(), &srvDesc, &srv);
    if (FAILED(hr)) {
        qWarning("Failed to create srv: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }

    generation += 1;
    return true;
}

bool QD3D11Texture::create()
{
    QSize size;
    if (!prepareCreate(&size))
        return false;

    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool is1D = m_flags.testFlag(OneDimensional);

    uint bindFlags = D3D11_BIND_SHADER_RESOURCE;
    uint miscFlags = isCube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
    if (m_flags.testFlag(RenderTarget)) {
        if (isDepth)
            bindFlags |= D3D11_BIND_DEPTH_STENCIL;
        else
            bindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    if (m_flags.testFlag(UsedWithGenerateMips)) {
        if (isDepth) {
            qWarning("Depth texture cannot have mipmaps generated");
            return false;
        }
        bindFlags |= D3D11_BIND_RENDER_TARGET;
        miscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    if (m_flags.testFlag(UsedWithLoadStore))
        bindFlags |= D3D11_BIND_UNORDERED_ACCESS;

    QRHI_RES_RHI(QRhiD3D11);
    if (is1D) {
        D3D11_TEXTURE1D_DESC desc = {};
        desc.Width = UINT(size.width());
        desc.MipLevels = mipLevelCount;
        desc.ArraySize = isArray ? UINT(qMax(0, m_arraySize)) : 1;
        desc.Format = dxgiFormat;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = bindFlags;
        desc.MiscFlags = miscFlags;

        HRESULT hr = rhiD->dev->CreateTexture1D(&desc, nullptr, &tex1D);
        if (FAILED(hr)) {
            qWarning("Failed to create 1D texture: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
        if (!m_objectName.isEmpty())
            tex->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(m_objectName.size()),
                                m_objectName.constData());
    } else if (!is3D) {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = UINT(size.width());
        desc.Height = UINT(size.height());
        desc.MipLevels = mipLevelCount;
        desc.ArraySize = isCube ? 6 : (isArray ? UINT(qMax(0, m_arraySize)) : 1);
        desc.Format = dxgiFormat;
        desc.SampleDesc = sampleDesc;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = bindFlags;
        desc.MiscFlags = miscFlags;

        HRESULT hr = rhiD->dev->CreateTexture2D(&desc, nullptr, &tex);
        if (FAILED(hr)) {
            qWarning("Failed to create 2D texture: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
        if (!m_objectName.isEmpty())
            tex->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(m_objectName.size()), m_objectName.constData());
    } else {
        D3D11_TEXTURE3D_DESC desc = {};
        desc.Width = UINT(size.width());
        desc.Height = UINT(size.height());
        desc.Depth = UINT(qMax(1, m_depth));
        desc.MipLevels = mipLevelCount;
        desc.Format = dxgiFormat;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = bindFlags;
        desc.MiscFlags = miscFlags;

        HRESULT hr = rhiD->dev->CreateTexture3D(&desc, nullptr, &tex3D);
        if (FAILED(hr)) {
            qWarning("Failed to create 3D texture: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
        if (!m_objectName.isEmpty())
            tex3D->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(m_objectName.size()), m_objectName.constData());
    }

    if (!finishCreate())
        return false;

    owns = true;
    rhiD->registerResource(this);
    return true;
}

bool QD3D11Texture::createFrom(QRhiTexture::NativeTexture src)
{
    if (!src.object)
        return false;

    if (!prepareCreate())
        return false;

    if (m_flags.testFlag(ThreeDimensional))
        tex3D = reinterpret_cast<ID3D11Texture3D *>(src.object);
    else if (m_flags.testFlags(OneDimensional))
        tex1D = reinterpret_cast<ID3D11Texture1D *>(src.object);
    else
        tex = reinterpret_cast<ID3D11Texture2D *>(src.object);

    if (!finishCreate())
        return false;

    owns = false;
    QRHI_RES_RHI(QRhiD3D11);
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::NativeTexture QD3D11Texture::nativeTexture()
{
    return { quint64(textureResource()), 0 };
}

ID3D11UnorderedAccessView *QD3D11Texture::unorderedAccessViewForLevel(int level)
{
    if (perLevelViews[level])
        return perLevelViews[level];

    const bool isCube = m_flags.testFlag(CubeMap);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format = dxgiFormat;
    if (isCube) {
        desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.MipSlice = UINT(level);
        desc.Texture2DArray.FirstArraySlice = 0;
        desc.Texture2DArray.ArraySize = 6;
    } else if (isArray) {
        desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.MipSlice = UINT(level);
        desc.Texture2DArray.FirstArraySlice = 0;
        desc.Texture2DArray.ArraySize = UINT(qMax(0, m_arraySize));
    } else if (is3D) {
        desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
        desc.Texture3D.MipSlice = UINT(level);
    } else {
        desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = UINT(level);
    }

    QRHI_RES_RHI(QRhiD3D11);
    ID3D11UnorderedAccessView *uav = nullptr;
    HRESULT hr = rhiD->dev->CreateUnorderedAccessView(textureResource(), &desc, &uav);
    if (FAILED(hr)) {
        qWarning("Failed to create UAV: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return nullptr;
    }

    perLevelViews[level] = uav;
    return uav;
}

QD3D11Sampler::QD3D11Sampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                             AddressMode u, AddressMode v, AddressMode w)
    : QRhiSampler(rhi, magFilter, minFilter, mipmapMode, u, v, w)
{
}

QD3D11Sampler::~QD3D11Sampler()
{
    destroy();
}

void QD3D11Sampler::destroy()
{
    if (!samplerState)
        return;

    samplerState->Release();
    samplerState = nullptr;

    QRHI_RES_RHI(QRhiD3D11);
    if (rhiD)
        rhiD->unregisterResource(this);
}

static inline D3D11_FILTER toD3DFilter(QRhiSampler::Filter minFilter, QRhiSampler::Filter magFilter, QRhiSampler::Filter mipFilter)
{
    if (minFilter == QRhiSampler::Nearest) {
        if (magFilter == QRhiSampler::Nearest) {
            if (mipFilter == QRhiSampler::Linear)
                return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            else
                return D3D11_FILTER_MIN_MAG_MIP_POINT;
        } else {
            if (mipFilter == QRhiSampler::Linear)
                return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
            else
                return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        }
    } else {
        if (magFilter == QRhiSampler::Nearest) {
            if (mipFilter == QRhiSampler::Linear)
                return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            else
                return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        } else {
            if (mipFilter == QRhiSampler::Linear)
                return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            else
                return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        }
    }

    Q_UNREACHABLE();
    return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
}

static inline D3D11_TEXTURE_ADDRESS_MODE toD3DAddressMode(QRhiSampler::AddressMode m)
{
    switch (m) {
    case QRhiSampler::Repeat:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    case QRhiSampler::ClampToEdge:
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    case QRhiSampler::Mirror:
        return D3D11_TEXTURE_ADDRESS_MIRROR;
    default:
        Q_UNREACHABLE();
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    }
}

static inline D3D11_COMPARISON_FUNC toD3DTextureComparisonFunc(QRhiSampler::CompareOp op)
{
    switch (op) {
    case QRhiSampler::Never:
        return D3D11_COMPARISON_NEVER;
    case QRhiSampler::Less:
        return D3D11_COMPARISON_LESS;
    case QRhiSampler::Equal:
        return D3D11_COMPARISON_EQUAL;
    case QRhiSampler::LessOrEqual:
        return D3D11_COMPARISON_LESS_EQUAL;
    case QRhiSampler::Greater:
        return D3D11_COMPARISON_GREATER;
    case QRhiSampler::NotEqual:
        return D3D11_COMPARISON_NOT_EQUAL;
    case QRhiSampler::GreaterOrEqual:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case QRhiSampler::Always:
        return D3D11_COMPARISON_ALWAYS;
    default:
        Q_UNREACHABLE();
        return D3D11_COMPARISON_NEVER;
    }
}

bool QD3D11Sampler::create()
{
    if (samplerState)
        destroy();

    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = toD3DFilter(m_minFilter, m_magFilter, m_mipmapMode);
    if (m_compareOp != Never)
        desc.Filter = D3D11_FILTER(desc.Filter | 0x80);
    desc.AddressU = toD3DAddressMode(m_addressU);
    desc.AddressV = toD3DAddressMode(m_addressV);
    desc.AddressW = toD3DAddressMode(m_addressW);
    desc.MaxAnisotropy = 1.0f;
    desc.ComparisonFunc = toD3DTextureComparisonFunc(m_compareOp);
    desc.MaxLOD = m_mipmapMode == None ? 0.0f : 1000.0f;

    QRHI_RES_RHI(QRhiD3D11);
    HRESULT hr = rhiD->dev->CreateSamplerState(&desc, &samplerState);
    if (FAILED(hr)) {
        qWarning("Failed to create sampler state: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }

    generation += 1;
    rhiD->registerResource(this);
    return true;
}

// dummy, no Vulkan-style RenderPass+Framebuffer concept here
QD3D11RenderPassDescriptor::QD3D11RenderPassDescriptor(QRhiImplementation *rhi)
    : QRhiRenderPassDescriptor(rhi)
{
}

QD3D11RenderPassDescriptor::~QD3D11RenderPassDescriptor()
{
    destroy();
}

void QD3D11RenderPassDescriptor::destroy()
{
    QRHI_RES_RHI(QRhiD3D11);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QD3D11RenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const
{
    Q_UNUSED(other);
    return true;
}

QRhiRenderPassDescriptor *QD3D11RenderPassDescriptor::newCompatibleRenderPassDescriptor() const
{
    QD3D11RenderPassDescriptor *rpD = new QD3D11RenderPassDescriptor(m_rhi);
    QRHI_RES_RHI(QRhiD3D11);
    rhiD->registerResource(rpD, false);
    return rpD;
}

QVector<quint32> QD3D11RenderPassDescriptor::serializedFormat() const
{
    return {};
}

QD3D11SwapChainRenderTarget::QD3D11SwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain)
    : QRhiSwapChainRenderTarget(rhi, swapchain),
      d(rhi)
{
}

QD3D11SwapChainRenderTarget::~QD3D11SwapChainRenderTarget()
{
    destroy();
}

void QD3D11SwapChainRenderTarget::destroy()
{
    // nothing to do here
}

QSize QD3D11SwapChainRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QD3D11SwapChainRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QD3D11SwapChainRenderTarget::sampleCount() const
{
    return d.sampleCount;
}

QD3D11TextureRenderTarget::QD3D11TextureRenderTarget(QRhiImplementation *rhi,
                                                     const QRhiTextureRenderTargetDescription &desc,
                                                     Flags flags)
    : QRhiTextureRenderTarget(rhi, desc, flags),
      d(rhi)
{
    for (int i = 0; i < QD3D11RenderTargetData::MAX_COLOR_ATTACHMENTS; ++i) {
        ownsRtv[i] = false;
        rtv[i] = nullptr;
    }
}

QD3D11TextureRenderTarget::~QD3D11TextureRenderTarget()
{
    destroy();
}

void QD3D11TextureRenderTarget::destroy()
{
    if (!rtv[0] && !dsv)
        return;

    if (dsv) {
        if (ownsDsv)
            dsv->Release();
        dsv = nullptr;
    }

    for (int i = 0; i < QD3D11RenderTargetData::MAX_COLOR_ATTACHMENTS; ++i) {
        if (rtv[i]) {
            if (ownsRtv[i])
                rtv[i]->Release();
            rtv[i] = nullptr;
        }
    }

    QRHI_RES_RHI(QRhiD3D11);
    if (rhiD)
        rhiD->unregisterResource(this);
}

QRhiRenderPassDescriptor *QD3D11TextureRenderTarget::newCompatibleRenderPassDescriptor()
{
    QD3D11RenderPassDescriptor *rpD = new QD3D11RenderPassDescriptor(m_rhi);
    QRHI_RES_RHI(QRhiD3D11);
    rhiD->registerResource(rpD, false);
    return rpD;
}

bool QD3D11TextureRenderTarget::create()
{
    if (rtv[0] || dsv)
        destroy();

    Q_ASSERT(m_desc.colorAttachmentCount() > 0 || m_desc.depthTexture());
    Q_ASSERT(!m_desc.depthStencilBuffer() || !m_desc.depthTexture());
    const bool hasDepthStencil = m_desc.depthStencilBuffer() || m_desc.depthTexture();

    QRHI_RES_RHI(QRhiD3D11);

    d.colorAttCount = 0;
    int attIndex = 0;
    for (auto it = m_desc.cbeginColorAttachments(), itEnd = m_desc.cendColorAttachments(); it != itEnd; ++it, ++attIndex) {
        d.colorAttCount += 1;
        const QRhiColorAttachment &colorAtt(*it);
        QRhiTexture *texture = colorAtt.texture();
        QRhiRenderBuffer *rb = colorAtt.renderBuffer();
        Q_ASSERT(texture || rb);
        if (texture) {
            QD3D11Texture *texD = QRHI_RES(QD3D11Texture, texture);
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = toD3DTextureFormat(texD->format(), texD->flags());
            if (texD->flags().testFlag(QRhiTexture::CubeMap)) {
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = UINT(colorAtt.level());
                rtvDesc.Texture2DArray.FirstArraySlice = UINT(colorAtt.layer());
                rtvDesc.Texture2DArray.ArraySize = 1;
            } else if (texD->flags().testFlag(QRhiTexture::OneDimensional)) {
                if (texD->flags().testFlag(QRhiTexture::TextureArray)) {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                    rtvDesc.Texture1DArray.MipSlice = UINT(colorAtt.level());
                    rtvDesc.Texture1DArray.FirstArraySlice = UINT(colorAtt.layer());
                    rtvDesc.Texture1DArray.ArraySize = 1;
                } else {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                    rtvDesc.Texture1D.MipSlice = UINT(colorAtt.level());
                }
            } else if (texD->flags().testFlag(QRhiTexture::TextureArray)) {
                if (texD->sampleDesc.Count > 1) {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    rtvDesc.Texture2DMSArray.FirstArraySlice = UINT(colorAtt.layer());
                    rtvDesc.Texture2DMSArray.ArraySize = 1;
                } else {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtvDesc.Texture2DArray.MipSlice = UINT(colorAtt.level());
                    rtvDesc.Texture2DArray.FirstArraySlice = UINT(colorAtt.layer());
                    rtvDesc.Texture2DArray.ArraySize = 1;
                }
            } else if (texD->flags().testFlag(QRhiTexture::ThreeDimensional)) {
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                rtvDesc.Texture3D.MipSlice = UINT(colorAtt.level());
                rtvDesc.Texture3D.FirstWSlice = UINT(colorAtt.layer());
                rtvDesc.Texture3D.WSize = 1;
            } else {
                if (texD->sampleDesc.Count > 1) {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                } else {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    rtvDesc.Texture2D.MipSlice = UINT(colorAtt.level());
                }
            }
            HRESULT hr = rhiD->dev->CreateRenderTargetView(texD->textureResource(), &rtvDesc, &rtv[attIndex]);
            if (FAILED(hr)) {
                qWarning("Failed to create rtv: %s",
                    qPrintable(QSystemError::windowsComString(hr)));
                return false;
            }
            ownsRtv[attIndex] = true;
            if (attIndex == 0) {
                d.pixelSize = rhiD->q->sizeForMipLevel(colorAtt.level(), texD->pixelSize());
                d.sampleCount = int(texD->sampleDesc.Count);
            }
        } else if (rb) {
            QD3D11RenderBuffer *rbD = QRHI_RES(QD3D11RenderBuffer, rb);
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
            QD3D11Texture *depthTexD = QRHI_RES(QD3D11Texture, m_desc.depthTexture());
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = toD3DDepthTextureDSVFormat(depthTexD->format());
            dsvDesc.ViewDimension = depthTexD->sampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS
                                                                    : D3D11_DSV_DIMENSION_TEXTURE2D;
            if (depthTexD->flags().testFlag(QRhiTexture::TextureArray)) {
                if (depthTexD->sampleDesc.Count > 1) {
                    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    if (depthTexD->arrayRangeStart() >= 0 && depthTexD->arrayRangeLength() >= 0) {
                        dsvDesc.Texture2DMSArray.FirstArraySlice = UINT(depthTexD->arrayRangeStart());
                        dsvDesc.Texture2DMSArray.ArraySize = UINT(depthTexD->arrayRangeLength());
                    } else {
                        dsvDesc.Texture2DMSArray.FirstArraySlice = 0;
                        dsvDesc.Texture2DMSArray.ArraySize = UINT(qMax(0, depthTexD->arraySize()));
                    }
                } else {
                    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    if (depthTexD->arrayRangeStart() >= 0 && depthTexD->arrayRangeLength() >= 0) {
                        dsvDesc.Texture2DArray.FirstArraySlice = UINT(depthTexD->arrayRangeStart());
                        dsvDesc.Texture2DArray.ArraySize = UINT(depthTexD->arrayRangeLength());
                    } else {
                        dsvDesc.Texture2DArray.FirstArraySlice = 0;
                        dsvDesc.Texture2DArray.ArraySize = UINT(qMax(0, depthTexD->arraySize()));
                    }
                }
            }
            HRESULT hr = rhiD->dev->CreateDepthStencilView(depthTexD->tex, &dsvDesc, &dsv);
            if (FAILED(hr)) {
                qWarning("Failed to create dsv: %s",
                    qPrintable(QSystemError::windowsComString(hr)));
                return false;
            }
            if (d.colorAttCount == 0) {
                d.pixelSize = depthTexD->pixelSize();
                d.sampleCount = int(depthTexD->sampleDesc.Count);
            }
        } else {
            ownsDsv = false;
            QD3D11RenderBuffer *depthRbD = QRHI_RES(QD3D11RenderBuffer, m_desc.depthStencilBuffer());
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

    for (int i = 0; i < QD3D11RenderTargetData::MAX_COLOR_ATTACHMENTS; ++i)
        d.rtv[i] = i < d.colorAttCount ? rtv[i] : nullptr;

    d.dsv = dsv;
    d.rp = QRHI_RES(QD3D11RenderPassDescriptor, m_renderPassDesc);

    QRhiRenderTargetAttachmentTracker::updateResIdList<QD3D11Texture, QD3D11RenderBuffer>(m_desc, &d.currentResIdList);

    rhiD->registerResource(this);
    return true;
}

QSize QD3D11TextureRenderTarget::pixelSize() const
{
    if (!QRhiRenderTargetAttachmentTracker::isUpToDate<QD3D11Texture, QD3D11RenderBuffer>(m_desc, d.currentResIdList))
        const_cast<QD3D11TextureRenderTarget *>(this)->create();

    return d.pixelSize;
}

float QD3D11TextureRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QD3D11TextureRenderTarget::sampleCount() const
{
    return d.sampleCount;
}

QD3D11ShaderResourceBindings::QD3D11ShaderResourceBindings(QRhiImplementation *rhi)
    : QRhiShaderResourceBindings(rhi)
{
}

QD3D11ShaderResourceBindings::~QD3D11ShaderResourceBindings()
{
    destroy();
}

void QD3D11ShaderResourceBindings::destroy()
{
    sortedBindings.clear();
    boundResourceData.clear();

    QRHI_RES_RHI(QRhiD3D11);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QD3D11ShaderResourceBindings::create()
{
    if (!sortedBindings.isEmpty())
        destroy();

    QRHI_RES_RHI(QRhiD3D11);
    if (!rhiD->sanityCheckShaderResourceBindings(this))
        return false;

    rhiD->updateLayoutDesc(this);

    std::copy(m_bindings.cbegin(), m_bindings.cend(), std::back_inserter(sortedBindings));
    std::sort(sortedBindings.begin(), sortedBindings.end(), QRhiImplementation::sortedBindingLessThan);

    boundResourceData.resize(sortedBindings.count());

    for (BoundResourceData &bd : boundResourceData)
        memset(&bd, 0, sizeof(BoundResourceData));

    hasDynamicOffset = false;
    for (const QRhiShaderResourceBinding &b : sortedBindings) {
        const QRhiShaderResourceBinding::Data *bd = QRhiImplementation::shaderResourceBindingData(b);
        if (bd->type == QRhiShaderResourceBinding::UniformBuffer && bd->u.ubuf.hasDynamicOffset) {
            hasDynamicOffset = true;
            break;
        }
    }

    generation += 1;
    rhiD->registerResource(this, false);
    return true;
}

void QD3D11ShaderResourceBindings::updateResources(UpdateFlags flags)
{
    sortedBindings.clear();
    std::copy(m_bindings.cbegin(), m_bindings.cend(), std::back_inserter(sortedBindings));
    if (!flags.testFlag(BindingsAreSorted))
        std::sort(sortedBindings.begin(), sortedBindings.end(), QRhiImplementation::sortedBindingLessThan);

    Q_ASSERT(boundResourceData.count() == sortedBindings.count());
    for (BoundResourceData &bd : boundResourceData)
        memset(&bd, 0, sizeof(BoundResourceData));

    generation += 1;
}

QD3D11GraphicsPipeline::QD3D11GraphicsPipeline(QRhiImplementation *rhi)
    : QRhiGraphicsPipeline(rhi)
{
}

QD3D11GraphicsPipeline::~QD3D11GraphicsPipeline()
{
    destroy();
}

template<typename T>
inline void releasePipelineShader(T &s)
{
    if (s.shader) {
        s.shader->Release();
        s.shader = nullptr;
    }
    s.nativeResourceBindingMap.clear();
}

void QD3D11GraphicsPipeline::destroy()
{
    if (!dsState)
        return;

    dsState->Release();
    dsState = nullptr;

    if (blendState) {
        blendState->Release();
        blendState = nullptr;
    }

    if (inputLayout) {
        inputLayout->Release();
        inputLayout = nullptr;
    }

    if (rastState) {
        rastState->Release();
        rastState = nullptr;
    }

    releasePipelineShader(vs);
    releasePipelineShader(hs);
    releasePipelineShader(ds);
    releasePipelineShader(gs);
    releasePipelineShader(fs);

    QRHI_RES_RHI(QRhiD3D11);
    if (rhiD)
        rhiD->unregisterResource(this);
}

static inline D3D11_CULL_MODE toD3DCullMode(QRhiGraphicsPipeline::CullMode c)
{
    switch (c) {
    case QRhiGraphicsPipeline::None:
        return D3D11_CULL_NONE;
    case QRhiGraphicsPipeline::Front:
        return D3D11_CULL_FRONT;
    case QRhiGraphicsPipeline::Back:
        return D3D11_CULL_BACK;
    default:
        Q_UNREACHABLE();
        return D3D11_CULL_NONE;
    }
}

static inline D3D11_FILL_MODE toD3DFillMode(QRhiGraphicsPipeline::PolygonMode mode)
{
    switch (mode) {
    case QRhiGraphicsPipeline::Fill:
        return D3D11_FILL_SOLID;
    case QRhiGraphicsPipeline::Line:
        return D3D11_FILL_WIREFRAME;
    default:
        Q_UNREACHABLE();
        return D3D11_FILL_SOLID;
    }
}

static inline D3D11_COMPARISON_FUNC toD3DCompareOp(QRhiGraphicsPipeline::CompareOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::Never:
        return D3D11_COMPARISON_NEVER;
    case QRhiGraphicsPipeline::Less:
        return D3D11_COMPARISON_LESS;
    case QRhiGraphicsPipeline::Equal:
        return D3D11_COMPARISON_EQUAL;
    case QRhiGraphicsPipeline::LessOrEqual:
        return D3D11_COMPARISON_LESS_EQUAL;
    case QRhiGraphicsPipeline::Greater:
        return D3D11_COMPARISON_GREATER;
    case QRhiGraphicsPipeline::NotEqual:
        return D3D11_COMPARISON_NOT_EQUAL;
    case QRhiGraphicsPipeline::GreaterOrEqual:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case QRhiGraphicsPipeline::Always:
        return D3D11_COMPARISON_ALWAYS;
    default:
        Q_UNREACHABLE();
        return D3D11_COMPARISON_ALWAYS;
    }
}

static inline D3D11_STENCIL_OP toD3DStencilOp(QRhiGraphicsPipeline::StencilOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::StencilZero:
        return D3D11_STENCIL_OP_ZERO;
    case QRhiGraphicsPipeline::Keep:
        return D3D11_STENCIL_OP_KEEP;
    case QRhiGraphicsPipeline::Replace:
        return D3D11_STENCIL_OP_REPLACE;
    case QRhiGraphicsPipeline::IncrementAndClamp:
        return D3D11_STENCIL_OP_INCR_SAT;
    case QRhiGraphicsPipeline::DecrementAndClamp:
        return D3D11_STENCIL_OP_DECR_SAT;
    case QRhiGraphicsPipeline::Invert:
        return D3D11_STENCIL_OP_INVERT;
    case QRhiGraphicsPipeline::IncrementAndWrap:
        return D3D11_STENCIL_OP_INCR;
    case QRhiGraphicsPipeline::DecrementAndWrap:
        return D3D11_STENCIL_OP_DECR;
    default:
        Q_UNREACHABLE();
        return D3D11_STENCIL_OP_KEEP;
    }
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
    default:
        Q_UNREACHABLE();
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }
}

static inline D3D11_PRIMITIVE_TOPOLOGY toD3DTopology(QRhiGraphicsPipeline::Topology t, int patchControlPointCount)
{
    switch (t) {
    case QRhiGraphicsPipeline::Triangles:
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case QRhiGraphicsPipeline::TriangleStrip:
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case QRhiGraphicsPipeline::Lines:
        return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    case QRhiGraphicsPipeline::LineStrip:
        return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case QRhiGraphicsPipeline::Points:
        return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    case QRhiGraphicsPipeline::Patches:
        Q_ASSERT(patchControlPointCount >= 1 && patchControlPointCount <= 32);
        return D3D11_PRIMITIVE_TOPOLOGY(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (patchControlPointCount - 1));
    default:
        Q_UNREACHABLE();
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

static inline UINT8 toD3DColorWriteMask(QRhiGraphicsPipeline::ColorMask c)
{
    UINT8 f = 0;
    if (c.testFlag(QRhiGraphicsPipeline::R))
        f |= D3D11_COLOR_WRITE_ENABLE_RED;
    if (c.testFlag(QRhiGraphicsPipeline::G))
        f |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    if (c.testFlag(QRhiGraphicsPipeline::B))
        f |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    if (c.testFlag(QRhiGraphicsPipeline::A))
        f |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    return f;
}

static inline D3D11_BLEND toD3DBlendFactor(QRhiGraphicsPipeline::BlendFactor f, bool rgb)
{
    // SrcBlendAlpha and DstBlendAlpha do not accept *_COLOR. With other APIs
    // this is handled internally (so that e.g. VK_BLEND_FACTOR_SRC_COLOR is
    // accepted and is in effect equivalent to VK_BLEND_FACTOR_SRC_ALPHA when
    // set as an alpha src/dest factor), but for D3D we have to take care of it
    // ourselves. Hence the rgb argument.

    switch (f) {
    case QRhiGraphicsPipeline::Zero:
        return D3D11_BLEND_ZERO;
    case QRhiGraphicsPipeline::One:
        return D3D11_BLEND_ONE;
    case QRhiGraphicsPipeline::SrcColor:
        return rgb ? D3D11_BLEND_SRC_COLOR : D3D11_BLEND_SRC_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrcColor:
        return rgb ? D3D11_BLEND_INV_SRC_COLOR : D3D11_BLEND_INV_SRC_ALPHA;
    case QRhiGraphicsPipeline::DstColor:
        return rgb ? D3D11_BLEND_DEST_COLOR : D3D11_BLEND_DEST_ALPHA;
    case QRhiGraphicsPipeline::OneMinusDstColor:
        return rgb ? D3D11_BLEND_INV_DEST_COLOR : D3D11_BLEND_INV_DEST_ALPHA;
    case QRhiGraphicsPipeline::SrcAlpha:
        return D3D11_BLEND_SRC_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrcAlpha:
        return D3D11_BLEND_INV_SRC_ALPHA;
    case QRhiGraphicsPipeline::DstAlpha:
        return D3D11_BLEND_DEST_ALPHA;
    case QRhiGraphicsPipeline::OneMinusDstAlpha:
        return D3D11_BLEND_INV_DEST_ALPHA;
    case QRhiGraphicsPipeline::ConstantColor:
    case QRhiGraphicsPipeline::ConstantAlpha:
        return D3D11_BLEND_BLEND_FACTOR;
    case QRhiGraphicsPipeline::OneMinusConstantColor:
    case QRhiGraphicsPipeline::OneMinusConstantAlpha:
        return D3D11_BLEND_INV_BLEND_FACTOR;
    case QRhiGraphicsPipeline::SrcAlphaSaturate:
        return D3D11_BLEND_SRC_ALPHA_SAT;
    case QRhiGraphicsPipeline::Src1Color:
        return rgb ? D3D11_BLEND_SRC1_COLOR : D3D11_BLEND_SRC1_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrc1Color:
        return rgb ? D3D11_BLEND_INV_SRC1_COLOR : D3D11_BLEND_INV_SRC1_ALPHA;
    case QRhiGraphicsPipeline::Src1Alpha:
        return D3D11_BLEND_SRC1_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrc1Alpha:
        return D3D11_BLEND_INV_SRC1_ALPHA;
    default:
        Q_UNREACHABLE();
        return D3D11_BLEND_ZERO;
    }
}

static inline D3D11_BLEND_OP toD3DBlendOp(QRhiGraphicsPipeline::BlendOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::Add:
        return D3D11_BLEND_OP_ADD;
    case QRhiGraphicsPipeline::Subtract:
        return D3D11_BLEND_OP_SUBTRACT;
    case QRhiGraphicsPipeline::ReverseSubtract:
        return D3D11_BLEND_OP_REV_SUBTRACT;
    case QRhiGraphicsPipeline::Min:
        return D3D11_BLEND_OP_MIN;
    case QRhiGraphicsPipeline::Max:
        return D3D11_BLEND_OP_MAX;
    default:
        Q_UNREACHABLE();
        return D3D11_BLEND_OP_ADD;
    }
}

static inline QByteArray sourceHash(const QByteArray &source)
{
    // taken from the GL backend, use the same mechanism to get a key
    QCryptographicHash keyBuilder(QCryptographicHash::Sha1);
    keyBuilder.addData(source);
    return keyBuilder.result().toHex();
}

QByteArray QRhiD3D11::compileHlslShaderSource(const QShader &shader, QShader::Variant shaderVariant, uint flags,
                                              QString *error, QShaderKey *usedShaderKey)
{
    QShaderKey key = { QShader::DxbcShader, 50, shaderVariant };
    QShaderCode dxbc = shader.shader(key);
    if (!dxbc.shader().isEmpty()) {
        if (usedShaderKey)
            *usedShaderKey = key;
        return dxbc.shader();
    }

    key = { QShader::HlslShader, 50, shaderVariant };
    QShaderCode hlslSource = shader.shader(key);
    if (hlslSource.shader().isEmpty()) {
        qWarning() << "No HLSL (shader model 5.0) code found in baked shader" << shader;
        return QByteArray();
    }

    if (usedShaderKey)
        *usedShaderKey = key;

    const char *target;
    switch (shader.stage()) {
    case QShader::VertexStage:
        target = "vs_5_0";
        break;
    case QShader::TessellationControlStage:
        target = "hs_5_0";
        break;
    case QShader::TessellationEvaluationStage:
        target = "ds_5_0";
        break;
    case QShader::GeometryStage:
        target = "gs_5_0";
        break;
    case QShader::FragmentStage:
        target = "ps_5_0";
        break;
    case QShader::ComputeStage:
        target = "cs_5_0";
        break;
    default:
        Q_UNREACHABLE();
        return QByteArray();
    }

    BytecodeCacheKey cacheKey;
    if (rhiFlags.testFlag(QRhi::EnablePipelineCacheDataSave)) {
        cacheKey.sourceHash = sourceHash(hlslSource.shader());
        cacheKey.target = target;
        cacheKey.entryPoint = hlslSource.entryPoint();
        cacheKey.compileFlags = flags;
        auto cacheIt = m_bytecodeCache.constFind(cacheKey);
        if (cacheIt != m_bytecodeCache.constEnd())
            return cacheIt.value();
    }

    static const pD3DCompile d3dCompile = QRhiD3D::resolveD3DCompile();
    if (d3dCompile == nullptr) {
        qWarning("Unable to resolve function D3DCompile()");
        return QByteArray();
    }

    ID3DBlob *bytecode = nullptr;
    ID3DBlob *errors = nullptr;
    HRESULT hr = d3dCompile(hlslSource.shader().constData(), SIZE_T(hlslSource.shader().size()),
                            nullptr, nullptr, nullptr,
                            hlslSource.entryPoint().constData(), target, flags, 0, &bytecode, &errors);
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

    if (rhiFlags.testFlag(QRhi::EnablePipelineCacheDataSave))
        m_bytecodeCache.insert(cacheKey, result);

    return result;
}

bool QD3D11GraphicsPipeline::create()
{
    if (dsState)
        destroy();

    QRHI_RES_RHI(QRhiD3D11);
    rhiD->pipelineCreationStart();
    if (!rhiD->sanityCheckGraphicsPipeline(this))
        return false;

    D3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.FillMode = toD3DFillMode(m_polygonMode);
    rastDesc.CullMode = toD3DCullMode(m_cullMode);
    rastDesc.FrontCounterClockwise = m_frontFace == CCW;
    rastDesc.DepthBias = m_depthBias;
    rastDesc.SlopeScaledDepthBias = m_slopeScaledDepthBias;
    rastDesc.DepthClipEnable = true;
    rastDesc.ScissorEnable = m_flags.testFlag(UsesScissor);
    rastDesc.MultisampleEnable = rhiD->effectiveSampleCount(m_sampleCount).Count > 1;
    HRESULT hr = rhiD->dev->CreateRasterizerState(&rastDesc, &rastState);
    if (FAILED(hr)) {
        qWarning("Failed to create rasterizer state: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = m_depthTest;
    dsDesc.DepthWriteMask = m_depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = toD3DCompareOp(m_depthOp);
    dsDesc.StencilEnable = m_stencilTest;
    if (m_stencilTest) {
        dsDesc.StencilReadMask = UINT8(m_stencilReadMask);
        dsDesc.StencilWriteMask = UINT8(m_stencilWriteMask);
        dsDesc.FrontFace.StencilFailOp = toD3DStencilOp(m_stencilFront.failOp);
        dsDesc.FrontFace.StencilDepthFailOp = toD3DStencilOp(m_stencilFront.depthFailOp);
        dsDesc.FrontFace.StencilPassOp = toD3DStencilOp(m_stencilFront.passOp);
        dsDesc.FrontFace.StencilFunc = toD3DCompareOp(m_stencilFront.compareOp);
        dsDesc.BackFace.StencilFailOp = toD3DStencilOp(m_stencilBack.failOp);
        dsDesc.BackFace.StencilDepthFailOp = toD3DStencilOp(m_stencilBack.depthFailOp);
        dsDesc.BackFace.StencilPassOp = toD3DStencilOp(m_stencilBack.passOp);
        dsDesc.BackFace.StencilFunc = toD3DCompareOp(m_stencilBack.compareOp);
    }
    hr = rhiD->dev->CreateDepthStencilState(&dsDesc, &dsState);
    if (FAILED(hr)) {
        qWarning("Failed to create depth-stencil state: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.IndependentBlendEnable = m_targetBlends.count() > 1;
    for (int i = 0, ie = m_targetBlends.count(); i != ie; ++i) {
        const QRhiGraphicsPipeline::TargetBlend &b(m_targetBlends[i]);
        D3D11_RENDER_TARGET_BLEND_DESC blend = {};
        blend.BlendEnable = b.enable;
        blend.SrcBlend = toD3DBlendFactor(b.srcColor, true);
        blend.DestBlend = toD3DBlendFactor(b.dstColor, true);
        blend.BlendOp = toD3DBlendOp(b.opColor);
        blend.SrcBlendAlpha = toD3DBlendFactor(b.srcAlpha, false);
        blend.DestBlendAlpha = toD3DBlendFactor(b.dstAlpha, false);
        blend.BlendOpAlpha = toD3DBlendOp(b.opAlpha);
        blend.RenderTargetWriteMask = toD3DColorWriteMask(b.colorWrite);
        blendDesc.RenderTarget[i] = blend;
    }
    if (m_targetBlends.isEmpty()) {
        D3D11_RENDER_TARGET_BLEND_DESC blend = {};
        blend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[0] = blend;
    }
    hr = rhiD->dev->CreateBlendState(&blendDesc, &blendState);
    if (FAILED(hr)) {
        qWarning("Failed to create blend state: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }

    QByteArray vsByteCode;
    for (const QRhiShaderStage &shaderStage : std::as_const(m_shaderStages)) {
        auto cacheIt = rhiD->m_shaderCache.constFind(shaderStage);
        if (cacheIt != rhiD->m_shaderCache.constEnd()) {
            switch (shaderStage.type()) {
            case QRhiShaderStage::Vertex:
                vs.shader = static_cast<ID3D11VertexShader *>(cacheIt->s);
                vs.shader->AddRef();
                vsByteCode = cacheIt->bytecode;
                vs.nativeResourceBindingMap = cacheIt->nativeResourceBindingMap;
                break;
            case QRhiShaderStage::TessellationControl:
                hs.shader = static_cast<ID3D11HullShader *>(cacheIt->s);
                hs.shader->AddRef();
                hs.nativeResourceBindingMap = cacheIt->nativeResourceBindingMap;
                break;
            case QRhiShaderStage::TessellationEvaluation:
                ds.shader = static_cast<ID3D11DomainShader *>(cacheIt->s);
                ds.shader->AddRef();
                ds.nativeResourceBindingMap = cacheIt->nativeResourceBindingMap;
                break;
            case QRhiShaderStage::Geometry:
                gs.shader = static_cast<ID3D11GeometryShader *>(cacheIt->s);
                gs.shader->AddRef();
                gs.nativeResourceBindingMap = cacheIt->nativeResourceBindingMap;
                break;
            case QRhiShaderStage::Fragment:
                fs.shader = static_cast<ID3D11PixelShader *>(cacheIt->s);
                fs.shader->AddRef();
                fs.nativeResourceBindingMap = cacheIt->nativeResourceBindingMap;
                break;
            default:
                break;
            }
        } else {
            QString error;
            QShaderKey shaderKey;
            UINT compileFlags = 0;
            if (m_flags.testFlag(CompileShadersWithDebugInfo))
                compileFlags |= D3DCOMPILE_DEBUG;

            const QByteArray bytecode = rhiD->compileHlslShaderSource(shaderStage.shader(), shaderStage.shaderVariant(), compileFlags,
                                                                      &error, &shaderKey);
            if (bytecode.isEmpty()) {
                qWarning("HLSL shader compilation failed: %s", qPrintable(error));
                return false;
            }

            if (rhiD->m_shaderCache.count() >= QRhiD3D11::MAX_SHADER_CACHE_ENTRIES) {
                // Use the simplest strategy: too many cached shaders -> drop them all.
                rhiD->clearShaderCache();
            }

            switch (shaderStage.type()) {
            case QRhiShaderStage::Vertex:
                hr = rhiD->dev->CreateVertexShader(bytecode.constData(), SIZE_T(bytecode.size()), nullptr, &vs.shader);
                if (FAILED(hr)) {
                    qWarning("Failed to create vertex shader: %s",
                        qPrintable(QSystemError::windowsComString(hr)));
                    return false;
                }
                vsByteCode = bytecode;
                vs.nativeResourceBindingMap = shaderStage.shader().nativeResourceBindingMap(shaderKey);
                rhiD->m_shaderCache.insert(shaderStage, QRhiD3D11::Shader(vs.shader, bytecode, vs.nativeResourceBindingMap));
                vs.shader->AddRef();
                break;
            case QRhiShaderStage::TessellationControl:
                hr = rhiD->dev->CreateHullShader(bytecode.constData(), SIZE_T(bytecode.size()), nullptr, &hs.shader);
                if (FAILED(hr)) {
                    qWarning("Failed to create hull shader: %s",
                        qPrintable(QSystemError::windowsComString(hr)));
                    return false;
                }
                hs.nativeResourceBindingMap = shaderStage.shader().nativeResourceBindingMap(shaderKey);
                rhiD->m_shaderCache.insert(shaderStage, QRhiD3D11::Shader(hs.shader, bytecode, hs.nativeResourceBindingMap));
                hs.shader->AddRef();
                break;
            case QRhiShaderStage::TessellationEvaluation:
                hr = rhiD->dev->CreateDomainShader(bytecode.constData(), SIZE_T(bytecode.size()), nullptr, &ds.shader);
                if (FAILED(hr)) {
                    qWarning("Failed to create domain shader: %s",
                        qPrintable(QSystemError::windowsComString(hr)));
                    return false;
                }
                ds.nativeResourceBindingMap = shaderStage.shader().nativeResourceBindingMap(shaderKey);
                rhiD->m_shaderCache.insert(shaderStage, QRhiD3D11::Shader(ds.shader, bytecode, ds.nativeResourceBindingMap));
                ds.shader->AddRef();
                break;
            case QRhiShaderStage::Geometry:
                hr = rhiD->dev->CreateGeometryShader(bytecode.constData(), SIZE_T(bytecode.size()), nullptr, &gs.shader);
                if (FAILED(hr)) {
                    qWarning("Failed to create geometry shader: %s",
                        qPrintable(QSystemError::windowsComString(hr)));
                    return false;
                }
                gs.nativeResourceBindingMap = shaderStage.shader().nativeResourceBindingMap(shaderKey);
                rhiD->m_shaderCache.insert(shaderStage, QRhiD3D11::Shader(gs.shader, bytecode, gs.nativeResourceBindingMap));
                gs.shader->AddRef();
                break;
            case QRhiShaderStage::Fragment:
                hr = rhiD->dev->CreatePixelShader(bytecode.constData(), SIZE_T(bytecode.size()), nullptr, &fs.shader);
                if (FAILED(hr)) {
                    qWarning("Failed to create pixel shader: %s",
                        qPrintable(QSystemError::windowsComString(hr)));
                    return false;
                }
                fs.nativeResourceBindingMap = shaderStage.shader().nativeResourceBindingMap(shaderKey);
                rhiD->m_shaderCache.insert(shaderStage, QRhiD3D11::Shader(fs.shader, bytecode, fs.nativeResourceBindingMap));
                fs.shader->AddRef();
                break;
            default:
                break;
            }
        }
    }

    d3dTopology = toD3DTopology(m_topology, m_patchControlPointCount);

    if (!vsByteCode.isEmpty()) {
        QByteArrayList matrixSliceSemantics;
        QVarLengthArray<D3D11_INPUT_ELEMENT_DESC, 4> inputDescs;
        for (auto it = m_vertexInputLayout.cbeginAttributes(), itEnd = m_vertexInputLayout.cendAttributes();
             it != itEnd; ++it)
        {
            D3D11_INPUT_ELEMENT_DESC desc = {};
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
                qsnprintf(sem.data(), sem.size(), "TEXCOORD%d_", it->location() - matrixSlice);
                matrixSliceSemantics.append(sem);
                desc.SemanticName = matrixSliceSemantics.last().constData();
                desc.SemanticIndex = UINT(matrixSlice);
            }
            desc.Format = toD3DAttributeFormat(it->format());
            desc.InputSlot = UINT(it->binding());
            desc.AlignedByteOffset = it->offset();
            const QRhiVertexInputBinding *inputBinding = m_vertexInputLayout.bindingAt(it->binding());
            if (inputBinding->classification() == QRhiVertexInputBinding::PerInstance) {
                desc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                desc.InstanceDataStepRate = inputBinding->instanceStepRate();
            } else {
                desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            }
            inputDescs.append(desc);
        }
        if (!inputDescs.isEmpty()) {
            hr = rhiD->dev->CreateInputLayout(inputDescs.constData(), UINT(inputDescs.count()),
                                              vsByteCode, SIZE_T(vsByteCode.size()), &inputLayout);
            if (FAILED(hr)) {
                qWarning("Failed to create input layout: %s",
                    qPrintable(QSystemError::windowsComString(hr)));
                return false;
            }
        } // else leave inputLayout set to nullptr; that's valid and it avoids a debug layer warning about an input layout with 0 elements
    }

    rhiD->pipelineCreationEnd();
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QD3D11ComputePipeline::QD3D11ComputePipeline(QRhiImplementation *rhi)
    : QRhiComputePipeline(rhi)
{
}

QD3D11ComputePipeline::~QD3D11ComputePipeline()
{
    destroy();
}

void QD3D11ComputePipeline::destroy()
{
    if (!cs.shader)
        return;

    cs.shader->Release();
    cs.shader = nullptr;
    cs.nativeResourceBindingMap.clear();

    QRHI_RES_RHI(QRhiD3D11);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QD3D11ComputePipeline::create()
{
    if (cs.shader)
        destroy();

    QRHI_RES_RHI(QRhiD3D11);
    rhiD->pipelineCreationStart();

    auto cacheIt = rhiD->m_shaderCache.constFind(m_shaderStage);
    if (cacheIt != rhiD->m_shaderCache.constEnd()) {
        cs.shader = static_cast<ID3D11ComputeShader *>(cacheIt->s);
        cs.nativeResourceBindingMap = cacheIt->nativeResourceBindingMap;
    } else {
        QString error;
        QShaderKey shaderKey;
        UINT compileFlags = 0;
        if (m_flags.testFlag(CompileShadersWithDebugInfo))
            compileFlags |= D3DCOMPILE_DEBUG;

        const QByteArray bytecode = rhiD->compileHlslShaderSource(m_shaderStage.shader(), m_shaderStage.shaderVariant(), compileFlags,
                                                                  &error, &shaderKey);
        if (bytecode.isEmpty()) {
            qWarning("HLSL compute shader compilation failed: %s", qPrintable(error));
            return false;
        }

        HRESULT hr = rhiD->dev->CreateComputeShader(bytecode.constData(), SIZE_T(bytecode.size()), nullptr, &cs.shader);
        if (FAILED(hr)) {
            qWarning("Failed to create compute shader: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }

        cs.nativeResourceBindingMap = m_shaderStage.shader().nativeResourceBindingMap(shaderKey);

        if (rhiD->m_shaderCache.count() >= QRhiD3D11::MAX_SHADER_CACHE_ENTRIES)
            rhiD->clearShaderCache();

        rhiD->m_shaderCache.insert(m_shaderStage, QRhiD3D11::Shader(cs.shader, bytecode, cs.nativeResourceBindingMap));
    }

    cs.shader->AddRef();

    rhiD->pipelineCreationEnd();
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QD3D11CommandBuffer::QD3D11CommandBuffer(QRhiImplementation *rhi)
    : QRhiCommandBuffer(rhi)
{
    resetState();
}

QD3D11CommandBuffer::~QD3D11CommandBuffer()
{
    destroy();
}

void QD3D11CommandBuffer::destroy()
{
    // nothing to do here
}

bool QD3D11Timestamps::prepare(int pairCount, QRhiD3D11 *rhiD)
{
    // Creates the query objects if not yet done, but otherwise calling this
    // function is expected to be a no-op.

    Q_ASSERT(pairCount <= MAX_TIMESTAMP_PAIRS);
    D3D11_QUERY_DESC queryDesc = {};
    for (int i = 0; i < pairCount; ++i) {
        if (!disjointQuery[i]) {
            queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
            HRESULT hr = rhiD->dev->CreateQuery(&queryDesc, &disjointQuery[i]);
            if (FAILED(hr)) {
                qWarning("Failed to create timestamp disjoint query: %s",
                         qPrintable(QSystemError::windowsComString(hr)));
                return false;
            }
        }
        queryDesc.Query = D3D11_QUERY_TIMESTAMP;
        for (int j = 0; j < 2; ++j) {
            const int idx = pairCount * i + j;
            if (!query[idx]) {
                HRESULT hr = rhiD->dev->CreateQuery(&queryDesc, &query[idx]);
                if (FAILED(hr)) {
                    qWarning("Failed to create timestamp query: %s",
                             qPrintable(QSystemError::windowsComString(hr)));
                    return false;
                }
            }
        }
    }
    this->pairCount = pairCount;
    return true;
}

void QD3D11Timestamps::destroy()
{
    for (int i = 0; i < MAX_TIMESTAMP_PAIRS; ++i) {
        active[i] = false;
        if (disjointQuery[i]) {
            disjointQuery[i]->Release();
            disjointQuery[i] = nullptr;
        }
        for (int j = 0; j < 2; ++j) {
            const int idx = MAX_TIMESTAMP_PAIRS * i + j;
            if (query[idx]) {
                query[idx]->Release();
                query[idx] = nullptr;
            }
        }
    }
}

bool QD3D11Timestamps::tryQueryTimestamps(int idx, ID3D11DeviceContext *context, double *elapsedSec)
{
    bool result = false;
    if (!active[idx])
        return result;

    ID3D11Query *tsDisjoint = disjointQuery[idx];
    const int tsIdx = pairCount * idx;
    ID3D11Query *tsStart = query[tsIdx];
    ID3D11Query *tsEnd = query[tsIdx + 1];
    quint64 timestamps[2];
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT dj;

    bool ok = true;
    ok &= context->GetData(tsDisjoint, &dj, sizeof(dj), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK;
    ok &= context->GetData(tsEnd, &timestamps[1], sizeof(quint64), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK;
    // this above is often not ready, not even in frame_where_recorded+2,
    // not clear why. so make the whole thing async and do not touch the
    // queries until they are finally all available in frame this+2 or
    // this+4 or ...
    ok &= context->GetData(tsStart, &timestamps[0], sizeof(quint64), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK;

    if (ok) {
        if (!dj.Disjoint && dj.Frequency) {
            const float elapsedMs = (timestamps[1] - timestamps[0]) / float(dj.Frequency) * 1000.0f;
            *elapsedSec = elapsedMs / 1000.0;
            result = true;
        }
        active[idx] = false;
    } // else leave active set, will retry in a subsequent beginFrame or similar

    return result;
}

QD3D11SwapChain::QD3D11SwapChain(QRhiImplementation *rhi)
    : QRhiSwapChain(rhi),
      rt(rhi, this),
      cb(rhi)
{
    backBufferTex = nullptr;
    backBufferRtv = nullptr;
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        msaaTex[i] = nullptr;
        msaaRtv[i] = nullptr;
    }
}

QD3D11SwapChain::~QD3D11SwapChain()
{
    destroy();
}

void QD3D11SwapChain::releaseBuffers()
{
    if (backBufferRtv) {
        backBufferRtv->Release();
        backBufferRtv = nullptr;
    }
    if (backBufferTex) {
        backBufferTex->Release();
        backBufferTex = nullptr;
    }
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        if (msaaRtv[i]) {
            msaaRtv[i]->Release();
            msaaRtv[i] = nullptr;
        }
        if (msaaTex[i]) {
            msaaTex[i]->Release();
            msaaTex[i] = nullptr;
        }
    }
}

void QD3D11SwapChain::destroy()
{
    if (!swapChain)
        return;

    releaseBuffers();

    timestamps.destroy();

    swapChain->Release();
    swapChain = nullptr;

    if (dcompVisual) {
        dcompVisual->Release();
        dcompVisual = nullptr;
    }

    if (dcompTarget) {
        dcompTarget->Release();
        dcompTarget = nullptr;
    }

    QRHI_RES_RHI(QRhiD3D11);
    if (rhiD)
        rhiD->unregisterResource(this);
}

QRhiCommandBuffer *QD3D11SwapChain::currentFrameCommandBuffer()
{
    return &cb;
}

QRhiRenderTarget *QD3D11SwapChain::currentFrameRenderTarget()
{
    return &rt;
}

QSize QD3D11SwapChain::surfacePixelSize()
{
    Q_ASSERT(m_window);
    return m_window->size() * m_window->devicePixelRatio();
}

static bool output6ForWindow(QWindow *w, IDXGIAdapter1 *adapter, IDXGIOutput6 **result)
{
    bool ok = false;
    QRect wr = w->geometry();
    wr = QRect(wr.topLeft() * w->devicePixelRatio(), wr.size() * w->devicePixelRatio());
    const QPoint center = wr.center();
    IDXGIOutput *currentOutput = nullptr;
    IDXGIOutput *output = nullptr;
    for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i) {
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

static bool outputDesc1ForWindow(QWindow *w, IDXGIAdapter1 *adapter, DXGI_OUTPUT_DESC1 *result)
{
    bool ok = false;
    IDXGIOutput6 *out6 = nullptr;
    if (output6ForWindow(w, adapter, &out6)) {
        ok = SUCCEEDED(out6->GetDesc1(result));
        out6->Release();
    }
    return ok;
}

bool QD3D11SwapChain::isFormatSupported(Format f)
{
    if (f == SDR)
        return true;

    if (!m_window) {
        qWarning("Attempted to call isFormatSupported() without a window set");
        return false;
    }

    QRHI_RES_RHI(QRhiD3D11);
    DXGI_OUTPUT_DESC1 desc1;
    if (outputDesc1ForWindow(m_window, rhiD->activeAdapter, &desc1)) {
        if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
            return f == QRhiSwapChain::HDRExtendedSrgbLinear || f == QRhiSwapChain::HDR10;
    }

    return false;
}

QRhiSwapChainHdrInfo QD3D11SwapChain::hdrInfo()
{
    QRhiSwapChainHdrInfo info = QRhiSwapChain::hdrInfo();
    if (m_window) {
        QRHI_RES_RHI(QRhiD3D11);
        DXGI_OUTPUT_DESC1 hdrOutputDesc;
        if (outputDesc1ForWindow(m_window, rhiD->activeAdapter, &hdrOutputDesc)) {
            info.isHardCodedDefaults = false;
            info.limitsType = QRhiSwapChainHdrInfo::LuminanceInNits;
            info.limits.luminanceInNits.minLuminance = hdrOutputDesc.MinLuminance;
            info.limits.luminanceInNits.maxLuminance = hdrOutputDesc.MaxLuminance;
        }
    }
    return info;
}

QRhiRenderPassDescriptor *QD3D11SwapChain::newCompatibleRenderPassDescriptor()
{
    QD3D11RenderPassDescriptor *rpD = new QD3D11RenderPassDescriptor(m_rhi);
    QRHI_RES_RHI(QRhiD3D11);
    rhiD->registerResource(rpD, false);
    return rpD;
}

bool QD3D11SwapChain::newColorBuffer(const QSize &size, DXGI_FORMAT format, DXGI_SAMPLE_DESC sampleDesc,
                                     ID3D11Texture2D **tex, ID3D11RenderTargetView **rtv) const
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = UINT(size.width());
    desc.Height = UINT(size.height());
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc = sampleDesc;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    QRHI_RES_RHI(QRhiD3D11);
    HRESULT hr = rhiD->dev->CreateTexture2D(&desc, nullptr, tex);
    if (FAILED(hr)) {
        qWarning("Failed to create color buffer texture: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = sampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = rhiD->dev->CreateRenderTargetView(*tex, &rtvDesc, rtv);
    if (FAILED(hr)) {
        qWarning("Failed to create color buffer rtv: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        (*tex)->Release();
        *tex = nullptr;
        return false;
    }

    return true;
}

bool QRhiD3D11::ensureDirectCompositionDevice()
{
    if (dcompDevice)
        return true;

    qCDebug(QRHI_LOG_INFO, "Creating Direct Composition device (needed for semi-transparent windows)");
    dcompDevice = QRhiD3D::createDirectCompositionDevice();
    return dcompDevice ? true : false;
}

static const DXGI_FORMAT DEFAULT_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
static const DXGI_FORMAT DEFAULT_SRGB_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

bool QD3D11SwapChain::createOrResize()
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

    QRHI_RES_RHI(QRhiD3D11);

    if (m_flags.testFlag(SurfaceHasPreMulAlpha) || m_flags.testFlag(SurfaceHasNonPreMulAlpha)) {
        if (!rhiD->useLegacySwapchainModel && rhiD->ensureDirectCompositionDevice()) {
            if (!dcompTarget) {
                hr = rhiD->dcompDevice->CreateTargetForHwnd(hwnd, true, &dcompTarget);
                if (FAILED(hr)) {
                    qWarning("Failed to create Direct Compsition target for the window: %s",
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

    // A non-flip swapchain can do Present(0) as expected without
    // ALLOW_TEARING, and ALLOW_TEARING is not compatible with it at all so the
    // flag must not be set then. Whereas for flip we should use it, if
    // supported, to get better results for 'unthrottled' presentation.
    if (swapInterval == 0 && rhiD->supportsAllowTearing)
        swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    if (!swapChain) {
        sampleDesc = rhiD->effectiveSampleCount(m_sampleCount);
        colorFormat = DEFAULT_FORMAT;
        srgbAdjustedColorFormat = m_flags.testFlag(sRGB) ? DEFAULT_SRGB_FORMAT : DEFAULT_FORMAT;

        DXGI_COLOR_SPACE_TYPE hdrColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709; // SDR
        DXGI_OUTPUT_DESC1 hdrOutputDesc;
        if (outputDesc1ForWindow(m_window, rhiD->activeAdapter, &hdrOutputDesc) && m_format != SDR) {
            // https://docs.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
            if (hdrOutputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
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

        // We use a FLIP model swapchain which implies a buffer count of 2
        // (as opposed to the old DISCARD with back buffer count == 1).
        // This makes no difference for the rest of the stuff except that
        // automatic MSAA is unsupported and needs to be implemented via a
        // custom multisample render target and an explicit resolve.

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = UINT(pixelSize.width());
        desc.Height = UINT(pixelSize.height());
        desc.Format = colorFormat;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = BUFFER_COUNT;
        desc.Flags = swapChainFlags;
        desc.Scaling = rhiD->useLegacySwapchainModel ? DXGI_SCALING_STRETCH : DXGI_SCALING_NONE;
        desc.SwapEffect = rhiD->useLegacySwapchainModel ? DXGI_SWAP_EFFECT_DISCARD : DXGI_SWAP_EFFECT_FLIP_DISCARD;

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

        IDXGIFactory2 *fac = static_cast<IDXGIFactory2 *>(rhiD->dxgiFactory);
        IDXGISwapChain1 *sc1;

        if (dcompVisual)
            hr = fac->CreateSwapChainForComposition(rhiD->dev, &desc, nullptr, &sc1);
        else
            hr = fac->CreateSwapChainForHwnd(rhiD->dev, hwnd, &desc, nullptr, nullptr, &sc1);

        // If failed and we tried a HDR format, then try with SDR. This
        // matches other backends, such as Vulkan where if the format is
        // not supported, the default one is used instead.
        if (FAILED(hr) && m_format != SDR) {
            colorFormat = DEFAULT_FORMAT;
            desc.Format = DEFAULT_FORMAT;
            if (dcompVisual)
                hr = fac->CreateSwapChainForComposition(rhiD->dev, &desc, nullptr, &sc1);
            else
                hr = fac->CreateSwapChainForHwnd(rhiD->dev, hwnd, &desc, nullptr, nullptr, &sc1);
        }

        if (SUCCEEDED(hr)) {
            swapChain = sc1;
            if (m_format != SDR) {
                IDXGISwapChain3 *sc3 = nullptr;
                if (SUCCEEDED(sc1->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void **>(&sc3)))) {
                    hr = sc3->SetColorSpace1(hdrColorSpace);
                    if (FAILED(hr))
                        qWarning("Failed to set color space on swapchain: %s",
                            qPrintable(QSystemError::windowsComString(hr)));
                    sc3->Release();
                } else {
                    qWarning("IDXGISwapChain3 not available, HDR swapchain will not work as expected");
                }
            }
            if (dcompVisual) {
                hr = dcompVisual->SetContent(sc1);
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
            qWarning("Failed to create D3D11 swapchain: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    } else {
        releaseBuffers();
        // flip model -> buffer count is the real buffer count, not 1 like with the legacy modes
        hr = swapChain->ResizeBuffers(UINT(BUFFER_COUNT), UINT(pixelSize.width()), UINT(pixelSize.height()),
                                      colorFormat, swapChainFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            qWarning("Device loss detected in ResizeBuffers()");
            rhiD->deviceLost = true;
            return false;
        } else if (FAILED(hr)) {
            qWarning("Failed to resize D3D11 swapchain: %s",
                qPrintable(QSystemError::windowsComString(hr)));
            return false;
        }
    }

    // This looks odd (for FLIP_*, esp. compared with backends for Vulkan
    // & co.) but the backbuffer is always at index 0, with magic underneath.
    // Some explanation from
    // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-1-4-improvements
    //
    // "In Direct3D 11, applications could call GetBuffer( 0, … ) only once.
    // Every call to Present implicitly changed the resource identity of the
    // returned interface. Direct3D 12 no longer supports that implicit
    // resource identity change, due to the CPU overhead required and the
    // flexible resource descriptor design. As a result, the application must
    // manually call GetBuffer for every each buffer created with the
    // swapchain."

    // So just query index 0 once (per resize) and be done with it.
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&backBufferTex));
    if (FAILED(hr)) {
        qWarning("Failed to query swapchain backbuffer: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = srgbAdjustedColorFormat;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = rhiD->dev->CreateRenderTargetView(backBufferTex, &rtvDesc, &backBufferRtv);
    if (FAILED(hr)) {
        qWarning("Failed to create rtv for swapchain backbuffer: %s",
            qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }

    // Try to reduce stalls by having a dedicated MSAA texture per swapchain buffer.
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        if (sampleDesc.Count > 1) {
            if (!newColorBuffer(pixelSize, srgbAdjustedColorFormat, sampleDesc, &msaaTex[i], &msaaRtv[i]))
                return false;
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

    currentFrameSlot = 0;
    frameCount = 0;
    ds = m_depthStencil ? QRHI_RES(QD3D11RenderBuffer, m_depthStencil) : nullptr;

    rt.setRenderPassDescriptor(m_renderPassDesc); // for the public getter in QRhiRenderTarget
    QD3D11SwapChainRenderTarget *rtD = QRHI_RES(QD3D11SwapChainRenderTarget, &rt);
    rtD->d.rp = QRHI_RES(QD3D11RenderPassDescriptor, m_renderPassDesc);
    rtD->d.pixelSize = pixelSize;
    rtD->d.dpr = float(window->devicePixelRatio());
    rtD->d.sampleCount = int(sampleDesc.Count);
    rtD->d.colorAttCount = 1;
    rtD->d.dsAttCount = m_depthStencil ? 1 : 0;

    if (rhiD->rhiFlags.testFlag(QRhi::EnableTimestamps)) {
        timestamps.prepare(BUFFER_COUNT, rhiD);
        // timestamp queries are optional so we can go on even if they failed
    }

    if (needsRegistration)
        rhiD->registerResource(this);

    return true;
}

QT_END_NAMESPACE
