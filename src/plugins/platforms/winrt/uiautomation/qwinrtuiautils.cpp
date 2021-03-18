/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiautils.h"

using namespace ABI::Windows::UI::Xaml::Automation::Peers;
using namespace ABI::Windows::UI::Xaml::Automation::Text;
using namespace ABI::Windows::Foundation::Collections;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaUiAutomation, "qt.qpa.uiautomation")

namespace QWinRTUiAutomation {

// Returns the window containing the element (usually the top window),
QWindow *windowForAccessible(const QAccessibleInterface *accessible)
{
    QWindow *window = accessible->window();
    if (!window) {
        QAccessibleInterface *acc = accessible->parent();
        while (acc && acc->isValid() && !window) {
            window = acc->window();
            QAccessibleInterface *par = acc->parent();
            acc = par;
        }
    }
    return window;
}

QAccessibleInterface *accessibleForId(QAccessible::Id id)
{
    QAccessibleInterface *accessible = QAccessible::accessibleInterface(id);
    if (!accessible || !accessible->isValid())
        return nullptr;
    return accessible;
}

QAccessible::Id idForAccessible(QAccessibleInterface *accessible)
{
    if (!accessible)
        return QAccessible::Id(0);
    return QAccessible::uniqueId(accessible);
}

// Maps an accessibility role ID to an UI Automation control type ID.
AutomationControlType roleToControlType(QAccessible::Role role)
{
    static const QHash<QAccessible::Role, AutomationControlType> mapping {
        {QAccessible::TitleBar, AutomationControlType::AutomationControlType_TitleBar},
        {QAccessible::MenuBar, AutomationControlType::AutomationControlType_MenuBar},
        {QAccessible::ScrollBar, AutomationControlType::AutomationControlType_ScrollBar},
        {QAccessible::Grip, AutomationControlType::AutomationControlType_Thumb},
        {QAccessible::Sound, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Cursor, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Caret, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::AlertMessage, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Window, AutomationControlType::AutomationControlType_Window},
        {QAccessible::Client, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::PopupMenu, AutomationControlType::AutomationControlType_Menu},
        {QAccessible::MenuItem, AutomationControlType::AutomationControlType_MenuItem},
        {QAccessible::ToolTip, AutomationControlType::AutomationControlType_ToolTip},
        {QAccessible::Application, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Document, AutomationControlType::AutomationControlType_Document},
        {QAccessible::Pane, AutomationControlType::AutomationControlType_Pane},
        {QAccessible::Chart, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Dialog, AutomationControlType::AutomationControlType_Window},
        {QAccessible::Border, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Grouping, AutomationControlType::AutomationControlType_Group},
        {QAccessible::Separator, AutomationControlType::AutomationControlType_Separator},
        {QAccessible::ToolBar, AutomationControlType::AutomationControlType_ToolBar},
        {QAccessible::StatusBar, AutomationControlType::AutomationControlType_StatusBar},
        {QAccessible::Table, AutomationControlType::AutomationControlType_Table},
        {QAccessible::ColumnHeader, AutomationControlType::AutomationControlType_Header},
        {QAccessible::RowHeader, AutomationControlType::AutomationControlType_Header},
        {QAccessible::Column, AutomationControlType::AutomationControlType_HeaderItem},
        {QAccessible::Row, AutomationControlType::AutomationControlType_HeaderItem},
        {QAccessible::Cell, AutomationControlType::AutomationControlType_DataItem},
        {QAccessible::Link, AutomationControlType::AutomationControlType_Hyperlink},
        {QAccessible::HelpBalloon, AutomationControlType::AutomationControlType_ToolTip},
        {QAccessible::Assistant, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::List, AutomationControlType::AutomationControlType_List},
        {QAccessible::ListItem, AutomationControlType::AutomationControlType_ListItem},
        {QAccessible::Tree, AutomationControlType::AutomationControlType_Tree},
        {QAccessible::TreeItem, AutomationControlType::AutomationControlType_TreeItem},
        {QAccessible::PageTab, AutomationControlType::AutomationControlType_TabItem},
        {QAccessible::PropertyPage, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Indicator, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Graphic, AutomationControlType::AutomationControlType_Image},
        {QAccessible::StaticText, AutomationControlType::AutomationControlType_Text},
        {QAccessible::EditableText, AutomationControlType::AutomationControlType_Edit},
        {QAccessible::Button, AutomationControlType::AutomationControlType_Button},
        {QAccessible::CheckBox, AutomationControlType::AutomationControlType_CheckBox},
        {QAccessible::RadioButton, AutomationControlType::AutomationControlType_RadioButton},
        {QAccessible::ComboBox, AutomationControlType::AutomationControlType_ComboBox},
        {QAccessible::ProgressBar, AutomationControlType::AutomationControlType_ProgressBar},
        {QAccessible::Dial, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::HotkeyField, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Slider, AutomationControlType::AutomationControlType_Slider},
        {QAccessible::SpinBox, AutomationControlType::AutomationControlType_Spinner},
        {QAccessible::Canvas, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Animation, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Equation, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::ButtonDropDown, AutomationControlType::AutomationControlType_Button},
        {QAccessible::ButtonMenu, AutomationControlType::AutomationControlType_Button},
        {QAccessible::ButtonDropGrid, AutomationControlType::AutomationControlType_Button},
        {QAccessible::Whitespace, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::PageTabList, AutomationControlType::AutomationControlType_Tab},
        {QAccessible::Clock, AutomationControlType::AutomationControlType_Custom},
        {QAccessible::Splitter, AutomationControlType::AutomationControlType_Custom},
    };

    return mapping.value(role, AutomationControlType::AutomationControlType_Custom);
}

// True if a character can be a separator for a text unit.
bool isTextUnitSeparator(TextUnit unit, const QChar &ch)
{
    return (((unit == TextUnit_Word) || (unit == TextUnit_Format)) && ch.isSpace())
            || ((unit == TextUnit_Line) && (ch.toLatin1() == '\n'));
}

HRESULT qHString(const QString &str, HSTRING *returnValue)
{
    if (!returnValue)
        return E_INVALIDARG;

    const wchar_t *wstr = reinterpret_cast<const wchar_t *>(str.utf16());
    return ::WindowsCreateString(wstr, static_cast<UINT32>(::wcslen(wstr)), returnValue);
}

QString hStrToQStr(const HSTRING &hStr)
{
    quint32 len;
    const wchar_t *wstr = ::WindowsGetStringRawBuffer(hStr, &len);
    return QString::fromWCharArray(wstr, len);
}

} // namespace QWinRTUiAutomation

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
