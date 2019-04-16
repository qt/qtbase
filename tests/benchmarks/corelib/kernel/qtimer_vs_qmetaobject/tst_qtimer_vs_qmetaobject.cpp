/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <QtTest/QtTest>
#include <QThread>

#define INVOKE_COUNT 10000

class qtimer_vs_qmetaobject : public QObject
{
    Q_OBJECT
private slots:
    void bench();
    void bench_data();
    void benchBackgroundThread();
    void benchBackgroundThread_data() { bench_data(); }
};

class InvokeCounter : public QObject {
    Q_OBJECT
public:
    InvokeCounter() : count(0) { };
public slots:
    void invokeSlot() {
        count++;
        if (count == INVOKE_COUNT)
            emit allInvoked();
    }
signals:
    void allInvoked();
protected:
    int count;
};

void qtimer_vs_qmetaobject::bench()
{
    QFETCH(int, type);

    std::function<void(InvokeCounter*)> invoke;
    if (type == 0) {
        invoke = [](InvokeCounter* invokeCounter) {
            QTimer::singleShot(0, invokeCounter, SLOT(invokeSlot()));
        };
    } else if (type == 1) {
        invoke = [](InvokeCounter* invokeCounter) {
            QTimer::singleShot(0, invokeCounter, &InvokeCounter::invokeSlot);
        };
    } else if (type == 2) {
        invoke = [](InvokeCounter* invokeCounter) {
            QTimer::singleShot(0, invokeCounter, [invokeCounter]() {
                invokeCounter->invokeSlot();
            });
        };
    } else if (type == 3) {
        invoke = [](InvokeCounter* invokeCounter) {
            QTimer::singleShot(0, [invokeCounter]() {
                invokeCounter->invokeSlot();
            });
        };
    } else if (type == 4) {
        invoke = [](InvokeCounter* invokeCounter) {
            QMetaObject::invokeMethod(invokeCounter, "invokeSlot", Qt::QueuedConnection);
        };
    } else if (type == 5) {
        invoke = [](InvokeCounter* invokeCounter) {
            QMetaObject::invokeMethod(invokeCounter, &InvokeCounter::invokeSlot, Qt::QueuedConnection);
        };
    } else if (type == 6) {
        invoke = [](InvokeCounter* invokeCounter) {
            QMetaObject::invokeMethod(invokeCounter, [invokeCounter]() {
                invokeCounter->invokeSlot();
            }, Qt::QueuedConnection);
        };
    } else {
        QFAIL("unhandled data tag");
    }

    QBENCHMARK {
        InvokeCounter invokeCounter;
        QSignalSpy spy(&invokeCounter, &InvokeCounter::allInvoked);
        for(int i = 0; i < INVOKE_COUNT; ++i) {
            invoke(&invokeCounter);
        }
        QVERIFY(spy.wait(10000));
    }
}

void qtimer_vs_qmetaobject::bench_data()
{
    QTest::addColumn<int>("type");
    QTest::addRow("singleShot_slot") << 0;
    QTest::addRow("singleShot_pmf") << 1;
    QTest::addRow("singleShot_functor") << 2;
    QTest::addRow("singleShot_functor_noctx") << 3;
    QTest::addRow("invokeMethod_string") << 4;
    QTest::addRow("invokeMethod_pmf") << 5;
    QTest::addRow("invokeMethod_functor") << 6;
}

void qtimer_vs_qmetaobject::benchBackgroundThread()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires QThread::create");
#else
    QScopedPointer<QThread> thread(QThread::create([this]() { bench(); }));
    thread->start();
    QVERIFY(thread->wait());
#endif
}

QTEST_MAIN(qtimer_vs_qmetaobject)

#include "tst_qtimer_vs_qmetaobject.moc"
