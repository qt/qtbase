// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

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

[[nodiscard]] inline // defined in qbytearrayview.h
qsizetype findByteArray(QByteArrayView haystack, qsizetype from, char needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
qsizetype findByteArray(QByteArrayView haystack, qsizetype from, QByteArrayView needle) noexcept;

[[nodiscard]] inline // defined in qbytearrayview.h
qsizetype lastIndexOf(QByteArrayView haystack, qsizetype from, uchar needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
qsizetype lastIndexOf(QByteArrayView haystack, qsizetype from, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
qsizetype count(QByteArrayView haystack, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT int compareMemory(QByteArrayView lhs, QByteArrayView rhs);

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION QByteArrayView trimmed(QByteArrayView s) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isValidUtf8(QByteArrayView s) noexcept;

template <typename T>
class ParsedNumber
{
    T m_value;
    quint32 m_error : 1;
    quint32 m_reserved : 31;
    void *m_reserved2 = nullptr;
public:
    constexpr ParsedNumber() noexcept : m_value(), m_error(true), m_reserved(0) {}
    constexpr explicit ParsedNumber(T v) : m_value(v), m_error(false), m_reserved(0) {}

    // minimal optional-like API:
    explicit operator bool() const noexcept { return !m_error; }
    T &operator*() { Q_ASSERT(*this); return m_value; }
    const T &operator*() const { Q_ASSERT(*this); return m_value; }
    T *operator->() noexcept { return *this ? &m_value : nullptr; }
    const T *operator->() const noexcept { return *this ? &m_value : nullptr; }
    template <typename U> // not = T, as that'd allow calls that are incompatible with std::optional
    T value_or(U &&u) const { return *this ? m_value : T(std::forward<U>(u)); }
};

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION ParsedNumber<double> toDouble(QByteArrayView a) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION ParsedNumber<float> toFloat(QByteArrayView a) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION ParsedNumber<qlonglong> toSignedInteger(QByteArrayView data, int base);
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION ParsedNumber<qulonglong> toUnsignedInteger(QByteArrayView data, int base);

// QByteArrayView has incomplete type here, and we can't include qbytearrayview.h,
// since it includes qbytearrayalgorithms.h. Use the ByteArrayView template type as
// a workaround.
template <typename T, typename ByteArrayView,
          typename = std::enable_if_t<std::is_same_v<ByteArrayView, QByteArrayView>>>
static inline T toIntegral(ByteArrayView data, bool *ok, int base)
{
    const auto val = [&] {
        if constexpr (std::is_unsigned_v<T>)
            return toUnsignedInteger(data, base);
        else
            return toSignedInteger(data, base);
    }();
    const bool failed = !val || T(*val) != *val;
    if (ok)
        *ok = !failed;
    if (failed)
        return 0;
    return T(*val);
}

[[nodiscard]] constexpr inline bool isAsciiUpper(char32_t c) noexcept
{
    return c >= 'A' && c <= 'Z';
}

[[nodiscard]] constexpr inline char toAsciiLower(char ch) noexcept
{
    return isAsciiUpper(ch) ? ch - 'A' + 'a' : ch;
}

[[nodiscard]] constexpr inline int caseCompareAscii(char lhs, char rhs) noexcept
{
    const char lhsLower = toAsciiLower(lhs);
    const char rhsLower = toAsciiLower(rhs);
    return int(uchar(lhsLower)) - int(uchar(rhsLower));
}

constexpr
int qstrnicmp_impl(const char *s1, const char *s2, size_t len)
{
    if (!s1 || !s2)
        return s1 ? 1 : (s2 ? -1 : 0);
    for (; len--; ++s1, ++s2) {
        const char c = *s1;
        if (int res = QtPrivate::caseCompareAscii(c, *s2))
            return res;
        if (!c)                                // strings are equal
            break;
    }
    return 0;
}

constexpr
int qstrnicmp_impl(const char *s1, qsizetype len1, const char *s2, qsizetype len2)
{
    Q_PRE(len1 >= 0);
    Q_PRE(len2 >= -1);
    Q_PRE(s1 || len1 == 0);
    Q_PRE(s2 || len2 == 0 || len2 == -1);
    if (!s1 || !len1) {
        if (len2 == 0)
            return 0;
        if (len2 == -1)
            return (!s2 || !*s2) ? 0 : -1;
        return -1;
    }
    if (!s2)
        return len1 == 0 ? 0 : 1;

    if (len2 == -1) {
        // null-terminated s2
        qsizetype i = 0;
        for (i = 0; i < len1; ++i) {
            const char c = s2[i];
            if (!c)
                return 1;

            if (int res = QtPrivate::caseCompareAscii(s1[i], c))
                return res;
        }
        return s2[i] ? -1 : 0;
    } else {
        // not null-terminated
        const qsizetype len = qMin(len1, len2);
        for (qsizetype i = 0; i < len; ++i) {
            if (int res = QtPrivate::caseCompareAscii(s1[i], s2[i]))
                return res;
        }
        if (len1 == len2)
            return 0;
        return len1 < len2 ? -1 : 1;
    }
}
} // namespace QtPrivate

/*****************************************************************************
  Safe and portable C string functions; extensions to standard string.h
 *****************************************************************************/

[[nodiscard]] Q_DECL_PURE_FUNCTION Q_CORE_EXPORT
const void *qmemrchr(const void *s, int needle, size_t n) noexcept;
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
    if (!str)
        return 0;
    auto end = static_cast<const char *>(memchr(str, '\0', maxlen));
    return end ? end - str : maxlen;
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


#ifdef QT_CORE_BUILD_REMOVED_API
QT6_ONLY(Q_CORE_EXPORT)
#endif
// These are unfortunately NOT constexpr in static builds (e.g. iOS)
QT_CORE_CONSTEXPR_INLINE_SINCE(6, 12)
int qstrnicmp(const char *s1, const char *s2, size_t len);

#ifdef QT_CORE_BUILD_REMOVED_API
QT6_ONLY(Q_CORE_EXPORT)
#endif
QT_CORE_CONSTEXPR_INLINE_SINCE(6, 12)
int qstrnicmp(const char *s1, qsizetype len1, const char *s2, qsizetype len2 = -1);

#if QT_CORE_INLINE_IMPL_SINCE(6, 12)
QT_CORE_CONSTEXPR_INLINE_SINCE(6, 12)
int qstrnicmp(const char *s1, const char *s2, size_t len)
{
    // ### Qt7: inline the function here
    return QtPrivate::qstrnicmp_impl(s1, s2, len);
}

QT_CORE_CONSTEXPR_INLINE_SINCE(6, 12)
int qstrnicmp(const char *s1, qsizetype len1, const char *s2, qsizetype len2)
{
    // ### Qt7: inline the function here
    return QtPrivate::qstrnicmp_impl(s1, len1, s2, len2);
}
#endif // INLINE_SINCE(6, 12)

#ifndef QT_NO_QSNPRINTF // use std::(v)snprintf() from <cstdio> instead
#if QT_DEPRECATED_SINCE(6, 9)
#define QSNPF_DEPR(vsn) \
    QT_DEPRECATED_VERSION_X_6_9("Use C++11 std::" #vsn "printf() instead, taking care to " \
                                "ensure that you didn't rely on QString::asprintf() " \
                                "idiosyncrasies that q" #vsn "printf might, but " \
                                "std::" #vsn "printf() does not, support.")
// implemented in qvsnprintf.cpp
QSNPF_DEPR(vsn)
Q_CORE_EXPORT int qvsnprintf(char *str, size_t n, const char *fmt, va_list ap)
    Q_ATTRIBUTE_FORMAT_PRINTF(3, 0);
QSNPF_DEPR(sn)
Q_CORE_EXPORT int qsnprintf(char *str, size_t n, const char *fmt, ...)
    Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
#undef QSNPF_DEPR
#endif // QT_DEPRECATED_SINCE(6, 9)
#endif // QT_NO_QSNPRINTF

// qChecksum: Internet checksum
Q_CORE_EXPORT quint16 qChecksum(QByteArrayView data, Qt::ChecksumType standard = Qt::ChecksumIso3309);

QT_END_NAMESPACE

#endif // QBYTEARRAYALGORITHMS_H
