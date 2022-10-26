// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsclipboard.h"
#include "qwindowscontext.h"
#include "qwindowsole.h"

#include <QtGui/qguiapplication.h>
#include <QtGui/qclipboard.h>
#include <QtGui/qcolor.h>
#include <QtGui/qimage.h>

#include <QtCore/qdebug.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qthread.h>
#include <QtCore/qvariant.h>
#include <QtCore/qurl.h>
#include <QtCore/private/qsystemerror_p.h>

#include <QtGui/private/qwindowsguieventdispatcher_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsClipboard
    \brief Clipboard implementation.

    Registers a non-visible clipboard viewer window that
    receives clipboard events in its own window procedure to be
    able to receive clipboard-changed events, which
    QPlatformClipboard needs to emit. That requires housekeeping
    of the next in the viewer chain.

    \note The OLE-functions used in this class require OleInitialize().

    \internal
*/

#ifndef QT_NO_DEBUG_STREAM
static QDebug operator<<(QDebug d, const QMimeData *mimeData)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "QMimeData(";
    if (mimeData) {
        const QStringList formats = mimeData->formats();
        d << "formats=" << formats.join(u", ");
        if (mimeData->hasText())
            d << ", text=" << mimeData->text();
        if (mimeData->hasHtml())
            d << ", html=" << mimeData->html();
        if (mimeData->hasColor())
            d << ", colorData=" << qvariant_cast<QColor>(mimeData->colorData());
        if (mimeData->hasImage())
            d << ", imageData=" << qvariant_cast<QImage>(mimeData->imageData());
        if (mimeData->hasUrls())
             d << ", urls=" << mimeData->urls();
    } else {
        d << '0';
    }
    d << ')';
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

/*!
    \class QWindowsClipboardRetrievalMimeData
    \brief Special mime data class managing delayed retrieval of clipboard data.

    Implementation of QWindowsInternalMimeDataBase that obtains the
    IDataObject from the clipboard.

    \sa QWindowsInternalMimeDataBase, QWindowsClipboard
    \internal
*/

IDataObject *QWindowsClipboardRetrievalMimeData::retrieveDataObject() const
{
    enum : int { attempts = 3 };
    IDataObject * pDataObj = nullptr;
    // QTBUG-53979, retry in case the other application has clipboard locked
    for (int i = 1; i <= attempts; ++i) {
        if (SUCCEEDED(OleGetClipboard(&pDataObj))) {
            if (QWindowsContext::verbose > 1)
                qCDebug(lcQpaMime) << __FUNCTION__ << pDataObj;
            return pDataObj;
        }
        qCWarning(lcQpaMime, i == attempts
                  ? "Unable to obtain clipboard."
                  : "Retrying to obtain clipboard.");
        QThread::msleep(50);
    }

    return nullptr;
}

void QWindowsClipboardRetrievalMimeData::releaseDataObject(IDataObject *dataObject) const
{
    dataObject->Release();
}

extern "C" LRESULT QT_WIN_CALLBACK qClipboardViewerWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    if (QWindowsClipboard::instance()
        && QWindowsClipboard::instance()->clipboardViewerWndProc(hwnd, message, wParam, lParam, &result))
        return result;
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// QTBUG-36958, ensure the clipboard is flushed before
// QGuiApplication is destroyed since OleFlushClipboard()
// might query the data again which causes problems
// for QMimeData-derived classes using QPixmap/QImage.
static void cleanClipboardPostRoutine()
{
    if (QWindowsClipboard *cl = QWindowsClipboard::instance())
        cl->cleanup();
}

QWindowsClipboard *QWindowsClipboard::m_instance = nullptr;

QWindowsClipboard::QWindowsClipboard()
{
    QWindowsClipboard::m_instance = this;
    qAddPostRoutine(cleanClipboardPostRoutine);
}

QWindowsClipboard::~QWindowsClipboard()
{
    cleanup();
    QWindowsClipboard::m_instance = nullptr;
}

void QWindowsClipboard::cleanup()
{
    unregisterViewer(); // Should release data if owner.
    releaseIData();
}

void QWindowsClipboard::releaseIData()
{
    if (m_data) {
        delete m_data->mimeData();
        m_data->releaseQt();
        m_data->Release();
        m_data = nullptr;
    }
}

void QWindowsClipboard::registerViewer()
{
    m_clipboardViewer = QWindowsContext::instance()->
        createDummyWindow(QStringLiteral("ClipboardView"), L"QtClipboardView",
                          qClipboardViewerWndProc, WS_OVERLAPPED);

    m_formatListenerRegistered = AddClipboardFormatListener(m_clipboardViewer);
    if (!m_formatListenerRegistered)
        qErrnoWarning("AddClipboardFormatListener() failed.");

    if (!m_formatListenerRegistered)
        m_nextClipboardViewer = SetClipboardViewer(m_clipboardViewer);

    qCDebug(lcQpaMime) << __FUNCTION__ << "m_clipboardViewer:" << m_clipboardViewer
        << "format listener:" << m_formatListenerRegistered
        << "next:" << m_nextClipboardViewer;
}

void QWindowsClipboard::unregisterViewer()
{
    if (m_clipboardViewer) {
        if (m_formatListenerRegistered) {
            RemoveClipboardFormatListener(m_clipboardViewer);
            m_formatListenerRegistered = false;
        } else {
            ChangeClipboardChain(m_clipboardViewer, m_nextClipboardViewer);
            m_nextClipboardViewer = nullptr;
        }
        DestroyWindow(m_clipboardViewer);
        m_clipboardViewer = nullptr;
    }
}

// ### FIXME: Qt 6: Remove the clipboard chain handling code and make the
// format listener the default.

static bool isProcessBeingDebugged(HWND hwnd)
{
    DWORD pid = 0;
    if (!GetWindowThreadProcessId(hwnd, &pid) || !pid)
        return false;
    const HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!processHandle)
        return false;
    BOOL debugged = FALSE;
    CheckRemoteDebuggerPresent(processHandle, &debugged);
    CloseHandle(processHandle);
    return debugged != FALSE;
}

void QWindowsClipboard::propagateClipboardMessage(UINT message, WPARAM wParam, LPARAM lParam) const
{
    if (!m_nextClipboardViewer)
        return;
    // In rare cases, a clipboard viewer can hang (application crashed,
    // suspended by a shell prompt 'Select' or debugger).
    if (IsHungAppWindow(m_nextClipboardViewer)) {
        qWarning("Cowardly refusing to send clipboard message to hung application...");
        return;
    }
    // Do not block if the process is being debugged, specifically, if it is
    // displaying a runtime assert, which is not caught by isHungAppWindow().
    if (isProcessBeingDebugged(m_nextClipboardViewer))
        PostMessage(m_nextClipboardViewer, message, wParam, lParam);
    else
        SendMessage(m_nextClipboardViewer, message, wParam, lParam);
}

/*!
    \brief Windows procedure of the clipboard viewer. Emits changed and does
    housekeeping of the viewer chain.
*/

bool QWindowsClipboard::clipboardViewerWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result)
{
    enum { wMClipboardUpdate = 0x031D };

    *result = 0;
    if (QWindowsContext::verbose)
        qCDebug(lcQpaMime) << __FUNCTION__ << hwnd << message << QWindowsGuiEventDispatcher::windowsMessageName(message);

    switch (message) {
    case WM_CHANGECBCHAIN: {
        const HWND toBeRemoved = reinterpret_cast<HWND>(wParam);
        if (toBeRemoved == m_nextClipboardViewer) {
            m_nextClipboardViewer = reinterpret_cast<HWND>(lParam);
        } else {
            propagateClipboardMessage(message, wParam, lParam);
        }
    }
        return true;
    case wMClipboardUpdate:  // Clipboard Format listener (Vista onwards)
    case WM_DRAWCLIPBOARD: { // Clipboard Viewer Chain handling (up to XP)
        const bool owned = ownsClipboard();
        qCDebug(lcQpaMime) << "Clipboard changed owned " << owned;
        emitChanged(QClipboard::Clipboard);
        // clean up the clipboard object if we no longer own the clipboard
        if (!owned && m_data)
            releaseIData();
        if (!m_formatListenerRegistered)
            propagateClipboardMessage(message, wParam, lParam);
    }
        return true;
    case WM_DESTROY:
        // Recommended shutdown
        if (ownsClipboard()) {
            qCDebug(lcQpaMime) << "Clipboard owner on shutdown, releasing.";
            OleFlushClipboard();
            releaseIData();
        }
        return true;
    } // switch (message)
    return false;
}

QMimeData *QWindowsClipboard::mimeData(QClipboard::Mode mode)
{
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mode;
    if (mode != QClipboard::Clipboard)
        return nullptr;
    if (ownsClipboard())
        return m_data->mimeData();
    return &m_retrievalData;
}

void QWindowsClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mode << mimeData;
    if (mode != QClipboard::Clipboard)
        return;

    const bool newData = !m_data || m_data->mimeData() != mimeData;
    if (newData) {
        releaseIData();
        if (mimeData)
            m_data = new QWindowsOleDataObject(mimeData);
    }

    HRESULT src = S_FALSE;
    int attempts = 0;
    for (; attempts < 3; ++attempts) {
        src = OleSetClipboard(m_data);
        if (src != CLIPBRD_E_CANT_OPEN || QWindowsContext::isSessionLocked())
            break;
        QThread::msleep(100);
    }

    if (src != S_OK) {
        QString mimeDataFormats = mimeData ?
            mimeData->formats().join(u", ") : QString(QStringLiteral("NULL"));
        qErrnoWarning("OleSetClipboard: Failed to set mime data (%s) on clipboard: %s",
                      qPrintable(mimeDataFormats),
                      qPrintable(QSystemError::windowsComString(src)));
        releaseIData();
        return;
    }
}

void QWindowsClipboard::clear()
{
    const HRESULT src = OleSetClipboard(nullptr);
    if (src != S_OK)
        qErrnoWarning("OleSetClipboard: Failed to clear the clipboard: 0x%lx", src);
}

bool QWindowsClipboard::supportsMode(QClipboard::Mode mode) const
{
        return mode == QClipboard::Clipboard;
}

// Need a non-virtual in destructor.
bool QWindowsClipboard::ownsClipboard() const
{
    return m_data && OleIsCurrentClipboard(m_data) == S_OK;
}

bool QWindowsClipboard::ownsMode(QClipboard::Mode mode) const
{
    const bool result = mode == QClipboard::Clipboard ?
        ownsClipboard() : false;
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mode << result;
    return result;
}

QT_END_NAMESPACE
