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
};
void completeTestFunction(TestResult result = TestResult::Pass);
void initTestCase(QObject *testObject, std::function<void ()> cleanup);
template <typename App>
void initTestCase(int argc, char **argv, std::shared_ptr<QObject> testObject)
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

}

#endif

