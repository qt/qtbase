/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QWINDOWSUIATEXTRANGEPROVIDER_H
#define QWINDOWSUIATEXTRANGEPROVIDER_H

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Text Range control pattern provider. Used for text controls.
class QWindowsUiaTextRangeProvider : public QWindowsUiaBaseProvider,
                                     public QWindowsComBase<ITextRangeProvider>
{
    Q_DISABLE_COPY(QWindowsUiaTextRangeProvider)
public:
    explicit QWindowsUiaTextRangeProvider(QAccessible::Id id, int startOffset, int endOffset);
    virtual ~QWindowsUiaTextRangeProvider();

    HRESULT STDMETHODCALLTYPE AddToSelection();
    HRESULT STDMETHODCALLTYPE Clone(ITextRangeProvider **pRetVal);
    HRESULT STDMETHODCALLTYPE Compare(ITextRangeProvider *range, BOOL *pRetVal);
    HRESULT STDMETHODCALLTYPE CompareEndpoints(TextPatternRangeEndpoint endpoint, ITextRangeProvider *targetRange, TextPatternRangeEndpoint targetEndpoint, int *pRetVal);
    HRESULT STDMETHODCALLTYPE ExpandToEnclosingUnit(TextUnit unit);
    HRESULT STDMETHODCALLTYPE FindAttribute(TEXTATTRIBUTEID attributeId, VARIANT val, BOOL backward, ITextRangeProvider **pRetVal);
    HRESULT STDMETHODCALLTYPE FindText(BSTR text, BOOL backward, BOOL ignoreCase, ITextRangeProvider **pRetVal);
    HRESULT STDMETHODCALLTYPE GetAttributeValue(TEXTATTRIBUTEID attributeId, VARIANT *pRetVal);
    HRESULT STDMETHODCALLTYPE GetBoundingRectangles(SAFEARRAY **pRetVal);
    HRESULT STDMETHODCALLTYPE GetChildren(SAFEARRAY **pRetVal);
    HRESULT STDMETHODCALLTYPE GetEnclosingElement(IRawElementProviderSimple **pRetVal);
    HRESULT STDMETHODCALLTYPE GetText(int maxLength, BSTR *pRetVal);
    HRESULT STDMETHODCALLTYPE Move(TextUnit unit, int count, int *pRetVal);
    HRESULT STDMETHODCALLTYPE MoveEndpointByRange(TextPatternRangeEndpoint endpoint, ITextRangeProvider *targetRange, TextPatternRangeEndpoint targetEndpoint);
    HRESULT STDMETHODCALLTYPE MoveEndpointByUnit(TextPatternRangeEndpoint endpoint, TextUnit unit, int count, int *pRetVal);
    HRESULT STDMETHODCALLTYPE RemoveFromSelection();
    HRESULT STDMETHODCALLTYPE ScrollIntoView(BOOL alignToTop);
    HRESULT STDMETHODCALLTYPE Select();

private:
    HRESULT unselect();
    int m_startOffset;
    int m_endOffset;
};

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY

#endif // QWINDOWSUIATEXTRANGEPROVIDER_H
