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
#include <QChar>
#include <QStringRef>
#include <QLatin1String>

#include <QTest>

Q_DECLARE_METATYPE(QLatin1String)
Q_DECLARE_METATYPE(QStringRef)

template <typename T>
QString toQString(const T &t) { return QString(t); }
QString toQString(const QStringRef &ref) { return ref.toString(); }

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

    void compare_data(bool hasConceptOfNullAndEmpty=true);
    template <typename LHS, typename RHS>
    void compare_impl() const;

private Q_SLOTS:
    // test all combinations of {QChar, QStringRef, QString, QLatin1String, QByteArray, const char*}
    void compare_QChar_QChar_data() { compare_data(false); }
    void compare_QChar_QChar() { compare_impl<QChar, QChar>(); }
    void compare_QChar_QStringRef_data() { compare_data(false); }
    void compare_QChar_QStringRef() { compare_impl<QChar, QStringRef>(); }
    void compare_QChar_QString_data() { compare_data(false); }
    void compare_QChar_QString() { compare_impl<QChar, QString>(); }
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
    void compare_QString_QLatin1String_data() { compare_data(); }
    void compare_QString_QLatin1String() { compare_impl<QString, QLatin1String>(); }
    void compare_QString_QByteArray_data() { compare_data(); }
    void compare_QString_QByteArray() { compare_impl<QString, QByteArray>(); }
    void compare_QString_const_char_star_data() { compare_data(); }
    void compare_QString_const_char_star() { compare_impl<QString, const char *>(); }

    void compare_QLatin1String_QChar_data() { compare_data(false); }
    void compare_QLatin1String_QChar() { compare_impl<QLatin1String, QChar>(); }
    void compare_QLatin1String_QStringRef_data() { compare_data(); }
    void compare_QLatin1String_QStringRef() { compare_impl<QLatin1String, QStringRef>(); }
    void compare_QLatin1String_QString_data() { compare_data(); }
    void compare_QLatin1String_QString() { compare_impl<QLatin1String, QString>(); }
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

QTEST_APPLESS_MAIN(tst_QStringApiSymmetry)

#include "tst_qstringapisymmetry.moc"
