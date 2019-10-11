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


#include <QtTest/QtTest>


#include <qdatetime.h>

class tst_QSignalSpy : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void sigFoo();

private slots:
    void spyWithoutArgs();
    void spyWithBasicArgs();
    void spyWithPointers();
    void spyWithQtClasses();
    void spyWithBasicQtClasses();
    void spyWithQtTypedefs();

    void wait_signalEmitted();
    void wait_timeout();
    void wait_signalEmittedLater();
    void wait_signalEmittedTooLate();
    void wait_signalEmittedMultipleTimes();

    void spyFunctionPointerWithoutArgs();
    void spyFunctionPointerWithBasicArgs();
    void spyFunctionPointerWithPointers();
    void spyFunctionPointerWithQtClasses();
    void spyFunctionPointerWithBasicQtClasses();
    void spyFunctionPointerWithQtTypedefs();

    void waitFunctionPointer_signalEmitted();
    void waitFunctionPointer_timeout();
    void waitFunctionPointer_signalEmittedLater();
    void waitFunctionPointer_signalEmittedTooLate();
    void waitFunctionPointer_signalEmittedMultipleTimes();

    void spyOnMetaMethod();

    void spyOnMetaMethod_invalid();
    void spyOnMetaMethod_invalid_data();
};

class QtTestObject: public QObject
{
    Q_OBJECT

public:
    QtTestObject();

signals:
    void sig0();
    void sig1(int, int);
    void sigLong(long, long);
    void sig2(int *, int *);

public:
    QString slotResult;
    friend class tst_QSignalSpy;
};

QtTestObject::QtTestObject()
{
}

void tst_QSignalSpy::spyWithoutArgs()
{
    QtTestObject obj;

    QSignalSpy spy(&obj, SIGNAL(sig0()));
    QCOMPARE(spy.count(), 0);

    emit obj.sig0();
    QCOMPARE(spy.count(), 1);
    emit obj.sig0();
    QCOMPARE(spy.count(), 2);

    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.count(), 0);
}

void tst_QSignalSpy::spyWithBasicArgs()
{
    QtTestObject obj;
    QSignalSpy spy(&obj, SIGNAL(sig1(int,int)));

    emit obj.sig1(1, 2);
    QCOMPARE(spy.count(), 1);

    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.count(), 2);
    QCOMPARE(args.at(0).toInt(), 1);
    QCOMPARE(args.at(1).toInt(), 2);

    QSignalSpy spyl(&obj, SIGNAL(sigLong(long,long)));

    emit obj.sigLong(1l, 2l);
    args = spyl.takeFirst();
    QCOMPARE(args.count(), 2);
    QCOMPARE(qvariant_cast<long>(args.at(0)), 1l);
    QCOMPARE(qvariant_cast<long>(args.at(1)), 2l);
}


void tst_QSignalSpy::spyWithPointers()
{
    qRegisterMetaType<int *>("int*");

    QtTestObject obj;
    QSignalSpy spy(&obj, SIGNAL(sig2(int*,int*)));

    int i1 = 1;
    int i2 = 2;

    emit obj.sig2(&i1, &i2);
    QCOMPARE(spy.count(), 1);

    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.count(), 2);
    QCOMPARE(*static_cast<int * const *>(args.at(0).constData()), &i1);
    QCOMPARE(*static_cast<int * const *>(args.at(1).constData()), &i2);
}

class QtTestObject2: public QObject
{
    Q_OBJECT
    friend class tst_QSignalSpy;

signals:
    void sig(QString);
    void sig2(const QDateTime &dt);
    void sig3(QObject *o);
    void sig4(QChar c);
    void sig5(const QVariant &v);
};

void tst_QSignalSpy::spyWithBasicQtClasses()
{
    QtTestObject2 obj;

    QSignalSpy spy(&obj, SIGNAL(sig(QString)));
    emit obj.sig(QString("bubu"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("bubu"));

    QSignalSpy spy2(&obj, SIGNAL(sig5(QVariant)));
    QVariant val(45);
    emit obj.sig5(val);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy2.at(0).count(), 1);
    QCOMPARE(spy2.at(0).at(0), val);
    QCOMPARE(qvariant_cast<QVariant>(spy2.at(0).at(0)), val);
}

void tst_QSignalSpy::spyWithQtClasses()
{
    QtTestObject2 obj;


    QSignalSpy spy(&obj, SIGNAL(sig2(QDateTime)));
    QDateTime dt = QDateTime::currentDateTime();
    emit obj.sig2(dt);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).count(), 1);
    QCOMPARE(spy.at(0).at(0).typeName(), "QDateTime");
    QCOMPARE(*static_cast<const QDateTime *>(spy.at(0).at(0).constData()), dt);
    QCOMPARE(spy.at(0).at(0).toDateTime(), dt);

    QSignalSpy spy2(&obj, SIGNAL(sig3(QObject*)));
    emit obj.sig3(this);
    QCOMPARE(*static_cast<QObject * const *>(spy2.value(0).value(0).constData()),
            (QObject *)this);
    QCOMPARE(qvariant_cast<QObject *>(spy2.value(0).value(0)), (QObject*)this);

    QSignalSpy spy3(&obj, SIGNAL(sig4(QChar)));
    emit obj.sig4(QChar('A'));
    QCOMPARE(qvariant_cast<QChar>(spy3.value(0).value(0)), QChar('A'));
}

class QtTestObject3: public QObject
{
    Q_OBJECT
    friend class tst_QSignalSpy;

signals:
    void sig1(quint16);
    void sig2(qlonglong, qulonglong);
    void sig3(qint64, quint64);
};

void tst_QSignalSpy::spyWithQtTypedefs()
{
    QtTestObject3 obj;

//    QSignalSpy spy1(&obj, SIGNAL(sig1(quint16)));
//    emit obj.sig1(42);
//    QCOMPARE(spy1.value(0).value(0).toInt(), 42);

    QSignalSpy spy2(&obj, SIGNAL(sig2(qlonglong,qulonglong)));
    emit obj.sig2(42, 43);
    QCOMPARE(spy2.value(0).value(0).toInt(), 42);
    QCOMPARE(spy2.value(0).value(1).toInt(), 43);

//    QSignalSpy spy3(&obj, SIGNAL(sig3(qint64,quint64)));
//    emit obj.sig3(44, 45);
//    QCOMPARE(spy3.value(0).value(0).toInt(), 44);
//    QCOMPARE(spy3.value(0).value(1).toInt(), 45);
}

void tst_QSignalSpy::wait_signalEmitted()
{
    QTimer::singleShot(0, this, SIGNAL(sigFoo()));
    QSignalSpy spy(this, SIGNAL(sigFoo()));
    QVERIFY(spy.wait(1));
}

void tst_QSignalSpy::wait_timeout()
{
    QSignalSpy spy(this, SIGNAL(sigFoo()));
    QVERIFY(!spy.wait(1));
}

void tst_QSignalSpy::wait_signalEmittedLater()
{
    QTimer::singleShot(500, this, SIGNAL(sigFoo()));
    QSignalSpy spy(this, SIGNAL(sigFoo()));
    QVERIFY(spy.wait(1000));
}

void tst_QSignalSpy::wait_signalEmittedTooLate()
{
    QTimer::singleShot(500, this, SIGNAL(sigFoo()));
    QSignalSpy spy(this, SIGNAL(sigFoo()));
    QVERIFY(!spy.wait(200));
    QTRY_COMPARE(spy.count(), 1);
}

void tst_QSignalSpy::wait_signalEmittedMultipleTimes()
{
    QTimer::singleShot(100, this, SIGNAL(sigFoo()));
    QTimer::singleShot(800, this, SIGNAL(sigFoo()));
    QSignalSpy spy(this, SIGNAL(sigFoo()));
    QVERIFY(spy.wait());
    QCOMPARE(spy.count(), 1); // we don't wait for the second signal...
    QVERIFY(spy.wait());
    QCOMPARE(spy.count(), 2);
    QVERIFY(!spy.wait(1));
    QTimer::singleShot(10, this, SIGNAL(sigFoo()));
    QVERIFY(spy.wait());
    QCOMPARE(spy.count(), 3);
}

void tst_QSignalSpy::spyFunctionPointerWithoutArgs()
{
    QtTestObject obj;

    QSignalSpy spy(&obj, &QtTestObject::sig0);
    QCOMPARE(spy.count(), 0);

    emit obj.sig0();
    QCOMPARE(spy.count(), 1);
    emit obj.sig0();
    QCOMPARE(spy.count(), 2);

    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.count(), 0);
}

void tst_QSignalSpy::spyFunctionPointerWithBasicArgs()
{
    QtTestObject obj;
    QSignalSpy spy(&obj, &QtTestObject::sig1);

    emit obj.sig1(1, 2);
    QCOMPARE(spy.count(), 1);

    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.count(), 2);
    QCOMPARE(args.at(0).toInt(), 1);
    QCOMPARE(args.at(1).toInt(), 2);

    QSignalSpy spyl(&obj, &QtTestObject::sigLong);

    emit obj.sigLong(1l, 2l);
    args = spyl.takeFirst();
    QCOMPARE(args.count(), 2);
    QCOMPARE(qvariant_cast<long>(args.at(0)), 1l);
    QCOMPARE(qvariant_cast<long>(args.at(1)), 2l);
}


void tst_QSignalSpy::spyFunctionPointerWithPointers()
{
    qRegisterMetaType<int *>("int*");

    QtTestObject obj;
    QSignalSpy spy(&obj, &QtTestObject::sig2);

    int i1 = 1;
    int i2 = 2;

    emit obj.sig2(&i1, &i2);
    QCOMPARE(spy.count(), 1);

    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.count(), 2);
    QCOMPARE(*static_cast<int * const *>(args.at(0).constData()), &i1);
    QCOMPARE(*static_cast<int * const *>(args.at(1).constData()), &i2);
}

void tst_QSignalSpy::spyFunctionPointerWithBasicQtClasses()
{
    QtTestObject2 obj;

    QSignalSpy spy(&obj, &QtTestObject2::sig);
    emit obj.sig(QString("bubu"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("bubu"));

    QSignalSpy spy2(&obj, &QtTestObject2::sig5);
    QVariant val(45);
    emit obj.sig5(val);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy2.at(0).count(), 1);
    QCOMPARE(spy2.at(0).at(0), val);
    QCOMPARE(qvariant_cast<QVariant>(spy2.at(0).at(0)), val);
}

void tst_QSignalSpy::spyFunctionPointerWithQtClasses()
{
    QtTestObject2 obj;

    QSignalSpy spy(&obj, &QtTestObject2::sig2);
    QDateTime dt = QDateTime::currentDateTime();
    emit obj.sig2(dt);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).count(), 1);
    QCOMPARE(spy.at(0).at(0).typeName(), "QDateTime");
    QCOMPARE(*static_cast<const QDateTime *>(spy.at(0).at(0).constData()), dt);
    QCOMPARE(spy.at(0).at(0).toDateTime(), dt);

    QSignalSpy spy2(&obj, &QtTestObject2::sig3);
    emit obj.sig3(this);
    QCOMPARE(*static_cast<QObject * const *>(spy2.value(0).value(0).constData()),
            (QObject *)this);
    QCOMPARE(qvariant_cast<QObject *>(spy2.value(0).value(0)), (QObject*)this);

    QSignalSpy spy3(&obj, &QtTestObject2::sig4);
    emit obj.sig4(QChar('A'));
    QCOMPARE(qvariant_cast<QChar>(spy3.value(0).value(0)), QChar('A'));
}

void tst_QSignalSpy::spyFunctionPointerWithQtTypedefs()
{
    QtTestObject3 obj;

    QSignalSpy spy2(&obj, &QtTestObject3::sig2);
    emit obj.sig2(42, 43);
    QCOMPARE(spy2.value(0).value(0).toInt(), 42);
    QCOMPARE(spy2.value(0).value(1).toInt(), 43);
}

void tst_QSignalSpy::waitFunctionPointer_signalEmitted()
{
    QTimer::singleShot(0, this, SIGNAL(sigFoo()));
    QSignalSpy spy(this, &tst_QSignalSpy::sigFoo);
    QVERIFY(spy.wait(1));
}

void tst_QSignalSpy::waitFunctionPointer_timeout()
{
    QSignalSpy spy(this, &tst_QSignalSpy::sigFoo);
    QVERIFY(!spy.wait(1));
}

void tst_QSignalSpy::waitFunctionPointer_signalEmittedLater()
{
    QTimer::singleShot(500, this, SIGNAL(sigFoo()));
    QSignalSpy spy(this, &tst_QSignalSpy::sigFoo);
    QVERIFY(spy.wait(1000));
}

void tst_QSignalSpy::waitFunctionPointer_signalEmittedTooLate()
{
    QTimer::singleShot(500, this, SIGNAL(sigFoo()));
    QSignalSpy spy(this, &tst_QSignalSpy::sigFoo);
    QVERIFY(!spy.wait(200));
    QTest::qWait(400);
    QCOMPARE(spy.count(), 1);
}

void tst_QSignalSpy::waitFunctionPointer_signalEmittedMultipleTimes()
{
    QTimer::singleShot(100, this, SIGNAL(sigFoo()));
    QTimer::singleShot(800, this, SIGNAL(sigFoo()));
    QSignalSpy spy(this, &tst_QSignalSpy::sigFoo);
    QVERIFY(spy.wait());
    QCOMPARE(spy.count(), 1); // we don't wait for the second signal...
    QVERIFY(spy.wait());
    QCOMPARE(spy.count(), 2);
    QVERIFY(!spy.wait(1));
    QTimer::singleShot(10, this, SIGNAL(sigFoo()));
    QVERIFY(spy.wait());
    QCOMPARE(spy.count(), 3);
}

void tst_QSignalSpy::spyOnMetaMethod()
{
    QObject obj;
    auto mo = obj.metaObject();

    auto signalIndex = mo->indexOfSignal("objectNameChanged(QString)");
    QVERIFY(signalIndex != -1);

    auto signal = mo->method(signalIndex);
    QVERIFY(signal.isValid());
    QCOMPARE(signal.methodType(), QMetaMethod::Signal);

    QSignalSpy spy(&obj, signal);
    QVERIFY(spy.isValid());

    obj.setObjectName("A new object name");
    QCOMPARE(spy.count(), 1);
}

Q_DECLARE_METATYPE(QMetaMethod);
void tst_QSignalSpy::spyOnMetaMethod_invalid()
{
    QFETCH(QObject*, object);
    QFETCH(QMetaMethod, signal);

    QSignalSpy spy(object, signal);
    QVERIFY(!spy.isValid());
}

void tst_QSignalSpy::spyOnMetaMethod_invalid_data()
{
    QTest::addColumn<QObject*>("object");
    QTest::addColumn<QMetaMethod>("signal");

    QTest::addRow("Invalid object")
        << static_cast<QObject*>(nullptr)
        << QMetaMethod();

    QTest::addRow("Empty signal")
        << new QObject(this)
        << QMetaMethod();

    QTest::addRow("Method is not a signal")
        << new QObject(this)
        << QObject::staticMetaObject.method(QObject::staticMetaObject.indexOfMethod("deleteLater()"));
}

QTEST_MAIN(tst_QSignalSpy)
#include "tst_qsignalspy.moc"
