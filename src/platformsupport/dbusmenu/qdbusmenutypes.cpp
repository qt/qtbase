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

#include "qdbusmenutypes_p.h"

#include <QDBusConnection>
#include <QDBusMetaType>
#include <QImage>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QDebug>
#include <QtEndian>
#include <QBuffer>
#include <qpa/qplatformmenu.h>
#include "qdbusplatformmenu_p.h"

QT_BEGIN_NAMESPACE

const QDBusArgument &operator<<(QDBusArgument &arg, const QDBusMenuItem &item)
{
    arg.beginStructure();
    arg << item.m_id << item.m_properties;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, QDBusMenuItem &item)
{
    arg.beginStructure();
    arg >> item.m_id >> item.m_properties;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator<<(QDBusArgument &arg, const QDBusMenuItemKeys &keys)
{
    arg.beginStructure();
    arg << keys.id << keys.properties;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, QDBusMenuItemKeys &keys)
{
    arg.beginStructure();
    arg >> keys.id >> keys.properties;
    arg.endStructure();
    return arg;
}

uint QDBusMenuLayoutItem::populate(int id, int depth, const QStringList &propertyNames)
{
    qCDebug(qLcMenu) << id << "depth" << depth << propertyNames;
    m_id = id;
    if (id == 0) {
        m_properties.insert(QLatin1String("children-display"), QLatin1String("submenu"));
        Q_FOREACH (const QDBusPlatformMenu *menu, QDBusPlatformMenu::topLevelMenus()) {
            if (menu)
                populate(menu, depth, propertyNames);
        }
        return 1; // revision
    }

    const QDBusPlatformMenu *menu = QDBusPlatformMenu::byId(id);
    if (!menu) {
        QDBusPlatformMenuItem *item = QDBusPlatformMenuItem::byId(id);
        if (item)
            menu = static_cast<const QDBusPlatformMenu *>(item->menu());
    }
    if (depth != 0 && menu)
        populate(menu, depth, propertyNames);
    if (menu)
        return menu->revision();

    return 1; // revision
}

void QDBusMenuLayoutItem::populate(const QDBusPlatformMenu *menu, int depth, const QStringList &propertyNames)
{
    Q_FOREACH (QDBusPlatformMenuItem *item, menu->items()) {
        QDBusMenuLayoutItem child;
        child.populate(item, depth - 1, propertyNames);
        m_children << child;
    }
}

void QDBusMenuLayoutItem::populate(const QDBusPlatformMenuItem *item, int depth, const QStringList &propertyNames)
{
    Q_UNUSED(depth)
    Q_UNUSED(propertyNames)
    m_id = item->dbusID();
    QDBusMenuItem proxy(item);
    m_properties = proxy.m_properties;
}

const QDBusArgument &operator<<(QDBusArgument &arg, const QDBusMenuLayoutItem &item)
{
    arg.beginStructure();
    arg << item.m_id << item.m_properties;
    arg.beginArray(qMetaTypeId<QDBusVariant>());
    foreach (const QDBusMenuLayoutItem& child, item.m_children)
        arg << QDBusVariant(QVariant::fromValue<QDBusMenuLayoutItem>(child));
    arg.endArray();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, QDBusMenuLayoutItem &item)
{
    arg.beginStructure();
    arg >> item.m_id >> item.m_properties;
    arg.beginArray();
    while (!arg.atEnd()) {
        QDBusVariant dbusVariant;
        arg >> dbusVariant;
        QDBusArgument childArgument = dbusVariant.variant().value<QDBusArgument>();

        QDBusMenuLayoutItem child;
        childArgument >> child;
        item.m_children.append(child);
    }
    arg.endArray();
    arg.endStructure();
    return arg;
}

void QDBusMenuItem::registerDBusTypes()
{
    qDBusRegisterMetaType<QDBusMenuItem>();
    qDBusRegisterMetaType<QDBusMenuItemList>();
    qDBusRegisterMetaType<QDBusMenuItemKeys>();
    qDBusRegisterMetaType<QDBusMenuItemKeysList>();
    qDBusRegisterMetaType<QDBusMenuLayoutItem>();
    qDBusRegisterMetaType<QDBusMenuLayoutItemList>();
    qDBusRegisterMetaType<QDBusMenuEvent>();
    qDBusRegisterMetaType<QDBusMenuEventList>();
}

QDBusMenuItem::QDBusMenuItem(const QDBusPlatformMenuItem *item)
    : m_id(item->dbusID())
{
    if (item->isSeparator()) {
        m_properties.insert(QLatin1String("type"), QLatin1String("separator"));
    } else {
        m_properties.insert(QLatin1String("label"), convertMnemonic(item->text()));
        if (item->menu())
            m_properties.insert(QLatin1String("children-display"), QLatin1String("submenu"));
        m_properties.insert(QLatin1String("enabled"), item->isEnabled());
        if (item->isCheckable()) {
            // dbusmenu supports "radio" too, but QPlatformMenuItem doesn't seem to
            // (QAction would have an exclusive actionGroup)
            m_properties.insert(QLatin1String("toggle-type"), QLatin1String("checkmark"));
            m_properties.insert(QLatin1String("toggle-state"), item->isChecked() ? 1 : 0);
        }
        /* TODO support shortcuts
        const QKeySequence &scut = item->shortcut();
        if (!scut.isEmpty()) {
            QDBusMenuShortcut shortcut(scut);
            properties.insert(QLatin1String("shortcut"), QVariant::fromValue(shortcut));
        }
        */
        const QIcon &icon = item->icon();
        if (!icon.name().isEmpty()) {
            m_properties.insert(QLatin1String("icon-name"), icon.name());
        } else if (!icon.isNull()) {
            QBuffer buf;
            icon.pixmap(16).save(&buf, "PNG");
            m_properties.insert(QLatin1String("icon-data"), buf.data());
        }
    }
    if (!item->isVisible())
        m_properties.insert(QLatin1String("visible"), false);
}

QDBusMenuItemList QDBusMenuItem::items(const QList<int> &ids, const QStringList &propertyNames)
{
    Q_UNUSED(propertyNames)
    QDBusMenuItemList ret;
    QList<const QDBusPlatformMenuItem *> items = QDBusPlatformMenuItem::byIds(ids);
    ret.reserve(items.size());
    Q_FOREACH (const QDBusPlatformMenuItem *item, items)
        ret << QDBusMenuItem(item);
    return ret;
}

QString QDBusMenuItem::convertMnemonic(const QString &label)
{
    // convert only the first occurrence of ampersand which is not at the end
    // dbusmenu uses underscore instead of ampersand
    int idx = label.indexOf(QLatin1Char('&'));
    if (idx < 0 || idx == label.length() - 1)
        return label;
    QString ret(label);
    ret[idx] = QLatin1Char('_');
    return ret;
}

const QDBusArgument &operator<<(QDBusArgument &arg, const QDBusMenuEvent &ev)
{
    arg.beginStructure();
    arg << ev.m_id << ev.m_eventId << ev.m_data << ev.m_timestamp;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, QDBusMenuEvent &ev)
{
    arg.beginStructure();
    arg >> ev.m_id >> ev.m_eventId >> ev.m_data >> ev.m_timestamp;
    arg.endStructure();
    return arg;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QDBusMenuItem &item)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "QDBusMenuItem(id=" << item.m_id << ", properties=" << item.m_properties << ')';
    return d;
}

QDebug operator<<(QDebug d, const QDBusMenuLayoutItem &item)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "QDBusMenuLayoutItem(id=" << item.m_id << ", properties=" << item.m_properties << ", " << item.m_children.count() << " children)";
    return d;
}
#endif

QT_END_NAMESPACE
