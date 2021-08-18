/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
