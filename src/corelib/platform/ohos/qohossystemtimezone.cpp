// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohossystemtimezone_p.h"

#include <BasicServicesKit/time_service.h>
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/private/qohosmemoizingjsthreadfetcher_p.h>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

namespace {

std::optional<QByteArray> readOhosSystemTimeZoneId(QOhosJsState &)
{
    const auto getTimeZoneFunc = Q_OHOS_NAMED_FUNC(::OH_TimeService_GetTimeZone);

    // No documented maximum id length, this cap only stops a call that keeps failing
    constexpr auto maxTzBufferSize = 1024 * 1024;

    QByteArray tzBuffer(32, '\0');
    while (tzBuffer.size() <= maxTzBufferSize) {
        const auto timeZoneReadResult =
            getTimeZoneFunc(tzBuffer.data(), static_cast<uint32_t>(tzBuffer.size()));
        if (timeZoneReadResult == ::TIMESERVICE_ERR_OK) {
            tzBuffer.truncate(qstrlen(tzBuffer.constData()));
            return tzBuffer;
        }

        if (timeZoneReadResult != ::TIMESERVICE_ERR_INVALID_PARAMETER) {
            qOhosPrintfError(
                "'%s' failed, retval: %d",
                getTimeZoneFunc.name(), static_cast<int>(timeZoneReadResult));
            return std::nullopt;
        }

        tzBuffer.resize(tzBuffer.size() * 2);
    }

    qOhosPrintfError(
        "'%s' failed, could not read time zone id into a buffer of up to %d bytes",
        getTimeZoneFunc.name(), maxTzBufferSize);
    return std::nullopt;
}

void setSystemTimeZoneId(QOhosMutexProtectedValue<QByteArray> &systemId, const QByteArray &id)
{
    systemId.processValue([&](auto &value) {
        value = id;
    });
}

struct TimeZoneChangedSubscriber
{
    QNapi::Reference<QNapi::Object> subscriber;
};

std::optional<std::shared_ptr<QOhosMutexProtectedValue<QByteArray>>> fetchOhosSystemTimeZoneId(
    QOhosJsState &jsState)
{
    auto systemId = std::make_shared<QOhosMutexProtectedValue<QByteArray>>();
    if (const auto optInitialSystemTimeZoneId = readOhosSystemTimeZoneId(jsState))
        setSystemTimeZoneId(*systemId, *optInitialSystemTimeZoneId);

    auto subscriber = jsState.eval<QNapi::Object>(
        "@ohos.commonEventManager.createSubscriberSync(*)",
        {
            QNapi::makeObject(
                jsState.env(),
                {
                    {
                        "events",
                        QNapi::makeArray(
                            jsState.env(),
                            {jsState.eval<QNapi::String>(
                                "@ohos.commonEventManager.Support.COMMON_EVENT_TIMEZONE_CHANGED")})
                    },
                })
        });

    try {
        jsState.eval(
            "@ohos.commonEventManager.subscribe(*)",
            {
                subscriber,
                [systemId](const QOhosCallbackInfo &cbInfo) {
                    if (const auto optSystemTimeZoneId = readOhosSystemTimeZoneId(cbInfo.jsState()))
                        setSystemTimeZoneId(*systemId, *optSystemTimeZoneId);
                }
            });
    } catch (const Napi::Error &error) {
        qOhosPrintfError("%s: subscribe() failed: %s", Q_FUNC_INFO, error.what());
        return systemId;
    }

    jsState.getAttachedObjectWithLazyCreate<TimeZoneChangedSubscriber>().subscriber =
        QNapi::Reference<>::makePersistentFrom(subscriber);

    return systemId;
}

QOhosMemoizingJsThreadFetcher<std::shared_ptr<QOhosMutexProtectedValue<QByteArray>>>
    systemTimeZoneIdFetcher(fetchOhosSystemTimeZoneId, "system time zone id");

}

QByteArray qOhosSystemTimeZoneId()
{
    const auto optSystemId = systemTimeZoneIdFetcher.optValue();
    if (!optSystemId)
        return QByteArray();

    return optSystemId.value()->evalWithValue([](const auto &id) {
        return id;
    });
}

QT_END_NAMESPACE
