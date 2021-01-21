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

#ifndef QGENERICENGINE_H
#define QGENERICENGINE_H

#include "../qbearerengine_impl.h"

#include <QMap>
#include <QTimer>

QT_BEGIN_NAMESPACE

class QNetworkConfigurationPrivate;
class QNetworkSessionPrivate;

class QGenericEngine : public QBearerEngineImpl
{
    Q_OBJECT

public:
    QGenericEngine(QObject *parent = nullptr);
    ~QGenericEngine();

    QString getInterfaceFromId(const QString &id) override;
    bool hasIdentifier(const QString &id) override;

    void connectToId(const QString &id) override;
    void disconnectFromId(const QString &id) override;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate();

    QNetworkSession::State sessionStateForId(const QString &id) override;

    QNetworkConfigurationManager::Capabilities capabilities() const override;

    QNetworkSessionPrivate *createSessionBackend() override;

    QNetworkConfigurationPrivatePointer defaultConfiguration() override;

    bool requiresPolling() const override;

private Q_SLOTS:
    void doRequestUpdate();

private:
    QMap<QString, QString> configurationInterface;
};

QT_END_NAMESPACE

#endif

