/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfsintegration.h"

#include "qeglfswindow.h"
#include "qeglfsbackingstore.h"
#include "qeglfscompositor.h"
#include "qeglfshooks.h"

#include <QtGui/private/qguiapplication_p.h>

#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qgenericunixservices_p.h>
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>
#include <QtPlatformSupport/private/qeglpbuffer_p.h>

#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
#include <QtPlatformSupport/private/qevdevmousemanager_p.h>
#include <QtPlatformSupport/private/qevdevkeyboardmanager_p.h>
#include <QtPlatformSupport/private/qevdevtouch_p.h>
#endif

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <QtGui/QOffscreenSurface>
#include <qpa/qplatformcursor.h>

#include <qpa/qplatforminputcontextfactory_p.h>

#include "qeglfscontext.h"

#include <EGL/egl.h>

static void initResources()
{
    Q_INIT_RESOURCE(cursor);
}

QT_BEGIN_NAMESPACE

QEglFSIntegration::QEglFSIntegration()
    : mFontDb(new QGenericUnixFontDatabase)
    , mServices(new QGenericUnixServices)
    , mScreen(0)
    , mInputContext(0)
{
    initResources();
}

QEglFSIntegration::~QEglFSIntegration()
{
    QEglFSCompositor::destroy();
    delete mScreen;
    eglTerminate(mDisplay);
    QEglFSHooks::hooks()->platformDestroy();
}

bool QEglFSIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    // We assume that devices will have more and not less capabilities
    if (QEglFSHooks::hooks() && QEglFSHooks::hooks()->hasCapability(cap))
        return true;

    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    case WindowManagement: return false;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QEglFSIntegration::createPlatformWindow(QWindow *window) const
{
    QWindowSystemInterface::flushWindowSystemEvents();
    QEglFSWindow *w = new QEglFSWindow(window);
    w->create();
    if (window->type() != Qt::ToolTip)
        w->requestActivateWindow();
    return w;
}

QPlatformBackingStore *QEglFSIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QEglFSBackingStore(window);
}

QPlatformOpenGLContext *QEglFSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QEglFSContext(QEglFSHooks::hooks()->surfaceFormatFor(context->format()), context->shareHandle(), mDisplay);
}

QPlatformOffscreenSurface *QEglFSIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QEglFSScreen *screen = static_cast<QEglFSScreen *>(surface->screen()->handle());
    return new QEGLPbuffer(screen->display(), QEglFSHooks::hooks()->surfaceFormatFor(surface->requestedFormat()), surface);
}

QPlatformFontDatabase *QEglFSIntegration::fontDatabase() const
{
    return mFontDb.data();
}

QAbstractEventDispatcher *QEglFSIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

void QEglFSIntegration::initialize()
{
    QEglFSHooks::hooks()->platformInit();

    EGLint major, minor;

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        qWarning("Could not bind GL_ES API\n");
        qFatal("EGL error");
    }

    mDisplay = eglGetDisplay(QEglFSHooks::hooks() ? QEglFSHooks::hooks()->platformDisplay() : EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL_NO_DISPLAY) {
        qWarning("Could not open egl display\n");
        qFatal("EGL error");
    }

    if (!eglInitialize(mDisplay, &major, &minor)) {
        qWarning("Could not initialize egl display\n");
        qFatal("EGL error");
    }

    mScreen = createScreen();
    screenAdded(mScreen);

    mInputContext = QPlatformInputContextFactory::create();

    createInputHandlers();
}

QEglFSScreen *QEglFSIntegration::createScreen() const
{
    return new QEglFSScreen(mDisplay);
}

QVariant QEglFSIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    switch (hint)
    {
    case QPlatformIntegration::ShowIsFullScreen:
        return mScreen->rootWindow() == 0;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
}

QPlatformServices *QEglFSIntegration::services() const
{
    return mServices.data();
}

QPlatformNativeInterface *QEglFSIntegration::nativeInterface() const
{
    return const_cast<QEglFSIntegration *>(this);
}

enum ResourceType {
    EglDisplay,
    EglWindow,
    EglContext
};

static int resourceType(const QByteArray &key)
{
    static const QByteArray names[] = { // match ResourceType
        QByteArrayLiteral("egldisplay"),
        QByteArrayLiteral("eglwindow"),
        QByteArrayLiteral("eglcontext")
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
        result = mScreen->display();
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
            result = mScreen->display();
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
            result = static_cast<QEGLPlatformContext *>(context->handle())->eglContext();
        break;
    default:
        break;
    }

    return result;
}

static void *eglContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);

    QEGLPlatformContext *handle = static_cast<QEGLPlatformContext *>(context->handle());
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

EGLConfig QEglFSIntegration::chooseConfig(EGLDisplay display, const QSurfaceFormat &format)
{
    class Chooser : public QEglConfigChooser {
    public:
        Chooser(EGLDisplay display, QEglFSHooks *hooks)
            : QEglConfigChooser(display)
            , m_hooks(hooks)
        {
        }

    protected:
        bool filterConfig(EGLConfig config) const
        {
            return m_hooks->filterConfig(display(), config) && QEglConfigChooser::filterConfig(config);
        }

    private:
        QEglFSHooks *m_hooks;
    };

    Chooser chooser(display, QEglFSHooks::hooks());
    chooser.setSurfaceFormat(format);
    return chooser.chooseConfig();
}

void QEglFSIntegration::createInputHandlers()
{
#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
    new QEvdevKeyboardManager(QLatin1String("EvdevKeyboard"), QString() /* spec */, this);
    new QEvdevMouseManager(QLatin1String("EvdevMouse"), QString() /* spec */, this);
    new QEvdevTouchScreenHandlerThread(QString() /* spec */, this);
#endif
}

QT_END_NAMESPACE
