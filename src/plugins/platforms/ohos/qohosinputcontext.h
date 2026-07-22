// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSINPUTCONTEXT_H
#define QOHOSINPUTCONTEXT_H

#include <optional>
#include <qohosenums.h>
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
    using RequestKeyboardReason = QtOhos::enums::ohos::inputMethod::RequestKeyboardReason;

    using TextInputType = QtOhos::enums::ohos::inputMethod::TextInputType;

    using EnterKeyType = QtOhos::enums::ohos::inputMethod::EnterKeyType;

    using Direction = QtOhos::enums::ohos::inputMethod::Direction;

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
    void pushTextAroundCursorToProxy();
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
    std::optional<int> tryQueryCursorPosition() const;
    QSharedPointer<QInputMethodQueryEvent> tryQueryFocusObjectInputMethod(Qt::InputMethodQueries queries) const;

    QRect m_lastCursorRectangle;
    QPoint m_lastFocusedWindowPosition;

    ImConnectionState m_imConnectionState = ImConnectionState::Detached;
    ImConnectionState m_imConnectionRequestedState = ImConnectionState::Detached;
    bool m_imConnectionRequestActive = false;

    bool m_updateCursorRectangleAfterAttaching = false;
    bool m_softwareKeyboardVisible = false;

    std::optional<RequestKeyboardReason> m_lastInputTypeToTriggerSoftKeyboard;

    bool m_qtImEnabled;
    Qt::InputMethodHints m_qtInputMethodHints;
    Qt::EnterKeyType m_qtEnterKeyType;

    std::shared_ptr<JsScopeData> m_jsScopeData;
    std::shared_ptr<QOhosInputMethodProxy> m_imProxy;

    QPointer<QObject> m_focusObject;
    QString m_pendingPreeditText;
};

QT_END_NAMESPACE

#endif // QOHOSINPUTCONTEXT_H
