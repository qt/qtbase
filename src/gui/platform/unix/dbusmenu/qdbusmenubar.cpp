// Copyright (C) 2016 Dmitry Shachnev <mitya57@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdbusmenubar_p.h"
#include "qdbusmenuregistrarproxy_p.h"

#include <private/qguiapplication_p.h>
#include <private/qdesktopunixservices_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/* note: do not change these to QStringLiteral;
   we are unloaded before QtDBus is done using the strings.
 */
#define REGISTRAR_SERVICE "com.canonical.AppMenu.Registrar"_L1
#define REGISTRAR_PATH "/com/canonical/AppMenu/Registrar"_L1

QDBusMenuBar::QDBusMenuBar()
    : QPlatformMenuBar()
    , m_menu(new QDBusPlatformMenu())
    , m_menuAdaptor(new QDBusMenuAdaptor(m_menu))
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
        m_window = newParentWindow;
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

    if (QGuiApplication::platformName() == "xcb"_L1) {
        QDBusMenuRegistrarInterface registrar(REGISTRAR_SERVICE, REGISTRAR_PATH, connection, this);
        QDBusPendingReply<> r = registrar.RegisterWindow(m_window->winId(), QDBusObjectPath(m_objectPath));
        r.waitForFinished();
        if (r.isError()) {
            qWarning("Failed to register window menu, reason: %s (\"%s\")",
                     qUtf8Printable(r.error().name()), qUtf8Printable(r.error().message()));
            connection.unregisterObject(m_objectPath);
            return;
        }
    }

    const auto unixServices = dynamic_cast<QDesktopUnixServices *>(
            QGuiApplicationPrivate::platformIntegration()->services());
    unixServices->registerDBusMenuForWindow(m_window, connection.baseService(), m_objectPath);
}

void QDBusMenuBar::unregisterMenuBar()
{
    QDBusConnection connection = QDBusConnection::sessionBus();

    if (m_window) {
        if (QGuiApplication::platformName() == "xcb"_L1) {
            QDBusMenuRegistrarInterface registrar(REGISTRAR_SERVICE, REGISTRAR_PATH, connection, this);
            QDBusPendingReply<> r = registrar.UnregisterWindow(m_window->winId());
            r.waitForFinished();
            if (r.isError())
                qWarning("Failed to unregister window menu, reason: %s (\"%s\")",
                         qUtf8Printable(r.error().name()), qUtf8Printable(r.error().message()));
        }

        const auto unixServices = dynamic_cast<QDesktopUnixServices *>(
            QGuiApplicationPrivate::platformIntegration()->services());
        unixServices->unregisterDBusMenuForWindow(m_window);
    }

    if (!m_objectPath.isEmpty()) {
        connection.unregisterObject(m_objectPath);
    }
}

QT_END_NAMESPACE

#include "moc_qdbusmenubar_p.cpp"
