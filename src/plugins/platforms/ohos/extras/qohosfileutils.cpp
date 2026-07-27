// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosfileutils_p.h"

#include <QtCore/qeventloop.h>
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/private/qohospathutils_p.h>

#include <memory>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace {

void showFileDialogAuthorization(
    QtOhos::QObjectThreadSafeRef contextWindowRef, QString filePath,
    QOhosConsumer<bool> resultCallback)
{
    auto sharedResultCallback = QtOhos::moveToSharedPtr(std::move(resultCallback));

    QOhosJsThreadGateway::invoke(
        [contextWindowRef, filePath, sharedResultCallback](QOhosJsState &jsState) {
            auto optWindowQAbility = jsState.tryGetQAbilityByQWindow(contextWindowRef);
            auto optQAbility = optWindowQAbility ? optWindowQAbility : jsState.defaultQAbility();
            if (!optQAbility) {
                qOhosPrintfError("%s: no ability available for the file dialog", Q_FUNC_INFO);
                QtOhos::invokeInQtThread(
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
            auto optContextJsWindow = jsState.tryGetJsWindowByQWindow(contextWindowRef);
            std::vector<QNapi::ValueWrapper> constructorParams = {optQAbility.value().get("context")};
            if (optContextJsWindow)
                constructorParams.push_back(optContextJsWindow.value());
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
                    QtOhos::invokeInQtThread(
                        [sharedResultCallback, authorized]() {
                            (*sharedResultCallback)(authorized);
                        });
                },
                [sharedResultCallback]() {
                    qOhosPrintfError("DocumentViewPicker.select() call failed");
                    QtOhos::invokeInQtThread(
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

    showFileDialogAuthorization(
        QtOhos::QObjectThreadSafeRef(parentWindow), filePath,
        [filePathAuthorized, eventLoop](bool result) {
            *filePathAuthorized = result;
            eventLoop->quit();
        });

    eventLoop->exec();

    return *filePathAuthorized;
}

}

QT_END_NAMESPACE
