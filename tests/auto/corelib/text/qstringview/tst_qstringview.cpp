/****************************************************************************
**
** Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QStringView>
#include <QString>
#include <QChar>
#include <QStringRef>

#include <QTest>

#include <string>

template <typename T>
using CanConvert = std::is_convertible<T, QStringView>;

Q_STATIC_ASSERT(!CanConvert<QLatin1String>::value);
Q_STATIC_ASSERT(!CanConvert<const char*>::value);
Q_STATIC_ASSERT(!CanConvert<QByteArray>::value);

// QStringView qchar_does_not_compile() { return QStringView(QChar('a')); }
// QStringView qlatin1string_does_not_compile() { return QStringView(QLatin1String("a")); }
// QStringView const_char_star_does_not_compile() { return QStringView("a"); }
// QStringView qbytearray_does_not_compile() { return QStringView(QByteArray("a")); }

//
// QChar
//

Q_STATIC_ASSERT(!CanConvert<QChar>::value);

Q_STATIC_ASSERT(CanConvert<QChar[123]>::value);

Q_STATIC_ASSERT(CanConvert<      QString >::value);
Q_STATIC_ASSERT(CanConvert<const QString >::value);
Q_STATIC_ASSERT(CanConvert<      QString&>::value);
Q_STATIC_ASSERT(CanConvert<const QString&>::value);

Q_STATIC_ASSERT(CanConvert<      QStringRef >::value);
Q_STATIC_ASSERT(CanConvert<const QStringRef >::value);
Q_STATIC_ASSERT(CanConvert<      QStringRef&>::value);
Q_STATIC_ASSERT(CanConvert<const QStringRef&>::value);


//
// ushort
//

Q_STATIC_ASSERT(!CanConvert<ushort>::value);

Q_STATIC_ASSERT(CanConvert<ushort[123]>::value);

Q_STATIC_ASSERT(CanConvert<      ushort*>::value);
Q_STATIC_ASSERT(CanConvert<const ushort*>::value);


//
// char16_t
//

#if defined(Q_COMPILER_UNICODE_STRINGS)

Q_STATIC_ASSERT(!CanConvert<char16_t>::value);

Q_STATIC_ASSERT(CanConvert<      char16_t*>::value);
Q_STATIC_ASSERT(CanConvert<const char16_t*>::value);

#endif

#if defined(Q_STDLIB_UNICODE_STRINGS)

Q_STATIC_ASSERT(CanConvert<      std::u16string >::value);
Q_STATIC_ASSERT(CanConvert<const std::u16string >::value);
Q_STATIC_ASSERT(CanConvert<      std::u16string&>::value);
Q_STATIC_ASSERT(CanConvert<const std::u16string&>::value);

#endif


//
// wchar_t
//

Q_CONSTEXPR bool CanConvertFromWCharT =
#ifdef Q_OS_WIN
        true
#else
        false
#endif
        ;

Q_STATIC_ASSERT(!CanConvert<wchar_t>::value);

Q_STATIC_ASSERT(CanConvert<      wchar_t*>::value == CanConvertFromWCharT);
Q_STATIC_ASSERT(CanConvert<const wchar_t*>::value == CanConvertFromWCharT);

Q_STATIC_ASSERT(CanConvert<      std::wstring >::value == CanConvertFromWCharT);
Q_STATIC_ASSERT(CanConvert<const std::wstring >::value == CanConvertFromWCharT);
Q_STATIC_ASSERT(CanConvert<      std::wstring&>::value == CanConvertFromWCharT);
Q_STATIC_ASSERT(CanConvert<const std::wstring&>::value == CanConvertFromWCharT);


class tst_QStringView : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constExpr() const;
    void basics() const;
    void literals() const;
    void at() const;

    void arg() const;

    void fromQString() const;
    void fromQStringRef() const;

    void fromQCharStar() const
    {
        const QChar str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', 0 };
        fromLiteral(str);
    }

    void fromUShortStar() const
    {
        const ushort str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', 0 };
        fromLiteral(str);
    }

    void fromChar16TStar() const
    {
#if defined(Q_COMPILER_UNICODE_STRINGS)
        fromLiteral(u"Hello, World!");
#else
        QSKIP("This test requires C++11 char16_t support enabled in the compiler");
#endif
    }

    void fromWCharTStar() const
    {
#ifdef Q_OS_WIN
        fromLiteral(L"Hello, World!");
#else
        QSKIP("This is a Windows-only test");
#endif
    }

    void fromQCharRange() const
    {
        const QChar str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
        fromRange(std::begin(str), std::end(str));
    }

    void fromUShortRange() const
    {
        const ushort str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
        fromRange(std::begin(str), std::end(str));
    }

    void fromChar16TRange() const
    {
#if defined(Q_COMPILER_UNICODE_STRINGS)
        const char16_t str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
        fromRange(std::begin(str), std::end(str));
#else
        QSKIP("This test requires C++11 char16_t support enabled in the compiler");
#endif
    }

    void fromWCharTRange() const
    {
#ifdef Q_OS_WIN
        const wchar_t str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
        fromRange(std::begin(str), std::end(str));
#else
        QSKIP("This is a Windows-only test");
#endif
    }

    // std::basic_string
    void fromStdStringWCharT() const
    {
#ifdef Q_OS_WIN
        fromStdString<wchar_t>();
#else
        QSKIP("This is a Windows-only test");
#endif
    }
    void fromStdStringChar16T() const
    {
#ifdef Q_STDLIB_UNICODE_STRINGS
        fromStdString<char16_t>();
#else
        QSKIP("This test requires C++11 char16_t support enabled in compiler & stdlib");
#endif
    }

    void comparison();

    void overloadResolution();

private:
    template <typename String>
    void conversion_tests(String arg) const;
    template <typename Char>
    void fromLiteral(const Char *arg) const;
    template <typename Char>
    void fromRange(const Char *first, const Char *last) const;
    template <typename Char, typename Container>
    void fromContainer() const;
    template <typename Char>
    void fromStdString() const { fromContainer<Char, std::basic_string<Char> >(); }
};

void tst_QStringView::constExpr() const
{
    // compile-time checks
#ifdef Q_COMPILER_CONSTEXPR
    {
        constexpr QStringView sv;
        Q_STATIC_ASSERT(sv.size() == 0);
        Q_STATIC_ASSERT(sv.isNull());
        Q_STATIC_ASSERT(sv.empty());
        Q_STATIC_ASSERT(sv.isEmpty());
        Q_STATIC_ASSERT(sv.utf16() == nullptr);

        constexpr QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
        Q_STATIC_ASSERT(sv2.isNull());
        Q_STATIC_ASSERT(sv2.empty());
    }
    {
        constexpr QStringView sv = nullptr;
        Q_STATIC_ASSERT(sv.size() == 0);
        Q_STATIC_ASSERT(sv.isNull());
        Q_STATIC_ASSERT(sv.empty());
        Q_STATIC_ASSERT(sv.isEmpty());
        Q_STATIC_ASSERT(sv.utf16() == nullptr);
    }
    {
        constexpr QStringView sv = u"";
        Q_STATIC_ASSERT(sv.size() == 0);
        Q_STATIC_ASSERT(!sv.isNull());
        Q_STATIC_ASSERT(sv.empty());
        Q_STATIC_ASSERT(sv.isEmpty());
        Q_STATIC_ASSERT(sv.utf16() != nullptr);

        constexpr QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
        Q_STATIC_ASSERT(!sv2.isNull());
        Q_STATIC_ASSERT(sv2.empty());
    }
    {
        constexpr QStringView sv = u"Hello";
        Q_STATIC_ASSERT(sv.size() == 5);
        Q_STATIC_ASSERT(!sv.empty());
        Q_STATIC_ASSERT(!sv.isEmpty());
        Q_STATIC_ASSERT(!sv.isNull());
        Q_STATIC_ASSERT(*sv.utf16() == 'H');
        Q_STATIC_ASSERT(sv[0]      == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv.at(0)   == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv.front() == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv.first() == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv[4]      == QLatin1Char('o'));
        Q_STATIC_ASSERT(sv.at(4)   == QLatin1Char('o'));
        Q_STATIC_ASSERT(sv.back()  == QLatin1Char('o'));
        Q_STATIC_ASSERT(sv.last()  == QLatin1Char('o'));

        constexpr QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
        Q_STATIC_ASSERT(!sv2.isNull());
        Q_STATIC_ASSERT(!sv2.empty());
        Q_STATIC_ASSERT(sv2.size() == 5);
    }
#if !defined(Q_OS_WIN) || defined(Q_COMPILER_UNICODE_STRINGS)
    {
        Q_STATIC_ASSERT(QStringView(u"Hello").size() == 5);
        constexpr QStringView sv = u"Hello";
        Q_STATIC_ASSERT(sv.size() == 5);
        Q_STATIC_ASSERT(!sv.empty());
        Q_STATIC_ASSERT(!sv.isEmpty());
        Q_STATIC_ASSERT(!sv.isNull());
        Q_STATIC_ASSERT(*sv.utf16() == 'H');
        Q_STATIC_ASSERT(sv[0]      == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv.at(0)   == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv.front() == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv.first() == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv[4]      == QLatin1Char('o'));
        Q_STATIC_ASSERT(sv.at(4)   == QLatin1Char('o'));
        Q_STATIC_ASSERT(sv.back()  == QLatin1Char('o'));
        Q_STATIC_ASSERT(sv.last()  == QLatin1Char('o'));

        constexpr QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
        Q_STATIC_ASSERT(!sv2.isNull());
        Q_STATIC_ASSERT(!sv2.empty());
        Q_STATIC_ASSERT(sv2.size() == 5);

        constexpr char16_t *null = nullptr;
        constexpr QStringView sv3(null);
        Q_STATIC_ASSERT(sv3.isNull());
        Q_STATIC_ASSERT(sv3.isEmpty());
        Q_STATIC_ASSERT(sv3.size() == 0);
    }
#else // storage_type is wchar_t
    {
        Q_STATIC_ASSERT(QStringView(L"Hello").size() == 5);
        constexpr QStringView sv = L"Hello";
        Q_STATIC_ASSERT(sv.size() == 5);
        Q_STATIC_ASSERT(!sv.empty());
        Q_STATIC_ASSERT(!sv.isEmpty());
        Q_STATIC_ASSERT(!sv.isNull());
        Q_STATIC_ASSERT(*sv.utf16() == 'H');
        Q_STATIC_ASSERT(sv[0]      == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv.at(0)   == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv.front() == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv.first() == QLatin1Char('H'));
        Q_STATIC_ASSERT(sv[4]      == QLatin1Char('o'));
        Q_STATIC_ASSERT(sv.at(4)   == QLatin1Char('o'));
        Q_STATIC_ASSERT(sv.back()  == QLatin1Char('o'));
        Q_STATIC_ASSERT(sv.last()  == QLatin1Char('o'));

        constexpr QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
        Q_STATIC_ASSERT(!sv2.isNull());
        Q_STATIC_ASSERT(!sv2.empty());
        Q_STATIC_ASSERT(sv2.size() == 5);

        constexpr wchar_t *null = nullptr;
        constexpr QStringView sv3(null);
        Q_STATIC_ASSERT(sv3.isNull());
        Q_STATIC_ASSERT(sv3.isEmpty());
        Q_STATIC_ASSERT(sv3.size() == 0);
    }
#endif
#endif
}

void tst_QStringView::basics() const
{
    QStringView sv1;

    // a default-constructed QStringView is null:
    QVERIFY(sv1.isNull());
    // which implies it's empty();
    QVERIFY(sv1.isEmpty());

    QStringView sv2;

    QVERIFY(sv2 == sv1);
    QVERIFY(!(sv2 != sv1));
}

void tst_QStringView::literals() const
{
#if !defined(Q_OS_WIN) || defined(Q_COMPILER_UNICODE_STRINGS)
    const char16_t hello[] = u"Hello";
    const char16_t longhello[] =
            u"Hello World. This is a much longer message, to exercise qustrlen.";
    const char16_t withnull[] = u"a\0zzz";
#else // storage_type is wchar_t
    const wchar_t hello[] = L"Hello";
    const wchar_t longhello[] =
            L"Hello World. This is a much longer message, to exercise qustrlen.";
    const wchar_t withnull[] = L"a\0zzz";
#endif
    Q_STATIC_ASSERT(sizeof(longhello) >= 16);

    QCOMPARE(QStringView(hello).size(), 5);
    QCOMPARE(QStringView(hello + 0).size(), 5); // forces decay to pointer
    QStringView sv = hello;
    QCOMPARE(sv.size(), 5);
    QVERIFY(!sv.empty());
    QVERIFY(!sv.isEmpty());
    QVERIFY(!sv.isNull());
    QCOMPARE(*sv.utf16(), 'H');
    QCOMPARE(sv[0],      QLatin1Char('H'));
    QCOMPARE(sv.at(0),   QLatin1Char('H'));
    QCOMPARE(sv.front(), QLatin1Char('H'));
    QCOMPARE(sv.first(), QLatin1Char('H'));
    QCOMPARE(sv[4],      QLatin1Char('o'));
    QCOMPARE(sv.at(4),   QLatin1Char('o'));
    QCOMPARE(sv.back(),  QLatin1Char('o'));
    QCOMPARE(sv.last(),  QLatin1Char('o'));

    QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
    QVERIFY(!sv2.isNull());
    QVERIFY(!sv2.empty());
    QCOMPARE(sv2.size(), 5);

    QStringView sv3(longhello);
    QCOMPARE(size_t(sv3.size()), sizeof(longhello)/sizeof(longhello[0]) - 1);
    QCOMPARE(sv3.last(), QLatin1Char('.'));
    sv3 = longhello;
    QCOMPARE(size_t(sv3.size()), sizeof(longhello)/sizeof(longhello[0]) - 1);

    for (int i = 0; i < sv3.size(); ++i) {
        QStringView sv4(longhello + i);
        QCOMPARE(size_t(sv4.size()), sizeof(longhello)/sizeof(longhello[0]) - 1 - i);
        QCOMPARE(sv4.last(), QLatin1Char('.'));
        sv4 = longhello + i;
        QCOMPARE(size_t(sv4.size()), sizeof(longhello)/sizeof(longhello[0]) - 1 - i);
    }

    // these are different results
    QCOMPARE(size_t(QStringView(withnull).size()), sizeof(withnull)/sizeof(withnull[0]) - 1);
    QCOMPARE(QStringView(withnull + 0).size(), 1);
}

void tst_QStringView::at() const
{
    QString hello("Hello");
    QStringView sv(hello);
    QCOMPARE(sv.at(0), QChar('H')); QCOMPARE(sv[0], QChar('H'));
    QCOMPARE(sv.at(1), QChar('e')); QCOMPARE(sv[1], QChar('e'));
    QCOMPARE(sv.at(2), QChar('l')); QCOMPARE(sv[2], QChar('l'));
    QCOMPARE(sv.at(3), QChar('l')); QCOMPARE(sv[3], QChar('l'));
    QCOMPARE(sv.at(4), QChar('o')); QCOMPARE(sv[4], QChar('o'));
}

void tst_QStringView::arg() const
{
#define CHECK1(pattern, arg1, expected) \
    do { \
        auto p = QStringView(u"" pattern); \
        QCOMPARE(p.arg(QLatin1String(arg1)), expected); \
        QCOMPARE(p.arg(u"" arg1), expected); \
        QCOMPARE(p.arg(QStringLiteral(arg1)), expected); \
        QCOMPARE(p.arg(QString(QLatin1String(arg1))), expected); \
    } while (false) \
    /*end*/
#define CHECK2(pattern, arg1, arg2, expected) \
    do { \
        auto p = QStringView(u"" pattern); \
        QCOMPARE(p.arg(QLatin1String(arg1), QLatin1String(arg2)), expected); \
        QCOMPARE(p.arg(u"" arg1, QLatin1String(arg2)), expected); \
        QCOMPARE(p.arg(QLatin1String(arg1), u"" arg2), expected); \
        QCOMPARE(p.arg(u"" arg1, u"" arg2), expected); \
    } while (false) \
    /*end*/

    CHECK1("", "World", "");
    CHECK1("%1", "World", "World");
    CHECK1("!%1?", "World", "!World?");
    CHECK1("%1%1", "World", "WorldWorld");
    CHECK1("%1%2", "World", "World%2");
    CHECK1("%2%1", "World", "%2World");

    CHECK2("", "Hello", "World", "");
    CHECK2("%1", "Hello", "World", "Hello");
    CHECK2("!%1, %2?", "Hello", "World", "!Hello, World?");
    CHECK2("%1%1", "Hello", "World", "HelloHello");
    CHECK2("%1%2", "Hello", "World", "HelloWorld");
    CHECK2("%2%1", "Hello", "World", "WorldHello");

#undef CHECK2
#undef CHECK1

    QCOMPARE(QStringView(u" %2 %2 %1 %3 ").arg(QLatin1Char('c'), QChar::CarriageReturn, u'C'), " \r \r c C ");
}

void tst_QStringView::fromQString() const
{
    QString null;
    QString empty = "";

    QVERIFY( QStringView(null).isNull());
    QVERIFY( QStringView(null).isEmpty());
    QVERIFY( QStringView(empty).isEmpty());
    QVERIFY(!QStringView(empty).isNull());

    conversion_tests(QString("Hello World!"));
}

void tst_QStringView::fromQStringRef() const
{
    QStringRef null;
    QString emptyS = "";
    QStringRef empty(&emptyS);

    QVERIFY( QStringView(null).isNull());
    QVERIFY( QStringView(null).isEmpty());
    QVERIFY( QStringView(empty).isEmpty());
    QVERIFY(!QStringView(empty).isNull());

    conversion_tests(QString("Hello World!").midRef(6));
}

template <typename Char>
void tst_QStringView::fromLiteral(const Char *arg) const
{
    const Char *null = nullptr;
    const Char empty[] = { 0 };

    QCOMPARE(QStringView(null).size(), qsizetype(0));
    QCOMPARE(QStringView(null).data(), nullptr);
    QCOMPARE(QStringView(empty).size(), qsizetype(0));
    QCOMPARE(static_cast<const void*>(QStringView(empty).data()),
             static_cast<const void*>(empty));

    QVERIFY( QStringView(null).isNull());
    QVERIFY( QStringView(null).isEmpty());
    QVERIFY( QStringView(empty).isEmpty());
    QVERIFY(!QStringView(empty).isNull());

    conversion_tests(arg);
}

template <typename Char>
void tst_QStringView::fromRange(const Char *first, const Char *last) const
{
    const Char *null = nullptr;
    QCOMPARE(QStringView(null, null).size(), 0);
    QCOMPARE(QStringView(null, null).data(), nullptr);
    QCOMPARE(QStringView(first, first).size(), 0);
    QCOMPARE(static_cast<const void*>(QStringView(first, first).data()),
             static_cast<const void*>(first));

    const auto sv = QStringView(first, last);
    QCOMPARE(sv.size(), last - first);
    QCOMPARE(static_cast<const void*>(sv.data()),
             static_cast<const void*>(first));

    // can't call conversion_tests() here, as it requires a single object
}

template <typename Char, typename Container>
void tst_QStringView::fromContainer() const
{
    const QString s = "Hello World!";

    Container c;
    // unspecified whether empty containers make null QStringViews
    QVERIFY(QStringView(c).isEmpty());

    QCOMPARE(sizeof(Char), sizeof(QChar));

    const auto *data = reinterpret_cast<const Char *>(s.utf16());
    std::copy(data, data + s.size(), std::back_inserter(c));
    conversion_tests(std::move(c));
}

namespace help {
template <typename T>
size_t size(const T &t) { return size_t(t.size()); }
template <typename T>
size_t size(const T *t)
{
    size_t result = 0;
    if (t) {
        while (*t++)
            ++result;
    }
    return result;
}
size_t size(const QChar *t)
{
    size_t result = 0;
    if (t) {
        while (!t++->isNull())
            ++result;
    }
    return result;
}

template <typename T>
typename T::const_iterator cbegin(const T &t) { return t.cbegin(); }
template <typename T>
const T *                  cbegin(const T *t) { return t; }

template <typename T>
typename T::const_iterator cend(const T &t) { return t.cend(); }
template <typename T>
const T *                  cend(const T *t) { return t + size(t); }

template <typename T>
typename T::const_reverse_iterator crbegin(const T &t) { return t.crbegin(); }
template <typename T>
std::reverse_iterator<const T*>    crbegin(const T *t) { return std::reverse_iterator<const T*>(cend(t)); }

template <typename T>
typename T::const_reverse_iterator crend(const T &t) { return t.crend(); }
template <typename T>
std::reverse_iterator<const T*>    crend(const T *t) { return std::reverse_iterator<const T*>(cbegin(t)); }

} // namespace help

template <typename String>
void tst_QStringView::conversion_tests(String string) const
{
    // copy-construct:
    {
        QStringView sv = string;

        QCOMPARE(help::size(sv), help::size(string));

        // check iterators:

        QVERIFY(std::equal(help::cbegin(string), help::cend(string),
                           QT_MAKE_CHECKED_ARRAY_ITERATOR(sv.cbegin(), sv.size())));
        QVERIFY(std::equal(help::cbegin(string), help::cend(string),
                           QT_MAKE_CHECKED_ARRAY_ITERATOR(sv.begin(), sv.size())));
        QVERIFY(std::equal(help::crbegin(string), help::crend(string),
                           sv.crbegin()));
        QVERIFY(std::equal(help::crbegin(string), help::crend(string),
                           sv.rbegin()));

        QCOMPARE(sv, string);
    }

    QStringView sv;

    // copy-assign:
    {
        sv = string;

        QCOMPARE(help::size(sv), help::size(string));

        // check relational operators:

        QCOMPARE(sv, string);
        QCOMPARE(string, sv);

        QVERIFY(!(sv != string));
        QVERIFY(!(string != sv));

        QVERIFY(!(sv < string));
        QVERIFY(sv <= string);
        QVERIFY(!(sv > string));
        QVERIFY(sv >= string);

        QVERIFY(!(string < sv));
        QVERIFY(string <= sv);
        QVERIFY(!(string > sv));
        QVERIFY(string >= sv);
    }

    // copy-construct from rvalue (QStringView never assumes ownership):
    {
        QStringView sv2 = std::move(string);
        QCOMPARE(sv2, sv);
        QCOMPARE(sv2, string);
    }

    // copy-assign from rvalue (QStringView never assumes ownership):
    {
        QStringView sv2;
        sv2 = std::move(string);
        QCOMPARE(sv2, sv);
        QCOMPARE(sv2, string);
    }
}

void tst_QStringView::comparison()
{
    const QStringView aa = u"aa";
    const QStringView upperAa = u"AA";
    const QStringView bb = u"bb";

    QVERIFY(aa == aa);
    QVERIFY(aa != bb);
    QVERIFY(aa < bb);
    QVERIFY(bb > aa);

    QCOMPARE(aa.compare(aa), 0);
    QVERIFY(aa.compare(upperAa) != 0);
    QCOMPARE(aa.compare(upperAa, Qt::CaseInsensitive), 0);
    QVERIFY(aa.compare(bb) < 0);
    QVERIFY(bb.compare(aa) > 0);
}

namespace QStringViewOverloadResolution {
static void test(QString) = delete;
static void test(QStringView) {}
}

// Compile-time only test: overload resolution prefers QStringView over QString
void tst_QStringView::overloadResolution()
{
    {
        QChar qcharArray[42] = {};
        QStringViewOverloadResolution::test(qcharArray);
        QChar *qcharPointer = qcharArray;
        QStringViewOverloadResolution::test(qcharPointer);
    }

    {
        ushort ushortArray[42] = {};
        QStringViewOverloadResolution::test(ushortArray);
        ushort *ushortPointer = ushortArray;
        QStringViewOverloadResolution::test(ushortPointer);
    }

    {
        QStringRef stringRef;
        QStringViewOverloadResolution::test(stringRef);
        QStringViewOverloadResolution::test(qAsConst(stringRef));
        QStringViewOverloadResolution::test(std::move(stringRef));
    }

#if defined(Q_OS_WIN)
    {
        wchar_t wchartArray[42] = {};
        QStringViewOverloadResolution::test(wchartArray);
        QStringViewOverloadResolution::test(L"test");
    }
#endif

#if defined(Q_COMPILER_UNICODE_STRINGS)
    {
        char16_t char16Array[] = u"test";
        QStringViewOverloadResolution::test(char16Array);
        char16_t *char16Pointer = char16Array;
        QStringViewOverloadResolution::test(char16Pointer);
    }
#endif

#if defined(Q_STDLIB_UNICODE_STRINGS)
    {
        std::u16string string;
        QStringViewOverloadResolution::test(string);
        QStringViewOverloadResolution::test(qAsConst(string));
        QStringViewOverloadResolution::test(std::move(string));
    }
#endif
}

QTEST_APPLESS_MAIN(tst_QStringView)
#include "tst_qstringview.moc"
