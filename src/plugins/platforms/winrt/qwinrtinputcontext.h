/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINRTINPUTCONTEXT_H
#define QWINRTINPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>
#include <QtCore/QRectF>

#include <wrl.h>
#ifndef Q_OS_WINPHONE
#  include <UIAutomationCore.h>
#endif

namespace ABI {
    namespace Windows {
        namespace UI {
            namespace Core {
                struct ICoreWindow;
            }
            namespace ViewManagement {
                struct IInputPane;
                struct IInputPaneVisibilityEventArgs;
            }
        }
    }
}

QT_BEGIN_NAMESPACE

class QWinRTScreen;
class QWinRTInputContext : public QPlatformInputContext
#ifndef Q_OS_WINPHONE
                         , public Microsoft::WRL::RuntimeClass<
                           Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
                           IRawElementProviderSimple, ITextProvider, IValueProvider>
#endif // !Q_OS_WINPHONE
{
public:
    explicit QWinRTInputContext(QWinRTScreen *);

    QRectF keyboardRect() const;

    bool isInputPanelVisible() const;

#ifdef Q_OS_WINPHONE
    void showInputPanel();
    void hideInputPanel();
#else // Q_OS_WINPHONE
    // IRawElementProviderSimple
    HRESULT __stdcall get_ProviderOptions(ProviderOptions *retVal);
    HRESULT __stdcall GetPatternProvider(PATTERNID, IUnknown **);
    HRESULT __stdcall GetPropertyValue(PROPERTYID idProp, VARIANT *retVal);
    HRESULT __stdcall get_HostRawElementProvider(IRawElementProviderSimple **retVal);

    // ITextProvider
    HRESULT __stdcall GetSelection(SAFEARRAY **);
    HRESULT __stdcall GetVisibleRanges(SAFEARRAY **);
    HRESULT __stdcall RangeFromChild(IRawElementProviderSimple *,ITextRangeProvider **);
    HRESULT __stdcall RangeFromPoint(UiaPoint, ITextRangeProvider **);
    HRESULT __stdcall get_DocumentRange(ITextRangeProvider **);
    HRESULT __stdcall get_SupportedTextSelection(SupportedTextSelection *);

    // IValueProvider
    HRESULT __stdcall SetValue(LPCWSTR);
    HRESULT __stdcall get_Value(BSTR *);
    HRESULT __stdcall get_IsReadOnly(BOOL *);
#endif // !Q_OS_WINPHONE

private:
    HRESULT onShowing(ABI::Windows::UI::ViewManagement::IInputPane *,
                      ABI::Windows::UI::ViewManagement::IInputPaneVisibilityEventArgs *);
    HRESULT onHiding(ABI::Windows::UI::ViewManagement::IInputPane *,
                     ABI::Windows::UI::ViewManagement::IInputPaneVisibilityEventArgs *);

    HRESULT handleVisibilityChange(ABI::Windows::UI::ViewManagement::IInputPane *);

    QWinRTScreen *m_screen;
    QRectF m_keyboardRect;
    bool m_isInputPanelVisible;
};

QT_END_NAMESPACE

#endif // QWINRTINPUTCONTEXT_H
