// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosinputmethodproxy.h"
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/private/qstringconverter_p.h>
#include <codecvt>
#include <locale>
#include <qarkui/qarkuiutils.h>
#include <qohosplugincore.h>
#include <qohosutils.h>

QT_BEGIN_NAMESPACE

namespace {

std::string convertUtf16ToUtf8(const std::u16string &utf16String)
{
#ifdef __cpp_lib_string_resize_and_overwrite
    std::string ut8Str;
    // In the worst case, we need atleast 3 bytes when converting from utf16
    // to utf8
    const auto maxBytes = utf16String.size() * 3;
    utf8Str.resize_and_overwrite(maxBytes, [&] (char *dst, size_t) noexcept {
            return QUtf8::convertFromUnicode(dst, QStringView{utf16String}) - dst;
        });
    return utf8Str;
#else
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(utf16String);
QT_WARNING_POP
#endif
}

std::shared_ptr<::InputMethod_TextEditorProxy> makeTextEditorProxy()
{
    return std::shared_ptr<::InputMethod_TextEditorProxy>(
        QArkUi::callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_Create)),
        [](::InputMethod_TextEditorProxy *textEditorProxy) {
            QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_Destroy),
                textEditorProxy);
        });
}

std::shared_ptr<::InputMethod_AttachOptions> makeAttachOptions(
    bool showKeyboard, ::InputMethod_RequestKeyboardReason requestKeyboardReason)
{
    return std::shared_ptr<::InputMethod_AttachOptions>(
        QArkUi::callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_AttachOptions_CreateWithRequestKeyboardReason),
            showKeyboard, requestKeyboardReason),
        [](::InputMethod_AttachOptions *attachOptions) {
            QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_AttachOptions_Destroy),
                attachOptions);
        });
}

std::shared_ptr<::InputMethod_InputMethodProxy> tryMakeInputMethodProxy(
    std::shared_ptr<::InputMethod_TextEditorProxy> textEditorProxy,
    std::shared_ptr<::InputMethod_AttachOptions> attachOptions)
{
    ::InputMethod_InputMethodProxy *inputMethodProxyPtr = nullptr;
    ::InputMethod_ErrorCode errcode = QArkUi::callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_InputMethodController_Attach),
        textEditorProxy.get(), attachOptions.get(), &inputMethodProxyPtr);
    if (errcode != ::InputMethod_ErrorCode::IME_ERR_OK) {
        qOhosPrintfError("%s: OH_InputMethodController_Attach failed!", Q_FUNC_INFO);
        return {};
    }

    return std::shared_ptr<::InputMethod_InputMethodProxy>(
        inputMethodProxyPtr, [](::InputMethod_InputMethodProxy *proxy) {
            QArkUi::callArkUiOrFailOnErrorResult(
                Q_OHOS_NAMED_FUNC(::OH_InputMethodController_Detach), proxy);
        });
}

std::shared_ptr<::InputMethod_CursorInfo> makeCursorInfo(
    double x, double y, double width, double height)
{
    return std::shared_ptr<::InputMethod_CursorInfo>(
        QArkUi::callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_CursorInfo_Create),
            x, y, width, height),
        [](::InputMethod_CursorInfo *cursorInfo) {
            QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_CursorInfo_Destroy), cursorInfo);
        });
}

template<typename CallbackResult, typename... CallbackArgs>
using TextEditorProxyCallbackFunc = CallbackResult(::InputMethod_TextEditorProxy *, CallbackArgs...);
template<
    typename CallbackResult,
    typename... CallbackArgs,
    ::InputMethod_ErrorCode rawSetCallbackFunc(::InputMethod_TextEditorProxy *, TextEditorProxyCallbackFunc<CallbackResult, CallbackArgs...>)>
std::shared_ptr<void> registerTextEditorProxyCallbackImpl(
    ::InputMethod_TextEditorProxy *textEditorProxy,
    QOhosNamedFunc<::InputMethod_ErrorCode(*)(::InputMethod_TextEditorProxy *, TextEditorProxyCallbackFunc<CallbackResult, CallbackArgs...>), rawSetCallbackFunc> setCallbackFunc,
    std::function<CallbackResult(CallbackArgs...)> callback)
{
    static std::map<::InputMethod_TextEditorProxy *, std::shared_ptr<std::function<CallbackResult(CallbackArgs...)>>> callbacksMap;

    bool callbackRegistered;
    std::tie(std::ignore, callbackRegistered) = callbacksMap.emplace(
        textEditorProxy,
        std::make_shared<std::function<CallbackResult(CallbackArgs...)>>(std::move(callback)));

    if (!callbackRegistered)
        qOhosReportFatalErrorAndAbort(Q_FUNC_INFO, "encountered duplicate callback registration for %p", textEditorProxy);

    QArkUi::callArkUiOrFailOnErrorResult(
        setCallbackFunc, textEditorProxy,
        [](::InputMethod_TextEditorProxy *textEditorProxy, CallbackArgs ...callbackArgs) {
            auto callbackIter = callbacksMap.find(textEditorProxy);
            if (callbackIter != callbacksMap.end()) {
                auto sharedCallback = callbackIter->second;
                return (*sharedCallback)(callbackArgs...);
            } else {
                return CallbackResult();
            }
        });

    return QtOhos::makeDestroyNotifier(
        [textEditorProxy]() {
            callbacksMap.erase(textEditorProxy);
        });
}

template<
    typename CallbackResult,
    typename... CallbackArgs,
    ::InputMethod_ErrorCode rawSetCallbackFunc(::InputMethod_TextEditorProxy *, TextEditorProxyCallbackFunc<CallbackResult, CallbackArgs...>),
    typename Callback>
std::shared_ptr<void> registerTextEditorProxyCallback(
    ::InputMethod_TextEditorProxy *textEditorProxy,
    QOhosNamedFunc<::InputMethod_ErrorCode(*)(::InputMethod_TextEditorProxy *, TextEditorProxyCallbackFunc<CallbackResult, CallbackArgs...>), rawSetCallbackFunc> setCallbackFunc,
    Callback callback)
{
    static_assert(std::is_assignable<std::function<CallbackResult(CallbackArgs...)>, Callback>::value, "");
    return registerTextEditorProxyCallbackImpl<CallbackResult>(
        textEditorProxy, setCallbackFunc, std::function<CallbackResult(CallbackArgs...)>(std::move(callback)));
}

template<typename... CallbackArgs>
void callInQtThread(
    std::weak_ptr<QOhosInputMethodProxy::ClientCallbacks> weakClientCallbacks,
    void (QOhosInputMethodProxy::ClientCallbacks::*callbackMemPtr)(CallbackArgs...),
    CallbackArgs ...args)
{
    QtOhos::invokeInQtThread(
        [weakClientCallbacks, callbackMemPtr, args...]() {
            auto clientCallbacks = weakClientCallbacks.lock();
            if (clientCallbacks)
                (clientCallbacks.get()->*callbackMemPtr)(args...);
        });
}

}

std::shared_ptr<void> QOhosInputMethodProxy::registerCallbacks(
    std::shared_ptr<::InputMethod_TextEditorProxy> textEditorProxy,
    std::shared_ptr<JsScopeData::JsTextEditorProxyData> textEditorProxyData,
    std::weak_ptr<QOhosInputMethodProxy::ClientCallbacks> weakClientCallbacks)
{
    std::vector<std::shared_ptr<void>> registrationHandles;

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetGetTextConfigFunc),
            [](::InputMethod_TextConfig *config) {
                QArkUi::callArkUiOrFailOnErrorResult(
                    Q_OHOS_NAMED_FUNC(::OH_TextConfig_SetPreviewTextSupport),
                    config, true);
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetInsertTextFunc),
            [weakClientCallbacks](const char16_t *text, size_t length) {
                std::u16string utf16Text(text, length);
                callInQtThread(
                    weakClientCallbacks, &QOhosInputMethodProxy::ClientCallbacks::onInsertText,
                    convertUtf16ToUtf8(utf16Text));
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetDeleteForwardFunc),
            [weakClientCallbacks](std::int32_t length) {
                callInQtThread(
                    weakClientCallbacks, &QOhosInputMethodProxy::ClientCallbacks::onDeleteForward,
                    length);
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetDeleteBackwardFunc),
            [weakClientCallbacks](std::int32_t length) {
                callInQtThread(
                    weakClientCallbacks, &QOhosInputMethodProxy::ClientCallbacks::onDeleteBackward,
                    length);
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetSendKeyboardStatusFunc),
            [weakClientCallbacks, weakTextEditorProxyData = QtOhos::makeWeakPtr(textEditorProxyData)](::InputMethod_KeyboardStatus keyboardStatus) {
                auto sharedTextEditorProxyData = weakTextEditorProxyData.lock();
                if (sharedTextEditorProxyData)
                    sharedTextEditorProxyData->textInputShown = keyboardStatus == ::IME_KEYBOARD_STATUS_SHOW;

                callInQtThread(
                    weakClientCallbacks, &QOhosInputMethodProxy::ClientCallbacks::onSendKeyboardStatus,
                    keyboardStatus);
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetSendEnterKeyFunc),
            [weakClientCallbacks](::InputMethod_EnterKeyType enterKeyType) {
                callInQtThread(
                    weakClientCallbacks, &QOhosInputMethodProxy::ClientCallbacks::onSendEnterKey,
                    enterKeyType);
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetMoveCursorFunc),
            [weakClientCallbacks](::InputMethod_Direction direction) {
                callInQtThread(
                    weakClientCallbacks, &QOhosInputMethodProxy::ClientCallbacks::onMoveCursor,
                    direction);
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetHandleSetSelectionFunc),
            [](auto...) {
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetHandleExtendActionFunc),
            [](auto...) {
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetGetLeftTextOfCursorFunc),
            [](auto...) {
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetGetRightTextOfCursorFunc),
            [](auto...) {
            }));

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetGetTextIndexAtCursorFunc),
        textEditorProxy.get(),
        [](::InputMethod_TextEditorProxy *) {
            return 0;
        });

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetReceivePrivateCommandFunc),
        textEditorProxy.get(),
        [](::InputMethod_TextEditorProxy *, ::InputMethod_PrivateCommand **, unsigned long) {
            return 0;
        });

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(),
            Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetSetPreviewTextFunc),
            [weakClientCallbacks](const char16_t *text, std::size_t length, std::int32_t, std::int32_t) {
                std::u16string utf16Text(text, length);
                callInQtThread(
                    weakClientCallbacks, &QOhosInputMethodProxy::ClientCallbacks::onInsertPreviewText,
                    convertUtf16ToUtf8(utf16Text));
                return 0;
            }));

    registrationHandles.push_back(
        registerTextEditorProxyCallback(
            textEditorProxy.get(), Q_OHOS_NAMED_FUNC(::OH_TextEditorProxy_SetFinishTextPreviewFunc),
            [weakClientCallbacks]() {
                callInQtThread(
                    weakClientCallbacks, &QOhosInputMethodProxy::ClientCallbacks::onFinishPreviewText);
            }));

    return QtOhos::moveToSharedPtr(std::move(registrationHandles));
}

QOhosInputMethodProxy::QOhosInputMethodProxy(
    std::shared_ptr<ClientCallbacks> clientCallbacks, ::InputMethod_RequestKeyboardReason requestKeyboardReason)
    : m_clientCallbacks(clientCallbacks)
{
    auto weakClientCallbacks = QtOhos::makeWeakPtr(clientCallbacks);
    m_jsScopeData = QtOhos::evalInJsThread(
        [&](auto &) -> std::shared_ptr<JsScopeData> {
            auto textEditorProxyData = std::make_shared<JsScopeData::JsTextEditorProxyData>();
            auto textEditorProxy = makeTextEditorProxy();
            auto callbacksRegistrationHandle = registerCallbacks(textEditorProxy, textEditorProxyData, weakClientCallbacks);

            auto showKeyboard = true;
            auto attachOptions = makeAttachOptions(showKeyboard, requestKeyboardReason);

            auto inputMethodProxy = tryMakeInputMethodProxy(textEditorProxy, attachOptions);
            if (!inputMethodProxy) {
                qOhosPrintfError("%s: inputMethodProxy is nullptr!", Q_FUNC_INFO);
                return nullptr;
            }

            return QtOhos::makeProxyWithJsThreadDeleter(
                QtOhos::moveToSharedPtr(
                    JsScopeData {
                        .textEditorProxy = textEditorProxy,
                        .textEditorProxyData = textEditorProxyData,
                        .inputMethodProxy = inputMethodProxy,
                        .callbacksRegistrationHandle = callbacksRegistrationHandle,
                    }));
        },
        Q_FUNC_INFO);
}

bool QOhosInputMethodProxy::hasAttachedSuccessfully()
{
    return m_jsScopeData != nullptr;
}

void QOhosInputMethodProxy::showTextInput(::InputMethod_RequestKeyboardReason requestKeyboardReason)
{
    if (!hasAttachedSuccessfully()) {
        qOhosPrintfError("%s: operation aborted, IMC not attached.", Q_FUNC_INFO);
        return;
    }

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            if (m_jsScopeData->textEditorProxyData->textInputShown) {
                qOhosPrintfDebug("%s: text input already shown, skip showing it again", Q_FUNC_INFO);
                return;
            }

            auto showKeyboard = true;
            auto attachOptions = makeAttachOptions(showKeyboard, requestKeyboardReason);

            ::InputMethod_ErrorCode errcode = QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_InputMethodProxy_ShowTextInput),
                m_jsScopeData->inputMethodProxy.get(),
                attachOptions.get());
            m_jsScopeData->textEditorProxyData->textInputShown = errcode == ::InputMethod_ErrorCode::IME_ERR_OK;
            if (errcode != ::InputMethod_ErrorCode::IME_ERR_OK)
                qOhosPrintfError("%s: OH_InputMethodProxy_ShowTextInput failed!", Q_FUNC_INFO);
        },
        Q_FUNC_INFO);
}

void QOhosInputMethodProxy::notifyConfigurationChange(
    ::InputMethod_EnterKeyType enterKeyType, ::InputMethod_TextInputType textInputType)
{
    if (!hasAttachedSuccessfully()) {
        qOhosPrintfError("%s: operation aborted, IMC not attached.", Q_FUNC_INFO);
        return;
    }

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            ::InputMethod_ErrorCode errcode = QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_InputMethodProxy_NotifyConfigurationChange),
                m_jsScopeData->inputMethodProxy.get(),
                enterKeyType, textInputType);
            if (errcode != ::InputMethod_ErrorCode::IME_ERR_OK)
                qOhosPrintfError("%s: OH_InputMethodProxy_NotifyConfigurationChange failed!", Q_FUNC_INFO);
        },
        Q_FUNC_INFO);
}

void QOhosInputMethodProxy::notifyCursorUpdate(const QRectF &cursorRect)
{
    if (!hasAttachedSuccessfully()) {
        qOhosPrintfError("%s: operation aborted, IMC not attached.", Q_FUNC_INFO);
        return;
    }

    double x = cursorRect.x();
    double y = cursorRect.y();
    double width = cursorRect.width();
    double height = cursorRect.height();

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            auto ohCursorInfo = makeCursorInfo(x, y, width, height);

            ::InputMethod_ErrorCode errcode = QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_InputMethodProxy_NotifyCursorUpdate),
                m_jsScopeData->inputMethodProxy.get(), ohCursorInfo.get());
            if (errcode != ::InputMethod_ErrorCode::IME_ERR_OK)
                qOhosPrintfError("%s: OH_InputMethodProxy_NotifyCursorUpdate failed!", Q_FUNC_INFO);
        },
        Q_FUNC_INFO);
}

QOhosInputMethodProxy::ClientCallbacks::ClientCallbacks() = default;

QOhosInputMethodProxy::ClientCallbacks::~ClientCallbacks() = default;

QT_END_NAMESPACE
