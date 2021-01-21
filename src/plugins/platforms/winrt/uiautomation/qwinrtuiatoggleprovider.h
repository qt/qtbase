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

#ifndef QWINRTUIATOGGLEPROVIDER_H
#define QWINRTUIATOGGLEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiabaseprovider.h"

#include <wrl.h>
#include <windows.ui.xaml.h>

QT_BEGIN_NAMESPACE

// Implements the Toggle control pattern provider. Used for checkboxes.
class QWinRTUiaToggleProvider :
        public QWinRTUiaBaseProvider,
        public Microsoft::WRL::RuntimeClass<ABI::Windows::UI::Xaml::Automation::Provider::IToggleProvider>
{
    Q_OBJECT
    Q_DISABLE_COPY(QWinRTUiaToggleProvider)
    InspectableClass(L"QWinRTUiaToggleProvider", BaseTrust);

public:
    explicit QWinRTUiaToggleProvider(QAccessible::Id id);
    virtual ~QWinRTUiaToggleProvider();

    // IToggleProvider
    HRESULT STDMETHODCALLTYPE get_ToggleState(ABI::Windows::UI::Xaml::Automation::ToggleState *value) override;
    HRESULT STDMETHODCALLTYPE Toggle() override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIATOGGLEPROVIDER_H
