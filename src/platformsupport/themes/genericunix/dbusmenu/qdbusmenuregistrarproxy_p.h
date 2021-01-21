/****************************************************************************
**
** Copyright (C) 2016 Dmitry Shachnev <mitya57@gmail.com>
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

/*
 * This file was originally created by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -p qdbusmenuregistrarproxy ../../3rdparty/dbus-ifaces/com.canonical.AppMenu.Registrar.xml
 *
 * However it is maintained manually.
 */

#ifndef QDBUSMENUREGISTRARPROXY_P_H
#define QDBUSMENUREGISTRARPROXY_P_H

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

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtDBus/QDBusAbstractInterface>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>

QT_BEGIN_NAMESPACE

/*
 * Proxy class for interface com.canonical.AppMenu.Registrar
 */
class QDBusMenuRegistrarInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    {
        return "com.canonical.AppMenu.Registrar";
    }

public:
    explicit QDBusMenuRegistrarInterface(const QString &service,
                                         const QString &path,
                                         const QDBusConnection &connection,
                                         QObject *parent = nullptr);

    ~QDBusMenuRegistrarInterface();

public Q_SLOTS: // METHODS
    QDBusPendingReply<QString, QDBusObjectPath> GetMenuForWindow(uint windowId)
    {
        return asyncCall(QStringLiteral("GetMenuForWindow"), windowId);
    }
    QDBusReply<QString> GetMenuForWindow(uint windowId, QDBusObjectPath &menuObjectPath)
    {
        QDBusMessage reply = call(QDBus::Block, QStringLiteral("GetMenuForWindow"), windowId);
        QList<QVariant> arguments = reply.arguments();
        if (reply.type() == QDBusMessage::ReplyMessage && arguments.count() == 2)
            menuObjectPath = qdbus_cast<QDBusObjectPath>(arguments.at(1));
        return reply;
    }

    QDBusPendingReply<> RegisterWindow(uint windowId, const QDBusObjectPath &menuObjectPath)
    {
        return asyncCall(QStringLiteral("RegisterWindow"), windowId, menuObjectPath);
    }

    QDBusPendingReply<> UnregisterWindow(uint windowId)
    {
        return asyncCall(QStringLiteral("UnregisterWindow"), windowId);
    }
};

QT_END_NAMESPACE

#endif // QDBUSMENUREGISTRARPROXY_P_H
