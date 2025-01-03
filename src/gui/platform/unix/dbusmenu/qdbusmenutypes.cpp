// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#if QT_CONFIG(shortcut)
#  include <private/qkeysequence_p.h>
#endif
#include <qpa/qplatformmenu.h>
#include "qdbusplatformmenu_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QT_IMPL_METATYPE_EXTERN(QDBusMenuItem)
QT_IMPL_METATYPE_EXTERN(QDBusMenuItemList)
QT_IMPL_METATYPE_EXTERN(QDBusMenuItemKeys)
QT_IMPL_METATYPE_EXTERN(QDBusMenuItemKeysList)
QT_IMPL_METATYPE_EXTERN(QDBusMenuLayoutItem)
QT_IMPL_METATYPE_EXTERN(QDBusMenuLayoutItemList)
QT_IMPL_METATYPE_EXTERN(QDBusMenuEvent)
QT_IMPL_METATYPE_EXTERN(QDBusMenuEventList)
QT_IMPL_METATYPE_EXTERN(QDBusMenuShortcut)

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
        m_properties.insert("children-display"_L1, "submenu"_L1);
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
    const auto items = menu->items();
    for (QDBusPlatformMenuItem *item : items) {
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
    for (const QDBusMenuLayoutItem &child : item.m_children)
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
        QDBusArgument childArgument = qvariant_cast<QDBusArgument>(dbusVariant.variant());

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
        m_properties.insert("type"_L1, "separator"_L1);
    } else {
        m_properties.insert("label"_L1, convertMnemonic(item->text()));
        if (item->menu())
            m_properties.insert("children-display"_L1, "submenu"_L1);
        m_properties.insert("enabled"_L1, item->isEnabled());
        if (item->isCheckable()) {
            QString toggleType = item->hasExclusiveGroup() ? "radio"_L1 : "checkmark"_L1;
            m_properties.insert("toggle-type"_L1, toggleType);
            m_properties.insert("toggle-state"_L1, item->isChecked() ? 1 : 0);
        }
#ifndef QT_NO_SHORTCUT
        const QKeySequence &scut = item->shortcut();
        if (!scut.isEmpty()) {
            QDBusMenuShortcut shortcut = convertKeySequence(scut);
            m_properties.insert("shortcut"_L1, QVariant::fromValue(shortcut));
        }
#endif
        const QIcon &icon = item->icon();
        if (!icon.name().isEmpty()) {
            m_properties.insert("icon-name"_L1, icon.name());
        } else if (!icon.isNull()) {
            QBuffer buf;
            icon.pixmap(16).save(&buf, "PNG");
            m_properties.insert("icon-data"_L1, buf.data());
        }
    }
    m_properties.insert("visible"_L1, item->isVisible());
}

QDBusMenuItemList QDBusMenuItem::items(const QList<int> &ids, const QStringList &propertyNames)
{
    Q_UNUSED(propertyNames);
    QDBusMenuItemList ret;
    const QList<const QDBusPlatformMenuItem *> items = QDBusPlatformMenuItem::byIds(ids);
    ret.reserve(items.size());
    for (const QDBusPlatformMenuItem *item : items)
        ret << QDBusMenuItem(item);
    return ret;
}

QString QDBusMenuItem::convertMnemonic(const QString &label)
{
    // convert only the first occurrence of ampersand which is not at the end
    // dbusmenu uses underscore instead of ampersand
    int idx = label.indexOf(u'&');
    if (idx < 0 || idx == label.size() - 1)
        return label;
    QString ret(label);
    ret[idx] = u'_';
    return ret;
}

#ifndef QT_NO_SHORTCUT
QDBusMenuShortcut QDBusMenuItem::convertKeySequence(const QKeySequence &sequence)
{
    QDBusMenuShortcut shortcut;
    for (int i = 0; i < sequence.count(); ++i) {
        QStringList tokens;
        int key = sequence[i].toCombined();
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
        if (keyName == "+"_L1)
            tokens << QStringLiteral("plus");
        else if (keyName == "-"_L1)
            tokens << QStringLiteral("minus");
        else
            tokens << keyName;
        shortcut << tokens;
    }
    return shortcut;
}
#endif

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
    d << "QDBusMenuLayoutItem(id=" << item.m_id << ", properties=" << item.m_properties << ", " << item.m_children.size() << " children)";
    return d;
}
#endif

QT_END_NAMESPACE
