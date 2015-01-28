/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

/*
    This file was originally created by qdbusxml2cpp version 0.8
    Command line was:
    qdbusxml2cpp -a dbusmenu ../../3rdparty/dbus-ifaces/dbus-menu.xml

    However it is maintained manually.
*/

#include "qdbusmenuadaptor_p.h"
#include "qdbusplatformmenu_p.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

QDBusMenuAdaptor::QDBusMenuAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(true);
}

QDBusMenuAdaptor::~QDBusMenuAdaptor()
{
}

QString QDBusMenuAdaptor::status() const
{
    qCDebug(qLcMenu);
    return QLatin1String("normal");
}

QString QDBusMenuAdaptor::textDirection() const
{
    return QLocale().textDirection() == Qt::RightToLeft ? QLatin1String("rtl") : QLatin1String("ltr");
}

uint QDBusMenuAdaptor::version() const
{
    return 4;
}

bool QDBusMenuAdaptor::AboutToShow(int id)
{
    qCDebug(qLcMenu) << id;
    return false;
}

QList<int> QDBusMenuAdaptor::AboutToShowGroup(const QList<int> &ids, QList<int> &idErrors)
{
    qCDebug(qLcMenu) << ids;
    Q_UNUSED(idErrors)
    idErrors.clear();
    return QList<int>(); // updatesNeeded
}

void QDBusMenuAdaptor::Event(int id, const QString &eventId, const QDBusVariant &data, uint timestamp)
{
    Q_UNUSED(data)
    Q_UNUSED(timestamp)
    QDBusPlatformMenuItem *item = QDBusPlatformMenuItem::byId(id);
    qCDebug(qLcMenu) << id << (item ? item->text() : QLatin1String("")) << eventId;
    // Events occur on both menus and menuitems, but we only care if it's an item being clicked.
    if (item && eventId == QLatin1String("clicked"))
        item->trigger();
}

void QDBusMenuAdaptor::EventGroup(const QDBusMenuEventList &events)
{
    Q_FOREACH (const QDBusMenuEvent &ev, events)
        Event(ev.m_id, ev.m_eventId, ev.m_data, ev.m_timestamp);
}

QDBusMenuItemList QDBusMenuAdaptor::GetGroupProperties(const QList<int> &ids, const QStringList &propertyNames)
{
    qCDebug(qLcMenu) << ids << propertyNames << "=>" << QDBusMenuItem::items(ids, propertyNames);
    return QDBusMenuItem::items(ids, propertyNames);
}

uint QDBusMenuAdaptor::GetLayout(int parentId, int recursionDepth, const QStringList &propertyNames, QDBusMenuLayoutItem &layout)
{
    uint ret = layout.populate(parentId, recursionDepth, propertyNames);
    qCDebug(qLcMenu) << parentId << "depth" << recursionDepth << propertyNames << layout.m_id << layout.m_properties << "revision" << ret << layout;
    return ret;
}

QDBusVariant QDBusMenuAdaptor::GetProperty(int id, const QString &name)
{
    qCDebug(qLcMenu) << id << name;
    // handle method call com.canonical.dbusmenu.GetProperty
    QDBusVariant value;
    return value;
}

QT_END_NAMESPACE
