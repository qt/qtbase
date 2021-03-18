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

#include "qwinrtcanvas.h"
#include "uiautomation/qwinrtuiaaccessibility.h"
#include "uiautomation/qwinrtuiamainprovider.h"
#include "uiautomation/qwinrtuiametadatacache.h"
#include "uiautomation/qwinrtuiautils.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/qfunctions_winrt.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::System;
using namespace ABI::Windows::UI;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::UI::Xaml;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;

QT_BEGIN_NAMESPACE

QWinRTCanvas::QWinRTCanvas(const std::function<QWindow*()> &delegateWindow)
{
    ComPtr<Xaml::Controls::ICanvasFactory> factory;
    HRESULT hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Xaml_Controls_Canvas).Get(), IID_PPV_ARGS(&factory));
    Q_ASSERT_SUCCEEDED(hr);

    hr = factory->CreateInstance(this, &m_base, &m_core);
    Q_ASSERT_SUCCEEDED(hr);

    delegate = delegateWindow;
}

HRESULT STDMETHODCALLTYPE QWinRTCanvas::QueryInterface(REFIID iid, LPVOID *iface)
{
    if (!iface)
        return E_POINTER;
    *iface = nullptr;

    if (iid == IID_IUnknown) {
        *iface = static_cast<Xaml::IUIElementOverrides *>(this);
        AddRef();
        return S_OK;
    } else if (iid == Xaml::IID_IUIElementOverrides) {
        *iface = static_cast<Xaml::IUIElementOverrides *>(this);
        AddRef();
        return S_OK;
    } else {
        return m_base.CopyTo(iid, iface);
    }
}

HRESULT STDMETHODCALLTYPE QWinRTCanvas::GetIids(ULONG *iidCount, IID **iids)
{
    *iidCount = 0;
    *iids = nullptr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTCanvas::GetRuntimeClassName(HSTRING *className)
{
    const wchar_t *name = L"QWinRTCanvas";
    return ::WindowsCreateString(name, static_cast<UINT32>(::wcslen(name)), className);
}

HRESULT STDMETHODCALLTYPE QWinRTCanvas::GetTrustLevel(TrustLevel *trustLevel)
{
    *trustLevel = TrustLevel::BaseTrust;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTCanvas::OnCreateAutomationPeer(Xaml::Automation::Peers::IAutomationPeer **returnValue)
{
    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;

    if (delegate) {
        if (QWindow *window = delegate()) {
            QWinRTUiaAccessibility::activate();
            if (QAccessibleInterface *accessible = window->accessibleRoot()) {
                QAccessible::Id accid = QWinRTUiAutomation::idForAccessible(accessible);
                QWinRTUiaMetadataCache::instance()->load(accid);
                if (ComPtr<QWinRTUiaMainProvider> provider = QWinRTUiaMainProvider::providerForAccessibleId(accid))
                    return provider.CopyTo(returnValue);
            }
        }
    }
    return m_base->OnCreateAutomationPeer(returnValue);
}

HRESULT STDMETHODCALLTYPE QWinRTCanvas::OnDisconnectVisualChildren()
{
    return m_base->OnDisconnectVisualChildren();
}

HRESULT STDMETHODCALLTYPE QWinRTCanvas::FindSubElementsForTouchTargeting(Point point, Rect boundingRect, IIterable<IIterable<ABI::Windows::Foundation::Point>*> **returnValue)
{
    return m_base->FindSubElementsForTouchTargeting(point, boundingRect, returnValue);
}

QT_END_NAMESPACE

