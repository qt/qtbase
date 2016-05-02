/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qwindowsinputcontext.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"
#include "qwindowsmousehandler.h"

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QRectF>
#include <QtCore/QTextBoundaryFinder>

#include <QtGui/QInputMethodEvent>
#include <QtGui/QTextCharFormat>
#include <QtGui/QPalette>
#include <QtGui/QGuiApplication>

#include <private/qhighdpiscaling_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

static inline QByteArray debugComposition(int lParam)
{
    QByteArray str;
    if (lParam & GCS_RESULTSTR)
        str += "RESULTSTR ";
    if (lParam & GCS_COMPSTR)
        str += "COMPSTR ";
    if (lParam & GCS_COMPATTR)
        str += "COMPATTR ";
    if (lParam & GCS_CURSORPOS)
        str += "CURSORPOS ";
    if (lParam & GCS_COMPCLAUSE)
        str += "COMPCLAUSE ";
    if (lParam & CS_INSERTCHAR)
        str += "INSERTCHAR ";
    if (lParam & CS_NOMOVECARET)
        str += "NOMOVECARET ";
    return str;
}

// Cancel current IME composition.
static inline void imeNotifyCancelComposition(HWND hwnd)
{
    if (!hwnd) {
        qWarning() << __FUNCTION__ << "called with" << hwnd;
        return;
    }
    const HIMC himc = ImmGetContext(hwnd);
    ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ImmReleaseContext(hwnd, himc);
}

static inline LCID languageIdFromLocaleId(LCID localeId)
{
    return localeId & 0xFFFF;
}

static inline LCID currentInputLanguageId()
{
    return languageIdFromLocaleId(reinterpret_cast<quintptr>(GetKeyboardLayout(0)));
}

Q_CORE_EXPORT QLocale qt_localeFromLCID(LCID id); // from qlocale_win.cpp

/*!
    \class QWindowsInputContext
    \brief Windows Input context implementation

    Handles input of foreign characters (particularly East Asian)
    languages.

    \section1 Testing

    \list
    \li Install the East Asian language support and choose Japanese (say).
    \li Compile the \a mainwindows/mdi example and open a text window.
    \li In the language bar, switch to Japanese and choose the
       Input method 'Hiragana'.
    \li In a text editor control, type the syllable \a 'la'.
       Underlined characters show up, indicating that there is completion
       available. Press the Space key two times. A completion popup occurs
       which shows the options.
    \endlist

    Reconversion: Input texts can be 'converted' into different
    input modes or more completion suggestions can be made based on
    context to correct errors. This is bound to the 'Conversion key'
    (F13-key in Japanese, which can be changed in the
    configuration). After writing text, pressing the key selects text
    and triggers a conversion popup, which shows the alternatives for
    the word.

    \section1 Interaction

    When the user activates input methods,  Windows sends
    WM_IME_STARTCOMPOSITION, WM_IME_COMPOSITION,
    WM_IME_ENDCOMPOSITION messages that trigger startComposition(),
    composition(), endComposition(), respectively. No key events are sent.

    composition() determines the markup of the pre-edit or selected
    text and/or the final text and sends that to the focus object.

    In between startComposition(), endComposition(), multiple
    compositions may happen (isComposing).

    update() is called to synchronize the position of the candidate
    window with the microfocus rectangle of the focus object.
    Also, a hidden caret is moved along with that position,
    which is important for some Chinese input methods.

    reset() is called to cancel a composition if the mouse is
    moved outside or for example some Undo/Redo operation is
    invoked.

    \note Mouse interaction of popups with
    QtWindows::InputMethodOpenCandidateWindowEvent and
    QtWindows::InputMethodCloseCandidateWindowEvent
    needs to be checked (mouse grab might interfere with candidate window).

    \internal
    \ingroup qt-lighthouse-win
*/


HIMC QWindowsInputContext::m_defaultContext = 0;

QWindowsInputContext::CompositionContext::CompositionContext() :
    hwnd(0), haveCaret(false), position(0), isComposing(false),
    factor(1)
{
}

QWindowsInputContext::QWindowsInputContext() :
    m_WM_MSIME_MOUSE(RegisterWindowMessage(L"MSIMEMouseOperation")),
    m_endCompositionRecursionGuard(false),
    m_languageId(currentInputLanguageId()),
    m_locale(qt_localeFromLCID(m_languageId))
{
    connect(QGuiApplication::inputMethod(), &QInputMethod::cursorRectangleChanged,
            this, &QWindowsInputContext::cursorRectChanged);
}

QWindowsInputContext::~QWindowsInputContext()
{
}

bool QWindowsInputContext::hasCapability(Capability capability) const
{
    switch (capability) {
    case QPlatformInputContext::HiddenTextCapability:
#ifndef Q_OS_WINCE
        return false; // QTBUG-40691, do not show IME on desktop for password entry fields.
#else
        break; // Windows CE: Show software keyboard.
#endif
    default:
        break;
    }
    return true;
}

/*!
    \brief Cancels a composition.
*/

void QWindowsInputContext::reset()
{
    QPlatformInputContext::reset();
    if (!m_compositionContext.hwnd)
        return;
    qCDebug(lcQpaInputMethods) << __FUNCTION__;
    if (m_compositionContext.isComposing && !m_compositionContext.focusObject.isNull()) {
        QInputMethodEvent event;
        if (!m_compositionContext.composition.isEmpty())
            event.setCommitString(m_compositionContext.composition);
        QCoreApplication::sendEvent(m_compositionContext.focusObject, &event);
        endContextComposition();
    }
    imeNotifyCancelComposition(m_compositionContext.hwnd);
    doneContext();
}

void QWindowsInputContext::setFocusObject(QObject *)
{
    // ### fixme: On Windows 8.1, it has been observed that the Input context
    // remains active when this happens resulting in a lock-up. Consecutive
    // key events still have VK_PROCESSKEY set and are thus ignored.
    if (m_compositionContext.isComposing)
        reset();
    updateEnabled();
}

void QWindowsInputContext::updateEnabled()
{
    if (!QGuiApplication::focusObject())
        return;
    if (QWindowsWindow *platformWindow = QWindowsWindow::windowsWindowOf(QGuiApplication::focusWindow())) {
        const bool accepted = inputMethodAccepted();
        if (QWindowsContext::verbose > 1)
            qCDebug(lcQpaInputMethods) << __FUNCTION__ << platformWindow->window() << "accepted=" << accepted;
            QWindowsInputContext::setWindowsImeEnabled(platformWindow, accepted);
    }
}

void QWindowsInputContext::setWindowsImeEnabled(QWindowsWindow *platformWindow, bool enabled)
{
    if (!platformWindow || platformWindow->testFlag(QWindowsWindow::InputMethodDisabled) == !enabled)
        return;
    if (enabled) {
        // Re-enable Windows IME by associating default context saved on first disabling.
        ImmAssociateContext(platformWindow->handle(), QWindowsInputContext::m_defaultContext);
        platformWindow->clearFlag(QWindowsWindow::InputMethodDisabled);
    } else {
        // Disable Windows IME by associating 0 context. Store context first time.
        const HIMC oldImC = ImmAssociateContext(platformWindow->handle(), 0);
        platformWindow->setFlag(QWindowsWindow::InputMethodDisabled);
        if (!QWindowsInputContext::m_defaultContext && oldImC)
            QWindowsInputContext::m_defaultContext = oldImC;
    }
}

/*!
    \brief Moves the candidate window along with microfocus of the focus object.
*/

void QWindowsInputContext::update(Qt::InputMethodQueries queries)
{
    if (queries & Qt::ImEnabled)
        updateEnabled();
    QPlatformInputContext::update(queries);
}

void QWindowsInputContext::cursorRectChanged()
{
    if (!m_compositionContext.hwnd)
        return;
    const QInputMethod *inputMethod = QGuiApplication::inputMethod();
    const QRectF cursorRectangleF = inputMethod->cursorRectangle();
    if (!cursorRectangleF.isValid())
        return;
    const QRect cursorRectangle =
        QRectF(cursorRectangleF.topLeft() * m_compositionContext.factor,
               cursorRectangleF.size() * m_compositionContext.factor).toRect();

    qCDebug(lcQpaInputMethods) << __FUNCTION__<< cursorRectangle;

    const HIMC himc = ImmGetContext(m_compositionContext.hwnd);
    if (!himc)
        return;
    // Move candidate list window to the microfocus position.
    COMPOSITIONFORM cf;
    // ### need X-like inputStyle config settings
    cf.dwStyle = CFS_FORCE_POSITION;
    cf.ptCurrentPos.x = cursorRectangle.x();
    cf.ptCurrentPos.y = cursorRectangle.y();

    CANDIDATEFORM candf;
    candf.dwIndex = 0;
    candf.dwStyle = CFS_EXCLUDE;
    candf.ptCurrentPos.x = cursorRectangle.x();
    candf.ptCurrentPos.y = cursorRectangle.y() + cursorRectangle.height();
    candf.rcArea.left = cursorRectangle.x();
    candf.rcArea.top = cursorRectangle.y();
    candf.rcArea.right = cursorRectangle.x() + cursorRectangle.width();
    candf.rcArea.bottom = cursorRectangle.y() + cursorRectangle.height();

    if (m_compositionContext.haveCaret)
        SetCaretPos(cursorRectangle.x(), cursorRectangle.y());

    ImmSetCompositionWindow(himc, &cf);
    ImmSetCandidateWindow(himc, &candf);
    ImmReleaseContext(m_compositionContext.hwnd, himc);
}

void QWindowsInputContext::invokeAction(QInputMethod::Action action, int cursorPosition)
{
    if (action != QInputMethod::Click || !m_compositionContext.hwnd) {
        QPlatformInputContext::invokeAction(action, cursorPosition);
        return;
    }

    qCDebug(lcQpaInputMethods) << __FUNCTION__ << cursorPosition << action;
    if (cursorPosition < 0 || cursorPosition > m_compositionContext.composition.size())
        reset();

    // Magic code that notifies Japanese IME about the cursor
    // position.
    const HIMC himc = ImmGetContext(m_compositionContext.hwnd);
    const HWND imeWindow = ImmGetDefaultIMEWnd(m_compositionContext.hwnd);
    const WPARAM mouseOperationCode =
        MAKELONG(MAKEWORD(MK_LBUTTON, cursorPosition == 0 ? 2 : 1), cursorPosition);
    SendMessage(imeWindow, m_WM_MSIME_MOUSE, mouseOperationCode, LPARAM(himc));
    ImmReleaseContext(m_compositionContext.hwnd, himc);
}

static inline QString getCompositionString(HIMC himc, DWORD dwIndex)
{
    enum { bufferSize = 256 };
    wchar_t buffer[bufferSize];
    const int length = ImmGetCompositionString(himc, dwIndex, buffer, bufferSize * sizeof(wchar_t));
    return QString::fromWCharArray(buffer,  size_t(length) / sizeof(wchar_t));
}

// Determine the converted string range as pair of start/length to be selected.
static inline void getCompositionStringConvertedRange(HIMC himc, int *selStart, int *selLength)
{
    enum { bufferSize = 256 };
    // Find the range of bytes with ATTR_TARGET_CONVERTED set.
    char attrBuffer[bufferSize];
    *selStart = *selLength = 0;
    if (const int attrLength = ImmGetCompositionString(himc, GCS_COMPATTR, attrBuffer, bufferSize)) {
        int start = 0;
        while (start < attrLength && !(attrBuffer[start] & ATTR_TARGET_CONVERTED))
            start++;
        if (start < attrLength) {
            int end = start + 1;
            while (end < attrLength && (attrBuffer[end] & ATTR_TARGET_CONVERTED))
                end++;
            *selStart = start;
            *selLength = end - start;
        }
    }
}

enum StandardFormat {
    PreeditFormat,
    SelectionFormat
};

static inline QTextFormat standardFormat(StandardFormat format)
{
    QTextCharFormat result;
    switch (format) {
    case PreeditFormat:
        result.setUnderlineStyle(QTextCharFormat::DashUnderline);
        break;
    case SelectionFormat: {
        // TODO: Should be that of the widget?
        const QPalette palette = QGuiApplication::palette();
        const QColor background = palette.text().color();
        result.setBackground(QBrush(background));
        result.setForeground(palette.background());
        break;
    }
    }
    return result;
}

bool QWindowsInputContext::startComposition(HWND hwnd)
{
    QObject *fo = QGuiApplication::focusObject();
    if (!fo)
        return false;
    // This should always match the object.
    QWindow *window = QGuiApplication::focusWindow();
    if (!window)
        return false;
    qCDebug(lcQpaInputMethods) << __FUNCTION__ << fo << window << "language=" << m_languageId;
    if (!fo || QWindowsWindow::handleOf(window) != hwnd)
        return false;
    initContext(hwnd, QHighDpiScaling::factor(window), fo);
    startContextComposition();
    return true;
}

void QWindowsInputContext::startContextComposition()
{
    if (m_compositionContext.isComposing) {
        qWarning("%s: Called out of sequence.", __FUNCTION__);
        return;
    }
    m_compositionContext.isComposing = true;
    m_compositionContext.composition.clear();
    m_compositionContext.position = 0;
    cursorRectChanged(); // position cursor initially.
    update(Qt::ImQueryAll);
}

void QWindowsInputContext::endContextComposition()
{
    if (!m_compositionContext.isComposing) {
        qWarning("%s: Called out of sequence.", __FUNCTION__);
        return;
    }
    m_compositionContext.composition.clear();
    m_compositionContext.position = 0;
    m_compositionContext.isComposing = false;
}

// Create a list of markup attributes for QInputMethodEvent
// to display the selected part of the intermediate composition
// result differently.
static inline QList<QInputMethodEvent::Attribute>
    intermediateMarkup(int position, int compositionLength,
                       int selStart, int selLength)
{
    QList<QInputMethodEvent::Attribute> attributes;
    if (selStart > 0)
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 0, selStart,
                                                   standardFormat(PreeditFormat));
    if (selLength)
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, selStart, selLength,
                                                   standardFormat(SelectionFormat));
    if (selStart + selLength < compositionLength)
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, selStart + selLength,
                                                   compositionLength - selStart - selLength,
                                                   standardFormat(PreeditFormat));
    if (position >= 0)
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, position, selLength ? 0 : 1, QVariant());
    return attributes;
}

/*!
    \brief Notify focus object about markup or final text.
*/

bool QWindowsInputContext::composition(HWND hwnd, LPARAM lParamIn)
{
    const int lParam = int(lParamIn);
    qCDebug(lcQpaInputMethods) << '>' << __FUNCTION__ << m_compositionContext.focusObject
        << debugComposition(lParam) << " composing=" << m_compositionContext.isComposing;
    if (m_compositionContext.focusObject.isNull() || m_compositionContext.hwnd != hwnd || !lParam)
        return false;
    const HIMC himc = ImmGetContext(m_compositionContext.hwnd);
    if (!himc)
        return false;

    QScopedPointer<QInputMethodEvent> event;
    if (lParam & (GCS_COMPSTR | GCS_COMPATTR | GCS_CURSORPOS)) {
        if (!m_compositionContext.isComposing)
            startContextComposition();
        // Some intermediate composition result. Parametrize event with
        // attribute sequence specifying the formatting of the converted part.
        int selStart, selLength;
        m_compositionContext.composition = getCompositionString(himc, GCS_COMPSTR);
        m_compositionContext.position = ImmGetCompositionString(himc, GCS_CURSORPOS, 0, 0);
        getCompositionStringConvertedRange(himc, &selStart, &selLength);
        if ((lParam & CS_INSERTCHAR) && (lParam & CS_NOMOVECARET)) {
            // make Korean work correctly. Hope this is correct for all IMEs
            selStart = 0;
            selLength = m_compositionContext.composition.size();
        }
        if (!selLength)
            selStart = 0;

        event.reset(new QInputMethodEvent(m_compositionContext.composition,
                                          intermediateMarkup(m_compositionContext.position,
                                                             m_compositionContext.composition.size(),
                                                             selStart, selLength)));
    }
    if (event.isNull())
        event.reset(new QInputMethodEvent);

    if (lParam & GCS_RESULTSTR) {
        // A fixed result, return the converted string
        event->setCommitString(getCompositionString(himc, GCS_RESULTSTR));
        if (!(lParam & GCS_DELTASTART))
            endContextComposition();
    }
    const bool result = QCoreApplication::sendEvent(m_compositionContext.focusObject, event.data());
    qCDebug(lcQpaInputMethods) << '<' << __FUNCTION__ << "sending markup="
        << event->attributes().size() << " commit=" << event->commitString()
        << " to " << m_compositionContext.focusObject << " returns " << result;
    update(Qt::ImQueryAll);
    ImmReleaseContext(m_compositionContext.hwnd, himc);
    return result;
}

bool QWindowsInputContext::endComposition(HWND hwnd)
{
    qCDebug(lcQpaInputMethods) << __FUNCTION__ << m_endCompositionRecursionGuard << hwnd;
    // Googles Pinyin Input Method likes to call endComposition again
    // when we call notifyIME with CPS_CANCEL, so protect ourselves
    // against that.
    if (m_endCompositionRecursionGuard || m_compositionContext.hwnd != hwnd)
        return false;
    if (m_compositionContext.focusObject.isNull())
        return false;

    m_endCompositionRecursionGuard = true;

    imeNotifyCancelComposition(m_compositionContext.hwnd);
    if (m_compositionContext.isComposing) {
        QInputMethodEvent event;
        QCoreApplication::sendEvent(m_compositionContext.focusObject, &event);
    }
    doneContext();

    m_endCompositionRecursionGuard = false;
    return true;
}

void QWindowsInputContext::initContext(HWND hwnd, qreal factor, QObject *focusObject)
{
    if (m_compositionContext.hwnd)
        doneContext();
    m_compositionContext.hwnd = hwnd;
    m_compositionContext.focusObject = focusObject;
    m_compositionContext.factor = factor;
    // Create a hidden caret which is kept at the microfocus
    // position in update(). This is important for some
    // Chinese input methods.
    m_compositionContext.haveCaret = CreateCaret(hwnd, 0, 1, 1);
    HideCaret(hwnd);
    update(Qt::ImQueryAll);
    m_compositionContext.isComposing = false;
    m_compositionContext.position = 0;
}

void QWindowsInputContext::doneContext()
{
    if (!m_compositionContext.hwnd)
        return;
    if (m_compositionContext.haveCaret)
        DestroyCaret();
    m_compositionContext.hwnd = 0;
    m_compositionContext.composition.clear();
    m_compositionContext.position = 0;
    m_compositionContext.isComposing = m_compositionContext.haveCaret = false;
    m_compositionContext.focusObject = 0;
}

bool QWindowsInputContext::handleIME_Request(WPARAM wParam,
                                             LPARAM lParam,
                                             LRESULT *result)
{
    switch (int(wParam)) {
    case IMR_RECONVERTSTRING: {
        const int size = reconvertString(reinterpret_cast<RECONVERTSTRING *>(lParam));
        if (size < 0)
            return false;
        *result = size;
    }
        return true;
    case IMR_CONFIRMRECONVERTSTRING:
        return true;
    default:
        break;
    }
    return false;
}

void QWindowsInputContext::handleInputLanguageChanged(WPARAM wparam, LPARAM lparam)
{
    const LCID newLanguageId = languageIdFromLocaleId(WORD(lparam));
    if (newLanguageId == m_languageId)
        return;
    const LCID oldLanguageId = m_languageId;
    m_languageId = newLanguageId;
    m_locale = qt_localeFromLCID(m_languageId);
    emitLocaleChanged();

    qCDebug(lcQpaInputMethods) << __FUNCTION__ << hex << showbase
        << oldLanguageId  << "->" << newLanguageId << "Character set:"
        << DWORD(wparam) << dec << noshowbase << m_locale;
}

/*!
    \brief Determines the string for reconversion with selection.

    This is triggered twice by WM_IME_REQUEST, first with reconv=0
    to determine the length and later with a reconv struct to obtain
    the string with the position of the selection to be reconverted.

    Obtains the text from the focus object and marks the word
    for selection (might not be entirely correct for Japanese).
*/

int QWindowsInputContext::reconvertString(RECONVERTSTRING *reconv)
{
    QObject *fo = QGuiApplication::focusObject();
    if (!fo)
        return false;

    const QVariant surroundingTextV = QInputMethod::queryFocusObject(Qt::ImSurroundingText, QVariant());
    if (!surroundingTextV.isValid())
        return -1;
    const QString surroundingText = surroundingTextV.toString();
    const int memSize = int(sizeof(RECONVERTSTRING))
        + (surroundingText.length() + 1) * int(sizeof(ushort));
    qCDebug(lcQpaInputMethods) << __FUNCTION__ << " reconv=" << reconv
        << " surroundingText=" << surroundingText << " size=" << memSize;
    // If memory is not allocated, return the required size.
    if (!reconv)
        return surroundingText.isEmpty() ? -1 : memSize;

    const QVariant posV = QInputMethod::queryFocusObject(Qt::ImCursorPosition, QVariant());
    const int pos = posV.isValid() ? posV.toInt() : 0;
    // Find the word in the surrounding text.
    QTextBoundaryFinder bounds(QTextBoundaryFinder::Word, surroundingText);
    bounds.setPosition(pos);
    if (bounds.position() > 0 && !(bounds.boundaryReasons() & QTextBoundaryFinder::StartOfItem))
        bounds.toPreviousBoundary();
    const int startPos = bounds.position();
    bounds.toNextBoundary();
    const int endPos = bounds.position();
    qCDebug(lcQpaInputMethods) << __FUNCTION__ << " boundary=" << startPos << endPos;
    // Select the text, this will be overwritten by following IME events.
    QList<QInputMethodEvent::Attribute> attributes;
    attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, startPos, endPos-startPos, QVariant());
    QInputMethodEvent selectEvent(QString(), attributes);
    QCoreApplication::sendEvent(fo, &selectEvent);

    reconv->dwSize = DWORD(memSize);
    reconv->dwVersion = 0;

    reconv->dwStrLen = DWORD(surroundingText.size());
    reconv->dwStrOffset = sizeof(RECONVERTSTRING);
    reconv->dwCompStrLen = DWORD(endPos - startPos); // TCHAR count.
    reconv->dwCompStrOffset = DWORD(startPos) * sizeof(ushort); // byte count.
    reconv->dwTargetStrLen = reconv->dwCompStrLen;
    reconv->dwTargetStrOffset = reconv->dwCompStrOffset;
    ushort *pastReconv = reinterpret_cast<ushort *>(reconv + 1);
    std::copy(surroundingText.utf16(), surroundingText.utf16() + surroundingText.size(),
              QT_MAKE_UNCHECKED_ARRAY_ITERATOR(pastReconv));
    return memSize;
}

QT_END_NAMESPACE
