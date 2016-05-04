/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qtextstream.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QWindow>
#include <QtCore/QLoggingCategory>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatforminputcontextfactory_p.h>

#include "qeglfsintegration.h"
#include "qeglfswindow.h"
#include "qeglfshooks.h"
#include "qeglfscontext.h"
#include "qeglfsoffscreenwindow.h"
#include "qeglfscursor.h"

#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>
#include <QtPlatformSupport/private/qeglpbuffer_p.h>

#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixservices_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qfbvthandler_p.h>
#include <QtPlatformSupport/private/qopenglcompositorbackingstore_p.h>

#include <QtPlatformHeaders/QEGLNativeContext>

#ifndef QT_NO_LIBINPUT
#include <QtPlatformSupport/private/qlibinputhandler_p.h>
#endif

#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
#include <QtPlatformSupport/private/qevdevmousemanager_p.h>
#include <QtPlatformSupport/private/qevdevkeyboardmanager_p.h>
#include <QtPlatformSupport/private/qevdevtouchmanager_p.h>
#endif

#if !defined(QT_NO_TSLIB) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
#include <QtPlatformSupport/private/qtslib_p.h>
#endif

#include <QtPlatformHeaders/qeglfsfunctions.h>

#include <EGL/egl.h>

static void initResources()
{
#ifndef QT_NO_CURSOR
    Q_INIT_RESOURCE(cursor);
#endif
}

QT_BEGIN_NAMESPACE

QEglFSIntegration::QEglFSIntegration()
    : m_display(EGL_NO_DISPLAY),
      m_inputContext(0),
      m_fontDb(new QGenericUnixFontDatabase),
      m_services(new QGenericUnixServices),
      m_kbdMgr(0),
      m_disableInputHandlers(false)
{
    m_disableInputHandlers = qEnvironmentVariableIntValue("QT_QPA_EGLFS_DISABLE_INPUT");

    initResources();
}

void QEglFSIntegration::addScreen(QPlatformScreen *screen)
{
    screenAdded(screen);
}

void QEglFSIntegration::removeScreen(QPlatformScreen *screen)
{
    destroyScreen(screen);
}

void QEglFSIntegration::initialize()
{
    qt_egl_device_integration()->platformInit();

    m_display = qt_egl_device_integration()->createDisplay(nativeDisplay());
    if (m_display == EGL_NO_DISPLAY)
        qFatal("Could not open egl display");

    EGLint major, minor;
    if (!eglInitialize(m_display, &major, &minor))
        qFatal("Could not initialize egl display");

    m_inputContext = QPlatformInputContextFactory::create();

    m_vtHandler.reset(new QFbVtHandler);

    if (qt_egl_device_integration()->usesDefaultScreen())
        addScreen(new QEglFSScreen(display()));
    else
        qt_egl_device_integration()->screenInit();

    // Input code may rely on the screens, so do it only after the screen init.
    if (!m_disableInputHandlers)
        createInputHandlers();
}

void QEglFSIntegration::destroy()
{
    foreach (QWindow *w, qGuiApp->topLevelWindows())
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

QPlatformBackingStore *QEglFSIntegration::createPlatformBackingStore(QWindow *window) const
{
    QOpenGLCompositorBackingStore *bs = new QOpenGLCompositorBackingStore(window);
    if (!window->handle())
        window->create();
    static_cast<QEglFSWindow *>(window->handle())->setBackingStore(bs);
    return bs;
}

QPlatformWindow *QEglFSIntegration::createPlatformWindow(QWindow *window) const
{
    QWindowSystemInterface::flushWindowSystemEvents();
    QEglFSWindow *w = qt_egl_device_integration()->createWindow(window);
    w->create();
    if (window->type() != Qt::ToolTip)
        w->requestActivateWindow();
    return w;
}

QPlatformOpenGLContext *QEglFSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    EGLDisplay dpy = context->screen() ? static_cast<QEglFSScreen *>(context->screen()->handle())->display() : display();
    QPlatformOpenGLContext *share = context->shareHandle();
    QVariant nativeHandle = context->nativeHandle();

    QEglFSContext *ctx;
    QSurfaceFormat adjustedFormat = qt_egl_device_integration()->surfaceFormatFor(context->format());
    if (nativeHandle.isNull()) {
        EGLConfig config = QEglFSIntegration::chooseConfig(dpy, adjustedFormat);
        ctx = new QEglFSContext(adjustedFormat, share, dpy, &config, QVariant());
    } else {
        ctx = new QEglFSContext(adjustedFormat, share, dpy, 0, nativeHandle);
    }
    nativeHandle = QVariant::fromValue<QEGLNativeContext>(QEGLNativeContext(ctx->eglContext(), dpy));

    context->setNativeHandle(nativeHandle);
    return ctx;
}

QPlatformOffscreenSurface *QEglFSIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    EGLDisplay dpy = surface->screen() ? static_cast<QEglFSScreen *>(surface->screen()->handle())->display() : display();
    QSurfaceFormat fmt = qt_egl_device_integration()->surfaceFormatFor(surface->requestedFormat());
    if (qt_egl_device_integration()->supportsPBuffers()) {
        QEGLPlatformContext::Flags flags = 0;
        if (!qt_egl_device_integration()->supportsSurfacelessContexts())
            flags |= QEGLPlatformContext::NoSurfaceless;
        return new QEGLPbuffer(dpy, fmt, surface, flags);
    } else {
        return new QEglFSOffscreenWindow(dpy, fmt, surface);
    }
    // Never return null. Multiple QWindows are not supported by this plugin.
}

bool QEglFSIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    // We assume that devices will have more and not less capabilities
    if (qt_egl_device_integration()->hasCapability(cap))
        return true;

    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    case WindowManagement: return false;
    case RasterGLSurface: return true;
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
    WaylandDisplay
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
        QByteArrayLiteral("server_wl_display")
    };
    const QByteArray *end = names + sizeof(names) / sizeof(names[0]);
    const QByteArray *result = std::find(names, end, key);
    if (result == end)
        result = std::find(names, end, key.toLower());
    return int(result - names);
}

void *QEglFSIntegration::nativeResourceForIntegration(const QByteArray &resource)
{
    void *result = 0;

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
        break;
    }

    return result;
}

void *QEglFSIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *)
{
    void *result = 0;

    switch (resourceType(resource)) {
    case XlibDisplay:
        // Play nice when using the x11 hooks: Be compatible with xcb that allows querying
        // the X Display pointer, which is nothing but our native display.
        result = reinterpret_cast<void*>(nativeDisplay());
        break;
    default:
        break;
    }

    return result;
}

void *QEglFSIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    void *result = 0;

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
    default:
        break;
    }

    return result;
}

void *QEglFSIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    void *result = 0;

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
        return 0;

    return handle->eglContext();
}

QPlatformNativeInterface::NativeResourceForContextFunction QEglFSIntegration::nativeResourceFunctionForContext(const QByteArray &resource)
{
    QByteArray lowerCaseResource = resource.toLower();
    if (lowerCaseResource == "get_egl_context")
        return NativeResourceForContextFunction(eglContextForContext);

    return 0;
}

QFunctionPointer QEglFSIntegration::platformFunction(const QByteArray &function) const
{
#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
    if (function == QEglFSFunctions::loadKeymapTypeIdentifier())
        return QFunctionPointer(loadKeymapStatic);
#else
    Q_UNUSED(function)
#endif

    return 0;
}

void QEglFSIntegration::loadKeymapStatic(const QString &filename)
{
#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
    QEglFSIntegration *self = static_cast<QEglFSIntegration *>(QGuiApplicationPrivate::platformIntegration());
    if (self->m_kbdMgr)
        self->m_kbdMgr->loadKeymap(filename);
    else
        qWarning("QEglFSIntegration: Cannot load keymap, no keyboard handler found");
#else
    Q_UNUSED(filename);
#endif
}

void QEglFSIntegration::createInputHandlers()
{
#ifndef QT_NO_LIBINPUT
    if (!qEnvironmentVariableIntValue("QT_QPA_EGLFS_NO_LIBINPUT")) {
        new QLibInputHandler(QLatin1String("libinput"), QString());
        return;
    }
#endif

#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
    m_kbdMgr = new QEvdevKeyboardManager(QLatin1String("EvdevKeyboard"), QString() /* spec */, this);
    new QEvdevMouseManager(QLatin1String("EvdevMouse"), QString() /* spec */, this);
#ifndef QT_NO_TSLIB
    const bool useTslib = qEnvironmentVariableIntValue("QT_QPA_EGLFS_TSLIB");
    if (useTslib)
        new QTsLibMouseHandler(QLatin1String("TsLib"), QString() /* spec */);
    else
#endif // QT_NO_TSLIB
        new QEvdevTouchManager(QLatin1String("EvdevTouch"), QString() /* spec */, this);
#endif
}

EGLNativeDisplayType QEglFSIntegration::nativeDisplay() const
{
    return qt_egl_device_integration()->platformDisplay();
}

EGLConfig QEglFSIntegration::chooseConfig(EGLDisplay display, const QSurfaceFormat &format)
{
    class Chooser : public QEglConfigChooser {
    public:
        Chooser(EGLDisplay display)
            : QEglConfigChooser(display) { }
        bool filterConfig(EGLConfig config) const Q_DECL_OVERRIDE {
            return qt_egl_device_integration()->filterConfig(display(), config)
                    && QEglConfigChooser::filterConfig(config);
        }
    };

    Chooser chooser(display);
    chooser.setSurfaceType(qt_egl_device_integration()->surfaceType());
    chooser.setSurfaceFormat(format);
    return chooser.chooseConfig();
}

QT_END_NAMESPACE
