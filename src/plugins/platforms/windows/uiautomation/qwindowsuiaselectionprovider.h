// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIASELECTIONPROVIDER_H
#define QWINDOWSUIASELECTIONPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

namespace QtPrivate {

template <>
struct QComObjectTraits<ISelectionProvider2>
{
    static constexpr bool isGuidOf(REFIID riid) noexcept
    {
        return QComObjectTraits<ISelectionProvider2, ISelectionProvider>::isGuidOf(riid);
    }
};

} // namespace QtPrivate

// Implements the Selection control pattern provider. Used for Lists.
class QWindowsUiaSelectionProvider : public QWindowsUiaBaseProvider,
                                     public QComObject<ISelectionProvider2>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaSelectionProvider)
public:
    explicit QWindowsUiaSelectionProvider(QAccessible::Id id);
    virtual ~QWindowsUiaSelectionProvider();

    // ISelectionProvider
    HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(BOOL *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(BOOL *pRetVal) override;

    // ISelectionProvider2
    HRESULT STDMETHODCALLTYPE get_FirstSelectedItem(__RPC__deref_out_opt IRawElementProviderSimple **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_LastSelectedItem(__RPC__deref_out_opt IRawElementProviderSimple **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_CurrentSelectedItem(__RPC__deref_out_opt IRawElementProviderSimple **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_ItemCount(__RPC__out int *pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIASELECTIONPROVIDER_H
