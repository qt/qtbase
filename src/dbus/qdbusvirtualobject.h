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

#ifndef QDBUSVIRTUALOBJECT_H
#define QDBUSVIRTUALOBJECT_H

#include <QtDBus/qtdbusglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qobject.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE


class QDBusMessage;
class QDBusConnection;

class Q_DBUS_EXPORT QDBusVirtualObject : public QObject
{
    Q_OBJECT
public:
    explicit QDBusVirtualObject(QObject *parent = nullptr);
    virtual ~QDBusVirtualObject();

    virtual QString introspect(const QString &path) const = 0;
    virtual bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) = 0;

private:
    Q_DISABLE_COPY(QDBusVirtualObject)
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
