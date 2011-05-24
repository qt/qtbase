/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtTest/private/qabstracttestlogger_p.h"
#include "QtTest/private/qtestlog_p.h"
#include "QtTest/qtestassert.h"

#include "QtCore/qbytearray.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

QT_BEGIN_NAMESPACE

namespace QTest
{
    static FILE *stream = 0;
}

void QAbstractTestLogger::outputString(const char *msg)
{
    QTEST_ASSERT(QTest::stream);

    ::fputs(msg, QTest::stream);
    ::fflush(QTest::stream);
}

bool QAbstractTestLogger::isTtyOutput()
{
    QTEST_ASSERT(QTest::stream);

#if defined(Q_OS_WIN) || defined(Q_OS_INTEGRITY)
    return true;
#else
    static bool ttyoutput = isatty(fileno(QTest::stream));
    return ttyoutput;
#endif
}


void QAbstractTestLogger::startLogging()
{
    QTEST_ASSERT(!QTest::stream);

    const char *out = QTestLog::outputFileName();
    if (!out) {
        QTest::stream = stdout;
        return;
    }
#if defined(_MSC_VER) && _MSC_VER >= 1400 && !defined(Q_OS_WINCE)
    if (::fopen_s(&QTest::stream, out, "wt")) {
#else
    QTest::stream = ::fopen(out, "wt");
    if (!QTest::stream) {
#endif
        printf("Unable to open file for logging: %s", out);
        ::exit(1);
    }
}

void QAbstractTestLogger::stopLogging()
{
    QTEST_ASSERT(QTest::stream);
    if (QTest::stream != stdout) {
        fclose(QTest::stream);
    } else {
#ifdef Q_OS_SYMBIAN
        // Convenience sleep for Symbian and TRK. Without this sleep depending on the timing the
        // user would not see the complete output because it is still pending in any of the buffers
        // before arriving via the USB port on the development PC
        User::AfterHighRes(2*1000*1000);
#endif
    }
    QTest::stream = 0;
}

namespace QTest
{

extern void filter_unprintable(char *str);

/*!
    \fn int QTest::qt_asprintf(QTestCharBuffer *buf, const char *format, ...);
    \internal
 */
int qt_asprintf(QTestCharBuffer *str, const char *format, ...)
{
    static const int MAXSIZE = 1024*1024*2;

    Q_ASSERT(str);

    int size = str->size();

    va_list ap;
    int res = 0;

    for (;;) {
        va_start(ap, format);
        res = qvsnprintf(str->data(), size, format, ap);
        va_end(ap);
        str->data()[size - 1] = '\0';
        if (res >= 0 && res < size) {
            // We succeeded
            break;
        }
        // buffer wasn't big enough, try again.
        // Note, we're assuming that a result of -1 is always due to running out of space.
        size *= 2;
        if (size > MAXSIZE) {
            break;
        }
        if (!str->reset(size))
            break; // out of memory - take what we have
    }

    filter_unprintable(str->data());

    return res;
}

}

QT_END_NAMESPACE
