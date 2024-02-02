// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QScopedPointer>
#include <QProperty>

#include <qtest.h>

#include "propertytester.h"

class tst_QProperty : public QObject
{
    Q_OBJECT
private slots:
    void cppOldBinding();
    void cppOldBindingReadOnce();
    void cppOldBindingDirect();
    void cppOldBindingDirectReadOnce();

    void cppNewBinding();
    void cppNewBindingReadOnce();
    void cppNewBindingDirect();
    void cppNewBindingDirectReadOnce();

    void cppNotifying();
    void cppNotifyingReadOnce();
    void cppNotifyingDirect();
    void cppNotifyingDirectReadOnce();
};

void tst_QProperty::cppOldBinding()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    auto connection = connect(tester.data(), &PropertyTester::xOldChanged,
                              tester.data(), [&]() { tester->setYOld(tester->xOld()); });

    QCOMPARE(tester->property("yOld").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        tester->setProperty("xOld", ++i);
        if (tester->property("yOld").toInt() != i)
            QFAIL("boo");
    }

    QObject::disconnect(connection);
}

void tst_QProperty::cppOldBindingDirect()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    auto connection = connect(tester.data(), &PropertyTester::xOldChanged,
                              tester.data(), [&]() { tester->setYOld(tester->xOld()); });

    QCOMPARE(tester->yOld(), 0);
    int i = 0;
    QBENCHMARK {
        tester->setXOld(++i);
        if (tester->yOld() != i)
            QFAIL("boo");
    }

    QObject::disconnect(connection);
}

void tst_QProperty::cppOldBindingReadOnce()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    auto connection = connect(tester.data(), &PropertyTester::xOldChanged,
                              tester.data(), [&]() { tester->setYOld(tester->xOld()); });

    QCOMPARE(tester->property("yOld").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        tester->setProperty("xOld", ++i);
    }

    QCOMPARE(tester->property("yOld").toInt(), i);
    QObject::disconnect(connection);
}

void tst_QProperty::cppOldBindingDirectReadOnce()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    auto connection = connect(tester.data(), &PropertyTester::xOldChanged,
                              tester.data(), [&]() { tester->setYOld(tester->xOld()); });

    QCOMPARE(tester->yOld(), 0);
    int i = 0;
    QBENCHMARK {
        tester->setXOld(++i);
    }

    QCOMPARE(tester->yOld(), i);
    QObject::disconnect(connection);
}

void tst_QProperty::cppNewBinding()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    tester->y.setBinding([&](){return tester->x.value();});

    QCOMPARE(tester->property("y").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        tester->setProperty("x", ++i);
        if (tester->property("y").toInt() != i)
            QFAIL("boo");
    }
}

void tst_QProperty::cppNewBindingDirect()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    tester->y.setBinding([&](){return tester->x.value();});
    QCOMPARE(tester->y.value(), 0);
    int i = 0;
    QBENCHMARK {
        tester->x = ++i;
        if (tester->y.value() != i)
            QFAIL("boo");
    }
}

void tst_QProperty::cppNewBindingReadOnce()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    tester->y.setBinding([&](){return tester->x.value();});

    QCOMPARE(tester->property("y").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        tester->setProperty("x", ++i);
    }

    QCOMPARE(tester->property("y").toInt(), i);
}

void tst_QProperty::cppNewBindingDirectReadOnce()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    tester->y.setBinding([&](){return tester->x.value();});
    QCOMPARE(tester->y.value(), 0);
    int i = 0;
    QBENCHMARK {
        tester->x = ++i;
    }

    QCOMPARE(tester->y.value(), i);
}

void tst_QProperty::cppNotifying()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    tester->yNotified.setBinding([&](){return tester->xNotified.value();});

    QCOMPARE(tester->property("yNotified").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        tester->setProperty("xNotified", ++i);
        if (tester->property("yNotified").toInt() != i)
            QFAIL("boo");
    }
}

void tst_QProperty::cppNotifyingDirect()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    tester->yNotified.setBinding([&](){return tester->xNotified.value();});
    QCOMPARE(tester->yNotified.value(), 0);
    int i = 0;
    QBENCHMARK {
        tester->xNotified.setValue(++i);
        if (tester->yNotified.value() != i)
            QFAIL("boo");
    }
}

void tst_QProperty::cppNotifyingReadOnce()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    tester->yNotified.setBinding([&](){return tester->xNotified.value();});

    QCOMPARE(tester->property("yNotified").toInt(), 0);
    int i = 0;
    QBENCHMARK {
        tester->setProperty("xNotified", ++i);
    }

    QCOMPARE(tester->property("yNotified").toInt(), i);
}

void tst_QProperty::cppNotifyingDirectReadOnce()
{
    QScopedPointer<PropertyTester> tester {new PropertyTester};
    tester->yNotified.setBinding([&](){return tester->xNotified.value();});
    QCOMPARE(tester->yNotified.value(), 0);
    int i = 0;
    QBENCHMARK {
        tester->xNotified.setValue(++i);
    }

    QCOMPARE(tester->yNotified.value(), i);
}

QTEST_MAIN(tst_QProperty)

#include "tst_bench_qproperty.moc"
