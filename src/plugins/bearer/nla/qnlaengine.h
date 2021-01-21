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

#ifndef QNLAENGINE_P_H
#define QNLAENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "../qbearerengine_impl.h"

#include <QtNetwork/private/qnativesocketengine_p.h>

#include <QMap>

QT_BEGIN_NAMESPACE

class QNetworkConfigurationPrivate;
class QNlaThread;

class QWindowsSockInit2
{
public:
    QWindowsSockInit2();
    ~QWindowsSockInit2();
    int version;
};

class QNlaEngine : public QBearerEngineImpl
{
    Q_OBJECT

    friend class QNlaThread;

public:
    QNlaEngine(QObject *parent = 0);
    ~QNlaEngine();

    QString getInterfaceFromId(const QString &id);
    bool hasIdentifier(const QString &id);

    void connectToId(const QString &id);
    void disconnectFromId(const QString &id);

    Q_INVOKABLE void requestUpdate();

    QNetworkSession::State sessionStateForId(const QString &id);

    QNetworkConfigurationManager::Capabilities capabilities() const;

    QNetworkSessionPrivate *createSessionBackend();

    QNetworkConfigurationPrivatePointer defaultConfiguration();

private Q_SLOTS:
    void networksChanged();

private:
    QWindowsSockInit2 winSock;
    QNlaThread *nlaThread;
    QMap<uint, QString> configurationInterface;
};

QT_END_NAMESPACE

#endif
