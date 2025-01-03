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

    // First put selected items in a list, then build a safe array with the right size.
    QList<QAccessibleInterface *> selectedList;
    for (int i = 0; i < accessible->childCount(); ++i) {
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

    if ((*pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, selectedList.size()))) {
        for (LONG i = 0; i < selectedList.size(); ++i) {
            if (QWindowsUiaMainProvider *childProvider = QWindowsUiaMainProvider::providerForAccessible(selectedList.at(i))) {
                SafeArrayPutElement(*pRetVal, &i, static_cast<IRawElementProviderSimple *>(childProvider));
                childProvider->Release();
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
        for (int i = 0; i < accessible->childCount(); ++i) {
            if (QAccessibleInterface *child = accessible->child(i)) {
                if (child->state().selected) {
                    anySelected = true;
                    break;
                }
            }
        }

        *pRetVal = anySelected && !accessible->state().multiSelectable && !accessible->state().extSelectable;
    }
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
