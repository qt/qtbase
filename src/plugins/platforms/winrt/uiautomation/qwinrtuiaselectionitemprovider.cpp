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

#include "qwinrtuiaselectionitemprovider.h"
#include "qwinrtuiamainprovider.h"
#include "qwinrtuiametadatacache.h"
#include "qwinrtuiautils.h"

#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/private/qeventdispatcher_winrt_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

using namespace QWinRTUiAutomation;
using namespace ABI::Windows::UI::Xaml::Automation;
using namespace ABI::Windows::UI::Xaml::Automation::Provider;
using namespace ABI::Windows::UI::Xaml::Automation::Peers;

QWinRTUiaSelectionItemProvider::QWinRTUiaSelectionItemProvider(QAccessible::Id id) :
    QWinRTUiaBaseProvider(id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

QWinRTUiaSelectionItemProvider::~QWinRTUiaSelectionItemProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

// Returns true if element is currently selected.
HRESULT STDMETHODCALLTYPE QWinRTUiaSelectionItemProvider::get_IsSelected(boolean *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    if (metadata->role() == QAccessible::RadioButton)
        *value = metadata->state().checked;
    else
        *value = metadata->state().selected;
    return S_OK;
}

// Returns the provider for the container element (e.g., the list for the list item).
HRESULT STDMETHODCALLTYPE QWinRTUiaSelectionItemProvider::get_SelectionContainer(IIRawElementProviderSimple **value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;
    *value = nullptr;

    auto accid = id();
    auto elementId = std::make_shared<QAccessible::Id>(0);

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([accid, elementId]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            // Radio buttons do not require a container.
            if (accessible->role() == QAccessible::ListItem) {
                if (QAccessibleInterface *parent = accessible->parent()) {
                    if (parent->role() == QAccessible::List) {
                        *elementId = idForAccessible(parent);
                    }
                }
            }
        }
        return S_OK;
    }))) {
        return E_FAIL;
    }

    if (!*elementId)
        return S_OK;

    return QWinRTUiaMainProvider::rawProviderForAccessibleId(*elementId, value);
}

// Adds the element to the list of selected elements.
HRESULT STDMETHODCALLTYPE QWinRTUiaSelectionItemProvider::AddToSelection()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    auto accid = id();

    QEventDispatcherWinRT::runOnMainThread([accid]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            if (QAccessibleActionInterface *actionInterface = accessible->actionInterface()) {
                if (accessible->role() == QAccessible::RadioButton) {
                    // For radio buttons we invoke the selection action.
                    actionInterface->doAction(QAccessibleActionInterface::pressAction());
                } else {
                    // Toggle list item if not already selected.
                    if (!accessible->state().selected) {
                        actionInterface->doAction(QAccessibleActionInterface::toggleAction());
                    }
                }
            }
        }
        QWinRTUiaMetadataCache::instance()->load(accid);
        return S_OK;
    }, 0);
    return S_OK;
}

// Removes a list item from selection.
HRESULT STDMETHODCALLTYPE QWinRTUiaSelectionItemProvider::RemoveFromSelection()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    auto accid = id();

    QEventDispatcherWinRT::runOnMainThread([accid]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            if (QAccessibleActionInterface *actionInterface = accessible->actionInterface()) {
                if (accessible->role() != QAccessible::RadioButton) {
                    if (accessible->state().selected) {
                        actionInterface->doAction(QAccessibleActionInterface::toggleAction());
                    }
                }
            }
        }
        QWinRTUiaMetadataCache::instance()->load(accid);
        return S_OK;
    }, 0);
    return S_OK;
}

// Selects the element (deselecting all others).
HRESULT STDMETHODCALLTYPE QWinRTUiaSelectionItemProvider::Select()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    auto accid = id();

    QEventDispatcherWinRT::runOnMainThread([accid]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            if (QAccessibleActionInterface *actionInterface = accessible->actionInterface()) {
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
            }
        }
        QWinRTUiaMetadataCache::instance()->load(accid);
        return S_OK;
    }, 0);
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
