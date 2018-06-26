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

#include "qwindowsuiaselectionitemprovider.h"
#include "qwindowsuiamainprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaSelectionItemProvider::QWindowsUiaSelectionItemProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaSelectionItemProvider::~QWindowsUiaSelectionItemProvider()
{
}

// Selects the element (deselecting all others).
HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionItemProvider::Select()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (accessible->role() == QAccessible::RadioButton) {
        // For radio buttons we just invoke the selection action; others are automatically deselected.
        actionInterface->doAction(QAccessibleActionInterface::pressAction());
    } else {
        // Toggle list item if not already selected. It must be done first to support all selection modes.
        if (!accessible->state().selected) {
            actionInterface->doAction(QAccessibleActionInterface::toggleAction());
        }
        // Toggle selected siblings.
        if (QAccessibleInterface *parent = accessible->parent()) {
            for (int i = 0; i < parent->childCount(); ++i) {
                if (QAccessibleInterface *sibling = parent->child(i)) {
                    if ((sibling != accessible) && (sibling->state().selected)) {
                        if (QAccessibleActionInterface *siblingAction = sibling->actionInterface()) {
                            siblingAction->doAction(QAccessibleActionInterface::toggleAction());
                        }
                    }
                }
            }
        }
    }
    return S_OK;
}

// Adds the element to the list of selected elements.
HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionItemProvider::AddToSelection()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (accessible->role() == QAccessible::RadioButton) {
        // For radio buttons we invoke the selection action.
        actionInterface->doAction(QAccessibleActionInterface::pressAction());
    } else {
        // Toggle list item if not already selected.
        if (!accessible->state().selected) {
            actionInterface->doAction(QAccessibleActionInterface::toggleAction());
        }
    }
    return S_OK;
}

// Removes a list item from selection.
HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionItemProvider::RemoveFromSelection()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (accessible->role() != QAccessible::RadioButton) {
        if (accessible->state().selected) {
            actionInterface->doAction(QAccessibleActionInterface::toggleAction());
        }
    }

    return S_OK;
}

// Returns true if element is currently selected.
HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionItemProvider::get_IsSelected(BOOL *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = FALSE;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (accessible->role() == QAccessible::RadioButton)
        *pRetVal = accessible->state().checked;
    else
        *pRetVal = accessible->state().selected;
    return S_OK;
}

// Returns the provider for the container element (e.g., the list for the list item).
HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionItemProvider::get_SelectionContainer(IRawElementProviderSimple **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    // Radio buttons do not require a container.
    if (accessible->role() == QAccessible::ListItem) {
        if (QAccessibleInterface *parent = accessible->parent()) {
            if (parent->role() == QAccessible::List) {
                *pRetVal = QWindowsUiaMainProvider::providerForAccessible(parent);
            }
        }
    }
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
