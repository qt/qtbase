// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qglobal.h>
#include "qsystemerror_p.h"
#include <errno.h>
#if defined(Q_CC_MSVC)
#  include <crtdbg.h>
#endif
#ifdef Q_OS_WIN
#  include <qt_windows.h>
#  include <comdef.h>
#endif
#ifndef QT_BOOTSTRAPPED
#  include <qcoreapplication.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if !defined(Q_OS_WIN) && QT_CONFIG(thread) && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_QNX) && \
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
    [[maybe_unused]] static inline QString fromstrerror_helper(int, const QByteArray &buf)
    {
        return QString::fromLocal8Bit(buf);
    }
    [[maybe_unused]] static inline QString fromstrerror_helper(const char *str, const QByteArray &)
    {
        return QString::fromLocal8Bit(str);
    }
}
#endif

#ifdef Q_OS_WIN
static QString windowsErrorString(int errorCode)
{
    QString ret;
    wchar_t *string = nullptr;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  errorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPWSTR)&string,
                  0,
                  NULL);
    ret = QString::fromWCharArray(string);
    LocalFree((HLOCAL)string);

    if (ret.isEmpty() && errorCode == ERROR_MOD_NOT_FOUND)
        ret = QString::fromLatin1("The specified module could not be found.");
    if (ret.endsWith("\r\n"_L1))
        ret.chop(2);
    if (ret.isEmpty())
        ret = QString::fromLatin1("Unknown error 0x%1.")
                .arg(unsigned(errorCode), 8, 16, '0'_L1);
    return ret;
}
#endif

static QString standardLibraryErrorString(int errorCode)
{
    const char *s = nullptr;
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
      #if QT_CONFIG(thread) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && _POSIX_VERSION >= 200112L && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_QNX)
            QByteArray buf(1024, Qt::Uninitialized);
            ret = fromstrerror_helper(strerror_r(errorCode, buf.data(), buf.size()), buf);
      #else
            ret = QString::fromLocal8Bit(strerror(errorCode));
      #endif
    break; }
    }
    if (s) {
#ifndef QT_BOOTSTRAPPED
        ret = QCoreApplication::translate("QIODevice", s);
#else
        ret = QString::fromLatin1(s);
#endif
    }
    return ret.trimmed();
}

QString QSystemError::string(ErrorScope errorScope, int errorCode)
{
    switch (errorScope) {
    case NativeError:
#if defined(Q_OS_WIN)
        return windowsErrorString(errorCode);
#endif // else unix: native and standard library are the same
    case StandardLibraryError:
        return standardLibraryErrorString(errorCode);
    default:
        qWarning("invalid error scope");
        Q_FALLTHROUGH();
    case NoError:
        return u"No error"_s;
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

QString QSystemError::windowsComString(HRESULT hr)
{
    const _com_error comError(hr);
    QString result = "COM error 0x"_L1 + QString::number(ulong(hr), 16);
    if (const wchar_t *msg = comError.ErrorMessage())
        result += ": "_L1 + QString::fromWCharArray(msg);
    return result;
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
