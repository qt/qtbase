// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qharmonyappjscontext.h>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>
#include <QtCore/qharmonyjsonconversions.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qset.h>
#include <QtCore/qthread.h>
#include <QtTest/qtest.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <napi/native_api.h>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

using QHarmonyApplication = QNativeInterface::QHarmonyApplication;

QT_BEGIN_NAMESPACE

namespace QTest {

template<> char *toString(const std::thread::id &threadId)
{
    std::ostringstream stream;
    stream << threadId;
    return qstrdup(stream.str().c_str());
}

}

QT_END_NAMESPACE

namespace {

enum class JsValueKind {
    NullPointer,
    Null,
    Undefined,
    Number,
    String,
    Boolean,
    Symbol,
    BigInt,
    PlainObject,
    Array,
    Function,
    FrozenObject,
};

constexpr char abilityConstantModuleName[] = "@ohos.app.ability.AbilityConstant";
constexpr char systemShareModuleName[] = "@kit.ShareKit.systemShare";

constexpr char keptJsObjectPropName[] = "qtTestKeptObject";

// Stays below the HarmonyOS app-freeze limit for a blocked JS thread.
constexpr auto jsThreadBlockTimeout = 3s;

QString nonAsciiText()
{
    return QStringLiteral(u"za\u017c\u00f3\u0142\u0107 \U0001F600");
}

QJsonObject scalarsJsonObject()
{
    return QJsonObject{
        {"true", true},
        {"false", false},
        {"integer", 42},
        {"negativeInteger", -7},
        {"double", 0.25},
        {"asciiString", "text"},
        {"nonAsciiString", nonAsciiText()},
        {"emptyString", ""},
        {"null", QJsonValue::Null},
    };
}

QJsonObject nestedJsonObject()
{
    return QJsonObject{
        {"object", QJsonObject{{"number", 1}, {"string", "text"}}},
        {"array", QJsonArray{1, "text", false, QJsonValue::Null, QJsonArray{2.5}, QJsonObject{{"number", 3}}}},
        {"emptyObject", QJsonObject()},
        {"emptyArray", QJsonArray()},
    };
}

std::mutex napiCallFailuresMutex;
std::vector<std::string> napiCallFailures;

void checkNapiStatus(napi_status status, const char *callName)
{
    if (status != napi_ok) {
        std::lock_guard<std::mutex> napiCallFailuresLock(napiCallFailuresMutex);
        napiCallFailures.push_back(std::string(callName) + ": " + std::to_string(static_cast<int>(status)));
    }
}

std::vector<std::string> takeNapiCallFailures()
{
    std::lock_guard<std::mutex> napiCallFailuresLock(napiCallFailuresMutex);
    return std::exchange(napiCallFailures, {});
}

napi_value makeJsUndefined(napi_env env)
{
    napi_value undefinedValue = nullptr;
    checkNapiStatus(napi_get_undefined(env, &undefinedValue), "napi_get_undefined");
    return undefinedValue;
}

napi_value makeJsNull(napi_env env)
{
    napi_value nullValue = nullptr;
    checkNapiStatus(napi_get_null(env, &nullValue), "napi_get_null");
    return nullValue;
}

napi_value makeJsBoolean(napi_env env, bool value)
{
    napi_value booleanValue = nullptr;
    checkNapiStatus(napi_get_boolean(env, value, &booleanValue), "napi_get_boolean");
    return booleanValue;
}

napi_value makeJsNumber(napi_env env, double value)
{
    napi_value numberValue = nullptr;
    checkNapiStatus(napi_create_double(env, value, &numberValue), "napi_create_double");
    return numberValue;
}

napi_value makeJsString(napi_env env, const std::string &value)
{
    napi_value stringValue = nullptr;
    checkNapiStatus(napi_create_string_utf8(env, value.c_str(), value.size(), &stringValue), "napi_create_string_utf8");
    return stringValue;
}

napi_value makeJsSymbol(napi_env env)
{
    napi_value symbolValue = nullptr;
    checkNapiStatus(napi_create_symbol(env, nullptr, &symbolValue), "napi_create_symbol");
    return symbolValue;
}

napi_value makeJsBigInt(napi_env env, std::int64_t value)
{
    napi_value bigIntValue = nullptr;
    checkNapiStatus(napi_create_bigint_int64(env, value, &bigIntValue), "napi_create_bigint_int64");
    return bigIntValue;
}

napi_value makeJsObject(napi_env env)
{
    napi_value objectValue = nullptr;
    checkNapiStatus(napi_create_object(env, &objectValue), "napi_create_object");
    return objectValue;
}

napi_value makeJsArray(napi_env env, const std::vector<napi_value> &elementValues)
{
    napi_value arrayValue = nullptr;
    checkNapiStatus(napi_create_array_with_length(env, elementValues.size(), &arrayValue), "napi_create_array_with_length");
    for (std::size_t i = 0; i < elementValues.size(); ++i)
        checkNapiStatus(napi_set_element(env, arrayValue, static_cast<std::uint32_t>(i), elementValues[i]), "napi_set_element");
    return arrayValue;
}

napi_value makeJsFunction(napi_env env, napi_callback callback, void *data = nullptr)
{
    napi_value functionValue = nullptr;
    checkNapiStatus(napi_create_function(env, "qtTestFunction", NAPI_AUTO_LENGTH, callback, data, &functionValue), "napi_create_function");
    return functionValue;
}

napi_value getJsGlobal(napi_env env)
{
    napi_value globalValue = nullptr;
    checkNapiStatus(napi_get_global(env, &globalValue), "napi_get_global");
    return globalValue;
}

napi_value getJsProperty(napi_env env, napi_value objectValue, const char *name)
{
    if (!objectValue)
        return nullptr;

    napi_value propertyValue = nullptr;
    checkNapiStatus(napi_get_named_property(env, objectValue, name, &propertyValue), "napi_get_named_property");
    return propertyValue;
}

napi_value makeJsObjectWithPrototype(napi_env env, napi_value prototypeValue)
{
    const auto objectClassValue = getJsProperty(env, getJsGlobal(env), "Object");
    napi_value objectValue = nullptr;
    checkNapiStatus(
        napi_call_function(
            env, objectClassValue, getJsProperty(env, objectClassValue, "create"), 1, &prototypeValue, &objectValue),
        "napi_call_function");
    return objectValue;
}

void setJsProperty(napi_env env, napi_value objectValue, const char *name, napi_value propertyValue)
{
    checkNapiStatus(napi_set_named_property(env, objectValue, name, propertyValue), "napi_set_named_property");
}

void deleteJsProperty(napi_env env, napi_value objectValue, const char *name)
{
    bool deleted = false;
    checkNapiStatus(napi_delete_property(env, objectValue, makeJsString(env, name), &deleted), "napi_delete_property");
}

void defineNonEnumerableJsProperty(napi_env env, napi_value objectValue, const char *name, napi_value propertyValue)
{
    napi_property_descriptor descriptor = {};
    descriptor.utf8name = name;
    descriptor.value = propertyValue;
    descriptor.attributes = napi_default;
    checkNapiStatus(napi_define_properties(env, objectValue, 1, &descriptor), "napi_define_properties");
}

void defineEnumerableJsGetter(napi_env env, napi_value objectValue, const char *name, napi_callback getter)
{
    napi_property_descriptor descriptor = {};
    descriptor.utf8name = name;
    descriptor.getter = getter;
    descriptor.attributes = napi_enumerable;
    checkNapiStatus(napi_define_properties(env, objectValue, 1, &descriptor), "napi_define_properties");
}

napi_value withJsNumberProperty(napi_env env, napi_value objectValue)
{
    setJsProperty(env, objectValue, "number", makeJsNumber(env, 1));
    return objectValue;
}

napi_value freezeJsObject(napi_env env, napi_value objectValue)
{
    checkNapiStatus(napi_object_freeze(env, objectValue), "napi_object_freeze");
    return objectValue;
}

void callJsFunction(napi_env env, napi_value functionValue)
{
    napi_value resultValue = nullptr;
    checkNapiStatus(napi_call_function(env, makeJsUndefined(env), functionValue, 0, nullptr, &resultValue), "napi_call_function");
}

napi_valuetype jsTypeOf(napi_env env, napi_value value)
{
    if (!value)
        return napi_undefined;

    napi_valuetype type = napi_undefined;
    checkNapiStatus(napi_typeof(env, value, &type), "napi_typeof");
    return type;
}

bool isJsArray(napi_env env, napi_value value)
{
    if (!value)
        return false;

    bool isArray = false;
    checkNapiStatus(napi_is_array(env, value, &isArray), "napi_is_array");
    return isArray;
}

bool jsStrictEquals(napi_env env, napi_value lhs, napi_value rhs)
{
    if (!lhs || !rhs)
        return false;

    bool equal = false;
    checkNapiStatus(napi_strict_equals(env, lhs, rhs, &equal), "napi_strict_equals");
    return equal;
}

bool isJsExceptionPending(napi_env env)
{
    bool exceptionPending = false;
    checkNapiStatus(napi_is_exception_pending(env, &exceptionPending), "napi_is_exception_pending");
    return exceptionPending;
}

std::uint32_t getJsArrayLength(napi_env env, napi_value arrayValue)
{
    if (!arrayValue)
        return 0;

    std::uint32_t length = 0;
    checkNapiStatus(napi_get_array_length(env, arrayValue, &length), "napi_get_array_length");
    return length;
}

napi_value getJsArrayElement(napi_env env, napi_value arrayValue, std::uint32_t index)
{
    if (!arrayValue)
        return nullptr;

    napi_value elementValue = nullptr;
    checkNapiStatus(napi_get_element(env, arrayValue, index, &elementValue), "napi_get_element");
    return elementValue;
}

bool isJsPlainObject(napi_env env, napi_value value)
{
    return jsTypeOf(env, value) == napi_object && !isJsArray(env, value);
}

std::uint32_t getJsPropertyCount(napi_env env, napi_value objectValue)
{
    if (!objectValue)
        return 0;

    napi_value propertyNamesValue = nullptr;
    checkNapiStatus(napi_get_property_names(env, objectValue, &propertyNamesValue), "napi_get_property_names");
    return getJsArrayLength(env, propertyNamesValue);
}

bool jsPropertyStrictEquals(napi_env env, napi_value objectValue, const char *name, napi_value expectedValue)
{
    return jsStrictEquals(env, getJsProperty(env, objectValue, name), expectedValue);
}

void keepJsObjectInGlobal(napi_env env, const char *propName, napi_value objectValue)
{
    setJsProperty(env, getJsGlobal(env), propName, objectValue);
}

napi_value takeJsObjectFromGlobal(napi_env env, const char *propName)
{
    const auto globalValue = getJsGlobal(env);
    const auto objectValue = getJsProperty(env, globalValue, propName);
    deleteJsProperty(env, globalValue, propName);
    return objectValue;
}

napi_value emptyJsCallback(napi_env, napi_callback_info)
{
    return nullptr;
}

napi_value failingJsGetterCallback(napi_env env, napi_callback_info)
{
    checkNapiStatus(napi_throw_error(env, nullptr, "getter failed"), "napi_throw_error");
    return nullptr;
}

napi_value jsContextRecordingCallback(napi_env env, napi_callback_info callbackInfo)
{
    void *data = nullptr;
    checkNapiStatus(napi_get_cb_info(env, callbackInfo, nullptr, nullptr, nullptr, &data), "napi_get_cb_info");
    if (data)
        *static_cast<QHarmonyJsContext **>(data) = &QHarmonyJsContext::fromEnv(env);
    return nullptr;
}

napi_value appJsContextRecordingCallback(napi_env env, napi_callback_info callbackInfo)
{
    void *data = nullptr;
    checkNapiStatus(napi_get_cb_info(env, callbackInfo, nullptr, nullptr, nullptr, &data), "napi_get_cb_info");
    if (data)
        *static_cast<QHarmonyAppJsContext **>(data) = &QHarmonyAppJsContext::fromEnv(env);
    return nullptr;
}

napi_value makeJsValueOfKind(napi_env env, JsValueKind kind)
{
    switch (kind) {
    case JsValueKind::NullPointer:
        return nullptr;
    case JsValueKind::Null:
        return makeJsNull(env);
    case JsValueKind::Undefined:
        return makeJsUndefined(env);
    case JsValueKind::Number:
        return makeJsNumber(env, 1);
    case JsValueKind::String:
        return makeJsString(env, "text");
    case JsValueKind::Boolean:
        return makeJsBoolean(env, true);
    case JsValueKind::Symbol:
        return makeJsSymbol(env);
    case JsValueKind::BigInt:
        return makeJsBigInt(env, 1);
    case JsValueKind::PlainObject:
        return withJsNumberProperty(env, makeJsObject(env));
    case JsValueKind::Array:
        return makeJsArray(env, {makeJsNumber(env, 1), makeJsNumber(env, 2)});
    case JsValueKind::Function:
        return withJsNumberProperty(env, makeJsFunction(env, emptyJsCallback));
    case JsValueKind::FrozenObject:
        return freezeJsObject(env, withJsNumberProperty(env, makeJsObject(env)));
    }
    Q_UNREACHABLE_RETURN(nullptr);
}

QHarmonyJsObjectRef makeRefToNewJsObject()
{
    return QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return QHarmonyJsObjectRef::fromValue(jsContext, makeJsObject(jsContext.env()));
        });
}

struct JsonMappingResult
{
    QJsonObject jsonObject;
    bool exceptionPending = true;
};

struct JsObjectRefPair
{
    QHarmonyJsObjectRef first;
    QHarmonyJsObjectRef second;
};

JsObjectRefPair makeTwoRefsToNewJsObject()
{
    return QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto objectValue = makeJsObject(jsContext.env());
            return JsObjectRefPair{
                .first = QHarmonyJsObjectRef::fromValue(jsContext, objectValue),
                .second = QHarmonyJsObjectRef::fromValue(jsContext, objectValue),
            };
        });
}

void queueJsTaskRecordingThreadId(std::shared_ptr<std::optional<std::thread::id>> optTaskThreadId)
{
    QHarmonyApplication::runOnJsThread(
        [optTaskThreadId](QHarmonyJsContext &) {
            QHarmonyApplication::runOnQtThread(
                [optTaskThreadId, taskThreadId = std::this_thread::get_id()]() {
                    *optTaskThreadId = taskThreadId;
                });
        });
}

void queueQtTaskRecordingThreadId(std::shared_ptr<std::optional<std::thread::id>> optTaskThreadId)
{
    QHarmonyApplication::runOnQtThread(
        [optTaskThreadId]() {
            *optTaskThreadId = std::this_thread::get_id();
        });
}

std::shared_ptr<void> makeDestroyThreadTracker(std::shared_ptr<std::promise<std::thread::id>> destroyThreadPromise)
{
    return std::shared_ptr<void>(
        nullptr,
        [destroyThreadPromise = std::move(destroyThreadPromise)](void *) {
            destroyThreadPromise->set_value(std::this_thread::get_id());
        });
}

// Blocks the JS thread until the returned handle is released, so runOnJsThreadAndWait() must not be called before that.
// Sets *timedOut on the JS thread if the block ends by the timeout instead.
std::shared_ptr<void> blockJsThread(std::shared_ptr<bool> timedOut)
{
    auto releasePromise = std::make_shared<std::promise<void>>();
    QHarmonyApplication::runOnJsThread(
        [releaseFuture = releasePromise->get_future().share(), timedOut](QHarmonyJsContext &) {
            *timedOut = releaseFuture.wait_for(jsThreadBlockTimeout) == std::future_status::timeout;
        });
    return std::shared_ptr<void>(
        nullptr,
        [releasePromise = std::move(releasePromise)](void *) {
            releasePromise->set_value();
        });
}

template <typename Func>
void runInWorkerThread(Func &&func)
{
    std::unique_ptr<QThread> workerThread(QThread::create(std::forward<Func>(func)));
    workerThread->start();
    workerThread->wait();
}

}

class tst_Harmony : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void application_nativeInterface_available();
    void application_runOnJsThreadAndWait_runsOnJsThread();
    void application_runOnJsThreadAndWait_blocks();
    void application_runOnJsThreadAndWait_returnsResult();
    void application_runOnJsThreadAndWait_cppException();
    void application_runOnJsThreadAndWait_jsException();
    void application_runOnJsThreadAndWait_nested();
    void application_runOnJsThreadAndWait_nestedException();
    void application_runOnJsThreadAndWait_fromWorkerThread();
    void application_runOnJsThread_runsOnJsThread();
    void application_runOnJsThread_fromWorkerThread();
    void application_runOnJsThread_ordered();
    void application_runOnJsThread_queuedFromJsThread();
    void application_runOnJsThread_queuedWhileJsThreadBlocked();
    void application_runOnJsThread_destroysTaskOnJsThread();
    void application_runOnJsThread_destroysMovedTaskOnJsThread();
    void application_runOnJsThread_cppException();
    void application_runOnJsThread_jsException();
    void application_runOnQtThread_fromJsThread();
    void application_runOnQtThread_fromWorkerThread();
    void application_runOnQtThread_queuedFromQtThread();
    void application_runOnQtThread_orderedFromJsThread();
    void application_runOnQtThread_orderedFromQtThread();
    void application_runOnQtThread_destroysTaskOnQtThread();

    void jsContext_fromEnv_returnsTaskContext();
    void jsContext_fromEnv_inJsCallback();
    void jsContext_tryImportModule_platformModule();
    void jsContext_tryImportModule_kitModule();
    void jsContext_tryImportModule_sameObject();
    void jsContext_tryImportModule_failure_data();
    void jsContext_tryImportModule_failure();

    void appJsContext_fromJsContext_sameObject();
    void appJsContext_fromEnv_inJsCallback();
    void appJsContext_defaultAbility_isUiAbility();
    void appJsContext_defaultAbility_stable();

    void jsObjectRef_defaultConstructor_isNull();
    void jsObjectRef_fromValue_nonObject_data();
    void jsObjectRef_fromValue_nonObject();
    void jsObjectRef_fromValue_objectKinds_data();
    void jsObjectRef_fromValue_objectKinds();
    void jsObjectRef_value_acrossTasks();
    void jsObjectRef_equality_sameObject();
    void jsObjectRef_equality_differentObjects();
    void jsObjectRef_copyAndMove_keepsObject();
    void jsObjectRef_copyAndCompare_offJsThread();
    void jsObjectRef_copyAndDestroy_concurrentThreads();
    void jsObjectRef_qHash_containers();
    void jsObjectRef_destructor_offJsThread();

    void jsonConversions_mapQJsonObjectToJsObject_empty();
    void jsonConversions_mapQJsonObjectToJsObject_scalars();
    void jsonConversions_mapQJsonObjectToJsObject_nested();
    void jsonConversions_mapQJsonObjectToJsObject_newObject();
    void jsonConversions_mapJsObjectToQJsonObject_notPlainObject_data();
    void jsonConversions_mapJsObjectToQJsonObject_notPlainObject();
    void jsonConversions_mapJsObjectToQJsonObject_scalars();
    void jsonConversions_mapJsObjectToQJsonObject_nested();
    void jsonConversions_mapJsObjectToQJsonObject_unmappable();
    void jsonConversions_mapJsObjectToQJsonObject_enumerableOnly();
    void jsonConversions_mapJsObjectToQJsonObject_ownOnly();
    void jsonConversions_mapJsObjectToQJsonObject_cycle();
    void jsonConversions_mapJsObjectToQJsonObject_throwingGetter();
    void jsonConversions_mapQJsonObjectToJsObjectAndBack_preservesValues_data();
    void jsonConversions_mapQJsonObjectToJsObjectAndBack_preservesValues();

private:
    std::thread::id m_qtThreadId;
    std::thread::id m_jsThreadId;
};

void tst_Harmony::initTestCase()
{
    m_qtThreadId = std::this_thread::get_id();
    m_jsThreadId = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &) {
            return std::this_thread::get_id();
        });

    QCOMPARE_NE(m_jsThreadId, m_qtThreadId);
}

void tst_Harmony::cleanup()
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

void tst_Harmony::application_nativeInterface_available()
{
    QVERIFY(qApp->nativeInterface<QHarmonyApplication>());
}

void tst_Harmony::application_runOnJsThreadAndWait_runsOnJsThread()
{
    struct TaskInfo
    {
        std::thread::id threadId;
        bool hasEnv = false;
    };

    const auto taskInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return TaskInfo{
                .threadId = std::this_thread::get_id(),
                .hasEnv = static_cast<bool>(jsContext.env()),
            };
        });

    QCOMPARE_NE(taskInfo.threadId, m_qtThreadId);
    QVERIFY(taskInfo.hasEnv);
}

void tst_Harmony::application_runOnJsThreadAndWait_blocks()
{
    bool taskRan = false;
    QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &) {
            taskRan = true;
        });

    QVERIFY(taskRan);
}

void tst_Harmony::application_runOnJsThreadAndWait_returnsResult()
{
    const auto intResult = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &) {
            return 42;
        });
    QCOMPARE(intResult, 42);

    const auto stringResult = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &) {
            return QStringLiteral("result");
        });
    QCOMPARE(stringResult, QStringLiteral("result"));

    const auto jsonObjectResult = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &) {
            return QJsonObject{{"key", "value"}};
        });
    QCOMPARE(jsonObjectResult, (QJsonObject{{"key", "value"}}));

    const auto refResult = makeRefToNewJsObject();
    QVERIFY(!refResult.isNull());

    const auto moveOnlyResult = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &) {
            return std::make_unique<int>(7);
        });
    QVERIFY(moveOnlyResult);
    QCOMPARE(*moveOnlyResult, 7);
}

void tst_Harmony::application_runOnJsThreadAndWait_cppException()
{
    bool taskRan = false;
    QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &) {
            taskRan = true;
            throw std::runtime_error("test exception");
        });
    QVERIFY(taskRan);

    bool resultTaskRan = false;
    const auto result = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &) -> int {
            resultTaskRan = true;
            throw std::runtime_error("test exception");
        });
    QVERIFY(resultTaskRan);
    QCOMPARE(result, 0);

    const auto laterTaskRan = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &) {
            return true;
        });
    QVERIFY(laterTaskRan);
}

void tst_Harmony::application_runOnJsThreadAndWait_jsException()
{
    bool taskRan = false;
    QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            taskRan = true;
            checkNapiStatus(napi_throw_error(jsContext.env(), nullptr, "test error"), "napi_throw_error");
        });
    QVERIFY(taskRan);

    bool resultTaskRan = false;
    const auto result = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            resultTaskRan = true;
            checkNapiStatus(napi_throw_error(jsContext.env(), nullptr, "test error"), "napi_throw_error");
            return 42;
        });
    QVERIFY(resultTaskRan);
    QCOMPARE(result, 0);

    const auto exceptionPending = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return isJsExceptionPending(jsContext.env());
        });
    QVERIFY(!exceptionPending);
}

// Called on the JS thread, the inner tasks run directly, before the task queued just before them.
void tst_Harmony::application_runOnJsThreadAndWait_nested()
{
    auto taskOrder = std::make_shared<std::vector<int>>();
    const auto innerTaskThreadId = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &) {
            QHarmonyApplication::runOnJsThread(
                [taskOrder](QHarmonyJsContext &) {
                    taskOrder->push_back(1);
                });
            const auto taskThreadId = QHarmonyApplication::runOnJsThreadAndWait(
                [&](QHarmonyJsContext &) {
                    taskOrder->push_back(2);
                    return std::this_thread::get_id();
                });
            QHarmonyApplication::runOnJsThreadAndWait(
                [&](QHarmonyJsContext &) {
                    taskOrder->push_back(3);
                });
            return taskThreadId;
        });
    const auto finalTaskOrder = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &) {
            return *taskOrder;
        });

    QCOMPARE(innerTaskThreadId, m_jsThreadId);
    QCOMPARE(finalTaskOrder, (std::vector<int>{2, 3, 1}));
}

void tst_Harmony::application_runOnJsThreadAndWait_nestedException()
{
    struct NestedExceptionInfo
    {
        int cppExceptionResult = -1;
        int jsExceptionResult = -1;
        bool exceptionPending = true;
        bool outerTaskFinished = false;
    };

    NestedExceptionInfo exceptionInfo;
    QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto cppExceptionResult = QHarmonyApplication::runOnJsThreadAndWait(
                [](QHarmonyJsContext &) -> int {
                    throw std::runtime_error("test exception");
                });
            const auto jsExceptionResult = QHarmonyApplication::runOnJsThreadAndWait(
                [](QHarmonyJsContext &nestedJsContext) {
                    checkNapiStatus(napi_throw_error(nestedJsContext.env(), nullptr, "test error"), "napi_throw_error");
                    return 42;
                });
            exceptionInfo = NestedExceptionInfo{
                .cppExceptionResult = cppExceptionResult,
                .jsExceptionResult = jsExceptionResult,
                .exceptionPending = isJsExceptionPending(jsContext.env()),
                .outerTaskFinished = true,
            };
        });

    QVERIFY(exceptionInfo.outerTaskFinished);
    QCOMPARE(exceptionInfo.cppExceptionResult, 0);
    QCOMPARE(exceptionInfo.jsExceptionResult, 0);
    QVERIFY(!exceptionInfo.exceptionPending);
}

void tst_Harmony::application_runOnJsThreadAndWait_fromWorkerThread()
{
    std::thread::id taskThreadId;
    runInWorkerThread(
        [&]() {
            taskThreadId = QHarmonyApplication::runOnJsThreadAndWait(
                [](QHarmonyJsContext &) {
                    return std::this_thread::get_id();
                });
        });

    QCOMPARE(taskThreadId, m_jsThreadId);
}

void tst_Harmony::application_runOnJsThread_runsOnJsThread()
{
    auto optTaskThreadId = std::make_shared<std::optional<std::thread::id>>();
    queueJsTaskRecordingThreadId(optTaskThreadId);

    QTRY_VERIFY(optTaskThreadId->has_value());
    QCOMPARE(**optTaskThreadId, m_jsThreadId);
}

void tst_Harmony::application_runOnJsThread_fromWorkerThread()
{
    auto optTaskThreadId = std::make_shared<std::optional<std::thread::id>>();
    runInWorkerThread(
        [optTaskThreadId]() {
            queueJsTaskRecordingThreadId(optTaskThreadId);
        });

    QTRY_VERIFY(optTaskThreadId->has_value());
    QCOMPARE(**optTaskThreadId, m_jsThreadId);
}

void tst_Harmony::application_runOnJsThread_ordered()
{
    auto taskOrder = std::make_shared<std::vector<int>>();
    for (int i = 0; i < 5; ++i) {
        QHarmonyApplication::runOnJsThread(
            [taskOrder, i](QHarmonyJsContext &) {
                taskOrder->push_back(i);
            });
    }
    const auto finalTaskOrder = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &) {
            taskOrder->push_back(5);
            return *taskOrder;
        });

    QCOMPARE(finalTaskOrder, (std::vector<int>{0, 1, 2, 3, 4, 5}));
}

void tst_Harmony::application_runOnJsThread_queuedFromJsThread()
{
    auto taskRan = std::make_shared<bool>(false);
    const auto taskRanDirectly = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &) {
            QHarmonyApplication::runOnJsThread(
                [taskRan](QHarmonyJsContext &) {
                    *taskRan = true;
                });
            return *taskRan;
        });
    const auto taskRanLater = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &) {
            return *taskRan;
        });

    QVERIFY(!taskRanDirectly);
    QVERIFY(taskRanLater);
}

void tst_Harmony::application_runOnJsThread_queuedWhileJsThreadBlocked()
{
    auto taskOrder = std::make_shared<std::vector<int>>();
    auto ranTaskCount = std::make_shared<std::atomic<int>>(0);
    auto blockTimedOut = std::make_shared<bool>(false);
    auto jsThreadBlocker = blockJsThread(blockTimedOut);
    QHarmonyApplication::runOnJsThread(
        [taskOrder, ranTaskCount](QHarmonyJsContext &) {
            taskOrder->push_back(1);
            ++*ranTaskCount;
        });
    runInWorkerThread(
        [taskOrder, ranTaskCount]() {
            QHarmonyApplication::runOnJsThread(
                [taskOrder, ranTaskCount](QHarmonyJsContext &) {
                    taskOrder->push_back(2);
                    ++*ranTaskCount;
                });
        });
    QHarmonyApplication::runOnJsThread(
        [taskOrder, ranTaskCount](QHarmonyJsContext &) {
            taskOrder->push_back(3);
            ++*ranTaskCount;
        });
    const auto ranTaskCountWhileBlocked = ranTaskCount->load();
    jsThreadBlocker.reset();
    const auto finalTaskOrder = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &) {
            return *taskOrder;
        });

    QVERIFY(!*blockTimedOut);
    QCOMPARE(ranTaskCountWhileBlocked, 0);
    QCOMPARE(finalTaskOrder, (std::vector<int>{1, 2, 3}));
}

// The task holds the only copy of the tracked capture.
void tst_Harmony::application_runOnJsThread_destroysTaskOnJsThread()
{
    auto destroyThreadPromise = std::make_shared<std::promise<std::thread::id>>();
    auto destroyThreadFuture = destroyThreadPromise->get_future();
    QHarmonyApplication::runOnJsThread(
        [tracker = makeDestroyThreadTracker(std::move(destroyThreadPromise))](QHarmonyJsContext &) {
        });

    QCOMPARE(destroyThreadFuture.wait_for(10s), std::future_status::ready);
    QCOMPARE(destroyThreadFuture.get(), m_jsThreadId);
}

// The caller keeps the task in its own std::function and moves it in.
void tst_Harmony::application_runOnJsThread_destroysMovedTaskOnJsThread()
{
    auto destroyThreadPromise = std::make_shared<std::promise<std::thread::id>>();
    auto destroyThreadFuture = destroyThreadPromise->get_future();
    std::function<void(QHarmonyJsContext &)> task =
        [tracker = makeDestroyThreadTracker(std::move(destroyThreadPromise))](QHarmonyJsContext &) {
        };
    QHarmonyApplication::runOnJsThread(std::move(task));

    QCOMPARE(destroyThreadFuture.wait_for(10s), std::future_status::ready);
    QCOMPARE(destroyThreadFuture.get(), m_jsThreadId);
}

void tst_Harmony::application_runOnJsThread_cppException()
{
    auto taskRan = std::make_shared<bool>(false);
    QHarmonyApplication::runOnJsThread(
        [taskRan](QHarmonyJsContext &) {
            *taskRan = true;
            throw std::runtime_error("test exception");
        });

    const auto laterTaskRan = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &) {
            return true;
        });
    QVERIFY(*taskRan);
    QVERIFY(laterTaskRan);
}

void tst_Harmony::application_runOnJsThread_jsException()
{
    auto taskRan = std::make_shared<bool>(false);
    QHarmonyApplication::runOnJsThread(
        [taskRan](QHarmonyJsContext &jsContext) {
            *taskRan = true;
            checkNapiStatus(napi_throw_error(jsContext.env(), nullptr, "test error"), "napi_throw_error");
        });

    const auto exceptionPending = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return isJsExceptionPending(jsContext.env());
        });
    QVERIFY(*taskRan);
    QVERIFY(!exceptionPending);
}

void tst_Harmony::application_runOnQtThread_fromJsThread()
{
    auto optTaskThreadId = std::make_shared<std::optional<std::thread::id>>();
    QHarmonyApplication::runOnJsThread(
        [optTaskThreadId](QHarmonyJsContext &) {
            queueQtTaskRecordingThreadId(optTaskThreadId);
        });

    QTRY_VERIFY(optTaskThreadId->has_value());
    QCOMPARE(**optTaskThreadId, m_qtThreadId);
}

void tst_Harmony::application_runOnQtThread_fromWorkerThread()
{
    auto optTaskThreadId = std::make_shared<std::optional<std::thread::id>>();
    runInWorkerThread(
        [optTaskThreadId]() {
            queueQtTaskRecordingThreadId(optTaskThreadId);
        });

    QTRY_VERIFY(optTaskThreadId->has_value());
    QCOMPARE(**optTaskThreadId, m_qtThreadId);
}

void tst_Harmony::application_runOnQtThread_queuedFromQtThread()
{
    auto optTaskThreadId = std::make_shared<std::optional<std::thread::id>>();
    queueQtTaskRecordingThreadId(optTaskThreadId);

    QVERIFY(!optTaskThreadId->has_value());
    QTRY_VERIFY(optTaskThreadId->has_value());
    QCOMPARE(**optTaskThreadId, m_qtThreadId);
}

void tst_Harmony::application_runOnQtThread_orderedFromJsThread()
{
    auto taskOrder = std::make_shared<std::vector<int>>();
    QHarmonyApplication::runOnJsThread(
        [taskOrder](QHarmonyJsContext &) {
            for (int i = 0; i < 5; ++i) {
                QHarmonyApplication::runOnQtThread(
                    [taskOrder, i]() {
                        taskOrder->push_back(i);
                    });
            }
        });

    QTRY_COMPARE(*taskOrder, (std::vector<int>{0, 1, 2, 3, 4}));
}

void tst_Harmony::application_runOnQtThread_orderedFromQtThread()
{
    auto taskOrder = std::make_shared<std::vector<int>>();
    for (int i = 0; i < 5; ++i) {
        QHarmonyApplication::runOnQtThread(
            [taskOrder, i]() {
                taskOrder->push_back(i);
            });
    }

    QTRY_COMPARE(*taskOrder, (std::vector<int>{0, 1, 2, 3, 4}));
}

// The JS task hands its only copy of the tracked capture to the Qt task.
void tst_Harmony::application_runOnQtThread_destroysTaskOnQtThread()
{
    auto destroyThreadPromise = std::make_shared<std::promise<std::thread::id>>();
    auto destroyThreadFuture = destroyThreadPromise->get_future();
    QHarmonyApplication::runOnJsThread(
        [tracker = makeDestroyThreadTracker(std::move(destroyThreadPromise))](QHarmonyJsContext &) mutable {
            QHarmonyApplication::runOnQtThread(
                [tracker = std::move(tracker)]() {
                });
        });

    QTRY_COMPARE(destroyThreadFuture.wait_for(0s), std::future_status::ready);
    QCOMPARE(destroyThreadFuture.get(), m_qtThreadId);
}

void tst_Harmony::jsContext_fromEnv_returnsTaskContext()
{
    const auto sameObject = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return &QHarmonyJsContext::fromEnv(jsContext.env()) == &jsContext;
        });

    QVERIFY(sameObject);
}

void tst_Harmony::jsContext_fromEnv_inJsCallback()
{
    const auto sameObject = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            QHarmonyJsContext *callbackJsContext = nullptr;
            callJsFunction(jsContext.env(), makeJsFunction(jsContext.env(), jsContextRecordingCallback, &callbackJsContext));
            return callbackJsContext == &jsContext;
        });

    QVERIFY(sameObject);
}

void tst_Harmony::jsContext_tryImportModule_platformModule()
{
    struct ModuleInfo
    {
        bool moduleIsObject = false;
        bool launchReasonIsObject = false;
        bool startAbilityMatches = false;
    };

    const auto moduleInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto moduleValue = jsContext.tryImportModule(abilityConstantModuleName);
            const auto launchReasonValue = getJsProperty(env, moduleValue, "LaunchReason");
            return ModuleInfo{
                .moduleIsObject = jsTypeOf(env, moduleValue) == napi_object,
                .launchReasonIsObject = jsTypeOf(env, launchReasonValue) == napi_object,
                .startAbilityMatches = jsPropertyStrictEquals(env, launchReasonValue, "START_ABILITY", makeJsNumber(env, 1)),
            };
        });

    QVERIFY(moduleInfo.moduleIsObject);
    QVERIFY(moduleInfo.launchReasonIsObject);
    QVERIFY(moduleInfo.startAbilityMatches);
}

// The share kit backs Qt's own share support, so every supported device has it.
void tst_Harmony::jsContext_tryImportModule_kitModule()
{
    struct KitModuleInfo
    {
        bool moduleIsObject = false;
        bool sharedDataIsFunction = false;
        bool shareControllerIsFunction = false;
    };

    const auto kitModuleInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto moduleValue = jsContext.tryImportModule(systemShareModuleName);
            return KitModuleInfo{
                .moduleIsObject = jsTypeOf(env, moduleValue) == napi_object,
                .sharedDataIsFunction = jsTypeOf(env, getJsProperty(env, moduleValue, "SharedData")) == napi_function,
                .shareControllerIsFunction = jsTypeOf(env, getJsProperty(env, moduleValue, "ShareController")) == napi_function,
            };
        });

    QVERIFY(kitModuleInfo.moduleIsObject);
    QVERIFY(kitModuleInfo.sharedDataIsFunction);
    QVERIFY(kitModuleInfo.shareControllerIsFunction);
}

void tst_Harmony::jsContext_tryImportModule_sameObject()
{
    struct ImportInfo
    {
        bool sameWithinTask = false;
        QHarmonyJsObjectRef moduleRef;
    };

    const auto importInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto moduleValue = jsContext.tryImportModule(abilityConstantModuleName);
            return ImportInfo{
                .sameWithinTask = jsStrictEquals(jsContext.env(), moduleValue, jsContext.tryImportModule(abilityConstantModuleName)),
                .moduleRef = QHarmonyJsObjectRef::fromValue(jsContext, moduleValue),
            };
        });
    const auto sameAcrossTasks = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            return jsStrictEquals(jsContext.env(), importInfo.moduleRef.value(jsContext), jsContext.tryImportModule(abilityConstantModuleName));
        });

    QVERIFY(!importInfo.moduleRef.isNull());
    QVERIFY(importInfo.sameWithinTask);
    QVERIFY(sameAcrossTasks);
}

void tst_Harmony::jsContext_tryImportModule_failure_data()
{
    QTest::addColumn<QByteArray>("moduleName");

    QTest::newRow("nonexistent") << QByteArray("@ohos.qt.nonexistent");
    QTest::newRow("empty") << QByteArray();
}

void tst_Harmony::jsContext_tryImportModule_failure()
{
    QFETCH(QByteArray, moduleName);

    struct ImportFailureInfo
    {
        bool moduleIsNull = false;
        bool exceptionPending = true;
    };

    const auto failureInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto moduleValue = jsContext.tryImportModule(moduleName.toStdString());
            return ImportFailureInfo{
                .moduleIsNull = !moduleValue,
                .exceptionPending = isJsExceptionPending(jsContext.env()),
            };
        });

    QVERIFY(failureInfo.moduleIsNull);
    QVERIFY(!failureInfo.exceptionPending);
}

void tst_Harmony::appJsContext_fromJsContext_sameObject()
{
    const auto sameObject = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            auto &appJsContext = QHarmonyAppJsContext::fromJsContext(jsContext);
            return &appJsContext == &QHarmonyAppJsContext::fromEnv(jsContext.env())
                && &appJsContext == &jsContext;
        });

    QVERIFY(sameObject);
}

void tst_Harmony::appJsContext_fromEnv_inJsCallback()
{
    const auto sameObject = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            QHarmonyAppJsContext *callbackAppJsContext = nullptr;
            callJsFunction(jsContext.env(), makeJsFunction(jsContext.env(), appJsContextRecordingCallback, &callbackAppJsContext));
            return callbackAppJsContext == &QHarmonyAppJsContext::fromJsContext(jsContext);
        });

    QVERIFY(sameObject);
}

void tst_Harmony::appJsContext_defaultAbility_isUiAbility()
{
    struct AbilityInfo
    {
        bool abilityIsObject = false;
        bool onWindowStageCreateIsFunction = false;
        bool contextIsObject = false;
        bool getApplicationContextIsFunction = false;
    };

    const auto abilityInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto abilityValue = QHarmonyAppJsContext::fromJsContext(jsContext).defaultAbility();
            const auto contextValue = getJsProperty(env, abilityValue, "context");
            return AbilityInfo{
                .abilityIsObject = jsTypeOf(env, abilityValue) == napi_object,
                .onWindowStageCreateIsFunction = jsTypeOf(env, getJsProperty(env, abilityValue, "onWindowStageCreate")) == napi_function,
                .contextIsObject = jsTypeOf(env, contextValue) == napi_object,
                .getApplicationContextIsFunction = jsTypeOf(env, getJsProperty(env, contextValue, "getApplicationContext")) == napi_function,
            };
        });

    QVERIFY(abilityInfo.abilityIsObject);
    QVERIFY(abilityInfo.onWindowStageCreateIsFunction);
    QVERIFY(abilityInfo.contextIsObject);
    QVERIFY(abilityInfo.getApplicationContextIsFunction);
}

void tst_Harmony::appJsContext_defaultAbility_stable()
{
    struct StableAbilityInfo
    {
        bool sameWithinTask = false;
        QHarmonyJsObjectRef abilityRef;
    };

    const auto abilityInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            auto &appJsContext = QHarmonyAppJsContext::fromJsContext(jsContext);
            const auto abilityValue = appJsContext.defaultAbility();
            return StableAbilityInfo{
                .sameWithinTask = jsStrictEquals(jsContext.env(), abilityValue, appJsContext.defaultAbility()),
                .abilityRef = QHarmonyJsObjectRef::fromValue(jsContext, abilityValue),
            };
        });
    const auto sameAcrossTasks = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            auto &appJsContext = QHarmonyAppJsContext::fromJsContext(jsContext);
            return jsStrictEquals(jsContext.env(), abilityInfo.abilityRef.value(jsContext), appJsContext.defaultAbility());
        });

    QVERIFY(!abilityInfo.abilityRef.isNull());
    QVERIFY(abilityInfo.sameWithinTask);
    QVERIFY(sameAcrossTasks);
}

void tst_Harmony::jsObjectRef_defaultConstructor_isNull()
{
    const QHarmonyJsObjectRef ref;
    const QHarmonyJsObjectRef otherRef;
    const auto valueIsNull = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            return !ref.value(jsContext);
        });

    QVERIFY(ref.isNull());
    QCOMPARE(ref, otherRef);
    QCOMPARE(qHash(ref), qHash(otherRef));
    QVERIFY(valueIsNull);
}

void tst_Harmony::jsObjectRef_fromValue_nonObject_data()
{
    QTest::addColumn<JsValueKind>("kind");

    QTest::newRow("nullPointer") << JsValueKind::NullPointer;
    QTest::newRow("null") << JsValueKind::Null;
    QTest::newRow("undefined") << JsValueKind::Undefined;
    QTest::newRow("number") << JsValueKind::Number;
    QTest::newRow("string") << JsValueKind::String;
    QTest::newRow("boolean") << JsValueKind::Boolean;
    QTest::newRow("symbol") << JsValueKind::Symbol;
    QTest::newRow("bigInt") << JsValueKind::BigInt;
}

void tst_Harmony::jsObjectRef_fromValue_nonObject()
{
    QFETCH(JsValueKind, kind);

    struct FromValueInfo
    {
        bool refIsNull = false;
        bool exceptionPending = true;
    };

    const auto fromValueInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto ref = QHarmonyJsObjectRef::fromValue(jsContext, makeJsValueOfKind(jsContext.env(), kind));
            return FromValueInfo{
                .refIsNull = ref.isNull(),
                .exceptionPending = isJsExceptionPending(jsContext.env()),
            };
        });

    QVERIFY(fromValueInfo.refIsNull);
    QVERIFY(!fromValueInfo.exceptionPending);
}

// A frozen object checks that the reference works without modifying the object.
void tst_Harmony::jsObjectRef_fromValue_objectKinds_data()
{
    QTest::addColumn<JsValueKind>("kind");

    QTest::newRow("plainObject") << JsValueKind::PlainObject;
    QTest::newRow("array") << JsValueKind::Array;
    QTest::newRow("function") << JsValueKind::Function;
    QTest::newRow("frozenObject") << JsValueKind::FrozenObject;
}

void tst_Harmony::jsObjectRef_fromValue_objectKinds()
{
    QFETCH(JsValueKind, kind);

    struct ObjectRefInfo
    {
        bool refIsNull = true;
        bool valueMatches = false;
    };

    const auto fromValueInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto value = makeJsValueOfKind(jsContext.env(), kind);
            const auto ref = QHarmonyJsObjectRef::fromValue(jsContext, value);
            return ObjectRefInfo{
                .refIsNull = ref.isNull(),
                .valueMatches = jsStrictEquals(jsContext.env(), ref.value(jsContext), value),
            };
        });

    QVERIFY(!fromValueInfo.refIsNull);
    QVERIFY(fromValueInfo.valueMatches);
}

// Only the reference keeps the object between the tasks.
void tst_Harmony::jsObjectRef_value_acrossTasks()
{
    const auto ref = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return QHarmonyJsObjectRef::fromValue(jsContext, withJsNumberProperty(jsContext.env(), makeJsObject(jsContext.env())));
        });
    const auto propertyMatches = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            return jsPropertyStrictEquals(env, ref.value(jsContext), "number", makeJsNumber(env, 1));
        });

    QVERIFY(!ref.isNull());
    QVERIFY(propertyMatches);
}

void tst_Harmony::jsObjectRef_equality_sameObject()
{
    const auto refsFromOneTask = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto objectValue = makeJsObject(jsContext.env());
            keepJsObjectInGlobal(jsContext.env(), keptJsObjectPropName, objectValue);
            return JsObjectRefPair{
                .first = QHarmonyJsObjectRef::fromValue(jsContext, objectValue),
                .second = QHarmonyJsObjectRef::fromValue(jsContext, objectValue),
            };
        });
    const auto refFromOtherTask = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return QHarmonyJsObjectRef::fromValue(jsContext, takeJsObjectFromGlobal(jsContext.env(), keptJsObjectPropName));
        });

    QVERIFY(!refsFromOneTask.first.isNull());
    QCOMPARE(refsFromOneTask.first, refsFromOneTask.second);
    QCOMPARE(refsFromOneTask.first, refFromOtherTask);
    QCOMPARE(qHash(refsFromOneTask.first), qHash(refsFromOneTask.second));
    QCOMPARE(qHash(refsFromOneTask.first), qHash(refFromOtherTask));

    constexpr std::size_t seed = 42;
    QCOMPARE(qHash(refsFromOneTask.first, seed), qHash(refsFromOneTask.second, seed));
    QCOMPARE(qHash(refsFromOneTask.first, seed), qHash(refFromOtherTask, seed));
}

void tst_Harmony::jsObjectRef_equality_differentObjects()
{
    const auto ref = makeRefToNewJsObject();
    const auto otherRef = makeRefToNewJsObject();

    QVERIFY(!ref.isNull());
    QVERIFY(!otherRef.isNull());
    QCOMPARE_NE(ref, otherRef);
    QCOMPARE_NE(ref, QHarmonyJsObjectRef());
}

void tst_Harmony::jsObjectRef_copyAndMove_keepsObject()
{
    const auto ref = makeRefToNewJsObject();
    QVERIFY(!ref.isNull());

    const QHarmonyJsObjectRef copy(ref);
    QCOMPARE(copy, ref);

    QHarmonyJsObjectRef copyAssigned;
    copyAssigned = ref;
    QCOMPARE(copyAssigned, ref);

    QHarmonyJsObjectRef moveSource(ref);
    const QHarmonyJsObjectRef moveConstructed(std::move(moveSource));
    QCOMPARE(moveConstructed, ref);
    QVERIFY(moveSource.isNull());

    QHarmonyJsObjectRef moveAssignSource(ref);
    QHarmonyJsObjectRef moveAssigned;
    moveAssigned = std::move(moveAssignSource);
    QCOMPARE(moveAssigned, ref);
    QVERIFY(moveAssignSource.isNull());
}

void tst_Harmony::jsObjectRef_copyAndCompare_offJsThread()
{
    struct WorkerResult
    {
        bool copyIsNull = true;
        bool copyEqualsSource = false;
        bool copyEqualsIndependentRef = false;
        bool hashesEqual = false;
    };

    const auto refs = makeTwoRefsToNewJsObject();
    WorkerResult workerResult;
    runInWorkerThread(
        [&]() {
            const QHarmonyJsObjectRef copy(refs.first);
            workerResult = WorkerResult{
                .copyIsNull = copy.isNull(),
                .copyEqualsSource = copy == refs.first,
                .copyEqualsIndependentRef = copy == refs.second,
                .hashesEqual = qHash(copy) == qHash(refs.second),
            };
        });

    QVERIFY(!workerResult.copyIsNull);
    QVERIFY(workerResult.copyEqualsSource);
    QVERIFY(workerResult.copyEqualsIndependentRef);
    QVERIFY(workerResult.hashesEqual);
}

void tst_Harmony::jsObjectRef_copyAndDestroy_concurrentThreads()
{
    constexpr int workerCount = 4;
    constexpr int iterationCount = 1000;

    const auto refs = makeTwoRefsToNewJsObject();
    std::vector<int> matchCounts(workerCount, 0);
    std::vector<std::unique_ptr<QThread>> workerThreads;
    for (int i = 0; i < workerCount; ++i) {
        workerThreads.emplace_back(QThread::create(
            [refs, &matchCount = matchCounts[i]]() {
                for (int j = 0; j < iterationCount; ++j) {
                    const QHarmonyJsObjectRef copy(refs.first);
                    if (!copy.isNull() && copy == refs.second && qHash(copy) == qHash(refs.second))
                        ++matchCount;
                }
            }));
    }
    for (const auto &workerThread : workerThreads)
        workerThread->start();
    for (const auto &workerThread : workerThreads)
        workerThread->wait();

    const auto sameObject = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            return jsStrictEquals(jsContext.env(), refs.first.value(jsContext), refs.second.value(jsContext));
        });

    QCOMPARE(matchCounts, std::vector<int>(workerCount, iterationCount));
    QVERIFY(sameObject);
}

void tst_Harmony::jsObjectRef_qHash_containers()
{
    const auto refs = makeTwoRefsToNewJsObject();
    const QSet<QHarmonyJsObjectRef> qtSet{refs.first, refs.second};
    const std::unordered_set<QHarmonyJsObjectRef> stdSet{refs.first, refs.second};

    QVERIFY(!refs.first.isNull());
    QCOMPARE(qtSet.size(), 1);
    QCOMPARE(stdSet.size(), 1u);
}

void tst_Harmony::jsObjectRef_destructor_offJsThread()
{
    {
        const auto ref = makeRefToNewJsObject();
        QVERIFY(!ref.isNull());
    }
    runInWorkerThread(
        [ref = makeRefToNewJsObject()]() mutable {
            ref = QHarmonyJsObjectRef();
        });

    const auto laterTaskRan = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &) {
            return true;
        });
    QVERIFY(laterTaskRan);
}

void tst_Harmony::jsonConversions_mapQJsonObjectToJsObject_empty()
{
    struct EmptyObjectInfo
    {
        bool isPlainObject = false;
        std::uint32_t propertyCount = 0;
    };

    const auto emptyObjectInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto objectValue = QtHarmony::mapQJsonObjectToJsObject(env, QJsonObject());
            return EmptyObjectInfo{
                .isPlainObject = isJsPlainObject(env, objectValue),
                .propertyCount = getJsPropertyCount(env, objectValue),
            };
        });

    QVERIFY(emptyObjectInfo.isPlainObject);
    QCOMPARE(emptyObjectInfo.propertyCount, 0u);
}

void tst_Harmony::jsonConversions_mapQJsonObjectToJsObject_scalars()
{
    struct ScalarsInfo
    {
        bool trueMatches = false;
        bool falseMatches = false;
        bool integerMatches = false;
        bool negativeIntegerMatches = false;
        bool doubleMatches = false;
        bool asciiStringMatches = false;
        bool nonAsciiStringMatches = false;
        bool emptyStringMatches = false;
        bool nullMatches = false;
        std::uint32_t propertyCount = 0;
    };

    const auto jsonObject = scalarsJsonObject();
    const auto nonAsciiUtf8Text = nonAsciiText().toStdString();
    const auto scalarsInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto objectValue = QtHarmony::mapQJsonObjectToJsObject(env, jsonObject);
            return ScalarsInfo{
                .trueMatches = jsPropertyStrictEquals(env, objectValue, "true", makeJsBoolean(env, true)),
                .falseMatches = jsPropertyStrictEquals(env, objectValue, "false", makeJsBoolean(env, false)),
                .integerMatches = jsPropertyStrictEquals(env, objectValue, "integer", makeJsNumber(env, 42)),
                .negativeIntegerMatches = jsPropertyStrictEquals(env, objectValue, "negativeInteger", makeJsNumber(env, -7)),
                .doubleMatches = jsPropertyStrictEquals(env, objectValue, "double", makeJsNumber(env, 0.25)),
                .asciiStringMatches = jsPropertyStrictEquals(env, objectValue, "asciiString", makeJsString(env, "text")),
                .nonAsciiStringMatches = jsPropertyStrictEquals(env, objectValue, "nonAsciiString", makeJsString(env, nonAsciiUtf8Text)),
                .emptyStringMatches = jsPropertyStrictEquals(env, objectValue, "emptyString", makeJsString(env, "")),
                .nullMatches = jsPropertyStrictEquals(env, objectValue, "null", makeJsNull(env)),
                .propertyCount = getJsPropertyCount(env, objectValue),
            };
        });

    QVERIFY(scalarsInfo.trueMatches);
    QVERIFY(scalarsInfo.falseMatches);
    QVERIFY(scalarsInfo.integerMatches);
    QVERIFY(scalarsInfo.negativeIntegerMatches);
    QVERIFY(scalarsInfo.doubleMatches);
    QVERIFY(scalarsInfo.asciiStringMatches);
    QVERIFY(scalarsInfo.nonAsciiStringMatches);
    QVERIFY(scalarsInfo.emptyStringMatches);
    QVERIFY(scalarsInfo.nullMatches);
    QCOMPARE(scalarsInfo.propertyCount, 9u);
}

void tst_Harmony::jsonConversions_mapQJsonObjectToJsObject_nested()
{
    struct NestedInfo
    {
        bool objectMatches = false;
        bool arrayMatches = false;
        bool innerArrayMatches = false;
        bool innerObjectMatches = false;
        bool emptyObjectMatches = false;
        bool emptyArrayMatches = false;
        std::uint32_t propertyCount = 0;
    };

    const auto jsonObject = nestedJsonObject();
    const auto nestedInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto objectValue = QtHarmony::mapQJsonObjectToJsObject(env, jsonObject);
            const auto nestedObjectValue = getJsProperty(env, objectValue, "object");
            const auto arrayValue = getJsProperty(env, objectValue, "array");
            const auto innerArrayValue = getJsArrayElement(env, arrayValue, 4);
            const auto innerObjectValue = getJsArrayElement(env, arrayValue, 5);
            const auto emptyObjectValue = getJsProperty(env, objectValue, "emptyObject");
            const auto emptyArrayValue = getJsProperty(env, objectValue, "emptyArray");
            return NestedInfo{
                .objectMatches = isJsPlainObject(env, nestedObjectValue)
                    && getJsPropertyCount(env, nestedObjectValue) == 2
                    && jsPropertyStrictEquals(env, nestedObjectValue, "number", makeJsNumber(env, 1))
                    && jsPropertyStrictEquals(env, nestedObjectValue, "string", makeJsString(env, "text")),
                .arrayMatches = isJsArray(env, arrayValue)
                    && getJsArrayLength(env, arrayValue) == 6
                    && jsStrictEquals(env, getJsArrayElement(env, arrayValue, 0), makeJsNumber(env, 1))
                    && jsStrictEquals(env, getJsArrayElement(env, arrayValue, 1), makeJsString(env, "text"))
                    && jsStrictEquals(env, getJsArrayElement(env, arrayValue, 2), makeJsBoolean(env, false))
                    && jsStrictEquals(env, getJsArrayElement(env, arrayValue, 3), makeJsNull(env)),
                .innerArrayMatches = isJsArray(env, innerArrayValue)
                    && getJsArrayLength(env, innerArrayValue) == 1
                    && jsStrictEquals(env, getJsArrayElement(env, innerArrayValue, 0), makeJsNumber(env, 2.5)),
                .innerObjectMatches = isJsPlainObject(env, innerObjectValue)
                    && getJsPropertyCount(env, innerObjectValue) == 1
                    && jsPropertyStrictEquals(env, innerObjectValue, "number", makeJsNumber(env, 3)),
                .emptyObjectMatches = isJsPlainObject(env, emptyObjectValue)
                    && getJsPropertyCount(env, emptyObjectValue) == 0,
                .emptyArrayMatches = isJsArray(env, emptyArrayValue)
                    && getJsArrayLength(env, emptyArrayValue) == 0,
                .propertyCount = getJsPropertyCount(env, objectValue),
            };
        });

    QVERIFY(nestedInfo.objectMatches);
    QVERIFY(nestedInfo.arrayMatches);
    QVERIFY(nestedInfo.innerArrayMatches);
    QVERIFY(nestedInfo.innerObjectMatches);
    QVERIFY(nestedInfo.emptyObjectMatches);
    QVERIFY(nestedInfo.emptyArrayMatches);
    QCOMPARE(nestedInfo.propertyCount, 4u);
}

void tst_Harmony::jsonConversions_mapQJsonObjectToJsObject_newObject()
{
    const auto distinctObjects = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const QJsonObject jsonObject{{"value", 1}};
            const auto objectValue = QtHarmony::mapQJsonObjectToJsObject(env, jsonObject);
            const auto otherObjectValue = QtHarmony::mapQJsonObjectToJsObject(env, jsonObject);
            return jsTypeOf(env, objectValue) == napi_object
                && jsTypeOf(env, otherObjectValue) == napi_object
                && !jsStrictEquals(env, objectValue, otherObjectValue);
        });

    QVERIFY(distinctObjects);
}

void tst_Harmony::jsonConversions_mapJsObjectToQJsonObject_notPlainObject_data()
{
    QTest::addColumn<JsValueKind>("kind");

    QTest::newRow("nullPointer") << JsValueKind::NullPointer;
    QTest::newRow("null") << JsValueKind::Null;
    QTest::newRow("undefined") << JsValueKind::Undefined;
    QTest::newRow("number") << JsValueKind::Number;
    QTest::newRow("string") << JsValueKind::String;
    QTest::newRow("boolean") << JsValueKind::Boolean;
    QTest::newRow("symbol") << JsValueKind::Symbol;
    QTest::newRow("bigInt") << JsValueKind::BigInt;
    QTest::newRow("array") << JsValueKind::Array;
    QTest::newRow("function") << JsValueKind::Function;
}

void tst_Harmony::jsonConversions_mapJsObjectToQJsonObject_notPlainObject()
{
    QFETCH(JsValueKind, kind);

    const auto mappingResult = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto jsonObject = QtHarmony::mapJsObjectToQJsonObject(env, makeJsValueOfKind(env, kind));
            return JsonMappingResult{
                .jsonObject = jsonObject,
                .exceptionPending = isJsExceptionPending(env),
            };
        });

    QVERIFY(mappingResult.jsonObject.isEmpty());
    QVERIFY(!mappingResult.exceptionPending);
}

void tst_Harmony::jsonConversions_mapJsObjectToQJsonObject_scalars()
{
    const auto nonAsciiUtf8Text = nonAsciiText().toStdString();
    const auto jsonObject = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto objectValue = makeJsObject(env);
            setJsProperty(env, objectValue, "true", makeJsBoolean(env, true));
            setJsProperty(env, objectValue, "false", makeJsBoolean(env, false));
            setJsProperty(env, objectValue, "integer", makeJsNumber(env, 42));
            setJsProperty(env, objectValue, "negativeInteger", makeJsNumber(env, -7));
            setJsProperty(env, objectValue, "double", makeJsNumber(env, 0.25));
            setJsProperty(env, objectValue, "asciiString", makeJsString(env, "text"));
            setJsProperty(env, objectValue, "nonAsciiString", makeJsString(env, nonAsciiUtf8Text));
            setJsProperty(env, objectValue, "emptyString", makeJsString(env, ""));
            setJsProperty(env, objectValue, "null", makeJsNull(env));
            return QtHarmony::mapJsObjectToQJsonObject(env, objectValue);
        });

    QCOMPARE(jsonObject, scalarsJsonObject());
}

void tst_Harmony::jsonConversions_mapJsObjectToQJsonObject_nested()
{
    const auto jsonObject = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto nestedObjectValue = makeJsObject(env);
            setJsProperty(env, nestedObjectValue, "number", makeJsNumber(env, 1));
            setJsProperty(env, nestedObjectValue, "string", makeJsString(env, "text"));
            const auto innerObjectValue = makeJsObject(env);
            setJsProperty(env, innerObjectValue, "number", makeJsNumber(env, 3));
            const auto arrayValue = makeJsArray(
                env,
                {
                    makeJsNumber(env, 1),
                    makeJsString(env, "text"),
                    makeJsBoolean(env, false),
                    makeJsNull(env),
                    makeJsArray(env, {makeJsNumber(env, 2.5)}),
                    innerObjectValue,
                });
            const auto objectValue = makeJsObject(env);
            setJsProperty(env, objectValue, "object", nestedObjectValue);
            setJsProperty(env, objectValue, "array", arrayValue);
            setJsProperty(env, objectValue, "emptyObject", makeJsObject(env));
            setJsProperty(env, objectValue, "emptyArray", makeJsArray(env, {}));
            return QtHarmony::mapJsObjectToQJsonObject(env, objectValue);
        });

    QCOMPARE(jsonObject, nestedJsonObject());
}

void tst_Harmony::jsonConversions_mapJsObjectToQJsonObject_unmappable()
{
    const auto jsonObject = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto objectValue = makeJsObject(env);
            setJsProperty(env, objectValue, "kept", makeJsNumber(env, 1));
            setJsProperty(env, objectValue, "undefined", makeJsUndefined(env));
            setJsProperty(env, objectValue, "function", makeJsFunction(env, emptyJsCallback));
            setJsProperty(env, objectValue, "symbol", makeJsSymbol(env));
            setJsProperty(env, objectValue, "bigInt", makeJsBigInt(env, 1));
            setJsProperty(
                env, objectValue, "array",
                makeJsArray(
                    env,
                    {
                        makeJsUndefined(env),
                        makeJsFunction(env, emptyJsCallback),
                        makeJsSymbol(env),
                        makeJsBigInt(env, 1),
                    }));
            return QtHarmony::mapJsObjectToQJsonObject(env, objectValue);
        });

    const QJsonObject expected{
        {"kept", 1},
        {"array", QJsonArray{QJsonValue::Null, QJsonValue::Null, QJsonValue::Null, QJsonValue::Null}},
    };
    QCOMPARE(jsonObject, expected);
}

void tst_Harmony::jsonConversions_mapJsObjectToQJsonObject_enumerableOnly()
{
    const auto jsonObject = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto objectValue = makeJsObject(env);
            setJsProperty(env, objectValue, "visible", makeJsNumber(env, 1));
            defineNonEnumerableJsProperty(env, objectValue, "hidden", makeJsNumber(env, 2));
            return QtHarmony::mapJsObjectToQJsonObject(env, objectValue);
        });

    QCOMPARE(jsonObject, (QJsonObject{{"visible", 1}}));
}

void tst_Harmony::jsonConversions_mapJsObjectToQJsonObject_ownOnly()
{
    const auto jsonObject = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto prototypeValue = makeJsObject(env);
            setJsProperty(env, prototypeValue, "inherited", makeJsNumber(env, 1));
            const auto objectValue = makeJsObjectWithPrototype(env, prototypeValue);
            setJsProperty(env, objectValue, "own", makeJsNumber(env, 2));
            return QtHarmony::mapJsObjectToQJsonObject(env, objectValue);
        });

    QCOMPARE(jsonObject, (QJsonObject{{"own", 2}}));
}

void tst_Harmony::jsonConversions_mapJsObjectToQJsonObject_cycle()
{
    const auto mappingResult = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto objectValue = withJsNumberProperty(env, makeJsObject(env));
            setJsProperty(env, objectValue, "self", objectValue);
            const auto jsonObject = QtHarmony::mapJsObjectToQJsonObject(env, objectValue);
            return JsonMappingResult{
                .jsonObject = jsonObject,
                .exceptionPending = isJsExceptionPending(env),
            };
        });

    QVERIFY(mappingResult.jsonObject.isEmpty());
    QVERIFY(!mappingResult.exceptionPending);
}

void tst_Harmony::jsonConversions_mapJsObjectToQJsonObject_throwingGetter()
{
    const auto mappingResult = QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto objectValue = withJsNumberProperty(env, makeJsObject(env));
            defineEnumerableJsGetter(env, objectValue, "failing", failingJsGetterCallback);
            const auto jsonObject = QtHarmony::mapJsObjectToQJsonObject(env, objectValue);
            return JsonMappingResult{
                .jsonObject = jsonObject,
                .exceptionPending = isJsExceptionPending(env),
            };
        });

    QVERIFY(mappingResult.jsonObject.isEmpty());
    QVERIFY(!mappingResult.exceptionPending);
}

void tst_Harmony::jsonConversions_mapQJsonObjectToJsObjectAndBack_preservesValues_data()
{
    QTest::addColumn<QJsonObject>("jsonObject");

    const auto maxSafeInteger = (static_cast<qint64>(1) << 53) - 1;

    QTest::newRow("empty") << QJsonObject();
    QTest::newRow("scalars") << scalarsJsonObject();
    QTest::newRow("unicode") << QJsonObject{
        {QStringLiteral(u"za\u017c\u00f3\u0142\u0107"), QStringLiteral(u"\U0001F600 \u4e2d\u6587")},
    };
    QTest::newRow("nested") << nestedJsonObject();
    QTest::newRow("deeplyNested") << QJsonObject{
        {"object", QJsonObject{{"array", QJsonArray{1, QJsonObject{{"emptyArray", QJsonArray()}}}}}},
    };
    QTest::newRow("largeIntegers") << QJsonObject{
        {"maxSafe", maxSafeInteger},
        {"minSafe", -maxSafeInteger},
    };
}

void tst_Harmony::jsonConversions_mapQJsonObjectToJsObjectAndBack_preservesValues()
{
    QFETCH(QJsonObject, jsonObject);

    const auto roundTripped = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            return QtHarmony::mapJsObjectToQJsonObject(env, QtHarmony::mapQJsonObjectToJsObject(env, jsonObject));
        });

    QCOMPARE(roundTripped, jsonObject);
}

QTEST_GUILESS_MAIN(tst_Harmony)
#include "tst_harmony.moc"
