/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>


#include <qdatetime.h>

class tst_QSignalSpy : public QObject
{
    Q_OBJECT

private slots:
    void spyWithoutArgs();
    void spyWithBasicArgs();
    void spyWithPointers();
    void spyWithQtClasses();
    void spyWithBasicQtClasses();
    void spyWithQtTypedefs();
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
};

void tst_QSignalSpy::spyWithBasicQtClasses()
{
    QtTestObject2 obj;

    QSignalSpy spy(&obj, SIGNAL(sig(QString)));
    emit obj.sig(QString("bubu"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("bubu"));
}

void tst_QSignalSpy::spyWithQtClasses()
{
    QtTestObject2 obj;

    qRegisterMetaType<QDateTime>("QDateTime");

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

QTEST_APPLESS_MAIN(tst_QSignalSpy)
#include "tst_qsignalspy.moc"
