// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosinputcontext.h"
#include <QtCore/private/qnapi_p.h>
#include <qohosjsenv_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qnamespace.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include "qohosjsmain.h"
#include "qohosplatformintegration.h"
#include "qohosplatformscreen.h"
#include <QRect>
#include <QInputDevice>
#include <QGuiApplication>
#include <QTextCharFormat>
#include <algorithm>
#include <qohosjsutils.h>
#include <inputmethod/inputmethod_controller_capi.h>
QT_BEGIN_NAMESPACE

namespace {

const Qt::InputMethodHints defaultInputMethodHints = Qt::ImhNone;
const Qt::EnterKeyType defaultEnterKeyType = Qt::EnterKeyDefault;

using InsertedTextId = QtOhos::TypedId<std::uint64_t, struct InsertedTextIdTag>;

class QOhosInsertedText
{
public:
    QOhosInsertedText(const std::string &text, const InsertedTextId &id);

    InsertedTextId id() const;
    std::string text() const;

private:
    InsertedTextId m_id;
    std::string m_text;
};

class JsInputMethodInsertedTextComposer
{
public:
    static JsInputMethodInsertedTextComposer &instance();

    JsInputMethodInsertedTextComposer(const JsInputMethodInsertedTextComposer &) = delete;
    JsInputMethodInsertedTextComposer(JsInputMethodInsertedTextComposer &&) = delete;
    JsInputMethodInsertedTextComposer &operator=(const JsInputMethodInsertedTextComposer &) = delete;
    JsInputMethodInsertedTextComposer &operator=(JsInputMethodInsertedTextComposer &&) = delete;

    QOhosOptional<InsertedTextId> lastId() const;
    QOhosInsertedText makeInsertedText(const std::string &text);

private:
    JsInputMethodInsertedTextComposer() = default;
    void initOrIncrementId();

    QOhosOptional<InsertedTextId> m_id;
};

QOhosInsertedText::QOhosInsertedText(const std::string &text, const InsertedTextId &id)
    : m_id(id)
    , m_text(text)
{}

InsertedTextId QOhosInsertedText::id() const
{
    return m_id;
}

std::string QOhosInsertedText::text() const
{
    return m_text;
}

JsInputMethodInsertedTextComposer &JsInputMethodInsertedTextComposer::instance()
{
    static JsInputMethodInsertedTextComposer imHelper;
    return imHelper;
}

QOhosOptional<InsertedTextId> JsInputMethodInsertedTextComposer::lastId() const
{
    return m_id;
}

void JsInputMethodInsertedTextComposer::initOrIncrementId()
{
    if (!m_id.hasValue()) {
        m_id = InsertedTextId(0);
    } else {
        auto oldValue = m_id.value().value();
        m_id = InsertedTextId(++oldValue);
    }
}

QOhosInsertedText JsInputMethodInsertedTextComposer::makeInsertedText(const std::string &text)
{
    initOrIncrementId();
    return QOhosInsertedText(text, m_id.value());
}

::InputMethod_TextInputType mapQtInputMethodHintsToOhosImeTextInputType(Qt::InputMethodHints hints)
{
    using TextInputType = ::InputMethod_TextInputType;
    if (hints & Qt::ImhMultiLine) {
        return TextInputType::IME_TEXT_INPUT_TYPE_MULTILINE;
    } else if (hints & Qt::ImhDigitsOnly) {
        return (hints & Qt::ImhHiddenText)
            ? TextInputType::IME_TEXT_INPUT_TYPE_NUMBER_PASSWORD
            : TextInputType::IME_TEXT_INPUT_TYPE_NUMBER;
    } else if (hints & Qt::ImhDialableCharactersOnly) {
        return TextInputType::IME_TEXT_INPUT_TYPE_PHONE;
    } else if (hints & (Qt::ImhDate | Qt::ImhTime)) {
        return TextInputType::IME_TEXT_INPUT_TYPE_DATETIME;
    } else if (hints & Qt::ImhEmailCharactersOnly) {
        return TextInputType::IME_TEXT_INPUT_TYPE_EMAIL_ADDRESS;
    } else if (hints & Qt::ImhUrlCharactersOnly) {
        return TextInputType::IME_TEXT_INPUT_TYPE_URL;
    } else if (hints & Qt::ImhHiddenText) {
        return TextInputType::IME_TEXT_INPUT_TYPE_VISIBLE_PASSWORD;
    } else {
        return TextInputType::IME_TEXT_INPUT_TYPE_TEXT;
    }
}

QOhosOptional<QOhosInputContext::Direction> tryMapInputMethodDirectionToQt(::InputMethod_Direction direction)
{
    switch (direction) {
    case ::InputMethod_Direction::IME_DIRECTION_NONE:
        return makeEmptyQOhosOptional();
    case ::InputMethod_Direction::IME_DIRECTION_UP:
        return makeQOhosOptional(QOhosInputContext::Direction::CURSOR_UP);
    case ::InputMethod_Direction::IME_DIRECTION_DOWN:
        return makeQOhosOptional(QOhosInputContext::Direction::CURSOR_DOWN);
    case ::InputMethod_Direction::IME_DIRECTION_LEFT:
        return makeQOhosOptional(QOhosInputContext::Direction::CURSOR_LEFT);
    case ::InputMethod_Direction::IME_DIRECTION_RIGHT:
        return makeQOhosOptional(QOhosInputContext::Direction::CURSOR_RIGHT);
    }
    return makeEmptyQOhosOptional();
}

::InputMethod_EnterKeyType mapQtToOhosImeEnterKeyType(Qt::EnterKeyType qtEnterKeyType)
{
    switch (qtEnterKeyType) {
    case Qt::EnterKeyType::EnterKeyReturn:
        return ::InputMethod_EnterKeyType::IME_ENTER_KEY_NEWLINE;
    case Qt::EnterKeyType::EnterKeyDone:
        return ::InputMethod_EnterKeyType::IME_ENTER_KEY_DONE;
    case Qt::EnterKeyType::EnterKeyGo:
        return ::InputMethod_EnterKeyType::IME_ENTER_KEY_GO;
    case Qt::EnterKeyType::EnterKeySend:
        return ::InputMethod_EnterKeyType::IME_ENTER_KEY_SEND;
    case Qt::EnterKeyType::EnterKeySearch:
        return ::InputMethod_EnterKeyType::IME_ENTER_KEY_SEARCH;
    case Qt::EnterKeyType::EnterKeyNext:
        return ::InputMethod_EnterKeyType::IME_ENTER_KEY_NEXT;
    case Qt::EnterKeyType::EnterKeyPrevious:
        return ::InputMethod_EnterKeyType::IME_ENTER_KEY_PREVIOUS;
    case Qt::EnterKeyType::EnterKeyDefault:
        return ::InputMethod_EnterKeyType::IME_ENTER_KEY_NONE;
    }
    qOhosPrintfError(
        "%s: Cannot map Qt::EnterKeyType to OHOS value. Returning ::InputMethod_EnterKeyType::IME_ENTER_KEY_UNSPECIFIED",
        Q_FUNC_INFO);
    return ::InputMethod_EnterKeyType::IME_ENTER_KEY_UNSPECIFIED;
}

::InputMethod_RequestKeyboardReason mapQtToOhosImeRequestReason(
    QOhosInputContext::RequestKeyboardReason qtRequestKeyboardReason)
{
    switch (qtRequestKeyboardReason) {
    case QOhosInputContext::RequestKeyboardReason::NONE:
        return ::InputMethod_RequestKeyboardReason::IME_REQUEST_REASON_NONE;
    case QOhosInputContext::RequestKeyboardReason::MOUSE:
        return ::InputMethod_RequestKeyboardReason::IME_REQUEST_REASON_MOUSE;
    case QOhosInputContext::RequestKeyboardReason::TOUCH:
        return ::InputMethod_RequestKeyboardReason::IME_REQUEST_REASON_TOUCH;
    case QOhosInputContext::RequestKeyboardReason::OTHER:
        return ::InputMethod_RequestKeyboardReason::IME_REQUEST_REASON_OTHER;
    }
    qOhosPrintfError(
        "%s: Cannot map QOhosInputContext::RequestKeyboardReason to OHOS value. Returning ::InputMethod_RequestKeyboardReason::IME_REQUEST_REASON_NONE",
        Q_FUNC_INFO);
    return ::InputMethod_RequestKeyboardReason::IME_REQUEST_REASON_NONE;
}

QOhosOptional<int> tryGetIntPropertyFromQuery(Qt::InputMethodQuery property, QSharedPointer<QInputMethodQueryEvent> query)
{
    bool converted;
    const auto value = query->value(property).toInt(&converted);
    return converted ? makeQOhosOptional(value) : makeEmptyQOhosOptional();
}

void notifyOhosInputMethodAboutPossibleAutocorrection(const QOhosInsertedText &insertedText, int cursorPosition)
{
    auto lastInsertedTextId = JsInputMethodInsertedTextComposer::instance().lastId();
    if (!lastInsertedTextId.hasValue()) {
        qOhosPrintfError("%s: JsInputMethodInsertedTextComposer has no last inserted text ID", Q_FUNC_INFO);
        return;
    }

    auto currentInsertedTextId = insertedText.id();
    if (currentInsertedTextId != lastInsertedTextId.value()) {
        qOhosPrintfWarning(
            "%s: inserted text and one currently processed differ from each other, system won't be notified with changeSelection()", Q_FUNC_INFO);
        return;
    }

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
            auto startPosition = cursorPosition;
            auto endPosition = cursorPosition + insertedText.text().length();
            jsState.evalToPromiseOrRejectOnThrow(
                "@ohos.inputMethod.getController().changeSelection(*)", {insertedText.text(), startPosition, endPosition})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("changeSelection()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

inline QPoint clampToRect(const QPoint &p, const QRect &rect)
{
    int x = qBound(rect.left(), p.x(), rect.right());
    int y = qBound(rect.top(), p.y(), rect.bottom());
    return QPoint(x, y);
}

}

QOhosInputContext::QOhosInputContext()
    : QPlatformInputContext()
    , m_lastCursorRectangle(0, 0, 0, 0)
    , m_qtImEnabled(false)
    , m_qtInputMethodHints(defaultInputMethodHints)
    , m_qtEnterKeyType(defaultEnterKeyType)
    , m_jsScopeData(
        QtOhos::makeProxyWithJsThreadDeleter(
            std::make_shared<JsScopeData>()))
{
    auto __dbg = make_QCScopedDebug("QOhosInputContext::QOhosInputContext");

    QObject::connect(
        QGuiApplication::inputMethod(), &QInputMethod::cursorRectangleChanged,
        this, &QOhosInputContext::onCursorRectangleChanged);

    QGuiApplication::instance()->installEventFilter(this);
}

QOhosInputContext::~QOhosInputContext() = default;

void QOhosInputContext::reset()
{
    if (m_imConnectionState == ImConnectionState::Attached) {
        setImConnectionState(ImConnectionState::Detached);
        setImConnectionState(ImConnectionState::Attached);
    }
}

void QOhosInputContext::commit()
{
    auto __dbg = make_QCScopedDebug("QOhosInputContext::commit");

    auto preeditText = std::exchange(m_pendingPreeditText, {});

    QInputMethodEvent event;
    event.setCommitString(preeditText);
    sendFocusObjectInputMethodEvent(&event);
}

void QOhosInputContext::update(Qt::InputMethodQueries queries)
{
    if (!focusObjectOrNull()) {
        hideInputPanel();
        return;
    }

    m_qtImEnabled = queryImEnabled();
    const auto qtInputMethodHintsValue = queryInputMethodHints();
    const auto qtEnterKeyTypeValue = queryEnterKeyType();

    const bool imStateChanged =
        m_qtInputMethodHints != qtInputMethodHintsValue
        || m_qtEnterKeyType != qtEnterKeyTypeValue;

    auto imAlreadyAttached = m_imConnectionState == ImConnectionState::Attached;
    if (imAlreadyAttached && imStateChanged)
        updateInputMethodControllerAttributes(qtInputMethodHintsValue, qtEnterKeyTypeValue);

    m_qtInputMethodHints = qtInputMethodHintsValue;
    m_qtEnterKeyType = qtEnterKeyTypeValue;

    if (inputMethodAccepted() && m_qtImEnabled) {
        auto updateCausedByWidgetTransform = queries == Qt::ImInputItemClipRectangle;
        auto updateCausedByQueryAllImParameters = queries == Qt::ImQueryAll;

        if (updateCausedByQueryAllImParameters)
            return;

        if (!updateCausedByWidgetTransform || !imAlreadyAttached) {
            showInputPanel();
        }
    } else {
        hideInputPanel();
    }
}

void QOhosInputContext::invokeAction(QInputMethod::Action, int)
{
    auto __dbg = make_QCScopedDebug("QOhosInputContext::invokeAction");
}

QRectF QOhosInputContext::keyboardRect() const
{
    auto __dbg = make_QCScopedDebug("QOhosInputContext::keyboardRect");
    return {};
}

bool QOhosInputContext::isAnimating() const
{
    auto __dbg = make_QCScopedDebug("QOhosInputContext::isAnimating");
    return {};
}

void QOhosInputContext::showInputPanel()
{
    auto __dbg = make_QCScopedDebug("QOhosInputContext::showInputPanel");
    setImConnectionState(ImConnectionState::Attached);
}

void QOhosInputContext::hideInputPanel()
{
    auto __dbg = make_QCScopedDebug("QOhosInputContext::hideInputPanel");
    setImConnectionState(ImConnectionState::Detached);
}

bool QOhosInputContext::isInputPanelVisible() const
{
    auto __dbg = make_QCScopedDebug("QOhosInputContext::isInputPanelVisible");
    return m_imConnectionRequestedState == ImConnectionState::Attached;
}

void QOhosInputContext::setFocusObject(QObject *object)
{
    m_focusObject = object;
}

QObject *QOhosInputContext::focusObjectOrNull() const
{
    return m_focusObject;
}

void QOhosInputContext::setImConnectionState(ImConnectionState requestedState)
{
    if (requestedState != m_imConnectionRequestedState) {
        setImConnectionStateImpl(requestedState);
    } else {
        if (m_imConnectionState == ImConnectionState::Attached && !m_softwareKeyboardVisible)
            showTextInput();
    }
}

void QOhosInputContext::setImConnectionStateImpl(ImConnectionState requestedState)
{
    m_imConnectionRequestedState = m_qtImEnabled ? requestedState : ImConnectionState::Detached;

    if (!m_imConnectionRequestActive) {
        m_imConnectionRequestActive = true;

        dispatchRequestedImStateChange(requestedState);
    }
}

void QOhosInputContext::dispatchRequestedImStateChange(ImConnectionState requestedState)
{
    auto result = false;
    if (requestedState == ImConnectionState::Attached)
        result = attachToInputMethodController();
    else
        result = detachFromInputMethodController();
    handleRequestedImConnectionState(requestedState, result);
}

bool QOhosInputContext::attachToInputMethodController()
{
    class ImCallbacks : public QOhosInputMethodProxy::ClientCallbacks
    {
    public:
        ImCallbacks(QOhosInputContext &inputContext)
            : m_inputContext(inputContext)
        {
        }

    private:
        void onInsertText(std::string text) override
        {
            m_inputContext.sendInsertedTextToQt(std::move(text));
        }

        void onInsertPreviewText(std::string previewText) override
        {
            m_inputContext.sendInsertedPreviewTextToQt(std::move(previewText));
        }

        void onFinishPreviewText() override
        {
            m_inputContext.commit();
        }

        void onDeleteForward(int length) override
        {
            m_inputContext.sendFocusObjectFunctionalKeyEvent(Qt::Key_Delete, QChar::fromLatin1('\u007F'), length);
        }

        void onDeleteBackward(int length) override
        {
            m_inputContext.sendFocusObjectFunctionalKeyEvent(Qt::Key_Backspace, QChar::fromLatin1('\u0008'), length);
        }

        void onSendKeyboardStatus(::InputMethod_KeyboardStatus keyboardStatus) override
        {
            bool ohosKeyboardShown = keyboardStatus == ::InputMethod_KeyboardStatus::IME_KEYBOARD_STATUS_SHOW;
            m_inputContext.setSoftwareKeyboardVisibilityStatus(ohosKeyboardShown);
        }

        void onSendEnterKey(::InputMethod_EnterKeyType) override
        {
            m_inputContext.sendFocusObjectFunctionalKeyEvent(Qt::Key_Enter, QChar::fromLatin1('\u000D'));
        }

        void onMoveCursor(::InputMethod_Direction direction) override
        {
            auto qtDirection = tryMapInputMethodDirectionToQt(direction);
            if (!qtDirection.hasValue()) {
                qOhosPrintfWarning(
                    "got unsupported InputMethod_Direction value (%d), cursor won't be moved!",
                    static_cast<int>(direction));
                return;
            }

            m_inputContext.sendCursorMoveToQt(qtDirection.value());
        }

        QOhosInputContext &m_inputContext;
    };

    if (m_imProxy)
        qOhosPrintfWarning("%s: proxy already exists!", Q_FUNC_INFO);

    m_imProxy = std::make_shared<QOhosInputMethodProxy>(
        std::make_shared<ImCallbacks>(*this),
        mapQtToOhosImeRequestReason(
            m_lastInputTypeToTriggerSoftKeyboard.valueOr(RequestKeyboardReason::OTHER)));

    if (m_imProxy->hasAttachedSuccessfully()) {
        m_imProxy->notifyConfigurationChange(
            mapQtToOhosImeEnterKeyType(m_qtEnterKeyType),
            mapQtInputMethodHintsToOhosImeTextInputType(m_qtInputMethodHints));
        return true;
    }
    return false;
}

bool QOhosInputContext::detachFromInputMethodController()
{
    if (!m_imProxy)
        qOhosPrintfWarning("%s: proxy doesn't exist!", Q_FUNC_INFO);

    m_imProxy.reset();
    return true;
}

void QOhosInputContext::handleRequestedImConnectionState(ImConnectionState requestedState, bool success)
{
    m_imConnectionRequestActive = false;

    if (!success) {
        qOhosWarning(QtForOhos)
            << "Cannot "
            << (requestedState == ImConnectionState::Attached ? "attach to" : "detach from")
            << " IMC!";
        return;
    }

    m_imConnectionState = requestedState;

    if (m_imConnectionRequestedState != m_imConnectionState)
        setImConnectionStateImpl(m_imConnectionRequestedState);

    onCursorRectangleChanged();
}

void QOhosInputContext::showTextInput()
{
    if (m_imProxy == nullptr) {
        qOhosPrintfError("%s: proxy doesn't exist!", Q_FUNC_INFO);
        return;
    }

    m_imProxy->showTextInput(mapQtToOhosImeRequestReason(
        m_lastInputTypeToTriggerSoftKeyboard.valueOr(RequestKeyboardReason::OTHER)));
}

void QOhosInputContext::setSoftwareKeyboardVisibilityStatus(bool visible)
{
    m_softwareKeyboardVisible = visible;
}

void QOhosInputContext::setLastInputTypeToTriggerSoftKeyboard(RequestKeyboardReason inputType)
{
    m_lastInputTypeToTriggerSoftKeyboard = inputType;
}

void QOhosInputContext::onCursorRectangleChanged()
{
    if (m_imConnectionState == ImConnectionState::Detached) {
        qOhosWarning(QtForOhos) << "Attempting to update cursor position when detached from controller";
        m_updateCursorRectangleAfterAttaching = true;
        return;
    }

    auto *focusedWindow = QGuiApplication::focusWindow();
    if (focusedWindow == nullptr) {
        qOhosCritical(QtForOhos)
            << "Could not retrieve focused window. Updating cursor position isn't possible";
        return;
    }

    auto focusedWindowPosition = focusedWindow->position();
    QRect cursorRectangle = QGuiApplication::inputMethod()->cursorRectangle().toRect();
    if (!m_updateCursorRectangleAfterAttaching
        && cursorRectangle == m_lastCursorRectangle
        && m_lastFocusedWindowPosition == focusedWindowPosition) {
        return;
    }

    m_lastCursorRectangle = cursorRectangle;
    m_lastFocusedWindowPosition = focusedWindowPosition;
    auto globalInputItemRectangle =
        QGuiApplication::inputMethod()->inputItemClipRectangle()
        .translated(m_lastFocusedWindowPosition)
        .toRect();
    auto globalCursorRectangle = m_lastCursorRectangle.translated(m_lastFocusedWindowPosition);
    auto inputItemClampedCursorPos = clampToRect(
        {globalCursorRectangle.x(), globalCursorRectangle.y()},
        globalInputItemRectangle.isValid()
            ? globalInputItemRectangle
            : focusedWindow->geometry());
    auto nativeCursorPos = QHighDpiScaling::mapPositionToNative(
        inputItemClampedCursorPos, focusedWindow->screen()->handle());
    auto screenBasedCursorPos = nativeCursorPos - focusedWindow->screen()->handle()->geometry().topLeft();
    updateOhosCursor({
        screenBasedCursorPos.x(), screenBasedCursorPos.y(),
        globalCursorRectangle.width(), globalCursorRectangle.height()
    });
    m_updateCursorRectangleAfterAttaching = false;
}

void QOhosInputContext::updateInputMethodControllerAttributes(
    Qt::InputMethodHints qtInputMethodHints, Qt::EnterKeyType qtEnterKeyType)
{
    if (m_imProxy == nullptr) {
        qOhosPrintfError("%s: proxy doesn't exist!", Q_FUNC_INFO);
        return;
    }

    m_imProxy->notifyConfigurationChange(
        mapQtToOhosImeEnterKeyType(qtEnterKeyType),
        mapQtInputMethodHintsToOhosImeTextInputType(qtInputMethodHints));
}

void QOhosInputContext::updateOhosCursor(const QRect &globalCursorRect)
{
    if (m_imProxy == nullptr) {
        qOhosPrintfError("%s: proxy doesn't exist!", Q_FUNC_INFO);
        return;
    }

    m_imProxy->notifyCursorUpdate(globalCursorRect);
}

void QOhosInputContext::handleFocusInEvent(QObject *obj, QFocusEvent *event)
{
    if (obj->isWidgetType() && queryImEnabled()) {
        auto focusReason = event->reason();
        if (focusReason == Qt::OtherFocusReason || focusReason == Qt::ActiveWindowFocusReason)
            setLastInputTypeToTriggerSoftKeyboard(RequestKeyboardReason::NONE);
    }
}

bool QOhosInputContext::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusIn)
        handleFocusInEvent(obj, static_cast<QFocusEvent *>(event));
    return false;
}

void QOhosInputContext::sendInsertedTextToQt(const std::string &textToInsert)
{
    auto insertedText = JsInputMethodInsertedTextComposer::instance().makeInsertedText(textToInsert);

    auto query = tryQueryFocusObjectInputMethod(Qt::ImEnabled);
    if (query.isNull())
        return;

    if (!query->value(Qt::ImEnabled).toBool()) {
        qOhosDebug(QtForOhos) << "onInsertText(): focus object is not able to handle InputMethod";
        return;
    }

    m_pendingPreeditText.clear();

    QInputMethodEvent event;
    event.setCommitString(QString::fromStdString(insertedText.text()));
    sendFocusObjectInputMethodEvent(&event);

    auto cursorPosition = tryQueryCursorPosition();
    if (cursorPosition.hasValue())
        notifyOhosInputMethodAboutPossibleAutocorrection(insertedText, cursorPosition.value());
    else
        qOhosWarning(QtForOhos) << "Couldn't obtain IM cursorPosition";
}

void QOhosInputContext::sendInsertedPreviewTextToQt(std::string previewText)
{
    auto query = tryQueryFocusObjectInputMethod(Qt::ImEnabled);
    if (query.isNull())
        return;

    if (!query->value(Qt::ImEnabled).toBool()) {
        qOhosPrintfDebug("%s: focus object is not able to handle InputMethod", Q_FUNC_INFO);
        return;
    }

    const QString previewString = QString::fromStdString(previewText);
    QList<QInputMethodEvent::Attribute> imEventAttributes;
    if (!previewString.isEmpty()) {
        QTextCharFormat format;
        format.setFontUnderline(true);
        imEventAttributes.append(
            QInputMethodEvent::Attribute(
                QInputMethodEvent::TextFormat, 0, previewString.length(), format));
    }

    m_pendingPreeditText = previewString;

    QInputMethodEvent preeditStringEvent(previewString, imEventAttributes);
    sendFocusObjectInputMethodEvent(&preeditStringEvent);
}

void QOhosInputContext::sendFocusObjectInputMethodEvent(QInputMethodEvent *event)
{
    auto *focusObject = focusObjectOrNull();
    if (focusObject == nullptr) {
        qOhosWarning(QtForOhos) << "sendFocusObjectInputMethodEvent(): no focus object to send event to!";
        return;
    }

    QCoreApplication::sendEvent(focusObject, event);
}

void QOhosInputContext::sendCursorMoveToQt(QOhosInputContext::Direction direction)
{
    switch (direction) {
    case QOhosInputContext::Direction::CURSOR_UP:
        sendFocusObjectFunctionalKeyEvent(Qt::Key_Up, QChar(std::int32_t(Qt::Key_Up)));
        break;
    case QOhosInputContext::Direction::CURSOR_DOWN:
        sendFocusObjectFunctionalKeyEvent(Qt::Key_Down, QChar(std::int32_t(Qt::Key_Down)));
        break;
    case QOhosInputContext::Direction::CURSOR_LEFT:
        sendFocusObjectFunctionalKeyEvent(Qt::Key_Left, QChar(std::int32_t(Qt::Key_Left)));
        break;
    case QOhosInputContext::Direction::CURSOR_RIGHT:
        sendFocusObjectFunctionalKeyEvent(Qt::Key_Right, QChar(std::int32_t(Qt::Key_Right)));
        break;
    }
}

void QOhosInputContext::sendFocusObjectFunctionalKeyEvent(Qt::Key key, const QChar &keyChar, int repeatCount)
{
    auto *focusObject = focusObjectOrNull();
    if (focusObject == nullptr) {
        qOhosWarning(QtForOhos) << "sendFocusObjectFunctionalKeyEvent(): no focus object to send event to!";
        return;
    }

    for (int i = 0; i < repeatCount; ++i) {
        QGuiApplication::postEvent(focusObject, new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier, QString(keyChar)));
        QGuiApplication::postEvent(focusObject, new QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier, QString(keyChar)));
    }
}

bool QOhosInputContext::queryImEnabled() const
{
    auto query = tryQueryFocusObjectInputMethod(Qt::ImEnabled);
    return !query.isNull() && query->value(Qt::ImEnabled).toBool();
}

Qt::InputMethodHints QOhosInputContext::queryInputMethodHints() const
{
    QOhosOptional<Qt::InputMethodHints> hints;
    auto query = tryQueryFocusObjectInputMethod(Qt::ImHints);
    if (!query.isNull()) {
        auto imHintsInt = tryGetIntPropertyFromQuery(Qt::ImHints, query);
        if (imHintsInt.hasValue())
            hints = static_cast<Qt::InputMethodHints>(imHintsInt.value());
    }

    return hints.valueOr(defaultInputMethodHints);
}

Qt::EnterKeyType QOhosInputContext::queryEnterKeyType() const
{
    QOhosOptional<Qt::EnterKeyType> enterKeyType;
    auto query = tryQueryFocusObjectInputMethod(Qt::ImEnterKeyType);
    if (!query.isNull()) {
        auto imEnterKeyTypeInt = tryGetIntPropertyFromQuery(Qt::ImEnterKeyType, query);
        if (imEnterKeyTypeInt.hasValue())
            enterKeyType = static_cast<Qt::EnterKeyType>(imEnterKeyTypeInt.value());
    }

    return enterKeyType.valueOr(defaultEnterKeyType);
}

QOhosOptional<int> QOhosInputContext::tryQueryCursorPosition() const
{
    auto query = tryQueryFocusObjectInputMethod(Qt::ImCursorPosition);
    return !query.isNull()
        ? tryGetIntPropertyFromQuery(Qt::ImCursorPosition, query)
        : makeEmptyQOhosOptional();
}

QSharedPointer<QInputMethodQueryEvent> QOhosInputContext::tryQueryFocusObjectInputMethod(Qt::InputMethodQueries queries) const
{
    auto *focusObject = focusObjectOrNull();
    if (focusObject == nullptr) {
        qOhosWarning(QtForOhos) << "tryQueryFocusObjectInputMethod(): no focus object to query information from!";
        return {};
    }

    auto imqEvent = QSharedPointer<QInputMethodQueryEvent>::create(queries);
    QCoreApplication::sendEvent(focusObject, imqEvent.data());
    return imqEvent;
}

QT_END_NAMESPACE
