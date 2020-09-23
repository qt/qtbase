/****************************************************************************
**
** Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Copyright (C) 2019 Mail.ru Group.
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
#include <QScopedArrayPointer>
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

template <typename Iterable>
QStringList toQStringList(const Iterable &i) {
    QStringList result;
    for (auto &e : i)
        result.push_back(toQString(e));
    return result;
}

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
MAKE_ALL(QByteArray, char16_t)
MAKE_ALL(char16_t, QByteArray)

MAKE_ALL(const char*, QChar)

#undef MAKE_ALL
#undef MAKE_RELOP
// END FIXME

static Q_DECL_CONSTEXPR int sign(int i) noexcept
{
    return i < 0 ? -1 :
           i > 0 ? +1 :
           /*else*/ 0 ;
}

// Return a plain ASCII row name consisting of maximum 16 chars and the
// size for data
static QByteArray rowName(const QByteArray &data)
{
    const int size = data.size();
    QScopedArrayPointer<char> prettyC(QTest::toPrettyCString(data.constData(), qMin(16, size)));
    QByteArray result = prettyC.data();
    result += " (";
    result += QByteArray::number(size);
    result += ')';
    return result;
}

# define QVERIFY_NOEXCEPT(expr) do { \
    if (!has_nothrow_compare<LHS, RHS>::value) \
        QEXPECT_FAIL("", "Qt is missing a nothrow utf8-utf16 comparator", Continue); \
    QVERIFY(noexcept(expr)); } while (0)

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
    // test all combinations of {QChar, char16_t, QStringRef, QString, QStringView, QLatin1String, QByteArray, const char*}
    void compare_QChar_QChar_data() { compare_data(false); }
    void compare_QChar_QChar() { compare_impl<QChar, QChar>(); }
    void compare_QChar_char16_t_data() { compare_data(false); }
    void compare_QChar_char16_t() { compare_impl<QChar, char16_t>(); }
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

    void compare_char16_t_QChar_data() { compare_data(false); }
    void compare_char16_t_QChar() { compare_impl<char16_t, QChar>(); }
    //void compare_char16_t_char16_t_data() { compare_data(false); }
    //void compare_char16_t_char16_t() { compare_impl<char16_t, char16_t>(); }
    void compare_char16_t_QStringRef_data() { compare_data(false); }
    void compare_char16_t_QStringRef() { compare_impl<char16_t, QStringRef>(); }
    void compare_char16_t_QString_data() { compare_data(false); }
    void compare_char16_t_QString() { compare_impl<char16_t, QString>(); }
    void compare_char16_t_QStringView_data() { compare_data(false); }
    void compare_char16_t_QStringView() { compare_impl<char16_t, QStringView>(); }
    void compare_char16_t_QLatin1String_data() { compare_data(false); }
    void compare_char16_t_QLatin1String() { compare_impl<char16_t, QLatin1String>(); }
    void compare_char16_t_QByteArray_data() { compare_data(false); }
    void compare_char16_t_QByteArray() { compare_impl<char16_t, QByteArray>(); }
    //void compare_char16_t_const_char_star_data() { compare_data(false); }
    //void compare_char16_t_const_char_star() { compare_impl<char16_t, const char *>(); }

    void compare_QStringRef_QChar_data() { compare_data(false); }
    void compare_QStringRef_QChar() { compare_impl<QStringRef, QChar>(); }
    void compare_QStringRef_char16_t_data() { compare_data(false); }
    void compare_QStringRef_char16_t() { compare_impl<QStringRef, char16_t>(); }
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
    void compare_QString_char16_t_data() { compare_data(false); }
    void compare_QString_char16_t() { compare_impl<QString, char16_t>(); }
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
    void compare_QStringView_char16_t_data() { compare_data(false); }
    void compare_QStringView_char16_t() { compare_impl<QStringView, char16_t>(); }
    void compare_QStringView_QStringRef_data() { compare_data(); }
    void compare_QStringView_QStringRef() { compare_impl<QStringView, QStringRef>(); }
    void compare_QStringView_QString_data() { compare_data(); }
    void compare_QStringView_QString() { compare_impl<QStringView, QString>(); }
    void compare_QStringView_QStringView_data() { compare_data(); }
    void compare_QStringView_QStringView() { compare_impl<QStringView, QStringView>(); }
    void compare_QStringView_QLatin1String_data() { compare_data(); }
    void compare_QStringView_QLatin1String() { compare_impl<QStringView, QLatin1String>(); }
#ifdef NOT_YET_IMPLMENTED
    void compare_QStringView_QByteArray_data() { compare_data(); }
    void compare_QStringView_QByteArray() { compare_impl<QStringView, QByteArray>(); }
    void compare_QStringView_const_char_star_data() { compare_data(); }
    void compare_QStringView_const_char_star() { compare_impl<QStringView, const char *>(); }
#endif

    void compare_QLatin1String_QChar_data() { compare_data(false); }
    void compare_QLatin1String_QChar() { compare_impl<QLatin1String, QChar>(); }
    void compare_QLatin1String_char16_t_data() { compare_data(false); }
    void compare_QLatin1String_char16_t() { compare_impl<QLatin1String, char16_t>(); }
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
    void compare_QByteArray_char16_t_data() { compare_data(false); }
    void compare_QByteArray_char16_t() { compare_impl<QByteArray, char16_t>(); }
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
    //void compare_const_char_star_char16_t_data() { compare_data(false); }
    //void compare_const_char_star_char16_t() { compare_impl<const char *, char16_t>(); }
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
    void member_compare_data(bool hasConceptOfNullAndEmpty=true) { compare_data(hasConceptOfNullAndEmpty); }
    template <typename LHS, typename RHS>
    void member_compare_impl() const;

private Q_SLOTS:
    // test all combinations of {QChar, char16_t, QStringRef, QString, QStringView, QLatin1String, QByteArray, const char*}
#ifdef NOT_YET_IMPLEMENTED // probably never will be - what's the point of QChar::compare(QStringView)?
    void member_compare_QChar_QChar_data() { member_compare_data(false); }
    void member_compare_QChar_QChar() { member_compare_impl<QChar, QChar>(); }
    void member_compare_QChar_char16_t_data() { member_compare_data(false); }
    void member_compare_QChar_char16_t() { member_compare_impl<QChar, char16_t>(); }
    void member_compare_QChar_QStringRef_data() { member_compare_data(false); }
    void member_compare_QChar_QStringRef() { member_compare_impl<QChar, QStringRef>(); }
    void member_compare_QChar_QString_data() { member_compare_data(false); }
    void member_compare_QChar_QString() { member_compare_impl<QChar, QString>(); }
    void member_compare_QChar_QStringView_data() { member_compare_data(false); }
    void member_compare_QChar_QStringView() { member_compare_impl<QChar, QStringView>(); }
    void member_compare_QChar_QLatin1String_data() { member_compare_data(false); }
    void member_compare_QChar_QLatin1String() { member_compare_impl<QChar, QLatin1String>(); }
    void member_compare_QChar_QByteArray_data() { member_compare_data(false); }
    void member_compare_QChar_QByteArray() { member_compare_impl<QChar, QByteArray>(); }
    void member_compare_QChar_const_char_star_data() { member_compare_data(false); }
    void member_compare_QChar_const_char_star() { member_compare_impl<QChar, const char *>(); }
#endif

    // void member_compare_char16_t_XXX() - not possible

    void member_compare_QStringRef_QChar_data() { member_compare_data(false); }
    void member_compare_QStringRef_QChar() { member_compare_impl<QStringRef, QChar>(); }
    void member_compare_QStringRef_char16_t_data() { member_compare_data(false); }
    void member_compare_QStringRef_char16_t() { member_compare_impl<QStringRef, char16_t>(); }
    void member_compare_QStringRef_QStringRef_data() { member_compare_data(); }
    void member_compare_QStringRef_QStringRef() { member_compare_impl<QStringRef, QStringRef>(); }
    void member_compare_QStringRef_QString_data() { member_compare_data(); }
    void member_compare_QStringRef_QString() { member_compare_impl<QStringRef, QString>(); }
#ifdef NOT_YET_IMPLEMENTED
    void member_compare_QStringRef_QStringView_data() { member_compare_data(); }
    void member_compare_QStringRef_QStringView() { member_compare_impl<QStringRef, QStringView>(); }
#endif
    void member_compare_QStringRef_QLatin1String_data() { member_compare_data(); }
    void member_compare_QStringRef_QLatin1String() { member_compare_impl<QStringRef, QLatin1String>(); }
    void member_compare_QStringRef_QByteArray_data() { member_compare_data(); }
    void member_compare_QStringRef_QByteArray() { member_compare_impl<QStringRef, QByteArray>(); }
#ifdef NOT_YET_IMPLEMENTED
    void member_compare_QStringRef_const_char_star_data() { member_compare_data(); }
    void member_compare_QStringRef_const_char_star() { member_compare_impl<QStringRef, const char *>(); }
#endif

    void member_compare_QString_QChar_data() { member_compare_data(false); }
    void member_compare_QString_QChar() { member_compare_impl<QString, QChar>(); }
    void member_compare_QString_char16_t_data() { member_compare_data(false); }
    void member_compare_QString_char16_t() { member_compare_impl<QString, char16_t>(); }
    void member_compare_QString_QStringRef_data() { member_compare_data(); }
    void member_compare_QString_QStringRef() { member_compare_impl<QString, QStringRef>(); }
    void member_compare_QString_QString_data() { member_compare_data(); }
    void member_compare_QString_QString() { member_compare_impl<QString, QString>(); }
    void member_compare_QString_QStringView_data() { member_compare_data(); }
    void member_compare_QString_QStringView() { member_compare_impl<QString, QStringView>(); }
    void member_compare_QString_QLatin1String_data() { member_compare_data(); }
    void member_compare_QString_QLatin1String() { member_compare_impl<QString, QLatin1String>(); }
    void member_compare_QString_QByteArray_data() { member_compare_data(); }
    void member_compare_QString_QByteArray() { member_compare_impl<QString, QByteArray>(); }
    void member_compare_QString_const_char_star_data() { member_compare_data(); }
    void member_compare_QString_const_char_star() { member_compare_impl<QString, const char *>(); }

    void member_compare_QStringView_QChar_data() { member_compare_data(false); }
    void member_compare_QStringView_QChar() { member_compare_impl<QStringView, QChar>(); }
    void member_compare_QStringView_char16_t_data() { member_compare_data(false); }
    void member_compare_QStringView_char16_t() { member_compare_impl<QStringView, char16_t>(); }
    void member_compare_QStringView_QStringRef_data() { member_compare_data(); }
    void member_compare_QStringView_QStringRef() { member_compare_impl<QStringView, QStringRef>(); }
    void member_compare_QStringView_QString_data() { member_compare_data(); }
    void member_compare_QStringView_QString() { member_compare_impl<QStringView, QString>(); }
    void member_compare_QStringView_QStringView_data() { member_compare_data(); }
    void member_compare_QStringView_QStringView() { member_compare_impl<QStringView, QStringView>(); }
    void member_compare_QStringView_QLatin1String_data() { member_compare_data(); }
    void member_compare_QStringView_QLatin1String() { member_compare_impl<QStringView, QLatin1String>(); }
#ifdef NOT_YET_IMPLEMENTED
    void member_compare_QStringView_QByteArray_data() { member_compare_data(); }
    void member_compare_QStringView_QByteArray() { member_compare_impl<QStringView, QByteArray>(); }
    void member_compare_QStringView_const_char_star_data() { member_compare_data(); }
    void member_compare_QStringView_const_char_star() { member_compare_impl<QStringView, const char *>(); }
#endif

    void member_compare_QLatin1String_QChar_data() { member_compare_data(false); }
    void member_compare_QLatin1String_QChar() { member_compare_impl<QLatin1String, QChar>(); }
    void member_compare_QLatin1String_char16_t_data() { member_compare_data(false); }
    void member_compare_QLatin1String_char16_t() { member_compare_impl<QLatin1String, char16_t>(); }
    void member_compare_QLatin1String_QStringRef_data() { member_compare_data(); }
    void member_compare_QLatin1String_QStringRef() { member_compare_impl<QLatin1String, QStringRef>(); }
    void member_compare_QLatin1String_QString_data() { member_compare_data(); }
    void member_compare_QLatin1String_QString() { member_compare_impl<QLatin1String, QString>(); }
    void member_compare_QLatin1String_QStringView_data() { member_compare_data(); }
    void member_compare_QLatin1String_QStringView() { member_compare_impl<QLatin1String, QStringView>(); }
    void member_compare_QLatin1String_QLatin1String_data() { member_compare_data(); }
    void member_compare_QLatin1String_QLatin1String() { member_compare_impl<QLatin1String, QLatin1String>(); }
#ifdef NOT_YET_IMPLEMENTED
    void member_compare_QLatin1String_QByteArray_data() { member_compare_data(); }
    void member_compare_QLatin1String_QByteArray() { member_compare_impl<QLatin1String, QByteArray>(); }
    void member_compare_QLatin1String_const_char_star_data() { member_compare_data(); }
    void member_compare_QLatin1String_const_char_star() { member_compare_impl<QLatin1String, const char *>(); }

    void member_compare_QByteArray_QChar_data() { member_compare_data(false); }
    void member_compare_QByteArray_QChar() { member_compare_impl<QByteArray, QChar>(); }
    void member_compare_QByteArray_char16_t_data() { member_compare_data(false); }
    void member_compare_QByteArray_char16_t() { member_compare_impl<QByteArray, char16_t>(); }
    void member_compare_QByteArray_QStringRef_data() { member_compare_data(); }
    void member_compare_QByteArray_QStringRef() { member_compare_impl<QByteArray, QStringRef>(); }
    void member_compare_QByteArray_QString_data() { member_compare_data(); }
    void member_compare_QByteArray_QString() { member_compare_impl<QByteArray, QString>(); }
    void member_compare_QByteArray_QLatin1String_data() { member_compare_data(); }
    void member_compare_QByteArray_QLatin1String() { member_compare_impl<QByteArray, QLatin1String>(); }
#endif
    void member_compare_QByteArray_QByteArray_data() { member_compare_data(); }
    void member_compare_QByteArray_QByteArray() { member_compare_impl<QByteArray, QByteArray>(); }
    void member_compare_QByteArray_const_char_star_data() { member_compare_data(); }
    void member_compare_QByteArray_const_char_star() { member_compare_impl<QByteArray, const char *>(); }

private:
    void startsWith_data(bool rhsIsQChar = false);
    template <typename Haystack, typename Needle> void startsWith_impl() const;

    void endsWith_data(bool rhsIsQChar = false);
    template <typename Haystack, typename Needle> void endsWith_impl() const;

private Q_SLOTS:
    // test all combinations of {QString, QStringRef, QStringView, QLatin1String} x {QString, QStringRef, QStringView, QLatin1String, QChar, char16_t}:
    void startsWith_QString_QString_data() { startsWith_data(); }
    void startsWith_QString_QString() { startsWith_impl<QString, QString>(); }
    void startsWith_QString_QStringRef_data() { startsWith_data(); }
    void startsWith_QString_QStringRef() { startsWith_impl<QString, QStringRef>(); }
    void startsWith_QString_QStringView_data() { startsWith_data(); }
    void startsWith_QString_QStringView() { startsWith_impl<QString, QStringView>(); }
    void startsWith_QString_QLatin1String_data() { startsWith_data(); }
    void startsWith_QString_QLatin1String() { startsWith_impl<QString, QLatin1String>(); }
    void startsWith_QString_QChar_data() { startsWith_data(false); }
    void startsWith_QString_QChar() { startsWith_impl<QString, QChar>(); }
    void startsWith_QString_char16_t_data() { startsWith_data(false); }
    void startsWith_QString_char16_t() { startsWith_impl<QString, char16_t>(); }

    void startsWith_QStringRef_QString_data() { startsWith_data(); }
    void startsWith_QStringRef_QString() { startsWith_impl<QStringRef, QString>(); }
    void startsWith_QStringRef_QStringRef_data() { startsWith_data(); }
    void startsWith_QStringRef_QStringRef() { startsWith_impl<QStringRef, QStringRef>(); }
    void startsWith_QStringRef_QStringView_data() { startsWith_data(); }
    void startsWith_QStringRef_QStringView() { startsWith_impl<QStringRef, QStringView>(); }
    void startsWith_QStringRef_QLatin1String_data() { startsWith_data(); }
    void startsWith_QStringRef_QLatin1String() { startsWith_impl<QStringRef, QLatin1String>(); }
    void startsWith_QStringRef_QChar_data() { startsWith_data(false); }
    void startsWith_QStringRef_QChar() { startsWith_impl<QStringRef, QChar>(); }
    void startsWith_QStringRef_char16_t_data() { startsWith_data(false); }
    void startsWith_QStringRef_char16_t() { startsWith_impl<QStringRef, char16_t>(); }

    void startsWith_QStringView_QString_data() { startsWith_data(); }
    void startsWith_QStringView_QString() { startsWith_impl<QStringView, QString>(); }
    void startsWith_QStringView_QStringRef_data() { startsWith_data(); }
    void startsWith_QStringView_QStringRef() { startsWith_impl<QStringView, QStringRef>(); }
    void startsWith_QStringView_QStringView_data() { startsWith_data(); }
    void startsWith_QStringView_QStringView() { startsWith_impl<QStringView, QStringView>(); }
    void startsWith_QStringView_QLatin1String_data() { startsWith_data(); }
    void startsWith_QStringView_QLatin1String() { startsWith_impl<QStringView, QLatin1String>(); }
    void startsWith_QStringView_QChar_data() { startsWith_data(false); }
    void startsWith_QStringView_QChar() { startsWith_impl<QStringView, QChar>(); }
    void startsWith_QStringView_char16_t_data() { startsWith_data(false); }
    void startsWith_QStringView_char16_t() { startsWith_impl<QStringView, char16_t>(); }

    void startsWith_QLatin1String_QString_data() { startsWith_data(); }
    void startsWith_QLatin1String_QString() { startsWith_impl<QLatin1String, QString>(); }
    void startsWith_QLatin1String_QStringRef_data() { startsWith_data(); }
    void startsWith_QLatin1String_QStringRef() { startsWith_impl<QLatin1String, QStringRef>(); }
    void startsWith_QLatin1String_QStringView_data() { startsWith_data(); }
    void startsWith_QLatin1String_QStringView() { startsWith_impl<QLatin1String, QStringView>(); }
    void startsWith_QLatin1String_QLatin1String_data() { startsWith_data(); }
    void startsWith_QLatin1String_QLatin1String() { startsWith_impl<QLatin1String, QLatin1String>(); }
    void startsWith_QLatin1String_QChar_data() { startsWith_data(false); }
    void startsWith_QLatin1String_QChar() { startsWith_impl<QLatin1String, QChar>(); }
    void startsWith_QLatin1String_char16_t_data() { startsWith_data(false); }
    void startsWith_QLatin1String_char16_t() { startsWith_impl<QLatin1String, char16_t>(); }

    void endsWith_QString_QString_data() { endsWith_data(); }
    void endsWith_QString_QString() { endsWith_impl<QString, QString>(); }
    void endsWith_QString_QStringRef_data() { endsWith_data(); }
    void endsWith_QString_QStringRef() { endsWith_impl<QString, QStringRef>(); }
    void endsWith_QString_QStringView_data() { endsWith_data(); }
    void endsWith_QString_QStringView() { endsWith_impl<QString, QStringView>(); }
    void endsWith_QString_QLatin1String_data() { endsWith_data(); }
    void endsWith_QString_QLatin1String() { endsWith_impl<QString, QLatin1String>(); }
    void endsWith_QString_QChar_data() { endsWith_data(false); }
    void endsWith_QString_QChar() { endsWith_impl<QString, QChar>(); }
    void endsWith_QString_char16_t_data() { endsWith_data(false); }
    void endsWith_QString_char16_t() { endsWith_impl<QString, char16_t>(); }

    void endsWith_QStringRef_QString_data() { endsWith_data(); }
    void endsWith_QStringRef_QString() { endsWith_impl<QStringRef, QString>(); }
    void endsWith_QStringRef_QStringRef_data() { endsWith_data(); }
    void endsWith_QStringRef_QStringRef() { endsWith_impl<QStringRef, QStringRef>(); }
    void endsWith_QStringRef_QStringView_data() { endsWith_data(); }
    void endsWith_QStringRef_QStringView() { endsWith_impl<QStringRef, QStringView>(); }
    void endsWith_QStringRef_QLatin1String_data() { endsWith_data(); }
    void endsWith_QStringRef_QLatin1String() { endsWith_impl<QStringRef, QLatin1String>(); }
    void endsWith_QStringRef_QChar_data() { endsWith_data(false); }
    void endsWith_QStringRef_QChar() { endsWith_impl<QStringRef, QChar>(); }
    void endsWith_QStringRef_char16_t_data() { endsWith_data(false); }
    void endsWith_QStringRef_char16_t() { endsWith_impl<QStringRef, char16_t>(); }

    void endsWith_QStringView_QString_data() { endsWith_data(); }
    void endsWith_QStringView_QString() { endsWith_impl<QStringView, QString>(); }
    void endsWith_QStringView_QStringRef_data() { endsWith_data(); }
    void endsWith_QStringView_QStringRef() { endsWith_impl<QStringView, QStringRef>(); }
    void endsWith_QStringView_QStringView_data() { endsWith_data(); }
    void endsWith_QStringView_QStringView() { endsWith_impl<QStringView, QStringView>(); }
    void endsWith_QStringView_QLatin1String_data() { endsWith_data(); }
    void endsWith_QStringView_QLatin1String() { endsWith_impl<QStringView, QLatin1String>(); }
    void endsWith_QStringView_QChar_data() { endsWith_data(false); }
    void endsWith_QStringView_QChar() { endsWith_impl<QStringView, QChar>(); }
    void endsWith_QStringView_char16_t_data() { endsWith_data(false); }
    void endsWith_QStringView_char16_t() { endsWith_impl<QStringView, char16_t>(); }

    void endsWith_QLatin1String_QString_data() { endsWith_data(); }
    void endsWith_QLatin1String_QString() { endsWith_impl<QLatin1String, QString>(); }
    void endsWith_QLatin1String_QStringRef_data() { endsWith_data(); }
    void endsWith_QLatin1String_QStringRef() { endsWith_impl<QLatin1String, QStringRef>(); }
    void endsWith_QLatin1String_QStringView_data() { endsWith_data(); }
    void endsWith_QLatin1String_QStringView() { endsWith_impl<QLatin1String, QStringView>(); }
    void endsWith_QLatin1String_QLatin1String_data() { endsWith_data(); }
    void endsWith_QLatin1String_QLatin1String() { endsWith_impl<QLatin1String, QLatin1String>(); }
    void endsWith_QLatin1String_QChar_data() { endsWith_data(false); }
    void endsWith_QLatin1String_QChar() { endsWith_impl<QLatin1String, QChar>(); }
    void endsWith_QLatin1String_char16_t_data() { endsWith_data(false); }
    void endsWith_QLatin1String_char16_t() { endsWith_impl<QLatin1String, char16_t>(); }

private:
    void split_data(bool rhsHasVariableLength = true);
    template <typename Haystack, typename Needle> void split_impl() const;

private Q_SLOTS:
    // test all combinations of {QString, QStringRef} x {QString, QLatin1String, QChar}:
    void split_QString_QString_data() { split_data(); }
    void split_QString_QString() { split_impl<QString, QString>(); }
    void split_QString_QLatin1String_data() { split_data(); }
    void split_QString_QLatin1String() { split_impl<QString, QLatin1String>(); }
    void split_QString_QChar_data() { split_data(false); }
    void split_QString_QChar() { split_impl<QString, QChar>(); }

    void split_QStringRef_QString_data() { split_data(); }
    void split_QStringRef_QString() { split_impl<QStringRef, QString>(); }
    void split_QStringRef_QLatin1String_data() { split_data(); }
    void split_QStringRef_QLatin1String() { split_impl<QStringRef, QLatin1String>(); }
    void split_QStringRef_QChar_data() { split_data(false); }
    void split_QStringRef_QChar() { split_impl<QStringRef, QChar>(); }

    void split_QStringView_QString_data() { split_data(); }
    void split_QStringView_QString() { split_impl<QStringView, QString>(); }
    void split_QStringView_QChar_data() { split_data(false); }
    void split_QStringView_QChar() { split_impl<QStringView, QChar>(); }

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

    void left_truncate_QString_data() { left_data(); }
    void left_truncate_QString() { left_impl<QString>(); }
    void left_truncate_QStringRef_data() { left_data(); }
    void left_truncate_QStringRef() { left_impl<QStringRef>(); }
    void left_truncate_QStringView_data() { left_data(); }
    void left_truncate_QStringView() { left_impl<QStringView>(); }
    void left_truncate_QLatin1String_data() { left_data(); }
    void left_truncate_QLatin1String() { left_impl<QLatin1String>(); }
    void left_truncate_QByteArray_data() { left_data(); }
    void left_truncate_QByteArray() { left_impl<QByteArray>(); }

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
    void chop_QStringView_data() { chop_data(); }
    void chop_QStringView() { chop_impl<QStringView>(); }
    void chop_QLatin1String_data() { chop_data(); }
    void chop_QLatin1String() { chop_impl<QLatin1String>(); }
    void chop_QByteArray_data() { chop_data(); }
    void chop_QByteArray() { chop_impl<QByteArray>(); }

private:
    void trimmed_data();
    template <typename String> void trimmed_impl();

private Q_SLOTS:
    void trim_trimmed_QString_data() { trimmed_data(); }
    void trim_trimmed_QString() { trimmed_impl<QString>(); }
    void trim_trimmed_QStringRef_data() { trimmed_data(); }
    void trim_trimmed_QStringRef() { trimmed_impl<QStringRef>(); }
    void trim_trimmed_QStringView_data() { trimmed_data(); }
    void trim_trimmed_QStringView() { trimmed_impl<QStringView>(); }
    void trim_trimmed_QLatin1String_data() { trimmed_data(); }
    void trim_trimmed_QLatin1String() { trimmed_impl<QLatin1String>(); }
    void trim_trimmed_QByteArray_data() { trimmed_data(); }
    void trim_trimmed_QByteArray() { trimmed_impl<QByteArray>(); }

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

private:
    template <typename Haystack, typename Needle> void indexOf_impl() const;
    void indexOf_data(bool rhsHasVariableLength = true);

private Q_SLOTS:
    // test all combinations of {QString, QLatin1String, QStringRef, QStringView} x {QString, QLatin1String, QStringRef, QStringView, QChar, char16_t}:
    void indexOf_QString_QString_data() { indexOf_data(); }
    void indexOf_QString_QString() { indexOf_impl<QString, QString>(); }
    void indexOf_QString_QLatin1String_data() { indexOf_data(); }
    void indexOf_QString_QLatin1String() { indexOf_impl<QString, QLatin1String>(); }
    void indexOf_QString_QStringRef_data() { indexOf_data(); }
    void indexOf_QString_QStringRef() { indexOf_impl<QString, QStringRef>(); }
    void indexOf_QString_QStringView_data() { indexOf_data(); }
    void indexOf_QString_QStringView() { indexOf_impl<QString, QStringView>(); }
    void indexOf_QString_QChar_data() { indexOf_data(false); }
    void indexOf_QString_QChar() { indexOf_impl<QString, QChar>(); }
    void indexOf_QString_char16_t_data() { indexOf_data(false); }
    void indexOf_QString_char16_t() { indexOf_impl<QString, char16_t>(); }

    void indexOf_QLatin1String_QString_data() { indexOf_data(); }
    void indexOf_QLatin1String_QString() { indexOf_impl<QLatin1String, QString>(); }
    void indexOf_QLatin1String_QLatin1String_data() { indexOf_data(); }
    void indexOf_QLatin1String_QLatin1String() { indexOf_impl<QLatin1String, QLatin1String>(); }
    void indexOf_QLatin1String_QStringRef_data() { indexOf_data(); }
    void indexOf_QLatin1String_QStringRef() { indexOf_impl<QLatin1String, QStringRef>(); }
    void indexOf_QLatin1String_QStringView_data() { indexOf_data(); }
    void indexOf_QLatin1String_QStringView() { indexOf_impl<QLatin1String, QStringView>(); }
    void indexOf_QLatin1String_QChar_data() { indexOf_data(false); }
    void indexOf_QLatin1String_QChar() { indexOf_impl<QLatin1String, QChar>(); }
    void indexOf_QLatin1String_char16_t_data() { indexOf_data(false); }
    void indexOf_QLatin1String_char16_t() { indexOf_impl<QLatin1String, char16_t>(); }

    void indexOf_QStringRef_QString_data() { indexOf_data(); }
    void indexOf_QStringRef_QString() { indexOf_impl<QStringRef, QString>(); }
    void indexOf_QStringRef_QLatin1String_data() { indexOf_data(); }
    void indexOf_QStringRef_QLatin1String() { indexOf_impl<QStringRef, QLatin1String>(); }
    void indexOf_QStringRef_QStringRef_data() { indexOf_data(); }
    void indexOf_QStringRef_QStringRef() { indexOf_impl<QStringRef, QStringRef>(); }
    void indexOf_QStringRef_QStringView_data() { indexOf_data(); }
    void indexOf_QStringRef_QStringView() { indexOf_impl<QStringRef, QStringView>(); }
    void indexOf_QStringRef_QChar_data() { indexOf_data(false); }
    void indexOf_QStringRef_QChar() { indexOf_impl<QStringRef, QChar>(); }
    void indexOf_QStringRef_char16_t_data() { indexOf_data(false); }
    void indexOf_QStringRef_char16_t() { indexOf_impl<QStringRef, char16_t>(); }

    void indexOf_QStringView_QString_data() { indexOf_data(); }
    void indexOf_QStringView_QString() { indexOf_impl<QStringView, QString>(); }
    void indexOf_QStringView_QLatin1String_data() { indexOf_data(); }
    void indexOf_QStringView_QLatin1String() { indexOf_impl<QStringView, QLatin1String>(); }
    void indexOf_QStringView_QStringRef_data() { indexOf_data(); }
    void indexOf_QStringView_QStringRef() { indexOf_impl<QStringView, QStringRef>(); }
    void indexOf_QStringView_QStringView_data() { indexOf_data(); }
    void indexOf_QStringView_QStringView() { indexOf_impl<QStringView, QStringView>(); }
    void indexOf_QStringView_QChar_data() { indexOf_data(false); }
    void indexOf_QStringView_QChar() { indexOf_impl<QStringView, QChar>(); }
    void indexOf_QStringView_char16_t_data() { indexOf_data(false); }
    void indexOf_QStringView_char16_t() { indexOf_impl<QStringView, char16_t>(); }

private:
    template <typename Haystack, typename Needle> void contains_impl() const;
    void contains_data(bool rhsHasVariableLength = true);

private Q_SLOTS:
    // test all combinations of {QString, QLatin1String, QStringRef, QStringView} x {QString, QLatin1String, QStringRef, QStringView, QChar, char16_t}:
    void contains_QString_QString_data() { contains_data(); }
    void contains_QString_QString() { contains_impl<QString, QString>(); }
    void contains_QString_QLatin1String_data() { contains_data(); }
    void contains_QString_QLatin1String() { contains_impl<QString, QLatin1String>(); }
    void contains_QString_QStringRef_data() { contains_data(); }
    void contains_QString_QStringRef() { contains_impl<QString, QStringRef>(); }
    void contains_QString_QStringView_data() { contains_data(); }
    void contains_QString_QStringView() { contains_impl<QString, QStringView>(); }
    void contains_QString_QChar_data() { contains_data(false); }
    void contains_QString_QChar() { contains_impl<QString, QChar>(); }
    void contains_QString_char16_t_data() { contains_data(false); }
    void contains_QString_char16_t() { contains_impl<QString, char16_t>(); }

    void contains_QLatin1String_QString_data() { contains_data(); }
    void contains_QLatin1String_QString() { contains_impl<QLatin1String, QString>(); }
    void contains_QLatin1String_QLatin1String_data() { contains_data(); }
    void contains_QLatin1String_QLatin1String() { contains_impl<QLatin1String, QLatin1String>(); }
    void contains_QLatin1String_QStringRef_data() { contains_data(); }
    void contains_QLatin1String_QStringRef() { contains_impl<QLatin1String, QStringRef>(); }
    void contains_QLatin1String_QStringView_data() { contains_data(); }
    void contains_QLatin1String_QStringView() { contains_impl<QLatin1String, QStringView>(); }
    void contains_QLatin1String_QChar_data() { contains_data(false); }
    void contains_QLatin1String_QChar() { contains_impl<QLatin1String, QChar>(); }
    void contains_QLatin1String_char16_t_data() { contains_data(false); }
    void contains_QLatin1String_char16_t() { contains_impl<QLatin1String, char16_t>(); }

    void contains_QStringRef_QString_data() { contains_data(); }
    void contains_QStringRef_QString() { contains_impl<QStringRef, QString>(); }
    void contains_QStringRef_QLatin1String_data() { contains_data(); }
    void contains_QStringRef_QLatin1String() { contains_impl<QStringRef, QLatin1String>(); }
    void contains_QStringRef_QStringRef_data() { contains_data(); }
    void contains_QStringRef_QStringRef() { contains_impl<QStringRef, QStringRef>(); }
    void contains_QStringRef_QStringView_data() { contains_data(); }
    void contains_QStringRef_QStringView() { contains_impl<QStringRef, QStringView>(); }
    void contains_QStringRef_QChar_data() { contains_data(false); }
    void contains_QStringRef_QChar() { contains_impl<QStringRef, QChar>(); }
    void contains_QStringRef_char16_t_data() { contains_data(false); }
    void contains_QStringRef_char16_t() { contains_impl<QStringRef, char16_t>(); }

    void contains_QStringView_QString_data() { contains_data(); }
    void contains_QStringView_QString() { contains_impl<QStringView, QString>(); }
    void contains_QStringView_QLatin1String_data() { contains_data(); }
    void contains_QStringView_QLatin1String() { contains_impl<QStringView, QLatin1String>(); }
    void contains_QStringView_QStringRef_data() { contains_data(); }
    void contains_QStringView_QStringRef() { contains_impl<QStringView, QStringRef>(); }
    void contains_QStringView_QStringView_data() { contains_data(); }
    void contains_QStringView_QStringView() { contains_impl<QStringView, QStringView>(); }
    void contains_QStringView_QChar_data() { contains_data(false); }
    void contains_QStringView_QChar() { contains_impl<QStringView, QChar>(); }
    void contains_QStringView_char16_t_data() { contains_data(false); }
    void contains_QStringView_char16_t() { contains_impl<QStringView, char16_t>(); }

private:
    template <typename Haystack, typename Needle> void lastIndexOf_impl() const;
    void lastIndexOf_data(bool rhsHasVariableLength = true);

private Q_SLOTS:
    // test all combinations of {QString, QLatin1String, QStringRef, QStringView} x {QString, QLatin1String, QStringRef, QStringView, QChar, char16_t}:
    void lastIndexOf_QString_QString_data() { lastIndexOf_data(); }
    void lastIndexOf_QString_QString() { lastIndexOf_impl<QString, QString>(); }
    void lastIndexOf_QString_QLatin1String_data() { lastIndexOf_data(); }
    void lastIndexOf_QString_QLatin1String() { lastIndexOf_impl<QString, QLatin1String>(); }
    void lastIndexOf_QString_QStringRef_data() { lastIndexOf_data(); }
    void lastIndexOf_QString_QStringRef() { lastIndexOf_impl<QString, QStringRef>(); }
    void lastIndexOf_QString_QStringView_data() { lastIndexOf_data(); }
    void lastIndexOf_QString_QStringView() { lastIndexOf_impl<QString, QStringView>(); }
    void lastIndexOf_QString_QChar_data() { lastIndexOf_data(false); }
    void lastIndexOf_QString_QChar() { lastIndexOf_impl<QString, QChar>(); }
    void lastIndexOf_QString_char16_t_data() { lastIndexOf_data(false); }
    void lastIndexOf_QString_char16_t() { lastIndexOf_impl<QString, char16_t>(); }

    void lastIndexOf_QLatin1String_QString_data() { lastIndexOf_data(); }
    void lastIndexOf_QLatin1String_QString() { lastIndexOf_impl<QLatin1String, QString>(); }
    void lastIndexOf_QLatin1String_QLatin1String_data() { lastIndexOf_data(); }
    void lastIndexOf_QLatin1String_QLatin1String() { lastIndexOf_impl<QLatin1String, QLatin1String>(); }
    void lastIndexOf_QLatin1String_QStringRef_data() { lastIndexOf_data(); }
    void lastIndexOf_QLatin1String_QStringRef() { lastIndexOf_impl<QLatin1String, QStringRef>(); }
    void lastIndexOf_QLatin1String_QStringView_data() { lastIndexOf_data(); }
    void lastIndexOf_QLatin1String_QStringView() { lastIndexOf_impl<QLatin1String, QStringView>(); }
    void lastIndexOf_QLatin1String_QChar_data() { lastIndexOf_data(false); }
    void lastIndexOf_QLatin1String_QChar() { lastIndexOf_impl<QLatin1String, QChar>(); }
    void lastIndexOf_QLatin1String_char16_t_data() { lastIndexOf_data(false); }
    void lastIndexOf_QLatin1String_char16_t() { lastIndexOf_impl<QLatin1String, char16_t>(); }

    void lastIndexOf_QStringRef_QString_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringRef_QString() { lastIndexOf_impl<QStringRef, QString>(); }
    void lastIndexOf_QStringRef_QLatin1String_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringRef_QLatin1String() { lastIndexOf_impl<QStringRef, QLatin1String>(); }
    void lastIndexOf_QStringRef_QStringRef_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringRef_QStringRef() { lastIndexOf_impl<QStringRef, QStringRef>(); }
    void lastIndexOf_QStringRef_QStringView_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringRef_QStringView() { lastIndexOf_impl<QStringRef, QStringView>(); }
    void lastIndexOf_QStringRef_QChar_data() { lastIndexOf_data(false); }
    void lastIndexOf_QStringRef_QChar() { lastIndexOf_impl<QStringRef, QChar>(); }
    void lastIndexOf_QStringRef_char16_t_data() { lastIndexOf_data(false); }
    void lastIndexOf_QStringRef_char16_t() { lastIndexOf_impl<QStringRef, char16_t>(); }

    void lastIndexOf_QStringView_QString_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringView_QString() { lastIndexOf_impl<QStringView, QString>(); }
    void lastIndexOf_QStringView_QLatin1String_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringView_QLatin1String() { lastIndexOf_impl<QStringView, QLatin1String>(); }
    void lastIndexOf_QStringView_QStringRef_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringView_QStringRef() { lastIndexOf_impl<QStringView, QStringRef>(); }
    void lastIndexOf_QStringView_QStringView_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringView_QStringView() { lastIndexOf_impl<QStringView, QStringView>(); }
    void lastIndexOf_QStringView_QChar_data() { lastIndexOf_data(false); }
    void lastIndexOf_QStringView_QChar() { lastIndexOf_impl<QStringView, QChar>(); }
    void lastIndexOf_QStringView_char16_t_data() { lastIndexOf_data(false); }
    void lastIndexOf_QStringView_char16_t() { lastIndexOf_impl<QStringView, char16_t>(); }
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
        QTest::newRow("empty <> null") << QStringRef(&empty) << QLatin1String("")
                                       << QStringRef() << QLatin1String()
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
            << sign(qstrcmp(lhs, rhs)) << sign(qstricmp(lhs, rhs)); \
    } while (false)
    ROW("", "0");
    ROW("0", "");
    ROW("0", "1");
    ROW("0", "0");
    ROW("10", "0");
    ROW("01", "1");
    ROW("\xE4", "\xE4"); // ä <> ä
    ROW("\xE4", "\xC4"); // ä <> Ä
#undef ROW
}

template <typename String> String detached(String s)
{
    if (!s.isNull()) { // detaching loses nullness, but we need to preserve it
        auto d = s.data();
        Q_UNUSED(d);
    }
    return s;
}

template <class Str> Str  make(const QStringRef &sf, QLatin1String l1, const QByteArray &u8);
template <> QChar         make(const QStringRef &sf, QLatin1String,    const QByteArray &)   { return sf.isEmpty() ? QChar() : sf.at(0); }
template <> char16_t      make(const QStringRef &sf, QLatin1String,    const QByteArray &)   { return sf.isEmpty() ? char16_t() : char16_t{sf.at(0).unicode()}; }
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
struct has_qCompareStrings {
    enum { value = !std::is_same<LHS, QChar>::value && !std::is_same<RHS, QChar>::value &&
                   !is_utf8_encoded<LHS>::value && !is_utf8_encoded<RHS>::value };
};

template <typename LHS, typename RHS>
void tst_QStringApiSymmetry::compare_impl() const
{
    QFETCH(QStringRef, lhsUnicode);
    QFETCH(QLatin1String, lhsLatin1);
    QFETCH(QStringRef, rhsUnicode);
    QFETCH(QLatin1String, rhsLatin1);
    QFETCH(int, caseSensitiveCompareResult);
    QFETCH(const int, caseInsensitiveCompareResult);
    Q_UNUSED(caseInsensitiveCompareResult);

    const auto lhsU8 = lhsUnicode.toUtf8();
    const auto rhsU8 = rhsUnicode.toUtf8();

    const auto lhs = make<LHS>(lhsUnicode, lhsLatin1, lhsU8);
    const auto rhs = make<RHS>(rhsUnicode, rhsLatin1, rhsU8);

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

template <typename LHS, typename RHS>
void tst_QStringApiSymmetry::member_compare_impl() const
{
    QFETCH(QStringRef, lhsUnicode);
    QFETCH(QLatin1String, lhsLatin1);
    QFETCH(QStringRef, rhsUnicode);
    QFETCH(QLatin1String, rhsLatin1);
    QFETCH(const int, caseSensitiveCompareResult);
    QFETCH(const int, caseInsensitiveCompareResult);

    const auto lhsU8 = lhsUnicode.toUtf8();
    const auto rhsU8 = rhsUnicode.toUtf8();

    const auto lhs = make<LHS>(lhsUnicode, lhsLatin1, lhsU8);
    const auto rhs = make<RHS>(rhsUnicode, rhsLatin1, rhsU8);

    QVERIFY_NOEXCEPT(lhs.compare(rhs, Qt::CaseSensitive));

    QCOMPARE(sign(lhs.compare(rhs)),                      caseSensitiveCompareResult);
    QCOMPARE(sign(lhs.compare(rhs, Qt::CaseSensitive)),   caseSensitiveCompareResult);
    if (is_utf8_encoded<LHS>::value && is_utf8_encoded<RHS>::value &&
            caseSensitiveCompareResult != caseInsensitiveCompareResult &&
            (!QtPrivate::isAscii(lhsUnicode) || !QtPrivate::isAscii(rhsUnicode)))
    {
        QEXPECT_FAIL("", "Qt is missing a case-insensitive UTF-8/UTF-8 comparator", Continue);
    }
    QCOMPARE(sign(lhs.compare(rhs, Qt::CaseInsensitive)), caseInsensitiveCompareResult);
}

static QString empty = QLatin1String("");
static QString null;
// the tests below rely on the fact that these objects' names match their contents:
static QString a = QStringLiteral("a");
static QString A = QStringLiteral("A");
static QString b = QStringLiteral("b");
static QString B = QStringLiteral("B");
static QString c = QStringLiteral("c");
static QString C = QStringLiteral("C");
static QString d = QStringLiteral("d");
static QString D = QStringLiteral("D");
static QString e = QStringLiteral("e");
static QString E = QStringLiteral("E");
static QString f = QStringLiteral("f");
static QString F = QStringLiteral("F");
static QString g = QStringLiteral("g");
static QString G = QStringLiteral("G");
static QString ab = QStringLiteral("ab");
static QString aB = QStringLiteral("aB");
static QString Ab = QStringLiteral("Ab");
static QString AB = QStringLiteral("AB");
static QString bc = QStringLiteral("bc");
static QString bC = QStringLiteral("bC");
static QString Bc = QStringLiteral("Bc");
static QString BC = QStringLiteral("BC");
static QString abc = QStringLiteral("abc");
static QString abC = QStringLiteral("abC");
static QString aBc = QStringLiteral("aBc");
static QString aBC = QStringLiteral("aBC");
static QString Abc = QStringLiteral("Abc");
static QString AbC = QStringLiteral("AbC");
static QString ABc = QStringLiteral("ABc");
static QString ABC = QStringLiteral("ABC");

void tst_QStringApiSymmetry::startsWith_data(bool rhsHasVariableLength)
{
    QTest::addColumn<QStringRef>("haystackU16");
    QTest::addColumn<QLatin1String>("haystackL1");
    QTest::addColumn<QStringRef>("needleU16");
    QTest::addColumn<QLatin1String>("needleL1");
    QTest::addColumn<bool>("resultCS");
    QTest::addColumn<bool>("resultCIS");

    if (rhsHasVariableLength) {
        QTest::addRow("null ~= ^null")   << QStringRef() << QLatin1String()
                                         << QStringRef() << QLatin1String() << true << true;
        QTest::addRow("empty ~= ^null")  << QStringRef(&empty) << QLatin1String("")
                                         << QStringRef() << QLatin1String() << true << true;
        QTest::addRow("a ~= ^null")      << QStringRef(&a) << QLatin1String("a")
                                         << QStringRef() << QLatin1String() << true << true;
        QTest::addRow("null ~= ^empty")  << QStringRef() << QLatin1String()
                                         << QStringRef(&empty) << QLatin1String("") << false << false;
        QTest::addRow("a ~= ^empty")     << QStringRef(&a) << QLatin1String("a")
                                         << QStringRef(&empty) << QLatin1String("") << true << true;
        QTest::addRow("empty ~= ^empty") << QStringRef(&empty) << QLatin1String("")
                                         << QStringRef(&empty) << QLatin1String("") << true << true;
    }
    QTest::addRow("null ~= ^a")      << QStringRef() << QLatin1String()
                                     << QStringRef(&a) << QLatin1String("a") << false << false;
    QTest::addRow("empty ~= ^a")     << QStringRef(&empty) << QLatin1String("")
                                     << QStringRef(&a) << QLatin1String("a") << false << false;

#define ROW(h, n, cs, cis) \
    QTest::addRow("%s ~= ^%s", #h, #n) << QStringRef(&h) << QLatin1String(#h) \
                                       << QStringRef(&n) << QLatin1String(#n) \
                                       << bool(cs) << bool(cis)
    ROW(a,  a, 1, 1);
    ROW(a,  A, 0, 1);
    ROW(a,  b, 0, 0);

    if (rhsHasVariableLength)
        ROW(a,  aB, 0, 0);

    ROW(ab, a,  1, 1);
    if (rhsHasVariableLength) {
        ROW(ab, ab, 1, 1);
        ROW(ab, aB, 0, 1);
        ROW(ab, Ab, 0, 1);
    }
    ROW(ab, c,  0, 0);

    if (rhsHasVariableLength)
        ROW(ab, abc, 0, 0);

    ROW(Abc, c, 0, 0);
    if (rhsHasVariableLength) {
        ROW(Abc, ab, 0, 1);
        ROW(Abc, aB, 0, 1);
        ROW(Abc, Ab, 1, 1);
        ROW(Abc, AB, 0, 1);
        ROW(aBC, ab, 0, 1);
        ROW(aBC, aB, 1, 1);
        ROW(aBC, Ab, 0, 1);
        ROW(aBC, AB, 0, 1);
    }
    ROW(ABC, b, 0, 0);
    ROW(ABC, a, 0, 1);
#undef ROW
}

template <typename Haystack, typename Needle>
void tst_QStringApiSymmetry::startsWith_impl() const
{
    QFETCH(const QStringRef, haystackU16);
    QFETCH(const QLatin1String, haystackL1);
    QFETCH(const QStringRef, needleU16);
    QFETCH(const QLatin1String, needleL1);
    QFETCH(const bool, resultCS);
    QFETCH(const bool, resultCIS);

    const auto haystackU8 = haystackU16.toUtf8();
    const auto needleU8 = needleU16.toUtf8();

    const auto haystack = make<Haystack>(haystackU16, haystackL1, haystackU8);
    const auto needle = make<Needle>(needleU16, needleL1, needleU8);

    QCOMPARE(haystack.startsWith(needle), resultCS);
    QCOMPARE(haystack.startsWith(needle, Qt::CaseSensitive), resultCS);
    QCOMPARE(haystack.startsWith(needle, Qt::CaseInsensitive), resultCIS);
}

void tst_QStringApiSymmetry::endsWith_data(bool rhsHasVariableLength)
{
    QTest::addColumn<QStringRef>("haystackU16");
    QTest::addColumn<QLatin1String>("haystackL1");
    QTest::addColumn<QStringRef>("needleU16");
    QTest::addColumn<QLatin1String>("needleL1");
    QTest::addColumn<bool>("resultCS");
    QTest::addColumn<bool>("resultCIS");

    if (rhsHasVariableLength) {
        QTest::addRow("null ~= null$")   << QStringRef() << QLatin1String()
                                         << QStringRef() << QLatin1String() << true << true;
        QTest::addRow("empty ~= null$")  << QStringRef(&empty) << QLatin1String("")
                                         << QStringRef() << QLatin1String() << true << true;
        QTest::addRow("a ~= null$")      << QStringRef(&a) << QLatin1String("a")
                                         << QStringRef() << QLatin1String() << true << true;
        QTest::addRow("null ~= empty$")  << QStringRef() << QLatin1String()
                                         << QStringRef(&empty) << QLatin1String("") << false << false;
        QTest::addRow("a ~= empty$")     << QStringRef(&a) << QLatin1String("a")
                                         << QStringRef(&empty) << QLatin1String("") << true << true;
        QTest::addRow("empty ~= empty$") << QStringRef(&empty) << QLatin1String("")
                                         << QStringRef(&empty) << QLatin1String("") << true << true;
    }
    QTest::addRow("null ~= a$")      << QStringRef() << QLatin1String()
                                     << QStringRef(&a) << QLatin1String("a") << false << false;
    QTest::addRow("empty ~= a$")     << QStringRef(&empty) << QLatin1String("")
                                     << QStringRef(&a) << QLatin1String("a") << false << false;

#define ROW(h, n, cs, cis) \
    QTest::addRow("%s ~= %s$", #h, #n) << QStringRef(&h) << QLatin1String(#h) \
                                       << QStringRef(&n) << QLatin1String(#n) \
                                       << bool(cs) << bool(cis)
    ROW(a,  a, 1, 1);
    ROW(a,  A, 0, 1);
    ROW(a,  b, 0, 0);

    if (rhsHasVariableLength)
        ROW(b, ab, 0, 0);

    ROW(ab, b,  1, 1);
    if (rhsHasVariableLength) {
        ROW(ab, ab, 1, 1);
        ROW(ab, aB, 0, 1);
        ROW(ab, Ab, 0, 1);
    }
    ROW(ab, c,  0, 0);

    if (rhsHasVariableLength)
        ROW(bc, abc, 0, 0);

    ROW(Abc, c, 1, 1);
    if (rhsHasVariableLength) {
        ROW(Abc, bc, 1, 1);
        ROW(Abc, bC, 0, 1);
        ROW(Abc, Bc, 0, 1);
        ROW(Abc, BC, 0, 1);
        ROW(aBC, bc, 0, 1);
        ROW(aBC, bC, 0, 1);
        ROW(aBC, Bc, 0, 1);
        ROW(aBC, BC, 1, 1);
    }
    ROW(ABC, b, 0, 0);
    ROW(ABC, a, 0, 0);
#undef ROW
}

template <typename Haystack, typename Needle>
void tst_QStringApiSymmetry::endsWith_impl() const
{
    QFETCH(const QStringRef, haystackU16);
    QFETCH(const QLatin1String, haystackL1);
    QFETCH(const QStringRef, needleU16);
    QFETCH(const QLatin1String, needleL1);
    QFETCH(const bool, resultCS);
    QFETCH(const bool, resultCIS);

    const auto haystackU8 = haystackU16.toUtf8();
    const auto needleU8 = needleU16.toUtf8();

    const auto haystack = make<Haystack>(haystackU16, haystackL1, haystackU8);
    const auto needle = make<Needle>(needleU16, needleL1, needleU8);

    QCOMPARE(haystack.endsWith(needle), resultCS);
    QCOMPARE(haystack.endsWith(needle, Qt::CaseSensitive), resultCS);
    QCOMPARE(haystack.endsWith(needle, Qt::CaseInsensitive), resultCIS);
}

void tst_QStringApiSymmetry::split_data(bool rhsHasVariableLength)
{
    QTest::addColumn<QStringRef>("haystackU16");
    QTest::addColumn<QLatin1String>("haystackL1");
    QTest::addColumn<QStringRef>("needleU16");
    QTest::addColumn<QLatin1String>("needleL1");
    QTest::addColumn<QStringList>("resultCS");
    QTest::addColumn<QStringList>("resultCIS");

    if (rhsHasVariableLength) {
        QTest::addRow("null ~= null$")   << QStringRef{} << QLatin1String{}
                                         << QStringRef{} << QLatin1String{}
                                         << QStringList{{}, {}} << QStringList{{}, {}};
        QTest::addRow("empty ~= null$")  << QStringRef{&empty} << QLatin1String("")
                                         << QStringRef{} << QLatin1String{}
                                         << QStringList{empty, empty} << QStringList{empty, empty};
        QTest::addRow("a ~= null$")      << QStringRef{&a} << QLatin1String{"a"}
                                         << QStringRef{} << QLatin1String{}
                                         << QStringList{empty, a, empty} << QStringList{empty, a, empty};
        QTest::addRow("null ~= empty$")  << QStringRef{} << QLatin1String{}
                                         << QStringRef{&empty} << QLatin1String{""}
                                         << QStringList{{}, {}} << QStringList{{}, {}};
        QTest::addRow("a ~= empty$")     << QStringRef{&a} << QLatin1String{"a"}
                                         << QStringRef{&empty} << QLatin1String{""}
                                         << QStringList{empty, a, empty} << QStringList{empty, a, empty};
        QTest::addRow("empty ~= empty$") << QStringRef{&empty} << QLatin1String{""}
                                         << QStringRef{&empty} << QLatin1String{""}
                                         << QStringList{empty, empty} << QStringList{empty, empty};
    }
    QTest::addRow("null ~= a$")      << QStringRef{} << QLatin1String{}
                                     << QStringRef{&a} << QLatin1String{"a"}
                                     << QStringList{{}} << QStringList{{}};
    QTest::addRow("empty ~= a$")     << QStringRef{&empty} << QLatin1String{""}
                                     << QStringRef{&a} << QLatin1String{"a"}
                                     << QStringList{empty} << QStringList{empty};

#define ROW(h, n, cs, cis) \
    QTest::addRow("%s ~= %s$", #h, #n) << QStringRef(&h) << QLatin1String(#h) \
                                       << QStringRef(&n) << QLatin1String(#n) \
                                       << QStringList cs << QStringList cis
    ROW(a,  a, ({empty, empty}), ({empty, empty}));
    ROW(a,  A, {a}, ({empty, empty}));
    ROW(a,  b, {a}, {a});

    if (rhsHasVariableLength)
        ROW(b, ab, {b}, {b});

    ROW(ab, b,  ({a, empty}), ({a, empty}));
    if (rhsHasVariableLength) {
        ROW(ab, ab, ({empty, empty}), ({empty, empty}));
        ROW(ab, aB, {ab}, ({empty, empty}));
        ROW(ab, Ab, {ab}, ({empty, empty}));
    }
    ROW(ab, c,  {ab}, {ab});

    if (rhsHasVariableLength)
        ROW(bc, abc, {bc}, {bc});

    ROW(Abc, c, ({Ab, empty}), ({Ab, empty}));
#if 0
    if (rhsHasVariableLength) {
        ROW(Abc, bc, 1, 1);
        ROW(Abc, bC, 0, 1);
        ROW(Abc, Bc, 0, 1);
        ROW(Abc, BC, 0, 1);
        ROW(aBC, bc, 0, 1);
        ROW(aBC, bC, 0, 1);
        ROW(aBC, Bc, 0, 1);
        ROW(aBC, BC, 1, 1);
    }
#endif
    ROW(ABC, b, {ABC}, ({A, C}));
    ROW(ABC, a, {ABC}, ({empty, BC}));
#undef ROW
}

static QStringList skipped(const QStringList &sl)
{
    QStringList result;
    result.reserve(sl.size());
    for (const QString &s : sl) {
        if (!s.isEmpty())
            result.push_back(s);
    }
    return result;
}

template <typename Haystack, typename Needle>
void tst_QStringApiSymmetry::split_impl() const
{
    QFETCH(const QStringRef, haystackU16);
    QFETCH(const QLatin1String, haystackL1);
    QFETCH(const QStringRef, needleU16);
    QFETCH(const QLatin1String, needleL1);
    QFETCH(const QStringList, resultCS);
    QFETCH(const QStringList, resultCIS);

    const QStringList skippedResultCS = skipped(resultCS);
    const QStringList skippedResultCIS = skipped(resultCIS);

    const auto haystackU8 = haystackU16.toUtf8();
    const auto needleU8 = needleU16.toUtf8();

    const auto haystack = make<Haystack>(haystackU16, haystackL1, haystackU8);
    const auto needle = make<Needle>(needleU16, needleL1, needleU8);

    QCOMPARE(toQStringList(haystack.split(needle)), resultCS);
    QCOMPARE(toQStringList(haystack.split(needle, Qt::KeepEmptyParts, Qt::CaseSensitive)), resultCS);
    QCOMPARE(toQStringList(haystack.split(needle, Qt::KeepEmptyParts, Qt::CaseInsensitive)), resultCIS);
    QCOMPARE(toQStringList(haystack.split(needle, Qt::SkipEmptyParts, Qt::CaseSensitive)), skippedResultCS);
    QCOMPARE(toQStringList(haystack.split(needle, Qt::SkipEmptyParts, Qt::CaseInsensitive)), skippedResultCIS);
}

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

    {
        const auto mid = s.mid(pos);
        const auto mid2 = s.mid(pos, n);

        QCOMPARE(mid, result);
        QCOMPARE(mid.isNull(), result.isNull());
        QCOMPARE(mid.isEmpty(), result.isEmpty());

        QCOMPARE(mid2, result2);
        QCOMPARE(mid2.isNull(), result2.isNull());
        QCOMPARE(mid2.isEmpty(), result2.isEmpty());
    }
    {
        const auto mid = detached(s).mid(pos);
        const auto mid2 = detached(s).mid(pos, n);

        QCOMPARE(mid, result);
        QCOMPARE(mid.isNull(), result.isNull());
        QCOMPARE(mid.isEmpty(), result.isEmpty());

        QCOMPARE(mid2, result2);
        QCOMPARE(mid2.isNull(), result2.isNull());
        QCOMPARE(mid2.isEmpty(), result2.isEmpty());
    }
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

    {
        const auto left = s.left(n);

        QCOMPARE(left, result);
        QCOMPARE(left.isNull(), result.isNull());
        QCOMPARE(left.isEmpty(), result.isEmpty());
    }
    {
        const auto left = detached(s).left(n);

        QCOMPARE(left, result);
        QCOMPARE(left.isNull(), result.isNull());
        QCOMPARE(left.isEmpty(), result.isEmpty());
    }
    {
        auto left = s;
        left.truncate(n);

        QCOMPARE(left, result);
        QCOMPARE(left.isNull(), result.isNull());
        QCOMPARE(left.isEmpty(), result.isEmpty());
    }
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

    {
        const auto right = s.right(n);

        QCOMPARE(right, result);
        QCOMPARE(right.isNull(), result.isNull());
        QCOMPARE(right.isEmpty(), result.isEmpty());
    }
    {
        const auto right = detached(s).right(n);

        QCOMPARE(right, result);
        QCOMPARE(right.isNull(), result.isNull());
        QCOMPARE(right.isEmpty(), result.isEmpty());
    }
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
        const auto chopped = s.chopped(n);

        QCOMPARE(chopped, result);
        QCOMPARE(chopped.isNull(), result.isNull());
        QCOMPARE(chopped.isEmpty(), result.isEmpty());
    }
    {
        const auto chopped = detached(s).chopped(n);

        QCOMPARE(chopped, result);
        QCOMPARE(chopped.isNull(), result.isNull());
        QCOMPARE(chopped.isEmpty(), result.isEmpty());
    }
    {
        auto chopped = s;
        chopped.chop(n);

        QCOMPARE(chopped, result);
        QCOMPARE(chopped.isNull(), result.isNull());
        QCOMPARE(chopped.isEmpty(), result.isEmpty());
    }
}

void tst_QStringApiSymmetry::trimmed_data()
{
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<QStringRef>("result");

    const auto latin1Whitespace = QLatin1String(" \r\n\t\f\v");

    QTest::addRow("null") << QString() << QStringRef();

    auto add = [latin1Whitespace](const QString &str) {
        // run through all substrings of latin1Whitespace
        for (int len = 0; len < latin1Whitespace.size(); ++len) {
            for (int pos = 0; pos < latin1Whitespace.size() - len; ++pos) {
                const QString unicode = latin1Whitespace.mid(pos, len) + str + latin1Whitespace.mid(pos, len);
                const QScopedArrayPointer<const char> escaped(QTest::toString(unicode));
                QTest::addRow("%s", escaped.data()) << unicode << QStringRef(&str);
            }
        }
    };

    add(empty);
    add(a);
    add(ab);
}

template <typename String>
void tst_QStringApiSymmetry::trimmed_impl()
{
    QFETCH(const QString, unicode);
    QFETCH(const QStringRef, result);

    const auto utf8 = unicode.toUtf8();
    const auto l1s  = unicode.toLatin1();
    const auto l1   = l1s.isNull() ? QLatin1String() : QLatin1String(l1s);

    const auto ref = unicode.isNull() ? QStringRef() : QStringRef(&unicode);
    const auto s = make<String>(ref, l1, utf8);

    QCOMPARE(s.isNull(), unicode.isNull());

    {
        const auto trimmed = s.trimmed();

        QCOMPARE(trimmed, result);
        QCOMPARE(trimmed.isNull(), result.isNull());
        QCOMPARE(trimmed.isEmpty(), result.isEmpty());
    }
    {
        const auto trimmed = detached(s).trimmed();

        QCOMPARE(trimmed, result);
        QCOMPARE(trimmed.isNull(), result.isNull());
        QCOMPARE(trimmed.isEmpty(), result.isEmpty());
    }
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
        QTest::newRow(rowName(ba).constData()) << s << ba;
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
        QTest::newRow(rowName(ba).constData()) << s << ba;
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
        QTest::newRow(rowName(ba).constData()) << s << ba;
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
        QTest::newRow(rowName(ba).constData()) << s << ucs4;
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

void tst_QStringApiSymmetry::indexOf_data(bool rhsHasVariableLength)
{
    QTest::addColumn<QString>("haystackU16");
    QTest::addColumn<QLatin1String>("haystackL1");
    QTest::addColumn<QString>("needleU16");
    QTest::addColumn<QLatin1String>("needleL1");
    QTest::addColumn<qsizetype>("startpos");
    QTest::addColumn<qsizetype>("resultCS");
    QTest::addColumn<qsizetype>("resultCIS");

    constexpr qsizetype zeroPos = 0;
    constexpr qsizetype minus1Pos = -1;

    if (rhsHasVariableLength) {
        QTest::addRow("haystack: null, needle: null") << null << QLatin1String()
                                     << null << QLatin1String() << zeroPos << zeroPos << zeroPos;
        QTest::addRow("haystack: empty, needle: null")  << empty << QLatin1String("")
                                     << null << QLatin1String() << zeroPos << zeroPos << zeroPos;
        QTest::addRow("haystack: a, needle: null") << a << QLatin1String("a")
                                     << null << QLatin1String() << zeroPos << zeroPos << zeroPos;
        QTest::addRow("haystack: null, needle: empty") << null << QLatin1String()
                                     << empty << QLatin1String("") << zeroPos << zeroPos << zeroPos;
        QTest::addRow("haystack: a, needle: empty") << a << QLatin1String("a")
                                     << empty << QLatin1String("") << zeroPos << zeroPos << zeroPos;
        QTest::addRow("haystack: empty, needle: empty") << empty << QLatin1String("")
                                     << empty << QLatin1String("") << zeroPos << zeroPos << zeroPos;
    }
    QTest::addRow("haystack: empty, needle: a") << empty << QLatin1String("")
                                     << a << QLatin1String("a") << zeroPos << minus1Pos << minus1Pos;
    QTest::addRow("haystack: null, needle: a") << null << QLatin1String()
                                     << a << QLatin1String("a") << zeroPos << minus1Pos << minus1Pos;


#define ROW(h, n, st, cs, cis) \
    QTest::addRow("haystack: %s, needle: %s", #h, #n) << h << QLatin1String(#h) \
                                       << n << QLatin1String(#n) \
                                       << qsizetype(st) << qsizetype(cs) << qsizetype(cis)

    ROW(abc, a, 0,  0,  0);
    ROW(abc, A, 0, -1,  0);
    ROW(abc, a, 1, -1, -1);
    ROW(abc, A, 1, -1, -1);
    ROW(abc, b, 0,  1,  1);
    ROW(abc, B, 0, -1,  1);
    ROW(abc, b, 1,  1,  1);
    ROW(abc, B, 1, -1,  1);
    ROW(abc, B, 2, -1, -1);

    ROW(ABC, A, 0,  0,  0);
    ROW(ABC, a, 0, -1,  0);
    ROW(ABC, A, 1, -1, -1);
    ROW(ABC, a, 1, -1, -1);
    ROW(ABC, B, 0,  1,  1);
    ROW(ABC, b, 0, -1,  1);
    ROW(ABC, B, 1,  1,  1);
    ROW(ABC, b, 1, -1,  1);
    ROW(ABC, B, 2, -1, -1);

    if (rhsHasVariableLength) {
        ROW(aBc, bc, 0, -1,  1);
        ROW(aBc, Bc, 0,  1,  1);
        ROW(aBc, bC, 0, -1,  1);
        ROW(aBc, BC, 0, -1,  1);

        ROW(AbC, bc, 0, -1,  1);
        ROW(AbC, Bc, 0, -1,  1);
        ROW(AbC, bC, 0,  1,  1);
        ROW(AbC, BC, 0, -1,  1);
        ROW(AbC, BC, 1, -1,  1);
        ROW(AbC, BC, 2, -1, -1);
    }
#undef ROW

}

template <typename Haystack, typename Needle>
void tst_QStringApiSymmetry::indexOf_impl() const
{
    QFETCH(const QString, haystackU16);
    QFETCH(const QLatin1String, haystackL1);
    QFETCH(const QString, needleU16);
    QFETCH(const QLatin1String, needleL1);
    QFETCH(const qsizetype, startpos);
    QFETCH(const qsizetype, resultCS);
    QFETCH(const qsizetype, resultCIS);

    const auto haystackU8 = haystackU16.toUtf8();
    const auto needleU8 = needleU16.toUtf8();

    const auto haystack = make<Haystack>(QStringRef(&haystackU16), haystackL1, haystackU8);
    const auto needle = make<Needle>(QStringRef(&needleU16), needleL1, needleU8);

    using size_type = typename Haystack::size_type;

    QCOMPARE(haystack.indexOf(needle, startpos), size_type(resultCS));
    QCOMPARE(haystack.indexOf(needle, startpos, Qt::CaseSensitive), size_type(resultCS));
    QCOMPARE(haystack.indexOf(needle, startpos, Qt::CaseInsensitive), size_type(resultCIS));
}

static QString ABCDEFGHIEfGEFG = QStringLiteral("ABCDEFGHIEfGEFG");
static QString EFG = QStringLiteral("EFG");
static QString efg = QStringLiteral("efg");
static QString asd = QStringLiteral("asd");
static QString asdf = QStringLiteral("asdf");
static QString Z = QStringLiteral("Z");

void tst_QStringApiSymmetry::contains_data(bool rhsHasVariableLength)
{
    QTest::addColumn<QString>("haystackU16");
    QTest::addColumn<QLatin1String>("haystackL1");
    QTest::addColumn<QString>("needleU16");
    QTest::addColumn<QLatin1String>("needleL1");
    QTest::addColumn<bool>("resultCS");
    QTest::addColumn<bool>("resultCIS");

    if (rhsHasVariableLength) {
        QTest::addRow("haystack: null, needle: null") << null << QLatin1String()
                                     << null << QLatin1String() << true << true;
        QTest::addRow("haystack: empty, needle: null")  << empty << QLatin1String("")
                                     << null << QLatin1String() << true << true;
        QTest::addRow("haystack: a, needle: null") << a << QLatin1String("a")
                                     << null << QLatin1String() << true << true;
        QTest::addRow("haystack: null, needle: empty") << null << QLatin1String()
                                     << empty << QLatin1String("") << true << true;
        QTest::addRow("haystack: a, needle: empty") << a << QLatin1String("a")
                                     << empty << QLatin1String("") << true << true;;
        QTest::addRow("haystack: empty, needle: empty") << empty << QLatin1String("")
                                     << empty << QLatin1String("") << true << true;
    }
    QTest::addRow("haystack: empty, needle: a") << empty << QLatin1String("")
                                     << a << QLatin1String("a") << false << false;
    QTest::addRow("haystack: null, needle: a") << null << QLatin1String()
                                     << a << QLatin1String("a") << false << false;

#define ROW(h, n, cs, cis) \
    QTest::addRow("haystack: %s, needle: %s", #h, #n) << h << QLatin1String(#h) \
                                       << n << QLatin1String(#n) \
                                       << cs << cis

    ROW(ABCDEFGHIEfGEFG, A, true, true);
    ROW(ABCDEFGHIEfGEFG, a, false, true);
    ROW(ABCDEFGHIEfGEFG, Z, false, false);
    if (rhsHasVariableLength) {
        ROW(ABCDEFGHIEfGEFG, EFG, true, true);
        ROW(ABCDEFGHIEfGEFG, efg, false, true);
    }
    ROW(ABCDEFGHIEfGEFG, E, true, true);
    ROW(ABCDEFGHIEfGEFG, e, false, true);
#undef ROW
}

template <typename Haystack, typename Needle>
void tst_QStringApiSymmetry::contains_impl() const
{
    QFETCH(const QString, haystackU16);
    QFETCH(const QLatin1String, haystackL1);
    QFETCH(const QString, needleU16);
    QFETCH(const QLatin1String, needleL1);
    QFETCH(const bool, resultCS);
    QFETCH(const bool, resultCIS);

    const auto haystackU8 = haystackU16.toUtf8();
    const auto needleU8 = needleU16.toUtf8();

    const auto haystack = make<Haystack>(QStringRef(&haystackU16), haystackL1, haystackU8);
    const auto needle = make<Needle>(QStringRef(&needleU16), needleL1, needleU8);

    QCOMPARE(haystack.contains(needle), resultCS);
    QCOMPARE(haystack.contains(needle, Qt::CaseSensitive), resultCS);
    QCOMPARE(haystack.contains(needle, Qt::CaseInsensitive), resultCIS);
}

void tst_QStringApiSymmetry::lastIndexOf_data(bool rhsHasVariableLength)
{
    QTest::addColumn<QString>("haystackU16");
    QTest::addColumn<QLatin1String>("haystackL1");
    QTest::addColumn<QString>("needleU16");
    QTest::addColumn<QLatin1String>("needleL1");
    QTest::addColumn<qsizetype>("startpos");
    QTest::addColumn<qsizetype>("resultCS");
    QTest::addColumn<qsizetype>("resultCIS");

    constexpr qsizetype zeroPos = 0;
    constexpr qsizetype minus1Pos = -1;

    if (rhsHasVariableLength) {
        QTest::addRow("haystack: null, needle: null") << null << QLatin1String()
                                     << null << QLatin1String() << minus1Pos << minus1Pos << minus1Pos;
        QTest::addRow("haystack: empty, needle: null")  << empty << QLatin1String("")
                                     << null << QLatin1String() << minus1Pos << minus1Pos << minus1Pos;
        QTest::addRow("haystack: a, needle: null") << a << QLatin1String("a")
                                     << null << QLatin1String() << minus1Pos << zeroPos << zeroPos;
        QTest::addRow("haystack: null, needle: empty") << null << QLatin1String()
                                     << empty << QLatin1String("") << minus1Pos << minus1Pos << minus1Pos;
        QTest::addRow("haystack: a, needle: empty") << a << QLatin1String("a")
                                     << empty << QLatin1String("") << minus1Pos << zeroPos << zeroPos;
        QTest::addRow("haystack: empty, needle: empty") << empty << QLatin1String("")
                                     << empty << QLatin1String("") << minus1Pos << minus1Pos << minus1Pos;
    }
    QTest::addRow("haystack: empty, needle: a") << empty << QLatin1String("")
                                     << a << QLatin1String("a") << minus1Pos << minus1Pos << minus1Pos;
    QTest::addRow("haystack: null, needle: a") << null << QLatin1String()
                                     << a << QLatin1String("a") << minus1Pos << minus1Pos << minus1Pos;

    if (rhsHasVariableLength) {
        QTest::addRow("haystack: a, needle: null")  << a << QLatin1String("a")
                                     << null << QLatin1String() << qsizetype(1) << qsizetype(1) << qsizetype(1);
        QTest::addRow("haystack: a, needle: empty")  << a << QLatin1String("a")
                                     << empty << QLatin1String("") << qsizetype(1) << qsizetype(1) << qsizetype(1);
        QTest::addRow("haystack: a, needle: null")  << a << QLatin1String("a")
                                     << null << QLatin1String() << qsizetype(2) << minus1Pos << minus1Pos;
        QTest::addRow("haystack: a, needle: empty")  << a << QLatin1String("a")
                                     << empty << QLatin1String("") << qsizetype(2) << minus1Pos << minus1Pos;
    }

#define ROW(h, n, st, cs, cis) \
    QTest::addRow("haystack: %s, needle: %s", #h, #n) << h << QLatin1String(#h) \
                                       << n << QLatin1String(#n) \
                                       << qsizetype(st) << qsizetype(cs) << qsizetype(cis)

    if (rhsHasVariableLength)
        ROW(asd,   asdf, -1, -1, -1);

    ROW(ABCDEFGHIEfGEFG, G,  -1, 14, 14);
    ROW(ABCDEFGHIEfGEFG, g,  -1, -1, 14);
    ROW(ABCDEFGHIEfGEFG, G,  -3, 11, 11);
    ROW(ABCDEFGHIEfGEFG, g,  -3, -1, 11);
    ROW(ABCDEFGHIEfGEFG, G,  -5,  6,  6);
    ROW(ABCDEFGHIEfGEFG, g,  -5, -1,  6);
    ROW(ABCDEFGHIEfGEFG, G,  14, 14, 14);
    ROW(ABCDEFGHIEfGEFG, g,  14, -1, 14);
    ROW(ABCDEFGHIEfGEFG, G,  13, 11, 11);
    ROW(ABCDEFGHIEfGEFG, g,  13, -1, 11);
    ROW(ABCDEFGHIEfGEFG, G,  15, -1, -1);
    ROW(ABCDEFGHIEfGEFG, g,  15, -1, -1);
    ROW(ABCDEFGHIEfGEFG, B,  14,  1,  1);
    ROW(ABCDEFGHIEfGEFG, b,  14, -1,  1);
    ROW(ABCDEFGHIEfGEFG, B,  -1,  1,  1);
    ROW(ABCDEFGHIEfGEFG, b,  -1, -1,  1);
    ROW(ABCDEFGHIEfGEFG, B,   1,  1,  1);
    ROW(ABCDEFGHIEfGEFG, b,   1, -1,  1);
    ROW(ABCDEFGHIEfGEFG, B,   0, -1, -1);
    ROW(ABCDEFGHIEfGEFG, b,   0, -1, -1);
    ROW(ABCDEFGHIEfGEFG, A,   0,  0,  0);
    ROW(ABCDEFGHIEfGEFG, a,   0, -1,  0);
    ROW(ABCDEFGHIEfGEFG, A, -15,  0,  0);
    ROW(ABCDEFGHIEfGEFG, a, -15, -1,  0);

    if (rhsHasVariableLength) {
        ROW(ABCDEFGHIEfGEFG, efg,   0, -1, -1);
        ROW(ABCDEFGHIEfGEFG, efg,  15, -1, -1);
        ROW(ABCDEFGHIEfGEFG, efg, -15, -1, -1);
        ROW(ABCDEFGHIEfGEFG, efg,  14, -1, 12);
        ROW(ABCDEFGHIEfGEFG, efg,  12, -1, 12);
        ROW(ABCDEFGHIEfGEFG, efg, -12, -1, -1);
        ROW(ABCDEFGHIEfGEFG, efg,  11, -1,  9);
    }
#undef ROW
}

template <typename Haystack, typename Needle>
void tst_QStringApiSymmetry::lastIndexOf_impl() const
{
    QFETCH(const QString, haystackU16);
    QFETCH(const QLatin1String, haystackL1);
    QFETCH(const QString, needleU16);
    QFETCH(const QLatin1String, needleL1);
    QFETCH(const qsizetype, startpos);
    QFETCH(const qsizetype, resultCS);
    QFETCH(const qsizetype, resultCIS);

    const auto haystackU8 = haystackU16.toUtf8();
    const auto needleU8 = needleU16.toUtf8();

    const auto haystack = make<Haystack>(QStringRef(&haystackU16), haystackL1, haystackU8);
    const auto needle = make<Needle>(QStringRef(&needleU16), needleL1, needleU8);

    using size_type = typename Haystack::size_type;

    QCOMPARE(haystack.lastIndexOf(needle, startpos), size_type(resultCS));
    QCOMPARE(haystack.lastIndexOf(needle, startpos, Qt::CaseSensitive), size_type(resultCS));
    QCOMPARE(haystack.lastIndexOf(needle, startpos, Qt::CaseInsensitive), size_type(resultCIS));

}

QTEST_APPLESS_MAIN(tst_QStringApiSymmetry)

#include "tst_qstringapisymmetry.moc"
