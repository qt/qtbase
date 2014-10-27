/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNMDBUSHELPERPRIVATE_H
#define QNMDBUSHELPERPRIVATE_H

#include <QDBusObjectPath>
#include <QDBusContext>
#include <QMap>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QNmDBusHelper: public QObject, protected QDBusContext
 {
     Q_OBJECT
 public:
    QNmDBusHelper(QObject *parent = 0);
    ~QNmDBusHelper();

 public slots:
    void deviceStateChanged(quint32);
    void slotAccessPointAdded(QDBusObjectPath);
    void slotAccessPointRemoved(QDBusObjectPath);
    void slotPropertiesChanged(QMap<QString,QVariant>);
    void slotSettingsRemoved();
    void activeConnectionPropertiesChanged(QMap<QString,QVariant>);

Q_SIGNALS:
    void pathForStateChanged(const QString &, quint32);
    void pathForAccessPointAdded(const QString &);
    void pathForAccessPointRemoved(const QString &);
    void pathForPropertiesChanged(const QString &, QMap<QString,QVariant>);
    void pathForSettingsRemoved(const QString &);
    void pathForConnectionsChanged(const QStringList &pathsList);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS

#endif// QNMDBUSHELPERPRIVATE_H
