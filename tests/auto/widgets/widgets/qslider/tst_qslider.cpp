// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qslider.h>

class tst_QSlider : public QObject
{
Q_OBJECT

public:
    tst_QSlider();
    virtual ~tst_QSlider();

private slots:
    void getSetCheck();
};

tst_QSlider::tst_QSlider()
{
}

tst_QSlider::~tst_QSlider()
{
}

// Testing get/set functions
void tst_QSlider::getSetCheck()
{
    QSlider obj1;
    // TickPosition QSlider::tickPosition()
    // void QSlider::setTickPosition(TickPosition)
    obj1.setTickPosition(QSlider::TickPosition(QSlider::NoTicks));
    QCOMPARE(QSlider::TickPosition(QSlider::NoTicks), obj1.tickPosition());
    obj1.setTickPosition(QSlider::TickPosition(QSlider::TicksAbove));
    QCOMPARE(QSlider::TickPosition(QSlider::TicksAbove), obj1.tickPosition());
    obj1.setTickPosition(QSlider::TickPosition(QSlider::TicksBelow));
    QCOMPARE(QSlider::TickPosition(QSlider::TicksBelow), obj1.tickPosition());
    obj1.setTickPosition(QSlider::TickPosition(QSlider::TicksBothSides));
    QCOMPARE(QSlider::TickPosition(QSlider::TicksBothSides), obj1.tickPosition());

    // int QSlider::tickInterval()
    // void QSlider::setTickInterval(int)
    obj1.setTickInterval(0);
    QCOMPARE(0, obj1.tickInterval());
    obj1.setTickInterval(INT_MIN);
    QCOMPARE(0, obj1.tickInterval()); // Can't have a negative interval
    obj1.setTickInterval(INT_MAX);
    QCOMPARE(INT_MAX, obj1.tickInterval());
}

QTEST_MAIN(tst_QSlider)
#include "tst_qslider.moc"
