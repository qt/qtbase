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
