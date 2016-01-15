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

#ifndef QCOREWLANENGINE_H
#define QCOREWLANENGINE_H

#include "../qbearerengine_impl.h"

#include <QMap>
#include <QTimer>
#include <SystemConfiguration/SystemConfiguration.h>
#include <QThread>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QNetworkConfigurationPrivate;
class QScanThread;

class QCoreWlanEngine : public QBearerEngineImpl
{
     friend class QScanThread;
    Q_OBJECT

public:
    QCoreWlanEngine(QObject *parent = 0);
    ~QCoreWlanEngine();

    QString getInterfaceFromId(const QString &id);
    bool hasIdentifier(const QString &id);

    void connectToId(const QString &id);
    void disconnectFromId(const QString &id);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate();

    QNetworkSession::State sessionStateForId(const QString &id);

    quint64 bytesWritten(const QString &id);
    quint64 bytesReceived(const QString &id);
    quint64 startTime(const QString &id);

    QNetworkConfigurationManager::Capabilities capabilities() const;

    QNetworkSessionPrivate *createSessionBackend();

    QNetworkConfigurationPrivatePointer defaultConfiguration();

    bool requiresPolling() const;

private Q_SLOTS:
    void doRequestUpdate();
    void networksChanged();
    void checkDisconnect();

private:
    bool isWifiReady(const QString &dev);
    QList<QNetworkConfigurationPrivate *> foundConfigurations;

    SCDynamicStoreRef storeSession;
    CFRunLoopSourceRef runloopSource;
    bool hasWifi;
    bool scanning;
    QScanThread *scanThread;

    quint64 getBytes(const QString &interfaceName,bool b);
    QString disconnectedInterfaceString;

protected:
    void startNetworkChangeLoop();

};

class QScanThread : public QThread
{
    Q_OBJECT

public:
    QScanThread(QObject *parent = 0);
    ~QScanThread();

    void quit();
    QList<QNetworkConfigurationPrivate *> getConfigurations();
    QString interfaceName;
    QMap<QString, QString> configurationInterface;
    void getUserConfigurations();
    QString getNetworkNameFromSsid(const QString &ssid);
    QString getSsidFromNetworkName(const QString &name);
    bool isKnownSsid(const QString &ssid);
    QMap<QString, QMap<QString,QString> > userProfiles;

signals:
    void networksChanged();

protected:
    void run();

private:
    QList<QNetworkConfigurationPrivate *> fetchedConfigurations;
    QMutex mutex;
    QStringList foundNetwork(const QString &id, const QString &ssid, const QNetworkConfiguration::StateFlags state, const QString &interfaceName, const QNetworkConfiguration::Purpose purpose);

};

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif
