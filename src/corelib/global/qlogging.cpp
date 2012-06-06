/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlogging.h"
#include "qlist.h"
#include "qbytearray.h"
#include "qstring.h"
#include "qvarlengtharray.h"
#include "qdebug.h"
#include "qmutex.h"
#ifndef QT_BOOTSTRAPPED
#include "qcoreapplication.h"
#include "qthread.h"
#endif
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

#include <stdio.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMessageLogContext
    \relates <QtGlobal>
    \brief The QMessageLogContext class provides additional information about a log message.
    \since 5.0

    The class provides information about the source code location a qDebug(), qWarning(),
    qCritical() or qFatal() message was generated.

    \sa QMessageLogger, QtMessageHandler, qInstallMessageHandler()
*/

/*!
    \class QMessageLogger
    \relates <QtGlobal>
    \brief The QMessageLogger class generates log messages.
    \since 5.0

    QMessageLogger is used to generate messages for the Qt logging framework. Usually one uses
    it through qDebug(), qWarning(), qCritical, or qFatal() functions,
    which are actually macros that expand to QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).debug()
    et al.

    One example of direct use is to forward errors that stem from a scripting language, e.g. QML:

    \snippet code/qlogging/qlogging.cpp 1

    \sa QMessageLogContext, qDebug(), qWarning(), qCritical(), qFatal()
*/

#if defined(Q_OS_WIN) && defined(QT_BUILD_CORE_LIB)
// defined in qcoreapplication_win.cpp
extern bool usingWinMain;
#endif

#if !defined(QT_NO_EXCEPTIONS)
/*!
    \internal
    Uses a local buffer to output the message. Not locale safe + cuts off
    everything after character 255, but will work in out of memory situations.
    Stop the execution afterwards.
*/
static void qEmergencyOut(QtMsgType msgType, const char *msg, va_list ap)
{
    char emergency_buf[256] = { '\0' };
    emergency_buf[255] = '\0';
    if (msg)
        qvsnprintf(emergency_buf, 255, msg, ap);

#if defined(Q_OS_WIN) && defined(QT_BUILD_CORE_LIB)
#ifdef Q_OS_WINCE
    OutputDebugStringW(reinterpret_cast<const wchar_t *> (
                            QString::fromLatin1(emergency_buf).utf16()));
#else
    if (usingWinMain) {
        OutputDebugStringA(emergency_buf);
    } else {
        fprintf(stderr, "%s", emergency_buf);
        fflush(stderr);
    }
#endif
#else
    fprintf(stderr, "%s", emergency_buf);
    fflush(stderr);
#endif

    if (msgType == QtFatalMsg
            || (msgType == QtWarningMsg
                && (!qgetenv("QT_FATAL_WARNINGS").isNull())) ) {
#if defined(Q_CC_MSVC) && defined(QT_DEBUG) && defined(_DEBUG) && defined(_CRT_ERROR)
        // get the current report mode
        int reportMode = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
        _CrtSetReportMode(_CRT_ERROR, reportMode);
        int ret = _CrtDbgReportW(_CRT_ERROR, _CRT_WIDE(__FILE__), __LINE__,
                                 _CRT_WIDE(QT_VERSION_STR),
                                 reinterpret_cast<const wchar_t *> (
                                     QString::fromLatin1(msg).utf16()));
        if (ret == 1)
            _CrtDbgBreak();
#endif

#if (defined(Q_OS_UNIX) || defined(Q_CC_MINGW))
        abort(); // trap; generates core dump
#else
        exit(1); // goodbye cruel world
#endif
    }
}
#endif

/*!
    \internal
*/
static void qt_message(QtMsgType msgType, const QMessageLogContext &context, const char *msg,
                       va_list ap)
{
#if !defined(QT_NO_EXCEPTIONS)
    if (std::uncaught_exception()) {
        qEmergencyOut(msgType, msg, ap);
        return;
    }
#endif
    QString buf;
    if (msg) {
        QT_TRY {
            buf = QString().vsprintf(msg, ap);
        } QT_CATCH(const std::bad_alloc &) {
#if !defined(QT_NO_EXCEPTIONS)
            qEmergencyOut(msgType, msg, ap);
            // don't rethrow - we use qWarning and friends in destructors.
            return;
#endif
        }
    }
    qt_message_output(msgType, context, buf);
}

#undef qDebug
void QMessageLogger::debug(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtDebugMsg, context, msg, ap);
    va_end(ap);
}

#ifndef QT_NO_DEBUG_STREAM

QDebug QMessageLogger::debug()
{
    QDebug dbg = QDebug(QtDebugMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copy(context);
    return dbg;
}

QNoDebug QMessageLogger::noDebug()
{
    return QNoDebug();
}

#endif

#undef qWarning
void QMessageLogger::warning(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtWarningMsg, context, msg, ap);
    va_end(ap);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug QMessageLogger::warning()
{
    QDebug dbg = QDebug(QtWarningMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copy(context);
    return dbg;
}
#endif

#undef qCritical
void QMessageLogger::critical(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtCriticalMsg, context, msg, ap);
    va_end(ap);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug QMessageLogger::critical()
{
    QDebug dbg = QDebug(QtCriticalMsg);
    QMessageLogContext &ctxt = dbg.stream->context;
    ctxt.copy(context);
    return dbg;
}
#endif

#undef qFatal
void QMessageLogger::fatal(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg); // use variable arg list
    qt_message(QtFatalMsg, context, msg, ap);
#ifndef Q_CC_MSVC
    Q_UNREACHABLE();
#endif
    va_end(ap);
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

    int pos;

    // skip trailing [with XXX] for templates (gcc)
    pos = info.size() - 1;
    if (info.endsWith(']')) {
        while (--pos) {
            if (info.at(pos) == '[')
                info.truncate(pos);
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

    // remove argument list
    forever {
        int parencount = 0;
        pos = info.lastIndexOf(')');
        if (pos == -1) {
            // Don't know how to parse this function name
            return info;
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
            if (info.indexOf(operator_call) == pos - (int)strlen(operator_call))
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
            if (info.indexOf(operator_call) == pos - (int)strlen(operator_call) + 1)
                pos -= 2;
            break;
        case '<':
            if (info.indexOf(operator_lessThan) == pos - (int)strlen(operator_lessThan) + 1)
                --pos;
            break;
        case '>':
            if (info.indexOf(operator_greaterThan) == pos - (int)strlen(operator_greaterThan) + 1)
                --pos;
            break;
        case '=': {
            int operatorLength = (int)strlen(operator_lessThanEqual);
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
        int end = pos;
        templatecount = 1;
        --pos;
        while (pos && templatecount) {
            register char c = info.at(pos);
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
static const char emptyTokenC[] = "";

static const char defaultPattern[] = "%{message}";


struct QMessagePattern {
    QMessagePattern();
    ~QMessagePattern();

    void setPattern(const QString &pattern);

    // 0 terminated arrays of literal tokens / literal or placeholder tokens
    const char **literals;
    const char **tokens;

    bool fromEnvironment;
    static QBasicMutex mutex;
};

QBasicMutex QMessagePattern::mutex;

QMessagePattern::QMessagePattern()
    : literals(0)
    , tokens(0)
    , fromEnvironment(false)
{
    const QString envPattern = QString::fromLocal8Bit(qgetenv("QT_MESSAGE_PATTERN"));
    if (envPattern.isEmpty()) {
        setPattern(QLatin1String(defaultPattern));
    } else {
        setPattern(envPattern);
        fromEnvironment = true;
    }
}

QMessagePattern::~QMessagePattern()
{
    for (int i = 0; literals[i] != 0; ++i)
        delete [] literals[i];
    delete [] literals;
    literals = 0;
    delete [] tokens;
    tokens = 0;
}

void QMessagePattern::setPattern(const QString &pattern)
{
    delete [] tokens;
    delete [] literals;

    // scanner
    QList<QString> lexemes;
    QString lexeme;
    bool inPlaceholder = false;
    for (int i = 0; i < pattern.size(); ++i) {
        const QChar c = pattern.at(i);
        if ((c == QLatin1Char('%'))
                && !inPlaceholder) {
            if ((i + 1 < pattern.size())
                    && pattern.at(i + 1) == QLatin1Char('{')) {
                // beginning of placeholder
                if (!lexeme.isEmpty()) {
                    lexemes.append(lexeme);
                    lexeme.clear();
                }
                inPlaceholder = true;
            }
        }

        lexeme.append(c);

        if ((c == QLatin1Char('}') && inPlaceholder)) {
            // end of placeholder
            lexemes.append(lexeme);
            lexeme.clear();
            inPlaceholder = false;
        }
    }
    if (!lexeme.isEmpty())
        lexemes.append(lexeme);

    // tokenizer
    QVarLengthArray<const char*> literalsVar;
    tokens = new const char*[lexemes.size() + 1];
    tokens[lexemes.size()] = 0;

    for (int i = 0; i < lexemes.size(); ++i) {
        const QString lexeme = lexemes.at(i);
        if (lexeme.startsWith(QLatin1String("%{"))
                && lexeme.endsWith(QLatin1Char('}'))) {
            // placeholder
            if (lexeme == QLatin1String(typeTokenC)) {
                tokens[i] = typeTokenC;
            } else if (lexeme == QLatin1String(categoryTokenC))
                tokens[i] = categoryTokenC;
            else if (lexeme == QLatin1String(messageTokenC))
                tokens[i] = messageTokenC;
            else if (lexeme == QLatin1String(fileTokenC))
                tokens[i] = fileTokenC;
            else if (lexeme == QLatin1String(lineTokenC))
                tokens[i] = lineTokenC;
            else if (lexeme == QLatin1String(functionTokenC))
                tokens[i] = functionTokenC;
            else if (lexeme == QLatin1String(pidTokenC))
                tokens[i] = pidTokenC;
            else if (lexeme == QLatin1String(appnameTokenC))
                tokens[i] = appnameTokenC;
            else if (lexeme == QLatin1String(threadidTokenC))
                tokens[i] = threadidTokenC;
            else {
                tokens[i] = emptyTokenC;

                QString error = QStringLiteral("QT_MESSAGE_PATTERN: Unknown placeholder %1\n")
                        .arg(lexeme);

#if defined(Q_OS_WINCE)
                OutputDebugString(reinterpret_cast<const wchar_t*>(error.utf16()));
                continue;
#elif defined(Q_OS_WIN) && defined(QT_BUILD_CORE_LIB)
                if (usingWinMain) {
                    OutputDebugString(reinterpret_cast<const wchar_t*>(error.utf16()));
                    continue;
                }
#endif
                fprintf(stderr, "%s", error.toLocal8Bit().constData());
                fflush(stderr);
            }
        } else {
            char *literal = new char[lexeme.size() + 1];
            strncpy(literal, lexeme.toLatin1().constData(), lexeme.size());
            literal[lexeme.size()] = '\0';
            literalsVar.append(literal);
            tokens[i] = literal;
        }
    }
    literals = new const char*[literalsVar.size() + 1];
    literals[literalsVar.size()] = 0;
    memcpy(literals, literalsVar.constData(), literalsVar.size() * sizeof(const char*));
}

Q_GLOBAL_STATIC(QMessagePattern, qMessagePattern)

/*!
    \internal
*/
Q_CORE_EXPORT QString qMessageFormatString(QtMsgType type, const QMessageLogContext &context,
                                              const QString &str)
{
    QString message;

    QMutexLocker lock(&QMessagePattern::mutex);

    QMessagePattern *pattern = qMessagePattern();
    if (!pattern) {
        // after destruction of static QMessagePattern instance
        message.append(str);
        message.append(QLatin1Char('\n'));
        return message;
    }

    // don't print anything if pattern was empty
    if (pattern->tokens[0] == 0)
        return message;

    // we do not convert file, function, line literals to local encoding due to overhead
    for (int i = 0; pattern->tokens[i] != 0; ++i) {
        const char *token = pattern->tokens[i];
        if (token == messageTokenC) {
            message.append(str);
        } else if (token == categoryTokenC) {
            message.append(QLatin1String(context.category));
        } else if (token == typeTokenC) {
            switch (type) {
            case QtDebugMsg:   message.append(QLatin1String("debug")); break;
            case QtWarningMsg: message.append(QLatin1String("warning")); break;
            case QtCriticalMsg:message.append(QLatin1String("critical")); break;
            case QtFatalMsg:   message.append(QLatin1String("fatal")); break;
            }
        } else if (token == fileTokenC) {
            if (context.file)
                message.append(QLatin1String(context.file));
            else
                message.append(QLatin1String("unknown"));
        } else if (token == lineTokenC) {
            message.append(QString::number(context.line));
        } else if (token == functionTokenC) {
            if (context.function)
                message.append(QString::fromLatin1(qCleanupFuncinfo(context.function)));
            else
                message.append(QLatin1String("unknown"));
#ifndef QT_BOOTSTRAPPED
        } else if (token == pidTokenC) {
            message.append(QString::number(QCoreApplication::applicationPid()));
        } else if (token == appnameTokenC) {
            message.append(QCoreApplication::applicationName());
        } else if (token == threadidTokenC) {
            message.append(QLatin1String("0x"));
            message.append(QString::number(qlonglong(QThread::currentThread()->currentThread()), 16));
#endif
        } else {
            message.append(QLatin1String(token));
        }
    }
    message.append(QLatin1Char('\n'));
    return message;
}

static QtMsgHandler msgHandler = 0;                // pointer to debug handler (without context)
static QtMessageHandler messageHandler = 0;         // pointer to debug handler (with context)

/*!
    \internal
*/
static void qDefaultMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                   const QString &buf)
{
    QString logMessage = qMessageFormatString(type, context, buf);
#if defined(Q_OS_WINCE)
    OutputDebugString(reinterpret_cast<const wchar_t *> (logMessage.utf16()));
#else
    fprintf(stderr, "%s", logMessage.toLocal8Bit().constData());
    fflush(stderr);
#endif
}

/*!
    \internal
*/
static void qDefaultMessageHandler2(QtMsgType type, const QMessageLogContext &context,
                                   const char *buf)
{
    qDefaultMessageHandler(type, context, QString::fromLocal8Bit(buf));
}

/*!
    \internal
*/
static void qDefaultMsgHandler(QtMsgType type, const char *buf)
{
    QMessageLogContext emptyContext;
    qDefaultMessageHandler(type, emptyContext, QLatin1String(buf));
}

/*!
    \internal
*/
void qt_message_output(QtMsgType msgType, const QMessageLogContext &context, const QString &message)
{
    if (!msgHandler)
        msgHandler = qDefaultMsgHandler;
    if (!messageHandler)
        messageHandler = qDefaultMessageHandler;

    // prefer new message handler over the old one
    if (msgHandler == qDefaultMsgHandler
            || messageHandler != qDefaultMessageHandler) {
        (*messageHandler)(msgType, context, message);
    } else {
        (*msgHandler)(msgType, message.toLocal8Bit().constData());
    }

    if (msgType == QtFatalMsg
            || (msgType == QtWarningMsg
                && (!qgetenv("QT_FATAL_WARNINGS").isNull())) ) {

#if defined(Q_CC_MSVC) && defined(QT_DEBUG) && defined(_DEBUG) && defined(_CRT_ERROR)
        // get the current report mode
        int reportMode = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
        _CrtSetReportMode(_CRT_ERROR, reportMode);

        int ret = _CrtDbgReportW(_CRT_ERROR, reinterpret_cast<const wchar_t *> (
                                     QString::fromLatin1(context.file).utf16()),
                                 context.line, _CRT_WIDE(QT_VERSION_STR),
                                 reinterpret_cast<const wchar_t *> (
                                     message.utf16()));
        if (ret == 0  && reportMode & _CRTDBG_MODE_WNDW)
            return; // ignore
        else if (ret == 1)
            _CrtDbgBreak();
#endif

#if (defined(Q_OS_UNIX) || defined(Q_CC_MINGW))
        abort(); // trap; generates core dump
#else
        exit(1); // goodbye cruel world
#endif
    }
}

void qErrnoWarning(const char *msg, ...)
{
    // qt_error_string() will allocate anyway, so we don't have
    // to be careful here (like we do in plain qWarning())
    QString buf;
    va_list ap;
    va_start(ap, msg);
    if (msg)
        buf.vsprintf(msg, ap);
    va_end(ap);

    buf += QLatin1String(" (") + qt_error_string(-1) + QLatin1Char(')');
    QMessageLogContext context;
    qt_message_output(QtCriticalMsg, context, buf);
}

void qErrnoWarning(int code, const char *msg, ...)
{
    // qt_error_string() will allocate anyway, so we don't have
    // to be careful here (like we do in plain qWarning())
    QString buf;
    va_list ap;
    va_start(ap, msg);
    if (msg)
        buf.vsprintf(msg, ap);
    va_end(ap);

    buf += QLatin1String(" (") + qt_error_string(code) + QLatin1Char(')');
    QMessageLogContext context;
    qt_message_output(QtCriticalMsg, context, buf);
}

#if defined(Q_OS_WIN) && defined(QT_BUILD_CORE_LIB)
extern Q_CORE_EXPORT void qWinMsgHandler(QtMsgType t, const char *str);
extern Q_CORE_EXPORT void qWinMessageHandler(QtMsgType t, const QMessageLogContext &context,
                                             const QString &str);

void qWinMessageHandler2(QtMsgType t, const QMessageLogContext &context,
                         const char *str)
{
    qWinMessageHandler(t, context, QString::fromLocal8Bit(str));
}
#endif

/*!
    \typedef QtMsgHandler
    \relates <QtGlobal>
    \deprecated

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/src_corelib_global_qglobal.cpp 7

    This typedef is deprecated, you should use QtMessageHandler instead.
    \sa QtMsgType, QtMessageHandler, qInstallMsgHandler(), qInstallMessageHandler()
*/

/*!
    \typedef QtMessageHandler
    \relates <QtGlobal>
    \since 5.0

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/src_corelib_global_qglobal.cpp 49

    \sa QtMsgType, qInstallMessageHandler()
*/

/*!
    \fn QtMessageHandler qInstallMessageHandler(QtMessageHandler handler)
    \relates <QtGlobal>
    \since 5.0

    Installs a Qt message \a handler which has been defined
    previously. Returns a pointer to the previous message handler
    (which may be 0).

    The message handler is a function that prints out debug messages,
    warnings, critical and fatal error messages. The Qt library (debug
    mode) contains hundreds of warning messages that are printed
    when internal errors (usually invalid function arguments)
    occur. Qt built in release mode also contains such warnings unless
    QT_NO_WARNING_OUTPUT and/or QT_NO_DEBUG_OUTPUT have been set during
    compilation. If you implement your own message handler, you get total
    control of these messages.

    The default message handler prints the message to the standard
    output under X11 or to the debugger under Windows. If it is a
    fatal message, the application aborts immediately.

    Only one message handler can be defined, since this is usually
    done on an application-wide basis to control debug output.

    To restore the message handler, call \c qInstallMessageHandler(0).

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 23

    \sa QtMessageHandler, QtMsgType, qDebug(), qWarning(), qCritical(), qFatal(),
    {Debugging Techniques}
*/

/*!
    \fn QtMsgHandler qInstallMsgHandler(QtMsgHandler handler)
    \relates <QtGlobal>
    \deprecated

    Installs a Qt message \a handler which has been defined
    previously. This method is deprecated, use qInstallMessageHandler
    instead.
    \sa QtMsgHandler, qInstallMessageHandler
*/
/*!
    \fn void qSetMessagePattern(const QString &pattern)
    \relates <QtGlobal>
    \since 5.0

    \brief Changes the output of the default message handler.

    Allows to tweak the output of qDebug(), qWarning(), qCritical() and qFatal().

    Following placeholders are supported:

    \table
    \header \li Placeholder \li Description
    \row \li \c %{appname} \li QCoreApplication::applicationName()
    \row \li \c %{file} \li Path to source file
    \row \li \c %{function} \li Function
    \row \li \c %{line} \li Line in source file
    \row \li \c %{message} \li The actual message
    \row \li \c %{pid} \li QCoreApplication::applicationPid()
    \row \li \c %{threadid} \li ID of current thread
    \row \li \c %{type} \li "debug", "warning", "critical" or "fatal"
    \endtable

    The default pattern is "%{message}".

    The pattern can also be changed at runtime by setting the QT_MESSAGE_PATTERN
    environment variable; if both qSetMessagePattern() is called and QT_MESSAGE_PATTERN is
    set, the environment variable takes precedence.

    qSetMessagePattern() has no effect if a custom message handler is installed.

    \sa qInstallMessageHandler, {Debugging Techniques}
 */

QtMessageHandler qInstallMessageHandler(QtMessageHandler h)
{
    if (!messageHandler)
        messageHandler = qDefaultMessageHandler;
    QtMessageHandler old = messageHandler;
    messageHandler = h;
#if defined(Q_OS_WIN) && defined(QT_BUILD_CORE_LIB)
    if (!messageHandler && usingWinMain)
        messageHandler = qWinMessageHandler;
#endif
    return old;
}

QtMsgHandler qInstallMsgHandler(QtMsgHandler h)
{
    //if handler is 0, set it to the
    //default message handler
    if (!msgHandler)
        msgHandler = qDefaultMsgHandler;
    QtMsgHandler old = msgHandler;
    msgHandler = h;
#if defined(Q_OS_WIN) && defined(QT_BUILD_CORE_LIB)
    if (!msgHandler && usingWinMain)
        msgHandler = qWinMsgHandler;
#endif
    return old;
}

void qSetMessagePattern(const QString &pattern)
{
    QMutexLocker lock(&QMessagePattern::mutex);

    if (!qMessagePattern()->fromEnvironment)
        qMessagePattern()->setPattern(pattern);
}

void QMessageLogContext::copy(const QMessageLogContext &logContext)
{
    this->category = logContext.category;
    this->file = logContext.file;
    this->line = logContext.line;
    this->function = logContext.function;
}

QT_END_NAMESPACE
