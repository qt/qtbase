// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2019 Intel Corporation.
// Copyright (C) 2019 Mail.ru Group.
// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLATIN1STRINGVIEW_H
#define QLATIN1STRINGVIEW_H

#include <QtCore/qchar.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qtversionchecks.h>
#include <QtCore/qstringview.h>

#if 0
// Workaround for generating forward headers
#pragma qt_class(QLatin1String)
#pragma qt_class(QLatin1StringView)
#endif

QT_BEGIN_NAMESPACE

class QString;

#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0) || defined(QT_BOOTSTRAPPED) || defined(Q_QDOC)
#    define Q_L1S_VIEW_IS_PRIMARY
class QLatin1StringView
#else
class QLatin1String
#endif
{
public:
#ifdef Q_L1S_VIEW_IS_PRIMARY
    constexpr QLatin1StringView() noexcept {}
    constexpr QLatin1StringView(std::nullptr_t) noexcept : QLatin1StringView() {}
    constexpr explicit QLatin1StringView(const char *s) noexcept
        : QLatin1StringView(s, s ? qsizetype(QtPrivate::lengthHelperPointer(s)) : 0) {}
    constexpr QLatin1StringView(const char *f, const char *l)
        : QLatin1StringView(f, qsizetype(l - f)) {}
    constexpr QLatin1StringView(const char *s, qsizetype sz) noexcept : m_data(s), m_size(sz) {}
    explicit QLatin1StringView(const QByteArray &s) noexcept
        : QLatin1StringView(s.constData(), s.size()) {}
    constexpr explicit QLatin1StringView(QByteArrayView s) noexcept
        : QLatin1StringView(s.constData(), s.size()) {}
#else
    constexpr QLatin1String() noexcept : m_size(0), m_data(nullptr) {}
    Q_WEAK_OVERLOAD
    constexpr QLatin1String(std::nullptr_t) noexcept : QLatin1String() {}
    constexpr explicit QLatin1String(const char *s) noexcept
        : m_size(s ? qsizetype(QtPrivate::lengthHelperPointer(s)) : 0), m_data(s) {}
    constexpr QLatin1String(const char *f, const char *l)
        : QLatin1String(f, qsizetype(l - f)) {}
    constexpr QLatin1String(const char *s, qsizetype sz) noexcept : m_size(sz), m_data(s) {}
    explicit QLatin1String(const QByteArray &s) noexcept : m_size(s.size()), m_data(s.constData()) {}
    constexpr explicit QLatin1String(QByteArrayView s) noexcept : m_size(s.size()), m_data(s.data()) {}
#endif // !Q_L1S_VIEW_IS_PRIMARY

    inline QString toString() const;

    constexpr const char *latin1() const noexcept { return m_data; }
    constexpr qsizetype size() const noexcept { return m_size; }
    constexpr const char *data() const noexcept { return m_data; }
    [[nodiscard]] constexpr const char *constData() const noexcept { return data(); }
    [[nodiscard]] constexpr const char *constBegin() const noexcept { return begin(); }
    [[nodiscard]] constexpr const char *constEnd() const noexcept { return end(); }

    [[nodiscard]] constexpr QLatin1Char first() const { return front(); }
    [[nodiscard]] constexpr QLatin1Char last() const { return back(); }

    [[nodiscard]] constexpr qsizetype length() const noexcept { return size(); }

    constexpr bool isNull() const noexcept { return !data(); }
    constexpr bool isEmpty() const noexcept { return !size(); }

    [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

    template <typename...Args>
    [[nodiscard]] inline QString arg(Args &&...args) const;

    [[nodiscard]] constexpr QLatin1Char at(qsizetype i) const
    {
        Q_ASSERT(i >= 0);
        Q_ASSERT(i < size());
        return QLatin1Char(m_data[i]);
    }
    [[nodiscard]] constexpr QLatin1Char operator[](qsizetype i) const { return at(i); }

    [[nodiscard]] constexpr QLatin1Char front() const { return at(0); }
    [[nodiscard]] constexpr QLatin1Char back() const { return at(size() - 1); }

    [[nodiscard]] int compare(QStringView other, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::compareStrings(*this, other, cs); }
    [[nodiscard]] int compare(QLatin1StringView other, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::compareStrings(*this, other, cs); }
    [[nodiscard]] inline int compare(QUtf8StringView other, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    [[nodiscard]] constexpr int compare(QChar c) const noexcept
    { return isEmpty() ? -1 : front() == c ? int(size() > 1) : uchar(m_data[0]) - c.unicode(); }
    [[nodiscard]] int compare(QChar c, Qt::CaseSensitivity cs) const noexcept
    { return QtPrivate::compareStrings(*this, QStringView(&c, 1), cs); }

    [[nodiscard]] bool startsWith(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::startsWith(*this, s, cs); }
    [[nodiscard]] bool startsWith(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::startsWith(*this, s, cs); }
    [[nodiscard]] constexpr bool startsWith(QChar c) const noexcept
    { return !isEmpty() && front() == c; }
    [[nodiscard]] bool startsWith(QChar c, Qt::CaseSensitivity cs) const noexcept
    { return QtPrivate::startsWith(*this, QStringView(&c, 1), cs); }

    [[nodiscard]] bool endsWith(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::endsWith(*this, s, cs); }
    [[nodiscard]] bool endsWith(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::endsWith(*this, s, cs); }
    [[nodiscard]] constexpr bool endsWith(QChar c) const noexcept
    { return !isEmpty() && back() == c; }
    [[nodiscard]] bool endsWith(QChar c, Qt::CaseSensitivity cs) const noexcept
    { return QtPrivate::endsWith(*this, QStringView(&c, 1), cs); }

    [[nodiscard]] qsizetype indexOf(QStringView s, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::findString(*this, from, s, cs); }
    [[nodiscard]] qsizetype indexOf(QLatin1StringView s, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::findString(*this, from, s, cs); }
    [[nodiscard]] qsizetype indexOf(QChar c, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::findString(*this, from, QStringView(&c, 1), cs); }

    [[nodiscard]] bool contains(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return indexOf(s, 0, cs) != -1; }
    [[nodiscard]] bool contains(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return indexOf(s, 0, cs) != -1; }
    [[nodiscard]] bool contains(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return indexOf(QStringView(&c, 1), 0, cs) != -1; }

    [[nodiscard]] qsizetype lastIndexOf(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return lastIndexOf(s, size(), cs); }
    [[nodiscard]] qsizetype lastIndexOf(QStringView s, qsizetype from, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::lastIndexOf(*this, from, s, cs); }
    [[nodiscard]] qsizetype lastIndexOf(QLatin1StringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return lastIndexOf(s, size(), cs); }
    [[nodiscard]] qsizetype lastIndexOf(QLatin1StringView s, qsizetype from, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::lastIndexOf(*this, from, s, cs); }
    [[nodiscard]] qsizetype lastIndexOf(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return lastIndexOf(c, -1, cs); }
    [[nodiscard]] qsizetype lastIndexOf(QChar c, qsizetype from, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::lastIndexOf(*this, from, QStringView(&c, 1), cs); }

    [[nodiscard]] qsizetype count(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return QtPrivate::count(*this, str, cs); }
    [[nodiscard]] qsizetype count(QLatin1StringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return QtPrivate::count(*this, str, cs); }
    [[nodiscard]] qsizetype count(QChar ch, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::count(*this, ch, cs); }

    [[nodiscard]] short toShort(bool *ok = nullptr, int base = 10) const
    { return QtPrivate::toIntegral<short>(QByteArrayView(*this), ok, base); }
    [[nodiscard]] ushort toUShort(bool *ok = nullptr, int base = 10) const
    { return QtPrivate::toIntegral<ushort>(QByteArrayView(*this), ok, base); }
    [[nodiscard]] int toInt(bool *ok = nullptr, int base = 10) const
    { return QtPrivate::toIntegral<int>(QByteArrayView(*this), ok, base); }
    [[nodiscard]] uint toUInt(bool *ok = nullptr, int base = 10) const
    { return QtPrivate::toIntegral<uint>(QByteArrayView(*this), ok, base); }
    [[nodiscard]] long toLong(bool *ok = nullptr, int base = 10) const
    { return QtPrivate::toIntegral<long>(QByteArrayView(*this), ok, base); }
    [[nodiscard]] ulong toULong(bool *ok = nullptr, int base = 10) const
    { return QtPrivate::toIntegral<ulong>(QByteArrayView(*this), ok, base); }
    [[nodiscard]] qlonglong toLongLong(bool *ok = nullptr, int base = 10) const
    { return QtPrivate::toIntegral<qlonglong>(QByteArrayView(*this), ok, base); }
    [[nodiscard]] qulonglong toULongLong(bool *ok = nullptr, int base = 10) const
    { return QtPrivate::toIntegral<qulonglong>(QByteArrayView(*this), ok, base); }
    [[nodiscard]] float toFloat(bool *ok = nullptr) const
    {
        const auto r = QtPrivate::toFloat(*this);
        if (ok)
            *ok = bool(r);
        return r.value_or(0.0f);
    }
    [[nodiscard]] double toDouble(bool *ok = nullptr) const
    {
        const auto r = QtPrivate::toDouble(*this);
        if (ok)
            *ok = bool(r);
        return r.value_or(0.0);
    }

    using value_type = const char;
    using pointer = value_type*;
    using const_pointer = pointer;
    using reference = value_type&;
    using const_reference = reference;
    using iterator = value_type*;
    using const_iterator = iterator;
    using difference_type = qsizetype; // violates Container concept requirements
    using size_type = qsizetype;       // violates Container concept requirements

    constexpr const_iterator begin() const noexcept { return data(); }
    constexpr const_iterator cbegin() const noexcept { return data(); }
    constexpr const_iterator end() const noexcept { return data() + size(); }
    constexpr const_iterator cend() const noexcept { return data() + size(); }

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = reverse_iterator;

    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

    [[nodiscard]] constexpr QLatin1StringView mid(qsizetype pos, qsizetype n = -1) const
    {
        using namespace QtPrivate;
        auto result = QContainerImplHelper::mid(size(), &pos, &n);
        return result == QContainerImplHelper::Null ? QLatin1StringView()
                                                    : QLatin1StringView(m_data + pos, n);
    }
    [[nodiscard]] constexpr QLatin1StringView left(qsizetype n) const
    {
        if (size_t(n) >= size_t(size()))
            n = size();
        return {m_data, n};
    }
    [[nodiscard]] constexpr QLatin1StringView right(qsizetype n) const
    {
        if (size_t(n) >= size_t(size()))
            n = size();
        return {m_data + m_size - n, n};
    }

    [[nodiscard]] constexpr QLatin1StringView sliced(qsizetype pos) const
    { verify(pos, 0); return {m_data + pos, m_size - pos}; }
    [[nodiscard]] constexpr QLatin1StringView sliced(qsizetype pos, qsizetype n) const
    { verify(pos, n); return {m_data + pos, n}; }
    [[nodiscard]] constexpr QLatin1StringView first(qsizetype n) const
    { verify(0, n); return sliced(0, n); }
    [[nodiscard]] constexpr QLatin1StringView last(qsizetype n) const
    { verify(0, n); return sliced(size() - n, n); }
    [[nodiscard]] constexpr QLatin1StringView chopped(qsizetype n) const
    { verify(0, n); return sliced(0, size() - n); }

    constexpr void chop(qsizetype n)
    { verify(0, n); m_size -= n; }
    constexpr void truncate(qsizetype n)
    { verify(0, n); m_size = n; }

    [[nodiscard]] QLatin1StringView trimmed() const noexcept { return QtPrivate::trimmed(*this); }

    template <typename Needle, typename...Flags>
    [[nodiscard]] constexpr auto tokenize(Needle &&needle, Flags...flags) const
        noexcept(noexcept(qTokenize(std::declval<const QLatin1StringView &>(),
                                    std::forward<Needle>(needle), flags...)))
            -> decltype(qTokenize(*this, std::forward<Needle>(needle), flags...))
    { return qTokenize(*this, std::forward<Needle>(needle), flags...); }

    friend bool operator==(QLatin1StringView s1, QLatin1StringView s2) noexcept
    { return QByteArrayView(s1) == QByteArrayView(s2); }
    friend bool operator!=(QLatin1StringView s1, QLatin1StringView s2) noexcept
    { return !(s1 == s2); }
    friend bool operator<(QLatin1StringView s1, QLatin1StringView s2) noexcept
    {
        const qsizetype len = qMin(s1.size(), s2.size());
        const int r = len ? memcmp(s1.latin1(), s2.latin1(), len) : 0;
        return r < 0 || (r == 0 && s1.size() < s2.size());
    }
    friend bool operator>(QLatin1StringView s1, QLatin1StringView s2) noexcept
    { return s2 < s1; }
    friend bool operator<=(QLatin1StringView s1, QLatin1StringView s2) noexcept
    { return !(s1 > s2); }
    friend bool operator>=(QLatin1StringView s1, QLatin1StringView s2) noexcept
    { return !(s1 < s2); }

    // QChar <> QLatin1StringView
    friend bool operator==(QChar lhs, QLatin1StringView rhs) noexcept { return rhs.size() == 1 && lhs == rhs.front(); }
    friend bool operator< (QChar lhs, QLatin1StringView rhs) noexcept { return compare_helper(&lhs, 1, rhs) < 0; }
    friend bool operator> (QChar lhs, QLatin1StringView rhs) noexcept { return compare_helper(&lhs, 1, rhs) > 0; }
    friend bool operator!=(QChar lhs, QLatin1StringView rhs) noexcept { return !(lhs == rhs); }
    friend bool operator<=(QChar lhs, QLatin1StringView rhs) noexcept { return !(lhs >  rhs); }
    friend bool operator>=(QChar lhs, QLatin1StringView rhs) noexcept { return !(lhs <  rhs); }

    friend bool operator==(QLatin1StringView lhs, QChar rhs) noexcept { return   rhs == lhs; }
    friend bool operator!=(QLatin1StringView lhs, QChar rhs) noexcept { return !(rhs == lhs); }
    friend bool operator< (QLatin1StringView lhs, QChar rhs) noexcept { return   rhs >  lhs; }
    friend bool operator> (QLatin1StringView lhs, QChar rhs) noexcept { return   rhs <  lhs; }
    friend bool operator<=(QLatin1StringView lhs, QChar rhs) noexcept { return !(rhs <  lhs); }
    friend bool operator>=(QLatin1StringView lhs, QChar rhs) noexcept { return !(rhs >  lhs); }

    // QStringView <> QLatin1StringView
    friend bool operator==(QStringView lhs, QLatin1StringView rhs) noexcept
    { return lhs.size() == rhs.size() && QtPrivate::equalStrings(lhs, rhs); }
    friend bool operator!=(QStringView lhs, QLatin1StringView rhs) noexcept { return !(lhs == rhs); }
    friend bool operator< (QStringView lhs, QLatin1StringView rhs) noexcept { return QtPrivate::compareStrings(lhs, rhs) <  0; }
    friend bool operator<=(QStringView lhs, QLatin1StringView rhs) noexcept { return QtPrivate::compareStrings(lhs, rhs) <= 0; }
    friend bool operator> (QStringView lhs, QLatin1StringView rhs) noexcept { return QtPrivate::compareStrings(lhs, rhs) >  0; }
    friend bool operator>=(QStringView lhs, QLatin1StringView rhs) noexcept { return QtPrivate::compareStrings(lhs, rhs) >= 0; }

    friend bool operator==(QLatin1StringView lhs, QStringView rhs) noexcept
    { return lhs.size() == rhs.size() && QtPrivate::equalStrings(lhs, rhs); }
    friend bool operator!=(QLatin1StringView lhs, QStringView rhs) noexcept { return !(lhs == rhs); }
    friend bool operator< (QLatin1StringView lhs, QStringView rhs) noexcept { return QtPrivate::compareStrings(lhs, rhs) <  0; }
    friend bool operator<=(QLatin1StringView lhs, QStringView rhs) noexcept { return QtPrivate::compareStrings(lhs, rhs) <= 0; }
    friend bool operator> (QLatin1StringView lhs, QStringView rhs) noexcept { return QtPrivate::compareStrings(lhs, rhs) >  0; }
    friend bool operator>=(QLatin1StringView lhs, QStringView rhs) noexcept { return QtPrivate::compareStrings(lhs, rhs) >= 0; }


#if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
    QT_ASCII_CAST_WARN inline bool operator==(const char *s) const;
    QT_ASCII_CAST_WARN inline bool operator!=(const char *s) const;
    QT_ASCII_CAST_WARN inline bool operator<(const char *s) const;
    QT_ASCII_CAST_WARN inline bool operator>(const char *s) const;
    QT_ASCII_CAST_WARN inline bool operator<=(const char *s) const;
    QT_ASCII_CAST_WARN inline bool operator>=(const char *s) const;

    QT_ASCII_CAST_WARN inline bool operator==(const QByteArray &s) const;
    QT_ASCII_CAST_WARN inline bool operator!=(const QByteArray &s) const;
    QT_ASCII_CAST_WARN inline bool operator<(const QByteArray &s) const;
    QT_ASCII_CAST_WARN inline bool operator>(const QByteArray &s) const;
    QT_ASCII_CAST_WARN inline bool operator<=(const QByteArray &s) const;
    QT_ASCII_CAST_WARN inline bool operator>=(const QByteArray &s) const;

    QT_ASCII_CAST_WARN friend bool operator==(const char *s1, QLatin1StringView s2) { return compare_helper(s2, s1) == 0; }
    QT_ASCII_CAST_WARN friend bool operator!=(const char *s1, QLatin1StringView s2) { return compare_helper(s2, s1) != 0; }
    QT_ASCII_CAST_WARN friend bool operator< (const char *s1, QLatin1StringView s2) { return compare_helper(s2, s1) >  0; }
    QT_ASCII_CAST_WARN friend bool operator> (const char *s1, QLatin1StringView s2) { return compare_helper(s2, s1) <  0; }
    QT_ASCII_CAST_WARN friend bool operator<=(const char *s1, QLatin1StringView s2) { return compare_helper(s2, s1) >= 0; }
    QT_ASCII_CAST_WARN friend bool operator>=(const char *s1, QLatin1StringView s2) { return compare_helper(s2, s1) <= 0; }
#endif // !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)

private:
    Q_ALWAYS_INLINE constexpr void verify([[maybe_unused]] qsizetype pos,
                                          [[maybe_unused]] qsizetype n = 1) const
    {
        Q_ASSERT(pos >= 0);
        Q_ASSERT(pos <= size());
        Q_ASSERT(n >= 0);
        Q_ASSERT(n <= size() - pos);
    }
    static int compare_helper(const QLatin1StringView &s1, const char *s2) noexcept
    { return compare_helper(s1, s2, qstrlen(s2)); }
    Q_CORE_EXPORT static int compare_helper(const QLatin1StringView &s1, const char *s2, qsizetype len) noexcept;
    Q_CORE_EXPORT static int compare_helper(const QChar *data1, qsizetype length1,
                                            QLatin1StringView s2,
                                            Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0) || defined(QT_BOOTSTRAPPED)
    const char *m_data = nullptr;
    qsizetype m_size = 0;
#else
    qsizetype m_size;
    const char *m_data;
#endif
};
#ifdef Q_L1S_VIEW_IS_PRIMARY
Q_DECLARE_TYPEINFO(QLatin1StringView, Q_RELOCATABLE_TYPE);
#else
Q_DECLARE_TYPEINFO(QLatin1String, Q_RELOCATABLE_TYPE);
#endif

namespace Qt {
inline namespace Literals {
inline namespace StringLiterals {

constexpr inline QLatin1StringView operator""_L1(const char *str, size_t size) noexcept
{
    return {str, qsizetype(size)};
}

} // StringLiterals
} // Literals
} // Qt

QT_END_NAMESPACE

#ifdef Q_L1S_VIEW_IS_PRIMARY
#    undef Q_L1S_VIEW_IS_PRIMARY
#endif

#endif // QLATIN1STRINGVIEW_H
