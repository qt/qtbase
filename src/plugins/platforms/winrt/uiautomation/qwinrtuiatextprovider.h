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

#ifndef QWINRTUIATEXTPROVIDER_H
#define QWINRTUIATEXTPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiabaseprovider.h"

#include <wrl.h>
#include <windows.ui.xaml.h>

QT_BEGIN_NAMESPACE

// Implements the Text control pattern provider. Used for text controls.
class QWinRTUiaTextProvider :
        public QWinRTUiaBaseProvider,
        public Microsoft::WRL::RuntimeClass<ABI::Windows::UI::Xaml::Automation::Provider::ITextProvider, ABI::Windows::UI::Xaml::Automation::Provider::ITextProvider2>
{
    Q_OBJECT
    Q_DISABLE_COPY(QWinRTUiaTextProvider)
    InspectableClass(L"QWinRTUiaTextProvider", BaseTrust);

public:
    explicit QWinRTUiaTextProvider(QAccessible::Id id);
    virtual ~QWinRTUiaTextProvider();

    // ITextProvider
    HRESULT STDMETHODCALLTYPE get_DocumentRange(ABI::Windows::UI::Xaml::Automation::Provider::ITextRangeProvider **value) override;
    HRESULT STDMETHODCALLTYPE get_SupportedTextSelection(ABI::Windows::UI::Xaml::Automation::SupportedTextSelection *value) override;
    HRESULT STDMETHODCALLTYPE GetSelection(UINT32 *returnValueSize, ABI::Windows::UI::Xaml::Automation::Provider::ITextRangeProvider ***returnValue) override;
    HRESULT STDMETHODCALLTYPE GetVisibleRanges(UINT32 *returnValueSize, ABI::Windows::UI::Xaml::Automation::Provider::ITextRangeProvider ***returnValue) override;
    HRESULT STDMETHODCALLTYPE RangeFromChild(ABI::Windows::UI::Xaml::Automation::Provider::IIRawElementProviderSimple *childElement, ABI::Windows::UI::Xaml::Automation::Provider::ITextRangeProvider **returnValue) override;
    HRESULT STDMETHODCALLTYPE RangeFromPoint(ABI::Windows::Foundation::Point screenLocation, ABI::Windows::UI::Xaml::Automation::Provider::ITextRangeProvider **returnValue) override;

    // ITextProvider2
    HRESULT STDMETHODCALLTYPE RangeFromAnnotation(ABI::Windows::UI::Xaml::Automation::Provider::IIRawElementProviderSimple *annotationElement, ABI::Windows::UI::Xaml::Automation::Provider::ITextRangeProvider **returnValue) override;
    HRESULT STDMETHODCALLTYPE GetCaretRange(boolean *isActive, ABI::Windows::UI::Xaml::Automation::Provider::ITextRangeProvider **returnValue) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIATEXTPROVIDER_H
