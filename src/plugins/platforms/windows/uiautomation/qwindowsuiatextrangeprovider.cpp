// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiatextrangeprovider.h"
#include "qwindowsuiamainprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/private/qcomvariant_p.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaTextRangeProvider::QWindowsUiaTextRangeProvider(QAccessible::Id id, int startOffset, int endOffset) :
    QWindowsUiaBaseProvider(id),
    m_startOffset(startOffset),
    m_endOffset(endOffset)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this << startOffset << endOffset;
}

QWindowsUiaTextRangeProvider::~QWindowsUiaTextRangeProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;
}

HRESULT QWindowsUiaTextRangeProvider::AddToSelection()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;
    return Select();
}

HRESULT QWindowsUiaTextRangeProvider::Clone(ITextRangeProvider **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;

    *pRetVal = makeComObject<QWindowsUiaTextRangeProvider>(id(), m_startOffset, m_endOffset).Detach();
    return S_OK;
}

// Two ranges are considered equal if their start/end points are the same.
HRESULT QWindowsUiaTextRangeProvider::Compare(ITextRangeProvider *range, BOOL *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!range || !pRetVal)
        return E_INVALIDARG;

    auto *targetProvider = static_cast<QWindowsUiaTextRangeProvider *>(range);
    *pRetVal = ((targetProvider->m_startOffset == m_startOffset) && (targetProvider->m_endOffset == m_endOffset));
    return S_OK;
}

// Compare different endpoinds between two providers.
HRESULT QWindowsUiaTextRangeProvider::CompareEndpoints(TextPatternRangeEndpoint endpoint,
                                                       ITextRangeProvider *targetRange,
                                                       TextPatternRangeEndpoint targetEndpoint,
                                                       int *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__
        << "endpoint=" << endpoint << "targetRange=" << targetRange
        << "targetEndpoint=" << targetEndpoint << "this: " << this;

    if (!targetRange || !pRetVal)
        return E_INVALIDARG;

    auto *targetProvider = static_cast<QWindowsUiaTextRangeProvider *>(targetRange);

    int point = (endpoint == TextPatternRangeEndpoint_Start) ? m_startOffset : m_endOffset;
    int targetPoint = (targetEndpoint == TextPatternRangeEndpoint_Start) ?
                targetProvider->m_startOffset : targetProvider->m_endOffset;
    *pRetVal = point - targetPoint;
    return S_OK;
}

// Expands/normalizes the range for a given text unit.
HRESULT QWindowsUiaTextRangeProvider::ExpandToEnclosingUnit(TextUnit unit)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << "unit=" << unit << "this: " << this;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    int len = textInterface->characterCount();
    if (len < 1) {
        m_startOffset = 0;
        m_endOffset = 0;
    } else {
        if (unit == TextUnit_Character) {
            m_startOffset = qBound(0, m_startOffset, len - 1);
            m_endOffset = m_startOffset + 1;
        } else {
            QString text = textInterface->text(0, len);
            const int start = m_startOffset >= 0 && m_startOffset < len
                              ? m_startOffset : len - 1;
            for (int t = start; t >= 0; --t) {
                if (!text.isEmpty() && !isTextUnitSeparator(unit, text[t]) && ((t == 0) || isTextUnitSeparator(unit, text[t - 1]))) {
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
HRESULT QWindowsUiaTextRangeProvider::FindAttribute(TEXTATTRIBUTEID /* attributeId */,
                                                    VARIANT /* val */, BOOL /* backward */,
                                                    ITextRangeProvider **pRetVal)
{
    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;
    return S_OK;
}

// Returns the value of a given attribute.
HRESULT STDMETHODCALLTYPE QWindowsUiaTextRangeProvider::GetAttributeValue(TEXTATTRIBUTEID attributeId,
                                                                          VARIANT *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << "attributeId=" << attributeId << "this: " << this;

    if (!pRetVal)
        return E_INVALIDARG;
    clearVariant(pRetVal);

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    switch (attributeId) {
    case UIA_IsReadOnlyAttributeId:
        *pRetVal = QComVariant{ accessible->state().readOnly ? true : false }.release();
        break;
    case UIA_CaretPositionAttributeId:
        if (textInterface->cursorPosition() == 0)
            *pRetVal = QComVariant{ static_cast<long>(CaretPosition_BeginningOfLine) }.release();
        else if (textInterface->cursorPosition() == textInterface->characterCount())
            *pRetVal = QComVariant{ static_cast<long>(CaretPosition_EndOfLine) }.release();
        else
            *pRetVal = QComVariant{ static_cast<long>(CaretPosition_Unknown) }.release();
        break;
    case UIA_StrikethroughStyleAttributeId:
    {
        const QString value = valueForIA2Attribute(textInterface, QStringLiteral("text-line-through-type"));
        if (value.isEmpty())
            break;
        const TextDecorationLineStyle uiaLineStyle = uiaLineStyleForIA2LineStyle(value);
        *pRetVal = QComVariant{ static_cast<long>(uiaLineStyle) }.release();
        break;
    }
    default:
        break;
    }
    return S_OK;
}

// Returns an array of bounding rectangles for text lines within the range.
HRESULT QWindowsUiaTextRangeProvider::GetBoundingRectangles(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QWindow *window = windowForAccessible(accessible);
    if (!window)
        return UIA_E_ELEMENTNOTAVAILABLE;

    int len = textInterface->characterCount();
    QVarLengthArray<QRect> rectList;

    if ((m_startOffset >= 0) && (m_endOffset <= len) && (m_startOffset < m_endOffset)) {
        int start, end;
        textInterface->textAtOffset(m_startOffset, QAccessible::LineBoundary, &start, &end);
        while ((start >= 0) && (end >= 0)) {
            int startRange = qMax(start, m_startOffset);
            int endRange = qMin(end, m_endOffset);
            if (startRange < endRange) {
                // Calculates a bounding rectangle for the line and adds it to the list.
                const QRect startRect = textInterface->characterRect(startRange);
                const QRect endRect = textInterface->characterRect(endRange - 1);
                const QRect lineRect(qMin(startRect.x(), endRect.x()),
                                     qMin(startRect.y(), endRect.y()),
                                     qMax(startRect.x() + startRect.width(), endRect.x() + endRect.width()) - qMin(startRect.x(), endRect.x()),
                                     qMax(startRect.y() + startRect.height(), endRect.y() + endRect.height()) - qMin(startRect.y(), endRect.y()));
                rectList.append(lineRect);
            }
            if (end >= len) break;
            textInterface->textAfterOffset(end + 1, QAccessible::LineBoundary, &start, &end);
        }
    }

    if ((*pRetVal = SafeArrayCreateVector(VT_R8, 0, 4 * rectList.size()))) {
        for (int i = 0; i < rectList.size(); ++i) {
            // Scale rect for high DPI screens.
            UiaRect uiaRect;
            rectToNativeUiaRect(rectList[i], window, &uiaRect);
            double coords[4] = { uiaRect.left, uiaRect.top, uiaRect.width, uiaRect.height };
            for (int j = 0; j < 4; ++j) {
                LONG idx = 4 * i + j;
                SafeArrayPutElement(*pRetVal, &idx, &coords[j]);
            }
        }
    }
    return S_OK;
}

// Returns an array of children elements embedded within the range.
HRESULT QWindowsUiaTextRangeProvider::GetChildren(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    // Not supporting any children.
    *pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, 0);
    return S_OK;
}

// Returns a provider for the enclosing element (text to which the range belongs).
HRESULT QWindowsUiaTextRangeProvider::GetEnclosingElement(IRawElementProviderSimple **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = QWindowsUiaMainProvider::providerForAccessible(accessible).Detach();
    return S_OK;
}

// Gets the text within the range.
HRESULT QWindowsUiaTextRangeProvider::GetText(int maxLength, BSTR *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << maxLength << "this: " << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    int len = textInterface->characterCount();
    QString rangeText;
    if ((m_startOffset >= 0) && (m_endOffset <= len) && (m_startOffset < m_endOffset))
        rangeText = textInterface->text(m_startOffset, m_endOffset);

    if ((maxLength > -1) && (rangeText.size() > maxLength))
        rangeText.truncate(maxLength);
    *pRetVal = QBStr(rangeText).release();
    return S_OK;
}

// Moves the range a specified number of units (and normalizes it).
HRESULT QWindowsUiaTextRangeProvider::Move(TextUnit unit, int count, int *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << "unit=" << unit << "count=" << count << "this: " << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = 0;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    int len = textInterface->characterCount();

    if (len < 1 || count == 0)  // MSDN: "Zero has no effect."
        return S_OK;

    if (unit == TextUnit_Character) {
        // Moves the start point, ensuring it lies within the bounds.
        int start = qBound(0, m_startOffset + count, len);
        // If range was initially empty, leaves it as is; otherwise, normalizes it to one char.
        m_endOffset = (m_endOffset > m_startOffset) ? qMin(start + 1, len) : start;
        *pRetVal = start - m_startOffset; // Returns the actually moved distance.
        m_startOffset = start;
    } else {
        if (count > 0) {
            MoveEndpointByUnit(TextPatternRangeEndpoint_End, unit, count, pRetVal);
            MoveEndpointByUnit(TextPatternRangeEndpoint_Start, unit, count, pRetVal);
        } else {
            MoveEndpointByUnit(TextPatternRangeEndpoint_Start, unit, count, pRetVal);
            MoveEndpointByUnit(TextPatternRangeEndpoint_End, unit, count, pRetVal);
        }
    }
    return S_OK;
}

// Copies the value of an end point from one range to another.
HRESULT QWindowsUiaTextRangeProvider::MoveEndpointByRange(TextPatternRangeEndpoint endpoint,
                                                          ITextRangeProvider *targetRange,
                                                          TextPatternRangeEndpoint targetEndpoint)
{
    if (!targetRange)
        return E_INVALIDARG;

    qCDebug(lcQpaUiAutomation) << __FUNCTION__
        << "endpoint=" << endpoint << "targetRange=" << targetRange << "targetEndpoint=" << targetEndpoint << "this: " << this;

    auto *targetProvider = static_cast<QWindowsUiaTextRangeProvider *>(targetRange);

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

// Moves an endpoint an specific number of units.
HRESULT QWindowsUiaTextRangeProvider::MoveEndpointByUnit(TextPatternRangeEndpoint endpoint,
                                                         TextUnit unit, int count,
                                                         int *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__
        << "endpoint=" << endpoint << "unit=" << unit << "count=" << count << "this: " << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = 0;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    int len = textInterface->characterCount();

    if (len < 1)
        return S_OK;

    if (unit == TextUnit_Character) {
        if (endpoint == TextPatternRangeEndpoint_Start) {
            int boundedValue = qBound(0, m_startOffset + count, len);
            *pRetVal = boundedValue - m_startOffset;
            m_startOffset = boundedValue;
            m_endOffset = qBound(m_startOffset, m_endOffset, len);
        } else {
            int boundedValue = qBound(0, m_endOffset + count, len);
            *pRetVal = boundedValue - m_endOffset;
            m_endOffset = boundedValue;
            m_startOffset = qBound(0, m_startOffset, m_endOffset);
        }
    } else {
        QString text = textInterface->text(0, len);
        int moved = 0;

        if (endpoint == TextPatternRangeEndpoint_Start) {
            if (count > 0) {
                for (int t = m_startOffset; (t < len - 1) && (moved < count); ++t) {
                    if (!text.isEmpty() && isTextUnitSeparator(unit, text[t]) && !isTextUnitSeparator(unit, text[t + 1])) {
                        m_startOffset = t + 1;
                        ++moved;
                    }
                }
                m_endOffset = qBound(m_startOffset, m_endOffset, len);
            } else {
                const int start = m_startOffset >= 0 && m_startOffset <= len
                                  ? m_startOffset : len;
                for (int t = start - 1; (t >= 0) && (moved > count); --t) {
                    if (!text.isEmpty() &&!isTextUnitSeparator(unit, text[t]) && ((t == 0) || isTextUnitSeparator(unit, text[t - 1]))) {
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
        *pRetVal = moved;
    }
    return S_OK;
}

HRESULT QWindowsUiaTextRangeProvider::RemoveFromSelection()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    // unselects all
    return unselect();
}

// Scrolls the range into view.
HRESULT QWindowsUiaTextRangeProvider::ScrollIntoView(BOOL alignToTop)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << "alignToTop=" << alignToTop << "this: " << this;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    textInterface->scrollToSubstring(m_startOffset, m_endOffset);
    return S_OK;
}

// Selects the range.
HRESULT QWindowsUiaTextRangeProvider::Select()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    // unselects all and adds a new selection
    unselect();
    textInterface->addSelection(m_startOffset, m_endOffset);
    return S_OK;
}

// Not supported.
HRESULT QWindowsUiaTextRangeProvider::FindText(BSTR /* text */, BOOL /* backward */,
                                               BOOL /* ignoreCase */,
                                               ITextRangeProvider **pRetVal)
{
    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;
    return S_OK;
}

// Removes all selected ranges from the text element.
HRESULT QWindowsUiaTextRangeProvider::unselect()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    int selCount = textInterface->selectionCount();

    for (int i = selCount - 1; i >= 0; --i)
        textInterface->removeSelection(i);
    return S_OK;
}

// helper method to retrieve the value of the given IAccessible2 text attribute,
// or an empty string if not set
QString QWindowsUiaTextRangeProvider::valueForIA2Attribute(QAccessibleTextInterface *textInterface,
                                                           const QString &key)
{
    Q_ASSERT(textInterface);

    int startOffset;
    int endOffset;
    const QString attributes = textInterface->attributes(m_startOffset, &startOffset, &endOffset);
    // don't report if attributes don't apply for the whole range
    if (startOffset > m_startOffset || endOffset < m_endOffset)
        return {};

    for (auto attr : QStringTokenizer{attributes, u';'})
    {
        const QList<QStringView> items = attr.split(u':', Qt::SkipEmptyParts, Qt::CaseSensitive);
        if (items.count() == 2 && items[0] == key)
            return items[1].toString();
    }

    return {};
}

TextDecorationLineStyle QWindowsUiaTextRangeProvider::uiaLineStyleForIA2LineStyle(const QString &ia2LineStyle)
{
    if (ia2LineStyle == QStringLiteral("none"))
        return TextDecorationLineStyle_None;
    if (ia2LineStyle == QStringLiteral("single"))
        return TextDecorationLineStyle_Single;
    if (ia2LineStyle == QStringLiteral("double"))
        return TextDecorationLineStyle_Double;

    return TextDecorationLineStyle_Other;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
