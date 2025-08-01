// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qplatformdefs.h"

#include "qbytearray.h"
#include "qstring.h"

#include <cerrno>
#include <QtCore/q26numeric.h>

#include "string.h"

QT_BEGIN_NAMESPACE

/*!
    \macro QT_NO_QSNPRINTF
    \since 6.8
    \relates QByteArray

    Defining this macro removes the availability of the qsnprintf() and
    qvsnprintf() functions. See the functions' documentation for why you may
    want to disable them.

    \sa qsnprintf(), qvsnprintf().
*/

#if QT_DEPRECATED_SINCE(6, 9)

#if !defined(QT_VSNPRINTF) || defined(Q_QDOC)

/*!
    \fn int qvsnprintf(char *str, size_t n, const char *fmt, va_list ap)
    \relates QByteArray

    \deprecated [6.9] Use C++11's \c{std::vsnprintf()} from \c{<cstdio>} instead.

    A portable \c vsnprintf() function. Will call \c ::vsnprintf(), \c
    ::_vsnprintf(), or \c ::vsnprintf_s depending on the system, or
    fall back to an internal version.

    \a fmt is the \c printf() format string. The result is put into
    \a str, which is a buffer of at least \a n bytes.

    The caller is responsible to call \c va_end() on \a ap.

    \warning Since vsnprintf() shows different behavior on certain
    platforms, you should not rely on the return value or on the fact
    that you will always get a 0 terminated string back. There are also
    differences in how \c{%a} (hex floats) and \c{%ls} (wide strings) are
    handled on WebAssembly and Android.

    Ideally, you should never call this function but use QString::asprintf()
    instead.

    \sa qsnprintf(), QString::asprintf()
*/

Q_CORE_EXPORT // QT_NO_QSNPRINTF is in effect
int qvsnprintf(char *str, size_t n, const char *fmt, va_list ap)
{
    if (!str || !fmt)
        return -1;

    const QByteArray ba = QString::vasprintf(fmt, ap).toLocal8Bit();

    const auto realSize = ba.size();
    int result;
    if constexpr (sizeof(int) != sizeof(realSize)) {
        result = q26::saturate_cast<int>(realSize);
        if (result != realSize) {
            errno = EOVERFLOW;
            return -1;
        }
    } else {
        result = realSize;
    }

    if (n > 0) {
        size_t blen = (std::min)(size_t(realSize), n - 1);
        memcpy(str, ba.constData(), blen);
        str[blen] = '\0'; // make sure str is always 0 terminated
    }

    return result;
}

#else

QT_BEGIN_INCLUDE_NAMESPACE
#include <stdio.h>
QT_END_INCLUDE_NAMESPACE

Q_CORE_EXPORT // QT_NO_QSNPRINTF is in effect
int qvsnprintf(char *str, size_t n, const char *fmt, va_list ap)
{
    return QT_VSNPRINTF(str, n, fmt, ap);
}

#endif

/*!
    \fn int qsnprintf(char *str, size_t n, const char *fmt, ...)
    \target bytearray-qsnprintf
    \relates QByteArray

    \deprecated [6.9] Use C++11's \c{std::snprintf()} from \c{<cstdio>} instead.

    A portable snprintf() function, calls qvsnprintf.

    \a fmt is the \c printf() format string. The result is put into
    \a str, which is a buffer of at least \a n bytes.

    \warning Call this function only when you know what you are doing
    since it shows different behavior on certain platforms.
    Use QString::asprintf() to format a string instead.

    \sa qvsnprintf(), QString::asprintf()
*/

Q_CORE_EXPORT // QT_NO_QSNPRINTF is in effect
int qsnprintf(char *str, size_t n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    QT_IGNORE_DEPRECATIONS(
    int ret = qvsnprintf(str, n, fmt, ap);
    )
    va_end(ap);

    return ret;
}

#endif // QT_DEPRECATED_SINCE(6, 9)

QT_END_NAMESPACE
