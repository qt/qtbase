/****************************************************************************
**
** Copyright (C) 2017 Samuel Gaist <samuel.gaist@edeltech.ch>
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

// Make sure we get a real Q_ASSERT even in release builds
#ifdef QT_NO_DEBUG
# undef QT_NO_DEBUG
#endif

#include <QtTest/QtTest>

class tst_TupleDiagnostics: public QObject
{
    Q_OBJECT

private slots:
    void testEmptyTuple() const;
    void testSimpleTuple() const;
    void testTuple() const;
};

void tst_TupleDiagnostics::testEmptyTuple() const
{
    QCOMPARE(std::tuple<>{}, std::tuple<>{});
}

void tst_TupleDiagnostics::testSimpleTuple() const
{
    QCOMPARE(std::tuple<int>{1}, std::tuple<int>{2});
}

void tst_TupleDiagnostics::testTuple() const
{
    std::tuple<int, char, QString> tuple1{42, 'Y', QStringLiteral("tuple1")};
    std::tuple<int, char, QString> tuple2{42, 'Y', QStringLiteral("tuple2")};
    QCOMPARE(tuple1, tuple2);
}

QTEST_MAIN(tst_TupleDiagnostics)

#include "tst_tuplediagnostics.moc"
