/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

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
