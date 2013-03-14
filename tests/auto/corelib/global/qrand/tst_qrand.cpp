/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
