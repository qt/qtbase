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

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class tst_Crashes: public QObject
{
    Q_OBJECT

private slots:
    void crash();
};

void tst_Crashes::crash()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
   //we avoid the error dialogbox to appear on windows
   SetErrorMode( SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
#endif
    /*
        We deliberately dereference an invalid but non-zero address;
        it should be non-zero because a few platforms may have special crash behavior
        when dereferencing exactly 0 (e.g. some macs have been observed to generate SIGILL
        rather than SIGSEGV).
    */
    int *i = 0;
    i[1] = 1;
}

QTEST_MAIN(tst_Crashes)

#include "tst_crashes.moc"
