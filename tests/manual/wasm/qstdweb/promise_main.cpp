// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/private/qstdweb_p.h>

#include <qtwasmtestlib.h>
#include <emscripten.h>

using namespace emscripten;

class WasmPromiseTest : public QObject
{
    Q_OBJECT

public:
    WasmPromiseTest() : m_window(val::global("window")), m_testSupport(val::object()) {}

    ~WasmPromiseTest() noexcept = default;

private:
    void init() {
        m_testSupport = val::object();
        m_window.set("testSupport", m_testSupport);

        EM_ASM({
            testSupport.resolve = {};
            testSupport.reject = {};
            testSupport.promises = {};
            testSupport.waitConditionPromise = new Promise((resolve, reject) => {
                testSupport.finishWaiting = resolve;
            });

            testSupport.makeTestPromise = (param) => {
                testSupport.promises[param] = new Promise((resolve, reject) => {
                    testSupport.resolve[param] = resolve;
                    testSupport.reject[param] = reject;
                });

                return testSupport.promises[param];
            };
        });
    }

    val m_window;
    val m_testSupport;

private slots:
    void simpleResolve();
    void multipleResolve();
    void simpleReject();
    void multipleReject();
    void throwInThen();
    void bareFinally();
    void finallyWithThen();
    void finallyWithThrow();
    void finallyWithThrowInThen();
    void nested();
    void all();
    void allWithThrow();
    void allWithFinally();
    void allWithFinallyAndThrow();
};

class BarrierCallback {
public:
    BarrierCallback(int number, std::function<void()> onDone)
        : m_remaining(number), m_onDone(std::move(onDone)) {}

    void operator()() {
        if (!--m_remaining) {
            m_onDone();
        }
    }

private:
    int m_remaining;
    std::function<void()> m_onDone;
};

// Post event to the main thread and verify that it is processed.
void WasmPromiseTest::simpleResolve()
{
    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            QWASMVERIFY(result.isString());
            QWASMCOMPARE("Some lovely data", result.as<std::string>());

            QWASMSUCCESS();
        },
        .catchFunc = [](val error) {
            Q_UNUSED(error);

            QWASMFAIL("Unexpected catch");
        }
    }, std::string("simpleResolve"));

    EM_ASM({
        testSupport.resolve["simpleResolve"]("Some lovely data");
    });
}

void WasmPromiseTest::multipleResolve()
{
    init();

    static constexpr int promiseCount = 1000;

    auto onThen = std::make_shared<BarrierCallback>(promiseCount, []() {
        QWASMSUCCESS();
    });

    for (int i = 0; i < promiseCount; ++i) {
        qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
            .thenFunc = [=](val result) {
                QWASMVERIFY(result.isString());
                QWASMCOMPARE(QString::number(i).toStdString(), result.as<std::string>());

                (*onThen)();
            },
            .catchFunc = [](val error) {
                Q_UNUSED(error);
                QWASMFAIL("Unexpected catch");
            }
        }, (QStringLiteral("test") + QString::number(i)).toStdString());
    }

    EM_ASM({
        for (let i = $0 - 1; i >= 0; --i) {
            testSupport.resolve['test' + i](`${i}`);
        }
    }, promiseCount);
}

void WasmPromiseTest::simpleReject()
{
    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            QWASMFAIL("Unexpected then");
        },
        .catchFunc = [](val result) {
            QWASMVERIFY(result.isString());
            QWASMCOMPARE("Evil error", result.as<std::string>());
            QWASMSUCCESS();
        }
    }, std::string("simpleReject"));

    EM_ASM({
        testSupport.reject["simpleReject"]("Evil error");
    });
}

void WasmPromiseTest::multipleReject()
{
    static constexpr int promiseCount = 1000;

    auto onCatch = std::make_shared<BarrierCallback>(promiseCount, []() {
        QWASMSUCCESS();
    });

    for (int i = 0; i < promiseCount; ++i) {
        qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
            .thenFunc = [=](val result) {
                QWASMVERIFY(result.isString());
                QWASMCOMPARE(QString::number(i).toStdString(), result.as<std::string>());

                (*onCatch)();
            },
            .catchFunc = [](val error) {
                Q_UNUSED(error);
                QWASMFAIL("Unexpected catch");
            }
        }, (QStringLiteral("test") + QString::number(i)).toStdString());
    }

    EM_ASM({
        for (let i = $0 - 1; i >= 0; --i) {
            testSupport.resolve['test' + i](`${i}`);
        }
    }, promiseCount);
}

void WasmPromiseTest::throwInThen()
{
    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            EM_ASM({
                throw "Expected error";
            });
        },
        .catchFunc = [](val error) {
            QWASMCOMPARE("Expected error", error.as<std::string>());
            QWASMSUCCESS();
        }
    }, std::string("throwInThen"));

    EM_ASM({
        testSupport.resolve["throwInThen"]();
    });
}

void WasmPromiseTest::bareFinally()
{
    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .finallyFunc = []() {
            QWASMSUCCESS();
        }
    }, std::string("bareFinally"));

    EM_ASM({
        testSupport.resolve["bareFinally"]();
    });
}

void WasmPromiseTest::finallyWithThen()
{
    init();

    auto thenCalled = std::make_shared<bool>();
    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [thenCalled] (val result) {
            Q_UNUSED(result);
            *thenCalled = true;
        },
        .finallyFunc = [thenCalled]() {
            QWASMVERIFY(*thenCalled);
            QWASMSUCCESS();
        }
    }, std::string("finallyWithThen"));

    EM_ASM({
        testSupport.resolve["finallyWithThen"]();
    });
}

void WasmPromiseTest::finallyWithThrow()
{
    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .catchFunc = [](val error) {
            Q_UNUSED(error);
        },
        .finallyFunc = []() {
            QWASMSUCCESS();
        }
    }, std::string("finallyWithThrow"));

    EM_ASM({
        testSupport.reject["finallyWithThrow"]();
    });
}

void WasmPromiseTest::finallyWithThrowInThen()
{
    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            EM_ASM({
                throw "Expected error";
            });
        },
        .catchFunc = [](val result) {
            QWASMVERIFY(result.isString());
            QWASMCOMPARE("Expected error", result.as<std::string>());
        },
        .finallyFunc = []() {
            QWASMSUCCESS();
        }
    }, std::string("bareFinallyWithThen"));

    EM_ASM({
        testSupport.resolve["bareFinallyWithThen"]();
    });
}

void WasmPromiseTest::nested()
{
    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [this](val result) {
            QWASMVERIFY(result.isString());
            QWASMCOMPARE("Outer data", result.as<std::string>());

            qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
                .thenFunc = [this](val innerResult) {
                    QWASMVERIFY(innerResult.isString());
                    QWASMCOMPARE("Inner data", innerResult.as<std::string>());

                    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
                        .thenFunc = [](val innerResult) {
                            QWASMVERIFY(innerResult.isString());
                            QWASMCOMPARE("Innermost data", innerResult.as<std::string>());

                            QWASMSUCCESS();
                        },
                        .catchFunc = [](val error) {
                            Q_UNUSED(error);
                            QWASMFAIL("Unexpected catch");
                        }
                    }, std::string("innermost"));

                    EM_ASM({
                        testSupport.resolve["innermost"]("Innermost data");
                    });
                },
                .catchFunc = [](val error) {
                    Q_UNUSED(error);
                    QWASMFAIL("Unexpected catch");
                }
            }, std::string("inner"));

            EM_ASM({
                testSupport.resolve["inner"]("Inner data");
            });
        },
        .catchFunc = [](val error) {
            Q_UNUSED(error);
            QWASMFAIL("Unexpected catch");
        }
    }, std::string("outer"));

    EM_ASM({
        testSupport.resolve["outer"]("Outer data");
    });
}

void WasmPromiseTest::all()
{
    init();

    static constexpr int promiseCount = 1000;
    auto thenCalledOnce = std::shared_ptr<bool>();
    *thenCalledOnce = true;

    std::vector<val> promises;
    promises.reserve(promiseCount);

    for (int i = 0; i < promiseCount; ++i) {
        promises.push_back(m_testSupport.call<val>("makeTestPromise", val(("all" + QString::number(i)).toStdString())));
    }

    qstdweb::Promise::all(std::move(promises), {
        .thenFunc = [=](val result) {
            QWASMVERIFY(*thenCalledOnce);
            *thenCalledOnce = false;

            QWASMVERIFY(result.isArray());
            QWASMCOMPARE(promiseCount, result["length"].as<int>());
            for (int i = 0; i < promiseCount; ++i) {
                QWASMCOMPARE(QStringLiteral("Data %1").arg(i).toStdString(), result[i].as<std::string>());
            }

            QWASMSUCCESS();
        },
        .catchFunc = [](val error) {
            Q_UNUSED(error);
            QWASMFAIL("Unexpected catch");
        }
    });

    EM_ASM({
        console.log('Resolving');
        for (let i = $0 - 1; i >= 0; --i) {
            testSupport.resolve['all' + i](`Data ${i}`);
        }
    }, promiseCount);
}

void WasmPromiseTest::allWithThrow()
{
    init();

    val promise1 = m_testSupport.call<val>("makeTestPromise", val("promise1"));
    val promise2 = m_testSupport.call<val>("makeTestPromise", val("promise2"));
    val promise3 = m_testSupport.call<val>("makeTestPromise", val("promise3"));

    auto catchCalledOnce = std::shared_ptr<bool>();
    *catchCalledOnce = true;

    qstdweb::Promise::all({promise1, promise2, promise3}, {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            QWASMFAIL("Unexpected then");
        },
        .catchFunc = [catchCalledOnce](val result) {
            QWASMVERIFY(*catchCalledOnce);
            *catchCalledOnce = false;
            QWASMVERIFY(result.isString());
            QWASMCOMPARE("Error 2", result.as<std::string>());
            QWASMSUCCESS();
        }
    });

    EM_ASM({
        testSupport.resolve["promise3"]("Data 3");
        testSupport.resolve["promise1"]("Data 1");
        testSupport.reject["promise2"]("Error 2");
    });
}

void WasmPromiseTest::allWithFinally()
{
    init();

    val promise1 = m_testSupport.call<val>("makeTestPromise", val("promise1"));
    val promise2 = m_testSupport.call<val>("makeTestPromise", val("promise2"));
    val promise3 = m_testSupport.call<val>("makeTestPromise", val("promise3"));

    auto finallyCalledOnce = std::shared_ptr<bool>();
    *finallyCalledOnce = true;

    qstdweb::Promise::all({promise1, promise2, promise3}, {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
        },
        .finallyFunc = [finallyCalledOnce]() {
            QWASMVERIFY(*finallyCalledOnce);
            *finallyCalledOnce = false;
            QWASMSUCCESS();
        }
    });

    EM_ASM({
        testSupport.resolve["promise3"]("Data 3");
        testSupport.resolve["promise1"]("Data 1");
        testSupport.resolve["promise2"]("Data 2");
    });
}

void WasmPromiseTest::allWithFinallyAndThrow()
{
    init();

    val promise1 = m_testSupport.call<val>("makeTestPromise", val("promise1"));
    val promise2 = m_testSupport.call<val>("makeTestPromise", val("promise2"));
    val promise3 = m_testSupport.call<val>("makeTestPromise", val("promise3"));

    auto finallyCalledOnce = std::shared_ptr<bool>();
    *finallyCalledOnce = true;

    qstdweb::Promise::all({promise1, promise2, promise3}, {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            EM_ASM({
                throw "This breaks it all!!!";
            });
        },
        .finallyFunc = [finallyCalledOnce]() {
            QWASMVERIFY(*finallyCalledOnce);
            *finallyCalledOnce = false;
            QWASMSUCCESS();
        }
    });

    EM_ASM({
        testSupport.resolve["promise3"]("Data 3");
        testSupport.resolve["promise1"]("Data 1");
        testSupport.resolve["promise2"]("Data 2");
    });
}

int main(int argc, char **argv)
{
    auto testObject = std::make_shared<WasmPromiseTest>();
    QtWasmTest::initTestCase<QCoreApplication>(argc, argv, testObject);
    return 0;
}

#include "promise_main.moc"
