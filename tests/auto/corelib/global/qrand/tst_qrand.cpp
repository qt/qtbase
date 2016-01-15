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

class tst_QRand: public QObject
{
    Q_OBJECT
private slots:
    void testqrand();
};

void tst_QRand::testqrand()
{
    const int numTestValues = 100;

    int generatedNumbers[numTestValues];
    bool generatesSameSequence = true;

    // test without calling srand() first
    // should give same sequence as with srand(1)

    for (int i=0; i<numTestValues; ++i)
        generatedNumbers[i] = qrand();

    qsrand(1);
    for (int i=0; i<numTestValues; ++i)
        if (generatedNumbers[i] != qrand())
            generatesSameSequence = false;

    QVERIFY(generatesSameSequence);

    for (unsigned int seed=1; seed < 10; seed+=100) {

        qsrand(seed);
        for (int i=0; i<numTestValues; ++i)
            generatedNumbers[i] = qrand();

        qsrand(seed);
        generatesSameSequence = true;
        for (int i=0; i<numTestValues; ++i)
            if (generatedNumbers[i] != qrand())
                generatesSameSequence = false;

        QVERIFY(generatesSameSequence);
    }
}

QTEST_MAIN(tst_QRand)
#include "tst_qrand.moc"
