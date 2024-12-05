// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qintegrityfbintegration.h"
#include "qintegrityfbscreen.h"
#include "qintegrityhidmanager.h"

#include <QtGui/private/qgenericunixfontdatabase_p.h>
#include <QtGui/private/qgenericunixeventdispatcher_p.h>

#include <QtFbSupport/private/qfbbackingstore_p.h>
#include <QtFbSupport/private/qfbwindow_p.h>
#include <QtFbSupport/private/qfbcursor_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

QIntegrityFbIntegration::QIntegrityFbIntegration(const QStringList &paramList)
    : m_fontDb(new QGenericUnixFontDatabase)
{
    m_primaryScreen = new QIntegrityFbScreen(paramList);
}

QIntegrityFbIntegration::~QIntegrityFbIntegration()
{
    QWindowSystemInterface::handleScreenRemoved(m_primaryScreen);
}

void QIntegrityFbIntegration::initialize()
{
    if (m_primaryScreen->initialize())
        QWindowSystemInterface::handleScreenAdded(m_primaryScreen);
    else
        qWarning("integrityfb: Failed to initialize screen");

    m_inputContext = QPlatformInputContextFactory::create();

    m_nativeInterface.reset(new QPlatformNativeInterface);

    if (!qEnvironmentVariableIntValue("QT_QPA_FB_DISABLE_INPUT"))
        createInputHandlers();
}

bool QIntegrityFbIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case WindowManagement: return false;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformBackingStore *QIntegrityFbIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QFbBackingStore(window);
}

QPlatformWindow *QIntegrityFbIntegration::createPlatformWindow(QWindow *window) const
{
    return new QFbWindow(window);
}

QAbstractEventDispatcher *QIntegrityFbIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QList<QPlatformScreen *> QIntegrityFbIntegration::screens() const
{
    QList<QPlatformScreen *> list;
    list.append(m_primaryScreen);
    return list;
}

QPlatformFontDatabase *QIntegrityFbIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QPlatformServices *QIntegrityFbIntegration::services() const
{
    if (m_services.isNull())
        m_services.reset(new QPlatformServices);

    return m_services.data();
}

void QIntegrityFbIntegration::createInputHandlers()
{
    new QIntegrityHIDManager("HID", "", this);
}

QPlatformNativeInterface *QIntegrityFbIntegration::nativeInterface() const
{
    return m_nativeInterface.data();
}

QT_END_NAMESPACE
