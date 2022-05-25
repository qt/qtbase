// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiaexpandcollapseprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;

static bool isExpanded(QAccessibleInterface *accessible)
{
    Q_ASSERT(accessible);
    if (accessible->role() == QAccessible::MenuItem)
        return accessible->childCount() > 0 && !accessible->child(0)->state().invisible;
    else
        return accessible->state().expandable && accessible->state().expanded;
}

QWindowsUiaExpandCollapseProvider::QWindowsUiaExpandCollapseProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaExpandCollapseProvider::~QWindowsUiaExpandCollapseProvider() = default;

HRESULT STDMETHODCALLTYPE QWindowsUiaExpandCollapseProvider::Expand()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (!isExpanded(accessible))
        actionInterface->doAction(QAccessibleActionInterface::showMenuAction());

    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaExpandCollapseProvider::Collapse()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (isExpanded(accessible))
        actionInterface->doAction(QAccessibleActionInterface::showMenuAction());

    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaExpandCollapseProvider::get_ExpandCollapseState(__RPC__out ExpandCollapseState *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = ExpandCollapseState_LeafNode;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = isExpanded(accessible) ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed;

    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
