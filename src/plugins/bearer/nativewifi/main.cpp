/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qnativewifiengine.h"
#include "platformdefs.h"

#include <QtCore/qmutex.h>
#include <QtCore/qlibrary.h>

#include <QtNetwork/private/qbearerplugin_p.h>

#include <QtCore/qdebug.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

static bool resolveLibraryInternal()
{
    QLibrary wlanapiLib(QLatin1String("wlanapi"));
    local_WlanOpenHandle = (WlanOpenHandleProto)
                           wlanapiLib.resolve("WlanOpenHandle");
    local_WlanRegisterNotification = (WlanRegisterNotificationProto)
                                     wlanapiLib.resolve("WlanRegisterNotification");
    local_WlanEnumInterfaces = (WlanEnumInterfacesProto)
                               wlanapiLib.resolve("WlanEnumInterfaces");
    local_WlanGetAvailableNetworkList = (WlanGetAvailableNetworkListProto)
                                        wlanapiLib.resolve("WlanGetAvailableNetworkList");
    local_WlanQueryInterface = (WlanQueryInterfaceProto)
                               wlanapiLib.resolve("WlanQueryInterface");
    local_WlanConnect = (WlanConnectProto)
                        wlanapiLib.resolve("WlanConnect");
    local_WlanDisconnect = (WlanDisconnectProto)
                           wlanapiLib.resolve("WlanDisconnect");
    local_WlanScan = (WlanScanProto)
                     wlanapiLib.resolve("WlanScan");
    local_WlanFreeMemory = (WlanFreeMemoryProto)
                           wlanapiLib.resolve("WlanFreeMemory");
    local_WlanCloseHandle = (WlanCloseHandleProto)
                            wlanapiLib.resolve("WlanCloseHandle");
    return true;
}
Q_GLOBAL_STATIC_WITH_ARGS(bool, resolveLibrary, (resolveLibraryInternal()))

class QNativeWifiEnginePlugin : public QBearerEnginePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QBearerEngineFactoryInterface" FILE "nativewifi.json")

public:
    QNativeWifiEnginePlugin();
    ~QNativeWifiEnginePlugin();

    QBearerEngine *create(const QString &key) const;
};

QNativeWifiEnginePlugin::QNativeWifiEnginePlugin()
{
}

QNativeWifiEnginePlugin::~QNativeWifiEnginePlugin()
{
}

QBearerEngine *QNativeWifiEnginePlugin::create(const QString &key) const
{
    if (key != QLatin1String("nativewifi"))
        return 0;

    resolveLibrary();

    // native wifi dll not available
    if (!local_WlanOpenHandle)
        return 0;

    QNativeWifiEngine *engine = new QNativeWifiEngine;

    // could not initialise subsystem
    if (engine && !engine->available()) {
        delete engine;
        return 0;
    }

    return engine;
}

QT_END_NAMESPACE

#include "main.moc"

#endif // QT_NO_BEARERMANAGEMENT
