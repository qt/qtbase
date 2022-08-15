// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QT_WASM_TESTRUNNER_H
#define QT_WASM_TESTRUNNER_H

#include <QtCore/qobject.h>

#include <functional>

namespace QtWasmTest {

enum TestResult {
    Pass,
    Fail,
    Skip,
};

std::string formatMessage(std::string_view file,
                          int line,
                          std::string_view message);

void completeTestFunction(TestResult result, std::string message);
void completeTestFunction();
void initTestCase(QObject *testObject, std::function<void ()> cleanup);
template <typename App>
void initTestCase(int argc,
                  char **argv,
                std::shared_ptr<QObject> testObject)
{
    auto app = std::make_shared<App>(argc, argv);
    auto cleanup = [testObject, app]() mutable {
        // C++ lambda capture destruction order is unspecified;
        // delete test before app by calling reset().
        testObject.reset();
        app.reset();
    };
    initTestCase(testObject.get(), cleanup);
}
void verify(bool condition,
            std::string_view conditionString,
            std::string_view file,
            int line);

template<class L, class R>
void compare(const L& lhs,
             const R& rhs,
             std::string_view lhsString,
             std::string_view rhsString,
             std::string_view file,
             int line) {
    if (lhs != rhs) {
        completeTestFunction(
            TestResult::Fail,
            formatMessage(file, line, "Comparison failed: " + std::string(lhsString) + " == " + std::string(rhsString)));
    }
}

}  // namespace QtWasmTest

#define QWASMVERIFY(condition) \
    QtWasmTest::verify((condition), #condition, __FILE__, __LINE__);

#define QWASMCOMPARE(left, right) \
    QtWasmTest::compare((left), (right), #left, #right, __FILE__, __LINE__);

#define QWASMSUCCESS() \
    QtWasmTest::completeTestFunction(QtWasmTest::Pass, "")

#define QWASMFAIL(message) \
    QtWasmTest::completeTestFunction(QtWasmTest::Fail, QtWasmTest::formatMessage(__FILE__, __LINE__, message))

#endif
