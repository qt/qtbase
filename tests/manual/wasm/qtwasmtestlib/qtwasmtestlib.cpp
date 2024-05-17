// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qtwasmtestlib.h"

#include <QtCore/qmetaobject.h>

#include <emscripten/bind.h>
#include <emscripten.h>
#include <emscripten/threading.h>

namespace QtWasmTest {
namespace {
QObject *g_testObject = nullptr;
std::string g_currentTestName;
std::function<void ()> g_cleanup;
}

void runOnMainThread(std::function<void(void)> fn);
static bool isValidSlot(const QMetaMethod &sl);


//
// Public API
//

// Initializes the test case with a test object and cleanup function. The
// cleanup function is called when all test functions have completed.
void initTestCase(QObject *testObject, std::function<void ()> cleanup)
{
    g_testObject = testObject;
    g_cleanup = cleanup;
}

void verify(bool condition, std::string_view conditionString, std::string_view file, int line)
{
    if (!condition) {
        completeTestFunction(
            TestResult::Fail,
            formatMessage(file, line, "Condition failed: " + std::string(conditionString)));
    }
}

// Completes the currently running test function with a result. This function is
// thread-safe and call be called from any thread.
void completeTestFunction(TestResult result, std::string message)
{
    auto resultString = [](TestResult result) {
        switch (result) {
        case TestResult::Pass:
            return "PASS";
        break;
        case TestResult::Fail:
            return "FAIL";
        break;
        case TestResult::Skip:
            return "SKIP";
        break;
        }
    };

    // Report test result to JavaScript test runner, on the main thread
    runOnMainThread([resultString = resultString(result), message](){
        EM_ASM({
            completeTestFunction(UTF8ToString($0), UTF8ToString($1), UTF8ToString($2));
        }, g_currentTestName.c_str(), resultString, message.c_str());
    });
}

// Completes the currently running test function with a Pass result.
void completeTestFunction()
{
    completeTestFunction(TestResult::Pass, std::string());
}

//
// Private API for the Javascript test runnner
//

std::string formatMessage(std::string_view file, int line, std::string_view message)
{
    return "[" + std::string(file) + ":" + QString::number(line).toStdString() + "] " + std::string(message);
}

void cleanupTestCase()
{
    g_testObject = nullptr;
    g_cleanup();
}

std::string getTestFunctions()
{
    std::string testFunctions;

    // Duplicate qPrintTestSlots (private QTestLib function) logic.
    for (int i = 0; i < g_testObject->metaObject()->methodCount(); ++i) {
        QMetaMethod sl = g_testObject->metaObject()->method(i);
        if (!isValidSlot(sl))
            continue;
        QByteArray signature = sl.methodSignature();
        Q_ASSERT(signature.endsWith("()"));
        signature.chop(2);
        if (!testFunctions.empty())
            testFunctions += " ";
        testFunctions += std::string(signature.constData());
    }

    return testFunctions;
}

void runTestFunction(std::string name)
{
    g_currentTestName = name;
    QMetaObject::invokeMethod(g_testObject, name.c_str());
}

void failTest(std::string message)
{
    completeTestFunction(QtWasmTest::Fail, std::move(message));
}

void passTest()
{
    completeTestFunction(QtWasmTest::Pass, "");
}

EMSCRIPTEN_BINDINGS(qtwebtestrunner) {
    emscripten::function("cleanupTestCase", &cleanupTestCase);
    emscripten::function("getTestFunctions", &getTestFunctions);
    emscripten::function("runTestFunction", &runTestFunction);
    emscripten::function("qtWasmFail", &failTest);
    emscripten::function("qtWasmPass", &passTest);
}

//
// Test lib implementation
//

static bool isValidSlot(const QMetaMethod &sl)
{
    if (sl.access() != QMetaMethod::Private || sl.parameterCount() != 0
        || sl.returnType() != QMetaType::Void || sl.methodType() != QMetaMethod::Slot)
        return false;
    const QByteArray name = sl.name();
    return !(name.isEmpty() || name.endsWith("_data")
        || name == "initTestCase" || name == "cleanupTestCase"
        || name == "init" || name == "cleanup");
}

void trampoline(void *context)
{
    Q_ASSERT(emscripten_is_main_runtime_thread());

    emscripten_async_call([](void *context) {
        std::function<void(void)> *fn = reinterpret_cast<std::function<void(void)> *>(context);
        (*fn)();
        delete fn;
    }, context, 0);
}

// Runs the given function on the main thread, asynchronously
void runOnMainThread(std::function<void(void)> fn)
{
    void *context = new std::function<void(void)>(fn);
    if (emscripten_is_main_runtime_thread()) {
        trampoline(context);
    } else {
#if QT_CONFIG(thread)
        emscripten_async_run_in_main_runtime_thread_(EM_FUNC_SIG_VI, reinterpret_cast<void *>(trampoline), context);
#endif
    }
}

} // namespace QtWasmTest

