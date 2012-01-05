/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNETWORKSESSION_IMPL_H
#define QNETWORKSESSION_IMPL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qnetworksession_p.h>
#include <QtNetwork/qnetworkconfigmanager.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qtimer.h>
#include <QtCore/quuid.h>

#include <QtDBus/qdbusconnection.h>
#include <QtDBus/qdbusinterface.h>
#include <QtDBus/qdbusmessage.h>
#include <QtDBus/qdbusmetatype.h>

#include <icd/dbus_api.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QIcdEngine;

struct ICd2DetailsDBusStruct
{
    QString serviceType;
    uint serviceAttributes;
    QString setviceId;
    QString networkType;
    uint networkAttributes;
    QByteArray networkId;
};

typedef QList<ICd2DetailsDBusStruct> ICd2DetailsList;

class QNetworkSessionPrivateImpl : public QNetworkSessionPrivate
{
    Q_OBJECT

public:
    QNetworkSessionPrivateImpl(QIcdEngine *engine)
    :   engine(engine),
        connectFlags(ICD_CONNECTION_FLAG_USER_EVENT),
        currentState(QNetworkSession::Invalid),
        m_asynchCallActive(false)
    {
        m_stopTimer.setSingleShot(true);
        connect(&m_stopTimer, SIGNAL(timeout()), this, SLOT(finishStopBySendingClosedSignal()));

        QDBusConnection systemBus = QDBusConnection::connectToBus(
            QDBusConnection::SystemBus,
            QUuid::createUuid().toString());

        m_dbusInterface = new QDBusInterface(ICD_DBUS_API_INTERFACE,
                                         ICD_DBUS_API_PATH,
                                         ICD_DBUS_API_INTERFACE,
                                         systemBus,
                                         this);

        systemBus.connect(ICD_DBUS_API_INTERFACE,
                        ICD_DBUS_API_PATH,
                        ICD_DBUS_API_INTERFACE,
                        ICD_DBUS_API_CONNECT_SIG,
                        this,
                        SLOT(stateChange(const QDBusMessage&)));

        qDBusRegisterMetaType<ICd2DetailsDBusStruct>();
        qDBusRegisterMetaType<ICd2DetailsList>();

        m_connectRequestTimer.setSingleShot(true);
        connect(&m_connectRequestTimer, SIGNAL(timeout()), this, SLOT(connectTimeout()));
    }

    ~QNetworkSessionPrivateImpl()
    {
        cleanupSession();

        QDBusConnection::disconnectFromBus(m_dbusInterface->connection().name());
    }

    //called by QNetworkSession constructor and ensures
    //that the state is immediately updated (w/o actually opening
    //a session). Also this function should take care of 
    //notification hooks to discover future state changes.
    void syncStateWithInterface();

#ifndef QT_NO_NETWORKINTERFACE
    QNetworkInterface currentInterface() const;
#endif
    QVariant sessionProperty(const QString& key) const;
    void setSessionProperty(const QString& key, const QVariant& value);

    void open();
    void close();
    void stop();

    void migrate();
    void accept();
    void ignore();
    void reject();

    QString errorString() const; //must return translated string
    QNetworkSession::SessionError error() const;

    quint64 bytesWritten() const;
    quint64 bytesReceived() const;
    quint64 activeTime() const;

private:
    void updateStateFromServiceNetwork();
    void updateStateFromActiveConfig();

private Q_SLOTS:
    void do_open();
    void networkConfigurationsChanged();
    void iapStateChanged(const QString& iapid, uint icd_connection_state);
    void updateProxies(QNetworkSession::State newState);
    void finishStopBySendingClosedSignal();
    void stateChange(const QDBusMessage& rep);
    void connectTimeout();

private:
    QNetworkConfigurationManager manager;
    QIcdEngine *engine;

    struct Statistics {
        quint64 txData;
        quint64 rxData;
        quint64 activeTime;
    };

    // The config set on QNetworkSession.
    QNetworkConfiguration config;

    QNetworkConfiguration& copyConfig(QNetworkConfiguration &fromConfig, QNetworkConfiguration &toConfig, bool deepCopy = true);
    void clearConfiguration(QNetworkConfiguration &config);

    bool opened;
    icd_connection_flags connectFlags;

    QNetworkSession::SessionError lastError;

    QDateTime startTime;
    QString currentNetworkInterface;
    friend class IcdListener;
    void updateState(QNetworkSession::State);
    void updateIdentifier(const QString &newId);
    Statistics getStatistics() const;
    void cleanupSession(void);

    void updateProxyInformation();
    void clearProxyInformation();
    QNetworkSession::State currentState;

    QDBusInterface *m_dbusInterface;

    QTimer m_stopTimer;

    bool m_asynchCallActive;
    QTimer m_connectRequestTimer;
};

// Marshall the ICd2DetailsDBusStruct data into a D-Bus argument
QDBusArgument &operator<<(QDBusArgument &argument, const ICd2DetailsDBusStruct &icd2);

// Retrieve the ICd2DetailsDBusStruct data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, ICd2DetailsDBusStruct &icd2);

Q_DECLARE_METATYPE(ICd2DetailsDBusStruct);
Q_DECLARE_METATYPE(ICd2DetailsList);

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif //QNETWORKSESSIONPRIVATE_H

