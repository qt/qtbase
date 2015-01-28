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
#include <QtCore/QSize>
#include <QtCore/QRectF>
#include <QtGui/QTransform>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>

#include "tst_qgraphicsview.h"

Q_DECLARE_METATYPE(ExpectedValueDescription)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<QRectF>)
Q_DECLARE_METATYPE(QMatrix)
Q_DECLARE_METATYPE(QPainterPath)
Q_DECLARE_METATYPE(Qt::ScrollBarPolicy)
Q_DECLARE_METATYPE(ScrollBarCount)

static void _scrollBarRanges_addTestData(const QString &style, bool styled)
{
    const QString styleString = styled ? style + ", Styled" : style;
    int viewWidth = 250;
    int viewHeight = 100;
    QTest::newRow(qPrintable(styleString + ", 1"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription()
            << ExpectedValueDescription() << ExpectedValueDescription() <<  styled;
    QTest::newRow(qPrintable(styleString + ", 2"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription(50, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 1, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 3"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
             << ExpectedValueDescription() << ExpectedValueDescription(50, 1, 1)
             << ExpectedValueDescription(0, 0) << ExpectedValueDescription(100, 1, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 4"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription()
            << ExpectedValueDescription() << ExpectedValueDescription() <<  styled;
    QTest::newRow(qPrintable(styleString + ", 5"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-100, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 6"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100) << ExpectedValueDescription(0, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 7"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 1, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription(1, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(1, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 8"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 51, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription(51, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(1, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 9"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 51, viewHeight + 101) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription(51, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(101, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 10"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 1, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100, 1, 1)
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 11"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 51, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 12"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 51, viewHeight + 101) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-101) << ExpectedValueDescription(0, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 13"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription(0, 2, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 2, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 14"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription(50, 2, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 2, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 15"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription(50, 2, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(100, 2, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 16"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100, 1, 1)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 17"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 18"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(0, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 1 x2"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 2 x2"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth + 100, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 3 x2"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth + 100, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight + 200, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 4 x2"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 200, 1, 1)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight - 200, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 5 x2"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 100, 1, 1)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight - 200, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 6 x2"))
            << style << QSize(viewWidth, viewHeight) <<
               QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 100, 1, 1)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 1 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription()
            << ExpectedValueDescription() << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 2 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(50)
            << ExpectedValueDescription() << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 3 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(50)
            << ExpectedValueDescription() << ExpectedValueDescription(100) << styled;
    QTest::newRow(qPrintable(styleString + ", 4 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription()
            << ExpectedValueDescription() << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 5 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-50)
            << ExpectedValueDescription() << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 6 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-50)
            << ExpectedValueDescription(-100) << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 7 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 1, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(1)
            << ExpectedValueDescription() << ExpectedValueDescription(1) << styled;
    QTest::newRow(qPrintable(styleString + ", 8 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 51, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(51)
            << ExpectedValueDescription() << ExpectedValueDescription(1) << styled;
    QTest::newRow(qPrintable(styleString + ", 9 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 51, viewHeight + 101) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(51)
            << ExpectedValueDescription() << ExpectedValueDescription(101) << styled;
    QTest::newRow(qPrintable(styleString + ", 10 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 1, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100)
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100) << styled;
    QTest::newRow(qPrintable(styleString + ", 11 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 51, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-50)
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100) << styled;
    QTest::newRow(qPrintable(styleString + ", 12 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 51, viewHeight + 101) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-50)
            << ExpectedValueDescription(-101) << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 13 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(0, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 14 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(50, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 15 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(50, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(100, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 16 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100) << styled;
    QTest::newRow(qPrintable(styleString + ", 17 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-50)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100) << styled;
    QTest::newRow(qPrintable(styleString + ", 18 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-50)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 1 x2 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight) << styled;
    QTest::newRow(qPrintable(styleString + ", 2 x2 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth + 100)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight) << styled;
    QTest::newRow(qPrintable(styleString + ", 3 x2 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth + 100)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight + 200) << styled;
    QTest::newRow(qPrintable(styleString + ", 4 x2 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 200)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight - 200) << styled;
    QTest::newRow(qPrintable(styleString + ", 5 x2 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 100)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight - 200) << styled;
    QTest::newRow(qPrintable(styleString + ", 6 x2 No ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 100)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight) << styled;
    QTest::newRow(qPrintable(styleString + ", 1 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(0, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 1, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 2 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(50, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 1, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 3 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(50, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(100, 1, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 4 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-100, 1, 1)
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-100, 1, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 5 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-100, 1, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 6 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100) << ExpectedValueDescription(0, 1, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 7 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 1, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(1, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(1, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 8 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 51, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(51, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(1, 1, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 9 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 51, viewHeight + 101) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(51, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(101, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 10 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 1, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100, 1, 1)
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 11 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 51, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 12 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 51, viewHeight + 101) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-101) << ExpectedValueDescription(0, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 13 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(0, 2, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 2, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 14 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(50, 2, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 2, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 15 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(50, 2, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(100, 2, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 16 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100, 1, 1)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 17 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 18 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(0, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 1 x2 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 2 x2 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth + 100, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 3 x2 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth + 100, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight + 200, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 4 x2 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 200, 1, 1)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight - 200, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 5 x2 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 100, 1, 1)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight - 200, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 6 x2 Always ScrollBars"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 100, 1, 1)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight, 1, 1) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 1 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(0, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription() <<  styled;
    QTest::newRow(qPrintable(styleString + ", 2 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(50, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription() <<  styled;
    QTest::newRow(qPrintable(styleString + ", 3 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(50, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(100) <<  styled;
    QTest::newRow(qPrintable(styleString + ", 4 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-100, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription() <<  styled;
    QTest::newRow(qPrintable(styleString + ", 5 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 6 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100) << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 7 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 1, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(1, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(1) << styled;
    QTest::newRow(qPrintable(styleString + ", 8 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 51, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(51, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(1) << styled;
    QTest::newRow(qPrintable(styleString + ", 9 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 51, viewHeight + 101) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(51, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(101) << styled;
    QTest::newRow(qPrintable(styleString + ", 10 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 1, viewHeight +1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100, 1, 1)
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100) << styled;
    QTest::newRow(qPrintable(styleString + ", 11 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 51, viewHeight + 1) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-100) << styled;
    QTest::newRow(qPrintable(styleString + ", 12 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-101, -101, viewWidth + 51, viewHeight + 101) << ScrollBarCount(0, 0, 0, 0)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-101) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-101) << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 13 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(0, 2, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 14 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(50, 2, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(0, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 15 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(50, 2, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(100, 1) << styled;
    QTest::newRow(qPrintable(styleString + ", 16 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100, 1, 1)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100) << styled;
    QTest::newRow(qPrintable(styleString + ", 17 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-100) << styled;
    QTest::newRow(qPrintable(styleString + ", 18 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(-1, -1, 1, 1)
            << QTransform() << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription(-50, 1, 1)
            << ExpectedValueDescription(-100, -1) << ExpectedValueDescription() << styled;
    QTest::newRow(qPrintable(styleString + ", 1 x2 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight) << styled;
    QTest::newRow(qPrintable(styleString + ", 2 x2 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth + 100, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight) << styled;
    QTest::newRow(qPrintable(styleString + ", 3 x2 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(0, 0, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription() << ExpectedValueDescription(viewWidth + 100, 1, 1)
            << ExpectedValueDescription() << ExpectedValueDescription(viewHeight + 200) << styled;
    QTest::newRow(qPrintable(styleString + ", 4 x2 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 200, 1, 1)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight - 200) << styled;
    QTest::newRow(qPrintable(styleString + ", 5 x2 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 100, 1, 1)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight - 200) << styled;
    QTest::newRow(qPrintable(styleString + ", 6 x2 Vertical Only"))
            << style << QSize(viewWidth, viewHeight)
            << QRectF(-100, -100, viewWidth + 50, viewHeight + 100) << ScrollBarCount(0, 0, 0, 0)
            << QTransform().scale(2, 2) << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewWidth - 100, 1, 1)
            << ExpectedValueDescription(-200) << ExpectedValueDescription(viewHeight) << styled;
}

void _scrollBarRanges_data()
{
    QTest::addColumn<QString>("style");
    QTest::addColumn<QSize>("viewportSize");
    QTest::addColumn<QRectF>("sceneRect");
    QTest::addColumn<ScrollBarCount>("sceneRectOffsetFactors");
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<Qt::ScrollBarPolicy>("hbarpolicy");
    QTest::addColumn<Qt::ScrollBarPolicy>("vbarpolicy");
    QTest::addColumn<ExpectedValueDescription>("hmin");
    QTest::addColumn<ExpectedValueDescription>("hmax");
    QTest::addColumn<ExpectedValueDescription>("vmin");
    QTest::addColumn<ExpectedValueDescription>("vmax");
    QTest::addColumn<bool>("useStyledPanel");

    foreach (const QString &style, QStyleFactory::keys()) {
        _scrollBarRanges_addTestData(style, false);
        _scrollBarRanges_addTestData(style, true);
    }

    const QScreen *screen = QGuiApplication::primaryScreen();
    if (screen && qFuzzyCompare((double)screen->logicalDotsPerInchX(), 96.0)) {
        _scrollBarRanges_addTestData(QString("motif"), false);
        _scrollBarRanges_addTestData(QString("motif"), true);
    }
}
