// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoswantinfo_p.h"

#include <QtHarmonyExtras/private/qohosenums_p.h>
#include <QtHarmonyExtras/private/qohosjsenv_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <database/udmf/udmf_meta.h>
#include <database/udmf/utd.h>

QT_BEGIN_NAMESPACE

namespace QtHarmonyExtras::Private {

detail::WantInfoPriv::WantInfoPriv() = default;
detail::WantInfoPriv::~WantInfoPriv() = default;

namespace {

template<typename T>
std::optional<T> getOptionalProperty(const QNapi::Object &object, const std::string &propName)
{
    auto optPropValue = QNapi::getOptionalPropOrEmpty<T>(object, propName);
    return !optPropValue.IsEmpty()
        ? std::make_optional(optPropValue)
        : std::nullopt;
}

std::optional<std::string> tryMapUtdTypeIdToMimeType(const std::string &utdTypeId)
{
    std::shared_ptr<::OH_Utd> utd(
        ::OH_Utd_Create(utdTypeId.c_str()),
        [](::OH_Utd *utd) {
            if (utd != nullptr)
                ::OH_Utd_Destroy(utd);
        });

    std::vector<std::string> mimeTypes;
    if (utd) {
        unsigned int typesCount = 0;
        const char **rawMimeTypes = ::OH_Utd_GetMimeTypes(utd.get(), &typesCount);
        if (rawMimeTypes == nullptr && typesCount != 0) {
            qOhosReportFatalErrorAndAbort(
                "%s: got inconsistent result from OH_Utd_GetMimeTypes() call: array is null, size is %u",
                Q_FUNC_INFO, typesCount);
        }
        if (rawMimeTypes != nullptr)
            mimeTypes = std::vector<std::string>(rawMimeTypes, rawMimeTypes + typesCount);
    }

    return !mimeTypes.empty()
        ? std::make_optional(mimeTypes.front())
        : std::nullopt;
}

std::optional<detail::SharedRecord> tryConvertNapiObjectToSharedRecord(QNapi::Object record)
{
    auto tryGetOptionalStringProp = [](const QNapi::Object &object, const std::string &propName) -> std::optional<QString> {
        auto optProp = getOptionalProperty<QNapi::String>(object, propName);
        return optProp.has_value()
            ? std::make_optional(QString::fromStdString(optProp.value()))
            : std::nullopt;
    };

    auto tryGetOptionalByteArrayProp = [](const QNapi::Object &object, const std::string &propName) -> std::optional<QByteArray> {
        auto optProp = getOptionalProperty<QNapi::TypedArrayOf<std::uint8_t>>(object, propName);
        return optProp.has_value()
            ? std::make_optional(QByteArray(
                  reinterpret_cast<const char *>(optProp.value().Data()),
                  optProp.value().ByteLength()))
            : std::nullopt;
    };

    auto tryGetOptionalJsonObjectProp = [](const QNapi::Object &object, const std::string &propName) -> std::optional<QJsonObject> {
        auto optProp = getOptionalProperty<QNapi::Object>(object, propName);
        return optProp.has_value()
            ? std::make_optional(QOhosJsEnv::fromNapiValue<QJsonObject>(optProp.value()))
            : std::nullopt;
    };

    std::string utd = record.get<QNapi::String>("utd");
    auto optMimeType = utd != UDMF_META_HYPERLINK
        ? tryMapUtdTypeIdToMimeType(utd)
        : std::optional<std::string>("text/uri-list");
    if (!optMimeType.has_value()) {
        qOhosPrintfWarning(
            "%s: can't map utd '%s' to mimetype, not mapping the record",
            Q_FUNC_INFO, utd.c_str());
        return std::nullopt;
    }

    auto content = tryGetOptionalStringProp(record, "content");
    auto uri = tryGetOptionalStringProp(record, "uri");
    if (!content.has_value() && !uri.has_value()) {
        qOhosPrintfWarning(
            "%s: cannot create Shared Record, content and uri properties are empty", Q_FUNC_INFO);
        return std::nullopt;
    }

    auto optExtraDataJson = tryGetOptionalJsonObjectProp(record, "extraData");

    return std::make_optional(
        detail::SharedRecord{
            .mimeType = QString::fromStdString(optMimeType.value()),
            .content = content,
            .filePath = uri,
            .title = tryGetOptionalStringProp(record, "title"),
            .label = tryGetOptionalStringProp(record, "label"),
            .description = tryGetOptionalStringProp(record, "description"),
            .thumbnail = tryGetOptionalByteArrayProp(record, "thumbnail"),
            .thumbnailFilePath = tryGetOptionalStringProp(record, "thumbnailUri"),
            .extraData = optExtraDataJson.has_value()
                ? std::make_optional(optExtraDataJson.value().toVariantMap())
                : std::nullopt,
        });
}

std::optional<detail::WantInfoPriv::LaunchReason> tryMapOhosLaunchReasonToWantInfoEnum(
    QtOhos::enums::ohos::app::ability::AbilityConstant::LaunchReason ohosLaunchReason)
{
    using OhosLaunchReason = QtOhos::enums::ohos::app::ability::AbilityConstant::LaunchReason;
    using WantInfoPriv = detail::WantInfoPriv;

    switch (ohosLaunchReason) {
    case OhosLaunchReason::START_ABILITY:
        return std::make_optional(WantInfoPriv::LaunchReason::START_ABILITY);
    case OhosLaunchReason::CONTINUATION:
        return std::make_optional(WantInfoPriv::LaunchReason::CONTINUATION);
    case OhosLaunchReason::PREPARE_CONTINUATION:
        return std::make_optional(WantInfoPriv::LaunchReason::PREPARE_CONTINUATION);
    case OhosLaunchReason::PRELOAD:
        return std::make_optional(WantInfoPriv::LaunchReason::PRELOAD);
    case OhosLaunchReason::UNKNOWN:
    case OhosLaunchReason::CALL:
    case OhosLaunchReason::APP_RECOVERY:
    case OhosLaunchReason::SHARE:
    case OhosLaunchReason::AUTO_STARTUP:
    case OhosLaunchReason::INSIGHT_INTENT:
        return std::make_optional(WantInfoPriv::LaunchReason::UNKNOWN);
    }

    return {};
}

detail::WantInfoPriv::LaunchReason mapJsLaunchReasonToWantInfoEnumWithFallback(
    QOhosJsState &jsState, QNapi::Number jsLaunchReason)
{
    auto optLaunchReasonJsEnum =
        jsState.tryMapOhosEnumFromJs<QtOhos::enums::ohos::app::ability::AbilityConstant::LaunchReason>(jsLaunchReason);
    auto optLaunchReason =
        optLaunchReasonJsEnum.has_value()
            ? tryMapOhosLaunchReasonToWantInfoEnum(optLaunchReasonJsEnum.value())
            : std::nullopt;
    return optLaunchReason.value_or(detail::WantInfoPriv::LaunchReason::UNKNOWN);
}

class WantInfoImpl : public detail::WantInfoPriv
{
public:
    WantInfoImpl(QNapi::Object want, LaunchReason launchReason);

    QJsonObject jsonObject() const override;

    std::optional<QList<detail::SharedRecord>> tryGetSharedDataRecords() const override;

    std::optional<ContactInfo> tryGetContactInfo() const override;

    LaunchReason launchReason() const override;

private:
    struct JsScopeData
    {
        QNapi::Reference<QNapi::Object> want;
    };

    std::shared_ptr<JsScopeData> m_jsScopeData;
    QJsonObject m_jsonObject;
    LaunchReason m_launchReason;
};

WantInfoImpl::WantInfoImpl(QNapi::Object want, LaunchReason launchReason)
    : detail::WantInfoPriv()
    , m_jsScopeData(
        QtOhos::makeProxyWithJsThreadDeleter(
            QtOhos::moveToSharedPtr(
                JsScopeData {
                    .want = QNapi::Reference<>::makePersistentFrom(want),
                })))
    , m_jsonObject(QOhosJsEnv::fromNapiValue<QJsonObject>(want))
    , m_launchReason(launchReason)
{
}

QJsonObject WantInfoImpl::jsonObject() const
{
    return m_jsonObject;
}

std::optional<QList<detail::SharedRecord>> WantInfoImpl::tryGetSharedDataRecords() const
{
    using SharedRecord = detail::SharedRecord;

    return QOhosJsThreadGateway::evalWithPromise<std::optional<QList<SharedRecord>>>(
        [&](QOhosJsState &jsState, QOhosTaskPromise<std::optional<QList<SharedRecord>>> evalPromise) {
            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            jsState.evalToPromiseOrRejectOnThrow(
                "@kit.ShareKit.systemShare.getSharedData(*)", {m_jsScopeData->want.Value()})
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QOhosCallbackInfo &cbInfo) {
                    QNapi::Object sharedData = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

                    auto optRecords = QNapi::getArrayElements<QList<std::optional<SharedRecord>>, QNapi::Object>(
                        sharedData.eval<QNapi::Array>("getRecords()"), &tryConvertNapiObjectToSharedRecord);

                    QList<SharedRecord> records;
                    for (const auto &optRecord : optRecords) {
                        if (optRecord.has_value())
                            records.append(optRecord.value());
                    }

                    std::size_t unconvertedRecordsCount = optRecords.size() - records.size();
                    if (unconvertedRecordsCount != 0) {
                        qOhosPrintfWarning(
                            "%s: can't convert %zu Shared Records, ignoring them",
                            Q_FUNC_INFO, unconvertedRecordsCount);
                    }

                    thenPromise(std::make_optional(records));
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QOhosCallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "@kit.ShareKit.systemShare.getSharedData() failed");
                    catchPromise(std::nullopt);
                });
        },
        Q_FUNC_INFO);
}

std::optional<detail::WantInfoPriv::ContactInfo> WantInfoImpl::tryGetContactInfo() const
{
    using ContactInfo = detail::WantInfoPriv::ContactInfo;

    return QOhosJsThreadGateway::evalWithPromise<std::optional<ContactInfo>>(
        [&](QOhosJsState &jsState, QOhosTaskPromise<std::optional<ContactInfo>> evalPromise) {
            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            jsState.evalToPromiseOrRejectOnThrow(
                "@kit.ShareKit.systemShare.getContactInfo(*)", {m_jsScopeData->want.Value()})
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QOhosCallbackInfo &cbInfo) {
                    auto contactInfoObj = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                    ContactInfo contactInfo = {
                        .contactType = QString::fromStdString(
                            contactInfoObj.get<QNapi::String>("contactType")),
                        .contactId = QString::fromStdString(
                            contactInfoObj.get<QNapi::String>("contactId")),
                    };
                    thenPromise(std::make_optional(contactInfo));
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QOhosCallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "@kit.ShareKit.systemShare.getContactInfo() failed");
                    catchPromise(std::nullopt);
                });
        },
        Q_FUNC_INFO);
}

detail::WantInfoPriv::LaunchReason WantInfoImpl::launchReason() const
{
    return m_launchReason;
}

}

QSharedPointer<detail::WantInfoPriv> makeAppLaunchWantInfo()
{
    return QOhosJsThreadGateway::eval(
        [](QOhosJsState &jsState) {
            auto optAppLaunchParam = jsState.optAppLaunchParam();
            auto optAppLaunchReason = optAppLaunchParam.has_value()
                ? std::make_optional(mapJsLaunchReasonToWantInfoEnumWithFallback(
                      jsState, optAppLaunchParam.value().get<QNapi::Number>("launchReason")))
                : std::nullopt;
            auto appLaunchReason = optAppLaunchReason.value_or(detail::WantInfoPriv::LaunchReason::UNKNOWN);
            return QSharedPointer<WantInfoImpl>::create(jsState.appLaunchWant(), appLaunchReason);
        },
        Q_FUNC_INFO);
}

void addNewWantConsumer(QObject *context, QOhosConsumer<QJsonObject> wantConsumer)
{
    auto sharedWantConsumer = QtOhos::moveToSharedPtr(std::move(wantConsumer));
    addNewWantConsumer(
        context,
        [sharedWantConsumer](QSharedPointer<detail::WantInfoPriv> wantInfo) {
            (*sharedWantConsumer)(wantInfo->jsonObject());
        });
}

void addNewWantConsumer(
    QObject *context, QOhosConsumer<QSharedPointer<detail::WantInfoPriv>> wantConsumer)
{
    auto contextRef = QtOhos::makeQThreadSafeRef(context);
    auto sharedWantConsumer = QtOhos::moveToSharedPtr(std::move(wantConsumer));
    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
            jsState.addNewWantConsumer(
                [contextRef, sharedWantConsumer](QOhosJsState &jsState, QNapi::Object napiWant, QNapi::Object launchParam) {
                    auto launchReason = mapJsLaunchReasonToWantInfoEnumWithFallback(
                        jsState, launchParam.get<QNapi::Number>("launchReason"));
                    auto wantInfo = QSharedPointer<WantInfoImpl>::create(napiWant, launchReason);
                    contextRef.visitInQtThreadIfAlive(
                        [sharedWantConsumer, wantInfo](auto &) {
                            (*sharedWantConsumer)(wantInfo);
                        });
                });
        },
        Q_FUNC_INFO);
}

}

QT_END_NAMESPACE
