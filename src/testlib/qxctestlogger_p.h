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

#ifndef QXCTESTLOGGER_P_H
#define QXCTESTLOGGER_P_H

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

#include <QtTest/private/qabstracttestlogger_p.h>

#include <dispatch/dispatch.h>

Q_FORWARD_DECLARE_OBJC_CLASS(XCTest);
Q_FORWARD_DECLARE_OBJC_CLASS(XCTestRun);
Q_FORWARD_DECLARE_OBJC_CLASS(NSMutableArray);

QT_BEGIN_NAMESPACE

class QXcodeTestLogger : public QAbstractTestLogger
{
public:
    QXcodeTestLogger();
    ~QXcodeTestLogger() override;

    void startLogging() override;
    void stopLogging() override;

    void enterTestFunction(const char *function) override;
    void leaveTestFunction() override;

    void addIncident(IncidentTypes type, const char *description,
        const char *file = 0, int line = 0) override;

    void addMessage(MessageTypes type, const QString &message,
        const char *file = 0, int line = 0) override;

    void addBenchmarkResult(const QBenchmarkResult &result) override;

    static bool canLogTestProgress();
    static int parseCommandLineArgument(const char *argument);

    static bool isActive();

private:
    void pushTestRunForTest(XCTest *test, bool start);
    XCTestRun *popTestRun();

    NSMutableArray<XCTestRun *> *m_testRuns;

    static QXcodeTestLogger *s_currentTestLogger;
};


QT_END_NAMESPACE

#endif
