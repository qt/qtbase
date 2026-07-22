// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSINPUTMETHODPROXY_H
#define QOHOSINPUTMETHODPROXY_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qrect.h>
#include <functional>
#include <inputmethod/inputmethod_controller_capi.h>
#include <memory>
#include <string>

QT_BEGIN_NAMESPACE

class QOhosInputMethodProxy
{
public:
    class ClientCallbacks
    {
    public:
        virtual ~ClientCallbacks();

        virtual void onInsertText(std::string text) = 0;
        virtual void onInsertPreviewText(std::string text) = 0;
        virtual void onFinishPreviewText() = 0;
        virtual void onDeleteForward(int length) = 0;
        virtual void onDeleteBackward(int length) = 0;
        virtual void onSendKeyboardStatus(::InputMethod_KeyboardStatus keyboardStatus) = 0;
        virtual void onSendEnterKey(::InputMethod_EnterKeyType enterKeyType) = 0;
        virtual void onMoveCursor(::InputMethod_Direction direction) = 0;

    protected:
        ClientCallbacks();
    };

    QOhosInputMethodProxy(
        std::shared_ptr<ClientCallbacks> clientCallbacks,
        ::InputMethod_RequestKeyboardReason requestKeyboardReason);
    bool hasAttachedSuccessfully();

    void showTextInput(::InputMethod_RequestKeyboardReason requestKeyboardReason);
    void notifyConfigurationChange(
        ::InputMethod_EnterKeyType enterKeyType, ::InputMethod_TextInputType textInputType);
    void notifyCursorUpdate(const QRectF &cursorRect);
    void setTextAroundCursor(std::u16string leftText, std::u16string rightText);

private:
    struct TextAroundCursor
    {
        std::u16string leftText;
        std::u16string rightText;
    };

    struct JsScopeData
    {
        struct JsTextEditorProxyData
        {
            bool textInputShown = false;
        };

        std::shared_ptr<::InputMethod_TextEditorProxy> textEditorProxy;
        std::shared_ptr<JsTextEditorProxyData> textEditorProxyData;
        std::shared_ptr<::InputMethod_InputMethodProxy> inputMethodProxy;
        std::shared_ptr<void> callbacksRegistrationHandle;
    };

    std::shared_ptr<void> registerCallbacks(
        std::shared_ptr<::InputMethod_TextEditorProxy> textEditorProxy,
        std::shared_ptr<JsScopeData::JsTextEditorProxyData> textEditorProxyData,
        std::shared_ptr<QOhosMutexProtectedValue<TextAroundCursor>> textAroundCursor,
        std::weak_ptr<QOhosInputMethodProxy::ClientCallbacks> weakClientCallbacks);

    std::shared_ptr<ClientCallbacks> m_clientCallbacks;
    std::shared_ptr<QOhosMutexProtectedValue<TextAroundCursor>> m_textAroundCursor;
    std::shared_ptr<JsScopeData> m_jsScopeData;
};

QT_END_NAMESPACE

#endif // QOHOSINPUTMETHODPROXY_H
