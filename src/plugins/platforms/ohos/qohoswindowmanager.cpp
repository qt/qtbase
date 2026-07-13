// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoswindowmanager.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/private/qohospathutils_p.h>
#include <qohosjsenv_p.h>
#include <qohosplugincore.h>
#include <QtCore/qurl.h>
#include <algorithm>
#include <iterator>
#include <vector>
#include "qohosplatformwindow.h"

QT_BEGIN_NAMESPACE

namespace {

struct FilePickerResult
{
    std::vector<std::string> resultPaths;
    int selectedIndex;
};

std::shared_ptr<QtOhos::QAbilityPeer> getQAbilityPeerForQWindow(
    QtOhos::JsState &jsState, QtOhos::QObjectThreadSafeRef qWindowRef)
{
    auto qAbilityPeer = jsState.tryGetQAbilityPeerByQWindow(qWindowRef);
    return qAbilityPeer ? qAbilityPeer : jsState.defaultQAbilityPeer();
}

QNapi::Object makeDocumentViewPicker(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer,
    QtOhos::QObjectThreadSafeRef contextWindowRef)
{
    auto optContextJsWindow = jsState.tryGetJsWindowByQWindow(contextWindowRef);

    std::vector<QNapi::ValueWrapper> constructorParams = {qAbilityPeer->qAbility().get("context")};
    if (optContextJsWindow)
        constructorParams.push_back(optContextJsWindow.value());

    return jsState.eval<QNapi::Object>(
        "@ohos.file.picker.DocumentViewPicker<new>(*)", constructorParams);
}

void startOhosFilePicker(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer,
    QtOhos::QObjectThreadSafeRef contextWindowRef,
    const std::string &pickerActionName, QNapi::Object pickerActionOptions,
    QOhosConsumer<QOhosOptional<FilePickerResult>> resultConsumer)
{
    auto sharedResultConsumer = QtOhos::moveToSharedPtr(std::move(resultConsumer));

    qOhosPrintfDebug(
        "Calling DocumentViewPicker.%s() with options: %s",
        pickerActionName.c_str(), QNapi::toJsonString(pickerActionOptions).c_str());
    auto documentViewPicker = QtOhos::moveToSharedPtr(
        QNapi::Reference<>::makePersistentFrom(
            makeDocumentViewPicker(jsState, qAbilityPeer, contextWindowRef)));
    documentViewPicker->evalToPromiseOrRejectOnThrow(pickerActionName + "(*)", {pickerActionOptions}).onThen(
        [documentViewPicker, pickerActionName, sharedResultConsumer](const QtOhos::CallbackInfo &cbInfo) {
            auto actionResult = cbInfo.getFirstArg<QNapi::Array>(Q_FUNC_INFO);
            auto resultOhosUris = QNapi::getArrayElements<std::vector<std::string>, QNapi::String>(actionResult);

            qOhosPrintfDebug(
                "Called DocumentViewPicker.%s() callback with result: %s",
                pickerActionName.c_str(), QNapi::toJsonString(actionResult).c_str());

            std::vector<std::string> resultPaths;
            std::transform(
                resultOhosUris.cbegin(), resultOhosUris.cend(), std::back_inserter(resultPaths),
                [&](const auto &uri) {
                    return tryMapOhosFileUriToPath(uri).value_or("");
                });

            (*sharedResultConsumer)(
                QOhosOptional<FilePickerResult>(
                    {
                        .resultPaths = std::move(resultPaths),
                        .selectedIndex = documentViewPicker->eval<QNapi::Number>("getSelectedIndex()"),
                    }));
        },
        [pickerActionName, sharedResultConsumer]() {
            qOhosPrintfError("DocumentViewPicker.%s() call failed", pickerActionName.c_str());
            (*sharedResultConsumer)(makeEmptyQOhosOptional());
        });
}

QStringList mapFilePathsToQtUrls(const std::vector<std::string> &filePaths)
{
    QStringList qtUrls;
    std::transform(
        filePaths.cbegin(), filePaths.cend(), std::back_inserter(qtUrls),
        [&](const auto &path) {
            return QUrl::fromLocalFile(QString::fromStdString(path)).toString();
        });
    return qtUrls;
}

}

namespace QOhosWindowManager {

void showFileDialogOpen(
    QtOhos::QObjectThreadSafeRef contextWindowRef, QStringList filters, QString defaultPath,
    DocumentSelectMode documentSelectMode, ResultMultiplicity resultMultiplicity,
    QOhosConsumer<QOhosOptional<OpenResult>> resultCallback)
{
    auto sharedResultCallback = QtOhos::moveToSharedPtr(std::move(resultCallback));

    QtOhos::invokeInJsThread(
        [contextWindowRef, filters, defaultPath, documentSelectMode, resultMultiplicity, sharedResultCallback](QtOhos::JsState &jsState) {
            constexpr auto ohosMaxValueForMaxSelectNumber = 500;

            auto *env = jsState.env();

            auto documentSelectOptions = QNapi::makeObject(
                env,
                {
                    {"maxSelectNumber", resultMultiplicity == ResultMultiplicity::SINGLE ? 1 : ohosMaxValueForMaxSelectNumber},
                    {"fileSuffixFilters", QNapi::makeArray(env, filters, std::mem_fn(&QString::toStdString))},
                    {"defaultFilePathUri", tryMapPathToOhosFileUri(defaultPath.toStdString()).value_or("")},
                    {"selectMode", jsState.mapOhosEnumToJs(documentSelectMode)},
                });

            startOhosFilePicker(
                jsState, getQAbilityPeerForQWindow(jsState, contextWindowRef), contextWindowRef,
                "select", documentSelectOptions,
                [sharedResultCallback](auto optResult) {
                    auto optQtOpenResult = qTransform(
                        optResult,
                        [](const auto &result) {
                            return OpenResult{
                                .selectedUrls = mapFilePathsToQtUrls(result.resultPaths),
                            };
                        });
                    QtOhos::invokeInQtThread(
                        [sharedResultCallback, optQtOpenResult]() {
                            (*sharedResultCallback)(optQtOpenResult);
                        });
                });
        });
}

void showFileDialogSave(
    QtOhos::QObjectThreadSafeRef contextWindowRef, QStringList newFileNames,
    QString defaultFilePath, QStringList fileSuffixChoices,
    QOhosConsumer<QOhosOptional<SaveResult>> resultCallback)
{
    auto sharedResultCallback = QtOhos::moveToSharedPtr(std::move(resultCallback));

    QtOhos::invokeInJsThread(
        [contextWindowRef, newFileNames, defaultFilePath, fileSuffixChoices, sharedResultCallback](QtOhos::JsState &jsState) {
            auto *env = jsState.env();
            auto documentSaveOptions = QNapi::Object::New(env);
            if (!newFileNames.isEmpty())
                documentSaveOptions.Set("newFileNames", QNapi::makeArray(env, newFileNames, std::mem_fn(&QString::toStdString)));
            if (!defaultFilePath.isEmpty())
                documentSaveOptions.Set("defaultFilePathUri", tryMapPathToOhosFileUri(defaultFilePath.toStdString()).value_or(""));
            if (!fileSuffixChoices.isEmpty())
                documentSaveOptions.Set("fileSuffixChoices", QNapi::makeArray(env, fileSuffixChoices, std::mem_fn(&QString::toStdString)));
            documentSaveOptions.Set("autoCreateEmptyFile", false);

            startOhosFilePicker(
                jsState, getQAbilityPeerForQWindow(jsState, contextWindowRef), contextWindowRef,
                "save", documentSaveOptions,
                [sharedResultCallback](auto optResult) {
                    auto optQtSaveResult = qTransform(
                        optResult,
                        [](const auto &result) {
                            return SaveResult{
                                .savedUrls = mapFilePathsToQtUrls(result.resultPaths),
                                .selectedFileSuffixChoiceIndex = result.selectedIndex,
                            };
                        });
                    QtOhos::invokeInQtThread(
                        [sharedResultCallback, optQtSaveResult]() {
                            (*sharedResultCallback)(optQtSaveResult);
                        });
                });
        });
}

void showFileDialogAuthorization(
    QtOhos::QObjectThreadSafeRef contextWindowRef, QString filePath,
    QOhosConsumer<bool> resultCallback)
{
    auto sharedResultCallback = QtOhos::moveToSharedPtr(std::move(resultCallback));

    QtOhos::invokeInJsThread(
        [contextWindowRef, filePath, sharedResultCallback](QtOhos::JsState &jsState) {
            auto documentSelectOptions = QNapi::makeObject(
                jsState.env(),
                {
                    {"defaultFilePathUri", tryMapPathToOhosFileUri(filePath.toStdString()).value_or("")},
                    {"authMode", true},
                });

            startOhosFilePicker(
                jsState, getQAbilityPeerForQWindow(jsState, contextWindowRef), contextWindowRef,
                "select", documentSelectOptions,
                [sharedResultCallback](auto optResult) {
                    auto optSelectedUrls = qTransform(
                        optResult,
                        [](const auto &result) {
                            return mapFilePathsToQtUrls(result.resultPaths);
                        });
                    QtOhos::invokeInQtThread(
                        [sharedResultCallback, optSelectedUrls]() {
                            bool authorized = optSelectedUrls.has_value() && !optSelectedUrls.value().isEmpty();
                            (*sharedResultCallback)(authorized);
                        });
                });
        });
}

}

QT_END_NAMESPACE
