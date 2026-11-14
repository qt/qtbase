// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QT_TESTS_SHARED_HARMONY_TEST_UTILS_H
#define QT_TESTS_SHARED_HARMONY_TEST_UTILS_H

#include <QtTest/qtest.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qharmonyappjscontext.h>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <napi/native_api.h>

namespace QTestHarmony {

inline std::mutex napiCallFailuresMutex;
inline std::vector<std::string> napiCallFailures;

inline void checkNapiStatus(napi_status status, const char *callName)
{
    if (status != napi_ok) {
        std::lock_guard<std::mutex> napiCallFailuresLock(napiCallFailuresMutex);
        napiCallFailures.push_back(std::string(callName) + ": " + std::to_string(static_cast<int>(status)));
    }
}

inline std::vector<std::string> takeNapiCallFailures()
{
    std::lock_guard<std::mutex> napiCallFailuresLock(napiCallFailuresMutex);
    return std::exchange(napiCallFailures, {});
}

inline void verifyNoNapiCallFailures()
{
    const auto failures = takeNapiCallFailures();
    std::string failuresText;
    for (const auto &failure : failures) {
        if (!failuresText.empty())
            failuresText += ", ";
        failuresText += failure;
    }
    const auto message = "Node-API calls failed: " + failuresText;
    QVERIFY2(failures.empty(), message.c_str());
}

inline napi_valuetype jsTypeOf(napi_env env, napi_value value)
{
    if (!value)
        return napi_undefined;

    napi_valuetype type = napi_undefined;
    checkNapiStatus(napi_typeof(env, value, &type), "napi_typeof");
    return type;
}

inline napi_value getJsProperty(napi_env env, napi_value objectValue, const char *name)
{
    if (!objectValue)
        return nullptr;

    napi_value propertyValue = nullptr;
    checkNapiStatus(napi_get_named_property(env, objectValue, name, &propertyValue), "napi_get_named_property");
    return propertyValue;
}

inline napi_value callJsMethod(napi_env env, napi_value objectValue, const char *name)
{
    const auto functionValue = getJsProperty(env, objectValue, name);
    if (jsTypeOf(env, functionValue) != napi_function)
        return nullptr;

    napi_value resultValue = nullptr;
    checkNapiStatus(napi_call_function(env, objectValue, functionValue, 0, nullptr, &resultValue), "napi_call_function");
    return resultValue;
}

inline std::optional<int> tryMapJsNumberToInt(napi_env env, napi_value value)
{
    if (jsTypeOf(env, value) != napi_number)
        return std::nullopt;

    std::int32_t number = 0;
    checkNapiStatus(napi_get_value_int32(env, value, &number), "napi_get_value_int32");
    return number;
}

inline std::optional<bool> tryMapJsBooleanToBool(napi_env env, napi_value value)
{
    if (jsTypeOf(env, value) != napi_boolean)
        return std::nullopt;

    bool boolean = false;
    checkNapiStatus(napi_get_value_bool(env, value, &boolean), "napi_get_value_bool");
    return boolean;
}

inline std::optional<int> tryGetJsEnumerator(
    QHarmonyJsContext &jsContext, const char *moduleName, const char *enumName, const char *enumeratorName)
{
    const auto env = jsContext.env();
    const auto moduleValue = jsContext.tryImportModule(moduleName);
    const auto enumValue = getJsProperty(env, moduleValue, enumName);
    return tryMapJsNumberToInt(env, getJsProperty(env, enumValue, enumeratorName));
}

inline QHarmonyJsObjectRef fetchDefaultAbility()
{
    return QNativeInterface::QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return QHarmonyJsObjectRef::fromValue(jsContext, QHarmonyAppJsContext::fromJsContext(jsContext).defaultAbility());
        });
}

}

#endif // QT_TESTS_SHARED_HARMONY_TEST_UTILS_H
