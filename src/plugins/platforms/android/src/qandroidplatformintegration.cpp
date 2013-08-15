/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#include "qandroidplatformintegration.h"
#include "qabstracteventdispatcher.h"
#include "androidjnimain.h"
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/qguiapplication.h>
#include <qpa/qwindowsysteminterface.h>
#include <QThread>
#include <qpa/qplatformwindow.h>
#include "qandroidplatformservices.h"
#include "qandroidplatformfontdatabase.h"
#include "qandroidplatformclipboard.h"
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>

#ifndef ANDROID_PLUGIN_OPENGL
#  include "qandroidplatformscreen.h"
#  include "qandroidplatformwindow.h"
#  include <QtPlatformSupport/private/qfbbackingstore_p.h>
#else
#  include "qeglfswindow.h"
#  include "androidjnimenu.h"
#  include "qandroidopenglcontext.h"
#  include "qandroidopenglplatformwindow.h"
#  include "qeglfshooks.h"
#  include <QtGui/qopenglcontext.h>
#endif

#include "qandroidplatformtheme.h"

QT_BEGIN_NAMESPACE

int QAndroidPlatformIntegration::m_defaultGeometryWidth = 320;
int QAndroidPlatformIntegration::m_defaultGeometryHeight = 455;
int QAndroidPlatformIntegration::m_defaultPhysicalSizeWidth = 50;
int QAndroidPlatformIntegration::m_defaultPhysicalSizeHeight = 71;

void *QAndroidPlatformNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
    if (resource=="JavaVM")
        return QtAndroid::javaVM();
    if (resource == "QtActivity")
        return QtAndroid::activity();

    return 0;
}

QAndroidPlatformIntegration::QAndroidPlatformIntegration(const QStringList &paramList)
    : m_touchDevice(0)
{
    Q_UNUSED(paramList);

#ifndef ANDROID_PLUGIN_OPENGL
    m_eventDispatcher = createUnixEventDispatcher();
#endif

    m_androidPlatformNativeInterface = new QAndroidPlatformNativeInterface();

#ifndef ANDROID_PLUGIN_OPENGL
    m_primaryScreen = new QAndroidPlatformScreen();
    screenAdded(m_primaryScreen);
    m_primaryScreen->setPhysicalSize(QSize(m_defaultPhysicalSizeWidth, m_defaultPhysicalSizeHeight));
    m_primaryScreen->setGeometry(QRect(0, 0, m_defaultGeometryWidth, m_defaultGeometryHeight));
#endif

    m_mainThread = QThread::currentThread();
    QtAndroid::setAndroidPlatformIntegration(this);

    m_androidFDB = new QAndroidPlatformFontDatabase();
    m_androidPlatformServices = new QAndroidPlatformServices();
    m_androidPlatformClipboard = new QAndroidPlatformClipboard();
}

bool QAndroidPlatformIntegration::hasCapability(Capability cap) const
{
    switch (cap) {
        case ThreadedPixmaps: return true;
        case NonFullScreenWindows: return false;
        default:
#ifndef ANDROID_PLUGIN_OPENGL
        return QPlatformIntegration::hasCapability(cap);
#else
        return QEglFSIntegration::hasCapability(cap);
#endif
    }
}

#ifndef ANDROID_PLUGIN_OPENGL
QPlatformBackingStore *QAndroidPlatformIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QFbBackingStore(window);
}

QPlatformWindow *QAndroidPlatformIntegration::createPlatformWindow(QWindow *window) const
{
    return new QAndroidPlatformWindow(window);
}

QAbstractEventDispatcher *QAndroidPlatformIntegration::guiThreadEventDispatcher() const
{
    return m_eventDispatcher;
}
#else // !ANDROID_PLUGIN_OPENGL
QPlatformWindow *QAndroidPlatformIntegration::createPlatformWindow(QWindow *window) const
{
    QAndroidOpenGLPlatformWindow *platformWindow = new QAndroidOpenGLPlatformWindow(window);
    platformWindow->create();
    platformWindow->requestActivateWindow();
    QtAndroidMenu::setActiveTopLevelWindow(window);

    return platformWindow;
}

void QAndroidPlatformIntegration::invalidateNativeSurface()
{
    foreach (QWindow *w, QGuiApplication::topLevelWindows()) {
        QAndroidOpenGLPlatformWindow *window =
                static_cast<QAndroidOpenGLPlatformWindow *>(w->handle());
        if (window != 0)
            window->invalidateSurface();
    }
}

void QAndroidPlatformIntegration::surfaceChanged()
{
    QAndroidOpenGLPlatformWindow::updateStaticNativeWindow();
    foreach (QWindow *w, QGuiApplication::topLevelWindows()) {
        QAndroidOpenGLPlatformWindow *window =
                static_cast<QAndroidOpenGLPlatformWindow *>(w->handle());
        if (window != 0)
            window->resetSurface();
    }
}

QPlatformOpenGLContext *QAndroidPlatformIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QAndroidOpenGLContext(this,
                                     QEglFSHooks::hooks()->surfaceFormatFor(context->format()),
                                     context->shareHandle(),
                                     display());
}
#endif // ANDROID_PLUGIN_OPENGL

QAndroidPlatformIntegration::~QAndroidPlatformIntegration()
{
    delete m_androidPlatformNativeInterface;
    delete m_androidFDB;
    QtAndroid::setAndroidPlatformIntegration(NULL);
}
QPlatformFontDatabase *QAndroidPlatformIntegration::fontDatabase() const
{
    return m_androidFDB;
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QAndroidPlatformIntegration::clipboard() const
{
static QAndroidPlatformClipboard *clipboard = 0;
    if (!clipboard)
        clipboard = new QAndroidPlatformClipboard;

    return clipboard;
}
#endif

QPlatformInputContext *QAndroidPlatformIntegration::inputContext() const
{
    return &m_platformInputContext;
}

QPlatformNativeInterface *QAndroidPlatformIntegration::nativeInterface() const
{
    return m_androidPlatformNativeInterface;
}

QPlatformServices *QAndroidPlatformIntegration::services() const
{
    return m_androidPlatformServices;
}

QVariant QAndroidPlatformIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case ShowIsFullScreen:
        return true;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
}

static const QLatin1String androidThemeName("android");
QStringList QAndroidPlatformIntegration::themeNames() const
{
    return QStringList(QString(androidThemeName));
}

QPlatformTheme *QAndroidPlatformIntegration::createPlatformTheme(const QString &name) const
{
    if (androidThemeName == name)
        return new QAndroidPlatformTheme;

    return 0;
}

void QAndroidPlatformIntegration::setDefaultDisplayMetrics(int gw, int gh, int sw, int sh)
{
    m_defaultGeometryWidth = gw;
    m_defaultGeometryHeight = gh;
    m_defaultPhysicalSizeWidth = sw;
    m_defaultPhysicalSizeHeight = sh;
}

void QAndroidPlatformIntegration::setDefaultDesktopSize(int gw, int gh)
{
    m_defaultGeometryWidth = gw;
    m_defaultGeometryHeight = gh;
}


#ifndef ANDROID_PLUGIN_OPENGL
void QAndroidPlatformIntegration::setDesktopSize(int width, int height)
{
    if (m_primaryScreen)
        QMetaObject::invokeMethod(m_primaryScreen, "setGeometry", Qt::AutoConnection, Q_ARG(QRect, QRect(0,0,width, height)));
}

void QAndroidPlatformIntegration::setDisplayMetrics(int width, int height)
{
    if (m_primaryScreen)
        QMetaObject::invokeMethod(m_primaryScreen, "setPhysicalSize", Qt::AutoConnection, Q_ARG(QSize, QSize(width, height)));
}
#else
void QAndroidPlatformIntegration::setDesktopSize(int width, int height)
{
    m_defaultGeometryWidth = width;
    m_defaultGeometryHeight = height;
}

void QAndroidPlatformIntegration::setDisplayMetrics(int width, int height)
{
    m_defaultPhysicalSizeWidth = width;
    m_defaultPhysicalSizeHeight = height;
}

#endif

void QAndroidPlatformIntegration::pauseApp()
{
    if (QAbstractEventDispatcher::instance(m_mainThread))
        QAbstractEventDispatcher::instance(m_mainThread)->interrupt();
}

void QAndroidPlatformIntegration::resumeApp()
{
    if (QAbstractEventDispatcher::instance(m_mainThread))
        QAbstractEventDispatcher::instance(m_mainThread)->wakeUp();
}

QT_END_NAMESPACE
