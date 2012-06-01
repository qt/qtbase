/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>

#include <QtPlatformSupport/private/qevdevmousemanager_p.h>
#include <QtPlatformSupport/private/qevdevkeyboardmanager_p.h>
#include <QtPlatformSupport/private/qevdevtouch_p.h>

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <qpa/qplatformcursor.h>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

QEglFSIntegration::QEglFSIntegration()
    : mEventDispatcher(createUnixEventDispatcher()), mFontDb(new QGenericUnixFontDatabase())
{
    QGuiApplicationPrivate::instance()->setEventDispatcher(mEventDispatcher);

    new QEvdevKeyboardManager(QLatin1String("EvdevKeyboard"), QString() /* spec */, this);
    new QEvdevMouseManager(QLatin1String("EvdevMouse"), QString() /* spec */, this);
    new QEvdevTouchScreenHandlerThread(QString() /* spec */, this);

    hooks->platformInit();

    EGLint major, minor;

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        qWarning("Could not bind GL_ES API\n");
        qFatal("EGL error");
    }

    mDisplay = eglGetDisplay(hooks ? hooks->platformDisplay() : EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL_NO_DISPLAY) {
        qWarning("Could not open egl display\n");
        qFatal("EGL error");
    }
    qWarning("Opened display %p\n", mDisplay);

    if (!eglInitialize(mDisplay, &major, &minor)) {
        qWarning("Could not initialize egl display\n");
        qFatal("EGL error");
    }

    qWarning("Initialized display %d %d\n", major, minor);

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

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglFSIntegration\n");
#endif
}

QEglFSIntegration::~QEglFSIntegration()
{
    delete mScreen;

    eglTerminate(mDisplay);
    hooks->platformDestroy();
}

bool QEglFSIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    // We assume that devices will have more and not less capabilities
    if (hooks && hooks->hasCapability(cap))
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
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglFSIntegration::createPlatformWindow %p\n",window);
#endif
    QPlatformWindow *w = new QEglFSWindow(window);
    w->requestActivateWindow();

    return w;
}


QPlatformBackingStore *QEglFSIntegration::createPlatformBackingStore(QWindow *window) const
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglFSIntegration::createWindowSurface %p\n", window);
#endif
    return new QEglFSBackingStore(window);
}

QPlatformOpenGLContext *QEglFSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return static_cast<QEglFSScreen *>(context->screen()->handle())->platformContext();
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

QT_END_NAMESPACE
