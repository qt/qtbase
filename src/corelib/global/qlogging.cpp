// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qglobal_p.h"
#include "qlogging.h"
#include "qlogging_p.h"
#include "qlist.h"
#include "qbytearray.h"
#include "qscopeguard.h"
#include "qstring.h"
#include "qvarlengtharray.h"
#include "qdebug.h"
#include "qmutex.h"
#include <QtCore/private/qlocking_p.h>
#include <QtCore/private/qsimd_p.h>
#include "qloggingcategory.h"
#ifndef QT_BOOTSTRAPPED
#include "qelapsedtimer.h"
#include "qdeadlinetimer.h"
#include "qdatetime.h"
#include "qcoreapplication.h"
#include "qthread.h"
#include "private/qloggingregistry_p.h"
#include "private/qcoreapplication_p.h"
#include <qtcore_tracepoints_p.h>
#endif
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif
#ifdef Q_CC_MSVC
#include <intrin.h>
#endif
#if QT_CONFIG(slog2)
#include <sys/slog2.h>
#endif
#if __has_include(<paths.h>)
#include <paths.h>
#endif

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

#ifdef Q_OS_DARWIN
#include <QtCore/private/qcore_mac_p.h>
#endif

#if QT_CONFIG(journald)
# define SD_JOURNAL_SUPPRESS_LOCATION
# include <systemd/sd-journal.h>
# include <syslog.h>
#endif
#if QT_CONFIG(syslog)
# include <syslog.h>
#endif
#ifdef Q_OS_UNIX
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include "private/qcore_unix_p.h"
#endif

#ifdef Q_OS_WASM
#include <emscripten/emscripten.h>
#endif

#if QT_CONFIG(slog2)
extern char *__progname;
#endif

#ifndef QT_BOOTSTRAPPED
#if __has_include(<cxxabi.h>) && QT_CONFIG(backtrace) && QT_CONFIG(regularexpression)
#  include <qregularexpression.h>
#  if QT_CONFIG(dladdr)
#    include <dlfcn.h>
#  endif
#  include BACKTRACE_HEADER
#  include <cxxabi.h>
#  define QLOGGING_HAVE_BACKTRACE
#endif

#if defined(Q_OS_LINUX) && (defined(__GLIBC__) || __has_include(<sys/syscall.h>))
#  include <sys/syscall.h>

# if defined(Q_OS_ANDROID) && !defined(SYS_gettid)
#  define SYS_gettid __NR_gettid
# endif

static long qt_gettid()
{
    // no error handling
    // this syscall has existed since Linux 2.4.11 and cannot fail
    return syscall(SYS_gettid);
}
#elif defined(Q_OS_DARWIN)
#  include <pthread.h>
static int qt_gettid()
{
    // no error handling: this call cannot fail
    __uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return tid;
}
#elif defined(Q_OS_FREEBSD_KERNEL) && defined(__FreeBSD_version) && __FreeBSD_version >= 900031
#  include <pthread_np.h>
static int qt_gettid()
{
    return pthread_getthreadid_np();
}
#else
static QT_PREPEND_NAMESPACE(qint64) qt_gettid()
{
    QT_USE_NAMESPACE
    return qintptr(QThread::currentThreadId());
}
#endif
#endif // !QT_BOOTSTRAPPED

#include <cstdlib>
#include <algorithm>
#include <memory>
#include <vector>

#include <stdio.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#ifndef QT_BOOTSTRAPPED
Q_TRACE_POINT(qtcore, qt_message_print, int type, const char *category, const char *function, const char *file, int line, const QString &message);
#endif

/*!
    \headerfile <QtLogging>
    \inmodule QtCore
    \title Qt Logging Types

    \brief The <QtLogging> header file defines Qt logging types, functions
    and macros.

    The <QtLogging> header file contains several types, functions and
    macros for logging.

    The QtMsgType enum  identifies the various messages that can be generated
    and sent to a Qt message handler; QtMessageHandler is a type definition for
    a pointer to a function with the signature
    \c {void myMessageHandler(QtMsgType, const QMessageLogContext &, const char *)}.
    qInstallMessageHandler() function can be used to install the given
    QtMessageHandler. QMessageLogContext class contains the line, file, and
    function the message was logged at. This information is created by the
    QMessageLogger class.

    <QtLogging> also contains functions that generate messages from the
    given string argument: qDebug(), qInfo(), qWarning(), qCritical(),
    and qFatal(). These functions call the message handler
    with the given message.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 4
*/

template <typename String>
#if !defined(Q_CC_MSVC_ONLY)
Q_NORETURN
#endif
static void qt_message_fatal(QtMsgType, const QMessageLogContext &context, String &&message);
static void qt_message_print(QtMsgType, const QMessageLogContext &context, const QString &message);
static void qt_message_print(const QString &message);

static int checked_var_value(const char *varname)
{
    // qEnvironmentVariableIntValue returns 0 on both parsing failure and on
    // empty, but we need to distinguish between the two for backwards
    // compatibility reasons.
    QByteArray str = qgetenv(varname);
    if (str.isEmpty())
        return 0;

    bool ok;
    int value = str.toInt(&ok, 0);
    return ok ? value : 1;
}

static bool is_fatal_count_down(QAtomicInt &n)
{
    // it's fatal if the current value is exactly 1,
    // otherwise decrement if it's non-zero

    int v = n.loadRelaxed();
    while (v != 0 && !n.testAndSetRelaxed(v, v - 1, v))
        qYieldCpu();
    return v == 1; // we exited the loop, so either v == 0 or CAS succeeded to set n from v to v-1
}

static bool isFatal(QtMsgType msgType)
{
    if (msgType == QtFatalMsg)
        return true;

    if (msgType == QtCriticalMsg) {
        static QAtomicInt fatalCriticals = checked_var_value("QT_FATAL_CRITICALS");
        return is_fatal_count_down(fatalCriticals);
    }

    if (msgType == QtWarningMsg || msgType == QtCriticalMsg) {
        static QAtomicInt fatalWarnings = checked_var_value("QT_FATAL_WARNINGS");
        return is_fatal_count_down(fatalWarnings);
    }

    return false;
}

static bool isDefaultCategory(const char *category)
{
    return !category || strcmp(category, "default") == 0;
}

/*!
    Returns true if writing to \c stderr is supported.

    \internal
    \sa stderrHasConsoleAttached()
*/
static bool systemHasStderr()
{
    return true;
}

/*!
    Returns true if writing to \c stderr will end up in a console/terminal visible to the user.

    This is typically the case if the application was started from the command line.

    If the application is started without a controlling console/terminal, but the parent
    process reads \c stderr and presents it to the user in some other way, the parent process
    may override the detection in this function by setting the QT_ASSUME_STDERR_HAS_CONSOLE
    environment variable to \c 1.

    \note Qt Creator does not implement a pseudo TTY, nor does it launch apps with
    the override environment variable set, but it will read stderr and print it to
    the user, so in effect this function cannot be used to conclude that stderr
    output will _not_ be visible to the user, as even if this function returns false,
    the output might still end up visible to the user. For this reason, we don't guard
    the stderr output in the default message handler with stderrHasConsoleAttached().

    \internal
    \sa systemHasStderr()
*/
static bool stderrHasConsoleAttached()
{
    static const bool stderrHasConsoleAttached = []() -> bool {
        if (!systemHasStderr())
            return false;

        if (qEnvironmentVariableIntValue("QT_LOGGING_TO_CONSOLE")) {
            fprintf(stderr, "warning: Environment variable QT_LOGGING_TO_CONSOLE is deprecated, use\n"
                            "QT_ASSUME_STDERR_HAS_CONSOLE and/or QT_FORCE_STDERR_LOGGING instead.\n");
            return true;
        }

        if (qEnvironmentVariableIntValue("QT_ASSUME_STDERR_HAS_CONSOLE"))
            return true;

#if defined(Q_OS_WIN)
        return GetConsoleWindow();
#elif defined(Q_OS_UNIX)
#       ifndef _PATH_TTY
#       define _PATH_TTY "/dev/tty"
#       endif

        // If we can open /dev/tty, we have a controlling TTY
        int ttyDevice = -1;
        if ((ttyDevice = qt_safe_open(_PATH_TTY, O_RDONLY)) >= 0) {
            qt_safe_close(ttyDevice);
            return true;
        } else if (errno == ENOENT || errno == EPERM || errno == ENXIO) {
            // Fall back to isatty for some non-critical errors
            return isatty(STDERR_FILENO);
        } else {
            return false;
        }
#else
        return false; // No way to detect if stderr has a console attached
#endif
    }();

    return stderrHasConsoleAttached;
}


namespace QtPrivate {

/*!
    Returns true if logging \c stderr should be ensured.

    This is normally the case if \c stderr has a console attached, but may be overridden
    by the user by setting the QT_FORCE_STDERR_LOGGING environment variable to \c 1.

    \internal
    \sa stderrHasConsoleAttached()
*/
bool shouldLogToStderr()
{
    static bool forceStderrLogging = qEnvironmentVariableIntValue("QT_FORCE_STDERR_LOGGING");
    return forceStderrLogging || stderrHasConsoleAttached();
}


} // QtPrivate

using namespace QtPrivate;

/*!
    \class QMessageLogContext
    \inmodule QtCore
    \brief The QMessageLogContext class provides additional information about a log message.
    \since 5.0

    The class provides information about the source code location a qDebug(), qInfo(), qWarning(),
    qCritical() or qFatal() message was generated.

    \note By default, this information is recorded only in debug builds. You can overwrite
    this explicitly by defining \c QT_MESSAGELOGCONTEXT or \c{QT_NO_MESSAGELOGCONTEXT}.

    \sa QMessageLogger, QtMessageHandler, qInstallMessageHandler()
*/

/*!
    \class QMessageLogger
    \inmodule QtCore
    \brief The QMessageLogger class generates log messages.
    \since 5.0

    QMessageLogger is used to generate messages for the Qt logging framework. Usually one uses
    it through qDebug(), qInfo(), qWarning(), qCritical, or qFatal() functions,
    which are actually macros: For example qDebug() expands to
    QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).debug()
    for debug builds, and QMessageLogger(0, 0, 0).debug() for release builds.

    One example of direct use is to forward errors that stem from a scripting language, e.g. QML:

    \snippet code/qlogging/qlogging.cpp 1

    \sa QMessageLogContext, qDebug(), qInfo(), qWarning(), qCritical(), qFatal()
*/

#if defined(Q_CC_MSVC_ONLY) && defined(QT_DEBUG) && defined(_DEBUG) && defined(_CRT_ERROR)
static inline void convert_to_wchar_t_elided(wchar_t *d, size_t space, const char *s) noexcept
{
    size_t len = qstrlen(s);
    if (len + 1 > space) {
        const size_t skip = len - space + 4; // 4 for "..." + '\0'
        s += skip;
        len -= skip;
        for (int i = 0; i < 3; ++i)
          *d++ = L'.';
    }
    while (len--)
        *d++ = *s++;
    *d++ = 0;
}
#endif

/*!
    \internal
*/
Q_NEVER_INLINE
static void qt_message(QtMsgType msgType, const QMessageLogContext &context, const char *msg, va_list ap)
{
    QString buf = QString::vasprintf(msg, ap);
    qt_message_print(msgType, context, buf);

    if (isFatal(msgType))
        qt_message_fatal(msgType, context, buf);
}

#undef qDebug
/*!
    Logs a debug message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qDebug()
*/
void QMessageLogger::debug(const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtDebugMsg, context, msg, ap);
    va_end(ap);
}


#undef qInfo
/*!
    Logs an informational message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qInfo()
    \since 5.5
*/
void QMessageLogger::info(const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtInfoMsg, context, msg, ap);
    va_end(ap);
}

/*!
    \typedef QMessageLogger::CategoryFunction

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/qlogging/qlogging.cpp 2

    The \c Q_DECLARE_LOGGING_CATEGORY macro generates a function declaration
    with this signature, and \c Q_LOGGING_CATEGORY generates its definition.

    \since 5.3
*/

/*!
    Logs a debug message specified with format \a msg for the context \a cat.
    Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCDebug()
*/
void QMessageLogger::debug(const QLoggingCategory &cat, const char *msg, ...) const
{
    if (!cat.isDebugEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtDebugMsg, ctxt, msg, ap);
    va_end(ap);
}

/*!
    Logs a debug message specified with format \a msg for the context returned
    by \a catFunc. Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCDebug()
*/
void QMessageLogger::debug(QMessageLogger::CategoryFunction catFunc,
                           const char *msg, ...) const
{
    const QLoggingCategory &cat = (*catFunc)();
    if (!cat.isDebugEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtDebugMsg, ctxt, msg, ap);
    va_end(ap);
}

#ifndef QT_NO_DEBUG_STREAM

/*!
    Logs a debug message using a QDebug stream

    \sa qDebug(), QDebug
*/
QDebug QMessageLogger::debug() const
{
    QDebug dbg = QDebug(QtDebugMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copyContextFrom(context);
    return dbg;
}

/*!
    Logs a debug message into category \a cat using a QDebug stream.

    \since 5.3
    \sa qCDebug(), QDebug
*/
QDebug QMessageLogger::debug(const QLoggingCategory &cat) const
{
    QDebug dbg = QDebug(QtDebugMsg);
    if (!cat.isDebugEnabled())
        dbg.stream->message_output = false;

    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    return dbg;
}

/*!
    Logs a debug message into category returned by \a catFunc using a QDebug stream.

    \since 5.3
    \sa qCDebug(), QDebug
*/
QDebug QMessageLogger::debug(QMessageLogger::CategoryFunction catFunc) const
{
    return debug((*catFunc)());
}

/*!
    \internal

    Returns a QNoDebug object, which is used to ignore debugging output.

    \sa QNoDebug, qDebug()
*/
QNoDebug QMessageLogger::noDebug() const noexcept
{
    return QNoDebug();
}

#endif

/*!
    Logs an informational message specified with format \a msg for the context \a cat.
    Additional parameters, specified by \a msg, may be used.

    \since 5.5
    \sa qCInfo()
*/
void QMessageLogger::info(const QLoggingCategory &cat, const char *msg, ...) const
{
    if (!cat.isInfoEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtInfoMsg, ctxt, msg, ap);
    va_end(ap);
}

/*!
    Logs an informational message specified with format \a msg for the context returned
    by \a catFunc. Additional parameters, specified by \a msg, may be used.

    \since 5.5
    \sa qCInfo()
*/
void QMessageLogger::info(QMessageLogger::CategoryFunction catFunc,
                           const char *msg, ...) const
{
    const QLoggingCategory &cat = (*catFunc)();
    if (!cat.isInfoEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtInfoMsg, ctxt, msg, ap);
    va_end(ap);
}

#ifndef QT_NO_DEBUG_STREAM

/*!
    Logs an informational message using a QDebug stream.

    \since 5.5
    \sa qInfo(), QDebug
*/
QDebug QMessageLogger::info() const
{
    QDebug dbg = QDebug(QtInfoMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copyContextFrom(context);
    return dbg;
}

/*!
    Logs an informational message into the category \a cat using a QDebug stream.

    \since 5.5
    \sa qCInfo(), QDebug
*/
QDebug QMessageLogger::info(const QLoggingCategory &cat) const
{
    QDebug dbg = QDebug(QtInfoMsg);
    if (!cat.isInfoEnabled())
        dbg.stream->message_output = false;

    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    return dbg;
}

/*!
    Logs an informational message into category returned by \a catFunc using a QDebug stream.

    \since 5.5
    \sa qCInfo(), QDebug
*/
QDebug QMessageLogger::info(QMessageLogger::CategoryFunction catFunc) const
{
    return info((*catFunc)());
}

#endif

#undef qWarning
/*!
    Logs a warning message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qWarning()
*/
void QMessageLogger::warning(const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtWarningMsg, context, msg, ap);
    va_end(ap);
}

/*!
    Logs a warning message specified with format \a msg for the context \a cat.
    Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCWarning()
*/
void QMessageLogger::warning(const QLoggingCategory &cat, const char *msg, ...) const
{
    if (!cat.isWarningEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtWarningMsg, ctxt, msg, ap);
    va_end(ap);
}

/*!
    Logs a warning message specified with format \a msg for the context returned
    by \a catFunc. Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCWarning()
*/
void QMessageLogger::warning(QMessageLogger::CategoryFunction catFunc,
                             const char *msg, ...) const
{
    const QLoggingCategory &cat = (*catFunc)();
    if (!cat.isWarningEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtWarningMsg, ctxt, msg, ap);
    va_end(ap);
}

#ifndef QT_NO_DEBUG_STREAM
/*!
    Logs a warning message using a QDebug stream

    \sa qWarning(), QDebug
*/
QDebug QMessageLogger::warning() const
{
    QDebug dbg = QDebug(QtWarningMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copyContextFrom(context);
    return dbg;
}

/*!
    Logs a warning message into category \a cat using a QDebug stream.

    \sa qCWarning(), QDebug
*/
QDebug QMessageLogger::warning(const QLoggingCategory &cat) const
{
    QDebug dbg = QDebug(QtWarningMsg);
    if (!cat.isWarningEnabled())
        dbg.stream->message_output = false;

    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    return dbg;
}

/*!
    Logs a warning message into category returned by \a catFunc using a QDebug stream.

    \since 5.3
    \sa qCWarning(), QDebug
*/
QDebug QMessageLogger::warning(QMessageLogger::CategoryFunction catFunc) const
{
    return warning((*catFunc)());
}

#endif

#undef qCritical

/*!
    Logs a critical message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qCritical()
*/
void QMessageLogger::critical(const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtCriticalMsg, context, msg, ap);
    va_end(ap);
}

/*!
    Logs a critical message specified with format \a msg for the context \a cat.
    Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCCritical()
*/
void QMessageLogger::critical(const QLoggingCategory &cat, const char *msg, ...) const
{
    if (!cat.isCriticalEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtCriticalMsg, ctxt, msg, ap);
    va_end(ap);
}

/*!
    Logs a critical message specified with format \a msg for the context returned
    by \a catFunc. Additional parameters, specified by \a msg, may be used.

    \since 5.3
    \sa qCCritical()
*/
void QMessageLogger::critical(QMessageLogger::CategoryFunction catFunc,
                              const char *msg, ...) const
{
    const QLoggingCategory &cat = (*catFunc)();
    if (!cat.isCriticalEnabled())
        return;

    QMessageLogContext ctxt;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtCriticalMsg, ctxt, msg, ap);
    va_end(ap);
}

#ifndef QT_NO_DEBUG_STREAM
/*!
    Logs a critical message using a QDebug stream

    \sa qCritical(), QDebug
*/
QDebug QMessageLogger::critical() const
{
    QDebug dbg = QDebug(QtCriticalMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copyContextFrom(context);
    return dbg;
}

/*!
    Logs a critical message into category \a cat using a QDebug stream.

    \since 5.3
    \sa qCCritical(), QDebug
*/
QDebug QMessageLogger::critical(const QLoggingCategory &cat) const
{
    QDebug dbg = QDebug(QtCriticalMsg);
    if (!cat.isCriticalEnabled())
        dbg.stream->message_output = false;

    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    return dbg;
}

/*!
    Logs a critical message into category returned by \a catFunc using a QDebug stream.

    \since 5.3
    \sa qCCritical(), QDebug
*/
QDebug QMessageLogger::critical(QMessageLogger::CategoryFunction catFunc) const
{
    return critical((*catFunc)());
}

#endif

#undef qFatal

/*!
    Logs a fatal message specified with format \a msg for the context \a cat.
    Additional parameters, specified by \a msg, may be used.

    \since 6.5
    \sa qCFatal()
*/
void QMessageLogger::fatal(const QLoggingCategory &cat, const char *msg, ...) const noexcept
{
    QMessageLogContext ctxt;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    va_list ap;
    va_start(ap, msg); // use variable arg list
    QT_TERMINATE_ON_EXCEPTION(qt_message(QtFatalMsg, ctxt, msg, ap));
    va_end(ap);

#ifndef Q_CC_MSVC_ONLY
    Q_UNREACHABLE();
#endif
}

/*!
    Logs a fatal message specified with format \a msg for the context returned
    by \a catFunc. Additional parameters, specified by \a msg, may be used.

    \since 6.5
    \sa qCFatal()
*/
void QMessageLogger::fatal(QMessageLogger::CategoryFunction catFunc,
                           const char *msg, ...) const noexcept
{
    const QLoggingCategory &cat = (*catFunc)();

    QMessageLogContext ctxt;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    va_list ap;
    va_start(ap, msg); // use variable arg list
    QT_TERMINATE_ON_EXCEPTION(qt_message(QtFatalMsg, ctxt, msg, ap));
    va_end(ap);

#ifndef Q_CC_MSVC_ONLY
    Q_UNREACHABLE();
#endif
}

/*!
    Logs a fatal message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qFatal()
*/
void QMessageLogger::fatal(const char *msg, ...) const noexcept
{
    va_list ap;
    va_start(ap, msg); // use variable arg list
    QT_TERMINATE_ON_EXCEPTION(qt_message(QtFatalMsg, context, msg, ap));
    va_end(ap);

#ifndef Q_CC_MSVC_ONLY
    Q_UNREACHABLE();
#endif
}

#ifndef QT_NO_DEBUG_STREAM
/*!
    Logs a fatal message using a QDebug stream.

    \since 6.5

    \sa qFatal(), QDebug
*/
QDebug QMessageLogger::fatal() const
{
    QDebug dbg = QDebug(QtFatalMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copyContextFrom(context);
    return dbg;
}

/*!
    Logs a fatal message into category \a cat using a QDebug stream.

    \since 6.5
    \sa qCFatal(), QDebug
*/
QDebug QMessageLogger::fatal(const QLoggingCategory &cat) const
{
    QDebug dbg = QDebug(QtFatalMsg);

    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copyContextFrom(context);
    ctxt.category = cat.categoryName();

    return dbg;
}

/*!
    Logs a fatal message into category returned by \a catFunc using a QDebug stream.

    \since 6.5
    \sa qCFatal(), QDebug
*/
QDebug QMessageLogger::fatal(QMessageLogger::CategoryFunction catFunc) const
{
    return fatal((*catFunc)());
}
#endif // QT_NO_DEBUG_STREAM

/*!
    \internal
*/
Q_AUTOTEST_EXPORT QByteArray qCleanupFuncinfo(QByteArray info)
{
    // Strip the function info down to the base function name
    // note that this throws away the template definitions,
    // the parameter types (overloads) and any const/volatile qualifiers.

    if (info.isEmpty())
        return info;

    qsizetype pos;

    // Skip trailing [with XXX] for templates (gcc), but make
    // sure to not affect Objective-C message names.
    pos = info.size() - 1;
    if (info.endsWith(']') && !(info.startsWith('+') || info.startsWith('-'))) {
        while (--pos) {
          if (info.at(pos) == '[') {
              info.truncate(pos);
              break;
          }
        }
        if (info.endsWith(' ')) {
          info.chop(1);
        }
    }

    // operator names with '(', ')', '<', '>' in it
    static const char operator_call[] = "operator()";
    static const char operator_lessThan[] = "operator<";
    static const char operator_greaterThan[] = "operator>";
    static const char operator_lessThanEqual[] = "operator<=";
    static const char operator_greaterThanEqual[] = "operator>=";

    // canonize operator names
    info.replace("operator ", "operator");

    pos = -1;
    // remove argument list
    forever {
        int parencount = 0;
        pos = info.lastIndexOf(')', pos);
        if (pos == -1) {
            // Don't know how to parse this function name
            return info;
        }
        if (info.indexOf('>', pos) != -1
                || info.indexOf(':', pos) != -1) {
            // that wasn't the function argument list.
            --pos;
            continue;
        }

        // find the beginning of the argument list
        --pos;
        ++parencount;
        while (pos && parencount) {
            if (info.at(pos) == ')')
                ++parencount;
            else if (info.at(pos) == '(')
                --parencount;
            --pos;
        }
        if (parencount != 0)
            return info;

        info.truncate(++pos);

        if (info.at(pos - 1) == ')') {
            if (info.indexOf(operator_call) == pos - qsizetype(strlen(operator_call)))
                break;

            // this function returns a pointer to a function
            // and we matched the arguments of the return type's parameter list
            // try again
            info.remove(0, info.indexOf('('));
            info.chop(1);
            continue;
        } else {
            break;
        }
    }

    // find the beginning of the function name
    int parencount = 0;
    int templatecount = 0;
    --pos;

    // make sure special characters in operator names are kept
    if (pos > -1) {
        switch (info.at(pos)) {
        case ')':
            if (info.indexOf(operator_call) == pos - qsizetype(strlen(operator_call)) + 1)
                pos -= 2;
            break;
        case '<':
            if (info.indexOf(operator_lessThan) == pos - qsizetype(strlen(operator_lessThan)) + 1)
                --pos;
            break;
        case '>':
            if (info.indexOf(operator_greaterThan) == pos - qsizetype(strlen(operator_greaterThan)) + 1)
                --pos;
            break;
        case '=': {
            auto operatorLength = qsizetype(strlen(operator_lessThanEqual));
            if (info.indexOf(operator_lessThanEqual) == pos - operatorLength + 1)
                pos -= 2;
            else if (info.indexOf(operator_greaterThanEqual) == pos - operatorLength + 1)
                pos -= 2;
            break;
        }
        default:
            break;
        }
    }

    while (pos > -1) {
        if (parencount < 0 || templatecount < 0)
            return info;

        char c = info.at(pos);
        if (c == ')')
            ++parencount;
        else if (c == '(')
            --parencount;
        else if (c == '>')
            ++templatecount;
        else if (c == '<')
            --templatecount;
        else if (c == ' ' && templatecount == 0 && parencount == 0)
            break;

        --pos;
    }
    info = info.mid(pos + 1);

    // remove trailing '*', '&' that are part of the return argument
    while ((info.at(0) == '*')
           || (info.at(0) == '&'))
        info = info.mid(1);

    // we have the full function name now.
    // clean up the templates
    while ((pos = info.lastIndexOf('>')) != -1) {
        if (!info.contains('<'))
            break;

        // find the matching close
        qsizetype end = pos;
        templatecount = 1;
        --pos;
        while (pos && templatecount) {
            char c = info.at(pos);
            if (c == '>')
                ++templatecount;
            else if (c == '<')
                --templatecount;
            --pos;
        }
        ++pos;
        info.remove(pos, end - pos + 1);
    }

    return info;
}

// tokens as recognized in QT_MESSAGE_PATTERN
static const char categoryTokenC[] = "%{category}";
static const char typeTokenC[] = "%{type}";
static const char messageTokenC[] = "%{message}";
static const char fileTokenC[] = "%{file}";
static const char lineTokenC[] = "%{line}";
static const char functionTokenC[] = "%{function}";
static const char pidTokenC[] = "%{pid}";
static const char appnameTokenC[] = "%{appname}";
static const char threadidTokenC[] = "%{threadid}";
static const char qthreadptrTokenC[] = "%{qthreadptr}";
static const char timeTokenC[] = "%{time"; //not a typo: this command has arguments
static const char backtraceTokenC[] = "%{backtrace"; //ditto
static const char ifCategoryTokenC[] = "%{if-category}";
static const char ifDebugTokenC[] = "%{if-debug}";
static const char ifInfoTokenC[] = "%{if-info}";
static const char ifWarningTokenC[] = "%{if-warning}";
static const char ifCriticalTokenC[] = "%{if-critical}";
static const char ifFatalTokenC[] = "%{if-fatal}";
static const char endifTokenC[] = "%{endif}";
static const char emptyTokenC[] = "";

static const char defaultPattern[] = "%{if-category}%{category}: %{endif}%{message}";

struct QMessagePattern
{
    QMessagePattern();
    ~QMessagePattern();

    void setPattern(const QString &pattern);

    // 0 terminated arrays of literal tokens / literal or placeholder tokens
    std::unique_ptr<std::unique_ptr<const char[]>[]> literals;
    std::unique_ptr<const char *[]> tokens;
    QList<QString> timeArgs; // timeFormats in sequence of %{time
#ifndef QT_BOOTSTRAPPED
    QElapsedTimer timer;
#endif
#ifdef QLOGGING_HAVE_BACKTRACE
    struct BacktraceParams
    {
        QString backtraceSeparator;
        int backtraceDepth;
    };
    QList<BacktraceParams> backtraceArgs; // backtrace arguments in sequence of %{backtrace
#endif

    bool fromEnvironment;
    static QBasicMutex mutex;
};
#ifdef QLOGGING_HAVE_BACKTRACE
Q_DECLARE_TYPEINFO(QMessagePattern::BacktraceParams, Q_RELOCATABLE_TYPE);
#endif

Q_CONSTINIT QBasicMutex QMessagePattern::mutex;

QMessagePattern::QMessagePattern()
{
#ifndef QT_BOOTSTRAPPED
    timer.start();
#endif
    const QString envPattern = QString::fromLocal8Bit(qgetenv("QT_MESSAGE_PATTERN"));
    if (envPattern.isEmpty()) {
        setPattern(QLatin1StringView(defaultPattern));
        fromEnvironment = false;
    } else {
        setPattern(envPattern);
        fromEnvironment = true;
    }
}

QMessagePattern::~QMessagePattern() = default;

void QMessagePattern::setPattern(const QString &pattern)
{
    timeArgs.clear();
#ifdef QLOGGING_HAVE_BACKTRACE
    backtraceArgs.clear();
#endif

    // scanner
    QList<QString> lexemes;
    QString lexeme;
    bool inPlaceholder = false;
    for (int i = 0; i < pattern.size(); ++i) {
        const QChar c = pattern.at(i);
        if (c == u'%' && !inPlaceholder) {
            if ((i + 1 < pattern.size())
                    && pattern.at(i + 1) == u'{') {
                // beginning of placeholder
                if (!lexeme.isEmpty()) {
                    lexemes.append(lexeme);
                    lexeme.clear();
                }
                inPlaceholder = true;
            }
        }

        lexeme.append(c);

        if (c == u'}' && inPlaceholder) {
            // end of placeholder
            lexemes.append(lexeme);
            lexeme.clear();
            inPlaceholder = false;
        }
    }
    if (!lexeme.isEmpty())
        lexemes.append(lexeme);

    // tokenizer
    std::vector<std::unique_ptr<const char[]>> literalsVar;
    tokens.reset(new const char *[lexemes.size() + 1]);
    tokens[lexemes.size()] = nullptr;

    bool nestedIfError = false;
    bool inIf = false;
    QString error;

    for (int i = 0; i < lexemes.size(); ++i) {
        const QString lexeme = lexemes.at(i);
        if (lexeme.startsWith("%{"_L1) && lexeme.endsWith(u'}')) {
            // placeholder
            if (lexeme == QLatin1StringView(typeTokenC)) {
                tokens[i] = typeTokenC;
            } else if (lexeme == QLatin1StringView(categoryTokenC))
                tokens[i] = categoryTokenC;
            else if (lexeme == QLatin1StringView(messageTokenC))
                tokens[i] = messageTokenC;
            else if (lexeme == QLatin1StringView(fileTokenC))
                tokens[i] = fileTokenC;
            else if (lexeme == QLatin1StringView(lineTokenC))
                tokens[i] = lineTokenC;
            else if (lexeme == QLatin1StringView(functionTokenC))
                tokens[i] = functionTokenC;
            else if (lexeme == QLatin1StringView(pidTokenC))
                tokens[i] = pidTokenC;
            else if (lexeme == QLatin1StringView(appnameTokenC))
                tokens[i] = appnameTokenC;
            else if (lexeme == QLatin1StringView(threadidTokenC))
                tokens[i] = threadidTokenC;
            else if (lexeme == QLatin1StringView(qthreadptrTokenC))
                tokens[i] = qthreadptrTokenC;
            else if (lexeme.startsWith(QLatin1StringView(timeTokenC))) {
                tokens[i] = timeTokenC;
                qsizetype spaceIdx = lexeme.indexOf(QChar::fromLatin1(' '));
                if (spaceIdx > 0)
                    timeArgs.append(lexeme.mid(spaceIdx + 1, lexeme.size() - spaceIdx - 2));
                else
                    timeArgs.append(QString());
            } else if (lexeme.startsWith(QLatin1StringView(backtraceTokenC))) {
#ifdef QLOGGING_HAVE_BACKTRACE
                tokens[i] = backtraceTokenC;
                QString backtraceSeparator = QStringLiteral("|");
                int backtraceDepth = 5;
                static const QRegularExpression depthRx(QStringLiteral(" depth=(?|\"([^\"]*)\"|([^ }]*))"));
                static const QRegularExpression separatorRx(QStringLiteral(" separator=(?|\"([^\"]*)\"|([^ }]*))"));
                QRegularExpressionMatch m = depthRx.match(lexeme);
                if (m.hasMatch()) {
                    int depth = m.capturedView(1).toInt();
                    if (depth <= 0)
                        error += "QT_MESSAGE_PATTERN: %{backtrace} depth must be a number greater than 0\n"_L1;
                    else
                        backtraceDepth = depth;
                }
                m = separatorRx.match(lexeme);
                if (m.hasMatch())
                    backtraceSeparator = m.captured(1);
                BacktraceParams backtraceParams;
                backtraceParams.backtraceDepth = backtraceDepth;
                backtraceParams.backtraceSeparator = backtraceSeparator;
                backtraceArgs.append(backtraceParams);
#else
                error += "QT_MESSAGE_PATTERN: %{backtrace} is not supported by this Qt build\n"_L1;
                tokens[i] = "";
#endif
            }

#define IF_TOKEN(LEVEL) \
            else if (lexeme == QLatin1StringView(LEVEL)) { \
                if (inIf) \
                    nestedIfError = true; \
                tokens[i] = LEVEL; \
                inIf = true; \
            }
            IF_TOKEN(ifCategoryTokenC)
            IF_TOKEN(ifDebugTokenC)
            IF_TOKEN(ifInfoTokenC)
            IF_TOKEN(ifWarningTokenC)
            IF_TOKEN(ifCriticalTokenC)
            IF_TOKEN(ifFatalTokenC)
#undef IF_TOKEN
            else if (lexeme == QLatin1StringView(endifTokenC)) {
                tokens[i] = endifTokenC;
                if (!inIf && !nestedIfError)
                    error += "QT_MESSAGE_PATTERN: %{endif} without an %{if-*}\n"_L1;
                inIf = false;
            } else {
                tokens[i] = emptyTokenC;
                error += QStringLiteral("QT_MESSAGE_PATTERN: Unknown placeholder %1\n")
                        .arg(lexeme);
            }
        } else {
            using UP = std::unique_ptr<char[]>;
            tokens[i] = literalsVar.emplace_back(UP(qstrdup(lexeme.toLatin1().constData()))).get();
        }
    }
    if (nestedIfError)
        error += "QT_MESSAGE_PATTERN: %{if-*} cannot be nested\n"_L1;
    else if (inIf)
        error += "QT_MESSAGE_PATTERN: missing %{endif}\n"_L1;

    if (!error.isEmpty())
        qt_message_print(error);

    literals.reset(new std::unique_ptr<const char[]>[literalsVar.size() + 1]);
    std::move(literalsVar.begin(), literalsVar.end(), &literals[0]);
}

#if defined(QLOGGING_HAVE_BACKTRACE) && !defined(QT_BOOTSTRAPPED)
// make sure the function has "Message" in the name so the function is removed
/*
  A typical backtrace in debug mode looks like:
    #0  backtraceFramesForLogMessage (frameCount=5) at qlogging.cpp:1296
    #1  formatBacktraceForLogMessage (backtraceParams=..., function=0x4040b8 "virtual void MyClass::myFunction(int)") at qlogging.cpp:1344
    #2  qFormatLogMessage (type=QtDebugMsg, context=..., str=...) at qlogging.cpp:1452
    #3  stderr_message_handler (type=QtDebugMsg, context=..., message=...) at qlogging.cpp:1744
    #4  qDefaultMessageHandler (type=QtDebugMsg, context=..., message=...) at qlogging.cpp:1795
    #5  qt_message_print (msgType=QtDebugMsg, context=..., message=...) at qlogging.cpp:1840
    #6  qt_message_output (msgType=QtDebugMsg, context=..., message=...) at qlogging.cpp:1891
    #7  QDebug::~QDebug (this=<optimized out>, __in_chrg=<optimized out>) at qdebug.h:111
*/
static constexpr int TypicalBacktraceFrameCount = 8;

#  if defined(Q_CC_GNU) && !defined(Q_CC_CLANG)
// force skipping the frame pointer, to save the backtrace() function some work
#    pragma GCC push_options
#    pragma GCC optimize ("omit-frame-pointer")
#  endif

static QStringList backtraceFramesForLogMessage(int frameCount)
{
    struct DecodedFrame {
        QString library;
        QString function;
    };

    QStringList result;
    if (frameCount == 0)
        return result;

    QVarLengthArray<void *, 32> buffer(TypicalBacktraceFrameCount + frameCount);
    int n = backtrace(buffer.data(), buffer.size());
    if (n <= 0)
        return result;
    buffer.resize(n);

    auto shouldSkipFrame = [&result](const auto &library, const auto &function) {
        if (!result.isEmpty() || !library.contains("Qt6Core"_L1))
            return false;
        if (function.isEmpty())
            return true;
        if (function.contains("6QDebug"_L1))
            return true;
        if (function.contains("Message"_L1) || function.contains("_message"_L1))
            return true;
        return false;
    };

    auto demangled = [](auto &function) -> QString {
        if (!function.startsWith("_Z"_L1))
            return function;

        // we optimize for the case where __cxa_demangle succeeds
        auto fn = [&]() {
            if constexpr (sizeof(function.at(0)) == 1)
                return function.data();                 // -> const char *
            else
                return std::move(function).toUtf8();    // -> QByteArray
        }();
        QScopedPointer<char, QScopedPointerPodDeleter> demangled;
        demangled.reset(abi::__cxa_demangle(fn, nullptr, nullptr, nullptr));

        if (demangled)
            return QString::fromUtf8(qCleanupFuncinfo(demangled.data()));
        else
            return QString::fromUtf8(fn);       // restore
    };

#  if QT_CONFIG(dladdr)
    // use dladdr() instead of backtrace_symbols()
    QString cachedLibrary;
    const char *cachedFname = nullptr;
    auto decodeFrame = [&](void *addr) -> DecodedFrame {
        Dl_info info;
        if (!dladdr(addr, &info))
            return {};

        // These are actually UTF-8, so we'll correct below
        QLatin1StringView fn(info.dli_sname);
        QLatin1StringView lib;
        if (const char *lastSlash = strrchr(info.dli_fname, '/'))
            lib = QLatin1StringView(lastSlash + 1);
        else
            lib = QLatin1StringView(info.dli_fname);

        if (shouldSkipFrame(lib, fn))
            return {};

        QString function = demangled(fn);
        if (lib.data() != cachedFname) {
            cachedFname = lib.data();
            cachedLibrary = QString::fromUtf8(cachedFname, lib.size());
        }
        return { cachedLibrary, function };
    };
#  else
    // The results of backtrace_symbols looks like this:
    //    /lib/libc.so.6(__libc_start_main+0xf3) [0x4a937413]
    // The offset and function name are optional.
    // This regexp tries to extract the library name (without the path) and the function name.
    // This code is protected by QMessagePattern::mutex so it is thread safe on all compilers
    static const QRegularExpression rx(QStringLiteral("^(?:[^(]*/)?([^(/]+)\\(([^+]*)(?:[\\+[a-f0-9x]*)?\\) \\[[a-f0-9x]*\\]$"));

    auto decodeFrame = [&](void *&addr) -> DecodedFrame {
        QScopedPointer<char*, QScopedPointerPodDeleter> strings(backtrace_symbols(&addr, 1));
        QString trace = QString::fromUtf8(strings.data()[0]);
        QRegularExpressionMatch m = rx.match(trace);
        if (!m.hasMatch())
            return {};

        QString library = m.captured(1);
        QString function = m.captured(2);

        // skip the trace from QtCore that are because of the qDebug itself
        if (shouldSkipFrame(library, function))
            return {};

        function = demangled(function);
        return { library, function };
    };
#  endif

    for (void *&addr : buffer) {
        DecodedFrame frame = decodeFrame(addr);
        if (!frame.library.isEmpty()) {
            if (frame.function.isEmpty())
                result.append(u'?' + frame.library + u'?');
            else
                result.append(frame.function);
        } else {
            // innermost, unknown frames are usually the logging framework itself
            if (!result.isEmpty())
                result.append(QStringLiteral("???"));
        }

        if (result.size() == frameCount)
            break;
    }
    return result;
}

static QString formatBacktraceForLogMessage(const QMessagePattern::BacktraceParams backtraceParams,
                                            const char *function)
{
    QString backtraceSeparator = backtraceParams.backtraceSeparator;
    int backtraceDepth = backtraceParams.backtraceDepth;

    QStringList frames = backtraceFramesForLogMessage(backtraceDepth);
    if (frames.isEmpty())
        return QString();

    // if the first frame is unknown, replace it with the context function
    if (function && frames.at(0).startsWith(u'?'))
        frames[0] = QString::fromUtf8(qCleanupFuncinfo(function));

    return frames.join(backtraceSeparator);
}

#  if defined(Q_CC_GNU) && !defined(Q_CC_CLANG)
#    pragma GCC pop_options
#  endif
#endif // QLOGGING_HAVE_BACKTRACE && !QT_BOOTSTRAPPED

Q_GLOBAL_STATIC(QMessagePattern, qMessagePattern)

/*!
    \relates <QtLogging>
    \since 5.4

    Generates a formatted string out of the \a type, \a context, \a str arguments.

    qFormatLogMessage returns a QString that is formatted according to the current message pattern.
    It can be used by custom message handlers to format output similar to Qt's default message
    handler.

    The function is thread-safe.

    \sa qInstallMessageHandler(), qSetMessagePattern()
 */
QString qFormatLogMessage(QtMsgType type, const QMessageLogContext &context, const QString &str)
{
    QString message;

    const auto locker = qt_scoped_lock(QMessagePattern::mutex);

    QMessagePattern *pattern = qMessagePattern();
    if (!pattern) {
        // after destruction of static QMessagePattern instance
        message.append(str);
        return message;
    }

    bool skip = false;

#ifndef QT_BOOTSTRAPPED
    int timeArgsIdx = 0;
#ifdef QLOGGING_HAVE_BACKTRACE
    int backtraceArgsIdx = 0;
#endif
#endif

    // we do not convert file, function, line literals to local encoding due to overhead
    for (int i = 0; pattern->tokens[i]; ++i) {
        const char *token = pattern->tokens[i];
        if (token == endifTokenC) {
            skip = false;
        } else if (skip) {
            // we skip adding messages, but we have to iterate over
            // timeArgsIdx and backtraceArgsIdx anyway
#ifndef QT_BOOTSTRAPPED
            if (token == timeTokenC)
                timeArgsIdx++;
#ifdef QLOGGING_HAVE_BACKTRACE
            else if (token == backtraceTokenC)
                backtraceArgsIdx++;
#endif
#endif
        } else if (token == messageTokenC) {
            message.append(str);
        } else if (token == categoryTokenC) {
#ifndef Q_OS_ANDROID
            // Don't add the category to the message on Android
            message.append(QLatin1StringView(context.category));
#endif
        } else if (token == typeTokenC) {
            switch (type) {
            case QtDebugMsg:   message.append("debug"_L1); break;
            case QtInfoMsg:    message.append("info"_L1); break;
            case QtWarningMsg: message.append("warning"_L1); break;
            case QtCriticalMsg:message.append("critical"_L1); break;
            case QtFatalMsg:   message.append("fatal"_L1); break;
            }
        } else if (token == fileTokenC) {
            if (context.file)
                message.append(QLatin1StringView(context.file));
            else
                message.append("unknown"_L1);
        } else if (token == lineTokenC) {
            message.append(QString::number(context.line));
        } else if (token == functionTokenC) {
            if (context.function)
                message.append(QString::fromLatin1(qCleanupFuncinfo(context.function)));
            else
                message.append("unknown"_L1);
#ifndef QT_BOOTSTRAPPED
        } else if (token == pidTokenC) {
            message.append(QString::number(QCoreApplication::applicationPid()));
        } else if (token == appnameTokenC) {
            message.append(QCoreApplication::applicationName());
        } else if (token == threadidTokenC) {
            // print the TID as decimal
            message.append(QString::number(qt_gettid()));
        } else if (token == qthreadptrTokenC) {
            message.append("0x"_L1);
            message.append(QString::number(qlonglong(QThread::currentThread()->currentThread()), 16));
#ifdef QLOGGING_HAVE_BACKTRACE
        } else if (token == backtraceTokenC) {
            QMessagePattern::BacktraceParams backtraceParams = pattern->backtraceArgs.at(backtraceArgsIdx);
            backtraceArgsIdx++;
            message.append(formatBacktraceForLogMessage(backtraceParams, context.function));
#endif
        } else if (token == timeTokenC) {
            QString timeFormat = pattern->timeArgs.at(timeArgsIdx);
            timeArgsIdx++;
            if (timeFormat == "process"_L1) {
                quint64 ms = pattern->timer.elapsed();
                message.append(QString::asprintf("%6d.%03d", uint(ms / 1000), uint(ms % 1000)));
            } else if (timeFormat == "boot"_L1) {
                // just print the milliseconds since the elapsed timer reference
                // like the Linux kernel does
                qint64 ms = QDeadlineTimer::current().deadline();
                message.append(QString::asprintf("%6d.%03d", uint(ms / 1000), uint(ms % 1000)));
#if QT_CONFIG(datestring)
            } else if (timeFormat.isEmpty()) {
                    message.append(QDateTime::currentDateTime().toString(Qt::ISODate));
            } else {
                message.append(QDateTime::currentDateTime().toString(timeFormat));
#endif // QT_CONFIG(datestring)
            }
#endif // !QT_BOOTSTRAPPED
        } else if (token == ifCategoryTokenC) {
            if (isDefaultCategory(context.category))
                skip = true;
#define HANDLE_IF_TOKEN(LEVEL)  \
        } else if (token == if##LEVEL##TokenC) { \
            skip = type != Qt##LEVEL##Msg;
        HANDLE_IF_TOKEN(Debug)
        HANDLE_IF_TOKEN(Info)
        HANDLE_IF_TOKEN(Warning)
        HANDLE_IF_TOKEN(Critical)
        HANDLE_IF_TOKEN(Fatal)
#undef HANDLE_IF_TOKEN
        } else {
            message.append(QLatin1StringView(token));
        }
    }
    return message;
}

static void qDefaultMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &buf);

// pointer to QtMessageHandler debug handler (with context)
Q_CONSTINIT static QBasicAtomicPointer<void (QtMsgType, const QMessageLogContext &, const QString &)> messageHandler = Q_BASIC_ATOMIC_INITIALIZER(nullptr);

// ------------------------ Alternate logging sinks -------------------------

#if defined(QT_BOOTSTRAPPED)
    // Bootstrapped tools always print to stderr, so no need for alternate sinks
#else

#if QT_CONFIG(slog2)
#ifndef QT_LOG_CODE
#define QT_LOG_CODE 9000
#endif

static bool slog2_default_handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler

    QString formattedMessage = qFormatLogMessage(type, context, message);
    formattedMessage.append(u'\n');
    if (slog2_set_default_buffer((slog2_buffer_t)-1) == 0) {
        slog2_buffer_set_config_t buffer_config;
        slog2_buffer_t buffer_handle;

        buffer_config.buffer_set_name = __progname;
        buffer_config.num_buffers = 1;
        buffer_config.verbosity_level = SLOG2_DEBUG1;
        buffer_config.buffer_config[0].buffer_name = "default";
        buffer_config.buffer_config[0].num_pages = 8;

        if (slog2_register(&buffer_config, &buffer_handle, 0) == -1) {
            fprintf(stderr, "Error registering slogger2 buffer!\n");
            fprintf(stderr, "%s", formattedMessage.toLocal8Bit().constData());
            fflush(stderr);
            return false;
        }

        // Set as the default buffer
        slog2_set_default_buffer(buffer_handle);
    }
    int severity = SLOG2_INFO;
    //Determines the severity level
    switch (type) {
    case QtDebugMsg:
        severity = SLOG2_DEBUG1;
        break;
    case QtInfoMsg:
        severity = SLOG2_INFO;
        break;
    case QtWarningMsg:
        severity = SLOG2_NOTICE;
        break;
    case QtCriticalMsg:
        severity = SLOG2_WARNING;
        break;
    case QtFatalMsg:
        severity = SLOG2_ERROR;
        break;
    }
    //writes to the slog2 buffer
    slog2c(NULL, QT_LOG_CODE, severity, formattedMessage.toLocal8Bit().constData());

    return true; // Prevent further output to stderr
}
#endif // slog2

#if QT_CONFIG(journald)
static bool systemd_default_message_handler(QtMsgType type,
                                            const QMessageLogContext &context,
                                            const QString &message)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler

    QString formattedMessage = qFormatLogMessage(type, context, message);

    int priority = LOG_INFO; // Informational
    switch (type) {
    case QtDebugMsg:
        priority = LOG_DEBUG; // Debug-level messages
        break;
    case QtInfoMsg:
        priority = LOG_INFO; // Informational conditions
        break;
    case QtWarningMsg:
        priority = LOG_WARNING; // Warning conditions
        break;
    case QtCriticalMsg:
        priority = LOG_CRIT; // Critical conditions
        break;
    case QtFatalMsg:
        priority = LOG_ALERT; // Action must be taken immediately
        break;
    }

    sd_journal_send("MESSAGE=%s",     formattedMessage.toUtf8().constData(),
                    "PRIORITY=%i",    priority,
                    "CODE_FUNC=%s",   context.function ? context.function : "unknown",
                    "CODE_LINE=%d",   context.line,
                    "CODE_FILE=%s",   context.file ? context.file : "unknown",
                    "QT_CATEGORY=%s", context.category ? context.category : "unknown",
                    NULL);

    return true; // Prevent further output to stderr
}
#endif

#if QT_CONFIG(syslog)
static bool syslog_default_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler

    QString formattedMessage = qFormatLogMessage(type, context, message);

    int priority = LOG_INFO; // Informational
    switch (type) {
    case QtDebugMsg:
        priority = LOG_DEBUG; // Debug-level messages
        break;
    case QtInfoMsg:
        priority = LOG_INFO; // Informational conditions
        break;
    case QtWarningMsg:
        priority = LOG_WARNING; // Warning conditions
        break;
    case QtCriticalMsg:
        priority = LOG_CRIT; // Critical conditions
        break;
    case QtFatalMsg:
        priority = LOG_ALERT; // Action must be taken immediately
        break;
    }

    syslog(priority, "%s", formattedMessage.toUtf8().constData());

    return true; // Prevent further output to stderr
}
#endif

#ifdef Q_OS_ANDROID
static bool android_default_message_handler(QtMsgType type,
                                  const QMessageLogContext &context,
                                  const QString &message)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler

    QString formattedMessage = qFormatLogMessage(type, context, message);

    android_LogPriority priority = ANDROID_LOG_DEBUG;
    switch (type) {
    case QtDebugMsg:
        priority = ANDROID_LOG_DEBUG;
        break;
    case QtInfoMsg:
        priority = ANDROID_LOG_INFO;
        break;
    case QtWarningMsg:
        priority = ANDROID_LOG_WARN;
        break;
    case QtCriticalMsg:
        priority = ANDROID_LOG_ERROR;
        break;
    case QtFatalMsg:
        priority = ANDROID_LOG_FATAL;
        break;
    };

    // If application name is a tag ensure it has no spaces
    // If a category is defined, use it as an Android logging tag
    __android_log_print(priority, isDefaultCategory(context.category) ?
                        qPrintable(QCoreApplication::applicationName().replace(u' ', u'_')) : context.category,
                        "%s\n", qPrintable(formattedMessage));

    return true; // Prevent further output to stderr
}
#endif //Q_OS_ANDROID

#ifdef Q_OS_WIN
static void win_outputDebugString_helper(const QString &message)
{
    const qsizetype maxOutputStringLength = 32766;
    Q_CONSTINIT static QBasicMutex m;
    auto locker = qt_unique_lock(m);
    // fast path: Avoid string copies if one output is enough
    if (message.length() <= maxOutputStringLength) {
        OutputDebugString(reinterpret_cast<const wchar_t *>(message.utf16()));
    } else {
        wchar_t *messagePart = new wchar_t[maxOutputStringLength + 1];
        for (qsizetype i = 0; i < message.length(); i += maxOutputStringLength) {
            const qsizetype length = qMin(message.length() - i, maxOutputStringLength);
            const qsizetype len = QStringView{message}.mid(i, length).toWCharArray(messagePart);
            Q_ASSERT(len == length);
            messagePart[len] = 0;
            OutputDebugString(messagePart);
        }
        delete[] messagePart;
    }
}

static bool win_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler

    const QString formattedMessage = qFormatLogMessage(type, context, message).append(u'\n');
    win_outputDebugString_helper(formattedMessage);

    return true; // Prevent further output to stderr
}
#endif

#ifdef Q_OS_WASM
static bool wasm_default_message_handler(QtMsgType type,
                                  const QMessageLogContext &context,
                                  const QString &message)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler

    QString formattedMessage = qFormatLogMessage(type, context, message);
    int emOutputFlags = (EM_LOG_CONSOLE | EM_LOG_DEMANGLE);
    QByteArray localMsg = message.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        break;
    case QtInfoMsg:
        break;
    case QtWarningMsg:
        emOutputFlags |= EM_LOG_WARN;
        break;
    case QtCriticalMsg:
        emOutputFlags |= EM_LOG_ERROR;
        break;
    case QtFatalMsg:
        emOutputFlags |= EM_LOG_ERROR;
    }
    emscripten_log(emOutputFlags, "%s\n", qPrintable(formattedMessage));

    return true; // Prevent further output to stderr
}
#endif

#endif // Bootstrap check

// --------------------------------------------------------------------------

static void stderr_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QString formattedMessage = qFormatLogMessage(type, context, message);

    // print nothing if message pattern didn't apply / was empty.
    // (still print empty lines, e.g. because message itself was empty)
    if (formattedMessage.isNull())
        return;

    fprintf(stderr, "%s\n", formattedMessage.toLocal8Bit().constData());
    fflush(stderr);
}

/*!
    \internal
*/
static void qDefaultMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                   const QString &message)
{
    bool handledStderr = false;

    // A message sink logs the message to a structured or unstructured destination,
    // optionally formatting the message if the latter, and returns true if the sink
    // handled stderr output as well, which will shortcut our default stderr output.
    // In the future, if we allow multiple/dynamic sinks, this will be iterating
    // a list of sinks.

#if !defined(QT_BOOTSTRAPPED)
# if defined(Q_OS_WIN)
    handledStderr |= win_message_handler(type, context, message);
# elif QT_CONFIG(slog2)
    handledStderr |= slog2_default_handler(type, context, message);
# elif QT_CONFIG(journald)
    handledStderr |= systemd_default_message_handler(type, context, message);
# elif QT_CONFIG(syslog)
    handledStderr |= syslog_default_message_handler(type, context, message);
# elif defined(Q_OS_ANDROID)
    handledStderr |= android_default_message_handler(type, context, message);
# elif defined(QT_USE_APPLE_UNIFIED_LOGGING)
    handledStderr |= AppleUnifiedLogger::messageHandler(type, context, message);
# elif defined Q_OS_WASM
    handledStderr |= wasm_default_message_handler(type, context, message);
# endif
#endif

    if (!handledStderr)
        stderr_message_handler(type, context, message);
}

#if defined(Q_COMPILER_THREAD_LOCAL)

Q_CONSTINIT static thread_local bool msgHandlerGrabbed = false;

static bool grabMessageHandler()
{
    if (msgHandlerGrabbed)
        return false;

    msgHandlerGrabbed = true;
    return true;
}

static void ungrabMessageHandler()
{
    msgHandlerGrabbed = false;
}

#else
static bool grabMessageHandler() { return true; }
static void ungrabMessageHandler() { }
#endif // (Q_COMPILER_THREAD_LOCAL)

static void qt_message_print(QtMsgType msgType, const QMessageLogContext &context, const QString &message)
{
#ifndef QT_BOOTSTRAPPED
    Q_TRACE(qt_message_print, msgType, context.category, context.function, context.file, context.line, message);

    // qDebug, qWarning, ... macros do not check whether category is enabledgc
    if (msgType != QtFatalMsg && isDefaultCategory(context.category)) {
        if (QLoggingCategory *defaultCategory = QLoggingCategory::defaultCategory()) {
            if (!defaultCategory->isEnabled(msgType))
                return;
        }
    }
#endif

    // prevent recursion in case the message handler generates messages
    // itself, e.g. by using Qt API
    if (grabMessageHandler()) {
        const auto ungrab = qScopeGuard([]{ ungrabMessageHandler(); });
        auto msgHandler = messageHandler.loadAcquire();
        (msgHandler ? msgHandler : qDefaultMessageHandler)(msgType, context, message);
    } else {
        fprintf(stderr, "%s\n", message.toLocal8Bit().constData());
    }
}

static void qt_message_print(const QString &message)
{
#if defined(Q_OS_WIN) && !defined(QT_BOOTSTRAPPED)
    if (!shouldLogToStderr()) {
        win_outputDebugString_helper(message);
        return;
    }
#endif
    fprintf(stderr, "%s", message.toLocal8Bit().constData());
    fflush(stderr);
}

template <typename String>
static void qt_message_fatal(QtMsgType, const QMessageLogContext &context, String &&message)
{
#if defined(Q_CC_MSVC_ONLY) && defined(QT_DEBUG) && defined(_DEBUG) && defined(_CRT_ERROR)
    wchar_t contextFileL[256];
    // we probably should let the compiler do this for us, by declaring QMessageLogContext::file to
    // be const wchar_t * in the first place, but the #ifdefery above is very complex  and we
    // wouldn't be able to change it later on...
    convert_to_wchar_t_elided(contextFileL, sizeof contextFileL / sizeof *contextFileL,
                              context.file);
    // get the current report mode
    int reportMode = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
    _CrtSetReportMode(_CRT_ERROR, reportMode);

    int ret = _CrtDbgReportW(_CRT_ERROR, contextFileL, context.line, _CRT_WIDE(QT_VERSION_STR),
                             reinterpret_cast<const wchar_t *>(message.utf16()));
    if ((ret == 0) && (reportMode & _CRTDBG_MODE_WNDW))
        return; // ignore
    else if (ret == 1)
        _CrtDbgBreak();
#else
    Q_UNUSED(context);
#endif

    if constexpr (std::is_class_v<String> && !std::is_const_v<String>)
        message.clear();
    else
        Q_UNUSED(message);
    qAbort();
}

/*!
    \internal
*/
void qt_message_output(QtMsgType msgType, const QMessageLogContext &context, const QString &message)
{
    qt_message_print(msgType, context, message);
    if (isFatal(msgType))
        qt_message_fatal(msgType, context, message);
}

void qErrnoWarning(const char *msg, ...)
{
    // qt_error_string() will allocate anyway, so we don't have
    // to be careful here (like we do in plain qWarning())
    QString error_string = qt_error_string(-1);  // before vasprintf changes errno/GetLastError()

    va_list ap;
    va_start(ap, msg);
    QString buf = QString::vasprintf(msg, ap);
    va_end(ap);

    buf += " ("_L1 + error_string + u')';
    QMessageLogContext context;
    qt_message_output(QtCriticalMsg, context, buf);
}

void qErrnoWarning(int code, const char *msg, ...)
{
    // qt_error_string() will allocate anyway, so we don't have
    // to be careful here (like we do in plain qWarning())
    va_list ap;
    va_start(ap, msg);
    QString buf = QString::vasprintf(msg, ap);
    va_end(ap);

    buf += " ("_L1 + qt_error_string(code) + u')';
    QMessageLogContext context;
    qt_message_output(QtCriticalMsg, context, buf);
}

/*!
    \typedef QtMessageHandler
    \relates <QtLogging>
    \since 5.0

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/src_corelib_global_qglobal.cpp 49

    \sa QtMsgType, qInstallMessageHandler()
*/

/*!
    \fn QtMessageHandler qInstallMessageHandler(QtMessageHandler handler)
    \relates <QtLogging>
    \since 5.0

    Installs a Qt message \a handler.
    Returns a pointer to the previously installed message handler.

    A message handler is a function that prints out debug, info,
    warning, critical, and fatal messages from Qt's logging infrastructure.
    By default, Qt uses a standard message handler that formats and
    prints messages to different sinks specific to the operating system
    and Qt configuration. Installing your own message handler allows you
    to assume full control, and for instance log messages to the
    file system.

    Note that Qt supports \l{QLoggingCategory}{logging categories} for
    grouping related messages in semantic categories. You can use these
    to enable or disable logging per category and \l{QtMsgType}{message type}.
    As the filtering for logging categories is done even before a message
    is created, messages for disabled types and categories will not reach
    the message handler.

    A message handler needs to be
    \l{Reentrancy and Thread-Safety}{reentrant}. That is, it might be called
    from different threads, in parallel. Therefore, writes to common sinks
    (like a database, or a file) often need to be synchronized.

    Qt allows to enrich logging messages with further meta-information
    by calling \l qSetMessagePattern(), or setting the \c QT_MESSAGE_PATTERN
    environment variable. To keep this formatting, a custom message handler
    can use \l qFormatLogMessage().

    Try to keep the code in the message handler itself minimal, as expensive
    operations might block the application. Also, to avoid recursion, any
    logging messages generated in the message handler itself will be ignored.

    The message handler should always return. For
    \l{QtFatalMsg}{fatal messages}, the application aborts immediately after
    handling that message.

    Only one message handler can be installed at a time, for the whole application.
    If there was a previous custom message handler installed,
    the function will return a pointer to it. This handler can then
    be later reinstalled by another call to the method. Also, calling
    \c qInstallMessageHandler(nullptr) will restore the default
    message handler.

    Here is an example of a message handler that logs to a local file
    before calling the default handler:

    \snippet code/src_corelib_global_qglobal.cpp 23

    Note that the C++ standard guarantees that \c{static FILE *f} is
    initialized in a thread-safe way. We can also expect \c{fprintf()}
    and \c{fflush()} to be thread-safe, so no further synchronization
    is necessary.

    \sa QtMessageHandler, QtMsgType, qDebug(), qInfo(), qWarning(), qCritical(), qFatal(),
    {Debugging Techniques}, qFormatLogMessage()
*/

/*!
    \fn void qSetMessagePattern(const QString &pattern)
    \relates <QtLogging>
    \since 5.0

    \brief Changes the output of the default message handler.

    Allows to tweak the output of qDebug(), qInfo(), qWarning(), qCritical(),
    and qFatal(). The category logging output of qCDebug(), qCInfo(),
    qCWarning(), and qCCritical() is formatted, too.

    Following placeholders are supported:

    \table
    \header \li Placeholder \li Description
    \row \li \c %{appname} \li QCoreApplication::applicationName()
    \row \li \c %{category} \li Logging category
    \row \li \c %{file} \li Path to source file
    \row \li \c %{function} \li Function
    \row \li \c %{line} \li Line in source file
    \row \li \c %{message} \li The actual message
    \row \li \c %{pid} \li QCoreApplication::applicationPid()
    \row \li \c %{threadid} \li The system-wide ID of current thread (if it can be obtained)
    \row \li \c %{qthreadptr} \li A pointer to the current QThread (result of QThread::currentThread())
    \row \li \c %{type} \li "debug", "warning", "critical" or "fatal"
    \row \li \c %{time process} \li time of the message, in seconds since the process started (the token "process" is literal)
    \row \li \c %{time boot} \li the time of the message, in seconds since the system boot if that
        can be determined (the token "boot" is literal). If the time since boot could not be obtained,
        the output is indeterminate (see QElapsedTimer::msecsSinceReference()).
    \row \li \c %{time [format]} \li system time when the message occurred, formatted by
        passing the \c format to \l QDateTime::toString(). If the format is
        not specified, the format of Qt::ISODate is used.
    \row \li \c{%{backtrace [depth=N] [separator="..."]}} \li A backtrace with the number of frames
        specified by the optional \c depth parameter (defaults to 5), and separated by the optional
        \c separator parameter (defaults to "|").

        This expansion is available only on some platforms (currently only platfoms using glibc).
        Names are only known for exported functions. If you want to see the name of every function
        in your application, make sure your application is compiled and linked with \c{-rdynamic},
        or an equivalent of it.

        When reading backtraces, take into account that frames might be missing due to inlining or
        tail call optimization.
    \endtable

    You can also use conditionals on the type of the message using \c %{if-debug}, \c %{if-info}
    \c %{if-warning}, \c %{if-critical} or \c %{if-fatal} followed by an \c %{endif}.
    What is inside the \c %{if-*} and \c %{endif} will only be printed if the type matches.

    Finally, text inside \c %{if-category} ... \c %{endif} is only printed if the category
    is not the default one.

    Example:
    \snippet code/src_corelib_global_qlogging.cpp 0

    The default \a pattern is \c{%{if-category}%{category}: %{endif}%{message}}.

    The \a pattern can also be changed at runtime by setting the QT_MESSAGE_PATTERN
    environment variable; if both \l qSetMessagePattern() is called and QT_MESSAGE_PATTERN is
    set, the environment variable takes precedence.

    \note The information for the placeholders \c category, \c file, \c function and \c line is
    only recorded in debug builds. Alternatively, \c QT_MESSAGELOGCONTEXT can be defined
    explicitly. For more information refer to the QMessageLogContext documentation.

    \note The message pattern only applies to unstructured logging, such as the default
    \c stderr output. Structured logging such as systemd will record the message as is,
    along with as much structured information as can be captured.

    Custom message handlers can use qFormatLogMessage() to take \a pattern into account.

    \sa qInstallMessageHandler(), {Debugging Techniques}, {QLoggingCategory}, QMessageLogContext
 */

QtMessageHandler qInstallMessageHandler(QtMessageHandler h)
{
    const auto old = messageHandler.fetchAndStoreOrdered(h);
    if (old)
        return old;
    else
        return qDefaultMessageHandler;
}

void qSetMessagePattern(const QString &pattern)
{
    const auto locker = qt_scoped_lock(QMessagePattern::mutex);

    if (!qMessagePattern()->fromEnvironment)
        qMessagePattern()->setPattern(pattern);
}


/*!
    Copies context information from \a logContext into this QMessageLogContext.
    Returns a reference to this object.

    Note that the version is \b not copied, only the context information.

    \internal
*/
QMessageLogContext &QMessageLogContext::copyContextFrom(const QMessageLogContext &logContext) noexcept
{
    this->category = logContext.category;
    this->file = logContext.file;
    this->line = logContext.line;
    this->function = logContext.function;
    return *this;
}

/*!
    \fn QMessageLogger::QMessageLogger()

    Constructs a default QMessageLogger. See the other constructors to specify
    context information.
*/

/*!
    \fn QMessageLogger::QMessageLogger(const char *file, int line, const char *function)

    Constructs a QMessageLogger to record log messages for \a file at \a line
    in \a function. The is equivalent to QMessageLogger(file, line, function, "default")
*/
/*!
    \fn QMessageLogger::QMessageLogger(const char *file, int line, const char *function, const char *category)

    Constructs a QMessageLogger to record \a category messages for \a file at \a line
    in \a function.
*/

/*!
    \fn void QMessageLogger::noDebug(const char *, ...) const
    \internal

    Ignores logging output

    \sa QNoDebug, qDebug()
*/

/*!
    \fn QMessageLogContext::QMessageLogContext()
    \internal

    Constructs a QMessageLogContext
*/

/*!
    \fn QMessageLogContext::QMessageLogContext(const char *fileName, int lineNumber, const char *functionName, const char *categoryName)
    \internal

    Constructs a QMessageLogContext with for file \a fileName at line
    \a lineNumber, in function \a functionName, and category \a categoryName.
*/

/*!
    \macro qDebug(const char *message, ...)
    \relates <QtLogging>
    \threadsafe

    Calls the message handler with the debug message \a message. If no
    message handler has been installed, the message is printed to
    stderr. Under Windows the message is sent to the console, if it is a
    console application; otherwise, it is sent to the debugger. On QNX, the
    message is sent to slogger2. This function does nothing if \c QT_NO_DEBUG_OUTPUT
    was defined during compilation.

    If you pass the function a format string and a list of arguments,
    it works in similar way to the C printf() function. The format
    should be a Latin-1 string.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 24

    If you include \c <QtDebug>, a more convenient syntax is also
    available:

    \snippet code/src_corelib_global_qglobal.cpp 25

    With this syntax, the function returns a QDebug object that is
    configured to use the QtDebugMsg message type. It automatically
    puts a single space between each item, and outputs a newline at
    the end. It supports many C++ and Qt types.

    To suppress the output at runtime, install your own message handler
    with qInstallMessageHandler().

    \sa qInfo(), qWarning(), qCritical(), qFatal(), qInstallMessageHandler(),
        {Debugging Techniques}
*/

/*!
    \macro qInfo(const char *message, ...)
    \relates <QtLogging>
    \threadsafe
    \since 5.5

    Calls the message handler with the informational message \a message. If no
    message handler has been installed, the message is printed to
    stderr. Under Windows, the message is sent to the console, if it is a
    console application; otherwise, it is sent to the debugger. On QNX the
    message is sent to slogger2. This function does nothing if \c QT_NO_INFO_OUTPUT
    was defined during compilation.

    If you pass the function a format string and a list of arguments,
    it works in similar way to the C printf() function. The format
    should be a Latin-1 string.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qInfo_printf

    If you include \c <QtDebug>, a more convenient syntax is also
    available:

    \snippet code/src_corelib_global_qglobal.cpp qInfo_stream

    With this syntax, the function returns a QDebug object that is
    configured to use the QtInfoMsg message type. It automatically
    puts a single space between each item, and outputs a newline at
    the end. It supports many C++ and Qt types.

    To suppress the output at runtime, install your own message handler
    using qInstallMessageHandler().

    \sa qDebug(), qWarning(), qCritical(), qFatal(), qInstallMessageHandler(),
        {Debugging Techniques}
*/

/*!
    \macro qWarning(const char *message, ...)
    \relates <QtLogging>
    \threadsafe

    Calls the message handler with the warning message \a message. If no
    message handler has been installed, the message is printed to
    stderr. Under Windows, the message is sent to the debugger.
    On QNX the message is sent to slogger2.

    This function takes a format string and a list of arguments,
    similar to the C printf() function. The format should be a Latin-1
    string.

    Example:
    \snippet code/src_corelib_global_qglobal.cpp 26

    If you include <QtDebug>, a more convenient syntax is
    also available:

    \snippet code/src_corelib_global_qglobal.cpp 27

    This syntax inserts a space between each item, and
    appends a newline at the end.

    This function does nothing if \c QT_NO_WARNING_OUTPUT was defined
    during compilation.
    To suppress the output at runtime, you can set
    \l{QLoggingCategory}{logging rules} or register a custom
    \l{QLoggingCategory::installFilter()}{filter}.

    For debugging purposes, it is sometimes convenient to let the
    program abort for warning messages. This allows you then
    to inspect the core dump, or attach a debugger - see also \l{qFatal()}.
    To enable this, set the environment variable \c{QT_FATAL_WARNINGS}
    to a number \c n. The program terminates then for the n-th warning.
    That is, if the environment variable is set to 1, it will terminate
    on the first call; if it contains the value 10, it will exit on the 10th
    call. Any non-numeric value in the environment variable is equivalent to 1.

    \sa qDebug(), qInfo(), qCritical(), qFatal(), qInstallMessageHandler(),
        {Debugging Techniques}
*/

/*!
    \macro qCritical(const char *message, ...)
    \relates <QtLogging>
    \threadsafe

    Calls the message handler with the critical message \a message. If no
    message handler has been installed, the message is printed to
    stderr. Under Windows, the message is sent to the debugger.
    On QNX the message is sent to slogger2.

    This function takes a format string and a list of arguments,
    similar to the C printf() function. The format should be a Latin-1
    string.

    Example:
    \snippet code/src_corelib_global_qglobal.cpp 28

    If you include <QtDebug>, a more convenient syntax is
    also available:

    \snippet code/src_corelib_global_qglobal.cpp 29

    A space is inserted between the items, and a newline is
    appended at the end.

    To suppress the output at runtime, you can define
    \l{QLoggingCategory}{logging rules} or register a custom
    \l{QLoggingCategory::installFilter()}{filter}.

    For debugging purposes, it is sometimes convenient to let the
    program abort for critical messages. This allows you then
    to inspect the core dump, or attach a debugger - see also \l{qFatal()}.
    To enable this, set the environment variable \c{QT_FATAL_CRITICALS}
    to a number \c n. The program terminates then for the n-th critical
    message.
    That is, if the environment variable is set to 1, it will terminate
    on the first call; if it contains the value 10, it will exit on the 10th
    call. Any non-numeric value in the environment variable is equivalent to 1.

    \sa qDebug(), qInfo(), qWarning(), qFatal(), qInstallMessageHandler(),
        {Debugging Techniques}
*/

/*!
    \macro qFatal(const char *message, ...)
    \relates <QtLogging>

    Calls the message handler with the fatal message \a message. If no
    message handler has been installed, the message is printed to
    stderr. Under Windows, the message is sent to the debugger.
    On QNX the message is sent to slogger2.

    If you are using the \b{default message handler} this function will
    abort to create a core dump. On Windows, for debug builds,
    this function will report a _CRT_ERROR enabling you to connect a debugger
    to the application.

    This function takes a format string and a list of arguments,
    similar to the C printf() function.

    Example:
    \snippet code/src_corelib_global_qglobal.cpp 30

    To suppress the output at runtime, install your own message handler
    with qInstallMessageHandler().

    \sa qDebug(), qInfo(), qWarning(), qCritical(), qInstallMessageHandler(),
        {Debugging Techniques}
*/

/*!
    \enum QtMsgType
    \relates <QtLogging>

    This enum describes the messages that can be sent to a message
    handler (QtMessageHandler). You can use the enum to identify and
    associate the various message types with the appropriate
    actions.

    \value QtDebugMsg
           A message generated by the qDebug() function.
    \value QtInfoMsg
           A message generated by the qInfo() function.
    \value QtWarningMsg
           A message generated by the qWarning() function.
    \value QtCriticalMsg
           A message generated by the qCritical() function.
    \value QtFatalMsg
           A message generated by the qFatal() function.
    \value QtSystemMsg

    \c QtInfoMsg was added in Qt 5.5.

    \sa QtMessageHandler, qInstallMessageHandler()
*/

QT_END_NAMESPACE
