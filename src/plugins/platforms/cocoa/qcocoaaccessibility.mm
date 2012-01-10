/****************************************************************************
 **
 ** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the plugins of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL$
 ** GNU Lesser General Public License Usage
 ** This file may be used under the terms of the GNU Lesser General Public
 ** License version 2.1 as published by the Free Software Foundation and
 ** appearing in the file LICENSE.LGPL included in the packaging of this
 ** file. Please review the following information to ensure the GNU Lesser
 ** General Public License version 2.1 requirements will be met:
 ** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** In addition, as a special exception, Nokia gives you certain additional
 ** rights. These rights are described in the Nokia Qt LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU General
 ** Public License version 3.0 as published by the Free Software Foundation
 ** and appearing in the file LICENSE.GPL included in the packaging of this
 ** file. Please review the following information to ensure the GNU General
 ** Public License version 3.0 requirements will be met:
 ** http://www.gnu.org/copyleft/gpl.html.
 **
 ** Other Usage
 ** Alternatively, this file may be used in accordance with the terms and
 ** conditions contained in a signed written agreement between you and Nokia.
 **
 **
 **
 **
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/
#include "qcocoaaccessibility.h"

namespace QCocoaAccessible {

typedef QMap<QAccessible::Role, NSString *> QMacAccessibiltyRoleMap;
Q_GLOBAL_STATIC(QMacAccessibiltyRoleMap, qMacAccessibiltyRoleMap);

static void populateRoleMap()
{
    QMacAccessibiltyRoleMap &roleMap = *qMacAccessibiltyRoleMap();
    roleMap[QAccessible::MenuItem] = NSAccessibilityMenuItemRole;
    roleMap[QAccessible::MenuBar] = NSAccessibilityMenuBarRole;
    roleMap[QAccessible::ScrollBar] = NSAccessibilityScrollBarRole;
    roleMap[QAccessible::Grip] = NSAccessibilityGrowAreaRole;
    roleMap[QAccessible::Window] = NSAccessibilityWindowRole;
    roleMap[QAccessible::Dialog] = NSAccessibilityWindowRole;
    roleMap[QAccessible::AlertMessage] = NSAccessibilityWindowRole;
    roleMap[QAccessible::ToolTip] = NSAccessibilityWindowRole;
    roleMap[QAccessible::HelpBalloon] = NSAccessibilityWindowRole;
    roleMap[QAccessible::PopupMenu] = NSAccessibilityMenuRole;
    roleMap[QAccessible::Application] = NSAccessibilityApplicationRole;
    roleMap[QAccessible::Pane] = NSAccessibilityGroupRole;
    roleMap[QAccessible::Grouping] = NSAccessibilityGroupRole;
    roleMap[QAccessible::Separator] = NSAccessibilitySplitterRole;
    roleMap[QAccessible::ToolBar] = NSAccessibilityToolbarRole;
    roleMap[QAccessible::PageTab] = NSAccessibilityRadioButtonRole;
    roleMap[QAccessible::ButtonMenu] = NSAccessibilityMenuButtonRole;
    roleMap[QAccessible::ButtonDropDown] = NSAccessibilityPopUpButtonRole;
    roleMap[QAccessible::SpinBox] = NSAccessibilityIncrementorRole;
    roleMap[QAccessible::Slider] = NSAccessibilitySliderRole;
    roleMap[QAccessible::ProgressBar] = NSAccessibilityProgressIndicatorRole;
    roleMap[QAccessible::ComboBox] = NSAccessibilityPopUpButtonRole;
    roleMap[QAccessible::RadioButton] = NSAccessibilityRadioButtonRole;
    roleMap[QAccessible::CheckBox] = NSAccessibilityCheckBoxRole;
    roleMap[QAccessible::StaticText] = NSAccessibilityStaticTextRole;
    roleMap[QAccessible::Table] = NSAccessibilityTableRole;
    roleMap[QAccessible::StatusBar] = NSAccessibilityStaticTextRole;
    roleMap[QAccessible::Column] = NSAccessibilityColumnRole;
    roleMap[QAccessible::ColumnHeader] = NSAccessibilityColumnRole;
    roleMap[QAccessible::Row] = NSAccessibilityRowRole;
    roleMap[QAccessible::RowHeader] = NSAccessibilityRowRole;
    roleMap[QAccessible::Cell] = NSAccessibilityTextFieldRole;
    roleMap[QAccessible::PushButton] = NSAccessibilityButtonRole;
    roleMap[QAccessible::EditableText] = NSAccessibilityTextFieldRole;
    roleMap[QAccessible::Link] = NSAccessibilityTextFieldRole;
    roleMap[QAccessible::Indicator] = NSAccessibilityValueIndicatorRole;
    roleMap[QAccessible::Splitter] = NSAccessibilitySplitGroupRole;
    roleMap[QAccessible::List] = NSAccessibilityListRole;
    roleMap[QAccessible::ListItem] = NSAccessibilityStaticTextRole;
    roleMap[QAccessible::Cell] = NSAccessibilityStaticTextRole;
    roleMap[QAccessible::Client] = NSAccessibilityGroupRole;
}

/*
    Returns a Mac accessibility role for the given interface, or
    NSAccessibilityUnknownRole if no role mapping is found.
*/
NSString *macRole(QAccessible::Role qtRole)
{
    QMacAccessibiltyRoleMap &roleMap = *qMacAccessibiltyRoleMap();

    if (roleMap.isEmpty())
        populateRoleMap();

    // MAC_ACCESSIBILTY_DEBUG() << "role for" << interface.object() << "interface role" << hex << qtRole;

    if (roleMap.contains(qtRole)) {
       // MAC_ACCESSIBILTY_DEBUG() << "return" <<  roleMap[qtRole];
        return roleMap[qtRole];
    }

    // MAC_ACCESSIBILTY_DEBUG() << "return NSAccessibilityUnknownRole";
    return NSAccessibilityUnknownRole;
}

/*
    Translates a predefined QAccessibleActionInterface action to a Mac action constant.
    Returns 0 if the Qt Action has no mac equivalent. Ownership of the NSString is
    not transferred.
*/
NSString *getTranslatedAction(const QString &qtAction)
{
    if (qtAction == QAccessibleActionInterface::pressAction())
        return NSAccessibilityPressAction;
    else if (qtAction == QAccessibleActionInterface::increaseAction())
        return NSAccessibilityIncrementAction;
    else if (qtAction == QAccessibleActionInterface::decreaseAction())
        return NSAccessibilityDecrementAction;
    else if (qtAction == QAccessibleActionInterface::showMenuAction())
        return NSAccessibilityShowMenuAction;
    else if (qtAction == QAccessibleActionInterface::setFocusAction()) // Not 100% sure on this one
        return NSAccessibilityRaiseAction;

    // Not translated:
    //
    // Qt:
    //     static const QString &checkAction();
    //     static const QString &uncheckAction();
    //
    // Cocoa:
    //      NSAccessibilityConfirmAction;
    //      NSAccessibilityPickAction;
    //      NSAccessibilityCancelAction;
    //      NSAccessibilityDeleteAction;

    return 0;
}


/*
    Translates between a Mac action constant and a QAccessibleActionInterface action
    Returns an empty QString if there is no Qt predefined equivalent.
*/
QString translateAction(NSString *nsAction)
{
    if ([nsAction compare: NSAccessibilityPressAction] == NSOrderedSame)
        return QAccessibleActionInterface::pressAction();
    else if ([nsAction compare: NSAccessibilityIncrementAction] == NSOrderedSame)
        return QAccessibleActionInterface::increaseAction();
    else if ([nsAction compare: NSAccessibilityDecrementAction] == NSOrderedSame)
        return QAccessibleActionInterface::decreaseAction();
    else if ([nsAction compare: NSAccessibilityShowMenuAction] == NSOrderedSame)
        return QAccessibleActionInterface::showMenuAction();
    else if ([nsAction compare: NSAccessibilityRaiseAction] == NSOrderedSame)
        return QAccessibleActionInterface::setFocusAction();

    // See getTranslatedAction for not matched translations.

    return QString();
}

} // namespace QCocoaAccessible
