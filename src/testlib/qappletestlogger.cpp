// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qappletestlogger_p.h"

QT_BEGIN_NAMESPACE

#if defined(QT_USE_APPLE_UNIFIED_LOGGING)

using namespace QTestPrivate;

/*! \internal
    \class QAppleTestLogger
    \inmodule QtTest

    QAppleTestLogger reports test results through Apple's unified system logging.
    Results can be viewed in the Console app.
*/

bool QAppleTestLogger::debugLoggingEnabled()
{
    // Debug-level messages are only captured in memory when debug logging is
    // enabled through a configuration change, which can happen automatically
    // when running inside Xcode, or with the Console application open.
    return os_log_type_enabled(OS_LOG_DEFAULT, OS_LOG_TYPE_DEBUG);
}

QAppleTestLogger::QAppleTestLogger()
    : QAbstractTestLogger(nullptr)
{
}

static QAppleLogActivity testFunctionActivity;

void QAppleTestLogger::enterTestFunction(const char *function)
{
    Q_UNUSED(function);

    // Re-create activity each time
    testFunctionActivity = QT_APPLE_LOG_ACTIVITY("Running test function").enter();

    QTestCharBuffer testIdentifier;
    QTestPrivate::generateTestIdentifier(&testIdentifier);
    QString identifier = QString::fromLatin1(testIdentifier.data());
    QMessageLogContext context(nullptr, 0, nullptr, "qt.test.enter");
    QString message = identifier;

    AppleUnifiedLogger::messageHandler(QtDebugMsg, context, message, identifier);
}

void QAppleTestLogger::leaveTestFunction()
{
    testFunctionActivity.leave();
}

struct MessageData
{
    QtMsgType messageType = QtFatalMsg;
    const char *categorySuffix = nullptr;

    void generateCategory(QTestCharBuffer *category)
    {
        if (categorySuffix)
            QTest::qt_asprintf(category, "qt.test.%s", categorySuffix);
        else
            QTest::qt_asprintf(category, "qt.test");
    }
};


void QAppleTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    MessageData messageData = [=]() {
        switch (type) {
        case QAbstractTestLogger::Skip:
            return MessageData{QtInfoMsg, "skip"};
        case QAbstractTestLogger::Pass:
            return MessageData{QtInfoMsg, "pass"};
        case QAbstractTestLogger::XFail:
            return MessageData{QtInfoMsg, "xfail"};
        case QAbstractTestLogger::Fail:
            return MessageData{QtCriticalMsg, "fail"};
        case QAbstractTestLogger::XPass:
            return MessageData{QtInfoMsg, "xpass"};
        case QAbstractTestLogger::BlacklistedPass:
            return MessageData{QtWarningMsg, "bpass"};
        case QAbstractTestLogger::BlacklistedFail:
            return MessageData{QtInfoMsg, "bfail"};
        case QAbstractTestLogger::BlacklistedXPass:
            return MessageData{QtWarningMsg, "bxpass"};
        case QAbstractTestLogger::BlacklistedXFail:
            return MessageData{QtInfoMsg, "bxfail"};
        }
        Q_UNREACHABLE();
    }();

    QTestCharBuffer category;
    messageData.generateCategory(&category);

    QMessageLogContext context(file, line, /* function = */ nullptr, category.data());

    QString message = testIdentifier();
    if (qstrlen(description))
        message += u'\n' % QString::fromLatin1(description);

    // As long as the Apple logger doesn't propagate the context's file and
    // line number we need to manually print it.
    if (context.line && context.file) {
        QTestCharBuffer line;
        QTest::qt_asprintf(&line, "\n   [Loc: %s:%d]", context.file, context.line);
        message += QLatin1String(line.data());
    }

    AppleUnifiedLogger::messageHandler(messageData.messageType, context, message, subsystem());
}

void QAppleTestLogger::addMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    AppleUnifiedLogger::messageHandler(type, context, message);
}

void QAppleTestLogger::addMessage(MessageTypes type, const QString &message, const char *file, int line)
{
    MessageData messageData = [=]() {
        switch (type) {
        case QAbstractTestLogger::Warn:
        case QAbstractTestLogger::QWarning:
            return MessageData{QtWarningMsg, nullptr};
        case QAbstractTestLogger::QDebug:
            return MessageData{QtDebugMsg, nullptr};
        case QAbstractTestLogger::QCritical:
            return MessageData{QtWarningMsg, "critical"};
        case QAbstractTestLogger::QFatal:
            return MessageData{QtFatalMsg, nullptr};
        case QAbstractTestLogger::Info:
        case QAbstractTestLogger::QInfo:
            return MessageData{QtInfoMsg, nullptr};
        }
        Q_UNREACHABLE();
    }();

    QTestCharBuffer category;
    messageData.generateCategory(&category);

    QMessageLogContext context(file, line, /* function = */ nullptr, category.data());

    AppleUnifiedLogger::messageHandler(messageData.messageType, context, message, subsystem());
}

QString QAppleTestLogger::subsystem() const
{
    QTestCharBuffer buffer;
    // It would be nice to have the data tag as part of the subsystem too, but that
    // will for some tests result in hundreds of thousands of log objects being
    // created, so we limit the subsystem to test functions, which we can hope
    // are reasonably limited.
    generateTestIdentifier(&buffer, TestObject | TestFunction);
    return QString::fromLatin1(buffer.data());
}

QString QAppleTestLogger::testIdentifier() const
{
    QTestCharBuffer buffer;
    generateTestIdentifier(&buffer);
    return QString::fromLatin1(buffer.data());
}

#endif // QT_USE_APPLE_UNIFIED_LOGGING

QT_END_NAMESPACE
