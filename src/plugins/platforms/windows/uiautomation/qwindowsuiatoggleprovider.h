// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QWINDOWSUIATOGGLEPROVIDER_H
#define QWINDOWSUIATOGGLEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Toggle control pattern provider. Used for checkboxes.
class QWindowsUiaToggleProvider : public QWindowsUiaBaseProvider,
                                  public QWindowsComBase<IToggleProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaToggleProvider)
public:
    explicit QWindowsUiaToggleProvider(QAccessible::Id id);
    virtual ~QWindowsUiaToggleProvider();

    // IToggleProvider
    HRESULT STDMETHODCALLTYPE Toggle() override;
    HRESULT STDMETHODCALLTYPE get_ToggleState(ToggleState *pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIATOGGLEPROVIDER_H
