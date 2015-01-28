/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
