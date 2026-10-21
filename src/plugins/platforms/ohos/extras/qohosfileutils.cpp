// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosfileutils_p.h"

#include <QtCore/qcoreapplication_platform.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qharmonyappjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/private/qohospathutils_p.h>

#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE

namespace {

void showFileDialogAuthorization(
    QHarmonyJsObjectRef contextWindowAbilityRef, QHarmonyJsObjectRef contextJsWindowRef, QString filePath,
    QOhosConsumer<bool> resultCallback)
{
    auto sharedResultCallback = QtOhos::moveToSharedPtr(std::move(resultCallback));

    QOhosJsThreadGateway::invoke(
        [contextWindowAbilityRef, contextJsWindowRef, filePath, sharedResultCallback](QOhosJsState &jsState) {
            auto &appJsContext = QHarmonyAppJsContext::fromEnv(jsState.env());
            auto optQAbility = QNapi::Object(
                jsState.env(),
                contextWindowAbilityRef.isNull() ? appJsContext.defaultAbility() : contextWindowAbilityRef.value(appJsContext));
            if (optQAbility.IsEmpty()) {
                qOhosPrintfError("%s: no ability available for the file dialog", Q_FUNC_INFO);
                QNativeInterface::QHarmonyApplication::runOnQtThread(
                    [sharedResultCallback]() {
                        (*sharedResultCallback)(false);
                    });
                return;
            }

            auto documentSelectOptions = QNapi::makeObject(
                jsState.env(),
                {
                    {"defaultFilePathUri", tryMapPathToOhosFileUri(filePath.toStdString()).value_or("")},
                    {"authMode", true},
                });

            qOhosPrintfDebug(
                "Calling DocumentViewPicker.select() with options: %s",
                QNapi::toJsonString(documentSelectOptions).c_str());
            std::vector<QNapi::ValueWrapper> constructorParams = {optQAbility.get("context")};
            if (!contextJsWindowRef.isNull())
                constructorParams.push_back(QNapi::Object(jsState.env(), contextJsWindowRef.value(appJsContext)));
            auto documentViewPicker = QtOhos::moveToSharedPtr(
                QNapi::Reference<>::makePersistentFrom(
                    jsState.eval<QNapi::Object>(
                        "@ohos.file.picker.DocumentViewPicker<new>(*)", constructorParams)));
            documentViewPicker->evalToPromiseOrRejectOnThrow("select(*)", {documentSelectOptions}).onThen(
                [documentViewPicker, sharedResultCallback](const QOhosCallbackInfo &cbInfo) {
                    auto actionResult = cbInfo.getFirstArg<QNapi::Array>(Q_FUNC_INFO);

                    qOhosPrintfDebug(
                        "Called DocumentViewPicker.select() callback with result: %s",
                        QNapi::toJsonString(actionResult).c_str());

                    bool authorized = actionResult.Length() > 0;
                    QNativeInterface::QHarmonyApplication::runOnQtThread(
                        [sharedResultCallback, authorized]() {
                            (*sharedResultCallback)(authorized);
                        });
                },
                [sharedResultCallback]() {
                    qOhosPrintfError("DocumentViewPicker.select() call failed");
                    QNativeInterface::QHarmonyApplication::runOnQtThread(
                        [sharedResultCallback]() {
                            (*sharedResultCallback)(false);
                        });
                });
        });
}

}

namespace QtHarmonyExtras {

bool authorizeFilePath(QWindow *parentWindow, const QString &filePath)
{
    auto eventLoop = std::make_shared<QEventLoop>();
    auto filePathAuthorized = std::make_shared<bool>(false);

    auto *harmonyWindow = parentWindow ? parentWindow->nativeInterface<QNativeInterface::QHarmonyWindow>() : nullptr;
    showFileDialogAuthorization(
        harmonyWindow ? harmonyWindow->ability() : QHarmonyJsObjectRef(),
        harmonyWindow ? harmonyWindow->jsWindow() : QHarmonyJsObjectRef(),
        filePath,
        [filePathAuthorized, eventLoop](bool result) {
            *filePathAuthorized = result;
            eventLoop->quit();
        });

    eventLoop->exec();

    return *filePathAuthorized;
}

}

QT_END_NAMESPACE
