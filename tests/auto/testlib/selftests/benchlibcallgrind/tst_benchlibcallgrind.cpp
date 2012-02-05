/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

/* This test must be explicitly enabled since there are no compile tests for valgrind.h */
#ifdef QT_BUG236484
#include <valgrind/valgrind.h>
#endif

class tst_BenchlibCallgrind: public QObject
{
    Q_OBJECT

private slots:
#ifdef QT_BUG236484
    void failInChildProcess();
#endif

    void twoHundredMillionInstructions();
};

#ifdef QT_BUG236484
void tst_BenchlibCallgrind::failInChildProcess()
{
    static double f = 1.0;
    QBENCHMARK {
        for (int i = 0; i < 1000000; ++i) {
            f *= 1.1;
            if (RUNNING_ON_VALGRIND) QFAIL("Running under valgrind!");
        }
    }
}
#endif

void tst_BenchlibCallgrind::twoHundredMillionInstructions()
{
#if !defined(__GNUC__) || !defined(__i386)
    QSKIP("This test is only defined for gcc and x86.");
#else
    QBENCHMARK {
        __asm__ __volatile__(
            "mov $100000000,%%eax   \n"
            "LOOPTOP:               \n"
            "dec %%eax              \n"
            "jnz LOOPTOP            \n"
            : /* no output */
            : /* no input */
            : /* clobber */ "eax"
        );
    }
#endif
}

QTEST_MAIN(tst_BenchlibCallgrind)

#include "tst_benchlibcallgrind.moc"
