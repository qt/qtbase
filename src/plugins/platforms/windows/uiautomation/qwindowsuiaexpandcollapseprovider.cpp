/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

    if (accessible->childCount() > 0 && accessible->child(0)->state().invisible)
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

    if (accessible->childCount() > 0 && !accessible->child(0)->state().invisible)
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

    if (accessible->childCount() > 0)
        *pRetVal = accessible->child(0)->state().invisible ?
                ExpandCollapseState_Collapsed : ExpandCollapseState_Expanded;

    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
