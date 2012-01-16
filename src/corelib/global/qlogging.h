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

#include <QtCore/qglobal.h>

#ifndef QLOGGING_H
#define QLOGGING_H

#if 0
// header is automatically included in qglobal.h
#pragma qt_no_master_include
#endif

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

/*
  Forward declarations only.

  In order to use the qDebug() stream, you must #include<QDebug>
*/
class QDebug;
class QNoDebug;

enum QtMsgType { QtDebugMsg, QtWarningMsg, QtCriticalMsg, QtFatalMsg, QtSystemMsg = QtCriticalMsg };

class QMessageLogContext
{
    Q_DISABLE_COPY(QMessageLogContext)
public:
    QMessageLogContext() : version(1), line(0), file(0), function(0), category(0) {}
    Q_DECL_CONSTEXPR QMessageLogContext(const char *fileName, int lineNumber, const char *functionName, const char *categoryName)
        : version(1), line(lineNumber), file(fileName), function(functionName), category(categoryName) {}

    void copy(const QMessageLogContext &logContext);

    int version;
    int line;
    const char *file;
    const char *function;
    const char *category;

private:
    friend class QMessageLogger;
    friend class QDebug;
};

class Q_CORE_EXPORT QMessageLogger
{
    Q_DISABLE_COPY(QMessageLogger)
public:
    QMessageLogger() : context() {}
    Q_DECL_CONSTEXPR QMessageLogger(const char *file, int line, const char *function)
        : context(file, line, function, "default") {}
    Q_DECL_CONSTEXPR QMessageLogger(const char *file, int line, const char *function, const char *category)
        : context(file, line, function, category) {}

    void debug(const char *msg, ...)
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 2, 3)))
#endif
    ;
    void noDebug(const char *, ...)
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 2, 3)))
#endif
    {}
    void warning(const char *msg, ...)
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 2, 3)))
#endif
    ;
    void critical(const char *msg, ...)
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 2, 3)))
#endif
    ;
    void fatal(const char *msg, ...)
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 2, 3)))
#endif
    ;

#ifndef QT_NO_DEBUG_STREAM
    QDebug debug();
    QDebug warning();
    QDebug critical();

    QNoDebug noDebug();
#endif // QT_NO_DEBUG_STREAM

private:
    QMessageLogContext context;
};

/*
  qDebug, qWarning, qCritical, qFatal are redefined to automatically include context information
 */
#define qDebug QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).debug
#define qWarning QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).warning
#define qCritical QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).critical
#define qFatal QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).fatal

#define QT_NO_QDEBUG_MACRO while (false) QMessageLogger().noDebug
#define QT_NO_QWARNING_MACRO while (false) QMessageLogger().noDebug

#if defined(QT_NO_DEBUG_OUTPUT)
#  undef qDebug
#  define qDebug QT_NO_QDEBUG_MACRO
#endif
#if defined(QT_NO_WARNING_OUTPUT)
#  undef qWarning
#  define qWarning QT_NO_QWARNING_MACRO
#endif

Q_CORE_EXPORT void qt_message_output(QtMsgType, const QMessageLogContext &context, const char *buf);

Q_CORE_EXPORT void qErrnoWarning(int code, const char *msg, ...);
Q_CORE_EXPORT void qErrnoWarning(const char *msg, ...);

// deprecated. Use qInstallMessageHandler instead!
typedef void (*QtMsgHandler)(QtMsgType, const char *);
Q_CORE_EXPORT QtMsgHandler qInstallMsgHandler(QtMsgHandler);

typedef void (*QMessageHandler)(QtMsgType, const QMessageLogContext &, const char *);
Q_CORE_EXPORT QMessageHandler qInstallMessageHandler(QMessageHandler);

QT_END_HEADER
QT_END_NAMESPACE

#endif // QLOGGING_H
