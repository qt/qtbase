/****************************************************************************
**
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#include <QChar>
#include <QLatin1String>
#include <QList>
#include <QScopedArrayPointer>
#include <QString>
#include <QStringTokenizer>
#include <QStringView>
#include <QTest>

#include "../../../../shared/localechange.h"

#include <locale.h>

Q_DECLARE_METATYPE(QLatin1String)

struct QAnyStringViewUsingL1  : QAnyStringView {};  // QAnyStringView with Latin-1 content
struct QAnyStringViewUsingU8  : QAnyStringView {};  // QAnyStringView with Utf-8 content
struct QAnyStringViewUsingU16 : QAnyStringView {};  // QAnyStringView with Utf-16 content

template <typename T>
QString toQString(const T &t) { return QString(t); }
QString toQString(QStringView view) { return view.toString(); }
QString toQString(QUtf8StringView view) { return view.toString(); }

template <typename Iterable>
QStringList toQStringList(const Iterable &i) {
    QStringList result;
    for (auto &e : i)
        result.push_back(toQString(e));
    return result;
}

template <typename LHS, typename RHS>
constexpr bool is_fake_comparator_v = false;

// FIXME: these are missing at the time of writing, add them, then remove the dummies here:
#define MAKE_RELOP(op, A1, A2) \
    static bool operator op (A1 lhs, A2 rhs) \
    { return toQString(lhs) op toQString(rhs); } \
    /*end*/
#define MAKE_ALL(A1, A2) \
    template <> constexpr bool is_fake_comparator_v<A1, A2> = true; \
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

MAKE_ALL(QChar, QByteArray)
MAKE_ALL(QChar, const char*)
MAKE_ALL(QChar, QUtf8StringView)

MAKE_ALL(QString, QUtf8StringView)
MAKE_ALL(QByteArray, QUtf8StringView)
MAKE_ALL(const char*, QUtf8StringView)

MAKE_ALL(QUtf8StringView, QChar)
MAKE_ALL(QUtf8StringView, char16_t)
MAKE_ALL(QUtf8StringView, QStringView)
MAKE_ALL(QUtf8StringView, QLatin1String)

#undef MAKE_ALL
#undef MAKE_RELOP
// END FIXME

static constexpr int sign(int i) noexcept
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
    // test all combinations of {QChar, char16_t, QString, QStringView, QLatin1String, QByteArray, const char*}
    void compare_QChar_QChar_data() { compare_data(false); }
    void compare_QChar_QChar() { compare_impl<QChar, QChar>(); }
    void compare_QChar_char16_t_data() { compare_data(false); }
    void compare_QChar_char16_t() { compare_impl<QChar, char16_t>(); }
    void compare_QChar_QString_data() { compare_data(false); }
    void compare_QChar_QString() { compare_impl<QChar, QString>(); }
    void compare_QChar_QStringView_data() { compare_data(false); }
    void compare_QChar_QStringView() { compare_impl<QChar, QStringView>(); }
    void compare_QChar_QUtf8StringView_data() { compare_data(false); }
    void compare_QChar_QUtf8StringView() { compare_impl<QChar, QUtf8StringView>(); }
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
    void compare_char16_t_QString_data() { compare_data(false); }
    void compare_char16_t_QString() { compare_impl<char16_t, QString>(); }
    void compare_char16_t_QStringView_data() { compare_data(false); }
    void compare_char16_t_QStringView() { compare_impl<char16_t, QStringView>(); }
    void compare_char16_t_QUtf8StringView_data() { compare_data(false); }
    void compare_char16_t_QUtf8StringView() { compare_impl<char16_t, QUtf8StringView>(); }
    void compare_char16_t_QLatin1String_data() { compare_data(false); }
    void compare_char16_t_QLatin1String() { compare_impl<char16_t, QLatin1String>(); }
    void compare_char16_t_QByteArray_data() { compare_data(false); }
    void compare_char16_t_QByteArray() { compare_impl<char16_t, QByteArray>(); }
    //void compare_char16_t_const_char_star_data() { compare_data(false); }
    //void compare_char16_t_const_char_star() { compare_impl<char16_t, const char *>(); }

    void compare_QString_QChar_data() { compare_data(false); }
    void compare_QString_QChar() { compare_impl<QString, QChar>(); }
    void compare_QString_char16_t_data() { compare_data(false); }
    void compare_QString_char16_t() { compare_impl<QString, char16_t>(); }
    void compare_QString_QString_data() { compare_data(); }
    void compare_QString_QString() { compare_impl<QString, QString>(); }
    void compare_QString_QStringView_data() { compare_data(); }
    void compare_QString_QStringView() { compare_impl<QString, QStringView>(); }
    void compare_QString_QUtf8StringView_data() { compare_data(); }
    void compare_QString_QUtf8StringView() { compare_impl<QString, QUtf8StringView>(); }
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
    void compare_QStringView_QString_data() { compare_data(); }
    void compare_QStringView_QString() { compare_impl<QStringView, QString>(); }
    void compare_QStringView_QStringView_data() { compare_data(); }
    void compare_QStringView_QStringView() { compare_impl<QStringView, QStringView>(); }
    void compare_QStringView_QUtf8StringView_data() { compare_data(); }
    void compare_QStringView_QUtf8StringView() { compare_impl<QStringView, QStringView>(); }
    void compare_QStringView_QLatin1String_data() { compare_data(); }
    void compare_QStringView_QLatin1String() { compare_impl<QStringView, QLatin1String>(); }
#ifdef NOT_YET_IMPLMENTED
    void compare_QStringView_QByteArray_data() { compare_data(); }
    void compare_QStringView_QByteArray() { compare_impl<QStringView, QByteArray>(); }
    void compare_QStringView_const_char_star_data() { compare_data(); }
    void compare_QStringView_const_char_star() { compare_impl<QStringView, const char *>(); }
#endif

    void compare_QUtf8StringView_QChar_data() { compare_data(false); }
    void compare_QUtf8StringView_QChar() { compare_impl<QUtf8StringView, QChar>(); }
    void compare_QUtf8StringView_char16_t_data() { compare_data(false); }
    void compare_QUtf8StringView_char16_t() { compare_impl<QUtf8StringView, char16_t>(); }
    void compare_QUtf8StringView_QString_data() { compare_data(); }
    void compare_QUtf8StringView_QString() { compare_impl<QUtf8StringView, QString>(); }
    void compare_QUtf8StringView_QStringView_data() { compare_data(); }
    void compare_QUtf8StringView_QStringView() { compare_impl<QUtf8StringView, QStringView>(); }
    void compare_QUtf8StringView_QUtf8StringView_data() { compare_data(); }
    void compare_QUtf8StringView_QUtf8StringView() { compare_impl<QUtf8StringView, QUtf8StringView>(); }
    void compare_QUtf8StringView_QLatin1String_data() { compare_data(); }
    void compare_QUtf8StringView_QLatin1String() { compare_impl<QUtf8StringView, QLatin1String>(); }
#ifdef NOT_YET_IMPLMENTED
    void compare_QUtf8StringView_QByteArray_data() { compare_data(); }
    void compare_QUtf8StringView_QByteArray() { compare_impl<QUtf8StringView, QByteArray>(); }
    void compare_QUtf8StringView_const_char_star_data() { compare_data(); }
    void compare_QUtf8StringView_const_char_star() { compare_impl<QUtf8StringView, const char *>(); }
#endif

    void compare_QLatin1String_QChar_data() { compare_data(false); }
    void compare_QLatin1String_QChar() { compare_impl<QLatin1String, QChar>(); }
    void compare_QLatin1String_char16_t_data() { compare_data(false); }
    void compare_QLatin1String_char16_t() { compare_impl<QLatin1String, char16_t>(); }
    void compare_QLatin1String_QString_data() { compare_data(); }
    void compare_QLatin1String_QString() { compare_impl<QLatin1String, QString>(); }
    void compare_QLatin1String_QStringView_data() { compare_data(); }
    void compare_QLatin1String_QStringView() { compare_impl<QLatin1String, QStringView>(); }
    void compare_QLatin1String_QUtf8StringView_data() { compare_data(); }
    void compare_QLatin1String_QUtf8StringView() { compare_impl<QLatin1String, QUtf8StringView>(); }
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
    void compare_QByteArray_QString_data() { compare_data(); }
    void compare_QByteArray_QString() { compare_impl<QByteArray, QString>(); }
#ifdef NOT_YET_IMPLEMENTED
    void compare_QByteArray_QStringView_data() { compare_data(); }
    void compare_QByteArray_QStringView() { compare_impl<QByteArray, QStringView>(); }
#endif
    void compare_QByteArray_QUtf8StringView_data() { compare_data(); }
    void compare_QByteArray_QUtf8StringView() { compare_impl<QByteArray, QUtf8StringView>(); }
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
    void compare_const_char_star_QString_data() { compare_data(); }
    void compare_const_char_star_QString() { compare_impl<const char *, QString>(); }
    void compare_const_char_star_QUtf8StringView_data() { compare_data(); }
    void compare_const_char_star_QUtf8StringView() { compare_impl<const char *, QUtf8StringView>(); }
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
    // test all combinations of {QChar, char16_t, QString, QStringView, QLatin1String, QByteArray, const char*}
#ifdef NOT_YET_IMPLEMENTED // probably never will be - what's the point of QChar::compare(QStringView)?
    void member_compare_QChar_QChar_data() { member_compare_data(false); }
    void member_compare_QChar_QChar() { member_compare_impl<QChar, QChar>(); }
    void member_compare_QChar_char16_t_data() { member_compare_data(false); }
    void member_compare_QChar_char16_t() { member_compare_impl<QChar, char16_t>(); }
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

    void member_compare_QString_QChar_data() { member_compare_data(false); }
    void member_compare_QString_QChar() { member_compare_impl<QString, QChar>(); }
    void member_compare_QString_char16_t_data() { member_compare_data(false); }
    void member_compare_QString_char16_t() { member_compare_impl<QString, char16_t>(); }
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
    void localeAwareCompare_data();
    template<typename LHS, typename RHS>
    void localeAwareCompare_impl();

private Q_SLOTS:
    void localeAwareCompare_QString_QString_data() { localeAwareCompare_data(); }
    void localeAwareCompare_QString_QString() { localeAwareCompare_impl<QString, QString>(); }
    void localeAwareCompare_QString_QStringView_data() { localeAwareCompare_data(); }
    void localeAwareCompare_QString_QStringView() { localeAwareCompare_impl<QString, QStringView>(); }
    void localeAwareCompare_QStringView_QString_data() { localeAwareCompare_data(); }
    void localeAwareCompare_QStringView_QString() { localeAwareCompare_impl<QStringView, QString>(); }
    void localeAwareCompare_QStringView_QStringView_data() { localeAwareCompare_data(); }
    void localeAwareCompare_QStringView_QStringView() { localeAwareCompare_impl<QStringView, QStringView>(); }

private:
    void member_localeAwareCompare_data() { localeAwareCompare_data(); }
    template<typename LHS, typename RHS>
    void member_localeAwareCompare_impl();

private Q_SLOTS:
    void member_localeAwareCompare_QString_QString_data() { member_localeAwareCompare_data(); }
    void member_localeAwareCompare_QString_QString() { member_localeAwareCompare_impl<QString, QString>(); }
    void member_localeAwareCompare_QString_QStringView_data() { member_localeAwareCompare_data(); }
    void member_localeAwareCompare_QString_QStringView() { member_localeAwareCompare_impl<QString, QStringView>(); }

private:
    void startsWith_data(bool rhsIsQChar = false);
    template <typename Haystack, typename Needle> void startsWith_impl() const;

    void endsWith_data(bool rhsIsQChar = false);
    template <typename Haystack, typename Needle> void endsWith_impl() const;

private Q_SLOTS:
    // test all combinations of {QString, QStringView, QLatin1String} x {QString, QStringView, QLatin1String, QChar, char16_t}:
    void startsWith_QString_QString_data() { startsWith_data(); }
    void startsWith_QString_QString() { startsWith_impl<QString, QString>(); }
    void startsWith_QString_QStringView_data() { startsWith_data(); }
    void startsWith_QString_QStringView() { startsWith_impl<QString, QStringView>(); }
    void startsWith_QString_QLatin1String_data() { startsWith_data(); }
    void startsWith_QString_QLatin1String() { startsWith_impl<QString, QLatin1String>(); }
    void startsWith_QString_QChar_data() { startsWith_data(false); }
    void startsWith_QString_QChar() { startsWith_impl<QString, QChar>(); }
    void startsWith_QString_char16_t_data() { startsWith_data(false); }
    void startsWith_QString_char16_t() { startsWith_impl<QString, char16_t>(); }

    void startsWith_QStringView_QString_data() { startsWith_data(); }
    void startsWith_QStringView_QString() { startsWith_impl<QStringView, QString>(); }
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
    void endsWith_QString_QStringView_data() { endsWith_data(); }
    void endsWith_QString_QStringView() { endsWith_impl<QString, QStringView>(); }
    void endsWith_QString_QLatin1String_data() { endsWith_data(); }
    void endsWith_QString_QLatin1String() { endsWith_impl<QString, QLatin1String>(); }
    void endsWith_QString_QChar_data() { endsWith_data(false); }
    void endsWith_QString_QChar() { endsWith_impl<QString, QChar>(); }
    void endsWith_QString_char16_t_data() { endsWith_data(false); }
    void endsWith_QString_char16_t() { endsWith_impl<QString, char16_t>(); }

    void endsWith_QStringView_QString_data() { endsWith_data(); }
    void endsWith_QStringView_QString() { endsWith_impl<QStringView, QString>(); }
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
    // test all combinations of {QString} x {QString, QLatin1String, QChar, char16_t}:
    void split_QString_QString_data() { split_data(); }
    void split_QString_QString() { split_impl<QString, QString>(); }
    void split_QString_QLatin1String_data() { split_data(); }
    void split_QString_QLatin1String() { split_impl<QString, QLatin1String>(); }
    void split_QString_QChar_data() { split_data(false); }
    void split_QString_QChar() { split_impl<QString, QChar>(); }
    void split_QString_char16_t_data() { split_data(false); }
    void split_QString_char16_t() { split_impl<QString, char16_t>(); }

private:
    void tok_data(bool rhsHasVariableLength = true);
    template <typename Haystack, typename Needle> void tok_impl() const;

private Q_SLOTS:
    // let Splittable = {QString, QStringView, QLatin1String, const char16_t*, std::u16string}
    // let Separators = Splittable ∪ {QChar, char16_t}
    // test Splittable × Separators:
    void tok_QString_QString_data() { tok_data(); }
    void tok_QString_QString() { tok_impl<QString, QString>(); }
    void tok_QString_QStringView_data() { tok_data(); }
    void tok_QString_QStringView() { tok_impl<QString, QStringView>(); }
    void tok_QString_QLatin1String_data() { tok_data(); }
    void tok_QString_QLatin1String() { tok_impl<QString, QLatin1String>(); }
    void tok_QString_const_char16_t_star_data() { tok_data(); }
    void tok_QString_const_char16_t_star() { tok_impl<QString, const char16_t*>(); }
    void tok_QString_stdu16string_data() { tok_data(); }
    void tok_QString_stdu16string() { tok_impl<QString, std::u16string>(); }
    void tok_QString_QChar_data() { tok_data(false); }
    void tok_QString_QChar() { tok_impl<QString, QChar>(); }
    void tok_QString_char16_t_data() { tok_data(false); }
    void tok_QString_char16_t() { tok_impl<QString, char16_t>(); }

    void tok_QStringView_QString_data() { tok_data(); }
    void tok_QStringView_QString() { tok_impl<QStringView, QString>(); }
    void tok_QStringView_QStringView_data() { tok_data(); }
    void tok_QStringView_QStringView() { tok_impl<QStringView, QStringView>(); }
    void tok_QStringView_QLatin1String_data() { tok_data(); }
    void tok_QStringView_QLatin1String() { tok_impl<QStringView, QLatin1String>(); }
    void tok_QStringView_const_char16_t_star_data() { tok_data(); }
    void tok_QStringView_const_char16_t_star() { tok_impl<QStringView, const char16_t*>(); }
    void tok_QStringView_stdu16string_data() { tok_data(); }
    void tok_QStringView_stdu16string() { tok_impl<QStringView, std::u16string>(); }
    void tok_QStringView_QChar_data() { tok_data(false); }
    void tok_QStringView_QChar() { tok_impl<QStringView, QChar>(); }
    void tok_QStringView_char16_t_data() { tok_data(false); }
    void tok_QStringView_char16_t() { tok_impl<QStringView, char16_t>(); }

    void tok_QLatin1String_QString_data() { tok_data(); }
    void tok_QLatin1String_QString() { tok_impl<QLatin1String, QString>(); }
    void tok_QLatin1String_QStringView_data() { tok_data(); }
    void tok_QLatin1String_QStringView() { tok_impl<QLatin1String, QStringView>(); }
    void tok_QLatin1String_QLatin1String_data() { tok_data(); }
    void tok_QLatin1String_QLatin1String() { tok_impl<QLatin1String, QLatin1String>(); }
    void tok_QLatin1String_const_char16_t_star_data() { tok_data(); }
    void tok_QLatin1String_const_char16_t_star() { tok_impl<QLatin1String, const char16_t*>(); }
    void tok_QLatin1String_stdu16string_data() { tok_data(); }
    void tok_QLatin1String_stdu16string() { tok_impl<QLatin1String, std::u16string>(); }
    void tok_QLatin1String_QChar_data() { tok_data(false); }
    void tok_QLatin1String_QChar() { tok_impl<QLatin1String, QChar>(); }
    void tok_QLatin1String_char16_t_data() { tok_data(false); }
    void tok_QLatin1String_char16_t() { tok_impl<QLatin1String, char16_t>(); }

    void tok_const_char16_t_star_QString_data() { tok_data(); }
    void tok_const_char16_t_star_QString() { tok_impl<const char16_t*, QString>(); }
    void tok_const_char16_t_star_QStringView_data() { tok_data(); }
    void tok_const_char16_t_star_QStringView() { tok_impl<const char16_t*, QStringView>(); }
    void tok_const_char16_t_star_QLatin1String_data() { tok_data(); }
    void tok_const_char16_t_star_QLatin1String() { tok_impl<const char16_t*, QLatin1String>(); }
    void tok_const_char16_t_star_const_char16_t_star_data() { tok_data(); }
    void tok_const_char16_t_star_const_char16_t_star() { tok_impl<const char16_t*, const char16_t*>(); }
    void tok_const_char16_t_star_stdu16string_data() { tok_data(); }
    void tok_const_char16_t_star_stdu16string() { tok_impl<const char16_t*, std::u16string>(); }
    void tok_const_char16_t_star_QChar_data() { tok_data(false); }
    void tok_const_char16_t_star_QChar() { tok_impl<const char16_t*, QChar>(); }
    void tok_const_char16_t_star_char16_t_data() { tok_data(false); }
    void tok_const_char16_t_star_char16_t() { tok_impl<const char16_t*, char16_t>(); }

    void tok_stdu16string_QString_data() { tok_data(); }
    void tok_stdu16string_QString() { tok_impl<std::u16string, QString>(); }
    void tok_stdu16string_QStringView_data() { tok_data(); }
    void tok_stdu16string_QStringView() { tok_impl<std::u16string, QStringView>(); }
    void tok_stdu16string_QLatin1String_data() { tok_data(); }
    void tok_stdu16string_QLatin1String() { tok_impl<std::u16string, QLatin1String>(); }
    void tok_stdu16string_const_char16_t_star_data() { tok_data(); }
    void tok_stdu16string_const_char16_t_star() { tok_impl<std::u16string, const char16_t*>(); }
    void tok_stdu16string_stdu16string_data() { tok_data(); }
    void tok_stdu16string_stdu16string() { tok_impl<std::u16string, std::u16string>(); }
    void tok_stdu16string_QChar_data() { tok_data(false); }
    void tok_stdu16string_QChar() { tok_impl<std::u16string, QChar>(); }
    void tok_stdu16string_char16_t_data() { tok_data(false); }
    void tok_stdu16string_char16_t() { tok_impl<std::u16string, char16_t>(); }

private:
    void mid_data();
    template <typename String> void mid_impl();

    void left_data();
    template <typename String> void left_impl();

    void right_data();
    template <typename String> void right_impl();

    void sliced_data();
    template <typename String> void sliced_impl();

    void first_data();
    template <typename String> void first_impl();

    void last_data();
    template <typename String> void last_impl();

    void chop_data();
    template <typename String> void chop_impl();

private Q_SLOTS:

    void mid_QString_data() { mid_data(); }
    void mid_QString() { mid_impl<QString>(); }
    void mid_QStringView_data() { mid_data(); }
    void mid_QStringView() { mid_impl<QStringView>(); }
    void mid_QUtf8StringView_data() { mid_data(); }
    void mid_QUtf8StringView() { mid_impl<QUtf8StringView>(); }
    void mid_QLatin1String_data() { mid_data(); }
    void mid_QLatin1String() { mid_impl<QLatin1String>(); }
    void mid_QByteArray_data() { mid_data(); }
    void mid_QByteArray() { mid_impl<QByteArray>(); }

    void left_QString_data() { left_data(); }
    void left_QString() { left_impl<QString>(); }
    void left_QStringView_data() { left_data(); }
    void left_QStringView() { left_impl<QStringView>(); }
    void left_QUtf8StringView_data() { left_data(); }
    void left_QUtf8StringView() { left_impl<QUtf8StringView>(); }
    void left_QLatin1String_data() { left_data(); }
    void left_QLatin1String() { left_impl<QLatin1String>(); }
    void left_QByteArray_data();
    void left_QByteArray() { left_impl<QByteArray>(); }

    void right_QString_data() { right_data(); }
    void right_QString() { right_impl<QString>(); }
    void right_QStringView_data() { right_data(); }
    void right_QStringView() { right_impl<QStringView>(); }
    void right_QUtf8StringView_data() { right_data(); }
    void right_QUtf8StringView() { right_impl<QUtf8StringView>(); }
    void right_QLatin1String_data() { right_data(); }
    void right_QLatin1String() { right_impl<QLatin1String>(); }
    void right_QByteArray_data();
    void right_QByteArray() { right_impl<QByteArray>(); }

    void sliced_QString_data() { sliced_data(); }
    void sliced_QString() { sliced_impl<QString>(); }
    void sliced_QStringView_data() { sliced_data(); }
    void sliced_QStringView() { sliced_impl<QStringView>(); }
    void sliced_QLatin1String_data() { sliced_data(); }
    void sliced_QLatin1String() { sliced_impl<QLatin1String>(); }
    void sliced_QUtf8StringView_data() { sliced_data(); }
    void sliced_QUtf8StringView() { sliced_impl<QUtf8StringView>(); }
    void sliced_QByteArray_data() { sliced_data(); }
    void sliced_QByteArray() { sliced_impl<QByteArray>(); }

    void first_truncate_QString_data() { first_data(); }
    void first_truncate_QString() { first_impl<QString>(); }
    void first_truncate_QStringView_data() { first_data(); }
    void first_truncate_QStringView() { first_impl<QStringView>(); }
    void first_truncate_QLatin1String_data() { first_data(); }
    void first_truncate_QLatin1String() { first_impl<QLatin1String>(); }
    void first_truncate_QUtf8StringView_data() { first_data(); }
    void first_truncate_QUtf8StringView() { first_impl<QUtf8StringView>(); }
    void first_truncate_QByteArray_data() { first_data(); }
    void first_truncate_QByteArray() { first_impl<QByteArray>(); }

    void last_QString_data() { last_data(); }
    void last_QString() { last_impl<QString>(); }
    void last_QStringView_data() { last_data(); }
    void last_QStringView() { last_impl<QStringView>(); }
    void last_QLatin1String_data() { last_data(); }
    void last_QLatin1String() { last_impl<QLatin1String>(); }
    void last_QUtf8StringView_data() { last_data(); }
    void last_QUtf8StringView() { last_impl<QUtf8StringView>(); }
    void last_QByteArray_data() { last_data(); }
    void last_QByteArray() { last_impl<QByteArray>(); }

    void chop_QString_data() { chop_data(); }
    void chop_QString() { chop_impl<QString>(); }
    void chop_QStringView_data() { chop_data(); }
    void chop_QStringView() { chop_impl<QStringView>(); }
    void chop_QUtf8StringView_data() { chop_data(); }
    void chop_QUtf8StringView() { chop_impl<QUtf8StringView>(); }
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
    void trim_trimmed_QStringView_data() { trimmed_data(); }
    void trim_trimmed_QStringView() { trimmed_impl<QStringView>(); }
    void trim_trimmed_QLatin1String_data() { trimmed_data(); }
    void trim_trimmed_QLatin1String() { trimmed_impl<QLatin1String>(); }
    void trim_trimmed_QByteArray_data() { trimmed_data(); }
    void trim_trimmed_QByteArray() { /*trimmed_impl<QByteArray>();*/ } // TODO

private:
    void toNumber_data();
    template <typename String> void toNumber_impl();

private Q_SLOTS:
    void toNumber_QString_data() { toNumber_data(); }
    void toNumber_QString() { toNumber_impl<QString>(); }
    void toNumber_QStringView_data() { toNumber_data(); }
    void toNumber_QStringView() { toNumber_impl<QStringView>(); }
    void toNumber_QByteArray_data() { toNumber_data(); }
    void toNumber_QByteArray() { toNumber_impl<QByteArray>(); }

private:
    void count_data();
    template <typename String> void count_impl();

private Q_SLOTS:
    void count_QString_data() { count_data(); }
    void count_QString() { count_impl<QString>(); }
    void count_QStringView_data() { count_data(); }
    void count_QStringView() { count_impl<QStringView>(); }
    void count_QByteArray_data() { count_data(); }
    void count_QByteArray() { count_impl<QByteArray>(); }

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
    void toLocal8Bit_QStringView_data() { toLocal8Bit_data(); }
    void toLocal8Bit_QStringView() { toLocal8Bit_impl<QStringView>(); }

    void toLatin1_QString_data() { toLatin1_data(); }
    void toLatin1_QString() { toLatin1_impl<QString>(); }
    void toLatin1_QStringView_data() { toLatin1_data(); }
    void toLatin1_QStringView() { toLatin1_impl<QStringView>(); }

    void toUtf8_QString_data() { toUtf8_data(); }
    void toUtf8_QString() { toUtf8_impl<QString>(); }
    void toUtf8_QStringView_data() { toUtf8_data(); }
    void toUtf8_QStringView() { toUtf8_impl<QStringView>(); }

    void toUcs4_QString_data() { toUcs4_data(); }
    void toUcs4_QString() { toUcs4_impl<QString>(); }
    void toUcs4_QStringView_data() { toUcs4_data(); }
    void toUcs4_QStringView() { toUcs4_impl<QStringView>(); }

private:
    template <typename Haystack, typename Needle> void indexOf_impl() const;
    void indexOf_data(bool rhsHasVariableLength = true);

private Q_SLOTS:
    // test all combinations of {QString, QLatin1String, QStringView} x {QString, QLatin1String, QStringView, QChar, char16_t}:
    void indexOf_QString_QString_data() { indexOf_data(); }
    void indexOf_QString_QString() { indexOf_impl<QString, QString>(); }
    void indexOf_QString_QLatin1String_data() { indexOf_data(); }
    void indexOf_QString_QLatin1String() { indexOf_impl<QString, QLatin1String>(); }
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
    void indexOf_QLatin1String_QStringView_data() { indexOf_data(); }
    void indexOf_QLatin1String_QStringView() { indexOf_impl<QLatin1String, QStringView>(); }
    void indexOf_QLatin1String_QChar_data() { indexOf_data(false); }
    void indexOf_QLatin1String_QChar() { indexOf_impl<QLatin1String, QChar>(); }
    void indexOf_QLatin1String_char16_t_data() { indexOf_data(false); }
    void indexOf_QLatin1String_char16_t() { indexOf_impl<QLatin1String, char16_t>(); }

    void indexOf_QStringView_QString_data() { indexOf_data(); }
    void indexOf_QStringView_QString() { indexOf_impl<QStringView, QString>(); }
    void indexOf_QStringView_QLatin1String_data() { indexOf_data(); }
    void indexOf_QStringView_QLatin1String() { indexOf_impl<QStringView, QLatin1String>(); }
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
    // test all combinations of {QString, QLatin1String, QStringView} x {QString, QLatin1String, QStringView, QChar, char16_t}:
    void contains_QString_QString_data() { contains_data(); }
    void contains_QString_QString() { contains_impl<QString, QString>(); }
    void contains_QString_QLatin1String_data() { contains_data(); }
    void contains_QString_QLatin1String() { contains_impl<QString, QLatin1String>(); }
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
    void contains_QLatin1String_QStringView_data() { contains_data(); }
    void contains_QLatin1String_QStringView() { contains_impl<QLatin1String, QStringView>(); }
    void contains_QLatin1String_QChar_data() { contains_data(false); }
    void contains_QLatin1String_QChar() { contains_impl<QLatin1String, QChar>(); }
    void contains_QLatin1String_char16_t_data() { contains_data(false); }
    void contains_QLatin1String_char16_t() { contains_impl<QLatin1String, char16_t>(); }

    void contains_QStringView_QString_data() { contains_data(); }
    void contains_QStringView_QString() { contains_impl<QStringView, QString>(); }
    void contains_QStringView_QLatin1String_data() { contains_data(); }
    void contains_QStringView_QLatin1String() { contains_impl<QStringView, QLatin1String>(); }
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
    // test all combinations of {QString, QLatin1String, QStringView} x {QString, QLatin1String, QStringView, QChar, char16_t}:
    void lastIndexOf_QString_QString_data() { lastIndexOf_data(); }
    void lastIndexOf_QString_QString() { lastIndexOf_impl<QString, QString>(); }
    void lastIndexOf_QString_QLatin1String_data() { lastIndexOf_data(); }
    void lastIndexOf_QString_QLatin1String() { lastIndexOf_impl<QString, QLatin1String>(); }
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
    void lastIndexOf_QLatin1String_QStringView_data() { lastIndexOf_data(); }
    void lastIndexOf_QLatin1String_QStringView() { lastIndexOf_impl<QLatin1String, QStringView>(); }
    void lastIndexOf_QLatin1String_QChar_data() { lastIndexOf_data(false); }
    void lastIndexOf_QLatin1String_QChar() { lastIndexOf_impl<QLatin1String, QChar>(); }
    void lastIndexOf_QLatin1String_char16_t_data() { lastIndexOf_data(false); }
    void lastIndexOf_QLatin1String_char16_t() { lastIndexOf_impl<QLatin1String, char16_t>(); }

    void lastIndexOf_QStringView_QString_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringView_QString() { lastIndexOf_impl<QStringView, QString>(); }
    void lastIndexOf_QStringView_QLatin1String_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringView_QLatin1String() { lastIndexOf_impl<QStringView, QLatin1String>(); }
    void lastIndexOf_QStringView_QStringView_data() { lastIndexOf_data(); }
    void lastIndexOf_QStringView_QStringView() { lastIndexOf_impl<QStringView, QStringView>(); }
    void lastIndexOf_QStringView_QChar_data() { lastIndexOf_data(false); }
    void lastIndexOf_QStringView_QChar() { lastIndexOf_impl<QStringView, QChar>(); }
    void lastIndexOf_QStringView_char16_t_data() { lastIndexOf_data(false); }
    void lastIndexOf_QStringView_char16_t() { lastIndexOf_impl<QStringView, char16_t>(); }

private:
    void indexOf_contains_lastIndexOf_count_regexp_data();
    template <typename String> void indexOf_contains_lastIndexOf_count_regexp_impl() const;

private Q_SLOTS:
    void indexOf_regexp_QString_data() { indexOf_contains_lastIndexOf_count_regexp_data(); }
    void indexOf_regexp_QString() { indexOf_contains_lastIndexOf_count_regexp_impl<QString>(); }
    void indexOf_regexp_QStringView_data() { indexOf_contains_lastIndexOf_count_regexp_data(); }
    void indexOf_regexp_QStringView() { indexOf_contains_lastIndexOf_count_regexp_impl<QStringView>(); }
};

void tst_QStringApiSymmetry::compare_data(bool hasConceptOfNullAndEmpty)
{
    QTest::addColumn<QStringView>("lhsUnicode");
    QTest::addColumn<QLatin1String>("lhsLatin1");
    QTest::addColumn<QStringView>("rhsUnicode");
    QTest::addColumn<QLatin1String>("rhsLatin1");
    QTest::addColumn<int>("caseSensitiveCompareResult");
    QTest::addColumn<int>("caseInsensitiveCompareResult");

    if (hasConceptOfNullAndEmpty) {
        QTest::newRow("null <> null") << QStringView() << QLatin1String()
                                      << QStringView() << QLatin1String()
                                      << 0 << 0;
        static const QString empty("");
        QTest::newRow("null <> empty") << QStringView() << QLatin1String()
                                       << QStringView(empty) << QLatin1String("")
                                       << 0 << 0;
        QTest::newRow("empty <> null") << QStringView(empty) << QLatin1String("")
                                       << QStringView() << QLatin1String()
                                       << 0 << 0;
    }

#define ROW(lhs, rhs, caseless) \
    do { \
        static const QString pinned[] = { \
            QString(QLatin1String(lhs)), \
            QString(QLatin1String(rhs)), \
        }; \
        QTest::newRow(qUtf8Printable(QLatin1String("'" lhs "' <> '" rhs "': "))) \
            << QStringView(pinned[0]) << QLatin1String(lhs) \
            << QStringView(pinned[1]) << QLatin1String(rhs) \
            << sign(qstrcmp(lhs, rhs)) << caseless; \
    } while (false)
#define ASCIIROW(lhs, rhs) ROW(lhs, rhs, sign(qstricmp(lhs, rhs)))
    ASCIIROW("", "0");
    ASCIIROW("0", "");
    ASCIIROW("0", "1");
    ASCIIROW("0", "0");
    ASCIIROW("10", "0");
    ASCIIROW("01", "1");
    ASCIIROW("e", "e");
    ASCIIROW("e", "E");
    ROW("\xE4", "\xE4", 0); // ä <> ä
    ROW("\xE4", "\xC4", 0); // ä <> Ä
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

template <class Str> Str  make(const QString &s);
template <> QString       make(const QString &s)   { return s; }
template <> QStringView   make(const QString &s)   { return s; }

template <class Str> Str  make(QStringView sf, QLatin1String l1, const QByteArray &u8);

#define MAKE(Which) \
    template <> Which make([[maybe_unused]] QStringView sv, \
                           [[maybe_unused]] QLatin1String l1, \
                           [[maybe_unused]] const QByteArray &u8) \
    /*end*/
MAKE(QChar)                  { return sv.isEmpty() ? QChar() : sv.at(0); }
MAKE(char16_t)               { return sv.isEmpty() ? char16_t() : char16_t{sv.at(0).unicode()}; }
MAKE(QString)                { return sv.toString(); }
MAKE(QStringView)            { return sv; }
MAKE(QLatin1String)          { return l1; }
MAKE(QByteArray)             { return u8; }
MAKE(const char *)           { return u8.data(); }
MAKE(const char16_t *)       { return sv.utf16(); } // assumes `sv` doesn't represent a substring
MAKE(std::u16string)         { return sv.toString().toStdU16String(); }
MAKE(QUtf8StringView)        { return u8; }
MAKE(QAnyStringViewUsingL1)  { return {QAnyStringView{l1}}; }
MAKE(QAnyStringViewUsingU8)  { return {QAnyStringView{u8}}; }
MAKE(QAnyStringViewUsingU16) { return {QAnyStringView{sv}}; }
#undef MAKE

template <typename> struct is_utf8_encoded              : std::false_type {};
template <>         struct is_utf8_encoded<const char*> : std::true_type {};
template <>         struct is_utf8_encoded<QByteArray>  : std::true_type {};
template <>         struct is_utf8_encoded<QUtf8StringView> : std::true_type {};

template <typename> struct is_latin1_encoded                : std::false_type {};
template <>         struct is_latin1_encoded<QLatin1String> : std::true_type {};

template <typename LHS, typename RHS>
constexpr bool has_nothrow_member_compare_v = is_utf8_encoded<LHS>::value == is_utf8_encoded<RHS>::value;

template <typename LHS, typename RHS>
void tst_QStringApiSymmetry::compare_impl() const
{
    QFETCH(QStringView, lhsUnicode);
    QFETCH(QLatin1String, lhsLatin1);
    QFETCH(QStringView, rhsUnicode);
    QFETCH(QLatin1String, rhsLatin1);
    QFETCH(int, caseSensitiveCompareResult);
    QFETCH(const int, caseInsensitiveCompareResult);

    const auto lhsU8 = lhsUnicode.toUtf8();
    const auto rhsU8 = rhsUnicode.toUtf8();

    const auto lhs = make<LHS>(lhsUnicode, lhsLatin1, lhsU8);
    const auto rhs = make<RHS>(rhsUnicode, rhsLatin1, rhsU8);

    auto icResult = sign(
            QAnyStringView::compare(QAnyStringView(lhs), QAnyStringView(rhs), Qt::CaseInsensitive));
    QCOMPARE(icResult, caseInsensitiveCompareResult);

    auto scResult = sign(
            QAnyStringView::compare(QAnyStringView(lhs), QAnyStringView(rhs), Qt::CaseSensitive));
    QCOMPARE(scResult, caseSensitiveCompareResult);

#define CHECK(op) \
    do { \
        /* comment out the noexcept check for now, as we first need to sort all the overloads anew */ \
        if (false) { \
            if constexpr (!is_fake_comparator_v<LHS, RHS>) \
                QVERIFY(noexcept(lhs op rhs)); \
            } \
        if (caseSensitiveCompareResult op 0) \
            QVERIFY(lhs op rhs); \
        else \
            QVERIFY(!(lhs op rhs)); \
    } while (false)

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
    QFETCH(QStringView, lhsUnicode);
    QFETCH(QLatin1String, lhsLatin1);
    QFETCH(QStringView, rhsUnicode);
    QFETCH(QLatin1String, rhsLatin1);
    QFETCH(const int, caseSensitiveCompareResult);
    QFETCH(const int, caseInsensitiveCompareResult);

    const auto lhsU8 = lhsUnicode.toUtf8();
    const auto rhsU8 = rhsUnicode.toUtf8();

    const auto lhs = make<LHS>(lhsUnicode, lhsLatin1, lhsU8);
    const auto rhs = make<RHS>(rhsUnicode, rhsLatin1, rhsU8);

    if constexpr (has_nothrow_member_compare_v<LHS, RHS>)
        QVERIFY(noexcept(lhs.compare(rhs, Qt::CaseSensitive)));

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

void tst_QStringApiSymmetry::localeAwareCompare_data()
{
    QTest::addColumn<QByteArray>("locale");
    QTest::addColumn<QString>("s1");
    QTest::addColumn<QString>("s2");
    QTest::addColumn<int>("result");

#if defined(Q_OS_WIN) || defined(Q_OS_DARWIN) || QT_CONFIG(icu)
    // Although the test sets LC_ALL (and adds a suffix to wanted) test
    // LC_COLLATE because setlocale(LC_ALL, nullptr) encodes the whole locale,
    // it's not simply the value of LC_ALL. We need our own copy of the reported
    // value, as later setlocale() calls may stomp the value:
    const QByteArray current(setlocale(LC_COLLATE, nullptr));
    const auto canTest = [current](const char *wanted) {
#  if QT_CONFIG(icu)
        // ICU will correctly use en when relevant environment variables are set
        // to en.UTF-8, but setlocale() reports that as C, whose sort order is
        // simpler. Only believe we can run C tests if the environment variables
        // (which, conveniently, QLocale::system()'s Unix backend uses) say it
        // really is. Conversely, don't reject "en_US" just because setlocale()
        // misdescribes it.
        if (current == "C") {
            const QString sys = QLocale::system().name();
            if (wanted == current ? sys == u"C" : sys.startsWith(wanted))
                return true;
            qDebug("Skipping %s test-cases as we can only test in locale %s (seen as C)",
                   wanted, sys.toUtf8().constData());
            return false;
        }
#  endif
        if (current.startsWith(wanted))
            return true;
#  ifdef Q_OS_WIN
        // Unhelpfully, MS doesn't deign to use the usual format of locale tags,
        // but expands the tag names to full names (in English):
        const auto want = QLocale(QLatin1String(wanted));
        if (current.startsWith(
                QString(QLocale::languageToString(want.language()) + QChar('_')
                        + QLocale::territoryToString(want.territory())).toLocal8Bit())) {
            return true;
        }
#  endif
        qDebug("Skipping %s test-cases as we can only test in locale %s (seen as %s)",
               wanted, QLocale::system().name().toUtf8().constData(), current.data());
        return false;
    };
#else
    // Otherwise, trust that setlocale() reconfigures QString::localeAwareCompare():
    const auto canTest = [](const char *) { return true; };
#endif
    // Update tailpiece's max-value for this if you add a new locale group
    int countGroups = 0;

    // Compare decomposed and composed form
    if (canTest("en_US")) {
        // From ES6 test262 test suite (built-ins/String/prototype/localeCompare/15.5.4.9_CE.js).
        // The test cases boil down to code like this:
        //     console.log("\u1111\u1171\u11B6".localeCompare("\ud4db")

        // example from Unicode 5.0, section 3.7, definition D70
        QTest::newRow("normalize1")
            << QByteArray("en_US")
            << QString::fromUtf8("o\xCC\x88")
            << QString::fromUtf8("\xC3\xB6") << 0;
        // examples from Unicode 5.0, chapter 3.11
        QTest::newRow("normalize2")
            << QByteArray("en_US")
            << QString::fromUtf8("\xC3\xA4\xCC\xA3")
            << QString::fromUtf8("a\xCC\xA3\xCC\x88") << 0;
        QTest::newRow("normalize3")
            << QByteArray("en_US")
            << QString::fromUtf8("a\xCC\x88\xCC\xA3")
            << QString::fromUtf8("a\xCC\xA3\xCC\x88") << 0;
        QTest::newRow("normalize4")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE1\xBA\xA1\xCC\x88")
            << QString::fromUtf8("a\xCC\xA3\xCC\x88") << 0;
        QTest::newRow("normalize5")
            << QByteArray("en_US")
            << QString::fromUtf8("\xC3\xA4\xCC\x86")
            << QString::fromUtf8("a\xCC\x88\xCC\x86") << 0;
        QTest::newRow("normalize6")
            << QByteArray("en_US")
            << QString::fromUtf8("\xC4\x83\xCC\x88")
            << QString::fromUtf8("a\xCC\x86\xCC\x88") << 0;
        // example from Unicode 5.0, chapter 3.12
        QTest::newRow("normalize7")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE1\x84\x91\xE1\x85\xB1\xE1\x86\xB6")
            << QString::fromUtf8("\xED\x93\x9B") << 0;
        // examples from UTS 10, Unicode Collation Algorithm
        QTest::newRow("normalize8")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE2\x84\xAB")
            << QString::fromUtf8("\xC3\x85") << 0;
        QTest::newRow("normalize9")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE2\x84\xAB")
            << QString::fromUtf8("A\xCC\x8A") << 0;
        QTest::newRow("normalize10")
            << QByteArray("en_US")
            << QString::fromUtf8("x\xCC\x9B\xCC\xA3")
            << QString::fromUtf8("x\xCC\xA3\xCC\x9B") << 0;
        QTest::newRow("normalize11")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE1\xBB\xB1")
            << QString::fromUtf8("\xE1\xBB\xA5\xCC\x9B") << 0;
        QTest::newRow("normalize12")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE1\xBB\xB1")
            << QString::fromUtf8("u\xCC\x9B\xCC\xA3") << 0;
        QTest::newRow("normalize13")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE1\xBB\xB1")
            << QString::fromUtf8("\xC6\xB0\xCC\xA3") << 0;
        QTest::newRow("normalize14")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE1\xBB\xB1")
            << QString::fromUtf8("u\xCC\xA3\xCC\x9B") << 0;
        // examples from UAX 15, Unicode Normalization Forms
        QTest::newRow("normalize15")
            << QByteArray("en_US")
            << QString::fromUtf8("\xC3\x87")
            << QString::fromUtf8("C\xCC\xA7") << 0;
        QTest::newRow("normalize16")
            << QByteArray("en_US")
            << QString::fromUtf8("q\xCC\x87\xCC\xA3")
            << QString::fromUtf8("q\xCC\xA3\xCC\x87") << 0;
        QTest::newRow("normalize17")
            << QByteArray("en_US")
            << QString::fromUtf8("\xEA\xB0\x80")
            << QString::fromUtf8("\xE1\x84\x80\xE1\x85\xA1") << 0;
        QTest::newRow("normalize18")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE2\x84\xAB")
            << QString::fromUtf8("A\xCC\x8A") << 0;
        QTest::newRow("normalize19")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE2\x84\xA6")
            << QString::fromUtf8("\xCE\xA9") << 0;
        QTest::newRow("normalize20")
            << QByteArray("en_US")
            << QString::fromUtf8("\xC3\x85")
            << QString::fromUtf8("A\xCC\x8A") << 0;
        QTest::newRow("normalize21")
            << QByteArray("en_US")
            << QString::fromUtf8("\xC3\xB4")
            << QString::fromUtf8("o\xCC\x82") << 0;
        QTest::newRow("normalize22")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE1\xB9\xA9")
            << QString::fromUtf8("s\xCC\xA3\xCC\x87") << 0;
        QTest::newRow("normalize23")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE1\xB8\x8B\xCC\xA3")
            << QString::fromUtf8("d\xCC\xA3\xCC\x87") << 0;
        QTest::newRow("normalize24")
            << QByteArray("en_US")
            << QString::fromUtf8("\xE1\xB8\x8B\xCC\xA3")
            << QString::fromUtf8("\xE1\xB8\x8D\xCC\x87") << 0;
        QTest::newRow("normalize25")
            << QByteArray("en_US")
            << QString::fromUtf8("q\xCC\x87\xCC\xA3")
            << QString::fromUtf8("q\xCC\xA3\xCC\x87") << 0;

        QTest::newRow("en@5.gt.4") << QByteArray("en_US") << QString("5") << QString("4") << 1;
        QTest::newRow("en@4.lt.6") << QByteArray("en_US") << QString("4") << QString("6") << -1;
        QTest::newRow("en@5.l6.6") << QByteArray("en_US") << QString("5") << QString("6") << -1;

        QTest::newRow("en@null.eq.null") << QByteArray("en_US") << QString() << QString() << 0;
        QTest::newRow("en@empty.eq.null") << QByteArray("en_US") << QString("") << QString() << 0;
        QTest::newRow("en@null.lt.non-empty") << QByteArray("en_US") << QString()
                                              << QString("test") << -1;
        QTest::newRow("en@empty.lt.non-empty") << QByteArray("en_US") << QString("")
                                               << QString("test") << -1;

        countGroups++;
    }

    /*
        The C locale performs simple code-point number comparison of successive
        characters until if finds a difference. Contrast with Swedish below,
        particularly the a-umlaut vs a-ring comparison.
    */
    if (canTest("C")) {
        QTest::newRow("C@auml.lt.aring")
            << QByteArray("C")
            << QString::fromLatin1("\xe4") // &auml;
            << QString::fromLatin1("\xe5") << -1;
        QTest::newRow("C@auml.lt.ouml")
            << QByteArray("C")
            << QString::fromLatin1("\xe4")
            << QString::fromLatin1("\xf6") << -1; // &ouml;
        QTest::newRow("C.aring.lt.ouml")
            << QByteArray("C")
            << QString::fromLatin1("\xe5") // &aring;
            << QString::fromLatin1("\xf6") << -1;

        countGroups++;
    }

    /*
        In Swedish, a with ring above (E5) comes before a with
        diaresis (E4), which comes before o diaresis (F6), which
        all come after z.
    */
    if (canTest("sv_SE")) {
        QTest::newRow("swede@aring.lt.auml")
            << QByteArray("sv_SE")
            << QString::fromLatin1("\xe5")
            << QString::fromLatin1("\xe4") << -1;
        QTest::newRow("swede@auml.lt.ouml")
            << QByteArray("sv_SE")
            << QString::fromLatin1("\xe4")
            << QString::fromLatin1("\xf6") << -1;
        QTest::newRow("swede.aring.lt.ouml")
            << QByteArray("sv_SE")
            << QString::fromLatin1("\xe5")
            << QString::fromLatin1("\xf6") << -1;
        QTest::newRow("swede.z.lt.aring")
            << QByteArray("sv_SE")
            << QString::fromLatin1("z")
            << QString::fromLatin1("\xe5") << -1;

        countGroups++;
    }

    /*
        In Norwegian, ae (E6) comes before o with stroke (D8), which
        comes before a with ring above (E5).
    */
    if (canTest("nb_NO")) {
        QTest::newRow("norsk.ae.lt.oslash")
            << QByteArray("nb_NO")
            << QString::fromLatin1("\xe6")
            << QString::fromLatin1("\xd8") << -1;
        QTest::newRow("norsk.oslash.lt.aring")
            << QByteArray("nb_NO")
            << QString::fromLatin1("\xd8")
            << QString::fromLatin1("\xe5") << -1;
        QTest::newRow("norsk.ae.lt.aring")
            << QByteArray("nb_NO")
            << QString::fromLatin1("\xe6")
            << QString::fromLatin1("\xe5") << -1;

        countGroups++;
    }

    /*
        In German, z comes *after* a with diaresis (E4),
        which comes before o diaresis (F6).
    */
    if (canTest("de_DE")) {
        QTest::newRow("german.z.gt.auml")
            << QByteArray("de_DE")
            << QString::fromLatin1("z")
            << QString::fromLatin1("\xe4") << 1;
        QTest::newRow("german.auml.lt.ouml")
            << QByteArray("de_DE")
            << QString::fromLatin1("\xe4")
            << QString::fromLatin1("\xf6") << -1;
        QTest::newRow("german.z.gt.ouml")
            << QByteArray("de_DE")
            << QString::fromLatin1("z")
            << QString::fromLatin1("\xf6") << 1;

        countGroups++;
    }
    // Tell developers how to get all the results (bot don't spam Coin logs):
    if (countGroups < 5 && !qgetenv("QTEST_ENVIRONMENT").split(' ').contains("ci")) {
        qDebug(R"(On platforms where this test cannot control the locale used by
QString::localeAwareCompare(), it only runs test-cases for the locale in use.
To test thoroughly, it is necessary to run this test repeatedly with each of
C.UTF-8, en_US.UTF-8, sv_SE.UTF-8, nb_NO.UTF-8 and de_DE.UTF-8 as the system
locale.)");
    }
    if (!countGroups)
        QSKIP("No data for available locale");
}

template<typename LHS, typename RHS>
void tst_QStringApiSymmetry::localeAwareCompare_impl()
{
    QFETCH(QByteArray, locale);
    QFETCH(const QString, s1);
    QFETCH(const QString, s2);
    QFETCH(int, result);
    locale += ".UTF-8"; // So we don't have to repeat it on every data row !

    const QTestLocaleChange::TransientLocale tested(LC_ALL, locale.constData());
    if (!tested.isValid())
        QSKIP(QByteArray("Test needs locale " + locale + " installed on this machine").constData());

    const auto lhs = make<LHS>(s1);
    const auto rhs = make<RHS>(s2);

    // qDebug() << s1.toUtf8().toHex(' ') << "as" << result << "to" << s2.toUtf8().toHex(' ');
    QCOMPARE(sign(QString::localeAwareCompare(lhs, rhs)), result);
}

template<typename LHS, typename RHS>
void tst_QStringApiSymmetry::member_localeAwareCompare_impl()
{
    QFETCH(QByteArray, locale);
    QFETCH(const QString, s1);
    QFETCH(const QString, s2);
    QFETCH(int, result);
    locale += ".UTF-8"; // So we don't have to repeat it on every data row !

    const QTestLocaleChange::TransientLocale tested(LC_ALL, locale.constData());
    if (!tested.isValid())
        QSKIP(QByteArray("Test needs locale " + locale + " installed on this machine").constData());

    const auto lhs = make<LHS>(s1);
    const auto rhs = make<RHS>(s2);

    // qDebug() << s1.toUtf8().toHex(' ') << "as" << result << "to" << s2.toUtf8().toHex(' ');
    QCOMPARE(sign(lhs.localeAwareCompare(rhs)), result);
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
    QTest::addColumn<QStringView>("haystackU16");
    QTest::addColumn<QLatin1String>("haystackL1");
    QTest::addColumn<QStringView>("needleU16");
    QTest::addColumn<QLatin1String>("needleL1");
    QTest::addColumn<bool>("resultCS");
    QTest::addColumn<bool>("resultCIS");

    if (rhsHasVariableLength) {
        QTest::addRow("null ~= ^null")   << QStringView() << QLatin1String()
                                         << QStringView() << QLatin1String() << true << true;
        QTest::addRow("empty ~= ^null")  << QStringView(empty) << QLatin1String("")
                                         << QStringView() << QLatin1String() << true << true;
        QTest::addRow("a ~= ^null")      << QStringView(a) << QLatin1String("a")
                                         << QStringView() << QLatin1String() << true << true;
        QTest::addRow("null ~= ^empty")  << QStringView() << QLatin1String()
                                         << QStringView(empty) << QLatin1String("") << false << false;
        QTest::addRow("a ~= ^empty")     << QStringView(a) << QLatin1String("a")
                                         << QStringView(empty) << QLatin1String("") << true << true;
        QTest::addRow("empty ~= ^empty") << QStringView(empty) << QLatin1String("")
                                         << QStringView(empty) << QLatin1String("") << true << true;
    }
    QTest::addRow("null ~= ^a")      << QStringView() << QLatin1String()
                                     << QStringView(a) << QLatin1String("a") << false << false;
    QTest::addRow("empty ~= ^a")     << QStringView(empty) << QLatin1String("")
                                     << QStringView(a) << QLatin1String("a") << false << false;

#define ROW(h, n, cs, cis) \
    QTest::addRow("%s ~= ^%s", #h, #n) << QStringView(h) << QLatin1String(#h) \
                                       << QStringView(n) << QLatin1String(#n) \
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
    QFETCH(const QStringView, haystackU16);
    QFETCH(const QLatin1String, haystackL1);
    QFETCH(const QStringView, needleU16);
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
    QTest::addColumn<QStringView>("haystackU16");
    QTest::addColumn<QLatin1String>("haystackL1");
    QTest::addColumn<QStringView>("needleU16");
    QTest::addColumn<QLatin1String>("needleL1");
    QTest::addColumn<bool>("resultCS");
    QTest::addColumn<bool>("resultCIS");

    if (rhsHasVariableLength) {
        QTest::addRow("null ~= null$")   << QStringView() << QLatin1String()
                                         << QStringView() << QLatin1String() << true << true;
        QTest::addRow("empty ~= null$")  << QStringView(empty) << QLatin1String("")
                                         << QStringView() << QLatin1String() << true << true;
        QTest::addRow("a ~= null$")      << QStringView(a) << QLatin1String("a")
                                         << QStringView() << QLatin1String() << true << true;
        QTest::addRow("null ~= empty$")  << QStringView() << QLatin1String()
                                         << QStringView(empty) << QLatin1String("") << false << false;
        QTest::addRow("a ~= empty$")     << QStringView(a) << QLatin1String("a")
                                         << QStringView(empty) << QLatin1String("") << true << true;
        QTest::addRow("empty ~= empty$") << QStringView(empty) << QLatin1String("")
                                         << QStringView(empty) << QLatin1String("") << true << true;
    }
    QTest::addRow("null ~= a$")      << QStringView() << QLatin1String()
                                     << QStringView(a) << QLatin1String("a") << false << false;
    QTest::addRow("empty ~= a$")     << QStringView(empty) << QLatin1String("")
                                     << QStringView(a) << QLatin1String("a") << false << false;

#define ROW(h, n, cs, cis) \
    QTest::addRow("%s ~= %s$", #h, #n) << QStringView(h) << QLatin1String(#h) \
                                       << QStringView(n) << QLatin1String(#n) \
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
    QFETCH(const QStringView, haystackU16);
    QFETCH(const QLatin1String, haystackL1);
    QFETCH(const QStringView, needleU16);
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
    QTest::addColumn<QStringView>("haystackU16");
    QTest::addColumn<QLatin1String>("haystackL1");
    QTest::addColumn<QStringView>("needleU16");
    QTest::addColumn<QLatin1String>("needleL1");
    QTest::addColumn<QStringList>("resultCS");
    QTest::addColumn<QStringList>("resultCIS");

    if (rhsHasVariableLength) {
        QTest::addRow("null ~= null$")   << QStringView{} << QLatin1String{}
                                         << QStringView{} << QLatin1String{}
                                         << QStringList{{}, {}} << QStringList{{}, {}};
        QTest::addRow("empty ~= null$")  << QStringView{empty} << QLatin1String("")
                                         << QStringView{} << QLatin1String{}
                                         << QStringList{empty, empty} << QStringList{empty, empty};
        QTest::addRow("a ~= null$")      << QStringView{a} << QLatin1String{"a"}
                                         << QStringView{} << QLatin1String{}
                                         << QStringList{empty, a, empty} << QStringList{empty, a, empty};
        QTest::addRow("null ~= empty$")  << QStringView{} << QLatin1String{}
                                         << QStringView{empty} << QLatin1String{""}
                                         << QStringList{{}, {}} << QStringList{{}, {}};
        QTest::addRow("a ~= empty$")     << QStringView{a} << QLatin1String{"a"}
                                         << QStringView{empty} << QLatin1String{""}
                                         << QStringList{empty, a, empty} << QStringList{empty, a, empty};
        QTest::addRow("empty ~= empty$") << QStringView{empty} << QLatin1String{""}
                                         << QStringView{empty} << QLatin1String{""}
                                         << QStringList{empty, empty} << QStringList{empty, empty};
    }
    QTest::addRow("null ~= a$")      << QStringView{} << QLatin1String{}
                                     << QStringView{a} << QLatin1String{"a"}
                                     << QStringList{{}} << QStringList{{}};
    QTest::addRow("empty ~= a$")     << QStringView{empty} << QLatin1String{""}
                                     << QStringView{a} << QLatin1String{"a"}
                                     << QStringList{empty} << QStringList{empty};

#define ROW(h, n, cs, cis) \
    QTest::addRow("%s ~= %s$", #h, #n) << QStringView(h) << QLatin1String(#h) \
                                       << QStringView(n) << QLatin1String(#n) \
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

template <typename T> T deepCopied(T s) { return s; }
template <> QString deepCopied(QString s) { return detached(s); }
template <> QByteArray deepCopied(QByteArray s) { return detached(s); }

template <typename Haystack, typename Needle>
void tst_QStringApiSymmetry::split_impl() const
{
    QFETCH(const QStringView, haystackU16);
    QFETCH(const QLatin1String, haystackL1);
    QFETCH(const QStringView, needleU16);
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

void tst_QStringApiSymmetry::tok_data(bool rhsHasVariableLength)
{
    split_data(rhsHasVariableLength);
}

template <typename T> struct has_tokenize_method : std::false_type {};
template <> struct has_tokenize_method<QString> : std::true_type {};
template <> struct has_tokenize_method<QStringView> : std::true_type {};
template <> struct has_tokenize_method<QLatin1String> : std::true_type {};

template <typename T>
constexpr inline bool has_tokenize_method_v = has_tokenize_method<std::decay_t<T>>::value;

template <typename Haystack, typename Needle>
void tst_QStringApiSymmetry::tok_impl() const
{
    QFETCH(const QStringView, haystackU16);
    QFETCH(const QLatin1String, haystackL1);
    QFETCH(const QStringView, needleU16);
    QFETCH(const QLatin1String, needleL1);
    QFETCH(const QStringList, resultCS);
    QFETCH(const QStringList, resultCIS);

    const QStringList skippedResultCS = skipped(resultCS);
    const QStringList skippedResultCIS = skipped(resultCIS);

    const auto haystackU8 = haystackU16.toUtf8();
    const auto needleU8 = needleU16.toUtf8();

    const auto haystack = make<Haystack>(haystackU16, haystackL1, haystackU8);
    const auto needle = make<Needle>(needleU16, needleL1, needleU8);

    QCOMPARE(toQStringList(qTokenize(haystack, needle)), resultCS);
    QCOMPARE(toQStringList(qTokenize(haystack, needle, Qt::KeepEmptyParts, Qt::CaseSensitive)), resultCS);
    QCOMPARE(toQStringList(qTokenize(haystack, needle, Qt::CaseInsensitive, Qt::KeepEmptyParts)), resultCIS);
    QCOMPARE(toQStringList(qTokenize(haystack, needle, Qt::SkipEmptyParts, Qt::CaseSensitive)), skippedResultCS);
    QCOMPARE(toQStringList(qTokenize(haystack, needle, Qt::CaseInsensitive, Qt::SkipEmptyParts)), skippedResultCIS);

    {
        const auto tok = qTokenize(deepCopied(haystack), deepCopied(needle));
        // here, the temporaries returned from deepCopied() have already been destroyed,
        // yet `tok` should have kept a copy alive as needed:
        QCOMPARE(toQStringList(tok), resultCS);
    }

#ifdef __cpp_deduction_guides
    QCOMPARE(toQStringList(QStringTokenizer{haystack, needle}), resultCS);
    QCOMPARE(toQStringList(QStringTokenizer{haystack, needle, Qt::KeepEmptyParts, Qt::CaseSensitive}), resultCS);
    QCOMPARE(toQStringList(QStringTokenizer{haystack, needle, Qt::CaseInsensitive, Qt::KeepEmptyParts}), resultCIS);
    QCOMPARE(toQStringList(QStringTokenizer{haystack, needle, Qt::SkipEmptyParts, Qt::CaseSensitive}), skippedResultCS);
    QCOMPARE(toQStringList(QStringTokenizer{haystack, needle, Qt::CaseInsensitive, Qt::SkipEmptyParts}), skippedResultCIS);

    {
        const auto tok = QStringTokenizer{deepCopied(haystack), deepCopied(needle)};
        // here, the temporaries returned from deepCopied() have already been destroyed,
        // yet `tok` should have kept a copy alive as needed:
        QCOMPARE(toQStringList(tok), resultCS);
    }
#endif // __cpp_deduction_guides

    if constexpr (has_tokenize_method_v<Haystack>) {
        QCOMPARE(toQStringList(haystack.tokenize(needle)), resultCS);
        QCOMPARE(toQStringList(haystack.tokenize(needle, Qt::KeepEmptyParts, Qt::CaseSensitive)), resultCS);
        QCOMPARE(toQStringList(haystack.tokenize(needle, Qt::CaseInsensitive, Qt::KeepEmptyParts)), resultCIS);
        QCOMPARE(toQStringList(haystack.tokenize(needle, Qt::SkipEmptyParts, Qt::CaseSensitive)), skippedResultCS);
        QCOMPARE(toQStringList(haystack.tokenize(needle, Qt::CaseInsensitive, Qt::SkipEmptyParts)), skippedResultCIS);

        {
            const auto tok = deepCopied(haystack).tokenize(deepCopied(needle));
            // here, the temporaries returned from deepCopied() have already been destroyed,
            // yet `tok` should have kept a copy alive as needed:
            QCOMPARE(toQStringList(tok), resultCS);
        }
    }
}

void tst_QStringApiSymmetry::mid_data()
{
    sliced_data();

    // mid() has a wider contract compared to sliced(), so test those cases here:
#define ROW(base, p, n, r1, r2) \
    QTest::addRow("%s %d %d", #base, p, n) << QStringView(base) << QLatin1String(#base) << p << n << QAnyStringView(r1) << QAnyStringView(r2)

    ROW(a, -1, 0, a, null);
    ROW(a, -1, 2, a, a);
    ROW(a, -1, 3, a, a);
    ROW(a, 0, -1, a, a);
    ROW(a, 0, 2, a, a);
    ROW(a, -1, -1, a, a);
    ROW(a, 1, -1, empty, empty);
    ROW(a, 1, 1, empty, empty);
    ROW(a, 2, -1, null, null);
    ROW(a, 2, 1, null, null);

    ROW(abc, -1, -1, abc, abc);
    ROW(abc, -1, 0, abc, null);
    ROW(abc, -1, 2, abc, a);
    ROW(abc, -1, 3, abc, ab);
    ROW(abc, -1, 5, abc, abc);
    ROW(abc, 0, -1, abc, abc);
    ROW(abc, 0, 5, abc, abc);
    ROW(abc, -1, 1, abc, null);
    ROW(abc, -1, 2, abc, a);
    ROW(abc, -1, 4, abc, abc);
    ROW(abc, 1, -1, bc, bc);
    ROW(abc, 1, 1, bc, b);
    ROW(abc, 3, -1, empty, empty);
    ROW(abc, 3, 1, empty, empty);
#undef ROW
}

template <typename String>
void tst_QStringApiSymmetry::mid_impl()
{
    QFETCH(const QStringView, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, pos);
    QFETCH(const int, n);
    QFETCH(const QAnyStringView, result);
    QFETCH(const QAnyStringView, result2);

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
    first_data();

    // specific data testing out of bounds cases
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << QStringView(base) << QLatin1String(#base) << n << QAnyStringView(res);

    ROW(a, -1, a);
    ROW(a, 2, a);

    ROW(ab, -100, ab);
    ROW(ab, 100, ab);
#undef ROW
}

// This is different from the rest for historical reasons. As we're replacing
// left() with first() as the recommended API, there's no point fixing this anymore
void tst_QStringApiSymmetry::left_QByteArray_data()
{
    first_data();

    // specific data testing out of bounds cases
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << QStringView(base) << QLatin1String(#base) << n << QAnyStringView(res);

    ROW(a, -1, empty);
    ROW(a, 2, a);

    ROW(ab, -100, empty);
    ROW(ab, 100, ab);
#undef ROW
}

template <typename String>
void tst_QStringApiSymmetry::left_impl()
{
    QFETCH(const QStringView, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, n);
    QFETCH(const QAnyStringView, result);

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
}

void tst_QStringApiSymmetry::right_data()
{
    last_data();

    // specific data testing out of bounds cases
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << QStringView(base) << QLatin1String(#base) << n << QAnyStringView(res);

    ROW(a, -1, a);
    ROW(a, 2, a);

    ROW(ab, -100, ab);
    ROW(ab, 100, ab);
#undef ROW
}

// This is different from the rest for historical reasons. As we're replacing
// left() with first() as the recommended API, there's no point fixing this anymore
void tst_QStringApiSymmetry::right_QByteArray_data()
{
    last_data();

    // specific data testing out of bounds cases
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << QStringView(base) << QLatin1String(#base) << n << QAnyStringView(res);

    ROW(a, -1, empty);
    ROW(a, 2, a);

    ROW(ab, -100, empty);
    ROW(ab, 100, ab);
#undef ROW
}

template <typename String>
void tst_QStringApiSymmetry::right_impl()
{
    QFETCH(const QStringView, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, n);
    QFETCH(const QAnyStringView, result);

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

void tst_QStringApiSymmetry::sliced_data()
{
    QTest::addColumn<QStringView>("unicode");
    QTest::addColumn<QLatin1String>("latin1");
    QTest::addColumn<int>("pos");
    QTest::addColumn<int>("n");
    QTest::addColumn<QAnyStringView>("result");
    QTest::addColumn<QAnyStringView>("result2");

//    QTest::addRow("null") << QStringView() << QLatin1String() << 0 << 0 << QStringView() << QStringView();
    QTest::addRow("empty") << QStringView(empty) << QLatin1String("") << 0 << 0 << QAnyStringView(empty) << QAnyStringView(empty);

#define ROW(base, p, n, r1, r2) \
    QTest::addRow("%s%d%d", #base, p, n) << QStringView(base) << QLatin1String(#base) << p << n << QAnyStringView(r1) << QAnyStringView(r2)

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
void tst_QStringApiSymmetry::sliced_impl()
{
    QFETCH(const QStringView, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, pos);
    QFETCH(const int, n);
    QFETCH(const QAnyStringView, result);
    QFETCH(const QAnyStringView, result2);

    const auto utf8 = unicode.toUtf8();

    const auto s = make<String>(unicode, latin1, utf8);

    {
        const auto sliced = s.sliced(pos);

        QCOMPARE(sliced, result);
        QCOMPARE(sliced.isNull(), result.isNull());
        QCOMPARE(sliced.isEmpty(), result.isEmpty());
    }
    {
        const auto sliced = s.sliced(pos, n);

        QCOMPARE(sliced, result2);
        QCOMPARE(sliced.isNull(), result2.isNull());
        QCOMPARE(sliced.isEmpty(), result2.isEmpty());
    }
    {
        const auto sliced = detached(s).sliced(pos);

        QCOMPARE(sliced, result);
        QCOMPARE(sliced.isNull(), result.isNull());
        QCOMPARE(sliced.isEmpty(), result.isEmpty());
    }
    {
        const auto sliced = detached(s).sliced(pos, n);

        QCOMPARE(sliced, result2);
        QCOMPARE(sliced.isNull(), result2.isNull());
        QCOMPARE(sliced.isEmpty(), result2.isEmpty());
    }
}

void tst_QStringApiSymmetry::first_data()
{
    QTest::addColumn<QStringView>("unicode");
    QTest::addColumn<QLatin1String>("latin1");
    QTest::addColumn<int>("n");
    QTest::addColumn<QAnyStringView>("result");

//    QTest::addRow("null") << QStringView() << QLatin1String() << 0 << QStringView();
    QTest::addRow("empty") << QStringView(empty) << QLatin1String("") << 0 << QAnyStringView(empty);

    // Some classes' left() implementations have a wide contract, others a narrow one
    // so only test valid arguments here:
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << QStringView(base) << QLatin1String(#base) << n << QAnyStringView(res);

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
void tst_QStringApiSymmetry::first_impl()
{
    QFETCH(const QStringView, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, n);
    QFETCH(const QAnyStringView, result);

    const auto utf8 = unicode.toUtf8();

    const auto s = make<String>(unicode, latin1, utf8);

    {
        const auto first = s.first(n);

        QCOMPARE(first, result);
        QCOMPARE(first.isNull(), result.isNull());
        QCOMPARE(first.isEmpty(), result.isEmpty());
    }
    {
        const auto first = detached(s).first(n);

        QCOMPARE(first, result);
        QCOMPARE(first.isNull(), result.isNull());
        QCOMPARE(first.isEmpty(), result.isEmpty());
    }
    {
        auto first = s;
        first.truncate(n);

        QCOMPARE(first, result);
        QCOMPARE(first.isNull(), result.isNull());
        QCOMPARE(first.isEmpty(), result.isEmpty());
    }
}

void tst_QStringApiSymmetry::last_data()
{
    QTest::addColumn<QStringView>("unicode");
    QTest::addColumn<QLatin1String>("latin1");
    QTest::addColumn<int>("n");
    QTest::addColumn<QAnyStringView>("result");

//    QTest::addRow("null") << QStringView() << QLatin1String() << 0 << QStringView();
    QTest::addRow("empty") << QStringView(empty) << QLatin1String("") << 0 << QAnyStringView(empty);

    // Some classes' last() implementations have a wide contract, others a narrow one
    // so only test valid arguments here:
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << QStringView(base) << QLatin1String(#base) << n << QAnyStringView(res);

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
void tst_QStringApiSymmetry::last_impl()
{
    QFETCH(const QStringView, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, n);
    QFETCH(const QAnyStringView, result);

    const auto utf8 = unicode.toUtf8();

    const auto s = make<String>(unicode, latin1, utf8);

    {
        const auto last = s.last(n);

        QCOMPARE(last, result);
        QCOMPARE(last.isNull(), result.isNull());
        QCOMPARE(last.isEmpty(), result.isEmpty());
    }
    {
        const auto last = detached(s).last(n);

        QCOMPARE(last, result);
        QCOMPARE(last.isNull(), result.isNull());
        QCOMPARE(last.isEmpty(), result.isEmpty());
    }
}

void tst_QStringApiSymmetry::chop_data()
{
    QTest::addColumn<QStringView>("unicode");
    QTest::addColumn<QLatin1String>("latin1");
    QTest::addColumn<int>("n");
    QTest::addColumn<QAnyStringView>("result");

//    QTest::addRow("null") << QStringView() << QLatin1String() << 0 << QStringView();
    QTest::addRow("empty") << QStringView(empty) << QLatin1String("") << 0 << QAnyStringView(empty);

    // Some classes' truncate() implementations have a wide contract, others a narrow one
    // so only test valid arguents here:
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << QStringView(base) << QLatin1String(#base) << n << QAnyStringView(res);

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
    QFETCH(const QStringView, unicode);
    QFETCH(const QLatin1String, latin1);
    QFETCH(const int, n);
    QFETCH(const QAnyStringView, result);

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
    QTest::addColumn<QAnyStringView>("result");

    const auto latin1Whitespace = QLatin1String(" \r\n\t\f\v");

    QTest::addRow("null") << QString() << QAnyStringView();

    auto add = [latin1Whitespace](const QString &str) {
        // run through all substrings of latin1Whitespace
        for (int len = 0; len < latin1Whitespace.size(); ++len) {
            for (int pos = 0; pos < latin1Whitespace.size() - len; ++pos) {
                const QString unicode = latin1Whitespace.mid(pos, len) + str + latin1Whitespace.mid(pos, len);
                const QScopedArrayPointer<const char> escaped(QTest::toString(unicode));
                QTest::addRow("%s", escaped.data()) << unicode << QAnyStringView(str);
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
    QFETCH(const QAnyStringView, result);

    const auto utf8 = unicode.toUtf8();
    const auto l1s  = unicode.toLatin1();
    const auto l1   = l1s.isNull() ? QLatin1String() : QLatin1String(l1s);

    const auto ref = unicode.isNull() ? QStringView() : QStringView(unicode);
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

void tst_QStringApiSymmetry::toNumber_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<qint64>("result");
    QTest::addColumn<bool>("ok");

    QTest::addRow("0") << QString::fromUtf8("0") << qint64(0) << true;
    QTest::addRow("a0") << QString::fromUtf8("a0") << qint64(0) << false;
    QTest::addRow("10") << QString::fromUtf8("10") << qint64(10) << true;
    QTest::addRow("-10") << QString::fromUtf8("-10") << qint64(-10) << true;
    QTest::addRow("32767") << QString::fromUtf8("32767") << qint64(32767) << true;
    QTest::addRow("32768") << QString::fromUtf8("32768") << qint64(32768) << true;
    QTest::addRow("-32767") << QString::fromUtf8("-32767") << qint64(-32767) << true;
    QTest::addRow("-32768") << QString::fromUtf8("-32768") << qint64(-32768) << true;
    QTest::addRow("100x") << QString::fromUtf8("100x") << qint64(0) << false;
    QTest::addRow("-100x") << QString::fromUtf8("-100x") << qint64(0) << false;
}

template<typename T>
bool inRange(qint64 n)
{
    bool checkMax = quint64(std::numeric_limits<T>::max()) <= quint64(std::numeric_limits<qint64>::max());
    if (checkMax && n > qint64(std::numeric_limits<T>::max()))
        return false;
    return qint64(std::numeric_limits<T>::min()) <= n;
}

template<typename String>
void tst_QStringApiSymmetry::toNumber_impl()
{
    QFETCH(const QString, data);
    QFETCH(qint64, result);
    QFETCH(bool, ok);

    const auto utf8 = data.toUtf8();
    const auto l1s  = data.toLatin1();
    const auto l1   = l1s.isNull() ? QLatin1String() : QLatin1String(l1s);

    const auto ref = data.isNull() ? QStringView() : QStringView(data);
    const auto s = make<String>(ref, l1, utf8);

    bool is_ok = false;
    qint64 n = 0;

    n = s.toShort(&is_ok);
    QCOMPARE(is_ok, ok && inRange<short>(result));
    if (is_ok)
        QCOMPARE(n, result);

    n = s.toUShort(&is_ok);
    QCOMPARE(is_ok, ok && inRange<ushort>(result));
    if (is_ok)
        QCOMPARE(n, result);

    n = s.toInt(&is_ok);
    QCOMPARE(is_ok, ok && inRange<int>(result));
    if (is_ok)
        QCOMPARE(n, result);

    n = s.toUInt(&is_ok);
    QCOMPARE(is_ok, ok && inRange<uint>(result));
    if (is_ok)
        QCOMPARE(n, result);

    n = s.toLong(&is_ok);
    QCOMPARE(is_ok, ok && inRange<long>(result));
    if (is_ok)
        QCOMPARE(n, result);

    n = s.toULong(&is_ok);
    QCOMPARE(is_ok, ok && inRange<ulong>(result));
    if (is_ok)
        QCOMPARE(n, result);

    n = s.toLongLong(&is_ok);
    QCOMPARE(is_ok, ok && inRange<qlonglong>(result));
    if (is_ok)
        QCOMPARE(n, result);

    n = s.toULongLong(&is_ok);
    QCOMPARE(is_ok, ok && inRange<qulonglong>(result));
    if (is_ok)
        QCOMPARE(n, result);

    if (qint64(float(n)) == n) {
        float f = s.toFloat(&is_ok);
        QCOMPARE(is_ok, ok);
        if (is_ok)
            QCOMPARE(qint64(f), result);
    }

    if (qint64(double(n)) == n) {
        double d = s.toDouble(&is_ok);
        QCOMPARE(is_ok, ok);
        if (is_ok)
            QCOMPARE(qint64(d), result);
    }
}

void tst_QStringApiSymmetry::count_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<QString>("needle");
    QTest::addColumn<qsizetype>("result");

    QTest::addRow("xxx") << QString::fromUtf8("xxx") << QString::fromUtf8("x") << qsizetype(3);
    QTest::addRow("xyzaaaxyz") << QString::fromUtf8("xyzaaaxyz") << QString::fromUtf8("xyz") << qsizetype(2);
}

template <typename String>
void tst_QStringApiSymmetry::count_impl()
{
    QFETCH(const QString, data);
    QFETCH(const QString, needle);
    QFETCH(qsizetype, result);

    const auto utf8 = data.toUtf8();
    const auto l1s  = data.toLatin1();
    const auto l1   = l1s.isNull() ? QLatin1String() : QLatin1String(l1s);

    const auto ref = data.isNull() ? QStringView() : QStringView(data);
    const auto s = make<String>(ref, l1, utf8);

    const auto nutf8 = needle.toUtf8();
    const auto nl1s  = needle.toLatin1();
    const auto nl1   = nl1s.isNull() ? QLatin1String() : QLatin1String(nl1s);

    const auto nref = needle.isNull() ? QStringView() : QStringView(needle);
    const auto ns = make<String>(nref, nl1, nutf8);

    QCOMPARE(s.count(ns), result);
    if (ns.length() == 1)
        QCOMPARE(s.count(ns.data()[0]), result);
}

//
//
// UTF-16-only checks:
//
//

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
    QTest::addColumn<QList<uint>>("ucs4");

    auto add = [](const char *l1) {
        const QByteArray ba(l1);
        QString s;
        QList<uint> ucs4;
        for (char c : ba) {
            s += QLatin1Char(c);
            ucs4.append(uint(uchar(c)));
        }
        QTest::newRow(rowName(ba).constData()) << s << ucs4;
    };

    QTest::addRow("null") << QString() << QList<uint>();
    QTest::addRow("empty") << QString("") << QList<uint>();

    add("M\xF6" "bius");
    add(LONG_STRING_256);
}

template <typename String>
void tst_QStringApiSymmetry::toUcs4_impl()
{
    QFETCH(const QString, unicode);
    QFETCH(const QList<uint>, ucs4);

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

    const auto haystack = make<Haystack>(QStringView(haystackU16), haystackL1, haystackU8);
    const auto needle = make<Needle>(QStringView(needleU16), needleL1, needleU8);

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

    const auto haystack = make<Haystack>(QStringView(haystackU16), haystackL1, haystackU8);
    const auto needle = make<Needle>(QStringView(needleU16), needleL1, needleU8);

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
    QTest::addRow("haystack: %s, needle: %s, start %d", #h, #n, st) << h << QLatin1String(#h) \
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
    ROW(ABCDEFGHIEfGEFG, G,  15, 14, 14);
    ROW(ABCDEFGHIEfGEFG, g,  15, -1, 14);
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
        ROW(ABCDEFGHIEfGEFG, efg,  15, -1, 12);
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

    const auto haystack = make<Haystack>(QStringView(haystackU16), haystackL1, haystackU8);
    const auto needle = make<Needle>(QStringView(needleU16), needleL1, needleU8);

    using size_type = typename Haystack::size_type;

    QCOMPARE(haystack.lastIndexOf(needle, startpos), size_type(resultCS));
    QCOMPARE(haystack.lastIndexOf(needle, startpos, Qt::CaseSensitive), size_type(resultCS));
    QCOMPARE(haystack.lastIndexOf(needle, startpos, Qt::CaseInsensitive), size_type(resultCIS));

}

void tst_QStringApiSymmetry::indexOf_contains_lastIndexOf_count_regexp_data()
{
    QTest::addColumn<QString>("subject");
    QTest::addColumn<QRegularExpression>("regexp");
    QTest::addColumn<qsizetype>("leftFrom");
    QTest::addColumn<qsizetype>("indexOf");
    QTest::addColumn<qsizetype>("count");
    QTest::addColumn<qsizetype>("rightFrom");
    QTest::addColumn<qsizetype>("lastIndexOf");

    const auto ROW = [](const char *subject,
                        const char *pattern,
                        QRegularExpression::PatternOptions options,
                        qsizetype leftFrom, qsizetype indexOf, qsizetype count,
                        qsizetype rightFrom, qsizetype lastIndexOf)
    {
        QTest::addRow("subject \"%s\" pattern \"%s\" options %d leftFrom %d rightFrom %d",
                      subject, pattern, (int)options, (int)leftFrom, (int)rightFrom)
                << subject
                << QRegularExpression(pattern, options)
                << leftFrom
                << indexOf
                << count
                << rightFrom
                << lastIndexOf;
    };

    ROW("", "", QRegularExpression::NoPatternOption, 0, 0, 1, -1, -1);
    ROW("", "", QRegularExpression::NoPatternOption, 0, 0, 1, 0, 0);
    ROW("test", "", QRegularExpression::NoPatternOption, 0, 0, 5, -1, 3);
    ROW("test", "", QRegularExpression::NoPatternOption, 0, 0, 5, -2, 2);
    ROW("test", "", QRegularExpression::NoPatternOption, 0, 0, 5, -3, 1);
    ROW("test", "", QRegularExpression::NoPatternOption, 0, 0, 5, -4, 0);
    ROW("test", "", QRegularExpression::NoPatternOption, 0, 0, 5, -5, -1);
    ROW("test", "", QRegularExpression::NoPatternOption, 0, 0, 5, 0, 0);
    ROW("test", "", QRegularExpression::NoPatternOption, 0, 0, 5, 1, 1);
    ROW("test", "", QRegularExpression::NoPatternOption, 0, 0, 5, 2, 2);
    ROW("test", "", QRegularExpression::NoPatternOption, 0, 0, 5, 3, 3);
    ROW("test", "", QRegularExpression::NoPatternOption, 0, 0, 5, 4, 4);
    ROW("", "^", QRegularExpression::NoPatternOption, 0, 0, 1, -1, -1);
    ROW("", "^", QRegularExpression::NoPatternOption, 0, 0, 1, 0, 0);
    ROW("", "$", QRegularExpression::NoPatternOption, 0, 0, 1, -1, -1);
    ROW("", "$", QRegularExpression::NoPatternOption, 0, 0, 1, 0, 0);
    ROW("", "^$", QRegularExpression::NoPatternOption, 0, 0, 1, -1, -1);
    ROW("", "^$", QRegularExpression::NoPatternOption, 0, 0, 1, 0, 0);
    ROW("", "x", QRegularExpression::NoPatternOption, 0, -1, 0, -1, -1);
    ROW("", "x", QRegularExpression::NoPatternOption, 0, -1, 0, 0, -1);
    ROW("", "^x", QRegularExpression::NoPatternOption, 0, -1, 0, -1, -1);
    ROW("", "^x", QRegularExpression::NoPatternOption, 0, -1, 0, 0, -1);
    ROW("", "x$", QRegularExpression::NoPatternOption, 0, -1, 0, -1, -1);
    ROW("", "x$", QRegularExpression::NoPatternOption, 0, -1, 0, 0, -1);
    ROW("", "^x$", QRegularExpression::NoPatternOption, 0, -1, 0, -1, -1);
    ROW("", "^x$", QRegularExpression::NoPatternOption, 0, -1, 0, 0, -1);

    ROW("test", "e", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "e", QRegularExpression::NoPatternOption, 0, 1, 1, -2, 1);
    ROW("test", "es", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "es", QRegularExpression::NoPatternOption, 0, 1, 1, -2, 1);
    ROW("test", "es?", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "es?", QRegularExpression::NoPatternOption, 0, 1, 1, -2, 1);
    ROW("test", "es+", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "es+", QRegularExpression::NoPatternOption, 0, 1, 1, -2, 1);
    ROW("test", "e.", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "e.", QRegularExpression::NoPatternOption, 0, 1, 1, -2, 1);
    ROW("test", "e.*", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "e.*", QRegularExpression::NoPatternOption, 0, 1, 1, -2, 1);
    ROW("test", "e(?=s)", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "e(?!x)", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "ex?s", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "(?<=t)e", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "(?<!x)e", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "t", QRegularExpression::NoPatternOption, 0, 0, 2, 0, 0);
    ROW("test", "t", QRegularExpression::NoPatternOption, 0, 0, 2, 1, 0);
    ROW("test", "t", QRegularExpression::NoPatternOption, 0, 0, 2, 2, 0);
    ROW("test", "t", QRegularExpression::NoPatternOption, 0, 0, 2, 3, 3);
    ROW("test", "t", QRegularExpression::NoPatternOption, 0, 0, 2, 4, 3);
    ROW("test", "t", QRegularExpression::NoPatternOption, 0, 0, 2, -1, 3);
    ROW("test", "t", QRegularExpression::NoPatternOption, 0, 0, 2, -2, 0);
    ROW("test", "t", QRegularExpression::NoPatternOption, 0, 0, 2, -3, 0);

    ROW("test", "^es", QRegularExpression::NoPatternOption, 0, -1, 0, -1, -1);
    ROW("test", "^es", QRegularExpression::NoPatternOption, 0, -1, 0, -2, -1);
    ROW("test", "es$", QRegularExpression::NoPatternOption, 0, -1, 0, -1, -1);
    ROW("test", "ex", QRegularExpression::NoPatternOption, 0, -1, 0, -1, -1);
    ROW("test", "ex?", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "ex+", QRegularExpression::NoPatternOption, 0, -1, 0, -1, -1);
    ROW("test", "e(?=x)", QRegularExpression::NoPatternOption, 0, -1, 0, -1, -1);
    ROW("test", "e(?!s)", QRegularExpression::NoPatternOption, 0, -1, 0, -1, -1);


    ROW("test", "e.*t", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "e.*t", QRegularExpression::NoPatternOption, 0, 1, 1, -2, 1);
    ROW("test", "e.*t", QRegularExpression::NoPatternOption, 0, 1, 1, -3, 1);
    ROW("test", "e.*t", QRegularExpression::NoPatternOption, 0, 1, 1, -4, -1);
    ROW("test", "e.*t", QRegularExpression::NoPatternOption, 0, 1, 1, -5, -1);
    ROW("test", "e.*t$", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "e.*t$", QRegularExpression::NoPatternOption, 0, 1, 1, -2, 1);
    ROW("test", "e.*st", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "e.*st$", QRegularExpression::NoPatternOption, 0, 1, 1, -1, 1);
    ROW("test", "t.*t", QRegularExpression::NoPatternOption, 0, 0, 1, -1, 0);
    ROW("test", "t.*t", QRegularExpression::NoPatternOption, 0, 0, 1, -2, 0);
    ROW("test", "st", QRegularExpression::NoPatternOption, 0, 2, 1, -1, 2);
    ROW("test", "st", QRegularExpression::NoPatternOption, 0, 2, 1, -2, 2);
    ROW("test", "st", QRegularExpression::NoPatternOption, 0, 2, 1, -3, -1);
    ROW("test", "st", QRegularExpression::NoPatternOption, 0, 2, 1, -4, -1);

    ROW("", "", QRegularExpression::CaseInsensitiveOption, 0, 0, 1, -1, -1);
    ROW("", "", QRegularExpression::CaseInsensitiveOption, 0, 0, 1, 0, 0);
    ROW("test", "", QRegularExpression::CaseInsensitiveOption, 0, 0, 5, -1, 3);
    ROW("test", "", QRegularExpression::CaseInsensitiveOption, 0, 0, 5, 4, 4);
    ROW("test", "^", QRegularExpression::CaseInsensitiveOption, 0, 0, 1, -1, 0);
    ROW("test", "^", QRegularExpression::CaseInsensitiveOption, 0, 0, 1, 4, 0);
    ROW("test", "^t", QRegularExpression::CaseInsensitiveOption, 0, 0, 1, -1, 0);
    ROW("test", "^t", QRegularExpression::CaseInsensitiveOption, 0, 0, 1, 4, 0);
    ROW("TEST", "^t", QRegularExpression::CaseInsensitiveOption, 0, 0, 1, -1, 0);
    ROW("TEST", "^t", QRegularExpression::CaseInsensitiveOption, 0, 0, 1, 4, 0);
    ROW("test", "e", QRegularExpression::CaseInsensitiveOption, 0, 1, 1, -1, 1);
    ROW("TEST", "e", QRegularExpression::CaseInsensitiveOption, 0, 1, 1, -1, 1);
    ROW("TEST", "es", QRegularExpression::CaseInsensitiveOption, 0, 1, 1, -1, 1);
    ROW("test", "ES", QRegularExpression::CaseInsensitiveOption, 0, 1, 1, -1, 1);
    ROW("TEST", "ex?s", QRegularExpression::CaseInsensitiveOption, 0, 1, 1, -1, 1);

    ROW("testtest", "e", QRegularExpression::NoPatternOption, 0, 1, 2, -1, 5);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, 0, 1, 2, -2, 5);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, 0, 1, 2, -3, 5);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, 0, 1, 2, -4, 1);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, 0, 1, 2, -5, 1);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, 0, 1, 2, -6, 1);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, 0, 1, 2, -7, 1);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, 0, 1, 2, -8, -1);
    ROW("testtest", "es", QRegularExpression::NoPatternOption, 0, 1, 2, -1, 5);
    ROW("testtest", "e.*s", QRegularExpression::NoPatternOption, 0, 1, 2, -1, 1);

    ROW("testtest", "e", QRegularExpression::NoPatternOption, 1, 1, 2, -1, 5);
    ROW("testtest", "es", QRegularExpression::NoPatternOption, 1, 1, 2, -1, 5);
    ROW("testtest", "e.*s", QRegularExpression::NoPatternOption, 1, 1, 2, -1, 1);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, 2, 5, 1, -1, 5);
    ROW("testtest", "es", QRegularExpression::NoPatternOption, 2, 5, 1, -1, 5);
    ROW("testtest", "es", QRegularExpression::NoPatternOption, 2, 5, 1, -2, 5);
    ROW("testtest", "es", QRegularExpression::NoPatternOption, 2, 5, 1, -3, 5);
    ROW("testtest", "es", QRegularExpression::NoPatternOption, 2, 5, 1, -4, 1);
    ROW("testtest", "es", QRegularExpression::NoPatternOption, 2, 5, 1, -5, 1);
    ROW("testtest", "e.*s", QRegularExpression::NoPatternOption, 2, 5, 1, -1, 1);

    ROW("testtest", "e", QRegularExpression::NoPatternOption, -1, -1, 0, 0, -1);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, -1, -1, 0, 1, 1);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, -1, -1, 0, 2, 1);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, -1, -1, 0, 3, 1);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, -1, -1, 0, 4, 1);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, -1, -1, 0, 5, 5);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, -1, -1, 0, 6, 5);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, -1, -1, 0, 7, 5);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, -2, -1, 0, -1, 5);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, -3, 5, 1, -1, 5);
    ROW("testtest", "e", QRegularExpression::NoPatternOption, -4, 5, 1, -1, 5);
    ROW("testtest", "t", QRegularExpression::NoPatternOption, 0, 0, 4, 0, 0);
    ROW("testtest", "t", QRegularExpression::NoPatternOption, 0, 0, 4, 1, 0);
    ROW("testtest", "t", QRegularExpression::NoPatternOption, 0, 0, 4, 2, 0);
    ROW("testtest", "t", QRegularExpression::NoPatternOption, 0, 0, 4, 3, 3);
    ROW("testtest", "t", QRegularExpression::NoPatternOption, 0, 0, 4, 4, 4);
    ROW("testtest", "t", QRegularExpression::NoPatternOption, 0, 0, 4, 5, 4);
    ROW("testtest", "t", QRegularExpression::NoPatternOption, 0, 0, 4, 6, 4);
    ROW("testtest", "t", QRegularExpression::NoPatternOption, 0, 0, 4, 7, 7);
    ROW("testtest", "t(?!e)", QRegularExpression::NoPatternOption, 0, 3, 2, 0, -1);
    ROW("testtest", "t(?!e)", QRegularExpression::NoPatternOption, 0, 3, 2, 1, -1);
    ROW("testtest", "t(?!e)", QRegularExpression::NoPatternOption, 0, 3, 2, 2, -1);
    ROW("testtest", "t(?!e)", QRegularExpression::NoPatternOption, 0, 3, 2, 3, 3);
    ROW("testtest", "t(?!e)", QRegularExpression::NoPatternOption, 0, 3, 2, 4, 3);
    ROW("testtest", "t(?!e)", QRegularExpression::NoPatternOption, 0, 3, 2, -1, 7);
    ROW("testtest", "tt", QRegularExpression::NoPatternOption, -1, -1, 0, 0, -1);
    ROW("testtest", "tt", QRegularExpression::NoPatternOption, -1, -1, 0, 1, -1);
    ROW("testtest", "tt", QRegularExpression::NoPatternOption, -1, -1, 0, 2, -1);
    ROW("testtest", "tt", QRegularExpression::NoPatternOption, -1, -1, 0, 3, 3);
    ROW("testtest", "tt", QRegularExpression::NoPatternOption, -1, -1, 0, 4, 3);
    ROW("testtest", "tt", QRegularExpression::NoPatternOption, -2, -1, 0, 0, -1);
    ROW("testtest", "tt", QRegularExpression::NoPatternOption, -3, -1, 0, 0, -1);
    ROW("testtest", "tt", QRegularExpression::NoPatternOption, -4, -1, 0, 0, -1);
    ROW("testtest", "tt", QRegularExpression::NoPatternOption, -5, 3, 1, -1, 3);

    ROW("testtest", "(?<=t)e", QRegularExpression::NoPatternOption, 1, 1, 1, -1, 5); // the count is 1 because in the test we _cut_ the string before the lookbehind
    ROW("testtest", "(?<=t)e", QRegularExpression::NoPatternOption, 2, 5, 1, -1, 5);
    ROW("testtest", "(?<=t)e", QRegularExpression::NoPatternOption, 3, 5, 1, -1, 5);
    ROW("testtest", "(?<=t)e", QRegularExpression::NoPatternOption, 4, 5, 1, -1, 5);
    ROW("testtest", "(?<=t)e", QRegularExpression::NoPatternOption, 5, 5, 0, -1, 5); // the count is 0 because in the test we _cut_ the string before the lookbehind
    ROW("testtest", "(?<=t)e", QRegularExpression::NoPatternOption, 6, -1, 0, -1, 5);

    ROW("foo bar blubb", "\\s+", QRegularExpression::NoPatternOption, 0, 3, 2, -1, 7);
    ROW("foo bar blubb", "\\s+", QRegularExpression::NoPatternOption, 0, 3, 2, -7, 3);
}

template <typename String>
void tst_QStringApiSymmetry::indexOf_contains_lastIndexOf_count_regexp_impl() const
{
    QFETCH(QString, subject);
    QFETCH(QRegularExpression, regexp);
    QFETCH(qsizetype, leftFrom);
    QFETCH(qsizetype, indexOf);
    QFETCH(qsizetype, count);
    QFETCH(qsizetype, rightFrom);
    QFETCH(qsizetype, lastIndexOf);

    // indexOf
    String s = subject;
    qsizetype result = s.indexOf(regexp, leftFrom);
    QCOMPARE(result, indexOf);

    // contains
    if (result >= 0)
        QVERIFY(s.contains(regexp));
    else if (leftFrom == 0)
        QVERIFY(!s.contains(regexp));

    // count
    if (leftFrom >= 0)
        QCOMPARE(s.mid(leftFrom).count(regexp), count);
    else
        QCOMPARE(s.mid(leftFrom + s.size()).count(regexp), count);

    // lastIndexOf
    result = s.lastIndexOf(regexp, rightFrom);
    QCOMPARE(result, lastIndexOf);
    if (rightFrom == s.size()) {
        result = s.lastIndexOf(regexp);
        QCOMPARE(result, lastIndexOf);
    }
}

QTEST_APPLESS_MAIN(tst_QStringApiSymmetry)

#include "tst_qstringapisymmetry.moc"
