/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

Q_DECLARE_METATYPE(QPainterPath)
Q_DECLARE_METATYPE(Qt::ScrollBarPolicy)

static void _scrollBarRanges_data_1(int offset)
{    
    // No motif, flat frame
    QTest::newRow("1") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                       << 0 << 0 << 0 << 0 << false << false;
    QTest::newRow("2") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                       << 0 << (50 + offset) << 0 << offset  << false << false;
    QTest::newRow("3") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                       << 0 << (50 + offset) << 0 << (100 + offset) << false << false;
    QTest::newRow("4") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                       << 0 << 0 << 0 << 0 << false << false;
    QTest::newRow("5") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                       << -100 << (offset -50) << -100 << (-100 + offset) << false << false;
    QTest::newRow("6") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                       << -100 << (offset -50) << -100 << offset << false << false;
    QTest::newRow("7") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                       << 0 << (offset + 1)  << 0 << offset + 1 << false << false;
    QTest::newRow("8") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                       << 0 << (50 + offset + 1) << 0 << offset + 1 << false << false;
    QTest::newRow("9") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                       << 0 << (50 + offset + 1) << 0 << (100 + offset + 1) << false << false;
    QTest::newRow("10") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                        << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                        << -101 << (-100 + offset) << -101 << (-100 + offset) << false << false;
    QTest::newRow("11") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                        << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                        << (-101) << (offset + -50) << -101 << (-100 + offset) << false << false;
    QTest::newRow("12") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                        << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                        << (-101) << (offset -50) << (-101) << offset << false << false;
    QTest::newRow("13") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                        << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                        << 0 << (offset + 16) << 0 << (offset + 16) << false << false;
    QTest::newRow("14") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                        << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                        << 0 << (50 + offset + 16) << 0 << (offset + 16) << false << false;
    QTest::newRow("15") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                        << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                        << 0 << (50 + offset + 16) << 0 << (100 + offset + 16) << false << false;
    QTest::newRow("16") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                        << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                        << (-100 - 16) << (-100 + offset) << (-100 - 16 ) << (-100 + offset) << false << false;
    QTest::newRow("17") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                        << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                        << (-100 - 16) << (offset -50) << (-100 - 16) << (-100 + offset) << false << false;
    QTest::newRow("18") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                        << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                        << (-100 - 16) << (offset -50) << (-100 - 16) << offset << false << false;
    QTest::newRow("1 x2") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                          << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                          << 0 << (150 + offset) << 0 << (100 + offset) << false << false;
    QTest::newRow("2 x2") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                          << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                          << 0 << (250 + offset) << 0 << (100 + offset) << false << false;
    QTest::newRow("3 x2") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                          << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                          << 0 << (250 + offset) << 0 << (300 + offset) << false << false;
    QTest::newRow("4 x2") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                          << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                          << -200 << (-50 + offset) << -200 << (-100 + offset) << false << false;
    QTest::newRow("5 x2") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                          << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                          << -200 << (50 + offset) << -200 << (-100 + offset) << false << false;
    QTest::newRow("6 x2") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                          << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                          << -200 << (50 + offset) << -200 << (100 + offset) << false << false;
    QTest::newRow("1 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                     << 0 << 0 << 0 << 0 << false << false;
    QTest::newRow("2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                     << 0 << 50 << 0 << 0 << false << false;
    QTest::newRow("3 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                     << 0 << 50 << 0 << 100 << false << false;
    QTest::newRow("4 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                     << 0 << 0 << 0 << 0 << false << false;
    QTest::newRow("5 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                     << -100 << -50 << 0 << 0 << false << false;
    QTest::newRow("6 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                     << -100 << -50 << -100 << 0 << false << false;
    QTest::newRow("7 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                     << 0 << 1 << 0 << 1 << false << false;
    QTest::newRow("8 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                     << 0 << 51 << 0 << 1 << false << false;
    QTest::newRow("9 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                     << 0 << 51 << 0 << 101 << false << false;
    QTest::newRow("10 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                      << -101 << -100 << -101 << -100 << false << false;
    QTest::newRow("11 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                      << -101 << -50 << -101 << -100 << false << false;
    QTest::newRow("12 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                      << -101 << -50 << -101 << 0 << false << false;
    QTest::newRow("13 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                      << 0 << 16 << 0 << 16 << false << false;
    QTest::newRow("14 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                      << 0 << (50 + 16) << 0 << 16 << false << false;
    QTest::newRow("15 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                      << 0 << (50 + 16) << 0 << (100 + 16) << false << false;
    QTest::newRow("16 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                      << (-100 - 16) << -100 << (-100 - 16) << -100 << false << false;
    QTest::newRow("17 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                      << (-100 - 16) << -50 << (-100 - 16) << -100 << false << false;
    QTest::newRow("18 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                      << (-100 - 16) << -50 << (-100 - 16) << 0 << false << false;
    QTest::newRow("1 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                        << 0 << 150 << 0 << 100 << false << false;
    QTest::newRow("2 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                        << 0 << 250 << 0 << 100 << false << false;
    QTest::newRow("3 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                        << 0 << 250 << 0 << 300 << false << false;
    QTest::newRow("4 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                        << -200 << -50 << -200 << -100 << false << false;
    QTest::newRow("5 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                        << -200 << 50 << -200 << -100 << false << false;
    QTest::newRow("6 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                        << -200 << 50 << -200 << 100 << false << false;
    QTest::newRow("1 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                         << 0 << 16 << 0 << 16 << false << false;
    QTest::newRow("2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                         << 0 << (50 + 16) << 0 << 16 << false << false;
    QTest::newRow("3 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                         << 0 << (50 + 16) << 0 << (100 + 16) << false << false;
    QTest::newRow("4 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                         << -100 << (-100 + 16) << -100 << (-100 + 16) << false << false;
    QTest::newRow("5 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                         << -100 << (16-50) << -100 << (-100 + 16) << false << false;
    QTest::newRow("6 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                         << -100 << (16-50) << -100 << 16 << false << false;
    QTest::newRow("7 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                         << 0 << 17 << 0 << 17 << false << false;
    QTest::newRow("8 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                         << 0 << (17+50) << 0 << 17 << false << false;
    QTest::newRow("9 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                         << 0 << 67 << 0 << 117 << false << false;
    QTest::newRow("10 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                          << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                          << -101 << (-100 + 16) << -101 << (-100 + 16) << false << false;
    QTest::newRow("11 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                          << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                          << -101 << (16-50) << -101 << (-100 + 16) << false << false;
    QTest::newRow("12 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                          << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                          << -101 << (16-50) << -101 << 16 << false << false;
    QTest::newRow("13 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                          << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                          << 0 << 32 << 0 << 32 << false << false;
    QTest::newRow("14 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                          << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                          << 0 << (50 + 32) << 0 << 32 << false << false;
    QTest::newRow("15 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                          << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                          << 0 << (50 + 32) << 0 << (100 + 32) << false << false;
    QTest::newRow("16 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                          << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                          << (-100 - 16) << (-100 + 16) << (-100 - 16) << (-100 + 16) << false << false;
    QTest::newRow("17 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                          << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                          << (-100 - 16) << (16-50) << (-100 - 16) << (-100 + 16) << false << false;
    QTest::newRow("18 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                          << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                          << (-100 - 16) << (16-50) << (-100 - 16) << 16 << false << false;
    QTest::newRow("1 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                            << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                            << 0 << (150 + 16) << 0 << (100 + 16) << false << false;
    QTest::newRow("2 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                            << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                            << 0 << (250 + 16) << 0 << (100 + 16) << false << false;
    QTest::newRow("3 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                            << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                            << 0 << (250 + 16) << 0 << (300 + 16) << false << false;
    QTest::newRow("4 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                            << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                            << -200 << (-50 + 16) << -200 << (-100 + 16) << false << false;
    QTest::newRow("5 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                            << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                            << -200 << (50 + 16) << -200 << (-100 + 16) << false << false;
    QTest::newRow("6 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                            << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                            << -200 << (50 + 16) << -200 << (100 + 16) << false << false;
    QTest::newRow("1 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                     << 0 << 16 << 0 << 0 << false << false;
    QTest::newRow("2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                     << 0 << (50 + 16) << 0 << 0 << false << false;
    QTest::newRow("3 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                     << 0 << (50 + 16) << 0 << 100 << false << false;
    QTest::newRow("4 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                     << -100 << (-100 + 16) << 0 << 0 << false << false;
    QTest::newRow("5 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                     << -100 << (16-50) << 0 << 0 << false << false;
    QTest::newRow("6 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                     << -100 << (16-50) << -100 << 0 << false << false;
    QTest::newRow("7 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                     << 0 << 17 << 0 << 1 << false << false;
    QTest::newRow("8 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                     << 0 << (17+50) << 0 << 1 << false << false;
    QTest::newRow("9 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                     << 0 << 67 << 0 << 101 << false << false;
    QTest::newRow("10 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                      << -101 << (-100 + 16) << -101 << -100 << false << false;
    QTest::newRow("11 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                      << -101 << (16-50) << -101 << -100 << false << false;
    QTest::newRow("12 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                      << -101 << (16-50) << -101 << 0 << false << false;
    QTest::newRow("13 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                      << 0 << 32 << 0 << 16 << false << false;
    QTest::newRow("14 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                      << 0 << (50 + 32) << 0 << 16 << false << false;
    QTest::newRow("15 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                      << 0 << (50 + 32) << 0 << (100 + 16) << false << false;
    QTest::newRow("16 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                      << (-100 - 16) << (-100 + 16) << (-100 - 16) << -100 << false << false;
    QTest::newRow("17 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                      << (-100 - 16) << (16-50) << (-100 - 16) << -100 << false << false;
    QTest::newRow("18 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                      << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                      << (-100 - 16) << (16-50) << (-100 - 16) << 0 << false << false;
    QTest::newRow("1 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                        << 0 << (150 + 16) << 0 << 100 << false << false;
    QTest::newRow("2 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                        << 0 << (250 + 16) << 0 << 100 << false << false;
    QTest::newRow("3 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                        << 0 << (250 + 16) << 0 << 300 << false << false;
    QTest::newRow("4 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                        << -200 << (-50 + 16) << -200 << -100 << false << false;
    QTest::newRow("5 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                        << -200 << (50 + 16) << -200 << -100 << false << false;
    QTest::newRow("6 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                        << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                        << -200 << (50 + 16) << -200 << 100 << false << false;
}

static void _scrollBarRangesMotif_data_1(int offset)
{
    // Motif, flat frame
    QTest::newRow("Motif, 1") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                              << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                              << 0 << 0 << 0 << 0 << true << false;
    QTest::newRow("Motif, 2") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                              << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                              << 0 << (50 + offset) << 0 << offset << true << false;
    QTest::newRow("Motif, 3") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                              << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                              << 0 << (50 + offset) << 0 << (100 + offset) << true << false;
    QTest::newRow("Motif, 4") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                              << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                              << 0 << 0 << 0 << 0 << true << false;
    QTest::newRow("Motif, 5") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                              << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                              << -100 << (offset-50) << -100 << (-100 + offset) << true << false;
    QTest::newRow("Motif, 6") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                              << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                              << -100 << (offset-50) << -100 << offset << true << false;
    QTest::newRow("Motif, 7") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                              << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                              << 0 << offset + 1 << 0 << offset + 1 << true << false;
    QTest::newRow("Motif, 8") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                              << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                              << 0 << (50 + offset + 1) << 0 << offset + 1 << true << false;
    QTest::newRow("Motif, 9") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                              << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                              << 0 << (50 + offset + 1) << 0 << (100 + offset + 1) << true << false;
    QTest::newRow("Motif, 10") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << -101 << (-100 + offset) << -101 << (-100 + offset) << true << false;
    QTest::newRow("Motif, 11") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << (-101) << (offset-50) << -101 << (-100 + offset) << true << false;
    QTest::newRow("Motif, 12") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << (-101) << (offset-50) << (-101) << offset << true << false;
    QTest::newRow("Motif, 13") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << 0 << (offset + 16) << 0 << (offset + 16) << true << false;
    QTest::newRow("Motif, 14") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << 0 << (50 + offset + 16) << 0 << (offset + 16) << true << false;
    QTest::newRow("Motif, 15") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << 0 << (50 + offset + 16) << 0 << (100 + offset + 16) << true << false;
    QTest::newRow("Motif, 16") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << (-100 - 16) << (-100 + offset) << (-100 - 16) << (-100 + offset) << true << false;
    QTest::newRow("Motif, 17") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << (-100 - 16) << (offset-50) << (-100 - 16) << (-100 + offset) << true << false;
    QTest::newRow("Motif, 18") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << (-100 - 16) << (offset-50) << (-100 - 16) << offset << true << false;
    QTest::newRow("Motif, 1 x2") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                 << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                 << 0 << (150 + offset) << 0 << (100 + offset) << true << false;
    QTest::newRow("Motif, 2 x2") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                 << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                 << 0 << (250 + offset) << 0 << (100 + offset) << true << false;
    QTest::newRow("Motif, 3 x2") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                 << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                 << 0 << (250 + offset) << 0 << (300 + offset) << true << false;
    QTest::newRow("Motif, 4 x2") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                 << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                 << -200 << (-50 + offset) << -200 << (-100 + offset) << true << false;
    QTest::newRow("Motif, 5 x2") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                 << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                 << -200 << (50 + offset) << -200 << (-100 + offset) << true << false;
    QTest::newRow("Motif, 6 x2") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                 << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                 << -200 << (50 + offset) << -200 << (100 + offset) << true << false;
    QTest::newRow("Motif, 1 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                            << 0 << 0 << 0 << 0 << true << false;
    QTest::newRow("Motif, 2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                            << 0 << 50 << 0 << 0 << true << false;
    QTest::newRow("Motif, 3 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                            << 0 << 50 << 0 << 100 << true << false;
    QTest::newRow("Motif, 4 No ScrollBars") << QSize(100, 100) << QRectF(-100, -100, 100, 100) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                            << 0 << 0 << 0 << 0 << true << false;
    QTest::newRow("Motif, 5 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                            << -100 << -50 << 0 << 0 << true << false;
    QTest::newRow("Motif, 6 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                            << -100 << -50 << -100 << 0 << true << false;
    QTest::newRow("Motif, 7 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                            << 0 << 1 << 0 << 1 << true << false;
    QTest::newRow("Motif, 8 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                            << 0 << 51 << 0 << 1 << true << false;
    QTest::newRow("Motif, 9 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                            << 0 << 51 << 0 << 101 << true << false;
    QTest::newRow("Motif, 10 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << -101 << -100 << -101 << -100 << true << false;
    QTest::newRow("Motif, 11 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << -101 << -50 << -101 << -100 << true << false;
    QTest::newRow("Motif, 12 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << -101 << -50 << -101 << 0 << true << false;
    QTest::newRow("Motif, 13 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << 0 << 16 << 0 << 16 << true << false;
    QTest::newRow("Motif, 14 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << 0 << (50 + 16) << 0 << 16 << true << false;
    QTest::newRow("Motif, 15 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << 0 << (50 + 16) << 0 << (100 + 16) << true << false;
    QTest::newRow("Motif, 16 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << (-100 - 16) << -100 << (-100 - 16) << -100 << true << false;
    QTest::newRow("Motif, 17 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << (-100 - 16) << -50 << (-100 - 16) << -100 << true << false;
    QTest::newRow("Motif, 18 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << (-100 - 16) << -50 << (-100 - 16) << 0 << true << false;
    QTest::newRow("Motif, 1 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                               << 0 << 150 << 0 << 100 << true << false;
    QTest::newRow("Motif, 2 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                               << 0 << 250 << 0 << 100 << true << false;
    QTest::newRow("Motif, 3 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                               << 0 << 250 << 0 << 300 << true << false;
    QTest::newRow("Motif, 4 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                               << -200 << -50 << -200 << -100 << true << false;
    QTest::newRow("Motif, 5 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                               << -200 << 50 << -200 << -100 << true << false;
    QTest::newRow("Motif, 6 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                               << -200 << 50 << -200 << 100 << true << false;
    QTest::newRow("Motif, 1 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                                << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                << 0 << 16 << 0 << 16 << true << false;
    QTest::newRow("Motif, 2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                                << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                << 0 << (50 + 16) << 0 << 16 << true << false;
    QTest::newRow("Motif, 3 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                                << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                << 0 << (50 + 16) << 0 << (100 + 16) << true << false;
    QTest::newRow("Motif, 4 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                                << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                << -100 << (-100 + 16) << -100 << (-100 + 16) << true << false;
    QTest::newRow("Motif, 5 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                                << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                << -100 << (16-50) << -100 << (-100 + 16) << true << false;
    QTest::newRow("Motif, 6 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                                << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                << -100 << (16-50) << -100 << 16 << true << false;
    QTest::newRow("Motif, 7 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                                << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                << 0 << 17 << 0 << 17 << true << false;
    QTest::newRow("Motif, 8 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                                << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                << 0 << (117-50) << 0 << 17 << true << false;
    QTest::newRow("Motif, 9 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                                << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                << 0 << (117-50) << 0 << 117 << true << false;
    QTest::newRow("Motif, 10 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << -101 << (-100 + 16) << -101 << (-100 + 16) << true << false;
    QTest::newRow("Motif, 11 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << -101 << (16-50) << -101 << (-100 + 16) << true << false;
    QTest::newRow("Motif, 12 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << -101 << (16-50) << -101 << 16 << true << false;
    QTest::newRow("Motif, 13 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << 0 << 32 << 0 << 32 << true << false;
    QTest::newRow("Motif, 14 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << 0 << (50 + 32) << 0 << 32 << true << false;
    QTest::newRow("Motif, 15 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << 0 << (50 + 32) << 0 << (100 + 32) << true << false;
    QTest::newRow("Motif, 16 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << (-100 - 16) << (-100 + 16) << (-100 - 16) << (-100 + 16) << true << false;
    QTest::newRow("Motif, 17 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << (-100 - 16) << (16-50) << (-100 - 16) << (-100 + 16) << true << false;
    QTest::newRow("Motif, 18 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << (-100 - 16) << (16-50) << (-100 - 16) << 16 << true << false;
    QTest::newRow("Motif, 1 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                                   << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                   << 0 << (150 + 16) << 0 << (100 + 16) << true << false;
    QTest::newRow("Motif, 2 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                                   << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                   << 0 << (250 + 16) << 0 << (100 + 16) << true << false;
    QTest::newRow("Motif, 3 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                                   << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                   << 0 << (250 + 16) << 0 << (300 + 16) << true << false;
    QTest::newRow("Motif, 4 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                                   << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                   << -200 << (-50 + 16) << -200 << (-100 + 16) << true << false;
    QTest::newRow("Motif, 5 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                                   << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                   << -200 << (50 + 16) << -200 << (-100 + 16) << true << false;
    QTest::newRow("Motif, 6 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                                   << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                   << -200 << (50 + 16) << -200 << (100 + 16) << true << false;
    QTest::newRow("Motif, 1 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                            << 0 << 16 << 0 << 0 << true << false;
    QTest::newRow("Motif, 2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                            << 0 << (50 + 16) << 0 << 0 << true << false;
    QTest::newRow("Motif, 3 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                            << 0 << (50 + 16) << 0 << 100 << true << false;
    QTest::newRow("Motif, 4 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                            << -100 << (-100 + 16) << 0 << 0 << true << false;
    QTest::newRow("Motif, 5 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                            << -100 << (16-50) << 0 << 0 << true << false;
    QTest::newRow("Motif, 6 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                            << -100 << (16-50) << -100 << 0 << true << false;
    QTest::newRow("Motif, 7 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                            << 0 << 17 << 0 << 1 << true << false;
    QTest::newRow("Motif, 8 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                            << 0 << (17+50) << 0 << 1 << true << false;
    QTest::newRow("Motif, 9 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                            << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                            << 0 << 67 << 0 << 101 << true << false;
    QTest::newRow("Motif, 10 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << -101 << (-100 + 16) << -101 << -100 << true << false;
    QTest::newRow("Motif, 11 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << -101 << (16-50) << -101 << -100 << true << false;
    QTest::newRow("Motif, 12 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << -101 << (16-50) << -101 << 0 << true << false;
    QTest::newRow("Motif, 13 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << 0 << 32 << 0 << 16 << true << false;
    QTest::newRow("Motif, 14 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << 0 << (50 + 32) << 0 << 16 << true << false;
    QTest::newRow("Motif, 15 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << 0 << (50 + 32) << 0 << (100 + 16) << true << false;
    QTest::newRow("Motif, 16 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << (-100 - 16) << (-100 + 16) << (-100 - 16) << -100 << true << false;
    QTest::newRow("Motif, 17 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << (-100 - 16) << (16-50) << (-100 - 16) << -100 << true << false;
    QTest::newRow("Motif, 18 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << (-100 - 16) << (16-50) << (-100 - 16) << 0 << true << false;
    QTest::newRow("Motif, 1 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                               << 0 << (150 + 16) << 0 << 100 << true << false;
    QTest::newRow("Motif, 2 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                               << 0 << (250 + 16) << 0 << 100 << true << false;
    QTest::newRow("Motif, 3 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                               << 0 << (250 + 16) << 0 << 300 << true << false;
    QTest::newRow("Motif, 4 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                               << -200 << (-50 + 16) << -200 << -100 << true << false;
    QTest::newRow("Motif, 5 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                               << -200 << (50 + 16) << -200 << -100 << true << false;
    QTest::newRow("Motif, 6 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                               << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                               << -200 << (50 + 16) << -200 << 100 << true << false;
}

static void _scrollBarRanges_data_2(int offset)
{
    // No motif, styled panel
    QTest::newRow("Styled, 1") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << 0 << 0 << 0 << 0 << false << true;
    QTest::newRow("Styled, 2") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << 0 << (50 + offset) << 0 << offset << false << true;
    QTest::newRow("Styled, 3") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << 0 << (50 + offset) << 0 << (100 + offset) << false << true;
    QTest::newRow("Styled, 4") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << 0 << 0 << 0 << 0 << false << true;
    QTest::newRow("Styled, 5") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << -100 << (offset-50) << -100 << (-100 + offset) << false << true;
    QTest::newRow("Styled, 6") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << -100 << (offset-50) << -100 << offset << false << true;
    QTest::newRow("Styled, 7") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << 0 << offset + 1 << 0 << offset + 1 << false << true;
    QTest::newRow("Styled, 8") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << 0 << (50 + offset + 1) << 0 << offset + 1 << false << true;
    QTest::newRow("Styled, 9") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                               << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                               << 0 << (50 + offset + 1) << 0 << (100 + offset + 1) << false << true;
    QTest::newRow("Styled, 10") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                << -101 << (-100 + offset) << -101 << (-100 + offset) << false << true;
    QTest::newRow("Styled, 11") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                << (-101) << (offset-50) << -101 << (-100 + offset) << false << true;
    QTest::newRow("Styled, 12") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                << (-101) << (offset-50) << (-101) << offset << false << true;
    QTest::newRow("Styled, 13") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                << 0 << (offset + 16) << 0 << (offset + 16) << false << true;
    QTest::newRow("Styled, 14") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                << 0 << (50 + offset + 16) << 0 << (offset + 16) << false << true;
    QTest::newRow("Styled, 15") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                << 0 << (50 + offset + 16) << 0 << (100 + offset + 16) << false << true;
    QTest::newRow("Styled, 16") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                << (-100 - 16) << (-100 + offset) << (-100 - 16) << (-100 + offset) << false << true;
    QTest::newRow("Styled, 17") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                << (-100 - 16) << (offset-50) << (-100 - 16) << (-100 + offset) << false << true;
    QTest::newRow("Styled, 18") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                << (-100 - 16) << (offset-50) << (-100 - 16) << offset << false << true;
    QTest::newRow("Styled, 1 x2") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                  << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                  << 0 << (150 + offset) << 0 << (100 + offset) << false << true;
    QTest::newRow("Styled, 2 x2") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                  << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                  << 0 << (250 + offset) << 0 << (100 + offset) << false << true;
    QTest::newRow("Styled, 3 x2") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                  << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                  << 0 << (250 + offset) << 0 << (300 + offset) << false << true;
    QTest::newRow("Styled, 4 x2") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                  << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                  << -200 << (-50 + offset) << -200 << (-100 + offset) << false << true;
    QTest::newRow("Styled, 5 x2") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                  << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                  << -200 << (50 + offset) << -200 << (-100 + offset) << false << true;
    QTest::newRow("Styled, 6 x2") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                  << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                  << -200 << (50 + offset) << -200 << (100 + offset) << false << true;
    QTest::newRow("Styled, 1 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << 0 << 0 << 0 << 0 << false << true;
    QTest::newRow("Styled, 2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << 0 << 50 << 0 << 0 << false << true;
    QTest::newRow("Styled, 3 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << 0 << 50 << 0 << 100 << false << true;
    QTest::newRow("Styled, 4 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << 0 << 0 << 0 << 0 << false << true;
    QTest::newRow("Styled, 5 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << -100 << -50 << 0 << 0 << false << true;
    QTest::newRow("Styled, 6 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << -100 << -50 << -100 << 0 << false << true;
    QTest::newRow("Styled, 7 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << 0 << 1 << 0 << 1 << false << true;
    QTest::newRow("Styled, 8 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << 0 << 51 << 0 << 1 << false << true;
    QTest::newRow("Styled, 9 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                             << 0 << 51 << 0 << 101 << false << true;
    QTest::newRow("Styled, 10 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                              << -101 << -100 << -101 << -100 << false << true;
    QTest::newRow("Styled, 11 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                              << -101 << -50 << -101 << -100 << false << true;
    QTest::newRow("Styled, 12 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                              << -101 << -50 << -101 << 0 << false << true;
    QTest::newRow("Styled, 13 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                              << 0 << 16 << 0 << 16 << false << true;
    QTest::newRow("Styled, 14 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                              << 0 << (50 + 16) << 0 << 16 << false << true;
    QTest::newRow("Styled, 15 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                              << 0 << (50 + 16) << 0 << (100 + 16) << false << true;
    QTest::newRow("Styled, 16 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                              << (-100 - 16) << -100 << (-100 - 16) << -100 << false << true;
    QTest::newRow("Styled, 17 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                              << (-100 - 16) << -50 << (-100 - 16) << -100 << false << true;
    QTest::newRow("Styled, 18 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                              << (-100 - 16) << -50 << (-100 - 16) << 0 << false << true;
    QTest::newRow("Styled, 1 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                << 0 << 150 << 0 << 100 << false << true;
    QTest::newRow("Styled, 2 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                << 0 << 250 << 0 << 100 << false << true;
    QTest::newRow("Styled, 3 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                << 0 << 250 << 0 << 300 << false << true;
    QTest::newRow("Styled, 4 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                << -200 << -50 << -200 << -100 << false << true;
    QTest::newRow("Styled, 5 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                << -200 << 50 << -200 << -100 << false << true;
    QTest::newRow("Styled, 6 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                << -200 << 50 << -200 << 100 << false << true;
    QTest::newRow("Styled, 1 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << 0 << 16 << 0 << 16 << false << true;
    QTest::newRow("Styled, 2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << 0 << (50 + 16) << 0 << 16 << false << true;
    QTest::newRow("Styled, 3 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << 0 << (50 + 16) << 0 << (100 + 16) << false << true;
    QTest::newRow("Styled, 4 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << -100 << (-100 + 16) << -100 << (-100 + 16) << false << true;
    QTest::newRow("Styled, 5 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << -100 << (16-50) << -100 << (-100 + 16) << false << true;
    QTest::newRow("Styled, 6 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << -100 << (16-50) << -100 << 16 << false << true;
    QTest::newRow("Styled, 7 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << 0 << 17 << 0 << 17 << false << true;
    QTest::newRow("Styled, 8 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << 0 << (117-50) << 0 << 17 << false << true;
    QTest::newRow("Styled, 9 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                                 << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                 << 0 << (117-50) << 0 << 117 << false << true;
    QTest::newRow("Styled, 10 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                                  << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                  << -101 << (-100 + 16) << -101 << (-100 + 16) << false << true;
    QTest::newRow("Styled, 11 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                                  << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                  << -101 << (16-50) << -101 << (-100 + 16) << false << true;
    QTest::newRow("Styled, 12 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                                  << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                  << -101 << (16-50) << -101 << 16 << false << true;
    QTest::newRow("Styled, 13 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                                  << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                  << 0 << 32 << 0 << 32 << false << true;
    QTest::newRow("Styled, 14 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                                  << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                  << 0 << (50 + 32) << 0 << 32 << false << true;
    QTest::newRow("Styled, 15 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                                  << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                  << 0 << (50 + 32) << 0 << (100 + 32) << false << true;
    QTest::newRow("Styled, 16 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                                  << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                  << (-100 - 16) << (-100 + 16) << (-100 - 16) << (-100 + 16) << false << true;
    QTest::newRow("Styled, 17 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                                  << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                  << (-100 - 16) << (16-50) << (-100 - 16) << (-100 + 16) << false << true;
    QTest::newRow("Styled, 18 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                                  << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                  << (-100 - 16) << (16-50) << (-100 - 16) << 16 << false << true;
    QTest::newRow("Styled, 1 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                                    << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                    << 0 << (150 + 16) << 0 << (100 + 16) << false << true;
    QTest::newRow("Styled, 2 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                                    << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                    << 0 << (250 + 16) << 0 << (100 + 16) << false << true;
    QTest::newRow("Styled, 3 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                                    << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                    << 0 << (250 + 16) << 0 << (300 + 16) << false << true;
    QTest::newRow("Styled, 4 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                                    << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                    << -200 << (-50 + 16) << -200 << (-100 + 16) << false << true;
    QTest::newRow("Styled, 5 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                                    << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                    << -200 << (50 + 16) << -200 << (-100 + 16) << false << true;
    QTest::newRow("Styled, 6 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                                    << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                    << -200 << (50 + 16) << -200 << (100 + 16) << false << true;
    QTest::newRow("Styled, 1 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << 0 << 16 << 0 << 0 << false << true;
    QTest::newRow("Styled, 2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << 0 << (50 + 16) << 0 << 0 << false << true;
    QTest::newRow("Styled, 3 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << 0 << (50 + 16) << 0 << 100 << false << true;
    QTest::newRow("Styled, 4 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << -100 << (-100 + 16) << 0 << 0 << false << true;
    QTest::newRow("Styled, 5 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << -100 << (16-50) << 0 << 0 << false << true;
    QTest::newRow("Styled, 6 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << -100 << (16-50) << -100 << 0 << false << true;
    QTest::newRow("Styled, 7 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << 0 << 17 << 0 << 1 << false << true;
    QTest::newRow("Styled, 8 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << 0 << (17+50) << 0 << 1 << false << true;
    QTest::newRow("Styled, 9 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                             << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                             << 0 << 67 << 0 << 101 << false << true;
    QTest::newRow("Styled, 10 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                              << -101 << (-100 + 16) << -101 << -100 << false << true;
    QTest::newRow("Styled, 11 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                              << -101 << (16-50) << -101 << -100 << false << true;
    QTest::newRow("Styled, 12 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                              << -101 << (16-50) << -101 << 0 << false << true;
    QTest::newRow("Styled, 13 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                              << 0 << 32 << 0 << 16 << false << true;
    QTest::newRow("Styled, 14 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                              << 0 << (50 + 32) << 0 << 16 << false << true;
    QTest::newRow("Styled, 15 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                              << 0 << (50 + 32) << 0 << (100 + 16) << false << true;
    QTest::newRow("Styled, 16 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                              << (-100 - 16) << (-100 + 16) << (-100 - 16) << -100 << false << true;
    QTest::newRow("Styled, 17 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                              << (-100 - 16) << (16-50) << (-100 - 16) << -100 << false << true;
    QTest::newRow("Styled, 18 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                              << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                              << (-100 - 16) << (16-50) << (-100 - 16) << 0 << false << true;
    QTest::newRow("Styled, 1 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                << 0 << (150 + 16) << 0 << 100 << false << true;
    QTest::newRow("Styled, 2 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                << 0 << (250 + 16) << 0 << 100 << false << true;
    QTest::newRow("Styled, 3 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                << 0 << (250 + 16) << 0 << 300 << false << true;
    QTest::newRow("Styled, 4 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                << -200 << (-50 + 16) << -200 << -100 << false << true;
    QTest::newRow("Styled, 5 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                << -200 << (50 + 16) << -200 << -100 << false << true;
    QTest::newRow("Styled, 6 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                                << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                << -200 << (50 + 16) << -200 << 100 << false << true;
}

static void _scrollBarRangesMotif_data_2(int offset)
{
    // Motif, styled panel
    QTest::newRow("Motif, Styled, 1") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                      << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                      << 0 << 0 << 0 << 0 << true << true;
    QTest::newRow("Motif, Styled, 2") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                      << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                      << 0 << (50 + offset + 4) << 0 << (offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 3") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                      << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                      << 0 << (50 + offset + 4) << 0 << (100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 4") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                      << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                      << 0 << 0 << 0 << 0 << true << true;
    QTest::newRow("Motif, Styled, 5") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                      << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                      << -100 << (offset + 4 - 50) << -100 << (-100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 6") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                      << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                      << -100 << (offset + 4 - 50) << -100 << (offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 7") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                      << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                      << 0 << (offset + 1 + 4) << 0 << (offset + 1 + 4) << true << true;
    QTest::newRow("Motif, Styled, 8") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                      << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                      << 0 << (50 + offset + 1 + 4) << 0 << (offset + 1 + 4) << true << true;
    QTest::newRow("Motif, Styled, 9") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                      << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                      << 0 << (50 + offset + 1 + 4) << 0 << (100 + offset + 1 + 4) << true << true;
    QTest::newRow("Motif, Styled, 10") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                       << -101 << (-100 + offset + 4) << -101 << (-100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 11") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                       << (-101) << (offset + 4 - 50) << -101 << (-100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 12") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                       << (-101) << (offset + 4 - 50) << (-101) << (offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 13") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                       << 0 << (offset + 16 + 4) << 0 << (offset + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 14") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                       << 0 << (50 + offset + 16 + 4) << 0 << (offset + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 15") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                       << 0 << (50 + offset + 16 + 4) << 0 << (100 + offset + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 16") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                       << (-100 - 16) << (-100 + offset + 4) << (-100 - 16) << (-100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 17") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                       << (-100 - 16) << (offset + 4 - 50) << (-100 - 16) << (-100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 18") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                       << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                       << (-100 - 16) << (offset + 4 - 50) << (-100 - 16) << (offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 1 x2") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                         << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                         << 0 << (150 + offset + 4) << 0 << (100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 2 x2") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                         << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                         << 0 << (250 + offset + 4) << 0 << (100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 3 x2") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                         << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                         << 0 << (250 + offset + 4) << 0 << (300 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 4 x2") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                         << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                         << -200 << (-50 + offset + 4) << -200 << (-100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 5 x2") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                         << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                         << -200 << (50 + offset + 4) << -200 << (-100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 6 x2") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                         << Qt::ScrollBarAsNeeded << Qt::ScrollBarAsNeeded
                                         << -200 << (50 + offset + 4) << -200 << (100 + offset + 4) << true << true;
    QTest::newRow("Motif, Styled, 1 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                    << 0 << 0 << 0 << 0 << true << true;
    QTest::newRow("Motif, Styled, 2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                    << 0 << 50 << 0 << 0 << true << true;
    QTest::newRow("Motif, Styled, 3 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                    << 0 << 50 << 0 << 100 << true << true;
    QTest::newRow("Motif, Styled, 4 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                    << 0 << 0 << 0 << 0 << true << true;
    QTest::newRow("Motif, Styled, 5 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                    << -100 << -50 << 0 << 0 << true << true;
    QTest::newRow("Motif, Styled, 6 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                    << -100 << -50 << -100 << 0 << true << true;
    QTest::newRow("Motif, Styled, 7 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                    << 0 << 1 << 0 << 1 << true << true;
    QTest::newRow("Motif, Styled, 8 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                    << 0 << 51 << 0 << 1 << true << true;
    QTest::newRow("Motif, Styled, 9 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                    << 0 << 51 << 0 << 101 << true << true;
    QTest::newRow("Motif, Styled, 10 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                     << -101 << -100 << -101 << -100 << true << true;
    QTest::newRow("Motif, Styled, 11 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                     << -101 << -50 << -101 << -100 << true << true;
    QTest::newRow("Motif, Styled, 12 No ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                     << -101 << -50 << -101 << 0 << true << true;
    QTest::newRow("Motif, Styled, 13 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                     << 0 << 16 << 0 << 16 << true << true;
    QTest::newRow("Motif, Styled, 14 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                     << 0 << (50 + 16) << 0 << 16 << true << true;
    QTest::newRow("Motif, Styled, 15 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                     << 0 << (50 + 16) << 0 << (100 + 16) << true << true;
    QTest::newRow("Motif, Styled, 16 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                     << (-100 - 16) << -100 << (-100 - 16) << -100 << true << true;
    QTest::newRow("Motif, Styled, 17 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                     << (-100 - 16) << -50 << (-100 - 16) << -100 << true << true;
    QTest::newRow("Motif, Styled, 18 No ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                     << (-100 - 16) << -50 << (-100 - 16) << 0 << true << true;
    QTest::newRow("Motif, Styled, 1 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                       << 0 << 150 << 0 << 100 << true << true;
    QTest::newRow("Motif, Styled, 2 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                       << 0 << 250 << 0 << 100 << true << true;
    QTest::newRow("Motif, Styled, 3 x2 No ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                       << 0 << 250 << 0 << 300 << true << true;
    QTest::newRow("Motif, Styled, 4 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                       << -200 << -50 << -200 << -100 << true << true;
    QTest::newRow("Motif, Styled, 5 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                       << -200 << 50 << -200 << -100 << true << true;
    QTest::newRow("Motif, Styled, 6 x2 No ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOff
                                                       << -200 << 50 << -200 << 100 << true << true;
    QTest::newRow("Motif, Styled, 1 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                                        << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                        << 0 << (16 + 4) << 0 << (16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                                        << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                        << 0 << (50 + 16 + 4) << 0 << (16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 3 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                                        << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                        << 0 << (50 + 16 + 4) << 0 << (100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 4 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                                        << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                        << -100 << (-100 + 16 + 4) << -100 << (-100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 5 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                                        << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                        << -100 << (16 + 4 - 50) << -100 << (-100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 6 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                                        << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                        << -100 << (16 + 4 - 50) << -100 << (16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 7 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                                        << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                        << 0 << (17 + 4) << 0 << (17 + 4) << true << true;
    QTest::newRow("Motif, Styled, 8 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                                        << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                        << 0 << (117 + 4 - 50) << 0 << (17 + 4) << true << true;
    QTest::newRow("Motif, Styled, 9 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                                        << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                        << 0 << (117 + 4 - 50) << 0 << (117 + 4) << true << true;
    QTest::newRow("Motif, Styled, 10 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                         << -101 << (-100 + 16 + 4) << -101 << (-100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 11 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                         << -101 << (16 + 4 - 50) << -101 << (-100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 12 Always ScrollBars") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                         << -101 << (16 + 4 - 50) << -101 << (16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 13 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                         << 0 << (32 + 4) << 0 << (32 + 4) << true << true;
    QTest::newRow("Motif, Styled, 14 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                         << 0 << (50 + 32 + 4) << 0 << (32 + 4) << true << true;
    QTest::newRow("Motif, Styled, 15 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                         << 0 << (50 + 32 + 4) << 0 << (100 + 32 + 4) << true << true;
    QTest::newRow("Motif, Styled, 16 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                         << (-100 - 16) << (-100 + 16 + 4) << (-100 - 16) << (-100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 17 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                         << (-100 - 16) << (16 + 4 - 50) << (-100 - 16) << (-100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 18 Always ScrollBars") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                                         << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                         << (-100 - 16) << (16 + 4 - 50) << (-100 - 16) << (16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 1 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                                           << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                           << 0 << (150 + 16 + 4) << 0 << (100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 2 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                                           << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                           << 0 << (250 + 16 + 4) << 0 << (100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 3 x2 Always ScrollBars") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                                           << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                           << 0 << (250 + 16 + 4) << 0 << (300 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 4 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                                           << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                           << -200 << (-50 + 16 + 4) << -200 << (-100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 5 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                                           << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                           << -200 << (50 + 16 + 4) << -200 << (-100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 6 x2 Always ScrollBars") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                                           << Qt::ScrollBarAlwaysOn << Qt::ScrollBarAlwaysOn
                                                           << -200 << (50 + 16 + 4) << -200 << (100 + 16 + 4) << true << true;
    QTest::newRow("Motif, Styled, 1 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                    << 0 << (16 + 4) << 0 << 0 << true << true;
    QTest::newRow("Motif, Styled, 2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                    << 0 << (50 + 16 + 4) << 0 << 0 << true << true;
    QTest::newRow("Motif, Styled, 3 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                    << 0 << (50 + 16 + 4) << 0 << 100 << true << true;
    QTest::newRow("Motif, Styled, 4 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                    << -100 << (-100 + 16 + 4) << 0 << 0 << true << true;
    QTest::newRow("Motif, Styled, 5 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                    << -100 << (16 + 4 -50) << 0 << 0 << true << true;
    QTest::newRow("Motif, Styled, 6 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                    << -100 << (16 + 4 -50) << -100 << 0 << true << true;
    QTest::newRow("Motif, Styled, 7 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 151, 101) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                    << 0 << (17 + 4) << 0 << 1 << true << true;
    QTest::newRow("Motif, Styled, 8 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 201, 101) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                    << 0 << (17 + 4 + 50) << 0 << 1 << true << true;
    QTest::newRow("Motif, Styled, 9 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 201, 201) << QTransform()
                                                    << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                    << 0 << (117 + 4 - 50) << 0 << 101 << true << true;
    QTest::newRow("Motif, Styled, 10 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 151, 101) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                     << -101 << (-100 + 16 + 4) << -101 << -100 << true << true;
    QTest::newRow("Motif, Styled, 11 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 201, 101) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                     << -101 << (16 + 4 - 50) << -101 << -100 << true << true;
    QTest::newRow("Motif, Styled, 12 Vertical Only") << QSize(150, 100) << QRectF(-101, -101, 201, 201) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                     << -101 << (16 + 4 - 50) << -101 << 0 << true << true;
    QTest::newRow("Motif, Styled, 13 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 166, 116) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                     << 0 << (32 + 4) << 0 << 16 << true << true;
    QTest::newRow("Motif, Styled, 14 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 216, 116) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                     << 0 << (50 + 32 + 4) << 0 << 16 << true << true;
    QTest::newRow("Motif, Styled, 15 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 216, 216) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                     << 0 << (50 + 32 + 4) << 0 << (100 + 16) << true << true;
    QTest::newRow("Motif, Styled, 16 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 166, 116) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                     << (-100 - 16) << (-100 + 16 + 4) << (-100 - 16) << -100 << true << true;
    QTest::newRow("Motif, Styled, 17 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 216, 116) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                     << (-100 - 16) << (16 + 4 - 50) << (-100 - 16) << -100 << true << true;
    QTest::newRow("Motif, Styled, 18 Vertical Only") << QSize(150, 100) << QRectF(-116, -116, 216, 216) << QTransform()
                                                     << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                     << (-100 - 16) << (16 + 4 - 50) << (-100 - 16) << 0 << true << true;
    QTest::newRow("Motif, Styled, 1 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 150, 100) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                       << 0 << (150 + 16 + 4) << 0 << 100 << true << true;
    QTest::newRow("Motif, Styled, 2 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 100) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                       << 0 << (250 + 16 + 4) << 0 << 100 << true << true;
    QTest::newRow("Motif, Styled, 3 x2 Vertical Only") << QSize(150, 100) << QRectF(0, 0, 200, 200) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                       << 0 << (250 + 16 + 4) << 0 << 300 << true << true;
    QTest::newRow("Motif, Styled, 4 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 150, 100) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                       << -200 << (-50 + 16 + 4) << -200 << -100 << true << true;
    QTest::newRow("Motif, Styled, 5 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 100) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                       << -200 << (50 + 16 + 4) << -200 << -100 << true << true;
    QTest::newRow("Motif, Styled, 6 x2 Vertical Only") << QSize(150, 100) << QRectF(-100, -100, 200, 200) << QTransform().scale(2, 2)
                                                       << Qt::ScrollBarAlwaysOff << Qt::ScrollBarAlwaysOn
                                                       << -200 << (50 + 16 + 4) << -200 << 100 << true << true;
}

void _scrollBarRanges_data()
{
    QTest::addColumn<QSize>("viewportSize");
    QTest::addColumn<QRectF>("sceneRect");
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<Qt::ScrollBarPolicy>("hbarpolicy");
    QTest::addColumn<Qt::ScrollBarPolicy>("vbarpolicy");
    QTest::addColumn<int>("hmin");
    QTest::addColumn<int>("hmax");
    QTest::addColumn<int>("vmin");
    QTest::addColumn<int>("vmax");
    QTest::addColumn<bool>("useMotif");
    QTest::addColumn<bool>("useStyledPanel");

    const int offset = 16;

    _scrollBarRanges_data_1(offset);
    _scrollBarRanges_data_2(offset);
    // Motif tests are suitable for 96 DPI, only.
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (screen && qFuzzyCompare(screen->logicalDotsPerInchX(), 96.0)) {
        _scrollBarRangesMotif_data_1(offset);
        _scrollBarRangesMotif_data_2(offset);
    }
}
