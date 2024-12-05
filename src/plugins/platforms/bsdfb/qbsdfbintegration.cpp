// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2015-2016 Oleksandr Tymoshenko <gonzo@bluezbox.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbsdfbintegration.h"
#include "qbsdfbscreen.h"

#include <QtGui/private/qgenericunixfontdatabase_p.h>
#include <QtGui/private/qgenericunixeventdispatcher_p.h>

#include <QtFbSupport/private/qfbvthandler_p.h>
#include <QtFbSupport/private/qfbbackingstore_p.h>
#include <QtFbSupport/private/qfbwindow_p.h>
#include <QtFbSupport/private/qfbcursor_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformservices.h>

#if QT_CONFIG(tslib)
#include <QtInputSupport/private/qtslib_p.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QBsdFbIntegration::QBsdFbIntegration(const QStringList &paramList)
{
    m_fontDb.reset(new QGenericUnixFontDatabase);
    m_primaryScreen.reset(new QBsdFbScreen(paramList));
}

QBsdFbIntegration::~QBsdFbIntegration()
{
    QWindowSystemInterface::handleScreenRemoved(m_primaryScreen.take());
}

void QBsdFbIntegration::initialize()
{
    if (m_primaryScreen->initialize())
        QWindowSystemInterface::handleScreenAdded(m_primaryScreen.data());
    else
        qWarning("bsdfb: Failed to initialize screen");

    m_inputContext.reset(QPlatformInputContextFactory::create());
    m_nativeInterface.reset(new QPlatformNativeInterface);
    m_vtHandler.reset(new QFbVtHandler);

    if (!qEnvironmentVariableIntValue("QT_QPA_FB_DISABLE_INPUT"))
        createInputHandlers();
}

bool QBsdFbIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
        return true;
    case WindowManagement:
        return false;
    case RhiBasedRendering:
        return false;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformBackingStore *QBsdFbIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QFbBackingStore(window);
}

QPlatformWindow *QBsdFbIntegration::createPlatformWindow(QWindow *window) const
{
    return new QFbWindow(window);
}

QAbstractEventDispatcher *QBsdFbIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QList<QPlatformScreen *> QBsdFbIntegration::screens() const
{
    QList<QPlatformScreen *> list;
    list.append(m_primaryScreen.data());
    return list;
}

QPlatformFontDatabase *QBsdFbIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QPlatformServices *QBsdFbIntegration::services() const
{
    if (m_services.isNull())
        m_services.reset(new QPlatformServices);

    return m_services.data();
}

void QBsdFbIntegration::createInputHandlers()
{
#if QT_CONFIG(tslib)
    const bool useTslib = qEnvironmentVariableIntValue("QT_QPA_FB_TSLIB");
    if (useTslib)
        new QTsLibMouseHandler("TsLib"_L1, QString());
#endif
}

QPlatformNativeInterface *QBsdFbIntegration::nativeInterface() const
{
    return m_nativeInterface.data();
}

QT_END_NAMESPACE
