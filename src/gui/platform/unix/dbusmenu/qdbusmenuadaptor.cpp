// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*
    This file was originally created by qdbusxml2cpp version 0.8
    Command line was:
    qdbusxml2cpp -a dbusmenu ../../3rdparty/dbus-ifaces/dbus-menu.xml

    However it is maintained manually.
*/

#include <QMetaObject>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QLocale>

#include <private/qdbusmenuadaptor_p.h>
#include <private/qdbusplatformmenu_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QDBusMenuAdaptor::QDBusMenuAdaptor(QDBusPlatformMenu *topLevelMenu)
    : QDBusAbstractAdaptor(topLevelMenu)
    , m_topLevelMenu(topLevelMenu)
{
    setAutoRelaySignals(true);
}

QDBusMenuAdaptor::~QDBusMenuAdaptor()
{
}

QString QDBusMenuAdaptor::status() const
{
    qCDebug(qLcMenu);
    return "normal"_L1;
}

QString QDBusMenuAdaptor::textDirection() const
{
    return QLocale().textDirection() == Qt::RightToLeft ? "rtl"_L1 : "ltr"_L1;
}

uint QDBusMenuAdaptor::version() const
{
    return 4;
}

bool QDBusMenuAdaptor::AboutToShow(int id)
{
    qCDebug(qLcMenu) << id;
    if (id == 0) {
        emit m_topLevelMenu->aboutToShow();
    } else {
        QDBusPlatformMenuItem *item = QDBusPlatformMenuItem::byId(id);
        if (item) {
            const QDBusPlatformMenu *menu = static_cast<const QDBusPlatformMenu *>(item->menu());
            if (menu)
                emit const_cast<QDBusPlatformMenu *>(menu)->aboutToShow();
        }
    }
    return false;  // updateNeeded (we don't know that, so false)
}

QList<int> QDBusMenuAdaptor::AboutToShowGroup(const QList<int> &ids, QList<int> &idErrors)
{
    qCDebug(qLcMenu) << ids;
    Q_UNUSED(idErrors);
    idErrors.clear();
    for (int id : ids)
        AboutToShow(id);
    return QList<int>(); // updatesNeeded
}

void QDBusMenuAdaptor::Event(int id, const QString &eventId, const QDBusVariant &data, uint timestamp)
{
    Q_UNUSED(data);
    Q_UNUSED(timestamp);
    QDBusPlatformMenuItem *item = QDBusPlatformMenuItem::byId(id);
    qCDebug(qLcMenu) << id << (item ? item->text() : ""_L1) << eventId;
    if (item && eventId == "clicked"_L1)
        item->trigger();
    if (item && eventId == "hovered"_L1)
        emit item->hovered();
    if (eventId == "closed"_L1) {
        // There is no explicit AboutToHide method, so map closed event to aboutToHide method
        const QDBusPlatformMenu *menu = nullptr;
        if (item)
            menu = static_cast<const QDBusPlatformMenu *>(item->menu());
        else if (id == 0)
            menu = m_topLevelMenu;
        if (menu)
            emit const_cast<QDBusPlatformMenu *>(menu)->aboutToHide();
    }
}

QList<int> QDBusMenuAdaptor::EventGroup(const QDBusMenuEventList &events)
{
    for (const QDBusMenuEvent &ev : events)
        Event(ev.m_id, ev.m_eventId, ev.m_data, ev.m_timestamp);
    return QList<int>(); // idErrors
}

QDBusMenuItemList QDBusMenuAdaptor::GetGroupProperties(const QList<int> &ids, const QStringList &propertyNames)
{
    qCDebug(qLcMenu) << ids << propertyNames << "=>" << QDBusMenuItem::items(ids, propertyNames);
    return QDBusMenuItem::items(ids, propertyNames);
}

uint QDBusMenuAdaptor::GetLayout(int parentId, int recursionDepth, const QStringList &propertyNames, QDBusMenuLayoutItem &layout)
{
    uint ret = layout.populate(parentId, recursionDepth, propertyNames, m_topLevelMenu);
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

#include "moc_qdbusmenuadaptor_p.cpp"
