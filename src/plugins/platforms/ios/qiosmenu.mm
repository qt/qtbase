/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qglobal.h>
#include <qguiapplication.h>
#include <qpa/qplatformtheme.h>

#include "qiosglobal.h"
#include "qiosmenu.h"
#include "qioswindow.h"
#include "qiosinputcontext.h"
#include "qiosintegration.h"
#include "qiostextresponder.h"

#include <algorithm>
#include <iterator>

// m_currentMenu points to the currently visible menu.
// Only one menu will be visible at a time, and if a second menu
// is shown on top of a first, the first one will be told to hide.
QIOSMenu *QIOSMenu::m_currentMenu = 0;

// -------------------------------------------------------------------------

static NSString *const kSelectorPrefix = @"_qtMenuItem_";

@interface QUIMenuController : UIResponder {
    QIOSMenuItemList m_visibleMenuItems;
}
@end

@implementation QUIMenuController

- (id)initWithVisibleMenuItems:(const QIOSMenuItemList &)visibleMenuItems
{
    if (self = [super init]) {
        [self setVisibleMenuItems:visibleMenuItems];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(menuClosed)
            name:UIMenuControllerDidHideMenuNotification object:nil];
    }

    return self;
}

-(void)dealloc
{
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:UIMenuControllerDidHideMenuNotification object:nil];
    [super dealloc];
}

- (void)setVisibleMenuItems:(const QIOSMenuItemList &)visibleMenuItems
{
    m_visibleMenuItems = visibleMenuItems;
    NSMutableArray *menuItemArray = [NSMutableArray arrayWithCapacity:m_visibleMenuItems.size()];
    // Create an array of UIMenuItems, one for each visible QIOSMenuItem. Each
    // UIMenuItem needs a callback assigned, so we assign one of the placeholder methods
    // added to UIWindow (QIOSMenuActionTargets) below. Each method knows its own index, which
    // corresponds to the index of the corresponding QIOSMenuItem in m_visibleMenuItems. When
    // triggered, menuItemActionCallback will end up being called.
    for (int i = 0; i < m_visibleMenuItems.count(); ++i) {
        QIOSMenuItem *item = m_visibleMenuItems.at(i);
        SEL sel = NSSelectorFromString([NSString stringWithFormat:@"%@%i:", kSelectorPrefix, i]);
        [menuItemArray addObject:[[[UIMenuItem alloc] initWithTitle:item->m_text.toNSString() action:sel] autorelease]];
    }
    [UIMenuController sharedMenuController].menuItems = menuItemArray;
    if ([UIMenuController sharedMenuController].menuVisible)
        [[UIMenuController sharedMenuController] setMenuVisible:YES animated:NO];
}

-(void)menuClosed
{
    QIOSMenu::currentMenu()->dismiss();
}

- (id)targetForAction:(SEL)action withSender:(id)sender
{
    Q_UNUSED(sender);
    BOOL containsPrefix = ([NSStringFromSelector(action) rangeOfString:kSelectorPrefix].location != NSNotFound);
    return containsPrefix ? self : 0;
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)selector
{
    Q_UNUSED(selector);
    // Just return a dummy signature that NSObject can create an NSInvocation from.
    // We end up only checking selector in forwardInvocation anyway.
    return [super methodSignatureForSelector:@selector(methodSignatureForSelector:)];
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    // Since none of the menu item selector methods actually exist, this function
    // will end up being called as a final resort. We can then handle the action.
    NSString *selector = NSStringFromSelector(invocation.selector);
    NSRange range = NSMakeRange(kSelectorPrefix.length, selector.length - kSelectorPrefix.length - 1);
    NSInteger selectedIndex = [[selector substringWithRange:range] integerValue];
    QIOSMenu::currentMenu()->handleItemSelected(m_visibleMenuItems.at(selectedIndex));
}

@end

// -------------------------------------------------------------------------

@interface QUIPickerView : UIPickerView <UIPickerViewDelegate, UIPickerViewDataSource> {
    QIOSMenuItemList m_visibleMenuItems;
    QPointer<QObject> m_focusObjectWithPickerView;
    NSInteger m_selectedRow;
}

@property(retain) UIToolbar *toolbar;

@end

@implementation QUIPickerView

- (id)initWithVisibleMenuItems:(const QIOSMenuItemList &)visibleMenuItems selectItem:(const QIOSMenuItem *)selectItem
{
    if (self = [super init]) {
        [self setVisibleMenuItems:visibleMenuItems selectItem:selectItem];

        self.autoresizingMask = UIViewAutoresizingFlexibleWidth;
        self.toolbar = [[[UIToolbar alloc] init] autorelease];
        self.toolbar.frame.size = [self.toolbar sizeThatFits:self.bounds.size];
        self.toolbar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

        UIBarButtonItem *spaceButton = [[[UIBarButtonItem alloc]
                initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace
                target:self action:@selector(closeMenu)] autorelease];
        UIBarButtonItem *cancelButton = [[[UIBarButtonItem alloc]
                initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                target:self action:@selector(cancelMenu)] autorelease];
        UIBarButtonItem *doneButton = [[[UIBarButtonItem alloc]
                initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                target:self action:@selector(closeMenu)] autorelease];
        [self.toolbar setItems:[NSArray arrayWithObjects:cancelButton, spaceButton, doneButton, nil]];

        [self setDelegate:self];
        [self setDataSource:self];
        [self selectRow:m_selectedRow inComponent:0 animated:false];
        [self listenForKeyboardWillHideNotification:YES];
    }

    return self;
}

- (void)setVisibleMenuItems:(const QIOSMenuItemList &)visibleMenuItems selectItem:(const QIOSMenuItem *)selectItem
{
    m_visibleMenuItems = visibleMenuItems;
    m_selectedRow = visibleMenuItems.indexOf(const_cast<QIOSMenuItem *>(selectItem));
    if (m_selectedRow == -1)
        m_selectedRow = 0;
    [self reloadAllComponents];
}

- (void)listenForKeyboardWillHideNotification:(BOOL)listen
{
    if (listen) {
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(cancelMenu)
            name:@"UIKeyboardWillHideNotification" object:nil];
    } else {
        [[NSNotificationCenter defaultCenter]
            removeObserver:self
            name:@"UIKeyboardWillHideNotification" object:nil];
    }
}

- (void)dealloc
{
    [self listenForKeyboardWillHideNotification:NO];
    self.toolbar = 0;
    [super dealloc];
}

- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
    Q_UNUSED(pickerView);
    Q_UNUSED(component);
    return m_visibleMenuItems.at(row)->m_text.toNSString();
}

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
    Q_UNUSED(pickerView);
    return 1;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
    Q_UNUSED(pickerView);
    Q_UNUSED(component);
    return m_visibleMenuItems.length();
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
    Q_UNUSED(pickerView);
    Q_UNUSED(component);
    m_selectedRow = row;
}

- (void)closeMenu
{
    if (!m_visibleMenuItems.isEmpty())
        QIOSMenu::currentMenu()->handleItemSelected(m_visibleMenuItems.at(m_selectedRow));
    else
        QIOSMenu::currentMenu()->dismiss();
}

- (void)cancelMenu
{
    QIOSMenu::currentMenu()->dismiss();
}

@end

// -------------------------------------------------------------------------

QIOSMenuItem::QIOSMenuItem()
    : QPlatformMenuItem()
    , m_tag(0)
    , m_visible(true)
    , m_text(QString())
    , m_role(MenuRole(0))
    , m_enabled(true)
    , m_separator(false)
    , m_menu(0)
{
}

void QIOSMenuItem::setTag(quintptr tag)
{
    m_tag = tag;
}

quintptr QIOSMenuItem::tag() const
{
    return m_tag;
}

void QIOSMenuItem::setText(const QString &text)
{
    m_text = QPlatformTheme::removeMnemonics(text);
}

void QIOSMenuItem::setMenu(QPlatformMenu *menu)
{
   m_menu = static_cast<QIOSMenu *>(menu);
}

void QIOSMenuItem::setVisible(bool isVisible)
{
    m_visible = isVisible;
}

void QIOSMenuItem::setIsSeparator(bool isSeparator)
{
   m_separator = isSeparator;
}

void QIOSMenuItem::setRole(QPlatformMenuItem::MenuRole role)
{
    m_role = role;
}

void QIOSMenuItem::setShortcut(const QKeySequence &sequence)
{
    m_shortcut = sequence;
}

void QIOSMenuItem::setEnabled(bool enabled)
{
    m_enabled = enabled;
}


QIOSMenu::QIOSMenu()
    : QPlatformMenu()
    , m_tag(0)
    , m_enabled(true)
    , m_visible(true)
    , m_text(QString())
    , m_menuType(DefaultMenu)
    , m_effectiveMenuType(DefaultMenu)
    , m_parentWindow(0)
    , m_targetItem(0)
    , m_menuController(0)
    , m_pickerView(0)
{
}

QIOSMenu::~QIOSMenu()
{
    dismiss();
}

void QIOSMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
    if (!before) {
        m_menuItems.append(static_cast<QIOSMenuItem *>(menuItem));
    } else {
        int index = m_menuItems.indexOf(static_cast<QIOSMenuItem *>(before)) + 1;
        m_menuItems.insert(index, static_cast<QIOSMenuItem *>(menuItem));
    }
    if (m_currentMenu == this)
        syncMenuItem(menuItem);
}

void QIOSMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    m_menuItems.removeOne(static_cast<QIOSMenuItem *>(menuItem));
    if (m_currentMenu == this)
        syncMenuItem(menuItem);
}

void QIOSMenu::syncMenuItem(QPlatformMenuItem *)
{
    if (m_currentMenu != this)
        return;

    switch (m_effectiveMenuType) {
    case EditMenu:
        [m_menuController setVisibleMenuItems:filterFirstResponderActions(visibleMenuItems())];
        break;
    default:
        [m_pickerView setVisibleMenuItems:visibleMenuItems() selectItem:m_targetItem];
        break;
    }
}

void QIOSMenu::setTag(quintptr tag)
{
    m_tag = tag;
}

quintptr QIOSMenu::tag() const
{
    return m_tag;
}

void QIOSMenu::setText(const QString &text)
{
   m_text = text;
}

void QIOSMenu::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void QIOSMenu::setVisible(bool visible)
{
    m_visible = visible;
}

void QIOSMenu::setMenuType(QPlatformMenu::MenuType type)
{
    m_menuType = type;
}

void QIOSMenu::handleItemSelected(QIOSMenuItem *menuItem)
{
    emit menuItem->activated();
    dismiss();

    if (QIOSMenu *menu = menuItem->m_menu) {
        menu->setMenuType(m_effectiveMenuType);
        menu->showPopup(m_parentWindow, m_targetRect, 0);
    }
}

void QIOSMenu::showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item)
{
    if (m_currentMenu == this || !m_visible || !m_enabled || !parentWindow)
        return;

    emit aboutToShow();

    m_parentWindow = const_cast<QWindow *>(parentWindow);
    m_targetRect = targetRect;
    m_targetItem = static_cast<const QIOSMenuItem *>(item);

    if (!m_parentWindow->isActive())
        m_parentWindow->requestActivate();

    if (m_currentMenu && m_currentMenu != this)
        m_currentMenu->dismiss();

    m_currentMenu = this;
    m_effectiveMenuType = m_menuType;
    connect(qGuiApp, &QGuiApplication::focusObjectChanged, this, &QIOSMenu::dismiss);

    switch (m_effectiveMenuType) {
    case EditMenu:
        toggleShowUsingUIMenuController(true);
        break;
    default:
        toggleShowUsingUIPickerView(true);
        break;
    }
}

void QIOSMenu::dismiss()
{
    if (m_currentMenu != this)
        return;

    emit aboutToHide();

    disconnect(qGuiApp, &QGuiApplication::focusObjectChanged, this, &QIOSMenu::dismiss);

    switch (m_effectiveMenuType) {
    case EditMenu:
        toggleShowUsingUIMenuController(false);
        break;
    default:
        toggleShowUsingUIPickerView(false);
        break;
    }

    m_currentMenu = 0;
}

void QIOSMenu::toggleShowUsingUIMenuController(bool show)
{
    if (show) {
        Q_ASSERT(!m_menuController);
        m_menuController = [[QUIMenuController alloc] initWithVisibleMenuItems:filterFirstResponderActions(visibleMenuItems())];
        repositionMenu();
        connect(qGuiApp->inputMethod(), &QInputMethod::keyboardRectangleChanged, this, &QIOSMenu::repositionMenu);
    } else {
        disconnect(qGuiApp->inputMethod(), &QInputMethod::keyboardRectangleChanged, this, &QIOSMenu::repositionMenu);

        Q_ASSERT(m_menuController);
        [[UIMenuController sharedMenuController] setMenuVisible:NO animated:YES];
        [m_menuController release];
        m_menuController = 0;
    }
}

void QIOSMenu::toggleShowUsingUIPickerView(bool show)
{
    static QObject *focusObjectWithPickerView = 0;

    if (show) {
        Q_ASSERT(!m_pickerView);
        m_pickerView = [[QUIPickerView alloc] initWithVisibleMenuItems:visibleMenuItems() selectItem:m_targetItem];

        Q_ASSERT(!focusObjectWithPickerView);
        focusObjectWithPickerView = qApp->focusWindow()->focusObject();
        focusObjectWithPickerView->installEventFilter(this);
        qApp->inputMethod()->update(Qt::ImEnabled | Qt::ImPlatformData);
    } else {
        Q_ASSERT(focusObjectWithPickerView);
        focusObjectWithPickerView->removeEventFilter(this);
        focusObjectWithPickerView = 0;

        Q_ASSERT(m_pickerView);
        [m_pickerView listenForKeyboardWillHideNotification:NO];
        [m_pickerView release];
        m_pickerView = 0;

        qApp->inputMethod()->update(Qt::ImEnabled | Qt::ImPlatformData);
    }
}

bool QIOSMenu::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::InputMethodQuery) {
        QInputMethodQueryEvent *queryEvent = static_cast<QInputMethodQueryEvent *>(event);
        if (queryEvent->queries() & Qt::ImPlatformData) {
            // Let object fill inn default query results
            obj->event(queryEvent);

            QVariantMap imPlatformData = queryEvent->value(Qt::ImPlatformData).toMap();
            imPlatformData.insert(kImePlatformDataInputView, QVariant::fromValue(static_cast<void *>(m_pickerView)));
            imPlatformData.insert(kImePlatformDataInputAccessoryView, QVariant::fromValue(static_cast<void *>(m_pickerView.toolbar)));
            queryEvent->setValue(Qt::ImPlatformData, imPlatformData);
            queryEvent->setValue(Qt::ImEnabled, true);

            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

QIOSMenuItemList QIOSMenu::visibleMenuItems() const
{
    QIOSMenuItemList visibleMenuItems;
    visibleMenuItems.reserve(m_menuItems.size());
    std::copy_if(m_menuItems.begin(), m_menuItems.end(), std::back_inserter(visibleMenuItems),
                 [](QIOSMenuItem *item) { return item->m_enabled && item->m_visible && !item->m_separator; });
    return visibleMenuItems;
}

QIOSMenuItemList QIOSMenu::filterFirstResponderActions(const QIOSMenuItemList &menuItems)
{
    // UIResponderStandardEditActions found in first responder will be prepended to the edit
    // menu automatically (or e.g made available as buttons on the virtual keyboard). So we
    // filter them out to avoid duplicates, and let first responder handle the actions instead.
    // In case of QIOSTextResponder, edit actions will be converted to key events that ends up
    // triggering the shortcuts of the filtered menu items.
    QIOSMenuItemList filteredMenuItems;
    UIResponder *responder = [UIResponder currentFirstResponder];

    for (int i = 0; i < menuItems.count(); ++i) {
        QIOSMenuItem *menuItem = menuItems.at(i);
        QKeySequence shortcut = menuItem->m_shortcut;
        if ((shortcut == QKeySequence::Cut && [responder canPerformAction:@selector(cut:) withSender:nil])
                || (shortcut == QKeySequence::Copy && [responder canPerformAction:@selector(copy:) withSender:nil])
                || (shortcut == QKeySequence::Paste && [responder canPerformAction:@selector(paste:) withSender:nil])
                || (shortcut == QKeySequence::Delete && [responder canPerformAction:@selector(delete:) withSender:nil])
                || (shortcut == QKeySequence::SelectAll && [responder canPerformAction:@selector(selectAll:) withSender:nil])
                || (shortcut == QKeySequence::Undo && [responder canPerformAction:@selector(undo:) withSender:nil])
                || (shortcut == QKeySequence::Redo && [responder canPerformAction:@selector(redo:) withSender:nil])
                || (shortcut == QKeySequence::Bold && [responder canPerformAction:@selector(toggleBoldface:) withSender:nil])
                || (shortcut == QKeySequence::Italic && [responder canPerformAction:@selector(toggleItalics:) withSender:nil])
                || (shortcut == QKeySequence::Underline && [responder canPerformAction:@selector(toggleUnderline:) withSender:nil])) {
            continue;
        }
        filteredMenuItems.append(menuItem);
    }
    return filteredMenuItems;
}

void QIOSMenu::repositionMenu()
{
    switch (m_effectiveMenuType) {
    case EditMenu: {
        UIView *view = reinterpret_cast<UIView *>(m_parentWindow->winId());
        [[UIMenuController sharedMenuController] setTargetRect:toCGRect(m_targetRect) inView:view];
        [[UIMenuController sharedMenuController] setMenuVisible:YES animated:YES];
        break; }
    default:
        break;
    }
}

QPlatformMenuItem *QIOSMenu::menuItemAt(int position) const
{
    if (position < 0 || position >= m_menuItems.size())
        return 0;
    return m_menuItems.at(position);
}

QPlatformMenuItem *QIOSMenu::menuItemForTag(quintptr tag) const
{
    for (int i = 0; i < m_menuItems.size(); ++i) {
        QPlatformMenuItem *item = m_menuItems.at(i);
        if (item->tag() == tag)
            return item;
    }
    return 0;
}
