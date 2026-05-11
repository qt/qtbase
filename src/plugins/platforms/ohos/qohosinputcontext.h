// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSINPUTCONTEXT_H
#define QOHOSINPUTCONTEXT_H

#include <qohosinputmethodproxy.h>
#include <qohosplugincore.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class QOhosNativeXComponent;

class QOhosInputContext: public QPlatformInputContext
{
    Q_OBJECT

public:
    enum class RequestKeyboardReason {
        NONE,
        MOUSE,
        TOUCH,
        OTHER,
    };

    enum class TextInputType {
        NONE,
        TEXT,
        MULTILINE,
        NUMBER,
        PHONE,
        DATETIME,
        EMAIL_ADDRESS,
        URL,
        VISIBLE_PASSWORD,
        NUMBER_PASSWORD,
    };

    enum class EnterKeyType {
        UNSPECIFIED,
        NONE,
        GO,
        SEARCH,
        SEND,
        NEXT,
        DONE,
        PREVIOUS,
        NEWLINE,
    };

    enum class Direction {
        CURSOR_UP,
        CURSOR_DOWN,
        CURSOR_LEFT,
        CURSOR_RIGHT,
    };

    QOhosInputContext();
    ~QOhosInputContext();
    bool isValid() const override { return true; }

    void reset() override;
    void commit() override;
    void update(Qt::InputMethodQueries queries) override;
    void invokeAction(QInputMethod::Action action, int cursorPosition) override;
    QRectF keyboardRect() const override;
    bool isAnimating() const override;
    void showInputPanel() override;
    void hideInputPanel() override;
    bool isInputPanelVisible() const override;
    void setFocusObject(QObject *object) override;
    QObject *focusObjectOrNull() const;

    void setSoftwareKeyboardVisibilityStatus(bool visible);
    void setLastInputTypeToTriggerSoftKeyboard(RequestKeyboardReason inputType);

    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    enum class ImConnectionState {
        Attached,
        Detached,
    };

    struct JsScopeData
    {
        std::shared_ptr<void> imControllerCallbacksHandle;
    };

    void setImConnectionState(ImConnectionState requestedState);
    void setImConnectionStateImpl(ImConnectionState requestedState);
    void dispatchRequestedImStateChange(ImConnectionState requestedState);
    void showTextInput();
    void updateInputMethodControllerAttributes(
        Qt::InputMethodHints qtInputMethodHints, Qt::EnterKeyType qtEnterKeyType);
    void updateOhosCursor(const QRect &globalCursorRect);
    bool attachToInputMethodController();
    bool detachFromInputMethodController();
    void handleRequestedImConnectionState(ImConnectionState requestedState, bool success);

    void onCursorRectangleChanged();
    void handleFocusInEvent(QObject *obj, QFocusEvent *event);

    void sendInsertedTextToQt(const std::string &textToInsert);
    void sendInsertedPreviewTextToQt(std::string previewText);
    void sendFocusObjectInputMethodEvent(QInputMethodEvent *event);
    void sendCursorMoveToQt(QOhosInputContext::Direction direction);
    void sendFocusObjectFunctionalKeyEvent(Qt::Key key, const QChar &keyChar, int repeatCount = 1);

    bool queryImEnabled() const;
    Qt::InputMethodHints queryInputMethodHints() const;
    Qt::EnterKeyType queryEnterKeyType() const;
    QOhosOptional<int> tryQueryCursorPosition() const;
    QSharedPointer<QInputMethodQueryEvent> tryQueryFocusObjectInputMethod(Qt::InputMethodQueries queries) const;

    QRect m_lastCursorRectangle;
    QPoint m_lastFocusedWindowPosition;

    ImConnectionState m_imConnectionState = ImConnectionState::Detached;
    ImConnectionState m_imConnectionRequestedState = ImConnectionState::Detached;
    bool m_imConnectionRequestActive = false;

    bool m_updateCursorRectangleAfterAttaching = false;
    bool m_softwareKeyboardVisible = false;

    QOhosOptional<RequestKeyboardReason> m_lastInputTypeToTriggerSoftKeyboard;

    bool m_qtImEnabled;
    Qt::InputMethodHints m_qtInputMethodHints;
    Qt::EnterKeyType m_qtEnterKeyType;

    std::shared_ptr<JsScopeData> m_jsScopeData;
    std::shared_ptr<QOhosInputMethodProxy> m_imProxy;

    QPointer<QObject> m_focusObject;
    QString m_pendingPreeditText;
};

namespace QtOhos {

template<>
struct OhosEnumMeta<QOhosInputContext::RequestKeyboardReason>
{
    static constexpr const char *fullTypeName = "@ohos.inputMethod.RequestKeyboardReason";
    static constexpr std::array<std::pair<QOhosInputContext::RequestKeyboardReason, const char *>, 4> enumeratorsNames = {{
        {QOhosInputContext::RequestKeyboardReason::NONE, "NONE"},
        {QOhosInputContext::RequestKeyboardReason::MOUSE, "MOUSE"},
        {QOhosInputContext::RequestKeyboardReason::TOUCH, "TOUCH"},
        {QOhosInputContext::RequestKeyboardReason::OTHER, "OTHER"},
    }};
};

template<>
struct QtOhos::OhosEnumMeta<QOhosInputContext::TextInputType>
{
    static constexpr const char *fullTypeName = "@ohos.inputMethod.TextInputType";
    static constexpr std::array<std::pair<QOhosInputContext::TextInputType, const char *>, 10> enumeratorsNames = {{
        {QOhosInputContext::TextInputType::NONE, "NONE"},
        {QOhosInputContext::TextInputType::TEXT, "TEXT"},
        {QOhosInputContext::TextInputType::MULTILINE, "MULTILINE"},
        {QOhosInputContext::TextInputType::NUMBER, "NUMBER"},
        {QOhosInputContext::TextInputType::PHONE, "PHONE"},
        {QOhosInputContext::TextInputType::DATETIME, "DATETIME"},
        {QOhosInputContext::TextInputType::EMAIL_ADDRESS, "EMAIL_ADDRESS"},
        {QOhosInputContext::TextInputType::URL, "URL"},
        {QOhosInputContext::TextInputType::VISIBLE_PASSWORD, "VISIBLE_PASSWORD"},
        {QOhosInputContext::TextInputType::NUMBER_PASSWORD, "NUMBER_PASSWORD"},
    }};
};

template<>
struct OhosEnumMeta<QOhosInputContext::EnterKeyType>
{
    static constexpr const char *fullTypeName = "@ohos.inputMethod.EnterKeyType";
    static constexpr std::array<std::pair<QOhosInputContext::EnterKeyType, const char *>, 9> enumeratorsNames = {{
        {QOhosInputContext::EnterKeyType::UNSPECIFIED, "UNSPECIFIED"},
        {QOhosInputContext::EnterKeyType::NONE, "NONE"},
        {QOhosInputContext::EnterKeyType::GO, "GO"},
        {QOhosInputContext::EnterKeyType::SEARCH, "SEARCH"},
        {QOhosInputContext::EnterKeyType::SEND, "SEND"},
        {QOhosInputContext::EnterKeyType::NEXT, "NEXT"},
        {QOhosInputContext::EnterKeyType::DONE, "DONE"},
        {QOhosInputContext::EnterKeyType::PREVIOUS, "PREVIOUS"},
        {QOhosInputContext::EnterKeyType::NEWLINE, "NEWLINE"},
    }};
};

template<>
struct OhosEnumMeta<QOhosInputContext::Direction>
{
    static constexpr const char *fullTypeName = "@ohos.inputMethod.Direction";
    static constexpr std::array<std::pair<QOhosInputContext::Direction, const char *>, 4> enumeratorsNames = {{
        {QOhosInputContext::Direction::CURSOR_UP, "CURSOR_UP"},
        {QOhosInputContext::Direction::CURSOR_DOWN, "CURSOR_DOWN"},
        {QOhosInputContext::Direction::CURSOR_LEFT, "CURSOR_LEFT"},
        {QOhosInputContext::Direction::CURSOR_RIGHT, "CURSOR_RIGHT"},
    }};
};

}

QT_END_NAMESPACE

#endif // QOHOSINPUTCONTEXT_H
