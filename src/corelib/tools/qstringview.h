/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: http://www.qt.io/licensing/
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

#ifndef QSTRING_H
# include <QtCore/qstring.h>
#endif

#ifndef QSTRINGVIEW_H
#define QSTRINGVIEW_H

#include <string>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_UNICODE_LITERAL
# ifndef QT_UNICODE_LITERAL
#  error "If you change QStringLiteral, please change QStringViewLiteral, too"
# endif
# define QStringViewLiteral(str) QStringView(QT_UNICODE_LITERAL(str))
#endif

namespace QtPrivate {
template <typename Char>
struct IsCompatibleCharTypeHelper
    : std::integral_constant<bool,
                             std::is_same<Char, QChar>::value ||
                             std::is_same<Char, ushort>::value ||
#if !defined(Q_OS_WIN) || defined(Q_COMPILER_UNICODE_STRINGS)
                             std::is_same<Char, char16_t>::value ||
#endif
                             (std::is_same<Char, wchar_t>::value && sizeof(wchar_t) == sizeof(QChar))> {};
template <typename Char>
struct IsCompatibleCharType
    : IsCompatibleCharTypeHelper<typename std::remove_cv<typename std::remove_reference<Char>::type>::type> {};

template <typename Array>
struct IsCompatibleArrayHelper : std::false_type {};
template <typename Char, size_t N>
struct IsCompatibleArrayHelper<Char[N]>
    : IsCompatibleCharType<Char> {};
template <typename Array>
struct IsCompatibleArray
    : IsCompatibleArrayHelper<typename std::remove_cv<typename std::remove_reference<Array>::type>::type> {};

template <typename Pointer>
struct IsCompatiblePointerHelper : std::false_type {};
template <typename Char>
struct IsCompatiblePointerHelper<Char*>
    : IsCompatibleCharType<Char> {};
template <typename Pointer>
struct IsCompatiblePointer
    : IsCompatiblePointerHelper<typename std::remove_cv<typename std::remove_reference<Pointer>::type>::type> {};

template <typename T>
struct IsCompatibleStdBasicStringHelper : std::false_type {};
template <typename Char, typename...Args>
struct IsCompatibleStdBasicStringHelper<std::basic_string<Char, Args...> >
    : IsCompatibleCharType<Char> {};

template <typename T>
struct IsCompatibleStdBasicString
    : IsCompatibleStdBasicStringHelper<
        typename std::remove_cv<typename std::remove_reference<T>::type>::type
      > {};

} // namespace QtPrivate

class QStringView
{
public:
#if defined(Q_OS_WIN) && !defined(Q_COMPILER_UNICODE_STRINGS)
    typedef wchar_t storage_type;
#else
    typedef char16_t storage_type;
#endif
    typedef const QChar value_type;
    typedef std::ptrdiff_t difference_type;
    typedef qssize_t size_type;
    typedef value_type &reference;
    typedef value_type &const_reference;
    typedef value_type *pointer;
    typedef value_type *const_pointer;

    typedef pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

private:
    template <typename Char>
    using if_compatible_char = typename std::enable_if<QtPrivate::IsCompatibleCharType<Char>::value, bool>::type;

    template <typename Array>
    using if_compatible_array = typename std::enable_if<QtPrivate::IsCompatibleArray<Array>::value, bool>::type;

    template <typename Pointer>
    using if_compatible_pointer = typename std::enable_if<QtPrivate::IsCompatiblePointer<Pointer>::value, bool>::type;

    template <typename T>
    using if_compatible_string = typename std::enable_if<QtPrivate::IsCompatibleStdBasicString<T>::value, bool>::type;

    template <typename T>
    using if_compatible_qstring_like = typename std::enable_if<std::is_same<T, QString>::value || std::is_same<T, QStringRef>::value, bool>::type;

    template <typename Char, size_t N>
    static Q_DECL_CONSTEXPR qssize_t lengthHelperArray(const Char (&)[N]) Q_DECL_NOTHROW
    {
        return qssize_t(N - 1);
    }
    template <typename Char>
    static Q_DECL_RELAXED_CONSTEXPR qssize_t lengthHelperPointer(const Char *str) Q_DECL_NOTHROW
    {
        qssize_t result = 0;
        while (*str++)
            ++result;
        return result;
    }
    static Q_DECL_RELAXED_CONSTEXPR qssize_t lengthHelperPointer(const QChar *str) Q_DECL_NOTHROW
    {
        qssize_t result = 0;
        while (!str++->isNull())
            ++result;
        return result;
    }

    template <typename Char>
    static const storage_type *castHelper(const Char *str) Q_DECL_NOTHROW
    { return reinterpret_cast<const storage_type*>(str); }
    static Q_DECL_CONSTEXPR const storage_type *castHelper(const storage_type *str) Q_DECL_NOTHROW
    { return str; }

public:
    Q_DECL_CONSTEXPR QStringView() Q_DECL_NOTHROW
        : m_size(0), m_data(nullptr) {}
    Q_DECL_CONSTEXPR QStringView(std::nullptr_t) Q_DECL_NOTHROW
        : QStringView() {}

    template <typename Char, if_compatible_char<Char> = true>
    Q_DECL_CONSTEXPR QStringView(const Char *str, qssize_t len)
        : m_size((Q_ASSERT(len >= 0), Q_ASSERT(str || !len), len)),
          m_data(castHelper(str)) {}

#ifdef Q_QDOC
    template <typename Char, size_t N>
    Q_DECL_CONSTEXPR QStringView(const Char (&array)[N]) Q_DECL_NOTHROW;

    template <typename Char>
    Q_DECL_CONSTEXPR QStringView(const Char *str) Q_DECL_NOTHROW;
#else
    template <typename Array, if_compatible_array<Array> = true>
    Q_DECL_CONSTEXPR QStringView(const Array &str) Q_DECL_NOTHROW
        : QStringView(str, lengthHelperArray(str)) {}

    template <typename Pointer, if_compatible_pointer<Pointer> = true>
    Q_DECL_CONSTEXPR QStringView(const Pointer &str) Q_DECL_NOTHROW
        : QStringView(str, str ? lengthHelperPointer(str) : 0) {}
#endif

#ifdef Q_QDOC
    QStringView(const QString &str) Q_DECL_NOTHROW;
    QStringView(const QStringRef &str) Q_DECL_NOTHROW;
#else
    template <typename String, if_compatible_qstring_like<String> = true>
    QStringView(const String &str) Q_DECL_NOTHROW
        : QStringView(str.isNull() ? nullptr : str.data(), qssize_t(str.size())) {}
#endif

    template <typename StdBasicString, if_compatible_string<StdBasicString> = true>
    QStringView(const StdBasicString &str) Q_DECL_NOTHROW
        : QStringView(str.data(), qssize_t(str.size())) {}

    QString toString() const Q_REQUIRED_RESULT { return Q_ASSERT(size() == length()), QString(data(), length()); }

    Q_DECL_CONSTEXPR qssize_t size() const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return m_size; }
    const_pointer data() const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return reinterpret_cast<const_pointer>(m_data); }
    Q_DECL_CONSTEXPR const storage_type *utf16() const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return m_data; }

    Q_DECL_CONSTEXPR QChar operator[](qssize_t n) const Q_REQUIRED_RESULT
    { return Q_ASSERT(n >= 0), Q_ASSERT(n < size()), QChar(m_data[n]); }

    //
    // QString API
    //

    QByteArray toLatin1() const Q_REQUIRED_RESULT { return qConvertToLatin1(*this); }
    QByteArray toUtf8() const Q_REQUIRED_RESULT { return qConvertToUtf8(*this); }
    QByteArray toLocal8Bit() const Q_REQUIRED_RESULT { return qConvertToLocal8Bit(*this); }
    inline QVector<uint> toUcs4() const Q_REQUIRED_RESULT;

    Q_DECL_CONSTEXPR QChar at(qssize_t n) const Q_REQUIRED_RESULT { return (*this)[n]; }

    Q_DECL_CONSTEXPR QStringView mid(qssize_t pos) const Q_REQUIRED_RESULT
    { return Q_ASSERT(pos >= 0), Q_ASSERT(pos <= size()), QStringView(m_data + pos, m_size - pos); }
    Q_DECL_CONSTEXPR QStringView mid(qssize_t pos, qssize_t n) const Q_REQUIRED_RESULT
    { return Q_ASSERT(pos >= 0), Q_ASSERT(n >= 0), Q_ASSERT(pos + n <= size()), QStringView(m_data + pos, n); }
    Q_DECL_CONSTEXPR QStringView left(qssize_t n) const Q_REQUIRED_RESULT
    { return Q_ASSERT(n >= 0), Q_ASSERT(n <= size()), QStringView(m_data, n); }
    Q_DECL_CONSTEXPR QStringView right(qssize_t n) const Q_REQUIRED_RESULT
    { return Q_ASSERT(n >= 0), Q_ASSERT(n <= size()), QStringView(m_data + m_size - n, n); }
    Q_DECL_CONSTEXPR QStringView chopped(qssize_t n) const Q_REQUIRED_RESULT
    { return Q_ASSERT(n >= 0), Q_ASSERT(n <= size()), QStringView(m_data, m_size - n); }

    Q_DECL_RELAXED_CONSTEXPR void truncate(qssize_t n)
    { Q_ASSERT(n >= 0); Q_ASSERT(n <= size()); m_size = n; }
    Q_DECL_RELAXED_CONSTEXPR void chop(qssize_t n)
    { Q_ASSERT(n >= 0); Q_ASSERT(n <= size()); m_size -= n; }

    //
    // STL compatibility API:
    //
    const_iterator begin()   const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return data(); }
    const_iterator end()     const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return data() + size(); }
    const_iterator cbegin()  const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return begin(); }
    const_iterator cend()    const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return end(); }
    const_reverse_iterator rbegin()  const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return const_reverse_iterator(end()); }
    const_reverse_iterator rend()    const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return rbegin(); }
    const_reverse_iterator crend()   const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return rend(); }

    Q_DECL_CONSTEXPR bool empty() const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return size() == 0; }
    Q_DECL_CONSTEXPR QChar front() const Q_REQUIRED_RESULT { return Q_ASSERT(!empty()), QChar(m_data[0]); }
    Q_DECL_CONSTEXPR QChar back()  const Q_REQUIRED_RESULT { return Q_ASSERT(!empty()), QChar(m_data[m_size - 1]); }

    //
    // Qt compatibility API:
    //
    Q_DECL_CONSTEXPR bool isNull() const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return !m_data; }
    Q_DECL_CONSTEXPR bool isEmpty() const Q_DECL_NOTHROW Q_REQUIRED_RESULT { return empty(); }
    Q_DECL_CONSTEXPR int length() const /* not nothrow! */ Q_REQUIRED_RESULT
    { return Q_ASSERT(int(size()) == size()), int(size()); }
    Q_DECL_CONSTEXPR QChar first() const Q_REQUIRED_RESULT { return front(); }
    Q_DECL_CONSTEXPR QChar last()  const Q_REQUIRED_RESULT { return back(); }
private:
    qssize_t m_size;
    const storage_type *m_data;
};
Q_DECLARE_TYPEINFO(QStringView, Q_MOVABLE_TYPE);

template <typename QStringLike, typename std::enable_if<
    std::is_same<QStringLike, QString>::value || std::is_same<QStringLike, QStringRef>::value,
    bool>::type = true>
inline QStringView qToStringViewIgnoringNull(const QStringLike &s) Q_DECL_NOTHROW
{ return QStringView(s.data(), s.size()); }

QT_END_NAMESPACE

#endif /* QSTRINGVIEW_H */
