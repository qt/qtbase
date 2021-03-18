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

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiautils.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"

#include <QtGui/qwindow.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <cmath>

QT_BEGIN_NAMESPACE

namespace QWindowsUiAutomation {

// Returns the window containing the element (usually the top window),
QWindow *windowForAccessible(const QAccessibleInterface *accessible)
{
    QWindow *window = accessible->window();
    if (!window) {
        const QAccessibleInterface *acc = accessible;
        const QAccessibleInterface *par = accessible->parent();
        while (par && par->isValid() && !window) {
            window = par->window();
            acc = par;
            par = par->parent();
        }
        if (!window) {
            // Workaround for WebEngineView not knowing its parent.
            const auto appWindows = QGuiApplication::topLevelWindows();
            for (QWindow *w : appWindows) {
                if (QAccessibleInterface *root = w->accessibleRoot()) {
                    int count = root->childCount();
                    for (int i = 0; i < count; ++i) {
                        if (root->child(i) == acc)
                            return w;
                    }
                }
            }
        }
    }
    return window;
}

// Returns the native window handle associated with the element, if any.
// Usually it will be NULL, as Qt5 by default uses alien widgets with no native windows.
HWND hwndForAccessible(const QAccessibleInterface *accessible)
{
    if (QWindow *window = accessible->window()) {
        if (!accessible->parent() || (accessible->parent()->window() != window)) {
            return QWindowsBaseWindow::handleOf(window);
        }
    }
    return nullptr;
}

void clearVariant(VARIANT *variant)
{
    variant->vt = VT_EMPTY;
    variant->punkVal = nullptr;
}

void setVariantI4(int value, VARIANT *variant)
{
    variant->vt = VT_I4;
    variant->lVal = value;
}

void setVariantBool(bool value, VARIANT *variant)
{
    variant->vt = VT_BOOL;
    variant->boolVal = value ? -1 : 0;
}

void setVariantDouble(double value, VARIANT *variant)
{
    variant->vt = VT_R8;
    variant->dblVal = value;
}

BSTR bStrFromQString(const QString &value)
{
    return SysAllocString(reinterpret_cast<const wchar_t *>(value.utf16()));
}

void setVariantString(const QString &value, VARIANT *variant)
{
    variant->vt = VT_BSTR;
    variant->bstrVal = bStrFromQString(value);
}

// Scales a rect to native coordinates, according to high dpi settings.
void rectToNativeUiaRect(const QRect &rect, const QWindow *w, UiaRect *uiaRect)
{
    if (w && uiaRect) {
        const qreal factor = QHighDpiScaling::factor(w);
        uiaRect->left = qreal(rect.x()) * factor;
        uiaRect->top = qreal(rect.y()) * factor;
        uiaRect->width = qreal(rect.width()) * factor;
        uiaRect->height = qreal(rect.height()) * factor;
    }
}

// Scales a point from native coordinates, according to high dpi settings.
void nativeUiaPointToPoint(const UiaPoint &uiaPoint, const QWindow *w, QPoint *point)
{
    if (w && point) {
        const qreal factor = QHighDpiScaling::factor(w);
        point->setX(int(std::lround(uiaPoint.x / factor)));
        point->setY(int(std::lround(uiaPoint.y / factor)));
    }
}

// Maps an accessibility role ID to an UI Automation control type ID.
long roleToControlTypeId(QAccessible::Role role)
{
    static const QHash<QAccessible::Role, long> mapping {
        {QAccessible::TitleBar, UIA_TitleBarControlTypeId},
        {QAccessible::MenuBar, UIA_MenuBarControlTypeId},
        {QAccessible::ScrollBar, UIA_ScrollBarControlTypeId},
        {QAccessible::Grip, UIA_ThumbControlTypeId},
        {QAccessible::Sound, UIA_CustomControlTypeId},
        {QAccessible::Cursor, UIA_CustomControlTypeId},
        {QAccessible::Caret, UIA_CustomControlTypeId},
        {QAccessible::AlertMessage, UIA_WindowControlTypeId},
        {QAccessible::Window, UIA_WindowControlTypeId},
        {QAccessible::Client, UIA_GroupControlTypeId},
        {QAccessible::PopupMenu, UIA_MenuControlTypeId},
        {QAccessible::MenuItem, UIA_MenuItemControlTypeId},
        {QAccessible::ToolTip, UIA_ToolTipControlTypeId},
        {QAccessible::Application, UIA_CustomControlTypeId},
        {QAccessible::Document, UIA_DocumentControlTypeId},
        {QAccessible::Pane, UIA_PaneControlTypeId},
        {QAccessible::Chart, UIA_CustomControlTypeId},
        {QAccessible::Dialog, UIA_WindowControlTypeId},
        {QAccessible::Border, UIA_CustomControlTypeId},
        {QAccessible::Grouping, UIA_GroupControlTypeId},
        {QAccessible::Separator, UIA_SeparatorControlTypeId},
        {QAccessible::ToolBar, UIA_ToolBarControlTypeId},
        {QAccessible::StatusBar, UIA_StatusBarControlTypeId},
        {QAccessible::Table, UIA_TableControlTypeId},
        {QAccessible::ColumnHeader, UIA_HeaderControlTypeId},
        {QAccessible::RowHeader, UIA_HeaderControlTypeId},
        {QAccessible::Column, UIA_HeaderItemControlTypeId},
        {QAccessible::Row, UIA_HeaderItemControlTypeId},
        {QAccessible::Cell, UIA_DataItemControlTypeId},
        {QAccessible::Link, UIA_HyperlinkControlTypeId},
        {QAccessible::HelpBalloon, UIA_ToolTipControlTypeId},
        {QAccessible::Assistant, UIA_CustomControlTypeId},
        {QAccessible::List, UIA_ListControlTypeId},
        {QAccessible::ListItem, UIA_ListItemControlTypeId},
        {QAccessible::Tree, UIA_TreeControlTypeId},
        {QAccessible::TreeItem, UIA_TreeItemControlTypeId},
        {QAccessible::PageTab, UIA_TabItemControlTypeId},
        {QAccessible::PropertyPage, UIA_CustomControlTypeId},
        {QAccessible::Indicator, UIA_CustomControlTypeId},
        {QAccessible::Graphic, UIA_ImageControlTypeId},
        {QAccessible::StaticText, UIA_TextControlTypeId},
        {QAccessible::EditableText, UIA_EditControlTypeId},
        {QAccessible::Button, UIA_ButtonControlTypeId},
        {QAccessible::CheckBox, UIA_CheckBoxControlTypeId},
        {QAccessible::RadioButton, UIA_RadioButtonControlTypeId},
        {QAccessible::ComboBox, UIA_ComboBoxControlTypeId},
        {QAccessible::ProgressBar, UIA_ProgressBarControlTypeId},
        {QAccessible::Dial, UIA_CustomControlTypeId},
        {QAccessible::HotkeyField, UIA_CustomControlTypeId},
        {QAccessible::Slider, UIA_SliderControlTypeId},
        {QAccessible::SpinBox, UIA_SpinnerControlTypeId},
        {QAccessible::Canvas, UIA_CustomControlTypeId},
        {QAccessible::Animation, UIA_CustomControlTypeId},
        {QAccessible::Equation, UIA_CustomControlTypeId},
        {QAccessible::ButtonDropDown, UIA_ButtonControlTypeId},
        {QAccessible::ButtonMenu, UIA_ButtonControlTypeId},
        {QAccessible::ButtonDropGrid, UIA_ButtonControlTypeId},
        {QAccessible::Whitespace, UIA_CustomControlTypeId},
        {QAccessible::PageTabList, UIA_TabControlTypeId},
        {QAccessible::Clock, UIA_CustomControlTypeId},
        {QAccessible::Splitter, UIA_CustomControlTypeId},
    };

    return mapping.value(role, UIA_CustomControlTypeId);
}

// True if a character can be a separator for a text unit.
bool isTextUnitSeparator(TextUnit unit, const QChar &ch)
{
    return (((unit == TextUnit_Word) || (unit == TextUnit_Format)) && ch.isSpace())
            || ((unit == TextUnit_Line) && (ch.toLatin1() == '\n'));
}

} // namespace QWindowsUiAutomation


QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
