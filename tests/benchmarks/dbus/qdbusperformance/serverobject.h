// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#ifndef SERVEROBJECT_H
#define SERVEROBJECT_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusVariant>

class ServerObject: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.autotests.Performance")
public:
    ServerObject(const QString &objectPath, QDBusConnection conn, QObject *parent = nullptr)
        : QObject(parent)
    {
        conn.registerObject(objectPath, this, QDBusConnection::ExportAllSlots);
    }

public slots:
    Q_NOREPLY void noReply(const QByteArray &)
    {
        // black hole
    }
    Q_NOREPLY void noReply(const QString &)
    {
        // black hole
    }
    Q_NOREPLY void noReply(const QDBusVariant &)
    {
        // black hole
    }

    int size(const QByteArray &data)
    {
        return data.size();
    }
    int size(const QString &data)
    {
        return data.size();
    }
    int size(const QDBusVariant &data)
    {
        QVariant v = data.variant();
        switch (v.typeId()) {
        case QMetaType::QByteArray:
            return v.toByteArray().size();
        case QMetaType::QStringList:
            return v.toStringList().size();
        case QMetaType::QString:
        default:
            return v.toString().size();
        }
    }

    QByteArray echo(const QByteArray &data)
    {
        return data;
    }
    QString echo(const QString &data)
    {
        return data;
    }
    QDBusVariant echo(const QDBusVariant &data)
    {
        return data;
    }

    void nothing()
    {
    }
};

#endif
