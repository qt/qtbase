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

#ifndef QWINRTUIAMAINPROVIDER_H
#define QWINRTUIAMAINPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiabaseprovider.h"

#include <QtCore/qglobal.h>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>

#include <wrl.h>
#include <windows.ui.xaml.h>

QT_BEGIN_NAMESPACE

// The main WinRT UI Automation class.
class QWinRTUiaMainProvider:
        public QWinRTUiaBaseProvider,
        public Microsoft::WRL::RuntimeClass<ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides>
{
    Q_OBJECT
    Q_DISABLE_COPY(QWinRTUiaMainProvider)

public:
    explicit QWinRTUiaMainProvider(QAccessible::Id id);
    virtual ~QWinRTUiaMainProvider();
    static QWinRTUiaMainProvider *providerForAccessibleId(QAccessible::Id id);
    static HRESULT rawProviderForAccessibleId(QAccessible::Id elementId, ABI::Windows::UI::Xaml::Automation::Provider::IIRawElementProviderSimple **returnValue);
    static HRESULT rawProviderArrayForAccessibleIdList(const QVarLengthArray<QAccessible::Id> &elementIds, UINT32 *returnValueSize, ABI::Windows::UI::Xaml::Automation::Provider::IIRawElementProviderSimple ***returnValue);
    static void notifyFocusChange(QAccessibleEvent *event);
    static void notifyVisibilityChange(QAccessibleEvent *event);
    static void notifyStateChange(QAccessibleStateChangeEvent *event);
    static void notifyValueChange(QAccessibleValueChangeEvent *event);
    static void notifyTextChange(QAccessibleEvent *event);

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, LPVOID *iface) override;

    // IInspectable
    HRESULT STDMETHODCALLTYPE GetIids(ULONG *iidCount, IID **iids) override;
    HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING *className) override;
    HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel *trustLevel) override;

    // IAutomationPeerOverrides
    HRESULT STDMETHODCALLTYPE GetPatternCore(ABI::Windows::UI::Xaml::Automation::Peers::PatternInterface patternInterface, IInspectable **returnValue) override;
    HRESULT STDMETHODCALLTYPE GetAcceleratorKeyCore(HSTRING *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetAccessKeyCore(HSTRING *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetAutomationControlTypeCore(ABI::Windows::UI::Xaml::Automation::Peers::AutomationControlType *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetAutomationIdCore(HSTRING *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetBoundingRectangleCore(ABI::Windows::Foundation::Rect *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetChildrenCore(ABI::Windows::Foundation::Collections::IVector<ABI::Windows::UI::Xaml::Automation::Peers::AutomationPeer*> **returnValue) override;
    HRESULT STDMETHODCALLTYPE GetClassNameCore(HSTRING *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetClickablePointCore(ABI::Windows::Foundation::Point *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetHelpTextCore(HSTRING *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetItemStatusCore(HSTRING *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetItemTypeCore(HSTRING *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetLabeledByCore(ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeer **returnValue) override;
    HRESULT STDMETHODCALLTYPE GetLocalizedControlTypeCore(HSTRING *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetNameCore(HSTRING *returnValue) override;
    HRESULT STDMETHODCALLTYPE GetOrientationCore(ABI::Windows::UI::Xaml::Automation::Peers::AutomationOrientation *returnValue) override;
    HRESULT STDMETHODCALLTYPE HasKeyboardFocusCore(boolean *returnValue) override;
    HRESULT STDMETHODCALLTYPE IsContentElementCore(boolean *returnValue) override;
    HRESULT STDMETHODCALLTYPE IsControlElementCore(boolean *returnValue) override;
    HRESULT STDMETHODCALLTYPE IsEnabledCore(boolean *returnValue) override;
    HRESULT STDMETHODCALLTYPE IsKeyboardFocusableCore(boolean *returnValue) override;
    HRESULT STDMETHODCALLTYPE IsOffscreenCore(boolean *returnValue) override;
    HRESULT STDMETHODCALLTYPE IsPasswordCore(boolean *returnValue) override;
    HRESULT STDMETHODCALLTYPE IsRequiredForFormCore(boolean *returnValue) override;
    HRESULT STDMETHODCALLTYPE SetFocusCore() override;
    HRESULT STDMETHODCALLTYPE GetPeerFromPointCore(ABI::Windows::Foundation::Point point, ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeer **returnValue) override;
    HRESULT STDMETHODCALLTYPE GetLiveSettingCore(ABI::Windows::UI::Xaml::Automation::Peers::AutomationLiveSetting *returnValue) override;

private:
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides> m_base;
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeer> m_core;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIAMAINPROVIDER_H
