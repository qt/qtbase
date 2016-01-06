/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTESTLOG_P_H
#define QTESTLOG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtTest/qtest_global.h>

QT_BEGIN_NAMESPACE

class QBenchmarkResult;
class QRegularExpression;

class Q_TESTLIB_EXPORT QTestLog
{
public:
    enum LogMode {
        Plain = 0, XML, LightXML, XunitXML, CSV, TeamCity,
#if defined(HAVE_XCTEST)
        XCTest
#endif
    };

    static void enterTestFunction(const char* function);
    static void leaveTestFunction();

    static void addPass(const char *msg);
    static void addFail(const char *msg, const char *file, int line);
    static void addXFail(const char *msg, const char *file, int line);
    static void addXPass(const char *msg, const char *file, int line);
    static void addBPass(const char *msg);
    static void addBFail(const char *msg, const char *file, int line);
    static void addSkip(const char *msg, const char *file, int line);
    static void addBenchmarkResult(const QBenchmarkResult &result);

    static void ignoreMessage(QtMsgType type, const char *msg);
#ifndef QT_NO_REGULAREXPRESSION
    static void ignoreMessage(QtMsgType type, const QRegularExpression &expression);
#endif
    static int unhandledIgnoreMessages();
    static void printUnhandledIgnoreMessages();
    static void clearIgnoreMessages();

    static void warn(const char *msg, const char *file, int line);
    static void info(const char *msg, const char *file, int line);

    static void startLogging();
    static void stopLogging();

    static void addLogger(LogMode mode, const char *filename);

    static int loggerCount();
    static bool loggerUsingStdout();

    static void setVerboseLevel(int level);
    static int verboseLevel();

    static void setMaxWarnings(int max);

    static void setPrintAvailableTagsMode();

    static int passCount();
    static int failCount();
    static int skipCount();
    static int blacklistCount();

    static void resetCounters();

    static void setInstalledTestCoverage(bool installed);
    static bool installedTestCoverage();

    static qint64 nsecsTotalTime();
    static qreal msecsTotalTime()    { return QTestLog::nsecsTotalTime() / 1000000.; }
    static qint64 nsecsFunctionTime();
    static qreal msecsFunctionTime() { return QTestLog::nsecsFunctionTime() / 1000000.; }

private:
    QTestLog();
    ~QTestLog();

    static bool printAvailableTags;
};

QT_END_NAMESPACE

#endif
