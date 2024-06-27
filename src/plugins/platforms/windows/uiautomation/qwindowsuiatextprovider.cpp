// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiatextprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qcomptr_p.h>
QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaTextProvider::QWindowsUiaTextProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaTextProvider::~QWindowsUiaTextProvider()
{
}

// Returns an array of providers for the selected text ranges.
HRESULT STDMETHODCALLTYPE QWindowsUiaTextProvider::GetSelection(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    int selCount = textInterface->selectionCount();
    if (selCount > 0) {
        // Build a safe array with the text range providers.
        if ((*pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, selCount))) {
            for (LONG i = 0; i < selCount; ++i) {
                int startOffset = 0, endOffset = 0;
                textInterface->selection((int)i, &startOffset, &endOffset);
                ComPtr<IUnknown> textRangeProvider =
                        makeComObject<QWindowsUiaTextRangeProvider>(id(), startOffset, endOffset);
                SafeArrayPutElement(*pRetVal, &i, textRangeProvider.Get());
            }
        }
    } else {
        // If there is no selection, we return an array with a single degenerate (empty) text range at the cursor position.
        if ((*pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, 1))) {
            LONG i = 0;
            int cursorPosition = textInterface->cursorPosition();
            ComPtr<IUnknown> textRangeProvider = makeComObject<QWindowsUiaTextRangeProvider>(
                    id(), cursorPosition, cursorPosition);
            SafeArrayPutElement(*pRetVal, &i, textRangeProvider.Get());
        }
    }
    return S_OK;
}

// Returns an array of providers for the visible text ranges.
HRESULT STDMETHODCALLTYPE QWindowsUiaTextProvider::GetVisibleRanges(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    // Considering the entire text as visible.
    if ((*pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, 1))) {
        LONG i = 0;
        ComPtr<IUnknown> textRangeProvider =
                makeComObject<QWindowsUiaTextRangeProvider>(id(), 0, textInterface->characterCount());
        SafeArrayPutElement(*pRetVal, &i, textRangeProvider.Get());
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaTextProvider::RangeFromChild(IRawElementProviderSimple * /*childElement*/,
                                                                  ITextRangeProvider **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;
    // No children supported.
    return S_OK;
}

// Returns a degenerate text range at the specified point.
HRESULT STDMETHODCALLTYPE QWindowsUiaTextProvider::RangeFromPoint(UiaPoint point, ITextRangeProvider **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QWindow *window = windowForAccessible(accessible);
    if (!window)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QPoint pt;
    nativeUiaPointToPoint(point, window, &pt);

    int offset = textInterface->offsetAtPoint(pt);
    if (offset < 0 || offset >= textInterface->characterCount())
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = makeComObject<QWindowsUiaTextRangeProvider>(id(), offset, offset).Detach();
    return S_OK;
}

// Returns a text range provider for the entire text.
HRESULT STDMETHODCALLTYPE QWindowsUiaTextProvider::get_DocumentRange(ITextRangeProvider **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = makeComObject<QWindowsUiaTextRangeProvider>(id(), 0, textInterface->characterCount())
                       .Detach();
    return S_OK;
}

// Currently supporting single selection.
HRESULT STDMETHODCALLTYPE QWindowsUiaTextProvider::get_SupportedTextSelection(SupportedTextSelection *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = SupportedTextSelection_Single;
    return S_OK;
}

// Not supporting annotations.
HRESULT STDMETHODCALLTYPE QWindowsUiaTextProvider::RangeFromAnnotation(IRawElementProviderSimple * /*annotationElement*/, ITextRangeProvider **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaTextProvider::GetCaretRange(BOOL *isActive, ITextRangeProvider **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!isActive || !pRetVal)
        return E_INVALIDARG;
    *isActive = FALSE;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTextInterface *textInterface = accessible->textInterface();
    if (!textInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *isActive = accessible->state().focused;

    int cursorPosition = textInterface->cursorPosition();
    *pRetVal = makeComObject<QWindowsUiaTextRangeProvider>(id(), cursorPosition, cursorPosition)
                       .Detach();
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
