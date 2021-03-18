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

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiatextrangeprovider.h"
#include "qwinrtuiametadatacache.h"
#include "qwinrtuiamainprovider.h"
#include "qwinrtuiautils.h"

#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/private/qeventdispatcher_winrt_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

using namespace QWinRTUiAutomation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::Xaml;
using namespace ABI::Windows::UI::Xaml::Automation;
using namespace ABI::Windows::UI::Xaml::Automation::Provider;
using namespace ABI::Windows::UI::Xaml::Automation::Text;

QWinRTUiaTextRangeProvider::QWinRTUiaTextRangeProvider(QAccessible::Id id, int startOffset, int endOffset) :
    QWinRTUiaBaseProvider(id),
    m_startOffset(startOffset),
    m_endOffset(endOffset)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << startOffset << endOffset;
}

QWinRTUiaTextRangeProvider::~QWinRTUiaTextRangeProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::Clone(ITextRangeProvider **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!returnValue)
        return E_INVALIDARG;

    ComPtr<QWinRTUiaTextRangeProvider> textRangeProvider = Make<QWinRTUiaTextRangeProvider>(id(), m_startOffset, m_endOffset);
    textRangeProvider.CopyTo(returnValue);
    return S_OK;
}

// Two ranges are considered equal if their start/end points are the same.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::Compare(ITextRangeProvider *textRangeProvider, boolean *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!textRangeProvider || !returnValue)
        return E_INVALIDARG;

    QWinRTUiaTextRangeProvider *targetProvider = static_cast<QWinRTUiaTextRangeProvider *>(textRangeProvider);
    *returnValue = ((targetProvider->m_startOffset == m_startOffset) && (targetProvider->m_endOffset == m_endOffset));
    return S_OK;
}

// Compare different endpoinds between two providers.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::CompareEndpoints(TextPatternRangeEndpoint endpoint, ITextRangeProvider *textRangeProvider, TextPatternRangeEndpoint targetEndpoint, INT32 *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!textRangeProvider || !returnValue)
        return E_INVALIDARG;

    QWinRTUiaTextRangeProvider *targetProvider = static_cast<QWinRTUiaTextRangeProvider *>(textRangeProvider);

    int point = (endpoint == TextPatternRangeEndpoint_Start) ? m_startOffset : m_endOffset;
    int targetPoint = (targetEndpoint == TextPatternRangeEndpoint_Start) ?
                targetProvider->m_startOffset : targetProvider->m_endOffset;
    *returnValue = point - targetPoint;
    return S_OK;
}

// Expands/normalizes the range for a given text unit.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::ExpandToEnclosingUnit(TextUnit unit)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << "unit=" << unit << "this: " << this;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());

    int len = metadata->characterCount();
    if (len < 1) {
        m_startOffset = 0;
        m_endOffset = 0;
    } else {
        if (unit == TextUnit_Character) {
            m_startOffset = qBound(0, m_startOffset, len - 1);
            m_endOffset = m_startOffset + 1;
        } else {
            QString text = metadata->text();
            for (int t = m_startOffset; t >= 0; --t) {
                if (!isTextUnitSeparator(unit, text[t]) && ((t == 0) || isTextUnitSeparator(unit, text[t - 1]))) {
                    m_startOffset = t;
                    break;
                }
            }
            for (int t = m_startOffset; t < len; ++t) {
                if ((t == len - 1) || (isTextUnitSeparator(unit, text[t]) && ((unit == TextUnit_Word) || !isTextUnitSeparator(unit, text[t + 1])))) {
                    m_endOffset = t + 1;
                    break;
                }
            }
        }
    }
    return S_OK;
}

// Not supported.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::FindAttribute(INT32 /*attributeId*/, IInspectable * /*value*/, boolean /*backward*/, ITextRangeProvider **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::FindText(HSTRING /*text*/, boolean /*backward*/, boolean /*ignoreCase*/, ITextRangeProvider **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;
    return S_OK;
}

// Returns the value of a given attribute.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::GetAttributeValue(INT32 attributeId, IInspectable **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << "attributeId=" << attributeId;

    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;

    ComPtr<IPropertyValueStatics> propertyValueStatics;
    if (FAILED(RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), IID_PPV_ARGS(&propertyValueStatics))))
        return E_FAIL;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());

    switch (attributeId) {
    case AutomationTextAttributesEnum_IsReadOnlyAttribute:
        return propertyValueStatics->CreateBoolean(metadata->state().readOnly, returnValue);
    case AutomationTextAttributesEnum_CaretPositionAttribute:
        if (metadata->cursorPosition() == 0)
            return propertyValueStatics->CreateInt32(AutomationCaretPosition_BeginningOfLine, returnValue);
        else if (metadata->cursorPosition() == metadata->characterCount())
            return propertyValueStatics->CreateInt32(AutomationCaretPosition_EndOfLine, returnValue);
        else
            return propertyValueStatics->CreateInt32(AutomationCaretPosition_Unknown, returnValue);
    default:
        break;
    }
    return E_FAIL;
}

// Returns an array of bounding rectangles for text lines within the range.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::GetBoundingRectangles(UINT32 *returnValueSize, DOUBLE **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValueSize || !returnValue)
        return E_INVALIDARG;
    *returnValueSize = 0;
    *returnValue = nullptr;

    auto accid = id();
    auto startOffset = m_startOffset;
    auto endOffset = m_endOffset;
    auto rects = std::make_shared<QVarLengthArray<QRect>>();

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([accid, startOffset, endOffset, rects]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            if (QAccessibleTextInterface *textInterface = accessible->textInterface()) {
                int len = textInterface->characterCount();
                if ((startOffset >= 0) && (endOffset <= len) && (startOffset < endOffset)) {
                    int start, end;
                    textInterface->textAtOffset(startOffset, QAccessible::LineBoundary, &start, &end);
                    while ((start >= 0) && (end >= 0)) {
                        int startRange = qMax(start, startOffset);
                        int endRange = qMin(end, endOffset);
                        if (startRange < endRange) {
                            // Calculates a bounding rectangle for the line and adds it to the list.
                            const QRect startRect = textInterface->characterRect(startRange);
                            const QRect endRect = textInterface->characterRect(endRange - 1);
                            const QRect lineRect(qMin(startRect.x(), endRect.x()),
                                                 qMin(startRect.y(), endRect.y()),
                                                 qMax(startRect.x() + startRect.width(), endRect.x() + endRect.width()) - qMin(startRect.x(), endRect.x()),
                                                 qMax(startRect.y() + startRect.height(), endRect.y() + endRect.height()) - qMin(startRect.y(), endRect.y()));
                            rects->append(lineRect);
                        }
                        if (end >= len) break;
                        textInterface->textAfterOffset(end + 1, QAccessible::LineBoundary, &start, &end);
                    }
                }
            }
        }
        return S_OK;
    }))) {
        return E_FAIL;
    }

    DOUBLE *doubleArray = static_cast<DOUBLE *>(CoTaskMemAlloc(4 * rects->size() * sizeof(DOUBLE)));
    if (!doubleArray)
        return E_OUTOFMEMORY;

    DOUBLE *dst = doubleArray;
    for (auto rect : *rects) {
        *dst++ = rect.left();
        *dst++ = rect.top();
        *dst++ = rect.width();
        *dst++ = rect.height();
    }
    *returnValue = doubleArray;
    *returnValueSize = 4 * rects->size();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::GetEnclosingElement(IIRawElementProviderSimple **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!returnValue)
        return E_INVALIDARG;
    return QWinRTUiaMainProvider::rawProviderForAccessibleId(id(), returnValue);
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::GetText(INT32 maxLength, HSTRING *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());

    QString rangeText = metadata->text().mid(m_startOffset, m_endOffset - m_startOffset);

    if ((maxLength > -1) && (rangeText.size() > maxLength))
        rangeText.truncate(maxLength);
    return qHString(rangeText, returnValue);
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::Move(TextUnit unit, INT32 count, INT32 *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = 0;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());

    int len = metadata->characterCount();
    if (len < 1)
        return S_OK;

    if (unit == TextUnit_Character) {
        // Moves the start point, ensuring it lies within the bounds.
        int start = qBound(0, m_startOffset + count, len - 1);
        // If range was initially empty, leaves it as is; otherwise, normalizes it to one char.
        m_endOffset = (m_endOffset > m_startOffset) ? start + 1 : start;
        *returnValue = start - m_startOffset; // Returns the actually moved distance.
        m_startOffset = start;
    } else {
        if (count > 0) {
            MoveEndpointByUnit(TextPatternRangeEndpoint_End, unit, count, returnValue);
            MoveEndpointByUnit(TextPatternRangeEndpoint_Start, unit, count, returnValue);
        } else {
            MoveEndpointByUnit(TextPatternRangeEndpoint_Start, unit, count, returnValue);
            MoveEndpointByUnit(TextPatternRangeEndpoint_End, unit, count, returnValue);
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::MoveEndpointByUnit(TextPatternRangeEndpoint endpoint, TextUnit unit, INT32 count, INT32 *returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = 0;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());

    int len = metadata->characterCount();
    if (len < 1)
        return S_OK;

    if (unit == TextUnit_Character) {
        if (endpoint == TextPatternRangeEndpoint_Start) {
            int boundedValue = qBound(0, m_startOffset + count, len - 1);
            *returnValue = boundedValue - m_startOffset;
            m_startOffset = boundedValue;
            m_endOffset = qBound(m_startOffset, m_endOffset, len);
        } else {
            int boundedValue = qBound(0, m_endOffset + count, len);
            *returnValue = boundedValue - m_endOffset;
            m_endOffset = boundedValue;
            m_startOffset = qBound(0, m_startOffset, m_endOffset);
        }
    } else {
        QString text = metadata->text();
        int moved = 0;

        if (endpoint == TextPatternRangeEndpoint_Start) {
            if (count > 0) {
                for (int t = m_startOffset; (t < len - 1) && (moved < count); ++t) {
                    if (isTextUnitSeparator(unit, text[t]) && !isTextUnitSeparator(unit, text[t + 1])) {
                        m_startOffset = t + 1;
                        ++moved;
                    }
                }
                m_endOffset = qBound(m_startOffset, m_endOffset, len);
            } else {
                for (int t = m_startOffset - 1; (t >= 0) && (moved > count); --t) {
                    if (!isTextUnitSeparator(unit, text[t]) && ((t == 0) || isTextUnitSeparator(unit, text[t - 1]))) {
                        m_startOffset = t;
                        --moved;
                    }
                }
            }
        } else {
            if (count > 0) {
                for (int t = m_endOffset; (t < len) && (moved < count); ++t) {
                    if ((t == len - 1) || (isTextUnitSeparator(unit, text[t]) && ((unit == TextUnit_Word) || !isTextUnitSeparator(unit, text[t + 1])))) {
                        m_endOffset = t + 1;
                        ++moved;
                    }
                }
            } else {
                int end = 0;
                for (int t = m_endOffset - 2; (t > 0) && (moved > count); --t) {
                    if (isTextUnitSeparator(unit, text[t]) && ((unit == TextUnit_Word) || !isTextUnitSeparator(unit, text[t + 1]))) {
                        end = t + 1;
                        --moved;
                    }
                }
                m_endOffset = end;
                m_startOffset = qBound(0, m_startOffset, m_endOffset);
            }
        }
        *returnValue = moved;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::MoveEndpointByRange(TextPatternRangeEndpoint endpoint, ITextRangeProvider *textRangeProvider, TextPatternRangeEndpoint targetEndpoint)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!textRangeProvider)
        return E_INVALIDARG;

    QWinRTUiaTextRangeProvider *targetProvider = static_cast<QWinRTUiaTextRangeProvider *>(textRangeProvider);

    int targetPoint = (targetEndpoint == TextPatternRangeEndpoint_Start) ?
                targetProvider->m_startOffset : targetProvider->m_endOffset;

    // If the moved endpoint crosses the other endpoint, that one is moved too.
    if (endpoint == TextPatternRangeEndpoint_Start) {
        m_startOffset = targetPoint;
        if (m_endOffset < m_startOffset)
            m_endOffset = m_startOffset;
    } else {
        m_endOffset = targetPoint;
        if (m_endOffset < m_startOffset)
            m_startOffset = m_endOffset;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::Select()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    auto accid = id();
    auto startOffset = m_startOffset;
    auto endOffset = m_endOffset;

    QEventDispatcherWinRT::runOnMainThread([accid, startOffset, endOffset]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid))
            if (QAccessibleTextInterface *textInterface = accessible->textInterface()) {
                // unselects all and adds a new selection
                for (int i = textInterface->selectionCount() - 1; i >= 0; --i)
                    textInterface->removeSelection(i);
                textInterface->addSelection(startOffset, endOffset);
            }
        QWinRTUiaMetadataCache::instance()->load(accid);
        return S_OK;
    }, 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::AddToSelection()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    return Select();
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::RemoveFromSelection()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    auto accid = id();

    QEventDispatcherWinRT::runOnMainThread([accid]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid))
            if (QAccessibleTextInterface *textInterface = accessible->textInterface()) {
                // unselects all
                for (int i = textInterface->selectionCount() - 1; i >= 0; --i)
                    textInterface->removeSelection(i);
            }
        QWinRTUiaMetadataCache::instance()->load(accid);
        return S_OK;
    }, 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::ScrollIntoView(boolean /*alignToTop*/)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    auto accid = id();
    auto startOffset = m_startOffset;
    auto endOffset = m_endOffset;

    QEventDispatcherWinRT::runOnMainThread([accid, startOffset, endOffset]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid))
            if (QAccessibleTextInterface *textInterface = accessible->textInterface()) {
                textInterface->scrollToSubstring(startOffset, endOffset);
            }
        QWinRTUiaMetadataCache::instance()->load(accid);
        return S_OK;
    }, 0);
    return S_OK;
}

// Returns an array of children elements embedded within the range.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextRangeProvider::GetChildren(UINT32 *returnValueSize, IIRawElementProviderSimple ***returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    // Not supporting any children.
    returnValueSize = 0;
    *returnValue = nullptr;
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
