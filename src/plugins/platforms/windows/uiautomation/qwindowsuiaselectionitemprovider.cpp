// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    if (QAccessibleInterface *parent = accessible->parent()) {
        if (QAccessibleSelectionInterface *selectionInterface = parent->selectionInterface()) {
            selectionInterface->clear();
            bool ok = selectionInterface->select(accessible);
            return ok ? S_OK : S_FALSE;
        }
    }

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (accessible->role() == QAccessible::RadioButton || accessible->role() == QAccessible::PageTab) {
        // For radio buttons/tabs we just invoke the selection action; others are automatically deselected.
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

    if (QAccessibleInterface *parent = accessible->parent()) {
        if (QAccessibleSelectionInterface *selectionInterface = parent->selectionInterface()) {
            bool ok = selectionInterface->select(accessible);
            return ok ? S_OK : S_FALSE;
        }
    }

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (accessible->role() == QAccessible::RadioButton || accessible->role() == QAccessible::PageTab) {
        // For radio buttons and tabs we invoke the selection action.
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

    if (QAccessibleInterface *parent = accessible->parent()) {
        if (QAccessibleSelectionInterface *selectionInterface = parent->selectionInterface()) {
            bool ok = selectionInterface->unselect(accessible);
            return ok ? S_OK : S_FALSE;
        }
    }

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (accessible->role() != QAccessible::RadioButton && accessible->role() != QAccessible::PageTab) {
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

    if (QAccessibleInterface *parent = accessible->parent()) {
        if (QAccessibleSelectionInterface *selectionInterface = parent->selectionInterface()) {
            bool selected = selectionInterface->isSelected(accessible);
            *pRetVal = selected ? TRUE : FALSE;
            return S_OK;
        }
    }

    if (accessible->role() == QAccessible::RadioButton)
        *pRetVal = accessible->state().checked;
    else if (accessible->role() == QAccessible::PageTab)
        *pRetVal = accessible->state().focused;
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

    QAccessibleInterface *parent = accessible->parent();
    if (parent && parent->selectionInterface()) {
        *pRetVal = QWindowsUiaMainProvider::providerForAccessible(parent).Detach();
        return S_OK;
    }

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    // Radio buttons do not require a container.
    if (parent) {
        if ((accessible->role() == QAccessible::ListItem && parent->role() == QAccessible::List)
                || (accessible->role() == QAccessible::PageTab && parent->role() == QAccessible::PageTabList)) {
            *pRetVal = QWindowsUiaMainProvider::providerForAccessible(parent).Detach();
        }
    }
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
