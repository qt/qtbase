// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qtextstream.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QScreen>
#ifndef QT_NO_OPENGL
# include <QtGui/QOpenGLContext>
# include <QtGui/QOffscreenSurface>
#endif
#include <QtGui/QWindow>
#include <QtCore/QLoggingCategory>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatforminputcontextfactory_p.h>

#include "qeglfsintegration_p.h"
#include "qeglfswindow_p.h"
#include "qeglfshooks_p.h"
#ifndef QT_NO_OPENGL
# include "qeglfscontext_p.h"
# include "qeglfscursor_p.h"
#endif
#include "qeglfsoffscreenwindow_p.h"

#include <QtGui/private/qeglconvenience_p.h>
#ifndef QT_NO_OPENGL
# include <QtGui/private/qeglplatformcontext_p.h>
# include <QtGui/private/qeglpbuffer_p.h>
#endif

#include <QtGui/private/qgenericunixfontdatabase_p.h>
#include <QtGui/private/qgenericunixservices_p.h>
#include <QtGui/private/qgenericunixthemes_p.h>
#include <QtGui/private/qgenericunixeventdispatcher_p.h>
#include <QtFbSupport/private/qfbvthandler_p.h>
#ifndef QT_NO_OPENGL
# include <QtOpenGL/private/qopenglcompositorbackingstore_p.h>
#endif

#if QT_CONFIG(libinput)
#include <QtInputSupport/private/qlibinputhandler_p.h>
#endif

#if QT_CONFIG(evdev)
#include <QtInputSupport/private/qevdevmousemanager_p.h>
#include <QtInputSupport/private/qevdevkeyboardmanager_p.h>
#include <QtInputSupport/private/qevdevtouchmanager_p.h>
#endif

#if QT_CONFIG(tslib)
#include <QtInputSupport/private/qtslib_p.h>
#endif

#if QT_CONFIG(integrityhid)
#include <QtInputSupport/qintegrityhidmanager.h>
#endif

static void initResources()
{
#ifndef QT_NO_CURSOR
    Q_INIT_RESOURCE(cursor);
#endif
}

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QEglFSIntegration::QEglFSIntegration()
    : m_kbdMgr(nullptr),
      m_display(EGL_NO_DISPLAY),
      m_inputContext(nullptr),
      m_fontDb(new QGenericUnixFontDatabase),
      m_services(new QGenericUnixServices),
      m_disableInputHandlers(false)
{
    m_disableInputHandlers = qEnvironmentVariableIntValue("QT_QPA_EGLFS_DISABLE_INPUT");

    initResources();
}

void QEglFSIntegration::initialize()
{
    qt_egl_device_integration()->platformInit();

    m_display = qt_egl_device_integration()->createDisplay(nativeDisplay());
    if (Q_UNLIKELY(m_display == EGL_NO_DISPLAY))
        qFatal("Could not open egl display");

    EGLint major, minor;
    if (Q_UNLIKELY(!eglInitialize(m_display, &major, &minor)))
        qFatal("Could not initialize egl display");

    m_inputContext = QPlatformInputContextFactory::create();

    m_vtHandler.reset(new QFbVtHandler);

    if (qt_egl_device_integration()->usesDefaultScreen())
        QWindowSystemInterface::handleScreenAdded(new QEglFSScreen(display()));
    else
        qt_egl_device_integration()->screenInit();

    // Input code may rely on the screens, so do it only after the screen init.
    if (!m_disableInputHandlers)
        createInputHandlers();
}

void QEglFSIntegration::destroy()
{
    const auto toplevels = qGuiApp->topLevelWindows();
    for (QWindow *w : toplevels)
        w->destroy();

    qt_egl_device_integration()->screenDestroy();

    if (m_display != EGL_NO_DISPLAY)
        eglTerminate(m_display);

    qt_egl_device_integration()->platformDestroy();
}

QAbstractEventDispatcher *QEglFSIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformServices *QEglFSIntegration::services() const
{
    return m_services.data();
}

QPlatformFontDatabase *QEglFSIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QPlatformTheme *QEglFSIntegration::createPlatformTheme(const QString &name) const
{
    return QGenericUnixTheme::createUnixTheme(name);
}

QPlatformBackingStore *QEglFSIntegration::createPlatformBackingStore(QWindow *window) const
{
#ifndef QT_NO_OPENGL
    QOpenGLCompositorBackingStore *bs = new QOpenGLCompositorBackingStore(window);
    if (!window->handle())
        window->create();
    static_cast<QEglFSWindow *>(window->handle())->setBackingStore(bs);
    return bs;
#else
    Q_UNUSED(window);
    return nullptr;
#endif
}

QPlatformWindow *QEglFSIntegration::createPlatformWindow(QWindow *window) const
{
    QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
    QEglFSWindow *w = qt_egl_device_integration()->createWindow(window);
    w->create();

    const auto showWithoutActivating = window->property("_q_showWithoutActivating");
    if (showWithoutActivating.isValid() && showWithoutActivating.toBool())
        return w;

    // Activate only the window for the primary screen to make input work
    if (window->type() != Qt::ToolTip && window->screen() == QGuiApplication::primaryScreen())
        w->requestActivateWindow();

    return w;
}

#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QEglFSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    EGLDisplay dpy = context->screen() ? static_cast<QEglFSScreen *>(context->screen()->handle())->display() : display();
    QPlatformOpenGLContext *share = context->shareHandle();

    QEglFSContext *ctx;
    QSurfaceFormat adjustedFormat = qt_egl_device_integration()->surfaceFormatFor(context->format());
    EGLConfig config = QEglFSDeviceIntegration::chooseConfig(dpy, adjustedFormat);
    ctx = new QEglFSContext(adjustedFormat, share, dpy, &config);

    return ctx;
}

QOpenGLContext *QEglFSIntegration::createOpenGLContext(EGLContext context, EGLDisplay contextDisplay, QOpenGLContext *shareContext) const
{
    return QEGLPlatformContext::createFrom<QEglFSContext>(context, contextDisplay, display(), shareContext);
}

QPlatformOffscreenSurface *QEglFSIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    EGLDisplay dpy = surface->screen() ? static_cast<QEglFSScreen *>(surface->screen()->handle())->display() : display();
    QSurfaceFormat fmt = qt_egl_device_integration()->surfaceFormatFor(surface->requestedFormat());
    if (qt_egl_device_integration()->supportsPBuffers()) {
        QEGLPlatformContext::Flags flags;
        if (!qt_egl_device_integration()->supportsSurfacelessContexts())
            flags |= QEGLPlatformContext::NoSurfaceless;
        return new QEGLPbuffer(dpy, fmt, surface, flags);
    } else {
        return new QEglFSOffscreenWindow(dpy, fmt, surface);
    }
    // Never return null. Multiple QWindows are not supported by this plugin.
}
#endif // QT_NO_OPENGL

bool QEglFSIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    // We assume that devices will have more and not less capabilities
    if (qt_egl_device_integration()->hasCapability(cap))
        return true;

    switch (cap) {
    case ThreadedPixmaps: return true;
#ifndef QT_NO_OPENGL
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    case RasterGLSurface: return true;
#else
    case OpenGL: return false;
    case ThreadedOpenGL: return false;
    case RasterGLSurface: return false;
#endif
    case WindowManagement: return false;
    case OpenGLOnRasterSurface: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformNativeInterface *QEglFSIntegration::nativeInterface() const
{
    return const_cast<QEglFSIntegration *>(this);
}

enum ResourceType {
    EglDisplay,
    EglWindow,
    EglContext,
    EglConfig,
    NativeDisplay,
    XlibDisplay,
    WaylandDisplay,
    EglSurface,
    VkSurface
};

static int resourceType(const QByteArray &key)
{
    static const QByteArray names[] = { // match ResourceType
        QByteArrayLiteral("egldisplay"),
        QByteArrayLiteral("eglwindow"),
        QByteArrayLiteral("eglcontext"),
        QByteArrayLiteral("eglconfig"),
        QByteArrayLiteral("nativedisplay"),
        QByteArrayLiteral("display"),
        QByteArrayLiteral("server_wl_display"),
        QByteArrayLiteral("eglsurface"),
        QByteArrayLiteral("vksurface")
    };
    const QByteArray *end = names + sizeof(names) / sizeof(names[0]);
    const QByteArray *result = std::find(names, end, key);
    if (result == end)
        result = std::find(names, end, key.toLower());
    return int(result - names);
}

void *QEglFSIntegration::nativeResourceForIntegration(const QByteArray &resource)
{
    void *result = nullptr;

    switch (resourceType(resource)) {
    case EglDisplay:
        result = display();
        break;
    case NativeDisplay:
        result = reinterpret_cast<void*>(nativeDisplay());
        break;
    case WaylandDisplay:
        result = qt_egl_device_integration()->wlDisplay();
        break;
    default:
        result = qt_egl_device_integration()->nativeResourceForIntegration(resource);
        break;
    }

    return result;
}

void *QEglFSIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    void *result = nullptr;

    switch (resourceType(resource)) {
    case XlibDisplay:
        // Play nice when using the x11 hooks: Be compatible with xcb that allows querying
        // the X Display pointer, which is nothing but our native display.
        result = reinterpret_cast<void*>(nativeDisplay());
        break;
    default:
        result = qt_egl_device_integration()->nativeResourceForScreen(resource, screen);
        break;
    }

    return result;
}

void *QEglFSIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    void *result = nullptr;

    switch (resourceType(resource)) {
    case EglDisplay:
        if (window && window->handle())
            result = static_cast<QEglFSScreen *>(window->handle()->screen())->display();
        else
            result = display();
        break;
    case EglWindow:
        if (window && window->handle())
            result = reinterpret_cast<void*>(static_cast<QEglFSWindow *>(window->handle())->eglWindow());
        break;
    case EglSurface:
        if (window && window->handle())
            result = reinterpret_cast<void*>(static_cast<QEglFSWindow *>(window->handle())->surface());
        break;
    default:
        break;
    }

    return result;
}

#ifndef QT_NO_OPENGL
void *QEglFSIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    void *result = nullptr;

    switch (resourceType(resource)) {
    case EglContext:
        if (context->handle())
            result = static_cast<QEglFSContext *>(context->handle())->eglContext();
        break;
    case EglConfig:
        if (context->handle())
            result = static_cast<QEglFSContext *>(context->handle())->eglConfig();
        break;
    case EglDisplay:
        if (context->handle())
            result = static_cast<QEglFSContext *>(context->handle())->eglDisplay();
        break;
    default:
        break;
    }

    return result;
}

static void *eglContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);

    QEglFSContext *handle = static_cast<QEglFSContext *>(context->handle());
    if (!handle)
        return nullptr;

    return handle->eglContext();
}
#endif

QPlatformNativeInterface::NativeResourceForContextFunction QEglFSIntegration::nativeResourceFunctionForContext(const QByteArray &resource)
{
#ifndef QT_NO_OPENGL
    if (resource.compare("get_egl_context", Qt::CaseInsensitive) == 0)
        return NativeResourceForContextFunction(eglContextForContext);
#else
    Q_UNUSED(resource);
#endif
    return nullptr;
}

QFunctionPointer QEglFSIntegration::platformFunction(const QByteArray &function) const
{
    return qt_egl_device_integration()->platformFunction(function);
}

#if QT_CONFIG(evdev)
void QEglFSIntegration::loadKeymap(const QString &filename)
{
    if (m_kbdMgr)
        m_kbdMgr->loadKeymap(filename);
    else
        qWarning("QEglFSIntegration: Cannot load keymap, no keyboard handler found");
}

void QEglFSIntegration::switchLang()
{
    if (m_kbdMgr)
        m_kbdMgr->switchLang();
    else
        qWarning("QEglFSIntegration: Cannot switch language, no keyboard handler found");
}
#endif

void QEglFSIntegration::createInputHandlers()
{
#if QT_CONFIG(libinput)
    if (!qEnvironmentVariableIntValue("QT_QPA_EGLFS_NO_LIBINPUT")) {
        new QLibInputHandler("libinput"_L1, QString());
        return;
    }
#endif

#if QT_CONFIG(tslib)
    bool useTslib = qEnvironmentVariableIntValue("QT_QPA_EGLFS_TSLIB");
    if (useTslib)
        new QTsLibMouseHandler("TsLib"_L1, QString() /* spec */);
#endif

#if QT_CONFIG(evdev)
    m_kbdMgr = new QEvdevKeyboardManager("EvdevKeyboard"_L1, QString() /* spec */, this);
    new QEvdevMouseManager("EvdevMouse"_L1, QString() /* spec */, this);
#if QT_CONFIG(tslib)
    if (!useTslib)
#endif
        new QEvdevTouchManager("EvdevTouch"_L1, QString() /* spec */, this);
#endif

#if QT_CONFIG(integrityhid)
   new QIntegrityHIDManager("HID", "", this);
#endif
}

EGLNativeDisplayType QEglFSIntegration::nativeDisplay() const
{
    return qt_egl_device_integration()->platformDisplay();
}

QT_END_NAMESPACE
