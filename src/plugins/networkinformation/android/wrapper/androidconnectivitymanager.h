// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef ANDROIDCONNECTIVITYMANAGER_H
#define ANDROIDCONNECTIVITYMANAGER_H

#include <QObject>
#include <QtCore/qjniobject.h>

QT_BEGIN_NAMESPACE

class AndroidConnectivityManager : public QObject
{
    Q_OBJECT
public:
    // Keep synchronized with AndroidConnectivity in QtAndroidNetworkInformation.java
    enum class AndroidConnectivity { Connected, Unknown, Disconnected };
    Q_ENUM(AndroidConnectivity);

    // Keep synchronized with Transport in QtAndroidNetworkInformation.java
    enum class AndroidTransport {
        Unknown,
        Bluetooth,
        Cellular,
        Ethernet,
        LoWPAN,
        Usb,
        WiFi,
        WiFiAware,
    };
    Q_ENUM(AndroidTransport);

    static AndroidConnectivityManager *getInstance();
    ~AndroidConnectivityManager();

    inline bool isValid() const;

Q_SIGNALS:
    void connectivityChanged(AndroidConnectivity connectivity);
    void captivePortalChanged(bool state);
    void transportMediumChanged(AndroidTransport transport);
    void meteredChanged(bool state);

private:
    friend struct AndroidConnectivityManagerInstance;
    AndroidConnectivityManager();
    bool registerNatives() const;

    Q_DISABLE_COPY_MOVE(AndroidConnectivityManager);
};

QT_END_NAMESPACE

#endif // ANDROIDCONNECTIVITYMANAGER_H
