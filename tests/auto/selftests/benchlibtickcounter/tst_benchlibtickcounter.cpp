/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore>
#include <QtTest/QtTest>

#include <private/cycle_p.h>

class tst_BenchlibTickCounter: public QObject
{
    Q_OBJECT

private slots:
    void threeBillionTicks();
};

void tst_BenchlibTickCounter::threeBillionTicks()
{
#ifndef HAVE_TICK_COUNTER
    QSKIP("Tick counter not available on this platform", SkipAll);
#else
    QBENCHMARK {
        CycleCounterTicks start = getticks();
        double el = 0.;
        double max = el;
        while (el < 3000000000.) {
            /* Verify that elapsed time never decreases */
            QVERIFY2(el >= max, qPrintable(
                QString("Tick counter is not monotonic\nElapsed moved from %1 to %2")
                    .arg(max).arg(el)
            ));
            max = el;
            el = elapsed(getticks(), start);
        }
    }
#endif
}

QTEST_MAIN(tst_BenchlibTickCounter)

#include "tst_benchlibtickcounter.moc"
