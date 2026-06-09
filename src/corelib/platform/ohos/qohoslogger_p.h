// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSLOGGER_H
#define QOHOSLOGGER_H

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

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>
#include <cstdarg>
#include <hilog/log.h>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(QtForOhos, Q_CORE_EXPORT)

#define qOhosDebug(category) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    qCDebug(category) \
    _Pragma("GCC diagnostic pop")

#define qOhosWarning(category) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    qCWarning(category) \
    _Pragma("GCC diagnostic pop")

#define qOhosCritical(category) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    qCCritical(category) \
    _Pragma("GCC diagnostic pop")

#define qOhosFatal(category) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    qCFatal(category) \
    _Pragma("GCC diagnostic pop")

Q_CORE_EXPORT void qOhosLogMessage(LogLevel logLevel, const char *tag, const char *message);

Q_CORE_EXPORT Q_ATTRIBUTE_FORMAT_PRINTF(2, 0) void qOhosVPrintf(LogLevel logLevel, const char *format, std::va_list ap);

Q_ATTRIBUTE_FORMAT_PRINTF(1, 2) inline void qOhosPrintfDebug(const char *format, ...)
{
    std::va_list ap;
    va_start(ap, format);
    qOhosVPrintf(LOG_DEBUG, format, ap);
    va_end(ap);
}

Q_ATTRIBUTE_FORMAT_PRINTF(1, 2) inline void qOhosPrintfInfo(const char *format, ...)
{
    std::va_list ap;
    va_start(ap, format);
    qOhosVPrintf(LOG_INFO, format, ap);
    va_end(ap);
}

Q_ATTRIBUTE_FORMAT_PRINTF(1, 2) inline void qOhosPrintfWarning(const char *format, ...)
{
    std::va_list ap;
    va_start(ap, format);
    qOhosVPrintf(LOG_WARN, format, ap);
    va_end(ap);
}

Q_ATTRIBUTE_FORMAT_PRINTF(1, 2) inline void qOhosPrintfError(const char *format, ...)
{
    std::va_list ap;
    va_start(ap, format);
    qOhosVPrintf(LOG_ERROR, format, ap);
    va_end(ap);
}

template<typename StringType>
struct QCScopedDebug
{
    QCScopedDebug(StringType message):
    m_message(message)
    { qOhosDebug(QtForOhos) << "T:" << QThread::currentThreadId() << ", M:" << message << "begin";}

    ~QCScopedDebug() { qOhosDebug(QtForOhos) << "T:" << QThread::currentThreadId() << ", M:" << m_message
                                      << "end";}
private:
    StringType m_message;
};

template<typename StringType>
struct QCScopedDebugJS
{
    QCScopedDebugJS(StringType message): m_message(message) {qOhosPrintfDebug("%s begin", message);}
    ~QCScopedDebugJS() {qOhosPrintfDebug("%s end", m_message);}

private:
    StringType m_message;
};

template<typename StringType>
auto make_QCScopedDebug(StringType&& message) -> QCScopedDebug<typename std::decay<StringType>::type> {
    return {std::forward<StringType>(message)};
}

template<typename StringType>
auto make_QCScopedDebugJS(StringType&& message) -> QCScopedDebugJS<typename std::decay<StringType>::type> {
    return {std::forward<StringType>(message)};
}

#define DUMP(x) qOhosDebug(QtForOhos) << "T:" << QThread::currentThreadId() << #x << x;

QT_END_NAMESPACE

#endif // QOHOSLOGGER_H
