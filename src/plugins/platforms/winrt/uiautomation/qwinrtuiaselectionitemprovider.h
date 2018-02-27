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

#ifndef QWINRTUIASELECTIONITEMPROVIDER_H
#define QWINRTUIASELECTIONITEMPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiabaseprovider.h"

#include <wrl.h>
#include <windows.ui.xaml.h>

QT_BEGIN_NAMESPACE

// Implements the Selection Item control pattern provider. Used for List items and radio buttons.
class QWinRTUiaSelectionItemProvider :
        public QWinRTUiaBaseProvider,
        public Microsoft::WRL::RuntimeClass<ABI::Windows::UI::Xaml::Automation::Provider::ISelectionItemProvider>
{
    Q_OBJECT
    Q_DISABLE_COPY(QWinRTUiaSelectionItemProvider)
    InspectableClass(L"QWinRTUiaSelectionItemProvider", BaseTrust);

public:
    explicit QWinRTUiaSelectionItemProvider(QAccessible::Id id);
    virtual ~QWinRTUiaSelectionItemProvider();

    // ISelectionItemProvider
    HRESULT STDMETHODCALLTYPE get_IsSelected(boolean *value) override;
    HRESULT STDMETHODCALLTYPE get_SelectionContainer(ABI::Windows::UI::Xaml::Automation::Provider::IIRawElementProviderSimple **value) override;
    HRESULT STDMETHODCALLTYPE AddToSelection() override;
    HRESULT STDMETHODCALLTYPE RemoveFromSelection() override;
    HRESULT STDMETHODCALLTYPE Select() override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIASELECTIONITEMPROVIDER_H
