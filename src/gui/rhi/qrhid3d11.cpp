/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qrhid3d11_p_p.h"
#include "qshader_p.h"
#include "cs_tdr_p.h"
#include <QWindow>
#include <QOperatingSystemVersion>
#include <qmath.h>
#include <private/qsystemlibrary_p.h>

#include <d3dcompiler.h>
#include <comdef.h>

QT_BEGIN_NAMESPACE

/*
  Direct3D 11 backend. Provides a double-buffered flip model (FLIP_DISCARD)
  swapchain. Textures and "static" buffers are USAGE_DEFAULT, leaving it to
  UpdateSubResource to upload the data in any way it sees fit. "Dynamic"
  buffers are USAGE_DYNAMIC and updating is done by mapping with WRITE_DISCARD.
  (so here QRhiBuffer keeps a copy of the buffer contents and all of it is
  memcpy'd every time, leaving the rest (juggling with the memory area Map
  returns) to the driver).
*/

/*!
    \class QRhiD3D11InitParams
    \internal
    \inmodule QtGui
    \brief Direct3D 11 specific initialization parameters.

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
    instances that have their surface type set to QSurface::OpenGLSurface.
    There are currently no Direct3D specifics in the Windows platform support
    of Qt and therefore there is no separate QSurface type available.

    \section2 Working with existing Direct3D 11 devices

    When interoperating with another graphics engine, it may be necessary to
    get a QRhi instance that uses the same Direct3D device. This can be
    achieved by passing a pointer to a QRhiD3D11NativeHandles to
    QRhi::create(). Both the device and the device context must be set to a
    non-null value then.

    The QRhi does not take ownership of any of the external objects.

    \note QRhi works with immediate contexts only. Deferred contexts are not
    used in any way.

    \note Regardless of using an imported or a QRhi-created device context, the
    \c ID3D11DeviceContext1 interface (Direct3D 11.1) must be supported.
    Initialization will fail otherwise.
 */

/*!
    \class QRhiD3D11NativeHandles
    \internal
    \inmodule QtGui
    \brief Holds the D3D device and device context used by the QRhi.

    \note The class uses \c{void *} as the type since including the COM-based
    \c{d3d11.h} headers is not acceptable here. The actual types are
    \c{ID3D11Device *} and \c{ID3D11DeviceContext *}.
 */

// help mingw with its ancient sdk headers
#ifndef DXGI_ADAPTER_FLAG_SOFTWARE
#define DXGI_ADAPTER_FLAG_SOFTWARE 2
#endif

#ifndef D3D11_1_UAV_SLOT_COUNT
#define D3D11_1_UAV_SLOT_COUNT 64
#endif

QRhiD3D11::QRhiD3D11(QRhiD3D11InitParams *params, QRhiD3D11NativeHandles *importDevice)
    : ofr(this),
      deviceCurse(this)
{
    debugLayer = params->enableDebugLayer;

    deviceCurse.framesToActivate = params->framesUntilKillingDeviceViaTdr;
    deviceCurse.permanent = params->repeatDeviceKill;

    importedDevice = importDevice != nullptr;
    if (importedDevice) {
        dev = reinterpret_cast<ID3D11Device *>(importDevice->dev);
        if (dev) {
            ID3D11DeviceContext *ctx = reinterpret_cast<ID3D11DeviceContext *>(importDevice->context);
            if (SUCCEEDED(ctx->QueryInterface(IID_ID3D11DeviceContext1, reinterpret_cast<void **>(&context)))) {
                // get rid of the ref added by QueryInterface
                ctx->Release();
            } else {
                qWarning("ID3D11DeviceContext1 not supported by context, cannot import");
                importedDevice = false;
            }
        } else {
            qWarning("No ID3D11Device given, cannot import");
            importedDevice = false;
        }
    }
}

static QString comErrorMessage(HRESULT hr)
{
#ifndef Q_OS_WINRT
    const _com_error comError(hr);
#else
    const _com_error comError(hr, nullptr);
#endif
    QString result = QLatin1String("Error 0x") + QString::number(ulong(hr), 16);
    if (const wchar_t *msg = comError.ErrorMessage())
        result += QLatin1String(": ") + QString::fromWCharArray(msg);
    return result;
}

template <class Int>
inline Int aligned(Int v, Int byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

static IDXGIFactory1 *createDXGIFactory2()
{
    IDXGIFactory1 *result = nullptr;
    if (QOperatingSystemVersion::current() > QOperatingSystemVersion::Windows7) {
        using PtrCreateDXGIFactory2 = HRESULT (WINAPI *)(UINT, REFIID, void **);
        QSystemLibrary dxgilib(QStringLiteral("dxgi"));
        if (auto createDXGIFactory2 = reinterpret_cast<PtrCreateDXGIFactory2>(dxgilib.resolve("CreateDXGIFactory2"))) {
            const HRESULT hr = createDXGIFactory2(0, IID_IDXGIFactory2, reinterpret_cast<void **>(&result));
            if (FAILED(hr)) {
                qWarning("CreateDXGIFactory2() failed to create DXGI factory: %s", qPrintable(comErrorMessage(hr)));
                result = nullptr;
            }
        } else {
            qWarning("Unable to resolve CreateDXGIFactory2()");
        }
    }
    return result;
}

static IDXGIFactory1 *createDXGIFactory1()
{
    IDXGIFactory1 *result = nullptr;
    const HRESULT hr = CreateDXGIFactory1(IID_IDXGIFactory1, reinterpret_cast<void **>(&result));
    if (FAILED(hr)) {
        qWarning("CreateDXGIFactory1() failed to create DXGI factory: %s", qPrintable(comErrorMessage(hr)));
        result = nullptr;
    }
    return result;
}

bool QRhiD3D11::create(QRhi::Flags flags)
{
    Q_UNUSED(flags);

    uint devFlags = 0;
    if (debugLayer)
        devFlags |= D3D11_CREATE_DEVICE_DEBUG;

    dxgiFactory = createDXGIFactory2();
    if (dxgiFactory != nullptr) {
        hasDxgi2 = true;
        supportsFlipDiscardSwapchain = QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10
                && !qEnvironmentVariableIntValue("QT_D3D_NO_FLIP");
    } else {
        dxgiFactory = createDXGIFactory1();
        hasDxgi2 = false;
        supportsFlipDiscardSwapchain = false;
    }

    if (dxgiFactory == nullptr)
        return false;

    qCDebug(QRHI_LOG_INFO, "DXGI 1.2 = %s, FLIP_DISCARD swapchain supported = %s",
            hasDxgi2 ? "true" : "false", supportsFlipDiscardSwapchain ? "true" : "false");

    if (!importedDevice) {
        IDXGIAdapter1 *adapterToUse = nullptr;
        IDXGIAdapter1 *adapter;
        int requestedAdapterIndex = -1;
        if (qEnvironmentVariableIsSet("QT_D3D_ADAPTER_INDEX"))
            requestedAdapterIndex = qEnvironmentVariableIntValue("QT_D3D_ADAPTER_INDEX");

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
            if (!adapterToUse && (requestedAdapterIndex < 0 || requestedAdapterIndex == adapterIndex)) {
                adapterToUse = adapter;
                qCDebug(QRHI_LOG_INFO, "  using this adapter");
            } else {
                adapter->Release();
            }
        }
        if (!adapterToUse) {
            qWarning("No adapter");
            return false;
        }

        ID3D11DeviceContext *ctx = nullptr;
        HRESULT hr = D3D11CreateDevice(adapterToUse, D3D_DRIVER_TYPE_UNKNOWN, nullptr, devFlags,
                                       nullptr, 0, D3D11_SDK_VERSION,
                                       &dev, &featureLevel, &ctx);
        adapterToUse->Release();
        if (FAILED(hr)) {
            qWarning("Failed to create D3D11 device and context: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }
        if (SUCCEEDED(ctx->QueryInterface(IID_ID3D11DeviceContext1, reinterpret_cast<void **>(&context)))) {
            ctx->Release();
        } else {
            qWarning("ID3D11DeviceContext1 not supported");
            return false;
        }
    } else {
        Q_ASSERT(dev && context);
        featureLevel = dev->GetFeatureLevel();
    }

    if (FAILED(context->QueryInterface(IID_ID3DUserDefinedAnnotation, reinterpret_cast<void **>(&annotations))))
        annotations = nullptr;

    deviceLost = false;

    nativeHandlesStruct.dev = dev;
    nativeHandlesStruct.context = context;

    if (deviceCurse.framesToActivate > 0)
        deviceCurse.initResources();

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

    deviceCurse.releaseResources();

    if (annotations) {
        annotations->Release();
        annotations = nullptr;
    }

    if (!importedDevice) {
        if (context) {
            context->Release();
            context = nullptr;
        }
        if (dev) {
            dev->Release();
            dev = nullptr;
        }
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
    if (SUCCEEDED(device->QueryInterface(IID_ID3D11Debug, reinterpret_cast<void **>(&debug)))) {
        debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        debug->Release();
    }
}

QVector<int> QRhiD3D11::supportedSampleCounts() const
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

QRhiBuffer *QRhiD3D11::createBuffer(QRhiBuffer::Type type, QRhiBuffer::UsageFlags usage, int size)
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
    default:
        Q_UNREACHABLE();
        return 0;
    }
}

const QRhiNativeHandles *QRhiD3D11::nativeHandles()
{
    return &nativeHandlesStruct;
}

void QRhiD3D11::sendVMemStatsToProfiler()
{
    // nothing to do here
}

bool QRhiD3D11::makeThreadLocalNativeContextCurrent()
{
    // not applicable
    return false;
}

void QRhiD3D11::releaseCachedResources()
{
    clearShaderCache();
}

bool QRhiD3D11::isDeviceLost() const
{
    return deviceLost;
}

QRhiRenderBuffer *QRhiD3D11::createRenderBuffer(QRhiRenderBuffer::Type type, const QSize &pixelSize,
                                                int sampleCount, QRhiRenderBuffer::Flags flags)
{
    return new QD3D11RenderBuffer(this, type, pixelSize, sampleCount, flags);
}

QRhiTexture *QRhiD3D11::createTexture(QRhiTexture::Format format, const QSize &pixelSize,
                                      int sampleCount, QRhiTexture::Flags flags)
{
    return new QD3D11Texture(this, format, pixelSize, sampleCount, flags);
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

        QD3D11CommandBuffer::Command cmd;
        cmd.cmd = QD3D11CommandBuffer::Command::BindGraphicsPipeline;
        cmd.args.bindGraphicsPipeline.ps = psD;
        cbD->commands.append(cmd);
    }
}

static const int RBM_SUPPORTED_STAGES = 3;
static const int RBM_VERTEX = 0;
static const int RBM_FRAGMENT = 1;
static const int RBM_COMPUTE = 2;

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

    bool hasDynamicOffsetInSrb = false;
    bool srbUpdate = false;
    for (int i = 0, ie = srbD->sortedBindings.count(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = srbD->sortedBindings.at(i).data();
        QD3D11ShaderResourceBindings::BoundResourceData &bd(srbD->boundResourceData[i]);
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, b->u.ubuf.buf);
            if (bufD->m_type == QRhiBuffer::Dynamic)
                executeBufferHostWrites(bufD);

            if (bufD->generation != bd.ubuf.generation || bufD->m_id != bd.ubuf.id) {
                srbUpdate = true;
                bd.ubuf.id = bufD->m_id;
                bd.ubuf.generation = bufD->generation;
            }

            if (b->u.ubuf.hasDynamicOffset)
                hasDynamicOffsetInSrb = true;
        }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
        {
            const QRhiShaderResourceBinding::Data::SampledTextureData *data = &b->u.stex;
            if (bd.stex.count != data->count) {
                bd.stex.count = data->count;
                srbUpdate = true;
            }
            for (int elem = 0; elem < data->count; ++elem) {
                QD3D11Texture *texD = QRHI_RES(QD3D11Texture, data->texSamplers[elem].tex);
                QD3D11Sampler *samplerD = QRHI_RES(QD3D11Sampler, data->texSamplers[elem].sampler);
                if (texD->generation != bd.stex.d[elem].texGeneration
                        || texD->m_id != bd.stex.d[elem].texId
                        || samplerD->generation != bd.stex.d[elem].samplerGeneration
                        || samplerD->m_id != bd.stex.d[elem].samplerId)
                {
                    srbUpdate = true;
                    bd.stex.d[elem].texId = texD->m_id;
                    bd.stex.d[elem].texGeneration = texD->generation;
                    bd.stex.d[elem].samplerId = samplerD->m_id;
                    bd.stex.d[elem].samplerGeneration = samplerD->generation;
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
            resBindMaps[RBM_FRAGMENT] = &gfxPsD->fs.nativeResourceBindingMap;
        } else {
            resBindMaps[RBM_COMPUTE] = &compPsD->cs.nativeResourceBindingMap;
        }
        updateShaderResourceBindings(srbD, resBindMaps);
    }

    const bool srbChanged = gfxPsD ? (cbD->currentGraphicsSrb != srb) : (cbD->currentComputeSrb != srb);
    const bool srbRebuilt = cbD->currentSrbGeneration != srbD->generation;

    if (srbChanged || srbRebuilt || srbUpdate || hasDynamicOffsetInSrb) {
        if (gfxPsD) {
            cbD->currentGraphicsSrb = srb;
            cbD->currentComputeSrb = nullptr;
        } else {
            cbD->currentGraphicsSrb = nullptr;
            cbD->currentComputeSrb = srb;
        }
        cbD->currentSrbGeneration = srbD->generation;

        QD3D11CommandBuffer::Command cmd;
        cmd.cmd = QD3D11CommandBuffer::Command::BindShaderResources;
        cmd.args.bindShaderResources.srb = srbD;
        // dynamic offsets have to be applied at the time of executing the bind
        // operations, not here
        cmd.args.bindShaderResources.offsetOnlyChange = !srbChanged && !srbRebuilt && !srbUpdate && hasDynamicOffsetInSrb;
        cmd.args.bindShaderResources.dynamicOffsetCount = 0;
        if (hasDynamicOffsetInSrb) {
            if (dynamicOffsetCount < QD3D11CommandBuffer::Command::MAX_UBUF_BINDINGS) {
                cmd.args.bindShaderResources.dynamicOffsetCount = dynamicOffsetCount;
                uint *p = cmd.args.bindShaderResources.dynamicOffsetPairs;
                for (int i = 0; i < dynamicOffsetCount; ++i) {
                    const QRhiCommandBuffer::DynamicOffset &dynOfs(dynamicOffsets[i]);
                    const uint binding = uint(dynOfs.first);
                    Q_ASSERT(aligned(dynOfs.second, quint32(256)) == dynOfs.second);
                    const uint offsetInConstants = dynOfs.second / 16;
                    *p++ = binding;
                    *p++ = offsetInConstants;
                }
            } else {
                qWarning("Too many dynamic offsets (%d, max is %d)",
                         dynamicOffsetCount, QD3D11CommandBuffer::Command::MAX_UBUF_BINDINGS);
            }
        }

        cbD->commands.append(cmd);
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
        QD3D11CommandBuffer::Command cmd;
        cmd.cmd = QD3D11CommandBuffer::Command::BindVertexBuffers;
        cmd.args.bindVertexBuffers.startSlot = startBinding;
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
        cbD->commands.append(cmd);
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

            QD3D11CommandBuffer::Command cmd;
            cmd.cmd = QD3D11CommandBuffer::Command::BindIndexBuffer;
            cmd.args.bindIndexBuffer.buffer = ibufD->buffer;
            cmd.args.bindIndexBuffer.offset = indexOffset;
            cmd.args.bindIndexBuffer.format = dxgiFormat;
            cbD->commands.append(cmd);
        }
    }
}

void QRhiD3D11::setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);
    Q_ASSERT(cbD->currentTarget);
    const QSize outputSize = cbD->currentTarget->pixelSize();

    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::Viewport;

    // d3d expects top-left, QRhiViewport is bottom-left
    float x, y, w, h;
    if (!qrhi_toTopLeftRenderTargetRect(outputSize, viewport.viewport(), &x, &y, &w, &h))
        return;

    cmd.args.viewport.x = x;
    cmd.args.viewport.y = y;
    cmd.args.viewport.w = w;
    cmd.args.viewport.h = h;
    cmd.args.viewport.d0 = viewport.minDepth();
    cmd.args.viewport.d1 = viewport.maxDepth();
    cbD->commands.append(cmd);
}

void QRhiD3D11::setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);
    Q_ASSERT(cbD->currentTarget);
    const QSize outputSize = cbD->currentTarget->pixelSize();

    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::Scissor;

    // d3d expects top-left, QRhiScissor is bottom-left
    int x, y, w, h;
    if (!qrhi_toTopLeftRenderTargetRect(outputSize, scissor.scissor(), &x, &y, &w, &h))
        return;

    cmd.args.scissor.x = x;
    cmd.args.scissor.y = y;
    cmd.args.scissor.w = w;
    cmd.args.scissor.h = h;
    cbD->commands.append(cmd);
}

void QRhiD3D11::setBlendConstants(QRhiCommandBuffer *cb, const QColor &c)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);

    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::BlendConstants;
    cmd.args.blendConstants.ps = QRHI_RES(QD3D11GraphicsPipeline, cbD->currentGraphicsPipeline);
    cmd.args.blendConstants.c[0] = float(c.redF());
    cmd.args.blendConstants.c[1] = float(c.greenF());
    cmd.args.blendConstants.c[2] = float(c.blueF());
    cmd.args.blendConstants.c[3] = float(c.alphaF());
    cbD->commands.append(cmd);
}

void QRhiD3D11::setStencilRef(QRhiCommandBuffer *cb, quint32 refValue)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);

    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::StencilRef;
    cmd.args.stencilRef.ps = QRHI_RES(QD3D11GraphicsPipeline, cbD->currentGraphicsPipeline);
    cmd.args.stencilRef.ref = refValue;
    cbD->commands.append(cmd);
}

void QRhiD3D11::draw(QRhiCommandBuffer *cb, quint32 vertexCount,
                     quint32 instanceCount, quint32 firstVertex, quint32 firstInstance)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);

    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::Draw;
    cmd.args.draw.ps = QRHI_RES(QD3D11GraphicsPipeline, cbD->currentGraphicsPipeline);
    cmd.args.draw.vertexCount = vertexCount;
    cmd.args.draw.instanceCount = instanceCount;
    cmd.args.draw.firstVertex = firstVertex;
    cmd.args.draw.firstInstance = firstInstance;
    cbD->commands.append(cmd);
}

void QRhiD3D11::drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                            quint32 instanceCount, quint32 firstIndex, qint32 vertexOffset, quint32 firstInstance)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::RenderPass);

    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::DrawIndexed;
    cmd.args.drawIndexed.ps = QRHI_RES(QD3D11GraphicsPipeline, cbD->currentGraphicsPipeline);
    cmd.args.drawIndexed.indexCount = indexCount;
    cmd.args.drawIndexed.instanceCount = instanceCount;
    cmd.args.drawIndexed.firstIndex = firstIndex;
    cmd.args.drawIndexed.vertexOffset = vertexOffset;
    cmd.args.drawIndexed.firstInstance = firstInstance;
    cbD->commands.append(cmd);
}

void QRhiD3D11::debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name)
{
    if (!debugMarkers || !annotations)
        return;

    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::DebugMarkBegin;
    strncpy(cmd.args.debugMark.s, name.constData(), sizeof(cmd.args.debugMark.s));
    cmd.args.debugMark.s[sizeof(cmd.args.debugMark.s) - 1] = '\0';
    cbD->commands.append(cmd);
}

void QRhiD3D11::debugMarkEnd(QRhiCommandBuffer *cb)
{
    if (!debugMarkers || !annotations)
        return;

    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::DebugMarkEnd;
    cbD->commands.append(cmd);
}

void QRhiD3D11::debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg)
{
    if (!debugMarkers || !annotations)
        return;

    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::DebugMarkMsg;
    strncpy(cmd.args.debugMark.s, msg.constData(), sizeof(cmd.args.debugMark.s));
    cmd.args.debugMark.s[sizeof(cmd.args.debugMark.s) - 1] = '\0';
    cbD->commands.append(cmd);
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
        QD3D11CommandBuffer::Command fbCmd;
        fbCmd.cmd = QD3D11CommandBuffer::Command::SetRenderTarget;
        fbCmd.args.setRenderTarget.rt = cbD->currentTarget;
        cbD->commands.append(fbCmd);
    }
}

QRhi::FrameOpResult QRhiD3D11::beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);

    QD3D11SwapChain *swapChainD = QRHI_RES(QD3D11SwapChain, swapChain);
    contextState.currentSwapChain = swapChainD;
    const int currentFrameSlot = swapChainD->currentFrameSlot;
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();

    if (swapChainD->timestampActive[currentFrameSlot]) {
        ID3D11Query *tsDisjoint = swapChainD->timestampDisjointQuery[currentFrameSlot];
        const int tsIdx = QD3D11SwapChain::BUFFER_COUNT * currentFrameSlot;
        ID3D11Query *tsStart = swapChainD->timestampQuery[tsIdx];
        ID3D11Query *tsEnd = swapChainD->timestampQuery[tsIdx + 1];
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
                // finally got a value, just report it, the profiler cares about min/max/avg anyway
                QRHI_PROF_F(swapChainFrameGpuTime(swapChain, elapsedMs));
            }
            swapChainD->timestampActive[currentFrameSlot] = false;
        } // else leave timestampActive set to true, will retry in a subsequent beginFrame
    }

    swapChainD->cb.resetState();

    swapChainD->rt.d.rtv[0] = swapChainD->sampleDesc.Count > 1 ?
                swapChainD->msaaRtv[currentFrameSlot] : swapChainD->backBufferRtv;
    swapChainD->rt.d.dsv = swapChainD->ds ? swapChainD->ds->dsv : nullptr;

    QRHI_PROF_F(beginSwapChainFrame(swapChain));

    finishActiveReadbacks();

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiD3D11::endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags)
{
    QD3D11SwapChain *swapChainD = QRHI_RES(QD3D11SwapChain, swapChain);
    Q_ASSERT(contextState.currentSwapChain = swapChainD);
    const int currentFrameSlot = swapChainD->currentFrameSlot;

    ID3D11Query *tsDisjoint = swapChainD->timestampDisjointQuery[currentFrameSlot];
    const int tsIdx = QD3D11SwapChain::BUFFER_COUNT * currentFrameSlot;
    ID3D11Query *tsStart = swapChainD->timestampQuery[tsIdx];
    ID3D11Query *tsEnd = swapChainD->timestampQuery[tsIdx + 1];
    const bool recordTimestamps = tsDisjoint && tsStart && tsEnd && !swapChainD->timestampActive[currentFrameSlot];

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
        swapChainD->timestampActive[currentFrameSlot] = true;
    }

    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();
    // this must be done before the Present
    QRHI_PROF_F(endSwapChainFrame(swapChain, swapChainD->frameCount + 1));

    if (!flags.testFlag(QRhi::SkipPresent)) {
        const UINT presentFlags = 0;
        HRESULT hr = swapChainD->swapChain->Present(swapChainD->swapInterval, presentFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            qWarning("Device loss detected in Present()");
            deviceLost = true;
            return QRhi::FrameOpDeviceLost;
        } else if (FAILED(hr)) {
            qWarning("Failed to present: %s", qPrintable(comErrorMessage(hr)));
            return QRhi::FrameOpError;
        }

        // move on to the next buffer
        swapChainD->currentFrameSlot = (swapChainD->currentFrameSlot + 1) % QD3D11SwapChain::BUFFER_COUNT;
    } else {
        context->Flush();
    }

    swapChainD->frameCount += 1;
    contextState.currentSwapChain = nullptr;

    if (deviceCurse.framesToActivate > 0) {
        deviceCurse.framesLeft -= 1;
        if (deviceCurse.framesLeft == 0) {
            deviceCurse.framesLeft = deviceCurse.framesToActivate;
            if (!deviceCurse.permanent)
                deviceCurse.framesToActivate = -1;

            deviceCurse.activate();
        } else if (deviceCurse.framesLeft % 100 == 0) {
            qDebug("Impending doom: %d frames left", deviceCurse.framesLeft);
        }
    }

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiD3D11::beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);
    ofr.active = true;

    ofr.cbWrapper.resetState();
    *cb = &ofr.cbWrapper;

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiD3D11::endOffscreenFrame(QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    ofr.active = false;

    executeCommandBuffer(&ofr.cbWrapper);

    finishActiveReadbacks();

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
    case QRhiTexture::R16:
        return DXGI_FORMAT_R16_UNORM;
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

    case QRhiTexture::D16:
        return DXGI_FORMAT_R16_TYPELESS;
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

static inline QRhiTexture::Format colorTextureFormatFromDxgiFormat(DXGI_FORMAT format, QRhiTexture::Flags *flags)
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
    case DXGI_FORMAT_R8_UNORM:
        return QRhiTexture::R8;
    case DXGI_FORMAT_R16_UNORM:
        return QRhiTexture::R16;
    default: // this cannot assert, must warn and return unknown
        qWarning("DXGI_FORMAT %d is not a recognized uncompressed color format", format);
        break;
    }
    return QRhiTexture::UnknownFormat;
}

static inline bool isDepthTextureFormat(QRhiTexture::Format format)
{
    switch (format) {
    case QRhiTexture::Format::D16:
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
    UINT subres = D3D11CalcSubresource(UINT(level), UINT(layer), texD->mipLevelCount);
    const QPoint dp = subresDesc.destinationTopLeft();
    D3D11_BOX box;
    box.front = 0;
    // back, right, bottom are exclusive
    box.back = 1;
    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::UpdateSubRes;
    cmd.args.updateSubRes.dst = texD->tex;
    cmd.args.updateSubRes.dstSubRes = subres;

    bool cmdValid = true;
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
        textureFormatInfo(texD->m_format, size, &bpl, nullptr);
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
        cmdValid = false;
    }
    if (cmdValid)
        cbD->commands.append(cmd);
}

void QRhiD3D11::enqueueResourceUpdates(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    QRhiResourceUpdateBatchPrivate *ud = QRhiResourceUpdateBatchPrivate::get(resourceUpdates);
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();

    for (const QRhiResourceUpdateBatchPrivate::BufferOp &u : ud->bufferOps) {
        if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::DynamicUpdate) {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, u.buf);
            Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
            memcpy(bufD->dynBuf.data() + u.offset, u.data.constData(), size_t(u.data.size()));
            bufD->hasPendingDynamicUpdates = true;
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::StaticUpload) {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, u.buf);
            Q_ASSERT(bufD->m_type != QRhiBuffer::Dynamic);
            Q_ASSERT(u.offset + u.data.size() <= bufD->m_size);
            QD3D11CommandBuffer::Command cmd;
            cmd.cmd = QD3D11CommandBuffer::Command::UpdateSubRes;
            cmd.args.updateSubRes.dst = bufD->buffer;
            cmd.args.updateSubRes.dstSubRes = 0;
            cmd.args.updateSubRes.src = cbD->retainData(u.data);
            cmd.args.updateSubRes.srcRowPitch = 0;
            // Specify the region (even when offset is 0 and all data is provided)
            // since the ID3D11Buffer's size is rounded up to be a multiple of 256
            // while the data we have has the original size.
            D3D11_BOX box;
            box.left = UINT(u.offset);
            box.top = box.front = 0;
            box.back = box.bottom = 1;
            box.right = UINT(u.offset + u.data.size()); // no -1: right, bottom, back are exclusive, see D3D11_BOX doc
            cmd.args.updateSubRes.hasDstBox = true;
            cmd.args.updateSubRes.dstBox = box;
            cbD->commands.append(cmd);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::Read) {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, u.buf);
            if (bufD->m_type == QRhiBuffer::Dynamic) {
                u.result->data.resize(u.readSize);
                memcpy(u.result->data.data(), bufD->dynBuf.constData() + u.offset, size_t(u.readSize));
            } else {
                BufferReadback readback;
                readback.result = u.result;
                readback.byteSize = u.readSize;

                D3D11_BUFFER_DESC desc;
                memset(&desc, 0, sizeof(desc));
                desc.ByteWidth = readback.byteSize;
                desc.Usage = D3D11_USAGE_STAGING;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                HRESULT hr = dev->CreateBuffer(&desc, nullptr, &readback.stagingBuf);
                if (FAILED(hr)) {
                    qWarning("Failed to create buffer: %s", qPrintable(comErrorMessage(hr)));
                    continue;
                }
                QRHI_PROF_F(newReadbackBuffer(qint64(qintptr(readback.stagingBuf)), bufD, readback.byteSize));

                QD3D11CommandBuffer::Command cmd;
                cmd.cmd = QD3D11CommandBuffer::Command::CopySubRes;
                cmd.args.copySubRes.dst = readback.stagingBuf;
                cmd.args.copySubRes.dstSubRes = 0;
                cmd.args.copySubRes.dstX = 0;
                cmd.args.copySubRes.dstY = 0;
                cmd.args.copySubRes.src = bufD->buffer;
                cmd.args.copySubRes.srcSubRes = 0;
                cmd.args.copySubRes.hasSrcBox = true;
                D3D11_BOX box;
                box.left = UINT(u.offset);
                box.top = box.front = 0;
                box.back = box.bottom = 1;
                box.right = UINT(u.offset + u.readSize);
                cmd.args.copySubRes.srcBox = box;
                cbD->commands.append(cmd);

                activeBufferReadbacks.append(readback);
            }
            if (u.result->completed)
                u.result->completed();
        }
    }

    for (const QRhiResourceUpdateBatchPrivate::TextureOp &u : ud->textureOps) {
        if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Upload) {
            QD3D11Texture *texD = QRHI_RES(QD3D11Texture, u.dst);
            for (int layer = 0; layer < QRhi::MAX_LAYERS; ++layer) {
                for (int level = 0; level < QRhi::MAX_LEVELS; ++level) {
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : qAsConst(u.subresDesc[layer][level]))
                        enqueueSubresUpload(texD, cbD, layer, level, subresDesc);
                }
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Copy) {
            Q_ASSERT(u.src && u.dst);
            QD3D11Texture *srcD = QRHI_RES(QD3D11Texture, u.src);
            QD3D11Texture *dstD = QRHI_RES(QD3D11Texture, u.dst);
            UINT srcSubRes = D3D11CalcSubresource(UINT(u.desc.sourceLevel()), UINT(u.desc.sourceLayer()), srcD->mipLevelCount);
            UINT dstSubRes = D3D11CalcSubresource(UINT(u.desc.destinationLevel()), UINT(u.desc.destinationLayer()), dstD->mipLevelCount);
            const QPoint dp = u.desc.destinationTopLeft();
            const QSize mipSize = q->sizeForMipLevel(u.desc.sourceLevel(), srcD->m_pixelSize);
            const QSize copySize = u.desc.pixelSize().isEmpty() ? mipSize : u.desc.pixelSize();
            const QPoint sp = u.desc.sourceTopLeft();
            D3D11_BOX srcBox;
            srcBox.left = UINT(sp.x());
            srcBox.top = UINT(sp.y());
            srcBox.front = 0;
            // back, right, bottom are exclusive
            srcBox.right = srcBox.left + UINT(copySize.width());
            srcBox.bottom = srcBox.top + UINT(copySize.height());
            srcBox.back = 1;
            QD3D11CommandBuffer::Command cmd;
            cmd.cmd = QD3D11CommandBuffer::Command::CopySubRes;
            cmd.args.copySubRes.dst = dstD->tex;
            cmd.args.copySubRes.dstSubRes = dstSubRes;
            cmd.args.copySubRes.dstX = UINT(dp.x());
            cmd.args.copySubRes.dstY = UINT(dp.y());
            cmd.args.copySubRes.src = srcD->tex;
            cmd.args.copySubRes.srcSubRes = srcSubRes;
            cmd.args.copySubRes.hasSrcBox = true;
            cmd.args.copySubRes.srcBox = srcBox;
            cbD->commands.append(cmd);
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

            if (texD) {
                if (texD->sampleDesc.Count > 1) {
                    qWarning("Multisample texture cannot be read back");
                    continue;
                }
                src = texD->tex;
                dxgiFormat = texD->dxgiFormat;
                pixelSize = q->sizeForMipLevel(u.rb.level(), texD->m_pixelSize);
                format = texD->m_format;
                subres = D3D11CalcSubresource(UINT(u.rb.level()), UINT(u.rb.layer()), texD->mipLevelCount);
            } else {
                Q_ASSERT(contextState.currentSwapChain);
                swapChainD = QRHI_RES(QD3D11SwapChain, contextState.currentSwapChain);
                if (swapChainD->sampleDesc.Count > 1) {
                    // Unlike with textures, reading back a multisample swapchain image
                    // has to be supported. Insert a resolve.
                    QD3D11CommandBuffer::Command rcmd;
                    rcmd.cmd = QD3D11CommandBuffer::Command::ResolveSubRes;
                    rcmd.args.resolveSubRes.dst = swapChainD->backBufferTex;
                    rcmd.args.resolveSubRes.dstSubRes = 0;
                    rcmd.args.resolveSubRes.src = swapChainD->msaaTex[swapChainD->currentFrameSlot];
                    rcmd.args.resolveSubRes.srcSubRes = 0;
                    rcmd.args.resolveSubRes.format = swapChainD->colorFormat;
                    cbD->commands.append(rcmd);
                }
                src = swapChainD->backBufferTex;
                dxgiFormat = swapChainD->colorFormat;
                pixelSize = swapChainD->pixelSize;
                format = colorTextureFormatFromDxgiFormat(dxgiFormat, nullptr);
                if (format == QRhiTexture::UnknownFormat)
                    continue;
            }
            quint32 byteSize = 0;
            quint32 bpl = 0;
            textureFormatInfo(format, pixelSize, &bpl, &byteSize);

            D3D11_TEXTURE2D_DESC desc;
            memset(&desc, 0, sizeof(desc));
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
                qWarning("Failed to create readback staging texture: %s", qPrintable(comErrorMessage(hr)));
                return;
            }
            QRHI_PROF_F(newReadbackBuffer(qint64(qintptr(stagingTex)),
                                          texD ? static_cast<QRhiResource *>(texD) : static_cast<QRhiResource *>(swapChainD),
                                          byteSize));

            QD3D11CommandBuffer::Command cmd;
            cmd.cmd = QD3D11CommandBuffer::Command::CopySubRes;
            cmd.args.copySubRes.dst = stagingTex;
            cmd.args.copySubRes.dstSubRes = 0;
            cmd.args.copySubRes.dstX = 0;
            cmd.args.copySubRes.dstY = 0;
            cmd.args.copySubRes.src = src;
            cmd.args.copySubRes.srcSubRes = subres;
            cmd.args.copySubRes.hasSrcBox = false;
            cbD->commands.append(cmd);

            readback.stagingTex = stagingTex;
            readback.byteSize = byteSize;
            readback.bpl = bpl;
            readback.pixelSize = pixelSize;
            readback.format = format;

            activeTextureReadbacks.append(readback);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::GenMips) {
            Q_ASSERT(u.dst->flags().testFlag(QRhiTexture::UsedWithGenerateMips));
            QD3D11CommandBuffer::Command cmd;
            cmd.cmd = QD3D11CommandBuffer::Command::GenMip;
            cmd.args.genMip.srv = QRHI_RES(QD3D11Texture, u.dst)->srv;
            cbD->commands.append(cmd);
        }
    }

    ud->free();
}

void QRhiD3D11::finishActiveReadbacks()
{
    QVarLengthArray<std::function<void()>, 4> completedCallbacks;
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();

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
            qWarning("Failed to map readback staging texture: %s", qPrintable(comErrorMessage(hr)));
        }

        readback.stagingTex->Release();
        QRHI_PROF_F(releaseReadbackBuffer(qint64(qintptr(readback.stagingTex))));

        if (readback.result->completed)
            completedCallbacks.append(readback.result->completed);

        activeTextureReadbacks.removeAt(i);
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
            qWarning("Failed to map readback staging texture: %s", qPrintable(comErrorMessage(hr)));
        }

        readback.stagingBuf->Release();
        QRHI_PROF_F(releaseReadbackBuffer(qint64(qintptr(readback.stagingBuf))));

        if (readback.result->completed)
            completedCallbacks.append(readback.result->completed);

        activeBufferReadbacks.removeAt(i);
    }

    for (auto f : completedCallbacks)
        f();
}

static inline QD3D11RenderTargetData *rtData(QRhiRenderTarget *rt)
{
    switch (rt->resourceType()) {
    case QRhiResource::RenderTarget:
        return &QRHI_RES(QD3D11ReferenceRenderTarget, rt)->d;
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
                          QRhiResourceUpdateBatch *resourceUpdates)
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
    }

    QD3D11CommandBuffer::Command fbCmd;
    fbCmd.cmd = QD3D11CommandBuffer::Command::ResetShaderResources;
    cbD->commands.append(fbCmd);
    fbCmd.cmd = QD3D11CommandBuffer::Command::SetRenderTarget;
    fbCmd.args.setRenderTarget.rt = rt;
    cbD->commands.append(fbCmd);

    QD3D11CommandBuffer::Command clearCmd;
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
    cbD->commands.append(clearCmd);

    cbD->recordingPass = QD3D11CommandBuffer::RenderPass;
    cbD->currentTarget = rt;

    cbD->resetCachedShaderResourceState();
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
            QD3D11CommandBuffer::Command cmd;
            cmd.cmd = QD3D11CommandBuffer::Command::ResolveSubRes;
            cmd.args.resolveSubRes.dst = dstTexD->tex;
            cmd.args.resolveSubRes.dstSubRes = D3D11CalcSubresource(UINT(colorAtt.resolveLevel()),
                                                                    UINT(colorAtt.resolveLayer()),
                                                                    dstTexD->mipLevelCount);
            if (srcTexD) {
                cmd.args.resolveSubRes.src = srcTexD->tex;
                if (srcTexD->dxgiFormat != dstTexD->dxgiFormat) {
                    qWarning("Resolve source and destination formats do not match");
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
                cmd.args.resolveSubRes.src = srcRbD->tex;
                if (srcRbD->dxgiFormat != dstTexD->dxgiFormat) {
                    qWarning("Resolve source and destination formats do not match");
                    continue;
                }
                if (srcRbD->m_pixelSize != dstTexD->m_pixelSize) {
                    qWarning("Resolve source and destination sizes do not match");
                    continue;
                }
            }
            cmd.args.resolveSubRes.srcSubRes = D3D11CalcSubresource(0, UINT(colorAtt.layer()), 1);
            cmd.args.resolveSubRes.format = dstTexD->dxgiFormat;
            cbD->commands.append(cmd);
        }
    }

    cbD->recordingPass = QD3D11CommandBuffer::NoPass;
    cbD->currentTarget = nullptr;

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);
}

void QRhiD3D11::beginComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);

    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::ResetShaderResources;
    cbD->commands.append(cmd);

    cbD->recordingPass = QD3D11CommandBuffer::ComputePass;

    cbD->resetCachedShaderResourceState();
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

        QD3D11CommandBuffer::Command cmd;
        cmd.cmd = QD3D11CommandBuffer::Command::BindComputePipeline;
        cmd.args.bindComputePipeline.ps = psD;
        cbD->commands.append(cmd);
    }
}

void QRhiD3D11::dispatch(QRhiCommandBuffer *cb, int x, int y, int z)
{
    QD3D11CommandBuffer *cbD = QRHI_RES(QD3D11CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QD3D11CommandBuffer::ComputePass);

    QD3D11CommandBuffer::Command cmd;
    cmd.cmd = QD3D11CommandBuffer::Command::Dispatch;
    cmd.args.dispatch.x = UINT(x);
    cmd.args.dispatch.y = UINT(y);
    cmd.args.dispatch.z = UINT(z);
    cbD->commands.append(cmd);
}

static inline QPair<int, int> mapBinding(int binding,
                                         int stageIndex,
                                         const QShader::NativeResourceBindingMap *nativeResourceBindingMaps[])
{
    const QShader::NativeResourceBindingMap *map = nativeResourceBindingMaps[stageIndex];
    if (!map || map->isEmpty())
        return { binding, binding }; // old QShader versions do not have this map, assume 1:1 mapping then

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
    srbD->vsubufs.clear();
    srbD->vsubufoffsets.clear();
    srbD->vsubufsizes.clear();

    srbD->fsubufs.clear();
    srbD->fsubufoffsets.clear();
    srbD->fsubufsizes.clear();

    srbD->csubufs.clear();
    srbD->csubufoffsets.clear();
    srbD->csubufsizes.clear();

    srbD->vssamplers.clear();
    srbD->vsshaderresources.clear();

    srbD->fssamplers.clear();
    srbD->fsshaderresources.clear();

    srbD->cssamplers.clear();
    srbD->csshaderresources.clear();

    srbD->csUAVs.clear();

    struct Stage {
        struct Buffer {
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
    } res[RBM_SUPPORTED_STAGES];

    for (int i = 0, ie = srbD->sortedBindings.count(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = srbD->sortedBindings.at(i).data();
        QD3D11ShaderResourceBindings::BoundResourceData &bd(srbD->boundResourceData[i]);
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            QD3D11Buffer *bufD = QRHI_RES(QD3D11Buffer, b->u.ubuf.buf);
            Q_ASSERT(aligned(b->u.ubuf.offset, 256) == b->u.ubuf.offset);
            bd.ubuf.id = bufD->m_id;
            bd.ubuf.generation = bufD->generation;
            // dynamic ubuf offsets are not considered here, those are baked in
            // at a later stage, which is good as vsubufoffsets and friends are
            // per-srb, not per-setShaderResources call
            const uint offsetInConstants = uint(b->u.ubuf.offset) / 16;
            // size must be 16 mult. (in constants, i.e. multiple of 256 bytes).
            // We can round up if needed since the buffers's actual size
            // (ByteWidth) is always a multiple of 256.
            const uint sizeInConstants = uint(aligned(b->u.ubuf.maybeSize ? b->u.ubuf.maybeSize : bufD->m_size, 256) / 16);
            if (b->stage.testFlag(QRhiShaderResourceBinding::VertexStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_VERTEX, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0)
                    res[RBM_VERTEX].buffers.append({ nativeBinding.first, bufD->buffer, offsetInConstants, sizeInConstants });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::FragmentStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_FRAGMENT, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0)
                    res[RBM_FRAGMENT].buffers.append({ nativeBinding.first, bufD->buffer, offsetInConstants, sizeInConstants });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::ComputeStage)) {
                QPair<int, int> nativeBinding = mapBinding(b->binding, RBM_COMPUTE, nativeResourceBindingMaps);
                if (nativeBinding.first >= 0)
                    res[RBM_COMPUTE].buffers.append({ nativeBinding.first, bufD->buffer, offsetInConstants, sizeInConstants });
            }
        }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
        {
            const QRhiShaderResourceBinding::Data::SampledTextureData *data = &b->u.stex;
            bd.stex.count = data->count;
            const QPair<int, int> nativeBindingVert = mapBinding(b->binding, RBM_VERTEX, nativeResourceBindingMaps);
            const QPair<int, int> nativeBindingFrag = mapBinding(b->binding, RBM_FRAGMENT, nativeResourceBindingMaps);
            const QPair<int, int> nativeBindingComp = mapBinding(b->binding, RBM_COMPUTE, nativeResourceBindingMaps);
            // if SPIR-V binding b is mapped to tN and sN in HLSL, and it
            // is an array, then it will use tN, tN+1, tN+2, ..., and sN,
            // sN+1, sN+2, ...
            for (int elem = 0; elem < data->count; ++elem) {
                QD3D11Texture *texD = QRHI_RES(QD3D11Texture, data->texSamplers[elem].tex);
                QD3D11Sampler *samplerD = QRHI_RES(QD3D11Sampler, data->texSamplers[elem].sampler);
                bd.stex.d[elem].texId = texD->m_id;
                bd.stex.d[elem].texGeneration = texD->generation;
                bd.stex.d[elem].samplerId = samplerD->m_id;
                bd.stex.d[elem].samplerGeneration = samplerD->generation;
                if (b->stage.testFlag(QRhiShaderResourceBinding::VertexStage)) {
                    if (nativeBindingVert.first >= 0 && nativeBindingVert.second >= 0) {
                        res[RBM_VERTEX].textures.append({ nativeBindingVert.first + elem, texD->srv });
                        res[RBM_VERTEX].samplers.append({ nativeBindingVert.second + elem, samplerD->samplerState });
                    }
                }
                if (b->stage.testFlag(QRhiShaderResourceBinding::FragmentStage)) {
                    if (nativeBindingFrag.first >= 0 && nativeBindingFrag.second >= 0) {
                        res[RBM_FRAGMENT].textures.append({ nativeBindingFrag.first + elem, texD->srv });
                        res[RBM_FRAGMENT].samplers.append({ nativeBindingFrag.second + elem, samplerD->samplerState });
                    }
                }
                if (b->stage.testFlag(QRhiShaderResourceBinding::ComputeStage)) {
                    if (nativeBindingComp.first >= 0 && nativeBindingComp.second >= 0) {
                        res[RBM_COMPUTE].textures.append({ nativeBindingComp.first + elem, texD->srv });
                        res[RBM_COMPUTE].samplers.append({ nativeBindingComp.second + elem, samplerD->samplerState });
                    }
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
                    ID3D11UnorderedAccessView *uav = bufD->unorderedAccessView();
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

    for (const Stage::Buffer &buf : qAsConst(res[RBM_VERTEX].buffers)) {
        srbD->vsubufs.feed(buf.breg, buf.buffer);
        srbD->vsubufoffsets.feed(buf.breg, buf.offsetInConstants);
        srbD->vsubufsizes.feed(buf.breg, buf.sizeInConstants);
    }
    srbD->vsubufs.finish();
    srbD->vsubufoffsets.finish();
    srbD->vsubufsizes.finish();

    for (const Stage::Buffer &buf : qAsConst(res[RBM_FRAGMENT].buffers)) {
        srbD->fsubufs.feed(buf.breg, buf.buffer);
        srbD->fsubufoffsets.feed(buf.breg, buf.offsetInConstants);
        srbD->fsubufsizes.feed(buf.breg, buf.sizeInConstants);
    }
    srbD->fsubufs.finish();
    srbD->fsubufoffsets.finish();
    srbD->fsubufsizes.finish();

    for (const Stage::Buffer &buf : qAsConst(res[RBM_COMPUTE].buffers)) {
        srbD->csubufs.feed(buf.breg, buf.buffer);
        srbD->csubufoffsets.feed(buf.breg, buf.offsetInConstants);
        srbD->csubufsizes.feed(buf.breg, buf.sizeInConstants);
    }
    srbD->csubufs.finish();
    srbD->csubufoffsets.finish();
    srbD->csubufsizes.finish();

    for (const Stage::Texture &t : qAsConst(res[RBM_VERTEX].textures))
        srbD->vsshaderresources.feed(t.treg, t.srv);
    for (const Stage::Sampler &s : qAsConst(res[RBM_VERTEX].samplers))
        srbD->vssamplers.feed(s.sreg, s.sampler);
    srbD->vssamplers.finish();
    srbD->vsshaderresources.finish();

    for (const Stage::Texture &t : qAsConst(res[RBM_FRAGMENT].textures))
        srbD->fsshaderresources.feed(t.treg, t.srv);
    for (const Stage::Sampler &s : qAsConst(res[RBM_FRAGMENT].samplers))
        srbD->fssamplers.feed(s.sreg, s.sampler);
    srbD->fssamplers.finish();
    srbD->fsshaderresources.finish();

    for (const Stage::Texture &t : qAsConst(res[RBM_COMPUTE].textures))
        srbD->csshaderresources.feed(t.treg, t.srv);
    for (const Stage::Sampler &s : qAsConst(res[RBM_COMPUTE].samplers))
        srbD->cssamplers.feed(s.sreg, s.sampler);
    srbD->cssamplers.finish();
    srbD->csshaderresources.finish();

    for (const Stage::Uav &u : qAsConst(res[RBM_COMPUTE].uavs))
        srbD->csUAVs.feed(u.ureg, u.uav);
    srbD->csUAVs.finish();
}

void QRhiD3D11::executeBufferHostWrites(QD3D11Buffer *bufD)
{
    if (!bufD->hasPendingDynamicUpdates)
        return;

    Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
    bufD->hasPendingDynamicUpdates = false;
    D3D11_MAPPED_SUBRESOURCE mp;
    HRESULT hr = context->Map(bufD->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mp);
    if (SUCCEEDED(hr)) {
        memcpy(mp.pData, bufD->dynBuf.constData(), size_t(bufD->dynBuf.size()));
        context->Unmap(bufD->buffer, 0);
    } else {
        qWarning("Failed to map buffer: %s", qPrintable(comErrorMessage(hr)));
    }
}

static void applyDynamicOffsets(QVarLengthArray<UINT, 4> *offsets,
                                int batchIndex,
                                QRhiBatchedBindings<ID3D11Buffer *> *ubufs,
                                QRhiBatchedBindings<UINT> *ubufoffsets,
                                const uint *dynOfsPairs, int dynOfsPairCount)
{
    const int count = ubufs->batches[batchIndex].resources.count();
    const UINT startBinding = ubufs->batches[batchIndex].startBinding;
    *offsets = ubufoffsets->batches[batchIndex].resources;
    for (int b = 0; b < count; ++b) {
        for (int di = 0; di < dynOfsPairCount; ++di) {
            const uint binding = dynOfsPairs[2 * di];
            if (binding == startBinding + UINT(b)) {
                const uint offsetInConstants = dynOfsPairs[2 * di + 1];
                (*offsets)[b] = offsetInConstants;
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

void QRhiD3D11::bindShaderResources(QD3D11ShaderResourceBindings *srbD,
                                    const uint *dynOfsPairs, int dynOfsPairCount,
                                    bool offsetOnlyChange)
{
    if (!offsetOnlyChange) {
        for (const auto &batch : srbD->vssamplers.batches) {
            const uint count = clampedResourceCount(batch.startBinding, batch.resources.count(),
                                                    D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, "VS sampler");
            if (count)
                context->VSSetSamplers(batch.startBinding, count, batch.resources.constData());
        }

        for (const auto &batch : srbD->vsshaderresources.batches) {
            const uint count = clampedResourceCount(batch.startBinding, batch.resources.count(),
                                                    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, "VS SRV");
            if (count) {
                context->VSSetShaderResources(batch.startBinding, count, batch.resources.constData());
                contextState.vsHighestActiveSrvBinding = qMax(contextState.vsHighestActiveSrvBinding,
                                                            int(batch.startBinding + count) - 1);
            }
        }

        for (const auto &batch : srbD->fssamplers.batches) {
            const uint count = clampedResourceCount(batch.startBinding, batch.resources.count(),
                                                    D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, "PS sampler");
            if (count)
                context->PSSetSamplers(batch.startBinding, count, batch.resources.constData());
        }

        for (const auto &batch : srbD->fsshaderresources.batches) {
            const uint count = clampedResourceCount(batch.startBinding, batch.resources.count(),
                                                    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, "PS SRV");
            if (count) {
                context->PSSetShaderResources(batch.startBinding, count, batch.resources.constData());
                contextState.fsHighestActiveSrvBinding = qMax(contextState.fsHighestActiveSrvBinding,
                                                            int(batch.startBinding + count) - 1);
            }
        }

        for (const auto &batch : srbD->cssamplers.batches) {
            const uint count = clampedResourceCount(batch.startBinding, batch.resources.count(),
                                                    D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, "CS sampler");
            if (count)
                context->CSSetSamplers(batch.startBinding, count, batch.resources.constData());
        }

        for (const auto &batch : srbD->csshaderresources.batches) {
            const uint count = clampedResourceCount(batch.startBinding, batch.resources.count(),
                                                    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, "CS SRV");
            if (count) {
                context->CSSetShaderResources(batch.startBinding, count, batch.resources.constData());
                contextState.csHighestActiveSrvBinding = qMax(contextState.csHighestActiveSrvBinding,
                                                              int(batch.startBinding + count) - 1);
            }
        }
    }

    for (int i = 0, ie = srbD->vsubufs.batches.count(); i != ie; ++i) {
        const uint count = clampedResourceCount(srbD->vsubufs.batches[i].startBinding,
                                                srbD->vsubufs.batches[i].resources.count(),
                                                D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                                                "VS cbuf");
        if (count) {
            if (!dynOfsPairCount) {
                context->VSSetConstantBuffers1(srbD->vsubufs.batches[i].startBinding,
                                               count,
                                               srbD->vsubufs.batches[i].resources.constData(),
                                               srbD->vsubufoffsets.batches[i].resources.constData(),
                                               srbD->vsubufsizes.batches[i].resources.constData());
            } else {
                QVarLengthArray<UINT, 4> offsets;
                applyDynamicOffsets(&offsets, i, &srbD->vsubufs, &srbD->vsubufoffsets, dynOfsPairs, dynOfsPairCount);
                context->VSSetConstantBuffers1(srbD->vsubufs.batches[i].startBinding,
                                               count,
                                               srbD->vsubufs.batches[i].resources.constData(),
                                               offsets.constData(),
                                               srbD->vsubufsizes.batches[i].resources.constData());
            }
        }
    }

    for (int i = 0, ie = srbD->fsubufs.batches.count(); i != ie; ++i) {
        const uint count = clampedResourceCount(srbD->fsubufs.batches[i].startBinding,
                                                srbD->fsubufs.batches[i].resources.count(),
                                                D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                                                "PS cbuf");
        if (count) {
            if (!dynOfsPairCount) {
                context->PSSetConstantBuffers1(srbD->fsubufs.batches[i].startBinding,
                                               count,
                                               srbD->fsubufs.batches[i].resources.constData(),
                                               srbD->fsubufoffsets.batches[i].resources.constData(),
                                               srbD->fsubufsizes.batches[i].resources.constData());
            } else {
                QVarLengthArray<UINT, 4> offsets;
                applyDynamicOffsets(&offsets, i, &srbD->fsubufs, &srbD->fsubufoffsets, dynOfsPairs, dynOfsPairCount);
                context->PSSetConstantBuffers1(srbD->fsubufs.batches[i].startBinding,
                                               count,
                                               srbD->fsubufs.batches[i].resources.constData(),
                                               offsets.constData(),
                                               srbD->fsubufsizes.batches[i].resources.constData());
            }
        }
    }

    for (int i = 0, ie = srbD->csubufs.batches.count(); i != ie; ++i) {
        const uint count = clampedResourceCount(srbD->csubufs.batches[i].startBinding,
                                                srbD->csubufs.batches[i].resources.count(),
                                                D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                                                "CS cbuf");
        if (count) {
            if (!dynOfsPairCount) {
                context->CSSetConstantBuffers1(srbD->csubufs.batches[i].startBinding,
                                               count,
                                               srbD->csubufs.batches[i].resources.constData(),
                                               srbD->csubufoffsets.batches[i].resources.constData(),
                                               srbD->csubufsizes.batches[i].resources.constData());
            } else {
                QVarLengthArray<UINT, 4> offsets;
                applyDynamicOffsets(&offsets, i, &srbD->csubufs, &srbD->csubufoffsets, dynOfsPairs, dynOfsPairCount);
                context->CSSetConstantBuffers1(srbD->csubufs.batches[i].startBinding,
                                               count,
                                               srbD->csubufs.batches[i].resources.constData(),
                                               offsets.constData(),
                                               srbD->csubufsizes.batches[i].resources.constData());
            }
        }
    }

    for (const auto &batch : srbD->csUAVs.batches) {
        const uint count = clampedResourceCount(batch.startBinding, batch.resources.count(),
                                                D3D11_1_UAV_SLOT_COUNT, "CS UAV");
        if (count) {
            context->CSSetUnorderedAccessViews(batch.startBinding,
                                               count,
                                               batch.resources.constData(),
                                               nullptr);
            contextState.csHighestActiveUavBinding = qMax(contextState.csHighestActiveUavBinding,
                                                          int(batch.startBinding + count) - 1);
        }
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

void QRhiD3D11::executeCommandBuffer(QD3D11CommandBuffer *cbD, QD3D11SwapChain *timestampSwapChain)
{
    quint32 stencilRef = 0;
    float blendConstants[] = { 1, 1, 1, 1 };

    if (timestampSwapChain) {
        const int currentFrameSlot = timestampSwapChain->currentFrameSlot;
        ID3D11Query *tsDisjoint = timestampSwapChain->timestampDisjointQuery[currentFrameSlot];
        const int tsIdx = QD3D11SwapChain::BUFFER_COUNT * currentFrameSlot;
        ID3D11Query *tsStart = timestampSwapChain->timestampQuery[tsIdx];
        if (tsDisjoint && tsStart && !timestampSwapChain->timestampActive[currentFrameSlot]) {
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

    for (const QD3D11CommandBuffer::Command &cmd : qAsConst(cbD->commands)) {
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
            context->VSSetShader(psD->vs.shader, nullptr, 0);
            context->PSSetShader(psD->fs.shader, nullptr, 0);
            context->IASetPrimitiveTopology(psD->d3dTopology);
            context->IASetInputLayout(psD->inputLayout);
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
                                           cmd.args.copySubRes.dstX, cmd.args.copySubRes.dstY, 0,
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

QD3D11Buffer::QD3D11Buffer(QRhiImplementation *rhi, Type type, UsageFlags usage, int size)
    : QRhiBuffer(rhi, type, usage, size)
{
}

QD3D11Buffer::~QD3D11Buffer()
{
    release();
}

void QD3D11Buffer::release()
{
    if (!buffer)
        return;

    dynBuf.clear();

    buffer->Release();
    buffer = nullptr;

    if (uav) {
        uav->Release();
        uav = nullptr;
    }

    QRHI_RES_RHI(QRhiD3D11);
    QRHI_PROF;
    QRHI_PROF_F(releaseBuffer(this));
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

bool QD3D11Buffer::build()
{
    if (buffer)
        release();

    if (m_usage.testFlag(QRhiBuffer::UniformBuffer) && m_type != Dynamic) {
        qWarning("UniformBuffer must always be combined with Dynamic on D3D11");
        return false;
    }

    if (m_usage.testFlag(QRhiBuffer::StorageBuffer) && m_type == Dynamic) {
        qWarning("StorageBuffer cannot be combined with Dynamic");
        return false;
    }

    const int nonZeroSize = m_size <= 0 ? 256 : m_size;
    const int roundedSize = aligned(nonZeroSize, m_usage.testFlag(QRhiBuffer::UniformBuffer) ? 256 : 4);

    D3D11_BUFFER_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.ByteWidth = UINT(roundedSize);
    desc.Usage = m_type == Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.BindFlags = toD3DBufferUsage(m_usage);
    desc.CPUAccessFlags = m_type == Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.MiscFlags = m_usage.testFlag(QRhiBuffer::StorageBuffer) ? D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS : 0;

    QRHI_RES_RHI(QRhiD3D11);
    HRESULT hr = rhiD->dev->CreateBuffer(&desc, nullptr, &buffer);
    if (FAILED(hr)) {
        qWarning("Failed to create buffer: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    if (m_type == Dynamic) {
        dynBuf.resize(m_size);
        hasPendingDynamicUpdates = false;
    }

    if (!m_objectName.isEmpty())
        buffer->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(m_objectName.size()), m_objectName.constData());

    QRHI_PROF;
    QRHI_PROF_F(newBuffer(this, quint32(roundedSize), m_type == Dynamic ? 2 : 1, m_type == Dynamic ? 1 : 0));

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

ID3D11UnorderedAccessView *QD3D11Buffer::unorderedAccessView()
{
    if (uav)
        return uav;

    // SPIRV-Cross generated HLSL uses RWByteAddressBuffer
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = UINT(aligned(m_size, 4) / 4);
    desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

    QRHI_RES_RHI(QRhiD3D11);
    HRESULT hr = rhiD->dev->CreateUnorderedAccessView(buffer, &desc, &uav);
    if (FAILED(hr)) {
        qWarning("Failed to create UAV: %s", qPrintable(comErrorMessage(hr)));
        return nullptr;
    }

    return uav;
}

QD3D11RenderBuffer::QD3D11RenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                                       int sampleCount, QRhiRenderBuffer::Flags flags)
    : QRhiRenderBuffer(rhi, type, pixelSize, sampleCount, flags)
{
}

QD3D11RenderBuffer::~QD3D11RenderBuffer()
{
    release();
}

void QD3D11RenderBuffer::release()
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
    QRHI_PROF;
    QRHI_PROF_F(releaseRenderBuffer(this));
    rhiD->unregisterResource(this);
}

bool QD3D11RenderBuffer::build()
{
    if (tex)
        release();

    if (m_pixelSize.isEmpty())
        return false;

    QRHI_RES_RHI(QRhiD3D11);
    sampleDesc = rhiD->effectiveSampleCount(m_sampleCount);

    D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = UINT(m_pixelSize.width());
    desc.Height = UINT(m_pixelSize.height());
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc = sampleDesc;
    desc.Usage = D3D11_USAGE_DEFAULT;

    if (m_type == Color) {
        dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Format = dxgiFormat;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        HRESULT hr = rhiD->dev->CreateTexture2D(&desc, nullptr, &tex);
        if (FAILED(hr)) {
            qWarning("Failed to create color renderbuffer: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        memset(&rtvDesc, 0, sizeof(rtvDesc));
        rtvDesc.Format = dxgiFormat;
        rtvDesc.ViewDimension = desc.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS
                                                          : D3D11_RTV_DIMENSION_TEXTURE2D;
        hr = rhiD->dev->CreateRenderTargetView(tex, &rtvDesc, &rtv);
        if (FAILED(hr)) {
            qWarning("Failed to create rtv: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }
    } else if (m_type == DepthStencil) {
        dxgiFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.Format = dxgiFormat;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        HRESULT hr = rhiD->dev->CreateTexture2D(&desc, nullptr, &tex);
        if (FAILED(hr)) {
            qWarning("Failed to create depth-stencil buffer: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        memset(&dsvDesc, 0, sizeof(dsvDesc));
        dsvDesc.Format = dxgiFormat;
        dsvDesc.ViewDimension = desc.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS
                                                          : D3D11_DSV_DIMENSION_TEXTURE2D;
        hr = rhiD->dev->CreateDepthStencilView(tex, &dsvDesc, &dsv);
        if (FAILED(hr)) {
            qWarning("Failed to create dsv: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }
    } else {
        return false;
    }

    if (!m_objectName.isEmpty())
        tex->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(m_objectName.size()), m_objectName.constData());

    QRHI_PROF;
    QRHI_PROF_F(newRenderBuffer(this, false, false, int(sampleDesc.Count)));

    rhiD->registerResource(this);
    return true;
}

QRhiTexture::Format QD3D11RenderBuffer::backingFormat() const
{
    return m_type == Color ? QRhiTexture::RGBA8 : QRhiTexture::UnknownFormat;
}

QD3D11Texture::QD3D11Texture(QRhiImplementation *rhi, Format format, const QSize &pixelSize,
                             int sampleCount, Flags flags)
    : QRhiTexture(rhi, format, pixelSize, sampleCount, flags)
{
    for (int i = 0; i < QRhi::MAX_LEVELS; ++i)
        perLevelViews[i] = nullptr;
}

QD3D11Texture::~QD3D11Texture()
{
    release();
}

void QD3D11Texture::release()
{
    if (!tex)
        return;

    if (srv) {
        srv->Release();
        srv = nullptr;
    }

    for (int i = 0; i < QRhi::MAX_LEVELS; ++i) {
        if (perLevelViews[i]) {
            perLevelViews[i]->Release();
            perLevelViews[i] = nullptr;
        }
    }

    if (owns)
        tex->Release();

    tex = nullptr;

    QRHI_RES_RHI(QRhiD3D11);
    QRHI_PROF;
    QRHI_PROF_F(releaseTexture(this));
    rhiD->unregisterResource(this);
}

static inline DXGI_FORMAT toD3DDepthTextureSRVFormat(QRhiTexture::Format format)
{
    switch (format) {
    case QRhiTexture::Format::D16:
        return DXGI_FORMAT_R16_FLOAT;
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
    case QRhiTexture::Format::D32F:
        return DXGI_FORMAT_D32_FLOAT;
    default:
        Q_UNREACHABLE();
        return DXGI_FORMAT_D32_FLOAT;
    }
}

bool QD3D11Texture::prepareBuild(QSize *adjustedSize)
{
    if (tex)
        release();

    const QSize size = m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize;
    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);

    QRHI_RES_RHI(QRhiD3D11);
    dxgiFormat = toD3DTextureFormat(m_format, m_flags);
    mipLevelCount = uint(hasMipMaps ? rhiD->q->mipLevelsForSize(size) : 1);
    sampleDesc = rhiD->effectiveSampleCount(m_sampleCount);
    if (sampleDesc.Count > 1) {
        if (isCube) {
            qWarning("Cubemap texture cannot be multisample");
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

    if (adjustedSize)
        *adjustedSize = size;

    return true;
}

bool QD3D11Texture::finishBuild()
{
    QRHI_RES_RHI(QRhiD3D11);
    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    memset(&srvDesc, 0, sizeof(srvDesc));
    srvDesc.Format = isDepth ? toD3DDepthTextureSRVFormat(m_format) : dxgiFormat;
    if (isCube) {
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = mipLevelCount;
    } else {
        if (sampleDesc.Count > 1) {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        } else {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = mipLevelCount;
        }
    }

    HRESULT hr = rhiD->dev->CreateShaderResourceView(tex, &srvDesc, &srv);
    if (FAILED(hr)) {
        qWarning("Failed to create srv: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    generation += 1;
    return true;
}

bool QD3D11Texture::build()
{
    QSize size;
    if (!prepareBuild(&size))
        return false;

    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);

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

    D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = UINT(size.width());
    desc.Height = UINT(size.height());
    desc.MipLevels = mipLevelCount;
    desc.ArraySize = isCube ? 6 : 1;
    desc.Format = dxgiFormat;
    desc.SampleDesc = sampleDesc;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = bindFlags;
    desc.MiscFlags = miscFlags;

    QRHI_RES_RHI(QRhiD3D11);
    HRESULT hr = rhiD->dev->CreateTexture2D(&desc, nullptr, &tex);
    if (FAILED(hr)) {
        qWarning("Failed to create texture: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    if (!finishBuild())
        return false;

    if (!m_objectName.isEmpty())
        tex->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(m_objectName.size()), m_objectName.constData());

    QRHI_PROF;
    QRHI_PROF_F(newTexture(this, true, int(mipLevelCount), isCube ? 6 : 1, int(sampleDesc.Count)));

    owns = true;
    rhiD->registerResource(this);
    return true;
}

bool QD3D11Texture::buildFrom(QRhiTexture::NativeTexture src)
{
    auto *srcTex = static_cast<ID3D11Texture2D * const *>(src.object);
    if (!srcTex || !*srcTex)
        return false;

    if (!prepareBuild())
        return false;

    tex = *srcTex;

    if (!finishBuild())
        return false;

    QRHI_PROF;
    QRHI_PROF_F(newTexture(this, false, int(mipLevelCount), m_flags.testFlag(CubeMap) ? 6 : 1, int(sampleDesc.Count)));

    owns = false;
    QRHI_RES_RHI(QRhiD3D11);
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::NativeTexture QD3D11Texture::nativeTexture()
{
    return {&tex, 0};
}

ID3D11UnorderedAccessView *QD3D11Texture::unorderedAccessViewForLevel(int level)
{
    if (perLevelViews[level])
        return perLevelViews[level];

    const bool isCube = m_flags.testFlag(CubeMap);
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Format = dxgiFormat;
    if (isCube) {
        desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.MipSlice = UINT(level);
        desc.Texture2DArray.FirstArraySlice = 0;
        desc.Texture2DArray.ArraySize = 6;
    } else {
        desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = UINT(level);
    }

    QRHI_RES_RHI(QRhiD3D11);
    ID3D11UnorderedAccessView *uav = nullptr;
    HRESULT hr = rhiD->dev->CreateUnorderedAccessView(tex, &desc, &uav);
    if (FAILED(hr)) {
        qWarning("Failed to create UAV: %s", qPrintable(comErrorMessage(hr)));
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
    release();
}

void QD3D11Sampler::release()
{
    if (!samplerState)
        return;

    samplerState->Release();
    samplerState = nullptr;

    QRHI_RES_RHI(QRhiD3D11);
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

bool QD3D11Sampler::build()
{
    if (samplerState)
        release();

    D3D11_SAMPLER_DESC desc;
    memset(&desc, 0, sizeof(desc));
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
        qWarning("Failed to create sampler state: %s", qPrintable(comErrorMessage(hr)));
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
    release();
}

void QD3D11RenderPassDescriptor::release()
{
    // nothing to do here
}

bool QD3D11RenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const
{
    Q_UNUSED(other);
    return true;
}

QD3D11ReferenceRenderTarget::QD3D11ReferenceRenderTarget(QRhiImplementation *rhi)
    : QRhiRenderTarget(rhi),
      d(rhi)
{
}

QD3D11ReferenceRenderTarget::~QD3D11ReferenceRenderTarget()
{
    release();
}

void QD3D11ReferenceRenderTarget::release()
{
    // nothing to do here
}

QSize QD3D11ReferenceRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QD3D11ReferenceRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QD3D11ReferenceRenderTarget::sampleCount() const
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
    release();
}

void QD3D11TextureRenderTarget::release()
{
    QRHI_RES_RHI(QRhiD3D11);

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

    rhiD->unregisterResource(this);
}

QRhiRenderPassDescriptor *QD3D11TextureRenderTarget::newCompatibleRenderPassDescriptor()
{
    return new QD3D11RenderPassDescriptor(m_rhi);
}

bool QD3D11TextureRenderTarget::build()
{
    if (rtv[0] || dsv)
        release();

    const bool hasColorAttachments = m_desc.cbeginColorAttachments() != m_desc.cendColorAttachments();
    Q_ASSERT(hasColorAttachments || m_desc.depthTexture());
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
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            memset(&rtvDesc, 0, sizeof(rtvDesc));
            rtvDesc.Format = toD3DTextureFormat(texD->format(), texD->flags());
            if (texD->flags().testFlag(QRhiTexture::CubeMap)) {
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = UINT(colorAtt.level());
                rtvDesc.Texture2DArray.FirstArraySlice = UINT(colorAtt.layer());
                rtvDesc.Texture2DArray.ArraySize = 1;
            } else {
                if (texD->sampleDesc.Count > 1) {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                } else {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    rtvDesc.Texture2D.MipSlice = UINT(colorAtt.level());
                }
            }
            HRESULT hr = rhiD->dev->CreateRenderTargetView(texD->tex, &rtvDesc, &rtv[attIndex]);
            if (FAILED(hr)) {
                qWarning("Failed to create rtv: %s", qPrintable(comErrorMessage(hr)));
                return false;
            }
            ownsRtv[attIndex] = true;
            if (attIndex == 0) {
                d.pixelSize = texD->pixelSize();
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
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
            memset(&dsvDesc, 0, sizeof(dsvDesc));
            dsvDesc.Format = toD3DDepthTextureDSVFormat(depthTexD->format());
            dsvDesc.ViewDimension = depthTexD->sampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS
                                                                    : D3D11_DSV_DIMENSION_TEXTURE2D;
            HRESULT hr = rhiD->dev->CreateDepthStencilView(depthTexD->tex, &dsvDesc, &dsv);
            if (FAILED(hr)) {
                qWarning("Failed to create dsv: %s", qPrintable(comErrorMessage(hr)));
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

    rhiD->registerResource(this);
    return true;
}

QSize QD3D11TextureRenderTarget::pixelSize() const
{
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
    release();
}

void QD3D11ShaderResourceBindings::release()
{
    sortedBindings.clear();
    boundResourceData.clear();
}

bool QD3D11ShaderResourceBindings::build()
{
    if (!sortedBindings.isEmpty())
        release();

    std::copy(m_bindings.cbegin(), m_bindings.cend(), std::back_inserter(sortedBindings));
    std::sort(sortedBindings.begin(), sortedBindings.end(),
              [](const QRhiShaderResourceBinding &a, const QRhiShaderResourceBinding &b)
    {
        return a.data()->binding < b.data()->binding;
    });

    boundResourceData.resize(sortedBindings.count());

    for (BoundResourceData &bd : boundResourceData)
        memset(&bd, 0, sizeof(BoundResourceData));

    generation += 1;
    return true;
}

QD3D11GraphicsPipeline::QD3D11GraphicsPipeline(QRhiImplementation *rhi)
    : QRhiGraphicsPipeline(rhi)
{
}

QD3D11GraphicsPipeline::~QD3D11GraphicsPipeline()
{
    release();
}

void QD3D11GraphicsPipeline::release()
{
    QRHI_RES_RHI(QRhiD3D11);

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

    if (vs.shader) {
        vs.shader->Release();
        vs.shader = nullptr;
    }
    vs.nativeResourceBindingMap.clear();

    if (fs.shader) {
        fs.shader->Release();
        fs.shader = nullptr;
    }
    fs.nativeResourceBindingMap.clear();

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
    default:
        Q_UNREACHABLE();
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }
}

static inline D3D11_PRIMITIVE_TOPOLOGY toD3DTopology(QRhiGraphicsPipeline::Topology t)
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

static pD3DCompile resolveD3DCompile()
{
    for (const wchar_t *libraryName : {L"D3DCompiler_47", L"D3DCompiler_43"}) {
        QSystemLibrary library(libraryName);
        if (library.load()) {
            if (auto symbol = library.resolve("D3DCompile"))
                return reinterpret_cast<pD3DCompile>(symbol);
        }
    }
    return nullptr;
}

static QByteArray compileHlslShaderSource(const QShader &shader, QShader::Variant shaderVariant, QString *error, QShaderKey *usedShaderKey)
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

    static const pD3DCompile d3dCompile = resolveD3DCompile();
    if (d3dCompile == nullptr) {
        qWarning("Unable to resolve function D3DCompile()");
        return QByteArray();
    }

    ID3DBlob *bytecode = nullptr;
    ID3DBlob *errors = nullptr;
    HRESULT hr = d3dCompile(hlslSource.shader().constData(), SIZE_T(hlslSource.shader().size()),
                            nullptr, nullptr, nullptr,
                            hlslSource.entryPoint().constData(), target, 0, 0, &bytecode, &errors);
    if (FAILED(hr) || !bytecode) {
        qWarning("HLSL shader compilation failed: 0x%x", uint(hr));
        if (errors) {
            *error = QString::fromUtf8(static_cast<const char *>(errors->GetBufferPointer()),
                                       int(errors->GetBufferSize()));
            errors->Release();
        }
        return QByteArray();
    }

    if (usedShaderKey)
        *usedShaderKey = key;

    QByteArray result;
    result.resize(int(bytecode->GetBufferSize()));
    memcpy(result.data(), bytecode->GetBufferPointer(), size_t(result.size()));
    bytecode->Release();
    return result;
}

bool QD3D11GraphicsPipeline::build()
{
    if (dsState)
        release();

    QRHI_RES_RHI(QRhiD3D11);
    if (!rhiD->sanityCheckGraphicsPipeline(this))
        return false;

    D3D11_RASTERIZER_DESC rastDesc;
    memset(&rastDesc, 0, sizeof(rastDesc));
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = toD3DCullMode(m_cullMode);
    rastDesc.FrontCounterClockwise = m_frontFace == CCW;
    rastDesc.DepthBias = m_depthBias;
    rastDesc.SlopeScaledDepthBias = m_slopeScaledDepthBias;
    rastDesc.DepthClipEnable = true;
    rastDesc.ScissorEnable = m_flags.testFlag(UsesScissor);
    rastDesc.MultisampleEnable = rhiD->effectiveSampleCount(m_sampleCount).Count > 1;
    HRESULT hr = rhiD->dev->CreateRasterizerState(&rastDesc, &rastState);
    if (FAILED(hr)) {
        qWarning("Failed to create rasterizer state: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC dsDesc;
    memset(&dsDesc, 0, sizeof(dsDesc));
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
        qWarning("Failed to create depth-stencil state: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    D3D11_BLEND_DESC blendDesc;
    memset(&blendDesc, 0, sizeof(blendDesc));
    blendDesc.IndependentBlendEnable = m_targetBlends.count() > 1;
    for (int i = 0, ie = m_targetBlends.count(); i != ie; ++i) {
        const QRhiGraphicsPipeline::TargetBlend &b(m_targetBlends[i]);
        D3D11_RENDER_TARGET_BLEND_DESC blend;
        memset(&blend, 0, sizeof(blend));
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
        D3D11_RENDER_TARGET_BLEND_DESC blend;
        memset(&blend, 0, sizeof(blend));
        blend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[0] = blend;
    }
    hr = rhiD->dev->CreateBlendState(&blendDesc, &blendState);
    if (FAILED(hr)) {
        qWarning("Failed to create blend state: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    QByteArray vsByteCode;
    for (const QRhiShaderStage &shaderStage : qAsConst(m_shaderStages)) {
        auto cacheIt = rhiD->m_shaderCache.constFind(shaderStage);
        if (cacheIt != rhiD->m_shaderCache.constEnd()) {
            switch (shaderStage.type()) {
            case QRhiShaderStage::Vertex:
                vs.shader = static_cast<ID3D11VertexShader *>(cacheIt->s);
                vs.shader->AddRef();
                vsByteCode = cacheIt->bytecode;
                vs.nativeResourceBindingMap = cacheIt->nativeResourceBindingMap;
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
            const QByteArray bytecode = compileHlslShaderSource(shaderStage.shader(), shaderStage.shaderVariant(), &error, &shaderKey);
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
                    qWarning("Failed to create vertex shader: %s", qPrintable(comErrorMessage(hr)));
                    return false;
                }
                vsByteCode = bytecode;
                if (const QShader::NativeResourceBindingMap *map = shaderStage.shader().nativeResourceBindingMap(shaderKey))
                    vs.nativeResourceBindingMap = *map;
                rhiD->m_shaderCache.insert(shaderStage, QRhiD3D11::Shader(vs.shader, bytecode, vs.nativeResourceBindingMap));
                vs.shader->AddRef();
                break;
            case QRhiShaderStage::Fragment:
                hr = rhiD->dev->CreatePixelShader(bytecode.constData(), SIZE_T(bytecode.size()), nullptr, &fs.shader);
                if (FAILED(hr)) {
                    qWarning("Failed to create pixel shader: %s", qPrintable(comErrorMessage(hr)));
                    return false;
                }
                if (const QShader::NativeResourceBindingMap *map = shaderStage.shader().nativeResourceBindingMap(shaderKey))
                    fs.nativeResourceBindingMap = *map;
                rhiD->m_shaderCache.insert(shaderStage, QRhiD3D11::Shader(fs.shader, bytecode, fs.nativeResourceBindingMap));
                fs.shader->AddRef();
                break;
            default:
                break;
            }
        }
    }

    d3dTopology = toD3DTopology(m_topology);

    if (!vsByteCode.isEmpty()) {
        QVarLengthArray<D3D11_INPUT_ELEMENT_DESC, 4> inputDescs;
        for (auto it = m_vertexInputLayout.cbeginAttributes(), itEnd = m_vertexInputLayout.cendAttributes();
             it != itEnd; ++it)
        {
            D3D11_INPUT_ELEMENT_DESC desc;
            memset(&desc, 0, sizeof(desc));
            // the output from SPIRV-Cross uses TEXCOORD<location> as the semantic
            desc.SemanticName = "TEXCOORD";
            desc.SemanticIndex = UINT(it->location());
            desc.Format = toD3DAttributeFormat(it->format());
            desc.InputSlot = UINT(it->binding());
            desc.AlignedByteOffset = it->offset();
            const QRhiVertexInputBinding *inputBinding = m_vertexInputLayout.bindingAt(it->binding());
            if (inputBinding->classification() == QRhiVertexInputBinding::PerInstance) {
                desc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                desc.InstanceDataStepRate = UINT(inputBinding->instanceStepRate());
            } else {
                desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            }
            inputDescs.append(desc);
        }
        hr = rhiD->dev->CreateInputLayout(inputDescs.constData(), UINT(inputDescs.count()),
                                          vsByteCode, SIZE_T(vsByteCode.size()), &inputLayout);
        if (FAILED(hr)) {
            qWarning("Failed to create input layout: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }
    }

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
    release();
}

void QD3D11ComputePipeline::release()
{
    QRHI_RES_RHI(QRhiD3D11);

    if (!cs.shader)
        return;

    cs.shader->Release();
    cs.shader = nullptr;
    cs.nativeResourceBindingMap.clear();

    rhiD->unregisterResource(this);
}

bool QD3D11ComputePipeline::build()
{
    if (cs.shader)
        release();

    QRHI_RES_RHI(QRhiD3D11);

    auto cacheIt = rhiD->m_shaderCache.constFind(m_shaderStage);
    if (cacheIt != rhiD->m_shaderCache.constEnd()) {
        cs.shader = static_cast<ID3D11ComputeShader *>(cacheIt->s);
        cs.nativeResourceBindingMap = cacheIt->nativeResourceBindingMap;
    } else {
        QString error;
        QShaderKey shaderKey;
        const QByteArray bytecode = compileHlslShaderSource(m_shaderStage.shader(), m_shaderStage.shaderVariant(), &error, &shaderKey);
        if (bytecode.isEmpty()) {
            qWarning("HLSL compute shader compilation failed: %s", qPrintable(error));
            return false;
        }

        HRESULT hr = rhiD->dev->CreateComputeShader(bytecode.constData(), SIZE_T(bytecode.size()), nullptr, &cs.shader);
        if (FAILED(hr)) {
            qWarning("Failed to create compute shader: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }

        if (const QShader::NativeResourceBindingMap *map = m_shaderStage.shader().nativeResourceBindingMap(shaderKey))
            cs.nativeResourceBindingMap = *map;

        if (rhiD->m_shaderCache.count() >= QRhiD3D11::MAX_SHADER_CACHE_ENTRIES)
            rhiD->clearShaderCache();

        rhiD->m_shaderCache.insert(m_shaderStage, QRhiD3D11::Shader(cs.shader, bytecode, cs.nativeResourceBindingMap));
    }

    cs.shader->AddRef();

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
    release();
}

void QD3D11CommandBuffer::release()
{
    // nothing to do here
}

QD3D11SwapChain::QD3D11SwapChain(QRhiImplementation *rhi)
    : QRhiSwapChain(rhi),
      rt(rhi),
      cb(rhi)
{
    backBufferTex = nullptr;
    backBufferRtv = nullptr;
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        msaaTex[i] = nullptr;
        msaaRtv[i] = nullptr;
        timestampActive[i] = false;
        timestampDisjointQuery[i] = nullptr;
        timestampQuery[2 * i] = nullptr;
        timestampQuery[2 * i + 1] = nullptr;
    }
}

QD3D11SwapChain::~QD3D11SwapChain()
{
    release();
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

void QD3D11SwapChain::release()
{
    if (!swapChain)
        return;

    releaseBuffers();

    for (int i = 0; i < BUFFER_COUNT; ++i) {
        if (timestampDisjointQuery[i]) {
            timestampDisjointQuery[i]->Release();
            timestampDisjointQuery[i] = nullptr;
        }
        for (int j = 0; j < 2; ++j) {
            const int idx = BUFFER_COUNT * i + j;
            if (timestampQuery[idx]) {
                timestampQuery[idx]->Release();
                timestampQuery[idx] = nullptr;
            }
        }
    }

    swapChain->Release();
    swapChain = nullptr;

    QRHI_PROF;
    QRHI_PROF_F(releaseSwapChain(this));

    QRHI_RES_RHI(QRhiD3D11);
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

QRhiRenderPassDescriptor *QD3D11SwapChain::newCompatibleRenderPassDescriptor()
{
    return new QD3D11RenderPassDescriptor(m_rhi);
}

bool QD3D11SwapChain::newColorBuffer(const QSize &size, DXGI_FORMAT format, DXGI_SAMPLE_DESC sampleDesc,
                                     ID3D11Texture2D **tex, ID3D11RenderTargetView **rtv) const
{
    D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
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
        qWarning("Failed to create color buffer texture: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    memset(&rtvDesc, 0, sizeof(rtvDesc));
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = sampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = rhiD->dev->CreateRenderTargetView(*tex, &rtvDesc, rtv);
    if (FAILED(hr)) {
        qWarning("Failed to create color buffer rtv: %s", qPrintable(comErrorMessage(hr)));
        (*tex)->Release();
        *tex = nullptr;
        return false;
    }

    return true;
}

bool QD3D11SwapChain::buildOrResize()
{
    // Can be called multiple times due to window resizes - that is not the
    // same as a simple release+build (as with other resources). Just need to
    // resize the buffers then.

    const bool needsRegistration = !window || window != m_window;

    // except if the window actually changes
    if (window && window != m_window)
        release();

    window = m_window;
    m_currentPixelSize = surfacePixelSize();
    pixelSize = m_currentPixelSize;

    if (pixelSize.isEmpty())
        return false;

    colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    const DXGI_FORMAT srgbAdjustedFormat = m_flags.testFlag(sRGB) ?
                DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

    const UINT swapChainFlags = 0;

    QRHI_RES_RHI(QRhiD3D11);
    bool useFlipDiscard = rhiD->hasDxgi2 && rhiD->supportsFlipDiscardSwapchain;
    if (!swapChain) {
        HWND hwnd = reinterpret_cast<HWND>(window->winId());
        sampleDesc = rhiD->effectiveSampleCount(m_sampleCount);

        // Take a shortcut for alpha: our QWindow is OpenGLSurface so whatever
        // the platform plugin does to enable transparency for OpenGL window
        // will be sufficient for us too on the legacy (DISCARD) path. For
        // FLIP_DISCARD we'd need to use DirectComposition (create a
        // IDCompositionDevice/Target/Visual), avoid that for now.
        if (m_flags.testFlag(SurfaceHasPreMulAlpha) || m_flags.testFlag(SurfaceHasNonPreMulAlpha)) {
            useFlipDiscard = false;
            if (window->requestedFormat().alphaBufferSize() <= 0)
                qWarning("Swapchain says surface has alpha but the window has no alphaBufferSize set. "
                         "This may lead to problems.");
        }

        HRESULT hr;
        if (useFlipDiscard) {
            // We use FLIP_DISCARD which implies a buffer count of 2 (as opposed to the
            // old DISCARD with back buffer count == 1). This makes no difference for
            // the rest of the stuff except that automatic MSAA is unsupported and
            // needs to be implemented via a custom multisample render target and an
            // explicit resolve.

            DXGI_SWAP_CHAIN_DESC1 desc;
            memset(&desc, 0, sizeof(desc));
            desc.Width = UINT(pixelSize.width());
            desc.Height = UINT(pixelSize.height());
            desc.Format = colorFormat;
            desc.SampleDesc.Count = 1;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferCount = BUFFER_COUNT;
            desc.Scaling = DXGI_SCALING_STRETCH;
            desc.SwapEffect = DXGI_SWAP_EFFECT(4); // DXGI_SWAP_EFFECT_FLIP_DISCARD
            // Do not bother with AlphaMode, if won't work unless we go through
            // DirectComposition. Instead, we just take the other (DISCARD)
            // path for now when alpha is requested.
            desc.Flags = swapChainFlags;

            IDXGISwapChain1 *sc1;
            hr = static_cast<IDXGIFactory2 *>(rhiD->dxgiFactory)->CreateSwapChainForHwnd(rhiD->dev, hwnd, &desc,
                                                                                         nullptr, nullptr, &sc1);
            if (SUCCEEDED(hr))
                swapChain = sc1;
        } else {
            // Windows 7 for instance. Use DISCARD mode. Regardless, keep on
            // using our manual resolve for symmetry with the FLIP_DISCARD code
            // path when MSAA is requested.

            DXGI_SWAP_CHAIN_DESC desc;
            memset(&desc, 0, sizeof(desc));
            desc.BufferDesc.Width = UINT(pixelSize.width());
            desc.BufferDesc.Height = UINT(pixelSize.height());
            desc.BufferDesc.RefreshRate.Numerator = 60;
            desc.BufferDesc.RefreshRate.Denominator = 1;
            desc.BufferDesc.Format = colorFormat;
            desc.SampleDesc.Count = 1;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferCount = 1;
            desc.OutputWindow = hwnd;
            desc.Windowed = true;
            desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            desc.Flags = swapChainFlags;

            hr = rhiD->dxgiFactory->CreateSwapChain(rhiD->dev, &desc, &swapChain);
        }
        if (FAILED(hr)) {
            qWarning("Failed to create D3D11 swapchain: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }
        rhiD->dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    } else {
        releaseBuffers();
        const UINT count = useFlipDiscard ? BUFFER_COUNT : 1;
        HRESULT hr = swapChain->ResizeBuffers(count, UINT(pixelSize.width()), UINT(pixelSize.height()),
                                              colorFormat, swapChainFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            qWarning("Device loss detected in ResizeBuffers()");
            rhiD->deviceLost = true;
            return false;
        } else if (FAILED(hr)) {
            qWarning("Failed to resize D3D11 swapchain: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }
    }

    // This looks odd (for FLIP_DISCARD, esp. compared with backends for Vulkan
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
    HRESULT hr = swapChain->GetBuffer(0, IID_ID3D11Texture2D, reinterpret_cast<void **>(&backBufferTex));
    if (FAILED(hr)) {
        qWarning("Failed to query swapchain backbuffer: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    memset(&rtvDesc, 0, sizeof(rtvDesc));
    rtvDesc.Format = srgbAdjustedFormat;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = rhiD->dev->CreateRenderTargetView(backBufferTex, &rtvDesc, &backBufferRtv);
    if (FAILED(hr)) {
        qWarning("Failed to create rtv for swapchain backbuffer: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    // Try to reduce stalls by having a dedicated MSAA texture per swapchain buffer.
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        if (sampleDesc.Count > 1) {
            if (!newColorBuffer(pixelSize, srgbAdjustedFormat, sampleDesc, &msaaTex[i], &msaaRtv[i]))
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
            if (!m_depthStencil->build())
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
    swapInterval = m_flags.testFlag(QRhiSwapChain::NoVSync) ? 0 : 1;

    QD3D11ReferenceRenderTarget *rtD = QRHI_RES(QD3D11ReferenceRenderTarget, &rt);
    rtD->d.rp = QRHI_RES(QD3D11RenderPassDescriptor, m_renderPassDesc);
    rtD->d.pixelSize = pixelSize;
    rtD->d.dpr = float(window->devicePixelRatio());
    rtD->d.sampleCount = int(sampleDesc.Count);
    rtD->d.colorAttCount = 1;
    rtD->d.dsAttCount = m_depthStencil ? 1 : 0;

    QRHI_PROF;
    QRHI_PROF_F(resizeSwapChain(this, BUFFER_COUNT, sampleDesc.Count > 1 ? BUFFER_COUNT : 0, int(sampleDesc.Count)));
    if (rhiP) {
        D3D11_QUERY_DESC queryDesc;
        memset(&queryDesc, 0, sizeof(queryDesc));
        for (int i = 0; i < BUFFER_COUNT; ++i) {
            if (!timestampDisjointQuery[i]) {
                queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
                HRESULT hr = rhiD->dev->CreateQuery(&queryDesc, &timestampDisjointQuery[i]);
                if (FAILED(hr)) {
                    qWarning("Failed to create timestamp disjoint query: %s", qPrintable(comErrorMessage(hr)));
                    break;
                }
            }
            queryDesc.Query = D3D11_QUERY_TIMESTAMP;
            for (int j = 0; j < 2; ++j) {
                const int idx = BUFFER_COUNT * i + j; // one pair per buffer (frame)
                if (!timestampQuery[idx]) {
                    HRESULT hr = rhiD->dev->CreateQuery(&queryDesc, &timestampQuery[idx]);
                    if (FAILED(hr)) {
                        qWarning("Failed to create timestamp query: %s", qPrintable(comErrorMessage(hr)));
                        break;
                    }
                }
            }
        }
        // timestamp queries are optional so we can go on even if they failed
    }

    if (needsRegistration)
        rhiD->registerResource(this);

    return true;
}

void QRhiD3D11::DeviceCurse::initResources()
{
    framesLeft = framesToActivate;

    HRESULT hr = q->dev->CreateComputeShader(g_killDeviceByTimingOut, sizeof(g_killDeviceByTimingOut), nullptr, &cs);
    if (FAILED(hr)) {
        qWarning("Failed to create compute shader: %s", qPrintable(comErrorMessage(hr)));
        return;
    }
}

void QRhiD3D11::DeviceCurse::releaseResources()
{
    if (cs) {
        cs->Release();
        cs = nullptr;
    }
}

void QRhiD3D11::DeviceCurse::activate()
{
    if (!cs)
        return;

    qDebug("Activating Curse. Goodbye Cruel World.");

    q->context->CSSetShader(cs, nullptr, 0);
    q->context->Dispatch(256, 1, 1);
}

QT_END_NAMESPACE
