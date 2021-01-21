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

#include <QtGui/qtgui-config.h>

QT_BEGIN_NAMESPACE

class QDBusServiceWatcher;
#ifndef QT_NO_SYSTEMTRAYICON
class QDBusTrayIcon;
#endif // QT_NO_SYSTEMTRAYICON

class QDBusMenuConnection : public QObject
{
    Q_OBJECT

public:
    QDBusMenuConnection(QObject *parent = nullptr, const QString &serviceName = QString());
    QDBusConnection connection() const { return m_connection; }
    QDBusServiceWatcher *dbusWatcher() const { return m_dbusWatcher; }
    bool isStatusNotifierHostRegistered() const { return m_statusNotifierHostRegistered; }
#ifndef QT_NO_SYSTEMTRAYICON
    bool registerTrayIconMenu(QDBusTrayIcon *item);
    void unregisterTrayIconMenu(QDBusTrayIcon *item);
    bool registerTrayIcon(QDBusTrayIcon *item);
    bool registerTrayIconWithWatcher(QDBusTrayIcon *item);
    bool unregisterTrayIcon(QDBusTrayIcon *item);
#endif // QT_NO_SYSTEMTRAYICON

Q_SIGNALS:
#ifndef QT_NO_SYSTEMTRAYICON
    void trayIconRegistered();
#endif // QT_NO_SYSTEMTRAYICON

private Q_SLOTS:
    void dbusError(const QDBusError &error);

private:
    QDBusConnection m_connection;
    QDBusServiceWatcher *m_dbusWatcher;
    bool m_statusNotifierHostRegistered;
};

QT_END_NAMESPACE

#endif // DBUSCONNECTION_H
