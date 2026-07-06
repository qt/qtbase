// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosjsutils.h"
#include "qohosplatformservices.h"
#include "qohossharekit.h"
#include "qohosudmfconversions.h"
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <qohosjsenv_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <cstdint>

QT_BEGIN_NAMESPACE

namespace QOhosShareKit {

namespace {

QOhosOptional<QNapi::Object> tryMakeShareKitSharedDataRecordObject(
    QtOhos::JsState &jsState, const SharedRecord &record)
{
    auto optUtdType = record.mimeType != mimeTextUriList
        ? tryMapMimeTypeToUtdTypeId(record.mimeType)
        : QOhosOptional<std::string>(QOhosUdsMeta<::OH_UdsHyperlink>::udmfMetaId);

    if (!optUtdType.has_value()) {
        qOhosPrintfWarning(
            "%s: Cannot convert mime type: %s to utd type.", Q_FUNC_INFO, record.mimeType.c_str());
        return makeEmptyQOhosOptional();
    }

    std::vector<std::pair<std::string, QNapi::ValueWrapper>> objectProperties = {
        {"utd", optUtdType.value()},
    };

    if (record.content.has_value()) {
        objectProperties.emplace_back("content", record.content.value());
    } else if (record.filePath.has_value()) {
        objectProperties.emplace_back(
            "uri",
            QOhosPlatformServices::mapPathToOhosUriInJsThread(record.filePath.value()));
    } else {
        qOhosPrintfWarning("%s: Record doesn't have content nor uri.", Q_FUNC_INFO);
        return makeEmptyQOhosOptional();
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
            QOhosPlatformServices::mapPathToOhosUriInJsThread(record.thumbnailFilePath.value()));
    }

    if (record.extraData.has_value()) {
        objectProperties.emplace_back(
            "extraData",
            QOhosJsEnv::toNapiValue(jsState.env(), QJsonObject::fromVariantMap(record.extraData.value())));
    }

    return makeQOhosOptional(QNapi::makeObject(jsState.env(), objectProperties));
}

QOhosOptional<QNapi::Object> tryMakeShareKitShareControllerAnchorObject(
    QtOhos::JsState &jsState, const ShareControllerAnchor &anchor)
{
    if (anchor.windowOffset.isNull())
        return makeEmptyQOhosOptional();

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

    return makeQOhosOptional(shareControllerAnchorObject);
}

QNapi::Object makeShareKitControllerOptionsObject(
    QtOhos::JsState &jsState, ControllerOptions controllerOptions)
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
        [shareCompletedCallback = std::move(shareCompletedCallback)](const QtOhos::CallbackInfo &cbInfo) {
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
    QtOhos::JsState &jsState, QNapi::Object uiAbility,
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
        [](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
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
            ? makeQOhosOptional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
            : makeEmptyQOhosOptional();

    auto panelClosedJsCallback = makeAsyncConsumer(
        std::move(panelClosedCallback), &QtOhos::invokeInQtThread);
    auto shareCompletedJsCallback = makeAsyncConsumer(
        std::move(shareCompletedCallback), &QtOhos::invokeInQtThread);

    return QtOhos::evalInJsThreadWithPromise<std::shared_ptr<void>>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<std::shared_ptr<void>> evalPromise) {
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
                std::move(shareCompletedJsCallback),
                [evalPromise = QtOhos::moveToSharedPtr(std::move(evalPromise))](std::shared_ptr<void> shareCallbacksHandle) {
                    (*evalPromise)(
                        shareCallbacksHandle
                            ? QtOhos::makeProxyWithJsThreadDeleter(std::move(shareCallbacksHandle))
                            : nullptr);
                });
        },
        Q_FUNC_INFO);
}

}

QT_END_NAMESPACE
