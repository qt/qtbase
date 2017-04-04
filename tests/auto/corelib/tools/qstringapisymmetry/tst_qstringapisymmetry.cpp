/****************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#undef QT_NO_CAST_FROM_ASCII
#undef QT_NO_CAST_TO_ASCII
#undef QT_ASCII_CAST_WARNINGS

#include <QString>
#include <QStringView>
#include <QChar>
#include <QStringRef>
#include <QLatin1String>
#include <QVector>

#include <QTest>

Q_DECLARE_METATYPE(QLatin1String)
Q_DECLARE_METATYPE(QStringRef)

template <typename T>
QString toQString(const T &t) { return QString(t); }
QString toQString(const QStringRef &ref) { return ref.toString(); }
QString toQString(QStringView view) { return view.toString(); }

// FIXME: these are missing at the time of writing, add them, then remove the dummies here:
#define MAKE_RELOP(op, A1, A2) \
    static bool operator op (A1 lhs, A2 rhs) \
    { return toQString(lhs) op toQString(rhs); } \
    /*end*/
#define MAKE_ALL(A1, A2) \
    MAKE_RELOP(==, A1, A2) \
    MAKE_RELOP(!=, A1, A2) \
    MAKE_RELOP(<,  A1, A2) \
    MAKE_RELOP(>,  A1, A2) \
    MAKE_RELOP(<=, A1, A2) \
    MAKE_RELOP(>=, A1, A2) \
    /*end*/

MAKE_ALL(QByteArray, QChar)
MAKE_ALL(QByteArray, QLatin1String)

MAKE_ALL(const char*, QChar)

#undef MAKE_ALL
#undef MAKE_RELOP
// END FIXME

class tst_QStringApiSymmetry : public QObject
{
    Q_OBJECT

    //
    // Mixed UTF-16, UTF-8, Latin-1 checks:
    //

    void compare_data(bool hasConceptOfNullAndEmpty=true);
    template <typename LHS, typename RHS>
    void compare_impl() const;

private Q_SLOTS:
    // test all combinations of {QChar, QStringRef, QString, QStringView, QLatin1String, QByteArray, const char*}
    void compare_QChar_QChar_data() { compare_data(false); }
    void compare_QChar_QChar() { compare_impl<QChar, QChar>(); }
    void compare_QChar_QStringRef_data() { compare_data(false); }
    void compare_QChar_QStringRef() { compare_impl<QChar, QStringRef>(); }
    void compare_QChar_QString_data() { compare_data(false); }
    void compare_QChar_QString() { compare_impl<QChar, QString>(); }
    void compare_QChar_QStringView_data() { compare_data(false); }
    void compare_QChar_QStringView() { compare_impl<QChar, QStringView>(); }
    void compare_QChar_QLatin1String_data() { compare_data(false); }
    void compare_QChar_QLatin1String() { compare_impl<QChar, QLatin1String>(); }
    void compare_QChar_QByteArray_data() { compare_data(false); }
    void compare_QChar_QByteArray() { compare_impl<QChar, QByteArray>(); }
    void compare_QChar_const_char_star_data() { compare_data(false); }
    void compare_QChar_const_char_star() { compare_impl<QChar, const char *>(); }

    void compare_QStringRef_QChar_data() { compare_data(false); }
    void compare_QStringRef_QChar() { compare_impl<QStringRef, QChar>(); }
    void compare_QStringRef_QStringRef_data() { compare_data(); }
    void compare_QStringRef_QStringRef() { compare_impl<QStringRef, QStringRef>(); }
    void compare_QStringRef_QString_data() { compare_data(); }
    void compare_QStringRef_QString() { compare_impl<QStringRef, QString>(); }
    void compare_QStringRef_QStringView_data() { compare_data(); }
    void compare_QStringRef_QStringView() { compare_impl<QStringRef, QStringView>(); }
    void compare_QStringRef_QLatin1String_data() { compare_data(); }
    void compare_QStringRef_QLatin1String() { compare_impl<QStringRef, QLatin1String>(); }
    void compare_QStringRef_QByteArray_data() { compare_data(); }
    void compare_QStringRef_QByteArray() { compare_impl<QStringRef, QByteArray>(); }
    void compare_QStringRef_const_char_star_data() { compare_data(); }
    void compare_QStringRef_const_char_star() { compare_impl<QStringRef, const char *>(); }

    void compare_QString_QChar_data() { compare_data(false); }
    void compare_QString_QChar() { compare_impl<QString, QChar>(); }
    void compare_QString_QStringRef_data() { compare_data(); }
    void compare_QString_QStringRef() { compare_impl<QString, QStringRef>(); }
    void compare_QString_QString_data() { compare_data(); }
    void compare_QString_QString() { compare_impl<QString, QString>(); }
    void compare_QString_QStringView_data() { compare_data(); }
    void compare_QString_QStringView() { compare_impl<QString, QStringView>(); }
    void compare_QString_QLatin1String_data() { compare_data(); }
    void compare_QString_QLatin1String() { compare_impl<QString, QLatin1String>(); }
    void compare_QString_QByteArray_data() { compare_data(); }
    void compare_QString_QByteArray() { compare_impl<QString, QByteArray>(); }
    void compare_QString_const_char_star_data() { compare_data(); }
    void compare_QString_const_char_star() { compare_impl<QString, const char *>(); }

    void compare_QStringView_QChar_data() { compare_data(false); }
    void compare_QStringView_QChar() { compare_impl<QStringView, QChar>(); }
    void compare_QStringView_QStringRef_data() { compare_data(); }
    void compare_QStringView_QStringRef() { compare_impl<QStringView, QStringRef>(); }
    void compare_QStringView_QString_data() { compare_data(); }
    void compare_QStringView_QString() { compare_impl<QStringView, QString>(); }
    void compare_QStringView_QStringView_data() { compare_data(); }
    void compare_QStringView_QStringView() { compare_impl<QStringView, QStringView>(); }
    void compare_QStringView_QLatin1String_data() { compare_data(); }
    void compare_QStringView_QLatin1String() { compare_impl<QStringView, QLatin1String>(); }

    void compare_QLatin1String_QChar_data() { compare_data(false); }
    void compare_QLatin1String_QChar() { compare_impl<QLatin1String, QChar>(); }
    void compare_QLatin1String_QStringRef_data() { compare_data(); }
    void compare_QLatin1String_QStringRef() { compare_impl<QLatin1String, QStringRef>(); }
    void compare_QLatin1String_QString_data() { compare_data(); }
    void compare_QLatin1String_QString() { compare_impl<QLatin1String, QString>(); }
    void compare_QLatin1String_QStringView_data() { compare_data(); }
    void compare_QLatin1String_QStringView() { compare_impl<QLatin1String, QStringView>(); }
    void compare_QLatin1String_QLatin1String_data() { compare_data(); }
    void compare_QLatin1String_QLatin1String() { compare_impl<QLatin1String, QLatin1String>(); }
    void compare_QLatin1String_QByteArray_data() { compare_data(); }
    void compare_QLatin1String_QByteArray() { compare_impl<QLatin1String, QByteArray>(); }
    void compare_QLatin1String_const_char_star_data() { compare_data(); }
    void compare_QLatin1String_const_char_star() { compare_impl<QLatin1String, const char *>(); }

    void compare_QByteArray_QChar_data() { compare_data(false); }
    void compare_QByteArray_QChar() { compare_impl<QByteArray, QChar>(); }
    void compare_QByteArray_QStringRef_data() { compare_data(); }
    void compare_QByteArray_QStringRef() { compare_impl<QByteArray, QStringRef>(); }
    void compare_QByteArray_QString_data() { compare_data(); }
    void compare_QByteArray_QString() { compare_impl<QByteArray, QString>(); }
    void compare_QByteArray_QLatin1String_data() { compare_data(); }
    void compare_QByteArray_QLatin1String() { compare_impl<QByteArray, QLatin1String>(); }
    void compare_QByteArray_QByteArray_data() { compare_data(); }
    void compare_QByteArray_QByteArray() { compare_impl<QByteArray, QByteArray>(); }
    void compare_QByteArray_const_char_star_data() { compare_data(); }
    void compare_QByteArray_const_char_star() { compare_impl<QByteArray, const char *>(); }

    void compare_const_char_star_QChar_data() { compare_data(false); }
    void compare_const_char_star_QChar() { compare_impl<const char *, QChar>(); }
    void compare_const_char_star_QStringRef_data() { compare_data(); }
    void compare_const_char_star_QStringRef() { compare_impl<const char *, QStringRef>(); }
    void compare_const_char_star_QString_data() { compare_data(); }
    void compare_const_char_star_QString() { compare_impl<const char *, QString>(); }
    void compare_const_char_star_QLatin1String_data() { compare_data(false); }
    void compare_const_char_star_QLatin1String() { compare_impl<const char *, QLatin1String>(); }
    void compare_const_char_star_QByteArray_data() { compare_data(); }
    void compare_const_char_star_QByteArray() { compare_impl<const char *, QByteArray>(); }
    //void compare_const_char_star_const_char_star_data() { compare_data(); }
    //void compare_const_char_star_const_char_star() { compare_impl<const char *, const char *>(); }

private:
    void mid_data();
    template <typename String> void mid_impl();

    void left_data();
    template <typename String> void left_impl();

    void right_data();
    template <typename String> void right_impl();

    void chop_data();
    template <typename String> void chop_impl();

    void truncate_data() { left_data(); }
    template <typename String> void truncate_impl();

private Q_SLOTS:

    void mid_QString_data() { mid_data(); }
    void mid_QString() { mid_impl<QString>(); }
    void mid_QStringRef_data() { mid_data(); }
    void mid_QStringRef() { mid_impl<QStringRef>(); }
    void mid_QStringView_data() { mid_data(); }
    void mid_QStringView() { mid_impl<QStringView>(); }
    void mid_QLatin1String_data() { mid_data(); }
    void mid_QLatin1String() { mid_impl<QLatin1String>(); }
    void mid_QByteArray_data() { mid_data(); }
    void mid_QByteArray() { mid_impl<QByteArray>(); }

    void left_QString_data() { left_data(); }
    void left_QString() { left_impl<QString>(); }
    void left_QStringRef_data() { left_data(); }
    void left_QStringRef() { left_impl<QStringRef>(); }
    void left_QStringView_data() { left_data(); }
    void left_QStringView() { left_impl<QStringView>(); }
    void left_QLatin1String_data() { left_data(); }
    void left_QLatin1String() { left_impl<QLatin1String>(); }
    void left_QByteArray_data() { left_data(); }
    void left_QByteArray() { left_impl<QByteArray>(); }

    void right_QString_data() { right_data(); }
    void right_QString() { right_impl<QString>(); }
    void right_QStringRef_data() { right_data(); }
    void right_QStringRef() { right_impl<QStringRef>(); }
    void right_QStringView_data() { right_data(); }
    void right_QStringView() { right_impl<QStringView>(); }
    void right_QLatin1String_data() { right_data(); }
    void right_QLatin1String() { right_impl<QLatin1String>(); }
    void right_QByteArray_data() { right_data(); }
    void right_QByteArray() { right_impl<QByteArray>(); }

    void chop_QString_data() { chop_data(); }
    void chop_QString() { chop_impl<QString>(); }
    void chop_QStringRef_data() { chop_data(); }
    void chop_QStringRef() { chop_impl<QStringRef>(); }
    void chop_QByteArray_data() { chop_data(); }
    void chop_QByteArray() { chop_impl<QByteArray>(); }

    void truncate_QString_data() { truncate_data(); }
    void truncate_QString() { truncate_impl<QString>(); }
    void truncate_QStringRef_data() { truncate_data(); }
    void truncate_QStringRef() { truncate_impl<QStringRef>(); }
    void truncate_QByteArray_data() { truncate_data(); }
    void truncate_QByteArray() { truncate_impl<QByteArray>(); }

    //
    // UTF-16-only checks:
    //
private:

    void toLocal8Bit_data();
    template <typename String> void toLocal8Bit_impl();

    void toLatin1_data();
    template <typename String> void toLatin1_impl();

    void toUtf8_data();
    template <typename String> void toUtf8_impl();

    void toUcs4_data();
    template <typename String> void toUcs4_impl();

private Q_SLOTS:

    void toLocal8Bit_QString_data() { toLocal8Bit_data(); }
    void toLocal8Bit_QString() { toLocal8Bit_impl<QString>(); }
    void toLocal8Bit_QStringRef_data() { toLocal8Bit_data(); }
    void toLocal8Bit_QStringRef() { toLocal8Bit_impl<QStringRef>(); }
    void toLocal8Bit_QStringView_data() { toLocal8Bit_data(); }
    void toLocal8Bit_QStringView() { toLocal8Bit_impl<QStringView>(); }

    void toLatin1_QString_data() { toLatin1_data(); }
    void toLatin1_QString() { toLatin1_impl<QString>(); }
    void toLatin1_QStringRef_data() { toLatin1_data(); }
    void toLatin1_QStringRef() { toLatin1_impl<QStringRef>(); }
    void toLatin1_QStringView_data() { toLatin1_data(); }
    void toLatin1_QStringView() { toLatin1_impl<QStringView>(); }

    void toUtf8_QString_data() { toUtf8_data(); }
    void toUtf8_QString() { toUtf8_impl<QString>(); }
    void toUtf8_QStringRef_data() { toUtf8_data(); }
    void toUtf8_QStringRef() { toUtf8_impl<QStringRef>(); }
    void toUtf8_QStringView_data() { toUtf8_data(); }
    void toUtf8_QStringView() { toUtf8_impl<QStringView>(); }

    void toUcs4_QString_data() { toUcs4_data(); }
    void toUcs4_QString() { toUcs4_impl<QString>(); }
    void toUcs4_QStringRef_data() { toUcs4_data(); }
    void toUcs4_QStringRef() { toUcs4_impl<QStringRef>(); }
    void toUcs4_QStringView_data() { toUcs4_data(); }
    void toUcs4_QStringView() { toUcs4_impl<QStringView>(); }
};

void tst_QStringApiSymmetry::compare_data(bool hasConceptOfNullAndEmpty)
{
    QTest::addColumn<QStringRef>("lhsUnicode");
    QTest::addColumn<QLatin1String>("lhsLatin1");
    QTest::addColumn<QStringRef>("rhsUnicode");
    QTest::addColumn<QLatin1String>("rhsLatin1");
    QTest::addColumn<int>("caseSensitiveCompareResult");
    QTest::addColumn<int>("caseInsensitiveCompareResult");

    if (hasConceptOfNullAndEmpty) {
        QTest::newRow("null <> null") << QStringRef() << QLatin1String()
                                      << QStringRef() << QLatin1String()
                                      << 0 << 0;
        static const QString empty("");
        QTest::newRow("null <> empty") << QStringRef() << QLatin1String()
                                       << QStringRef(&empty) << QLatin1String("")
                                       << 0 << 0;
    }

#define ROW(lhs, rhs) \
    do { \
        static const QString pinned[] = { \
            QString(QLatin1String(lhs)), \
            QString(QLatin1String(rhs)), \
        }; \
        QTest::newRow(qUtf8Printable(QLatin1String("'" lhs "' <> '" rhs "': "))) \
            << QStringRef(&pinned[0]) << QLatin1String(lhs) \
            << QStringRef(&pinned[1]) << QLatin1String(rhs) \
            << qstrcmp(lhs, rhs) << qstricmp(lhs, rhs); \
    } while (false)
    ROW("", "0");
    ROW("0", "");
    ROW("0", "1");
    ROW("0", "0");
    ROW("\xE4", "\xE4"); // ä <> ä
    ROW("\xE4", "\xC4"); // ä <> Ä
#undef ROW
}

template <class Str> Str  make(const QStringRef &sf, QLatin1String l1, const QByteArray &u8);
template <> QChar         make(const QStringRef &sf, QLatin1String,    const QByteArray &)   { return sf.isEmpty() ? QChar() : sf.at(0); }
template <> QStringRef    make(const QStringRef &sf, QLatin1String,    const QByteArray &)   { return sf; }
template <> QString       make(const QStringRef &sf, QLatin1String,    const QByteArray &)   { return sf.toString(); }
template <> QStringView   make(const QStringRef &sf, QLatin1String,    const QByteArray &)   { return sf; }
template <> QLatin1String make(const QStringRef &,   QLatin1String l1, const QByteArray &)   { return l1; }
template <> QByteArray    make(const QStringRef &,   QLatin1String,    const QByteArray &u8) { return u8; }
template <> const char *  make(const QStringRef &,   QLatin1String,    const QByteArray &u8) { return u8.data(); }

template <typename> struct is_utf8_encoded              : std::false_type {};
template <>         struct is_utf8_encoded<const char*> : std::true_type {};
template <>         struct is_utf8_encoded<QByteArray>  : std::true_type {};

template <typename> struct is_latin1_encoded                : std::false_type {};
template <>         struct is_latin1_encoded<QLatin1String> : std::true_type {};

template <typename LHS, typename RHS>
struct has_nothrow_compare {
    enum { value = is_utf8_encoded<LHS>::value == is_utf8_encoded<RHS>::value };
};

template <typename LHS, typename RHS>
void tst_QStringApiSymmetry::compare_impl() const
{
    QFETCH(QStringRef, lhsUnicode);
    QFETCH(QLatin1String, lhsLatin1);
    QFETCH(QStringRef, rhsUnicode);
    QFETCH(QLatin1String, rhsLatin1);
    QFETCH(int, caseSensitiveCompareResult);

    const auto lhsU8 = lhsUnicode.toUtf8();
    const auto rhsU8 = rhsUnicode.toUtf8();

    const auto lhs = make<LHS>(lhsUnicode, lhsLatin1, lhsU8);
    const auto rhs = make<RHS>(rhsUnicode, rhsLatin1, rhsU8);

#ifdef Q_COMPILER_NOEXCEPT
# define QVERIFY_NOEXCEPT(expr) do { \
    if (has_nothrow_compare<LHS, RHS>::value) {} else \
        QEXPECT_FAIL("", "Qt is missing a nothrow utf8-utf16 comparator", Continue); \
    QVERIFY(noexcept(expr)); } while (0)
#else
# define QVERIFY_NOEXCEPT(expr)
#endif

#define CHECK(op) \
    QVERIFY_NOEXCEPT(lhs op rhs); \
    do { if (caseSensitiveCompareResult op 0) { \
        QVERIFY(lhs op rhs); \
    } else { \
        QVERIFY(!(lhs op rhs)); \
    } } while (false)

    CHECK(==);
    CHECK(!=);
    CHECK(<);
    CHECK(>);
    CHECK(<=);
    CHECK(>=);
#undef CHECK
}

static QString empty = QLatin1String("");
// the tests below rely on the fact that these objects' names match their contents:
static QString a = QStringLiteral("a");
static QString b = QStringLiteral("b");
static QString c = QStringLiteral("c");
static QString ab = QStringLiteral("ab");
static QString bc = QStringLiteral("bc");
static QString abc = QStringLiteral("abc");

void tst_QStringApiSymmetry::mid_data()
{
    QTest::addColumn<QStringRef>("unicode");
    QTest::addColumn<QLatin1String>("latin1");
    QTest::addColumn<int>("pos");
    QTest::addColumn<int>("n");
    QTest::addColumn<QStringRef>("result");
    QTest::addColumn<QStringRef>("result2");

    QTest::addRow("null") << QStringRef() << QLatin1String() << 0 << 0 << QStringRef() << QStringRef();
    QTest::addRow("empty") << QStringRef(&empty) << QLatin1String("") << 0 << 0 << QStringRef(&empty) << QStringRef(&empty);

    // Some classes' mid() implementations have a wide contract, others a narrow one
    // so only test valid arguents here:
#define ROW(base, p, n, r1, r2) \
    QTest::addRow("%s%d%d", #base, p, n) << QStringRef(&base) << QLatin1String(#base) << p << n << QStringRef(&r1) << QStringRef(&r2)

    ROW(a, 0, 0, a, empty);
    ROW(a, 0, 1, a, a);
    ROW(a, 1, 0, empty, empty);

    ROW(ab, 0, 0, ab, empty);
    ROW(ab, 0, 1, ab, a);
    ROW(ab, 0, 2, ab, ab);
    ROW(ab, 1, 0, b,  empty);
    ROW(ab, 1, 1, b,  b);
    ROW(ab, 2, 0, empty, empty);

    ROW(abc, 0, 0, abc, empty);
    ROW(abc, 0, 1, abc, a);
    ROW(abc, 0, 2, abc, ab);
    ROW(abc, 0, 3, abc, abc);
    ROW(abc, 1, 0, bc,  empty);
    ROW(abc, 1, 1, bc,  b);
    ROW(abc, 1, 2, bc,  bc);
    ROW(abc, 2, 0, c,   empty);
    ROW(abc, 2, 1, c,   c);
    ROW(abc, 3, 0, empty, empty);
#undef ROW
}

template <typename String>
void tst_QStringApiSymmetry::mid_impl()
{
    QFETCH(const QStringRef, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, pos);
    QFETCH(const int, n);
    QFETCH(const QStringRef, result);
    QFETCH(const QStringRef, result2);

    const auto utf8 = unicode.toUtf8();

    const auto s = make<String>(unicode, latin1, utf8);

    const auto mid = s.mid(pos);
    const auto mid2 = s.mid(pos, n);

    QVERIFY(mid == result);
    QCOMPARE(mid.isNull(), result.isNull());
    QCOMPARE(mid.isEmpty(), result.isEmpty());

    QVERIFY(mid2 == result2);
    QCOMPARE(mid2.isNull(), result2.isNull());
    QCOMPARE(mid2.isEmpty(), result2.isEmpty());
}

void tst_QStringApiSymmetry::left_data()
{
    QTest::addColumn<QStringRef>("unicode");
    QTest::addColumn<QLatin1String>("latin1");
    QTest::addColumn<int>("n");
    QTest::addColumn<QStringRef>("result");

    QTest::addRow("null") << QStringRef() << QLatin1String() << 0 << QStringRef();
    QTest::addRow("empty") << QStringRef(&empty) << QLatin1String("") << 0 << QStringRef(&empty);

    // Some classes' left() implementations have a wide contract, others a narrow one
    // so only test valid arguents here:
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << QStringRef(&base) << QLatin1String(#base) << n << QStringRef(&res);

    ROW(a, 0, empty);
    ROW(a, 1, a);

    ROW(ab, 0, empty);
    ROW(ab, 1, a);
    ROW(ab, 2, ab);

    ROW(abc, 0, empty);
    ROW(abc, 1, a);
    ROW(abc, 2, ab);
    ROW(abc, 3, abc);
#undef ROW
}

template <typename String>
void tst_QStringApiSymmetry::left_impl()
{
    QFETCH(const QStringRef, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, n);
    QFETCH(const QStringRef, result);

    const auto utf8 = unicode.toUtf8();

    const auto s = make<String>(unicode, latin1, utf8);

    const auto left = s.left(n);

    QVERIFY(left == result);
    QCOMPARE(left.isNull(), result.isNull());
    QCOMPARE(left.isEmpty(), result.isEmpty());
}

void tst_QStringApiSymmetry::right_data()
{
    QTest::addColumn<QStringRef>("unicode");
    QTest::addColumn<QLatin1String>("latin1");
    QTest::addColumn<int>("n");
    QTest::addColumn<QStringRef>("result");

    QTest::addRow("null") << QStringRef() << QLatin1String() << 0 << QStringRef();
    QTest::addRow("empty") << QStringRef(&empty) << QLatin1String("") << 0 << QStringRef(&empty);

    // Some classes' right() implementations have a wide contract, others a narrow one
    // so only test valid arguents here:
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << QStringRef(&base) << QLatin1String(#base) << n << QStringRef(&res);

    ROW(a, 0, empty);
    ROW(a, 1, a);

    ROW(ab, 0, empty);
    ROW(ab, 1, b);
    ROW(ab, 2, ab);

    ROW(abc, 0, empty);
    ROW(abc, 1, c);
    ROW(abc, 2, bc);
    ROW(abc, 3, abc);
#undef ROW
}

template <typename String>
void tst_QStringApiSymmetry::right_impl()
{
    QFETCH(const QStringRef, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, n);
    QFETCH(const QStringRef, result);

    const auto utf8 = unicode.toUtf8();

    const auto s = make<String>(unicode, latin1, utf8);

    const auto right = s.right(n);

    QVERIFY(right == result);
    QCOMPARE(right.isNull(), result.isNull());
    QCOMPARE(right.isEmpty(), result.isEmpty());
}

void tst_QStringApiSymmetry::chop_data()
{
    QTest::addColumn<QStringRef>("unicode");
    QTest::addColumn<QLatin1String>("latin1");
    QTest::addColumn<int>("n");
    QTest::addColumn<QStringRef>("result");

    QTest::addRow("null") << QStringRef() << QLatin1String() << 0 << QStringRef();
    QTest::addRow("empty") << QStringRef(&empty) << QLatin1String("") << 0 << QStringRef(&empty);

    // Some classes' truncate() implementations have a wide contract, others a narrow one
    // so only test valid arguents here:
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << QStringRef(&base) << QLatin1String(#base) << n << QStringRef(&res);

    ROW(a, 0, a);
    ROW(a, 1, empty);

    ROW(ab, 0, ab);
    ROW(ab, 1, a);
    ROW(ab, 2, empty);

    ROW(abc, 0, abc);
    ROW(abc, 1, ab);
    ROW(abc, 2, a);
    ROW(abc, 3, empty);
#undef ROW
}

template <typename String>
void tst_QStringApiSymmetry::chop_impl()
{
    QFETCH(const QStringRef, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, n);
    QFETCH(const QStringRef, result);

    const auto utf8 = unicode.toUtf8();

    const auto s = make<String>(unicode, latin1, utf8);

    {
        auto chopped = s;
        chopped.chop(n);

        QVERIFY(chopped == result);
        QCOMPARE(chopped.isNull(), result.isNull());
        QCOMPARE(chopped.isEmpty(), result.isEmpty());
    }
}

template <typename String>
void tst_QStringApiSymmetry::truncate_impl()
{
    QFETCH(const QStringRef, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, n);
    QFETCH(const QStringRef, result);

    const auto utf8 = unicode.toUtf8();

    auto trunc = make<String>(unicode, latin1, utf8);

    trunc.truncate(n);

    QVERIFY(trunc == result);
    QCOMPARE(trunc.isNull(), result.isNull());
    QCOMPARE(trunc.isEmpty(), result.isEmpty());
}

//
//
// UTF-16-only checks:
//
//

template <class Str> Str  make(const QString &s);
template <> QStringRef    make(const QString &s)   { return QStringRef(&s); }
template <> QString       make(const QString &s)   { return s; }
template <> QStringView   make(const QString &s)   { return s; }

#define REPEAT_16X(X) X X X X  X X X X   X X X X  X X X X
#define LONG_STRING_256 REPEAT_16X("0123456789abcdef")

void tst_QStringApiSymmetry::toLocal8Bit_data()
{
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<QByteArray>("local");

    auto add = [](const char *local) {
        const QByteArray ba(local);
        QString s;
        for (char c : ba)
            s += QLatin1Char(c);
        QTest::addRow("\"%s\" (%d)", ba.left(16).constData(), ba.size()) << s << ba;
    };

    QTest::addRow("null") << QString() << QByteArray();
    QTest::addRow("empty") << QString("") << QByteArray("");

    add("Moebius");
    add(LONG_STRING_256);
}

template <typename String>
void tst_QStringApiSymmetry::toLocal8Bit_impl()
{
    QFETCH(const QString, unicode);
    QFETCH(const QByteArray, local);

    const auto str = make<String>(unicode);

    const auto result = str.toLocal8Bit();

    QCOMPARE(result, local);
    QCOMPARE(unicode.isEmpty(), result.isEmpty());
    QCOMPARE(unicode.isNull(), result.isNull());
}

void tst_QStringApiSymmetry::toLatin1_data()
{
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<QByteArray>("latin1");

    auto add = [](const char *l1) {
        const QByteArray ba(l1);
        QString s;
        for (char c : ba)
            s += QLatin1Char(c);
        QTest::addRow("\"%s\" (%d)", ba.left(16).constData(), ba.size()) << s << ba;
    };

    QTest::addRow("null") << QString() << QByteArray();
    QTest::addRow("empty") << QString("") << QByteArray("");

    add("M\xF6" "bius");
    add(LONG_STRING_256);
}

template <typename String>
void tst_QStringApiSymmetry::toLatin1_impl()
{
    QFETCH(const QString, unicode);
    QFETCH(const QByteArray, latin1);

    const auto str = make<String>(unicode);

    const auto result = str.toLatin1();

    QCOMPARE(result, latin1);
    QCOMPARE(unicode.isEmpty(), result.isEmpty());
    QCOMPARE(unicode.isNull(), result.isNull());
}

void tst_QStringApiSymmetry::toUtf8_data()
{
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<QByteArray>("utf8");

    auto add = [](const char *u8) {
        QByteArray ba(u8);
        QString s = ba;
        QTest::addRow("\"%s\" (%d)", ba.left(16).constData(), ba.size()) << s << ba;
    };

    QTest::addRow("null") << QString() << QByteArray();
    QTest::addRow("empty") << QString("") << QByteArray("");

    add("M\xC3\xB6" "bius");
    add(LONG_STRING_256);
}

template <typename String>
void tst_QStringApiSymmetry::toUtf8_impl()
{
    QFETCH(const QString, unicode);
    QFETCH(const QByteArray, utf8);

    const auto str = make<String>(unicode);

    const auto result = str.toUtf8();

    QCOMPARE(result, utf8);
    QCOMPARE(unicode.isEmpty(), result.isEmpty());
    QCOMPARE(unicode.isNull(), result.isNull());
}

void tst_QStringApiSymmetry::toUcs4_data()
{
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<QVector<uint>>("ucs4");

    auto add = [](const char *l1) {
        const QByteArray ba(l1);
        QString s;
        QVector<uint> ucs4;
        for (char c : ba) {
            s += QLatin1Char(c);
            ucs4.append(uint(uchar(c)));
        }
        QTest::addRow("\"%s\" (%d)", ba.left(16).constData(), ba.size()) << s << ucs4;
    };

    QTest::addRow("null") << QString() << QVector<uint>();
    QTest::addRow("empty") << QString("") << QVector<uint>();

    add("M\xF6" "bius");
    add(LONG_STRING_256);
}

template <typename String>
void tst_QStringApiSymmetry::toUcs4_impl()
{
    QFETCH(const QString, unicode);
    QFETCH(const QVector<uint>, ucs4);

    const auto str = make<String>(unicode);

    const auto result = str.toUcs4();

    QCOMPARE(result, ucs4);
    QCOMPARE(unicode.isEmpty(), ucs4.isEmpty());
}

QTEST_APPLESS_MAIN(tst_QStringApiSymmetry)

#include "tst_qstringapisymmetry.moc"
