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

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qlcdnumber.h>

class tst_QLCDNumber : public QObject
{
Q_OBJECT

public:
    tst_QLCDNumber();
    virtual ~tst_QLCDNumber();

private slots:
    void getSetCheck();
};

tst_QLCDNumber::tst_QLCDNumber()
{
}

tst_QLCDNumber::~tst_QLCDNumber()
{
}

// Testing get/set functions
void tst_QLCDNumber::getSetCheck()
{
    QLCDNumber obj1;
    // int QLCDNumber::digitCount()
    // void QLCDNumber::setDigitCount(int)
    obj1.setDigitCount(0);
    QCOMPARE(0, obj1.digitCount());
    obj1.setDigitCount(INT_MIN);
    QCOMPARE(0, obj1.digitCount()); // Range<0, 99>
    obj1.setDigitCount(INT_MAX);
    QCOMPARE(99, obj1.digitCount()); // Range<0, 99>
}

QTEST_MAIN(tst_QLCDNumber)
#include "tst_qlcdnumber.moc"
