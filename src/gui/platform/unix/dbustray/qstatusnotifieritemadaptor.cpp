// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*
    This file was originally created by qdbusxml2cpp version 0.8
    Command line was:
    qdbusxml2cpp -a statusnotifieritem ../../3rdparty/dbus-ifaces/org.kde.StatusNotifierItem.xml

    However it is maintained manually, because this adapter needs to do
    significant interface adaptation, and can do it more efficiently using the
    QDBusTrayIcon API directly rather than via QObject::property() and
    QMetaObject::invokeMethod().
*/

#include "qstatusnotifieritemadaptor_p.h"

#ifndef QT_NO_SYSTEMTRAYICON

#include <QtCore/QLoggingCategory>
#include <QtCore/QCoreApplication>

#include "qdbustrayicon_p.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcMenu)
Q_DECLARE_LOGGING_CATEGORY(qLcTray)

QStatusNotifierItemAdaptor::QStatusNotifierItemAdaptor(QDBusTrayIcon *parent)
    : QDBusAbstractAdaptor(parent), m_trayIcon(parent)
{
    setAutoRelaySignals(true);
}

QStatusNotifierItemAdaptor::~QStatusNotifierItemAdaptor()
{
}

QString QStatusNotifierItemAdaptor::attentionIconName() const
{
    return m_trayIcon->attentionIconName();
}

QXdgDBusImageVector QStatusNotifierItemAdaptor::attentionIconPixmap() const
{
    return iconToQXdgDBusImageVector(m_trayIcon->attentionIcon());
}

QString QStatusNotifierItemAdaptor::attentionMovieName() const
{
    return QString();
}

QString QStatusNotifierItemAdaptor::category() const
{
    return m_trayIcon->category();
}

QString QStatusNotifierItemAdaptor::iconName() const
{
    return m_trayIcon->iconName();
}

QXdgDBusImageVector QStatusNotifierItemAdaptor::iconPixmap() const
{
    return iconToQXdgDBusImageVector(m_trayIcon->icon());
}

QString QStatusNotifierItemAdaptor::id() const
{
    // from the API docs: "a name that should be unique for this application and
    // consistent between sessions, such as the application name itself"
    return QCoreApplication::applicationName();
}

bool QStatusNotifierItemAdaptor::itemIsMenu() const
{
    // From KDE docs: if this is true, the item only supports the context menu,
    // so the visualization should prefer sending ContextMenu() instead of Activate().
    // But QSystemTrayIcon doesn't have such a setting: it will emit activated()
    // and the application is free to use it or ignore it; we don't know whether it will.
    return false;
}

QDBusObjectPath QStatusNotifierItemAdaptor::menu() const
{
    return QDBusObjectPath(m_trayIcon->menu() ? "/MenuBar" : "/NO_DBUSMENU");
}

QString QStatusNotifierItemAdaptor::overlayIconName() const
{
    return QString();
}

QXdgDBusImageVector QStatusNotifierItemAdaptor::overlayIconPixmap() const
{
    QXdgDBusImageVector ret; // empty vector
    return ret;
}

QString QStatusNotifierItemAdaptor::status() const
{
    return m_trayIcon->status();
}

QString QStatusNotifierItemAdaptor::title() const
{
    // Shown e.g. when the icon is hidden, in the popup showing all hidden items.
    // Since QSystemTrayIcon doesn't have this property, the application name
    // is the best information we have available.
    return QCoreApplication::applicationName();
}

QXdgDBusToolTipStruct QStatusNotifierItemAdaptor::toolTip() const
{
    QXdgDBusToolTipStruct ret;
    if (m_trayIcon->isRequestingAttention()) {
        ret.title = m_trayIcon->attentionTitle();
        ret.subTitle = m_trayIcon->attentionMessage();
        ret.icon = m_trayIcon->attentionIconName();
    } else {
        ret.title = m_trayIcon->tooltip();
    }
    return ret;
}

void QStatusNotifierItemAdaptor::Activate(int x, int y)
{
    qCDebug(qLcTray) << x << y;
    emit m_trayIcon->activated(QPlatformSystemTrayIcon::Trigger);
}

void QStatusNotifierItemAdaptor::ContextMenu(int x, int y)
{
    qCDebug(qLcTray) << x << y;
    emit m_trayIcon->activated(QPlatformSystemTrayIcon::Context);
}

void QStatusNotifierItemAdaptor::ProvideXdgActivationToken(const QString &token)
{
    qCDebug(qLcTray) << token;
    qputenv("XDG_ACTIVATION_TOKEN", token.toUtf8());
}

void QStatusNotifierItemAdaptor::Scroll(int w, const QString &s)
{
    qCDebug(qLcTray) << w << s;
    // unsupported
}

void QStatusNotifierItemAdaptor::SecondaryActivate(int x, int y)
{
    qCDebug(qLcTray) << x << y;
    emit m_trayIcon->activated(QPlatformSystemTrayIcon::MiddleClick);
}

QT_END_NAMESPACE

#include "moc_qstatusnotifieritemadaptor_p.cpp"

#endif // QT_NO_SYSTEMTRAYICON
