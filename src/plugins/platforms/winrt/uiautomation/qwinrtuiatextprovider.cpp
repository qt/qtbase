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

#include "qwinrtuiatextprovider.h"
#include "qwinrtuiametadatacache.h"
#include "qwinrtuiatextrangeprovider.h"
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
using namespace ABI::Windows::UI::Xaml::Automation;
using namespace ABI::Windows::UI::Xaml::Automation::Provider;

QWinRTUiaTextProvider::QWinRTUiaTextProvider(QAccessible::Id id) :
    QWinRTUiaBaseProvider(id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

QWinRTUiaTextProvider::~QWinRTUiaTextProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

// Returns a text range provider for the entire text.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextProvider::get_DocumentRange(ITextRangeProvider **value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    ComPtr<QWinRTUiaTextRangeProvider> textRangeProvider = Make<QWinRTUiaTextRangeProvider>(id(), 0, metadata->characterCount());
    return textRangeProvider.CopyTo(value);
}

// Currently supporting single selection.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextProvider::get_SupportedTextSelection(SupportedTextSelection *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!value)
        return E_INVALIDARG;
    *value = SupportedTextSelection_Single;
    return S_OK;

}

// Returns an array of providers for the selected text ranges.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextProvider::GetSelection(UINT32 *returnValueSize, ITextRangeProvider ***returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValueSize || !returnValue)
        return E_INVALIDARG;
    *returnValueSize = 0;
    *returnValue = nullptr;

    struct Selection { int startOffset, endOffset; };

    auto accid = id();
    auto selections = std::make_shared<QVarLengthArray<Selection>>();

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([accid, selections]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            if (QAccessibleTextInterface *textInterface = accessible->textInterface()) {
                for (int i = 0; i < textInterface->selectionCount(); ++i) {
                    int startOffset, endOffset;
                    textInterface->selection(i, &startOffset, &endOffset);
                    selections->append({startOffset, endOffset});
                }
                if (selections->size() == 0) {
                    // If there is no selection, we return an array with a single degenerate (empty) text range at the cursor position.
                    auto cur = textInterface->cursorPosition();
                    selections->append({cur, cur});
                }
            }
        }
        return S_OK;
    }))) {
        return E_FAIL;
    }

    int selCount = selections->size();
    if (selCount < 1)
        return E_FAIL;

    ITextRangeProvider **providerArray = static_cast<ITextRangeProvider **>(CoTaskMemAlloc(selCount  * sizeof(ITextRangeProvider *)));
    if (!providerArray)
        return E_OUTOFMEMORY;

    auto dst = providerArray;
    for (auto sel : *selections) {
        ComPtr<QWinRTUiaTextRangeProvider> textRangeProvider
                = Make<QWinRTUiaTextRangeProvider>(id(), sel.startOffset, sel.endOffset);
        textRangeProvider.CopyTo(dst++);
    }
    *returnValueSize = selCount;
    *returnValue = providerArray;
    return S_OK;
}

// Returns an array of providers for the visible text ranges.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextProvider::GetVisibleRanges(UINT32 *returnValueSize, ITextRangeProvider ***returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValueSize || !returnValue)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());

    // Considering the entire text as visible.
    ComPtr<QWinRTUiaTextRangeProvider> textRangeProvider = Make<QWinRTUiaTextRangeProvider>(id(), 0, metadata->characterCount());
    textRangeProvider.CopyTo(*returnValue);
    *returnValueSize = 1;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextProvider::RangeFromChild(IIRawElementProviderSimple *childElement, ITextRangeProvider **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!childElement || !returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;
    // No children supported.
    return S_OK;
}

// Returns a degenerate text range at the specified point.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextProvider::RangeFromPoint(ABI::Windows::Foundation::Point screenLocation, ITextRangeProvider **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;

    const QPoint pt(screenLocation.X, screenLocation.Y);
    auto accid = id();
    auto offset = std::make_shared<int>();

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([accid, pt, offset]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid))
            if (QAccessibleTextInterface *textInterface = accessible->textInterface())
                *offset = qBound(0, textInterface->offsetAtPoint(pt), textInterface->characterCount() - 1);
        return S_OK;
    }))) {
        return E_FAIL;
    }

    ComPtr<QWinRTUiaTextRangeProvider> textRangeProvider = Make<QWinRTUiaTextRangeProvider>(id(), *offset, *offset);
    textRangeProvider.CopyTo(returnValue);
    return S_OK;
}

// Not supporting annotations.
HRESULT STDMETHODCALLTYPE QWinRTUiaTextProvider::RangeFromAnnotation(IIRawElementProviderSimple *annotationElement, ITextRangeProvider **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    if (!annotationElement || !returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaTextProvider::GetCaretRange(boolean *isActive, ITextRangeProvider **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!isActive || !returnValue)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *isActive = metadata->state().focused;

    ComPtr<QWinRTUiaTextRangeProvider> textRangeProvider = Make<QWinRTUiaTextRangeProvider>(id(), metadata->cursorPosition(), metadata->cursorPosition());
    return textRangeProvider.CopyTo(returnValue);
}


QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
