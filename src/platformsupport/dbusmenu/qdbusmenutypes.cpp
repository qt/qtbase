/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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
#include <private/qkeysequence_p.h>
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

uint QDBusMenuLayoutItem::populate(int id, int depth, const QStringList &propertyNames, const QDBusPlatformMenu *topLevelMenu)
{
    qCDebug(qLcMenu) << id << "depth" << depth << propertyNames;
    m_id = id;
    if (id == 0) {
        m_properties.insert(QLatin1String("children-display"), QLatin1String("submenu"));
        if (topLevelMenu)
            populate(topLevelMenu, depth, propertyNames);
        return 1; // revision
    }

    QDBusPlatformMenuItem *item = QDBusPlatformMenuItem::byId(id);
    if (item) {
        const QDBusPlatformMenu *menu = static_cast<const QDBusPlatformMenu *>(item->menu());

        if (menu) {
            if (depth != 0)
                populate(menu, depth, propertyNames);
            return menu->revision();
        }
    }

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
    m_id = item->dbusID();
    QDBusMenuItem proxy(item);
    m_properties = proxy.m_properties;

    const QDBusPlatformMenu *menu = static_cast<const QDBusPlatformMenu *>(item->menu());
    if (depth != 0 && menu)
        populate(menu, depth, propertyNames);
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
    qDBusRegisterMetaType<QDBusMenuShortcut>();
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
            QString toggleType = item->hasExclusiveGroup() ? QLatin1String("radio") : QLatin1String("checkmark");
            m_properties.insert(QLatin1String("toggle-type"), toggleType);
            m_properties.insert(QLatin1String("toggle-state"), item->isChecked() ? 1 : 0);
        }
        const QKeySequence &scut = item->shortcut();
        if (!scut.isEmpty()) {
            QDBusMenuShortcut shortcut = convertKeySequence(scut);
            m_properties.insert(QLatin1String("shortcut"), QVariant::fromValue(shortcut));
        }
        const QIcon &icon = item->icon();
        if (!icon.name().isEmpty()) {
            m_properties.insert(QLatin1String("icon-name"), icon.name());
        } else if (!icon.isNull()) {
            QBuffer buf;
            icon.pixmap(16).save(&buf, "PNG");
            m_properties.insert(QLatin1String("icon-data"), buf.data());
        }
    }
    m_properties.insert(QLatin1String("visible"), item->isVisible());
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

QDBusMenuShortcut QDBusMenuItem::convertKeySequence(const QKeySequence &sequence)
{
    QDBusMenuShortcut shortcut;
    for (int i = 0; i < sequence.count(); ++i) {
        QStringList tokens;
        int key = sequence[i];
        if (key & Qt::MetaModifier)
            tokens << QStringLiteral("Super");
        if (key & Qt::ControlModifier)
            tokens << QStringLiteral("Control");
        if (key & Qt::AltModifier)
            tokens << QStringLiteral("Alt");
        if (key & Qt::ShiftModifier)
            tokens << QStringLiteral("Shift");
        if (key & Qt::KeypadModifier)
            tokens << QStringLiteral("Num");

        QString keyName = QKeySequencePrivate::keyName(key, QKeySequence::PortableText);
        if (keyName == QLatin1String("+"))
            tokens << QStringLiteral("plus");
        else if (keyName == QLatin1String("-"))
            tokens << QStringLiteral("minus");
        else
            tokens << keyName;
        shortcut << tokens;
    }
    return shortcut;
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
