// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qtimer.h>
#include <QtCore/private/qstdweb_p.h>
#include <QTest>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#if defined(QT_HAVE_EMSCRIPTEN_ASYNCIFY)
#define SKIP_IF_NO_ASYNCIFY()
#else
#define SKIP_IF_NO_ASYNCIFY() QSKIP("Needs QT_HAVE_EMSCRIPTEN_ASYNCIFY")
#endif

using namespace emscripten;

class tst_QStdWeb  : public QObject
{
    Q_OBJECT
public:
    tst_QStdWeb() : m_window(val::global("window")), m_testSupport(val::object()) {
        instance = this;

        m_window.set("testSupport", m_testSupport);
    }
    ~tst_QStdWeb() noexcept {}

private:
    static tst_QStdWeb* instance;

    void init() {
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

private Q_SLOTS:
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

tst_QStdWeb* tst_QStdWeb::instance = nullptr;

EM_ASYNC_JS(void, awaitCondition, (), {
    await testSupport.waitConditionPromise;
});

void tst_QStdWeb::simpleResolve()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            QVERIFY(result.isString());
            QCOMPARE("Some lovely data", result.as<std::string>());
            EM_ASM({
                testSupport.finishWaiting();
            });
        },
        .catchFunc = [](val error) {
            Q_UNUSED(error);
            QFAIL("Unexpected catch");
        }
    }, std::string("simpleResolve"));

    EM_ASM({
        testSupport.resolve["simpleResolve"]("Some lovely data");
    });

    awaitCondition();
}

void tst_QStdWeb::multipleResolve()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            QVERIFY(result.isString());
            QCOMPARE("Data 1", result.as<std::string>());

            EM_ASM({
                if (!--testSupport.promisesLeft) {
                    testSupport.finishWaiting();
                }
            });
        },
        .catchFunc = [](val error) {
            Q_UNUSED(error);
            QFAIL("Unexpected catch");
        }
    }, std::string("1"));
    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            QVERIFY(result.isString());
            QCOMPARE("Data 2", result.as<std::string>());

            EM_ASM({
                if (!--testSupport.promisesLeft) {
                    testSupport.finishWaiting();
                }
            });
        },
        .catchFunc = [](val error) {
            Q_UNUSED(error);
            QFAIL("Unexpected catch");
        }
    }, std::string("2"));
    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            QVERIFY(result.isString());
            QCOMPARE("Data 3", result.as<std::string>());

            EM_ASM({
                if (!--testSupport.promisesLeft) {
                    testSupport.finishWaiting();
                }
            });
        },
        .catchFunc = [](val error) {
            Q_UNUSED(error);
            QFAIL("Unexpected catch");
        }
    }, std::string("3"));

    EM_ASM({
        testSupport.resolve["3"]("Data 3");
        testSupport.resolve["1"]("Data 1");
        testSupport.resolve["2"]("Data 2");
    });

    awaitCondition();
}

void tst_QStdWeb::simpleReject()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            QFAIL("Unexpected then");
        },
        .catchFunc = [](val result) {
            QVERIFY(result.isString());
            QCOMPARE("Evil error", result.as<std::string>());
            EM_ASM({
                testSupport.finishWaiting();
            });
        }
    }, std::string("simpleReject"));

    EM_ASM({
        testSupport.reject["simpleReject"]("Evil error");
    });

    awaitCondition();
}

void tst_QStdWeb::multipleReject()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            QFAIL("Unexpected then");
        },
        .catchFunc = [](val error) {
            QVERIFY(error.isString());
            QCOMPARE("Error 1", error.as<std::string>());

            EM_ASM({
                if (!--testSupport.promisesLeft) {
                    testSupport.finishWaiting();
                }
            });
        }
    }, std::string("1"));
    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            QFAIL("Unexpected then");
        },
        .catchFunc = [](val error) {
            QVERIFY(error.isString());
            QCOMPARE("Error 2", error.as<std::string>());

            EM_ASM({
                if (!--testSupport.promisesLeft) {
                    testSupport.finishWaiting();
                }
            });
        }
    }, std::string("2"));
    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            QFAIL("Unexpected then");
        },
        .catchFunc = [](val error) {
            QVERIFY(error.isString());
            QCOMPARE("Error 3", error.as<std::string>());

            EM_ASM({
                if (!--testSupport.promisesLeft) {
                    testSupport.finishWaiting();
                }
            });
        }
    }, std::string("3"));

    EM_ASM({
        testSupport.reject["3"]("Error 3");
        testSupport.reject["1"]("Error 1");
        testSupport.reject["2"]("Error 2");
    });

    awaitCondition();
}

void tst_QStdWeb::throwInThen()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            EM_ASM({
                throw "Expected error";
            });
        },
        .catchFunc = [](val error) {
            QCOMPARE("Expected error", error.as<std::string>());
            EM_ASM({
                testSupport.finishWaiting();
            });
        }
    }, std::string("throwInThen"));

    EM_ASM({
        testSupport.resolve["throwInThen"]();
    });

    awaitCondition();
}

void tst_QStdWeb::bareFinally()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .finallyFunc = []() {
            EM_ASM({
                testSupport.finishWaiting();
            });
        }
    }, std::string("bareFinally"));

    EM_ASM({
        testSupport.resolve["bareFinally"]();
    });

    awaitCondition();
}

void tst_QStdWeb::finallyWithThen()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [] (val result) {
            Q_UNUSED(result);
        },
        .finallyFunc = []() {
            EM_ASM({
                testSupport.finishWaiting();
            });
        }
    }, std::string("finallyWithThen"));

    EM_ASM({
        testSupport.resolve["finallyWithThen"]();
    });

    awaitCondition();
}

void tst_QStdWeb::finallyWithThrow()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .catchFunc = [](val error) {
            Q_UNUSED(error);
        },
        .finallyFunc = []() {
            EM_ASM({
                testSupport.finishWaiting();
            });
        }
    }, std::string("finallyWithThrow"));

    EM_ASM({
        testSupport.reject["finallyWithThrow"]();
    });

    awaitCondition();
}

void tst_QStdWeb::finallyWithThrowInThen()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            EM_ASM({
                throw "Expected error";
            });
        },
        .catchFunc = [](val result) {
            QVERIFY(result.isString());
            QCOMPARE("Expected error", result.as<std::string>());
        },
        .finallyFunc = []() {
            EM_ASM({
                testSupport.finishWaiting();
            });
        }
    }, std::string("bareFinallyWithThen"));

    EM_ASM({
        testSupport.resolve["bareFinallyWithThen"]();
    });

    awaitCondition();
}

void tst_QStdWeb::nested()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
        .thenFunc = [this](val result) {
            QVERIFY(result.isString());
            QCOMPARE("Outer data", result.as<std::string>());

            qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
                .thenFunc = [this](val innerResult) {
                    QVERIFY(innerResult.isString());
                    QCOMPARE("Inner data", innerResult.as<std::string>());

                    qstdweb::Promise::make(m_testSupport, "makeTestPromise", {
                        .thenFunc = [](val innerResult) {
                            QVERIFY(innerResult.isString());
                            QCOMPARE("Innermost data", innerResult.as<std::string>());

                            EM_ASM({
                                testSupport.finishWaiting();
                            });
                        },
                        .catchFunc = [](val error) {
                            Q_UNUSED(error);
                            QFAIL("Unexpected catch");
                        }
                    }, std::string("innermost"));

                    EM_ASM({
                        testSupport.finishWaiting();
                    });
                },
                .catchFunc = [](val error) {
                    Q_UNUSED(error);
                    QFAIL("Unexpected catch");
                }
            }, std::string("inner"));

            EM_ASM({
                testSupport.finishWaiting();
            });
        },
        .catchFunc = [](val error) {
            Q_UNUSED(error);
            QFAIL("Unexpected catch");
        }
    }, std::string("outer"));

    EM_ASM({
        testSupport.resolve["outer"]("Outer data");
    });

    awaitCondition();

    EM_ASM({
        testSupport.waitConditionPromise = new Promise((resolve, reject) => {
            testSupport.finishWaiting = resolve;
        });
    });

    EM_ASM({
        testSupport.resolve["inner"]("Inner data");
    });

    awaitCondition();

    EM_ASM({
        testSupport.waitConditionPromise = new Promise((resolve, reject) => {
            testSupport.finishWaiting = resolve;
        });
    });

    EM_ASM({
        testSupport.resolve["innermost"]("Innermost data");
    });

    awaitCondition();
}

void tst_QStdWeb::all()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    val promise1 = m_testSupport.call<val>("makeTestPromise", val("promise1"));
    val promise2 = m_testSupport.call<val>("makeTestPromise", val("promise2"));
    val promise3 = m_testSupport.call<val>("makeTestPromise", val("promise3"));

    auto thenCalledOnce = std::shared_ptr<bool>();
    *thenCalledOnce = true;

    qstdweb::Promise::all({promise1, promise2, promise3}, {
        .thenFunc = [thenCalledOnce](val result) {
            QVERIFY(*thenCalledOnce);
            *thenCalledOnce = false;

            QVERIFY(result.isArray());
            QCOMPARE(3, result["length"].as<int>());
            QCOMPARE("Data 1", result[0].as<std::string>());
            QCOMPARE("Data 2", result[1].as<std::string>());
            QCOMPARE("Data 3", result[2].as<std::string>());

            EM_ASM({
                testSupport.finishWaiting();
            });
        },
        .catchFunc = [](val result) {
            Q_UNUSED(result);
            EM_ASM({
                throw new Error("Unexpected error");
            });
        }
    });

    EM_ASM({
        testSupport.resolve["promise3"]("Data 3");
        testSupport.resolve["promise1"]("Data 1");
        testSupport.resolve["promise2"]("Data 2");
    });

    awaitCondition();
}

void tst_QStdWeb::allWithThrow()
{
    SKIP_IF_NO_ASYNCIFY();

    init();

    val promise1 = m_testSupport.call<val>("makeTestPromise", val("promise1"));
    val promise2 = m_testSupport.call<val>("makeTestPromise", val("promise2"));
    val promise3 = m_testSupport.call<val>("makeTestPromise", val("promise3"));

    auto catchCalledOnce = std::shared_ptr<bool>();
    *catchCalledOnce = true;

    qstdweb::Promise::all({promise1, promise2, promise3}, {
        .thenFunc = [](val result) {
            Q_UNUSED(result);
            QFAIL("Unexpected then");
        },
        .catchFunc = [catchCalledOnce](val result) {
            QVERIFY(*catchCalledOnce);
            *catchCalledOnce = false;
            QVERIFY(result.isString());
            QCOMPARE("Error 2", result.as<std::string>());
            EM_ASM({
                testSupport.finishWaiting();
            });
        }
    });

    EM_ASM({
        testSupport.resolve["promise3"]("Data 3");
        testSupport.resolve["promise1"]("Data 1");
        testSupport.reject["promise2"]("Error 2");
    });

    awaitCondition();
}

void tst_QStdWeb::allWithFinally()
{
    SKIP_IF_NO_ASYNCIFY();

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
            QVERIFY(*finallyCalledOnce);
            *finallyCalledOnce = false;
            EM_ASM({
                testSupport.finishWaiting();
            });
        }
    });

    EM_ASM({
        testSupport.resolve["promise3"]("Data 3");
        testSupport.resolve["promise1"]("Data 1");
        testSupport.resolve["promise2"]("Data 2");
    });

    awaitCondition();
}

void tst_QStdWeb::allWithFinallyAndThrow()
{
    SKIP_IF_NO_ASYNCIFY();

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
            QVERIFY(*finallyCalledOnce);
            *finallyCalledOnce = false;
            EM_ASM({
                testSupport.finishWaiting();
            });
        }
    });

    EM_ASM({
        testSupport.resolve["promise3"]("Data 3");
        testSupport.resolve["promise1"]("Data 1");
        testSupport.resolve["promise2"]("Data 2");
    });

    awaitCondition();
}

QTEST_APPLESS_MAIN(tst_QStdWeb)
#include "tst_qstdweb.moc"
