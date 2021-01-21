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

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiatextprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaTextProvider::QWindowsUiaTextProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaTextProvider::~QWindowsUiaTextProvider()
{
}

HRESULT STDMETHODCALLTYPE QWindowsUiaTextProvider::QueryInterface(REFIID iid, LPVOID *iface)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!iface)
        return E_INVALIDARG;
    *iface = nullptr;

    const bool result = qWindowsComQueryUnknownInterfaceMulti<ITextProvider>(this, iid, iface)
        || qWindowsComQueryInterface<ITextProvider>(this, iid, iface)
        || qWindowsComQueryInterface<ITextProvider2>(this, iid, iface);
    return result ? S_OK : E_NOINTERFACE;
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
                auto *textRangeProvider = new QWindowsUiaTextRangeProvider(id(), startOffset, endOffset);
                SafeArrayPutElement(*pRetVal, &i, static_cast<IUnknown *>(textRangeProvider));
                textRangeProvider->Release();
            }
        }
    } else {
        // If there is no selection, we return an array with a single degenerate (empty) text range at the cursor position.
        if ((*pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, 1))) {
            LONG i = 0;
            int cursorPosition = textInterface->cursorPosition();
            auto *textRangeProvider = new QWindowsUiaTextRangeProvider(id(), cursorPosition, cursorPosition);
            SafeArrayPutElement(*pRetVal, &i, static_cast<IUnknown *>(textRangeProvider));
            textRangeProvider->Release();
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
        auto *textRangeProvider = new QWindowsUiaTextRangeProvider(id(), 0, textInterface->characterCount());
        SafeArrayPutElement(*pRetVal, &i, static_cast<IUnknown *>(textRangeProvider));
        textRangeProvider->Release();
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
    if ((offset >= 0) && (offset < textInterface->characterCount())) {
        *pRetVal = new QWindowsUiaTextRangeProvider(id(), offset, offset);
    }
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

    *pRetVal = new QWindowsUiaTextRangeProvider(id(), 0, textInterface->characterCount());
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
    *pRetVal = new QWindowsUiaTextRangeProvider(id(), cursorPosition, cursorPosition);
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
