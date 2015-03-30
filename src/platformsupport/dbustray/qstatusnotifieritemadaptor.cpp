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
    qdbusxml2cpp -a statusnotifieritem ../../3rdparty/dbus-ifaces/org.kde.StatusNotifierItem.xml

    However it is maintained manually, because this adapter needs to do
    significant interface adaptation, and can do it more efficiently using the
    QDBusTrayIcon API directly rather than via QObject::property() and
    QMetaObject::invokeMethod().
*/

#ifndef QT_NO_SYSTEMTRAYICON

#include "qstatusnotifieritemadaptor_p.h"
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

#endif // QT_NO_SYSTEMTRAYICON
