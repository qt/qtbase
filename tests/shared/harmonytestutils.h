// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QT_TESTS_SHARED_HARMONY_TEST_UTILS_H
#define QT_TESTS_SHARED_HARMONY_TEST_UTILS_H

#include <QtTest/qtest.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qharmonyappjscontext.h>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>

#include <mutex>
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

inline QHarmonyJsObjectRef fetchDefaultAbility()
{
    return QNativeInterface::QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return QHarmonyJsObjectRef::fromValue(jsContext, QHarmonyAppJsContext::fromJsContext(jsContext).defaultAbility());
        });
}

}

#endif // QT_TESTS_SHARED_HARMONY_TEST_UTILS_H
