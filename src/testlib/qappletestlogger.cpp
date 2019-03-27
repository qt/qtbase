/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qappletestlogger_p.h"

#include <QPair>

QT_BEGIN_NAMESPACE

#if defined(QT_USE_APPLE_UNIFIED_LOGGING)

using namespace QTestPrivate;

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
        message += QLatin1Char('\n') % QString::fromLatin1(description);

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
        case QAbstractTestLogger::QSystem:
            return MessageData{QtWarningMsg, "system"};
        case QAbstractTestLogger::QFatal:
            return MessageData{QtFatalMsg, nullptr};
        case QAbstractTestLogger::Skip:
            return MessageData{QtInfoMsg, "skip"};
        case QAbstractTestLogger::Info:
        case QAbstractTestLogger::QInfo:
            return MessageData{QtInfoMsg, nullptr};
        }
        Q_UNREACHABLE();
    }();

    QTestCharBuffer category;
    messageData.generateCategory(&category);

    QMessageLogContext context(file, line, /* function = */ nullptr, category.data());
    QString msg = message;

    if (type == Skip) {
        if (!message.isNull())
            msg.prepend(testIdentifier() + QLatin1Char('\n'));
        else
            msg = testIdentifier();
    }

    AppleUnifiedLogger::messageHandler(messageData.messageType, context, msg, subsystem());
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
