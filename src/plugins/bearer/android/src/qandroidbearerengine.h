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

#ifndef QANDROIDBEARERENGINE_H
#define QANDROIDBEARERENGINE_H

#include "../../qbearerengine_impl.h"

#include <QAbstractEventDispatcher>
#include <QAbstractNativeEventFilter>
#include <QtCore/private/qjni_p.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QNetworkConfigurationPrivate;
class QNetworkSessionPrivate;
class AndroidConnectivityManager;

class QAndroidBearerEngine : public QBearerEngineImpl
{
    Q_OBJECT

public:
    explicit QAndroidBearerEngine(QObject *parent = 0);
    ~QAndroidBearerEngine() Q_DECL_OVERRIDE;

    QString getInterfaceFromId(const QString &id) Q_DECL_OVERRIDE;
    bool hasIdentifier(const QString &id) Q_DECL_OVERRIDE;
    void connectToId(const QString &id) Q_DECL_OVERRIDE;
    void disconnectFromId(const QString &id) Q_DECL_OVERRIDE;
    QNetworkSession::State sessionStateForId(const QString &id) Q_DECL_OVERRIDE;
    QNetworkConfigurationManager::Capabilities capabilities() const Q_DECL_OVERRIDE;
    QNetworkSessionPrivate *createSessionBackend() Q_DECL_OVERRIDE;
    QNetworkConfigurationPrivatePointer defaultConfiguration() Q_DECL_OVERRIDE;
    bool requiresPolling() const Q_DECL_OVERRIDE;
    quint64 bytesWritten(const QString &id) Q_DECL_OVERRIDE;
    quint64 bytesReceived(const QString &id) Q_DECL_OVERRIDE;
    quint64 startTime(const QString &id) Q_DECL_OVERRIDE;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate();

private Q_SLOTS:
    void updateConfigurations();

private:
    QJNIObjectPrivate m_networkReceiver;
    AndroidConnectivityManager *m_connectivityManager;
    QMap<QString, QString> m_configurationInterface;
};


QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif // QANDROIDBEARERENGINE_H
