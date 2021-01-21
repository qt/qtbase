/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QNETWORKCONFIGMANAGER_H
#define QNETWORKCONFIGMANAGER_H

#if 0
#pragma qt_class(QNetworkConfigurationManager)
#endif

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qobject.h>
#include <QtNetwork/qnetworkconfiguration.h>

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QNetworkConfigurationManagerPrivate;
class QT_DEPRECATED_BEARER_MANAGEMENT Q_NETWORK_EXPORT QNetworkConfigurationManager : public QObject
{
    Q_OBJECT

public:
    enum Capability {
         CanStartAndStopInterfaces  = 0x00000001,
         DirectConnectionRouting = 0x00000002,
         SystemSessionSupport = 0x00000004,
         ApplicationLevelRoaming = 0x00000008,
         ForcedRoaming = 0x00000010,
         DataStatistics = 0x00000020,
         NetworkSessionRequired = 0x00000040
    };

    Q_DECLARE_FLAGS(Capabilities, Capability)

    explicit QNetworkConfigurationManager(QObject *parent = nullptr);
    virtual ~QNetworkConfigurationManager();

    QNetworkConfigurationManager::Capabilities capabilities() const;

    QNetworkConfiguration defaultConfiguration() const;
    QList<QNetworkConfiguration> allConfigurations(QNetworkConfiguration::StateFlags flags = QNetworkConfiguration::StateFlags()) const;
    QNetworkConfiguration configurationFromIdentifier(const QString &identifier) const;

    bool isOnline() const;

public Q_SLOTS:
    void updateConfigurations();

Q_SIGNALS:
    void configurationAdded(const QNetworkConfiguration &config);
    void configurationRemoved(const QNetworkConfiguration &config);
    void configurationChanged(const QNetworkConfiguration &config);
    void onlineStateChanged(bool isOnline);
    void updateCompleted();

private:
    Q_DISABLE_COPY(QNetworkConfigurationManager)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QNetworkConfigurationManager::Capabilities)

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

QT_WARNING_POP

#endif // QNETWORKCONFIGMANAGER_H
