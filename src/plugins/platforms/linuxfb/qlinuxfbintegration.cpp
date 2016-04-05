/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlinuxfbintegration.h"
#include "qlinuxfbscreen.h"

#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixservices_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>

#include <QtPlatformSupport/private/qfbvthandler_p.h>
#include <QtPlatformSupport/private/qfbbackingstore_p.h>
#include <QtPlatformSupport/private/qfbwindow_p.h>
#include <QtPlatformSupport/private/qfbcursor_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatforminputcontextfactory_p.h>

#ifndef QT_NO_LIBINPUT
#include <QtPlatformSupport/private/qlibinputhandler_p.h>
#endif

#if !defined(QT_NO_EVDEV) && !defined(Q_OS_ANDROID)
#include <QtPlatformSupport/private/qevdevmousemanager_p.h>
#include <QtPlatformSupport/private/qevdevkeyboardmanager_p.h>
#include <QtPlatformSupport/private/qevdevtouchmanager_p.h>
#endif

#if !defined(QT_NO_TSLIB) && !defined(Q_OS_ANDROID)
#include <QtPlatformSupport/private/qtslib_p.h>
#endif

QT_BEGIN_NAMESPACE

QLinuxFbIntegration::QLinuxFbIntegration(const QStringList &paramList)
    : m_fontDb(new QGenericUnixFontDatabase),
      m_services(new QGenericUnixServices)
{
    m_primaryScreen = new QLinuxFbScreen(paramList);
}

QLinuxFbIntegration::~QLinuxFbIntegration()
{
    destroyScreen(m_primaryScreen);
}

void QLinuxFbIntegration::initialize()
{
    if (m_primaryScreen->initialize())
        screenAdded(m_primaryScreen);
    else
        qWarning("linuxfb: Failed to initialize screen");

    m_inputContext = QPlatformInputContextFactory::create();

    m_nativeInterface.reset(new QPlatformNativeInterface);

    m_vtHandler.reset(new QFbVtHandler);

    if (!qEnvironmentVariableIntValue("QT_QPA_FB_DISABLE_INPUT"))
        createInputHandlers();
}

bool QLinuxFbIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case WindowManagement: return false;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformBackingStore *QLinuxFbIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QFbBackingStore(window);
}

QPlatformWindow *QLinuxFbIntegration::createPlatformWindow(QWindow *window) const
{
    return new QFbWindow(window);
}

QAbstractEventDispatcher *QLinuxFbIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QList<QPlatformScreen *> QLinuxFbIntegration::screens() const
{
    QList<QPlatformScreen *> list;
    list.append(m_primaryScreen);
    return list;
}

QPlatformFontDatabase *QLinuxFbIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QPlatformServices *QLinuxFbIntegration::services() const
{
    return m_services.data();
}

void QLinuxFbIntegration::createInputHandlers()
{
#ifndef QT_NO_LIBINPUT
    if (!qEnvironmentVariableIntValue("QT_QPA_FB_NO_LIBINPUT")) {
        new QLibInputHandler(QLatin1String("libinput"), QString());
        return;
    }
#endif

#if !defined(QT_NO_EVDEV) && !defined(Q_OS_ANDROID)
    new QEvdevKeyboardManager(QLatin1String("EvdevKeyboard"), QString(), this);
    new QEvdevMouseManager(QLatin1String("EvdevMouse"), QString(), this);
#ifndef QT_NO_TSLIB
    const bool useTslib = qEnvironmentVariableIntValue("QT_QPA_FB_TSLIB");
    if (useTslib)
        new QTsLibMouseHandler(QLatin1String("TsLib"), QString());
    else
#endif // QT_NO_TSLIB
        new QEvdevTouchManager(QLatin1String("EvdevTouch"), QString() /* spec */, this);
#endif
}

QPlatformNativeInterface *QLinuxFbIntegration::nativeInterface() const
{
    return m_nativeInterface.data();
}

QT_END_NAMESPACE
