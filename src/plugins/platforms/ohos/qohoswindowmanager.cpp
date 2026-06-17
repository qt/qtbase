// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoswindowmanager.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <qohosdeviceinfo_p.h>
#include <qohosjsenv_p.h>
#include <qohosplugincore.h>
#include <QtCore/qurl.h>
#include <algorithm>
#include <iterator>
#include <render/qohosjswindowregistry.h>
#include <render/qwindowproxyregistry.h>
#include <vector>
#include "qohosplatformservices.h"
#include "qohosplatformwindow.h"

QT_BEGIN_NAMESPACE

namespace {

struct FilePickerResult
{
    std::vector<std::string> resultPaths;
    int selectedIndex;
};

std::shared_ptr<QtOhos::QAbilityPeer> getQAbilityPeerForOptInstanceId(
    QtOhos::JsState &jsState, QOhosOptional<std::string> optQAbilityInstanceId)
{
    auto optQAbilityPeer = optQAbilityInstanceId.hasValue()
        ? jsState.tryGetQAbilityPeerByInstanceId(optQAbilityInstanceId.value())
        : std::shared_ptr<QtOhos::QAbilityPeer>();

    return optQAbilityPeer ? optQAbilityPeer : jsState.defaultQAbilityPeer();
}

QNapi::Object makeDocumentViewPicker(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer,
    QOhosOptional<QArkUi::JsWindowId> optContextJsWinId)
{
    auto &jsWindowRegistry = jsState.getAttachedObjectWithLazyCreate<QOhosJsWindowRegistry>();
    auto optJsWindowRef =
        optContextJsWinId.hasValue()
            ? jsWindowRegistry.tryFindJsWindowById(optContextJsWinId.value())
            : nullptr;

    std::vector<QNapi::ValueWrapper> constructorParams = {qAbilityPeer->qAbility().get("context")};
    if (optJsWindowRef)
        constructorParams.push_back(optJsWindowRef->jsObject());

    return jsState.eval<QNapi::Object>(
        "@ohos.file.picker.DocumentViewPicker<new>(*)", constructorParams);
}

void startOhosFilePicker(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer,
    QOhosOptional<QArkUi::JsWindowId> optContextJsWinId,
    const std::string &pickerActionName, QNapi::Object pickerActionOptions,
    QOhosConsumer<QOhosOptional<FilePickerResult>> resultConsumer)
{
    auto sharedResultConsumer = QtOhos::moveToSharedPtr(std::move(resultConsumer));

    qOhosPrintfDebug(
        "Calling DocumentViewPicker.%s() with options: %s",
        pickerActionName.c_str(), QNapi::toJsonString(pickerActionOptions).c_str());
    auto documentViewPicker = QtOhos::moveToSharedPtr(
        QNapi::Reference<>::makePersistentFrom(
            makeDocumentViewPicker(jsState, qAbilityPeer, optContextJsWinId)));
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
                    return QOhosPlatformServices::mapOhosFileUriToPathInJsThread(uri);
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
    QtOhos::InternalWindowId contextWinId, QStringList filters, QString defaultPath,
    DocumentSelectMode documentSelectMode, ResultMultiplicity resultMultiplicity,
    QOhosConsumer<QOhosOptional<OpenResult>> resultCallback)
{
    auto sharedResultCallback = QtOhos::moveToSharedPtr(std::move(resultCallback));

    auto qAbilityInstanceId =
        QWindowProxyRegistry::instance().tryFindQAbilityInstanceIdByInternalWindowId(contextWinId);
    auto optContextJsWinId = QWindowProxyRegistry::instance().tryMapInternalWindowIdToJsWindowId(contextWinId);

    QtOhos::invokeInJsThread(
        [qAbilityInstanceId, optContextJsWinId, filters, defaultPath, documentSelectMode, resultMultiplicity, sharedResultCallback](QtOhos::JsState &jsState) {
            constexpr auto ohosMaxValueForMaxSelectNumber = 500;

            auto *env = jsState.env();

            auto documentSelectOptions = QNapi::makeObject(
                env,
                {
                    {"maxSelectNumber", resultMultiplicity == ResultMultiplicity::SINGLE ? 1 : ohosMaxValueForMaxSelectNumber},
                    {"fileSuffixFilters", QNapi::makeArray(env, filters, std::mem_fn(&QString::toStdString))},
                    {"defaultFilePathUri", QOhosPlatformServices::mapPathToOhosUriInJsThread(defaultPath.toStdString())},
                    {"selectMode", jsState.mapOhosEnumToJs(documentSelectMode)},
                });

            startOhosFilePicker(
                jsState, getQAbilityPeerForOptInstanceId(jsState, qAbilityInstanceId), optContextJsWinId,
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
    QtOhos::InternalWindowId contextWinId, QStringList newFileNames,
    QString defaultFilePath, QStringList fileSuffixChoices,
    QOhosConsumer<QOhosOptional<SaveResult>> resultCallback)
{
    auto sharedResultCallback = QtOhos::moveToSharedPtr(std::move(resultCallback));

    auto qAbilityInstanceId =
        QWindowProxyRegistry::instance().tryFindQAbilityInstanceIdByInternalWindowId(contextWinId);
    auto optContextJsWinId = QWindowProxyRegistry::instance().tryMapInternalWindowIdToJsWindowId(contextWinId);

    QtOhos::invokeInJsThread(
        [qAbilityInstanceId, optContextJsWinId, newFileNames, defaultFilePath, fileSuffixChoices, sharedResultCallback](QtOhos::JsState &jsState) {
            auto *env = jsState.env();
            auto documentSaveOptions = QNapi::Object::New(env);
            if (!newFileNames.isEmpty())
                documentSaveOptions.Set("newFileNames", QNapi::makeArray(env, newFileNames, std::mem_fn(&QString::toStdString)));
            if (!defaultFilePath.isEmpty())
                documentSaveOptions.Set("defaultFilePathUri", QOhosPlatformServices::mapPathToOhosUriInJsThread(defaultFilePath.toStdString()));
            if (!fileSuffixChoices.isEmpty())
                documentSaveOptions.Set("fileSuffixChoices", QNapi::makeArray(env, fileSuffixChoices, std::mem_fn(&QString::toStdString)));

            constexpr auto minSupportedAutoCreateEmptyFilePropertyOhosSdkApiVersion = 23;
            if (QOhosDeviceInfo::sdkApiVersion() >= minSupportedAutoCreateEmptyFilePropertyOhosSdkApiVersion)
                documentSaveOptions.Set("autoCreateEmptyFile", false);

            startOhosFilePicker(
                jsState, getQAbilityPeerForOptInstanceId(jsState, qAbilityInstanceId), optContextJsWinId,
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
    QtOhos::InternalWindowId contextWinId, QString filePath,
    QOhosConsumer<bool> resultCallback)
{
    auto sharedResultCallback = QtOhos::moveToSharedPtr(std::move(resultCallback));

    auto qAbilityInstanceId =
        QWindowProxyRegistry::instance().tryFindQAbilityInstanceIdByInternalWindowId(contextWinId);
    auto optContextJsWinId = QWindowProxyRegistry::instance().tryMapInternalWindowIdToJsWindowId(contextWinId);

    QtOhos::invokeInJsThread(
        [qAbilityInstanceId, optContextJsWinId, filePath, sharedResultCallback](QtOhos::JsState &jsState) {
            auto documentSelectOptions = QNapi::makeObject(
                jsState.env(),
                {
                    {"defaultFilePathUri", QOhosPlatformServices::mapPathToOhosUriInJsThread(filePath.toStdString())},
                    {"authMode", true},
                });

            startOhosFilePicker(
                jsState, getQAbilityPeerForOptInstanceId(jsState, qAbilityInstanceId), optContextJsWinId,
                "select", documentSelectOptions,
                [sharedResultCallback](auto optResult) {
                    auto optSelectedUrls = qTransform(
                        optResult,
                        [](const auto &result) {
                            return mapFilePathsToQtUrls(result.resultPaths);
                        });
                    QtOhos::invokeInQtThread(
                        [sharedResultCallback, optSelectedUrls]() {
                            bool authorized = optSelectedUrls.hasValue() && !optSelectedUrls.value().isEmpty();
                            (*sharedResultCallback)(authorized);
                        });
                });
        });
}

}

QT_END_NAMESPACE
