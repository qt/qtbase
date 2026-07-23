// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoswantutils_p.h"
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qmimedatabase.h>
#include <array>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

namespace {

namespace WantProps {
    constexpr auto deviceId = "deviceId";
    constexpr auto bundleName = "bundleName";
    constexpr auto moduleName = "moduleName";
    constexpr auto abilityName = "abilityName";
    constexpr auto uri = "uri";
    constexpr auto type = "type";
    constexpr auto action = "action";
    constexpr auto entities = "entities";
    constexpr auto flags = "flags";
    constexpr auto parameters = "parameters";
    constexpr auto fds = "fds";
}

static const std::array<std::pair<WantFlag, int>, 3> qtToOhosFlagsMappings {
    std::make_pair(WantFlag::AuthReadUriPermission, 0x00000001),
    std::make_pair(WantFlag::AuthWriteUriPermission, 0x00000002),
    std::make_pair(WantFlag::InstallOnDemand, 0x00000800),
};

int convertToOhosFlags(const WantFlags &flags)
{
    int result = 0;
    for (const auto &flagsMapping : qtToOhosFlagsMappings) {
        if (flags.testFlag(flagsMapping.first))
            result |= flagsMapping.second;
    }

    return result;
}

WantFlags convertFromOhosFlags(int intFlags)
{
    WantFlags flags = {};
    for (const auto &flagsMapping : qtToOhosFlagsMappings) {
        if ((intFlags & flagsMapping.second) != 0)
            flags |= flagsMapping.first;
    }

    return flags;
}

QString getWantOptionalStringProp(const QJsonObject &jsonWant, const char *propName)
{
    auto propIter = jsonWant.find(QLatin1String(propName));
    return propIter != jsonWant.end() && propIter->isString()
        ? propIter->toString()
        : QString();
}

WantInfo::LaunchReason mapLaunchReasonFromQpaFunctions(
    detail::WantInfoPriv::LaunchReason reason)
{
    using WantInfoPriv = detail::WantInfoPriv;

    switch (reason) {
    case WantInfoPriv::LaunchReason::UNKNOWN:
        return WantInfo::LaunchReason::Unknown;
    case WantInfoPriv::LaunchReason::START_ABILITY:
        return WantInfo::LaunchReason::StartAbility;
    case WantInfoPriv::LaunchReason::CONTINUATION:
        return WantInfo::LaunchReason::Continuation;
    case WantInfoPriv::LaunchReason::PREPARE_CONTINUATION:
        return WantInfo::LaunchReason::PrepareContinuation;
    case WantInfoPriv::LaunchReason::PRELOAD:
        return WantInfo::LaunchReason::Preload;
    }

    return WantInfo::LaunchReason::Unknown;
}

class QOhosWantInfoImpl : public WantInfo
{
public:
    QOhosWantInfoImpl(QSharedPointer<detail::WantInfoPriv> want);

    Want want() const override;

    std::optional<QList<QSharedPointer<ShareKit::SharedRecord>>> tryGetSharedRecordsFromShareKit() const override;

    std::optional<ContactInfo> tryGetContactInfo() const override;

    LaunchReason launchReason() const override;

    QSharedPointer<detail::WantInfoPriv> qpaWantInfo() const;

private:
    QSharedPointer<detail::WantInfoPriv> m_qpaWantInfo;
};

QOhosWantInfoImpl::QOhosWantInfoImpl(QSharedPointer<detail::WantInfoPriv> want)
    : WantInfo()
    , m_qpaWantInfo(want)
{
}

Want QOhosWantInfoImpl::want() const
{
    return convertWantFromJsonObject(m_qpaWantInfo->jsonObject());
}

std::optional<QList<QSharedPointer<ShareKit::SharedRecord>>> QOhosWantInfoImpl::tryGetSharedRecordsFromShareKit() const
{
    auto records = qpaWantInfo()->tryGetSharedDataRecords();
    if (!records.has_value())
        return {};

    QList<QSharedPointer<ShareKit::SharedRecord>> result;
    for (auto &record : records.value()) {
        QSharedPointer<ShareKit::SharedRecord> extrasRecord;
        if (record.content.has_value()) {
            extrasRecord = ShareKit::createContentRecord(
                QMimeDatabase().mimeTypeForName(record.mimeType), record.content.value());
        } else if (record.filePath.has_value()) {
            extrasRecord = ShareKit::createFileRecord(QFileInfo(record.filePath.value()));
        } else {
            qOhosPrintfWarning("%s: record has no content nor file path, skip it", Q_FUNC_INFO);
            continue;
        }

        if (record.title.has_value())
            extrasRecord->setTitle(record.title.value());
        if (record.label.has_value())
            extrasRecord->setLabel(record.label.value());
        if (record.description.has_value())
            extrasRecord->setDescription(record.description.value());
        if (record.thumbnail.has_value())
            extrasRecord->setThumbnail(record.thumbnail.value());
        if (record.thumbnailFilePath.has_value())
            extrasRecord->setThumbnailFilePath(record.thumbnailFilePath.value());
        if (record.extraData.has_value())
            extrasRecord->setExtraData(record.extraData.value());

        result.push_back(extrasRecord);
    }

    return result;
}

std::optional<WantInfo::ContactInfo> QOhosWantInfoImpl::tryGetContactInfo() const
{
    auto optContactInfo = qpaWantInfo()->tryGetContactInfo();
    if (!optContactInfo.has_value())
        return {};

    return ContactInfo{
        .contactType = optContactInfo.value().contactType,
        .contactId = optContactInfo.value().contactId,
    };
}

WantInfo::LaunchReason QOhosWantInfoImpl::launchReason() const
{
    return mapLaunchReasonFromQpaFunctions(m_qpaWantInfo->launchReason());
}

QSharedPointer<detail::WantInfoPriv> QOhosWantInfoImpl::qpaWantInfo() const
{
    return m_qpaWantInfo;
}

}

QJsonObject convertWantToJsonObject(const Want &want)
{
    QJsonObject jsonWant;

    if (!want.deviceId.isEmpty())
        jsonWant.insert(QLatin1String(WantProps::deviceId), want.deviceId);
    if (!want.bundleName.isEmpty())
        jsonWant.insert(QLatin1String(WantProps::bundleName), want.bundleName);
    if (!want.moduleName.isEmpty())
        jsonWant.insert(QLatin1String(WantProps::moduleName), want.moduleName);
    if (!want.abilityName.isEmpty())
        jsonWant.insert(QLatin1String(WantProps::abilityName), want.abilityName);
    if (!want.uri.isEmpty())
        jsonWant.insert(QLatin1String(WantProps::uri), want.uri);
    if (!want.type.isEmpty())
        jsonWant.insert(QLatin1String(WantProps::type), want.type);
    if (!want.action.isEmpty())
        jsonWant.insert(QLatin1String(WantProps::action), want.action);
    jsonWant.insert(QLatin1String(WantProps::flags), convertToOhosFlags(want.flags));
    jsonWant.insert(QLatin1String(WantProps::entities), QJsonArray::fromStringList(want.entities));

    QJsonObject parametersObject = want.parameters;
    for (auto it = want.fds.cbegin(); it != want.fds.cend(); ++it) {
        QJsonObject fdEntry = {
            {QStringLiteral("type"), QStringLiteral("FD")},
            {QStringLiteral("value"), it.value()},
        };
        parametersObject.insert(it.key(), fdEntry);
    }
    jsonWant.insert(QLatin1String(WantProps::parameters), parametersObject);

    return jsonWant;
}

Want convertWantFromJsonObject(const QJsonObject &jsonWant)
{
    auto flagsIter = jsonWant.find(QLatin1String(WantProps::flags));
    auto flagsInt = flagsIter != jsonWant.end() && flagsIter->isDouble()
        ? flagsIter->toInt()
        : 0;

    auto entitiesIter = jsonWant.find(QLatin1String(WantProps::entities));
    auto entitiesArray = entitiesIter != jsonWant.end() && entitiesIter->isArray()
        ? entitiesIter->toArray()
        : QJsonArray();

    QStringList entities;
    for (const auto &entityValue : entitiesArray) {
        if (entityValue.isString())
            entities.append(entityValue.toString());
    }

    auto parametersIter = jsonWant.find(QLatin1String(WantProps::parameters));
    auto parametersObject = parametersIter != jsonWant.end() && parametersIter->isObject()
        ? parametersIter->toObject()
        : QJsonObject();

    auto fdsIter = jsonWant.find(QLatin1String(WantProps::fds));
    auto fdsObject = fdsIter != jsonWant.end() && fdsIter->isObject()
        ? fdsIter->toObject()
        : QJsonObject();
    QHash<QString, int> fds;
    for (auto it = fdsObject.constBegin(); it != fdsObject.constEnd(); ++it) {
        if (it.value().isDouble())
            fds.insert(it.key(), it.value().toInt());
    }

    Want want = {
        .deviceId = getWantOptionalStringProp(jsonWant, WantProps::deviceId),
        .bundleName = getWantOptionalStringProp(jsonWant, WantProps::bundleName),
        .moduleName = getWantOptionalStringProp(jsonWant, WantProps::moduleName),
        .abilityName = getWantOptionalStringProp(jsonWant, WantProps::abilityName),
        .uri = getWantOptionalStringProp(jsonWant, WantProps::uri),
        .type = getWantOptionalStringProp(jsonWant, WantProps::type),
        .action = getWantOptionalStringProp(jsonWant, WantProps::action),
        .entities = entities,
        .flags = convertFromOhosFlags(flagsInt),
        .parameters = parametersObject,
        .fds = fds,
    };

    return want;
}

QSharedPointer<WantInfo> convertToOhosAppKitWantInfo(
    QSharedPointer<detail::WantInfoPriv> wantInfo)
{
    return QSharedPointer<QOhosWantInfoImpl>::create(wantInfo);
}

QSharedPointer<detail::WantInfoPriv> convertToQpaWantInfo(
    QSharedPointer<WantInfo> wantInfo)
{
    return qSharedPointerCast<QOhosWantInfoImpl>(wantInfo)->qpaWantInfo();
}

}

QT_END_NAMESPACE
