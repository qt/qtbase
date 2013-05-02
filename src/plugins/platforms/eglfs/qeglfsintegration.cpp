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
#include "qeglfshooks.h"

#include <QtGui/private/qguiapplication_p.h>

#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
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

QT_BEGIN_NAMESPACE

QEglFSIntegration::QEglFSIntegration()
    : mEventDispatcher(createUnixEventDispatcher()), mFontDb(new QGenericUnixFontDatabase())
{
    QGuiApplicationPrivate::instance()->setEventDispatcher(mEventDispatcher);

#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
    new QEvdevKeyboardManager(QLatin1String("EvdevKeyboard"), QString() /* spec */, this);
    new QEvdevMouseManager(QLatin1String("EvdevMouse"), QString() /* spec */, this);
    new QEvdevTouchScreenHandlerThread(QString() /* spec */, this);
#endif

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

    int swapInterval = 1;
    QByteArray swapIntervalString = qgetenv("QT_QPA_EGLFS_SWAPINTERVAL");
    if (!swapIntervalString.isEmpty()) {
        bool ok;
        swapInterval = swapIntervalString.toInt(&ok);
        if (!ok)
            swapInterval = 1;
    }
    eglSwapInterval(mDisplay, swapInterval);

    mScreen = new QEglFSScreen(mDisplay);
    screenAdded(mScreen);

    mInputContext = QPlatformInputContextFactory::create();
}

QEglFSIntegration::~QEglFSIntegration()
{
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
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QEglFSIntegration::createPlatformWindow(QWindow *window) const
{
    QEglFSWindow *w = new QEglFSWindow(window);
    w->create();
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
    return mFontDb;
}

QAbstractEventDispatcher *QEglFSIntegration::guiThreadEventDispatcher() const
{
    return mEventDispatcher;
}

QVariant QEglFSIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    if (hint == QPlatformIntegration::ShowIsFullScreen)
        return true;

    return QPlatformIntegration::styleHint(hint);
}

QPlatformNativeInterface *QEglFSIntegration::nativeInterface() const
{
    return const_cast<QEglFSIntegration *>(this);
}

void *QEglFSIntegration::nativeResourceForIntegration(const QByteArray &resource)
{
    QByteArray lowerCaseResource = resource.toLower();

    if (lowerCaseResource == "egldisplay")
        return static_cast<QEglFSScreen *>(mScreen)->display();

    return 0;
}

void *QEglFSIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    QByteArray lowerCaseResource = resource.toLower();

    QEGLPlatformContext *handle = static_cast<QEGLPlatformContext *>(context->handle());

    if (!handle)
        return 0;

    if (lowerCaseResource == "eglcontext")
        return handle->eglContext();

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

QT_END_NAMESPACE
