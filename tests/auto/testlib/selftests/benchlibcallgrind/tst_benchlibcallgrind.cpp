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
