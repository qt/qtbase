/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtDBus module of the Qt Toolkit.
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
#ifndef QDBUSSERVER_H
#define QDBUSSERVER_H

#include <QtDBus/qtdbusglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE


class QDBusConnectionPrivate;
class QDBusError;
class QDBusConnection;

class Q_DBUS_EXPORT QDBusServer: public QObject
{
    Q_OBJECT
public:
    explicit QDBusServer(const QString &address, QObject *parent = nullptr);
    explicit QDBusServer(QObject *parent = nullptr);
    virtual ~QDBusServer();

    bool isConnected() const;
    QDBusError lastError() const;
    QString address() const;

    void setAnonymousAuthenticationAllowed(bool value);
    bool isAnonymousAuthenticationAllowed() const;

Q_SIGNALS:
    void newConnection(const QDBusConnection &connection);

private:
    Q_DISABLE_COPY(QDBusServer)
    Q_PRIVATE_SLOT(d, void _q_newConnection(QDBusConnectionPrivate*))
    QDBusConnectionPrivate *d;
    friend class QDBusConnectionPrivate;
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
