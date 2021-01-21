/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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

#include <QtTest/qttestglobal.h>

#if defined(Q_OS_DARWIN)
#include <QtCore/private/qcore_mac_p.h>
#endif

#include <QtCore/qobjectdefs.h>

QT_BEGIN_NAMESPACE

class QBenchmarkResult;
class QRegularExpression;
class QTestData;
class QAbstractTestLogger;

class Q_TESTLIB_EXPORT QTestLog
{
    Q_GADGET
public:
    QTestLog() = delete;
    ~QTestLog() = delete;
    Q_DISABLE_COPY_MOVE(QTestLog)

    enum LogMode {
        Plain = 0, XML, LightXML, JUnitXML, CSV, TeamCity, TAP
#if defined(QT_USE_APPLE_UNIFIED_LOGGING)
        , Apple
#endif
#if defined(HAVE_XCTEST)
        , XCTest
#endif
    };
    Q_ENUM(LogMode);

    static void enterTestFunction(const char* function);
    static void leaveTestFunction();

    static void enterTestData(QTestData *data);

    static void addPass(const char *msg);
    static void addFail(const char *msg, const char *file, int line);
    static void addXFail(const char *msg, const char *file, int line);
    static void addXPass(const char *msg, const char *file, int line);
    static void addBPass(const char *msg);
    static void addBFail(const char *msg, const char *file, int line);
    static void addBXPass(const char *msg, const char *file, int line);
    static void addBXFail(const char *msg, const char *file, int line);
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
    static void addLogger(QAbstractTestLogger *logger);

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
    static int totalCount();

    static void resetCounters();

    static void setInstalledTestCoverage(bool installed);
    static bool installedTestCoverage();

    static qint64 nsecsTotalTime();
    static qreal msecsTotalTime()    { return QTestLog::nsecsTotalTime() / 1000000.; }
    static qint64 nsecsFunctionTime();
    static qreal msecsFunctionTime() { return QTestLog::nsecsFunctionTime() / 1000000.; }

private:
    static bool printAvailableTags;
};

QT_END_NAMESPACE

#endif
