/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqnxinputcontext_imf.h"
#include "qqnxeventthread.h"
#include "qqnxabstractvirtualkeyboard.h"

#include <QtWidgets/QAbstractSpinBox>
#include <QtWidgets/QAction>

#include <QtGui/QGuiApplication>
#include <QtGui/QInputMethodEvent>
#include <QtGui/QTextCharFormat>

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QVariant>
#include <QtCore/QVariantHash>
#include <QtCore/QWaitCondition>

#include <dlfcn.h>
#include "imf/imf_client.h"
#include "imf/input_control.h"
#include <process.h>
#include <sys/keycodes.h>

/** TODO:
    Support inputMethodHints to restrict input (needs additional features in IMF).
*/

#define STRX(x) #x
#define STR(x) STRX(x)

// Someone tell me why input_control methods are in this namespace, but the rest is not.
using namespace InputMethodSystem;

#define qs(x) QString::fromLatin1(x)
#define iarg(name) event->mArgs[qs(#name)] = QVariant::fromValue(name)
#define parg(name) event->mArgs[qs(#name)] = QVariant::fromValue((void*)name)
namespace
{

spannable_string_t *toSpannableString(const QString &text);
static const input_session_t *sInputSession = 0;
bool isSessionOkay(input_session_t *ic)
{
    return ic !=0 && sInputSession != 0 && ic->component_id == sInputSession->component_id;
}

enum ImfEventType
{
    ImfBeginBatchEdit,
    ImfClearMetaKeyStates,
    ImfCommitText,
    ImfDeleteSurroundingText,
    ImfEndBatchEdit,
    ImfFinishComposingText,
    ImfGetCursorCapsMode,
    ImfGetCursorPosition,
    ImfGetExtractedText,
    ImfGetSelectedText,
    ImfGetTextAfterCursor,
    ImfGetTextBeforeCursor,
    ImfPerformEditorAction,
    ImfReportFullscreenMode,
    ImfSendEvent,
    ImfSendAsyncEvent,
    ImfSetComposingRegion,
    ImfSetComposingText,
    ImfSetSelection
};

// We use this class as a round about way to support a posting synchronous event into
// Qt's main thread from the IMF thread.
class ImfEventResult
{
public:
    ImfEventResult()
    {
        m_mutex.lock();
    }

    ~ImfEventResult()
    {
        m_mutex.unlock();
    }

    void wait()
    {
        m_wait.wait(&m_mutex);
    }

    void signal()
    {
        m_wait.wakeAll();
    }

    void setResult(const QVariant& result)
    {
        m_mutex.lock();
        m_retVal = result;
        signal();
        m_mutex.unlock();
    }

    QVariant result()
    {
        return m_retVal;
    }

private:
    QVariant m_retVal;
    QMutex m_mutex;
    QWaitCondition m_wait;
};

class ImfEvent : public QEvent
{
    public:
        ImfEvent(input_session_t *session, ImfEventType type, ImfEventResult *result) :
            QEvent((QEvent::Type)sUserEventType),
            m_session(session),
            m_imfType(type),
            m_result(result)
        {
        }
        ~ImfEvent() { }

    input_session_t *m_session;
    ImfEventType m_imfType;
    QVariantHash m_args;
    ImfEventResult *m_result;

    static int sUserEventType;
};
int ImfEvent::sUserEventType = QEvent::registerEventType();

static int32_t imfBeginBatchEdit(input_session_t *ic)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfBeginBatchEdit, &result);
    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();

    return ret;
}

static int32_t imfClearMetaKeyStates(input_session_t *ic, int32_t states)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfClearMetaKeyStates, &result);
    iarg(states);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();

    return ret;
}

static int32_t imfCommitText(input_session_t *ic, spannable_string_t *text, int32_t new_cursor_position)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfCommitText, &result);
    parg(text);
    iarg(new_cursor_position);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();

    return ret;
}

static int32_t imfDeleteSurroundingText(input_session_t *ic, int32_t left_length, int32_t right_length)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfDeleteSurroundingText, &result);
    iarg(left_length);
    iarg(right_length);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();

    return ret;
}

static int32_t imfEndBatchEdit(input_session_t *ic)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfEndBatchEdit, &result);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();

    return ret;
}

static int32_t imfFinishComposingText(input_session_t *ic)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfFinishComposingText, &result);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();

    return ret;
}

static int32_t imfGetCursorCapsMode(input_session_t *ic, int32_t req_modes)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfGetCursorCapsMode, &result);
    iarg(req_modes);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    int32_t ret = result.result().value<int32_t>();
    return ret;
}

static int32_t imfGetCursorPosition(input_session_t *ic)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfGetCursorPosition, &result);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();

    return ret;
}

static extracted_text_t *imfGetExtractedText(input_session_t *ic, extracted_text_request_t *request, int32_t flags)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic)) {
        extracted_text_t *et = (extracted_text_t *)calloc(sizeof(extracted_text_t),1);
        et->text = (spannable_string_t *)calloc(sizeof(spannable_string_t),1);
        return et;
    }

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfGetExtractedText, &result);
    parg(request);
    iarg(flags);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    return result.result().value<extracted_text_t *>();
}

static spannable_string_t *imfGetSelectedText(input_session_t *ic, int32_t flags)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return toSpannableString("");

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfGetSelectedText, &result);
    iarg(flags);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    return result.result().value<extracted_text_t *>();
}

static spannable_string_t *imfGetTextAfterCursor(input_session_t *ic, int32_t n, int32_t flags)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return toSpannableString("");

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfGetTextAfterCursor, &result);
    iarg(n);
    iarg(flags);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    return result.result().value<extracted_text_t *>();
}

static spannable_string_t *imfGetTextBeforeCursor(input_session_t *ic, int32_t n, int32_t flags)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return toSpannableString("");

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfGetTextBeforeCursor, &result);
    iarg(n);
    iarg(flags);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    return result.result().value<extracted_text_t *>();
}

static int32_t imfPerformEditorAction(input_session_t *ic, int32_t editor_action)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfPerformEditorAction, &result);
    iarg(editor_action);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();
    return ret;
}

static int32_t imfReportFullscreenMode(input_session_t *ic, int32_t enabled)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfReportFullscreenMode, &result);
    iarg(enabled);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();
    return ret;
}

static int32_t imfSendEvent(input_session_t *ic, event_t *event)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEvent *imfEvent = new ImfEvent(ic, ImfSendEvent, 0);
    imfEvent->m_args[qs("event")] = QVariant::fromValue(static_cast<void *>(event));

    QCoreApplication::postEvent(QCoreApplication::instance(), imfEvent);

    return 0;
}

static int32_t imfSendAsyncEvent(input_session_t *ic, event_t *event)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEvent *imfEvent = new ImfEvent(ic, ImfSendAsyncEvent, 0);
    imfEvent->m_args[qs("event")] = QVariant::fromValue(static_cast<void *>(event));

    QCoreApplication::postEvent(QCoreApplication::instance(), imfEvent);

    return 0;
}

static int32_t imfSetComposingRegion(input_session_t *ic, int32_t start, int32_t end)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfSetComposingRegion, &result);
    iarg(start);
    iarg(end);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();
    return ret;
}

static int32_t imfSetComposingText(input_session_t *ic, spannable_string_t *text, int32_t new_cursor_position)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfSetComposingText, &result);
    parg(text);
    iarg(new_cursor_position);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();
    return ret;
}

static int32_t imfSetSelection(input_session_t *ic, int32_t start, int32_t end)
{
#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    ImfEventResult result;
    ImfEvent *event = new ImfEvent(ic, ImfSetSelection, &result);
    iarg(start);
    iarg(end);

    QCoreApplication::postEvent(QCoreApplication::instance(), event);

    result.wait();
    int32_t ret = result.result().value<int32_t>();
    return ret;
}

static connection_interface_t ic_funcs = {
    imfBeginBatchEdit,
    imfClearMetaKeyStates,
    imfCommitText,
    imfDeleteSurroundingText,
    imfEndBatchEdit,
    imfFinishComposingText,
    imfGetCursorCapsMode,
    imfGetCursorPosition,
    imfGetExtractedText,
    imfGetSelectedText,
    imfGetTextAfterCursor,
    imfGetTextBeforeCursor,
    imfPerformEditorAction,
    imfReportFullscreenMode,
    NULL, //ic_send_key_event
    imfSendEvent,
    imfSendAsyncEvent,
    imfSetComposingRegion,
    imfSetComposingText,
    imfSetSelection,
    NULL, //ic_set_candidates,
};

static void
initEvent(event_t *pEvent, const input_session_t *pSession, EventType eventType, int eventId)
{
    static int s_transactionId;

    // Make sure structure is squeaky clean since it's not clear just what is significant.
    memset(pEvent, 0, sizeof(event_t));
    pEvent->event_type = eventType;
    pEvent->event_id = eventId;
    pEvent->pid = getpid();
    pEvent->component_id = pSession->component_id;
    pEvent->transaction_id = ++s_transactionId;
}

spannable_string_t *toSpannableString(const QString &text)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO << text;
#endif

    spannable_string_t *pString = reinterpret_cast<spannable_string_t *>(malloc(sizeof(spannable_string_t)));
    pString->str = (wchar_t *)malloc(sizeof(wchar_t) * text.length() + 1);
    pString->length = text.length();
    pString->spans = NULL;
    pString->spans_count = 0;

    const QChar *pData = text.constData();
    wchar_t *pDst = pString->str;

    while (!pData->isNull())
    {
        *pDst = pData->unicode();
        pDst++;
        pData++;
    }
    *pDst = 0;

    return pString;
}

} // namespace

static const input_session_t *(*p_ictrl_open_session)(connection_interface_t *) = 0;
static void (*p_ictrl_close_session)(input_session_t *) = 0;
static int32_t (*p_ictrl_dispatch_event)(event_t*) = 0;
static int32_t (*p_imf_client_init)() = 0;
static void (*p_imf_client_disconnect)() = 0;
static int32_t (*p_vkb_init_selection_service)() = 0;
static int32_t (*p_ictrl_get_num_active_sessions)() = 0;
static bool s_imfInitFailed = false;

static bool imfAvailable()
{
    static bool s_imfDisabled = getenv("DISABLE_IMF") != NULL;
    static bool s_imfReady = false;

    if ( s_imfInitFailed || s_imfDisabled) {
        return false;
    }
    else if ( s_imfReady ) {
        return true;
    }

    if ( p_imf_client_init == NULL ) {
        void *handle = dlopen("libinput_client.so.1", 0);
        if ( handle ) {
            p_imf_client_init = (int32_t (*)()) dlsym(handle, "imf_client_init");
            p_imf_client_disconnect = (void (*)()) dlsym(handle, "imf_client_disconnect");
            p_ictrl_open_session = (const input_session_t *(*)(connection_interface_t *))dlsym(handle, "ictrl_open_session");
            p_ictrl_close_session = (void (*)(input_session_t *))dlsym(handle, "ictrl_close_session");
            p_ictrl_dispatch_event = (int32_t (*)(event_t *))dlsym(handle, "ictrl_dispatch_event");
            p_vkb_init_selection_service = (int32_t (*)())dlsym(handle, "vkb_init_selection_service");
            p_ictrl_get_num_active_sessions = (int32_t (*)())dlsym(handle, "ictrl_get_num_active_sessions");
        }
        else
        {
            qCritical() << Q_FUNC_INFO << "libinput_client.so.1 is not present - IMF services are disabled.";
            s_imfDisabled = true;
            return false;
        }
        if ( p_imf_client_init && p_ictrl_open_session && p_ictrl_dispatch_event ) {
            s_imfReady = true;
        }
        else {
            p_ictrl_open_session = NULL;
            p_ictrl_dispatch_event = NULL;
            s_imfDisabled = true;
            qCritical() << Q_FUNC_INFO << "libinput_client.so.1 did not contain the correct symbols, library mismatch? IMF services are disabled.";
            return false;
        }
    }

    return s_imfReady;
}

QT_BEGIN_NAMESPACE

QQnxInputContext::QQnxInputContext(QQnxAbstractVirtualKeyboard &keyboard):
         QPlatformInputContext(),
         m_lastCaretPos(0),
         m_isComposing(false),
         m_inputPanelVisible(false),
         m_inputPanelLocale(QLocale::c()),
         m_virtualKeyboad(keyboard)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!imfAvailable())
        return;

    if ( p_imf_client_init() != 0 ) {
        s_imfInitFailed = true;
        qCritical("imf_client_init failed - IMF services will be unavailable");
    }

    QCoreApplication::instance()->installEventFilter(this);

    // p_vkb_init_selection_service();

    connect(&keyboard, SIGNAL(visibilityChanged(bool)), this, SLOT(keyboardVisibilityChanged(bool)));
    connect(&keyboard, SIGNAL(localeChanged(QLocale)), this, SLOT(keyboardLocaleChanged(QLocale)));
    keyboardVisibilityChanged(keyboard.isVisible());
    keyboardLocaleChanged(keyboard.locale());
}

QQnxInputContext::~QQnxInputContext()
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!imfAvailable())
        return;

    QCoreApplication::instance()->removeEventFilter(this);
    p_imf_client_disconnect();
}

#define getarg(type, name) type name = imfEvent->mArgs[qs(#name)].value<type>()
#define getparg(type, name) type name = (type)(imfEvent->mArgs[qs(#name)].value<void*>())

bool QQnxInputContext::isValid() const
{
    return imfAvailable();
}

bool QQnxInputContext::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == ImfEvent::sUserEventType) {
        // Forward the event to our real handler.
        ImfEvent *imfEvent = static_cast<ImfEvent *>(event);
        switch (imfEvent->m_imfType) {
        case ImfBeginBatchEdit: {
            int32_t ret = onBeginBatchEdit(imfEvent->m_session);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfClearMetaKeyStates: {
            getarg(int32_t, states);
            int32_t ret = onClearMetaKeyStates(imfEvent->m_session, states);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfCommitText: {
            getparg(spannable_string_t*, text);
            getarg(int32_t, new_cursor_position);
            int32_t ret = onCommitText(imfEvent->m_session, text, new_cursor_position);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfDeleteSurroundingText: {
            getarg(int32_t, left_length);
            getarg(int32_t, right_length);
            int32_t ret = onDeleteSurroundingText(imfEvent->m_session, left_length, right_length);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfEndBatchEdit: {
            int32_t ret = onEndBatchEdit(imfEvent->m_session);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfFinishComposingText: {
            int32_t ret = onFinishComposingText(imfEvent->m_session);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfGetCursorCapsMode: {
            getarg(int32_t, req_modes);
            int32_t ret = onGetCursorCapsMode(imfEvent->m_session, req_modes);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfGetCursorPosition: {
            int32_t ret = onGetCursorPosition(imfEvent->m_session);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfGetExtractedText: {
            getparg(extracted_text_request_t*, request);
            getarg(int32_t, flags);
            extracted_text_t *ret = onGetExtractedText(imfEvent->m_session, request, flags);
            imfEvent->m_result->setResult(QVariant::fromValue(static_cast<void *>(ret)));
            break;
        }

        case ImfGetSelectedText: {
            getarg(int32_t, flags);
            spannable_string_t *ret = onGetSelectedText(imfEvent->m_session, flags);
            imfEvent->m_result->setResult(QVariant::fromValue(static_cast<void *>(ret)));
            break;
        }

        case ImfGetTextAfterCursor: {
            getarg(int32_t, n);
            getarg(int32_t, flags);
            spannable_string_t *ret = onGetTextAfterCursor(imfEvent->m_session, n, flags);
            imfEvent->m_result->setResult(QVariant::fromValue(static_cast<void *>(ret)));
            break;
        }

        case ImfGetTextBeforeCursor: {
            getarg(int32_t, n);
            getarg(int32_t, flags);
            spannable_string_t *ret = onGetTextBeforeCursor(imfEvent->m_session, n, flags);
            imfEvent->m_result->setResult(QVariant::fromValue((void*)ret));
            break;
        }

        case ImfPerformEditorAction: {
            getarg(int32_t, editor_action);
            int32_t ret = onPerformEditorAction(imfEvent->m_session, editor_action);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfReportFullscreenMode: {
            getarg(int32_t, enabled);
            int32_t ret = onReportFullscreenMode(imfEvent->m_session, enabled);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfSendEvent: {
            getparg(event_t*, event);
            onSendEvent(imfEvent->m_session, event);
            break;
        }

        case ImfSendAsyncEvent: {
            getparg(event_t*, event);
            onSendAsyncEvent(imfEvent->m_session, event);
            break;
        }

        case ImfSetComposingRegion: {
            getarg(int32_t, start);
            getarg(int32_t, end);
            int32_t ret = onSetComposingRegion(imfEvent->m_session, start, end);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfSetComposingText: {
            getparg(spannable_string_t*, text);
            getarg(int32_t, new_cursor_position);
            int32_t ret = onSetComposingText(imfEvent->m_session, text, new_cursor_position);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }

        case ImfSetSelection: {
            getarg(int32_t, start);
            getarg(int32_t, end);
            int32_t ret = onSetSelection(imfEvent->m_session, start, end);
            imfEvent->m_result->setResult(QVariant::fromValue(ret));
            break;
        }
        }; //switch

        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}

bool QQnxInputContext::filterEvent( const QEvent *event )
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO << event;
#endif
    switch (event->type()) {
    case QEvent::CloseSoftwareInputPanel: {
        return dispatchCloseSoftwareInputPanel();
    }
    case QEvent::RequestSoftwareInputPanel: {
        return dispatchRequestSoftwareInputPanel();
    }
    default:
        return false;
    }
}

void QQnxInputContext::reset()
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    endComposition();
}

void QQnxInputContext::update(Qt::InputMethodQueries queries)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    reset();

    QPlatformInputContext::update(queries);
}

void QQnxInputContext::closeSession()
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO
#endif
    if (!imfAvailable())
        return;

    if (sInputSession) {
        p_ictrl_close_session((input_session_t *)sInputSession);
        sInputSession = 0;
    }
}

void QQnxInputContext::openSession()
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO
#endif
    if (!imfAvailable())
        return;

    closeSession();
    sInputSession = p_ictrl_open_session(&ic_funcs);
}

bool QQnxInputContext::hasSession()
{
    return sInputSession != 0;
}

bool QQnxInputContext::hasSelectedText()
{
    QObject *input = qGuiApp->focusObject();
    if (!input)
        return false;

    QInputMethodQueryEvent query(Qt::ImCurrentSelection);
    QCoreApplication::sendEvent(input, &query);

    return !query.value(Qt::ImCurrentSelection).toString().isEmpty();
}

bool QQnxInputContext::dispatchRequestSoftwareInputPanel()
{
    m_virtualKeyboard.showKeyboard();
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << "QQNX: requesting virtual keyboard";
#endif
    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input || !inputMethodAccepted())
        return true;

    if (!hasSession())
        openSession();

    // This also means that the caret position has moved
    QInputMethodQueryEvent query(Qt::ImCursorPosition);
    QCoreApplication::sendEvent(input, &query);
    int caretPos = query.value(Qt::ImCursorPosition).toInt();
    caret_event_t caretEvent;
    memset(&caretEvent, 0, sizeof(caret_event_t));
    initEvent(&caretEvent.event, sInputSession, EVENT_CARET, CARET_POS_CHANGED);
    caretEvent.old_pos = m_lastCaretPos;
    m_lastCaretPos = caretEvent.new_pos = caretPos;
    p_ictrl_dispatch_event((event_t *)&caretEvent);
    return true;
}

bool QQnxInputContext::dispatchCloseSoftwareInputPanel()
{
    m_virtualKeyboard.hideKeyboard();
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << "QQNX: hiding virtual keyboard";
#endif

    // This also means we are stopping composition, but we should already have done that.
    return true;
}

/**
 * IMF Event Dispatchers.
 */
bool QQnxInputContext::dispatchFocusEvent(FocusEventId id, int hints)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!sInputSession) {
        qWarning() << Q_FUNC_INFO << "Attempt to dispatch a focus event with no input session.";
        return false;
    }

    if (!imfAvailable())
        return false;

    // Set the last caret position to 0 since we don't really have one and we don't
    // want to have the old one.
    m_lastCaretPos = 0;

    focus_event_t focusEvent;
    memset(&focusEvent, 0, sizeof(focusEvent));
    initEvent(&focusEvent.event, sInputSession, EVENT_FOCUS, id);
    focusEvent.style = DEFAULT_STYLE;

    if (hints && Qt::ImhNoPredictiveText)
        focusEvent.style |= NO_PREDICTION | NO_AUTO_CORRECTION;
    if (hints && Qt::ImhNoAutoUppercase)
        focusEvent.style |= NO_AUTO_TEXT;

    p_ictrl_dispatch_event((event_t *)&focusEvent);

    return true;
}

bool QQnxInputContext::handleKeyboardEvent(int flags, int sym, int mod, int scan, int cap)
{
    if (!imfAvailable())
        return false;

    int key = (flags & KEY_SYM_VALID) ? sym : cap;
    bool navKey = false;
    switch ( key ) {
    case KEYCODE_RETURN:
         /* In a single line edit we should end composition because enter might be used by something.
            endComposition();
            return false;*/
        break;

    case KEYCODE_BACKSPACE:
    case KEYCODE_DELETE:
        // If there is a selection range, then we want a delete key to operate on that (by
        // deleting the contents of the select range) rather than operating on the composition
        // range.
        if (hasSelectedText())
            return false;
        break;
    case  KEYCODE_LEFT:
        key = NAVIGATE_LEFT;
        navKey = true;
        break;
    case  KEYCODE_RIGHT:
        key = NAVIGATE_RIGHT;
        navKey = true;
        break;
    case  KEYCODE_UP:
        key = NAVIGATE_UP;
        navKey = true;
        break;
    case  KEYCODE_DOWN:
        key = NAVIGATE_DOWN;
        navKey = true;
        break;
    case  KEYCODE_CAPS_LOCK:
    case  KEYCODE_LEFT_SHIFT:
    case  KEYCODE_RIGHT_SHIFT:
    case  KEYCODE_LEFT_CTRL:
    case  KEYCODE_RIGHT_CTRL:
    case  KEYCODE_LEFT_ALT:
    case  KEYCODE_RIGHT_ALT:
    case  KEYCODE_MENU:
    case  KEYCODE_LEFT_HYPER:
    case  KEYCODE_RIGHT_HYPER:
    case  KEYCODE_INSERT:
    case  KEYCODE_HOME:
    case  KEYCODE_PG_UP:
    case  KEYCODE_END:
    case  KEYCODE_PG_DOWN:
        // Don't send these
        key = 0;
        break;
    }

    if ( mod & KEYMOD_CTRL ) {
        // If CTRL is pressed, just let AIR handle it.  But terminate any composition first
        //endComposition();
        return false;
    }

    // Pass the keys we don't know about on through
    if ( key == 0 )
        return false;

    // IMF doesn't need key releases so just swallow them.
    if (!(flags & KEY_DOWN))
        return true;

    if ( navKey ) {
        // Even if we're forwarding up events, we can't do this for
        // navigation keys.
        if ( flags & KEY_DOWN ) {
            navigation_event_t navEvent;
            initEvent(&navEvent.event, sInputSession, EVENT_NAVIGATION, key);
            navEvent.magnitude = 1;
#if defined(QQNXINPUTCONTEXT_DEBUG)
            qDebug() << Q_FUNC_INFO << "dispatch navigation event " << key;
#endif
            p_ictrl_dispatch_event(&navEvent.event);
        }
    }
    else {
        key_event_t keyEvent;
        initEvent(&keyEvent.event, sInputSession, EVENT_KEY, flags & KEY_DOWN ? IMF_KEY_DOWN : IMF_KEY_UP);
        keyEvent.key_code = key;
        keyEvent.character = 0;
        keyEvent.meta_key_state = 0;

        p_ictrl_dispatch_event(&keyEvent.event);
#if defined(QQNXINPUTCONTEXT_DEBUG)
        qDebug() << Q_FUNC_INFO << "dispatch key event " << key;
#endif
    }

    scan = 0;
    return true;
}

void QQnxInputContext::endComposition()
{
    if (!m_isComposing)
        return;

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return;

    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event(QLatin1String(""), attributes);
    event.setCommitString(m_composingText);
    m_composingText = QString();
    m_isComposing = false;
    QCoreApplication::sendEvent(input, &event);

    action_event_t actionEvent;
    memset(&actionEvent, 0, sizeof(actionEvent));
    initEvent(&actionEvent.event, sInputSession, EVENT_ACTION, ACTION_END_COMPOSITION);
    p_ictrl_dispatch_event(&actionEvent.event);
}

void QQnxInputContext::setComposingText(QString const& composingText)
{
    m_composingText = composingText;
    m_isComposing = true;

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return;

    QList<QInputMethodEvent::Attribute> attributes;
    QTextCharFormat format;
    format.setFontUnderline(true);
    attributes.push_back(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 0, composingText.length(), format));

    QInputMethodEvent event(composingText, attributes);

    QCoreApplication::sendEvent(input, &event);
}

int32_t QQnxInputContext::processEvent(event_t *event)
{
    int32_t result = -1;
    switch (event->event_type) {
    case EVENT_SPELL_CHECK: {
        #if defined(QQNXINPUTCONTEXT_DEBUG)
        qDebug() << Q_FUNC_INFO << "EVENT_SPELL_CHECK";
        #endif
        result = 0;
        break;
    }

    case EVENT_NAVIGATION: {
        #if defined(QQNXINPUTCONTEXT_DEBUG)
        qDebug() << Q_FUNC_INFO << "EVENT_NAVIGATION";
        #endif

        int key = event->event_id == NAVIGATE_UP ? KEYCODE_UP :
            event->event_id == NAVIGATE_DOWN ? KEYCODE_DOWN :
            event->event_id == NAVIGATE_LEFT ? KEYCODE_LEFT :
            event->event_id == NAVIGATE_RIGHT ? KEYCODE_RIGHT : 0;

        QQnxEventThread::injectKeyboardEvent(KEY_DOWN | KEY_CAP_VALID, key, 0, 0, 0);
        QQnxEventThread::injectKeyboardEvent(KEY_CAP_VALID, key, 0, 0, 0);
        result = 0;
        break;
    }

    case EVENT_KEY: {
        #if defined(QQNXINPUTCONTEXT_DEBUG)
        qDebug() << Q_FUNC_INFO << "EVENT_KEY";
        #endif
        key_event_t *kevent = static_cast<key_event_t *>(event);

        QQnxEventThread::injectKeyboardEvent(KEY_DOWN | KEY_SYM_VALID | KEY_CAP_VALID, kevent->key_code, 0, 0, kevent->key_code);
        QQnxEventThread::injectKeyboardEvent(KEY_SYM_VALID | KEY_CAP_VALID, kevent->key_code, 0, 0, kevent->key_code);

        result = 0;
        break;
    }

    case EVENT_ACTION:
            // Don't care, indicates that IMF is done.
        break;

    case EVENT_CARET:
    case EVENT_NOTHING:
    case EVENT_FOCUS:
    case EVENT_USER_ACTION:
    case EVENT_STROKE:
    case EVENT_INVOKE_LATER:
        qCritical() << Q_FUNC_INFO << "Unsupported event type: " << event->event_type;
        break;
    default:
        qCritical() << Q_FUNC_INFO << "Unknown event type: " << event->event_type;
    }
    return result;
}

/**
 * IMF Event Handlers
 */

int32_t QQnxInputContext::onBeginBatchEdit(input_session_t *ic)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    // We don't care.
    return 0;
}

int32_t QQnxInputContext::onClearMetaKeyStates(input_session_t *ic, int32_t states)
{
    Q_UNUSED(states);
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    // Should never get called.
    qCritical() << Q_FUNC_INFO << "onClearMetaKeyStates is unsupported.";
    return 0;
}

int32_t QQnxInputContext::onCommitText(input_session_t *ic, spannable_string_t *text, int32_t new_cursor_position)
{
    Q_UNUSED(new_cursor_position);  // TODO: How can we set the cursor position it's not part of the API.
    if (!isSessionOkay(ic))
        return 0;

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return 0;

    QString commitString = QString::fromWCharArray(text->str, text->length);

#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO << "Committing [" << commitString << "]";
#endif

    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event(QLatin1String(""), attributes);
    event.setCommitString(commitString, 0, 0);

    QCoreApplication::sendEvent(input, &event);
    m_composingText = QString();

    return 0;
}

int32_t QQnxInputContext::onDeleteSurroundingText(input_session_t *ic, int32_t left_length, int32_t right_length)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO << "L:" << left_length << " R:" << right_length;
#endif

    if (!isSessionOkay(ic))
        return 0;

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return 0;

    if (hasSelectedText()) {
        QQnxEventThread::injectKeyboardEvent(KEY_DOWN | KEY_CAP_VALID, KEYCODE_DELETE, 0, 0, 0);
        QQnxEventThread::injectKeyboardEvent(KEY_CAP_VALID, KEYCODE_DELETE, 0, 0, 0);
        reset();
        return 0;
    }

    int replacementLength = left_length + right_length;
    int replacementStart = -left_length;

    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event(QLatin1String(""), attributes);
    event.setCommitString(QLatin1String(""), replacementStart, replacementLength);
    QCoreApplication::sendEvent(input, &event);

    return 0;
}

int32_t QQnxInputContext::onEndBatchEdit(input_session_t *ic)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    return 0;
}

int32_t QQnxInputContext::onFinishComposingText(input_session_t *ic)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return 0;

    // Only update the control, no need to send a message back to imf (don't call
    // end composition)
    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event(QLatin1String(""), attributes);
    event.setCommitString(m_composingText);
    m_composingText = QString();
    m_isComposing = false;
    QCoreApplication::sendEvent(input, &event);

    return 0;
}

int32_t QQnxInputContext::onGetCursorCapsMode(input_session_t *ic, int32_t req_modes)
{
    Q_UNUSED(req_modes);
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    // Should never get called.
    qCritical() << Q_FUNC_INFO << "onGetCursorCapsMode is unsupported.";

    return 0;
}

int32_t QQnxInputContext::onGetCursorPosition(input_session_t *ic)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return 0;

    QInputMethodQueryEvent query(Qt::ImCursorPosition);
    QCoreApplication::sendEvent(input, &query);
    m_lastCaretPos = query.value(Qt::ImCursorPosition).toInt();

    return m_lastCaretPos;
}

extracted_text_t *QQnxInputContext::onGetExtractedText(input_session_t *ic, extracted_text_request_t *request, int32_t flags)
{
    Q_UNUSED(flags);
    Q_UNUSED(request);
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic)) {
        extracted_text_t *et = (extracted_text_t *)calloc(sizeof(extracted_text_t),1);
        et->text = reinterpret_cast<spannable_string_t *>(calloc(sizeof(spannable_string_t),1));
        return et;
    }

    // Used to update dictionaries, but not supported right now.
    extracted_text_t *et = (extracted_text_t *)calloc(sizeof(extracted_text_t),1);
    et->text = reinterpret_cast<spannable_string_t *>(calloc(sizeof(spannable_string_t),1));

    return et;
}

spannable_string_t *QQnxInputContext::onGetSelectedText(input_session_t *ic, int32_t flags)
{
    Q_UNUSED(flags);
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return toSpannableString("");

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return 0;

    QInputMethodQueryEvent query(Qt::ImCurrentSelection);
    QCoreApplication::sendEvent(input, &query);
    QString text = query.value(Qt::ImCurrentSelection).toString();

    return toSpannableString(text);
}

spannable_string_t *QQnxInputContext::onGetTextAfterCursor(input_session_t *ic, int32_t n, int32_t flags)
{
    Q_UNUSED(flags);
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return toSpannableString("");

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return toSpannableString("");

    QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImSurroundingText);
    QCoreApplication::sendEvent(input, &query);
    QString text = query.value(Qt::ImSurroundingText).toString();
    m_lastCaretPos = query.value(Qt::ImCursorPosition).toInt();

    return toSpannableString(text.mid(m_lastCaretPos+1, n));
}

spannable_string_t *QQnxInputContext::onGetTextBeforeCursor(input_session_t *ic, int32_t n, int32_t flags)
{
    Q_UNUSED(flags);
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return toSpannableString("");

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return toSpannableString("");

    QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImSurroundingText);
    QCoreApplication::sendEvent(input, &query);
    QString text = query.value(Qt::ImSurroundingText).toString();
    m_lastCaretPos = query.value(Qt::ImCursorPosition).toInt();

    if (n < m_lastCaretPos) {
        return toSpannableString(text.mid(m_lastCaretPos - n, n));
    } else {
        return toSpannableString(text.mid(0, m_lastCaretPos));
    }
}

int32_t QQnxInputContext::onPerformEditorAction(input_session_t *ic, int32_t editor_action)
{
    Q_UNUSED(editor_action);
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    // Should never get called.
    qCritical() << Q_FUNC_INFO << "onPerformEditorAction is unsupported.";

    return 0;
}

int32_t QQnxInputContext::onReportFullscreenMode(input_session_t *ic, int32_t enabled)
{
    Q_UNUSED(enabled);
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    // Should never get called.
    qCritical() << Q_FUNC_INFO << "onReportFullscreenMode is unsupported.";

    return 0;
}

int32_t QQnxInputContext::onSendEvent(input_session_t *ic, event_t *event)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    return processEvent(event);
}

int32_t QQnxInputContext::onSendAsyncEvent(input_session_t *ic, event_t *event)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    return processEvent(event);
}

int32_t QQnxInputContext::onSetComposingRegion(input_session_t *ic, int32_t start, int32_t end)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return 0;

    QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImSurroundingText);
    QCoreApplication::sendEvent(input, &query);
    QString text = query.value(Qt::ImSurroundingText).toString();
    m_lastCaretPos = query.value(Qt::ImCursorPosition).toInt();

    QString empty = QString::fromLatin1("");
    text = text.mid(start, end - start);

    // Delete the current text.
    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event(empty, attributes);
    event.setCommitString(empty, start - m_lastCaretPos, end - start);
    QCoreApplication::sendEvent(input, &event);

    // Move the specified text into a preedit string.
    setComposingText(text);

    return 0;
}

int32_t QQnxInputContext::onSetComposingText(input_session_t *ic, spannable_string_t *text, int32_t new_cursor_position)
{
    Q_UNUSED(new_cursor_position);
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    QObject *input = qGuiApp->focusObject();
    if (!imfAvailable() || !input)
        return 0;

    m_isComposing = true;

    QString preeditString = QString::fromWCharArray(text->str, text->length);
    setComposingText(preeditString);

    return 0;
}

int32_t QQnxInputContext::onSetSelection(input_session_t *ic, int32_t start, int32_t end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!isSessionOkay(ic))
        return 0;

    // Should never get called.
    qCritical() << Q_FUNC_INFO << "onSetSelection is unsupported.";

    return 0;
}

void QQnxInputContext::showInputPanel()
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    dispatchRequestSoftwareInputPanel();
}

void QQnxInputContext::hideInputPanel()
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    dispatchCloseSoftwareInputPanel();
}

bool QQnxInputContext::isInputPanelVisible() const
{
    return m_inputPanelVisible;
}

QLocale QQnxInputContext::locale() const
{
    return m_inputPanelLocale;
}

void QQnxInputContext::keyboardVisibilityChanged(bool visible)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO << "visible=" << visible;
#endif
    if (m_inputPanelVisible != visible) {
        m_inputPanelVisible = visible;
        emitInputPanelVisibleChanged();
    }
}

void QQnxInputContext::keyboardLocaleChanged(const QLocale &locale)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO << "locale=" << locale;
#endif
    if (m_inputPanelLocale != locale) {
        m_inputPanelLocale = locale;
        emitLocaleChanged();
    }
}

void QQnxInputContext::setFocusObject(QObject *object)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO << "input item=" << object;
#endif

    if (!inputMethodAccepted()) {
        if (m_inputPanelVisible)
            hideInputPanel();
    } else {
        if (qobject_cast<QAbstractSpinBox*>(object))
            m_virtualKeyboard.setKeyboardMode(QQnxAbstractVirtualKeyboard::Phone);
        else
            m_virtualKeyboard.setKeyboardMode(QQnxAbstractVirtualKeyboard::Default);

        if (!m_inputPanelVisible)
            showInputPanel();
    }
}

QT_END_NAMESPACE
