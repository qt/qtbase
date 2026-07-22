// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohossettings.h"
#include <QtCore/private/qohoscommon_p.h>
#include <cmath>
#include <cstring>
#include <functional>
#include <optional>
#include <qohosdeviceinfo_p.h>
#include <qohosjsutils.h>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <stdexcept>
#include <string>
#include <utility>

QT_BEGIN_NAMESPACE

namespace {

constexpr const char *fontSizeScalePropertyName = "font_scale";
constexpr const char *windowPcModeSwitchStatusPropertyName = "window_pcmode_switch_status";

std::optional<std::string> tryGetDataItemValue(const std::string &name, const std::string &domainName)
{
    return QtOhos::evalInJsThreadWithPromise<std::optional<std::string>>(
        [&](QtOhos::JsState &jsState, auto evalPromise) {
        auto defaultQAbility = jsState.defaultQAbilityPeer()->qAbility();
        if (defaultQAbility.IsEmpty()) {
            evalPromise({});
            return;
        }

        auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
        jsState.evalToPromiseOrRejectOnThrow(
            "@ohos.settings.getValue(*)",
            {defaultQAbility.get("context"), name, domainName})
        .onThen([thenPromise = std::move(thenCatchPromises.first)](const QtOhos::CallbackInfo &cbInfo) {
            std::string result = cbInfo.getFirstArg<QNapi::String>(Q_FUNC_INFO);
            thenPromise(result);
        })
        .onCatch([catchPromise = std::move(thenCatchPromises.second), name, domainName](const QtOhos::CallbackInfo &) {
            qOhosPrintfError(
                "Got error from @ohos.settings.getValue(..., '%s', '%s').",
                name.c_str(), domainName.c_str());
            catchPromise({});
        });
    },
    Q_FUNC_INFO);
}

template<typename T>
std::optional<T> tryGetDataItemTypedValue(const std::string &name, const std::string &domainName);

template<>
std::optional<double> tryGetDataItemTypedValue(const std::string &name, const std::string &domainName)
{
    auto optStringValue = tryGetDataItemValue(name, domainName);
    auto optDoubleValue = optStringValue.has_value()
        ? QtOhos::tryParseStringAsFiniteDouble(optStringValue.value())
        : std::nullopt;

    if (optStringValue.has_value() && !optDoubleValue.has_value()) {
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

std::string settingsUserPropertyDomainName(QtOhos::JsState &jsState)
{
    return jsState.eval<QNapi::String>("@ohos.settings.domainName.USER_PROPERTY");
}

QNapi::Object settingsContext(QtOhos::JsState &jsState)
{
    return jsState.defaultQAbilityPeer()->qAbility().eval<QNapi::Object>("context");
}

std::string readSettingValue(
    QtOhos::JsState &jsState, const QNapi::Object &context, const std::string &name)
{
    return jsState.eval<QNapi::String>(
        "@ohos.settings.getValueSync(*)",
        {context, name, "", settingsUserPropertyDomainName(jsState)});
}

std::shared_ptr<void> registerSettingsKeyObserver(
    QtOhos::JsState &jsState, QNapi::Object context, const std::string &name,
    std::function<void(QtOhos::JsState &)> onChanged)
{
    const std::string domainName = settingsUserPropertyDomainName(jsState);

    const bool registered = jsState.eval<QNapi::Boolean>(
        "@ohos.settings.registerKeyObserver(*)",
        {
            context, name, domainName,
            [onChanged = std::move(onChanged)](const QtOhos::CallbackInfo &cbInfo) {
                onChanged(cbInfo.jsState());
            }
        });
    if (!registered) {
        qOhosPrintfWarning(
            "Failed to register observer for settings key '%s' in domain '%s'.",
            name.c_str(), domainName.c_str());
        return nullptr;
    }

    auto contextRefPtr = QtOhos::moveToSharedPtr(QNapi::Reference<>::makePersistentFrom(context));
    return std::shared_ptr<void>(
        nullptr,
        [contextRefPtr, name, domainName](auto) {
            QtOhos::runInJsThreadAndWait(
                [&](QtOhos::JsState &jsState) {
                    auto contextRef = std::move(*contextRefPtr);
                    jsState.eval<QNapi::Value>(
                        "@ohos.settings.unregisterKeyObserver(*)",
                        {contextRef.Value(), name, domainName});
                },
                Q_FUNC_INFO);
        });
}

bool readWindowPcModeEnabled(QtOhos::JsState &jsState)
{
    if (jsState.defaultQAbilityPeer()->qAbility().IsEmpty())
        return false;

    return readSettingValue(jsState, settingsContext(jsState), windowPcModeSwitchStatusPropertyName) == "true";
}

QOhosSupplier<bool> makeWindowPcModeEnabledSupplier()
{
    if (QOhosDeviceInfo::is2in1())
        return [] { return true; };

    return QtOhos::makeDataSource<bool>(
        readWindowPcModeEnabled,
        [](QtOhos::JsState &jsState, QOhosConsumer<bool> valueChangedConsumer) -> std::shared_ptr<void> {
            // FIXME: the observer must use the launching UIAbility's context. By
            // API definition @ohos.settings does not accept the application
            // context, and free-window state is not part of the app Configuration.
            // That context can be destroyed while the process lives, orphaning the
            // observer. The orphaned observer keeps firing (verified on device), so
            // the cached value stays correct. Revisit when the platform exposes
            // settings observation on the application context.
            if (jsState.defaultQAbilityPeer()->qAbility().IsEmpty())
                return nullptr;

            return registerSettingsKeyObserver(
                jsState, settingsContext(jsState), windowPcModeSwitchStatusPropertyName,
                [valueChangedConsumer = std::move(valueChangedConsumer)](QtOhos::JsState &jsState) {
                    valueChangedConsumer(readWindowPcModeEnabled(jsState));
                });
        },
        makeQOhosNoOpConsumer(),
        QtOhos::invokeInQtThread,
        Q_FUNC_INFO);
}

QOhosSupplier<bool> makeUncachedWindowPcModeEnabledSupplier()
{
    return [] {
        return QOhosDeviceInfo::is2in1()
            || QtOhos::evalInJsThread(readWindowPcModeEnabled, Q_FUNC_INFO);
    };
}

}

QOhosSettings &QOhosSettings::instance()
{
    static QOhosSettings instance;
    return instance;
}

std::shared_ptr<void> QOhosSettings::installSettingsCache()
{
    return QtOhos::makeDestroyNotifier(
        [this, previousSupplier = std::exchange(m_windowPcModeEnabled, makeWindowPcModeEnabledSupplier())]() mutable {
            m_windowPcModeEnabled = std::move(previousSupplier);
        });
}

QOhosSettings::QOhosSettings()
    : m_windowPcModeEnabled(makeUncachedWindowPcModeEnabledSupplier())
{
}

double QOhosSettings::fontSizeScale() const
{
    constexpr double defaultFontSizeScale = 1.0;
    const auto maybeFontSizeScaleSetting = tryGetDataItemTypedValue<double>(
        fontSizeScalePropertyName, getOhosSettingsUserPropertyDomainName());

    if (!maybeFontSizeScaleSetting.has_value()) {
        qOhosPrintfWarning(
            "Cannot obtain '%s' property. Assuming its default fallback mode value %f",
            fontSizeScalePropertyName, defaultFontSizeScale);
        return defaultFontSizeScale;
    }

    return maybeFontSizeScaleSetting.value();
}

bool QOhosSettings::isWindowPcModeEnabled() const
{
    return m_windowPcModeEnabled();
}

QT_END_NAMESPACE
