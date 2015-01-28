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
#include <qsize.h>


class tst_QSizeF : public QObject
{
    Q_OBJECT
private slots:
    void isNull_data();
    void isNull();

    void scale();

    void expandedTo();
    void expandedTo_data();

    void boundedTo_data();
    void boundedTo();

    void transpose_data();
    void transpose();
};

void tst_QSizeF::isNull_data()
{
    QTest::addColumn<qreal>("width");
    QTest::addColumn<qreal>("height");
    QTest::addColumn<bool>("isNull");

    QTest::newRow("0, 0") << qreal(0.0) << qreal(0.0) << true;
    QTest::newRow("-0, -0") << qreal(-0.0) << qreal(-0.0) << true;
    QTest::newRow("0, -0") << qreal(0) << qreal(-0.0) << true;
    QTest::newRow("-0, 0") << qreal(-0.0) << qreal(0) << true;
    QTest::newRow("-0.1, 0") << qreal(-0.1) << qreal(0) << false;
    QTest::newRow("0, -0.1") << qreal(0) << qreal(-0.1) << false;
    QTest::newRow("0.1, 0") << qreal(0.1) << qreal(0) << false;
    QTest::newRow("0, 0.1") << qreal(0) << qreal(0.1) << false;
}

void tst_QSizeF::isNull()
{
    QFETCH(qreal, width);
    QFETCH(qreal, height);
    QFETCH(bool, isNull);

    QSizeF size(width, height);
    QCOMPARE(size.width(), width);
    QCOMPARE(size.height(), height);
    QCOMPARE(size.isNull(), isNull);
}

void tst_QSizeF::scale() {
    QSizeF t1(10.4, 12.8);
    t1.scale(60.6, 60.6, Qt::IgnoreAspectRatio);
    QCOMPARE(t1, QSizeF(60.6, 60.6));

    QSizeF t2(10.4, 12.8);
    t2.scale(43.52, 43.52, Qt::KeepAspectRatio);
    QCOMPARE(t2, QSizeF(35.36, 43.52));

    QSizeF t3(9.6, 12.48);
    t3.scale(31.68, 31.68, Qt::KeepAspectRatioByExpanding);
    QCOMPARE(t3, QSizeF(31.68, 41.184));

    QSizeF t4(12.8, 10.4);
    t4.scale(43.52, 43.52, Qt::KeepAspectRatio);
    QCOMPARE(t4, QSizeF(43.52, 35.36));

    QSizeF t5(12.48, 9.6);
    t5.scale(31.68, 31.68, Qt::KeepAspectRatioByExpanding);
    QCOMPARE(t5, QSizeF(41.184, 31.68));

    QSizeF t6(0.0, 0.0);
    t6.scale(200, 240, Qt::IgnoreAspectRatio);
    QCOMPARE(t6, QSizeF(200, 240));

    QSizeF t7(0.0, 0.0);
    t7.scale(200, 240, Qt::KeepAspectRatio);
    QCOMPARE(t7, QSizeF(200, 240));

    QSizeF t8(0.0, 0.0);
    t8.scale(200, 240, Qt::KeepAspectRatioByExpanding);
    QCOMPARE(t8, QSizeF(200, 240));
}


void tst_QSizeF::expandedTo_data() {
    QTest::addColumn<QSizeF>("input1");
    QTest::addColumn<QSizeF>("input2");
    QTest::addColumn<QSizeF>("expected");

    QTest::newRow("data0") << QSizeF(10.4, 12.8) << QSizeF(6.6, 4.4) << QSizeF(10.4, 12.8);
    QTest::newRow("data1") << QSizeF(0.0, 0.0) << QSizeF(6.6, 4.4) << QSizeF(6.6, 4.4);
    // This should pick the highest of w,h components independently of each other,
    // thus the result don't have to be equal to neither input1 nor input2.
    QTest::newRow("data3") << QSizeF(6.6, 4.4) << QSizeF(4.4, 6.6) << QSizeF(6.6, 6.6);
}

void tst_QSizeF::expandedTo() {
    QFETCH( QSizeF, input1);
    QFETCH( QSizeF, input2);
    QFETCH( QSizeF, expected);

    QCOMPARE( input1.expandedTo(input2), expected);
}

void tst_QSizeF::boundedTo_data() {
    QTest::addColumn<QSizeF>("input1");
    QTest::addColumn<QSizeF>("input2");
    QTest::addColumn<QSizeF>("expected");

    QTest::newRow("data0") << QSizeF(10.4, 12.8) << QSizeF(6.6, 4.4) << QSizeF(6.6, 4.4);
    QTest::newRow("data1") << QSizeF(0.0, 0.0) << QSizeF(6.6, 4.4) << QSizeF(0.0, 0.0);
    // This should pick the lowest of w,h components independently of each other,
    // thus the result don't have to be equal to neither input1 nor input2.
    QTest::newRow("data3") << QSizeF(6.6, 4.4) << QSizeF(4.4, 6.6) << QSizeF(4.4, 4.4);
}

void tst_QSizeF::boundedTo() {
    QFETCH( QSizeF, input1);
    QFETCH( QSizeF, input2);
    QFETCH( QSizeF, expected);

    QCOMPARE( input1.boundedTo(input2), expected);
}

void tst_QSizeF::transpose_data() {
    QTest::addColumn<QSizeF>("input1");
    QTest::addColumn<QSizeF>("expected");

    QTest::newRow("data0") << QSizeF(10.4, 12.8) << QSizeF(12.8, 10.4);
    QTest::newRow("data1") << QSizeF(0.0, 0.0) << QSizeF(0.0, 0.0);
    QTest::newRow("data3") << QSizeF(6.6, 4.4) << QSizeF(4.4, 6.6);
}

void tst_QSizeF::transpose() {
    QFETCH( QSizeF, input1);
    QFETCH( QSizeF, expected);

    // transpose() works only inplace and does not return anything, so we must do the operation itself before the compare.
    input1.transpose();
    QCOMPARE(input1 , expected);
}

QTEST_APPLESS_MAIN(tst_QSizeF)
#include "tst_qsizef.moc"
