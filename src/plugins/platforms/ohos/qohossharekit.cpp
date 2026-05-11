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

    if (!optUtdType.hasValue()) {
        qOhosPrintfWarning(
            "%s: Cannot convert mime type: %s to utd type.", Q_FUNC_INFO, record.mimeType.c_str());
        return makeEmptyQOhosOptional();
    }

    std::vector<std::pair<std::string, QNapi::ValueWrapper>> objectProperties = {
        {"utd", optUtdType.value()},
    };

    if (record.content.hasValue()) {
        objectProperties.emplace_back("content", record.content.value());
    } else if (record.filePath.hasValue()) {
        objectProperties.emplace_back(
            "uri",
            QOhosPlatformServices::mapPathToOhosUriInJsThread(record.filePath.value()));
    } else {
        qOhosPrintfWarning("%s: Record doesn't have content nor uri.", Q_FUNC_INFO);
        return makeEmptyQOhosOptional();
    }

    if (record.title.hasValue())
        objectProperties.emplace_back("title", record.title.value());

    if (record.label.hasValue())
        objectProperties.emplace_back("label", record.label.value());

    if (record.description.hasValue())
        objectProperties.emplace_back("description", record.description.value());

    if (record.thumbnail.hasValue()) {
        auto thumbnail = record.thumbnail.value();
        auto napiThumbnail = QNapi::TypedArrayOf<std::uint8_t>::New(jsState.env(), thumbnail.length());
        std::memcpy(napiThumbnail.Data(), thumbnail.data(), thumbnail.length());

        objectProperties.emplace_back("thumbnail", napiThumbnail);
    }

    if (record.thumbnailFilePath.hasValue()) {
        objectProperties.emplace_back(
            "thumbnailUri",
            QOhosPlatformServices::mapPathToOhosUriInJsThread(record.thumbnailFilePath.value()));
    }

    if (record.extraData.hasValue()) {
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

    if (anchor.size.hasValue() && anchor.size.value().isValid()) {
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

    if (controllerOptions.selectionMode.hasValue()) {
        controllerOptionsObject.set(
            "selectionMode", jsState.mapOhosEnumToJs(controllerOptions.selectionMode.value()));
    }

    if (controllerOptions.anchor.hasValue()) {
        auto controllerAnchorObject = tryMakeShareKitShareControllerAnchorObject(
            jsState, controllerOptions.anchor.value());
        if (controllerAnchorObject.hasValue())
            controllerOptionsObject.set("anchor", controllerAnchorObject.value());
    }

    if (controllerOptions.previewMode.hasValue()) {
        controllerOptionsObject.set(
            "previewMode", jsState.mapOhosEnumToJs(controllerOptions.previewMode.value()));
    }

    if (controllerOptions.excludedAbilities.hasValue()) {
        std::vector<QNapi::ValueWrapper> jsExcludedAbilities;
        for (auto excludedAbilityType : controllerOptions.excludedAbilities.value())
            jsExcludedAbilities.push_back(jsState.mapOhosEnumToJs(excludedAbilityType));
        controllerOptionsObject.set(
            "excludedAbilities", QNapi::makeArray(jsState.env(), jsExcludedAbilities));
    }

    return controllerOptionsObject;
}

void shareDataImpl(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QUiAbilityPeer> uiAbilityPeer,
    const std::vector<SharedRecord> &recordsToShare, ControllerOptions controllerOptions,
    std::function<void()> panelClosedCallback, QOhosConsumer<std::shared_ptr<void>> resultConsumer)
{
    qOhosPrintfDebug("%s: sharing %lu records through ShareKit", Q_FUNC_INFO, recordsToShare.size());

    if (recordsToShare.empty()) {
        qOhosPrintfWarning("%s: No records to share. Skipping...", Q_FUNC_INFO);
        resultConsumer(nullptr);
        return;
    }

    auto firstRecord = tryMakeShareKitSharedDataRecordObject(jsState, recordsToShare.front());
    if (!firstRecord.hasValue()) {
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
        if (record.hasValue()) {
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

    auto dissmissCallbackHandle = QtOhos::registerOnOffMethodsBasedEventHandler(
        controller, "dismiss", std::move(panelClosedCallback));

    controller.call<QNapi::Promise>(
        "show",
        {
            uiAbilityPeer->qAbility().get<QNapi::Object>("context"),
            makeShareKitControllerOptionsObject(jsState, controllerOptions),
        })
    .withContext(std::move(resultConsumer))
    .onThenWithContext(
        [dissmissCallbackHandle](auto &resultConsumer) {
            resultConsumer(dissmissCallbackHandle);
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
    ControllerOptions controllerOptions, std::function<void()> panelClosedCallback)
{
    auto optMainWindowInstanceObjectRef =
        optInstanceMainWindow != nullptr
            ? makeQOhosOptional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
            : makeEmptyQOhosOptional();

    auto panelClosedJsCallback = makeAsyncConsumer(
        std::move(panelClosedCallback), &QtOhos::invokeInQtThread);

    return QtOhos::evalInJsThreadWithConsumer<std::shared_ptr<void>>(
        [&](QtOhos::JsState &jsState, QOhosConsumer<std::shared_ptr<void>> resultConsumer) {
            auto uiAbilityPeer = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(
                optMainWindowInstanceObjectRef.hasValue()
                    ? jsState.tryGetQAbilityPeerByQWindow(optMainWindowInstanceObjectRef.value())
                    : jsState.defaultQAbilityPeer());
            if (!uiAbilityPeer) {
                qOhosPrintfWarning("%s only UIAbilities can share data with ShareKit", Q_FUNC_INFO);
                resultConsumer(nullptr);
                return;
            }

            shareDataImpl(
                jsState, uiAbilityPeer, recordsToShare, controllerOptions, std::move(panelClosedJsCallback),
                std::move(resultConsumer));
        });
}

}

QT_END_NAMESPACE
