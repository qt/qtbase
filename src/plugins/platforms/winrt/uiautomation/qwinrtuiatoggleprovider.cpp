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

#include "qwinrtuiatoggleprovider.h"
#include "qwinrtuiametadatacache.h"
#include "qwinrtuiautils.h"

#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/private/qeventdispatcher_winrt_p.h>

QT_BEGIN_NAMESPACE

using namespace QWinRTUiAutomation;
using namespace ABI::Windows::UI::Xaml::Automation;
using namespace ABI::Windows::UI::Xaml::Automation::Provider;

QWinRTUiaToggleProvider::QWinRTUiaToggleProvider(QAccessible::Id id) :
    QWinRTUiaBaseProvider(id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

QWinRTUiaToggleProvider::~QWinRTUiaToggleProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

// Gets the current toggle state.
HRESULT STDMETHODCALLTYPE QWinRTUiaToggleProvider::get_ToggleState(ToggleState *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    if (metadata->state().checked)
        *value = metadata->state().checkStateMixed ? ToggleState_Indeterminate : ToggleState_On;
    else
        *value = ToggleState_Off;
    return S_OK;
}

// Toggles the state by invoking the toggle action.
HRESULT STDMETHODCALLTYPE QWinRTUiaToggleProvider::Toggle()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    auto accid = id();

    QEventDispatcherWinRT::runOnMainThread([accid]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid))
            if (QAccessibleActionInterface *actionInterface = accessible->actionInterface())
                actionInterface->doAction(QAccessibleActionInterface::toggleAction());
        QWinRTUiaMetadataCache::instance()->load(accid);
        return S_OK;
    }, 0);
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
