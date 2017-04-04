/****************************************************************************
**
** Copyright (C) 2016 Dmitry Shachnev <mitya57@gmail.com>
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

#include "qdbusmenubar_p.h"
#include "qdbusmenuregistrarproxy_p.h"

QT_BEGIN_NAMESPACE

/* note: do not change these to QStringLiteral;
   we are unloaded before QtDBus is done using the strings.
 */
#define REGISTRAR_SERVICE QLatin1String("com.canonical.AppMenu.Registrar")
#define REGISTRAR_PATH QLatin1String("/com/canonical/AppMenu/Registrar")

QDBusMenuBar::QDBusMenuBar()
    : QPlatformMenuBar()
    , m_menu(new QDBusPlatformMenu())
    , m_menuAdaptor(new QDBusMenuAdaptor(m_menu))
    , m_windowId(0)
{
    QDBusMenuItem::registerDBusTypes();
    connect(m_menu, &QDBusPlatformMenu::propertiesUpdated,
            m_menuAdaptor, &QDBusMenuAdaptor::ItemsPropertiesUpdated);
    connect(m_menu, &QDBusPlatformMenu::updated,
            m_menuAdaptor, &QDBusMenuAdaptor::LayoutUpdated);
    connect(m_menu, &QDBusPlatformMenu::popupRequested,
            m_menuAdaptor, &QDBusMenuAdaptor::ItemActivationRequested);
}

QDBusMenuBar::~QDBusMenuBar()
{
    unregisterMenuBar();
    delete m_menuAdaptor;
    delete m_menu;
    qDeleteAll(m_menuItems);
}

QDBusPlatformMenuItem *QDBusMenuBar::menuItemForMenu(QPlatformMenu *menu)
{
    if (!menu)
        return nullptr;
    quintptr tag = menu->tag();
    const auto it = m_menuItems.constFind(tag);
    if (it != m_menuItems.cend()) {
        return *it;
    } else {
        QDBusPlatformMenuItem *item = new QDBusPlatformMenuItem;
        updateMenuItem(item, menu);
        m_menuItems.insert(tag, item);
        return item;
    }
}

void QDBusMenuBar::updateMenuItem(QDBusPlatformMenuItem *item, QPlatformMenu *menu)
{
    const QDBusPlatformMenu *ourMenu = qobject_cast<const QDBusPlatformMenu *>(menu);
    item->setText(ourMenu->text());
    item->setIcon(ourMenu->icon());
    item->setEnabled(ourMenu->isEnabled());
    item->setVisible(ourMenu->isVisible());
    item->setMenu(menu);
}

void QDBusMenuBar::insertMenu(QPlatformMenu *menu, QPlatformMenu *before)
{
    QDBusPlatformMenuItem *menuItem = menuItemForMenu(menu);
    QDBusPlatformMenuItem *beforeItem = menuItemForMenu(before);
    m_menu->insertMenuItem(menuItem, beforeItem);
    m_menu->emitUpdated();
}

void QDBusMenuBar::removeMenu(QPlatformMenu *menu)
{
    QDBusPlatformMenuItem *menuItem = menuItemForMenu(menu);
    m_menu->removeMenuItem(menuItem);
    m_menu->emitUpdated();
}

void QDBusMenuBar::syncMenu(QPlatformMenu *menu)
{
    QDBusPlatformMenuItem *menuItem = menuItemForMenu(menu);
    updateMenuItem(menuItem, menu);
}

void QDBusMenuBar::handleReparent(QWindow *newParentWindow)
{
    if (newParentWindow) {
        unregisterMenuBar();
        m_windowId = newParentWindow->winId();
        registerMenuBar();
    }
}

QPlatformMenu *QDBusMenuBar::menuForTag(quintptr tag) const
{
    QDBusPlatformMenuItem *menuItem = m_menuItems.value(tag);
    if (menuItem)
        return const_cast<QPlatformMenu *>(menuItem->menu());
    return nullptr;
}

QPlatformMenu *QDBusMenuBar::createMenu() const
{
    return new QDBusPlatformMenu;
}

void QDBusMenuBar::registerMenuBar()
{
    static uint menuBarId = 0;

    QDBusConnection connection = QDBusConnection::sessionBus();
    m_objectPath = QStringLiteral("/MenuBar/%1").arg(++menuBarId);
    if (!connection.registerObject(m_objectPath, m_menu))
        return;

    QDBusMenuRegistrarInterface registrar(REGISTRAR_SERVICE, REGISTRAR_PATH, connection, this);
    QDBusPendingReply<> r = registrar.RegisterWindow(m_windowId, QDBusObjectPath(m_objectPath));
    r.waitForFinished();
    if (r.isError()) {
        qWarning("Failed to register window menu, reason: %s (\"%s\")",
                 qUtf8Printable(r.error().name()), qUtf8Printable(r.error().message()));
        connection.unregisterObject(m_objectPath);
    }
}

void QDBusMenuBar::unregisterMenuBar()
{
    QDBusConnection connection = QDBusConnection::sessionBus();

    if (m_windowId) {
        QDBusMenuRegistrarInterface registrar(REGISTRAR_SERVICE, REGISTRAR_PATH, connection, this);
        QDBusPendingReply<> r = registrar.UnregisterWindow(m_windowId);
        r.waitForFinished();
        if (r.isError())
            qWarning("Failed to unregister window menu, reason: %s (\"%s\")",
                     qUtf8Printable(r.error().name()), qUtf8Printable(r.error().message()));
    }

    if (!m_objectPath.isEmpty())
        connection.unregisterObject(m_objectPath);
}

QT_END_NAMESPACE
