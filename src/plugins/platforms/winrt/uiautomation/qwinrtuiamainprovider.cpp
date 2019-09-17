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

#include "qwinrtuiamainprovider.h"
#include "qwinrtuiaprovidercache.h"
#include "qwinrtuiavalueprovider.h"
#include "qwinrtuiarangevalueprovider.h"
#include "qwinrtuiatextprovider.h"
#include "qwinrtuiatoggleprovider.h"
#include "qwinrtuiainvokeprovider.h"
#include "qwinrtuiaselectionprovider.h"
#include "qwinrtuiaselectionitemprovider.h"
#include "qwinrtuiatableprovider.h"
#include "qwinrtuiatableitemprovider.h"
#include "qwinrtuiagridprovider.h"
#include "qwinrtuiagriditemprovider.h"
#include "qwinrtuiapeervector.h"
#include "qwinrtuiametadatacache.h"
#include "qwinrtuiaemptypropertyvalue.h"
#include "qwinrtuiautils.h"

#include <QCoreApplication>
#include <QSemaphore>
#include <QtCore/QLoggingCategory>
#include <QtCore/qfunctions_winrt.h>
#include <QtCore/private/qeventdispatcher_winrt_p.h>

#include <memory>

using namespace QWinRTUiAutomation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::System;
using namespace ABI::Windows::UI;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::UI::Xaml;
using namespace ABI::Windows::UI::Xaml::Automation;
using namespace ABI::Windows::UI::Xaml::Automation::Provider;
using namespace ABI::Windows::UI::Xaml::Automation::Peers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;

QT_BEGIN_NAMESPACE

QWinRTUiaMainProvider::QWinRTUiaMainProvider(QAccessible::Id id)
    : QWinRTUiaBaseProvider(id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    ComPtr<IAutomationPeerFactory> factory;
    HRESULT hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Xaml_Automation_Peers_AutomationPeer).Get(), IID_PPV_ARGS(&factory));
    Q_ASSERT_SUCCEEDED(hr);

    hr = factory->CreateInstance(this, &m_base, &m_core);
    Q_ASSERT_SUCCEEDED(hr);
}

QWinRTUiaMainProvider::~QWinRTUiaMainProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::QueryInterface(REFIID iid, LPVOID *iface)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!iface)
        return E_POINTER;
    *iface = nullptr;

    if (iid == IID_IUnknown) {
        *iface = static_cast<IAutomationPeerOverrides *>(this);
        AddRef();
        return S_OK;
    } else if (iid == IID_IAutomationPeerOverrides) {
        *iface = static_cast<IAutomationPeerOverrides *>(this);
        AddRef();
        return S_OK;
    } else {
        return m_base.CopyTo(iid, iface);
    }
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetIids(ULONG *iidCount, IID **iids)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    *iidCount = 0;
    *iids = nullptr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetRuntimeClassName(HSTRING *className)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    return qHString(QStringLiteral("QWinRTUiaMainProvider"), className);
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetTrustLevel(TrustLevel *trustLevel)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    *trustLevel = TrustLevel::BaseTrust;
    return S_OK;
}

// Returns a cached instance of the provider for a specific accessible interface.
QWinRTUiaMainProvider *QWinRTUiaMainProvider::providerForAccessibleId(QAccessible::Id id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QWinRTUiaProviderCache *providerCache = QWinRTUiaProviderCache::instance();
    QWinRTUiaMainProvider *provider = qobject_cast<QWinRTUiaMainProvider *>(providerCache->providerForId(id));

    if (provider) {
        provider->AddRef();
    } else {
        ComPtr<QWinRTUiaMainProvider> p = Make<QWinRTUiaMainProvider>(id);
        provider = p.Get();
        provider->AddRef();
        providerCache->insert(id, provider);
    }
    return provider;
}

// Returns an IIRawElementProviderSimple for a specific accessible interface.
HRESULT QWinRTUiaMainProvider::rawProviderForAccessibleId(QAccessible::Id elementId,
                                                          IIRawElementProviderSimple **returnValue)
{
    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;

    if (ComPtr<QWinRTUiaMainProvider> provider = providerForAccessibleId(elementId)) {
        ComPtr<IAutomationPeer> automationPeer;
        if (SUCCEEDED(provider.As(&automationPeer))) {
            ComPtr<IAutomationPeerProtected> automationPeerProtected;
            if (SUCCEEDED(provider.As(&automationPeerProtected))) {
                return automationPeerProtected->ProviderFromPeer(automationPeer.Get(), returnValue);
            }
        }
    }
    return E_FAIL;
}

// Returns an array of IIRawElementProviderSimple instances for a list of accessible interface ids.
HRESULT QWinRTUiaMainProvider::rawProviderArrayForAccessibleIdList(const QVarLengthArray<QAccessible::Id> &elementIds,
                                                                   UINT32 *returnValueSize,
                                                                   IIRawElementProviderSimple ***returnValue)
{
    if (!returnValueSize || !returnValue)
        return E_INVALIDARG;
    *returnValueSize = 0;
    *returnValue = nullptr;

    QList<IIRawElementProviderSimple *> rawProviderList;

    for (auto elementId : elementIds) {
        IIRawElementProviderSimple *rawProvider;
        if (SUCCEEDED(rawProviderForAccessibleId(elementId, &rawProvider)))
            rawProviderList.append(rawProvider);
    }

    if (rawProviderList.size() == 0)
        return S_OK;

    *returnValue = static_cast<IIRawElementProviderSimple **>(CoTaskMemAlloc(rawProviderList.size() * sizeof(IIRawElementProviderSimple *)));
    if (!*returnValue) {
        for (auto rawProvider : qAsConst(rawProviderList))
            rawProvider->Release();
        return E_OUTOFMEMORY;
    }

    int index = 0;
    for (auto rawProvider : qAsConst(rawProviderList))
        (*returnValue)[index++] = rawProvider;
    *returnValueSize = rawProviderList.size();
    return S_OK;
}

void QWinRTUiaMainProvider::notifyFocusChange(QAccessibleEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        QAccessible::Id accid = idForAccessible(accessible);
        QWinRTUiaMetadataCache::instance()->load(accid);
        QEventDispatcherWinRT::runOnXamlThread([accid]() {
            if (ComPtr<QWinRTUiaMainProvider> provider = providerForAccessibleId(accid)) {
                ComPtr<IAutomationPeer> automationPeer;
                if (SUCCEEDED(provider->QueryInterface(IID_PPV_ARGS(&automationPeer)))) {
                    automationPeer->RaiseAutomationEvent(AutomationEvents_AutomationFocusChanged);
                }
            }
            return S_OK;
        }, false);
    }
}

void QWinRTUiaMainProvider::notifyVisibilityChange(QAccessibleEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        QAccessible::Id accid = idForAccessible(accessible);
        QWinRTUiaMetadataCache::instance()->load(accid);
    }
}

void QWinRTUiaMainProvider::notifyStateChange(QAccessibleStateChangeEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        QAccessible::Id accid = idForAccessible(accessible);
        QWinRTUiaMetadataCache::instance()->load(accid);

        if (event->changedStates().checked || event->changedStates().checkStateMixed) {
            // Notifies states changes in checkboxes.
            if (accessible->role() == QAccessible::CheckBox) {
                QEventDispatcherWinRT::runOnXamlThread([accid]() {
                    if (ComPtr<QWinRTUiaMainProvider> provider = providerForAccessibleId(accid)) {
                        ComPtr<IAutomationPeer> automationPeer;
                        if (SUCCEEDED(provider->QueryInterface(IID_PPV_ARGS(&automationPeer)))) {
                            ComPtr<ITogglePatternIdentifiersStatics> toggleStatics;
                            if (SUCCEEDED(RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Xaml_Automation_TogglePatternIdentifiers).Get(), IID_PPV_ARGS(&toggleStatics)))) {
                                ComPtr<IAutomationProperty> toggleStateProperty;
                                if (SUCCEEDED(toggleStatics->get_ToggleStateProperty(&toggleStateProperty))) {
                                    ComPtr<QWinRTUiaEmptyPropertyValue> emptyValue = Make<QWinRTUiaEmptyPropertyValue>();
                                    // by sending an event with an empty value we force ui automation to refresh its state
                                    automationPeer->RaisePropertyChangedEvent(toggleStateProperty.Get(), emptyValue.Get(), emptyValue.Get());
                                }
                            }
                        }
                    }
                    return S_OK;
                }, false);
            }
        }
        if (event->changedStates().active) {
            if (accessible->role() == QAccessible::Window) {
                // Notifies window opened/closed.
                bool active = accessible->state().active;
                QEventDispatcherWinRT::runOnXamlThread([accid, active]() {
                    if (ComPtr<QWinRTUiaMainProvider> provider = providerForAccessibleId(accid)) {
                        ComPtr<IAutomationPeer> automationPeer;
                        if (SUCCEEDED(provider->QueryInterface(IID_PPV_ARGS(&automationPeer)))) {
                            if (active) {
                                automationPeer->RaiseAutomationEvent(AutomationEvents_WindowOpened);
                            } else {
                                automationPeer->RaiseAutomationEvent(AutomationEvents_WindowClosed);
                            }
                        }
                    }
                    return S_OK;
                }, false);
            }
        }
    }
}

void QWinRTUiaMainProvider::notifyValueChange(QAccessibleValueChangeEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        QAccessible::Id accid = idForAccessible(accessible);
        QWinRTUiaMetadataCache::instance()->load(accid);
        if (QAccessibleValueInterface *valueInterface = accessible->valueInterface()) {
            // Notifies changes in values of controls supporting the value interface.
            double value = valueInterface->currentValue().toDouble();
            QEventDispatcherWinRT::runOnXamlThread([accid, value]() {
                // For some reason RaisePropertyChangedEvent() does not seem to be
                // forwarding notifications for any property types except empty,
                // which would do nothing here. ToDo: find a workaround.
                return S_OK;
            }, false);
        }
    }
}

// Notifies changes in text content and selection state of text controls.
void QWinRTUiaMainProvider::notifyTextChange(QAccessibleEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        QAccessible::Id accid = idForAccessible(accessible);
        QWinRTUiaMetadataCache::instance()->load(accid);
        bool readOnly = accessible->state().readOnly;
        QAccessible::Event eventType = event->type();
        if (accessible->textInterface()) {
            QEventDispatcherWinRT::runOnXamlThread([accid, eventType, readOnly]() {
                if (ComPtr<QWinRTUiaMainProvider> provider = providerForAccessibleId(accid)) {
                    ComPtr<IAutomationPeer> automationPeer;
                    if (SUCCEEDED(provider->QueryInterface(IID_PPV_ARGS(&automationPeer)))) {
                        if (eventType == QAccessible::TextSelectionChanged) {
                            automationPeer->RaiseAutomationEvent(AutomationEvents_TextPatternOnTextSelectionChanged);
                        } else if (eventType == QAccessible::TextCaretMoved) {
                            if (!readOnly) {
                                automationPeer->RaiseAutomationEvent(AutomationEvents_TextPatternOnTextSelectionChanged);
                            }
                        } else {
                            automationPeer->RaiseAutomationEvent(AutomationEvents_TextPatternOnTextChanged);
                        }
                    }
                }
                return S_OK;
            }, false);
        }
    }
}

// Return providers for specific control patterns
HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetPatternCore(PatternInterface patternInterface, IInspectable **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << patternInterface;

    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return E_FAIL;

    switch (patternInterface) {
    case PatternInterface_Text:
    case PatternInterface_Text2: {
        // All text controls.
        if (accessible->textInterface()) {
            ComPtr<QWinRTUiaTextProvider> provider = Make<QWinRTUiaTextProvider>(id());
            return provider.CopyTo(returnValue);
        }
        break;
    }
    case PatternInterface_Value: {
        // All accessible controls return text(QAccessible::Value) (which may be empty).
        ComPtr<QWinRTUiaValueProvider> provider = Make<QWinRTUiaValueProvider>(id());
        return provider.CopyTo(returnValue);
    }
    case PatternInterface_RangeValue: {
        // Controls providing a numeric value within a range (e.g., sliders, scroll bars, dials).
        if (accessible->valueInterface()) {
            ComPtr<QWinRTUiaRangeValueProvider> provider = Make<QWinRTUiaRangeValueProvider>(id());
            return provider.CopyTo(returnValue);
        }
        break;
    }
    case PatternInterface_Toggle: {
        // Checkbox controls.
        if (accessible->role() == QAccessible::CheckBox) {
            ComPtr<QWinRTUiaToggleProvider> provider = Make<QWinRTUiaToggleProvider>(id());
            return provider.CopyTo(returnValue);
        }
        break;
    }
    case PatternInterface_Selection: {
        // Lists of items.
        if (accessible->role() == QAccessible::List) {
            ComPtr<QWinRTUiaSelectionProvider> provider = Make<QWinRTUiaSelectionProvider>(id());
            return provider.CopyTo(returnValue);
        }
        break;
    }
    case PatternInterface_SelectionItem: {
        // Items within a list and radio buttons.
        if ((accessible->role() == QAccessible::RadioButton)
                || (accessible->role() == QAccessible::ListItem)) {
            ComPtr<QWinRTUiaSelectionItemProvider> provider = Make<QWinRTUiaSelectionItemProvider>(id());
            return provider.CopyTo(returnValue);
        }
        break;
    }
    case PatternInterface_Table: {
        // Table/tree.
        if (accessible->tableInterface()
                && ((accessible->role() == QAccessible::Table) || (accessible->role() == QAccessible::Tree))) {
            ComPtr<QWinRTUiaTableProvider> provider = Make<QWinRTUiaTableProvider>(id());
            return provider.CopyTo(returnValue);
        }
        break;
    }
    case PatternInterface_TableItem: {
        // Item within a table/tree.
        if (accessible->tableCellInterface()
                && ((accessible->role() == QAccessible::Cell) || (accessible->role() == QAccessible::TreeItem))) {
            ComPtr<QWinRTUiaTableItemProvider> provider = Make<QWinRTUiaTableItemProvider>(id());
            return provider.CopyTo(returnValue);
        }
        break;
    }
    case PatternInterface_Grid: {
        // Table/tree.
        if (accessible->tableInterface()
                && ((accessible->role() == QAccessible::Table) || (accessible->role() == QAccessible::Tree))) {
            ComPtr<QWinRTUiaGridProvider> provider = Make<QWinRTUiaGridProvider>(id());
            return provider.CopyTo(returnValue);
        }
        break;
    }
    case PatternInterface_GridItem: {
        // Item within a table/tree.
        if (accessible->tableCellInterface()
                && ((accessible->role() == QAccessible::Cell) || (accessible->role() == QAccessible::TreeItem))) {
            ComPtr<QWinRTUiaGridItemProvider> provider = Make<QWinRTUiaGridItemProvider>(id());
            return provider.CopyTo(returnValue);
        }
        break;
    }
    case PatternInterface_Invoke: {
        // Things that have an invokable action (e.g., simple buttons).
        if (accessible->actionInterface()) {
            ComPtr<QWinRTUiaInvokeProvider> provider = Make<QWinRTUiaInvokeProvider>(id());
            return provider.CopyTo(returnValue);
        }
        break;
    }
    default:
        break;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetAcceleratorKeyCore(HSTRING *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    return qHString(metadata->accelerator(), returnValue);
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetAccessKeyCore(HSTRING *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    return qHString(metadata->access(), returnValue);
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetAutomationControlTypeCore(AutomationControlType *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *returnValue = roleToControlType(metadata->role());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetAutomationIdCore(HSTRING *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    return qHString(metadata->automationId(), returnValue);
}

// Returns the bounding rectangle for the accessible control.
HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetBoundingRectangleCore(ABI::Windows::Foundation::Rect *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());

    QRect rect = metadata->boundingRect();
    returnValue->X = rect.x();
    returnValue->Y = rect.y();
    returnValue->Width = rect.width();
    returnValue->Height = rect.height();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetChildrenCore(IVector<AutomationPeer *> **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;

    auto accid = id();
    auto children = std::make_shared<QVarLengthArray<QAccessible::Id>>();

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([accid, children]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            int childCount = accessible->childCount();
            for (int i = 0; i < childCount; ++i) {
                if (QAccessibleInterface *childAcc = accessible->child(i)) {
                    QAccessible::Id childId = idForAccessible(childAcc);
                    QWinRTUiaMetadataCache::instance()->load(childId);
                    if (!childAcc->state().invisible)
                        children->append(childId);
                }
            }
        }
        return S_OK;
    }))) {
        return E_FAIL;
    }

    ComPtr<IVector<AutomationPeer *>> peerVector = Make<QWinRTUiaPeerVector>();

    for (auto childId : *children) {
        if (ComPtr<QWinRTUiaMainProvider> provider = providerForAccessibleId(childId)) {
            IAutomationPeer *peer;
            if (SUCCEEDED(provider.CopyTo(&peer)))
                peerVector->Append(peer);
        }
    }
    return peerVector.CopyTo(returnValue);
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetClassNameCore(HSTRING *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    return qHString(metadata->className(), returnValue);
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetClickablePointCore(ABI::Windows::Foundation::Point *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetHelpTextCore(HSTRING *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    return qHString(metadata->help(), returnValue);
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetItemStatusCore(HSTRING *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetItemTypeCore(HSTRING *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetLabeledByCore(IAutomationPeer **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetLocalizedControlTypeCore(HSTRING *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetNameCore(HSTRING *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    return qHString(metadata->controlName(), returnValue);
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetOrientationCore(AutomationOrientation *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = AutomationOrientation_None;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::HasKeyboardFocusCore(boolean *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *returnValue = (metadata->state().focused != 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::IsContentElementCore(boolean *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = true;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::IsControlElementCore(boolean *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = true;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::IsEnabledCore(boolean *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *returnValue = (metadata->state().disabled == 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::IsKeyboardFocusableCore(boolean *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *returnValue = (metadata->state().focusable != 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::IsOffscreenCore(boolean *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *returnValue = (metadata->state().offscreen != 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::IsPasswordCore(boolean *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *returnValue = (metadata->role() == QAccessible::EditableText) && (metadata->state().passwordEdit != 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::IsRequiredForFormCore(boolean *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = false;
    return S_OK;
}

// Sets focus to the control.
HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::SetFocusCore()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return E_FAIL;

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return E_FAIL;

    QEventDispatcherWinRT::runOnMainThread([actionInterface]() {
        actionInterface->doAction(QAccessibleActionInterface::setFocusAction());
        return S_OK;
    });
    return S_OK;
}

// Returns a provider for the UI element present at the specified screen coordinates.
HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetPeerFromPointCore(ABI::Windows::Foundation::Point point, IAutomationPeer **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;

    // Scale coordinates from High DPI screens?

    auto accid = id();
    auto elementId = std::make_shared<QAccessible::Id>(0);

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([accid, elementId, point]() {
        // Controls can be embedded within grouping elements. By default returns the innermost control.
        QAccessibleInterface *target = accessibleForId(accid);
        while (QAccessibleInterface *tmpacc = target->childAt(point.X, point.Y)) {
            target = tmpacc;
            // For accessibility tools it may be better to return the text element instead of its subcomponents.
            if (target->textInterface()) break;
        }
        *elementId = idForAccessible(target);
        QWinRTUiaMetadataCache::instance()->load(*elementId);
        return S_OK;
    }))) {
        return E_FAIL;
    }

    if (ComPtr<QWinRTUiaMainProvider> provider = providerForAccessibleId(*elementId))
        return provider.CopyTo(returnValue);
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaMainProvider::GetLiveSettingCore(AutomationLiveSetting *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    return E_NOTIMPL;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

