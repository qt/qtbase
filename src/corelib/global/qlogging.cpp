// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlogging.h"
#include "qlogging_p.h"

#include "qbytearray.h"
#include "qlist.h"
#include "qcoreapplication.h"
#include "private/qcoreapplication_p.h"
#include "qdatetime.h"
#include "qdebug.h"
#include "qgettid_p.h"
#include "private/qlocking_p.h"
#include "qloggingcategory.h"
#include "private/qloggingregistry_p.h"
#include "qmutex.h"
#include "qscopeguard.h"
#include "qstring.h"
#include "qtcore_tracepoints_p.h"
#include "qthread.h"
#include "qvarlengtharray.h"

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

#ifdef QLOGGING_HAVE_BACKTRACE
#  include <qregularexpression.h>
#endif

#ifdef QLOGGING_USE_EXECINFO_BACKTRACE
#  if QT_CONFIG(dladdr)
#    include <dlfcn.h>
#  endif
#  include BACKTRACE_HEADER
#  include <cxxabi.h>
#endif // QLOGGING_USE_EXECINFO_BACKTRACE

#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <memory>
#include <vector>

#include <stdio.h>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <processthreadsapi.h>
#include "qfunctionpointer.h"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_TRACE_POINT(qtcore, qt_message_print, int type, const char *category, const char *function, const char *file, int line, const QString &message);

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

    \sa QLoggingCategory
*/

template <typename String>
static void qt_maybe_message_fatal(QtMsgType, const QMessageLogContext &context, String &&message);
static void qt_message_print(QtMsgType, const QMessageLogContext &context, const QString &message);
static void preformattedMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                       const QString &formattedMessage);
static QString formatLogMessage(QtMsgType type, const QMessageLogContext &context, const QString &str);

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
    return (ok && value >= 0) ? value : 1;
}

static bool isFatalCountDown(const char *varname, QBasicAtomicInt &n)
{
    static const int Uninitialized = 0;
    static const int NeverFatal = 1;
    static const int ImmediatelyFatal = 2;

    int v = n.loadRelaxed();
    if (v == Uninitialized) {
        // first, initialize from the environment
        // note that the atomic stores the env.var value plus 1, so adjust
        const int env = checked_var_value(varname) + 1;
        if (env == NeverFatal) {
            // not fatal, now or in the future, so use a fast path
            n.storeRelaxed(NeverFatal);
            return false;
        } else if (env == ImmediatelyFatal) {
            return true;
        } else if (n.testAndSetRelaxed(Uninitialized, env - 1, v)) {
            return false;       // not yet fatal, but decrement
        } else {
            // some other thread initialized before we did
        }
    }

    while (v > ImmediatelyFatal && !n.testAndSetRelaxed(v, v - 1, v))
        qYieldCpu();

    // We exited the loop, so either v already was ImmediatelyFatal or we
    // succeeded to set n from v to v-1.
    return v == ImmediatelyFatal;
}

Q_CONSTINIT static QBasicAtomicInt fatalCriticalsCount = Q_BASIC_ATOMIC_INITIALIZER(0);
Q_CONSTINIT static QBasicAtomicInt fatalWarningsCount = Q_BASIC_ATOMIC_INITIALIZER(0);
static bool isFatal(QtMsgType msgType)
{
    switch (msgType){
    case QtFatalMsg:
        return true;    // always fatal

    case QtCriticalMsg:
        return isFatalCountDown("QT_FATAL_CRITICALS", fatalCriticalsCount);

    case QtWarningMsg:
        return isFatalCountDown("QT_FATAL_WARNINGS", fatalWarningsCount);

    case QtDebugMsg:
    case QtInfoMsg:
        break;  // never fatal
    }

    return false;
}

#if defined(Q_OS_LINUX) || defined(Q_OS_DARWIN) || defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
static bool qt_append_thread_name_to(QString &message)
{
    std::array<char, 16> name{};
    if (pthread_getname_np(pthread_self(), name.data(), name.size()) == 0) {
        QUtf8StringView threadName(name.data());
        if (!threadName.isEmpty()) {
            message.append(threadName);
            return true;
        }
    }
    return false;
}
#elif defined(Q_OS_WIN)
typedef HRESULT (WINAPI *GetThreadDescriptionFunc)(HANDLE, PWSTR *);
static bool qt_append_thread_name_to(QString &message)
{
    // Once MinGW 12.0 is required for Qt, we can call GetThreadDescription directly
    // instead of this runtime resolve:
    static GetThreadDescriptionFunc pGetThreadDescription = []() -> GetThreadDescriptionFunc {
        HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
        if (!hKernel)
            return nullptr;
        auto funcPtr = reinterpret_cast<QFunctionPointer>(GetProcAddress(hKernel, "GetThreadDescription"));
        return reinterpret_cast<GetThreadDescriptionFunc>(funcPtr);
    } ();
    if (!pGetThreadDescription)
        return false; // Not available on this system
    PWSTR description = nullptr;
    HRESULT hr = pGetThreadDescription(GetCurrentThread(), &description);
    std::unique_ptr<WCHAR, decltype(&LocalFree)> descriptionOwner(description, &LocalFree);
    if (SUCCEEDED(hr)) {
        QStringView threadName(description);
        if (!threadName.isEmpty()) {
            message.append(threadName);
            return true;
        }
    }
    return false;
}
#else
static bool qt_append_thread_name_to(QString &message)
{
    Q_UNUSED(message)
    return false;
}
#endif

#ifndef Q_OS_WASM

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

#endif // ifndef Q_OS_WASM

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
    qt_maybe_message_fatal(msgType, context, buf);
}

/*!
    Logs a debug message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qDebug()
*/
void QMessageLogger::debug(const char *msg, ...) const
{
    QInternalMessageLogContext ctxt(context);
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtDebugMsg, ctxt, msg, ap);
    va_end(ap);
}

/*!
    Logs an informational message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qInfo()
    \since 5.5
*/
void QMessageLogger::info(const char *msg, ...) const
{
    QInternalMessageLogContext ctxt(context);
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtInfoMsg, ctxt, msg, ap);
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

    \sa QLoggingCategory
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

    QInternalMessageLogContext ctxt(context, cat());

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

    QInternalMessageLogContext ctxt(context, cat());

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

    QInternalMessageLogContext ctxt(context, cat());

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

    QInternalMessageLogContext ctxt(context, cat());

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

/*!
    Logs a warning message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qWarning()
*/
void QMessageLogger::warning(const char *msg, ...) const
{
    QInternalMessageLogContext ctxt(context);
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtWarningMsg, ctxt, msg, ap);
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

    QInternalMessageLogContext ctxt(context, cat());

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

    QInternalMessageLogContext ctxt(context, cat());

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

/*!
    Logs a critical message specified with format \a msg. Additional
    parameters, specified by \a msg, may be used.

    \sa qCritical()
*/
void QMessageLogger::critical(const char *msg, ...) const
{
    QInternalMessageLogContext ctxt(context);
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtCriticalMsg, ctxt, msg, ap);
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

    QInternalMessageLogContext ctxt(context, cat());

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

    QInternalMessageLogContext ctxt(context, cat());

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

/*!
    Logs a fatal message specified with format \a msg for the context \a cat.
    Additional parameters, specified by \a msg, may be used.

    \since 6.5
    \sa qCFatal()
*/
void QMessageLogger::fatal(const QLoggingCategory &cat, const char *msg, ...) const noexcept
{
    QInternalMessageLogContext ctxt(context, cat());

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtFatalMsg, ctxt, msg, ap);
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

    QInternalMessageLogContext ctxt(context, cat());

    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtFatalMsg, ctxt, msg, ap);
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
    QInternalMessageLogContext ctxt(context);
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtFatalMsg, ctxt, msg, ap);
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

static bool isDefaultCategory(const char *category)
{
    return !category || strcmp(category, QLoggingRegistry::defaultCategoryName) == 0;
}

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
static const char threadnameTokenC[] = "%{threadname}";
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

struct QMessagePattern
{
    QMessagePattern();
    ~QMessagePattern();

    void setPattern(const QString &pattern);
    void setDefaultPattern()
    {
        const char *const defaultTokens[] = {
#ifndef Q_OS_ANDROID
            // "%{if-category}%{category}: %{endif}%{message}"
            ifCategoryTokenC,
            categoryTokenC,
            ": ",   // won't point to literals[] but that's ok
            endifTokenC,
#endif
            messageTokenC,
        };

        // we don't attempt to free the pointers, so only call from the ctor
        Q_ASSERT(!tokens);
        Q_ASSERT(!literals);

        auto ptr = new const char *[std::size(defaultTokens) + 1];
        auto end = std::copy(std::begin(defaultTokens), std::end(defaultTokens), ptr);
        *end = nullptr;
        tokens.release();
        tokens.reset(ptr);
    }

    // 0 terminated arrays of literal tokens / literal or placeholder tokens
    std::unique_ptr<std::unique_ptr<const char[]>[]> literals;
    std::unique_ptr<const char *[]> tokens;
    QList<QString> timeArgs; // timeFormats in sequence of %{time
    std::chrono::steady_clock::time_point appStartTime = std::chrono::steady_clock::now();
    struct BacktraceParams
    {
        QString backtraceSeparator;
        int backtraceDepth;
    };
#ifdef QLOGGING_HAVE_BACKTRACE
    QList<BacktraceParams> backtraceArgs; // backtrace arguments in sequence of %{backtrace
    int maxBacktraceDepth = 0;
#endif

    bool fromEnvironment;
    static QBasicMutex mutex;

#ifdef Q_OS_ANDROID
    bool containsToken(const char *token) const
    {
        for (int i = 0; tokens[i]; ++i) {
            if (tokens[i] == token)
                return true;
        }

        return false;
    }
#endif
};
Q_DECLARE_TYPEINFO(QMessagePattern::BacktraceParams, Q_RELOCATABLE_TYPE);

Q_CONSTINIT QBasicMutex QMessagePattern::mutex;

QMessagePattern::QMessagePattern()
{
    const QString envPattern = qEnvironmentVariable("QT_MESSAGE_PATTERN");
    if (envPattern.isEmpty()) {
        setDefaultPattern();
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
    maxBacktraceDepth = 0;
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
            else if (lexeme == QLatin1StringView(threadnameTokenC))
                tokens[i] = threadnameTokenC;
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
                maxBacktraceDepth = qMax(maxBacktraceDepth, backtraceDepth);
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
                error += "QT_MESSAGE_PATTERN: Unknown placeholder "_L1 + lexeme + '\n'_L1;
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

    if (!error.isEmpty()) {
        // remove the last '\n' because the sinks deal with that on their own
        error.chop(1);

        QMessageLogContext ctx(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE,
                               "QMessagePattern::setPattern", nullptr);
        preformattedMessageHandler(QtWarningMsg, ctx, error);
    }

    literals.reset(new std::unique_ptr<const char[]>[literalsVar.size() + 1]);
    std::move(literalsVar.begin(), literalsVar.end(), &literals[0]);
}

#if defined(QLOGGING_HAVE_BACKTRACE)
// make sure the function has "Message" in the name so the function is removed
/*
  A typical backtrace in debug mode looks like:
    #0  QInternalMessageLogContext::populateBacktrace (this=0x7fffffffd660, frameCount=5) at qlogging.cpp:1342
    #1  QInternalMessageLogContext::QInternalMessageLogContext (logContext=..., this=<optimized out>) at qlogging_p.h:42
    #2  QDebug::~QDebug (this=0x7fffffffdac8, __in_chrg=<optimized out>) at qdebug.cpp:160

  In release mode, the QInternalMessageLogContext constructor will be usually
  inlined. Empirical testing with GCC 13 and Clang 17 suggest they do obey the
  Q_ALWAYS_INLINE in that constructor even in debug mode and do inline it.
  Unfortunately, we can't know for sure if it has been.
*/
static constexpr int TypicalBacktraceFrameCount = 3;
static constexpr const char *QtCoreLibraryName = "Qt" QT_STRINGIFY(QT_VERSION_MAJOR) "Core";

#if defined(QLOGGING_USE_STD_BACKTRACE)
Q_NEVER_INLINE void QInternalMessageLogContext::populateBacktrace(int frameCount)
{
    assert(frameCount >= 0);
    backtrace = std::stacktrace::current(0, TypicalBacktraceFrameCount + frameCount);
}

static QStringList
backtraceFramesForLogMessage(int frameCount,
                             const QInternalMessageLogContext::BacktraceStorage &buffer)
{
    QStringList result;
    result.reserve(buffer.size());

    const auto shouldSkipFrame = [](QByteArrayView description)
    {
#if defined(_MSVC_STL_VERSION)
        const auto libraryNameEnd = description.indexOf('!');
        if (libraryNameEnd != -1) {
            const auto libraryName = description.first(libraryNameEnd);
            if (!libraryName.contains(QtCoreLibraryName))
                return false;
        }
#endif
        if (description.contains("populateBacktrace"))
            return true;
        if (description.contains("QInternalMessageLogContext"))
            return true;
        if (description.contains("~QDebug"))
            return true;
        return false;
    };

    for (const auto &entry : buffer) {
        const std::string description = entry.description();
        if (result.isEmpty() && shouldSkipFrame(description))
            continue;
        result.append(QString::fromStdString(description));
    }

    return result;
}

#elif defined(QLOGGING_USE_EXECINFO_BACKTRACE)

Q_NEVER_INLINE void QInternalMessageLogContext::populateBacktrace(int frameCount)
{
    assert(frameCount >= 0);
    BacktraceStorage &result = backtrace.emplace(TypicalBacktraceFrameCount + frameCount);
    int n = ::backtrace(result.data(), result.size());
    if (n <= 0)
        result.clear();
    else
        result.resize(n);
}

static QStringList
backtraceFramesForLogMessage(int frameCount,
                             const QInternalMessageLogContext::BacktraceStorage &buffer)
{
    struct DecodedFrame {
        QString library;
        QString function;
    };

    QStringList result;
    if (frameCount == 0)
        return result;

    auto shouldSkipFrame = [&result](const auto &library, const auto &function) {
        if (!result.isEmpty() || !library.contains(QLatin1StringView(QtCoreLibraryName)))
            return false;
        if (function.isEmpty())
            return true;
        if (function.contains("6QDebug"_L1))
            return true;
        if (function.contains("14QMessageLogger"_L1))
            return true;
        if (function.contains("17qt_message_output"_L1))
            return true;
        if (function.contains("26QInternalMessageLogContext"_L1))
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

    for (void *const &addr : buffer) {
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
#else
#error "Internal error: backtrace enabled, but no way to gather backtraces available"
#endif // QLOGGING_USE_..._BACKTRACE

static QString formatBacktraceForLogMessage(const QMessagePattern::BacktraceParams backtraceParams,
                                            const QMessageLogContext &ctx)
{
    // do we have a backtrace stored?
    if (ctx.version <= QMessageLogContext::CurrentVersion)
        return QString();

    auto &fullctx = static_cast<const QInternalMessageLogContext &>(ctx);
    if (!fullctx.backtrace.has_value())
        return QString();

    QString backtraceSeparator = backtraceParams.backtraceSeparator;
    int backtraceDepth = backtraceParams.backtraceDepth;

    QStringList frames = backtraceFramesForLogMessage(backtraceDepth, *fullctx.backtrace);
    if (frames.isEmpty())
        return QString();

    // if the first frame is unknown, replace it with the context function
    if (ctx.function && frames.at(0).startsWith(u'?'))
        frames[0] = QString::fromUtf8(qCleanupFuncinfo(ctx.function));

    return frames.join(backtraceSeparator);
}
#else
void QInternalMessageLogContext::populateBacktrace(int)
{
    // initFrom() returns 0 to our caller, so we should never get here
    Q_UNREACHABLE();
}
#endif // !QLOGGING_HAVE_BACKTRACE

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
    return formatLogMessage(type, context, str);
}

// Separate function so the default message handler can bypass the public,
// exported function above. Static functions can't get added to the dynamic
// symbol tables, so they never show up in backtrace_symbols() or equivalent.
static QString formatLogMessage(QtMsgType type, const QMessageLogContext &context, const QString &str)
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

    int timeArgsIdx = 0;
#ifdef QLOGGING_HAVE_BACKTRACE
    int backtraceArgsIdx = 0;
#endif

    // we do not convert file, function, line literals to local encoding due to overhead
    for (int i = 0; pattern->tokens[i]; ++i) {
        const char *token = pattern->tokens[i];
        if (token == endifTokenC) {
            skip = false;
        } else if (skip) {
            // we skip adding messages, but we have to iterate over
            // timeArgsIdx and backtraceArgsIdx anyway
            if (token == timeTokenC)
                timeArgsIdx++;
#ifdef QLOGGING_HAVE_BACKTRACE
            else if (token == backtraceTokenC)
                backtraceArgsIdx++;
#endif
        } else if (token == messageTokenC) {
            message.append(str);
        } else if (token == categoryTokenC) {
            message.append(QLatin1StringView(context.category));
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
        } else if (token == pidTokenC) {
            message.append(QString::number(QCoreApplication::applicationPid()));
        } else if (token == appnameTokenC) {
            message.append(QCoreApplication::applicationName());
        } else if (token == threadidTokenC) {
            // print the TID as decimal
            message.append(QString::number(qt_gettid()));
        } else if (token == threadnameTokenC) {
            if (!qt_append_thread_name_to(message))
                message.append(QString::number(qt_gettid())); // fallback to the TID
        } else if (token == qthreadptrTokenC) {
            message.append("0x"_L1);
            message.append(QString::number(qlonglong(QThread::currentThread()->currentThread()), 16));
#ifdef QLOGGING_HAVE_BACKTRACE
        } else if (token == backtraceTokenC) {
            QMessagePattern::BacktraceParams backtraceParams = pattern->backtraceArgs.at(backtraceArgsIdx);
            backtraceArgsIdx++;
            message.append(formatBacktraceForLogMessage(backtraceParams, context));
#endif
        } else if (token == timeTokenC) {
            using namespace std::chrono;
            auto formatElapsedTime = [](steady_clock::duration time) {
                // we assume time > 0
                auto ms = duration_cast<milliseconds>(time);
                auto sec = duration_cast<seconds>(ms);
                ms -= sec;
                return QString::asprintf("%6lld.%03u", qint64(sec.count()), uint(ms.count()));
            };
            QString timeFormat = pattern->timeArgs.at(timeArgsIdx);
            timeArgsIdx++;
            if (timeFormat == "process"_L1) {
                message += formatElapsedTime(steady_clock::now() - pattern->appStartTime);
            } else if (timeFormat == "boot"_L1) {
                // just print the milliseconds since the elapsed timer reference
                // like the Linux kernel does
                message += formatElapsedTime(steady_clock::now().time_since_epoch());
#if QT_CONFIG(datestring)
            } else if (timeFormat.isEmpty()) {
                message.append(QDateTime::currentDateTime().toString(Qt::ISODate));
            } else {
                message.append(QDateTime::currentDateTime().toString(timeFormat));
#endif // QT_CONFIG(datestring)
            }
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

#if QT_CONFIG(slog2)
#ifndef QT_LOG_CODE
#define QT_LOG_CODE 9000
#endif

static bool slog2_default_handler(QtMsgType type, const QMessageLogContext &,
                                  const QString &message)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler

    QString formattedMessage = message;
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

    // Explicit QByteArray instead of auto, to resolve the QStringBuilder proxy
    const QByteArray messageField = "MESSAGE="_ba + message.toUtf8().constData();
    const QByteArray priorityField = "PRIORITY="_ba + QByteArray::number(priority);
    const QByteArray tidField = "TID="_ba + QByteArray::number(qlonglong(qt_gettid()));
    const QByteArray fileField = context.file
                                 ? "CODE_FILE="_ba + context.file : QByteArray();
    const QByteArray funcField = context.function
                                 ? "CODE_FUNC="_ba + context.function : QByteArray();
    const QByteArray lineField = context.line
                                 ? "CODE_LINE="_ba + QByteArray::number(context.line) : QByteArray();
    const QByteArray categoryField = context.category
                                 ? "QT_CATEGORY="_ba + context.category : QByteArray();

    auto toIovec = [](const QByteArray &ba) {
        return iovec{ const_cast<char*>(ba.data()), size_t(ba.size()) };
    };

    struct iovec fields[7] = {
        toIovec(messageField),
        toIovec(priorityField),
        toIovec(tidField),
    };
    int nFields = 3;
    if (context.file)
        fields[nFields++] = toIovec(fileField);
    if (context.function)
        fields[nFields++] = toIovec(funcField);
    if (context.line)
        fields[nFields++] = toIovec(lineField);
    if (context.category)
        fields[nFields++] = toIovec(categoryField);

    sd_journal_sendv(fields, nFields);

    return true; // Prevent further output to stderr
}
#endif

#if QT_CONFIG(syslog)
static bool syslog_default_message_handler(QtMsgType type, const QMessageLogContext &context,
                                           const QString &formattedMessage)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler

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
                                  const QString &formattedMessage)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler

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

    QMessagePattern *pattern = qMessagePattern();
    const QString tag = (pattern && pattern->containsToken(categoryTokenC))
        // If application name is a tag ensure it has no spaces
        ? QCoreApplication::applicationName().replace(u' ', u'_')
        : QString::fromUtf8(context.category);
    __android_log_print(priority, qPrintable(tag), "%s\n", qPrintable(formattedMessage));

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
        wchar_t messagePart[maxOutputStringLength + 1];
        for (qsizetype i = 0; i < message.length(); i += maxOutputStringLength) {
            const qsizetype length = qMin(message.length() - i, maxOutputStringLength);
            const qsizetype len = QStringView{message}.mid(i, length).toWCharArray(messagePart);
            Q_ASSERT(len == length);
            messagePart[len] = 0;
            OutputDebugString(messagePart);
        }
    }
}

static bool win_message_handler(QtMsgType, const QMessageLogContext &,
                                const QString &formattedMessage)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler

    win_outputDebugString_helper(formattedMessage + u'\n');

    return true; // Prevent further output to stderr
}
#endif

#ifdef Q_OS_WASM
static bool wasm_default_message_handler(QtMsgType type,
                                  const QMessageLogContext &,
                                  const QString &formattedMessage)
{
    static bool forceStderrLogging = qEnvironmentVariableIntValue("QT_FORCE_STDERR_LOGGING");
    if (forceStderrLogging)
        return false;

    int emOutputFlags = EM_LOG_CONSOLE;
    QByteArray localMsg = formattedMessage.toLocal8Bit();
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

// --------------------------------------------------------------------------

static void stderr_message_handler(QtMsgType type, const QMessageLogContext &context,
                                   const QString &formattedMessage)
{
    Q_UNUSED(type);
    Q_UNUSED(context);

    // print nothing if message pattern didn't apply / was empty.
    // (still print empty lines, e.g. because message itself was empty)
    if (formattedMessage.isNull())
        return;
    fprintf(stderr, "%s\n", formattedMessage.toLocal8Bit().constData());
    fflush(stderr);
}

namespace {
struct SystemMessageSink
{
    using Fn = bool(QtMsgType, const QMessageLogContext &, const QString &);
    Fn *sink;
    bool messageIsUnformatted = false;
};
}

static constexpr SystemMessageSink systemMessageSink = {
#if defined(Q_OS_WIN)
        win_message_handler
#elif QT_CONFIG(slog2)
        slog2_default_handler
#elif QT_CONFIG(journald)
        systemd_default_message_handler, true
#elif QT_CONFIG(syslog)
        syslog_default_message_handler
#elif defined(Q_OS_ANDROID)
        android_default_message_handler
#elif defined(QT_USE_APPLE_UNIFIED_LOGGING)
        AppleUnifiedLogger::messageHandler, true
#elif defined Q_OS_WASM
        wasm_default_message_handler
#else
        nullptr
#endif
};

static void preformattedMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                       const QString &formattedMessage)
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Waddress") // "the address of ~~ will never be NULL
    if (systemMessageSink.sink && systemMessageSink.sink(type, context, formattedMessage))
        return;
QT_WARNING_POP

    stderr_message_handler(type, context, formattedMessage);
}

/*!
    \internal
*/
static void qDefaultMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                   const QString &message)
{
    // A message sink logs the message to a structured or unstructured destination,
    // optionally formatting the message if the latter, and returns true if the sink
    // handled stderr output as well, which will shortcut our default stderr output.

    if (systemMessageSink.messageIsUnformatted) {
        if (systemMessageSink.sink(type, context, message))
            return;
    }

    preformattedMessageHandler(type, context, formatLogMessage(type, context, message));
}

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

static void qt_message_print(QtMsgType msgType, const QMessageLogContext &context, const QString &message)
{
    Q_TRACE(qt_message_print, msgType, context.category, context.function, context.file, context.line, message);

    // qDebug, qWarning, ... macros do not check whether category is enabledgc
    if (msgType != QtFatalMsg && isDefaultCategory(context.category)) {
        if (QLoggingCategory *defaultCategory = QLoggingCategory::defaultCategory()) {
            if (!defaultCategory->isEnabled(msgType))
                return;
        }
    }

    // prevent recursion in case the message handler generates messages
    // itself, e.g. by using Qt API
    if (grabMessageHandler()) {
        const auto ungrab = qScopeGuard([]{ ungrabMessageHandler(); });
        auto msgHandler = messageHandler.loadAcquire();
        (msgHandler ? msgHandler : qDefaultMessageHandler)(msgType, context, message);
    } else {
        stderr_message_handler(msgType, context, message);
    }
}

template <typename String> static void
qt_maybe_message_fatal(QtMsgType msgType, const QMessageLogContext &context, String &&message)
{
    if (!isFatal(msgType))
        return;
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
    QInternalMessageLogContext ctx(context);
    qt_message_print(msgType, ctx, message);
    qt_maybe_message_fatal(msgType, ctx, message);
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
    QInternalMessageLogContext context{QMessageLogContext()};
    qt_message_output(QtWarningMsg, context, buf);
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
    QInternalMessageLogContext context{QMessageLogContext()};
    qt_message_output(QtWarningMsg, context, buf);
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
    \row \li \c %{threadname} \li The current thread name (if it can be obtained, or the thread ID, since Qt 6.10)
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

        This expansion is available only on some platforms:

        \list
        \li platforms using glibc;
        \li platforms shipping C++23's \c{<stacktrace>} header (requires compiling Qt in C++23 mode).
        \endlist

        Depending on the platform, there are some restrictions on the function
        names printed by this expansion.

        On some platforms,
        names are only known for exported functions. If you want to see the name of every function
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

    \note On Android, the default \a pattern is \c{%{message}} because the category is used as
    \l{Android: log_print}{tag} since Android logcat has a dedicated field for the logging
    categories, see \l{Android: Log}{Android Logging}. If a custom \a pattern including the
    category is used, QCoreApplication::applicationName() is used as \l{Android: log_print}{tag}.

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

static void copyInternalContext(QInternalMessageLogContext *self,
                                const QMessageLogContext &logContext) noexcept
{
    if (logContext.version == self->version) {
        auto other = static_cast<const QInternalMessageLogContext *>(&logContext);
        self->backtrace = other->backtrace;
    }
}

/*!
    \internal
    Copies context information from \a logContext into this QMessageLogContext.
    Returns the number of backtrace frames that are desired.
*/
int QInternalMessageLogContext::initFrom(const QMessageLogContext &logContext)
{
    version = CurrentVersion + 1;
    copyContextFrom(logContext);

#ifdef QLOGGING_HAVE_BACKTRACE
    if (backtrace.has_value())
        return 0;       // we have a stored backtrace, no need to get it again

    // initializes the message pattern, if needed
    if (auto pattern = qMessagePattern())
        return pattern->maxBacktraceDepth;
#endif

    return 0;
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
    if (Q_UNLIKELY(version == CurrentVersion + 1))
        copyInternalContext(static_cast<QInternalMessageLogContext *>(this), logContext);
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

    \sa QLoggingCategory
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

    \sa QLoggingCategory
*/

/*!
    \macro qDebug(const char *format, ...)
    \relates <QtLogging>
    \threadsafe

    Logs debug message \a format to the central message handler.
    \a format can contain format specifiers that are
    replaced by values specificed in additional arguments.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 24

    \a format can contain format specifiers like \c {%s} for UTF-8 strings, or
    \c {%i} for integers. This is similar to how the C \c{printf()} function works.
    For more details on the formatting, see \l QString::asprintf().

    For more convenience and further type support, you can also use
    \l{QDebug::qDebug()}, which follows the streaming paradigm (similar to
     \c{std::cout} or \c{std::cerr}).

    This function does nothing if \c QT_NO_DEBUG_OUTPUT was defined during compilation.

    To suppress the output at runtime, install your own message handler
    with qInstallMessageHandler().

    \sa QDebug::qDebug(), qCDebug(), qInfo(), qWarning(), qCritical(), qFatal(),
        qInstallMessageHandler(), {Debugging Techniques}
*/

/*!
    \macro qInfo(const char *format, ...)
    \relates <QtLogging>
    \threadsafe
    \since 5.5

    Logs informational message \a format to the central message handler.
    \a format can contain format specifiers that are
    replaced by values specificed in additional arguments.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qInfo_printf

    \a format can contain format specifiers like \c {%s} for UTF-8 strings, or
    \c {%i} for integers. This is similar to how the C \c{printf()} function works.
    For more details on the formatting, see \l QString::asprintf().

    For more convenience and further type support, you can also use
    \l{QDebug::qInfo()}, which follows the streaming paradigm (similar to
    \c{std::cout} or \c{std::cerr}).

    This function does nothing if \c QT_NO_INFO_OUTPUT was defined during compilation.

    To suppress the output at runtime, install your own message handler
    using qInstallMessageHandler().

    \sa QDebug::qInfo(), qCInfo(), qDebug(), qWarning(), qCritical(), qFatal(),
        qInstallMessageHandler(), {Debugging Techniques}
*/

/*!
    \macro qWarning(const char *format, ...)
    \relates <QtLogging>
    \threadsafe

    Logs warning message \a format to the central message handler.
    \a format can contain format specifiers that are
    replaced by values specificed in additional arguments.

    Example:
    \snippet code/src_corelib_global_qglobal.cpp 26

    \a format can contain format specifiers like \c {%s} for UTF-8 strings, or
    \c {%i} for integers. This is similar to how the C \c{printf()} function works.
    For more details on the formatting, see \l QString::asprintf().

    For more convenience and further type support, you can also use
    \l{QDebug::qWarning()}, which follows the streaming paradigm (similar to
     \c{std::cout} or \c{std::cerr}).

    This function does nothing if \c QT_NO_WARNING_OUTPUT was defined
    during compilation.
    To suppress the output at runtime, you can set
    \l{QLoggingCategory}{logging rules} or register a custom
    \l{QLoggingCategory::installFilter()}{filter}.

    For debugging purposes, it is sometimes convenient to let the
    program abort for warning messages. This allows you
    to inspect the core dump, or attach a debugger - see also \l{qFatal()}.
    To enable this, set the environment variable \c{QT_FATAL_WARNINGS}
    to a number \c n. The program terminates then for the n-th warning.
    That is, if the environment variable is set to 1, it will terminate
    on the first call; if it contains the value 10, it will exit on the 10th
    call. Any non-numeric value in the environment variable is equivalent to 1.

    \sa QDebug::qWarning(), qCWarning(), qDebug(), qInfo(), qCritical(), qFatal(),
        qInstallMessageHandler(), {Debugging Techniques}
*/

/*!
    \macro qCritical(const char *format, ...)
    \relates <QtLogging>
    \threadsafe

    Logs critical message \a format to the central message handler.
    \a format can contain format specifiers that are
    replaced by values specificed in additional arguments.

    Example:
    \snippet code/src_corelib_global_qglobal.cpp 28

    \a format can contain format specifiers like \c {%s} for UTF-8 strings, or
    \c {%i} for integers. This is similar to how the C \c{printf()} function works.
    For more details on the formatting, see \l QString::asprintf().

    For more convenience and further type support, you can also use
    \l{QDebug::qCritical()}, which follows the streaming paradigm (similar to
    \c{std::cout} or \c{std::cerr}).

    To suppress the output at runtime, you can define
    \l{QLoggingCategory}{logging rules} or register a custom
    \l{QLoggingCategory::installFilter()}{filter}.

    For debugging purposes, it is sometimes convenient to let the
    program abort for critical messages. This allows you
    to inspect the core dump, or attach a debugger - see also \l{qFatal()}.
    To enable this, set the environment variable \c{QT_FATAL_CRITICALS}
    to a number \c n. The program terminates then for the n-th critical
    message.
    That is, if the environment variable is set to 1, it will terminate
    on the first call; if it contains the value 10, it will exit on the 10th
    call. Any non-numeric value in the environment variable is equivalent to 1.

    \sa QDebug::qCritical, qCCritical(), qDebug(), qInfo(), qWarning(), qFatal(),
        qInstallMessageHandler(), {Debugging Techniques}
*/

/*!
    \macro qFatal(const char *format, ...)
    \relates <QtLogging>

    Logs fatal message \a format to the central message handler.
    \a format can contain format specifiers that are
    replaced by values specificed in additional arguments.

    Example:
    \snippet code/src_corelib_global_qglobal.cpp 30

    If you are using the \b{default message handler} this function will
    abort to create a core dump. On Windows, for debug builds,
    this function will report a _CRT_ERROR enabling you to connect a debugger
    to the application.

    To suppress the output at runtime, install your own message handler
    with qInstallMessageHandler().

    \sa qCFatal(), qDebug(), qInfo(), qWarning(), qCritical(),
        qInstallMessageHandler(), {Debugging Techniques}
*/

/*!
    \enum QtMsgType
    \relates <QtLogging>

    This enum describes the messages that can be sent to a message
    handler (QtMessageHandler). You can use the enum to identify and
    associate the various message types with the appropriate
    actions. Its values are, in order of increasing severity:

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
    \omitvalue QtSystemMsg

    \sa QtMessageHandler, qInstallMessageHandler(), QLoggingCategory
*/

QT_END_NAMESPACE
