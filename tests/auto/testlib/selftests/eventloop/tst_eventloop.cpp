// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QTestEventLoop>
#include <QtCore/QTimer>

// Tests for QTestEventLoop (and some QTRY_* details)
class tst_EventLoop: public QObject
{
Q_OBJECT

    bool m_inTestFunction = false;
private slots:
    void cleanup();
    void fail();
    void skip();
    void pass();
};

class DeferredFlag
{
    bool m_flag;
public:
    explicit DeferredFlag(bool initial = false) : m_flag(initial)
    {
        if (!initial)
            QTimer::singleShot(50, [this] { m_flag = true; });
    }
    explicit operator bool() const { return m_flag; }
    bool operator!() const { return !m_flag; }
    friend bool operator==(DeferredFlag a, DeferredFlag b) { return bool(a) == bool(b); }
};

char *toString(DeferredFlag val)
{
    return qstrdup(bool(val) ? "DeferredFlag(true)" : "DeferredFlag(false)");
}

void tst_EventLoop::cleanup()
{
    // QTBUG-104441: looping didn't happen in cleanup() if test failed or skipped.
    {
        DeferredFlag flag;
        auto &loop = QTestEventLoop::instance();
        loop.enterLoopMSecs(100);
        QVERIFY2(loop.timeout(), "QTestEventLoop exited prematurely in cleanup()");
        QVERIFY(flag);
    }
    {
        DeferredFlag flag;
        QTRY_VERIFY2(flag, "QTRY_* loop exited prematurely in cleanup()");
    }

    m_inTestFunction = false;
}

void tst_EventLoop::fail()
{
    QVERIFY2(!std::exchange(m_inTestFunction, true), "Earlier test failed to clean up");
    QFAIL("Failing test should still clean up");
}

void tst_EventLoop::skip()
{
    QVERIFY2(!std::exchange(m_inTestFunction, true), "Earlier test failed to clean up");
    QSKIP("Skipping test should still clean up");
}

void tst_EventLoop::pass()
{
    QVERIFY2(!std::exchange(m_inTestFunction, true), "Earlier test failed to clean up");
    {
        DeferredFlag flag;
        auto &loop = QTestEventLoop::instance();
        loop.enterLoopMSecs(100);
        QVERIFY(loop.timeout());
        QVERIFY(flag);
    }
    {
        DeferredFlag flag;
        QTRY_VERIFY(flag);
    }
    DeferredFlag flag;
    QTestEventLoop loop(this);
    QVERIFY(!flag);
    loop.enterLoopMSecs(1);
    QVERIFY(loop.timeout());
    QVERIFY(!flag);
    loop.enterLoopMSecs(100);
    QVERIFY(loop.timeout());
    QVERIFY(flag);
}

QTEST_MAIN(tst_EventLoop)
#include "tst_eventloop.moc"
