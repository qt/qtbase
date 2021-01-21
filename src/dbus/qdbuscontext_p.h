/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the public API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//
//

#ifndef QDBUSCONTEXT_P_H
#define QDBUSCONTEXT_P_H

#include <QtDBus/private/qtdbusglobal_p.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusMessage;
class QDBusConnection;

class QDBusContext;
class QDBusContextPrivate
{
public:
    inline QDBusContextPrivate(const QDBusConnection &conn, const QDBusMessage &msg)
        : connection(conn), message(msg) {}

    QDBusConnection connection;
    const QDBusMessage &message;

    static QDBusContextPrivate *set(QObject *obj, QDBusContextPrivate *newContext);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif

