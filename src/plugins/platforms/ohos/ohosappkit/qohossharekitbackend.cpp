// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohossharekitbackend_p.h"
#include "qohosjsenv_p.h"
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohosjstools_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/private/qohospathutils_p.h>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <database/udmf/udmf_meta.h>
#include <database/udmf/utd.h>

QT_BEGIN_NAMESPACE

namespace QOhosShareKit {

namespace {

std::optional<std::string> tryMapMimeTypeToUtdTypeId(const std::string &mimeType)
{
    // FIXME: the `utdGetTypesByMimeType()` result interpretation is not quite clear.
    // Documentaion does not mention why there is a list of possible type ids.
    // Are they ordered in a specific manear? Is it possible to get en ampty list?
    // Improve and fix the code when these are known.
    unsigned int typesCount = 0;
    const char **utdTypeIds = ::OH_Utd_GetTypesByMimeType(mimeType.c_str(), &typesCount);
    if (utdTypeIds == nullptr && typesCount != 0) {
        qOhosReportFatalErrorAndAbort(
            "%s: got inconsistent result from OH_Utd_GetTypesByMimeType() call: array is null, size is %u",
            Q_FUNC_INFO, typesCount);
    }

    auto utdTypeIdsList = utdTypeIds != nullptr
        ? std::vector<std::string>(utdTypeIds, utdTypeIds + typesCount)
        : std::vector<std::string>();

    ::OH_Utd_DestroyStringList(utdTypeIds, typesCount);

    return !utdTypeIdsList.empty()
        ? std::make_optional(utdTypeIdsList.front())
        : std::nullopt;
}

std::optional<QNapi::Object> tryMakeShareKitSharedDataRecordObject(
    QOhosJsState &jsState, const SharedRecord &record)
{
    auto optUtdType = record.mimeType != mimeTextUriList
        ? tryMapMimeTypeToUtdTypeId(record.mimeType)
        : std::optional<std::string>(UDMF_META_HYPERLINK);

    if (!optUtdType.has_value()) {
        qOhosPrintfWarning(
            "%s: Cannot convert mime type: %s to utd type.", Q_FUNC_INFO, record.mimeType.c_str());
        return std::nullopt;
    }

    std::vector<std::pair<std::string, QNapi::ValueWrapper>> objectProperties = {
        {"utd", optUtdType.value()},
    };

    if (record.content.has_value()) {
        objectProperties.emplace_back("content", record.content.value());
    } else if (record.filePath.has_value()) {
        objectProperties.emplace_back(
            "uri",
            tryMapPathToOhosFileUri(record.filePath.value()).value_or(""));
    } else {
        qOhosPrintfWarning("%s: Record doesn't have content nor uri.", Q_FUNC_INFO);
        return std::nullopt;
    }

    if (record.title.has_value())
        objectProperties.emplace_back("title", record.title.value());

    if (record.label.has_value())
        objectProperties.emplace_back("label", record.label.value());

    if (record.description.has_value())
        objectProperties.emplace_back("description", record.description.value());

    if (record.thumbnail.has_value()) {
        auto thumbnail = record.thumbnail.value();
        auto napiThumbnail = QNapi::TypedArrayOf<std::uint8_t>::New(jsState.env(), thumbnail.length());
        std::memcpy(napiThumbnail.Data(), thumbnail.data(), thumbnail.length());

        objectProperties.emplace_back("thumbnail", napiThumbnail);
    }

    if (record.thumbnailFilePath.has_value()) {
        objectProperties.emplace_back(
            "thumbnailUri",
            tryMapPathToOhosFileUri(record.thumbnailFilePath.value()).value_or(""));
    }

    if (record.extraData.has_value()) {
        objectProperties.emplace_back(
            "extraData",
            QOhosJsEnv::toNapiValue(jsState.env(), QJsonObject::fromVariantMap(record.extraData.value())));
    }

    return std::make_optional(QNapi::makeObject(jsState.env(), objectProperties));
}

std::optional<QNapi::Object> tryMakeShareKitShareControllerAnchorObject(
    QOhosJsState &jsState, const ShareControllerAnchor &anchor)
{
    if (anchor.windowOffset.isNull())
        return std::nullopt;

    auto shareControllerAnchorObject = QNapi::makeObject(
        jsState.env(),
        {
            {
                "windowOffset",
                QNapi::makeObject(
                    jsState.env(),
                    {
                        {"x", anchor.windowOffset.x()},
                        {"y", anchor.windowOffset.y()},
                    })
            },
        });

    if (anchor.size.has_value() && anchor.size.value().isValid()) {
        shareControllerAnchorObject.set(
            "size",
            QNapi::makeObject(
                jsState.env(),
                {
                    {"width", anchor.size.value().width()},
                    {"height", anchor.size.value().height()},
                }));
    }

    return std::make_optional(shareControllerAnchorObject);
}

QNapi::Object makeShareKitControllerOptionsObject(
    QOhosJsState &jsState, ControllerOptions controllerOptions)
{
    auto controllerOptionsObject = QNapi::makeObject(jsState.env());

    if (controllerOptions.selectionMode.has_value()) {
        controllerOptionsObject.set(
            "selectionMode", jsState.mapOhosEnumToJs(controllerOptions.selectionMode.value()));
    }

    if (controllerOptions.anchor.has_value()) {
        auto controllerAnchorObject = tryMakeShareKitShareControllerAnchorObject(
            jsState, controllerOptions.anchor.value());
        if (controllerAnchorObject.has_value())
            controllerOptionsObject.set("anchor", controllerAnchorObject.value());
    }

    if (controllerOptions.previewMode.has_value()) {
        controllerOptionsObject.set(
            "previewMode", jsState.mapOhosEnumToJs(controllerOptions.previewMode.value()));
    }

    if (controllerOptions.excludedAbilities.has_value()) {
        controllerOptionsObject.set(
            "excludedAbilities",
            QNapi::makeArray(
                jsState.env(), controllerOptions.excludedAbilities.value(),
                [&](auto excludedAbilityType) {
                    return jsState.mapOhosEnumToJs(excludedAbilityType);
                }));
    }

    return controllerOptionsObject;
}

std::shared_ptr<void> registerOnOffShareCompletedEventHandler(
    QNapi::Object shareController, QOhosConsumer<ShareOperationResult> shareCompletedCallback)
{
    return registerQOhosOnOffMethodsBasedEventHandler(
        shareController, "shareCompleted",
        [shareCompletedCallback = std::move(shareCompletedCallback)](const QNapi::CallbackInfo &cbInfo) {
            const auto shareOperationResult = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
            const auto targetAbilityName = shareOperationResult.eval<QNapi::String>(
                "targetAbilityInfo.name");
            shareCompletedCallback(
                ShareOperationResult{
                    .targetAbilityName = targetAbilityName,
                });
        });
}

void shareDataImpl(
    QOhosJsState &jsState, QNapi::Object uiAbility,
    const std::vector<SharedRecord> &recordsToShare, ControllerOptions controllerOptions,
    std::function<void()> panelClosedCallback, QOhosConsumer<ShareOperationResult> shareCompletedCallback,
    QOhosConsumer<std::shared_ptr<void>> resultConsumer)
{
    qOhosPrintfDebug("%s: sharing %lu records through ShareKit", Q_FUNC_INFO, recordsToShare.size());

    if (recordsToShare.empty()) {
        qOhosPrintfWarning("%s: No records to share. Skipping...", Q_FUNC_INFO);
        resultConsumer(nullptr);
        return;
    }

    auto firstRecord = tryMakeShareKitSharedDataRecordObject(jsState, recordsToShare.front());
    if (!firstRecord.has_value()) {
        qOhosPrintfWarning("%s: Failed to create the very first record, skip sharing", Q_FUNC_INFO);
        resultConsumer(nullptr);
        return;
    }
    qOhosPrintfDebug(
        "%s: record #0 to be shared: %s",
        Q_FUNC_INFO, QNapi::toJsonString(firstRecord.value()).c_str());

    auto sharedDataObject = jsState.eval<QNapi::Object>(
        "@kit.ShareKit.systemShare.SharedData<new>(*)", {firstRecord.value()});

    for (std::size_t i = 1; i < recordsToShare.size(); i++) {
        auto record = tryMakeShareKitSharedDataRecordObject(jsState, recordsToShare.at(i));
        if (record.has_value()) {
            qOhosPrintfDebug(
                "%s: record #%zu to be shared: %s",
                Q_FUNC_INFO, i, QNapi::toJsonString(record.value()).c_str());
            sharedDataObject.call("addRecord", {record.value()});
        } else {
            qOhosPrintfWarning("%s: Failed to create record", Q_FUNC_INFO);
        }
    }

    auto controller = jsState.eval<QNapi::Object>(
        "@kit.ShareKit.systemShare.ShareController<new>(*)", {sharedDataObject});

    auto callbacksHandle = QtOhos::moveToSharedPtr(
        std::vector<std::shared_ptr<void>>{
            registerQOhosOnOffMethodsBasedEventHandler(
                controller, "dismiss", std::move(panelClosedCallback)),
            registerOnOffShareCompletedEventHandler(controller, std::move(shareCompletedCallback)),
        });

    controller.evalToPromiseOrRejectOnThrow(
        "show(*)",
        {
            uiAbility.get<QNapi::Object>("context"),
            makeShareKitControllerOptionsObject(jsState, controllerOptions),
        })
    .withContext(std::move(resultConsumer))
    .onThenWithContext(
        [callbacksHandle](auto &resultConsumer) {
            resultConsumer(callbacksHandle);
        })
    .onCatchWithContext(
        [](const QOhosCallbackInfo &cbInfo, auto &resultConsumer) {
            QtOhos::logJsCallbackError(cbInfo, "ShareController.show()");
            resultConsumer(nullptr);
        });
}

template<typename ...Ts>
QOhosConsumer<Ts...> makeAsyncConsumer(
    QOhosConsumer<Ts...> baseConsumer, QOhosConsumer<std::function<void()>> asyncExecutor)
{
    auto sharedBaseConsumer = QtOhos::moveToSharedPtr(std::move(baseConsumer));
    return [sharedBaseConsumer, asyncExecutor = std::move(asyncExecutor)](const Ts &...args) {
        asyncExecutor(
            [sharedBaseConsumer, args...]() mutable {
                (*sharedBaseConsumer)(std::move(args)...);
            });
    };
}

}

std::shared_ptr<void> shareData(
    QWindow *optInstanceMainWindow, const std::vector<SharedRecord> &recordsToShare,
    ControllerOptions controllerOptions, std::function<void()> panelClosedCallback,
    QOhosConsumer<ShareOperationResult> shareCompletedCallback)
{
    auto optMainWindowInstanceObjectRef =
        optInstanceMainWindow != nullptr
            ? std::make_optional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
            : std::nullopt;

    auto panelClosedJsCallback = makeAsyncConsumer(
        std::move(panelClosedCallback), &QtOhos::invokeInQtThread);
    auto shareCompletedJsCallback = makeAsyncConsumer(
        std::move(shareCompletedCallback), &QtOhos::invokeInQtThread);

    return QOhosJsThreadGateway::evalWithPromise<std::shared_ptr<void>>(
        [&](QOhosJsState &jsState, QOhosTaskPromise<std::shared_ptr<void>> evalPromise) {
            auto optUiAbility = optMainWindowInstanceObjectRef.has_value()
                ? jsState.tryGetQAbilityByQWindow(optMainWindowInstanceObjectRef.value())
                : jsState.defaultQAbility();
            if (!optUiAbility.has_value()) {
                qOhosPrintfWarning("%s: no UIAbility available to share data with ShareKit", Q_FUNC_INFO);
                evalPromise(nullptr);
                return;
            }

            shareDataImpl(
                jsState, optUiAbility.value(), recordsToShare, controllerOptions, std::move(panelClosedJsCallback),
                std::move(shareCompletedJsCallback), std::move(evalPromise));
        },
        Q_FUNC_INFO);
}

}

QT_END_NAMESPACE
