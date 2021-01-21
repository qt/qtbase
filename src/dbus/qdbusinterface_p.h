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

#ifndef QDBUSINTERFACE_P_H
#define QDBUSINTERFACE_P_H

#include <QtDBus/private/qtdbusglobal_p.h>
#include "qdbusabstractinterface_p.h"
#include "qdbusmetaobject_p.h"
#include <qdbusinterface.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusInterfacePrivate: public QDBusAbstractInterfacePrivate
{
public:
    Q_DECLARE_PUBLIC(QDBusInterface)

    QDBusMetaObject *metaObject;

    QDBusInterfacePrivate(const QString &serv, const QString &p, const QString &iface,
                          const QDBusConnection &con);
    ~QDBusInterfacePrivate();

    int metacall(QMetaObject::Call c, int id, void **argv);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
