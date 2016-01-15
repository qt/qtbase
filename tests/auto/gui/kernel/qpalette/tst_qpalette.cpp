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

#include "qpalette.h"

class tst_QPalette : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void roleValues_data();
    void roleValues();
    void copySemantics();
    void moveSemantics();
};

void tst_QPalette::roleValues_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<int>("value");

    QTest::newRow("QPalette::WindowText") << int(QPalette::WindowText) << 0;
    QTest::newRow("QPalette::Button") << int(QPalette::Button) << 1;
    QTest::newRow("QPalette::Light") << int(QPalette::Light) << 2;
    QTest::newRow("QPalette::Midlight") << int(QPalette::Midlight) << 3;
    QTest::newRow("QPalette::Dark") << int(QPalette::Dark) << 4;
    QTest::newRow("QPalette::Mid") << int(QPalette::Mid) << 5;
    QTest::newRow("QPalette::Text") << int(QPalette::Text) << 6;
    QTest::newRow("QPalette::BrightText") << int(QPalette::BrightText) << 7;
    QTest::newRow("QPalette::ButtonText") << int(QPalette::ButtonText) << 8;
    QTest::newRow("QPalette::Base") << int(QPalette::Base) << 9;
    QTest::newRow("QPalette::Window") << int(QPalette::Window) << 10;
    QTest::newRow("QPalette::Shadow") << int(QPalette::Shadow) << 11;
    QTest::newRow("QPalette::Highlight") << int(QPalette::Highlight) << 12;
    QTest::newRow("QPalette::HighlightedText") << int(QPalette::HighlightedText) << 13;
    QTest::newRow("QPalette::Link") << int(QPalette::Link) << 14;
    QTest::newRow("QPalette::LinkVisited") << int(QPalette::LinkVisited) << 15;
    QTest::newRow("QPalette::AlternateBase") << int(QPalette::AlternateBase) << 16;
    QTest::newRow("QPalette::NoRole") << int(QPalette::NoRole) << 17;
    QTest::newRow("QPalette::ToolTipBase") << int(QPalette::ToolTipBase) << 18;
    QTest::newRow("QPalette::ToolTipText") << int(QPalette::ToolTipText) << 19;

    // Change this value as you add more roles.
    QTest::newRow("QPalette::NColorRoles") << int(QPalette::NColorRoles) << 20;
}

void tst_QPalette::roleValues()
{
    QFETCH(int, role);
    QFETCH(int, value);
    QCOMPARE(role, value);
}

void tst_QPalette::copySemantics()
{
    QPalette src(Qt::red), dst;
    const QPalette control = src; // copy construction
    QVERIFY(src != dst);
    QVERIFY(!src.isCopyOf(dst));
    QCOMPARE(src, control);
    QVERIFY(src.isCopyOf(control));
    dst = src; // copy assignment
    QCOMPARE(dst, src);
    QCOMPARE(dst, control);
    QVERIFY(dst.isCopyOf(src));

    dst = QPalette(Qt::green);
    QVERIFY(dst != src);
    QVERIFY(dst != control);
    QCOMPARE(src, control);
    QVERIFY(!dst.isCopyOf(src));
    QVERIFY(src.isCopyOf(control));
}

void tst_QPalette::moveSemantics()
{
#ifdef Q_COMPILER_RVALUE_REFS
    QPalette src(Qt::red), dst;
    const QPalette control = src;
    QVERIFY(src != dst);
    QCOMPARE(src, control);
    QVERIFY(!dst.isCopyOf(src));
    QVERIFY(!dst.isCopyOf(control));
    dst = qMove(src); // move assignment
    QVERIFY(!dst.isCopyOf(src)); // isCopyOf() works on moved-from palettes, too
    QVERIFY(dst.isCopyOf(control));
    QCOMPARE(dst, control);
    src = control; // check moved-from 'src' can still be assigned to (doesn't crash)
    QVERIFY(src.isCopyOf(dst));
    QVERIFY(src.isCopyOf(control));
    QPalette dst2(qMove(src)); // move construction
    QVERIFY(!src.isCopyOf(dst));
    QVERIFY(!src.isCopyOf(dst2));
    QVERIFY(!src.isCopyOf(control));
    QCOMPARE(dst2, control);
    QVERIFY(dst2.isCopyOf(dst));
    QVERIFY(dst2.isCopyOf(control));
    // check moved-from 'src' can still be destroyed (doesn't crash)
#else
    QSKIP("Compiler doesn't support C++11 move semantics");
#endif
}

QTEST_MAIN(tst_QPalette)
#include "tst_qpalette.moc"
