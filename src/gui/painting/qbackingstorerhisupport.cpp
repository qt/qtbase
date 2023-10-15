// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbackingstorerhisupport_p.h"
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

#if QT_CONFIG(opengl)
#include <QtGui/qoffscreensurface.h>
#include <QtGui/private/qopenglcontext_p.h>
#endif

#if QT_CONFIG(vulkan)
#include <QtGui/private/qvulkandefaultinstance_p.h>
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaBackingStore)

QBackingStoreRhiSupport::~QBackingStoreRhiSupport()
{
    reset();
}

void QBackingStoreRhiSupport::SwapchainData::reset()
{
    delete swapchain;
    delete renderPassDescriptor;
    delete windowWatcher;
    *this = {};
}

void QBackingStoreRhiSupport::reset()
{
    for (SwapchainData &d : m_swapchains)
        d.reset();

    m_swapchains.clear();

    delete m_rhi;
    m_rhi = nullptr;

    delete m_openGLFallbackSurface;
    m_openGLFallbackSurface = nullptr;
}

bool QBackingStoreRhiSupport::create()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RhiBasedRendering))
        return false;

    // note: m_window may be null (special case for fully offscreen rendering)

    QRhi *rhi = nullptr;
    QOffscreenSurface *surface = nullptr;
    QRhi::Flags flags;

    // These must be the same env.vars Qt Quick uses (as documented), in order
    // to ensure symmetry in the behavior between a QQuickWindow and a
    // (QRhi-based) widget top-level window.
    if (qEnvironmentVariableIntValue("QSG_RHI_PREFER_SOFTWARE_RENDERER"))
        flags |= QRhi::PreferSoftwareRenderer;
    if (qEnvironmentVariableIntValue("QSG_RHI_PROFILE"))
        flags |= QRhi::EnableDebugMarkers | QRhi::EnableTimestamps;

    if (m_config.api() == QPlatformBackingStoreRhiConfig::Null) {
        QRhiNullInitParams params;
        rhi = QRhi::create(QRhi::Null, &params, flags);
    }

#if QT_CONFIG(opengl)
    if (!rhi && m_config.api() == QPlatformBackingStoreRhiConfig::OpenGL) {
        surface = QRhiGles2InitParams::newFallbackSurface(m_format);
        QRhiGles2InitParams params;
        params.fallbackSurface = surface;
        params.window = m_window;
        params.format = m_format;
        params.shareContext = qt_gl_global_share_context();
        rhi = QRhi::create(QRhi::OpenGLES2, &params, flags);
    }
#endif

#ifdef Q_OS_WIN
    if (!rhi) {
        if (m_config.api() == QPlatformBackingStoreRhiConfig::D3D11) {
            QRhiD3D11InitParams params;
            params.enableDebugLayer = m_config.isDebugLayerEnabled();
            rhi = QRhi::create(QRhi::D3D11, &params, flags);
            if (!rhi && !flags.testFlag(QRhi::PreferSoftwareRenderer)) {
                qCDebug(lcQpaBackingStore, "Failed to create a D3D11 device with default settings; "
                                           "attempting to get a software rasterizer backed device instead");
                flags |= QRhi::PreferSoftwareRenderer;
                rhi = QRhi::create(QRhi::D3D11, &params, flags);
            }
        } else if (m_config.api() == QPlatformBackingStoreRhiConfig::D3D12) {
            QRhiD3D12InitParams params;
            params.enableDebugLayer = m_config.isDebugLayerEnabled();
            rhi = QRhi::create(QRhi::D3D12, &params, flags);
            if (!rhi && !flags.testFlag(QRhi::PreferSoftwareRenderer)) {
                qCDebug(lcQpaBackingStore, "Failed to create a D3D12 device with default settings; "
                                           "attempting to get a software rasterizer backed device instead");
                flags |= QRhi::PreferSoftwareRenderer;
                rhi = QRhi::create(QRhi::D3D12, &params, flags);
            }
        }
    }
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    if (!rhi && m_config.api() == QPlatformBackingStoreRhiConfig::Metal) {
        QRhiMetalInitParams params;
        // For parity with Qt Quick, fall back to OpenGL when there is no Metal (f.ex. in macOS virtual machines).
        if (QRhi::probe(QRhi::Metal, &params)) {
            rhi = QRhi::create(QRhi::Metal, &params, flags);
        } else {
            qCDebug(lcQpaBackingStore, "Metal does not seem to be supported. Falling back to OpenGL.");
            rhi = QRhi::create(QRhi::OpenGLES2, &params, flags);
        }
    }
#endif

#if QT_CONFIG(vulkan)
    if (!rhi && m_config.api() == QPlatformBackingStoreRhiConfig::Vulkan) {
        if (m_config.isDebugLayerEnabled())
            QVulkanDefaultInstance::setFlag(QVulkanDefaultInstance::EnableValidation);
        QRhiVulkanInitParams params;
        if (m_window) {
            if (!m_window->vulkanInstance())
                m_window->setVulkanInstance(QVulkanDefaultInstance::instance());
            params.inst = m_window->vulkanInstance();
        } else {
            params.inst = QVulkanDefaultInstance::instance();
        }
        if (!params.inst) {
            qWarning("No QVulkanInstance set for the top-level window, this is wrong.");
            return false;
        }
        params.window = m_window;
        rhi = QRhi::create(QRhi::Vulkan, &params, flags);
    }
#endif

    if (!rhi) {
        qWarning("Failed to create QRhi for QBackingStoreRhiSupport");
        delete surface;
        return false;
    }

    m_rhi = rhi;
    m_openGLFallbackSurface = surface;
    return true;
}

QRhiSwapChain *QBackingStoreRhiSupport::swapChainForWindow(QWindow *window)
{
    auto it = m_swapchains.constFind(window);
    if (it != m_swapchains.constEnd())
        return it.value().swapchain;

    QRhiSwapChain *swapchain = nullptr;
    QRhiRenderPassDescriptor *rp = nullptr;
    if (window && m_rhi) {
        QRhiSwapChain::Flags flags;
        const QSurfaceFormat format = window->requestedFormat();
        if (format.swapInterval() == 0)
            flags |= QRhiSwapChain::NoVSync;
        if (format.alphaBufferSize() > 0)
            flags |= QRhiSwapChain::SurfaceHasNonPreMulAlpha;
#if QT_CONFIG(vulkan)
        if (m_config.api() == QPlatformBackingStoreRhiConfig::Vulkan && !window->vulkanInstance())
            window->setVulkanInstance(QVulkanDefaultInstance::instance());
#endif
        qCDebug(lcQpaBackingStore) << "Creating swapchain for window" << window;
        swapchain = m_rhi->newSwapChain();
        swapchain->setWindow(window);
        swapchain->setFlags(flags);
        rp = swapchain->newCompatibleRenderPassDescriptor();
        swapchain->setRenderPassDescriptor(rp);
        if (!swapchain->createOrResize()) {
            qWarning("Failed to create swapchain for window flushed with an RHI-enabled backingstore");
            delete rp;
            return nullptr;
        }
    }
    if (swapchain) {
        SwapchainData d;
        d.swapchain = swapchain;
        d.renderPassDescriptor = rp;
        d.windowWatcher = new QBackingStoreRhiSupportWindowWatcher(this);
        m_swapchains.insert(window, d);
        window->installEventFilter(d.windowWatcher);
    }
    return swapchain;
}

bool QBackingStoreRhiSupportWindowWatcher::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::PlatformSurface
            && static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
    {
        QWindow *window = qobject_cast<QWindow *>(obj);
        auto it = m_rhiSupport->m_swapchains.find(window);
        if (it != m_rhiSupport->m_swapchains.end()) {
            qCDebug(lcQpaBackingStore) << "SurfaceAboutToBeDestroyed received for tracked window" << window << "cleaning up swapchain";
            auto data = *it;
            m_rhiSupport->m_swapchains.erase(it);
            data.reset(); // deletes 'this'
        }
    }
    return false;
}

QSurface::SurfaceType QBackingStoreRhiSupport::surfaceTypeForConfig(const QPlatformBackingStoreRhiConfig &config)
{
    QSurface::SurfaceType type = QSurface::RasterSurface;
    switch (config.api()) {
    case QPlatformBackingStoreRhiConfig::D3D11:
    case QPlatformBackingStoreRhiConfig::D3D12:
        type = QSurface::Direct3DSurface;
        break;
    case QPlatformBackingStoreRhiConfig::Vulkan:
        type = QSurface::VulkanSurface;
        break;
    case QPlatformBackingStoreRhiConfig::Metal:
        type = QSurface::MetalSurface;
        break;
    case QPlatformBackingStoreRhiConfig::OpenGL:
        type = QSurface::OpenGLSurface;
        break;
    default:
        break;
    }
    return type;
}

QRhi::Implementation QBackingStoreRhiSupport::apiToRhiBackend(QPlatformBackingStoreRhiConfig::Api api)
{
    switch (api) {
    case QPlatformBackingStoreRhiConfig::OpenGL:
        return QRhi::OpenGLES2;
    case QPlatformBackingStoreRhiConfig::Metal:
        return QRhi::Metal;
    case QPlatformBackingStoreRhiConfig::Vulkan:
        return QRhi::Vulkan;
    case QPlatformBackingStoreRhiConfig::D3D11:
        return QRhi::D3D11;
    case QPlatformBackingStoreRhiConfig::D3D12:
        return QRhi::D3D12;
    case QPlatformBackingStoreRhiConfig::Null:
        return QRhi::Null;
    default:
        break;
    }
    return QRhi::Null;
}

bool QBackingStoreRhiSupport::checkForceRhi(QPlatformBackingStoreRhiConfig *outConfig, QSurface::SurfaceType *outType)
{
    static QPlatformBackingStoreRhiConfig config;
    static bool checked = false;

    if (!checked) {
        checked = true;

        const bool alwaysRhi = qEnvironmentVariableIntValue("QT_WIDGETS_RHI");
        if (alwaysRhi)
            config.setEnabled(true);

        // if enabled, choose an api
        if (config.isEnabled()) {
#if defined(Q_OS_WIN)
            config.setApi(QPlatformBackingStoreRhiConfig::D3D11);
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
            config.setApi(QPlatformBackingStoreRhiConfig::Metal);
#elif QT_CONFIG(opengl)
            config.setApi(QPlatformBackingStoreRhiConfig::OpenGL);
#elif QT_CONFIG(vulkan)
            config.setApi(QPlatformBackingStoreRhiConfig::Vulkan);
#else
            qWarning("QT_WIDGETS_RHI is set but no backend is available; ignoring");
            return false;
#endif

            // the env.var. will always override
            if (qEnvironmentVariableIsSet("QT_WIDGETS_RHI_BACKEND")) {
                const QString backend = qEnvironmentVariable("QT_WIDGETS_RHI_BACKEND");
#ifdef Q_OS_WIN
                if (backend == QStringLiteral("d3d11") || backend == QStringLiteral("d3d"))
                    config.setApi(QPlatformBackingStoreRhiConfig::D3D11);
                if (backend == QStringLiteral("d3d12"))
                    config.setApi(QPlatformBackingStoreRhiConfig::D3D12);
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
                if (backend == QStringLiteral("metal"))
                    config.setApi(QPlatformBackingStoreRhiConfig::Metal);
#endif
#if QT_CONFIG(opengl)
                if (backend == QStringLiteral("opengl") || backend == QStringLiteral("gl"))
                    config.setApi(QPlatformBackingStoreRhiConfig::OpenGL);
#endif
#if QT_CONFIG(vulkan)
                if (backend == QStringLiteral("vulkan"))
                    config.setApi(QPlatformBackingStoreRhiConfig::Vulkan);
#endif
            }

            if (qEnvironmentVariableIntValue("QT_WIDGETS_RHI_DEBUG_LAYER"))
                config.setDebugLayer(true);
        }

        qCDebug(lcQpaBackingStore) << "Check for forced use of QRhi resulted in enable"
                                   << config.isEnabled() << "with api" << QRhi::backendName(apiToRhiBackend(config.api()));
    }

    if (config.isEnabled()) {
        if (outConfig)
            *outConfig = config;
        if (outType)
            *outType = surfaceTypeForConfig(config);
        return true;
    }
    return false;
}

QT_END_NAMESPACE
