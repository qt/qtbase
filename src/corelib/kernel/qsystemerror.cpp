/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <qglobal.h>
#include "qsystemerror_p.h"
#include <errno.h>
#if defined(Q_CC_MSVC)
#  include <crtdbg.h>
#endif
#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

#if !defined(Q_OS_WIN) && !defined(QT_NO_THREAD) && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_QNX) && \
    defined(_POSIX_THREAD_SAFE_FUNCTIONS) && _POSIX_VERSION >= 200112L
namespace {
    // There are two incompatible versions of strerror_r:
    // a) the XSI/POSIX.1 version, which returns an int,
    //    indicating success or not
    // b) the GNU version, which returns a char*, which may or may not
    //    be the beginning of the buffer we used
    // The GNU libc manpage for strerror_r says you should use the XSI
    // version in portable code. However, it's impossible to do that if
    // _GNU_SOURCE is defined so we use C++ overloading to decide what to do
    // depending on the return type
    static inline Q_DECL_UNUSED QString fromstrerror_helper(int, const QByteArray &buf)
    {
        return QString::fromLocal8Bit(buf);
    }
    static inline Q_DECL_UNUSED QString fromstrerror_helper(const char *str, const QByteArray &)
    {
        return QString::fromLocal8Bit(str);
    }
}
#endif

#ifdef Q_OS_WIN
static QString windowsErrorString(int errorCode)
{
    QString ret;
#ifndef Q_OS_WINRT
    wchar_t *string = 0;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  errorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPWSTR)&string,
                  0,
                  NULL);
    ret = QString::fromWCharArray(string);
    LocalFree((HLOCAL)string);
#else
    wchar_t errorString[1024];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  errorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPWSTR)&errorString,
                  sizeof(errorString)/sizeof(wchar_t),
                  NULL);
    ret = QString::fromWCharArray(errorString);
#endif  // Q_OS_WINRT

    if (ret.isEmpty() && errorCode == ERROR_MOD_NOT_FOUND)
        ret = QString::fromLatin1("The specified module could not be found.");
    if (ret.endsWith(QLatin1String("\r\n")))
        ret.chop(2);
    if (ret.isEmpty())
        ret = QString::fromLatin1("Unknown error 0x%1.")
                .arg(unsigned(errorCode), 8, 16, QLatin1Char('0'));
    return ret;
}
#endif

static QString standardLibraryErrorString(int errorCode)
{
    const char *s = 0;
    QString ret;
    switch (errorCode) {
    case 0:
        break;
    case EACCES:
        s = QT_TRANSLATE_NOOP("QIODevice", "Permission denied");
        break;
    case EMFILE:
        s = QT_TRANSLATE_NOOP("QIODevice", "Too many open files");
        break;
    case ENOENT:
        s = QT_TRANSLATE_NOOP("QIODevice", "No such file or directory");
        break;
    case ENOSPC:
        s = QT_TRANSLATE_NOOP("QIODevice", "No space left on device");
        break;
    default: {
      #if !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && _POSIX_VERSION >= 200112L && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_QNX)
            QByteArray buf(1024, Qt::Uninitialized);
            ret = fromstrerror_helper(strerror_r(errorCode, buf.data(), buf.size()), buf);
      #else
            ret = QString::fromLocal8Bit(strerror(errorCode));
      #endif
    break; }
    }
    if (s) {
        // ######## this breaks moc build currently
        // ret = QCoreApplication::translate("QIODevice", s);
        ret = QString::fromLatin1(s);
    }
    return ret.trimmed();
}

QString QSystemError::string(ErrorScope errorScope, int errorCode)
{
    switch(errorScope) {
    case NativeError:
#if defined (Q_OS_WIN)
        return windowsErrorString(errorCode);
#else
        //unix: fall through as native and standard library are the same
        Q_FALLTHROUGH();
#endif
    case StandardLibraryError:
        return standardLibraryErrorString(errorCode);
    default:
        qWarning("invalid error scope");
        Q_FALLTHROUGH();
    case NoError:
        return QLatin1String("No error");
    }
}

QString QSystemError::stdString(int errorCode)
{
    return standardLibraryErrorString(errorCode == -1 ? errno : errorCode);
}

#ifdef Q_OS_WIN
QString QSystemError::windowsString(int errorCode)
{
    return windowsErrorString(errorCode == -1 ? GetLastError() : errorCode);
}

QString qt_error_string(int code)
{
    return windowsErrorString(code == -1 ? GetLastError() : code);
}
#else
QString qt_error_string(int code)
{
    return standardLibraryErrorString(code == -1 ? errno : code);
}
#endif

QT_END_NAMESPACE
