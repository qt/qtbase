/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QBYTEARRAYALGORITHMS_H
#define QBYTEARRAYALGORITHMS_H

#include <QtCore/qnamespace.h>

#include <string.h>
#include <stdarg.h>

#if 0
#pragma qt_class(QByteArrayAlgorithms)
#endif

QT_BEGIN_NAMESPACE

class QByteArrayView;

namespace QtPrivate {

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
bool startsWith(QByteArrayView haystack, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
bool endsWith(QByteArrayView haystack, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
qsizetype findByteArray(QByteArrayView haystack, qsizetype from, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
qsizetype lastIndexOf(QByteArrayView haystack, qsizetype from, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
qsizetype count(QByteArrayView haystack, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT int compareMemory(QByteArrayView lhs, QByteArrayView rhs);

} // namespace QtPrivate

/*****************************************************************************
  Safe and portable C string functions; extensions to standard string.h
 *****************************************************************************/

Q_CORE_EXPORT char *qstrdup(const char *);

inline size_t qstrlen(const char *str)
{
    QT_WARNING_PUSH
#if defined(Q_CC_GNU_ONLY) && Q_CC_GNU >= 900 && Q_CC_GNU < 1000
    // spurious compiler warning (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91490#c6)
    // when Q_DECLARE_METATYPE_TEMPLATE_1ARG is used
    QT_WARNING_DISABLE_GCC("-Wstringop-overflow")
#endif
    return str ? strlen(str) : 0;
    QT_WARNING_POP
}

inline size_t qstrnlen(const char *str, size_t maxlen)
{
    size_t length = 0;
    if (str) {
        while (length < maxlen && *str++)
            length++;
    }
    return length;
}

// implemented in qbytearray.cpp
Q_CORE_EXPORT char *qstrcpy(char *dst, const char *src);
Q_CORE_EXPORT char *qstrncpy(char *dst, const char *src, size_t len);

Q_CORE_EXPORT int qstrcmp(const char *str1, const char *str2);

inline int qstrncmp(const char *str1, const char *str2, size_t len)
{
    return (str1 && str2) ? strncmp(str1, str2, len)
        : (str1 ? 1 : (str2 ? -1 : 0));
}
Q_CORE_EXPORT int qstricmp(const char *, const char *);
Q_CORE_EXPORT int qstrnicmp(const char *, const char *, size_t len);
Q_CORE_EXPORT int qstrnicmp(const char *, qsizetype, const char *, qsizetype = -1);

// implemented in qvsnprintf.cpp
Q_CORE_EXPORT int qvsnprintf(char *str, size_t n, const char *fmt, va_list ap);
Q_CORE_EXPORT int qsnprintf(char *str, size_t n, const char *fmt, ...);

// qChecksum: Internet checksum
Q_CORE_EXPORT quint16 qChecksum(QByteArrayView data, Qt::ChecksumType standard = Qt::ChecksumIso3309);

QT_END_NAMESPACE

#endif // QBYTEARRAYALGORITHMS_H
