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
    if (__builtin_available(macOS 10.12, iOS 10, tvOS 10, watchOS 3, *))
        return os_log_type_enabled(OS_LOG_DEFAULT, OS_LOG_TYPE_DEBUG);

    return false;
}

QAppleTestLogger::QAppleTestLogger(QAbstractTestLogger *logger)
    : QAbstractTestLogger(nullptr)
    , m_logger(logger)
{
}

static QAppleLogActivity testFunctionActivity;

void QAppleTestLogger::enterTestFunction(const char *function)
{
    // Re-create activity each time
    testFunctionActivity = QT_APPLE_LOG_ACTIVITY("Running test function").enter();

    if (__builtin_available(macOS 10.12, iOS 10, tvOS 10, watchOS 3, *)) {
        QTestCharBuffer testIdentifier;
        QTestPrivate::generateTestIdentifier(&testIdentifier);
        QString identifier = QString::fromLatin1(testIdentifier.data());
        QMessageLogContext context(nullptr, 0, nullptr, "qt.test.enter");
        QString message = identifier;
        if (AppleUnifiedLogger::messageHandler(QtDebugMsg, context, message, identifier))
            return; // AUL already printed to stderr
    }

    m_logger->enterTestFunction(function);
}

void QAppleTestLogger::leaveTestFunction()
{
    m_logger->leaveTestFunction();
    testFunctionActivity.leave();
}

typedef QPair<QtMsgType, const char *> IncidentClassification;
static IncidentClassification incidentTypeToClassification(QAbstractTestLogger::IncidentTypes type)
{
    switch (type) {
    case QAbstractTestLogger::Pass:
        return IncidentClassification(QtInfoMsg, "pass");
    case QAbstractTestLogger::XFail:
        return IncidentClassification(QtInfoMsg, "xfail");
    case QAbstractTestLogger::Fail:
        return IncidentClassification(QtCriticalMsg, "fail");
    case QAbstractTestLogger::XPass:
        return IncidentClassification(QtInfoMsg, "xpass");
    case QAbstractTestLogger::BlacklistedPass:
        return IncidentClassification(QtWarningMsg, "bpass");
    case QAbstractTestLogger::BlacklistedFail:
        return IncidentClassification(QtInfoMsg, "bfail");
    }
    return IncidentClassification(QtFatalMsg, nullptr);
}

void QAppleTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    if (__builtin_available(macOS 10.12, iOS 10, tvOS 10, watchOS 3, *)) {
        IncidentClassification incidentClassification = incidentTypeToClassification(type);

        QTestCharBuffer category;
        QTest::qt_asprintf(&category, "qt.test.%s", incidentClassification.second);
        QMessageLogContext context(file, line, /* function = */ nullptr, category.data());

        QTestCharBuffer subsystemBuffer;
        // It would be nice to have the data tag as part of the subsystem too, but that
        // will for some tests results in hundreds of thousands of log objects being
        // created, so we limit the subsystem to test functions, which we can hope
        // are reasonably limited.
        generateTestIdentifier(&subsystemBuffer, TestObject | TestFunction);
        QString subsystem = QString::fromLatin1(subsystemBuffer.data());

        // We still want the full identifier as part of the message though
        QTestCharBuffer testIdentifier;
        generateTestIdentifier(&testIdentifier);
        QString message = QString::fromLatin1(testIdentifier.data());
        if (qstrlen(description))
            message += QLatin1Char('\n') % QString::fromLatin1(description);

        if (AppleUnifiedLogger::messageHandler(incidentClassification.first, context, message, subsystem))
            return; // AUL already printed to stderr
    }

    m_logger->addIncident(type, description, file, line);
}

void QAppleTestLogger::addMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (__builtin_available(macOS 10.12, iOS 10, tvOS 10, watchOS 3, *)) {
        if (AppleUnifiedLogger::messageHandler(type, context, message))
            return; // AUL already printed to stderr
    }

    m_logger->addMessage(type, context, message);
}

#endif // QT_USE_APPLE_UNIFIED_LOGGING

QT_END_NAMESPACE
