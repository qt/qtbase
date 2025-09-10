// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiaselectionprovider.h"
#include "qwindowsuiamainprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaSelectionProvider::QWindowsUiaSelectionProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaSelectionProvider::~QWindowsUiaSelectionProvider()
{
}

// Returns an array of providers with the selected items.
HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionProvider::GetSelection(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    // First get/create list of selected items, then build a safe array with the right size.
    QList<QAccessibleInterface *> selectedList;
    if (QAccessibleSelectionInterface *selectionInterface = accessible->selectionInterface()) {
        selectedList = selectionInterface->selectedItems();
    } else {
        const int childCount = accessible->childCount();
        selectedList.reserve(childCount);
        for (int i = 0; i < childCount; ++i) {
            if (QAccessibleInterface *child = accessible->child(i)) {
                if (accessible->role() == QAccessible::PageTabList) {
                    if (child->role() == QAccessible::PageTab && child->state().focused) {
                        selectedList.append(child);
                    }
                } else {
                    if (child->state().selected) {
                        selectedList.append(child);
                    }
                }
            }
        }
    }

    if ((*pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, selectedList.size()))) {
        for (LONG i = 0; i < selectedList.size(); ++i) {
            if (ComPtr<IRawElementProviderSimple> provider =
                        QWindowsUiaMainProvider::providerForAccessible(selectedList.at(i))) {
                SafeArrayPutElement(*pRetVal, &i, provider.Get());
            }
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionProvider::get_CanSelectMultiple(BOOL *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = FALSE;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = accessible->state().multiSelectable;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionProvider::get_IsSelectionRequired(BOOL *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = FALSE;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (accessible->role() == QAccessible::PageTabList) {
        *pRetVal = TRUE;
    } else {

        // Initially returns false if none are selected. After the first selection, it may be required.
        bool anySelected = false;
        if (QAccessibleSelectionInterface *selectionInterface = accessible->selectionInterface()) {
            anySelected = selectionInterface->selectedItem(0) != nullptr;
        } else {
            for (int i = 0; i < accessible->childCount(); ++i) {
                if (QAccessibleInterface *child = accessible->child(i)) {
                    if (child->state().selected) {
                        anySelected = true;
                        break;
                    }
                }
            }
        }

        *pRetVal = anySelected && !accessible->state().multiSelectable && !accessible->state().extSelectable;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionProvider::get_FirstSelectedItem(__RPC__deref_out_opt IRawElementProviderSimple **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleInterface *firstSelectedChild = nullptr;
    if (QAccessibleSelectionInterface *selectionInterface = accessible->selectionInterface()) {
        firstSelectedChild = selectionInterface->selectedItem(0);
        if (!firstSelectedChild)
            return UIA_E_ELEMENTNOTAVAILABLE;
    } else {
        int i = 0;
        while (!firstSelectedChild && i < accessible->childCount()) {
            if (QAccessibleInterface *child = accessible->child(i)) {
                if (accessible->role() == QAccessible::PageTabList) {
                    if (child->role() == QAccessible::PageTab && child->state().focused)
                        firstSelectedChild = child;
                } else if (child->state().selected) {
                    firstSelectedChild = child;
                }
            }
            i++;
        }
    }

    if (!firstSelectedChild)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (ComPtr<IRawElementProviderSimple> childProvider =
                QWindowsUiaMainProvider::providerForAccessible(firstSelectedChild)) {
        *pRetVal = childProvider.Detach();
        return S_OK;
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionProvider::get_LastSelectedItem(__RPC__deref_out_opt IRawElementProviderSimple **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleInterface *lastSelectedChild = nullptr;
    if (QAccessibleSelectionInterface *selectionInterface = accessible->selectionInterface()) {
        const int selectedItemCount = selectionInterface->selectedItemCount();
        if (selectedItemCount <= 0)
            return UIA_E_ELEMENTNOTAVAILABLE;
        lastSelectedChild = selectionInterface->selectedItem(selectedItemCount - 1);
    } else {
        int i = accessible->childCount() - 1;
        while (!lastSelectedChild && i >= 0) {
            if (QAccessibleInterface *child = accessible->child(i)) {
                if (accessible->role() == QAccessible::PageTabList) {
                    if (child->role() == QAccessible::PageTab && child->state().focused)
                        lastSelectedChild = child;
                } else if (child->state().selected) {
                    lastSelectedChild = child;
                }
            }
            i--;
        }
    }

    if (!lastSelectedChild)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (ComPtr<IRawElementProviderSimple> childProvider =
                QWindowsUiaMainProvider::providerForAccessible(lastSelectedChild)) {
        *pRetVal = childProvider.Detach();
        return S_OK;
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionProvider::get_CurrentSelectedItem(__RPC__deref_out_opt IRawElementProviderSimple **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    return get_FirstSelectedItem(pRetVal);
}

HRESULT STDMETHODCALLTYPE QWindowsUiaSelectionProvider::get_ItemCount(__RPC__out int *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = -1;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;


    if (QAccessibleSelectionInterface *selectionInterface = accessible->selectionInterface())
        *pRetVal = selectionInterface->selectedItemCount();
    else {
        int selectedCount = 0;
        for (int i = 0; i < accessible->childCount(); i++) {
            if (QAccessibleInterface *child = accessible->child(i)) {
                if (accessible->role() == QAccessible::PageTabList) {
                    if (child->role() == QAccessible::PageTab && child->state().focused)
                        selectedCount++;
                } else if (child->state().selected) {
                    selectedCount++;
                }
            }
        }
        *pRetVal = selectedCount;
    }

    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
