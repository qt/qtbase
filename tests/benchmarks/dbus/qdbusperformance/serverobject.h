/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef SERVEROBJECT_H
#define SERVEROBJECT_H

#include <QObject>
#include <QtDBus/QtDBus>

class ServerObject: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.autotests.Performance")
public:
    ServerObject(const QString &objectPath, QDBusConnection conn, QObject *parent = 0)
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
        switch (v.type())
        {
        case QVariant::ByteArray:
            return v.toByteArray().size();
        case QVariant::StringList:
            return v.toStringList().size();
        case QVariant::String:
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
