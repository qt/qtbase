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

// Make sure we get a real Q_ASSERT even in release builds
#ifdef QT_NO_DEBUG
# undef QT_NO_DEBUG
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QPair>
#include <QtTest/QtTest>

class tst_PairDiagnostics: public QObject
{
    Q_OBJECT

private slots:
    void testQPair() const;
    void testStdPair() const;
};

void tst_PairDiagnostics::testQPair() const
{
    QPair<int, int> pair1 = qMakePair(1, 1);
    QPair<int, int> pair2 = qMakePair(1, 2);
    QCOMPARE(pair1, pair2);
}

void tst_PairDiagnostics::testStdPair() const
{
    std::pair<int, int> pair1 = std::make_pair(1, 1);
    std::pair<int, int> pair2 = std::make_pair(1, 2);
    QCOMPARE(pair1, pair2);
}

QTEST_MAIN(tst_PairDiagnostics)

#include "tst_pairdiagnostics.moc"
