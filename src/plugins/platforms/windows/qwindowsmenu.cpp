/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwindowsmenu.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"

#include <QtGui/qwindow.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qpointer.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsMenuBar
    \brief Windows native menu bar

    \list
    \li \l{https://msdn.microsoft.com/de-de/library/windows/desktop/ms647553(v=vs.85).aspx#_win32_Menu_Creation_Functions},
         \e{About Menus}
    \endlist

    \note The destruction order of the QWindowsMenu/Item/Bar instances is
    arbitrary depending on whether the application is Qt Quick or
    Qt Widgets, either the containers or the items might be deleted first.

    \internal
*/

static uint nextId = 1;

// Find a QPlatformMenu[Item]* in a vector of QWindowsMenu[Item], where
// QVector::indexOf() cannot be used since it wants a QWindowsMenu[Item]*
template <class Derived, class Needle>
static int indexOf(const QVector<Derived *> &v, const Needle *needle)
{
    for (int i = 0, size = v.size(); i < size; ++i) {
        if (v.at(i) == needle)
            return i;
    }
    return -1;
}

// Helper for inserting a QPlatformMenu[Item]* into a vector of QWindowsMenu[Item].
template <class Derived, class Base>
static int insertBefore(QVector<Derived *> *v, Base *newItemIn, const Base *before = nullptr)
{
    int index = before ? indexOf(*v, before) : -1;
    if (index != -1) {
        v->insert(index, static_cast<Derived *>(newItemIn));
    } else {
        index = v->size();
        v->append(static_cast<Derived *>(newItemIn));
    }
    return index;
}

static inline const wchar_t *qStringToWChar(const QString &s)
{
    return reinterpret_cast<const wchar_t *>(s.utf16());
}

// Traverse menu and return the item for which predicate
// "bool Function(QWindowsMenuItem *)" returns true
template <class Predicate>
static QWindowsMenuItem *traverseMenuItems(const QWindowsMenu *menu, Predicate p)
{
    const QWindowsMenu::MenuItems &items = menu->menuItems();
    for (QWindowsMenuItem *item : items) {
        if (p(item))
            return item;
        if (item->subMenu()) {
            if (QWindowsMenuItem *subMenuItem = traverseMenuItems(item->subMenu(), p))
                return subMenuItem;
        }
    }
    return nullptr;
}

// Traverse menu bar return the item for which predicate
// "bool Function(QWindowsMenuItem *)" returns true
template <class Predicate>
static QWindowsMenuItem *traverseMenuItems(const QWindowsMenuBar *menuBar, Predicate p)
{
    const QWindowsMenuBar::Menus &menus = menuBar->menus();
    for (QWindowsMenu *menu : menus) {
        if (QWindowsMenuItem *item = traverseMenuItems(menu, p))
            return item;
    }
    return nullptr;
}

template <class Menu /* Menu[Bar] */>
static QWindowsMenuItem *findMenuItemById(const Menu *menu, uint id)
{
    return traverseMenuItems(menu, [id] (const QWindowsMenuItem *i) { return i->id() == id; });
}

// Traverse menu and return the menu for which predicate
// "bool Function(QWindowsMenu *)" returns true
template <class Predicate>
static QWindowsMenu *traverseMenus(const QWindowsMenu *menu, Predicate p)
{
    const QWindowsMenu::MenuItems &items = menu->menuItems();
    for (QWindowsMenuItem *item : items) {
        if (QWindowsMenu *subMenu = item->subMenu()) {
            if (p(subMenu))
                return subMenu;
            if (QWindowsMenu *menu = traverseMenus(subMenu, p))
                return menu;
        }
    }
    return nullptr;
}

// Traverse menu bar return the item for which
// function "bool Function(QWindowsMenu *)" returns true
template <class Predicate>
static QWindowsMenu *traverseMenus(const QWindowsMenuBar *menuBar, Predicate p)
{
    const QWindowsMenuBar::Menus &menus = menuBar->menus();
    for (QWindowsMenu *menu : menus) {
            if (p(menu))
                return menu;
        if (QWindowsMenu *subMenu = traverseMenus(menu, p))
            return subMenu;
    }
    return nullptr;
}

template <class Menu /* Menu[Bar] */>
static QWindowsMenu *findMenuByHandle(const Menu *menu, HMENU hMenu)
{
    return traverseMenus(menu, [hMenu] (const QWindowsMenu *i) { return i->menuHandle() == hMenu; });
}

template <class MenuType>
static int findNextVisibleEntry(const QVector<MenuType *> &entries, int pos)
{
    for (int i = pos, size = entries.size(); i < size; ++i) {
        if (entries.at(i)->isVisible())
            return i;
    }
    return -1;
}

static inline void menuItemInfoInit(MENUITEMINFO &menuItemInfo)
{
    memset(&menuItemInfo, 0, sizeof(MENUITEMINFO));
    menuItemInfo.cbSize = sizeof(MENUITEMINFO);
}

static inline void menuItemInfoSetText(MENUITEMINFO &menuItemInfo, const QString &text)
{
    menuItemInfoInit(menuItemInfo);
    menuItemInfo.fMask = MIIM_STRING;
    menuItemInfo.dwTypeData = const_cast<wchar_t *>(qStringToWChar(text));
    menuItemInfo.cch = UINT(text.size());
}

static UINT menuItemState(HMENU hMenu, UINT uItem, BOOL fByPosition)
{
    MENUITEMINFO menuItemInfo;
    menuItemInfoInit(menuItemInfo);
    menuItemInfo.fMask = MIIM_STATE;
    return GetMenuItemInfo(hMenu, uItem, fByPosition, &menuItemInfo) == TRUE ? menuItemInfo.fState : 0;
}

static void menuItemSetState(HMENU hMenu, UINT uItem, BOOL fByPosition, UINT flags)
{
    MENUITEMINFO menuItemInfo;
    menuItemInfoInit(menuItemInfo);
    menuItemInfo.fMask = MIIM_STATE;
    menuItemInfo.fState = flags;
    SetMenuItemInfo(hMenu, uItem, fByPosition, &menuItemInfo);
}

static void menuItemSetChangeState(HMENU hMenu, UINT uItem, BOOL fByPosition,
                                   bool value, UINT trueState, UINT falseState)
{
     const UINT oldState = menuItemState(hMenu, uItem, fByPosition);
     UINT newState = oldState;
     if (value) {
         newState |= trueState;
         newState &= ~falseState;
     } else {
         newState &= ~trueState;
         newState |= falseState;
     }
     if (oldState != newState)
         menuItemSetState(hMenu, uItem, fByPosition, newState);
}

// ------------ QWindowsMenuItem
QWindowsMenuItem::QWindowsMenuItem(QWindowsMenu *parentMenu)
    : m_parentMenu(parentMenu)
    , m_id(0)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << static_cast<const void *>(this)
        << "parentMenu=" << parentMenu;
}

QWindowsMenuItem::~QWindowsMenuItem()
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << static_cast<const void *>(this);
    removeFromMenu();
    freeBitmap();
}

void QWindowsMenuItem::freeBitmap()
{
    if (m_hbitmap) {
        DeleteObject(m_hbitmap);
        m_hbitmap = nullptr;
    }
}

void QWindowsMenuItem::setIcon(const QIcon &icon)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << icon << ')' << this;
    if (m_icon.cacheKey() == icon.cacheKey())
        return;
    m_icon = icon;
    if (m_parentMenu != nullptr)
        updateBitmap();
}

Q_GUI_EXPORT HBITMAP qt_pixmapToWinHBITMAP(const QPixmap &p, int hbitmapFormat = 0);

void QWindowsMenuItem::updateBitmap()
{
    freeBitmap();
    if (!m_icon.isNull()) {
        const int size = m_iconSize ? m_iconSize : GetSystemMetrics(SM_CYMENUCHECK);
        m_hbitmap = qt_pixmapToWinHBITMAP(m_icon.pixmap(QSize(size, size)), 1);
    }
    MENUITEMINFO itemInfo;
    menuItemInfoInit(itemInfo);
    itemInfo.fMask = MIIM_BITMAP;
    itemInfo.hbmpItem = m_hbitmap;
    SetMenuItemInfo(parentMenuHandle(), m_id, FALSE, &itemInfo);
}

void QWindowsMenuItem::setText(const QString &text)
{
    qCDebug(lcQpaMenus).nospace().noquote()
        << __FUNCTION__ << "(\"" << text << "\") " << this;
    if (m_text == text)
        return;
    m_text = text;
    if (m_parentMenu != nullptr)
        updateText();
}

void QWindowsMenuItem::updateText()
{
    MENUITEMINFO menuItemInfo;
    const QString &text = nativeText();
    menuItemInfoSetText(menuItemInfo, text);
    SetMenuItemInfo(parentMenuHandle(), m_id, FALSE, &menuItemInfo);
}

void QWindowsMenuItem::setMenu(QPlatformMenu *menuIn)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << menuIn << ')' << this;
    if (menuIn == m_subMenu)
        return;
    const uint oldId = m_id;
    if (menuIn != nullptr) { // Set submenu
        m_subMenu = static_cast<QWindowsMenu *>(menuIn);
        m_subMenu->setAsItemSubMenu(this);
        m_id = m_subMenu->id();
        if (m_parentMenu != nullptr) {
            ModifyMenu(m_parentMenu->menuHandle(), oldId, MF_BYCOMMAND | MF_POPUP,
                       m_id, qStringToWChar(m_text));
        }
        return;
    }
    // Clear submenu
    m_subMenu = nullptr;
    if (m_parentMenu != nullptr) {
        m_id = nextId++;
        ModifyMenu(m_parentMenu->menuHandle(), oldId, MF_BYCOMMAND,
                   m_id, qStringToWChar(m_text));
    } else {
        m_id = 0;
    }
}

void QWindowsMenuItem::setVisible(bool isVisible)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << isVisible << ')' << this;
    if (m_visible == isVisible)
        return;
    m_visible = isVisible;
    if (m_parentMenu == nullptr)
        return;
    // Windows menu items do not implement settable visibility, we need to work
    // around by removing the item from the menu. It will be kept in the list.
    if (isVisible)
        insertIntoMenuHelper(m_parentMenu, false, m_parentMenu->menuItems().indexOf(this));
    else
        RemoveMenu(parentMenuHandle(), m_id, MF_BYCOMMAND);
}

void QWindowsMenuItem::setIsSeparator(bool isSeparator)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << isSeparator << ')' << this;
    if (m_separator == isSeparator)
        return;
    m_separator = isSeparator;
    if (m_parentMenu == nullptr)
        return;
    MENUITEMINFO menuItemInfo;
    menuItemInfoInit(menuItemInfo);
    menuItemInfo.fMask = MIIM_FTYPE;
    menuItemInfo.fType = isSeparator ? MFT_SEPARATOR : MFT_STRING;
    SetMenuItemInfo(parentMenuHandle(), m_id, FALSE, &menuItemInfo);
}

void QWindowsMenuItem::setCheckable(bool checkable)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << checkable << ')' << this;
    if (m_checkable == checkable)
        return;
    m_checkable = checkable;
    if (m_parentMenu == nullptr)
        return;
    UINT state = menuItemState(parentMenuHandle(), m_id, FALSE);
    if (m_checkable)
        state |= m_checked ? MF_CHECKED : MF_UNCHECKED;
    else
        state &= ~(MF_CHECKED | MF_UNCHECKED);
    menuItemSetState(parentMenuHandle(), m_id, FALSE, state);
}

void QWindowsMenuItem::setChecked(bool isChecked)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << isChecked << ')' << this;
    if (m_checked == isChecked)
        return;
    m_checked = isChecked;
    // Convenience: Allow to set checkable by calling setChecked(true) for
    // Quick Controls 1
    if (isChecked)
        m_checkable = true;
    if (m_parentMenu == nullptr || !m_checkable)
        return;
    menuItemSetChangeState(parentMenuHandle(), m_id, FALSE, m_checked, MF_CHECKED, MF_UNCHECKED);
}

#if QT_CONFIG(shortcut)
void QWindowsMenuItem::setShortcut(const QKeySequence &shortcut)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << shortcut << ')' << this;
    if (m_shortcut == shortcut)
        return;
    m_shortcut = shortcut;
    if (m_parentMenu != nullptr)
        updateText();
}
#endif

void QWindowsMenuItem::setEnabled(bool enabled)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << enabled << ')' << this;
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    if (m_parentMenu != nullptr)
        menuItemSetChangeState(parentMenuHandle(), m_id, FALSE, m_enabled, MF_ENABLED, MF_GRAYED);
}

void QWindowsMenuItem::setIconSize(int size)
{
    if (m_iconSize == size)
        return;
    m_iconSize = size;
    if (m_parentMenu != nullptr)
        updateBitmap();
}

HMENU QWindowsMenuItem::parentMenuHandle() const
{
    return m_parentMenu ? m_parentMenu->menuHandle() : nullptr;
}

UINT QWindowsMenuItem::state() const
{
    if (m_separator)
        return MF_SEPARATOR;
    UINT result = MF_STRING | (m_enabled ? MF_ENABLED : MF_GRAYED);
    if (m_subMenu != nullptr)
        result |= MF_POPUP;
    if (m_checkable)
        result |= m_checked ? MF_CHECKED : MF_UNCHECKED;
    if (QGuiApplication::layoutDirection() == Qt::RightToLeft)
        result |= MFT_RIGHTORDER;
    return result;
}

QString QWindowsMenuItem::nativeText() const
{
    QString result = m_text;
#if QT_CONFIG(shortcut)
    if (!m_shortcut.isEmpty()) {
        result += u'\t';
        result += m_shortcut.toString(QKeySequence::NativeText);
    }
#endif
    return result;
}

void QWindowsMenuItem::insertIntoMenu(QWindowsMenu *menu, bool append, int index)
{
    if (m_id == 0 && m_subMenu == nullptr)
        m_id = nextId++;
    insertIntoMenuHelper(menu, append, index);
    m_parentMenu = menu;
}

void QWindowsMenuItem::insertIntoMenuHelper(QWindowsMenu *menu, bool append, int index)
{
    const QString &text = nativeText();

    UINT_PTR idBefore = 0;
    if (!append) {
        // Skip over self (either newly inserted or when called from setVisible()
        const int nextIndex = findNextVisibleEntry(menu->menuItems(), index + 1);
        if (nextIndex != -1)
            idBefore = menu->menuItems().at(nextIndex)->id();
    }

    if (idBefore)
        InsertMenu(menu->menuHandle(), idBefore, state(), m_id, qStringToWChar(text));
    else
        AppendMenu(menu->menuHandle(), state(), m_id, qStringToWChar(text));

    updateBitmap();
}

bool QWindowsMenuItem::removeFromMenu()
{
    if (QWindowsMenu *parentMenu = m_parentMenu) {
        m_parentMenu = nullptr;
        RemoveMenu(parentMenu->menuHandle(), m_id, MF_BYCOMMAND);
        parentMenu->notifyRemoved(this);
        return true;
    }
    return false;
}

// ------------ QWindowsMenu

QWindowsMenu::QWindowsMenu() : QWindowsMenu(nullptr, CreateMenu())
{
}

QWindowsMenu::QWindowsMenu(QWindowsMenu *parentMenu, HMENU menu)
    : m_parentMenu(parentMenu)
    , m_hMenu(menu)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << static_cast<const void *>(this)
        << "parentMenu=" << parentMenu << "HMENU=" << m_hMenu;
}

QWindowsMenu::~QWindowsMenu()
{
    qCDebug(lcQpaMenus).noquote().nospace() << __FUNCTION__
      << " \"" <<m_text << "\", " << static_cast<const void *>(this);
    for (int i = m_menuItems.size() - 1; i>= 0; --i)
        m_menuItems.at(i)->removeFromMenu();
    removeFromParent();
    DestroyMenu(m_hMenu);
}

void QWindowsMenu::insertMenuItem(QPlatformMenuItem *menuItemIn, QPlatformMenuItem *before)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << menuItemIn << ", before=" << before << ')' << this;
    auto *menuItem = static_cast<QWindowsMenuItem *>(menuItemIn);
    const int index = insertBefore(&m_menuItems, menuItemIn, before);
    const bool append = index == m_menuItems.size() - 1;
    menuItem->insertIntoMenu(this, append, index);
}

void QWindowsMenu::removeMenuItem(QPlatformMenuItem *menuItemIn)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << menuItemIn << ')' << this;
    static_cast<QWindowsMenuItem *>(menuItemIn)->removeFromMenu();
}

void QWindowsMenu::setText(const QString &text)
{
    qCDebug(lcQpaMenus).nospace().noquote()
        << __FUNCTION__ << "(\"" << text << "\") " << this;
    if (m_text == text)
        return;
    m_text = text;
    if (!m_visible)
        return;
    const HMENU ph = parentHandle();
    if (ph == nullptr)
        return;
    MENUITEMINFO menuItemInfo;
    menuItemInfoSetText(menuItemInfo, m_text);
    SetMenuItemInfo(ph, id(), FALSE, &menuItemInfo);
}

void QWindowsMenu::setIcon(const QIcon &icon)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << icon << ')' << this;
    m_icon = icon;
}

void QWindowsMenu::setEnabled(bool enabled)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << enabled << ')' << this;
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    if (!m_visible)
        return;
    if (const HMENU ph = parentHandle())
        menuItemSetChangeState(ph, id(), FALSE, m_enabled, MF_ENABLED, MF_GRAYED);
}

QWindowsMenuItem *QWindowsMenu::itemForSubMenu(const QWindowsMenu *subMenu) const
{
    const auto it = std::find_if(m_menuItems.cbegin(), m_menuItems.cend(),
                                 [subMenu] (const QWindowsMenuItem *i) { return i->subMenu() == subMenu; });
    return it != m_menuItems.cend() ? *it : nullptr;
}

void QWindowsMenu::insertIntoMenuBar(QWindowsMenuBar *bar, bool append, int index)
{
    UINT_PTR idBefore = 0;
    if (!append) {
        // Skip over self (either newly inserted or when called from setVisible()
        const int nextIndex = findNextVisibleEntry(bar->menus(), index + 1);
        if (nextIndex != -1)
            idBefore = bar->menus().at(nextIndex)->id();
    }
    m_parentMenuBar = bar;
    m_parentMenu = nullptr;
    if (idBefore)
        InsertMenu(bar->menuBarHandle(), idBefore, MF_POPUP | MF_BYCOMMAND, id(), qStringToWChar(m_text));
    else
        AppendMenu(bar->menuBarHandle(), MF_POPUP, id(), qStringToWChar(m_text));
}

bool QWindowsMenu::removeFromParent()
{
    if (QWindowsMenuBar *bar = m_parentMenuBar) {
        m_parentMenuBar = nullptr;
        bar->notifyRemoved(this);
        return RemoveMenu(bar->menuBarHandle(), id(), MF_BYCOMMAND) == TRUE;
    }
    if (QWindowsMenu *menu = m_parentMenu) {
         m_parentMenu = nullptr;
         QWindowsMenuItem *item = menu->itemForSubMenu(this);
         if (item)
             item->setMenu(nullptr);
         return item != nullptr;
    }
    return false;
}

void QWindowsMenu::setVisible(bool visible)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << visible << ')' << this;
    if (m_visible == visible)
        return;
    m_visible = visible;
    const HMENU ph = parentHandle();
    if (ph == nullptr)
        return;
    // Windows menus do not implement settable visibility, we need to work
    // around by removing the menu from the parent. It will be kept in the list.
    if (visible) {
        if (m_parentMenuBar)
            insertIntoMenuBar(m_parentMenuBar, false, m_parentMenuBar->menus().indexOf(this));
    } else {
        RemoveMenu(ph, id(), MF_BYCOMMAND);
    }
    if (m_parentMenuBar)
        m_parentMenuBar->redraw();
}

QPlatformMenuItem *QWindowsMenu::menuItemAt(int position) const
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << position;
    return position >= 0 && position < m_menuItems.size()
        ? m_menuItems.at(position) : nullptr;
}

QPlatformMenuItem *QWindowsMenu::menuItemForTag(quintptr tag) const
{
    return traverseMenuItems(this, [tag] (const QPlatformMenuItem *i) { return i->tag() == tag; });
}

QPlatformMenuItem *QWindowsMenu::createMenuItem() const
{
    QPlatformMenuItem *result = new QWindowsMenuItem;
    qCDebug(lcQpaMenus) << __FUNCTION__ << this << "returns" << result;
    return result;
}

QPlatformMenu *QWindowsMenu::createSubMenu() const
{
    QPlatformMenu *result = new QWindowsMenu;
    qCDebug(lcQpaMenus) << __FUNCTION__ << this << "returns" << result;
    return result;
}

void QWindowsMenu::setAsItemSubMenu(QWindowsMenuItem *item)
{
    m_parentMenu = item->parentMenu();
}

HMENU QWindowsMenu::parentMenuHandle() const
{
    return m_parentMenu ? m_parentMenu->menuHandle() : nullptr;
}

HMENU QWindowsMenu::parentMenuBarHandle() const
{
    return m_parentMenuBar ? m_parentMenuBar->menuBarHandle() : nullptr;
}

HMENU QWindowsMenu::parentHandle() const
{
    if (m_parentMenuBar)
        return m_parentMenuBar->menuBarHandle();
    if (m_parentMenu)
      return m_parentMenu->menuHandle();
    return nullptr;
}

// --------------- QWindowsPopupMenu

static QPointer<QWindowsPopupMenu> lastShownPopupMenu;

QWindowsPopupMenu::QWindowsPopupMenu() : QWindowsMenu(nullptr, CreatePopupMenu())
{
}

void QWindowsPopupMenu::showPopup(const QWindow *parentWindow, const QRect &targetRect,
                                  const QPlatformMenuItem *item)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '>' << this << parentWindow << targetRect << item;
    const auto *window = static_cast<const QWindowsBaseWindow *>(parentWindow->handle());
    const QPoint globalPos = window->mapToGlobal(targetRect.topLeft());
    trackPopupMenu(window->handle(), globalPos.x(), globalPos.y());
}

bool QWindowsPopupMenu::trackPopupMenu(HWND windowHandle, int x, int y)
{
    lastShownPopupMenu = this;
    // Emulate Show()/Hide() signals. Could be implemented by catching the
    // WM_EXITMENULOOP, WM_ENTERMENULOOP messages; but they do not carry
    // information telling which menu was opened.
    emit aboutToShow();
    const bool result =
        TrackPopupMenu(menuHandle(),
                          QGuiApplication::layoutDirection() == Qt::RightToLeft ? UINT(TPM_RIGHTALIGN) : UINT(0),
                          x, y, 0, windowHandle, nullptr) == TRUE;
    emit aboutToHide();
    return result;
}

bool QWindowsPopupMenu::notifyTriggered(uint id)
{
    QPlatformMenuItem *result = lastShownPopupMenu.isNull()
        ? nullptr
        : findMenuItemById(lastShownPopupMenu.data(), id);
    if (result != nullptr) {
        qCDebug(lcQpaMenus) << __FUNCTION__ << "id=" << id;
        emit result->activated();
    }
    lastShownPopupMenu = nullptr;
    return result != nullptr;
}

bool QWindowsPopupMenu::notifyAboutToShow(HMENU hmenu)
{
    if (lastShownPopupMenu.isNull())
        return false;
    if (lastShownPopupMenu->menuHandle() == hmenu) {
        emit lastShownPopupMenu->aboutToShow();
        return true;
    }
    if (QWindowsMenu *menu = findMenuByHandle(lastShownPopupMenu.data(), hmenu)) {
        emit menu->aboutToShow();
        return true;
    }
    return false;
}

// --------------- QWindowsMenuBar

QWindowsMenuBar::QWindowsMenuBar() : m_hMenuBar(CreateMenu())
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << static_cast<const void *>(this);
}

QWindowsMenuBar::~QWindowsMenuBar()
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << static_cast<const void *>(this);
    for (int m = m_menus.size() - 1; m >= 0; --m)
        m_menus.at(m)->removeFromParent();
    removeFromWindow();
    DestroyMenu(m_hMenuBar);
}

void QWindowsMenuBar::insertMenu(QPlatformMenu *menuIn, QPlatformMenu *before)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << menuIn << "before=" << before;
    auto *menu = static_cast<QWindowsMenu *>(menuIn);
    const int index = insertBefore(&m_menus, menuIn, before);
    menu->insertIntoMenuBar(this, index == m_menus.size() - 1, index);
}

void QWindowsMenuBar::removeMenu(QPlatformMenu *menu)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ << '(' << menu << ')' << this;
    const int index = indexOf(m_menus, menu);
    if (index != -1)
        m_menus[index]->removeFromParent();
}

// When calling handleReparent() for a QWindow instances that does not have
// a platform window yet, set the menubar as dynamic property to be installed
// on platform window creation.
static const char menuBarPropertyName[] = "_q_windowsNativeMenuBar";

void QWindowsMenuBar::handleReparent(QWindow *newParentWindow)
{
    qCDebug(lcQpaMenus) << __FUNCTION__ <<  '(' << newParentWindow << ')' << this;
    if (newParentWindow == nullptr) {
        removeFromWindow();
        return; // Happens during Quick Controls 1 property setup
    }
    if (QPlatformWindow *platWin = newParentWindow->handle())
        install(static_cast<QWindowsWindow *>(platWin));
    else // Store for later creation, see menuBarOf()
        newParentWindow->setProperty(menuBarPropertyName, QVariant::fromValue<QObject *>(this));
}

QWindowsMenuBar *QWindowsMenuBar::menuBarOf(const QWindow *notYetCreatedWindow)
{
    const QVariant menuBarV = notYetCreatedWindow->property(menuBarPropertyName);
    return menuBarV.canConvert<QObject *>()
        ? qobject_cast<QWindowsMenuBar *>(menuBarV.value<QObject *>()) : nullptr;
}

void QWindowsMenuBar::install(QWindowsWindow *window)
{
    const HWND hwnd = window->handle();
    const BOOL result = SetMenu(hwnd, m_hMenuBar);
    if (result) {
        window->setMenuBar(this);
        QWindowsContext::forceNcCalcSize(hwnd);
    }
}

void QWindowsMenuBar::removeFromWindow()
{
    if (QWindowsWindow *window = platformWindow()) {
        const HWND hwnd = window->handle();
        SetMenu(hwnd, nullptr);
        window->setMenuBar(nullptr);
        QWindowsContext::forceNcCalcSize(hwnd);
    }
}

QPlatformMenu *QWindowsMenuBar::menuForTag(quintptr tag) const
{
    return traverseMenus(this, [tag] (const QWindowsMenu *m) { return m->tag() == tag; });
}

QPlatformMenu *QWindowsMenuBar::createMenu() const
{
    QPlatformMenu *result = new QWindowsMenu;
    qCDebug(lcQpaMenus) << __FUNCTION__ << this << "returns" << result;
    return result;
}

bool QWindowsMenuBar::notifyTriggered(uint id)
{
    QPlatformMenuItem *result = findMenuItemById(this, id);
    if (result  != nullptr) {
        qCDebug(lcQpaMenus) << __FUNCTION__ << "id=" << id;
        emit result->activated();
    }
    return result != nullptr;
}

bool QWindowsMenuBar::notifyAboutToShow(HMENU hmenu)
{
    if (QWindowsMenu *menu = findMenuByHandle(this, hmenu)) {
        emit menu->aboutToShow();
        return true;
    }
    return false;
}

QWindowsWindow *QWindowsMenuBar::platformWindow() const
{
    if (const QWindowsContext *ctx = QWindowsContext::instance()) {
        if (QWindowsWindow *w = ctx->findPlatformWindow(this))
            return w;
    }
    return nullptr;
}

void QWindowsMenuBar::redraw() const
{
    if (const QWindowsWindow *window = platformWindow())
        DrawMenuBar(window->handle());
}

#ifndef QT_NO_DEBUG_STREAM

template <class M>  /* Menu[Item] */
static void formatTextSequence(QDebug &d, const QVector<M *> &v)
{
    if (const int size = v.size()) {
        d << '[' << size << "](";
        for (int i = 0; i < size; ++i) {
            if (i)
                d << ", ";
            if (!v.at(i)->isVisible())
                d << "[hidden] ";
            d << '"' << v.at(i)->text() << '"';
        }
        d << ')';
    }
}

void QWindowsMenuItem::formatDebug(QDebug &d) const
{
    if (m_separator)
        d << "separator, ";
    else
        d << '"' << m_text << "\", ";
    d << static_cast<const void *>(this);
    if (m_parentMenu)
        d << ", parentMenu=" << static_cast<const void *>(m_parentMenu);
    if (m_subMenu)
        d << ", subMenu=" << static_cast<const void *>(m_subMenu);
    d << ", tag=" << Qt::showbase << Qt::hex
      << tag() << Qt::noshowbase << Qt::dec << ", id=" << m_id;
#if QT_CONFIG(shortcut)
    if (!m_shortcut.isEmpty())
        d << ", shortcut=" << m_shortcut;
#endif
    if (m_visible)
        d << " [visible]";
    if (m_enabled)
        d << " [enabled]";
    if (m_checkable)
        d << ", checked=" << m_checked;
}

QDebug operator<<(QDebug d, const QPlatformMenuItem *i)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "QPlatformMenuItem(";
    if (i)
        static_cast<const QWindowsMenuItem *>(i)->formatDebug(d);
    else
        d << '0';
    d << ')';
    return d;
}

void QWindowsMenu::formatDebug(QDebug &d) const
{
    d << '"' << m_text << "\", " << static_cast<const void *>(this)
      << ", handle=" << m_hMenu;
    if (m_parentMenuBar != nullptr)
        d << " [on menubar]";
    if (m_parentMenu != nullptr)
        d << " [on menu]";
    if (tag())
        d << ", tag=" << Qt::showbase << Qt::hex << tag() << Qt::noshowbase << Qt::dec;
    if (m_visible)
        d << " [visible]";
    if (m_enabled)
        d << " [enabled]";
    d <<' ';
    formatTextSequence(d, m_menuItems);
}

void QWindowsMenuBar::formatDebug(QDebug &d) const
{
    d << static_cast<const void *>(this) << ' ';
    formatTextSequence(d, m_menus);
}

QDebug operator<<(QDebug d, const QPlatformMenu *m)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    if (m) {
        d << m->metaObject()->className() << '(';
        static_cast<const QWindowsMenu *>(m)->formatDebug(d);
        d << ')';
    } else {
        d << "QPlatformMenu(0)";
    }
    return d;
}

QDebug operator<<(QDebug d, const QPlatformMenuBar *mb)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "QPlatformMenuBar(";
    if (mb)
        static_cast<const QWindowsMenuBar *>(mb)->formatDebug(d);
    else
        d << '0';
    d << ')';
    return d;
}

#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
