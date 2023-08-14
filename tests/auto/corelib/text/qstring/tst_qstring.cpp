/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2020 Intel Corporation.
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

#ifdef QT_NO_CAST_FROM_ASCII
# undef QT_NO_CAST_FROM_ASCII
#endif
#ifdef QT_NO_CAST_TO_ASCII
# undef QT_NO_CAST_TO_ASCII
#endif
#ifdef QT_ASCII_CAST_WARNINGS
# undef QT_ASCII_CAST_WARNINGS
#endif

#include <private/qglobal_p.h> // for the icu feature test
#include <QTest>
#include <QString>
#include <QStringBuilder>
#if QT_CONFIG(regularexpression)
#include <qregularexpression.h>
#endif
#include <qtextstream.h>
#include <qstringlist.h>
#include <qstringmatcher.h>
#include <qbytearraymatcher.h>
#include <qvariant.h>

#include <qlocale.h>
#include <locale.h>
#include <qhash.h>

#include <string>
#include <algorithm>

#define CREATE_VIEW(string)                                              \
    const QString padded = QLatin1Char(' ') + string + QLatin1Char(' '); \
    const QStringView view = QStringView{ padded }.mid(1, padded.size() - 2);

namespace {

// this wraps an argument to a QString function, as well as how to apply
// the argument to a given QString member function.
template <typename T>
class Arg;

template <typename T>
class Reversed {}; // marker for Arg<QChar> to apply the operation in reverse order (for prepend())

class ArgBase
{
protected:
    QString pinned;
    explicit ArgBase(const char *str)
        : pinned(QString::fromUtf8(str)) {}
};

template <>
class Arg<QChar> : protected ArgBase
{
public:
    explicit Arg(const char *str) : ArgBase(str) {}

    template <typename MemFun>
    void apply0(QString &s, MemFun mf) const
    { for (QChar ch : qAsConst(this->pinned)) (s.*mf)(ch); }

    template <typename MemFun, typename A1>
    void apply1(QString &s, MemFun mf, A1 a1) const
    { for (QChar ch : qAsConst(this->pinned)) (s.*mf)(a1, ch); }
};

template <>
class Arg<Reversed<QChar> > : private Arg<QChar>
{
public:
    explicit Arg(const char *str) : Arg<QChar>(str)
    {
        std::reverse(this->pinned.begin(), this->pinned.end());
    }

    using Arg<QChar>::apply0;
    using Arg<QChar>::apply1;
};

template <>
class Arg<QString> : ArgBase
{
public:
    explicit Arg(const char *str) : ArgBase(str) {}

    template <typename MemFun>
    void apply0(QString &s, MemFun mf) const
    { (s.*mf)(this->pinned); }

    template <typename MemFun, typename A1>
    void apply1(QString &s, MemFun mf, A1 a1) const
    { (s.*mf)(a1, this->pinned); }
};

template <>
class Arg<QStringView> : ArgBase
{
    QStringView view() const
    { return this->pinned.isNull() ? QStringView() : QStringView(this->pinned) ; }
public:
    explicit Arg(const char *str) : ArgBase(str) {}

    template <typename MemFun>
    void apply0(QString &s, MemFun mf) const
    { (s.*mf)(view()); }

    template <typename MemFun, typename A1>
    void apply1(QString &s, MemFun mf, A1 a1) const
    { (s.*mf)(a1, view()); }
};

template <>
class Arg<QPair<const QChar *, int> > : ArgBase
{
public:
    explicit Arg(const char *str) : ArgBase(str) {}

    template <typename MemFun>
    void apply0(QString &s, MemFun mf) const
    { (s.*mf)(this->pinned.constData(), this->pinned.length()); }

    template <typename MemFun, typename A1>
    void apply1(QString &s, MemFun mf, A1 a1) const
    { (s.*mf)(a1, this->pinned.constData(), this->pinned.length()); }
};

template <>
class Arg<QLatin1String>
{
    QLatin1String l1;
public:
    explicit Arg(const char *str) : l1(str) {}

    template <typename MemFun>
    void apply0(QString &s, MemFun mf) const
    { (s.*mf)(l1); }

    template <typename MemFun, typename A1>
    void apply1(QString &s, MemFun mf, A1 a1) const
    { (s.*mf)(a1, l1); }
};

template <>
class Arg<char>
{
protected:
    const char *str;
public:
    explicit Arg(const char *str) : str(str) {}

    template <typename MemFun>
    void apply0(QString &s, MemFun mf) const
    {
        if (str) {
            for (const char *it = str; *it; ++it)
                (s.*mf)(*it);
        }
    }

    template <typename MemFun, typename A1>
    void apply1(QString &s, MemFun mf, A1 a1) const
    {
        if (str) {
            for (const char *it = str; *it; ++it)
                (s.*mf)(a1, *it);
        }
    }
};

template <>
class Arg<Reversed<char> > : private Arg<char>
{
    static const char *dupAndReverse(const char *s)
    {
        char *s2 = qstrdup(s);
        std::reverse(s2, s2 + qstrlen(s2));
        return s2;
    }
public:
    explicit Arg(const char *str) : Arg<char>(dupAndReverse(str)) {}
    ~Arg() { delete[] str; }

    using Arg<char>::apply0;
    using Arg<char>::apply1;
};

template <>
class Arg<const char*>
{
    const char *str;
public:
    explicit Arg(const char *str) : str(str) {}

    template <typename MemFun>
    void apply0(QString &s, MemFun mf) const
    { (s.*mf)(str); }

    template <typename MemFun, typename A1>
    void apply1(QString &s, MemFun mf, A1 a1) const
    { (s.*mf)(a1, str); }
};

template <>
class Arg<QByteArray>
{
    QByteArray ba;
public:
    explicit Arg(const char *str) : ba(str) {}

    template <typename MemFun>
    void apply0(QString &s, MemFun mf) const
    { (s.*mf)(ba); }

    template <typename MemFun, typename A1>
    void apply1(QString &s, MemFun mf, A1 a1) const
    { (s.*mf)(a1, ba); }
};

// const char* is not allowed as columns in data-driven tests (causes static_assert failure),
// so wrap it in a container (default ctor is a QMetaType/QVariant requirement):
class CharStarContainer
{
    const char *str;
public:
    explicit constexpr CharStarContainer(const char *s = nullptr) : str(s) {}
    constexpr operator const char *() const { return str; }
};

} // unnamed namespace
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(CharStarContainer, Q_PRIMITIVE_TYPE);
QT_END_NAMESPACE

Q_DECLARE_METATYPE(CharStarContainer)

// implementation helpers for append_impl/prepend_impl etc
template <typename ArgType, typename MemFun>
static void do_apply0(MemFun mf)
{
    QFETCH(QString, s);
    QFETCH(CharStarContainer, arg);
    QFETCH(QString, expected);

    Arg<ArgType>(arg).apply0(s, mf);

    QCOMPARE(s, expected);
    QCOMPARE(s.isEmpty(), expected.isEmpty());
    QCOMPARE(s.isNull(), expected.isNull());
}

template <typename ArgType, typename A1, typename MemFun>
static void do_apply1(MemFun mf)
{
    QFETCH(QString, s);
    QFETCH(CharStarContainer, arg);
    QFETCH(A1, a1);
    QFETCH(QString, expected);

    Arg<ArgType>(arg).apply1(s, mf, a1);

    QCOMPARE(s, expected);
    QCOMPARE(s.isEmpty(), expected.isEmpty());
    QCOMPARE(s.isNull(), expected.isNull());
}

class tst_QString : public QObject
{
    Q_OBJECT
public:
    enum DataOption {
        EmptyIsNoop = 0x1,
        Latin1Encoded = 0x2
    };
    Q_DECLARE_FLAGS(DataOptions, DataOption)
private:

#if QT_CONFIG(regularexpression)
    template<typename List, class RegExp>
    void split_regexp(const QString &string, const QString &pattern, QStringList result);
#endif
    template<typename List>
    void split(const QString &string, const QString &separator, QStringList result);

    template <typename ArgType, typename MemFun>
    void append_impl() const { do_apply0<ArgType>(MemFun(&QString::append)); }
    template <typename ArgType>
    void append_impl() const { append_impl<ArgType, QString &(QString::*)(const ArgType&)>(); }
    void append_data(DataOptions options = {});
    template <typename ArgType, typename MemFun>
    void operator_pluseq_impl() const { do_apply0<ArgType>(MemFun(&QString::operator+=)); }
    template <typename ArgType>
    void operator_pluseq_impl() const { operator_pluseq_impl<ArgType, QString &(QString::*)(const ArgType&)>(); }
    void operator_pluseq_data(DataOptions options = {});
    template <typename ArgType, typename MemFun>
    void prepend_impl() const { do_apply0<ArgType>(MemFun(&QString::prepend)); }
    template <typename ArgType>
    void prepend_impl() const { prepend_impl<ArgType, QString &(QString::*)(const ArgType&)>(); }
    void prepend_data(DataOptions options = {});
    template <typename ArgType, typename MemFun>
    void insert_impl() const { do_apply1<ArgType, int>(MemFun(&QString::insert)); }
    template <typename ArgType>
    void insert_impl() const { insert_impl<ArgType, QString &(QString::*)(qsizetype, const ArgType&)>(); }
    void insert_data(DataOptions options = {});

    class TransientDefaultLocale
    {
        const QLocale prior; // Records what *was* the default before we set it.
    public:
        TransientDefaultLocale(const QLocale &transient) { revise(transient); }
        void revise(const QLocale &transient) { QLocale::setDefault(transient); }
        ~TransientDefaultLocale() { QLocale::setDefault(prior); }
    };

public:
    tst_QString();
private slots:
    void fromStdString();
    void toStdString();
    void check_QTextIOStream();
    void check_QTextStream();
    void check_QDataStream();
    void fromRawData();
    void setRawData();
    void setUnicode();
    void endsWith();
    void startsWith();
    void setNum();
    void toDouble_data();
    void toDouble();
    void toFloat();
    void toLong_data();
    void toLong();
    void toULong_data();
    void toULong();
    void toLongLong();
    void toULongLong();
    void toUInt();
    void toInt();
    void toShort();
    void toUShort();
    void replace_qchar_qchar_data();
    void replace_qchar_qchar();
    void replace_qchar_qstring_data();
    void replace_qchar_qstring();
    void replace_uint_uint_data();
    void replace_uint_uint();
    void replace_uint_uint_extra();
    void replace_extra();
    void replace_string_data();
    void replace_string();
    void replace_string_extra();
#if QT_CONFIG(regularexpression)
    void replace_regexp_data();
    void replace_regexp();
    void replace_regexp_extra();
#endif
    void remove_uint_uint_data();
    void remove_uint_uint();
    void remove_string_data();
    void remove_string();
#if QT_CONFIG(regularexpression)
    void remove_regexp_data();
    void remove_regexp();
#endif
    void remove_extra();
    void swap();

    void prepend_qstring()            { prepend_impl<QString>(); }
    void prepend_qstring_data()       { prepend_data(EmptyIsNoop); }
    void prepend_qstringview()        { prepend_impl<QStringView, QString &(QString::*)(QStringView)>(); }
    void prepend_qstringview_data()   { prepend_data(EmptyIsNoop); }
    void prepend_qlatin1string()      { prepend_impl<QLatin1String, QString &(QString::*)(QLatin1String)>(); }
    void prepend_qlatin1string_data() { prepend_data({EmptyIsNoop, Latin1Encoded}); }
    void prepend_qcharstar_int()      { prepend_impl<QPair<const QChar *, int>, QString &(QString::*)(const QChar *, qsizetype)>(); }
    void prepend_qcharstar_int_data() { prepend_data(EmptyIsNoop); }
    void prepend_qchar()              { prepend_impl<Reversed<QChar>, QString &(QString::*)(QChar)>(); }
    void prepend_qchar_data()         { prepend_data(EmptyIsNoop); }
    void prepend_qbytearray()         { prepend_impl<QByteArray>(); }
    void prepend_qbytearray_data()    { prepend_data(EmptyIsNoop); }
    void prepend_char()               { prepend_impl<Reversed<char>, QString &(QString::*)(QChar)>(); }
    void prepend_char_data()          { prepend_data({EmptyIsNoop, Latin1Encoded}); }
    void prepend_charstar()           { prepend_impl<const char *, QString &(QString::*)(const char *)>(); }
    void prepend_charstar_data()      { prepend_data(EmptyIsNoop); }
    void prepend_bytearray_special_cases_data();
    void prepend_bytearray_special_cases();

    void append_qstring()            { append_impl<QString>(); }
    void append_qstring_data()       { append_data(); }
    void append_qstringview()        { append_impl<QStringView,  QString &(QString::*)(QStringView)>(); }
    void append_qstringview_data()   { append_data(EmptyIsNoop); }
    void append_qlatin1string()      { append_impl<QLatin1String, QString &(QString::*)(QLatin1String)>(); }
    void append_qlatin1string_data() { append_data(Latin1Encoded); }
    void append_qcharstar_int()      { append_impl<QPair<const QChar *, int>, QString&(QString::*)(const QChar *, qsizetype)>(); }
    void append_qcharstar_int_data() { append_data(EmptyIsNoop); }
    void append_qchar()              { append_impl<QChar, QString &(QString::*)(QChar)>(); }
    void append_qchar_data()         { append_data(EmptyIsNoop); }
    void append_qbytearray()         { append_impl<QByteArray>(); }
    void append_qbytearray_data()    { append_data(); }
    void append_char()               { append_impl<char, QString &(QString::*)(QChar)>(); }
    void append_char_data()          { append_data({EmptyIsNoop, Latin1Encoded}); }
    void append_charstar()           { append_impl<const char *, QString &(QString::*)(const char *)>(); }
    void append_charstar_data()      { append_data(); }
    void append_special_cases();
    void append_bytearray_special_cases_data();
    void append_bytearray_special_cases();

    void appendFromRawData();

    void operator_pluseq_qstring()            { operator_pluseq_impl<QString>(); }
    void operator_pluseq_qstring_data()       { operator_pluseq_data(); }
    void operator_pluseq_qstringview()        { operator_pluseq_impl<QStringView, QString &(QString::*)(QStringView)>(); }
    void operator_pluseq_qstringview_data()   { operator_pluseq_data(EmptyIsNoop); }
    void operator_pluseq_qlatin1string()      { operator_pluseq_impl<QLatin1String, QString &(QString::*)(QLatin1String)>(); }
    void operator_pluseq_qlatin1string_data() { operator_pluseq_data(Latin1Encoded); }
    void operator_pluseq_qchar()              { operator_pluseq_impl<QChar, QString &(QString::*)(QChar)>(); }
    void operator_pluseq_qchar_data()         { operator_pluseq_data(EmptyIsNoop); }
    void operator_pluseq_qbytearray()         { operator_pluseq_impl<QByteArray>(); }
    void operator_pluseq_qbytearray_data()    { operator_pluseq_data(); }
    void operator_pluseq_charstar()           { operator_pluseq_impl<const char *, QString &(QString::*)(const char *)>(); }
    void operator_pluseq_charstar_data()      { operator_pluseq_data(); }
    void operator_pluseq_special_cases();
    void operator_pluseq_bytearray_special_cases_data();
    void operator_pluseq_bytearray_special_cases();

    void operator_eqeq_bytearray_data();
    void operator_eqeq_bytearray();
    void operator_eqeq_nullstring();
    void operator_smaller();

    void insert_qstring()            { insert_impl<QString>(); }
    void insert_qstring_data()       { insert_data(EmptyIsNoop); }
    void insert_qstringview()        { insert_impl<QStringView, QString &(QString::*)(qsizetype, QStringView)>(); }
    void insert_qstringview_data()   { insert_data(EmptyIsNoop); }
    void insert_qlatin1string()      { insert_impl<QLatin1String, QString &(QString::*)(qsizetype, QLatin1String)>(); }
    void insert_qlatin1string_data() { insert_data({EmptyIsNoop, Latin1Encoded}); }
    void insert_qcharstar_int()      { insert_impl<QPair<const QChar *, int>, QString &(QString::*)(qsizetype, const QChar*, qsizetype) >(); }
    void insert_qcharstar_int_data() { insert_data(EmptyIsNoop); }
    void insert_qchar()              { insert_impl<Reversed<QChar>, QString &(QString::*)(qsizetype, QChar)>(); }
    void insert_qchar_data()         { insert_data(EmptyIsNoop); }
    void insert_qbytearray()         { insert_impl<QByteArray>(); }
    void insert_qbytearray_data()    { insert_data(EmptyIsNoop); }
    void insert_char()               { insert_impl<Reversed<char>, QString &(QString::*)(qsizetype, QChar)>(); }
    void insert_char_data()          { insert_data({EmptyIsNoop, Latin1Encoded}); }
    void insert_charstar()           { insert_impl<const char *, QString &(QString::*)(qsizetype, const char*) >(); }
    void insert_charstar_data()      { insert_data(EmptyIsNoop); }
    void insert_special_cases();

    void simplified_data();
    void simplified();
    void trimmed();
    void unicodeTableAccess_data();
    void unicodeTableAccess();
    void toUpper();
    void toLower();
    void isLower_isUpper_data();
    void isLower_isUpper();
    void toCaseFolded();
    void rightJustified();
    void leftJustified();
    void mid();
    void right();
    void left();
    void contains();
    void count();
    void lastIndexOf_data();
    void lastIndexOf();
    void indexOf_data();
    void indexOf();
#if QT_CONFIG(regularexpression)
    void indexOfInvalidRegex();
    void lastIndexOfInvalidRegex();
#endif
    void indexOf2_data();
    void indexOf2();
    void indexOf3_data();
//  void indexOf3();
    void asprintf();
    void asprintfS();
    void fill();
    void truncate();
    void chop_data();
    void chop();
    void constructor();
    void constructorQByteArray_data();
    void constructorQByteArray();
    void STL();
    void macTypes();
    void isEmpty();
    void isNull();
    void acc_01();
    void length_data();
    void length();
    void utf8_data();
    void utf8();
    void fromUtf8_data();
    void fromUtf8();
    void nullFromUtf8();
    void fromLocal8Bit_data();
    void fromLocal8Bit();
    void local8Bit_data();
    void local8Bit();
    void invalidToLocal8Bit_data();
    void invalidToLocal8Bit();
    void nullFromLocal8Bit();
    void fromLatin1Roundtrip_data();
    void fromLatin1Roundtrip();
    void toLatin1Roundtrip_data();
    void toLatin1Roundtrip();
    void fromLatin1();
    void fromUcs4();
    void toUcs4();
    void arg();
    void number();
    void number_base_data();
    void number_base();
    void doubleOut();
    void arg_fillChar_data();
    void arg_fillChar();
    void capacity_data();
    void capacity();
    void section_data();
    void section();
    void double_conversion_data();
    void double_conversion();
    void integer_conversion_data();
    void integer_conversion();
    void tortureSprintfDouble();
    void toNum();
    void iterators();
    void reverseIterators();
    void split_data();
    void split();
#if QT_CONFIG(regularexpression)
    void split_regularexpression_data();
    void split_regularexpression();
#endif
    void fromUtf16_data();
    void fromUtf16();
    void fromUtf16_char16_data() { fromUtf16_data(); }

    void fromUtf16_char16();
    void latin1String();
    void nanAndInf();
    void compare_data();
    void compare();
    void resize();
    void resizeAfterFromRawData();
    void resizeAfterReserve();
    void resizeWithNegative() const;
    void truncateWithNegative() const;
    void QCharRefMutableUnicode() const;
    void QCharRefDetaching() const;
    void repeatedSignature() const;
    void repeated() const;
    void repeated_data() const;
    void arg_locale();
#if QT_CONFIG(icu)
    void toUpperLower_icu();
#endif
    void literals();
    void userDefinedLiterals();
    void eightBitLiterals_data();
    void eightBitLiterals();
    void reserve();
    void toHtmlEscaped_data();
    void toHtmlEscaped();
    void operatorGreaterWithQLatin1String();
    void compareQLatin1Strings();
    void fromQLatin1StringWithLength();
    void assignQLatin1String();
    void assignQChar();
    void isRightToLeft_data();
    void isRightToLeft();
    void isValidUtf16_data();
    void isValidUtf16();
    void unicodeStrings();
    void vasprintfWithPrecision();

    void rawData();
    void clear();
    void sliced();
    void chopped();
    void removeIf();
};
Q_DECLARE_OPERATORS_FOR_FLAGS(tst_QString::DataOptions)


template <class T> const T &verifyZeroTermination(const T &t) { return t; }

QString verifyZeroTermination(const QString &str)
{
    // This test does some evil stuff, it's all supposed to work.

    QString::DataPointer strDataPtr = const_cast<QString &>(str).data_ptr();

    // Skip if isStatic() or fromRawData(), as those offer no guarantees
    if (!strDataPtr->isMutable())
        return str;

    qsizetype strSize = str.size();
    QChar strTerminator = str.constData()[strSize];
    if (QChar('\0') != strTerminator)
        return QString::fromLatin1(
            "*** Result ('%1') not null-terminated: 0x%2 ***").arg(str)
                .arg(strTerminator.unicode(), 4, 16, QChar('0'));

    // Skip mutating checks on shared strings
    if (strDataPtr->isShared())
        return str;

    const QChar *strData = str.constData();
    const QString strCopy(strData, strSize); // Deep copy

    const_cast<QChar *>(strData)[strSize] = QChar('x');
    if (QChar('x') != str.constData()[strSize]) {
        return QString::fromLatin1("*** Failed to replace null-terminator in "
                "result ('%1') ***").arg(str);
    }
    if (str != strCopy) {
        return QString::fromLatin1( "*** Result ('%1') differs from its copy "
                "after null-terminator was replaced ***").arg(str);
    }
    const_cast<QChar *>(strData)[strSize] = QChar('\0'); // Restore sanity

    return str;
}

// Overriding QTest's QCOMPARE, to check QString for null termination
#undef QCOMPARE
#define QCOMPARE(actual, expected)                                      \
    do {                                                                \
        if (!QTest::qCompare(verifyZeroTermination(actual), expected,   \
                #actual, #expected, __FILE__, __LINE__))                \
            return;                                                     \
    } while (0)                                                         \
    /**/
#undef QTEST
#define QTEST(actual, testElement)                                      \
    do {                                                                \
        if (!QTest::qTest(verifyZeroTermination(actual), testElement,   \
                #actual, #testElement, __FILE__, __LINE__))             \
            return;                                                     \
    } while (0)                                                         \
    /**/

typedef QList<int> IntList;

tst_QString::tst_QString()
{
}

void tst_QString::remove_uint_uint_data()
{
    replace_uint_uint_data();
}

void tst_QString::remove_string_data()
{
    replace_string_data();
}

void tst_QString::indexOf3_data()
{
    indexOf2_data();
}

void tst_QString::replace_qchar_qchar_data()
{
    QTest::addColumn<QString>("src" );
    QTest::addColumn<QChar>("before" );
    QTest::addColumn<QChar>("after" );
    QTest::addColumn<Qt::CaseSensitivity>("cs");
    QTest::addColumn<QString>("expected" );

    QTest::newRow("1") << QString("foo") << QChar('o') << QChar('a') << Qt::CaseSensitive
                       << QString("faa");
    QTest::newRow("2") << QString("foo") << QChar('o') << QChar('a') << Qt::CaseInsensitive
                       << QString("faa");
    QTest::newRow("3") << QString("foo") << QChar('O') << QChar('a') << Qt::CaseSensitive
                       << QString("foo");
    QTest::newRow("4") << QString("foo") << QChar('O') << QChar('a') << Qt::CaseInsensitive
                       << QString("faa");
    QTest::newRow("5") << QString("ababABAB") << QChar('a') << QChar(' ') << Qt::CaseSensitive
                       << QString(" b bABAB");
    QTest::newRow("6") << QString("ababABAB") << QChar('a') << QChar(' ') << Qt::CaseInsensitive
                       << QString(" b b B B");
    QTest::newRow("7") << QString("ababABAB") << QChar() << QChar(' ') << Qt::CaseInsensitive
                       << QString("ababABAB");
    QTest::newRow("8") << QString() << QChar() << QChar('x') << Qt::CaseInsensitive << QString();
    QTest::newRow("9") << QString() << QChar('a') << QChar('x') << Qt::CaseInsensitive << QString();
}

void tst_QString::replace_qchar_qchar()
{
    QFETCH(QString, src);
    QFETCH(QChar, before);
    QFETCH(QChar, after);
    QFETCH(Qt::CaseSensitivity, cs);
    QFETCH(QString, expected);

    QCOMPARE(src.replace(before, after, cs), expected);
}

void tst_QString::replace_qchar_qstring_data()
{
    QTest::addColumn<QString>("src" );
    QTest::addColumn<QChar>("before" );
    QTest::addColumn<QString>("after" );
    QTest::addColumn<Qt::CaseSensitivity>("cs");
    QTest::addColumn<QString>("expected" );

    QTest::newRow("1") << QString("foo") << QChar('o') << QString("aA") << Qt::CaseSensitive
                       << QString("faAaA");
    QTest::newRow("2") << QString("foo") << QChar('o') << QString("aA") << Qt::CaseInsensitive
                       << QString("faAaA");
    QTest::newRow("3") << QString("foo") << QChar('O') << QString("aA") << Qt::CaseSensitive
                       << QString("foo");
    QTest::newRow("4") << QString("foo") << QChar('O') << QString("aA") << Qt::CaseInsensitive
                       << QString("faAaA");
    QTest::newRow("5") << QString("ababABAB") << QChar('a') << QString("  ") << Qt::CaseSensitive
                       << QString("  b  bABAB");
    QTest::newRow("6") << QString("ababABAB") << QChar('a') << QString("  ") << Qt::CaseInsensitive
                       << QString("  b  b  B  B");
    QTest::newRow("7") << QString("ababABAB") << QChar() << QString("  ") << Qt::CaseInsensitive
                       << QString("ababABAB");
    QTest::newRow("8") << QString("ababABAB") << QChar() << QString() << Qt::CaseInsensitive
                       << QString("ababABAB");
    QTest::newRow("null-in-null-with-X") << QString() << QChar() << QString("X")
                                         << Qt::CaseSensitive << QString();
    QTest::newRow("x-in-null-with-abc") << QString() << QChar('x') << QString("abc")
                                        << Qt::CaseSensitive << QString();
    QTest::newRow("null-in-empty-with-X") << QString("") << QChar() << QString("X")
                                          << Qt::CaseInsensitive << QString();
    QTest::newRow("x-in-empty-with-abc") << QString("") << QChar('x') << QString("abc")
                                          << Qt::CaseInsensitive << QString();
}

void tst_QString::replace_qchar_qstring()
{
    QFETCH(QString, src);
    QFETCH(QChar, before);
    QFETCH(QString, after);
    QFETCH(Qt::CaseSensitivity, cs);
    QFETCH(QString, expected);

    QCOMPARE(src.replace(before, after, cs), expected);
}

void tst_QString::replace_uint_uint_data()
{
    QTest::addColumn<QString>("string" );
    QTest::addColumn<int>("index" );
    QTest::addColumn<int>("len" );
    QTest::addColumn<QString>("after" );
    QTest::addColumn<QString>("result" );

    QTest::newRow("empty_rem00") << QString() << 0 << 0 << QString("") << QString();
    QTest::newRow("empty_rem01") << QString() << 0 << 3 << QString("") << QString();
    QTest::newRow("empty_rem02") << QString() << 5 << 3 << QString("") << QString();

    QTest::newRow( "rem00" ) << QString("-<>ABCABCABCABC>") << 0 << 3 << QString("") << QString("ABCABCABCABC>");
    QTest::newRow( "rem01" ) << QString("ABCABCABCABC>") << 1 << 4 << QString("") << QString("ACABCABC>");
    QTest::newRow( "rem04" ) << QString("ACABCABC>") << 8 << 4 << QString("") << QString("ACABCABC");
    QTest::newRow( "rem05" ) << QString("ACABCABC") << 7 << 1 << QString("") << QString("ACABCAB");
    QTest::newRow( "rem06" ) << QString("ACABCAB") << 4 << 0 << QString("") << QString("ACABCAB");

    QTest::newRow("empty_rep00") << QString() << 0 << 0 << QString("X") << QString("X");
    QTest::newRow("empty_rep01") << QString() << 0 << 3 << QString("X") << QString("X");
    QTest::newRow("empty_rep02") << QString() << 5 << 3 << QString("X") << QString();

    QTest::newRow( "rep00" ) << QString("ACABCAB") << 4 << 0 << QString("X") << QString("ACABXCAB");
    QTest::newRow( "rep01" ) << QString("ACABXCAB") << 4 << 1 << QString("Y") << QString("ACABYCAB");
    QTest::newRow( "rep02" ) << QString("ACABYCAB") << 4 << 1 << QString("") << QString("ACABCAB");
    QTest::newRow( "rep03" ) << QString("ACABCAB") << 0 << 9999 << QString("XX") << QString("XX");
    QTest::newRow( "rep04" ) << QString("XX") << 0 << 9999 << QString("") << QString("");
    QTest::newRow( "rep05" ) << QString("ACABCAB") << 0 << 2 << QString("XX") << QString("XXABCAB");
    QTest::newRow( "rep06" ) << QString("ACABCAB") << 1 << 2 << QString("XX") << QString("AXXBCAB");
    QTest::newRow( "rep07" ) << QString("ACABCAB") << 2 << 2 << QString("XX") << QString("ACXXCAB");
    QTest::newRow( "rep08" ) << QString("ACABCAB") << 3 << 2 << QString("XX") << QString("ACAXXAB");
    QTest::newRow( "rep09" ) << QString("ACABCAB") << 4 << 2 << QString("XX") << QString("ACABXXB");
    QTest::newRow( "rep10" ) << QString("ACABCAB") << 5 << 2 << QString("XX") << QString("ACABCXX");
    QTest::newRow( "rep11" ) << QString("ACABCAB") << 6 << 2 << QString("XX") << QString("ACABCAXX");
    QTest::newRow( "rep12" ) << QString() << 0 << 10 << QString("X") << QString("X");
    QTest::newRow( "rep13" ) << QString("short") << 0 << 10 << QString("X") << QString("X");
    QTest::newRow( "rep14" ) << QString() << 0 << 10 << QString("XX") << QString("XX");
    QTest::newRow( "rep15" ) << QString("short") << 0 << 10 << QString("XX") << QString("XX");

    // This is a regression test for an old bug where QString would add index and len parameters,
    // potentially causing integer overflow.
    QTest::newRow( "no overflow" ) << QString("ACABCAB") << 1 << INT_MAX - 1 << QString("") << QString("A");
    QTest::newRow( "overflow" ) << QString("ACABCAB") << 1 << INT_MAX << QString("") << QString("A");
}

void tst_QString::replace_string_data()
{
    QTest::addColumn<QString>("string" );
    QTest::addColumn<QString>("before" );
    QTest::addColumn<QString>("after" );
    QTest::addColumn<QString>("result" );
    QTest::addColumn<bool>("bcs" );

    QTest::newRow( "rem00" ) << QString("") << QString("") << QString("") << QString("") << true;
    QTest::newRow( "rem01" ) << QString("A") << QString("") << QString("") << QString("A") << true;
    QTest::newRow( "rem02" ) << QString("A") << QString("A") << QString("") << QString("") << true;
    QTest::newRow( "rem03" ) << QString("A") << QString("B") << QString("") << QString("A") << true;
    QTest::newRow( "rem04" ) << QString("AA") << QString("A") << QString("") << QString("") << true;
    QTest::newRow( "rem05" ) << QString("AB") << QString("A") << QString("") << QString("B") << true;
    QTest::newRow( "rem06" ) << QString("AB") << QString("B") << QString("") << QString("A") << true;
    QTest::newRow( "rem07" ) << QString("AB") << QString("C") << QString("") << QString("AB") << true;
    QTest::newRow( "rem08" ) << QString("ABA") << QString("A") << QString("") << QString("B") << true;
    QTest::newRow( "rem09" ) << QString("ABA") << QString("B") << QString("") << QString("AA") << true;
    QTest::newRow( "rem10" ) << QString("ABA") << QString("C") << QString("") << QString("ABA") << true;
    QTest::newRow( "rem11" ) << QString("banana") << QString("an") << QString("") << QString("ba") << true;
    QTest::newRow( "rem12" ) << QString("") << QString("A") << QString("") << QString("") << true;
    QTest::newRow( "rem13" ) << QString("") << QString("A") << QString() << QString("") << true;
    QTest::newRow( "rem14" ) << QString() << QString("A") << QString("") << QString() << true;
    QTest::newRow( "rem15" ) << QString() << QString("A") << QString() << QString() << true;
    QTest::newRow( "rem16" ) << QString() << QString("") << QString("") << QString("") << true;
    QTest::newRow( "rem17" ) << QString("") << QString() << QString("") << QString("") << true;
    QTest::newRow( "rem18" ) << QString("a") << QString("a") << QString("") << QString("") << false;
    QTest::newRow( "rem19" ) << QString("A") << QString("A") << QString("") << QString("") << false;
    QTest::newRow( "rem20" ) << QString("a") << QString("A") << QString("") << QString("") << false;
    QTest::newRow( "rem21" ) << QString("A") << QString("a") << QString("") << QString("") << false;
    QTest::newRow( "rem22" ) << QString("Alpha beta") << QString("a") << QString("") << QString("lph bet") << false;

    QTest::newRow( "rep00" ) << QString("ABC") << QString("B") << QString("-") << QString("A-C") << true;
    QTest::newRow( "rep01" ) << QString("$()*+.?[\\]^{|}") << QString("$()*+.?[\\]^{|}") << QString("X") << QString("X") << true;
    QTest::newRow( "rep02" ) << QString("ABCDEF") << QString("") << QString("X") << QString("XAXBXCXDXEXFX") << true;
    QTest::newRow( "rep03" ) << QString("") << QString("") << QString("X") << QString("X") << true;
    QTest::newRow( "rep04" ) << QString("a") << QString("a") << QString("b") << QString("b") << false;
    QTest::newRow( "rep05" ) << QString("A") << QString("A") << QString("b") << QString("b") << false;
    QTest::newRow( "rep06" ) << QString("a") << QString("A") << QString("b") << QString("b") << false;
    QTest::newRow( "rep07" ) << QString("A") << QString("a") << QString("b") << QString("b") << false;
    QTest::newRow( "rep08" ) << QString("a") << QString("a") << QString("a") << QString("a") << false;
    QTest::newRow( "rep09" ) << QString("A") << QString("A") << QString("a") << QString("a") << false;
    QTest::newRow( "rep10" ) << QString("a") << QString("A") << QString("a") << QString("a") << false;
    QTest::newRow( "rep11" ) << QString("A") << QString("a") << QString("a") << QString("a") << false;
    QTest::newRow( "rep12" ) << QString("Alpha beta") << QString("a") << QString("o") << QString("olpho beto") << false;
    QTest::newRow( "rep13" ) << QString() << QString("") << QString("A") << QString("A") << true;
    QTest::newRow( "rep14" ) << QString("") << QString() << QString("A") << QString("A") << true;
    QTest::newRow( "rep15" ) << QString("fooxbarxbazxblub") << QString("x") << QString("yz") << QString("fooyzbaryzbazyzblub") << true;
    QTest::newRow( "rep16" ) << QString("fooxbarxbazxblub") << QString("x") << QString("z") << QString("foozbarzbazzblub") << true;
    QTest::newRow( "rep17" ) << QString("fooxybarxybazxyblub") << QString("xy") << QString("z") << QString("foozbarzbazzblub") << true;
    QTest::newRow("rep18") << QString() << QString() << QString("X") << QString("X") << false;
    QTest::newRow("rep19") << QString() << QString("A") << QString("X") << QString("") << false;
}

#if QT_CONFIG(regularexpression)
void tst_QString::replace_regexp_data()
{
    remove_regexp_data(); // Sets up the columns, adds rows with empty replacement text.
    // Columns (all QString): string, regexp, after, result; string.replace(regexp, after) == result
    // Test-cases with empty after (replacement text, third column) go in remove_regexp_data()

    QTest::newRow("empty-in-null") << QString() << "" << "after" << "after";
    QTest::newRow("empty-in-empty") << "" << "" << "after" << "after";

    QTest::newRow( "rep00" ) << QString("A <i>bon mot</i>.") << QString("<i>([^<]*)</i>") << QString("\\emph{\\1}") << QString("A \\emph{bon mot}.");
    QTest::newRow( "rep01" ) << QString("banana") << QString("^.a()") << QString("\\1") << QString("nana");
    QTest::newRow( "rep02" ) << QString("banana") << QString("(ba)") << QString("\\1X\\1") << QString("baXbanana");
    QTest::newRow( "rep03" ) << QString("banana") << QString("(ba)(na)na") << QString("\\2X\\1") << QString("naXba");
    QTest::newRow("rep04") << QString() << QString("(ba)") << QString("\\1X\\1") << QString();

    QTest::newRow("backref00") << QString("\\1\\2\\3\\4\\5\\6\\7\\8\\9\\A\\10\\11") << QString("\\\\[34]")
                               << QString("X") << QString("\\1\\2XX\\5\\6\\7\\8\\9\\A\\10\\11");
    QTest::newRow("backref01") << QString("foo") << QString("[fo]") << QString("\\1") << QString("\\1\\1\\1");
    QTest::newRow("backref02") << QString("foo") << QString("([fo])") << QString("(\\1)") << QString("(f)(o)(o)");
    QTest::newRow("backref03") << QString("foo") << QString("([fo])") << QString("\\2") << QString("\\2\\2\\2");
    QTest::newRow("backref04") << QString("foo") << QString("([fo])") << QString("\\10") << QString("f0o0o0");
    QTest::newRow("backref05") << QString("foo") << QString("([fo])") << QString("\\11") << QString("f1o1o1");
    QTest::newRow("backref06") << QString("foo") << QString("([fo])") << QString("\\19") << QString("f9o9o9");
    QTest::newRow("backref07") << QString("foo") << QString("(f)(o+)")
                               << QString("\\2\\1\\10\\20\\11\\22\\19\\29\\3")
                               << QString("ooff0oo0f1oo2f9oo9\\3");
    QTest::newRow("backref08") << QString("abc") << QString("(((((((((((((([abc]))))))))))))))")
                               << QString("{\\14}") << QString("{a}{b}{c}");
    QTest::newRow("backref09") << QString("abcdefghijklmn")
                               << QString("(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)(k)(l)(m)(n)")
                               << QString("\\19\\18\\17\\16\\15\\14\\13\\12\\11\\10"
                                          "\\9\\90\\8\\80\\7\\70\\6\\60\\5\\50\\4\\40\\3\\30\\2\\20\\1")
                               << QString("a9a8a7a6a5nmlkjii0hh0gg0ff0ee0dd0cc0bb0a");
    QTest::newRow("backref10") << QString("abc") << QString("((((((((((((((abc))))))))))))))")
                               << QString("\\0\\01\\011") << QString("\\0\\01\\011");
}
#endif

void tst_QString::utf8_data()
{
    QString str;
    QTest::addColumn<QByteArray>("utf8" );
    QTest::addColumn<QString>("res" );

    QTest::newRow("null") << QByteArray() << QString();
    QTest::newRow("empty") << QByteArray("") << QString("");

    QTest::newRow( "str0" ) << QByteArray("abcdefgh")
                          << QString("abcdefgh");
    QTest::newRow( "str1" ) << QByteArray("\303\266\303\244\303\274\303\226\303\204\303\234\303\270\303\246\303\245\303\230\303\206\303\205")
                          << QString::fromLatin1("\366\344\374\326\304\334\370\346\345\330\306\305") ;
    str += QChar( 0x05e9 );
    str += QChar( 0x05d3 );
    str += QChar( 0x05d2 );
    QTest::newRow( "str2" ) << QByteArray("\327\251\327\223\327\222")
                          << str;

    str = QChar( 0x20ac );
    str += " some text";
    QTest::newRow( "str3" ) << QByteArray("\342\202\254 some text")
                          << str;

    str = "Old Italic: ";
    str += QChar(0xd800);
    str += QChar(0xdf00);
    str += QChar(0xd800);
    str += QChar(0xdf01);
    str += QChar(0xd800);
    str += QChar(0xdf02);
    str += QChar(0xd800);
    str += QChar(0xdf03);
    str += QChar(0xd800);
    str += QChar(0xdf04);
    QTest::newRow("surrogate") << QByteArray("Old Italic: \360\220\214\200\360\220\214\201\360\220\214\202\360\220\214\203\360\220\214\204") << str;
}

void tst_QString::length_data()
{
    QTest::addColumn<QString>("s1");
    QTest::addColumn<qsizetype>("res");

    QTest::newRow("null") << QString() << qsizetype(0);
    QTest::newRow("empty") << QString("") << qsizetype(0);
    QTest::newRow("data0") << QString("Test") << qsizetype(4);
    QTest::newRow("data1") << QString("The quick brown fox jumps over the lazy dog")
                           << qsizetype(43);
    QTest::newRow("data3") << QString("A") << qsizetype(1);
    QTest::newRow("data4") << QString("AB") << qsizetype(2);
    QTest::newRow("data5") << QString("AB\n") << qsizetype(3);
    QTest::newRow("data6") << QString("AB\nC") << qsizetype(4);
    QTest::newRow("data7") << QString("\n") << qsizetype(1);
    QTest::newRow("data8") << QString("\nA") << qsizetype(2);
    QTest::newRow("data9") << QString("\nAB") << qsizetype(3);
    QTest::newRow("data10") << QString("\nAB\nCDE") << qsizetype(7);
    QTest::newRow("data11") << QString("shdnftrheid fhgnt gjvnfmd chfugkh bnfhg thgjf vnghturkf "
                                       "chfnguh bjgnfhvygh hnbhgutjfv dhdnjds dcjs d")
                            << qsizetype(100);
}

void tst_QString::length()
{
    // size(), length() and count() do the same
    QFETCH(QString, s1);
    QTEST(s1.length(), "res");
    QTEST(s1.size(), "res");
    QTEST(s1.count(), "res");
}

#include <qfile.h>

void tst_QString::acc_01()
{
    QString a;
    QString b; //b(10);
    QString bb; //bb((int)0);
    QString c("String C");
    QChar tmp[10];
    tmp[0] = 'S';
    tmp[1] = 't';
    tmp[2] = 'r';
    tmp[3] = 'i';
    tmp[4] = 'n';
    tmp[5] = 'g';
    tmp[6] = ' ';
    tmp[7] = 'D';
    tmp[8] = 'X';
    tmp[9] = '\0';
    QString d(tmp,8);
    QString ca(a);
    QString cb(b);
    QString cc(c);
    QString n;
    QString e("String E");
    QString f;
    f = e;
    f[7]='F';
    QCOMPARE(e, QLatin1String("String E"));
    char text[]="String f";
    f = text;
    text[7]='!';
    QCOMPARE(f, QLatin1String("String f"));
    f[7]='F';
    QCOMPARE(text[7],'!');

    a="123";
    b="456";
    a[0]=a[1];
    QCOMPARE(a, QLatin1String("223"));
    a[1]=b[1];
    QCOMPARE(b, QLatin1String("456"));
    QCOMPARE(a, QLatin1String("253"));

    char t[]="TEXT";
    a="A";
    a=t;
    QCOMPARE(a, QLatin1String("TEXT"));
    QCOMPARE(a,(QString)t);
    a[0]='X';
    QCOMPARE(a, QLatin1String("XEXT"));
    QCOMPARE(t[0],'T');
    t[0]='Z';
    QCOMPARE(a, QLatin1String("XEXT"));

    a="ABC";
    QCOMPARE(char(a.toLatin1()[1]),'B');
    QCOMPARE(strcmp(a.toLatin1(), QByteArrayLiteral("ABC")), 0);
    QCOMPARE(a+="DEF", QLatin1String("ABCDEF"));
    QCOMPARE(a+='G', QLatin1String("ABCDEFG"));
    QCOMPARE(a+=((const char*)(0)), QLatin1String("ABCDEFG"));

    // non-member operators

    a="ABC";
    b="ABC";
    c="ACB";
    d="ABCD";
    QVERIFY(a==b);
    QVERIFY(!(a==d));
    QVERIFY(!(a!=b));
    QVERIFY(a!=d);
    QVERIFY(!(a<b));
    QVERIFY(a<c);
    QVERIFY(a<d);
    QVERIFY(!(d<a));
    QVERIFY(!(c<a));
    QVERIFY(a<=b);
    QVERIFY(a<=d);
    QVERIFY(a<=c);
    QVERIFY(!(c<=a));
    QVERIFY(!(d<=a));
    QCOMPARE(QString(a+b), QLatin1String("ABCABC"));
    QCOMPARE(QString(a+"XXXX"), QLatin1String("ABCXXXX"));
    QCOMPARE(QString(a+'X'), QLatin1String("ABCX"));
    QCOMPARE(QString("XXXX"+a), QLatin1String("XXXXABC"));
    QCOMPARE(QString('X'+a), QLatin1String("XABC"));
    a = (const char*)0;
    QVERIFY(a.isNull());
    QVERIFY(*a.toLatin1().constData() == '\0');
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wformat-security")
QT_WARNING_DISABLE_CLANG("-Wformat-security")

void tst_QString::isNull()
{
    QString a;
    QVERIFY(a.isNull());
    QVERIFY(!a.isDetached());

    const char *zero = nullptr;
    QVERIFY(!QString::asprintf(zero).isNull());
}

QT_WARNING_POP

void tst_QString::isEmpty()
{
    QString a;
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    QString c("Not empty");
    QVERIFY(!c.isEmpty());
}

void tst_QString::constructor()
{
    QString a;
    QString b; //b(10);
    QString c("String C");
    QChar tmp[10];
    tmp[0] = 'S';
    tmp[1] = 't';
    tmp[2] = 'r';
    tmp[3] = 'i';
    tmp[4] = 'n';
    tmp[5] = 'g';
    tmp[6] = ' ';
    tmp[7] = 'D';
    tmp[8] = 'X';
    tmp[9] = '\0';
    QString d(tmp,8);
    QString ca(a);
    QString cb(b);
    QString cc(c);

    QCOMPARE(a,ca);
    QVERIFY(a.isNull());
    QVERIFY(a == (QString)"");
    QCOMPARE(b,cb);
    QCOMPARE(c,cc);
    QCOMPARE(d, QLatin1String("String D"));

    QString nullStr;
    QVERIFY( nullStr.isNull() );
    QVERIFY( nullStr.isEmpty() );
    QString empty("");
    QVERIFY( !empty.isNull() );
    QVERIFY( empty.isEmpty() );
}

void tst_QString::constructorQByteArray_data()
{
    QTest::addColumn<QByteArray>("src" );
    QTest::addColumn<QString>("expected" );

    QByteArray ba( 4, 0 );
    ba[0] = 'C';
    ba[1] = 'O';
    ba[2] = 'M';
    ba[3] = 'P';

    QTest::newRow( "1" ) << ba << QString("COMP");

    QByteArray ba1( 7, 0 );
    ba1[0] = 'a';
    ba1[1] = 'b';
    ba1[2] = 'c';
    ba1[3] = '\0';
    ba1[4] = 'd';
    ba1[5] = 'e';
    ba1[6] = 'f';

    QTest::newRow( "2" ) << ba1 << QString::fromUtf16(u"abc\0def", 7);

    QTest::newRow( "3" ) << QByteArray::fromRawData("abcd", 3) << QString("abc");
    QTest::newRow( "4" ) << QByteArray("\xc3\xa9") << QString("\xc3\xa9");
    QTest::newRow( "4-bis" ) << QByteArray("\xc3\xa9") << QString::fromUtf8("\xc3\xa9");
    QTest::newRow( "4-tre" ) << QByteArray("\xc3\xa9") << QString::fromLatin1("\xe9");
}

void tst_QString::constructorQByteArray()
{
    QFETCH(QByteArray, src);
    QFETCH(QString, expected);

    QString strBA(src);
    QCOMPARE( strBA, expected );

    // test operator= too
    strBA.clear();
    strBA = src;
    QCOMPARE( strBA, expected );

    // test constructor/operator=(const char *)
    if (src.constData()[src.length()] == '\0') {
        qsizetype zero = expected.indexOf(QLatin1Char('\0'));
        if (zero < 0)
            zero = expected.length();

        QString str1(src.constData());
        QCOMPARE(str1.length(), zero);
        QCOMPARE(str1, expected.left(zero));

        str1.clear();
        str1 = src.constData();
        QCOMPARE(str1, expected.left(zero));
    }
}

void tst_QString::STL()
{
    QString nullStr;
    QVERIFY(nullStr.toStdWString().empty());
    QVERIFY(!nullStr.isDetached());

    wchar_t dataArray[] = { 'w', 'o', 'r', 'l', 'd', 0 };

    QCOMPARE(nullStr.toWCharArray(dataArray), 0);
    QVERIFY(dataArray[0] == 'w'); // array was not modified
    QVERIFY(!nullStr.isDetached());

    QString emptyStr("");
    QVERIFY(emptyStr.toStdWString().empty());
    QVERIFY(!emptyStr.isDetached());

    QCOMPARE(emptyStr.toWCharArray(dataArray), 0);
    QVERIFY(dataArray[0] == 'w'); // array was not modified
    QVERIFY(!emptyStr.isDetached());

    std::string stdstr( "QString" );

    QString stlqt = QString::fromStdString(stdstr);
    QCOMPARE(stlqt, QString::fromLatin1(stdstr.c_str()));
    QCOMPARE(stlqt.toStdString(), stdstr);

    const wchar_t arr[] = {'h', 'e', 'l', 'l', 'o', 0};
    std::wstring stlStr = arr;

    QString s = QString::fromStdWString(stlStr);

    QCOMPARE(s, QString::fromLatin1("hello"));
    QCOMPARE(stlStr, s.toStdWString());

    // replacing the content of dataArray by calling toWCharArray()
    QCOMPARE(s.toWCharArray(dataArray), 5);
    const std::wstring stlStrFromUpdatedArray = dataArray;
    QCOMPARE(stlStrFromUpdatedArray, stlStr);
}

void tst_QString::macTypes()
{
#ifndef Q_OS_MAC
    QSKIP("This is a Mac-only test");
#else
    extern void tst_QString_macTypes(); // in qcore_foundation.mm
    tst_QString_macTypes();
#endif
}

void tst_QString::truncate()
{
    QString nullStr;
    nullStr.truncate(5);
    QVERIFY(nullStr.isEmpty());
    nullStr.truncate(0);
    QVERIFY(nullStr.isEmpty());
    nullStr.truncate(-3);
    QVERIFY(nullStr.isEmpty());
    QVERIFY(!nullStr.isDetached());

    QString emptyStr("");
    emptyStr.truncate(5);
    QVERIFY(emptyStr.isEmpty());
    emptyStr.truncate(0);
    QVERIFY(emptyStr.isEmpty());
    emptyStr.truncate(-3);
    QVERIFY(emptyStr.isEmpty());
    QVERIFY(!emptyStr.isDetached());

    QString e("String E");
    e.truncate(4);
    QCOMPARE(e, QLatin1String("Stri"));

    e = "String E";
    e.truncate(0);
    QCOMPARE(e, QLatin1String(""));
    QVERIFY(e.isEmpty());
    QVERIFY(!e.isNull());

}

void tst_QString::chop_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("count" );
    QTest::addColumn<QString>("result");

    const QString original("abcd");

    QTest::newRow("null chop 1") << QString() << 1 << QString();
    QTest::newRow("null chop -1") << QString() << -1 << QString();
    QTest::newRow("empty chop 1") << QString("") << 1 << QString("");
    QTest::newRow("empty chop -1") << QString("") << -1 << QString("");
    QTest::newRow("data0") << original << 1 << QString("abc");
    QTest::newRow("data1") << original << 0 << original;
    QTest::newRow("data2") << original << -1 << original;
    QTest::newRow("data3") << original << int(original.size()) << QString();
    QTest::newRow("data4") << original << 1000  << QString();
}

void tst_QString::chop()
{
    QFETCH(QString, input);
    QFETCH(int, count);
    QFETCH(QString, result);

    input.chop(count);
    QCOMPARE(input, result);
}

void tst_QString::fill()
{
    QString e;
    e.fill('e',1);
    QCOMPARE(e, QLatin1String("e"));
    QString f;
    f.fill('f',3);
    QCOMPARE(f, QLatin1String("fff"));
    f.fill('F');
    QCOMPARE(f, QLatin1String("FFF"));
    f.fill('a', 2);
    QCOMPARE(f, QLatin1String("aa"));
}

static inline const void *ptrValue(quintptr v)
{
    return reinterpret_cast<const void *>(v);
}

void tst_QString::asprintf()
{
    QString a;
    QCOMPARE(QString::asprintf("COMPARE"), QLatin1String("COMPARE"));
    QCOMPARE(QString::asprintf("%%%d", 1), QLatin1String("%1"));
    QCOMPARE(QString::asprintf("X%dY",2), QLatin1String("X2Y"));
    QCOMPARE(QString::asprintf("X%9iY", 50000 ), QLatin1String("X    50000Y"));
    QCOMPARE(QString::asprintf("X%-9sY","hello"), QLatin1String("Xhello    Y"));
    QCOMPARE(QString::asprintf("X%-9iY", 50000 ), QLatin1String("X50000    Y"));
    QCOMPARE(QString::asprintf("%lf", 1.23), QLatin1String("1.230000"));
    QCOMPARE(QString::asprintf("%lf", 1.23456789), QLatin1String("1.234568"));
    QCOMPARE(QString::asprintf("%p", ptrValue(0xbfffd350)), QLatin1String("0xbfffd350"));
    QCOMPARE(QString::asprintf("%p", ptrValue(0)), QLatin1String("0x0"));
    QCOMPARE(QString::asprintf("%td", ptrdiff_t(6)), QString::fromLatin1("6"));
    QCOMPARE(QString::asprintf("%td", ptrdiff_t(-6)), QString::fromLatin1("-6"));
    QCOMPARE(QString::asprintf("%zu", size_t(6)), QString::fromLatin1("6"));
    QCOMPARE(QString::asprintf("%zu", size_t(1) << 31), QString::fromLatin1("2147483648"));

    // cross z and t
    using ssize_t = std::make_signed<size_t>::type;         // should be ptrdiff_t
    using uptrdiff_t = std::make_unsigned<ptrdiff_t>::type; // should be size_t
    QCOMPARE(QString::asprintf("%tu", uptrdiff_t(6)), QString::fromLatin1("6"));
    QCOMPARE(QString::asprintf("%tu", uptrdiff_t(1) << 31), QString::fromLatin1("2147483648"));
    QCOMPARE(QString::asprintf("%zd", ssize_t(-6)), QString::fromLatin1("-6"));

    if (sizeof(qsizetype) > sizeof(int)) {
        // 64-bit test
        QCOMPARE(QString::asprintf("%zu", SIZE_MAX), QString::fromLatin1("18446744073709551615"));
        QCOMPARE(QString::asprintf("%td", PTRDIFF_MAX), QString::fromLatin1("9223372036854775807"));
        QCOMPARE(QString::asprintf("%td", PTRDIFF_MIN), QString::fromLatin1("-9223372036854775808"));

        // sign extension is easy, make sure we can get something middle-ground
        // (24 + 8 = 32; addition used to avoid warning about shifting more
        // than size type on 32-bit systems)
        size_t ubig = size_t(1) << (24 + sizeof(size_t));
        ptrdiff_t sbig = ptrdiff_t(1) << (24 + sizeof(ptrdiff_t));
        QCOMPARE(QString::asprintf("%zu", ubig), QString::fromLatin1("4294967296"));
        QCOMPARE(QString::asprintf("%td", sbig), QString::fromLatin1("4294967296"));
        QCOMPARE(QString::asprintf("%td", -sbig), QString::fromLatin1("-4294967296"));
    }

    int i = 6;
    long l = -2;
    float f = 4.023f;
    QCOMPARE(QString::asprintf("%d %ld %f", i, l, f), QLatin1String("6 -2 4.023000"));

    double d = -514.25683;
    QCOMPARE(QString::asprintf("%f", d), QLatin1String("-514.256830"));
    QCOMPARE(QString::asprintf("%.f", d), QLatin1String("-514"));
    QCOMPARE(QString::asprintf("%.0f", d), QLatin1String("-514"));
    QCOMPARE(QString::asprintf("%1f", d), QLatin1String("-514.256830"));
    QCOMPARE(QString::asprintf("%1.f", d), QLatin1String("-514"));
    QCOMPARE(QString::asprintf("%1.0f", d), QLatin1String("-514"));
    QCOMPARE(QString::asprintf("%1.6f", d), QLatin1String("-514.256830"));
    QCOMPARE(QString::asprintf("%1.10f", d), QLatin1String("-514.2568300000"));
    QCOMPARE(QString::asprintf("%-1f", d), QLatin1String("-514.256830"));
    QCOMPARE(QString::asprintf("%-1.f", d), QLatin1String("-514"));
    QCOMPARE(QString::asprintf("%-1.0f", d), QLatin1String("-514"));
    QCOMPARE(QString::asprintf("%-1.6f", d), QLatin1String("-514.256830"));
    QCOMPARE(QString::asprintf("%-1.10f", d), QLatin1String("-514.2568300000"));
    QCOMPARE(QString::asprintf("%10f", d), QLatin1String("-514.256830"));
    QCOMPARE(QString::asprintf("%10.f", d), QLatin1String("      -514"));
    QCOMPARE(QString::asprintf("%10.0f", d), QLatin1String("      -514"));
    QCOMPARE(QString::asprintf("%-10f", d), QLatin1String("-514.256830"));
    QCOMPARE(QString::asprintf("%-10.f", d), QLatin1String("-514      "));
    QCOMPARE(QString::asprintf("%-10.0f", d), QLatin1String("-514      "));
    QCOMPARE(QString::asprintf("%010f", d), QLatin1String("-514.256830"));
    QCOMPARE(QString::asprintf("%010.f", d), QLatin1String("-000000514"));
    QCOMPARE(QString::asprintf("%010.0f", d), QLatin1String("-000000514"));
    QCOMPARE(QString::asprintf("%15f", d), QLatin1String("    -514.256830"));
    QCOMPARE(QString::asprintf("%15.6f", d), QLatin1String("    -514.256830"));
    QCOMPARE(QString::asprintf("%15.10f", d), QLatin1String("-514.2568300000"));
    QCOMPARE(QString::asprintf("%-15f", d), QLatin1String("-514.256830    "));
    QCOMPARE(QString::asprintf("%-15.6f", d), QLatin1String("-514.256830    "));
    QCOMPARE(QString::asprintf("%-15.10f", d), QLatin1String("-514.2568300000"));
    QCOMPARE(QString::asprintf("%015f", d), QLatin1String("-0000514.256830"));
    QCOMPARE(QString::asprintf("%015.6f", d), QLatin1String("-0000514.256830"));
    QCOMPARE(QString::asprintf("%015.10f", d), QLatin1String("-514.2568300000"));
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wformat")
QT_WARNING_DISABLE_CLANG("-Wformat") // Flag '0' ignored when flag '-' is present
    QCOMPARE(QString::asprintf("%-015f", d), QLatin1String("-514.256830    "));
    QCOMPARE(QString::asprintf("%-015.6f", d), QLatin1String("-514.256830    "));
    QCOMPARE(QString::asprintf("%-015.10f", d), QLatin1String("-514.2568300000"));
QT_WARNING_POP

    {
        /* This code crashed. I don't know how to reduce it further. In other words,
         * both %zu and %s needs to be present. */
        size_t s = 6;
        QCOMPARE(QString::asprintf("%zu%s", s, "foo"), QString::fromLatin1("6foo"));
        QCOMPARE(QString::asprintf("%zu %s\n", s, "foo"), QString::fromLatin1("6 foo\n"));
    }
}

void tst_QString::asprintfS()
{
    QCOMPARE(QString::asprintf("%.3s", "Hello" ), QLatin1String("Hel"));
    QCOMPARE(QString::asprintf("%10.3s", "Hello" ), QLatin1String("       Hel"));
    QCOMPARE(QString::asprintf("%.10s", "Hello" ), QLatin1String("Hello"));
    QCOMPARE(QString::asprintf("%10.10s", "Hello" ), QLatin1String("     Hello"));
    QCOMPARE(QString::asprintf("%-10.10s", "Hello" ), QLatin1String("Hello     "));
    QCOMPARE(QString::asprintf("%-10.3s", "Hello" ), QLatin1String("Hel       "));
    QCOMPARE(QString::asprintf("%-5.5s", "Hello" ), QLatin1String("Hello"));
    QCOMPARE(QString::asprintf("%*s", 4, "Hello"), QLatin1String("Hello"));
    QCOMPARE(QString::asprintf("%*s", 10, "Hello"), QLatin1String("     Hello"));
    QCOMPARE(QString::asprintf("%-*s", 10, "Hello"), QLatin1String("Hello     "));

    // Check utf8 conversion for %s
    QCOMPARE(QString::asprintf("%s", "\303\266\303\244\303\274\303\226\303\204\303\234\303\270\303\246\303\245\303\230\303\206\303\205"), QString::fromLatin1("\366\344\374\326\304\334\370\346\345\330\306\305"));

    int n1;
    QCOMPARE(QString::asprintf("%s%n%s", "hello", &n1, "goodbye"), QString("hellogoodbye"));
    QCOMPARE(n1, 5);
    qlonglong n2;
    QCOMPARE(QString::asprintf("%s%s%lln%s", "foo", "bar", &n2, "whiz"), QString("foobarwhiz"));
    QCOMPARE((int)n2, 6);

    { // %ls
        QCOMPARE(QString::asprintf("%.3ls",     qUtf16Printable("Hello")), QLatin1String("Hel"));
        QCOMPARE(QString::asprintf("%10.3ls",   qUtf16Printable("Hello")), QLatin1String("       Hel"));
        QCOMPARE(QString::asprintf("%.10ls",    qUtf16Printable("Hello")), QLatin1String("Hello"));
        QCOMPARE(QString::asprintf("%10.10ls",  qUtf16Printable("Hello")), QLatin1String("     Hello"));
        QCOMPARE(QString::asprintf("%-10.10ls", qUtf16Printable("Hello")), QLatin1String("Hello     "));
        QCOMPARE(QString::asprintf("%-10.3ls",  qUtf16Printable("Hello")), QLatin1String("Hel       "));
        QCOMPARE(QString::asprintf("%-5.5ls",   qUtf16Printable("Hello")), QLatin1String("Hello"));
        QCOMPARE(QString::asprintf("%*ls",   4, qUtf16Printable("Hello")), QLatin1String("Hello"));
        QCOMPARE(QString::asprintf("%*ls",  10, qUtf16Printable("Hello")), QLatin1String("     Hello"));
        QCOMPARE(QString::asprintf("%-*ls", 10, qUtf16Printable("Hello")), QLatin1String("Hello     "));

        // Check utf16 is preserved for %ls
        QCOMPARE(QString::asprintf("%ls",
                           qUtf16Printable("\303\266\303\244\303\274\303\226\303\204\303\234\303\270\303\246\303\245\303\230\303\206\303\205")),
                 QLatin1String("\366\344\374\326\304\334\370\346\345\330\306\305"));

        int n;
        QCOMPARE(QString::asprintf("%ls%n%s", qUtf16Printable("hello"), &n, "goodbye"), QLatin1String("hellogoodbye"));
        QCOMPARE(n, 5);
    }
}

/*
    indexOf() and indexOf02() test QString::indexOf(),
    QString::lastIndexOf(), and their QByteArray equivalents.

    lastIndexOf() tests QString::lastIndexOf() more in depth, but it
    should probably be rewritten to use a data table.
*/

void tst_QString::indexOf_data()
{
    QTest::addColumn<QString>("haystack" );
    QTest::addColumn<QString>("needle" );
    QTest::addColumn<int>("startpos" );
    QTest::addColumn<bool>("bcs" );
    QTest::addColumn<int>("resultpos" );

    QTest::newRow( "data0" ) << QString("abc") << QString("a") << 0 << true << 0;
    QTest::newRow( "data1" ) << QString("abc") << QString("a") << 0 << false << 0;
    QTest::newRow( "data2" ) << QString("abc") << QString("A") << 0 << true << -1;
    QTest::newRow( "data3" ) << QString("abc") << QString("A") << 0 << false << 0;
    QTest::newRow( "data4" ) << QString("abc") << QString("a") << 1 << true << -1;
    QTest::newRow( "data5" ) << QString("abc") << QString("a") << 1 << false << -1;
    QTest::newRow( "data6" ) << QString("abc") << QString("A") << 1 << true << -1;
    QTest::newRow( "data7" ) << QString("abc") << QString("A") << 1 << false << -1;
    QTest::newRow( "data8" ) << QString("abc") << QString("b") << 0 << true << 1;
    QTest::newRow( "data9" ) << QString("abc") << QString("b") << 0 << false << 1;
    QTest::newRow( "data10" ) << QString("abc") << QString("B") << 0 << true << -1;
    QTest::newRow( "data11" ) << QString("abc") << QString("B") << 0 << false << 1;
    QTest::newRow( "data12" ) << QString("abc") << QString("b") << 1 << true << 1;
    QTest::newRow( "data13" ) << QString("abc") << QString("b") << 1 << false << 1;
    QTest::newRow( "data14" ) << QString("abc") << QString("B") << 1 << true << -1;
    QTest::newRow( "data15" ) << QString("abc") << QString("B") << 1 << false << 1;
    QTest::newRow( "data16" ) << QString("abc") << QString("b") << 2 << true << -1;
    QTest::newRow( "data17" ) << QString("abc") << QString("b") << 2 << false << -1;

    QTest::newRow( "data20" ) << QString("ABC") << QString("A") << 0 << true << 0;
    QTest::newRow( "data21" ) << QString("ABC") << QString("A") << 0 << false << 0;
    QTest::newRow( "data22" ) << QString("ABC") << QString("a") << 0 << true << -1;
    QTest::newRow( "data23" ) << QString("ABC") << QString("a") << 0 << false << 0;
    QTest::newRow( "data24" ) << QString("ABC") << QString("A") << 1 << true << -1;
    QTest::newRow( "data25" ) << QString("ABC") << QString("A") << 1 << false << -1;
    QTest::newRow( "data26" ) << QString("ABC") << QString("a") << 1 << true << -1;
    QTest::newRow( "data27" ) << QString("ABC") << QString("a") << 1 << false << -1;
    QTest::newRow( "data28" ) << QString("ABC") << QString("B") << 0 << true << 1;
    QTest::newRow( "data29" ) << QString("ABC") << QString("B") << 0 << false << 1;
    QTest::newRow( "data30" ) << QString("ABC") << QString("b") << 0 << true << -1;
    QTest::newRow( "data31" ) << QString("ABC") << QString("b") << 0 << false << 1;
    QTest::newRow( "data32" ) << QString("ABC") << QString("B") << 1 << true << 1;
    QTest::newRow( "data33" ) << QString("ABC") << QString("B") << 1 << false << 1;
    QTest::newRow( "data34" ) << QString("ABC") << QString("b") << 1 << true << -1;
    QTest::newRow( "data35" ) << QString("ABC") << QString("b") << 1 << false << 1;
    QTest::newRow( "data36" ) << QString("ABC") << QString("B") << 2 << true << -1;
    QTest::newRow( "data37" ) << QString("ABC") << QString("B") << 2 << false << -1;

    QTest::newRow( "data40" ) << QString("aBc") << QString("bc") << 0 << true << -1;
    QTest::newRow( "data41" ) << QString("aBc") << QString("Bc") << 0 << true << 1;
    QTest::newRow( "data42" ) << QString("aBc") << QString("bC") << 0 << true << -1;
    QTest::newRow( "data43" ) << QString("aBc") << QString("BC") << 0 << true << -1;
    QTest::newRow( "data44" ) << QString("aBc") << QString("bc") << 0 << false << 1;
    QTest::newRow( "data45" ) << QString("aBc") << QString("Bc") << 0 << false << 1;
    QTest::newRow( "data46" ) << QString("aBc") << QString("bC") << 0 << false << 1;
    QTest::newRow( "data47" ) << QString("aBc") << QString("BC") << 0 << false << 1;
    QTest::newRow( "data48" ) << QString("AbC") << QString("bc") << 0 << true << -1;
    QTest::newRow( "data49" ) << QString("AbC") << QString("Bc") << 0 << true << -1;
    QTest::newRow( "data50" ) << QString("AbC") << QString("bC") << 0 << true << 1;
    QTest::newRow( "data51" ) << QString("AbC") << QString("BC") << 0 << true << -1;
    QTest::newRow( "data52" ) << QString("AbC") << QString("bc") << 0 << false << 1;
    QTest::newRow( "data53" ) << QString("AbC") << QString("Bc") << 0 << false << 1;

    QTest::newRow( "data54" ) << QString("AbC") << QString("bC") << 0 << false << 1;
    QTest::newRow( "data55" ) << QString("AbC") << QString("BC") << 0 << false << 1;
    QTest::newRow( "data56" ) << QString("AbC") << QString("BC") << 1 << false << 1;
    QTest::newRow( "data57" ) << QString("AbC") << QString("BC") << 2 << false << -1;

    QTest::newRow( "null-in-null") << QString() << QString() << 0 << false << 0;
    QTest::newRow( "empty-in-null") << QString() << QString("") << 0 << false << 0;
    QTest::newRow( "null-in-empty") << QString("") << QString() << 0 << false << 0;
    QTest::newRow( "empty-in-empty") << QString("") << QString("") << 0 << false << 0;
    QTest::newRow( "data-in-null") << QString() << QString("a") << 0 << false << -1;
    QTest::newRow( "data-in-empty") << QString("") << QString("a") << 0 << false << -1;


    QString s1 = "abc";
    s1 += QChar(0xb5);
    QString s2;
    s2 += QChar(0x3bc);
    QTest::newRow( "data58" ) << s1 << s2 << 0 << false << 3;
    s2.prepend(QLatin1Char('C'));
    QTest::newRow( "data59" ) << s1 << s2 << 0 << false << 2;

    QString veryBigHaystack(500, 'a');
    veryBigHaystack += 'B';
    QTest::newRow("BoyerMooreStressTest") << veryBigHaystack << veryBigHaystack << 0 << true << 0;
    QTest::newRow("BoyerMooreStressTest2") << QString(veryBigHaystack + 'c') << veryBigHaystack << 0 << true << 0;
    QTest::newRow("BoyerMooreStressTest3") << QString('c' + veryBigHaystack) << veryBigHaystack << 0 << true << 1;
    QTest::newRow("BoyerMooreStressTest4") << veryBigHaystack << QString(veryBigHaystack + 'c') << 0 << true << -1;
    QTest::newRow("BoyerMooreStressTest5") << veryBigHaystack << QString('c' + veryBigHaystack) << 0 << true << -1;
    QTest::newRow("BoyerMooreStressTest6") << QString('d' + veryBigHaystack) << QString('c' + veryBigHaystack) << 0 << true << -1;
    QTest::newRow("BoyerMooreStressTest7") << QString(veryBigHaystack + 'c') << QString('c' + veryBigHaystack) << 0 << true << -1;

    QTest::newRow("BoyerMooreInsensitiveStressTest") << veryBigHaystack << veryBigHaystack << 0 << false << 0;

}

void tst_QString::indexOf()
{
    QFETCH( QString, haystack );
    QFETCH( QString, needle );
    QFETCH( int, startpos );
    QFETCH( bool, bcs );
    QFETCH( int, resultpos );
    CREATE_VIEW(needle);

    Qt::CaseSensitivity cs = bcs ? Qt::CaseSensitive : Qt::CaseInsensitive;

    bool needleIsLatin = (QString::fromLatin1(needle.toLatin1()) == needle);

    QCOMPARE( haystack.indexOf(needle, startpos, cs), resultpos );
    QCOMPARE( haystack.indexOf(view, startpos, cs), resultpos );
    if (needleIsLatin) {
        QCOMPARE( haystack.indexOf(needle.toLatin1(), startpos, cs), resultpos );
        QCOMPARE( haystack.indexOf(needle.toLatin1().data(), startpos, cs), resultpos );
    }

#if QT_CONFIG(regularexpression)
    {
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
        if (!bcs)
            options |= QRegularExpression::CaseInsensitiveOption;

        QRegularExpression re(QRegularExpression::escape(needle), options);
        QCOMPARE( haystack.indexOf(re, startpos), resultpos );
        QCOMPARE(haystack.indexOf(re, startpos, nullptr), resultpos);

        QRegularExpressionMatch match;
        QVERIFY(!match.hasMatch());
        QCOMPARE(haystack.indexOf(re, startpos, &match), resultpos);
        QCOMPARE(match.hasMatch(), resultpos != -1);
        if (resultpos > -1 && needleIsLatin) {
            if (bcs)
                QVERIFY(match.captured() == needle);
            else
                QVERIFY(match.captured().toLower() == needle.toLower());
        }
    }
#endif

    if (cs == Qt::CaseSensitive) {
        QCOMPARE( haystack.indexOf(needle, startpos), resultpos );
        QCOMPARE( haystack.indexOf(view, startpos), resultpos );
        if (needleIsLatin) {
            QCOMPARE( haystack.indexOf(needle.toLatin1(), startpos), resultpos );
            QCOMPARE( haystack.indexOf(needle.toLatin1().data(), startpos), resultpos );
        }
        if (startpos == 0) {
            QCOMPARE( haystack.indexOf(needle), resultpos );
            QCOMPARE( haystack.indexOf(view), resultpos );
            if (needleIsLatin) {
                QCOMPARE( haystack.indexOf(needle.toLatin1()), resultpos );
                QCOMPARE( haystack.indexOf(needle.toLatin1().data()), resultpos );
            }
        }
    }
    if (needle.size() == 1) {
        QCOMPARE(haystack.indexOf(needle.at(0), startpos, cs), resultpos);
        QCOMPARE(haystack.indexOf(view.at(0), startpos, cs), resultpos);
    }

}

void tst_QString::indexOf2_data()
{
    QTest::addColumn<QString>("haystack" );
    QTest::addColumn<QString>("needle" );
    QTest::addColumn<int>("resultpos" );

    QTest::newRow( "data0" ) << QString() << QString() << 0;
    QTest::newRow( "data1" ) << QString() << QString("") << 0;
    QTest::newRow( "data2" ) << QString("") << QString() << 0;
    QTest::newRow( "data3" ) << QString("") << QString("") << 0;
    QTest::newRow( "data4" ) << QString() << QString("a") << -1;
    QTest::newRow( "data5" ) << QString() << QString("abcdefg") << -1;
    QTest::newRow( "data6" ) << QString("") << QString("a") << -1;
    QTest::newRow( "data7" ) << QString("") << QString("abcdefg") << -1;

    QTest::newRow( "data8" ) << QString("a") << QString() << 0;
    QTest::newRow( "data9" ) << QString("a") << QString("") << 0;
    QTest::newRow( "data10" ) << QString("a") << QString("a") << 0;
    QTest::newRow( "data11" ) << QString("a") << QString("b") << -1;
    QTest::newRow( "data12" ) << QString("a") << QString("abcdefg") << -1;
    QTest::newRow( "data13" ) << QString("ab") << QString() << 0;
    QTest::newRow( "data14" ) << QString("ab") << QString("") << 0;
    QTest::newRow( "data15" ) << QString("ab") << QString("a") << 0;
    QTest::newRow( "data16" ) << QString("ab") << QString("b") << 1;
    QTest::newRow( "data17" ) << QString("ab") << QString("ab") << 0;
    QTest::newRow( "data18" ) << QString("ab") << QString("bc") << -1;
    QTest::newRow( "data19" ) << QString("ab") << QString("abcdefg") << -1;

    QTest::newRow( "data30" ) << QString("abc") << QString("a") << 0;
    QTest::newRow( "data31" ) << QString("abc") << QString("b") << 1;
    QTest::newRow( "data32" ) << QString("abc") << QString("c") << 2;
    QTest::newRow( "data33" ) << QString("abc") << QString("d") << -1;
    QTest::newRow( "data34" ) << QString("abc") << QString("ab") << 0;
    QTest::newRow( "data35" ) << QString("abc") << QString("bc") << 1;
    QTest::newRow( "data36" ) << QString("abc") << QString("cd") << -1;
    QTest::newRow( "data37" ) << QString("abc") << QString("ac") << -1;

    // sizeof(whale) > 32
    QString whale = "a5zby6cx7dw8evf9ug0th1si2rj3qkp4lomn";
    QString minnow = "zby";
    QTest::newRow( "data40" ) << whale << minnow << 2;
    QTest::newRow( "data41" ) << QString(whale + whale) << minnow << 2;
    QTest::newRow( "data42" ) << QString(minnow + whale) << minnow << 0;
    QTest::newRow( "data43" ) << whale << whale << 0;
    QTest::newRow( "data44" ) << QString(whale + whale) << whale << 0;
    QTest::newRow( "data45" ) << whale << QString(whale + whale) << -1;
    QTest::newRow( "data46" ) << QString(whale + whale) << QString(whale + whale) << 0;
    QTest::newRow( "data47" ) << QString(whale + whale) << QString(whale + minnow) << -1;
    QTest::newRow( "data48" ) << QString(minnow + whale) << whale << (int)minnow.length();
}

void tst_QString::indexOf2()
{
    QFETCH( QString, haystack );
    QFETCH( QString, needle );
    QFETCH( int, resultpos );
    CREATE_VIEW(needle);

    QByteArray chaystack = haystack.toLatin1();
    QByteArray cneedle = needle.toLatin1();
    int got;

    QCOMPARE( haystack.indexOf(needle, 0, Qt::CaseSensitive), resultpos );
    QCOMPARE( haystack.indexOf(view, 0, Qt::CaseSensitive), resultpos );
    QCOMPARE( QStringMatcher(needle, Qt::CaseSensitive).indexIn(haystack, 0), resultpos );
    QCOMPARE( haystack.indexOf(needle, 0, Qt::CaseInsensitive), resultpos );
    QCOMPARE( haystack.indexOf(view, 0, Qt::CaseInsensitive), resultpos );
    QCOMPARE( QStringMatcher(needle, Qt::CaseInsensitive).indexIn(haystack, 0), resultpos );
    if ( needle.length() > 0 ) {
        got = haystack.lastIndexOf( needle, -1, Qt::CaseSensitive );
        QVERIFY( got == resultpos || (resultpos >= 0 && got >= resultpos) );
        got = haystack.lastIndexOf( needle, -1, Qt::CaseInsensitive );
        QVERIFY( got == resultpos || (resultpos >= 0 && got >= resultpos) );
    }

    QCOMPARE( chaystack.indexOf(cneedle, 0), resultpos );
    QCOMPARE( QByteArrayMatcher(cneedle).indexIn(chaystack, 0), resultpos );
    if ( cneedle.length() > 0 ) {
        got = chaystack.lastIndexOf(cneedle, -1);
        QVERIFY( got == resultpos || (resultpos >= 0 && got >= resultpos) );
    }
}

#if QT_CONFIG(regularexpression)
void tst_QString::indexOfInvalidRegex()
{
    QTest::ignoreMessage(QtWarningMsg, "QString::indexOf: invalid QRegularExpression object");
    QCOMPARE(QString("invalid regex\\").indexOf(QRegularExpression("invalid regex\\")), -1);
    QTest::ignoreMessage(QtWarningMsg, "QString::indexOf: invalid QRegularExpression object");
    QCOMPARE(QString("invalid regex\\").indexOf(QRegularExpression("invalid regex\\"), -1, nullptr), -1);

    QRegularExpressionMatch match;
    QVERIFY(!match.hasMatch());
    QTest::ignoreMessage(QtWarningMsg, "QString::indexOf: invalid QRegularExpression object");
    QCOMPARE(QString("invalid regex\\").indexOf(QRegularExpression("invalid regex\\"), -1, &match), -1);
    QVERIFY(!match.hasMatch());
}
#endif

void tst_QString::lastIndexOf_data()
{
    QTest::addColumn<QString>("haystack" );
    QTest::addColumn<QString>("needle" );
    QTest::addColumn<int>("from" );
    QTest::addColumn<int>("expected" );
    QTest::addColumn<bool>("caseSensitive" );

    QString a = "ABCDEFGHIEfGEFG";

    QTest::newRow("-1") << a << "G" << int(a.size()) - 1 << 14 << true;
    QTest::newRow("1") << a << "G" << - 1 << 14 << true;
    QTest::newRow("2") << a << "G" << -3 << 11 << true;
    QTest::newRow("3") << a << "G" << -5 << 6 << true;
    QTest::newRow("4") << a << "G" << 14 << 14 << true;
    QTest::newRow("5") << a << "G" << 13 << 11 << true;
    QTest::newRow("6") << a << "B" << int(a.size()) - 1 << 1 << true;
    QTest::newRow("7") << a << "B" << - 1 << 1 << true;
    QTest::newRow("8") << a << "B" << 1 << 1 << true;
    QTest::newRow("9") << a << "B" << 0 << -1 << true;

    QTest::newRow("10") << a << "G" <<  -1 <<  int(a.size())-1 << true;
    QTest::newRow("11") << a << "G" <<  int(a.size())-1 <<  int(a.size())-1 << true;
    QTest::newRow("12") << a << "G" <<  int(a.size()) <<  int(a.size())-1 << true;
    QTest::newRow("13") << a << "A" <<  0 <<  0 << true;
    QTest::newRow("14") << a << "A" <<  -1*int(a.size()) <<  0 << true;

    QTest::newRow("15") << a << "efg" << 0 << -1 << false;
    QTest::newRow("16") << a << "efg" << int(a.size()) << 12 << false;
    QTest::newRow("17") << a << "efg" << -1 * int(a.size()) << -1 << false;
    QTest::newRow("19") << a << "efg" << int(a.size()) - 1 << 12 << false;
    QTest::newRow("20") << a << "efg" << 12 << 12 << false;
    QTest::newRow("21") << a << "efg" << -12 << -1 << false;
    QTest::newRow("22") << a << "efg" << 11 << 9 << false;

    QTest::newRow("24") << "" << "asdf" << -1 << -1 << false;
    QTest::newRow("25") << "asd" << "asdf" << -1 << -1 << false;
    QTest::newRow("26") << "" << QString() << -1 << -1 << false;

    QTest::newRow("27") << a << "" << int(a.size()) << int(a.size()) << false;
    QTest::newRow("28") << a << "" << int(a.size()) + 10 << -1 << false;

    QTest::newRow("null-in-null") << QString() << QString() << 0 << 0 << false;
    QTest::newRow("empty-in-null") << QString() << QString("") << 0 << 0 << false;
    QTest::newRow("null-in-empty") << QString("") << QString() << 0 << 0 << false;
    QTest::newRow("empty-in-empty") << QString("") << QString("") << 0 << 0 << false;
    QTest::newRow("data-in-null") << QString() << QString("a") << 0 << -1 << false;
    QTest::newRow("data-in-empty") << QString("") << QString("a") << 0 << -1 << false;
}

void tst_QString::lastIndexOf()
{
    QFETCH(QString, haystack);
    QFETCH(QString, needle);
    QFETCH(int, from);
    QFETCH(int, expected);
    QFETCH(bool, caseSensitive);
    CREATE_VIEW(needle);

    Qt::CaseSensitivity cs = (caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

    QCOMPARE(haystack.lastIndexOf(needle, from, cs), expected);
    QCOMPARE(haystack.lastIndexOf(view, from, cs), expected);
    QCOMPARE(haystack.lastIndexOf(needle.toLatin1(), from, cs), expected);
    QCOMPARE(haystack.lastIndexOf(needle.toLatin1().data(), from, cs), expected);

#if QT_CONFIG(regularexpression)
    if (from >= -1 && from < haystack.size() && needle.size() > 0) {
        // unfortunately, QString and QRegularExpression don't have the same out of bound semantics
        // I think QString is wrong -- See file log for contact information.
        {
            QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
            if (!caseSensitive)
                options |= QRegularExpression::CaseInsensitiveOption;

            QRegularExpression re(QRegularExpression::escape(needle), options);
            QCOMPARE(haystack.lastIndexOf(re, from), expected);
            QCOMPARE(haystack.lastIndexOf(re, from, nullptr), expected);
            QRegularExpressionMatch match;
            QVERIFY(!match.hasMatch());
            QCOMPARE(haystack.lastIndexOf(re, from, &match), expected);
            QCOMPARE(match.hasMatch(), expected > -1);
            if (expected > -1) {
                if (caseSensitive)
                    QCOMPARE(match.captured(), needle);
                else
                    QCOMPARE(match.captured().toLower(), needle.toLower());
            }
        }
    }
#endif

    if (cs == Qt::CaseSensitive) {
        QCOMPARE(haystack.lastIndexOf(needle, from), expected);
        QCOMPARE(haystack.lastIndexOf(view, from), expected);
        QCOMPARE(haystack.lastIndexOf(needle.toLatin1(), from), expected);
        QCOMPARE(haystack.lastIndexOf(needle.toLatin1().data(), from), expected);
        if (from == haystack.size()) {
            QCOMPARE(haystack.lastIndexOf(needle), expected);
            QCOMPARE(haystack.lastIndexOf(view), expected);
            QCOMPARE(haystack.lastIndexOf(needle.toLatin1()), expected);
            QCOMPARE(haystack.lastIndexOf(needle.toLatin1().data()), expected);
        }
    }
    if (needle.size() == 1) {
        QCOMPARE(haystack.lastIndexOf(needle.at(0), from), expected);
        QCOMPARE(haystack.lastIndexOf(view.at(0), from), expected);
    }
}

#if QT_CONFIG(regularexpression)
void tst_QString::lastIndexOfInvalidRegex()
{
    QTest::ignoreMessage(QtWarningMsg, "QString::lastIndexOf: invalid QRegularExpression object");
    QCOMPARE(QString("invalid regex\\").lastIndexOf(QRegularExpression("invalid regex\\"), 0), -1);
    QTest::ignoreMessage(QtWarningMsg, "QString::lastIndexOf: invalid QRegularExpression object");
    QCOMPARE(QString("invalid regex\\").lastIndexOf(QRegularExpression("invalid regex\\"), -1, nullptr), -1);

    QRegularExpressionMatch match;
    QVERIFY(!match.hasMatch());
    QTest::ignoreMessage(QtWarningMsg, "QString::lastIndexOf: invalid QRegularExpression object");
    QCOMPARE(QString("invalid regex\\").lastIndexOf(QRegularExpression("invalid regex\\"), -1, &match), -1);
    QVERIFY(!match.hasMatch());
}
#endif

void tst_QString::count()
{
    QString a;
    a="ABCDEFGHIEfGEFG"; // 15 chars
    QCOMPARE(a.count('A'),1);
    QCOMPARE(a.count('Z'),0);
    QCOMPARE(a.count('E'),3);
    QCOMPARE(a.count('F'),2);
    QCOMPARE(a.count('F',Qt::CaseInsensitive),3);
    QCOMPARE(a.count("FG"),2);
    QCOMPARE(a.count("FG",Qt::CaseInsensitive),3);
    QCOMPARE(a.count( QString(), Qt::CaseInsensitive), 16);
    QCOMPARE(a.count( "", Qt::CaseInsensitive), 16);
#if QT_CONFIG(regularexpression)
    QCOMPARE(a.count(QRegularExpression("")), 16);
    QCOMPARE(a.count(QRegularExpression("[FG][HI]")), 1);
    QCOMPARE(a.count(QRegularExpression("[G][HE]")), 2);
    QTest::ignoreMessage(QtWarningMsg, "QString::count: invalid QRegularExpression object");
    QCOMPARE(a.count(QRegularExpression("invalid regex\\")), 0);
#endif

    CREATE_VIEW(QLatin1String("FG"));
    QCOMPARE(a.count(view),2);
    QCOMPARE(a.count(view,Qt::CaseInsensitive),3);
    QCOMPARE(a.count( QStringView(), Qt::CaseInsensitive), 16);

    QString nullStr;
    QCOMPARE(nullStr.count(), 0);
    QCOMPARE(nullStr.count('A'), 0);
    QCOMPARE(nullStr.count("AB"), 0);
    QCOMPARE(nullStr.count(view), 0);
    QCOMPARE(nullStr.count(QString()), 1);
    QCOMPARE(nullStr.count(""), 1);
#if QT_CONFIG(regularexpression)
    QCOMPARE(nullStr.count(QRegularExpression("")), 1);
    QCOMPARE(nullStr.count(QRegularExpression("[FG][HI]")), 0);
    QTest::ignoreMessage(QtWarningMsg, "QString::count: invalid QRegularExpression object");
    QCOMPARE(nullStr.count(QRegularExpression("invalid regex\\")), 0);
#endif

    QString emptyStr("");
    QCOMPARE(emptyStr.count(), 0);
    QCOMPARE(emptyStr.count('A'), 0);
    QCOMPARE(emptyStr.count("AB"), 0);
    QCOMPARE(emptyStr.count(view), 0);
    QCOMPARE(emptyStr.count(QString()), 1);
    QCOMPARE(emptyStr.count(""), 1);
#if QT_CONFIG(regularexpression)
    QCOMPARE(emptyStr.count(QRegularExpression("")), 1);
    QCOMPARE(emptyStr.count(QRegularExpression("[FG][HI]")), 0);
    QTest::ignoreMessage(QtWarningMsg, "QString::count: invalid QRegularExpression object");
    QCOMPARE(emptyStr.count(QRegularExpression("invalid regex\\")), 0);
#endif
}

void tst_QString::contains()
{
    QString a;
    a="ABCDEFGHIEfGEFG"; // 15 chars
    QVERIFY(a.contains('A'));
    QVERIFY(!a.contains('Z'));
    QVERIFY(a.contains('E'));
    QVERIFY(a.contains('F'));
    QVERIFY(a.contains('F',Qt::CaseInsensitive));
    QVERIFY(a.contains("FG"));
    QVERIFY(a.contains("FG",Qt::CaseInsensitive));
    QVERIFY(a.contains(QLatin1String("FG")));
    QVERIFY(a.contains(QLatin1String("fg"),Qt::CaseInsensitive));
    QVERIFY(a.contains( QString(), Qt::CaseInsensitive));
    QVERIFY(a.contains( "", Qt::CaseInsensitive));
#if QT_CONFIG(regularexpression)
    QVERIFY(a.contains(QRegularExpression("[FG][HI]")));
    QVERIFY(a.contains(QRegularExpression("[G][HE]")));

    {
        QRegularExpressionMatch match;
        QVERIFY(!match.hasMatch());

        QVERIFY(a.contains(QRegularExpression("[FG][HI]"), &match));
        QVERIFY(match.hasMatch());
        QCOMPARE(match.capturedStart(), 6);
        QCOMPARE(match.capturedEnd(), 8);
        QCOMPARE(match.captured(), QStringLiteral("GH"));

        QVERIFY(a.contains(QRegularExpression("[G][HE]"), &match));
        QVERIFY(match.hasMatch());
        QCOMPARE(match.capturedStart(), 6);
        QCOMPARE(match.capturedEnd(), 8);
        QCOMPARE(match.captured(), QStringLiteral("GH"));

        QVERIFY(a.contains(QRegularExpression("[f](.*)[FG]"), &match));
        QVERIFY(match.hasMatch());
        QCOMPARE(match.capturedStart(), 10);
        QCOMPARE(match.capturedEnd(), 15);
        QCOMPARE(match.captured(), QString("fGEFG"));
        QCOMPARE(match.capturedStart(1), 11);
        QCOMPARE(match.capturedEnd(1), 14);
        QCOMPARE(match.captured(1), QStringLiteral("GEF"));

        QVERIFY(a.contains(QRegularExpression("[f](.*)[F]"), &match));
        QVERIFY(match.hasMatch());
        QCOMPARE(match.capturedStart(), 10);
        QCOMPARE(match.capturedEnd(), 14);
        QCOMPARE(match.captured(), QString("fGEF"));
        QCOMPARE(match.capturedStart(1), 11);
        QCOMPARE(match.capturedEnd(1), 13);
        QCOMPARE(match.captured(1), QStringLiteral("GE"));

        QVERIFY(!a.contains(QRegularExpression("ZZZ"), &match));
        // doesn't match, but ensure match didn't change
        QVERIFY(match.hasMatch());
        QCOMPARE(match.capturedStart(), 10);
        QCOMPARE(match.capturedEnd(), 14);
        QCOMPARE(match.captured(), QStringLiteral("fGEF"));
        QCOMPARE(match.capturedStart(1), 11);
        QCOMPARE(match.capturedEnd(1), 13);
        QCOMPARE(match.captured(1), QStringLiteral("GE"));

        // don't crash with a null pointer
        QVERIFY(a.contains(QRegularExpression("[FG][HI]"), 0));
        QVERIFY(!a.contains(QRegularExpression("ZZZ"), 0));
    }

    QTest::ignoreMessage(QtWarningMsg, "QString::contains: invalid QRegularExpression object");
    QVERIFY(!a.contains(QRegularExpression("invalid regex\\")));
#endif

    CREATE_VIEW(QLatin1String("FG"));
    QVERIFY(a.contains(view));
    QVERIFY(a.contains(view, Qt::CaseInsensitive));
    QVERIFY(a.contains( QStringView(), Qt::CaseInsensitive));

    QString nullStr;
    QVERIFY(!nullStr.contains('A'));
    QVERIFY(!nullStr.contains("AB"));
    QVERIFY(!nullStr.contains(view));
#if QT_CONFIG(regularexpression)
    QVERIFY(!nullStr.contains(QRegularExpression("[FG][HI]")));
    QRegularExpressionMatch nullMatch;
    QVERIFY(nullStr.contains(QRegularExpression(""), &nullMatch));
    QVERIFY(nullMatch.hasMatch());
    QCOMPARE(nullMatch.captured(), "");
    QCOMPARE(nullMatch.capturedStart(), 0);
    QCOMPARE(nullMatch.capturedEnd(), 0);
#endif
    QVERIFY(!nullStr.isDetached());

    QString emptyStr("");
    QVERIFY(!emptyStr.contains('A'));
    QVERIFY(!emptyStr.contains("AB"));
    QVERIFY(!emptyStr.contains(view));
#if QT_CONFIG(regularexpression)
    QVERIFY(!emptyStr.contains(QRegularExpression("[FG][HI]")));
    QRegularExpressionMatch emptyMatch;
    QVERIFY(emptyStr.contains(QRegularExpression(""), &emptyMatch));
    QVERIFY(emptyMatch.hasMatch());
    QCOMPARE(emptyMatch.captured(), "");
    QCOMPARE(emptyMatch.capturedStart(), 0);
    QCOMPARE(emptyMatch.capturedEnd(), 0);
#endif
    QVERIFY(!emptyStr.isDetached());
}


void tst_QString::left()
{
    QString a;

    QVERIFY(a.left(0).isNull());
    QVERIFY(a.left(5).isNull());
    QVERIFY(a.left(-4).isNull());
    QVERIFY(!a.isDetached());

    a="ABCDEFGHIEfGEFG"; // 15 chars
    QCOMPARE(a.left(3), QLatin1String("ABC"));
    QVERIFY(!a.left(0).isNull());
    QCOMPARE(a.left(0), QLatin1String(""));

    QString n;
    QVERIFY(n.left(3).isNull());
    QVERIFY(n.left(0).isNull());
    QVERIFY(n.left(0).isNull());

    QString l = "Left";
    QCOMPARE(l.left(-1), l);
    QCOMPARE(l.left(100), l);
}

void tst_QString::right()
{
    QString a;

    QVERIFY(a.right(0).isNull());
    QVERIFY(a.right(5).isNull());
    QVERIFY(a.right(-4).isNull());
    QVERIFY(!a.isDetached());

    a="ABCDEFGHIEfGEFG"; // 15 chars
    QCOMPARE(a.right(3), QLatin1String("EFG"));
    QCOMPARE(a.right(0), QLatin1String(""));

    QString n;
    QVERIFY(n.right(3).isNull());
    QVERIFY(n.right(0).isNull());

    QString r = "Right";
    QCOMPARE(r.right(-1), r);
    QCOMPARE(r.right(100), r);
}

void tst_QString::mid()
{
    QString a;

    QVERIFY(a.mid(0).isNull());
    QVERIFY(a.mid(5, 6).isNull());
    QVERIFY(a.mid(-4, 3).isNull());
    QVERIFY(a.mid(4, -3).isNull());
    QVERIFY(!a.isDetached());

    a="ABCDEFGHIEfGEFG"; // 15 chars

    QCOMPARE(a.mid(3,3), QLatin1String("DEF"));
    QCOMPARE(a.mid(0,0), QLatin1String(""));
    QVERIFY(!a.mid(15,0).isNull());
    QVERIFY(a.mid(15,0).isEmpty());
    QVERIFY(!a.mid(15,1).isNull());
    QVERIFY(a.mid(15,1).isEmpty());
    QVERIFY(a.mid(9999).isNull());
    QVERIFY(a.mid(9999,1).isNull());

    QCOMPARE(a.mid(-1, 6), a.mid(0, 5));
    QVERIFY(a.mid(-100, 6).isEmpty());
    QVERIFY(a.mid(INT_MIN, 0).isEmpty());
    QCOMPARE(a.mid(INT_MIN, -1), a);
    QVERIFY(a.mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(a.mid(INT_MIN + 1, INT_MAX).isEmpty());
    QCOMPARE(a.mid(INT_MIN + 2, INT_MAX), a.left(1));
    QCOMPARE(a.mid(INT_MIN + a.size() + 1, INT_MAX), a);
    QVERIFY(a.mid(INT_MAX).isNull());
    QVERIFY(a.mid(INT_MAX, INT_MAX).isNull());
    QCOMPARE(a.mid(-5, INT_MAX), a);
    QCOMPARE(a.mid(-1, INT_MAX), a);
    QCOMPARE(a.mid(0, INT_MAX), a);
    QCOMPARE(a.mid(1, INT_MAX), QString("BCDEFGHIEfGEFG"));
    QCOMPARE(a.mid(5, INT_MAX), QString("FGHIEfGEFG"));
    QVERIFY(a.mid(20, INT_MAX).isNull());
    QCOMPARE(a.mid(-1, -1), a);

    QString n;
    QVERIFY(n.mid(3,3).isNull());
    QVERIFY(n.mid(0,0).isNull());
    QVERIFY(n.mid(9999,0).isNull());
    QVERIFY(n.mid(9999,1).isNull());

    QVERIFY(n.mid(-1, 6).isNull());
    QVERIFY(n.mid(-100, 6).isNull());
    QVERIFY(n.mid(INT_MIN, 0).isNull());
    QVERIFY(n.mid(INT_MIN, -1).isNull());
    QVERIFY(n.mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(n.mid(INT_MIN + 1, INT_MAX).isNull());
    QVERIFY(n.mid(INT_MIN + 2, INT_MAX).isNull());
    QVERIFY(n.mid(INT_MIN + n.size() + 1, INT_MAX).isNull());
    QVERIFY(n.mid(INT_MAX).isNull());
    QVERIFY(n.mid(INT_MAX, INT_MAX).isNull());
    QVERIFY(n.mid(-5, INT_MAX).isNull());
    QVERIFY(n.mid(-1, INT_MAX).isNull());
    QVERIFY(n.mid(0, INT_MAX).isNull());
    QVERIFY(n.mid(1, INT_MAX).isNull());
    QVERIFY(n.mid(5, INT_MAX).isNull());
    QVERIFY(n.mid(20, INT_MAX).isNull());
    QVERIFY(n.mid(-1, -1).isNull());

    QString x = "Nine pineapples";
    QCOMPARE(x.mid(5, 4), QString("pine"));
    QCOMPARE(x.mid(5), QString("pineapples"));

    QCOMPARE(x.mid(-1, 6), x.mid(0, 5));
    QVERIFY(x.mid(-100, 6).isEmpty());
    QVERIFY(x.mid(INT_MIN, 0).isEmpty());
    QCOMPARE(x.mid(INT_MIN, -1), x);
    QVERIFY(x.mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(x.mid(INT_MIN + 1, INT_MAX).isEmpty());
    QCOMPARE(x.mid(INT_MIN + 2, INT_MAX), x.left(1));
    QCOMPARE(x.mid(INT_MIN + x.size() + 1, INT_MAX), x);
    QVERIFY(x.mid(INT_MAX).isNull());
    QVERIFY(x.mid(INT_MAX, INT_MAX).isNull());
    QCOMPARE(x.mid(-5, INT_MAX), x);
    QCOMPARE(x.mid(-1, INT_MAX), x);
    QCOMPARE(x.mid(0, INT_MAX), x);
    QCOMPARE(x.mid(1, INT_MAX), QString("ine pineapples"));
    QCOMPARE(x.mid(5, INT_MAX), QString("pineapples"));
    QVERIFY(x.mid(20, INT_MAX).isNull());
    QCOMPARE(x.mid(-1, -1), x);
}

void tst_QString::leftJustified()
{
    QString a;

    QCOMPARE(a.leftJustified(3, '-'), QLatin1String("---"));
    QCOMPARE(a.leftJustified(2), QLatin1String("  "));
    QVERIFY(!a.isDetached());

    a="ABC";
    QCOMPARE(a.leftJustified(5,'-'), QLatin1String("ABC--"));
    QCOMPARE(a.leftJustified(4,'-'), QLatin1String("ABC-"));
    QCOMPARE(a.leftJustified(4), QLatin1String("ABC "));
    QCOMPARE(a.leftJustified(3), QLatin1String("ABC"));
    QCOMPARE(a.leftJustified(2), QLatin1String("ABC"));
    QCOMPARE(a.leftJustified(1), QLatin1String("ABC"));
    QCOMPARE(a.leftJustified(0), QLatin1String("ABC"));

    QCOMPARE(a.leftJustified(4,' ',true), QLatin1String("ABC "));
    QCOMPARE(a.leftJustified(3,' ',true), QLatin1String("ABC"));
    QCOMPARE(a.leftJustified(2,' ',true), QLatin1String("AB"));
    QCOMPARE(a.leftJustified(1,' ',true), QLatin1String("A"));
    QCOMPARE(a.leftJustified(0,' ',true), QLatin1String(""));
}

void tst_QString::rightJustified()
{
    QString a;

    QCOMPARE(a.rightJustified(3, '-'), QLatin1String("---"));
    QCOMPARE(a.rightJustified(2), QLatin1String("  "));
    QVERIFY(!a.isDetached());

    a="ABC";
    QCOMPARE(a.rightJustified(5,'-'), QLatin1String("--ABC"));
    QCOMPARE(a.rightJustified(4,'-'), QLatin1String("-ABC"));
    QCOMPARE(a.rightJustified(4), QLatin1String(" ABC"));
    QCOMPARE(a.rightJustified(3), QLatin1String("ABC"));
    QCOMPARE(a.rightJustified(2), QLatin1String("ABC"));
    QCOMPARE(a.rightJustified(1), QLatin1String("ABC"));
    QCOMPARE(a.rightJustified(0), QLatin1String("ABC"));

    QCOMPARE(a.rightJustified(4,'-',true), QLatin1String("-ABC"));
    QCOMPARE(a.rightJustified(4,' ',true), QLatin1String(" ABC"));
    QCOMPARE(a.rightJustified(3,' ',true), QLatin1String("ABC"));
    QCOMPARE(a.rightJustified(2,' ',true), QLatin1String("AB"));
    QCOMPARE(a.rightJustified(1,' ',true), QLatin1String("A"));
    QCOMPARE(a.rightJustified(0,' ',true), QLatin1String(""));
    QCOMPARE(a, QLatin1String("ABC"));
}

void tst_QString::unicodeTableAccess_data()
{
    QTest::addColumn<QString>("invalid");

    const auto join = [](char16_t high, char16_t low) {
        const QChar pair[2] = { high, low };
        return QString(pair, 2);
    };
    // Least high surrogate for which an invalid successor produces an error:
    QTest::newRow("least-high") << join(0xdbf8, 0xfc00);
    // Least successor that, after a high surrogate, produces invalid:
    QTest::newRow("least-follow") << join(0xdbff, 0xe000);
}

void tst_QString::unicodeTableAccess()
{
    // QString processing must not access unicode tables out of bounds.
    QFETCH(QString, invalid);
    // Exercise methods, to see if any assertions trigger:
    const auto upper = invalid.toUpper();
    const auto lower = invalid.toLower();
    const auto folded = invalid.toCaseFolded();
    // Fatuous test, just to use those.
    QVERIFY(upper == invalid || lower == invalid || folded == invalid || lower != upper);
}

void tst_QString::toUpper()
{
    const QString s;
    QCOMPARE( s.toUpper(), QString() ); // lvalue
    QCOMPARE( QString().toUpper(), QString() ); // rvalue
    QCOMPARE( QString("").toUpper(), QString("") );
    QCOMPARE( QStringLiteral("text").toUpper(), QString("TEXT") );
    QCOMPARE( QString("text").toUpper(), QString("TEXT") );
    QCOMPARE( QString("Text").toUpper(), QString("TEXT") );
    QCOMPARE( QString("tExt").toUpper(), QString("TEXT") );
    QCOMPARE( QString("teXt").toUpper(), QString("TEXT") );
    QCOMPARE( QString("texT").toUpper(), QString("TEXT") );
    QCOMPARE( QString("TExt").toUpper(), QString("TEXT") );
    QCOMPARE( QString("teXT").toUpper(), QString("TEXT") );
    QCOMPARE( QString("tEXt").toUpper(), QString("TEXT") );
    QCOMPARE( QString("tExT").toUpper(), QString("TEXT") );
    QCOMPARE( QString("TEXT").toUpper(), QString("TEXT") );
    QCOMPARE( QString("@ABYZ[").toUpper(), QString("@ABYZ["));
    QCOMPARE( QString("@abyz[").toUpper(), QString("@ABYZ["));
    QCOMPARE( QString("`ABYZ{").toUpper(), QString("`ABYZ{"));
    QCOMPARE( QString("`abyz{").toUpper(), QString("`ABYZ{"));

    QCOMPARE( QString(1, QChar(0xdf)).toUpper(), QString("SS"));
    {
        QString s = QString::fromUtf8("Gro\xc3\x9fstra\xc3\x9f""e");

        // call lvalue-ref version, mustn't change the original
        QCOMPARE(s.toUpper(), QString("GROSSSTRASSE"));
        QCOMPARE(s, QString::fromUtf8("Gro\xc3\x9fstra\xc3\x9f""e"));

        // call rvalue-ref while shared (the original mustn't change)
        QString copy = s;
        QCOMPARE(std::move(copy).toUpper(), QString("GROSSSTRASSE"));
        QCOMPARE(s, QString::fromUtf8("Gro\xc3\x9fstra\xc3\x9f""e"));

        // call rvalue-ref version on detached case
        copy.clear();
        QCOMPARE(std::move(s).toUpper(), QString("GROSSSTRASSE"));
    }

    QString lower, upper;
    lower += QChar(QChar::highSurrogate(0x10428));
    lower += QChar(QChar::lowSurrogate(0x10428));
    upper += QChar(QChar::highSurrogate(0x10400));
    upper += QChar(QChar::lowSurrogate(0x10400));
    QCOMPARE( lower.toUpper(), upper);
    lower += lower;
    upper += upper;
    QCOMPARE( lower.toUpper(), upper);

    // test for broken surrogate pair handling (low low hi low hi low)
    lower.prepend(QChar(QChar::lowSurrogate(0x10428)));
    lower.prepend(QChar(QChar::lowSurrogate(0x10428)));
    upper.prepend(QChar(QChar::lowSurrogate(0x10428)));
    upper.prepend(QChar(QChar::lowSurrogate(0x10428)));
    QCOMPARE(lower.toUpper(), upper);
    // test for broken surrogate pair handling (low low hi low hi low hi hi)
    lower += QChar(QChar::highSurrogate(0x10428));
    lower += QChar(QChar::highSurrogate(0x10428));
    upper += QChar(QChar::highSurrogate(0x10428));
    upper += QChar(QChar::highSurrogate(0x10428));
    QCOMPARE(lower.toUpper(), upper);

#if QT_CONFIG(icu)
    // test doesn't work with ICU support, since QChar is unaware of any locale
    QEXPECT_FAIL("", "test doesn't work with ICU support, since QChar is unaware of any locale", Continue);
    QVERIFY(false);
#else
    for (int i = 0; i < 65536; ++i) {
        QString str(1, QChar(i));
        QString upper = str.toUpper();
        QVERIFY(upper.length() >= 1);
        if (upper.length() == 1)
            QVERIFY(upper == QString(1, QChar(i).toUpper()));
    }
#endif // icu
}

void tst_QString::toLower()
{
    const QString s;
    QCOMPARE(s.toLower(), QString()); // lvalue
    QCOMPARE( QString().toLower(), QString() ); // rvalue
    QCOMPARE( QString("").toLower(), QString("") );
    QCOMPARE( QString("text").toLower(), QString("text") );
    QCOMPARE( QStringLiteral("Text").toLower(), QString("text") );
    QCOMPARE( QString("Text").toLower(), QString("text") );
    QCOMPARE( QString("tExt").toLower(), QString("text") );
    QCOMPARE( QString("teXt").toLower(), QString("text") );
    QCOMPARE( QString("texT").toLower(), QString("text") );
    QCOMPARE( QString("TExt").toLower(), QString("text") );
    QCOMPARE( QString("teXT").toLower(), QString("text") );
    QCOMPARE( QString("tEXt").toLower(), QString("text") );
    QCOMPARE( QString("tExT").toLower(), QString("text") );
    QCOMPARE( QString("TEXT").toLower(), QString("text") );
    QCOMPARE( QString("@ABYZ[").toLower(), QString("@abyz["));
    QCOMPARE( QString("@abyz[").toLower(), QString("@abyz["));
    QCOMPARE( QString("`ABYZ{").toLower(), QString("`abyz{"));
    QCOMPARE( QString("`abyz{").toLower(), QString("`abyz{"));

    QCOMPARE( QString(1, QChar(0x130)).toLower(), QString(QString(1, QChar(0x69)) + QChar(0x307)));

    QString lower, upper;
    lower += QChar(QChar::highSurrogate(0x10428));
    lower += QChar(QChar::lowSurrogate(0x10428));
    upper += QChar(QChar::highSurrogate(0x10400));
    upper += QChar(QChar::lowSurrogate(0x10400));
    QCOMPARE( upper.toLower(), lower);
    lower += lower;
    upper += upper;
    QCOMPARE( upper.toLower(), lower);

    // test for broken surrogate pair handling (low low hi low hi low)
    lower.prepend(QChar(QChar::lowSurrogate(0x10400)));
    lower.prepend(QChar(QChar::lowSurrogate(0x10400)));
    upper.prepend(QChar(QChar::lowSurrogate(0x10400)));
    upper.prepend(QChar(QChar::lowSurrogate(0x10400)));
    QCOMPARE( upper.toLower(), lower);
    // test for broken surrogate pair handling (low low hi low hi low hi hi)
    lower += QChar(QChar::highSurrogate(0x10400));
    lower += QChar(QChar::highSurrogate(0x10400));
    upper += QChar(QChar::highSurrogate(0x10400));
    upper += QChar(QChar::highSurrogate(0x10400));
    QCOMPARE( upper.toLower(), lower);

#if QT_CONFIG(icu)
    // test doesn't work with ICU support, since QChar is unaware of any locale
    QEXPECT_FAIL("", "test doesn't work with ICU support, since QChar is unaware of any locale", Continue);
    QVERIFY(false);
#else
    for (int i = 0; i < 65536; ++i) {
        QString str(1, QChar(i));
        QString lower = str.toLower();
        QVERIFY(lower.length() >= 1);
        if (lower.length() == 1)
            QVERIFY(str.toLower() == QString(1, QChar(i).toLower()));
    }
#endif // icu
}

void tst_QString::isLower_isUpper_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<bool>("isLower");
    QTest::addColumn<bool>("isUpper");

    int row = 0;
    QTest::addRow("lower-and-upper-%02d", row++) << QString() << true << true;
    QTest::addRow("lower-and-upper-%02d", row++) << QString("") << true << true;
    QTest::addRow("lower-and-upper-%02d", row++) << QString(" ") << true << true;
    QTest::addRow("lower-and-upper-%02d", row++) << QString("123") << true << true;
    QTest::addRow("lower-and-upper-%02d", row++) << QString("@123$#") << true << true;
    QTest::addRow("lower-and-upper-%02d", row++) << QString("𝄞𝄴𝆏♫") << true << true; // Unicode Block 'Musical Symbols'
    // not foldable
    QTest::addRow("lower-and-upper-%02d", row++) << QString("𝚊𝚋𝚌𝚍𝚎") << true << true; // MATHEMATICAL MONOSPACE SMALL A, ... E
    QTest::addRow("lower-and-upper-%02d", row++) << QString("𝙖,𝙗,𝙘,𝙙,𝙚") << true << true; // MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL A, ... E
    QTest::addRow("lower-and-upper-%02d", row++) << QString("𝗔𝗕𝗖𝗗𝗘") << true << true; // MATHEMATICAL SANS-SERIF BOLD CAPITAL A, ... E
    QTest::addRow("lower-and-upper-%02d", row++) << QString("𝐀,𝐁,𝐂,𝐃,𝐄") << true << true; // MATHEMATICAL BOLD CAPITAL A, ... E

    row = 0;
    QTest::addRow("only-lower-%02d", row++) << QString("text") << true << false;
    QTest::addRow("only-lower-%02d", row++) << QString("àaa") << true << false;
    QTest::addRow("only-lower-%02d", row++) << QString("øæß") << true << false;
    QTest::addRow("only-lower-%02d", row++) << QString("text ") << true << false;
    QTest::addRow("only-lower-%02d", row++) << QString(" text") << true << false;
    QTest::addRow("only-lower-%02d", row++) << QString("hello, world!") << true << false;
    QTest::addRow("only-lower-%02d", row++) << QString("123@abyz[") << true << false;
    QTest::addRow("only-lower-%02d", row++) << QString("`abyz{") << true << false;
    QTest::addRow("only-lower-%02d", row++) << QString("a𝙖a|b𝙗b|c𝙘c|d𝙙d|e𝙚e") << true << false; // MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL A, ... E
    QTest::addRow("only-lower-%02d", row++) << QString("𐐨") << true << false; // DESERET SMALL LETTER LONG I
    // uppercase letters, not foldable
    QTest::addRow("only-lower-%02d", row++) << QString("text𝗔text") << true << false; // MATHEMATICAL SANS-SERIF BOLD CAPITAL A

    row = 0;
    QTest::addRow("only-upper-%02d", row++) << QString("TEXT") << false << true;
    QTest::addRow("only-upper-%02d", row++) << QString("ÀAA") << false << true;
    QTest::addRow("only-upper-%02d", row++) << QString("ØÆẞ") << false << true;
    QTest::addRow("only-upper-%02d", row++) << QString("TEXT ") << false << true;
    QTest::addRow("only-upper-%02d", row++) << QString(" TEXT") << false << true;
    QTest::addRow("only-upper-%02d", row++) << QString("HELLO, WORLD!") << false << true;
    QTest::addRow("only-upper-%02d", row++) << QString("123@ABYZ[") << false << true;
    QTest::addRow("only-upper-%02d", row++) << QString("`ABYZ{") << false << true;
    QTest::addRow("only-upper-%02d", row++) << QString("A𝐀A|B𝐁B|C𝐂C|D𝐃D|E𝐄E") << false << true; // MATHEMATICAL BOLD CAPITAL A, ... E
    QTest::addRow("only-upper-%02d", row++) << QString("𐐀") << false << true; // DESERET CAPITAL LETTER LONG I
    // lowercase letters, not foldable
    QTest::addRow("only-upper-%02d", row++) << QString("TEXT𝚊TEXT") << false << true; // MATHEMATICAL MONOSPACE SMALL A

    row = 0;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("Text") << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("tExt") << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("teXt") << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("texT") << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("TExt") << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("teXT") << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("tEXt") << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("tExT") << false << false;
    // not foldable
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("TEXT𝚊text") << false << false; // MATHEMATICAL MONOSPACE SMALL A
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("text𝗔TEXT") << false << false; // MATHEMATICAL SANS-SERIF BOLD CAPITAL A
    // titlecase, foldable
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("abcǈdef") << false << false; // LATIN CAPITAL LETTER L WITH SMALL LETTER J
    QTest::addRow("not-lower-nor-upper-%02d", row++) << QString("ABCǈDEF") << false << false; // LATIN CAPITAL LETTER L WITH SMALL LETTER J
}

void tst_QString::isLower_isUpper()
{
    QFETCH(QString, string);
    QFETCH(bool, isLower);
    QFETCH(bool, isUpper);

    QCOMPARE(string.isLower(), isLower);
    QCOMPARE(string.toLower() == string, isLower);
    QVERIFY(string.toLower().isLower());

    QCOMPARE(string.isUpper(), isUpper);
    QCOMPARE(string.toUpper() == string, isUpper);
    QVERIFY(string.toUpper().isUpper());
}

void tst_QString::toCaseFolded()
{
    const QString s;
    QCOMPARE( s.toCaseFolded(), QString() ); // lvalue
    QCOMPARE( QString().toCaseFolded(), QString() ); // rvalue
    QCOMPARE( QString("").toCaseFolded(), QString("") );
    QCOMPARE( QString("text").toCaseFolded(), QString("text") );
    QCOMPARE( QString("Text").toCaseFolded(), QString("text") );
    QCOMPARE( QString("tExt").toCaseFolded(), QString("text") );
    QCOMPARE( QString("teXt").toCaseFolded(), QString("text") );
    QCOMPARE( QString("texT").toCaseFolded(), QString("text") );
    QCOMPARE( QString("TExt").toCaseFolded(), QString("text") );
    QCOMPARE( QString("teXT").toCaseFolded(), QString("text") );
    QCOMPARE( QString("tEXt").toCaseFolded(), QString("text") );
    QCOMPARE( QString("tExT").toCaseFolded(), QString("text") );
    QCOMPARE( QString("TEXT").toCaseFolded(), QString("text") );
    QCOMPARE( QString("@ABYZ[").toCaseFolded(), QString("@abyz["));
    QCOMPARE( QString("@abyz[").toCaseFolded(), QString("@abyz["));
    QCOMPARE( QString("`ABYZ{").toCaseFolded(), QString("`abyz{"));
    QCOMPARE( QString("`abyz{").toCaseFolded(), QString("`abyz{"));

    QCOMPARE( QString(1, QChar(0xa77d)).toCaseFolded(), QString(1, QChar(0x1d79)));
    QCOMPARE( QString(1, QChar(0xa78d)).toCaseFolded(), QString(1, QChar(0x0265)));

    QString lower, upper;
    upper += QChar(QChar::highSurrogate(0x10400));
    upper += QChar(QChar::lowSurrogate(0x10400));
    lower += QChar(QChar::highSurrogate(0x10428));
    lower += QChar(QChar::lowSurrogate(0x10428));
    QCOMPARE( upper.toCaseFolded(), lower);
    lower += lower;
    upper += upper;
    QCOMPARE( upper.toCaseFolded(), lower);

    // test for broken surrogate pair handling (low low hi low hi low)
    lower.prepend(QChar(QChar::lowSurrogate(0x10400)));
    lower.prepend(QChar(QChar::lowSurrogate(0x10400)));
    upper.prepend(QChar(QChar::lowSurrogate(0x10400)));
    upper.prepend(QChar(QChar::lowSurrogate(0x10400)));
    QCOMPARE(upper.toCaseFolded(), lower);
    // test for broken surrogate pair handling (low low hi low hi low hi hi)
    lower += QChar(QChar::highSurrogate(0x10400));
    lower += QChar(QChar::highSurrogate(0x10400));
    upper += QChar(QChar::highSurrogate(0x10400));
    upper += QChar(QChar::highSurrogate(0x10400));
    QCOMPARE(upper.toCaseFolded(), lower);

    //### we currently don't support full case foldings
    for (int i = 0; i < 65536; ++i) {
        QString str(1, QChar(i));
        QString lower = str.toCaseFolded();
        QVERIFY(lower.length() >= 1);
        if (lower.length() == 1)
            QVERIFY(str.toCaseFolded() == QString(1, QChar(i).toCaseFolded()));
    }
}

void tst_QString::trimmed()
{
    QString a;

    QVERIFY(a.trimmed().isNull()); // lvalue
    QVERIFY(QString().trimmed().isNull()); // rvalue
    QVERIFY(!a.isDetached());

    a="Text";
    QCOMPARE(a, QLatin1String("Text"));
    QCOMPARE(a.trimmed(), QLatin1String("Text"));
    QCOMPARE(a, QLatin1String("Text"));
    a=" ";
    QCOMPARE(a.trimmed(), QLatin1String(""));
    QCOMPARE(a, QLatin1String(" "));
    a=" a   ";
    QCOMPARE(a.trimmed(), QLatin1String("a"));

    a="Text";
    QCOMPARE(std::move(a).trimmed(), QLatin1String("Text"));
    a=" ";
    QCOMPARE(std::move(a).trimmed(), QLatin1String(""));
    a=" a   ";
    QCOMPARE(std::move(a).trimmed(), QLatin1String("a"));
}

void tst_QString::simplified_data()
{
    QTest::addColumn<QString>("full" );
    QTest::addColumn<QString>("simple" );

    QTest::newRow("null") << QString() << QString();
    QTest::newRow("empty") << "" << "";
    QTest::newRow("one char") << "a" << "a";
    QTest::newRow("one word") << "foo" << "foo";
    QTest::newRow("chars trivial") << "a b" << "a b";
    QTest::newRow("words trivial") << "foo bar" << "foo bar";
    QTest::newRow("allspace") << "  \t\v " << "";
    QTest::newRow("char trailing") << "a " << "a";
    QTest::newRow("char trailing tab") << "a\t" << "a";
    QTest::newRow("char multitrailing") << "a   " << "a";
    QTest::newRow("char multitrailing tab") << "a   \t" << "a";
    QTest::newRow("char leading") << " a" << "a";
    QTest::newRow("char leading tab") << "\ta" << "a";
    QTest::newRow("char multileading") << "   a" << "a";
    QTest::newRow("char multileading tab") << "\t   a" << "a";
    QTest::newRow("chars apart") << "a  b" << "a b";
    QTest::newRow("words apart") << "foo  bar" << "foo bar";
    QTest::newRow("enclosed word") << "   foo \t " << "foo";
    QTest::newRow("enclosed chars apart") << " a   b " << "a b";
    QTest::newRow("enclosed words apart") << " foo   bar " << "foo bar";
    QTest::newRow("chars apart posttab") << "a \tb" << "a b";
    QTest::newRow("chars apart pretab") << "a\t b" << "a b";
    QTest::newRow("many words") << "  just some    random\ttext here" << "just some random text here";
    QTest::newRow("newlines") << "a\nb\nc" << "a b c";
    QTest::newRow("newlines-trailing") << "a\nb\nc\n" << "a b c";
}

void tst_QString::simplified()
{
    QFETCH(QString, full);
    QFETCH(QString, simple);

    QString orig_full = full;
    orig_full.data();       // forces a detach

    QString result = full.simplified();
    if (simple.isNull()) {
        QVERIFY2(result.isNull(), qPrintable("'" + full + "' did not yield null: " + result));
    } else if (simple.isEmpty()) {
        QVERIFY2(result.isEmpty() && !result.isNull(), qPrintable("'" + full + "' did not yield empty: " + result));
    } else {
        QCOMPARE(result, simple);
    }
    QCOMPARE(full, orig_full);

    // without detaching:
    QString copy1 = full;
    QCOMPARE(std::move(full).simplified(), simple);
    QCOMPARE(full, orig_full);

    // force a detach
    if (!full.isEmpty())
        full[0] = full[0];
    QCOMPARE(std::move(full).simplified(), simple);
}

void tst_QString::insert_data(DataOptions options)
{
    QTest::addColumn<QString>("s");
    QTest::addColumn<CharStarContainer>("arg");
    QTest::addColumn<int>("a1");
    QTest::addColumn<QString>("expected");

    const bool emptyIsNoop = options.testFlag(EmptyIsNoop);

    const CharStarContainer nullC;
    const CharStarContainer emptyC("");
    const CharStarContainer aC("a");
    const CharStarContainer bC("b");
    //const CharStarContainer abC("ab");
    const CharStarContainer baC("ba");
    const CharStarContainer yumlautC(options.testFlag(Latin1Encoded) ? "\xff" : "\xc3\xbf");

    const QString null;
    const QString empty("");
    const QString a("a");
    const QString b("b");
    const QString ab("ab");
    const QString ba("ba");

    const QString yumlaut = QStringLiteral("\u00ff");           // LATIN LETTER SMALL Y WITH UMLAUT
    const QString yumlautA = QStringLiteral("\u00ffa");
    const QString aYumlaut = QStringLiteral("a\u00ff");

    QTest::newRow("null.insert(0, null)") << null << nullC << 0 << null;
    QTest::newRow("null.insert(0, empty)") << null << emptyC << 0 << (emptyIsNoop ? null : empty);
    QTest::newRow("null.insert(0, a)") << null << aC << 0 << a;
    QTest::newRow("empty.insert(0, null)") << empty << nullC << 0 << empty;
    QTest::newRow("empty.insert(0, empty)") << empty << emptyC << 0 << empty;
    QTest::newRow("empty.insert(0, a)") << empty << aC << 0 << a;
    QTest::newRow("a.insert(0, null)") << a << nullC << 0 << a;
    QTest::newRow("a.insert(0, empty)") << a << emptyC << 0 << a;
    QTest::newRow("a.insert(0, b)") << a << bC << 0 << ba;
    QTest::newRow("a.insert(0, ba)") << a << baC << 0 << (ba + a);
    QTest::newRow("a.insert(1, null)") << a << nullC << 1 << a;
    QTest::newRow("a.insert(1, empty)") << a << emptyC << 1 << a;
    QTest::newRow("a.insert(1, b)") << a << bC << 1 << ab;
    QTest::newRow("a.insert(1, ba)") << a << baC << 1 << (a + ba);
    QTest::newRow("ba.insert(1, a)") << ba << aC << 1 << (ba + a);
    QTest::newRow("ba.insert(2, b)") << ba << bC << 2 << (ba + b);

    QTest::newRow("null-insert-0-yumlaut") << null << yumlautC << 0 << yumlaut;
    QTest::newRow("empty-insert-0-yumlaut") << empty << yumlautC << 0 << yumlaut;
    QTest::newRow("yumlaut-insert-0-null") << yumlaut << nullC << 0 << yumlaut;
    QTest::newRow("yumlaut-insert-0-empty") << yumlaut << emptyC << 0 << yumlaut;
    QTest::newRow("a-insert-0-yumlaut") << a << yumlautC << 0 << yumlautA;
    QTest::newRow("a-insert-1-yumlaut") << a << yumlautC << 1 << aYumlaut;

    if (!options.testFlag(Latin1Encoded)) {
        const auto smallTheta = QStringLiteral("\u03b8");      // GREEK LETTER SMALL THETA
        const auto ssa = QStringLiteral("\u0937");             // DEVANAGARI LETTER SSA
        const auto chakmaZero = QStringLiteral("\U00011136");  // CHAKMA DIGIT ZERO

        const auto aSmallTheta = QStringLiteral("a\u03b8");
        const auto aSsa = QStringLiteral("a\u0937");
        const auto aChakmaZero = QStringLiteral("a\U00011136");

        const auto smallThetaA = QStringLiteral("\u03b8a");
        const auto ssaA = QStringLiteral("\u0937a");
        const auto chakmaZeroA = QStringLiteral("\U00011136a");

        const auto umlautTheta = QStringLiteral("\u00ff\u03b8");
        const auto thetaUmlaut = QStringLiteral("\u03b8\u00ff");
        const auto ssaChakma = QStringLiteral("\u0937\U00011136");
        const auto chakmaSsa = QStringLiteral("\U00011136\u0937");

        const CharStarContainer smallThetaC("\xce\xb8");           // non-Latin1
        const CharStarContainer ssaC("\xe0\xa4\xb7");              // Higher BMP
        const CharStarContainer chakmaZeroC("\xf0\x91\x84\xb6");   // Non-BMP

        QTest::newRow("null-insert-0-theta") << null << smallThetaC << 0 << smallTheta;
        QTest::newRow("null-insert-0-ssa") << null << ssaC << 0 << ssa;
        QTest::newRow("null-insert-0-chakma") << null << chakmaZeroC << 0 << chakmaZero;

        QTest::newRow("empty-insert-0-theta") << empty << smallThetaC << 0 << smallTheta;
        QTest::newRow("empty-insert-0-ssa") << empty << ssaC << 0 << ssa;
        QTest::newRow("empty-insert-0-chakma") << empty << chakmaZeroC << 0 << chakmaZero;

        QTest::newRow("theta-insert-0-null") << smallTheta << nullC << 0 << smallTheta;
        QTest::newRow("ssa-insert-0-null") << ssa << nullC << 0 << ssa;
        QTest::newRow("chakma-insert-0-null") << chakmaZero << nullC << 0 << chakmaZero;

        QTest::newRow("theta-insert-0-empty") << smallTheta << emptyC << 0 << smallTheta;
        QTest::newRow("ssa-insert-0-empty") << ssa << emptyC << 0 << ssa;
        QTest::newRow("chakma-insert-0-empty") << chakmaZero << emptyC << 0 << chakmaZero;

        QTest::newRow("a-insert-0-theta") << a << smallThetaC << 0 << smallThetaA;
        QTest::newRow("a-insert-0-ssa") << a << ssaC << 0 << ssaA;
        QTest::newRow("a-insert-0-chakma") << a << chakmaZeroC << 0 << chakmaZeroA;
        QTest::newRow("yumlaut-insert-0-theta") << yumlaut << smallThetaC << 0 << thetaUmlaut;
        QTest::newRow("theta-insert-0-yumlaut") << smallTheta << yumlautC << 0 << umlautTheta;
        QTest::newRow("ssa-insert-0-chakma") << ssa << chakmaZeroC << 0 << chakmaSsa;
        QTest::newRow("chakma-insert-0-ssa") << chakmaZero << ssaC << 0 << ssaChakma;

        QTest::newRow("theta-insert-1-null") << smallTheta << nullC << 1 << smallTheta;
        QTest::newRow("ssa-insert-1-null") << ssa << nullC << 1 << ssa;
        QTest::newRow("chakma-insert-1-null") << chakmaZero << nullC << 1 << chakmaZero;

        QTest::newRow("theta-insert-1-empty") << smallTheta << emptyC << 1 << smallTheta;
        QTest::newRow("ssa-insert-1-empty") << ssa << emptyC << 1 << ssa;
        QTest::newRow("chakma-insert-1-empty") << chakmaZero << emptyC << 1 << chakmaZero;

        QTest::newRow("a-insert-1-theta") << a << smallThetaC << 1 << aSmallTheta;
        QTest::newRow("a-insert-1-ssa") << a << ssaC << 1 << aSsa;
        QTest::newRow("a-insert-1-chakma") << a << chakmaZeroC << 1 << aChakmaZero;
        QTest::newRow("yumlaut-insert-1-theta") << yumlaut << smallThetaC << 1 << umlautTheta;
        QTest::newRow("theta-insert-1-yumlaut") << smallTheta << yumlautC << 1 << thetaUmlaut;
        QTest::newRow("ssa-insert-1-chakma") << ssa << chakmaZeroC << 1 << ssaChakma;
        // Beware, this will insert ssa right into the middle of the chakma:
        // Actual   (s)       : "\uD804\u0937\uDD36"
        // Expected (expected): "\uD804\uDD36\u0937"
        // QTest::newRow("chakma.insert(1, ssa)") << chakmaZero << ssaC << 1 << chakmaSsa;
    }
}

void tst_QString::insert_special_cases()
{
    QString a;

    a = "Ys";
    QCOMPARE(a.insert(1,'e'), QString("Yes"));
    QCOMPARE(a.insert(3,'!'), QString("Yes!"));
    QCOMPARE(a.insert(5,'?'), QString("Yes! ?"));
    QCOMPARE(a.insert(-1,'a'), QString("Yes! a?"));

    a = "ABC";
    QCOMPARE(a.insert(5,"DEF"), QString("ABC  DEF"));

    a = "ABC";
    QCOMPARE(a.insert(2, QString()), QString("ABC"));
    QCOMPARE(a.insert(0,"ABC"), QString("ABCABC"));
    QCOMPARE(a, QString("ABCABC"));
    QCOMPARE(a.insert(0,a), QString("ABCABCABCABC"));

    QCOMPARE(a, QString("ABCABCABCABC"));
    QCOMPARE(a.insert(0,'<'), QString("<ABCABCABCABC"));
    QCOMPARE(a.insert(1,'>'), QString("<>ABCABCABCABC"));

    a = "Meal";
    const QString montreal = QStringLiteral("Montreal");
    QCOMPARE(a.insert(1, QLatin1String("ontr")), montreal);
    QCOMPARE(a.insert(4, ""), montreal);
    QCOMPARE(a.insert(3, QLatin1String("")), montreal);
    QCOMPARE(a.insert(3, QLatin1String(0)), montreal);
    QCOMPARE(a.insert(3, static_cast<const char *>(0)), montreal);
    QCOMPARE(a.insert(0, QLatin1String("a")), QLatin1String("aMontreal"));

    a = "Mont";
    QCOMPARE(a.insert(a.size(), QLatin1String("real")), montreal);
    QCOMPARE(a.insert(a.size() + 1, QLatin1String("ABC")), QString("Montreal ABC"));

    a = "AEF";
    QCOMPARE(a.insert(1, QLatin1String("BCD")), QString("ABCDEF"));
    QCOMPARE(a.insert(3, QLatin1String("-")), QString("ABC-DEF"));
    QCOMPARE(a.insert(a.size() + 1, QLatin1String("XYZ")), QString("ABC-DEF XYZ"));

    {
        a = "one";
        a.prepend(u'a');
        QString b(a.data_ptr()->freeSpaceAtEnd(), u'b');
        QCOMPARE(a.insert(a.size() + 1, QLatin1String(b.toLatin1())), QString("aone ") + b);
    }

    {
        a = "onetwothree";
        while (a.size() - 1)
            a.remove(0, 1);
        QString b(a.data_ptr()->freeSpaceAtEnd() + 1, u'b');
        QCOMPARE(a.insert(a.size() + 1, QLatin1String(b.toLatin1())), QString("e ") + b);
    }

    {
        a = "one";
        a.prepend(u'a');
        QString b(a.data_ptr()->freeSpaceAtEnd(), u'b');
        QCOMPARE(a.insert(a.size() + 1, b), QString("aone ") + b);
    }

    {
        a = "onetwothree";
        while (a.size() - 1)
            a.remove(0, 1);
        QString b(a.data_ptr()->freeSpaceAtEnd() + 1, u'b');
        QCOMPARE(a.insert(a.size() + 1, b), QString("e ") + b);
    }
}

void tst_QString::append_data(DataOptions options)
{
    QTest::addColumn<QString>("s");
    QTest::addColumn<CharStarContainer>("arg");
    QTest::addColumn<QString>("expected");

    const bool emptyIsNoop = options.testFlag(EmptyIsNoop);

    const CharStarContainer nullC;
    const CharStarContainer emptyC("");
    const CharStarContainer aC("a");
    const CharStarContainer bC("b");
    //const CharStarContainer abC("ab");
    const CharStarContainer yumlautC(options.testFlag(Latin1Encoded) ? "\xff" : "\xc3\xbf");

    const QString null;
    const QString empty("");
    const QString a("a");
    //const QString b("b");
    const QString ab("ab");

    const QString yumlaut = QStringLiteral("\u00ff");           // LATIN LETTER SMALL Y WITH UMLAUT
    const QString aYumlaut = QStringLiteral("a\u00ff");

    QTest::newRow("null + null") << null << nullC << null;
    QTest::newRow("null + empty") << null << emptyC << (emptyIsNoop ? null : empty);
    QTest::newRow("null + a") << null << aC << a;
    QTest::newRow("empty + null") << empty << nullC << empty;
    QTest::newRow("empty + empty") << empty << emptyC << empty;
    QTest::newRow("empty + a") << empty << aC << a;
    QTest::newRow("a + null") << a << nullC << a;
    QTest::newRow("a + empty") << a << emptyC << a;
    QTest::newRow("a + b") << a << bC << ab;

    QTest::newRow("null+yumlaut") << null << yumlautC << yumlaut;
    QTest::newRow("empty+yumlaut") << empty << yumlautC << yumlaut;
    QTest::newRow("a+yumlaut") << a << yumlautC << aYumlaut;

    if (!options.testFlag(Latin1Encoded)) {
        const auto smallTheta = QStringLiteral("\u03b8");        // GREEK LETTER SMALL THETA
        const auto ssa = QStringLiteral("\u0937");               // DEVANAGARI LETTER SSA
        const auto chakmaZero = QStringLiteral("\U00011136");    // CHAKMA DIGIT ZERO

        const auto aSmallTheta = QStringLiteral("a\u03b8");
        const auto aSsa = QStringLiteral("a\u0937");
        const auto aChakmaZero = QStringLiteral("a\U00011136");

        const auto thetaChakma = QStringLiteral("\u03b8\U00011136");
        const auto chakmaTheta = QStringLiteral("\U00011136\u03b8");
        const auto ssaTheta = QStringLiteral("\u0937\u03b8");
        const auto thetaSsa = QStringLiteral("\u03b8\u0937");
        const auto ssaChakma = QStringLiteral("\u0937\U00011136");
        const auto chakmaSsa = QStringLiteral("\U00011136\u0937");
        const auto thetaUmlaut = QStringLiteral("\u03b8\u00ff");
        const auto umlautTheta = QStringLiteral("\u00ff\u03b8");
        const auto ssaUmlaut = QStringLiteral("\u0937\u00ff");
        const auto umlautSsa = QStringLiteral("\u00ff\u0937");
        const auto chakmaUmlaut = QStringLiteral("\U00011136\u00ff");
        const auto umlautChakma = QStringLiteral("\u00ff\U00011136");

        const CharStarContainer smallThetaC("\xce\xb8");           // non-Latin1
        const CharStarContainer ssaC("\xe0\xa4\xb7");              // Higher BMP
        const CharStarContainer chakmaZeroC("\xf0\x91\x84\xb6");   // Non-BMP

        QTest::newRow("null+smallTheta") << null << smallThetaC << smallTheta;
        QTest::newRow("empty+smallTheta") << empty << smallThetaC << smallTheta;
        QTest::newRow("a+smallTheta") << a << smallThetaC << aSmallTheta;

        QTest::newRow("null+ssa") << null << ssaC << ssa;
        QTest::newRow("empty+ssa") << empty << ssaC << ssa;
        QTest::newRow("a+ssa") << a << ssaC << aSsa;

        QTest::newRow("null+chakma") << null << chakmaZeroC << chakmaZero;
        QTest::newRow("empty+chakma") << empty << chakmaZeroC << chakmaZero;
        QTest::newRow("a+chakma") << a << chakmaZeroC << aChakmaZero;

        QTest::newRow("smallTheta+chakma") << smallTheta << chakmaZeroC << thetaChakma;
        QTest::newRow("chakma+smallTheta") << chakmaZero << smallThetaC << chakmaTheta;
        QTest::newRow("smallTheta+ssa") << smallTheta << ssaC << thetaSsa;

        QTest::newRow("ssa+smallTheta") << ssa << smallThetaC << ssaTheta;
        QTest::newRow("ssa+chakma") << ssa << chakmaZeroC << ssaChakma;
        QTest::newRow("chakma+ssa") << chakmaZero << ssaC << chakmaSsa;

        QTest::newRow("smallTheta+yumlaut") << smallTheta << yumlautC << thetaUmlaut;
        QTest::newRow("yumlaut+smallTheta") << yumlaut << smallThetaC << umlautTheta;
        QTest::newRow("ssa+yumlaut") << ssa << yumlautC << ssaUmlaut;
        QTest::newRow("yumlaut+ssa") << yumlaut << ssaC << umlautSsa;
        QTest::newRow("chakma+yumlaut") << chakmaZero << yumlautC << chakmaUmlaut;
        QTest::newRow("yumlaut+chakma") << yumlaut << chakmaZeroC << umlautChakma;
    }
}

void tst_QString::append_special_cases()
{
    {
        QString a;
        static const QChar unicode[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
        a.append(unicode, sizeof unicode / sizeof *unicode);
        QCOMPARE(a, QLatin1String("Hello, World!"));
        static const QChar nl('\n');
        a.append(&nl, 1);
        QCOMPARE(a, QLatin1String("Hello, World!\n"));
        a.append(unicode, sizeof unicode / sizeof *unicode);
        QCOMPARE(a, QLatin1String("Hello, World!\nHello, World!"));
        a.append(unicode, 0); // no-op
        QCOMPARE(a, QLatin1String("Hello, World!\nHello, World!"));
        a.append(unicode, -1); // no-op
        QCOMPARE(a, QLatin1String("Hello, World!\nHello, World!"));
        a.append(0, 1); // no-op
        QCOMPARE(a, QLatin1String("Hello, World!\nHello, World!"));
    }

    {
        QString a;
        a.insert(0, QChar(u'A'));
        QCOMPARE(a.size(), 1);
        QVERIFY(a.capacity() > 0);
        a.append(QLatin1String("BC"));
        QCOMPARE(a, QLatin1String("ABC"));
    }

    {
        QString a = "one";
        a.prepend(u'a');
        QString b(a.data_ptr()->freeSpaceAtEnd(), u'b');
        QCOMPARE(a.append(QLatin1String(b.toLatin1())), QString("aone") + b);
    }

    {
        QString a = "onetwothree";
        while (a.size() - 1)
            a.remove(0, 1);
        QString b(a.data_ptr()->freeSpaceAtEnd(), u'b');
        QCOMPARE(a.append(QLatin1String(b.toLatin1())), QString("e") + b);
    }

    {
        QString a = "one";
        a.prepend(u'a');
        QString b(a.data_ptr()->freeSpaceAtEnd(), u'b');
        QCOMPARE(a.append(b), QString("aone") + b);
    }

    {
        QString a = "onetwothree";
        while (a.size() - 1)
            a.remove(0, 1);
        QString b(a.data_ptr()->freeSpaceAtEnd() + 1, u'b');
        QCOMPARE(a.append(b), QString("e") + b);
    }

    {
        QString a = "one";
        a.prepend(u'a');
        QCOMPARE(a.append(u'b'), QString("aoneb"));
    }

    {
        QString a = "onetwothree";
        while (a.size() - 1)
            a.remove(0, 1);
        QCOMPARE(a.append(u'b'), QString("eb"));
    }
}

void tst_QString::append_bytearray_special_cases_data()
{
    QTest::addColumn<QString>("str" );
    QTest::addColumn<QByteArray>("ba" );
    QTest::addColumn<QString>("res" );

    QByteArray ba( 5, 0 );
    ba[0] = 'a';
    ba[1] = 'b';
    ba[2] = 'c';
    ba[3] = 'd';

    // no 0 termination
    ba.resize( 4 );
    QTest::newRow( "notTerminated_0" ) << QString() << ba << QString("abcd");
    QTest::newRow( "notTerminated_1" ) << QString("") << ba << QString("abcd");
    QTest::newRow( "notTerminated_2" ) << QString("foobar ") << ba << QString("foobar abcd");

    // byte array with only a 0
    ba.resize( 1 );
    ba[0] = 0;
    QByteArray ba2("foobar ");
    ba2.append('\0');
    QTest::newRow( "emptyString" ) << QString("foobar ") << ba << QString(ba2);

    // empty byte array
    ba.resize( 0 );
    QTest::newRow( "emptyByteArray" ) << QString("foobar ") << ba << QString("foobar ");

    // non-ascii byte array
    QTest::newRow( "nonAsciiByteArray") << QString() << QByteArray("\xc3\xa9") << QString("\xc3\xa9");
    QTest::newRow( "nonAsciiByteArray2") << QString() << QByteArray("\xc3\xa9") << QString::fromUtf8("\xc3\xa9");
}

void tst_QString::append_bytearray_special_cases()
{
    {
        QFETCH( QString, str );
        QFETCH( QByteArray, ba );

        str.append( ba );

        QTEST( str, "res" );
    }
    {
        QFETCH( QString, str );
        QFETCH( QByteArray, ba );

        str.append( ba );

        QTEST( str, "res" );
    }

    QFETCH( QByteArray, ba );
    if (!ba.contains('\0') && ba.constData()[ba.length()] == '\0') {
        QFETCH( QString, str );

        str.append(ba.constData());
        QTEST( str, "res" );
    }
}

void tst_QString::appendFromRawData()
{
    const char16_t utf[] = u"Hello World!";
    auto *rawData = reinterpret_cast<const QChar *>(utf);
    QString str = QString::fromRawData(rawData, std::size(utf) - 1);

    QString copy;
    copy.append(str);
    QCOMPARE(copy, str);
    // We make an _actual_ copy, because appending a byte array
    // created with fromRawData() might be optimized to copy the DataPointer,
    // which means we may point to temporary stack data.
    QVERIFY(copy.constData() != str.constData());
}

void tst_QString::operator_pluseq_special_cases()
{
    {
        QString a;
        a += QChar::CarriageReturn;
        a += '\r';
        a += u'\x1111';
        QCOMPARE(a, QStringView(u"\r\r\x1111"));
    }
}

void tst_QString::operator_pluseq_data(DataOptions options)
{
    append_data(options);
}

void tst_QString::operator_pluseq_bytearray_special_cases_data()
{
    append_bytearray_special_cases_data();
}

void tst_QString::operator_pluseq_bytearray_special_cases()
{
    {
        QFETCH( QString, str );
        QFETCH( QByteArray, ba );

        str += ba;

        QTEST( str, "res" );
    }
    {
        QFETCH( QString, str );
        QFETCH( QByteArray, ba );

        str += ba;

        QTEST( str, "res" );
    }

    QFETCH( QByteArray, ba );
    if (!ba.contains('\0') && ba.constData()[ba.length()] == '\0') {
        QFETCH( QString, str );

        str += ba.constData();
        QTEST( str, "res" );
    }
}

void tst_QString::operator_eqeq_bytearray_data()
{
    constructorQByteArray_data();
}

void tst_QString::operator_eqeq_bytearray()
{
    QFETCH(QByteArray, src);
    QFETCH(QString, expected);

    QVERIFY(expected == src);
    QVERIFY(!(expected != src));

    if (!src.contains('\0') && src.constData()[src.length()] == '\0') {
        QVERIFY(expected == src.constData());
        QVERIFY(!(expected != src.constData()));
    }
}

void tst_QString::swap()
{
    QString s1, s2;
    s1 = "s1";
    s2 = "s2";
    s1.swap(s2);
    QCOMPARE(s1,QLatin1String("s2"));
    QCOMPARE(s2,QLatin1String("s1"));
}

void tst_QString::prepend_data(DataOptions options)
{
    QTest::addColumn<QString>("s");
    QTest::addColumn<CharStarContainer>("arg");
    QTest::addColumn<QString>("expected");

    const bool emptyIsNoop = options.testFlag(EmptyIsNoop);

    const CharStarContainer nullC;
    const CharStarContainer emptyC("");
    const CharStarContainer aC("a");
    const CharStarContainer bC("b");
    const CharStarContainer baC("ba");
    const CharStarContainer yumlautC(options.testFlag(Latin1Encoded) ? "\xff" : "\xc3\xbf");

    const QString null;
    const QString empty("");
    const QString a("a");
    //const QString b("b");
    const QString ba("ba");

    const QString yumlaut = QStringLiteral("\u00ff");           // LATIN LETTER SMALL Y WITH UMLAUT
    const QString yumlautA = QStringLiteral("\u00ffa");

    QTest::newRow("null.prepend(null)") << null << nullC << null;
    QTest::newRow("null.prepend(empty)") << null << emptyC << (emptyIsNoop ? null : empty);
    QTest::newRow("null.prepend(a)") << null << aC << a;
    QTest::newRow("empty.prepend(null)") << empty << nullC << empty;
    QTest::newRow("empty.prepend(empty)") << empty << emptyC << empty;
    QTest::newRow("empty.prepend(a)") << empty << aC << a;
    QTest::newRow("a.prepend(null)") << a << nullC << a;
    QTest::newRow("a.prepend(empty)") << a << emptyC << a;
    QTest::newRow("a.prepend(b)") << a << bC << ba;
    QTest::newRow("a.prepend(ba)") << a << baC << (ba + a);

    QTest::newRow("null-prepend-yumlaut") << null << yumlautC << yumlaut;
    QTest::newRow("empty-prepend-yumlaut") << empty << yumlautC << yumlaut;
    QTest::newRow("a-prepend-yumlaut") << a << yumlautC << yumlautA;

    if (!options.testFlag(Latin1Encoded)) {
        const auto smallTheta = QStringLiteral("\u03b8");      // GREEK LETTER SMALL THETA
        const auto ssa = QStringLiteral("\u0937");             // DEVANAGARI LETTER SSA
        const auto chakmaZero = QStringLiteral("\U00011136");  // CHAKMA DIGIT ZERO

        const auto smallThetaA = QStringLiteral("\u03b8a");
        const auto ssaA = QStringLiteral("\u0937a");
        const auto chakmaZeroA = QStringLiteral("\U00011136a");

        const auto thetaChakma = QStringLiteral("\u03b8\U00011136");
        const auto chakmaTheta = QStringLiteral("\U00011136\u03b8");
        const auto ssaTheta = QStringLiteral("\u0937\u03b8");
        const auto thetaSsa = QStringLiteral("\u03b8\u0937");
        const auto ssaChakma = QStringLiteral("\u0937\U00011136");
        const auto chakmaSsa = QStringLiteral("\U00011136\u0937");
        const auto thetaUmlaut = QStringLiteral("\u03b8\u00ff");
        const auto umlautTheta = QStringLiteral("\u00ff\u03b8");
        const auto ssaUmlaut = QStringLiteral("\u0937\u00ff");
        const auto umlautSsa = QStringLiteral("\u00ff\u0937");
        const auto chakmaUmlaut = QStringLiteral("\U00011136\u00ff");
        const auto umlautChakma = QStringLiteral("\u00ff\U00011136");

        const CharStarContainer smallThetaC("\xce\xb8");           // non-Latin1
        const CharStarContainer ssaC("\xe0\xa4\xb7");              // Higher BMP
        const CharStarContainer chakmaZeroC("\xf0\x91\x84\xb6");   // Non-BMP

        QTest::newRow("null-prepend-smallTheta") << null << smallThetaC << smallTheta;
        QTest::newRow("empty-prepend-smallTheta") << empty << smallThetaC << smallTheta;
        QTest::newRow("a-prepend-smallTheta") << a << smallThetaC << smallThetaA;

        QTest::newRow("null-prepend-ssa") << null << ssaC << ssa;
        QTest::newRow("empty-prepend-ssa") << empty << ssaC << ssa;
        QTest::newRow("a-prepend-ssa") << a << ssaC << ssaA;

        QTest::newRow("null-prepend-chakma") << null << chakmaZeroC << chakmaZero;
        QTest::newRow("empty-prepend-chakma") << empty << chakmaZeroC << chakmaZero;
        QTest::newRow("a-prepend-chakma") << a << chakmaZeroC << chakmaZeroA;

        QTest::newRow("smallTheta-prepend-chakma") << smallTheta << chakmaZeroC << chakmaTheta;
        QTest::newRow("chakma-prepend-smallTheta") << chakmaZero << smallThetaC << thetaChakma;
        QTest::newRow("smallTheta-prepend-ssa") << smallTheta << ssaC << ssaTheta;
        QTest::newRow("ssa-prepend-smallTheta") << ssa << smallThetaC << thetaSsa;
        QTest::newRow("ssa-prepend-chakma") << ssa << chakmaZeroC << chakmaSsa;
        QTest::newRow("chakma-prepend-ssa") << chakmaZero << ssaC << ssaChakma;
        QTest::newRow("smallTheta-prepend-yumlaut") << smallTheta << yumlautC << umlautTheta;
        QTest::newRow("yumlaut-prepend-smallTheta") << yumlaut << smallThetaC << thetaUmlaut;
        QTest::newRow("ssa-prepend-yumlaut") << ssa << yumlautC << umlautSsa;
        QTest::newRow("yumlaut-prepend-ssa") << yumlaut << ssaC << ssaUmlaut;
        QTest::newRow("chakma-prepend-yumlaut") << chakmaZero << yumlautC << umlautChakma;
        QTest::newRow("yumlaut-prepend-chakma") << yumlaut << chakmaZeroC << chakmaUmlaut;
    }
}

void tst_QString::prepend_bytearray_special_cases_data()
{
    QTest::addColumn<QString>("str" );
    QTest::addColumn<QByteArray>("ba" );
    QTest::addColumn<QString>("res" );

    QByteArray ba( 5, 0 );
    ba[0] = 'a';
    ba[1] = 'b';
    ba[2] = 'c';
    ba[3] = 'd';

    // byte array with only a 0
    ba.resize( 1 );
    ba[0] = 0;
    QTest::newRow( "emptyString" ) << QString("foobar ") << ba << QStringView::fromArray(u"\0foobar ").chopped(1).toString();

    // empty byte array
    ba.resize( 0 );
    QTest::newRow( "emptyByteArray" ) << QString(" foobar") << ba << QString(" foobar");

    // non-ascii byte array
    QTest::newRow( "nonAsciiByteArray") << QString() << QByteArray("\xc3\xa9") << QString("\xc3\xa9");
    QTest::newRow( "nonAsciiByteArray2") << QString() << QByteArray("\xc3\xa9") << QString::fromUtf8("\xc3\xa9");
}

void tst_QString::prepend_bytearray_special_cases()
{
    {
        QFETCH( QString, str );
        QFETCH( QByteArray, ba );

        str.prepend( ba );

        QFETCH( QString, res );
        QCOMPARE( str, res );
    }
    {
        QFETCH( QString, str );
        QFETCH( QByteArray, ba );

        str.prepend( ba );

        QTEST( str, "res" );
    }

    QFETCH( QByteArray, ba );
    if (!ba.contains('\0') && ba.constData()[ba.length()] == '\0') {
        QFETCH( QString, str );

        str.prepend(ba.constData());
        QTEST( str, "res" );
    }
}

void tst_QString::replace_uint_uint()
{
    QFETCH( QString, string );
    QFETCH( int, index );
    QFETCH( int, len );
    QFETCH( QString, after );

    QString s1 = string;
    s1.replace( (uint) index, (int) len, after );
    QTEST( s1, "result" );

    QString s2 = string;
    s2.replace( (uint) index, (uint) len, after.unicode(), after.length() );
    QTEST( s2, "result" );

    if ( after.length() == 1 ) {
        QString s3 = string;
        s3.replace( (uint) index, (uint) len, QChar(after[0]) );
        QTEST( s3, "result" );

        QString s4 = string;
        s4.replace( (uint) index, (uint) len, QChar(after[0]).toLatin1() );
        QTEST( s4, "result" );
    }
}

void tst_QString::replace_uint_uint_extra()
{
    {
        QString s;
        s.insert(0, QChar('A'));

        auto bigReplacement = QString("B").repeated(s.capacity() * 3);

        s.replace( 0, 1, bigReplacement );
        QCOMPARE( s, bigReplacement );
    }

    {
        QString s;
        s.insert(0, QLatin1String("BBB"));

        auto smallReplacement = QString("C");

        s.replace( 0, 3, smallReplacement );
        QCOMPARE( s, smallReplacement );
    }

    {
        QString s;
        s.insert(0, QLatin1String("BBB"));

        auto smallReplacement = QString("C");

        s.replace( 5, 3, smallReplacement );
        QCOMPARE( s, QLatin1String("BBB") );
    }
}

void tst_QString::replace_extra()
{
    /*
        This test is designed to be extremely slow if QString::replace() doesn't optimize the case
        len == after.size().
    */
    QString str("dsfkljfdsjklsdjsfjklfsdjkldfjslkjsdfkllkjdsfjklsfdkjsdflkjlsdfjklsdfkjldsflkjsddlkj");
    for (int j = 1; j < 12; ++j)
        str += str;

    QString str2("aaaaaaaaaaaaaaaaaaaa");
    for (int i = 0; i < 2000000; ++i) {
        str.replace(10, 20, str2);
    }

    /*
        Make sure that replacing with itself works.
    */
    QString copy(str);
    copy.detach();
    str.replace(0, str.length(), str);
    QVERIFY(copy == str);

    /*
        Make sure that replacing a part of oneself with itself works.
    */
    QString str3("abcdefghij");
    str3.replace(0, 1, str3);
    QCOMPARE(str3, QString("abcdefghijbcdefghij"));

    QString str4("abcdefghij");
    str4.replace(1, 3, str4);
    QCOMPARE(str4, QString("aabcdefghijefghij"));

    QString str5("abcdefghij");
    str5.replace(8, 10, str5);
    QCOMPARE(str5, QString("abcdefghabcdefghij"));

    // Replacements using only part of the string modified:
    QString str6("abcdefghij");
    str6.replace(1, 8, str6.constData() + 3, 3);
    QCOMPARE(str6, QString("adefj"));

    QString str7("abcdefghibcdefghij");
    str7.replace(str7.constData() + 1, 6, str7.constData() + 2, 3);
    QCOMPARE(str7, QString("acdehicdehij"));

    const int many = 1024;
    /*
      QS::replace(const QChar *, int, const QChar *, int, Qt::CaseSensitivity)
      does its replacements in batches of many (please keep in sync with any
      changes to batch size), which lead to misbehaviour if ether QChar * array
      was part of the data being modified.
    */
    QString str8("abcdefg"), ans8("acdeg");
    {
        // Make str8 and ans8 repeat themselves many + 1 times:
        int i = many;
        QString big(str8), small(ans8);
        while (i && !(i & 1)) { // Exploit many being a power of 2:
            big += big;
            small += small;
            i >>= 1;
        }
        while (i-- > 0) {
            str8 += big;
            ans8 += small;
        }
    }
    str8.replace(str8.constData() + 1, 5, str8.constData() + 2, 3);
    // Pre-test the bit where the diff happens, so it gets displayed:
    QCOMPARE(str8.mid((many - 3) * 5), ans8.mid((many - 3) * 5));
    // Also check the full values match, of course:
    QCOMPARE(str8.size(), ans8.size());
    QCOMPARE(str8, ans8);
}

void tst_QString::replace_string()
{
    QFETCH( QString, string );
    QFETCH( QString, before );
    QFETCH( QString, after );
    QFETCH( bool, bcs );

    Qt::CaseSensitivity cs = bcs ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if ( before.length() == 1 ) {
        QChar ch = before.at( 0 );

        QString s1 = string;
        s1.replace( ch, after, cs );
        QTEST( s1, "result" );

        if ( QChar(ch.toLatin1()) == ch ) {
            QString s2 = string;
            s2.replace( ch.toLatin1(), after, cs );
            QTEST( s2, "result" );
        }
    }

    QString s3 = string;
    s3.replace( before, after, cs );
    QTEST( s3, "result" );
}

void tst_QString::replace_string_extra()
{
    {
        QString s;
        s.insert(0, QChar('A'));

        auto bigReplacement = QString("B").repeated(s.capacity() * 3);

        s.replace( QString("A"), bigReplacement );
        QCOMPARE( s, bigReplacement );
    }

    {
        QString s;
        s.insert(0, QLatin1String("BBB"));

        auto smallReplacement = QString("C");

        s.replace( QString("BBB"), smallReplacement );
        QCOMPARE( s, smallReplacement );
    }

    {
        QString s(QLatin1String("BBB"));
        QString expected(QLatin1String("BBB"));
        for (int i = 0; i < 1028; ++i) {
            s.append("X");
            expected.append("GXU");
        }
        s.replace(QChar('X'), "GXU");
        QCOMPARE(s, expected);
    }
}

#if QT_CONFIG(regularexpression)
void tst_QString::replace_regexp()
{
    QFETCH( QString, string );
    QFETCH( QString, regexp );
    QFETCH( QString, after );

    QRegularExpression regularExpression(regexp);
    if (!regularExpression.isValid())
        QTest::ignoreMessage(QtWarningMsg, "QString::replace: invalid QRegularExpression object");
    string.replace(regularExpression, after);
    QTEST(string, "result");
}

void tst_QString::replace_regexp_extra()
{
    {
        QString s;
        s.insert(0, QChar('A'));

        auto bigReplacement = QString("B").repeated(s.capacity() * 3);

        QRegularExpression regularExpression(QString("A"));
        QVERIFY(regularExpression.isValid());

        s.replace( regularExpression, bigReplacement );
        QCOMPARE( s, bigReplacement );
    }

    {
        QString s;
        s.insert(0, QLatin1String("BBB"));

        auto smallReplacement = QString("C");

        QRegularExpression regularExpression(QString("BBB"));
        QVERIFY(regularExpression.isValid());

        s.replace( regularExpression, smallReplacement );
        QCOMPARE( s, smallReplacement );
    }
}
#endif

void tst_QString::remove_uint_uint()
{
    QFETCH( QString, string );
    QFETCH( int, index );
    QFETCH( int, len );
    QFETCH( QString, after );

    if ( after.length() == 0 ) {
        QString s1 = string;
        s1.remove( (uint) index, (uint) len );
        QTEST( s1, "result" );
    } else
        QCOMPARE( 0, 0 ); // shut Qt Test
}

void tst_QString::remove_string()
{
    QFETCH( QString, string );
    QFETCH( QString, before );
    QFETCH( QString, after );
    QFETCH( bool, bcs );

    Qt::CaseSensitivity cs = bcs ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if ( after.length() == 0 ) {
        if ( before.length() == 1 && cs ) {
            QChar ch = before.at( 0 );

            QString s1 = string;
            s1.remove( ch );
            QTEST( s1, "result" );

            if ( QChar(ch.toLatin1()) == ch ) {
                QString s2 = string;
                s2.remove( ch );
                QTEST( s2, "result" );
            }
        }

        QString s3 = string;
        s3.remove( before, cs );
        QTEST( s3, "result" );

        if (QtPrivate::isLatin1(before)) {
            QString s6 = string;
            s6.remove( QLatin1String(before.toLatin1()), cs );
            QTEST( s6, "result" );
        }
    } else {
        QCOMPARE( 0, 0 ); // shut Qt Test
    }
}

#if QT_CONFIG(regularexpression)
void tst_QString::remove_regexp_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("regexp");
    QTest::addColumn<QString>("after"); // For the benefit of replace_regexp; empty = remove.
    QTest::addColumn<QString>("result");
    // string.remove(regexp) == result

    QTest::newRow("alpha:s/a+//")
        << QString("alpha") << QString("a+") << QString("") << QString("lph");
    QTest::newRow("banana:s/^.a//")
        << QString("banana") << QString("^.a") << QString("") << QString("nana");
    QTest::newRow("<empty>:s/^.a//")
        << QString("") << QString("^.a") << QString("") << QString("");
    // The null-vs-empty distinction in after is only relevant to repplace_regexp(), but
    // include both cases here to keep after's "empty here, non-empty there" rule simple.
    QTest::newRow("<empty>:s/^.a/<null>/")
        << QString("") << QString("^.a") << QString() << QString("");
    QTest::newRow("<null>:s/^.a//") << QString() << QString("^.a") << QString("") << QString();
    QTest::newRow("<null>s/.a/<null>/") << QString() << QString("^.a") << QString() << QString();
    QTest::newRow("invalid")
        << QString("") << QString("invalid regex\\") << QString("") << QString("");
}

void tst_QString::remove_regexp()
{
    QFETCH( QString, string );
    QFETCH( QString, regexp );
    QTEST(QString(), "after"); // non-empty replacement text tests should go in replace_regexp_data()

    QRegularExpression regularExpression(regexp);
    // remove() delegates to replace(), which produces this warning:
    if (!regularExpression.isValid())
        QTest::ignoreMessage(QtWarningMsg, "QString::replace: invalid QRegularExpression object");
    string.remove(regularExpression);
    QTEST(string, "result");
}
#endif

void tst_QString::remove_extra()
{
    {
        QString s = "The quick brown fox jumps over the lazy dog. "
                    "The lazy dog jumps over the quick brown fox.";
        s.remove(s);
    }

    {
        QString s = "BCDEFGHJK";
        QString s1 = s;
        s1.insert(0, u'A');  // detaches
        s1.remove(0, 1);
        QCOMPARE(s1, s);
    }
}

void tst_QString::toNum()
{
#if defined (Q_OS_WIN) && defined (Q_CC_MSVC)
#define TEST_TO_INT(num, func) \
    a = #num; \
    QVERIFY2(a.func(&ok) == num ## i64 && ok, "Failed: num=" #num ", func=" #func);
#else
#define TEST_TO_INT(num, func) \
    a = #num; \
    QVERIFY2(a.func(&ok) == num ## LL && ok, "Failed: num=" #num ", func=" #func);
#endif

    QString a;
    bool ok = false;

    TEST_TO_INT(0, toInt)
    TEST_TO_INT(-1, toInt)
    TEST_TO_INT(1, toInt)
    TEST_TO_INT(2147483647, toInt)
    TEST_TO_INT(-2147483648, toInt)

    TEST_TO_INT(0, toShort)
    TEST_TO_INT(-1, toShort)
    TEST_TO_INT(1, toShort)
    TEST_TO_INT(32767, toShort)
    TEST_TO_INT(-32768, toShort)

    TEST_TO_INT(0, toLong)
    TEST_TO_INT(-1, toLong)
    TEST_TO_INT(1, toLong)
    TEST_TO_INT(2147483647, toLong)
    TEST_TO_INT(-2147483648, toLong)
    TEST_TO_INT(0, toLongLong)
    TEST_TO_INT(-1, toLongLong)
    TEST_TO_INT(1, toLongLong)
    TEST_TO_INT(9223372036854775807, toLongLong)
    TEST_TO_INT(-9223372036854775807, toLongLong)

#undef TEST_TO_INT

#if defined (Q_OS_WIN) && defined (Q_CC_MSVC)
#define TEST_TO_UINT(num, func) \
    a = #num; \
    QVERIFY2(a.func(&ok) == num ## i64 && ok, "Failed: num=" #num ", func=" #func);
#else
#define TEST_TO_UINT(num, func) \
    a = #num; \
    QVERIFY2(a.func(&ok) == num ## ULL && ok, "Failed: num=" #num ", func=" #func);
#endif

    TEST_TO_UINT(0, toUInt)
    TEST_TO_UINT(1, toUInt)
    TEST_TO_UINT(4294967295, toUInt)

    TEST_TO_UINT(0, toUShort)
    TEST_TO_UINT(1, toUShort)
    TEST_TO_UINT(65535, toUShort)

    TEST_TO_UINT(0, toULong)
    TEST_TO_UINT(1, toULong)
    TEST_TO_UINT(4294967295, toULong)

    TEST_TO_UINT(0, toULongLong)
    TEST_TO_UINT(1, toULongLong)
    TEST_TO_UINT(18446744073709551615, toULongLong)
#undef TEST_TO_UINT


#define TEST_BASE(str, base, num) \
    a = str; \
    QVERIFY2(a.toInt(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toInt"); \
    QVERIFY2(a.toUInt(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toUInt"); \
    QVERIFY2(a.toShort(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toShort"); \
    QVERIFY2(a.toUShort(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toUShort"); \
    QVERIFY2(a.toLong(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toLong"); \
    QVERIFY2(a.toULong(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toULong"); \
    QVERIFY2(a.toLongLong(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toLongLong"); \
    QVERIFY2(a.toULongLong(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toULongLong");

    TEST_BASE("FF", 16, 255)
    TEST_BASE("0xFF", 16, 255)
    TEST_BASE("77", 8, 63)
    TEST_BASE("077", 8, 63)

    TEST_BASE("0xFF", 0, 255)
    TEST_BASE("077", 0, 63)
    TEST_BASE("255", 0, 255)

    TEST_BASE(" FF", 16, 255)
    TEST_BASE(" 0xFF", 16, 255)
    TEST_BASE(" 77", 8, 63)
    TEST_BASE(" 077", 8, 63)

    TEST_BASE(" 0xFF", 0, 255)
    TEST_BASE(" 077", 0, 63)
    TEST_BASE(" 255", 0, 255)

    TEST_BASE("\tFF\t", 16, 255)
    TEST_BASE("\t0xFF  ", 16, 255)
    TEST_BASE("   77   ", 8, 63)
    TEST_BASE("77  ", 8, 63)

#undef TEST_BASE

#define TEST_NEG_BASE(str, base, num) \
    a = str; \
    QVERIFY2(a.toInt(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toInt"); \
    QVERIFY2(a.toShort(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toShort"); \
    QVERIFY2(a.toLong(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toLong"); \
    QVERIFY2(a.toLongLong(&ok, base) == num && ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toLongLong");

    TEST_NEG_BASE("-FE", 16, -254)
    TEST_NEG_BASE("-0xFE", 16, -254)
    TEST_NEG_BASE("-77", 8, -63)
    TEST_NEG_BASE("-077", 8, -63)

    TEST_NEG_BASE("-0xFE", 0, -254)
    TEST_NEG_BASE("-077", 0, -63)
    TEST_NEG_BASE("-254", 0, -254)

#undef TEST_NEG_BASE

#define TEST_DOUBLE(num, str) \
    a = str; \
    QCOMPARE(a.toDouble(&ok), num); \
    QVERIFY(ok);

    TEST_DOUBLE(1.2345, "1.2345")
    TEST_DOUBLE(12.345, "1.2345e+01")
    TEST_DOUBLE(12.345, "1.2345E+01")
    TEST_DOUBLE(12345.6, "12345.6")

#undef TEST_DOUBLE


#define TEST_BAD(str, func) \
    a = str; \
    a.func(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str " func=" #func);

    TEST_BAD("32768", toShort)
    TEST_BAD("-32769", toShort)
    TEST_BAD("65536", toUShort)
    TEST_BAD("2147483648", toInt)
    TEST_BAD("-2147483649", toInt)
    TEST_BAD("4294967296", toUInt)
    if (sizeof(long) == 4) {
        TEST_BAD("2147483648", toLong)
        TEST_BAD("-2147483649", toLong)
        TEST_BAD("4294967296", toULong)
    }
    TEST_BAD("9223372036854775808", toLongLong)
    TEST_BAD("-9223372036854775809", toLongLong)
    TEST_BAD("18446744073709551616", toULongLong)
    TEST_BAD("-1", toUShort)
    TEST_BAD("-1", toUInt)
    TEST_BAD("-1", toULong)
    TEST_BAD("-1", toULongLong)
#undef TEST_BAD

#define TEST_BAD_ALL(str) \
    a = str; \
    a.toShort(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    a.toUShort(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    a.toInt(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    a.toUInt(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    a.toLong(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    a.toULong(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    a.toLongLong(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    a.toULongLong(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    a.toFloat(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    a.toDouble(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str);

    TEST_BAD_ALL((const char*)0);
    TEST_BAD_ALL("");
    TEST_BAD_ALL(" ");
    TEST_BAD_ALL(".");
    TEST_BAD_ALL("-");
    TEST_BAD_ALL("hello");
    TEST_BAD_ALL("1.2.3");
    TEST_BAD_ALL("0x0x0x");
    TEST_BAD_ALL("123-^~<");
    TEST_BAD_ALL("123ThisIsNotANumber");

#undef TEST_BAD_ALL

    a = "FF";
    a.toULongLong(&ok, 10);
    QVERIFY(!ok);

    a = "FF";
    a.toULongLong(&ok, 0);
    QVERIFY(!ok);

#ifdef QT_NO_FPU
    double d = 3.40282346638528e+38; // slightly off FLT_MAX when using hardfloats
#else
    double d = 3.4028234663852886e+38; // FLT_MAX
#endif
    QString::number(d, 'e', 17).toFloat(&ok);
    QVERIFY(ok);
    QString::number(d + 1e32, 'e', 17).toFloat(&ok);
    QVERIFY(!ok);
    QString::number(-d, 'e', 17).toFloat(&ok);
    QVERIFY(ok);
    QString::number(-d - 1e32, 'e', 17).toFloat(&ok);
    QVERIFY(!ok);
    QString::number(d + 1e32, 'e', 17).toDouble(&ok);
    QVERIFY(ok);
    QString::number(-d - 1e32, 'e', 17).toDouble(&ok);
    QVERIFY(ok);
}

void tst_QString::toUShort()
{
    QString a;
    bool ok;
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a="";
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a="COMPARE";
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a="123";
    QCOMPARE(a.toUShort(),(ushort)123);
    QCOMPARE(a.toUShort(&ok),(ushort)123);
    QVERIFY(ok);

    a="123A";
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a="1234567";
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = "aaa123aaa";
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = "aaa123";
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = "123aaa";
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = "32767";
    QCOMPARE(a.toUShort(),(ushort)32767);
    QCOMPARE(a.toUShort(&ok),(ushort)32767);
    QVERIFY(ok);

    a = "-32767";
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = "65535";
    QCOMPARE(a.toUShort(),(ushort)65535);
    QCOMPARE(a.toUShort(&ok),(ushort)65535);
    QVERIFY(ok);

    if (sizeof(short) == 2) {
        a = "65536";
        QCOMPARE(a.toUShort(),(ushort)0);
        QCOMPARE(a.toUShort(&ok),(ushort)0);
        QVERIFY(!ok);

        a = "123456";
        QCOMPARE(a.toUShort(),(ushort)0);
        QCOMPARE(a.toUShort(&ok),(ushort)0);
        QVERIFY(!ok);
    }
}

void tst_QString::toShort()
{
    QString a;
    bool ok;
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a="";
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a="COMPARE";
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a="123";
    QCOMPARE(a.toShort(),(short)123);
    QCOMPARE(a.toShort(&ok),(short)123);
    QVERIFY(ok);

    a="123A";
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a="1234567";
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = "aaa123aaa";
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = "aaa123";
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = "123aaa";
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = "32767";
    QCOMPARE(a.toShort(),(short)32767);
    QCOMPARE(a.toShort(&ok),(short)32767);
    QVERIFY(ok);

    a = "-32767";
    QCOMPARE(a.toShort(),(short)-32767);
    QCOMPARE(a.toShort(&ok),(short)-32767);
    QVERIFY(ok);

    a = "-32768";
    QCOMPARE(a.toShort(),(short)-32768);
    QCOMPARE(a.toShort(&ok),(short)-32768);
    QVERIFY(ok);

    if (sizeof(short) == 2) {
        a = "32768";
        QCOMPARE(a.toShort(),(short)0);
        QCOMPARE(a.toShort(&ok),(short)0);
        QVERIFY(!ok);

        a = "-32769";
        QCOMPARE(a.toShort(),(short)0);
        QCOMPARE(a.toShort(&ok),(short)0);
        QVERIFY(!ok);
    }
}

void tst_QString::toInt()
{
    QString a;
    bool ok;
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = "";
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a="COMPARE";
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a="123";
    QCOMPARE(a.toInt(),123);
    QCOMPARE(a.toInt(&ok),123);
    QVERIFY(ok);

    a="123A";
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a="1234567";
    QCOMPARE(a.toInt(),1234567);
    QCOMPARE(a.toInt(&ok),1234567);
    QVERIFY(ok);

    a="12345678901234";
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a="3234567890";
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = "aaa12345aaa";
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = "aaa12345";
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = "12345aaa";
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = "2147483647"; // 2**31 - 1
    QCOMPARE(a.toInt(),2147483647);
    QCOMPARE(a.toInt(&ok),2147483647);
    QVERIFY(ok);

    if (sizeof(int) == 4) {
        a = "-2147483647"; // -(2**31 - 1)
        QCOMPARE(a.toInt(),-2147483647);
        QCOMPARE(a.toInt(&ok),-2147483647);
        QVERIFY(ok);

        a = "2147483648"; // 2**31
        QCOMPARE(a.toInt(),0);
        QCOMPARE(a.toInt(&ok),0);
        QVERIFY(!ok);

        a = "-2147483648"; // -2**31
        QCOMPARE(a.toInt(),-2147483647 - 1);
        QCOMPARE(a.toInt(&ok),-2147483647 - 1);
        QVERIFY(ok);

        a = "2147483649"; // 2**31 + 1
        QCOMPARE(a.toInt(),0);
        QCOMPARE(a.toInt(&ok),0);
        QVERIFY(!ok);
    }
}

void tst_QString::toUInt()
{
    bool ok;
    QString a;

    QCOMPARE(a.toUInt(), 0u);
    QCOMPARE(a.toUInt(&ok), 0u);
    QVERIFY(!ok);

    a="3234567890";
    QCOMPARE(a.toUInt(&ok),3234567890u);
    QVERIFY(ok);

    a = "-50";
    QCOMPARE(a.toUInt(),0u);
    QCOMPARE(a.toUInt(&ok),0u);
    QVERIFY(!ok);

    a = "4294967295"; // 2**32 - 1
    QCOMPARE(a.toUInt(),4294967295u);
    QCOMPARE(a.toUInt(&ok),4294967295u);
    QVERIFY(ok);

    if (sizeof(int) == 4) {
        a = "4294967296"; // 2**32
        QCOMPARE(a.toUInt(),0u);
        QCOMPARE(a.toUInt(&ok),0u);
        QVERIFY(!ok);
    }
}

///////////////////////////// to*Long //////////////////////////////////////

void tst_QString::toULong_data()
{
    QTest::addColumn<QString>("str" );
    QTest::addColumn<int>("base" );
    QTest::addColumn<ulong>("result" );
    QTest::addColumn<bool>("ok" );

    QTest::newRow( "default" ) << QString() << 10 << 0UL << false;
    QTest::newRow( "empty" ) << QString("") << 10 << 0UL << false;
    QTest::newRow( "ulong1" ) << QString("3234567890") << 10 << 3234567890UL << true;
    QTest::newRow( "ulong2" ) << QString("fFFfFfFf") << 16 << 0xFFFFFFFFUL << true;
}

void tst_QString::toULong()
{
    QFETCH( QString, str );
    QFETCH( int, base );
    QFETCH( ulong, result );
    QFETCH( bool, ok );

    bool b;
    QCOMPARE( str.toULong( 0, base ), result );
    QCOMPARE( str.toULong( &b, base ), result );
    QCOMPARE( b, ok );
}

void tst_QString::toLong_data()
{
    QTest::addColumn<QString>("str" );
    QTest::addColumn<int>("base" );
    QTest::addColumn<long>("result" );
    QTest::addColumn<bool>("ok" );

    QTest::newRow( "default" ) << QString() << 10 << 0L << false;
    QTest::newRow( "empty" ) << QString("") << 10 << 0L << false;
    QTest::newRow( "normal" ) << QString("7fFFfFFf") << 16 << 0x7fFFfFFfL << true;
    QTest::newRow( "long_max" ) << QString("2147483647") << 10 << 2147483647L << true;
    if (sizeof(long) == 4) {
        QTest::newRow( "long_max+1" ) << QString("2147483648") << 10 << 0L << false;
        QTest::newRow( "long_min-1" ) << QString("-80000001") << 16 << 0L << false;
    }
    QTest::newRow( "negative" ) << QString("-7fffffff") << 16 << -0x7fffffffL << true;
//    QTest::newRow( "long_min" ) << QString("-80000000") << 16 << 0x80000000uL << true;
}

void tst_QString::toLong()
{
    QFETCH( QString, str );
    QFETCH( int, base );
    QFETCH( long, result );
    QFETCH( bool, ok );

    bool b;
    QCOMPARE( str.toLong( 0, base ), result );
    QCOMPARE( str.toLong( &b, base ), result );
    QCOMPARE( b, ok );
}


////////////////////////// to*LongLong //////////////////////////////////////

void tst_QString::toULongLong()
{
    QString str;
    bool ok;

    QCOMPARE(str.toULongLong(), Q_UINT64_C(0));
    QCOMPARE(str.toULongLong(&ok), Q_UINT64_C(0));
    QVERIFY(!ok);

    str = "18446744073709551615"; // ULLONG_MAX
    QCOMPARE( str.toULongLong( 0 ), Q_UINT64_C(18446744073709551615) );
    QCOMPARE( str.toULongLong( &ok ), Q_UINT64_C(18446744073709551615) );
    QVERIFY( ok );

    str = "18446744073709551616"; // ULLONG_MAX + 1
    QCOMPARE( str.toULongLong( 0 ), Q_UINT64_C(0) );
    QCOMPARE( str.toULongLong( &ok ), Q_UINT64_C(0) );
    QVERIFY( !ok );

    str = "-150";
    QCOMPARE( str.toULongLong( 0 ), Q_UINT64_C(0) );
    QCOMPARE( str.toULongLong( &ok ), Q_UINT64_C(0) );
    QVERIFY( !ok );
}

void tst_QString::toLongLong()
{
    QString str;
    bool ok;

    QCOMPARE(str.toLongLong(0), Q_INT64_C(0));
    QCOMPARE(str.toLongLong(&ok), Q_INT64_C(0));
    QVERIFY(!ok);

    str = "9223372036854775807"; // LLONG_MAX
    QCOMPARE( str.toLongLong( 0 ), Q_INT64_C(9223372036854775807) );
    QCOMPARE( str.toLongLong( &ok ), Q_INT64_C(9223372036854775807) );
    QVERIFY( ok );

    str = "-9223372036854775808"; // LLONG_MIN
    QCOMPARE( str.toLongLong( 0 ),
             -Q_INT64_C(9223372036854775807) - Q_INT64_C(1) );
    QCOMPARE( str.toLongLong( &ok ),
             -Q_INT64_C(9223372036854775807) - Q_INT64_C(1) );
    QVERIFY( ok );

    str = "aaaa9223372036854775807aaaa";
    QCOMPARE( str.toLongLong( 0 ), Q_INT64_C(0) );
    QCOMPARE( str.toLongLong( &ok ), Q_INT64_C(0) );
    QVERIFY( !ok );

    str = "9223372036854775807aaaa";
    QCOMPARE( str.toLongLong( 0 ), Q_INT64_C(0) );
    QCOMPARE( str.toLongLong( &ok ), Q_INT64_C(0) );
    QVERIFY( !ok );

    str = "aaaa9223372036854775807";
    QCOMPARE( str.toLongLong( 0 ), Q_INT64_C(0) );
    QCOMPARE( str.toLongLong( &ok ), Q_INT64_C(0) );
    QVERIFY( !ok );

    static char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (int i = 0; i < 36; ++i) {
        for (int j = 0; j < 36; ++j) {
            for (int k = 0; k < 36; ++k) {
                QString str;
                str += QChar(digits[i]);
                str += QChar(digits[j]);
                str += QChar(digits[k]);
                qlonglong value = (((i * 36) + j) * 36) + k;
                QVERIFY(str.toLongLong(0, 36) == value);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////

void tst_QString::toFloat()
{
    QString a;
    bool ok;

    QCOMPARE(a.toFloat(), 0.0f);
    QCOMPARE(a.toFloat(&ok), 0.0f);
    QVERIFY(!ok);

    a="0.000000000931322574615478515625";
    QCOMPARE(a.toFloat(&ok),(float)(0.000000000931322574615478515625));
    QVERIFY(ok);
}

void tst_QString::toDouble_data()
{
    QTest::addColumn<QString>("str" );
    QTest::addColumn<double>("result" );
    QTest::addColumn<bool>("result_ok" );

    QTest::newRow("null") << QString() << 0.0 << false;
    QTest::newRow("empty") << QString("") << 0.0 << false;

    QTest::newRow( "ok00" ) << QString("0.000000000931322574615478515625") << 0.000000000931322574615478515625 << true;
    QTest::newRow( "ok01" ) << QString(" 123.45") << 123.45 << true;

    QTest::newRow( "ok02" ) << QString("0.1e10") << 0.1e10 << true;
    QTest::newRow( "ok03" ) << QString("0.1e-10") << 0.1e-10 << true;

    QTest::newRow( "ok04" ) << QString("1e10") << 1.0e10 << true;
    QTest::newRow( "ok05" ) << QString("1e+10") << 1.0e10 << true;
    QTest::newRow( "ok06" ) << QString("1e-10") << 1.0e-10 << true;

    QTest::newRow( "ok07" ) << QString(" 1e10") << 1.0e10 << true;
    QTest::newRow( "ok08" ) << QString("  1e+10") << 1.0e10 << true;
    QTest::newRow( "ok09" ) << QString("   1e-10") << 1.0e-10 << true;

    QTest::newRow( "ok10" ) << QString("1.") << 1.0 << true;
    QTest::newRow( "ok11" ) << QString(".1") << 0.1 << true;

    QTest::newRow( "wrong00" ) << QString("123.45 ") << 123.45 << true;
    QTest::newRow( "wrong01" ) << QString(" 123.45 ") << 123.45 << true;

    QTest::newRow( "wrong02" ) << QString("aa123.45aa") << 0.0 << false;
    QTest::newRow( "wrong03" ) << QString("123.45aa") << 0.0 << false;
    QTest::newRow( "wrong04" ) << QString("123erf") << 0.0 << false;

    QTest::newRow( "wrong05" ) << QString("abc") << 0.0 << false;
    QTest::newRow( "wrong06" ) << QString() << 0.0 << false;
    QTest::newRow( "wrong07" ) << QString("") << 0.0 << false;
}

void tst_QString::toDouble()
{
    QFETCH( QString, str );
    QFETCH( bool, result_ok );
    bool ok;
    double d = str.toDouble( &ok );
    if ( result_ok ) {
        QTEST( d, "result" );
        QVERIFY( ok );
    } else {
        QVERIFY( !ok );
    }
}

void tst_QString::setNum()
{
    QString a;
    QCOMPARE(a.setNum(123), QLatin1String("123"));
    QCOMPARE(a.setNum(-123), QLatin1String("-123"));
    QCOMPARE(a.setNum(0x123,16), QLatin1String("123"));
    QCOMPARE(a.setNum((short)123), QLatin1String("123"));
    QCOMPARE(a.setNum(123L), QLatin1String("123"));
    QCOMPARE(a.setNum(123UL), QLatin1String("123"));
    QCOMPARE(a.setNum(2147483647L), QString("2147483647")); // 32 bit LONG_MAX
    QCOMPARE(a.setNum(-2147483647L), QString("-2147483647")); // LONG_MIN + 1
    QCOMPARE(a.setNum(-2147483647L-1L), QString("-2147483648")); // LONG_MIN
    QCOMPARE(a.setNum(1.23), QString("1.23"));
    QCOMPARE(a.setNum(1.234567), QString("1.23457"));
#if defined(LONG_MAX) && defined(LLONG_MAX) && LONG_MAX == LLONG_MAX
    // LONG_MAX and LONG_MIN on 64 bit systems
    QCOMPARE(a.setNum(9223372036854775807L), QString("9223372036854775807"));
    QCOMPARE(a.setNum(-9223372036854775807L-1L), QString("-9223372036854775808"));
    QCOMPARE(a.setNum(18446744073709551615UL), QString("18446744073709551615"));
#endif
    QCOMPARE(a.setNum(Q_INT64_C(123)), QString("123"));
    // 2^40 == 1099511627776
    QCOMPARE(a.setNum(Q_INT64_C(-1099511627776)), QString("-1099511627776"));
    QCOMPARE(a.setNum(Q_UINT64_C(1099511627776)), QString("1099511627776"));
    QCOMPARE(a.setNum(Q_INT64_C(9223372036854775807)), // LLONG_MAX
            QString("9223372036854775807"));
    QCOMPARE(a.setNum(-Q_INT64_C(9223372036854775807) - Q_INT64_C(1)),
            QString("-9223372036854775808"));
    QCOMPARE(a.setNum(Q_UINT64_C(18446744073709551615)), // ULLONG_MAX
            QString("18446744073709551615"));
    QCOMPARE(a.setNum(0.000000000931322574615478515625),QString("9.31323e-10"));

//  QCOMPARE(a.setNum(0.000000000931322574615478515625,'g',30),(QString)"9.31322574615478515625e-010");
//  QCOMPARE(a.setNum(0.000000000931322574615478515625,'f',30),(QString)"0.00000000093132257461547852");
}

void tst_QString::startsWith()
{
    QString a;

    QVERIFY(!a.startsWith('A'));
    QVERIFY(!a.startsWith("AB"));
    {
        CREATE_VIEW("AB");
        QVERIFY(!a.startsWith(view));
    }
    QVERIFY(!a.isDetached());

    a = "AB";
    QVERIFY( a.startsWith("A") );
    QVERIFY( a.startsWith("AB") );
    QVERIFY( !a.startsWith("C") );
    QVERIFY( !a.startsWith("ABCDEF") );
    QVERIFY( a.startsWith("") );
    QVERIFY( a.startsWith(QString()) );
    QVERIFY( a.startsWith('A') );
    QVERIFY( a.startsWith(QLatin1Char('A')) );
    QVERIFY( a.startsWith(QChar('A')) );
    QVERIFY( !a.startsWith('C') );
    QVERIFY( !a.startsWith(QChar()) );
    QVERIFY( !a.startsWith(QLatin1Char(0)) );

    QVERIFY( a.startsWith(QLatin1String("A")) );
    QVERIFY( a.startsWith(QLatin1String("AB")) );
    QVERIFY( !a.startsWith(QLatin1String("C")) );
    QVERIFY( !a.startsWith(QLatin1String("ABCDEF")) );
    QVERIFY( a.startsWith(QLatin1String("")) );
    QVERIFY( a.startsWith(QLatin1String(0)) );

    QVERIFY( a.startsWith("A", Qt::CaseSensitive) );
    QVERIFY( a.startsWith("A", Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith("a", Qt::CaseSensitive) );
    QVERIFY( a.startsWith("a", Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith("aB", Qt::CaseSensitive) );
    QVERIFY( a.startsWith("aB", Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith("C", Qt::CaseSensitive) );
    QVERIFY( !a.startsWith("C", Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith("c", Qt::CaseSensitive) );
    QVERIFY( !a.startsWith("c", Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith("abcdef", Qt::CaseInsensitive) );
    QVERIFY( a.startsWith("", Qt::CaseInsensitive) );
    QVERIFY( a.startsWith(QString(), Qt::CaseInsensitive) );
    QVERIFY( a.startsWith('a', Qt::CaseInsensitive) );
    QVERIFY( a.startsWith('A', Qt::CaseInsensitive) );
    QVERIFY( a.startsWith(QLatin1Char('a'), Qt::CaseInsensitive) );
    QVERIFY( a.startsWith(QChar('a'), Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith('c', Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith(QChar(), Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith(QLatin1Char(0), Qt::CaseInsensitive) );

    QVERIFY( a.startsWith(QLatin1String("A"), Qt::CaseSensitive) );
    QVERIFY( a.startsWith(QLatin1String("A"), Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith(QLatin1String("a"), Qt::CaseSensitive) );
    QVERIFY( a.startsWith(QLatin1String("a"), Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith(QLatin1String("aB"), Qt::CaseSensitive) );
    QVERIFY( a.startsWith(QLatin1String("aB"), Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith(QLatin1String("C"), Qt::CaseSensitive) );
    QVERIFY( !a.startsWith(QLatin1String("C"), Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith(QLatin1String("c"), Qt::CaseSensitive) );
    QVERIFY( !a.startsWith(QLatin1String("c"), Qt::CaseInsensitive) );
    QVERIFY( !a.startsWith(QLatin1String("abcdef"), Qt::CaseInsensitive) );
    QVERIFY( a.startsWith(QLatin1String(""), Qt::CaseInsensitive) );
    QVERIFY( a.startsWith(QLatin1String(0), Qt::CaseInsensitive) );
    QVERIFY( a.startsWith('A', Qt::CaseSensitive) );
    QVERIFY( a.startsWith(QLatin1Char('A'), Qt::CaseSensitive) );
    QVERIFY( a.startsWith(QChar('A'), Qt::CaseSensitive) );
    QVERIFY( !a.startsWith('a', Qt::CaseSensitive) );
    QVERIFY( !a.startsWith(QChar(), Qt::CaseSensitive) );
    QVERIFY( !a.startsWith(QLatin1Char(0), Qt::CaseSensitive) );

#define TEST_VIEW_STARTS_WITH(string, yes) { CREATE_VIEW(string); QCOMPARE(a.startsWith(view), yes); }
    TEST_VIEW_STARTS_WITH("A", true);
    TEST_VIEW_STARTS_WITH("AB", true);
    TEST_VIEW_STARTS_WITH("C", false);
    TEST_VIEW_STARTS_WITH("ABCDEF", false);
#undef TEST_VIEW_STARTS_WITH

    a = "";
    QVERIFY( a.startsWith("") );
    QVERIFY( a.startsWith(QString()) );
    QVERIFY( !a.startsWith("ABC") );

    QVERIFY( a.startsWith(QLatin1String("")) );
    QVERIFY( a.startsWith(QLatin1String(0)) );
    QVERIFY( !a.startsWith(QLatin1String("ABC")) );

    QVERIFY( !a.startsWith(QLatin1Char(0)) );
    QVERIFY( !a.startsWith(QLatin1Char('x')) );
    QVERIFY( !a.startsWith(QChar()) );

    a = QString();
    QVERIFY( !a.startsWith("") );
    QVERIFY( a.startsWith(QString()) );
    QVERIFY( !a.startsWith("ABC") );

    QVERIFY( !a.startsWith(QLatin1String("")) );
    QVERIFY( a.startsWith(QLatin1String(0)) );
    QVERIFY( !a.startsWith(QLatin1String("ABC")) );

    QVERIFY( !a.startsWith(QLatin1Char(0)) );
    QVERIFY( !a.startsWith(QLatin1Char('x')) );
    QVERIFY( !a.startsWith(QChar()) );

    // this test is independent of encoding
    a = "\xc3\xa9";
    QVERIFY( a.startsWith("\xc3\xa9") );
    QVERIFY( !a.startsWith("\xc3\xa1") );

    // this one is dependent of encoding
    QVERIFY( a.startsWith("\xc3\x89", Qt::CaseInsensitive) );
}

void tst_QString::endsWith()
{
    QString a;

    QVERIFY(!a.endsWith('A'));
    QVERIFY(!a.endsWith("AB"));
    {
        CREATE_VIEW("AB");
        QVERIFY(!a.endsWith(view));
    }
    QVERIFY(!a.isDetached());

    a = "AB";
    QVERIFY( a.endsWith("B") );
    QVERIFY( a.endsWith("AB") );
    QVERIFY( !a.endsWith("C") );
    QVERIFY( !a.endsWith("ABCDEF") );
    QVERIFY( a.endsWith("") );
    QVERIFY( a.endsWith(QString()) );
    QVERIFY( a.endsWith('B') );
    QVERIFY( a.endsWith(QLatin1Char('B')) );
    QVERIFY( a.endsWith(QChar('B')) );
    QVERIFY( !a.endsWith('C') );
    QVERIFY( !a.endsWith(QChar()) );
    QVERIFY( !a.endsWith(QLatin1Char(0)) );

    QVERIFY( a.endsWith(QLatin1String("B")) );
    QVERIFY( a.endsWith(QLatin1String("AB")) );
    QVERIFY( !a.endsWith(QLatin1String("C")) );
    QVERIFY( !a.endsWith(QLatin1String("ABCDEF")) );
    QVERIFY( a.endsWith(QLatin1String("")) );
    QVERIFY( a.endsWith(QLatin1String(0)) );

    QVERIFY( a.endsWith("B", Qt::CaseSensitive) );
    QVERIFY( a.endsWith("B", Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith("b", Qt::CaseSensitive) );
    QVERIFY( a.endsWith("b", Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith("aB", Qt::CaseSensitive) );
    QVERIFY( a.endsWith("aB", Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith("C", Qt::CaseSensitive) );
    QVERIFY( !a.endsWith("C", Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith("c", Qt::CaseSensitive) );
    QVERIFY( !a.endsWith("c", Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith("abcdef", Qt::CaseInsensitive) );
    QVERIFY( a.endsWith("", Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(QString(), Qt::CaseInsensitive) );
    QVERIFY( a.endsWith('b', Qt::CaseInsensitive) );
    QVERIFY( a.endsWith('B', Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(QLatin1Char('b'), Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(QChar('b'), Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith('c', Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(QChar(), Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(QLatin1Char(0), Qt::CaseInsensitive) );

    QVERIFY( a.endsWith(QLatin1String("B"), Qt::CaseSensitive) );
    QVERIFY( a.endsWith(QLatin1String("B"), Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(QLatin1String("b"), Qt::CaseSensitive) );
    QVERIFY( a.endsWith(QLatin1String("b"), Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(QLatin1String("aB"), Qt::CaseSensitive) );
    QVERIFY( a.endsWith(QLatin1String("aB"), Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(QLatin1String("C"), Qt::CaseSensitive) );
    QVERIFY( !a.endsWith(QLatin1String("C"), Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(QLatin1String("c"), Qt::CaseSensitive) );
    QVERIFY( !a.endsWith(QLatin1String("c"), Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(QLatin1String("abcdef"), Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(QLatin1String(""), Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(QLatin1String(0), Qt::CaseInsensitive) );
    QVERIFY( a.endsWith('B', Qt::CaseSensitive) );
    QVERIFY( a.endsWith(QLatin1Char('B'), Qt::CaseSensitive) );
    QVERIFY( a.endsWith(QChar('B'), Qt::CaseSensitive) );
    QVERIFY( !a.endsWith('b', Qt::CaseSensitive) );
    QVERIFY( !a.endsWith(QChar(), Qt::CaseSensitive) );
    QVERIFY( !a.endsWith(QLatin1Char(0), Qt::CaseSensitive) );

#define TEST_VIEW_ENDS_WITH(string, yes) { CREATE_VIEW(string); QCOMPARE(a.endsWith(view), yes); }
    TEST_VIEW_ENDS_WITH(QLatin1String("B"), true);
    TEST_VIEW_ENDS_WITH(QLatin1String("AB"), true);
    TEST_VIEW_ENDS_WITH(QLatin1String("C"), false);
    TEST_VIEW_ENDS_WITH(QLatin1String("ABCDEF"), false);
    TEST_VIEW_ENDS_WITH(QLatin1String(""), true);
    TEST_VIEW_ENDS_WITH(QLatin1String(0), true);
#undef TEST_VIEW_ENDS_WITH

    a = "";
    QVERIFY( a.endsWith("") );
    QVERIFY( a.endsWith(QString()) );
    QVERIFY( !a.endsWith("ABC") );
    QVERIFY( !a.endsWith(QLatin1Char(0)) );
    QVERIFY( !a.endsWith(QLatin1Char('x')) );
    QVERIFY( !a.endsWith(QChar()) );

    QVERIFY( a.endsWith(QLatin1String("")) );
    QVERIFY( a.endsWith(QLatin1String(0)) );
    QVERIFY( !a.endsWith(QLatin1String("ABC")) );

    a = QString();
    QVERIFY( !a.endsWith("") );
    QVERIFY( a.endsWith(QString()) );
    QVERIFY( !a.endsWith("ABC") );

    QVERIFY( !a.endsWith(QLatin1String("")) );
    QVERIFY( a.endsWith(QLatin1String(0)) );
    QVERIFY( !a.endsWith(QLatin1String("ABC")) );

    QVERIFY( !a.endsWith(QLatin1Char(0)) );
    QVERIFY( !a.endsWith(QLatin1Char('x')) );
    QVERIFY( !a.endsWith(QChar()) );

    // this test is independent of encoding
    a = "\xc3\xa9";
    QVERIFY( a.endsWith("\xc3\xa9") );
    QVERIFY( !a.endsWith("\xc3\xa1") );

    // this one is dependent of encoding
    QVERIFY( a.endsWith("\xc3\x89", Qt::CaseInsensitive) );
}

void tst_QString::check_QDataStream()
{
    QString a;
    QByteArray ar;
    {
        QDataStream out(&ar,QIODevice::WriteOnly);
        out << QString("COMPARE Text");
    }
    {
        QDataStream in(&ar,QIODevice::ReadOnly);
        in >> a;
        QCOMPARE(a, QLatin1String("COMPARE Text"));
    }
}

void tst_QString::check_QTextStream()
{
    QString a;
    QByteArray ar;
    {
        QTextStream out(&ar,QIODevice::WriteOnly);
        out << QString("This is COMPARE Text");
    }
    {
        QTextStream in(&ar,QIODevice::ReadOnly);
        in >> a;
        QCOMPARE(a, QLatin1String("This"));
    }
}

void tst_QString::check_QTextIOStream()
{
    QString a;
    {
        a="";
        QTextStream ts(&a);
        // invalid Utf8
        ts << "pi \261= " << 3.125;
        QCOMPARE(a, QString::fromUtf16(u"pi \xfffd= 3.125"));
    }
    {
        a="";
        QTextStream ts(&a);
        // valid Utf8
        ts << "pi ø= " << 3.125;
        QCOMPARE(a, QString::fromUtf16(u"pi ø= 3.125"));
    }
    {
        a="123 456";
        int x,y;
        QTextStream(&a) >> x >> y;
        QCOMPARE(x,123);
        QCOMPARE(y,456);
    }
}

void tst_QString::fromRawData()
{
    const QChar ptr[] = { QChar(0x1234), QChar(0x0000) };
    QString cstr = QString::fromRawData(ptr, 1);
    QVERIFY(!cstr.isDetached());
    QVERIFY(cstr.constData() == ptr);
    QVERIFY(cstr == QString(ptr, 1));
    cstr.squeeze();
    QVERIFY(cstr.constData() == ptr);
    cstr.detach();
    QVERIFY(cstr.size() == 1);
    QVERIFY(cstr.capacity() == 1);
    QVERIFY(cstr.constData() != ptr);
    QVERIFY(cstr.constData()[0] == QChar(0x1234));
    QVERIFY(cstr.constData()[1] == QChar(0x0000));
}

void tst_QString::setRawData()
{
    const QChar ptr[] = { QChar(0x1234), QChar(0x0000) };
    const QChar ptr2[] = { QChar(0x4321), QChar(0x0000) };
    QString cstr;

    // This just tests the fromRawData() fallback
    QVERIFY(!cstr.isDetached());
    cstr.setRawData(ptr, 1);
    QVERIFY(!cstr.isDetached());
    QVERIFY(cstr.constData() == ptr);
    QVERIFY(cstr == QString(ptr, 1));

    QSKIP("This is currently not working.");
    // This actually tests the recycling of the shared data object
    QString::DataPointer csd = cstr.data_ptr();
    cstr.setRawData(ptr2, 1);
    QVERIFY(cstr.isDetached());
    QVERIFY(cstr.constData() == ptr2);
    QVERIFY(cstr == QString(ptr2, 1));
    QVERIFY(cstr.data_ptr() == csd);

    // This tests the discarding of the shared data object
    cstr = "foo";
    QVERIFY(cstr.isDetached());
    QVERIFY(cstr.constData() != ptr2);

    // Another test of the fallback
    csd = cstr.data_ptr();
    cstr.setRawData(ptr2, 1);
    QVERIFY(cstr.isDetached());
    QVERIFY(cstr.constData() == ptr2);
    QVERIFY(cstr == QString(ptr2, 1));
    QVERIFY(cstr.data_ptr() != csd);
}

void tst_QString::setUnicode()
{
    const QChar ptr[] = { QChar(0x1234), QChar(0x0000) };

    QString str;
    QVERIFY(!str.isDetached());
    str.setUnicode(ptr, 1);
    // make sure that the data is copied
    QVERIFY(str.constData() != ptr);
    QVERIFY(str.isDetached());
    QCOMPARE(str, QString(ptr, 1));

    // make sure that the string is resized, even if the data is nullptr
    str = "test";
    QCOMPARE(str.size(), 4);
    str.setUnicode(nullptr, 1);
    QCOMPARE(str.size(), 1);
    QCOMPARE(str, u"t");
}

void tst_QString::fromStdString()
{
    QVERIFY(QString::fromStdString(std::string()).isEmpty());
    std::string stroustrup = "foo";
    QString eng = QString::fromStdString( stroustrup );
    QCOMPARE( eng, QString("foo") );
    const char cnull[] = "Embedded\0null\0character!";
    std::string stdnull( cnull, sizeof(cnull)-1 );
    QString qtnull = QString::fromStdString( stdnull );
    QCOMPARE(qtnull.size(), qsizetype(stdnull.size()));
}

void tst_QString::toStdString()
{
    QString nullStr;
    QVERIFY(nullStr.toStdString().empty());
    QVERIFY(!nullStr.isDetached());

    QString emptyStr("");
    QVERIFY(emptyStr.toStdString().empty());
    QVERIFY(!emptyStr.isDetached());

    QString nord = "foo";
    std::string stroustrup1 = nord.toStdString();
    QVERIFY( qstrcmp(stroustrup1.c_str(), "foo") == 0 );
    // For now, most QString constructors are also broken with respect
    // to embedded null characters, had to find one that works...
    const QChar qcnull[] = {
        'E', 'm', 'b', 'e', 'd', 'd', 'e', 'd', '\0',
        'n', 'u', 'l', 'l', '\0',
        'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', '!'
    };
    QString qtnull( qcnull, sizeof(qcnull)/sizeof(QChar) );
    std::string stdnull = qtnull.toStdString();
    QCOMPARE( int(stdnull.size()), qtnull.size() );
}

void tst_QString::utf8()
{
    QFETCH( QByteArray, utf8 );
    QFETCH( QString, res );

    QCOMPARE(res.toUtf8(), utf8);

    // try rvalue version
    QCOMPARE(std::move(res).toUtf8(), utf8);
}

void tst_QString::fromUtf8_data()
{
    QTest::addColumn<QByteArray>("utf8");
    QTest::addColumn<QString>("res");
    QTest::addColumn<int>("len");
    QString str;

    QTest::newRow("str0") << QByteArray("abcdefgh") << QString("abcdefgh") << -1;
    QTest::newRow("str0-len") << QByteArray("abcdefgh") << QString("abc") << 3;
    QTest::newRow("str1") << QByteArray("\303\266\303\244\303\274\303\226\303\204\303\234\303\270\303\246\303\245\303\230\303\206\303\205")
                          << QString::fromLatin1("\366\344\374\326\304\334\370\346\345\330\306\305") << -1;
    QTest::newRow("str1-len") << QByteArray("\303\266\303\244\303\274\303\226\303\204\303\234\303\270\303\246\303\245\303\230\303\206\303\205")
                              << QString::fromLatin1("\366\344\374\326\304") << 10;

    str += QChar(0x05e9);
    str += QChar(0x05d3);
    str += QChar(0x05d2);
    QTest::newRow("str2") << QByteArray("\327\251\327\223\327\222") << str << -1;

    str = QChar(0x05e9);
    QTest::newRow("str2-len") << QByteArray("\327\251\327\223\327\222") << str << 2;

    str = QChar(0x20ac);
    str += " some text";
    QTest::newRow("str3") << QByteArray("\342\202\254 some text") << str << -1;

    str = QChar(0x20ac);
    str += " some ";
    QTest::newRow("str3-len") << QByteArray("\342\202\254 some text") << str << 9;

    // test that QString::fromUtf8 suppresses an initial BOM, but not a ZWNBSP
    str = "hello";
    QByteArray bom("\357\273\277");
    QTest::newRow("bom0") << bom << QString() << 3;
    QTest::newRow("bom1") << bom + "hello" << str << -1;
    QTest::newRow("bom+zwnbsp0") << bom + bom << QString(QChar(0xfeff)) << -1;
    QTest::newRow("bom+zwnbsp1") << bom + "hello" + bom << str + QChar(0xfeff) << -1;

    str = "hello";
    str += QChar::ReplacementCharacter;
    str += QChar(0x68);
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += QChar(0x61);
    str += QChar::ReplacementCharacter;
    QTest::newRow("invalid utf8") << QByteArray("hello\344h\344\344\366\344a\304") << str << -1;
    QTest::newRow("invalid utf8-len") << QByteArray("hello\344h\344\344\366\344a\304") << QString("hello") << 5;

    str = "Prohl";
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += "e";
    str += QChar::ReplacementCharacter;
    str += " plugin";
    str += QChar::ReplacementCharacter;
    str += " Netscape";

    QTest::newRow("invalid utf8 2") << QByteArray("Prohl\355\276e\350 plugin\371 Netscape") << str << -1;
    QTest::newRow("invalid utf8-len 2") << QByteArray("Prohl\355\276e\350 plugin\371 Netscape") << QString("") << 0;

    QTest::newRow("null-1") << QByteArray() << QString() << -1;
    QTest::newRow("null0") << QByteArray() << QString() << 0;
    QTest::newRow("empty-1") << QByteArray("\0abcd", 5) << QString() << -1;
    QTest::newRow("empty5") << QByteArray("\0abcd", 5) << QString::fromLatin1("\0abcd", 5) << 5;
    QTest::newRow("other-1") << QByteArray("ab\0cd", 5) << QString::fromLatin1("ab") << -1;
    QTest::newRow("other5") << QByteArray("ab\0cd", 5) << QString::fromLatin1("ab\0cd", 5) << 5;

    str = "Old Italic: ";
    str += QChar(0xd800);
    str += QChar(0xdf00);
    str += QChar(0xd800);
    str += QChar(0xdf01);
    str += QChar(0xd800);
    str += QChar(0xdf02);
    str += QChar(0xd800);
    str += QChar(0xdf03);
    str += QChar(0xd800);
    str += QChar(0xdf04);
    QTest::newRow("surrogate") << QByteArray("Old Italic: \360\220\214\200\360\220\214\201\360\220\214\202\360\220\214\203\360\220\214\204") << str << -1;

    QTest::newRow("surrogate-len") << QByteArray("Old Italic: \360\220\214\200\360\220\214\201\360\220\214\202\360\220\214\203\360\220\214\204") << str.left(16) << 20;

}

void tst_QString::fromUtf8()
{
    QFETCH(QByteArray, utf8);
    QFETCH(QString, res);
    QFETCH(int, len);

    QCOMPARE(QString::fromUtf8(utf8.isNull() ? 0 : utf8.data(), len), res);
}

void tst_QString::nullFromUtf8()
{
    QString a;
    a = QString::fromUtf8(0);
    QVERIFY(a.isNull());
    QVERIFY(a.isEmpty());
    a = QString::fromUtf8(nullptr);
    QVERIFY(a.isNull());
    QVERIFY(a.isEmpty());
    a = QString::fromUtf8("");
    QVERIFY(!a.isNull());
    QVERIFY(a.isEmpty());
    a = QString::fromUtf8(u8""); // char in C++17 / char8_t in C++20
    QVERIFY(!a.isNull());
    QVERIFY(a.isEmpty());
    a = QString::fromUtf8(QByteArray());
    QVERIFY(a.isNull());
    QVERIFY(a.isEmpty());
    a = QString::fromUtf8(QByteArray(""));
    QVERIFY(!a.isNull());
    QVERIFY(a.isEmpty());
}

void tst_QString::fromLocal8Bit_data()
{
    QTest::addColumn<QByteArray>("local8Bit");
    QTest::addColumn<int>("len");
    QTest::addColumn<QString>("result");

    //QTest::newRow("nullString") << QByteArray() << -1 << QString();
    //QTest::newRow("emptyString") << QByteArray("") << -1 << QString("");
    //QTest::newRow("string") << QByteArray("test") << -1 << QString("test");
    //QTest::newRow("stringlen0") << QByteArray("test") << 0 << QString("");
    //QTest::newRow("stringlen3") << QByteArray("test") << 3 << QString("tes");
    QTest::newRow("stringlen99") << QByteArray("test\0foo", 8) << 8 << QString::fromLatin1("test\0foo", 8);

    QByteArray longQByteArray;
    QString longQString;

    for (int l=0;l<111;l++) {
        longQByteArray = longQByteArray + "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        longQString += "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    }

    //QTest::newRow("longString") << longQByteArray << -1 << longQString;
    //QTest::newRow("longStringlen0") << longQByteArray << 0 << QString("");
    //QTest::newRow("longStringlen3") << longQByteArray << 3 << QString("aaa");
    //QTest::newRow("someNonAlphaChars") << QByteArray("d:/this/is/a/test.h") << -1 << QString("d:/this/is/a/test.h");

    //QTest::newRow("null-1") << QByteArray() << -1 << QString();
    //QTest::newRow("null0") << QByteArray() << 0 << QString();
    //QTest::newRow("null5") << QByteArray() << 5 << QString();
    //QTest::newRow("empty-1") << QByteArray("\0abcd", 5) << -1 << QString();
    //QTest::newRow("empty0") << QByteArray() << 0 << QString();
    //QTest::newRow("empty5") << QByteArray("\0abcd", 5) << 5 << QString::fromLatin1("\0abcd", 5);
    //QTest::newRow("other-1") << QByteArray("ab\0cd", 5) << -1 << QString::fromLatin1("ab");
    //QTest::newRow("other5") << QByteArray("ab\0cd", 5) << 5 << QString::fromLatin1("ab\0cd", 5);
}

void tst_QString::fromLocal8Bit()
{
    QFETCH(QByteArray, local8Bit);
    QFETCH(int, len);
    QFETCH(QString, result);

    QCOMPARE(QString::fromLocal8Bit(local8Bit.isNull() ? 0 : local8Bit.data(), len).length(),
            result.length());
    QCOMPARE(QString::fromLocal8Bit(local8Bit.isNull() ? 0 : local8Bit.data(), len), result);
}

void tst_QString::local8Bit_data()
{
    QTest::addColumn<QString>("local8Bit");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("nullString") << QString() << QByteArray();
    QTest::newRow("emptyString") << QString("") << QByteArray("");
    QTest::newRow("string") << QString("test") << QByteArray("test");

    QByteArray longQByteArray;
    QString longQString;

    for (int l=0;l<111;l++) {
        longQByteArray = longQByteArray + "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        longQString += "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    }

    QTest::newRow("longString") << longQString << longQByteArray;
    QTest::newRow("someNonAlphaChars") << QString("d:/this/is/a/test.h") << QByteArray("d:/this/is/a/test.h");
}

void tst_QString::local8Bit()
{
    QFETCH(QString, local8Bit);
    QFETCH(QByteArray, result);

    QCOMPARE(local8Bit.toLocal8Bit(), QByteArray(result));
}

void tst_QString::invalidToLocal8Bit_data()
{
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<QByteArray>("expect"); // Initial validly-converted prefix

    {
        const QChar malformed[] = { 'A', QChar(0xd800), 'B', '\0' };
        const char expected[] = "A";
        QTest::newRow("LoneHighSurrogate")
            << QString(malformed, sizeof(malformed) / sizeof(QChar))
            // Don't include the terminating '\0' of expected:
            << QByteArray(expected, sizeof(expected) / sizeof(char) - 1);
    }
    {
        const QChar malformed[] = { 'A', QChar(0xdc00), 'B', '\0' };
        const char expected[] = "A";
        QTest::newRow("LoneLowSurrogate")
            << QString(malformed, sizeof(malformed) / sizeof(QChar))
            << QByteArray(expected, sizeof(expected) / sizeof(char) - 1);
    }
    {
        const QChar malformed[] = { 'A', QChar(0xd800), QChar(0xd801), 'B', '\0' };
        const char expected[] = "A";
        QTest::newRow("DoubleHighSurrogate")
            << QString(malformed, sizeof(malformed) / sizeof(QChar))
            << QByteArray(expected, sizeof(expected) / sizeof(char) - 1);
    }
    {
        const QChar malformed[] = { 'A', QChar(0xdc00), QChar(0xdc01), 'B', '\0' };
        const char expected[] = "A";
        QTest::newRow("DoubleLowSurrogate")
            << QString(malformed, sizeof(malformed) / sizeof(QChar))
            << QByteArray(expected, sizeof(expected) / sizeof(char) - 1);
    }
    {
        const QChar malformed[] = { 'A', QChar(0xdc00), QChar(0xd800), 'B', '\0' };
        const char expected[] = "A";
        QTest::newRow("ReversedSurrogates") // low before high
            << QString(malformed, sizeof(malformed) / sizeof(QChar))
            << QByteArray(expected, sizeof(expected) / sizeof(char) - 1);
    }
}

void tst_QString::invalidToLocal8Bit()
{
    QFETCH(QString, unicode);
    QFETCH(QByteArray, expect);
    QByteArray local = unicode.toLocal8Bit();
    /*
      The main concern of this test is to check that any error-reporting that
      toLocal8Bit() prompts on failure isn't dependent on outputting the data
      it's converting via toLocal8Bit(), which would be apt to recurse.  So the
      real purpose of this QVERIFY(), for all that we should indeed check we get
      the borked output that matches what we can reliably expect (despite
      variation in how codecs respond to errors), is to verify that we got here
      - i.e. we didn't crash in such a recursive stack over-flow.
     */
    QVERIFY(local.startsWith(expect));
}

void tst_QString::nullFromLocal8Bit()
{
    QString a;
    a = QString::fromLocal8Bit(0);
    QVERIFY(a.isNull());
    QVERIFY(a.isEmpty());
    a = QString::fromLocal8Bit("");
    QVERIFY(!a.isNull());
    QVERIFY(a.isEmpty());
    a = QString::fromLocal8Bit(QByteArray());
    QVERIFY(a.isNull());
    QVERIFY(a.isEmpty());
    a = QString::fromLocal8Bit(QByteArray(""));
    QVERIFY(!a.isNull());
    QVERIFY(a.isEmpty());
}

void tst_QString::fromLatin1Roundtrip_data()
{
    QTest::addColumn<QByteArray>("latin1");
    QTest::addColumn<QString>("unicode");

    QTest::newRow("null") << QByteArray() << QString();
    QTest::newRow("empty") << QByteArray("") << "";

    static const char16_t unicode1[] = { 'H', 'e', 'l', 'l', 'o', 1, '\r', '\n', 0x7f };
    QTest::newRow("ascii-only") << QByteArray("Hello") << QString::fromUtf16(unicode1, 5);
    QTest::newRow("ascii+control") << QByteArray("Hello\1\r\n\x7f") << QString::fromUtf16(unicode1, 9);

    static const char16_t unicode3[] = { 'a', 0, 'z' };
    QTest::newRow("ascii+nul") << QByteArray("a\0z", 3) << QString::fromUtf16(unicode3, 3);

    static const char16_t unicode4[] = { 0x80, 0xc0, 0xff };
    QTest::newRow("non-ascii") << QByteArray("\x80\xc0\xff") << QString::fromUtf16(unicode4, 3);
}

void tst_QString::fromLatin1Roundtrip()
{
    QFETCH(QByteArray, latin1);
    QFETCH(QString, unicode);

    // Qt Test safety check:
    QCOMPARE(latin1.isNull(), unicode.isNull());
    QCOMPARE(latin1.isEmpty(), unicode.isEmpty());
    QCOMPARE(latin1.length(), unicode.length());

    if (!latin1.isEmpty())
        while (latin1.length() < 128) {
            latin1 += latin1;
            unicode += unicode;
        }

    // fromLatin1
    QCOMPARE(QString::fromLatin1(latin1, latin1.length()).length(), unicode.length());
    QCOMPARE(QString::fromLatin1(latin1, latin1.length()), unicode);

    // and back:
    QCOMPARE(unicode.toLatin1().length(), latin1.length());
    QCOMPARE(unicode.toLatin1(), latin1);
}

void tst_QString::toLatin1Roundtrip_data()
{
    QTest::addColumn<QByteArray>("latin1");
    QTest::addColumn<QString>("unicodesrc");
    QTest::addColumn<QString>("unicodedst");

    QTest::newRow("null") << QByteArray() << QString() << QString();
    QTest::newRow("empty") << QByteArray("") << "" << "";

    static const char16_t unicode1[] = { 'H', 'e', 'l', 'l', 'o', 1, '\r', '\n', 0x7f };
    QTest::newRow("ascii-only") << QByteArray("Hello") << QString::fromUtf16(unicode1, 5) << QString::fromUtf16(unicode1, 5);
    QTest::newRow("ascii+control") << QByteArray("Hello\1\r\n\x7f") << QString::fromUtf16(unicode1, 9)  << QString::fromUtf16(unicode1, 9);

    static const char16_t unicode3[] = { 'a', 0, 'z' };
    QTest::newRow("ascii+nul") << QByteArray("a\0z", 3) << QString::fromUtf16(unicode3, 3) << QString::fromUtf16(unicode3, 3);

    static const char16_t unicode4[] = { 0x80, 0xc0, 0xff };
    QTest::newRow("non-ascii") << QByteArray("\x80\xc0\xff") << QString::fromUtf16(unicode4, 3) << QString::fromUtf16(unicode4, 3);

    static const char16_t unicodeq[] = { '?', '?', '?', '?', '?' };
    const QString questionmarks = QString::fromUtf16(unicodeq, 5);

    static const char16_t unicode5[] = { 0x100, 0x101, 0x17f, 0x7f00, 0x7f7f };
    QTest::newRow("non-latin1a") << QByteArray("?????") << QString::fromUtf16(unicode5, 5) << questionmarks;

    static const char16_t unicode6[] = { 0x180, 0x1ff, 0x8001, 0x8080, 0xfffc };
    QTest::newRow("non-latin1b") << QByteArray("?????") << QString::fromUtf16(unicode6, 5) << questionmarks;
}

void tst_QString::toLatin1Roundtrip()
{
    QFETCH(QByteArray, latin1);
    QFETCH(QString, unicodesrc);
    QFETCH(QString, unicodedst);

    // Qt Test safety check:
    QCOMPARE(latin1.isNull(), unicodesrc.isNull());
    QCOMPARE(latin1.isEmpty(), unicodesrc.isEmpty());
    QCOMPARE(latin1.length(), unicodesrc.length());
    QCOMPARE(latin1.isNull(), unicodedst.isNull());
    QCOMPARE(latin1.isEmpty(), unicodedst.isEmpty());
    QCOMPARE(latin1.length(), unicodedst.length());

    if (!latin1.isEmpty())
        while (latin1.length() < 128) {
            latin1 += latin1;
            unicodesrc += unicodesrc;
            unicodedst += unicodedst;
        }

    // toLatin1
    QCOMPARE(unicodesrc.toLatin1().length(), latin1.length());
    QCOMPARE(unicodesrc.toLatin1(), latin1);

    // and back:
    QCOMPARE(QString::fromLatin1(latin1, latin1.length()).length(), unicodedst.length());
    QCOMPARE(QString::fromLatin1(latin1, latin1.length()), unicodedst);

    // try the rvalue version of toLatin1()
    QString s = unicodesrc;
    QCOMPARE(std::move(s).toLatin1(), latin1);

    // and verify that the moved-from object can still be used
    s = "foo";
    s.clear();
}

void tst_QString::fromLatin1()
{
    QString a;
    a = QString::fromLatin1( 0 );
    QVERIFY( a.isNull() );
    QVERIFY( a.isEmpty() );
    a = QString::fromLatin1( "" );
    QVERIFY( !a.isNull() );
    QVERIFY( a.isEmpty() );
    a = QString::fromLatin1(QByteArray());
    QVERIFY(a.isNull());
    QVERIFY(a.isEmpty());
    a = QString::fromLatin1(QByteArray(""));
    QVERIFY(!a.isNull());
    QVERIFY(a.isEmpty());

    a = QString::fromLatin1(0, 0);
    QVERIFY(a.isNull());
    a = QString::fromLatin1("\0abcd", 0);
    QVERIFY(!a.isNull());
    QVERIFY(a.isEmpty());
    a = QString::fromLatin1("\0abcd", 5);
    QVERIFY(a.size() == 5);
}

void tst_QString::fromUcs4()
{
    const char32_t *null = nullptr;
    QString s;
    s = QString::fromUcs4( null );
    QVERIFY( s.isNull() );
    QCOMPARE( s.size(), 0 );
    s = QString::fromUcs4( null, 0 );
    QVERIFY( s.isNull() );
    QCOMPARE( s.size(), 0 );
    s = QString::fromUcs4( null, 5 );
    QVERIFY( s.isNull() );
    QCOMPARE( s.size(), 0 );

    char32_t nil = '\0';
    s = QString::fromUcs4( &nil );
    QVERIFY( !s.isNull() );
    QCOMPARE( s.size(), 0 );
    s = QString::fromUcs4( &nil, 0 );
    QVERIFY( !s.isNull() );
    QCOMPARE( s.size(), 0 );

    char32_t bmp = 'a';
    s = QString::fromUcs4( &bmp, 1 );
    QVERIFY( !s.isNull() );
    QCOMPARE( s.size(), 1 );

    char32_t smp = 0x10000;
    s = QString::fromUcs4( &smp, 1 );
    QVERIFY( !s.isNull() );
    QCOMPARE( s.size(), 2 );

    static const char32_t str1[] = U"Hello Unicode World";
    s = QString::fromUcs4(str1, sizeof(str1) / sizeof(str1[0]) - 1);
    QCOMPARE(s, QString("Hello Unicode World"));

    s = QString::fromUcs4(str1);
    QCOMPARE(s, QString("Hello Unicode World"));

    s = QString::fromUcs4(str1, 5);
    QCOMPARE(s, QString("Hello"));

    s = QString::fromUcs4(U"\u221212\U000020AC\U00010000");
    QCOMPARE(s, QString::fromUtf8("\342\210\222" "12" "\342\202\254" "\360\220\200\200"));

    // QTBUG-62011: don't mistake ZWNBS for BOM
    // Start with one BOM, to ensure we use the right endianness:
    const char32_t text[] = { 0xfeff, 97, 0xfeff, 98, 0xfeff, 99, 0xfeff, 100 };
    s = QString::fromUcs4(text, 8);
    QCOMPARE(s, QStringView(u"a\xfeff" u"b\xfeff" u"c\xfeff" "d"));
}

void tst_QString::toUcs4()
{
    QString s;
    QList<uint> ucs4;
    QCOMPARE( s.toUcs4().size(), 0 );
    QVERIFY(!s.isDetached());

    static const QChar bmp = QLatin1Char('a');
    s = QString(&bmp, 1);
    ucs4 = s.toUcs4();
    QCOMPARE( ucs4.size(), 1 );
    QCOMPARE( ucs4.at(0), 0x0061u );

#define QSTRING_FROM_QCHARARRAY(x) (QString((x), sizeof(x)/sizeof((x)[0])))

    static const QChar smp[] = { QChar::highSurrogate(0x10000), QChar::lowSurrogate(0x10000) };
    s = QSTRING_FROM_QCHARARRAY(smp);
    ucs4 = s.toUcs4();
    QCOMPARE( ucs4.size(), 1 );
    QCOMPARE( ucs4.at(0), 0x10000u );

    static const QChar smp2[] = { QChar::highSurrogate(0x10000), QChar::lowSurrogate(0x10000), QChar::highSurrogate(0x10000), QChar::lowSurrogate(0x10000) };
    s = QSTRING_FROM_QCHARARRAY(smp2);
    ucs4 = s.toUcs4();
    QCOMPARE( ucs4.size(), 2 );
    QCOMPARE( ucs4.at(0), 0x10000u );
    QCOMPARE( ucs4.at(1), 0x10000u );

    static const QChar invalid_01[] = { QChar(0xd800) };
    s = QSTRING_FROM_QCHARARRAY(invalid_01);
    ucs4 = s.toUcs4();
    QCOMPARE( ucs4.size(), 1 );
    QCOMPARE( ucs4.at(0), 0xFFFDu );

    static const QChar invalid_02[] = { QChar(0xdc00) };
    s = QSTRING_FROM_QCHARARRAY(invalid_02);
    ucs4 = s.toUcs4();
    QCOMPARE( ucs4.size(), 1 );
    QCOMPARE( ucs4.at(0), 0xFFFDu );

    static const QChar invalid_03[] = { QLatin1Char('a'), QChar(0xd800), QLatin1Char('b') };
    s = QSTRING_FROM_QCHARARRAY(invalid_03);
    ucs4 = s.toUcs4();
    QCOMPARE( ucs4.size(), 3 );
    QCOMPARE( ucs4.at(0), 0x0061u );
    QCOMPARE( ucs4.at(1), 0xFFFDu );
    QCOMPARE( ucs4.at(2), 0x0062u );

    static const QChar invalid_04[] = { QLatin1Char('a'), QChar(0xdc00), QLatin1Char('b') };
    s = QSTRING_FROM_QCHARARRAY(invalid_04);
    ucs4 = s.toUcs4();
    QCOMPARE( ucs4.size(), 3 );
    QCOMPARE( ucs4.at(0), 0x0061u );
    QCOMPARE( ucs4.at(1), 0xFFFDu );
    QCOMPARE( ucs4.at(2), 0x0062u );

    static const QChar invalid_05[] = { QLatin1Char('a'), QChar(0xd800), QChar(0xd800), QLatin1Char('b') };
    s = QSTRING_FROM_QCHARARRAY(invalid_05);
    ucs4 = s.toUcs4();
    QCOMPARE( ucs4.size(), 4 );
    QCOMPARE( ucs4.at(0), 0x0061u );
    QCOMPARE( ucs4.at(1), 0xFFFDu );
    QCOMPARE( ucs4.at(2), 0xFFFDu );
    QCOMPARE( ucs4.at(3), 0x0062u );

    static const QChar invalid_06[] = { QLatin1Char('a'), QChar(0xdc00), QChar(0xdc00), QLatin1Char('b') };
    s = QSTRING_FROM_QCHARARRAY(invalid_06);
    ucs4 = s.toUcs4();
    QCOMPARE( ucs4.size(), 4 );
    QCOMPARE( ucs4.at(0), 0x0061u );
    QCOMPARE( ucs4.at(1), 0xFFFDu );
    QCOMPARE( ucs4.at(2), 0xFFFDu );
    QCOMPARE( ucs4.at(3), 0x0062u );

#undef QSTRING_FROM_QCHARARRAY

}

void tst_QString::arg()
{
/*
    Warning: If any of these test fails, the warning given by Qt Test
    is all messed up, because Qt Test itself uses QString::arg().
*/

    TransientDefaultLocale transient(QLocale(QString("de_DE")));

    QString s3;
    QString s4( "[%0]" );
    QString s5( "[%1]" );
    QString s6( "[%3]" );
    QString s7( "[%9]" );
    QString s8( "[%0 %1]" );
    QString s9( "[%0 %3]" );
    QString s10( "[%1 %2 %3]" );
    QString s11( "[%9 %3 %0]" );
    QString s12( "[%9 %1 %3 %9 %0 %8]" );
    QString s13( "%1% %x%c%2 %d%2-%" );
    QString s14( "%1%2%3" );

    QCOMPARE( s4.arg("foo"), QLatin1String("[foo]") );
    QCOMPARE( s5.arg(QLatin1String("foo")), QLatin1String("[foo]") );
    QCOMPARE( s6.arg(u"foo"), QLatin1String("[foo]") );
    QCOMPARE( s7.arg("foo"), QLatin1String("[foo]") );
    QCOMPARE( s8.arg("foo"), QLatin1String("[foo %1]") );
    QCOMPARE( s8.arg("foo").arg("bar"), QLatin1String("[foo bar]") );
    QCOMPARE( s8.arg("foo", "bar"), QLatin1String("[foo bar]") );
    QCOMPARE( s9.arg("foo"), QLatin1String("[foo %3]") );
    QCOMPARE( s9.arg("foo").arg("bar"), QLatin1String("[foo bar]") );
    QCOMPARE( s9.arg("foo", "bar"), QLatin1String("[foo bar]") );
    QCOMPARE( s10.arg("foo"), QLatin1String("[foo %2 %3]") );
    QCOMPARE( s10.arg("foo").arg("bar"), QLatin1String("[foo bar %3]") );
    QCOMPARE( s10.arg("foo", "bar"), QLatin1String("[foo bar %3]") );
    QCOMPARE( s10.arg("foo").arg("bar").arg("baz"), QLatin1String("[foo bar baz]") );
    QCOMPARE( s10.arg("foo", "bar", "baz"), QLatin1String("[foo bar baz]") );
    QCOMPARE( s11.arg("foo"), QLatin1String("[%9 %3 foo]") );
    QCOMPARE( s11.arg("foo").arg("bar"), QLatin1String("[%9 bar foo]") );
    QCOMPARE( s11.arg("foo", "bar"), QLatin1String("[%9 bar foo]") );
    QCOMPARE( s11.arg("foo").arg("bar").arg("baz"), QLatin1String("[baz bar foo]") );
    QCOMPARE( s11.arg("foo", "bar", "baz"), QLatin1String("[baz bar foo]") );
    QCOMPARE( s12.arg("a").arg("b").arg("c").arg("d").arg("e"),
             QLatin1String("[e b c e a d]") );
    QCOMPARE( s12.arg("a", "b", "c", "d").arg("e"), QLatin1String("[e b c e a d]") );
    QCOMPARE( s12.arg("a").arg("b", "c", "d", "e"), QLatin1String("[e b c e a d]") );
    QCOMPARE( s13.arg("alpha").arg("beta"),
             QLatin1String("alpha% %x%cbeta %dbeta-%") );
    QCOMPARE( s13.arg("alpha", "beta"), QLatin1String("alpha% %x%cbeta %dbeta-%") );
    QCOMPARE( s14.arg("a", "b", "c"), QLatin1String("abc") );
    QCOMPARE( s8.arg("%1").arg("foo"), QLatin1String("[foo foo]") );
    QCOMPARE( s8.arg("%1", "foo"), QLatin1String("[%1 foo]") );
    QCOMPARE( s4.arg("foo", 2), QLatin1String("[foo]") );
    QCOMPARE( s4.arg("foo", -2), QLatin1String("[foo]") );
    QCOMPARE( s4.arg("foo", 10), QLatin1String("[       foo]") );
    QCOMPARE( s4.arg("foo", -10), QLatin1String("[foo       ]") );

    QString firstName( "James" );
    QString lastName( "Bond" );
    QString fullName = QString( "My name is %2, %1 %2" )
                       .arg( firstName ).arg( lastName );
    QCOMPARE( fullName, QLatin1String("My name is Bond, James Bond") );

    // number overloads
    QCOMPARE( s4.arg(0), QLatin1String("[0]") );
    QCOMPARE( s4.arg(-1), QLatin1String("[-1]") );
    QCOMPARE( s4.arg(4294967295UL), QLatin1String("[4294967295]") ); // ULONG_MAX 32
    QCOMPARE( s4.arg(Q_INT64_C(9223372036854775807)), // LLONG_MAX
             QLatin1String("[9223372036854775807]") );

    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: , foo");
    QCOMPARE(QString().arg("foo"), QString());
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"\" , 0");
    QCOMPARE( QString().arg(0), QString() );
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"\" , 0");
    QCOMPARE( QString("").arg(0), QString("") );
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \" \" , 0");
    QCOMPARE( QString(" ").arg(0), QLatin1String(" ") );
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"%\" , 0");
    QCOMPARE( QString("%").arg(0), QLatin1String("%") );
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"%%\" , 0");
    QCOMPARE( QString("%%").arg(0), QLatin1String("%%") );
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"%%%\" , 0");
    QCOMPARE( QString("%%%").arg(0), QLatin1String("%%%") );
    QCOMPARE( QString("%%%1%%%2").arg("foo").arg("bar"), QLatin1String("%%foo%%bar") );

    QCOMPARE( QString("%1").arg("hello", -10), QLatin1String("hello     ") );
    QCOMPARE( QString("%1").arg(QLatin1String("hello"), -5), QLatin1String("hello") );
    QCOMPARE( QString("%1").arg(u"hello", -2), QLatin1String("hello") );
    QCOMPARE( QString("%1").arg("hello", 0), QLatin1String("hello") );
    QCOMPARE( QString("%1").arg(QLatin1String("hello"), 2), QLatin1String("hello") );
    QCOMPARE( QString("%1").arg(u"hello", 5), QLatin1String("hello") );
    QCOMPARE( QString("%1").arg("hello", 10), QLatin1String("     hello") );
    QCOMPARE( QString("%1%1").arg("hello"), QLatin1String("hellohello") );
    QCOMPARE( QString("%2%1").arg("hello"), QLatin1String("%2hello") );
    QCOMPARE( QString("%1%1").arg(QString()), QLatin1String("") );
    QCOMPARE( QString("%2%1").arg(""), QLatin1String("%2") );

    QCOMPARE( QString("%2 %L1").arg(12345.6789).arg(12345.6789),
             QLatin1String("12345.7 12.345,7") );
    QCOMPARE( QString("[%2] [%L1]").arg(12345.6789, 9).arg(12345.6789, 9),
              QLatin1String("[  12345.7] [ 12.345,7]") );
    QCOMPARE( QString("[%2] [%L1]").arg(12345.6789, 9, 'g', 7).arg(12345.6789, 9, 'g', 7),
              QLatin1String("[ 12345.68] [12.345,68]") );
    QCOMPARE( QString("[%2] [%L1]").arg(12345.6789, 10, 'g', 7, QLatin1Char('0')).arg(12345.6789, 10, 'g', 7, QLatin1Char('0')),
              QLatin1String("[0012345.68] [012.345,68]") );

    QCOMPARE( QString("%2 %L1").arg(123456789).arg(123456789),
             QLatin1String("123456789 123.456.789") );
    QCOMPARE( QString("[%2] [%L1]").arg(123456789, 12).arg(123456789, 12),
              QLatin1String("[   123456789] [ 123.456.789]") );
    QCOMPARE( QString("[%2] [%L1]").arg(123456789, 13, 10, QLatin1Char('0')).arg(123456789, 12, 10, QLatin1Char('0')),
              QLatin1String("[000123456789] [00123.456.789]") );
    QCOMPARE( QString("[%2] [%L1]").arg(123456789, 13, 16, QLatin1Char('0')).arg(123456789, 12, 16, QLatin1Char('0')),
              QLatin1String("[0000075bcd15] [00000075bcd15]") );

    QCOMPARE( QString("%L2 %L1 %3").arg(12345.7).arg(123456789).arg('c'),
             QLatin1String("123.456.789 12.345,7 c") );

    // multi-digit replacement
    QString input("%%%L0 %1 %02 %3 %4 %5 %L6 %7 %8 %%% %090 %10 %11 %L12 %14 %L9888 %9999 %%%%%%%L");
    input = input.arg("A").arg("B").arg("C")
                 .arg("D").arg("E").arg("f")
                 .arg("g").arg("h").arg("i").arg("j")
                 .arg("k").arg("l").arg("m")
                 .arg("n").arg("o").arg("p");

    QCOMPARE(input, QLatin1String("%%A B C D E f g h i %%% j0 k l m n o88 p99 %%%%%%%L"));

    QString str("%1 %2 %3 %4 %5 %6 %7 %8 %9 foo %10 %11 bar");
    str = str.arg("one", "2", "3", "4", "5", "6", "7", "8", "9");
    str = str.arg("ahoy", "there");
    QCOMPARE(str, QLatin1String("one 2 3 4 5 6 7 8 9 foo ahoy there bar"));

    QString str2("%123 %234 %345 %456 %567 %999 %1000 %1230");
    str2 = str2.arg("A", "B", "C", "D", "E", "F");
    QCOMPARE(str2, QLatin1String("A B C D E F %1000 %1230"));

    QCOMPARE(QString("%1").arg(-1, 3, 10, QChar('0')), QLatin1String("-01"));
    QCOMPARE(QString("%1").arg(-100, 3, 10, QChar('0')), QLatin1String("-100"));
    QCOMPARE(QString("%1").arg(-1, 3, 10, QChar(' ')), QLatin1String(" -1"));
    QCOMPARE(QString("%1").arg(-100, 3, 10, QChar(' ')), QLatin1String("-100"));
    QCOMPARE(QString("%1").arg(1U, 3, 10, QChar(' ')), QLatin1String("  1"));
    QCOMPARE(QString("%1").arg(1000U, 3, 10, QChar(' ')), QLatin1String("1000"));
    QCOMPARE(QString("%1").arg(-1, 3, 10, QChar('x')), QLatin1String("x-1"));
    QCOMPARE(QString("%1").arg(-100, 3, 10, QChar('x')), QLatin1String("-100"));
    QCOMPARE(QString("%1").arg(1U, 3, 10, QChar('x')), QLatin1String("xx1"));
    QCOMPARE(QString("%1").arg(1000U, 3, 10, QChar('x')), QLatin1String("1000"));

    QCOMPARE(QString("%1").arg(-1., 3, 'g', -1, QChar('0')), QLatin1String("-01"));
    QCOMPARE(QString("%1").arg(-100., 3, 'g', -1, QChar('0')), QLatin1String("-100"));
    QCOMPARE(QString("%1").arg(-1., 3, 'g', -1, QChar(' ')), QLatin1String(" -1"));
    QCOMPARE(QString("%1").arg(-100., 3, 'g', -1, QChar(' ')), QLatin1String("-100"));
    QCOMPARE(QString("%1").arg(1., 3, 'g', -1, QChar('x')), QLatin1String("xx1"));
    QCOMPARE(QString("%1").arg(1000., 3, 'g', -1, QChar('x')), QLatin1String("1000"));
    QCOMPARE(QString("%1").arg(-1., 3, 'g', -1, QChar('x')), QLatin1String("x-1"));
    QCOMPARE(QString("%1").arg(-100., 3, 'g', -1, QChar('x')), QLatin1String("-100"));

    transient.revise(QLocale(QString("ar")));
    QCOMPARE( QString("%L1").arg(12345.6789, 10, 'g', 7, QLatin1Char('0')),
              QString::fromUtf8("\xd9\xa0\xd9\xa1\xd9\xa2\xd9\xac\xd9\xa3\xd9\xa4\xd9\xa5\xd9\xab\xd9\xa6\xd9\xa8") ); // "٠١٢٬٣٤٥٫٦٨"
    QCOMPARE( QString("%L1").arg(123456789, 13, 10, QLatin1Char('0')),
              QString("\xd9\xa0\xd9\xa0\xd9\xa1\xd9\xa2\xd9\xa3\xd9\xac\xd9\xa4\xd9\xa5\xd9\xa6\xd9\xac\xd9\xa7\xd9\xa8\xd9\xa9") ); // ٠٠١٢٣٬٤٥٦٬٧٨٩
}

void tst_QString::number()
{
    QCOMPARE(QString::number(int(0)), QLatin1String("0"));
    QCOMPARE(QString::number(std::copysign(0.0, -1.0)), QLatin1String("0"));
    QCOMPARE(QString::number((unsigned int)(11)), QLatin1String("11"));
    QCOMPARE(QString::number(-22L), QLatin1String("-22"));
    QCOMPARE(QString::number(333UL), QLatin1String("333"));
    QCOMPARE(QString::number(4.4), QLatin1String("4.4"));
    QCOMPARE(QString::number(Q_INT64_C(-555)), QLatin1String("-555"));
    QCOMPARE(QString::number(Q_UINT64_C(6666)), QLatin1String("6666"));

#ifndef QT_NO_DOUBLECONVERSION // snprintf_l is too stupid for this
    QCOMPARE( QString::number(12.05, 'f', 1), QString("12.1") );
    QCOMPARE( QString::number(12.5, 'f', 0), QString("13") );
#endif
}

void tst_QString::number_base_data()
{
    QTest::addColumn<qlonglong>("n");
    QTest::addColumn<int>("base");
    QTest::addColumn<QString>("expected");

    QTest::newRow("base 10, positive") << 12346LL << 10 << QString("12346");
    QTest::newRow("base  2, positive") << 12346LL <<  2 << QString("11000000111010");
    QTest::newRow("base  8, positive") << 12346LL <<  8 << QString("30072");
    QTest::newRow("base 16, positive") << 12346LL << 16 << QString("303a");
    QTest::newRow("base 17, positive") << 12346LL << 17 << QString("28c4");
    QTest::newRow("base 36, positive") << 2181789482LL << 36 << QString("102zbje");

    QTest::newRow("base 10, negative") << -12346LL << 10 << QString("-12346");
    QTest::newRow("base  2, negative") << -12346LL <<  2 << QString("-11000000111010");
    QTest::newRow("base  8, negative") << -12346LL <<  8 << QString("-30072");
    QTest::newRow("base 16, negative") << -12346LL << 16 << QString("-303a");
    QTest::newRow("base 17, negative") << -12346LL << 17 << QString("-28c4");
    QTest::newRow("base 36, negative") << -2181789482LL << 36 << QString("-102zbje");

    QTest::newRow("base  2, negative") << -1LL << 2 << QString("-1");

    QTest::newRow("largeint, base 10, positive")
            << 123456789012LL << 10 << QString("123456789012");
    QTest::newRow("largeint, base  2, positive")
            << 123456789012LL <<  2 << QString("1110010111110100110010001101000010100");
    QTest::newRow("largeint, base  8, positive")
            << 123456789012LL <<  8 << QString("1627646215024");
    QTest::newRow("largeint, base 16, positive")
            << 123456789012LL << 16 << QString("1cbe991a14");
    QTest::newRow("largeint, base 17, positive")
            << 123456789012LL << 17 << QString("10bec2b629");

    QTest::newRow("largeint, base 10, negative")
            << -123456789012LL << 10 << QString("-123456789012");
    QTest::newRow("largeint, base  2, negative")
            << -123456789012LL <<  2 << QString("-1110010111110100110010001101000010100");
    QTest::newRow("largeint, base  8, negative")
            << -123456789012LL <<  8 << QString("-1627646215024");
    QTest::newRow("largeint, base 16, negative")
            << -123456789012LL << 16 << QString("-1cbe991a14");
    QTest::newRow("largeint, base 17, negative")
            << -123456789012LL << 17 << QString("-10bec2b629");
}

void tst_QString::number_base()
{
    QFETCH( qlonglong, n );
    QFETCH( int, base );
    QFETCH( QString, expected );
    QCOMPARE(QString::number(n, base), expected);

    // check qlonglong->QString->qlonglong round trip
    for (int ibase = 2; ibase <= 36; ++ibase) {
        auto stringrep = QString::number(n, ibase);
        bool ok(false);
        auto result = stringrep.toLongLong(&ok, ibase);
        QVERIFY(ok);
        QCOMPARE(n, result);
    }
}

void tst_QString::doubleOut()
{
    // Regression test for QTBUG-63620; the first two paths lost the exponent's
    // leading 0 at 5.7; C's printf() family guarantee a two-digit exponent (in
    // contrast with ECMAScript, which forbids leading zeros).
    const QString expect(QStringLiteral("1e-06"));
    const double micro = 1e-6;
    QCOMPARE(QString::number(micro), expect);
    QCOMPARE(QString("%1").arg(micro), expect);
    {
        QCOMPARE(QString::asprintf("%g", micro), expect);
    }
    {
        QString text;
        QTextStream stream(&text);
        stream << micro;
        QCOMPARE(text, expect);
    }
}

void tst_QString::capacity_data()
{
    length_data();
}

void tst_QString::capacity()
{
    QFETCH( QString, s1 );
    QFETCH( qsizetype, res );

    QString s2( s1 );
    s2.reserve( res );
    QVERIFY( s2.capacity() >= res );
    QCOMPARE( s2, s1 );

    s2 = s1;    // share again
    s2.reserve( res * 2 );
    QVERIFY( s2.capacity() >=  res * 2 );
    if (res != 0)  // can both point to QString::_empty when empty
        QVERIFY(s2.constData() != s1.constData());
    QCOMPARE( s2, s1 );

    // don't share again -- s2 must be detached for squeeze() to do anything
    s2.squeeze();
    QVERIFY( s2.capacity() == res );
    QCOMPARE( s2, s1 );

    s2 = s1;    // share again
    int oldsize = s1.size();
    s2.reserve( res / 2 );
    QVERIFY( s2.capacity() >=  res / 2 );
    QVERIFY( s2.capacity() >=  oldsize );
    QCOMPARE( s2, s1 );
}

void tst_QString::section_data()
{
    QTest::addColumn<QString>("wholeString" );
    QTest::addColumn<QString>("sep" );
    QTest::addColumn<int>("start" );
    QTest::addColumn<int>("end" );
    QTest::addColumn<int>("flags" );
    QTest::addColumn<QString>("sectionString" );
    QTest::addColumn<bool>("regexp" );

    QTest::newRow("null") << QString() << QString(",") << 0 << -1 << int(QString::SectionDefault) << QString() << false;
    QTest::newRow("empty") << QString("") << QString(",") << 0 << -1 << int(QString::SectionDefault) << QString("") << false;
    QTest::newRow( "data0" ) << QString("forename,middlename,surname,phone") << QString(",") << 2 << 2 << int(QString::SectionDefault) << QString("surname") << false;
    QTest::newRow( "data1" ) << QString("/usr/local/bin/myapp") << QString("/") << 3 << 4 << int(QString::SectionDefault) << QString("bin/myapp") << false;
    QTest::newRow( "data2" ) << QString("/usr/local/bin/myapp") << QString("/") << 3 << 3 << int(QString::SectionSkipEmpty) << QString("myapp") << false;
    QTest::newRow( "data3" ) << QString("forename**middlename**surname**phone") << QString("**") << 2 << 2 << int(QString::SectionDefault) << QString("surname") << false;
    QTest::newRow( "data4" ) << QString("forename**middlename**surname**phone") << QString("**") << -3 << -2 << int(QString::SectionDefault) << QString("middlename**surname") << false;
    QTest::newRow( "data5" ) << QString("##Datt######wollen######wir######mal######sehen##") << QString("#") << 0 << 0 << int(QString::SectionSkipEmpty) << QString("Datt") << false;
    QTest::newRow( "data6" ) << QString("##Datt######wollen######wir######mal######sehen##") << QString("#") << 1 << 1 << int(QString::SectionSkipEmpty) << QString("wollen") << false;
    QTest::newRow( "data7" ) << QString("##Datt######wollen######wir######mal######sehen##") << QString("#") << 2 << 2 << int(QString::SectionSkipEmpty) << QString("wir") << false;
    QTest::newRow( "data8" ) << QString("##Datt######wollen######wir######mal######sehen##") << QString("#") << 3 << 3 << int(QString::SectionSkipEmpty) << QString("mal") << false;
    QTest::newRow( "data9" ) << QString("##Datt######wollen######wir######mal######sehen##") << QString("#") << 4 << 4 << int(QString::SectionSkipEmpty) << QString("sehen") << false;
    // not fixed for 3.1
    QTest::newRow( "data10" ) << QString("a/b/c/d") << QString("/") << 1 << -1 << int(QString::SectionIncludeLeadingSep | QString::SectionIncludeTrailingSep) << QString("/b/c/d") << false;
    QTest::newRow( "data11" ) << QString("aoLoboLocolod") << QString("olo") << -1 << -1 << int(QString::SectionCaseInsensitiveSeps) << QString("d") << false;
    QTest::newRow( "data12" ) << QString("F0") << QString("F") << 0 << 0 << int(QString::SectionSkipEmpty) << QString("0") << false;
    QTest::newRow( "foo1" ) << QString("foo;foo;") << QString(";") << 0 << 0
                         << int(QString::SectionIncludeLeadingSep) << QString("foo") << false;
    QTest::newRow( "foo2" ) << QString("foo;foo;") << QString(";") << 1 << 1
                         << int(QString::SectionIncludeLeadingSep) << QString(";foo") << false;
    QTest::newRow( "foo3" ) << QString("foo;foo;") << QString(";") << 2 << 2
                         << int(QString::SectionIncludeLeadingSep) << QString(";") << false;
    QTest::newRow( "foo1rx" ) << QString("foo;foo;") << QString(";") << 0 << 0
                           << int(QString::SectionIncludeLeadingSep) << QString("foo") << true;
    QTest::newRow( "foo2rx" ) << QString("foo;foo;") << QString(";") << 1 << 1
                           << int(QString::SectionIncludeLeadingSep) << QString(";foo") << true;
    QTest::newRow( "foo3rx" ) << QString("foo;foo;") << QString(";") << 2 << 2
                           << int(QString::SectionIncludeLeadingSep) << QString(";") << true;

    QTest::newRow( "qmake_path" ) << QString("/Users/sam/troll/qt4.0/src/corelib/QtCore_debug.xcode/")
                               << QString("/") << 0 << -2 << int(QString::SectionDefault)
                               << QString("/Users/sam/troll/qt4.0/src/corelib/QtCore_debug.xcode") << false;
    QTest::newRow( "qmake_pathrx" ) << QString("/Users/sam/troll/qt4.0/src/corelib/QtCore_debug.xcode/")
                                 << QString("/") << 0 << -2 << int(QString::SectionDefault)
                                 << QString("/Users/sam/troll/qt4.0/src/corelib/QtCore_debug.xcode") << true;
    QTest::newRow( "data13" ) << QString("||2|3|||")
                              << QString("|") << 0 << 1 << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                              << QString("||") << false;
    QTest::newRow( "data14" ) << QString("||2|3|||")
                                << QString("\\|") << 0 << 1 << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                                << QString("||") << true;
    QTest::newRow( "data15" ) << QString("|1|2|")
                                << QString("|") << 0 << 1 << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                                << QString("|1|") << false;
    QTest::newRow( "data16" ) << QString("|1|2|")
                                << QString("\\|") << 0 << 1 << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                                << QString("|1|") << true;
    QTest::newRow( "normal1" ) << QString("o1o2o")
                            << QString("o") << 0 << 0
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString("o") << false;
    QTest::newRow( "normal2" ) << QString("o1o2o")
                            << QString("o") << 1 << 1
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString("o1o") << false;
    QTest::newRow( "normal3" ) << QString("o1o2o")
                            << QString("o") << 2 << 2
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString("o2o") << false;
    QTest::newRow( "normal4" ) << QString("o1o2o")
                            << QString("o") << 2 << 3
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString("o2o") << false;
    QTest::newRow( "normal5" ) << QString("o1o2o")
                            << QString("o") << 1 << 2
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString("o1o2o") << false;
    QTest::newRow( "range1" ) << QString("o1o2o")
                            << QString("o") << -5 << -5
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString() << false;
    QTest::newRow( "range2" ) << QString("oo1o2o")
                            << QString("o") << -5 << 1
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep
                                   |QString::SectionSkipEmpty)
                            << QString("oo1o2o") << false;
    QTest::newRow( "range3" ) << QString("o1o2o")
                            << QString("o") << 2 << 1
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString() << false;
    QTest::newRow( "range4" ) << QString("o1o2o")
                            << QString("o") << 4 << 4
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString() << false;
    QTest::newRow( "range5" ) << QString("o1oo2o")
                            << QString("o") << -2 << -1
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep
                                   |QString::SectionSkipEmpty)
                            << QString("o1oo2o") << false;
    QTest::newRow( "rx1" ) << QString("o1o2o")
                        << QString("[a-z]") << 0 << 0
                        << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                        << QString("o") << true;
    QTest::newRow( "rx2" ) << QString("o1o2o")
                        << QString("[a-z]") << 1 << 1
                        << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                        << QString("o1o") << true;
    QTest::newRow( "rx3" ) << QString("o1o2o")
                        << QString("[a-z]") << 2 << 2
                        << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                        << QString("o2o") << true;
    QTest::newRow( "rx4" ) << QString("o1o2o")
                        << QString("[a-z]") << 2 << 3
                        << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                        << QString("o2o") << true;
    QTest::newRow( "rx5" ) << QString("o1o2o")
                        << QString("[a-z]") << 1 << 2
                        << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                        << QString("o1o2o") << true;
    QTest::newRow( "data17" ) << QString("This is a story, a small story")
                        << QString("\\b") << 3 << 3
                        << int(QString::SectionDefault)
                        << QString("is") << true;
    QTest::newRow( "data18" ) << QString("99.0 42.3")
                        << QString("\\s*[AaBb]\\s*") << 1 << 1
                        << int(QString::SectionIncludeLeadingSep)
                        << QString() << true;
}

void tst_QString::section()
{
    QFETCH( QString, wholeString );
    QFETCH( QString, sep );
    QFETCH( int, start );
    QFETCH( int, end );
    QFETCH( int, flags );
    QFETCH( QString, sectionString );
    QFETCH( bool, regexp );
    if (regexp) {
#if QT_CONFIG(regularexpression)
        QCOMPARE( wholeString.section( QRegularExpression(sep), start, end, QString::SectionFlag(flags) ), sectionString );
#else
        QSKIP("QRegularExpression not supported");
#endif
    } else {
        if (sep.size() == 1)
            QCOMPARE( wholeString.section( sep[0], start, end, QString::SectionFlag(flags) ), sectionString );
        QCOMPARE( wholeString.section( sep, start, end, QString::SectionFlag(flags) ), sectionString );
#if QT_CONFIG(regularexpression)
        QCOMPARE( wholeString.section( QRegularExpression(QRegularExpression::escape(sep)), start, end, QString::SectionFlag(flags) ), sectionString );
#endif
    }
}


void tst_QString::operator_eqeq_nullstring()
{
    /* Some of these might not be all that logical but it's the behaviour we've had since 3.0.0
       so we should probably stick with it. */

    QVERIFY( QString() == "" );
    QVERIFY( "" == QString() );

    QVERIFY( QString("") == "" );
    QVERIFY( "" == QString("") );

    QVERIFY(QString() == nullptr);
    QVERIFY(nullptr == QString());

    QVERIFY(QString("") == nullptr);
    QVERIFY(nullptr == QString(""));

    QVERIFY( QString().size() == 0 );

    QVERIFY( QString("").size() == 0 );

    QVERIFY( QString() == QString("") );
    QVERIFY( QString("") == QString() );
}

void tst_QString::operator_smaller()
{
    QString null;
    QString empty("");
    QString foo("foo");
    const char *nullC = nullptr;
    const char *emptyC = "";

    QVERIFY( !(null < QString()) );
    QVERIFY( !(null > QString()) );

    QVERIFY( !(empty < QString("")) );
    QVERIFY( !(empty > QString("")) );

    QVERIFY( !(null < empty) );
    QVERIFY( !(null > empty) );

    QVERIFY( !(nullC < empty) );
    QVERIFY( !(nullC > empty) );

    QVERIFY( !(null < emptyC) );
    QVERIFY( !(null > emptyC) );

    QVERIFY( null < foo );
    QVERIFY( !(null > foo) );
    QVERIFY( foo > null );
    QVERIFY( !(foo < null) );

    QVERIFY( empty < foo );
    QVERIFY( !(empty > foo) );
    QVERIFY( foo > empty );
    QVERIFY( !(foo < empty) );

    QVERIFY( !(null < QLatin1String(0)) );
    QVERIFY( !(null > QLatin1String(0)) );
    QVERIFY( !(null < QLatin1String("")) );
    QVERIFY( !(null > QLatin1String("")) );

    QVERIFY( !(null < QLatin1String("")) );
    QVERIFY( !(null > QLatin1String("")) );
    QVERIFY( !(empty < QLatin1String("")) );
    QVERIFY( !(empty > QLatin1String("")) );

    QVERIFY( !(QLatin1String(0) < null) );
    QVERIFY( !(QLatin1String(0) > null) );
    QVERIFY( !(QLatin1String("") < null) );
    QVERIFY( !(QLatin1String("") > null) );

    QVERIFY( !(QLatin1String(0) < empty) );
    QVERIFY( !(QLatin1String(0) > empty) );
    QVERIFY( !(QLatin1String("") < empty) );
    QVERIFY( !(QLatin1String("") > empty) );

    QVERIFY( QLatin1String(0) < foo );
    QVERIFY( !(QLatin1String(0) > foo) );
    QVERIFY( QLatin1String("") < foo );
    QVERIFY( !(QLatin1String("") > foo) );

    QVERIFY( foo > QLatin1String(0) );
    QVERIFY( !(foo < QLatin1String(0)) );
    QVERIFY( foo > QLatin1String("") );
    QVERIFY( !(foo < QLatin1String("")) );

    QVERIFY( QLatin1String(0) == empty);
    QVERIFY( QLatin1String(0) == null);
    QVERIFY( QLatin1String("") == empty);
    QVERIFY( QLatin1String("") == null);

    QVERIFY( !(foo < QLatin1String("foo")));
    QVERIFY( !(foo > QLatin1String("foo")));
    QVERIFY( !(QLatin1String("foo") < foo));
    QVERIFY( !(QLatin1String("foo") > foo));

    QVERIFY( !(foo < QLatin1String("a")));
    QVERIFY( (foo > QLatin1String("a")));
    QVERIFY( (QLatin1String("a") < foo));
    QVERIFY( !(QLatin1String("a") > foo));

    QVERIFY( (foo < QLatin1String("z")));
    QVERIFY( !(foo > QLatin1String("z")));
    QVERIFY( !(QLatin1String("z") < foo));
    QVERIFY( (QLatin1String("z") > foo));

    // operator< is not locale-aware (or shouldn't be)
    QVERIFY( foo < QString("\xc3\xa9") );
    QVERIFY( foo < "\xc3\xa9" );

    QVERIFY(QString("a") < QString("b"));
    QVERIFY(QString("a") <= QString("b"));
    QVERIFY(QString("a") <= QString("a"));
    QVERIFY(QString("a") == QString("a"));
    QVERIFY(QString("a") >= QString("a"));
    QVERIFY(QString("b") >= QString("a"));
    QVERIFY(QString("b") > QString("a"));

    QVERIFY("a" < QString("b"));
    QVERIFY("a" <= QString("b"));
    QVERIFY("a" <= QString("a"));
    QVERIFY("a" == QString("a"));
    QVERIFY("a" >= QString("a"));
    QVERIFY("b" >= QString("a"));
    QVERIFY("b" > QString("a"));

    QVERIFY(QString("a") < "b");
    QVERIFY(QString("a") <= "b");
    QVERIFY(QString("a") <= "a");
    QVERIFY(QString("a") == "a");
    QVERIFY(QString("a") >= "a");
    QVERIFY(QString("b") >= "a");
    QVERIFY(QString("b") > "a");

    QVERIFY(QString("a") < QByteArray("b"));
    QVERIFY(QString("a") <= QByteArray("b"));
    QVERIFY(QString("a") <= QByteArray("a"));
    QVERIFY(QString("a") == QByteArray("a"));
    QVERIFY(QString("a") >= QByteArray("a"));
    QVERIFY(QString("b") >= QByteArray("a"));
    QVERIFY(QString("b") > QByteArray("a"));

    QVERIFY(QLatin1String("a") < QString("b"));
    QVERIFY(QLatin1String("a") <= QString("b"));
    QVERIFY(QLatin1String("a") <= QString("a"));
    QVERIFY(QLatin1String("a") == QString("a"));
    QVERIFY(QLatin1String("a") >= QString("a"));
    QVERIFY(QLatin1String("b") >= QString("a"));
    QVERIFY(QLatin1String("b") > QString("a"));

    QVERIFY(QString("a") < QLatin1String("b"));
    QVERIFY(QString("a") <= QLatin1String("b"));
    QVERIFY(QString("a") <= QLatin1String("a"));
    QVERIFY(QString("a") == QLatin1String("a"));
    QVERIFY(QString("a") >= QLatin1String("a"));
    QVERIFY(QString("b") >= QLatin1String("a"));
    QVERIFY(QString("b") > QLatin1String("a"));

    QVERIFY("a" < QLatin1String("b"));
    QVERIFY("a" <= QLatin1String("b"));
    QVERIFY("a" <= QLatin1String("a"));
    QVERIFY("a" == QLatin1String("a"));
    QVERIFY("a" >= QLatin1String("a"));
    QVERIFY("b" >= QLatin1String("a"));
    QVERIFY("b" > QLatin1String("a"));

    QVERIFY(QLatin1String("a") < "b");
    QVERIFY(QLatin1String("a") <= "b");
    QVERIFY(QLatin1String("a") <= "a");
    QVERIFY(QLatin1String("a") == "a");
    QVERIFY(QLatin1String("a") >= "a");
    QVERIFY(QLatin1String("b") >= "a");
    QVERIFY(QLatin1String("b") > "a");
}

void tst_QString::integer_conversion_data()
{
    QTest::addColumn<QString>("num_str");
    QTest::addColumn<int>("base");
    QTest::addColumn<bool>("good");
    QTest::addColumn<qlonglong>("num");

    QTest::newRow("C empty 0")      << QString("")         << 0  << false << (qlonglong)0;
    QTest::newRow("C empty 8")      << QString("")         << 8  << false << (qlonglong)0;
    QTest::newRow("C empty 10")     << QString("")         << 10 << false << (qlonglong)0;
    QTest::newRow("C empty 16")     << QString("")         << 16 << false << (qlonglong)0;

    QTest::newRow("C null 0")       << QString()           << 0  << false << (qlonglong)0;
    QTest::newRow("C null 8")       << QString()           << 8  << false << (qlonglong)0;
    QTest::newRow("C null 10")      << QString()           << 10 << false << (qlonglong)0;
    QTest::newRow("C null 16")      << QString()           << 16 << false << (qlonglong)0;

    QTest::newRow("C   -0xf 0")     << QString("  -0xf")   << 0  << true  << (qlonglong)-15;
    QTest::newRow("C -0xf   0")     << QString("-0xf  ")   << 0  << true  << (qlonglong)-15;
    QTest::newRow("C \t0xf\t 0")    << QString("\t0xf\t")  << 0  << true  << (qlonglong)15;
    QTest::newRow("C   -010 0")     << QString("  -010")   << 0  << true  << (qlonglong)-8;
    QTest::newRow("C 010   0")      << QString("010  ")    << 0  << true  << (qlonglong)8;
    QTest::newRow("C \t-010\t 0")   << QString("\t-010\t") << 0  << true  << (qlonglong)-8;
    QTest::newRow("C   123 10")     << QString("  123")    << 10 << true  << (qlonglong)123;
    QTest::newRow("C 123   10")     << QString("123  ")    << 10 << true  << (qlonglong)123;
    QTest::newRow("C \t123\t 10")   << QString("\t123\t")  << 10 << true  << (qlonglong)123;
    QTest::newRow("C   -0xf 16")    << QString("  -0xf")   << 16 << true  << (qlonglong)-15;
    QTest::newRow("C -0xf   16")    << QString("-0xf  ")   << 16 << true  << (qlonglong)-15;
    QTest::newRow("C \t0xf\t 16")   << QString("\t0xf\t")  << 16 << true  << (qlonglong)15;

    QTest::newRow("C -0 0")         << QString("-0")       << 0   << true << (qlonglong)0;
    QTest::newRow("C -0 8")         << QString("-0")       << 8   << true << (qlonglong)0;
    QTest::newRow("C -0 10")        << QString("-0")       << 10  << true << (qlonglong)0;
    QTest::newRow("C -0 16")        << QString("-0")       << 16  << true << (qlonglong)0;

    QTest::newRow("C 1.234 10")     << QString("1.234")    << 10 << false << (qlonglong)0;
    QTest::newRow("C 1,234 10")     << QString("1,234")    << 10 << false << (qlonglong)0;

    QTest::newRow("C 0x 0")         << QString("0x")       << 0  << false << (qlonglong)0;
    QTest::newRow("C 0x 16")        << QString("0x")       << 16 << false << (qlonglong)0;

    QTest::newRow("C 10 0")         << QString("10")       << 0  << true  << (qlonglong)10;
    QTest::newRow("C 010 0")        << QString("010")      << 0  << true  << (qlonglong)8;
    QTest::newRow("C 0x10 0")       << QString("0x10")     << 0  << true  << (qlonglong)16;
    QTest::newRow("C 10 8")         << QString("10")       << 8  << true  << (qlonglong)8;
    QTest::newRow("C 010 8")        << QString("010")      << 8  << true  << (qlonglong)8;
    QTest::newRow("C 0x10 8")       << QString("0x10")     << 8  << false << (qlonglong)0;
    QTest::newRow("C 10 10")        << QString("10")       << 10 << true  << (qlonglong)10;
    QTest::newRow("C 010 10")       << QString("010")      << 10 << true  << (qlonglong)10;
    QTest::newRow("C 0x10 10")      << QString("0x10")     << 10 << false << (qlonglong)0;
    QTest::newRow("C 10 16")        << QString("10")       << 16 << true  << (qlonglong)16;
    QTest::newRow("C 010 16")       << QString("010")      << 16 << true  << (qlonglong)16;
    QTest::newRow("C 0x10 16")      << QString("0x10")     << 16 << true  << (qlonglong)16;

    QTest::newRow("C -10 0")        << QString("-10")      << 0  << true  << (qlonglong)-10;
    QTest::newRow("C -010 0")       << QString("-010")     << 0  << true  << (qlonglong)-8;
    QTest::newRow("C -0x10 0")      << QString("-0x10")    << 0  << true  << (qlonglong)-16;
    QTest::newRow("C -10 8")        << QString("-10")      << 8  << true  << (qlonglong)-8;
    QTest::newRow("C -010 8")       << QString("-010")     << 8  << true  << (qlonglong)-8;
    QTest::newRow("C -0x10 8")      << QString("-0x10")    << 8  << false << (qlonglong)0;
    QTest::newRow("C -10 10")       << QString("-10")      << 10 << true  << (qlonglong)-10;
    QTest::newRow("C -010 10")      << QString("-010")     << 10 << true  << (qlonglong)-10;
    QTest::newRow("C -0x10 10")     << QString("-0x10")    << 10 << false << (qlonglong)0;
    QTest::newRow("C -10 16")       << QString("-10")      << 16 << true  << (qlonglong)-16;
    QTest::newRow("C -010 16")      << QString("-010")     << 16 << true  << (qlonglong)-16;
    QTest::newRow("C -0x10 16")     << QString("-0x10")    << 16 << true  << (qlonglong)-16;

    // Let's try some Arabic
    const char16_t arabic_str[] = { 0x0661, 0x0662, 0x0663, 0x0664, 0x0000 }; // "1234"
    QTest::newRow("ar_SA 1234 0")  << QString::fromUtf16(arabic_str)  << 0  << false << (qlonglong)0;
}

void tst_QString::integer_conversion()
{
    QFETCH(QString, num_str);
    QFETCH(int, base);
    QFETCH(bool, good);
    QFETCH(qlonglong, num);

    bool ok;
    qlonglong d = num_str.toLongLong(&ok, base);
    QCOMPARE(ok, good);

    if (ok) {
        QCOMPARE(d, num);
    }
}

void tst_QString::double_conversion_data()
{
    QTest::addColumn<QString>("num_str");
    QTest::addColumn<bool>("good");
    QTest::addColumn<double>("num");

    // The good...

    QTest::newRow("C 1")             << QString("1")          << true  << 1.0;
    QTest::newRow("C 1.0")           << QString("1.0")        << true  << 1.0;
    QTest::newRow("C 1.234")         << QString("1.234")      << true  << 1.234;
    QTest::newRow("C 1.234e-10")     << QString("1.234e-10")  << true  << 1.234e-10;
    QTest::newRow("C 1.234E10")      << QString("1.234E10")   << true  << 1.234e10;
    QTest::newRow("C 1e10")          << QString("1e10")       << true  << 1.0e10;

    // The bad...

    QTest::newRow("C empty")         << QString("")           << false << 0.0;
    QTest::newRow("C null")          << QString()             << false << 0.0;
    QTest::newRow("C .")             << QString(".")          << false << 0.0;
    QTest::newRow("C 1e")            << QString("1e")         << false << 0.0;
    QTest::newRow("C 1,")            << QString("1,")         << false << 0.0;
    QTest::newRow("C 1,0")           << QString("1,0")        << false << 0.0;
    QTest::newRow("C 1,000")         << QString("1,000")      << false << 0.0;
    QTest::newRow("C 1e1.0")         << QString("1e1.0")      << false << 0.0;
    QTest::newRow("C 1e+")           << QString("1e+")        << false << 0.0;
    QTest::newRow("C 1e-")           << QString("1e-")        << false << 0.0;
    QTest::newRow("de_DE 1,0")       << QString("1,0")        << false << 0.0;
    QTest::newRow("de_DE 1,234")     << QString("1,234")      << false << 0.0;
    QTest::newRow("de_DE 1,234e-10") << QString("1,234e-10")  << false << 0.0;
    QTest::newRow("de_DE 1,234E10")  << QString("1,234E10")   << false << 0.0;

    // And the ugly...

    QTest::newRow("C .1")            << QString(".1")         << true  << 0.1;
    QTest::newRow("C -.1")           << QString("-.1")        << true  << -0.1;
    QTest::newRow("C 1.")            << QString("1.")         << true  << 1.0;
    QTest::newRow("C 1.E10")         << QString("1.E10")      << true  << 1.0e10;
    QTest::newRow("C 1e+10")         << QString("1e+10")      << true  << 1.0e+10;
    QTest::newRow("C   1")           << QString("  1")        << true  << 1.0;
    QTest::newRow("C 1  ")           << QString("1  ")        << true  << 1.0;

    // Let's try some Arabic
    const char16_t arabic_str[] = { 0x0660, 0x066B, 0x0661, 0x0662,
                                    0x0663, 0x0664, 0x0065, 0x0662,
                                    0x0000 };                            // "0.1234e2"
    QTest::newRow("ar_SA") << QString::fromUtf16(arabic_str) << false << 0.0;
}

void tst_QString::double_conversion()
{
#define MY_DOUBLE_EPSILON (2.22045e-16)

    QFETCH(QString, num_str);
    QFETCH(bool, good);
    QFETCH(double, num);

    bool ok;
    double d = num_str.toDouble(&ok);
    QCOMPARE(ok, good);

    if (ok) {
        double diff = d - num;
        if (diff < 0)
            diff = -diff;
        QVERIFY(diff <= MY_DOUBLE_EPSILON);
    }
}

#ifndef Q_MOC_RUN
#include "double_data.h"
#endif

void tst_QString::tortureSprintfDouble()
{
    const SprintfDoubleData *data = g_sprintf_double_data;

    for (; data->fmt != 0; ++data) {
        double d;
        char *buff = (char *)&d;
#        ifndef Q_BYTE_ORDER
#            error "Q_BYTE_ORDER not defined"
#        endif

#        if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        for (uint i = 0; i < 8; ++i)
            buff[i] = data->bytes[i];
#        else
        for (uint i = 0; i < 8; ++i)
            buff[7 - i] = data->bytes[i];
#        endif
        const QString s = QString::asprintf(data->fmt, d);
#ifdef QT_NO_FPU // reduced precision when running with hardfloats in qemu
        if (d - 0.1 < 1e12)
            QSKIP("clib sprintf doesn't fill with 0's on this platform");
        QCOMPARE(s.left(16), QString(data->expected).left(16));
#else
        QCOMPARE(s, QString(data->expected));
#endif
    }
}

void tst_QString::iterators()
{
    QString emptyStr;
    QCOMPARE(emptyStr.constBegin(), emptyStr.constEnd());
    QCOMPARE(emptyStr.cbegin(), emptyStr.cend());
    QVERIFY(!emptyStr.isDetached());
    QCOMPARE(emptyStr.begin(), emptyStr.end());

    QString s = "0123456789";

    auto it = s.begin();
    auto constIt = s.cbegin();
    qsizetype idx = 0;

    QCOMPARE(*it, s[idx]);
    QCOMPARE(*constIt, s[idx]);

    it++;
    constIt++;
    idx++;
    QCOMPARE(*it, s[idx]);
    QCOMPARE(*constIt, s[idx]);

    it += 5;
    constIt += 5;
    idx += 5;
    QCOMPARE(*it, s[idx]);
    QCOMPARE(*constIt, s[idx]);

    it -= 3;
    constIt -= 3;
    idx -= 3;
    QCOMPARE(*it, s[idx]);
    QCOMPARE(*constIt, s[idx]);

    it--;
    constIt--;
    idx--;
    QCOMPARE(*it, s[idx]);
    QCOMPARE(*constIt, s[idx]);
}

void tst_QString::reverseIterators()
{
    QString emptyStr;
    QCOMPARE(emptyStr.crbegin(), emptyStr.crend());
    QVERIFY(!emptyStr.isDetached());
    QCOMPARE(emptyStr.rbegin(), emptyStr.rend());

    QString s = "1234";
    QString sr = s;
    std::reverse(sr.begin(), sr.end());
    const QString &csr = sr;
    QVERIFY(std::equal(s.begin(), s.end(), sr.rbegin()));
    QVERIFY(std::equal(s.begin(), s.end(), sr.crbegin()));
    QVERIFY(std::equal(s.begin(), s.end(), csr.rbegin()));
    QVERIFY(std::equal(sr.rbegin(), sr.rend(), s.begin()));
    QVERIFY(std::equal(sr.crbegin(), sr.crend(), s.begin()));
    QVERIFY(std::equal(csr.rbegin(), csr.rend(), s.begin()));
}

void tst_QString::split_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<QString>("sep");
    QTest::addColumn<QStringList>("result");

    QTest::newRow("1") << "a,b,c" << "," << (QStringList() << "a" << "b" << "c");
    QTest::newRow("2") << QString("-rw-r--r--  1 0  0  519240 Jul  9  2002 bigfile")
                    << " "
                    << (QStringList() << "-rw-r--r--" << "" << "1" << "0" << "" << "0" << ""
                                      << "519240" << "Jul" << "" << "9" << "" << "2002" << "bigfile");
    QTest::newRow("one-empty") << "" << " " << (QStringList() << "");
    QTest::newRow("two-empty") << " " << " " << (QStringList() << "" << "");
    QTest::newRow("three-empty") << "  " << " " << (QStringList() << "" << "" << "");

    QTest::newRow("all-empty") << "" << "" << (QStringList() << "" << "");
    QTest::newRow("sep-empty") << "abc" << "" << (QStringList() << "" << "a" << "b" << "c" << "");
    QTest::newRow("null-empty") << QString() << " " << QStringList { "" };
}

template<class> struct StringSplitWrapper;
template<> struct StringSplitWrapper<QString>
{
    const QString &string;

    QStringList split(const QString &sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return string.split(sep, behavior, cs); }
    QStringList split(QChar sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return string.split(sep, behavior, cs); }
#if QT_CONFIG(regularexpression)
    QStringList split(const QRegularExpression &sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const { return string.split(sep, behavior); }
#endif
};

template<> struct StringSplitWrapper<QStringView>
{
    const QString &string;
    QList<QStringView> split(const QString &sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return QStringView{string}.split(sep, behavior, cs); }
    QList<QStringView> split(QChar sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return QStringView{string}.split(sep, behavior, cs); }
#if QT_CONFIG(regularexpression)
    QList<QStringView> split(const QRegularExpression &sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const
    { return QStringView{string}.split(sep, behavior); }
#endif
};

static bool operator==(const QList<QStringView> &result, const QStringList &expected)
{
    if (expected.size() != result.size())
        return false;
    for (int i = 0; i < expected.size(); ++i)
        if (expected.at(i) != result.at(i))
            return false;
    return true;
}

template<class List>
void tst_QString::split(const QString &string, const QString &sep, QStringList result)
{
#if QT_CONFIG(regularexpression)
    QRegularExpression re(QRegularExpression::escape(sep));
#endif

    List list;
    StringSplitWrapper<typename List::value_type> str = {string};

    list = str.split(sep);
    QVERIFY(list == result);
#if QT_CONFIG(regularexpression)
    list = str.split(re);
    QVERIFY(list == result);
#endif
    if (sep.size() == 1) {
        list = str.split(sep.at(0));
        QVERIFY(list == result);
    }

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    list = str.split(sep, Qt::KeepEmptyParts);
    QVERIFY(list == result);
#if QT_CONFIG(regularexpression)
    list = str.split(re, Qt::KeepEmptyParts);
    QVERIFY(list == result);
#endif
    if (sep.size() == 1) {
        list = str.split(sep.at(0), Qt::KeepEmptyParts);
        QVERIFY(list == result);
    }

    result.removeAll("");
    list = str.split(sep, Qt::SkipEmptyParts);
    QVERIFY(list == result);
#if QT_CONFIG(regularexpression)
    list = str.split(re, Qt::SkipEmptyParts);
    QVERIFY(list == result);
#endif
    if (sep.size() == 1) {
        list = str.split(sep.at(0), Qt::SkipEmptyParts);
        QVERIFY(list == result);
    }
QT_WARNING_POP
}

void tst_QString::split()
{
    QFETCH(QString, str);
    QFETCH(QString, sep);
    QFETCH(QStringList, result);
    split<QStringList>(str, sep, result);
    split<QList<QStringView>>(str, sep, result);
}

#if QT_CONFIG(regularexpression)
void tst_QString::split_regularexpression_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QStringList>("result");

    QTest::newRow("data01") << "Some  text\n\twith  strange whitespace."
                            << "\\s+"
                            << (QStringList() << "Some" << "text" << "with" << "strange" << "whitespace." );

    QTest::newRow("data02") << "This time, a normal English sentence."
                            << "\\W+"
                            << (QStringList() << "This" << "time" << "a" << "normal" << "English" << "sentence" << "");

    QTest::newRow("data03") << "Now: this sentence fragment."
                            << "\\b"
                            << (QStringList() << "" << "Now" << ": " << "this" << " " << "sentence" << " " << "fragment" << ".");
}

template<class List, class RegExp>
void tst_QString::split_regexp(const QString &_string, const QString &pattern, QStringList result)
{
    List list;
    StringSplitWrapper<typename List::value_type> string = {_string};

    list = string.split(RegExp(pattern));
    QVERIFY(list == result);

    result.removeAll(QString());

    list = string.split(RegExp(pattern), Qt::SkipEmptyParts);
    QVERIFY(list == result);
}

void tst_QString::split_regularexpression()
{
    QFETCH(QString, string);
    QFETCH(QString, pattern);
    QFETCH(QStringList, result);
    split_regexp<QStringList, QRegularExpression>(string, pattern, result);
    split_regexp<QList<QStringView>, QRegularExpression>(string, pattern, result);
}
#endif

void tst_QString::fromUtf16_data()
{
    QTest::addColumn<QString>("ucs2");
    QTest::addColumn<QString>("res");
    QTest::addColumn<int>("len");

    QTest::newRow("str0") << QString("abcdefgh") << QString("abcdefgh") << -1;
    QTest::newRow("str0-len") << QString("abcdefgh") << QString("abc") << 3;
}

void tst_QString::fromUtf16()
{
    QFETCH(QString, ucs2);
    QFETCH(QString, res);
    QFETCH(int, len);

    QCOMPARE(QString::fromUtf16(ucs2.utf16(), len), res);
}

void tst_QString::fromUtf16_char16()
{
    QFETCH(QString, ucs2);
    QFETCH(QString, res);
    QFETCH(int, len);

    QCOMPARE(QString::fromUtf16(reinterpret_cast<const char16_t *>(ucs2.utf16()), len), res);
}

void tst_QString::unicodeStrings()
{
    QString nullStr;
    QVERIFY(nullStr.toStdU16String().empty());
    QVERIFY(nullStr.toStdU32String().empty());
    QVERIFY(!nullStr.isDetached());

    QString emptyStr("");
    QVERIFY(emptyStr.toStdU16String().empty());
    QVERIFY(emptyStr.toStdU32String().empty());
    QVERIFY(!emptyStr.isDetached());

    QString s1, s2;
    static const std::u16string u16str1(u"Hello Unicode World");
    static const std::u32string u32str1(U"Hello Unicode World");
    s1 = QString::fromStdU16String(u16str1);
    s2 = QString::fromStdU32String(u32str1);
    QCOMPARE(s1, QString("Hello Unicode World"));
    QCOMPARE(s1, s2);

    QCOMPARE(s2.toStdU16String(), u16str1);
    QCOMPARE(s1.toStdU32String(), u32str1);

    s1 = QString::fromStdU32String(std::u32string(U"\u221212\U000020AC\U00010000"));
    QCOMPARE(s1, QString::fromUtf8("\342\210\222" "12" "\342\202\254" "\360\220\200\200"));
}

void tst_QString::latin1String()
{
    QString s("Hello");

    QVERIFY(s == QLatin1String("Hello"));
    QVERIFY(s != QLatin1String("Hello World"));
    QVERIFY(s < QLatin1String("Helloa"));
    QVERIFY(!(s > QLatin1String("Helloa")));
    QVERIFY(s > QLatin1String("Helln"));
    QVERIFY(s > QLatin1String("Hell"));
    QVERIFY(!(s < QLatin1String("Helln")));
    QVERIFY(!(s < QLatin1String("Hell")));
}

void tst_QString::nanAndInf()
{
    bool ok;
    double d;

#define CHECK_DOUBLE(str, expected_ok, expected_inf) \
    d = QString(str).toDouble(&ok); \
    QVERIFY(ok == expected_ok); \
    QVERIFY(qIsInf(d) == expected_inf);

    CHECK_DOUBLE("inf", true, true)
    CHECK_DOUBLE("INF", true, true)
    CHECK_DOUBLE("inf  ", true, true)
    CHECK_DOUBLE("+inf", true, true)
    CHECK_DOUBLE("\t +INF", true, true)
    CHECK_DOUBLE("\t INF", true, true)
    CHECK_DOUBLE("inF  ", true, true)
    CHECK_DOUBLE("+iNf", true, true)
    CHECK_DOUBLE("INFe-10", false, false)
    CHECK_DOUBLE("0xINF", false, false)
    CHECK_DOUBLE("- INF", false, false)
    CHECK_DOUBLE("+ INF", false, false)
    CHECK_DOUBLE("-- INF", false, false)
    CHECK_DOUBLE("inf0", false, false)
    CHECK_DOUBLE("--INF", false, false)
    CHECK_DOUBLE("++INF", false, false)
    CHECK_DOUBLE("INF++", false, false)
    CHECK_DOUBLE("INF--", false, false)
    CHECK_DOUBLE("INF +", false, false)
    CHECK_DOUBLE("INF -", false, false)
    CHECK_DOUBLE("0INF", false, false)
#undef CHECK_INF

#define CHECK_NAN(str, expected_ok, expected_nan) \
    d = QString(str).toDouble(&ok); \
    QVERIFY(ok == expected_ok); \
    QVERIFY(qIsNaN(d) == expected_nan);

    CHECK_NAN("nan", true, true)
    CHECK_NAN("NAN", true, true)
    CHECK_NAN("nan  ", true, true)
    CHECK_NAN("\t NAN", true, true)
    CHECK_NAN("\t NAN  ", true, true)
    CHECK_NAN("-nan", false, false)
    CHECK_NAN("+NAN", false, false)
    CHECK_NAN("NaN", true, true)
    CHECK_NAN("nAn", true, true)
    CHECK_NAN("NANe-10", false, false)
    CHECK_NAN("0xNAN", false, false)
    CHECK_NAN("0NAN", false, false)
#undef CHECK_NAN

    d = QString("-INF").toDouble(&ok);
    QVERIFY(ok);
    QVERIFY(d == -qInf());

    QString("INF").toLong(&ok);
    QVERIFY(!ok);

    QString("INF").toLong(&ok, 36);
    QVERIFY(ok);

    QString("INF0").toLong(&ok, 36);
    QVERIFY(ok);

    QString("0INF0").toLong(&ok, 36);
    QVERIFY(ok);

    // Check that inf (float) => "inf" (QString) => inf (float).
    float value = qInf();
    QString valueAsString = QString::number(value);
    QCOMPARE(valueAsString, QString::fromLatin1("inf"));
    float valueFromString = valueAsString.toFloat();
    QVERIFY(qIsInf(valueFromString));

    // Check that -inf (float) => "-inf" (QString) => -inf (float).
    value = -qInf();
    valueAsString = QString::number(value);
    QCOMPARE(valueAsString, QString::fromLatin1("-inf"));
    valueFromString = valueAsString.toFloat();
    QVERIFY(value == -qInf());
    QVERIFY(qIsInf(valueFromString));

    // Check that .arg(inf-or-nan, wide, fmt, 3, '0') padds with zeros
    QString form = QStringLiteral("%1");
    QCOMPARE(form.arg(qInf(), 5, 'f', 3, '0'), u"00inf");
    QCOMPARE(form.arg(qInf(), -5, 'f', 3, '0'), u"inf00");
    QCOMPARE(form.arg(-qInf(), 6, 'f', 3, '0'), u"00-inf");
    QCOMPARE(form.arg(-qInf(), -6, 'f', 3, '0'), u"-inf00");
    QCOMPARE(form.arg(qQNaN(), -5, 'f', 3, '0'), u"nan00");
    QCOMPARE(form.arg(qInf(), 5, 'e', 3, '0'), u"00inf");
    QCOMPARE(form.arg(qInf(), -5, 'e', 3, '0'), u"inf00");
    QCOMPARE(form.arg(-qInf(), 6, 'e', 3, '0'), u"00-inf");
    QCOMPARE(form.arg(-qInf(), -6, 'e', 3, '0'), u"-inf00");
    QCOMPARE(form.arg(qQNaN(), -5, 'e', 3, '0'), u"nan00");
    QCOMPARE(form.arg(qInf(), 5, 'E', 3, '0'), u"00INF");
    QCOMPARE(form.arg(qInf(), -5, 'E', 3, '0'), u"INF00");
    QCOMPARE(form.arg(-qInf(), 6, 'E', 3, '0'), u"00-INF");
    QCOMPARE(form.arg(-qInf(), -6, 'E', 3, '0'), u"-INF00");
    QCOMPARE(form.arg(qQNaN(), -5, 'E', 3, '0'), u"NAN00");
    QCOMPARE(form.arg(qInf(), 5, 'g', 3, '0'), u"00inf");
    QCOMPARE(form.arg(qInf(), -5, 'g', 3, '0'), u"inf00");
    QCOMPARE(form.arg(-qInf(), 6, 'g', 3, '0'), u"00-inf");
    QCOMPARE(form.arg(-qInf(), -6, 'g', 3, '0'), u"-inf00");
    QCOMPARE(form.arg(qQNaN(), -5, 'g', 3, '0'), u"nan00");
    QCOMPARE(form.arg(qInf(), 5, 'G', 3, '0'), u"00INF");
    QCOMPARE(form.arg(qInf(), -5, 'G', 3, '0'), u"INF00");
    QCOMPARE(form.arg(-qInf(), 6, 'G', 3, '0'), u"00-INF");
    QCOMPARE(form.arg(-qInf(), -6, 'G', 3, '0'), u"-INF00");
    QCOMPARE(form.arg(qQNaN(), -5, 'G', 3, '0'), u"NAN00");
}

void tst_QString::arg_fillChar_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QList<QVariant> >("replaceValues");
    QTest::addColumn<IntList>("widths");
    QTest::addColumn<QString>("fillChars");
    QTest::addColumn<QString>("expected");

    using DataList = QList<QVariant>;

    QTest::newRow("str0")
        << QStringLiteral("%1%2%3")
        << DataList{QVariant(int(5)), QVariant(QString("f")), QVariant(int(0))}
        << IntList{3, 2, 5} << QString("abc") << QString("aa5bfcccc0");

    QTest::newRow("str1")
        << QStringLiteral("%3.%1.%3.%2")
        << DataList{QVariant(int(5)), QVariant(QString("foo")), QVariant(qulonglong(INT_MAX))}
        << IntList{10, 2, 5} << QString("0 c") << QString("2147483647.0000000005.2147483647.foo");

    QTest::newRow("str2")
        << QStringLiteral("%9 og poteter")
        << DataList{QVariant(QString("fisk"))} << IntList{100} << QString("f")
        << QString("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
                   "fffffffffffffffffffffffffffffffffffffisk og poteter");

    // Left-padding with zeros fits them before the minus sign:
    QTest::newRow("zero-left")
        << QStringLiteral("%1 %2 %3 %4 %5 %6")
        << DataList{QVariant(-314.0), QVariant(-3.14e8), QVariant(-3.14e12),
                    QVariant(-.0314), QVariant(-3.14e-7), QVariant(-3.14e-13)}
        << IntList{6, 11, 11, 9, 11, 11} << QStringLiteral("000000")
        << QStringLiteral("-00314 -003.14e+08 -003.14e+12 -000.0314 -003.14e-07 -003.14e-13");

    // Right-padding with zeros is not so sensible:
    QTest::newRow("zero-right")
        << QStringLiteral("%1 %2 %3 %4 %5 %6")
        << DataList{QVariant(-314.0), QVariant(-3.14e8), QVariant(-3.14e12),
                    QVariant(-.0314), QVariant(-3.14e-7), QVariant(-3.14e-13)}
        << IntList{-6, -11, -11, -9, -11, -11} << QStringLiteral("000000")
        << QStringLiteral("-31400 -3.14e+0800 -3.14e+1200 -0.031400 -3.14e-0700 -3.14e-1300");
}

void tst_QString::arg_fillChar()
{
    static const int base = 10;
    static const char fmt = 'g';
    static const int prec = -1;

    QFETCH(QString, pattern);
    QFETCH(QList<QVariant>, replaceValues);
    QFETCH(IntList, widths);
    QFETCH(QString, fillChars);
    QFETCH(QString, expected);
    QCOMPARE(replaceValues.count(), fillChars.count());
    QCOMPARE(replaceValues.count(), widths.count());

    QString actual = pattern;
    for (int i=0; i<replaceValues.count(); ++i) {
        const QVariant &var = replaceValues.at(i);
        const int width = widths.at(i);
        const QChar fillChar = fillChars.at(i);
        switch (var.type()) {
        case QVariant::String: actual = actual.arg(var.toString(), width, fillChar); break;
        case QVariant::Int: actual = actual.arg(var.toInt(), width, base, fillChar); break;
        case QVariant::UInt: actual = actual.arg(var.toUInt(), width, base, fillChar); break;
        case QVariant::Double: actual = actual.arg(var.toDouble(), width, fmt, prec, fillChar); break;
        case QVariant::LongLong: actual = actual.arg(var.toLongLong(), width, base, fillChar); break;
        case QVariant::ULongLong: actual = actual.arg(var.toULongLong(), width, base, fillChar); break;
        default: QVERIFY(0); break;
        }
    }

    QCOMPARE(actual, expected);
}

static inline int sign(int x)
{
    return x == 0 ? 0 : (x < 0 ? -1 : 1);
}

void tst_QString::compare_data()
{
    QTest::addColumn<QString>("s1");
    QTest::addColumn<QString>("s2");
    QTest::addColumn<int>("csr"); // case sensitive result
    QTest::addColumn<int>("cir"); // case insensitive result

    // null strings
    QTest::newRow("null-null") << QString() << QString() << 0 << 0;
    QTest::newRow("text-null") << QString("a") << QString() << 1 << 1;
    QTest::newRow("null-text") << QString() << QString("a") << -1 << -1;
    QTest::newRow("null-empty") << QString() << QString("") << 0 << 0;
    QTest::newRow("empty-null") << QString("") << QString() << 0 << 0;

    // empty strings
    QTest::newRow("data0") << QString("") << QString("") << 0 << 0;
    QTest::newRow("data1") << QString("a") << QString("") << 1 << 1;
    QTest::newRow("data2") << QString("") << QString("a") << -1 << -1;

    // equal length
    QTest::newRow("data3") << QString("abc") << QString("abc") << 0 << 0;
    QTest::newRow("data4") << QString("abC") << QString("abc") << -1 << 0;
    QTest::newRow("data5") << QString("abc") << QString("abC") << 1 << 0;

    // different length
    QTest::newRow("data6") << QString("abcdef") << QString("abc") << 1 << 1;
    QTest::newRow("data7") << QString("abCdef") << QString("abc") << -1 << 1;
    QTest::newRow("data8") << QString("abc") << QString("abcdef") << -1 << -1;

    QString upper;
    upper += QChar(QChar::highSurrogate(0x10400));
    upper += QChar(QChar::lowSurrogate(0x10400));
    QString lower;
    lower += QChar(QChar::highSurrogate(0x10428));
    lower += QChar(QChar::lowSurrogate(0x10428));
    QTest::newRow("data8") << upper << lower << -1 << 0;

    // embedded nulls
    // These don't work as of now. It's OK that these don't work since \0 is not a valid unicode
    /*QTest::newRow("data10") << QString(QByteArray("\0", 1)) << QString(QByteArray("\0", 1)) << 0 << 0;
    QTest::newRow("data11") << QString(QByteArray("\0", 1)) << QString("") << 1 << 1;
    QTest::newRow("data12") << QString("") << QString(QByteArray("\0", 1)) << -1 << -1;
    QTest::newRow("data13") << QString("ab\0c") << QString(QByteArray("ab\0c", 4)) << 0 << 0;
    QTest::newRow("data14") << QString(QByteArray("ab\0c", 4)) << QString("abc") << -1 << -1;
    QTest::newRow("data15") << QString("abc") << QString(QByteArray("ab\0c", 4)) << 1 << 1;*/

    // All tests below (generated by the 3 for-loops) are meant to excercise the vectorized versions
    // of ucstrncmp.

    QString in1, in2;
    for (int i = 0; i < 70; ++i) {
        in1 += QString::number(i % 10);
        in2 += QString::number((70 - i + 1) % 10);
    }
    Q_ASSERT(in1.length() == in2.length());
    Q_ASSERT(in1 != in2);
    Q_ASSERT(in1.at(0) < in2.at(0));
    for (int i = 0; i < in1.length(); ++i) {
        Q_ASSERT(in1.at(i) != in2.at(i));
    }

    for (int i = 1; i <= 65; ++i) {
        QString inp1 = in1.left(i);
        QString inp2 = in2.left(i);
        QTest::addRow("all-different-%d", i) << inp1 << inp2 << -1 << -1;
    }

    for (int i = 1; i <= 65; ++i) {
        QString start(i - 1, 'a');

        QString in = start + QLatin1Char('a');
        QTest::addRow("all-same-%d", i) << in << in << 0 << 0;

        QString in2 = start + QLatin1Char('b');
        QTest::addRow("last-different-%d", i) << in << in2 << -1 << -1;
    }

    for (int i = 0; i < 16; ++i) {
        QString in1(16, 'a');
        QString in2 = in1;
        in2[i] = 'b';
        QTest::addRow("all-same-except-char-%d", i) << in1 << in2 << -1 << -1;
    }

    // some non-US-ASCII comparisons
    QChar smallA = u'a';
    QChar smallAWithAcute = u'á';
    QChar capitalAWithAcute = u'Á';
    QChar nbsp = u'\u00a0';
    for (int i = 1; i <= 65; ++i) {
        QString padding(i - 1, ' ');
        QTest::addRow("ascii-nonascii-%d", i)
                << (padding + smallA) << (padding + smallAWithAcute) << -1 << -1;
        QTest::addRow("nonascii-nonascii-equal-%d", i)
                << (padding + smallAWithAcute) << (padding + smallAWithAcute) << 0 << 0;
        QTest::addRow("nonascii-nonascii-caseequal-%d", i)
                << (padding + capitalAWithAcute) << (padding + smallAWithAcute) << -1 << 0;
        QTest::addRow("nonascii-nonascii-notequal-%d", i)
                << (padding + nbsp) << (padding + smallAWithAcute) << -1 << -1;
    }
}

static bool isLatin(const QString &s)
{
    for (int i = 0; i < s.length(); ++i)
        if (s.at(i).unicode() > 0xff)
            return false;
    return true;
}

void tst_QString::compare()
{
    QFETCH(QString, s1);
    QFETCH(QString, s2);
    QFETCH(int, csr);
    QFETCH(int, cir);

    QByteArray s1_8 = s1.toUtf8();
    QByteArray s2_8 = s2.toUtf8();

    const QStringView v1(s1);
    const QStringView v2(s2);

    QCOMPARE(sign(QString::compare(s1, s2)), csr);
    QCOMPARE(sign(s1.compare(s2)), csr);
    QCOMPARE(sign(s1.compare(v2)), csr);

    QCOMPARE(sign(s1.compare(s2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(s1.compare(s2, Qt::CaseInsensitive)), cir);
    QCOMPARE(sign(s1.compare(v2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(s1.compare(v2, Qt::CaseInsensitive)), cir);
    QCOMPARE(sign(QtPrivate::compareStrings(QUtf8StringView(s1_8), v2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(QtPrivate::compareStrings(QUtf8StringView(s1_8), v2, Qt::CaseInsensitive)), cir);
    QCOMPARE(sign(QtPrivate::compareStrings(QUtf8StringView(s2_8), v1, Qt::CaseSensitive)), -csr);
    QCOMPARE(sign(QtPrivate::compareStrings(QUtf8StringView(s2_8), v1, Qt::CaseInsensitive)), -cir);

    QCOMPARE(sign(QString::compare(s1, s2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(QString::compare(s1, s2, Qt::CaseInsensitive)), cir);

    if (csr == 0)
        QVERIFY(qHash(s1) == qHash(s2));

    if (!cir)
        QCOMPARE(s1.toCaseFolded(), s2.toCaseFolded());

    if (isLatin(s2)) {
        QVERIFY(QtPrivate::isLatin1(s2));
        QCOMPARE(sign(QString::compare(s1, QLatin1String(s2.toLatin1()))), csr);
        QCOMPARE(sign(QString::compare(s1, QLatin1String(s2.toLatin1()), Qt::CaseInsensitive)), cir);
        QByteArray l1 = s2.toLatin1();
        l1 += "x";
        QLatin1String l1str(l1.constData(), l1.size() - 1);
        QCOMPARE(sign(QString::compare(s1, l1str)), csr);
        QCOMPARE(sign(QString::compare(s1, l1str, Qt::CaseInsensitive)), cir);
    }

    if (isLatin(s1)) {
        QVERIFY(QtPrivate::isLatin1(s1));
        QCOMPARE(sign(QString::compare(QLatin1String(s1.toLatin1()), s2)), csr);
        QCOMPARE(sign(QString::compare(QLatin1String(s1.toLatin1()), s2, Qt::CaseInsensitive)), cir);
    }
}

void tst_QString::resize()
{
    QString s;
    s.resize(11, ' ');
    QCOMPARE(s.size(), 11);
    QCOMPARE(s, QLatin1String("           "));

    s = QLatin1String("hello world");

    s.resize(5);
    QCOMPARE(s, QLatin1String("hello"));
    s.resize(8);
    QCOMPARE(s.size(), 8);
    QVERIFY(s.startsWith(QLatin1String("hello")));

    s.resize(10, QLatin1Char('n'));
    QCOMPARE(s.size(), 10);
    QVERIFY(s.startsWith(QLatin1String("hello")));
    QCOMPARE(s.right(2), QLatin1String("nn"));
}

void tst_QString::resizeAfterFromRawData()
{
    QString buffer("hello world");

    QString array = QString::fromRawData(buffer.constData(), buffer.size());
    QVERIFY(array.constData() == buffer.constData());
    array.resize(5);
    QVERIFY(array.constData() != buffer.constData());
    // check null termination
    QVERIFY(array.constData()[5] == 0);
}

void tst_QString::resizeAfterReserve()
{

    QString s;
    s.reserve(100);

    s += "hello world";

    // resize should not affect capacity
    s.resize(s.size());
    QVERIFY(s.capacity() == 100);

    // but squeeze does
    s.squeeze();
    QVERIFY(s.capacity() == s.size());

    // clear does too
    s.clear();
    QVERIFY(s.capacity() == 0);

    // test resize(0) border case
    s.reserve(100);
    s += "hello world";
    s.resize(0);
    QVERIFY(s.capacity() == 100);

    // reserve() can't be used to truncate data
    s.fill('x', 100);
    s.reserve(50);
    QVERIFY(s.capacity() == 100);
    QVERIFY(s.size() == 100);

    // even with increased ref count truncation isn't allowed
    QString t = s;
    s.reserve(50);
    QVERIFY(s.capacity() == 100);
    QVERIFY(s.size() == 100);
}

void tst_QString::resizeWithNegative() const
{
    {
        QString string(QLatin1String("input"));
        string.resize(-1);
        QCOMPARE(string, QString());
    }

    {
        QString string(QLatin1String("input"));
        string.resize(-9099);
        QCOMPARE(string, QString());
    }

    {
        /* Example code from customer. */
        QString s(QLatin1String("hola"));
        s.reserve(1);
        s.resize(-1);
        QCOMPARE(s, QString());
    }
}

void tst_QString::truncateWithNegative() const
{
    {
        QString string(QLatin1String("input"));
        string.truncate(-1);
        QCOMPARE(string, QString());
    }

    {
        QString string(QLatin1String("input"));
        string.truncate(-9099);
        QCOMPARE(string, QString());
    }
}

void tst_QString::QCharRefMutableUnicode() const
{
    QString str;
    str.resize(3);
    str[0].unicode() = 115;
    str[1].unicode() = 116;
    str[2].unicode() = 114;

    QCOMPARE(str, QString::fromLatin1("str"));
}

void tst_QString::QCharRefDetaching() const
{
    {
        QString str = QString::fromLatin1("str");
        QString copy = str;
        copy[0] = QLatin1Char('S');

        QCOMPARE(str, QString::fromLatin1("str"));
    }

    {
        ushort buf[] = { 's', 't', 'r' };
        QString str = QString::fromRawData((const QChar *)buf, 3);
        str[0] = QLatin1Char('S');

        QCOMPARE(buf[0], ushort('s'));
    }

    {
        static const ushort buf[] = { 's', 't', 'r' };
        QString str = QString::fromRawData((const QChar *)buf, 3);

        // this causes a crash in most systems if the detaching doesn't work
        str[0] = QLatin1Char('S');

        QCOMPARE(buf[0], ushort('s'));
    }
}

void tst_QString::repeatedSignature() const
{
    /* repated() should be a const member. */
    const QString string;
    (void) string.repeated(3);
}

void tst_QString::repeated() const
{
    QFETCH(QString, string);
    QFETCH(QString, expected);
    QFETCH(int, count);

    QCOMPARE(string.repeated(count), expected);
}

void tst_QString::repeated_data() const
{
    QTest::addColumn<QString>("string" );
    QTest::addColumn<QString>("expected" );
    QTest::addColumn<int>("count" );

    /* Empty strings. */
    QTest::newRow("data1")
        << QString()
        << QString()
        << 0;

    QTest::newRow("data2")
        << QString()
        << QString()
        << -1004;

    QTest::newRow("data3")
        << QString()
        << QString()
        << 1;

    QTest::newRow("data4")
        << QString()
        << QString()
        << 5;

    /* On simple string. */
    QTest::newRow("data5")
        << QString(QLatin1String("abc"))
        << QString()
        << -1004;

    QTest::newRow("data6")
        << QString(QLatin1String("abc"))
        << QString()
        << -1;

    QTest::newRow("data7")
        << QString(QLatin1String("abc"))
        << QString()
        << 0;

    QTest::newRow("data8")
        << QString(QLatin1String("abc"))
        << QString(QLatin1String("abc"))
        << 1;

    QTest::newRow("data9")
        << QString(QLatin1String("abc"))
        << QString(QLatin1String("abcabc"))
        << 2;

    QTest::newRow("data10")
        << QString(QLatin1String("abc"))
        << QString(QLatin1String("abcabcabc"))
        << 3;

    QTest::newRow("data11")
        << QString(QLatin1String("abc"))
        << QString(QLatin1String("abcabcabcabc"))
        << 4;
}

void tst_QString::arg_locale()
{
    QLocale l(QLocale::English, QLocale::UnitedKingdom);
    QString str("*%L1*%L2*");

    TransientDefaultLocale transient(l);
    QCOMPARE(str.arg(123456).arg(1234.56), QString::fromLatin1("*123,456*1,234.56*"));

    l.setNumberOptions(QLocale::OmitGroupSeparator);
    transient.revise(l);
    QCOMPARE(str.arg(123456).arg(1234.56), QString::fromLatin1("*123456*1234.56*"));

    transient.revise(QLocale::C);
    QCOMPARE(str.arg(123456).arg(1234.56), QString::fromLatin1("*123456*1234.56*"));
}


#if QT_CONFIG(icu)
// Qt has to be built with ICU support
void tst_QString::toUpperLower_icu()
{
    QString s = QString::fromLatin1("i");

    QCOMPARE(s.toUpper(), QString::fromLatin1("I"));
    QCOMPARE(s.toLower(), QString::fromLatin1("i"));

    TransientDefaultLocale transient(QLocale(QLocale::Turkish, QLocale::Turkey));

    QCOMPARE(s.toUpper(), QString::fromLatin1("I"));
    QCOMPARE(s.toLower(), QString::fromLatin1("i"));

    // turkish locale has a capital I with a dot (U+0130, utf8 c4b0)
    QLocale l;

    QCOMPARE(l.toUpper(s), QString::fromUtf8("\xc4\xb0"));
    QCOMPARE(l.toLower(QString::fromUtf8("\xc4\xb0")), s);

    // nothing should happen here
    QCOMPARE(l.toLower(s), s);
    QCOMPARE(l.toUpper(QString::fromLatin1("I")), QString::fromLatin1("I"));

    // U+0131, utf8 c4b1 is the lower-case i without a dot
    QString sup = QString::fromUtf8("\xc4\xb1");

    QCOMPARE(l.toUpper(sup), QString::fromLatin1("I"));
    QCOMPARE(l.toLower(QString::fromLatin1("I")), sup);

    // nothing should happen here
    QCOMPARE(l.toLower(sup), sup);
    QCOMPARE(l.toLower(QString::fromLatin1("i")), QString::fromLatin1("i"));
}
#endif // icu

void tst_QString::literals()
{
    QString str(QStringLiteral("abcd"));

    QVERIFY(str.length() == 4);
    QCOMPARE(str.capacity(), 0);
    QVERIFY(str == QLatin1String("abcd"));
    QVERIFY(!str.data_ptr()->isMutable());

    const QChar *s = str.constData();
    QString str2 = str;
    QVERIFY(str2.constData() == s);
    QCOMPARE(str2.capacity(), 0);

    // detach on non const access
    QVERIFY(str.data() != s);
    QVERIFY(str.capacity() >= str.length());

    QVERIFY(str2.constData() == s);
    QVERIFY(str2.data() != s);
    QVERIFY(str2.capacity() >= str2.length());
}

void tst_QString::userDefinedLiterals()
{
    QString str = u"abcd"_qs;

    QVERIFY(str.length() == 4);
    QCOMPARE(str.capacity(), 0);
    QVERIFY(str == QLatin1String("abcd"));
    QVERIFY(!str.data_ptr()->isMutable());

    const QChar *s = str.constData();
    QString str2 = str;
    QVERIFY(str2.constData() == s);
    QCOMPARE(str2.capacity(), 0);

    // detach on non const access
    QVERIFY(str.data() != s);
    QVERIFY(str.capacity() >= str.length());

    QVERIFY(str2.constData() == s);
    QVERIFY(str2.data() != s);
    QVERIFY(str2.capacity() >= str2.length());
}

void tst_QString::eightBitLiterals_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("stringData");

    QTest::newRow("null") << QByteArray() << QString();
    QTest::newRow("empty") << QByteArray("") << QString("");
    QTest::newRow("regular") << QByteArray("foo") << "foo";
    QTest::newRow("non-ascii") << QByteArray("\xc3\xa9") << QString::fromLatin1("\xe9");
}

void tst_QString::eightBitLiterals()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, stringData);

    {
        QString s(data);
        QCOMPARE(s, stringData);
    }
    {
        QString s(data.constData());
        QCOMPARE(s, stringData);
    }
    {
        QString s;
        s = data;
        QCOMPARE(s, stringData);
    }
    {
        QString s;
        s = data.constData();
        QCOMPARE(s, stringData);
    }
    {
        QString s;
        s.append(data);
        QCOMPARE(s, stringData);
    }
    {
        QString s;
        s.append(data.constData());
        QCOMPARE(s, stringData);
    }
    {
        QString s;
        s += data;
        QCOMPARE(s, stringData);
    }
    {
        QString s;
        s += data.constData();
        QCOMPARE(s, stringData);
    }
    {
        QString s;
        s.prepend(data);
        QCOMPARE(s, stringData);
    }
    {
        QString s;
        s.prepend(data.constData());
        QCOMPARE(s, stringData);
    }
    {
        QString s = QString() + data;
        QCOMPARE(s, stringData);
    }
    {
        QString s = QString() + data.constData();
        QCOMPARE(s, stringData);
    }
    {
        QString s = data + QString();
        QCOMPARE(s, stringData);
    }
    {
        QString s = QString() % data;
        QCOMPARE(s, stringData);
    }
    {
        QString s = QString() % data.constData();
        QCOMPARE(s, stringData);
    }
    {
        QString s = data % QString();
        QCOMPARE(s, stringData);
    }

    {
        QVERIFY(stringData == data);
        QVERIFY(stringData == data.constData());
        QVERIFY(!(stringData != data));
        QVERIFY(!(stringData != data.constData()));
        QVERIFY(!(stringData < data));
        QVERIFY(!(stringData < data.constData()));
        QVERIFY(!(stringData > data));
        QVERIFY(!(stringData > data.constData()));
        QVERIFY(stringData <= data);
        QVERIFY(stringData <= data.constData());
        QVERIFY(stringData >= data);
        QVERIFY(stringData >= data.constData());
    }
}

void tst_QString::reserve()
{
    QString nil1, nil2;
    nil1.reserve(0);
    nil2.squeeze();
    nil1.squeeze();
    nil2.reserve(0);
    QVERIFY(!nil1.isDetached());
    QVERIFY(!nil2.isDetached());
}

void tst_QString::toHtmlEscaped_data()
{
    QTest::addColumn<QString>("original");
    QTest::addColumn<QString>("expected");

    QTest::newRow("null") << QString() << QString();
    QTest::newRow("empty") << QString("") << QString("");
    QTest::newRow("1") << "Hello World\n" << "Hello World\n";
    QTest::newRow("2") << "#include <QtCore>" << "#include &lt;QtCore&gt;";
    QTest::newRow("3") << "<p class=\"cool\"><a href=\"http://example.com/?foo=bar&amp;bar=foo\">plop --&gt; </a></p>"
                       << "&lt;p class=&quot;cool&quot;&gt;&lt;a href=&quot;http://example.com/?foo=bar&amp;amp;bar=foo&quot;&gt;plop --&amp;gt; &lt;/a&gt;&lt;/p&gt;";
    QTest::newRow("4") << QString::fromUtf8("<\320\222\321\201>") << QString::fromUtf8("&lt;\320\222\321\201&gt;");
}

void tst_QString::toHtmlEscaped()
{
    QFETCH(QString, original);
    QFETCH(QString, expected);

    QCOMPARE(original.toHtmlEscaped(), expected);
}

void tst_QString::operatorGreaterWithQLatin1String()
{
    QLatin1String latin1foo("fooZZ", 3);
    QString stringfoo = QString::fromLatin1("foo");
    QVERIFY(stringfoo >= latin1foo);
    QVERIFY(!(stringfoo > latin1foo));
    QVERIFY(stringfoo <= latin1foo);
    QVERIFY(!(stringfoo < latin1foo));
}

void tst_QString::compareQLatin1Strings()
{
    QLatin1String abc("abc");
    QLatin1String abcd("abcd");
    QLatin1String cba("cba");
    QLatin1String de("de");

    QVERIFY(abc == abc);
    QVERIFY(!(abc == cba));
    QVERIFY(!(cba == abc));
    QVERIFY(!(abc == abcd));
    QVERIFY(!(abcd == abc));

    QVERIFY(abc != cba);
    QVERIFY(!(abc != abc));
    QVERIFY(cba != abc);
    QVERIFY(abc != abcd);
    QVERIFY(abcd != abc);

    QVERIFY(abc < abcd);
    QVERIFY(abc < cba);
    QVERIFY(abc < de);
    QVERIFY(abcd < cba);
    QVERIFY(!(abc < abc));
    QVERIFY(!(abcd < abc));
    QVERIFY(!(de < cba));

    QVERIFY(abcd > abc);
    QVERIFY(cba > abc);
    QVERIFY(de > abc);
    QVERIFY(!(abc > abc));
    QVERIFY(!(abc > abcd));
    QVERIFY(!(abcd > cba));

    QVERIFY(abc <= abc);
    QVERIFY(abc <= abcd);
    QVERIFY(abc <= cba);
    QVERIFY(abc <= de);
    QVERIFY(!(abcd <= abc));
    QVERIFY(!(cba <= abc));
    QVERIFY(!(cba <= abcd));
    QVERIFY(!(de <= abc));

    QVERIFY(abc >= abc);
    QVERIFY(abcd >= abc);
    QVERIFY(!(abc >= abcd));
    QVERIFY(cba >= abc);
    QVERIFY(!(abc >= cba));
    QVERIFY(de >= abc);
    QVERIFY(!(abc >= de));

    QLatin1String subfoo("fooZZ", 3);
    QLatin1String foo("foo");
    QVERIFY(subfoo == foo);
    QVERIFY(foo == subfoo);
    QVERIFY(!(subfoo != foo));
    QVERIFY(!(foo != subfoo));
    QVERIFY(!(foo < subfoo));
    QVERIFY(!(subfoo < foo));
    QVERIFY(foo >= subfoo);
    QVERIFY(subfoo >= foo);
    QVERIFY(!(foo > subfoo));
    QVERIFY(!(subfoo > foo));
    QVERIFY(foo <= subfoo);
    QVERIFY(subfoo <= foo);

    QLatin1String subabc("abcZZ", 3);
    QLatin1String subab("abcZZ", 2);
    QVERIFY(subabc != subab);
    QVERIFY(subab != subabc);
    QVERIFY(!(subabc == subab));
    QVERIFY(!(subab == subabc));
    QVERIFY(subab < subabc);
    QVERIFY(!(subabc < subab));
    QVERIFY(subabc > subab);
    QVERIFY(!(subab > subabc));
    QVERIFY(subab <= subabc);
    QVERIFY(!(subabc <= subab));
    QVERIFY(subabc >= subab);
    QVERIFY(!(subab >= subabc));
}

void tst_QString::fromQLatin1StringWithLength()
{
    QLatin1String latin1foo("foobar", 3);
    QString foo(latin1foo);
    QCOMPARE(foo.size(), latin1foo.size());
    QCOMPARE(foo, QString::fromLatin1("foo"));
}

void tst_QString::assignQLatin1String()
{
    QString empty = QLatin1String("");
    QVERIFY(empty.isEmpty());
    QVERIFY(!empty.isNull());

    QString null = QLatin1String(0);
    QVERIFY(null.isEmpty());
    QVERIFY(null.isNull());

    QLatin1String latin1foo("foo");
    QString foo = latin1foo;
    QCOMPARE(foo.size(), latin1foo.size());
    QCOMPARE(foo, QString::fromLatin1("foo"));

    QLatin1String latin1subfoo("foobar", 3);
    foo = latin1subfoo;
    QCOMPARE(foo.size(), latin1subfoo.size());
    QCOMPARE(foo, QString::fromLatin1("foo"));

    // check capacity re-use:
    QString s;
    QCOMPARE(s.capacity(), 0);

    // assign to null QString:
    s = latin1foo;
    QCOMPARE(s, QString::fromLatin1("foo"));
    QCOMPARE(s.capacity(), 3);

    // assign to non-null QString with enough capacity:
    s = QString::fromLatin1("foofoo");
    const int capacity = s.capacity();
    s = latin1foo;
    QCOMPARE(s, QString::fromLatin1("foo"));
    QCOMPARE(s.capacity(), capacity);

    // assign to shared QString (enough capacity, but can't use):
    s = QString::fromLatin1("foofoo");
    QString s2 = s;
    s = latin1foo;
    QCOMPARE(s, QString::fromLatin1("foo"));
    QCOMPARE(s.capacity(), 3);

    // assign to QString with too little capacity:
    s = QString::fromLatin1("fo");
    QCOMPARE(s.capacity(), 2);
    s = latin1foo;
    QCOMPARE(s, QString::fromLatin1("foo"));
    QCOMPARE(s.capacity(), 3);

}

void tst_QString::assignQChar()
{
    const QChar sp = QLatin1Char(' ');
    QString s;
    QCOMPARE(s.capacity(), 0);

    // assign to null QString:
    s = sp;
    QCOMPARE(s, QString(sp));
    QCOMPARE(s.capacity(), 1);

    // assign to non-null QString with enough capacity:
    s = QLatin1String("foo");
    const int capacity = s.capacity();
    QCOMPARE(capacity, 3);
    s = sp;
    QCOMPARE(s, QString(sp));
    QCOMPARE(s.capacity(), capacity);

    // assign to shared QString (enough capacity, but can't use):
    s = QLatin1String("foo");
    QString s2 = s;
    s = sp;
    QCOMPARE(s, QString(sp));
    QCOMPARE(s.capacity(), 1);

    // assign to empty QString:
    s = QString("");
    s.detach();
    QCOMPARE(s.capacity(), 0);
    s = sp;
    QCOMPARE(s, QString(sp));
    QCOMPARE(s.capacity(), 1);
}

void tst_QString::isRightToLeft_data()
{
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<bool>("rtl");

    QTest::newRow("null") << QString() << false;
    QTest::newRow("empty") << QString("") << false;

    QTest::newRow("numbers-only") << QString("12345") << false;
    QTest::newRow("latin1-only") << QString("hello") << false;
    QTest::newRow("numbers-latin1") << (QString("12345") + QString("hello")) << false;

    static const char16_t unicode1[] = { 0x627, 0x627 };
    QTest::newRow("arabic-only") << QString::fromUtf16(unicode1, 2) << true;
    QTest::newRow("numbers-arabic") << (QString("12345") + QString::fromUtf16(unicode1, 2)) << true;
    QTest::newRow("numbers-latin1-arabic") << (QString("12345") + QString("hello") + QString::fromUtf16(unicode1, 2)) << false;
    QTest::newRow("numbers-arabic-latin1") << (QString("12345") + QString::fromUtf16(unicode1, 2) + QString("hello")) << true;

    static const char16_t unicode2[] = { QChar::highSurrogate(0xE01DAu), QChar::lowSurrogate(0xE01DAu), QChar::highSurrogate(0x2F800u), QChar::lowSurrogate(0x2F800u) };
    QTest::newRow("surrogates-VS-CJK") << QString::fromUtf16(unicode2, 4) << false;

    static const char16_t unicode3[] = { QChar::highSurrogate(0x10800u), QChar::lowSurrogate(0x10800u), QChar::highSurrogate(0x10805u), QChar::lowSurrogate(0x10805u) };
    QTest::newRow("surrogates-cypriot") << QString::fromUtf16(unicode3, 4) << true;

    QTest::newRow("lre") << (QString("12345") + QChar(0x202a) + QString("9") + QChar(0x202c)) << false;
    QTest::newRow("rle") << (QString("12345") + QChar(0x202b) + QString("9") + QChar(0x202c)) << false;
    QTest::newRow("r in lre") << (QString("12345") + QChar(0x202a) + QString::fromUtf16(unicode1, 2) + QChar(0x202c) + QString("a")) << true;
    QTest::newRow("l in lre") << (QString("12345") + QChar(0x202a) + QString("a") + QChar(0x202c) + QString::fromUtf16(unicode1, 2)) << false;
    QTest::newRow("r in rle") << (QString("12345") + QChar(0x202b) + QString::fromUtf16(unicode1, 2) + QChar(0x202c) + QString("a")) << true;
    QTest::newRow("l in rle") << (QString("12345") + QChar(0x202b) + QString("a") + QChar(0x202c) + QString::fromUtf16(unicode1, 2)) << false;

    QTest::newRow("lro") << (QString("12345") + QChar(0x202d) + QString("9") + QChar(0x202c)) << false;
    QTest::newRow("rlo") << (QString("12345") + QChar(0x202e) + QString("9") + QChar(0x202c)) << false;
    QTest::newRow("r in lro") << (QString("12345") + QChar(0x202d) + QString::fromUtf16(unicode1, 2) + QChar(0x202c) + QString("a")) << true;
    QTest::newRow("l in lro") << (QString("12345") + QChar(0x202d) + QString("a") + QChar(0x202c) + QString::fromUtf16(unicode1, 2)) << false;
    QTest::newRow("r in rlo") << (QString("12345") + QChar(0x202e) + QString::fromUtf16(unicode1, 2) + QChar(0x202c) + QString("a")) << true;
    QTest::newRow("l in rlo") << (QString("12345") + QChar(0x202e) + QString("a") + QChar(0x202c) + QString::fromUtf16(unicode1, 2)) << false;

    QTest::newRow("lri") << (QString("12345") + QChar(0x2066) + QString("a") + QChar(0x2069) + QString::fromUtf16(unicode1, 2)) << true;
    QTest::newRow("rli") << (QString("12345") + QChar(0x2067) + QString::fromUtf16(unicode1, 2) + QChar(0x2069) + QString("a")) << false;
    QTest::newRow("fsi1") << (QString("12345") + QChar(0x2068) + QString("a") + QChar(0x2069) + QString::fromUtf16(unicode1, 2)) << true;
    QTest::newRow("fsi2") << (QString("12345") + QChar(0x2068) + QString::fromUtf16(unicode1, 2) + QChar(0x2069) + QString("a")) << false;
}

void tst_QString::isRightToLeft()
{
    QFETCH(QString, unicode);
    QFETCH(bool, rtl);

    QCOMPARE(unicode.isRightToLeft(), rtl);
}

void tst_QString::isValidUtf16_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<bool>("valid");

    int row = 0;
    QTest::addRow("valid-%02d", row++) << QString() << true;
    QTest::addRow("valid-%02d", row++) << QString("") << true;
    QTest::addRow("valid-%02d", row++) << QString("abc def") << true;
    QTest::addRow("valid-%02d", row++) << QString("àbç") << true;
    QTest::addRow("valid-%02d", row++) << QString("ßẞ") << true;
    QTest::addRow("valid-%02d", row++) << QString("𝐀𝐁𝐂abc𝐃𝐄𝐅def") << true;
    QTest::addRow("valid-%02d", row++) << QString("abc𝐀𝐁𝐂def𝐃𝐄𝐅") << true;
    QTest::addRow("valid-%02d", row++) << (QString("abc") + QChar(0x0000) + QString("def")) << true;
    QTest::addRow("valid-%02d", row++) << (QString("abc") + QChar(0xFFFF) + QString("def")) << true;
    // check that BOM presence doesn't make any difference
    QTest::addRow("valid-%02d", row++) << (QString() + QChar(0xFEFF) + QString("abc𝐀𝐁𝐂def𝐃𝐄𝐅")) << true;
    QTest::addRow("valid-%02d", row++) << (QString() + QChar(0xFFFE) + QString("abc𝐀𝐁𝐂def𝐃𝐄𝐅")) << true;

    row = 0;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QChar(0xD800)) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QString("abc") + QChar(0xD800)) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QChar(0xD800) + QString("def")) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QString("abc") + QChar(0xD800) + QString("def")) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QChar(0xD800) + QChar(0xD800)) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QString("abc") + QChar(0xD800) + QChar(0xD800)) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QChar(0xD800) + QChar(0xD800) + QString("def")) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QString("abc") + QChar(0xD800) + QChar(0xD800) + QString("def")) << false;

    row = 0;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QChar(0xDC00)) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QString("abc") + QChar(0xDC00)) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QChar(0xDC00) + QString("def")) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QString("abc") + QChar(0xDC00) + QString("def")) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QChar(0xDC00) + QChar(0xDC00)) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QString("abc") + QChar(0xDC00) + QChar(0xDC00)) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QChar(0xDC00) + QChar(0xDC00) + QString("def")) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QString("abc") + QChar(0xDC00) + QChar(0xDC00) + QString("def")) << false;
}

void tst_QString::isValidUtf16()
{
    QFETCH(QString, string);
    QTEST(string.isValidUtf16(), "valid");
}

static QString doVasprintf(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    const QString result = QString::vasprintf(msg, args);
    va_end(args);
    return result;
}

void tst_QString::vasprintfWithPrecision()
{
    {
        const char *msg = "Endpoint %.*s with";
        static const char arg0[3] = { 'a', 'b', 'c' };
        static const char arg1[4] = { 'a', 'b', 'c', '\0' };
        QCOMPARE(doVasprintf(msg, 3, arg0), QStringLiteral("Endpoint abc with"));
        QCOMPARE(doVasprintf(msg, 9, arg1), QStringLiteral("Endpoint abc with"));
        QCOMPARE(doVasprintf(msg, 0, nullptr), QStringLiteral("Endpoint  with"));
    }

    {
        const char *msg = "Endpoint %.*ls with";
        static const ushort arg0[3] = { 'a', 'b', 'c' };
        static const ushort arg1[4] = { 'a', 'b', 'c', '\0' };
        QCOMPARE(doVasprintf(msg, 3, arg0), QStringLiteral("Endpoint abc with"));
        QCOMPARE(doVasprintf(msg, 9, arg1), QStringLiteral("Endpoint abc with"));
        QCOMPARE(doVasprintf(msg, 0, nullptr), QStringLiteral("Endpoint  with"));
    }
}

void tst_QString::rawData()
{
    QString s;
    // it can be nullptr or a pointer to static shared data
    const QChar *constPtr = s.constData();
    QCOMPARE(s.unicode(), constPtr); // unicode() is the same as constData()
    QCOMPARE(s.data(), constPtr); // does not detach() on empty string
    QCOMPARE(s.utf16(), reinterpret_cast<const ushort *>(constPtr));
    QVERIFY(!s.isDetached());

    s = "abc"; // detached
    const QChar *dataConstPtr = s.constData();
    QVERIFY(dataConstPtr != constPtr);

    const ushort *utf16Ptr = s.utf16();

    QString s1 = s;
    QCOMPARE(s1.constData(), dataConstPtr);
    QCOMPARE(s1.unicode(), s.unicode());

    QChar *s1Ptr = s1.data(); // detaches here, because the string is not empty
    QVERIFY(s1Ptr != dataConstPtr);
    QVERIFY(s1.unicode() != s.unicode());

    *s1Ptr = 'd';
    QCOMPARE(s1, "dbc");

    // utf pointer is valid while the string is not changed
    QCOMPARE(QString::fromUtf16(utf16Ptr), s);
}

void tst_QString::clear()
{
    QString s;
    s.clear();
    QVERIFY(s.isEmpty());
    QVERIFY(!s.isDetached());

    s = "some tests string";
    QVERIFY(!s.isEmpty());

    s.clear();
    QVERIFY(s.isEmpty());
}

void tst_QString::sliced()
{
    QString a;

    QVERIFY(a.sliced(0).isEmpty());
    QVERIFY(a.sliced(0, 0).isEmpty());
    QVERIFY(!a.isDetached());

    a = "ABCDEFGHIEfGEFG"; // 15 chars

    QCOMPARE(a.sliced(5), u"FGHIEfGEFG");
    QCOMPARE(a.sliced(5, 3), u"FGH");
}

void tst_QString::chopped()
{
    QString a;

    QVERIFY(a.chopped(0).isEmpty());
    QVERIFY(!a.isDetached());

    a = "ABCDEFGHIEfGEFG"; // 15 chars

    QCOMPARE(a.chopped(10), u"ABCDE");
}

void tst_QString::removeIf()
{
    QString a;

    auto pred = [](const QChar &c) { return c.isLower(); };

    a.removeIf(pred);
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    a = "aABbcCDd";
    a.removeIf(pred);
    QCOMPARE(a, u"ABCD");
}

// QString's collation order is only supported during the lifetime as QCoreApplication
QTEST_GUILESS_MAIN(tst_QString)

#include "tst_qstring.moc"
