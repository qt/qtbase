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

#ifndef QCONNMANENGINE_P_H
#define QCONNMANENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "../qbearerengine_impl.h"

#include "qconnmanservice_linux_p.h"
#include "../linux_common/qofonoservice_linux_p.h"

#include <QMap>
#include <QVariant>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QConnmanEngine : public QBearerEngineImpl
{
    Q_OBJECT

public:
    QConnmanEngine(QObject *parent = nullptr);
    ~QConnmanEngine();

    bool connmanAvailable() const;

    virtual QString getInterfaceFromId(const QString &id);
    bool hasIdentifier(const QString &id);

    virtual void connectToId(const QString &id);
    virtual void disconnectFromId(const QString &id);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate();

    QNetworkSession::State sessionStateForId(const QString &id);
    QNetworkSessionPrivate *createSessionBackend();

    virtual quint64 bytesWritten(const QString &id);
    virtual quint64 bytesReceived(const QString &id);
    virtual quint64 startTime(const QString &id);

    virtual QNetworkConfigurationManager::Capabilities capabilities() const;
    virtual QNetworkConfigurationPrivatePointer defaultConfiguration();

    QList<QNetworkConfigurationPrivate *> getConfigurations();

private Q_SLOTS:

    void doRequestUpdate();
    void updateServices(const ConnmanMapList &changed, const QList<QDBusObjectPath> &removed);

    void servicesReady(const QStringList &);
    void finishedScan(bool error);
    void changedModem();
    void serviceStateChanged(const QString &state);
    void configurationChange(QConnmanServiceInterface * service);
    void reEvaluateCellular();
private:
    QConnmanManagerInterface *connmanManager;

    QOfonoManagerInterface *ofonoManager;
    QOfonoNetworkRegistrationInterface *ofonoNetwork;
    QOfonoDataConnectionManagerInterface *ofonoContextManager;

    QList<QNetworkConfigurationPrivate *> foundConfigurations;

    QString networkFromId(const QString &id);

    QNetworkConfiguration::StateFlags getStateForService(const QString &service);
    QNetworkConfiguration::BearerType typeToBearer(const QString &type);

    void removeConfiguration(const QString &servicePath);
    void addServiceConfiguration(const QString &servicePath);
    QDateTime activeTime;


    QMap<QString,QConnmanTechnologyInterface *> technologies; // techpath, tech interface
    QMap<QString,QString> configInterfaces; // id, interface name
    QList<QString> serviceNetworks; //servpath

    QNetworkConfiguration::BearerType ofonoTechToBearerType(const QString &type);
    bool isRoamingAllowed(const QString &context);
    QMap <QString,QConnmanServiceInterface *> connmanServiceInterfaces;

protected:
    bool requiresPolling() const;
};


QT_END_NAMESPACE

#endif // QT_NO_DBUS

#endif

