// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIATEXTPROVIDER_H
#define QWINDOWSUIATEXTPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"
#include "qwindowsuiatextrangeprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Text control pattern provider. Used for text controls.
class QWindowsUiaTextProvider : public QWindowsUiaBaseProvider,
                                public QWindowsComBase<ITextProvider2>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaTextProvider)
public:
    explicit QWindowsUiaTextProvider(QAccessible::Id id);
    ~QWindowsUiaTextProvider();

    // IUnknown overrides
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, LPVOID *iface) override;

    // ITextProvider
    HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetVisibleRanges(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE RangeFromChild(IRawElementProviderSimple *childElement, ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE RangeFromPoint(UiaPoint point, ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_DocumentRange(ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_SupportedTextSelection(SupportedTextSelection *pRetVal) override;

    // ITextProvider2
    HRESULT STDMETHODCALLTYPE RangeFromAnnotation(IRawElementProviderSimple *annotationElement, ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetCaretRange(BOOL *isActive, ITextRangeProvider **pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIATEXTPROVIDER_H
