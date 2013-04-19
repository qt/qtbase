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

#include "qiosintegration.h"
#include "qioswindow.h"
#include "qiosbackingstore.h"
#include "qiosscreen.h"
#include "qioseventdispatcher.h"
#include "qioscontext.h"
#include "qiosinputcontext.h"
#include "qiostheme.h"

#include <QtPlatformSupport/private/qcoretextfontdatabase_p.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

QIOSIntegration::QIOSIntegration()
    : m_fontDatabase(new QCoreTextFontDatabase)
    , m_inputContext(new QIOSInputContext)
    , m_screen(new QIOSScreen(QIOSScreen::MainScreen))
{
    if (![UIApplication sharedApplication]) {
        qWarning()
            << "Error: You are creating QApplication before calling UIApplicationMain.\n"
            << "If you are writing a native iOS application, and only want to use Qt for\n"
            << "parts of the application, a good place to create QApplication is from within\n"
            << "'applicationDidFinishLaunching' inside your UIApplication delegate.\n"
            << "If you instead create a cross-platform Qt application and do not intend to call\n"
            << "UIApplicationMain, you need to link in libqtmain.a, and substitute main with qt_main.\n"
            << "This is normally done automatically by qmake.\n";
        exit(-1);
    }

    screenAdded(m_screen);

    m_touchDevice = new QTouchDevice;
    m_touchDevice->setType(QTouchDevice::TouchScreen);
    m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::NormalizedPosition);
    QWindowSystemInterface::registerTouchDevice(m_touchDevice);
}

bool QIOSIntegration::hasCapability(Capability cap) const
{
    switch (cap) {
    case OpenGL:
        return true;
    case MultipleWindows:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QIOSIntegration::createPlatformWindow(QWindow *window) const
{
    return new QIOSWindow(window);
}

// Used when the QWindow's surface type is set by the client to QSurface::RasterSurface
QPlatformBackingStore *QIOSIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QIOSBackingStore(window);
}

// Used when the QWindow's surface type is set by the client to QSurface::OpenGLSurface
QPlatformOpenGLContext *QIOSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    Q_UNUSED(context);
    return new QIOSContext(context);
}

QAbstractEventDispatcher *QIOSIntegration::guiThreadEventDispatcher() const
{
    return new QIOSEventDispatcher();
}

QPlatformFontDatabase * QIOSIntegration::fontDatabase() const
{
    return m_fontDatabase;
}

QPlatformInputContext *QIOSIntegration::inputContext() const
{
    return m_inputContext;
}

QVariant QIOSIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case ShowIsFullScreen:
        return true;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
}

QStringList QIOSIntegration::themeNames() const
{
    return QStringList(QLatin1String(QIOSTheme::name));
}

QPlatformTheme *QIOSIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1String(QIOSTheme::name))
        return new QIOSTheme;

    return QPlatformIntegration::createPlatformTheme(name);
}

QPlatformNativeInterface *QIOSIntegration::nativeInterface() const
{
    return const_cast<QIOSIntegration *>(this);
}

void *QIOSIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (!window || !window->handle())
        return 0;

    QByteArray lowerCaseResource = resource.toLower();

    QIOSWindow *platformWindow = static_cast<QIOSWindow *>(window->handle());

    if (lowerCaseResource == "uiview")
        return reinterpret_cast<void *>(platformWindow->winId());

    return 0;
}

QTouchDevice *QIOSIntegration::touchDevice()
{
    return m_touchDevice;
}

QT_END_NAMESPACE
