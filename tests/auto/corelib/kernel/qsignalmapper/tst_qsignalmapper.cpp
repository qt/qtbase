// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qsignalmapper.h>

class tst_QSignalMapper : public QObject
{
    Q_OBJECT
private slots:
    void mapped();
};

class QtTestObject : public QObject
{
    Q_OBJECT
public slots:
    void myIntSlot(int id);
    void myStringSlot(const QString &str);

signals:
    void mysignal(int);

public:
    void emit_mysignal(int);

    int id;
    QString str;
};

void QtTestObject::myIntSlot(int id)
{
    this->id = id;
}

void QtTestObject::myStringSlot(const QString &str)
{
    this->str = str;
}

void QtTestObject::emit_mysignal(int value)
{
    emit mysignal(value);
}

void tst_QSignalMapper::mapped()
{
    QSignalMapper mapper;

    QtTestObject target;
    QtTestObject src1;
    QtTestObject src2;
    QtTestObject src3;

    connect(&src1, SIGNAL(mysignal(int)), &mapper, SLOT(map()));
    connect(&src2, SIGNAL(mysignal(int)), &mapper, SLOT(map()));
    connect(&src3, SIGNAL(mysignal(int)), &mapper, SLOT(map()));

    mapper.setMapping(&src1, 7);
    mapper.setMapping(&src1, 1);
    mapper.setMapping(&src2, 2);
    mapper.setMapping(&src2, "two");
    mapper.setMapping(&src3, "three");

    connect(&mapper, &QSignalMapper::mappedInt, &target, &QtTestObject::myIntSlot);
    connect(&mapper, &QSignalMapper::mappedString, &target, &QtTestObject::myStringSlot);

    src1.emit_mysignal(20);
    QCOMPARE(target.id, 1);
    QVERIFY(target.str.isEmpty());

    src2.emit_mysignal(20);
    QCOMPARE(target.id, 2);
    QCOMPARE(target.str, QLatin1String("two"));

    src3.emit_mysignal(20);
    QCOMPARE(target.id, 2);
    QCOMPARE(target.str, QLatin1String("three"));
}

QTEST_MAIN(tst_QSignalMapper)
#include "tst_qsignalmapper.moc"
