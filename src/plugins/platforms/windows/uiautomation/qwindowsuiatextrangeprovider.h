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

#ifndef QWINDOWSUIATEXTRANGEPROVIDER_H
#define QWINDOWSUIATEXTRANGEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Text Range control pattern provider. Used for text controls.
class QWindowsUiaTextRangeProvider : public QWindowsUiaBaseProvider,
                                     public QWindowsComBase<ITextRangeProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaTextRangeProvider)
public:
    explicit QWindowsUiaTextRangeProvider(QAccessible::Id id, int startOffset, int endOffset);
    virtual ~QWindowsUiaTextRangeProvider();

    // ITextRangeProvider
    HRESULT STDMETHODCALLTYPE AddToSelection() override;
    HRESULT STDMETHODCALLTYPE Clone(ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE Compare(ITextRangeProvider *range, BOOL *pRetVal) override;
    HRESULT STDMETHODCALLTYPE CompareEndpoints(TextPatternRangeEndpoint endpoint, ITextRangeProvider *targetRange, TextPatternRangeEndpoint targetEndpoint, int *pRetVal) override;
    HRESULT STDMETHODCALLTYPE ExpandToEnclosingUnit(TextUnit unit) override;
    HRESULT STDMETHODCALLTYPE FindAttribute(TEXTATTRIBUTEID attributeId, VARIANT val, BOOL backward, ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE FindText(BSTR text, BOOL backward, BOOL ignoreCase, ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetAttributeValue(TEXTATTRIBUTEID attributeId, VARIANT *pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetBoundingRectangles(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetChildren(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetEnclosingElement(IRawElementProviderSimple **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetText(int maxLength, BSTR *pRetVal) override;
    HRESULT STDMETHODCALLTYPE Move(TextUnit unit, int count, int *pRetVal) override;
    HRESULT STDMETHODCALLTYPE MoveEndpointByRange(TextPatternRangeEndpoint endpoint, ITextRangeProvider *targetRange, TextPatternRangeEndpoint targetEndpoint) override;
    HRESULT STDMETHODCALLTYPE MoveEndpointByUnit(TextPatternRangeEndpoint endpoint, TextUnit unit, int count, int *pRetVal) override;
    HRESULT STDMETHODCALLTYPE RemoveFromSelection() override;
    HRESULT STDMETHODCALLTYPE ScrollIntoView(BOOL alignToTop) override;
    HRESULT STDMETHODCALLTYPE Select() override;

private:
    HRESULT unselect();
    int m_startOffset;
    int m_endOffset;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIATEXTRANGEPROVIDER_H
