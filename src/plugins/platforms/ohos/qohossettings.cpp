// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohossettings.h"
#include <QtCore/private/qohoscommon_p.h>
#include <cmath>
#include <cstring>
#include <functional>
#include <qohosdeviceinfo_p.h>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <stdexcept>
#include <string>

QT_BEGIN_NAMESPACE

namespace {

constexpr const char *fontSizeScalePropertyName = "font_scale";
constexpr const char *windowPcModeSwitchStatusPropertyName = "window_pcmode_switch_status";

QOhosOptional<std::string> tryGetDataItemValue(const std::string &name, const std::string &domainName)
{
    return QtOhos::evalInJsThreadWithPromise<QOhosOptional<std::string>>(
        [&](QtOhos::JsState &jsState, auto evalPromise) {
        auto defaultQAbility = jsState.defaultQAbilityPeer()->qAbility();
        if (defaultQAbility.IsEmpty()) {
            evalPromise(makeEmptyQOhosOptional());
            return;
        }

        auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
        jsState.evalToPromiseOrRejectOnThrow(
            "@ohos.settings.getValue(*)",
            {defaultQAbility.get("context"), name, domainName})
        .onThen([thenPromise = std::move(thenCatchPromises.first)](const QtOhos::CallbackInfo &cbInfo) {
            std::string result = cbInfo.getFirstArg<QNapi::String>(Q_FUNC_INFO);
            thenPromise(makeQOhosOptional(result));
        })
        .onCatch([catchPromise = std::move(thenCatchPromises.second), name, domainName](const QtOhos::CallbackInfo &) {
            qOhosPrintfError(
                "Got error from @ohos.settings.getValue(..., '%s', '%s').",
                name.c_str(), domainName.c_str());
            catchPromise(makeEmptyQOhosOptional());
        });
    },
    Q_FUNC_INFO);
}

template<typename T>
QOhosOptional<T> tryGetDataItemTypedValue(const std::string &name, const std::string &domainName);

template<>
QOhosOptional<double> tryGetDataItemTypedValue(const std::string &name, const std::string &domainName)
{
    auto optStringValue = tryGetDataItemValue(name, domainName);
    auto optDoubleValue = optStringValue.hasValue()
        ? QtOhos::tryParseStringAsFiniteDouble(optStringValue.value())
        : makeEmptyQOhosOptional();

    if (optStringValue.hasValue() && !optDoubleValue.hasValue()) {
        qOhosPrintfError(
            "OHOS settings value %s/%s ('%s') is not correct double value, assuming empty setting",
            name.c_str(), domainName.c_str(), optStringValue.value().c_str());
    }

    return optDoubleValue;
}

std::string getOhosSettingsUserPropertyDomainName()
{
    return QtOhos::evalInJsThread([](QtOhos::JsState &jsState) {
        return jsState.eval<QNapi::String>("@ohos.settings.domainName.USER_PROPERTY").Utf8Value();
    },
    Q_FUNC_INFO);
}

}

namespace QOhosSettings {

double fontSizeScale()
{
    constexpr double defaultFontSizeScale = 1.0;
    const auto maybeFontSizeScaleSetting = tryGetDataItemTypedValue<double>(
        fontSizeScalePropertyName, getOhosSettingsUserPropertyDomainName());

    if (!maybeFontSizeScaleSetting.hasValue()) {
        qOhosPrintfWarning(
            "Cannot obtain '%s' property. Assuming its default fallback mode value %f",
            fontSizeScalePropertyName, defaultFontSizeScale);
        return defaultFontSizeScale;
    }

    return maybeFontSizeScaleSetting.value();
}

bool isWindowPcModeEnabled()
{
    if (QOhosDeviceInfo::is2in1())
        return true;

    constexpr auto statusTrueStr = "true";
    constexpr auto statusFalseStr = "false";

    auto maybeWindowPcModeSwitchStatus = tryGetDataItemValue(
        windowPcModeSwitchStatusPropertyName, getOhosSettingsUserPropertyDomainName());

    if (!maybeWindowPcModeSwitchStatus.hasValue()) {
        qOhosPrintfWarning(
            "Cannot obtain '%s' property. Assuming it is NOT enabled.",
            windowPcModeSwitchStatusPropertyName);
        return false;
    }

    auto windowPcModeSwitchStatus = maybeWindowPcModeSwitchStatus.value();
    if (windowPcModeSwitchStatus != statusTrueStr && windowPcModeSwitchStatus != statusFalseStr) {
        qOhosPrintfError(
            "Unexpected value of '%s': %s (expected: '%s' or '%s').",
            windowPcModeSwitchStatusPropertyName, windowPcModeSwitchStatus.c_str(),
            statusTrueStr, statusFalseStr);
    }

    return windowPcModeSwitchStatus == statusTrueStr;
}

}

QT_END_NAMESPACE
