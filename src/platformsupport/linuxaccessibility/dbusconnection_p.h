/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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


#ifndef DBUSCONNECTION_H
#define DBUSCONNECTION_H

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

#include <QtCore/QString>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusVariant>

QT_BEGIN_NAMESPACE

class QDBusServiceWatcher;

class DBusConnection : public QObject
{
    Q_OBJECT

public:
    DBusConnection(QObject *parent = nullptr);
    QDBusConnection connection() const;
    bool isEnabled() const { return m_enabled; }

Q_SIGNALS:
    // Emitted when the global accessibility status changes to enabled
    void enabledChanged(bool enabled);

private Q_SLOTS:
    QString getAddressFromXCB();
    void serviceRegistered();
    void serviceUnregistered();
    void connectA11yBus(const QString &address);

    void dbusError(const QDBusError &error);

private:
    QString getAccessibilityBusAddress() const;

    QDBusServiceWatcher *dbusWatcher;
    QDBusConnection m_a11yConnection;
    bool m_enabled;
};

QT_END_NAMESPACE

#endif // DBUSCONNECTION_H
