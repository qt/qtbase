// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifdef QT_NO_CAST_TO_ASCII
# undef QT_NO_CAST_TO_ASCII
#endif
#ifdef QT_ASCII_CAST_WARNINGS
# undef QT_ASCII_CAST_WARNINGS
#endif

#include <private/qglobal_p.h> // for the icu feature test
#include <QtTest/private/qcomparisontesthelper_p.h>
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
#include <QtCore/qxptype_traits.h>

#include <locale.h>
#include <qhash.h>
#include <private/qtools_p.h>

#include <forward_list>
#include <string>
#include <algorithm>
#include <limits>
#include <sstream>

#include "../shared/test_number_shared.h"
#include "../../../../shared/localechange.h"

using namespace Qt::StringLiterals;

#define CREATE_VIEW(string)                                              \
    const QString padded = QLatin1Char(' ') + string + QLatin1Char(' '); \
    const QStringView view = QStringView{ padded }.mid(1, padded.size() - 2);

namespace {

template <typename String> String detached(String s)
{
    if (!s.isNull()) { // detaching loses nullness, but we need to preserve it
        auto d = s.data();
        Q_UNUSED(d);
    }
    return s;
}

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
    { for (QChar ch : std::as_const(this->pinned)) (s.*mf)(ch); }

    template <typename MemFun, typename A1>
    void apply1(QString &s, MemFun mf, A1 a1) const
    { for (QChar ch : std::as_const(this->pinned)) (s.*mf)(a1, ch); }
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
    { (s.*mf)(this->pinned.constData(), this->pinned.size()); }

    template <typename MemFun, typename A1>
    void apply1(QString &s, MemFun mf, A1 a1) const
    { (s.*mf)(a1, this->pinned.constData(), this->pinned.size()); }
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

template <bool b>
class Arg<QBasicUtf8StringView<b>>
{
    QUtf8StringView u8;
public:
    explicit Arg(const char *str) : u8(str) {}

    template <typename MemFunc>
    void apply0(QString &s, MemFunc mf) const
    { (s.*mf)(u8); }

    template <typename MemFunc, typename A1>
    void apply1(QString &s, MemFunc mf, A1 a1) const
    { (s.*mf)(a1, u8); }
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

    // Test when the string is shared
    QString str = s;
    Arg<ArgType>(arg).apply1(str, mf, a1);

    QCOMPARE(str, expected);
    QCOMPARE(str.isEmpty(), expected.isEmpty());
    QCOMPARE(str.isNull(), expected.isNull());

    // Test when the string is not shared
    str = s;
    str.detach();
    Arg<ArgType>(arg).apply1(str, mf, a1);
    QCOMPARE(str, expected);
    QCOMPARE(str.isEmpty(), expected.isEmpty());
    // A detached string is not null
    // QCOMPARE(str.isNull(), expected.isNull());
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
        // This default-constructed QLocale records what *was* the default before we changed it:
        const QLocale prior = {};
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
    void nullTerminated();
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
    void replace_pos_len_data();
    void replace_pos_len();
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
    void erase_single_arg();
    void erase();
    void swap();

    void prepend_qstring()            { prepend_impl<QString>(); }
    void prepend_qstring_data()       { prepend_data(EmptyIsNoop); }
    void prepend_qstringview()        { prepend_impl<QStringView, QString &(QString::*)(QStringView)>(); }
    void prepend_qstringview_data()   { prepend_data(EmptyIsNoop); }
    void prepend_qlatin1string()      { prepend_impl<QLatin1String, QString &(QString::*)(QLatin1String)>(); }
    void prepend_qlatin1string_data() { prepend_data({EmptyIsNoop, Latin1Encoded}); }
    void prepend_qutf8stringview()    { prepend_impl<QUtf8StringView, QString &(QString::*)(QUtf8StringView)>(); }
    void prepend_qutf8stringview_data() { prepend_data(EmptyIsNoop); }
    void prepend_qcharstar_int()      { prepend_impl<QPair<const QChar *, int>, QString &(QString::*)(const QChar *, qsizetype)>(); }
    void prepend_qcharstar_int_data() { prepend_data(EmptyIsNoop); }
    void prepend_qchar()              { prepend_impl<Reversed<QChar>, QString &(QString::*)(QChar)>(); }
    void prepend_qchar_data()         { prepend_data(EmptyIsNoop); }

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void prepend_qbytearray()         { prepend_impl<QByteArray>(); }
    void prepend_qbytearray_data()    { prepend_data(EmptyIsNoop); }
    void prepend_charstar()           { prepend_impl<const char *, QString &(QString::*)(const char *)>(); }
    void prepend_charstar_data()      { prepend_data(EmptyIsNoop); }
    void prepend_bytearray_special_cases_data();
    void prepend_bytearray_special_cases();
#endif // !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)

#if !defined(QT_NO_CAST_FROM_ASCII)
    void prepend_char()               { prepend_impl<Reversed<char>, QString &(QString::*)(QChar)>(); }
    void prepend_char_data()          { prepend_data({EmptyIsNoop, Latin1Encoded}); }
#endif

    void prependEventuallyProducesFreeSpaceAtBegin();

    void append_qstring()            { append_impl<QString>(); }
    void append_qstring_data()       { append_data(); }
    void append_qstringview()        { append_impl<QStringView,  QString &(QString::*)(QStringView)>(); }
    void append_qstringview_data()   { append_data(EmptyIsNoop); }
    void append_qlatin1string()      { append_impl<QLatin1String, QString &(QString::*)(QLatin1String)>(); }
    void append_qlatin1string_data() { append_data(Latin1Encoded); }
    void append_qutf8stringview()    { append_impl<QUtf8StringView,  QString &(QString::*)(QUtf8StringView)>(); }
    void append_qutf8stringview_data() { append_data(); }
    void append_qcharstar_int()      { append_impl<QPair<const QChar *, int>, QString&(QString::*)(const QChar *, qsizetype)>(); }
    void append_qcharstar_int_data() { append_data(EmptyIsNoop); }
    void append_qchar()              { append_impl<QChar, QString &(QString::*)(QChar)>(); }
    void append_qchar_data()         { append_data(EmptyIsNoop); }

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void append_qbytearray()         { append_impl<QByteArray>(); }
    void append_qbytearray_data()    { append_data(); }
#endif

#if !defined(QT_NO_CAST_FROM_ASCII)
    void append_char()               { append_impl<char, QString &(QString::*)(QChar)>(); }
    void append_char_data()          { append_data({EmptyIsNoop, Latin1Encoded}); }
#endif

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void append_charstar()           { append_impl<const char *, QString &(QString::*)(const char *)>(); }
    void append_charstar_data()      { append_data(); }
#endif

    void append_special_cases();

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void append_bytearray_special_cases_data();
    void append_bytearray_special_cases();
#endif

    void appendFromRawData();

    void operator_pluseq_qstring()            { operator_pluseq_impl<QString>(); }
    void operator_pluseq_qstring_data()       { operator_pluseq_data(); }
    void operator_pluseq_qstringview()        { operator_pluseq_impl<QStringView, QString &(QString::*)(QStringView)>(); }
    void operator_pluseq_qstringview_data()   { operator_pluseq_data(EmptyIsNoop); }
    void operator_pluseq_qlatin1string()      { operator_pluseq_impl<QLatin1String, QString &(QString::*)(QLatin1String)>(); }
    void operator_pluseq_qlatin1string_data() { operator_pluseq_data(Latin1Encoded); }
    void operator_pluseq_qutf8stringview()    { operator_pluseq_impl<QUtf8StringView, QString &(QString::*)(QUtf8StringView)>(); }
    void operator_pluseq_qutf8stringview_data() { operator_pluseq_data(); }
    void operator_pluseq_qchar()              { operator_pluseq_impl<QChar, QString &(QString::*)(QChar)>(); }
    void operator_pluseq_qchar_data()         { operator_pluseq_data(EmptyIsNoop); }
#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void operator_pluseq_qbytearray()         { operator_pluseq_impl<QByteArray>(); }
    void operator_pluseq_qbytearray_data()    { operator_pluseq_data(); }
    void operator_pluseq_charstar()           { operator_pluseq_impl<const char *, QString &(QString::*)(const char *)>(); }
    void operator_pluseq_charstar_data()      { operator_pluseq_data(); }
    void operator_assign_symmetry();
#endif // !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)

    void operator_pluseq_special_cases();

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void operator_pluseq_bytearray_special_cases_data();
    void operator_pluseq_bytearray_special_cases();
#endif

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void operator_eqeq_bytearray_data();
    void operator_eqeq_bytearray();
#endif
    void operator_eqeq_nullstring();
    void operator_smaller();

    void insert_qstring()            { insert_impl<QString>(); }
    void insert_qstring_data()       { insert_data(EmptyIsNoop); }
    void insert_qstringview()        { insert_impl<QStringView, QString &(QString::*)(qsizetype, QStringView)>(); }
    void insert_qstringview_data()   { insert_data(EmptyIsNoop); }
    void insert_qlatin1string()      { insert_impl<QLatin1String, QString &(QString::*)(qsizetype, QLatin1String)>(); }
    void insert_qlatin1string_data() { insert_data({EmptyIsNoop, Latin1Encoded}); }
    void insert_qutf8stringview()    { insert_impl<QUtf8StringView, QString &(QString::*)(qsizetype, QUtf8StringView)>(); }
    void insert_qutf8stringview_data() { insert_data(EmptyIsNoop); }
    void insert_qcharstar_int()      { insert_impl<QPair<const QChar *, int>, QString &(QString::*)(qsizetype, const QChar*, qsizetype) >(); }
    void insert_qcharstar_int_data() { insert_data(EmptyIsNoop); }
    void insert_qchar()              { insert_impl<Reversed<QChar>, QString &(QString::*)(qsizetype, QChar)>(); }
    void insert_qchar_data()         { insert_data(EmptyIsNoop); }

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void insert_qbytearray()         { insert_impl<QByteArray>(); }
    void insert_qbytearray_data()    { insert_data(EmptyIsNoop); }
#endif

#ifndef QT_NO_CAST_FROM_ASCII
    void insert_char()               { insert_impl<Reversed<char>, QString &(QString::*)(qsizetype, QChar)>(); }
    void insert_char_data()          { insert_data({EmptyIsNoop, Latin1Encoded}); }
#endif

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void insert_charstar()           { insert_impl<const char *, QString &(QString::*)(qsizetype, const char*) >(); }
    void insert_charstar_data()      { insert_data(EmptyIsNoop); }
#endif

    void insert_special_cases();

    void assign();
    void assign_shared();
    void assign_uses_prepend_buffer();

    void simplified_data();
    void simplified();
    void trimmed_data();
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
#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void constructorQByteArray_data();
    void constructorQByteArray();
#endif

    void STL();
    void macTypes();
    void wasmTypes();
    void isEmpty();
    void isNull();
    void nullness();
#ifndef QT_NO_CAST_FROM_ASCII
    void acc_01();
#endif
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
    void arg_negative_tests();
    void number();
    void number_double_data();
    void number_double();
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
    void toNum_base_data();
    void toNum_base();
    void toNum_base_neg_data();
    void toNum_base_neg();
    void toNum_Bad();
    void toNum_BadAll_data();
    void toNum_BadAll();
    void toNum();
    void iterators();
    void reverseIterators();
    void split_data();
    void split();
#if QT_CONFIG(regularexpression)
    void split_regularexpression_data();
    void split_regularexpression();
    void regularexpression_lifetime();
#endif
    void fromUtf16_data();
#if QT_DEPRECATED_SINCE(6, 0)
    void fromUtf16();
#endif
    void fromUtf16_char16_data() { fromUtf16_data(); }

    void fromUtf16_char16();
    void latin1String();
    void isInf_data();
    void isInf();
    void isNan_data();
    void isNan();
    void nanAndInf();
    void comparisonCompiles();
    void compare_data();
    void compare();
    void comparisonMacros_data();
    void comparisonMacros();
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
#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    void eightBitLiterals_data();
    void eightBitLiterals();
#endif
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
    void first();
    void last();
    void sliced();
    void slice();
    void chopped();
    void removeIf();

    void std_stringview_conversion();
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
    if (QChar(u'\0') != strTerminator)
        return QString::fromLatin1(
            "*** Result ('%1') not null-terminated: 0x%2 ***").arg(str)
                .arg(ushort{strTerminator.unicode()}, 4, 16, QChar(u'0'));

    // Skip mutating checks on shared strings
    if (strDataPtr->isShared())
        return str;

    const QChar *strData = str.constData();
    const QString strCopy(strData, strSize); // Deep copy

    const_cast<QChar *>(strData)[strSize] = QChar(u'x');
    if (QChar(u'x') != str.constData()[strSize]) {
        return QString::fromLatin1("*** Failed to replace null-terminator in "
                "result ('%1') ***").arg(str);
    }
    if (str != strCopy) {
        return QString::fromLatin1( "*** Result ('%1') differs from its copy "
                "after null-terminator was replaced ***").arg(str);
    }
    const_cast<QChar *>(strData)[strSize] = QChar(u'\0'); // Restore sanity

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
    setlocale(LC_ALL, "");
}

void tst_QString::remove_uint_uint_data()
{
    replace_pos_len_data();
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

    QTest::newRow("1") << u"foo"_s << QChar(u'o') << QChar(u'a') << Qt::CaseSensitive << u"faa"_s;
    QTest::newRow("2") << u"foo"_s << QChar(u'o') << QChar(u'a') << Qt::CaseInsensitive << u"faa"_s;
    QTest::newRow("3") << u"foo"_s << QChar(u'O') << QChar(u'a') << Qt::CaseSensitive << u"foo"_s;
    QTest::newRow("4") << u"foo"_s << QChar(u'O') << QChar(u'a') << Qt::CaseInsensitive << u"faa"_s;
    QTest::newRow("5") << u"ababABAB"_s << QChar(u'a') << QChar(u' ') << Qt::CaseSensitive
                       << u" b bABAB"_s;
    QTest::newRow("6") << u"ababABAB"_s << QChar(u'a') << QChar(u' ') << Qt::CaseInsensitive
                       << u" b b B B"_s;
    QTest::newRow("7") << u"ababABAB"_s << QChar() << QChar(u' ') << Qt::CaseInsensitive
                       << u"ababABAB"_s;
    QTest::newRow("8") << QString() << QChar() << QChar(u'x') << Qt::CaseInsensitive << QString();
    QTest::newRow("9") << QString() << QChar(u'a') << QChar(u'x') << Qt::CaseInsensitive
                       << QString();
}

void tst_QString::replace_qchar_qchar()
{
    QFETCH(QString, src);
    QFETCH(QChar, before);
    QFETCH(QChar, after);
    QFETCH(Qt::CaseSensitivity, cs);
    QFETCH(QString, expected);

    QString str = src;
    // Test when string is shared
    QCOMPARE(str.replace(before, after, cs), expected);

    str = src;
    // Test when it's not shared
    str.detach();
    QCOMPARE(str.replace(before, after, cs), expected);
}

void tst_QString::replace_qchar_qstring_data()
{
    QTest::addColumn<QString>("src" );
    QTest::addColumn<QChar>("before" );
    QTest::addColumn<QString>("after" );
    QTest::addColumn<Qt::CaseSensitivity>("cs");
    QTest::addColumn<QString>("expected" );

    QTest::newRow("1") << u"foo"_s << QChar(u'o') << u"aA"_s << Qt::CaseSensitive
                       << u"faAaA"_s;
    QTest::newRow("2") << u"foo"_s << QChar(u'o') << u"aA"_s << Qt::CaseInsensitive
                       << u"faAaA"_s;
    QTest::newRow("3") << u"foo"_s << QChar(u'O') << u"aA"_s << Qt::CaseSensitive
                       << u"foo"_s;
    QTest::newRow("4") << u"foo"_s << QChar(u'O') << u"aA"_s << Qt::CaseInsensitive
                       << u"faAaA"_s;
    QTest::newRow("5") << u"ababABAB"_s << QChar(u'a') << u"  "_s << Qt::CaseSensitive
                       << u"  b  bABAB"_s;
    QTest::newRow("6") << u"ababABAB"_s << QChar(u'a') << u"  "_s << Qt::CaseInsensitive
                       << u"  b  b  B  B"_s;
    QTest::newRow("7") << u"ababABAB"_s << QChar() << u"  "_s << Qt::CaseInsensitive
                       << u"ababABAB"_s;
    QTest::newRow("8") << u"ababABAB"_s << QChar() << QString() << Qt::CaseInsensitive
                       << u"ababABAB"_s;
    QTest::newRow("null-in-null-with-X") << QString() << QChar() << u"X"_s
                                         << Qt::CaseSensitive << QString();
    QTest::newRow("x-in-null-with-abc") << QString() << QChar(u'x') << u"abc"_s
                                        << Qt::CaseSensitive << QString();
    QTest::newRow("null-in-empty-with-X") << u""_s << QChar() << u"X"_s
                                          << Qt::CaseInsensitive << QString();
    QTest::newRow("x-in-empty-with-abc") << u""_s << QChar(u'x') << u"abc"_s
                                          << Qt::CaseInsensitive << QString();
}

void tst_QString::replace_qchar_qstring()
{
    QFETCH(QString, src);
    QFETCH(QChar, before);
    QFETCH(QString, after);
    QFETCH(Qt::CaseSensitivity, cs);
    QFETCH(QString, expected);

    // Test when string needs detach
    QString s = src;
    QCOMPARE(s.replace(before, after, cs), expected);

    // Test when it's not shared
    s = src;
    s.detach();
    QCOMPARE(s.replace(before, after, cs), expected);
}

void tst_QString::replace_pos_len_data()
{
    QTest::addColumn<QString>("string" );
    QTest::addColumn<qsizetype>("index" );
    QTest::addColumn<qsizetype>("len" );
    QTest::addColumn<QString>("after" );
    QTest::addColumn<QString>("result" );

    QTest::newRow("empty_rem00") << QString()
                                 << qsizetype(0) << qsizetype(0) << u""_s
                                 << QString();

    QTest::newRow("empty_rem01") << QString()
                                 << qsizetype(0) << qsizetype(3) << u""_s
                                 << QString();

    QTest::newRow("empty_rem02") << QString()
                                 << qsizetype(5) << qsizetype(3) << u""_s
                                 << QString();

    QTest::newRow("rem00") << u"-<>ABCABCABCABC>"_s
                           << qsizetype(0) << qsizetype(3) << u""_s
                           << u"ABCABCABCABC>"_s;

    QTest::newRow("rem01") << u"ABCABCABCABC>"_s
                           << qsizetype(1) << qsizetype(4) << u""_s
                           << u"ACABCABC>"_s;

    QTest::newRow("rem04") << u"ACABCABC>"_s << qsizetype(8) << qsizetype(4)
                           << u""_s
                           << u"ACABCABC"_s;

    QTest::newRow("rem05") << u"ACABCABC"_s << qsizetype(7) << qsizetype(1)
                           << u""_s
                           << u"ACABCAB"_s;

    QTest::newRow("rem06") << u"ACABCAB"_s
                           << qsizetype(4) << qsizetype(0) << u""_s
                           << u"ACABCAB"_s;

    QTest::newRow("empty_rep00") << QString()
                                 << qsizetype(0) << qsizetype(0) << u"X"_s
                                 << u"X"_s;

    QTest::newRow("empty_rep01") << QString()
                                 << qsizetype(0) << qsizetype(3) << u"X"_s
                                 << u"X"_s;

    QTest::newRow("empty_rep02") << QString()
                                 << qsizetype(5) << qsizetype(3) << u"X"_s
                                 << QString();

    QTest::newRow("rep00") << u"ACABCAB"_s
                           << qsizetype(4) << qsizetype(0) << u"X"_s
                           << u"ACABXCAB"_s;

    QTest::newRow("rep01") << u"ACABXCAB"_s
                           << qsizetype(4) << qsizetype(1) << u"Y"_s
                           << u"ACABYCAB"_s;

    QTest::newRow("rep02") << u"ACABYCAB"_s
                           << qsizetype(4) << qsizetype(1) << u""_s
                           << u"ACABCAB"_s;

    QTest::newRow("rep03") << u"ACABCAB"_s
                           << qsizetype(0) << qsizetype(9999) << u"XX"_s
                           << u"XX"_s;

    QTest::newRow("rep04") << u"XX"_s
                           << qsizetype(0) << qsizetype(9999) << u""_s
                           << u""_s;

    QTest::newRow("rep05") << u"ACABCAB"_s
                           << qsizetype(0) << qsizetype(2) << u"XX"_s
                           << u"XXABCAB"_s;

    QTest::newRow("rep06") << u"ACABCAB"_s
                           << qsizetype(1) << qsizetype(2) << u"XX"_s
                           << u"AXXBCAB"_s;

    QTest::newRow("rep07") << u"ACABCAB"_s
                           << qsizetype(2) << qsizetype(2) << u"XX"_s
                           << u"ACXXCAB"_s;

    QTest::newRow("rep08") << u"ACABCAB"_s
                           << qsizetype(3) << qsizetype(2) << u"XX"_s
                           << u"ACAXXAB"_s;

    QTest::newRow("rep09") << u"ACABCAB"_s
                           << qsizetype(4) << qsizetype(2) << u"XX"_s
                           << u"ACABXXB"_s;

    QTest::newRow("rep10") << u"ACABCAB"_s
                           << qsizetype(5) << qsizetype(2) << u"XX"_s
                           << u"ACABCXX"_s;

    QTest::newRow("rep11") << u"ACABCAB"_s
                           << qsizetype(6) << qsizetype(2) << u"XX"_s
                           << u"ACABCAXX"_s;

    QTest::newRow("rep12") << QString()
                           << qsizetype(0) << qsizetype(10) << u"X"_s
                           << u"X"_s;

    QTest::newRow("rep13") << u"short"_s
                           << qsizetype(0) << qsizetype(10) << u"X"_s
                           << u"X"_s;

    QTest::newRow("rep14") << QString()
                           << qsizetype(0) << qsizetype(10) << u"XX"_s
                           << u"XX"_s;

    QTest::newRow("rep15") << u"short"_s
                           << qsizetype(0) << qsizetype(10) << u"XX"_s
                           << u"XX"_s;

    // This is a regression test for an old bug where QString would add index and len parameters,
    // potentially causing integer overflow.
    constexpr qsizetype maxSize = std::numeric_limits<qsizetype>::max();
    QTest::newRow("no overflow") << u"ACABCAB"_s
                                 << qsizetype(1) << maxSize - 1 << u""_s
                                 << u"A"_s;

    QTest::newRow("overflow") << u"ACABCAB"_s
                              << qsizetype(1) << maxSize << u""_s
                              << u"A"_s;
}

void tst_QString::replace_string_data()
{
    QTest::addColumn<QString>("string" );
    QTest::addColumn<QString>("before" );
    QTest::addColumn<QString>("after" );
    QTest::addColumn<QString>("result" );
    QTest::addColumn<bool>("bcs" );

    QTest::newRow( "rem00" ) << u""_s << u""_s << u""_s << u""_s << true;
    QTest::newRow( "rem01" ) << u"A"_s << u""_s << u""_s << u"A"_s << true;
    QTest::newRow( "rem02" ) << u"A"_s << u"A"_s << u""_s << u""_s << true;
    QTest::newRow( "rem03" ) << u"A"_s << u"B"_s << u""_s << u"A"_s << true;
    QTest::newRow( "rem04" ) << u"AA"_s << u"A"_s << u""_s << u""_s << true;
    QTest::newRow( "rem05" ) << u"AB"_s << u"A"_s << u""_s << u"B"_s << true;
    QTest::newRow( "rem06" ) << u"AB"_s << u"B"_s << u""_s << u"A"_s << true;
    QTest::newRow( "rem07" ) << u"AB"_s << u"C"_s << u""_s << u"AB"_s << true;
    QTest::newRow( "rem08" ) << u"ABA"_s << u"A"_s << u""_s << u"B"_s << true;
    QTest::newRow( "rem09" ) << u"ABA"_s << u"B"_s << u""_s << u"AA"_s << true;
    QTest::newRow( "rem10" ) << u"ABA"_s << u"C"_s << u""_s << u"ABA"_s << true;
    QTest::newRow( "rem11" ) << u"banana"_s << u"an"_s << u""_s << u"ba"_s << true;
    QTest::newRow( "rem12" ) << u""_s << u"A"_s << u""_s << u""_s << true;
    QTest::newRow( "rem13" ) << u""_s << u"A"_s << QString() << u""_s << true;
    QTest::newRow( "rem14" ) << QString() << u"A"_s << u""_s << QString() << true;
    QTest::newRow( "rem15" ) << QString() << u"A"_s << QString() << QString() << true;
    QTest::newRow( "rem16" ) << QString() << u""_s << u""_s << u""_s << true;
    QTest::newRow( "rem17" ) << u""_s << QString() << u""_s << u""_s << true;
    QTest::newRow( "rem18" ) << u"a"_s << u"a"_s << u""_s << u""_s << false;
    QTest::newRow( "rem19" ) << u"A"_s << u"A"_s << u""_s << u""_s << false;
    QTest::newRow( "rem20" ) << u"a"_s << u"A"_s << u""_s << u""_s << false;
    QTest::newRow( "rem21" ) << u"A"_s << u"a"_s << u""_s << u""_s << false;
    QTest::newRow( "rem22" ) << u"Alpha beta"_s << u"a"_s << u""_s << u"lph bet"_s << false;
    QTest::newRow( "rem23" ) << u"+00:00"_s << u":"_s << u""_s << u"+0000"_s << false;

    QTest::newRow( "rep00" ) << u"ABC"_s << u"B"_s << u"-"_s << u"A-C"_s << true;
    QTest::newRow( "rep01" ) << u"$()*+.?[\\]^{|}"_s << u"$()*+.?[\\]^{|}"_s << u"X"_s << u"X"_s << true;
    QTest::newRow( "rep02" ) << u"ABCDEF"_s << u""_s << u"X"_s << u"XAXBXCXDXEXFX"_s << true;
    QTest::newRow( "rep03" ) << u""_s << u""_s << u"X"_s << u"X"_s << true;
    QTest::newRow( "rep04" ) << u"a"_s << u"a"_s << u"b"_s << u"b"_s << false;
    QTest::newRow( "rep05" ) << u"A"_s << u"A"_s << u"b"_s << u"b"_s << false;
    QTest::newRow( "rep06" ) << u"a"_s << u"A"_s << u"b"_s << u"b"_s << false;
    QTest::newRow( "rep07" ) << u"A"_s << u"a"_s << u"b"_s << u"b"_s << false;
    QTest::newRow( "rep08" ) << u"a"_s << u"a"_s << u"a"_s << u"a"_s << false;
    QTest::newRow( "rep09" ) << u"A"_s << u"A"_s << u"a"_s << u"a"_s << false;
    QTest::newRow( "rep10" ) << u"a"_s << u"A"_s << u"a"_s << u"a"_s << false;
    QTest::newRow( "rep11" ) << u"A"_s << u"a"_s << u"a"_s << u"a"_s << false;
    QTest::newRow( "rep12" ) << u"Alpha beta"_s << u"a"_s << u"o"_s << u"olpho beto"_s << false;
    QTest::newRow( "rep13" ) << QString() << u""_s << u"A"_s << u"A"_s << true;
    QTest::newRow( "rep14" ) << u""_s << QString() << u"A"_s << u"A"_s << true;
    QTest::newRow( "rep15" ) << u"fooxbarxbazxblub"_s << u"x"_s << u"yz"_s << u"fooyzbaryzbazyzblub"_s << true;
    QTest::newRow( "rep16" ) << u"fooxbarxbazxblub"_s << u"x"_s << u"z"_s << u"foozbarzbazzblub"_s << true;
    QTest::newRow( "rep17" ) << u"fooxybarxybazxyblub"_s << u"xy"_s << u"z"_s << u"foozbarzbazzblub"_s << true;
    QTest::newRow("rep18") << QString() << QString() << u"X"_s << u"X"_s << false;
    QTest::newRow("rep19") << QString() << u"A"_s << u"X"_s << u""_s << false;
}

#if QT_CONFIG(regularexpression)
void tst_QString::replace_regexp_data()
{
    remove_regexp_data(); // Sets up the columns, adds rows with empty replacement text.
    // Columns (all QString): string, regexp, after, result; string.replace(regexp, after) == result
    // Test-cases with empty after (replacement text, third column) go in remove_regexp_data()

    QTest::newRow("empty-in-null") << QString() << "" << "after" << "after";
    QTest::newRow("empty-in-empty") << "" << "" << "after" << "after";

    QTest::newRow( "rep00" ) << u"A <i>bon mot</i>."_s << u"<i>([^<]*)</i>"_s << u"\\emph{\\1}"_s << u"A \\emph{bon mot}."_s;
    QTest::newRow( "rep01" ) << u"banana"_s << u"^.a()"_s << u"\\1"_s << u"nana"_s;
    QTest::newRow( "rep02" ) << u"banana"_s << u"(ba)"_s << u"\\1X\\1"_s << u"baXbanana"_s;
    QTest::newRow( "rep03" ) << u"banana"_s << u"(ba)(na)na"_s << u"\\2X\\1"_s << u"naXba"_s;
    QTest::newRow("rep04") << QString() << u"(ba)"_s << u"\\1X\\1"_s << QString();

    QTest::newRow("backref00") << u"\\1\\2\\3\\4\\5\\6\\7\\8\\9\\A\\10\\11"_s << u"\\\\[34]"_s
                               << u"X"_s << u"\\1\\2XX\\5\\6\\7\\8\\9\\A\\10\\11"_s;
    QTest::newRow("backref01") << u"foo"_s << u"[fo]"_s << u"\\1"_s << u"\\1\\1\\1"_s;
    QTest::newRow("backref02") << u"foo"_s << u"([fo])"_s << u"(\\1)"_s << u"(f)(o)(o)"_s;
    QTest::newRow("backref03") << u"foo"_s << u"([fo])"_s << u"\\2"_s << u"\\2\\2\\2"_s;
    QTest::newRow("backref04") << u"foo"_s << u"([fo])"_s << u"\\10"_s << u"f0o0o0"_s;
    QTest::newRow("backref05") << u"foo"_s << u"([fo])"_s << u"\\11"_s << u"f1o1o1"_s;
    QTest::newRow("backref06") << u"foo"_s << u"([fo])"_s << u"\\19"_s << u"f9o9o9"_s;
    QTest::newRow("backref07") << u"foo"_s << u"(f)(o+)"_s
                               << u"\\2\\1\\10\\20\\11\\22\\19\\29\\3"_s
                               << u"ooff0oo0f1oo2f9oo9\\3"_s;
    QTest::newRow("backref08") << u"abc"_s << u"(((((((((((((([abc]))))))))))))))"_s
                               << u"{\\14}"_s << u"{a}{b}{c}"_s;
    QTest::newRow("backref09") << u"abcdefghijklmn"_s
                               << u"(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)(k)(l)(m)(n)"_s
                               << u"\\19\\18\\17\\16\\15\\14\\13\\12\\11\\10"
                                   "\\9\\90\\8\\80\\7\\70\\6\\60\\5\\50\\4\\40\\3\\30\\2\\20\\1"_s
                               << u"a9a8a7a6a5nmlkjii0hh0gg0ff0ee0dd0cc0bb0a"_s;
    QTest::newRow("backref10") << u"abc"_s << u"((((((((((((((abc))))))))))))))"_s
                               << u"\\0\\01\\011"_s << u"\\0\\01\\011"_s;
}
#endif

void tst_QString::utf8_data()
{
    QString str;
    QTest::addColumn<QByteArray>("utf8" );
    QTest::addColumn<QString>("res" );

    QTest::newRow("null") << QByteArray() << QString();
    QTest::newRow("empty") << QByteArray("") << u""_s;

    QTest::newRow("str0") << QByteArray("abcdefgh") << u"abcdefgh"_s;

    QTest::newRow( "str1" ) << QByteArray("\303\266\303\244\303\274\303\226\303\204\303\234\303\270\303\246\303\245\303\230\303\206\303\205")
                          << QString::fromLatin1("\366\344\374\326\304\334\370\346\345\330\306\305") ;
    str += QChar( 0x05e9 );
    str += QChar( 0x05d3 );
    str += QChar( 0x05d2 );
    QTest::newRow( "str2" ) << QByteArray("\327\251\327\223\327\222")
                          << str;

    str = QChar( 0x20ac );
    str += u" some text"_s;
    QTest::newRow( "str3" ) << QByteArray("\342\202\254 some text")
                          << str;

    str = u"Old Italic: "_s;
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
    QTest::newRow("empty") << u""_s << qsizetype(0);
    QTest::newRow("data0") << u"Test"_s << qsizetype(4);
    QTest::newRow("data1") << u"The quick brown fox jumps over the lazy dog"_s << qsizetype(43);
    QTest::newRow("data2") << u"Sphinx of black quartz, judge my vow!"_s << qsizetype(37);
    QTest::newRow("data3") << u"A"_s << qsizetype(1);
    QTest::newRow("data4") << u"AB"_s << qsizetype(2);
    QTest::newRow("data5") << u"AB\n"_s << qsizetype(3);
    QTest::newRow("data6") << u"AB\nC"_s << qsizetype(4);
    QTest::newRow("data7") << u"\n"_s << qsizetype(1);
    QTest::newRow("data8") << u"\nA"_s << qsizetype(2);
    QTest::newRow("data9") << u"\nAB"_s << qsizetype(3);
    QTest::newRow("data10") << u"\nAB\nCDE"_s << qsizetype(7);
    QTest::newRow("data11") << u"shdnftrheid fhgnt gjvnfmd chfugkh bnfhg thgjf vnghturkf "
                                "chfnguh bjgnfhvygh hnbhgutjfv dhdnjds dcjs d"_s
                            << qsizetype(100);
}

void tst_QString::length()
{
    // size(), length() and count() do the same
    QFETCH(QString, s1);
    QTEST(s1.size(), "res");
    QTEST(s1.size(), "res");
#if QT_DEPRECATED_SINCE(6, 4)
    QT_IGNORE_DEPRECATIONS(QTEST(s1.size(), "res");)
#endif
}

#include <qfile.h>

#ifndef QT_NO_CAST_FROM_ASCII
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

#ifndef QT_RESTRICTED_CAST_FROM_ASCII
    char text[]="String f";
    f = text;
    text[7]='!';
    QCOMPARE(f, QLatin1String("String f"));
    f[7]='F';
    QCOMPARE(text[7],'!');
#endif

    a="123";
    b="456";
    a[0]=a[1];
    QCOMPARE(a, QLatin1String("223"));
    a[1]=b[1];
    QCOMPARE(b, QLatin1String("456"));
    QCOMPARE(a, QLatin1String("253"));

#ifndef QT_RESTRICTED_CAST_FROM_ASCII
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
#endif

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
#ifndef QT_RESTRICTED_CAST_FROM_ASCII
    a = (const char*)0;
    QVERIFY(a.isNull());
    QVERIFY(*a.toLatin1().constData() == '\0');
#endif
}
#endif // QT_NO_CAST_FROM_ASCII

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

void tst_QString::nullness()
{
    {
        QString s;
        QVERIFY(s.isNull());
    }
#if defined(__cpp_char8_t) || !defined(QT_RESTRICTED_CAST_FROM_ASCII)
#if !defined(QT_NO_CAST_FROM_ASCII)
    // we don't have QString(std::nullptr_t), so this uses QString(const char8_t*) in C++20:
    {
        QString s = nullptr;
        QVERIFY(s.isNull());
    }
#endif
#endif

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    {
        const char *ptr = nullptr;
        QString s = ptr;
        QVERIFY(s.isNull());
    }
#endif
#ifdef __cpp_char8_t
    {
        const char8_t *ptr = nullptr;
        QString s = ptr;
        QVERIFY(s.isNull());
    }
#endif
    {
        QString s(nullptr, 0);
        QVERIFY(s.isNull());
    }
    {
        const QChar *ptr = nullptr;
        QString s(ptr, 0);
        QVERIFY(s.isNull());
    }
    {
        QLatin1String l1;
        QVERIFY(l1.isNull());
        QString s = l1;
        QVERIFY(s.isNull());
    }
    {
        QStringView sv;
        QVERIFY(sv.isNull());
        QString s = sv.toString();
        QVERIFY(s.isNull());
    }
}

void tst_QString::isEmpty()
{
    QString a;
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    QString b = QString::fromLatin1("Not empty");
    QVERIFY(!b.isEmpty());

    QString c = u"Not empty"_s;
    QVERIFY(!c.isEmpty());
}

void tst_QString::constructor()
{
    // String literal with explicit \0 character
    static constexpr char16_t utf16[] = u"String DX\u0000";
    const size_t size_minus_null_terminator = std::size(utf16) - 1;
    const auto *qchar = reinterpret_cast<const QChar *>(utf16);

    // Up to but not including the explicit \0 in utf16[]
    QString b1(qchar);
    QCOMPARE(b1, u"String DX");
    // Up to and including the explicit \0 in utf16[]
    QString b2(qchar, size_minus_null_terminator);
    QCOMPARE(b2, QStringView(utf16, size_minus_null_terminator));

    QString a;
    QString a_copy(a);
    QCOMPARE(a, a_copy);
    QVERIFY(a.isNull());
    QCOMPARE(a, u""_s);

    QString c(u"String C"_s);
    QString c_copy(c);
    QCOMPARE(c, c_copy);

    QString e(QLatin1StringView("String E"));
    QString e_copy(e);
    QCOMPARE(e, e_copy);
    QCOMPARE(e, "String E"_L1);

    QString d(qchar, 8);
    QCOMPARE(d, "String D"_L1);

    QString nullStr;
    QVERIFY( nullStr.isNull() );
    QVERIFY( nullStr.isEmpty() );

    QString empty(u""_s);
    QVERIFY( !empty.isNull() );
    QVERIFY( empty.isEmpty() );

    empty = QString::fromLatin1("");
    QVERIFY(!empty.isNull());
    QVERIFY(empty.isEmpty());

    empty = QString::fromUtf8("");
    QVERIFY(!empty.isNull());
    QVERIFY(empty.isEmpty());
}

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
void tst_QString::constructorQByteArray_data()
{
    QTest::addColumn<QByteArray>("src" );
    QTest::addColumn<QString>("expected" );

    QByteArray ba( 4, 0 );
    ba[0] = 'C';
    ba[1] = 'O';
    ba[2] = 'M';
    ba[3] = 'P';

    QTest::newRow( "1" ) << ba << u"COMP"_s;

    QByteArray ba1( 7, 0 );
    ba1[0] = 'a';
    ba1[1] = 'b';
    ba1[2] = 'c';
    ba1[3] = '\0';
    ba1[4] = 'd';
    ba1[5] = 'e';
    ba1[6] = 'f';

    QTest::newRow( "2" ) << ba1 << QString::fromUtf16(u"abc\0def", 7);

    QTest::newRow( "3" ) << QByteArray::fromRawData("abcd", 3) << u"abc"_s;
    QTest::newRow( "4" ) << QByteArray("\xc3\xa9") << QString::fromUtf8("\xc3\xa9");
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
    if (src.constData()[src.size()] == '\0') {
        qsizetype zero = expected.indexOf(QLatin1Char('\0'));
        if (zero < 0)
            zero = expected.size();

        QString str1(src.constData());
        QCOMPARE(str1.size(), zero);
        QCOMPARE(str1, expected.left(zero));

        str1.clear();
        str1 = src.constData();
        QCOMPARE(str1, expected.left(zero));
    }
}
#endif // !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)

void tst_QString::STL()
{
    QString nullStr;
    QVERIFY(nullStr.toStdWString().empty());
    QVERIFY(!nullStr.isDetached());

    wchar_t dataArray[] = { 'w', 'o', 'r', 'l', 'd', 0 };

    QCOMPARE(nullStr.toWCharArray(dataArray), 0);
    QVERIFY(dataArray[0] == 'w'); // array was not modified
    QVERIFY(!nullStr.isDetached());

    QString emptyStr(u""_s);

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
#ifndef Q_OS_DARWIN
    QSKIP("This is a Mac-only test");
#else
    extern void tst_QString_macTypes(); // in qcore_foundation.mm
    tst_QString_macTypes();
#endif
}

void tst_QString::wasmTypes()
{
#ifndef Q_OS_WASM
    QSKIP("This is a WASM-only test");
#else
    extern void tst_QString_wasmTypes(); // in qcore_wasm.cpp
    tst_QString_wasmTypes();
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

    QString emptyStr(u""_s);
    emptyStr.truncate(5);
    QVERIFY(emptyStr.isEmpty());
    emptyStr.truncate(0);
    QVERIFY(emptyStr.isEmpty());
    emptyStr.truncate(-3);
    QVERIFY(emptyStr.isEmpty());
    QVERIFY(!emptyStr.isDetached());

    QString e(u"String E"_s);
    e.truncate(4);
    QCOMPARE(e, QLatin1String("Stri"));

    e = u"String E"_s;
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

    const QString original(u"abcd"_s);

    QTest::newRow("null chop 1") << QString() << 1 << QString();
    QTest::newRow("null chop -1") << QString() << -1 << QString();
    QTest::newRow("empty chop 1") << u""_s << 1 << u""_s;
    QTest::newRow("empty chop -1") << u""_s << -1 << u""_s;
    QTest::newRow("data0") << original << 1 << u"abc"_s;
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
    e.fill(u'e', 1);
    QCOMPARE(e, QLatin1String("e"));
    QString f;
    f.fill(u'f', 3);
    QCOMPARE(f, QLatin1String("fff"));
    f.fill(u'F');
    QCOMPARE(f, QLatin1String("FFF"));
    f.fill(u'a', 2);
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

QT_WARNING_PUSH
// Android clang emits warning: '%n' specifier not supported on this platform
QT_WARNING_DISABLE_CLANG("-Wformat")
    int n1;
    QCOMPARE(QString::asprintf("%s%n%s", "hello", &n1, "goodbye"), u"hellogoodbye");
    QCOMPARE(n1, 5);
    qlonglong n2;
    QCOMPARE(QString::asprintf("%s%s%lln%s", "foo", "bar", &n2, "whiz"), u"foobarwhiz");
    QCOMPARE((int)n2, 6);
QT_WARNING_POP

    { // %ls

        QString str(u"Hello"_s);
        QCOMPARE(QString::asprintf("%.3ls",     qUtf16Printable(str)), "Hel"_L1);
        QCOMPARE(QString::asprintf("%10.3ls",   qUtf16Printable(str)), "       Hel"_L1);
        QCOMPARE(QString::asprintf("%.10ls",    qUtf16Printable(str)), "Hello"_L1);
        QCOMPARE(QString::asprintf("%10.10ls",  qUtf16Printable(str)), "     Hello"_L1);
        QCOMPARE(QString::asprintf("%-10.10ls", qUtf16Printable(str)), "Hello     "_L1);
        QCOMPARE(QString::asprintf("%-10.3ls",  qUtf16Printable(str)), "Hel       "_L1);
        QCOMPARE(QString::asprintf("%-5.5ls",   qUtf16Printable(str)), "Hello"_L1);
        QCOMPARE(QString::asprintf("%*ls",   4, qUtf16Printable(str)), "Hello"_L1);
        QCOMPARE(QString::asprintf("%*ls",  10, qUtf16Printable(str)), "     Hello"_L1);
        QCOMPARE(QString::asprintf("%-*ls", 10, qUtf16Printable(str)), "Hello     "_L1);

        // Check utf16 is preserved for %ls
        QCOMPARE(QString::asprintf("%ls",
                                   qUtf16Printable(QString::fromUtf8(
                                           "\303\266\303\244\303\274\303\226\303\204\303\234\303"
                                           "\270\303\246\303\245\303\230\303\206\303\205"))),
                 QLatin1String("\366\344\374\326\304\334\370\346\345\330\306\305"));

        int n;
QT_WARNING_PUSH
// Android clang emits warning: '%n' specifier not supported on this platform
QT_WARNING_DISABLE_CLANG("-Wformat")
        QCOMPARE(QString::asprintf("%ls%n%s", qUtf16Printable(u"hello"_s), &n, "goodbye"), "hellogoodbye"_L1);
QT_WARNING_POP
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

    QTest::newRow( "data0" ) << u"abc"_s << u"a"_s << 0 << true << 0;
    QTest::newRow( "data1" ) << u"abc"_s << u"a"_s << 0 << false << 0;
    QTest::newRow( "data2" ) << u"abc"_s << u"A"_s << 0 << true << -1;
    QTest::newRow( "data3" ) << u"abc"_s << u"A"_s << 0 << false << 0;
    QTest::newRow( "data4" ) << u"abc"_s << u"a"_s << 1 << true << -1;
    QTest::newRow( "data5" ) << u"abc"_s << u"a"_s << 1 << false << -1;
    QTest::newRow( "data6" ) << u"abc"_s << u"A"_s << 1 << true << -1;
    QTest::newRow( "data7" ) << u"abc"_s << u"A"_s << 1 << false << -1;
    QTest::newRow( "data8" ) << u"abc"_s << u"b"_s << 0 << true << 1;
    QTest::newRow( "data9" ) << u"abc"_s << u"b"_s << 0 << false << 1;
    QTest::newRow( "data10" ) << u"abc"_s << u"B"_s << 0 << true << -1;
    QTest::newRow( "data11" ) << u"abc"_s << u"B"_s << 0 << false << 1;
    QTest::newRow( "data12" ) << u"abc"_s << u"b"_s << 1 << true << 1;
    QTest::newRow( "data13" ) << u"abc"_s << u"b"_s << 1 << false << 1;
    QTest::newRow( "data14" ) << u"abc"_s << u"B"_s << 1 << true << -1;
    QTest::newRow( "data15" ) << u"abc"_s << u"B"_s << 1 << false << 1;
    QTest::newRow( "data16" ) << u"abc"_s << u"b"_s << 2 << true << -1;
    QTest::newRow( "data17" ) << u"abc"_s << u"b"_s << 2 << false << -1;

    QTest::newRow( "data20" ) << u"ABC"_s << u"A"_s << 0 << true << 0;
    QTest::newRow( "data21" ) << u"ABC"_s << u"A"_s << 0 << false << 0;
    QTest::newRow( "data22" ) << u"ABC"_s << u"a"_s << 0 << true << -1;
    QTest::newRow( "data23" ) << u"ABC"_s << u"a"_s << 0 << false << 0;
    QTest::newRow( "data24" ) << u"ABC"_s << u"A"_s << 1 << true << -1;
    QTest::newRow( "data25" ) << u"ABC"_s << u"A"_s << 1 << false << -1;
    QTest::newRow( "data26" ) << u"ABC"_s << u"a"_s << 1 << true << -1;
    QTest::newRow( "data27" ) << u"ABC"_s << u"a"_s << 1 << false << -1;
    QTest::newRow( "data28" ) << u"ABC"_s << u"B"_s << 0 << true << 1;
    QTest::newRow( "data29" ) << u"ABC"_s << u"B"_s << 0 << false << 1;
    QTest::newRow( "data30" ) << u"ABC"_s << u"b"_s << 0 << true << -1;
    QTest::newRow( "data31" ) << u"ABC"_s << u"b"_s << 0 << false << 1;
    QTest::newRow( "data32" ) << u"ABC"_s << u"B"_s << 1 << true << 1;
    QTest::newRow( "data33" ) << u"ABC"_s << u"B"_s << 1 << false << 1;
    QTest::newRow( "data34" ) << u"ABC"_s << u"b"_s << 1 << true << -1;
    QTest::newRow( "data35" ) << u"ABC"_s << u"b"_s << 1 << false << 1;
    QTest::newRow( "data36" ) << u"ABC"_s << u"B"_s << 2 << true << -1;
    QTest::newRow( "data37" ) << u"ABC"_s << u"B"_s << 2 << false << -1;

    QTest::newRow( "data40" ) << u"aBc"_s << u"bc"_s << 0 << true << -1;
    QTest::newRow( "data41" ) << u"aBc"_s << u"Bc"_s << 0 << true << 1;
    QTest::newRow( "data42" ) << u"aBc"_s << u"bC"_s << 0 << true << -1;
    QTest::newRow( "data43" ) << u"aBc"_s << u"BC"_s << 0 << true << -1;
    QTest::newRow( "data44" ) << u"aBc"_s << u"bc"_s << 0 << false << 1;
    QTest::newRow( "data45" ) << u"aBc"_s << u"Bc"_s << 0 << false << 1;
    QTest::newRow( "data46" ) << u"aBc"_s << u"bC"_s << 0 << false << 1;
    QTest::newRow( "data47" ) << u"aBc"_s << u"BC"_s << 0 << false << 1;
    QTest::newRow( "data48" ) << u"AbC"_s << u"bc"_s << 0 << true << -1;
    QTest::newRow( "data49" ) << u"AbC"_s << u"Bc"_s << 0 << true << -1;
    QTest::newRow( "data50" ) << u"AbC"_s << u"bC"_s << 0 << true << 1;
    QTest::newRow( "data51" ) << u"AbC"_s << u"BC"_s << 0 << true << -1;
    QTest::newRow( "data52" ) << u"AbC"_s << u"bc"_s << 0 << false << 1;
    QTest::newRow( "data53" ) << u"AbC"_s << u"Bc"_s << 0 << false << 1;

    QTest::newRow( "data54" ) << u"AbC"_s << u"bC"_s << 0 << false << 1;
    QTest::newRow( "data55" ) << u"AbC"_s << u"BC"_s << 0 << false << 1;
    QTest::newRow( "data56" ) << u"AbC"_s << u"BC"_s << 1 << false << 1;
    QTest::newRow( "data57" ) << u"AbC"_s << u"BC"_s << 2 << false << -1;

    QTest::newRow( "null-in-null") << QString() << QString() << 0 << false << 0;
    QTest::newRow( "empty-in-null") << QString() << u""_s << 0 << false << 0;
    QTest::newRow( "null-in-empty") << u""_s << QString() << 0 << false << 0;
    QTest::newRow( "empty-in-empty") << u""_s << u""_s << 0 << false << 0;
    QTest::newRow( "data-in-null") << QString() << u"a"_s << 0 << false << -1;
    QTest::newRow( "data-in-empty") << u""_s << u"a"_s << 0 << false << -1;


    QString s1 = u"abc"_s;
    s1 += QChar(0xb5);
    QString s2;
    s2 += QChar(0x3bc);
    QTest::newRow( "data58" ) << s1 << s2 << 0 << false << 3;
    s2.prepend(QLatin1Char('C'));
    QTest::newRow( "data59" ) << s1 << s2 << 0 << false << 2;

    QString veryBigHaystack(500, u'a');
    veryBigHaystack += u'B';
    QTest::newRow("BoyerMooreStressTest") << veryBigHaystack << veryBigHaystack << 0 << true << 0;
    QTest::newRow("BoyerMooreStressTest2") << QString(veryBigHaystack + u'c') << veryBigHaystack << 0 << true << 0;
    QTest::newRow("BoyerMooreStressTest3") << QString(u'c' + veryBigHaystack) << veryBigHaystack << 0 << true << 1;
    QTest::newRow("BoyerMooreStressTest4") << veryBigHaystack << QString(veryBigHaystack + u'c') << 0 << true << -1;
    QTest::newRow("BoyerMooreStressTest5") << veryBigHaystack << QString(u'c' + veryBigHaystack) << 0 << true << -1;
    QTest::newRow("BoyerMooreStressTest6") << QString(u'd' + veryBigHaystack) << QString(u'c' + veryBigHaystack) << 0 << true << -1;
    QTest::newRow("BoyerMooreStressTest7") << QString(veryBigHaystack + u'c') << QString(u'c' + veryBigHaystack) << 0 << true << -1;

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
#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && ! defined(QT_NO_CAST_FROM_ASCII)
        QCOMPARE( haystack.indexOf(needle.toLatin1(), startpos, cs), resultpos );
        QCOMPARE( haystack.indexOf(needle.toLatin1().data(), startpos, cs), resultpos );
#endif
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
#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
            QCOMPARE( haystack.indexOf(needle.toLatin1(), startpos), resultpos );
            QCOMPARE( haystack.indexOf(needle.toLatin1().data(), startpos), resultpos );
#endif
        }
        if (startpos == 0) {
            QCOMPARE( haystack.indexOf(needle), resultpos );
            QCOMPARE( haystack.indexOf(view), resultpos );
            if (needleIsLatin) {
#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
                QCOMPARE( haystack.indexOf(needle.toLatin1()), resultpos );
                QCOMPARE( haystack.indexOf(needle.toLatin1().data()), resultpos );
#endif
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
    QTest::newRow( "data1" ) << QString() << u""_s << 0;
    QTest::newRow( "data2" ) << u""_s << QString() << 0;
    QTest::newRow( "data3" ) << u""_s << u""_s << 0;
    QTest::newRow( "data4" ) << QString() << u"a"_s << -1;
    QTest::newRow( "data5" ) << QString() << u"abcdefg"_s << -1;
    QTest::newRow( "data6" ) << u""_s << u"a"_s << -1;
    QTest::newRow( "data7" ) << u""_s << u"abcdefg"_s << -1;

    QTest::newRow( "data8" ) << u"a"_s << QString() << 0;
    QTest::newRow( "data9" ) << u"a"_s << u""_s << 0;
    QTest::newRow( "data10" ) << u"a"_s << u"a"_s << 0;
    QTest::newRow( "data11" ) << u"a"_s << u"b"_s << -1;
    QTest::newRow( "data12" ) << u"a"_s << u"abcdefg"_s << -1;
    QTest::newRow( "data13" ) << u"ab"_s << QString() << 0;
    QTest::newRow( "data14" ) << u"ab"_s << u""_s << 0;
    QTest::newRow( "data15" ) << u"ab"_s << u"a"_s << 0;
    QTest::newRow( "data16" ) << u"ab"_s << u"b"_s << 1;
    QTest::newRow( "data17" ) << u"ab"_s << u"ab"_s << 0;
    QTest::newRow( "data18" ) << u"ab"_s << u"bc"_s << -1;
    QTest::newRow( "data19" ) << u"ab"_s << u"abcdefg"_s << -1;

    QTest::newRow( "data30" ) << u"abc"_s << u"a"_s << 0;
    QTest::newRow( "data31" ) << u"abc"_s << u"b"_s << 1;
    QTest::newRow( "data32" ) << u"abc"_s << u"c"_s << 2;
    QTest::newRow( "data33" ) << u"abc"_s << u"d"_s << -1;
    QTest::newRow( "data34" ) << u"abc"_s << u"ab"_s << 0;
    QTest::newRow( "data35" ) << u"abc"_s << u"bc"_s << 1;
    QTest::newRow( "data36" ) << u"abc"_s << u"cd"_s << -1;
    QTest::newRow( "data37" ) << u"abc"_s << u"ac"_s << -1;

    // sizeof(whale) > 32
    QString whale = u"a5zby6cx7dw8evf9ug0th1si2rj3qkp4lomn"_s;
    QString minnow = u"zby"_s;

    QTest::newRow( "data40" ) << whale << minnow << 2;
    QTest::newRow( "data41" ) << QString(whale + whale) << minnow << 2;
    QTest::newRow( "data42" ) << QString(minnow + whale) << minnow << 0;
    QTest::newRow( "data43" ) << whale << whale << 0;
    QTest::newRow( "data44" ) << QString(whale + whale) << whale << 0;
    QTest::newRow( "data45" ) << whale << QString(whale + whale) << -1;
    QTest::newRow( "data46" ) << QString(whale + whale) << QString(whale + whale) << 0;
    QTest::newRow( "data47" ) << QString(whale + whale) << QString(whale + minnow) << -1;
    QTest::newRow( "data48" ) << QString(minnow + whale) << whale << (int)minnow.size();
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
    if ( needle.size() > 0 ) {
        got = haystack.lastIndexOf( needle, -1, Qt::CaseSensitive );
        QVERIFY( got == resultpos || (resultpos >= 0 && got >= resultpos) );
        got = haystack.lastIndexOf( needle, -1, Qt::CaseInsensitive );
        QVERIFY( got == resultpos || (resultpos >= 0 && got >= resultpos) );
    }

    QCOMPARE( chaystack.indexOf(cneedle, 0), resultpos );
    QCOMPARE( QByteArrayMatcher(cneedle).indexIn(chaystack, 0), resultpos );
    if ( cneedle.size() > 0 ) {
        got = chaystack.lastIndexOf(cneedle, -1);
        QVERIFY( got == resultpos || (resultpos >= 0 && got >= resultpos) );
    }
}

#if QT_CONFIG(regularexpression)
void tst_QString::indexOfInvalidRegex()
{
    static const QRegularExpression ignoreMessagePattern(
        u"^QString\\(View\\)::indexOf\\(\\): called on an invalid QRegularExpression object"_s
    );

    QString str = u"invalid regex\\"_s;
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(str.indexOf(QRegularExpression(str)), -1);
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(str.indexOf(QRegularExpression(str), -1, nullptr), -1);

    QRegularExpressionMatch match;
    QVERIFY(!match.hasMatch());
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(str.indexOf(QRegularExpression(str), -1, &match), -1);
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

    QString a = u"ABCDEFGHIEfGEFG"_s;

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
    QTest::newRow("empty-in-null") << QString() << u""_s << 0 << 0 << false;
    QTest::newRow("null-in-empty") << u""_s << QString() << 0 << 0 << false;
    QTest::newRow("empty-in-empty") << u""_s << u""_s << 0 << 0 << false;
    QTest::newRow("data-in-null") << QString() << u"a"_s << 0 << -1 << false;
    QTest::newRow("data-in-empty") << u""_s << u"a"_s << 0 << -1 << false;
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
#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    QCOMPARE(haystack.lastIndexOf(needle.toLatin1(), from, cs), expected);
    QCOMPARE(haystack.lastIndexOf(needle.toLatin1().data(), from, cs), expected);
#endif

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
#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
        QCOMPARE(haystack.lastIndexOf(needle.toLatin1(), from), expected);
        QCOMPARE(haystack.lastIndexOf(needle.toLatin1().data(), from), expected);
#endif
        if (from == haystack.size()) {
            QCOMPARE(haystack.lastIndexOf(needle), expected);
            QCOMPARE(haystack.lastIndexOf(view), expected);
#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
            QCOMPARE(haystack.lastIndexOf(needle.toLatin1()), expected);
            QCOMPARE(haystack.lastIndexOf(needle.toLatin1().data()), expected);
#endif
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
    static const QRegularExpression ignoreMessagePattern(
        u"^QString\\(View\\)::lastIndexOf\\(\\): called on an invalid QRegularExpression object"_s
    );

    const QString str(u"invalid regex\\"_s);
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(str.lastIndexOf(QRegularExpression(str), 0), -1);
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(str.lastIndexOf(QRegularExpression(str), -1, nullptr), -1);

    QRegularExpressionMatch match;
    QVERIFY(!match.hasMatch());
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(str.lastIndexOf(QRegularExpression(str), -1, &match), -1);
    QVERIFY(!match.hasMatch());
}
#endif

void tst_QString::count()
{
    static const QRegularExpression ignoreMessagePattern(
        u"^QString\\(View\\)::count\\(\\): called on an invalid QRegularExpression object"_s
    );

    QString a(u"ABCDEFGHIEfGEFG"_s);
    QCOMPARE(a.size(), 15);

    QCOMPARE(a.count(QChar(u'A')), 1);
    QCOMPARE(a.count(QChar(u'Z')), 0);
    QCOMPARE(a.count(QChar(u'E')), 3);
    QCOMPARE(a.count(QChar(u'F')), 2);
    QCOMPARE(a.count(QChar(u'F'), Qt::CaseInsensitive), 3);

    QCOMPARE(a.count(QString(u"FG"_s)), 2);
    QCOMPARE(a.count(QString(u"FG"_s), Qt::CaseInsensitive), 3);

    QCOMPARE(a.count(QStringView(u"FG")), 2);
    QCOMPARE(a.count(QStringView(u"FG"), Qt::CaseInsensitive), 3);

    QCOMPARE(a.count(QLatin1StringView("FG")), 2);
    QCOMPARE(a.count(QLatin1StringView("FG"), Qt::CaseInsensitive), 3);

    QCOMPARE(a.count( QString(), Qt::CaseInsensitive), 16);
    QCOMPARE(a.count(QString(u""_s), Qt::CaseInsensitive), 16);
    QCOMPARE(a.count(QStringView(u""), Qt::CaseInsensitive), 16);
    QCOMPARE(a.count(QLatin1StringView(""), Qt::CaseInsensitive), 16);

#if QT_CONFIG(regularexpression)
    QCOMPARE(a.count(QRegularExpression(u""_s)), 16);
    QCOMPARE(a.count(QRegularExpression(u"[FG][HI]"_s)), 1);
    QCOMPARE(a.count(QRegularExpression(u"[G][HE]"_s)), 2);
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(a.count(QRegularExpression(u"invalid regex\\"_s)), 0);
#endif

    CREATE_VIEW(QLatin1String("FG"));
    QCOMPARE(a.count(view),2);
    QCOMPARE(a.count(view,Qt::CaseInsensitive),3);
    QCOMPARE(a.count( QStringView(), Qt::CaseInsensitive), 16);

    QString nullStr;
#if QT_DEPRECATED_SINCE(6, 4)
    QT_IGNORE_DEPRECATIONS(QCOMPARE(nullStr.size(), 0);)
#endif
    QCOMPARE(nullStr.count(QChar(u'A')), 0);
    QCOMPARE(nullStr.count(QString(u"AB"_s)), 0);
    QCOMPARE(nullStr.count(view), 0);
    QCOMPARE(nullStr.count(QLatin1StringView("AB")), 0);

    QCOMPARE(nullStr.count(QString()), 1);
    QCOMPARE(nullStr.count(QString(u""_s)), 1);
    QCOMPARE(nullStr.count(QStringView(u"")), 1);
    QCOMPARE(nullStr.count(QLatin1StringView("")), 1);

#if QT_CONFIG(regularexpression)
    QCOMPARE(nullStr.count(QRegularExpression(u""_s)), 1);
    QCOMPARE(nullStr.count(QRegularExpression(u"[FG][HI]"_s)), 0);
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(nullStr.count(QRegularExpression(u"invalid regex\\"_s)), 0);
#endif

    QString emptyStr(u""_s);
#if QT_DEPRECATED_SINCE(6, 4)
    QT_IGNORE_DEPRECATIONS(QCOMPARE(emptyStr.size(), 0);)
#endif
    QCOMPARE(emptyStr.count(u'A'), 0);
    QCOMPARE(emptyStr.count(QString(u"AB"_s)), 0);
    QCOMPARE(emptyStr.count(view), 0);
    QCOMPARE(emptyStr.count(QString()), 1);
    QCOMPARE(emptyStr.count(QStringView()), 1);
    QCOMPARE(emptyStr.count(QLatin1StringView()), 1);
    QCOMPARE(emptyStr.count(QString(u""_s)), 1);
    QCOMPARE(emptyStr.count(QStringView(u"")), 1);
    QCOMPARE(emptyStr.count(QLatin1StringView("")), 1);

#if QT_CONFIG(regularexpression)
    QCOMPARE(emptyStr.count(QRegularExpression(u""_s)), 1);
    QCOMPARE(emptyStr.count(QRegularExpression(u"[FG][HI]"_s)), 0);
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(emptyStr.count(QRegularExpression(u"invalid regex\\"_s)), 0);
#endif

    QString nonBmpString = u"\U00010000\U00010000abc\U00010000"_s;
    QCOMPARE(nonBmpString.count(u"\U00010000"), 3);
#if QT_CONFIG(regularexpression)
    QCOMPARE(nonBmpString.count(QRegularExpression(u"\U00010000"_s)), 3);
    QCOMPARE(nonBmpString.count(QRegularExpression(u"\U00010000a?"_s)), 3);
    QCOMPARE(nonBmpString.count(QRegularExpression(u"\U00010000a"_s)), 1);
    QCOMPARE(nonBmpString.count(QRegularExpression(u"."_s)), 6);

    // can't search for unpaired surrogates
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(nonBmpString.count(QRegularExpression(QChar(0xd800))), 0);
    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QCOMPARE(nonBmpString.count(QRegularExpression(QChar(0xdc00))), 0);
#endif // QT_CONFIG(regularexpression)
}

void tst_QString::contains()
{
    static const QRegularExpression ignoreMessagePattern(
        u"^QString\\(View\\)::contains\\(\\): called on an invalid QRegularExpression object"_s
    );

    QString a(u"ABCDEFGHIEfGEFG"_s);
    QCOMPARE(a.size(), 15);

    QVERIFY(a.contains(QChar(u'A')));
    QVERIFY(!a.contains(QChar(u'Z')));
    QVERIFY(a.contains(QChar(u'E')));
    QVERIFY(a.contains(QChar(u'F')));
    QVERIFY(a.contains(QChar(u'f'), Qt::CaseInsensitive));

    QVERIFY(a.contains(QString(u"FG"_s)));
    QVERIFY(a.contains(QString(u"FG"_s), Qt::CaseInsensitive));
    QVERIFY(a.contains(QStringView(u"FG")));
    QVERIFY(a.contains(QStringView(u"fg"), Qt::CaseInsensitive));

    QVERIFY(a.contains(QLatin1String("FG")));
    QVERIFY(a.contains(QLatin1String("fg"),Qt::CaseInsensitive));

    QVERIFY(a.contains(QString(), Qt::CaseInsensitive));
    QVERIFY(a.contains(QString(u""_s), Qt::CaseInsensitive));
    QVERIFY(a.contains(QStringView(), Qt::CaseInsensitive));
    QVERIFY(a.contains(QStringView(u""), Qt::CaseInsensitive));
    QVERIFY(a.contains(QLatin1StringView(), Qt::CaseInsensitive));
    QVERIFY(a.contains(QLatin1StringView(""), Qt::CaseInsensitive));

#if QT_CONFIG(regularexpression)
    QVERIFY(a.contains(QRegularExpression(u"[FG][HI]"_s)));
    QVERIFY(a.contains(QRegularExpression(u"[G][HE]"_s)));

    {
        QRegularExpressionMatch match;
        QVERIFY(!match.hasMatch());

        QVERIFY(a.contains(QRegularExpression(u"[FG][HI]"_s), &match));
        QVERIFY(match.hasMatch());
        QCOMPARE(match.capturedStart(), 6);
        QCOMPARE(match.capturedEnd(), 8);
        QCOMPARE(match.captured(), QStringLiteral("GH"));

        QVERIFY(a.contains(QRegularExpression(u"[G][HE]"_s), &match));
        QVERIFY(match.hasMatch());
        QCOMPARE(match.capturedStart(), 6);
        QCOMPARE(match.capturedEnd(), 8);
        QCOMPARE(match.captured(), QStringLiteral("GH"));

        QVERIFY(a.contains(QRegularExpression(u"[f](.*)[FG]"_s), &match));
        QVERIFY(match.hasMatch());
        QCOMPARE(match.capturedStart(), 10);
        QCOMPARE(match.capturedEnd(), 15);
        QCOMPARE(match.captured(), u"fGEFG");
        QCOMPARE(match.capturedStart(1), 11);
        QCOMPARE(match.capturedEnd(1), 14);
        QCOMPARE(match.captured(1), QStringLiteral("GEF"));

        QVERIFY(a.contains(QRegularExpression(u"[f](.*)[F]"_s), &match));
        QVERIFY(match.hasMatch());
        QCOMPARE(match.capturedStart(), 10);
        QCOMPARE(match.capturedEnd(), 14);
        QCOMPARE(match.captured(), u"fGEF");
        QCOMPARE(match.capturedStart(1), 11);
        QCOMPARE(match.capturedEnd(1), 13);
        QCOMPARE(match.captured(1), QStringLiteral("GE"));

        QVERIFY(!a.contains(QRegularExpression(u"ZZZ"_s), &match));
        // doesn't match, but ensure match didn't change
        QVERIFY(match.hasMatch());
        QCOMPARE(match.capturedStart(), 10);
        QCOMPARE(match.capturedEnd(), 14);
        QCOMPARE(match.captured(), QStringLiteral("fGEF"));
        QCOMPARE(match.capturedStart(1), 11);
        QCOMPARE(match.capturedEnd(1), 13);
        QCOMPARE(match.captured(1), QStringLiteral("GE"));

        // don't crash with a null pointer
        QVERIFY(a.contains(QRegularExpression(u"[FG][HI]"_s), 0));
        QVERIFY(!a.contains(QRegularExpression(u"ZZZ"_s), 0));
    }

    QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QVERIFY(!a.contains(QRegularExpression(u"invalid regex\\"_s)));
#endif

    CREATE_VIEW(QLatin1String("FG"));
    QVERIFY(a.contains(view));
    QVERIFY(a.contains(view, Qt::CaseInsensitive));
    QVERIFY(a.contains( QStringView(), Qt::CaseInsensitive));

    QString nullStr;
    QVERIFY(!nullStr.contains(u'A'));
    QVERIFY(!nullStr.contains(QString(u"AB"_s)));
    QVERIFY(!nullStr.contains(QLatin1StringView("AB")));
    QVERIFY(!nullStr.contains(view));
#if QT_CONFIG(regularexpression)
    QVERIFY(!nullStr.contains(QRegularExpression(u"[FG][HI]"_s)));
    QRegularExpressionMatch nullMatch;
    QVERIFY(nullStr.contains(QRegularExpression(u""_s), &nullMatch));
    QVERIFY(nullMatch.hasMatch());
    QCOMPARE(nullMatch.captured(), u"");
    QCOMPARE(nullMatch.capturedStart(), 0);
    QCOMPARE(nullMatch.capturedEnd(), 0);
#endif
    QVERIFY(!nullStr.isDetached());

    QString emptyStr(u""_s);
    QVERIFY(!emptyStr.contains(u'A'));
    QVERIFY(!emptyStr.contains(QString(u"AB"_s)));
    QVERIFY(!emptyStr.contains(QLatin1StringView("AB")));
    QVERIFY(!emptyStr.contains(view));
#if QT_CONFIG(regularexpression)
    QVERIFY(!emptyStr.contains(QRegularExpression(u"[FG][HI]"_s)));
    QRegularExpressionMatch emptyMatch;
    QVERIFY(emptyStr.contains(QRegularExpression(u""_s), &emptyMatch));
    QVERIFY(emptyMatch.hasMatch());
    QCOMPARE(emptyMatch.captured(), u"");
    QCOMPARE(emptyMatch.capturedStart(), 0);
    QCOMPARE(emptyMatch.capturedEnd(), 0);
#endif
    QVERIFY(!emptyStr.isDetached());
}


void tst_QString::left()
{
    QString a;

    // lvalue
    QVERIFY(a.left(0).isNull());
    QVERIFY(a.left(5).isNull());
    QVERIFY(a.left(-4).isNull());
    QVERIFY(!a.isDetached());

    // rvalue, not detached
    QVERIFY(QString(a).left(0).isNull());
    QVERIFY(QString(a).left(5).isNull());
    QVERIFY(QString(a).left(-4).isNull());
    QVERIFY(!QString(a).isDetached());

    // rvalue, detached is not applicable

    a = u"ABCDEFGHIEfGEFG"_s;
    QCOMPARE(a.size(), 15);

    // lvalue
    QCOMPARE(a.left(3), QLatin1String("ABC"));
    QVERIFY(!a.left(0).isNull());
    QCOMPARE(a.left(0), QLatin1String(""));
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, not detached
    QCOMPARE(QString(a).left(3), QLatin1String("ABC"));
    QVERIFY(!QString(a).left(0).isNull());
    QCOMPARE(QString(a).left(0), QLatin1String(""));
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, detached
    QCOMPARE(detached(a).left(3), QLatin1String("ABC"));
    QVERIFY(!detached(a).left(0).isNull());
    QCOMPARE(detached(a).left(0), QLatin1String(""));
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    QString n;
    QVERIFY(QString().left(3).isNull());
    QVERIFY(QString().left(0).isNull());
    QVERIFY(QString().left(0).isNull());
    QVERIFY(n.left(3).isNull());
    QVERIFY(n.left(0).isNull());
    QVERIFY(n.left(0).isNull());

    QString l = u"Left"_s;

    // lvalue
    QCOMPARE(l.left(-1), l);
    QCOMPARE(l.left(100), l);
    QCOMPARE(l, u"Left");

    // rvalue, not detached
    QCOMPARE(QString(l).left(-1), l);
    QCOMPARE(QString(l).left(100), l);
    QCOMPARE(l, u"Left");

    // rvalue, detached
    QCOMPARE(detached(l).left(-1), l);
    QCOMPARE(detached(l).left(100), l);
    QCOMPARE(l, u"Left");
}

void tst_QString::right()
{
    QString a;

    // lvalue
    QVERIFY(a.right(0).isNull());
    QVERIFY(a.right(5).isNull());
    QVERIFY(a.right(-4).isNull());
    QVERIFY(!a.isDetached());

    // rvalue, not detached
    QVERIFY(QString(a).right(0).isNull());
    QVERIFY(QString(a).right(5).isNull());
    QVERIFY(QString(a).right(-4).isNull());
    QVERIFY(!QString(a).isDetached());

    // rvalue, detached is not applicable

    a = u"ABCDEFGHIEfGEFG"_s;
    QCOMPARE(a.size(), 15);

    // lvalue
    QCOMPARE(a.right(3), QLatin1String("EFG"));
    QCOMPARE(a.right(0), QLatin1String(""));
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, not detached
    QCOMPARE(QString(a).right(3), QLatin1String("EFG"));
    QCOMPARE(QString(a).right(0), QLatin1String(""));
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, detached
    QCOMPARE(detached(a).right(3), QLatin1String("EFG"));
    QCOMPARE(detached(a).right(0), QLatin1String(""));
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    QString n;
    QVERIFY(QString().right(3).isNull());
    QVERIFY(QString().right(0).isNull());
    QVERIFY(n.right(3).isNull());
    QVERIFY(n.right(0).isNull());

    QString r = u"Right"_s;

    // lvalue
    QCOMPARE(r.right(-1), r);
    QCOMPARE(r.right(100), r);
    QCOMPARE(r, u"Right");

    // rvalue, not detached
    QCOMPARE(QString(r).right(-1), r);
    QCOMPARE(QString(r).right(100), r);
    QCOMPARE(r, u"Right");

    // rvalue, detached
    QCOMPARE(detached(r).right(-1), r);
    QCOMPARE(detached(r).right(100), r);
    QCOMPARE(r, u"Right");
}

void tst_QString::mid()
{
    QString a;

    QVERIFY(a.mid(0).isNull());
    QVERIFY(a.mid(5, 6).isNull());
    QVERIFY(a.mid(-4, 3).isNull());
    QVERIFY(a.mid(4, -3).isNull());
    QVERIFY(!a.isDetached());

    a = u"ABCDEFGHIEfGEFG"_s;
    QCOMPARE(a.size(), 15);

    // lvalue
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
    QCOMPARE(a.mid(1, INT_MAX), u"BCDEFGHIEfGEFG");
    QCOMPARE(a.mid(5, INT_MAX), u"FGHIEfGEFG");
    QVERIFY(a.mid(20, INT_MAX).isNull());
    QCOMPARE(a.mid(-1, -1), a);

    // rvalue, not detached
    QCOMPARE(QString(a).mid(3,3), QLatin1String("DEF"));
    QCOMPARE(QString(a).mid(0,0), QLatin1String(""));
    QVERIFY(!QString(a).mid(15,0).isNull());
    QVERIFY(QString(a).mid(15,0).isEmpty());
    QVERIFY(!QString(a).mid(15,1).isNull());
    QVERIFY(QString(a).mid(15,1).isEmpty());
    QVERIFY(QString(a).mid(9999).isNull());
    QVERIFY(QString(a).mid(9999,1).isNull());
    QCOMPARE(QString(a).mid(-1, 6), QString(a).mid(0, 5));
    QVERIFY(QString(a).mid(-100, 6).isEmpty());
    QVERIFY(QString(a).mid(INT_MIN, 0).isEmpty());
    QCOMPARE(QString(a).mid(INT_MIN, -1), a);
    QVERIFY(QString(a).mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(QString(a).mid(INT_MIN + 1, INT_MAX).isEmpty());
    QCOMPARE(QString(a).mid(INT_MIN + 2, INT_MAX), a.left(1));
    QCOMPARE(QString(a).mid(INT_MIN + a.size() + 1, INT_MAX), a);
    QVERIFY(QString(a).mid(INT_MAX).isNull());
    QVERIFY(QString(a).mid(INT_MAX, INT_MAX).isNull());
    QCOMPARE(QString(a).mid(-5, INT_MAX), a);
    QCOMPARE(QString(a).mid(-1, INT_MAX), a);
    QCOMPARE(QString(a).mid(0, INT_MAX), a);
    QCOMPARE(QString(a).mid(1, INT_MAX), u"BCDEFGHIEfGEFG");
    QCOMPARE(QString(a).mid(5, INT_MAX), u"FGHIEfGEFG");
    QVERIFY(QString(a).mid(20, INT_MAX).isNull());
    QCOMPARE(QString(a).mid(-1, -1), a);

    // rvalue, detached
    QCOMPARE(detached(a).mid(3,3), QLatin1String("DEF"));
    QCOMPARE(detached(a).mid(0,0), QLatin1String(""));
    QVERIFY(!detached(a).mid(15,0).isNull());
    QVERIFY(detached(a).mid(15,0).isEmpty());
    QVERIFY(!detached(a).mid(15,1).isNull());
    QVERIFY(detached(a).mid(15,1).isEmpty());
    QVERIFY(detached(a).mid(9999).isNull());
    QVERIFY(detached(a).mid(9999,1).isNull());
    QCOMPARE(detached(a).mid(-1, 6), detached(a).mid(0, 5));
    QVERIFY(detached(a).mid(-100, 6).isEmpty());
    QVERIFY(detached(a).mid(INT_MIN, 0).isEmpty());
    QCOMPARE(detached(a).mid(INT_MIN, -1), a);
    QVERIFY(detached(a).mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(detached(a).mid(INT_MIN + 1, INT_MAX).isEmpty());
    QCOMPARE(detached(a).mid(INT_MIN + 2, INT_MAX), a.left(1));
    QCOMPARE(detached(a).mid(INT_MIN + a.size() + 1, INT_MAX), a);
    QVERIFY(detached(a).mid(INT_MAX).isNull());
    QVERIFY(detached(a).mid(INT_MAX, INT_MAX).isNull());
    QCOMPARE(detached(a).mid(-5, INT_MAX), a);
    QCOMPARE(detached(a).mid(-1, INT_MAX), a);
    QCOMPARE(detached(a).mid(0, INT_MAX), a);
    QCOMPARE(detached(a).mid(1, INT_MAX), u"BCDEFGHIEfGEFG");
    QCOMPARE(detached(a).mid(5, INT_MAX), u"FGHIEfGEFG");
    QVERIFY(detached(a).mid(20, INT_MAX).isNull());
    QCOMPARE(detached(a).mid(-1, -1), a);

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

    QVERIFY(QString().mid(3,3).isNull());
    QVERIFY(QString().mid(0,0).isNull());
    QVERIFY(QString().mid(9999,0).isNull());
    QVERIFY(QString().mid(9999,1).isNull());
    QVERIFY(QString().mid(-1, 6).isNull());
    QVERIFY(QString().mid(-100, 6).isNull());
    QVERIFY(QString().mid(INT_MIN, 0).isNull());
    QVERIFY(QString().mid(INT_MIN, -1).isNull());
    QVERIFY(QString().mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(QString().mid(INT_MIN + 1, INT_MAX).isNull());
    QVERIFY(QString().mid(INT_MIN + 2, INT_MAX).isNull());
    QVERIFY(QString().mid(INT_MIN + QString().size() + 1, INT_MAX).isNull());
    QVERIFY(QString().mid(INT_MAX).isNull());
    QVERIFY(QString().mid(INT_MAX, INT_MAX).isNull());
    QVERIFY(QString().mid(-5, INT_MAX).isNull());
    QVERIFY(QString().mid(-1, INT_MAX).isNull());
    QVERIFY(QString().mid(0, INT_MAX).isNull());
    QVERIFY(QString().mid(1, INT_MAX).isNull());
    QVERIFY(QString().mid(5, INT_MAX).isNull());
    QVERIFY(QString().mid(20, INT_MAX).isNull());
    QVERIFY(QString().mid(-1, -1).isNull());

    QString x = u"Nine pineapples"_s;
    QCOMPARE(x.mid(5, 4), u"pine");
    QCOMPARE(x.mid(5), u"pineapples");
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
    QCOMPARE(x.mid(1, INT_MAX), u"ine pineapples");
    QCOMPARE(x.mid(5, INT_MAX), u"pineapples");
    QVERIFY(x.mid(20, INT_MAX).isNull());
    QCOMPARE(x.mid(-1, -1), x);
    QCOMPARE(x, u"Nine pineapples");

    // rvalue, not detached
    QCOMPARE(QString(x).mid(5, 4), u"pine");
    QCOMPARE(QString(x).mid(5), u"pineapples");
    QCOMPARE(QString(x).mid(-1, 6), QString(x).mid(0, 5));
    QVERIFY(QString(x).mid(-100, 6).isEmpty());
    QVERIFY(QString(x).mid(INT_MIN, 0).isEmpty());
    QCOMPARE(QString(x).mid(INT_MIN, -1), x);
    QVERIFY(QString(x).mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(QString(x).mid(INT_MIN + 1, INT_MAX).isEmpty());
    QCOMPARE(QString(x).mid(INT_MIN + 2, INT_MAX), x.left(1));
    QCOMPARE(QString(x).mid(INT_MIN + x.size() + 1, INT_MAX), x);
    QVERIFY(QString(x).mid(INT_MAX).isNull());
    QVERIFY(QString(x).mid(INT_MAX, INT_MAX).isNull());
    QCOMPARE(QString(x).mid(-5, INT_MAX), x);
    QCOMPARE(QString(x).mid(-1, INT_MAX), x);
    QCOMPARE(QString(x).mid(0, INT_MAX), x);
    QCOMPARE(QString(x).mid(1, INT_MAX), u"ine pineapples");
    QCOMPARE(QString(x).mid(5, INT_MAX), u"pineapples");
    QVERIFY(QString(x).mid(20, INT_MAX).isNull());
    QCOMPARE(QString(x).mid(-1, -1), x);
    QCOMPARE(x, u"Nine pineapples");

    // rvalue, detached
    QCOMPARE(detached(x).mid(5, 4), u"pine");
    QCOMPARE(detached(x).mid(5), u"pineapples");
    QCOMPARE(detached(x).mid(-1, 6), detached(x).mid(0, 5));
    QVERIFY(detached(x).mid(-100, 6).isEmpty());
    QVERIFY(detached(x).mid(INT_MIN, 0).isEmpty());
    QCOMPARE(detached(x).mid(INT_MIN, -1), x);
    QVERIFY(detached(x).mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(detached(x).mid(INT_MIN + 1, INT_MAX).isEmpty());
    QCOMPARE(detached(x).mid(INT_MIN + 2, INT_MAX), x.left(1));
    QCOMPARE(detached(x).mid(INT_MIN + x.size() + 1, INT_MAX), x);
    QVERIFY(detached(x).mid(INT_MAX).isNull());
    QVERIFY(detached(x).mid(INT_MAX, INT_MAX).isNull());
    QCOMPARE(detached(x).mid(-5, INT_MAX), x);
    QCOMPARE(detached(x).mid(-1, INT_MAX), x);
    QCOMPARE(detached(x).mid(0, INT_MAX), x);
    QCOMPARE(detached(x).mid(1, INT_MAX), u"ine pineapples");
    QCOMPARE(detached(x).mid(5, INT_MAX), u"pineapples");
    QVERIFY(detached(x).mid(20, INT_MAX).isNull());
    QCOMPARE(detached(x).mid(-1, -1), x);
    QCOMPARE(x, u"Nine pineapples");
}

void tst_QString::leftJustified()
{
    QString a;

    QCOMPARE(a.leftJustified(3, u'-'), "---"_L1);
    QCOMPARE(a.leftJustified(2), QLatin1String("  "));
    QVERIFY(!a.isDetached());

    a= u"ABC"_s;
    QCOMPARE(a.leftJustified(5, u'-'), "ABC--"_L1);
    QCOMPARE(a.leftJustified(4, u'-'), "ABC-"_L1);
    QCOMPARE(a.leftJustified(4), QLatin1String("ABC "));
    QCOMPARE(a.leftJustified(3), QLatin1String("ABC"));
    QCOMPARE(a.leftJustified(2), QLatin1String("ABC"));
    QCOMPARE(a.leftJustified(1), QLatin1String("ABC"));
    QCOMPARE(a.leftJustified(0), QLatin1String("ABC"));

    QCOMPARE(a.leftJustified(4, u' ', true), "ABC "_L1);
    QCOMPARE(a.leftJustified(3, u' ', true), "ABC"_L1);
    QCOMPARE(a.leftJustified(2, u' ', true), "AB"_L1);
    QCOMPARE(a.leftJustified(1, u' ', true), "A"_L1);
    QCOMPARE(a.leftJustified(0, u' ', true), ""_L1);
}

void tst_QString::rightJustified()
{
    QString a;

    QCOMPARE(a.rightJustified(3, u'-'), "---"_L1);
    QCOMPARE(a.rightJustified(2), QLatin1String("  "));
    QVERIFY(!a.isDetached());

    a = u"ABC"_s;
    QCOMPARE(a.rightJustified(5, u'-'), "--ABC"_L1);
    QCOMPARE(a.rightJustified(4, u'-'), "-ABC"_L1);
    QCOMPARE(a.rightJustified(4), QLatin1String(" ABC"));
    QCOMPARE(a.rightJustified(3), QLatin1String("ABC"));
    QCOMPARE(a.rightJustified(2), QLatin1String("ABC"));
    QCOMPARE(a.rightJustified(1), QLatin1String("ABC"));
    QCOMPARE(a.rightJustified(0), QLatin1String("ABC"));

    QCOMPARE(a.rightJustified(4, u'-', true), "-ABC"_L1);
    QCOMPARE(a.rightJustified(4, u' ', true), " ABC"_L1);
    QCOMPARE(a.rightJustified(3, u' ', true), "ABC"_L1);
    QCOMPARE(a.rightJustified(2, u' ', true), "AB"_L1);
    QCOMPARE(a.rightJustified(1, u' ', true), "A"_L1);
    QCOMPARE(a.rightJustified(0, u' ', true), ""_L1);
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
    QCOMPARE(QString(u""_s).toUpper(), QString(u""_s));

    const QString TEXT(u"TEXT"_s);
    QCOMPARE(QStringLiteral("text").toUpper(), TEXT);
    QCOMPARE(QString(u"text"_s).toUpper(), TEXT);
    QCOMPARE(QString(u"Text"_s).toUpper(), TEXT);
    QCOMPARE(QString(u"tExt"_s).toUpper(), TEXT);
    QCOMPARE(QString(u"teXt"_s).toUpper(), TEXT);
    QCOMPARE(QString(u"texT"_s).toUpper(), TEXT);
    QCOMPARE(QString(u"TExt"_s).toUpper(), TEXT);
    QCOMPARE(QString(u"teXT"_s).toUpper(), TEXT);
    QCOMPARE(QString(u"tEXt"_s).toUpper(), TEXT);
    QCOMPARE(QString(u"tExT"_s).toUpper(), TEXT);
    QCOMPARE(TEXT.toUpper(), TEXT);
    QCOMPARE(QString(u"@ABYZ["_s).toUpper(), u"@ABYZ[");
    QCOMPARE(QString(u"@abyz["_s).toUpper(), u"@ABYZ[");
    QCOMPARE(QString(u"`ABYZ{"_s).toUpper(), u"`ABYZ{");
    QCOMPARE(QString(u"`abyz{"_s).toUpper(), u"`ABYZ{");

    QCOMPARE(QString(1, QChar(0xdf)).toUpper(), u"SS");
    {
        QString s = QString::fromUtf8("Gro\xc3\x9fstra\xc3\x9f""e");

        // call lvalue-ref version, mustn't change the original
        QCOMPARE(s.toUpper(), u"GROSSSTRASSE");
        QCOMPARE(s, QString::fromUtf8("Gro\xc3\x9fstra\xc3\x9f""e"));

        // call rvalue-ref while shared (the original mustn't change)
        QString copy = s;
        QCOMPARE(std::move(copy).toUpper(), u"GROSSSTRASSE");
        QCOMPARE(s, QString::fromUtf8("Gro\xc3\x9fstra\xc3\x9f""e"));

        // call rvalue-ref version on detached case
        copy.clear();
        QCOMPARE(std::move(s).toUpper(), u"GROSSSTRASSE");
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

    for (int i = 0; i < 65536; ++i) {
        QString str(1, QChar(i));
        QString upper = str.toUpper();
        QVERIFY(upper.size() >= 1);
        if (upper.size() == 1)
            QVERIFY(upper == QString(1, QChar(i).toUpper()));
    }
}

void tst_QString::toLower()
{
    const QString s;
    QCOMPARE(s.toLower(), QString()); // lvalue
    QCOMPARE( QString().toLower(), QString() ); // rvalue
    QCOMPARE(QString(u""_s).toLower(), u"");

    const QString lowerText(u"text"_s);
    QCOMPARE(lowerText.toLower(), lowerText);
    QCOMPARE(QStringLiteral("Text").toLower(), lowerText);
    QCOMPARE(QString(u"Text"_s).toLower(), lowerText);
    QCOMPARE(QString(u"tExt"_s).toLower(), lowerText);
    QCOMPARE(QString(u"teXt"_s).toLower(), lowerText);
    QCOMPARE(QString(u"texT"_s).toLower(), lowerText);
    QCOMPARE(QString(u"TExt"_s).toLower(), lowerText);
    QCOMPARE(QString(u"teXT"_s).toLower(), lowerText);
    QCOMPARE(QString(u"tEXt"_s).toLower(), lowerText);
    QCOMPARE(QString(u"tExT"_s).toLower(), lowerText);
    QCOMPARE(QString(u"TEXT"_s).toLower(), lowerText);
    QCOMPARE(QString(u"@ABYZ["_s).toLower(), u"@abyz[");
    QCOMPARE(QString(u"@abyz["_s).toLower(), u"@abyz[");
    QCOMPARE(QString(u"`ABYZ{"_s).toLower(), u"`abyz{");
    QCOMPARE(QString(u"`abyz{"_s).toLower(), u"`abyz{");

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

    for (int i = 0; i < 65536; ++i) {
        QString str(1, QChar(i));
        QString lower = str.toLower();
        QVERIFY(lower.size() >= 1);
        if (lower.size() == 1)
            QVERIFY(str.toLower() == QString(1, QChar(i).toLower()));
    }
}

void tst_QString::isLower_isUpper_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<bool>("isLower");
    QTest::addColumn<bool>("isUpper");

    int row = 0;
    QTest::addRow("lower-and-upper-%02d", row++) << QString() << true << true;
    QTest::addRow("lower-and-upper-%02d", row++) << u""_s << true << true;
    QTest::addRow("lower-and-upper-%02d", row++) << u" "_s << true << true;
    QTest::addRow("lower-and-upper-%02d", row++) << u"123"_s << true << true;
    QTest::addRow("lower-and-upper-%02d", row++) << u"@123$#"_s << true << true;
    QTest::addRow("lower-and-upper-%02d", row++) << QString::fromUtf8("𝄞𝄴𝆏♫") << true << true; // Unicode Block 'Musical Symbols'
    // not foldable
    QTest::addRow("lower-and-upper-%02d", row++) << u"𝚊𝚋𝚌𝚍𝚎"_s << true << true; // MATHEMATICAL MONOSPACE SMALL A, ... E
    QTest::addRow("lower-and-upper-%02d", row++) << u"𝙖,𝙗,𝙘,𝙙,𝙚"_s << true << true; // MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL A, ... E
    QTest::addRow("lower-and-upper-%02d", row++) << u"𝗔𝗕𝗖𝗗𝗘"_s << true << true; // MATHEMATICAL SANS-SERIF BOLD CAPITAL A, ... E
    QTest::addRow("lower-and-upper-%02d", row++) << u"𝐀,𝐁,𝐂,𝐃,𝐄"_s << true << true; // MATHEMATICAL BOLD CAPITAL A, ... E

    row = 0;
    QTest::addRow("only-lower-%02d", row++) << u"text"_s << true << false;
    QTest::addRow("only-lower-%02d", row++) << QString::fromUtf8("àaa") << true << false;
    QTest::addRow("only-lower-%02d", row++) << QString::fromUtf8("øæß") << true << false;
    QTest::addRow("only-lower-%02d", row++) << u"text "_s << true << false;
    QTest::addRow("only-lower-%02d", row++) << u" text"_s << true << false;
    QTest::addRow("only-lower-%02d", row++) << u"hello, world!"_s << true << false;
    QTest::addRow("only-lower-%02d", row++) << u"123@abyz["_s << true << false;
    QTest::addRow("only-lower-%02d", row++) << u"`abyz{"_s << true << false;
    // MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL A, ... E
    QTest::addRow("only-lower-%02d", row++) << u"a𝙖a|b𝙗b|c𝙘c|d𝙙d|e𝙚e"_s << true << false;
    // DESERET SMALL LETTER LONG I
    QTest::addRow("only-lower-%02d", row++) << u"𐐨"_s << true << false;
    // uppercase letters, not foldable
    // MATHEMATICAL SANS-SERIF BOLD CAPITAL A
    QTest::addRow("only-lower-%02d", row++) << u"text𝗔text"_s << true << false;

    row = 0;
    QTest::addRow("only-upper-%02d", row++) << u"TEXT"_s << false << true;
    QTest::addRow("only-upper-%02d", row++) << u"ÀAA"_s << false << true;
    QTest::addRow("only-upper-%02d", row++) << u"ØÆẞ"_s << false << true;
    QTest::addRow("only-upper-%02d", row++) << u"TEXT "_s << false << true;
    QTest::addRow("only-upper-%02d", row++) << u" TEXT"_s << false << true;
    QTest::addRow("only-upper-%02d", row++) << u"HELLO, WORLD!"_s << false << true;
    QTest::addRow("only-upper-%02d", row++) << u"123@ABYZ["_s << false << true;
    QTest::addRow("only-upper-%02d", row++) << u"`ABYZ{"_s << false << true;
    // MATHEMATICAL BOLD CAPITAL A, ... E
    QTest::addRow("only-upper-%02d", row++) << u"A𝐀A|B𝐁B|C𝐂C|D𝐃D|E𝐄E"_s << false << true;
    // DESERET CAPITAL LETTER LONG I
    QTest::addRow("only-upper-%02d", row++) << u"𐐀"_s << false << true;
    // lowercase letters, not foldable
    // MATHEMATICAL MONOSPACE SMALL A
    QTest::addRow("only-upper-%02d", row++) << u"TEXT𝚊TEXT"_s << false << true;

    row = 0;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"Text"_s << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"tExt"_s << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"teXt"_s << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"texT"_s << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"TExt"_s << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"teXT"_s << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"tEXt"_s << false << false;
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"tExT"_s << false << false;
    // not foldable
    // MATHEMATICAL MONOSPACE SMALL A
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"TEXT𝚊text"_s << false << false;
    // MATHEMATICAL SANS-SERIF BOLD CAPITAL A
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"text𝗔TEXT"_s << false << false;
    // titlecase, foldable
    // LATIN CAPITAL LETTER L WITH SMALL LETTER J
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"abcǈdef"_s << false << false;
    // LATIN CAPITAL LETTER L WITH SMALL LETTER J
    QTest::addRow("not-lower-nor-upper-%02d", row++) << u"ABCǈDEF"_s << false << false;
}

void tst_QString::isLower_isUpper()
{
    QFETCH(QString, string);
    QFETCH(bool, isLower);
    QFETCH(bool, isUpper);

    QCOMPARE(string.isLower(), isLower);
    QCOMPARE(QStringView(string).isLower(), isLower);
    QCOMPARE(string.toLower() == string, isLower);
    QVERIFY(string.toLower().isLower());

    QCOMPARE(string.isUpper(), isUpper);
    QCOMPARE(QStringView(string).isUpper(), isUpper);
    QCOMPARE(string.toUpper() == string, isUpper);
    QVERIFY(string.toUpper().isUpper());
}

void tst_QString::toCaseFolded()
{
    const QString s;
    QCOMPARE( s.toCaseFolded(), QString() ); // lvalue
    QCOMPARE( QString().toCaseFolded(), QString() ); // rvalue
    QCOMPARE(QString(u""_s).toCaseFolded(), u"");

    const QString lowerText(u"text"_s);
    QCOMPARE(lowerText.toCaseFolded(), lowerText);
    QCOMPARE(QString(u"Text"_s).toCaseFolded(), lowerText);
    QCOMPARE(QString(u"tExt"_s).toCaseFolded(), lowerText);
    QCOMPARE(QString(u"teXt"_s).toCaseFolded(), lowerText);
    QCOMPARE(QString(u"texT"_s).toCaseFolded(), lowerText);
    QCOMPARE(QString(u"TExt"_s).toCaseFolded(), lowerText);
    QCOMPARE(QString(u"teXT"_s).toCaseFolded(), lowerText);
    QCOMPARE(QString(u"tEXt"_s).toCaseFolded(), lowerText);
    QCOMPARE(QString(u"tExT"_s).toCaseFolded(), lowerText);
    QCOMPARE(QString(u"TEXT"_s).toCaseFolded(), lowerText);
    QCOMPARE(QString(u"@ABYZ["_s).toCaseFolded(), u"@abyz[");
    QCOMPARE(QString(u"@abyz["_s).toCaseFolded(), u"@abyz[");
    QCOMPARE(QString(u"`ABYZ{"_s).toCaseFolded(), u"`abyz{");
    QCOMPARE(QString(u"`abyz{"_s).toCaseFolded(), u"`abyz{");

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
        QVERIFY(lower.size() >= 1);
        if (lower.size() == 1)
            QVERIFY(str.toCaseFolded() == QString(1, QChar(i).toCaseFolded()));
    }
}

void tst_QString::trimmed_data()
{
    QTest::addColumn<QString>("full" );
    QTest::addColumn<QString>("trimmed" );

    QTest::addRow("null") << QString() << QString();
    QTest::addRow("simple") << u"Text"_s << u"Text"_s;
    QTest::addRow("single-space") << u" "_s << u""_s;
    QTest::addRow("single-char") << u" a   "_s << u"a"_s;
    QTest::addRow("mixed") << u" a \t\n\v b  "_s << u"a \t\n\v b"_s;
}

void tst_QString::trimmed()
{
    QFETCH(QString, full);
    QFETCH(QString, trimmed);

    // Shared
    if (!full.isNull())
        QVERIFY(!full.isDetached());
    QCOMPARE(full.trimmed(), trimmed); // lvalue
    QCOMPARE(QString(full).trimmed(), trimmed); // rvalue
    QCOMPARE(full.isNull(), trimmed.isNull());

    // Not shared
    full = QStringView(full).toString();
    if (!full.isNull())
        QVERIFY(full.isDetached());
    QCOMPARE(full.trimmed(), trimmed); // lvalue
    QCOMPARE(QString(full).trimmed(), trimmed); // rvalue
    QCOMPARE(full.isNull(), trimmed.isNull());
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
        QVERIFY2(result.isNull(), qPrintable("'"_L1 + full + "' did not yield null: "_L1 + result));
    } else if (simple.isEmpty()) {
        QVERIFY2(result.isEmpty() && !result.isNull(), qPrintable("'"_L1 + full + "' did not yield empty: "_L1 + result));
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
    const QString empty(u""_s);
    const QString a(u'a');
    const QString b(u'b');
    const QString ab(u"ab"_s);
    const QString ba(u"ba"_s);

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
    QTest::newRow("ba.insert(10, b)") << ba << bC << 10 << (ba + QString(10 - ba.size(), u' ') + b);

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
    QString dummy_share;

    {
        // Test when string is not shared
        a = u"Ys"_s;
        QCOMPARE(a.insert(1, u'e'), u"Yes");
        QCOMPARE(a.insert(3, u'!'), u"Yes!");
        QCOMPARE(a.insert(5, u'?'), u"Yes! ?");
        QCOMPARE(a.insert(-1, u'a'), u"Yes! a?");
    }
    {
        // Test when string is shared
        a = u"Ys"_s;
        dummy_share = a;
        QCOMPARE(a.insert(1, u'e'), u"Yes");
        dummy_share = a;
        QCOMPARE(a.insert(3, u'!'), u"Yes!");
        dummy_share = a;
        QCOMPARE(a.insert(5, u'?'), u"Yes! ?");
        dummy_share = a;
        QCOMPARE(a.insert(-1, u'a'), u"Yes! a?");
    }

    a = u"ABC"_s;
    dummy_share = a;
    QCOMPARE(dummy_share.insert(5, u"DEF"_s), u"ABC  DEF"_s); // Shared
    QCOMPARE(a.insert(5, u"DEF"_s), u"ABC  DEF"_s); // Not shared after dummy_shared.insert()

    {
        // Test when string is not shared
        a = u"ABC"_s;
        QCOMPARE(a.insert(2, QString()), u"ABC");
        QCOMPARE(a.insert(0, u"ABC"_s), u"ABCABC");
        QCOMPARE(a, u"ABCABC");
        QCOMPARE(a.insert(0, a), u"ABCABCABCABC");

        QCOMPARE(a, u"ABCABCABCABC");
        QCOMPARE(a.insert(0, u'<'), u"<ABCABCABCABC");
        QCOMPARE(a.insert(1, u'>'), u"<>ABCABCABCABC");
    }
    {
        // Test when string is shared
        a = u"ABC"_s;
        dummy_share = a;
        QCOMPARE(a.insert(2, QString()), u"ABC");
        dummy_share = a;
        QCOMPARE(a.insert(0, u"ABC"_s), u"ABCABC");
        dummy_share = a;
        QCOMPARE(a, u"ABCABC");
        dummy_share = a;
        QCOMPARE(a.insert(0, a), u"ABCABCABCABC");

        QCOMPARE(a, u"ABCABCABCABC");
        dummy_share = a;
        QCOMPARE(a.insert(0, u'<'), u"<ABCABCABCABC");
        dummy_share = a;
        QCOMPARE(a.insert(1, u'>'), u"<>ABCABCABCABC");
    }

    const QString montreal = QStringLiteral("Montreal");
    {
        // Test when string is not shared
        a = u"Meal"_s;
        QCOMPARE(a.insert(1, "ontr"_L1), montreal);
        QCOMPARE(a.insert(4, ""_L1), montreal);
        QCOMPARE(a.insert(3, ""_L1), montreal);
        QCOMPARE(a.insert(3, QLatin1String(nullptr)), montreal);
#ifndef QT_NO_CAST_FROM_ASCII
        QCOMPARE(a.insert(3, static_cast<const char *>(0)), montreal);
#endif
        QCOMPARE(a.insert(0, u"a"_s), "aMontreal"_L1);
    }
    {
        // Test when string is shared
        a = u"Meal"_s;
        dummy_share = a;
        QCOMPARE(a.insert(1, "ontr"_L1), montreal);
        dummy_share = a;
        QCOMPARE(a.insert(4, ""_L1), montreal);
        dummy_share = a;
        QCOMPARE(a.insert(3, ""_L1), montreal);
        dummy_share = a;
        QCOMPARE(a.insert(3, QLatin1String(nullptr)), montreal);
        dummy_share = a;
        QCOMPARE(a.insert(3, QLatin1String(static_cast<const char *>(0))), montreal);
        dummy_share = a;
        QCOMPARE(a.insert(0, u"a"_s), "aMontreal"_L1);
    }

    {
        // Test when string is not shared
        a = u"Mont"_s;
        QCOMPARE(a.insert(a.size(), "real"_L1), montreal);
        QCOMPARE(a.insert(a.size() + 1, "ABC"_L1), u"Montreal ABC");
    }
    {
        // Test when string is shared
        a = u"Mont"_s;
        dummy_share = a;
        QCOMPARE(a.insert(a.size(), "real"_L1), montreal);
        dummy_share = a;
        QCOMPARE(a.insert(a.size() + 1, "ABC"_L1), u"Montreal ABC");
    }

    {
        // Test when string is not shared
        a = u"AEF"_s;
        QCOMPARE(a.insert(1, "BCD"_L1), u"ABCDEF");
        QCOMPARE(a.insert(3, "-"_L1), u"ABC-DEF");
        QCOMPARE(a.insert(a.size() + 1, "XYZ"_L1), u"ABC-DEF XYZ");
    }

    {
        // Test when string is shared
        a = u"AEF"_s;
        dummy_share = a ;
        QCOMPARE(a.insert(1, "BCD"_L1), u"ABCDEF");
        dummy_share = a ;
        QCOMPARE(a.insert(3, "-"_L1), u"ABC-DEF");
        dummy_share = a ;
        QCOMPARE(a.insert(a.size() + 1, "XYZ"_L1), u"ABC-DEF XYZ");
    }


    {
        a = u"one"_s;
        a.prepend(u'a');
        QString b(a.data_ptr()->freeSpaceAtEnd(), u'b');
        QCOMPARE(a.insert(a.size() + 1, QLatin1String(b.toLatin1())), u"aone "_s + b);
    }
    {
        a = u"one"_s;
        a.prepend(u'a');
        QString b(a.data_ptr()->freeSpaceAtEnd(), u'b');
        QCOMPARE(a.insert(a.size() + 1, b), u"aone "_s + b);
    }

    {
        a = u"onetwothree"_s;
        while (a.size() - 1)
            a.remove(0, 1);
        QString b(a.data_ptr()->freeSpaceAtEnd() + 1, u'b');
        QCOMPARE(a.insert(a.size() + 1, QLatin1String(b.toLatin1())), u"e "_s + b);
    }
    {
        a = u"onetwothree"_s;
        while (a.size() - 1)
            a.remove(0, 1);
        QString b(a.data_ptr()->freeSpaceAtEnd() + 1, u'b');
        QCOMPARE(a.insert(a.size() + 1, b), u"e "_s + b);
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
    const QString empty(u""_s);
    const QString a(u"a"_s);
    //const QString b("b");
    const QString ab(u"ab"_s);

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
        static constexpr char16_t utf16[] = u"Hello, World!";
        constexpr size_t len = std::char_traits<char16_t>::length(utf16);
        const auto *unicode = reinterpret_cast<const QChar *>(utf16);
        QString a;
        a.append(unicode, len);
        QCOMPARE(a, QLatin1String("Hello, World!"));
        static const QChar nl(u'\n');
        a.append(&nl, 1);
        QCOMPARE(a, QLatin1String("Hello, World!\n"));
        a.append(unicode, len);
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
        QString a = u"one"_s;
        a.prepend(u'a');
        QString b(a.data_ptr()->freeSpaceAtEnd(), u'b');
        QCOMPARE(a.append(QLatin1String(b.toLatin1())), u"aone"_s + b);
    }

    {
        QString a = u"onetwothree"_s;
        while (a.size() - 1)
            a.remove(0, 1);
        QString b(a.data_ptr()->freeSpaceAtEnd(), u'b');
        QCOMPARE(a.append(QLatin1String(b.toLatin1())), u'e' + b);
    }

    {
        QString a = u"one"_s;
        a.prepend(u'a');
        QString b(a.data_ptr()->freeSpaceAtEnd(), u'b');
        QCOMPARE(a.append(b), u"aone"_s + b);
    }

    {
        QString a = u"onetwothree"_s;
        while (a.size() - 1)
            a.remove(0, 1);
        QString b(a.data_ptr()->freeSpaceAtEnd() + 1, u'b');
        QCOMPARE(a.append(b), u'e' + b);
    }

    {
        QString a = u"one"_s;
        a.prepend(u'a');
        QCOMPARE(a.append(u'b'), u"aoneb");
    }

    {
        QString a = u"onetwothree"_s;
        a.erase(a.cbegin(), std::prev(a.cend()));
        QCOMPARE(a.append(u'b'), u"eb");
    }
}

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
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
    QTest::newRow( "notTerminated_0" ) << QString() << ba << u"abcd"_s;
    QTest::newRow( "notTerminated_1" ) << u""_s << ba << u"abcd"_s;
    QTest::newRow( "notTerminated_2" ) << u"foobar "_s << ba << u"foobar abcd"_s;

    // byte array with only a 0
    ba.resize( 1 );
    ba[0] = 0;
    QByteArray ba2("foobar ");
    ba2.append('\0');
    QTest::newRow( "emptyString" ) << u"foobar "_s << ba << QString(ba2);

    // empty byte array
    ba.resize( 0 );
    QTest::newRow( "emptyByteArray" ) << u"foobar "_s << ba << u"foobar "_s;

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
    if (!ba.contains('\0') && ba.constData()[ba.size()] == '\0') {
        QFETCH( QString, str );

        str.append(ba.constData());
        QTEST( str, "res" );
    }
}
#endif // !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)

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
    QCOMPARE_NE((void *)copy.constData(), (void *)str.constData());
}

void tst_QString::assign()
{
    // QString &assign(QAnyStringView)
    {
        QString str;
        QCOMPARE(str.assign("data"), u"data");
        QCOMPARE(str.size(), 4);
        QCOMPARE(str.assign(u8"data\0data"), u"data\0data");
        QCOMPARE(str.size(), 4);
        QCOMPARE(str.assign(u"\0data\0data"), u"\0data\0data");
        QCOMPARE(str.size(), 0);
        QCOMPARE(str.assign(QAnyStringView("data\0")), u"data\0");
        QCOMPARE(str.size(), 4);
        QCOMPARE(str.assign(QStringView(u"(ノಠ益ಠ)ノ彡┻━┻\0")), u"(ノಠ益ಠ)ノ彡┻━┻\0");
        QCOMPARE(str.size(), 11);
        QCOMPARE(str.assign(QUtf8StringView(u8"٩(⁎❛ᴗ❛⁎)۶")), u"٩(⁎❛ᴗ❛⁎)۶");
        QCOMPARE(str.size(), 9);
        QCOMPARE(str.assign(QLatin1String("datadata")), u"datadata");
        QCOMPARE(str.size(), 8);
    }
    // QString &assign(qsizetype, char);
    {
        QString str;
        QCOMPARE(str.assign(3, u'è'), u"èèè");
        QCOMPARE(str.size(), 3);
        QCOMPARE(str.assign(20, u'd').assign(2, u'ᴗ'), u"ᴗᴗ");
        QCOMPARE(str.size(), 2);
        QCOMPARE(str.assign(0, u'x').assign(5, QLatin1Char('d')), u"ddddd");
        QCOMPARE(str.size(), 5);
        QCOMPARE(str.assign(3, u'x'), u"xxx");
        QCOMPARE(str.size(), 3);
    }
    // QString &assign(InputIterator, InputIterator)
    {
        // This needs to be on its own to ensure we call them on empty str
        QString str;

        const char16_t c16[] = u"٩(⁎❛ᴗ❛⁎)۶ 🤷";
        std::u16string c16str(c16);
        str.assign(c16str.begin(), c16str.begin());
        QCOMPARE(str.size(), 0);
    }
    {
#ifndef QT_NO_CAST_FROM_ASCII
        // This needs to be on its own to ensure we call them on empty str
        QString str;
        const char c8[] = "a©☻🂤"; // [1, 2, 3, 4] bytes in utf-8 code points
        std::string c8str(c8);
        str.assign(c8str.begin(), c8str.begin());
        QCOMPARE(str.size(), 0);
#endif
    }
    {
        // Forward iterator versions
        QString str;
        const QString tstr = QString::fromUtf8(u8"(ノಠ益ಠ)\0ノ彡┻━┻");
        QCOMPARE(str.assign(tstr.begin(), tstr.end()), u"(ノಠ益ಠ)\0ノ彡┻━┻");
        QCOMPARE(str.size(), 6);

        auto oldCap = str.capacity();
        str.assign(tstr.begin(), tstr.begin()); // empty range
        QCOMPARE_EQ(str.capacity(), oldCap);
        QCOMPARE_EQ(str.size(), 0);

#ifndef QT_NO_CAST_FROM_ASCII
        const char c8[] = "a©☻🂤"; // [1, 2, 3, 4] bytes in utf-8 code points
        str.assign(std::begin(c8), std::end(c8) - 1);
        QCOMPARE(str, c8);

        std::string c8str(c8);
        str.assign(c8str.begin(), c8str.end());
        QCOMPARE(str, c8);
        QCOMPARE(str.capacity(), qsizetype(std::size(c8) - 1));

        oldCap = str.capacity();
        str.assign(c8str.begin(), c8str.begin()); // empty range
        QCOMPARE_EQ(str.capacity(), oldCap);
        QCOMPARE_EQ(str.size(), 0);

        std::forward_list<char> fwd(std::begin(c8), std::end(c8) - 1);
        str.assign(fwd.begin(), fwd.end());
        QCOMPARE(str, c8);
#endif
#ifdef __cpp_char8_t
        const char8_t c8t[] = u8"🂤🂤🂤🂤🂤🂤🂤🂤🂤🂤"; // 10 x 4 bytes in utf-8 code points
        str.assign(std::begin(c8t), std::end(c8t) - 1);
        QCOMPARE(str, c8t);
        QCOMPARE(str.size(), 20);
#endif
#ifdef __cpp_lib_char8_t
        std::u8string c8tstr(c8t);
        str.assign(c8tstr.begin(), c8tstr.end());
        QCOMPARE(str, c8t);
#endif

        const char16_t c16[] = u"٩(⁎❛ᴗ❛⁎)۶ 🤷";
        str.assign(std::begin(c16), std::end(c16) - 1);
        QCOMPARE(str, c16);

        std::u16string c16str(c16);
        str.assign(c16str.begin(), c16str.end());
        QCOMPARE(str, c16);

        oldCap = str.capacity();
        str.assign(c16str.begin(), c16str.begin()); // empty range
        QCOMPARE_EQ(str.capacity(), oldCap);
        QCOMPARE_EQ(str.size(), 0);

        const char32_t c32[] = U"٩(⁎❛ᴗ❛⁎)۶ 🤷";
        str.assign(std::begin(c32), std::end(c32) - 1);
        QCOMPARE(str, c16);

        std::u32string c32str(c32);
        str.assign(c32str.begin(), c32str.end());
        QCOMPARE(str, c16);

        oldCap = str.capacity();
        str.assign(c32str.begin(), c32str.begin()); // empty range
        QCOMPARE_EQ(str.capacity(), oldCap);
        QCOMPARE_EQ(str.size(), 0);

        QVarLengthArray<QLatin1Char, 5> l1ch = {'F'_L1, 'G'_L1, 'H'_L1, 'I'_L1, 'J'_L1};
        str.assign(l1ch.begin(), l1ch.end());
        QCOMPARE(str, u"FGHIJ");
        std::forward_list<QChar> qch = {u'G', u'H', u'I', u'J', u'K'};
        str.assign(qch.begin(), qch.end());
        QCOMPARE(str, u"GHIJK");
        const QList<char16_t> qch16 = {u'X', u'H', u'I', u'J', u'K'}; // QList<T>::iterator need not be T*
        str.assign(qch16.begin(), qch16.end());
        QCOMPARE(str, u"XHIJK");
#if defined(Q_OS_WIN)
        QVarLengthArray<wchar_t> wch = {L'A', L'B', L'C', L'D', L'E'};
        str.assign(wch.begin(), wch.end());
        QCOMPARE(str, u"ABCDE");
#endif
        // Input iterator versions
        std::stringstream ss("50 51 52 53 54");
        str.assign(std::istream_iterator<ushort>{ss}, std::istream_iterator<ushort>{});
        QCOMPARE(str, u"23456");

        oldCap = str.capacity();
        str.assign(std::istream_iterator<ushort>{}, std::istream_iterator<ushort>{}); // empty range
        QCOMPARE_EQ(str.capacity(), oldCap);
        QCOMPARE_EQ(str.size(), 0);

#ifndef QT_NO_CAST_FROM_ASCII
        str.resize(0);
        str.squeeze();
        str.reserve(5);
        const char c8cmp[] = "🂤🂤a"; // 2 + 2 + 1 byte
        ss.clear();
        ss.str(c8cmp);
        str.assign(std::istream_iterator<char>{ss}, std::istream_iterator<char>{});
        QCOMPARE(str, c8cmp);
        QCOMPARE(str.size(), 5);
        QCOMPARE(str.capacity(), 5);

        // 1 code-point + ill-formed sequence + 1 code-point.
        const char c8IllFormed[] = "a\xe0\x9f\x80""a";
        ss.clear();
        ss.str(c8IllFormed);
        str.assign(std::istream_iterator<char>{ss}, std::istream_iterator<char>{});
        QEXPECT_FAIL("", "Iconsistent handling of ill-formed sequences, QTBUG-117051", Continue);
        QCOMPARE_EQ(str, QString(c8IllFormed));

        const char c82[] = "ÌşṫһíᶊśꞧɨℼṩuDF49ïľι?";
        ss.clear();
        ss.str(c82);
        str.assign(std::istream_iterator<char>{ss}, std::istream_iterator<char>{});
        QCOMPARE(str, c82);

        const char uc8[] = "ẵƽ𝔰ȉ𝚐ꞑ𝒾𝝿𝕘";
        ss.clear();
        ss.str(uc8);
        str.assign(std::istream_iterator<uchar>{ss}, std::istream_iterator<uchar>{});
        QCOMPARE(str, uc8);

        ss.clear();
        const char sc8[] = "𓁇ख़ॵ௵";
        ss.str(sc8);
        str.assign(std::istream_iterator<signed char>{ss}, std::istream_iterator<signed char>{});
        QCOMPARE(str, sc8);

        oldCap = str.capacity();
        str.assign(std::istream_iterator<signed char>{}, // empty range
                   std::istream_iterator<signed char>{});
        QCOMPARE_EQ(str.capacity(), oldCap);
        QCOMPARE_EQ(str.size(), 0);
#endif
    }
    // Test chaining
    {
        QString str;
        QString tstr = u"TEST DATA"_s;
        str.assign(tstr.begin(), tstr.end()).assign({"Hello World!"}).assign(5, u'T');
        QCOMPARE(str, u"TTTTT");
        QCOMPARE(str.size(), 5);
        QCOMPARE(str.assign(300, u'T').assign({"[̲̅$̲̅(̲̅5̲̅)̲̅$̲̅]"}), u"[̲̅$̲̅(̲̅5̲̅)̲̅$̲̅]");
        QCOMPARE(str.size(), 19);
        QCOMPARE(str.assign(10, u'c').assign(str.begin(), str.end()), str);
        QCOMPARE(str.size(), 10);
        QCOMPARE(str.assign("data").assign(QByteArrayView::fromArray(
            {std::byte('T'), std::byte('T'), std::byte('T')})), u"TTT");
        QCOMPARE(str.size(), 3);
        QCOMPARE(str.assign("data").assign("\0data"), u"\0data");
        QCOMPARE(str.size(), 0);
    }
}

void tst_QString::assign_shared()
{
    {
        QString str = "DATA"_L1;
        QVERIFY(str.isDetached());
        auto strCopy = str;
        QVERIFY(!str.isDetached());
        QVERIFY(!strCopy.isDetached());
        QVERIFY(str.isSharedWith(strCopy));
        QVERIFY(strCopy.isSharedWith(str));

        str.assign(4, u'D');
        QVERIFY(str.isDetached());
        QVERIFY(strCopy.isDetached());
        QVERIFY(!str.isSharedWith(strCopy));
        QVERIFY(!strCopy.isSharedWith(str));
        QCOMPARE(str, u"DDDD");
        QCOMPARE(strCopy, u"DATA");
    }
    {
        QString str = "DATA"_L1;
        QVERIFY(str.isDetached());
        auto copyForwardIt = str;
        QVERIFY(!str.isDetached());
        QVERIFY(!copyForwardIt.isDetached());
        QVERIFY(str.isSharedWith(copyForwardIt));
        QVERIFY(copyForwardIt.isSharedWith(str));

        QString tstr = u"DDDD"_s;
        str.assign(tstr.begin(), tstr.end());
        QVERIFY(str.isDetached());
        QVERIFY(copyForwardIt.isDetached());
        QVERIFY(!str.isSharedWith(copyForwardIt));
        QVERIFY(!copyForwardIt.isSharedWith(str));
        QCOMPARE(str, u"DDDD");
        QCOMPARE(copyForwardIt, u"DATA");
    }
    {
        QString str = "DATA"_L1;
        QVERIFY(str.isDetached());
        auto copyInputIt = str;
        QVERIFY(!str.isDetached());
        QVERIFY(!copyInputIt.isDetached());
        QVERIFY(str.isSharedWith(copyInputIt));
        QVERIFY(copyInputIt.isSharedWith(str));

        std::stringstream ss("49 50 51 52 53 54 ");
        str.assign(std::istream_iterator<ushort>{ss}, std::istream_iterator<ushort>{});
        QVERIFY(str.isDetached());
        QVERIFY(copyInputIt.isDetached());
        QVERIFY(!str.isSharedWith(copyInputIt));
        QVERIFY(!copyInputIt.isSharedWith(str));

        QCOMPARE(str, u"123456");
        QCOMPARE(copyInputIt, u"DATA");
    }
}

void tst_QString::assign_uses_prepend_buffer()
{
    const auto capBegin = [](const QString &s) {
        return s.begin() - s.d.freeSpaceAtBegin();
    };
    const auto capEnd = [](const QString &s) {
        return s.end() + s.d.freeSpaceAtEnd();
    };
    // QString &assign(QAnyStringView)
    {
        QString withFreeSpaceAtBegin;
        for (int i = 0; i < 100 && withFreeSpaceAtBegin.d.freeSpaceAtBegin() < 2; ++i)
            withFreeSpaceAtBegin.prepend(u'd');
        QCOMPARE_GT(withFreeSpaceAtBegin.d.freeSpaceAtBegin(), 1);

        const auto oldCapBegin = capBegin(withFreeSpaceAtBegin);
        const auto oldCapEnd = capEnd(withFreeSpaceAtBegin);

        QString test(withFreeSpaceAtBegin.d.freeSpaceAtBegin(), u'ȍ');
        withFreeSpaceAtBegin.assign(test);

        QCOMPARE_EQ(withFreeSpaceAtBegin.d.freeSpaceAtBegin(), 0); // we used the prepend buffer
        QCOMPARE_EQ(capBegin(withFreeSpaceAtBegin), oldCapBegin);
        QCOMPARE_EQ(capEnd(withFreeSpaceAtBegin), oldCapEnd);
        QCOMPARE(withFreeSpaceAtBegin, test);
    }
    // QString &assign(InputIterator, InputIterator)
    {
        QString withFreeSpaceAtBegin;
        for (int i = 0; i < 100 && withFreeSpaceAtBegin.d.freeSpaceAtBegin() < 2; ++i)
            withFreeSpaceAtBegin.prepend(u'd');
        QCOMPARE_GT(withFreeSpaceAtBegin.d.freeSpaceAtBegin(), 1);

        const auto oldCapBegin = capBegin(withFreeSpaceAtBegin);
        const auto oldCapEnd = capEnd(withFreeSpaceAtBegin);

        std::stringstream ss;
        for (qsizetype i = 0; i < withFreeSpaceAtBegin.d.freeSpaceAtBegin(); ++i)
            ss << "d ";

        withFreeSpaceAtBegin.assign(std::istream_iterator<char>{ss}, std::istream_iterator<char>{});
        QCOMPARE_EQ(withFreeSpaceAtBegin.d.freeSpaceAtBegin(), 0); // we used the prepend buffer
        QCOMPARE_EQ(capBegin(withFreeSpaceAtBegin), oldCapBegin);
        QCOMPARE_EQ(capEnd(withFreeSpaceAtBegin), oldCapEnd);
    }
}

void tst_QString::operator_pluseq_special_cases()
{
    QString a;
    a += QChar::CarriageReturn;
    a += u'\r';
    a += u'\x1111';
    QCOMPARE(a, u"\r\r\x1111");
}

void tst_QString::operator_pluseq_data(DataOptions options)
{
    append_data(options);
}

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
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
    if (!ba.contains('\0') && ba.constData()[ba.size()] == '\0') {
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

    if (!src.contains('\0') && src.constData()[src.size()] == '\0') {
        QVERIFY(expected == src.constData());
        QVERIFY(!(expected != src.constData()));
    }
}

void tst_QString::operator_assign_symmetry()
{
    {
        QString str("DATA");
        str.operator=(QString());
        QCOMPARE_EQ(str.capacity(), 0);
        QVERIFY(str.isNull());
    }
    {
        QString str("DATA");
        str.operator=(QByteArray());
        QCOMPARE_EQ(str.capacity(), 0);
        QVERIFY(str.isNull());
    }
    {
        QString str("DATA");
        const char *data = nullptr;
        str.operator=(data);
        QCOMPARE_EQ(str.capacity(), 0);
        QVERIFY(str.isNull());
    }
}
#endif // !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)

void tst_QString::swap()
{
    QString s1 = QString::fromUtf8("s1");
    QString s2 = QString::fromUtf8("s2");
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
    const QString empty(u""_s);
    const QString a(u'a');
    //const QString b("b");
    const QString ba(u"ba"_s);

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

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
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
    QTest::newRow( "emptyString" ) << u"foobar "_s << ba << QStringView::fromArray(u"\0foobar ").chopped(1).toString();

    // empty byte array
    ba.resize( 0 );
    QTest::newRow( "emptyByteArray" ) << u" foobar"_s << ba << u" foobar"_s;

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
    if (!ba.contains('\0') && ba.constData()[ba.size()] == '\0') {
        QFETCH( QString, str );

        str.prepend(ba.constData());
        QTEST( str, "res" );
    }
}
#endif // !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)

void tst_QString::prependEventuallyProducesFreeSpaceAtBegin()
{
    QString s;
    for (int i = 0; i < 100 && !s.data_ptr().freeSpaceAtBegin(); ++i)
        s.prepend(u'd');
    QCOMPARE_GT(s.data_ptr().freeSpaceAtBegin(), 1);
}

void tst_QString::replace_pos_len()
{
    QFETCH( QString, string );
    QFETCH( qsizetype, index );
    QFETCH( qsizetype, len );
    QFETCH( QString, after );

    // Test when the string is shared
    QString s1 = string;
    s1.replace(index, len, after );
    QTEST( s1, "result" );
    // Test when it's not shared
    s1 = string;
    s1.detach();
    s1.replace(index, len, after);
    QTEST(s1, "result");

    // Test when the string is shared
    QString s2 = string;
    s2.replace(index, len, after.unicode(), after.size());
    QTEST(s2, "result");
    // Test when it's not shared
    s2 = string;
    s2.detach();
    s2.replace(index, len, after.unicode(), after.size());
    QTEST(s2, "result");

    if (after.size() == 1) {
        // Test when the string is shared
        QString s3 = string;
        s3.replace(index, len, QChar(after[0]));
        QTEST(s3, "result");
        // Test when it's not shared
        s3 = string;
        s3.detach();
        s3.replace(index, len, QChar(after[0]));
        QTEST(s3, "result");

#if !defined(QT_NO_CAST_FROM_ASCII)
        // Testing replace(qsizetype, qsizetype, QLatin1Char) calls aren't ambiguous

        // Test when the string is shared
        QString s4 = string;
        s4.replace(index, len, QChar(after[0]).toLatin1());
        QTEST(s4, "result");
        // Test when it's not shared
        s4 = string;
        s4.detach();
        s4.replace(index, len, QChar(after[0]).toLatin1());
        QTEST(s4, "result");
#endif
    }
}

void tst_QString::replace_uint_uint_extra()
{
    {
        QString s;
        s.insert(0, QChar(u'A'));

        auto bigReplacement = QString(u'B').repeated(s.capacity() * 3);

        s.replace( 0, 1, bigReplacement );
        QCOMPARE( s, bigReplacement );
    }

    {
        QString s;
        s.insert(0, QLatin1String("BBB"));

        auto smallReplacement = QString(u'C');

        s.replace( 0, 3, smallReplacement );
        QCOMPARE( s, smallReplacement );
    }

    {
        QString s;
        s.insert(0, QLatin1String("BBB"));

        auto smallReplacement = QString(u'C');

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
    QString str(u"dsfkljfdsjklsdjsfjklfsdjkldfjslkjsdfkllkjdsfjklsfdkjsdflkjlsdfjklsdfkjldsflkjsddlkj"_s);
    for (int j = 1; j < 12; ++j)
        str += str;

    QString str2(u"aaaaaaaaaaaaaaaaaaaa"_s);
    for (int i = 0; i < 2000000; ++i) {
        str.replace(10, 20, str2);
    }

    /*
        Make sure that replacing with itself works.
    */
    QString copy(str);
    copy.detach();
    str.replace(0, str.size(), str);
    QVERIFY(copy == str);

    /*
        Make sure that replacing a part of oneself with itself works.
    */
    QString str3(u"abcdefghij"_s);
    str3.replace(0, 1, str3);
    QCOMPARE(str3, u"abcdefghijbcdefghij");

    QString str4(u"abcdefghij"_s);
    str4.replace(1, 3, str4);
    QCOMPARE(str4, u"aabcdefghijefghij");

    QString str5(u"abcdefghij"_s);
    str5.replace(8, 10, str5);
    QCOMPARE(str5, u"abcdefghabcdefghij");

    // Replacements using only part of the string modified:
    QString str6(u"abcdefghij"_s);
    str6.replace(1, 8, str6.constData() + 3, 3);
    QCOMPARE(str6, u"adefj");

    QString str7(u"abcdefghibcdefghij"_s);
    str7.replace(str7.constData() + 1, 6, str7.constData() + 2, 3);
    QCOMPARE(str7, u"acdehicdehij");

    const int many = 1024;
    /*
      QS::replace(const QChar *, int, const QChar *, int, Qt::CaseSensitivity)
      does its replacements in batches of many (please keep in sync with any
      changes to batch size), which lead to misbehaviour if ether QChar * array
      was part of the data being modified.
    */
    QString str8(u"abcdefg"_s);
    QString ans8(u"acdeg"_s);
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
    QFETCH(QString, result);

    Qt::CaseSensitivity cs = bcs ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if ( before.size() == 1 ) {
        QChar ch = before.at( 0 );

        // Test when isShared() is true
        QString s1 = string;
        s1.replace( ch, after, cs );
        QCOMPARE(s1, result);

        QString s4 = string;
        s4.begin(); // Test when isShared() is false
        s4.replace(ch, after, cs);
        QCOMPARE(s4, result);
    }

    QString s3 = string;
    s3.replace( before, after, cs );
    QCOMPARE(s3, result);
}

void tst_QString::replace_string_extra()
{
    {
        QString s;
        s.insert(0, u'A');

        auto bigReplacement = QString(u'B').repeated(s.capacity() * 3);

        s.replace( u"A"_s, bigReplacement );
        QCOMPARE( s, bigReplacement );
    }

    {
        QString s;
        s.insert(0, QLatin1String("BBB"));

        auto smallReplacement = QString(u'C');

        s.replace( u"BBB"_s, smallReplacement );
        QCOMPARE( s, smallReplacement );
    }

    {
        QString s(QLatin1String("BBB"));
        QString expected(QLatin1String("BBB"));
        for (int i = 0; i < 1028; ++i) {
            s.append(u'X');
            expected.append(u"GXU"_s);
        }
        s.replace(QChar(u'X'), u"GXU"_s);
        QCOMPARE(s, expected);
    }
}

#if QT_CONFIG(regularexpression)
void tst_QString::replace_regexp()
{
    static const QRegularExpression ignoreMessagePattern(
        u"^QString::replace\\(\\): called on an invalid QRegularExpression object"_s
    );

    QFETCH( QString, string );
    QFETCH( QString, regexp );
    QFETCH( QString, after );

    QRegularExpression regularExpression(regexp);
    if (!regularExpression.isValid())
        QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    string.replace(regularExpression, after);
    QTEST(string, "result");
}

void tst_QString::replace_regexp_extra()
{
    {
        QString s;
        s.insert(0, QChar(u'A'));

        auto bigReplacement = QString(u'B').repeated(s.capacity() * 3);

        QRegularExpression regularExpression(u"A"_s);
        QVERIFY(regularExpression.isValid());

        s.replace( regularExpression, bigReplacement );
        QCOMPARE( s, bigReplacement );
    }

    {
        QString s;
        s.insert(0, QLatin1String("BBB"));

        auto smallReplacement = QString(u'C');

        QRegularExpression regularExpression(u"BBB"_s);
        QVERIFY(regularExpression.isValid());

        s.replace( regularExpression, smallReplacement );
        QCOMPARE( s, smallReplacement );
    }
}
#endif

void tst_QString::remove_uint_uint()
{
    QFETCH( QString, string );
    QFETCH( qsizetype, index );
    QFETCH( qsizetype, len );
    QFETCH( QString, after );
    QFETCH(QString, result);

    // For the replace() unitests?
    if ( after.size() != 0 ) {
        return;
    }

    // Test when isShared() is true
    QString s1 = string;
    s1.remove(index, len);
    QCOMPARE(s1, result);

    QString s2 = string;
    // Test when isShared() is false
    s2.detach();
    s2.remove(index, len);
    QCOMPARE(s2, result);
}

void tst_QString::remove_string()
{
    QFETCH( QString, string );
    QFETCH( QString, before );
    QFETCH( QString, after );
    QFETCH( bool, bcs );
    QFETCH(QString, result);

    Qt::CaseSensitivity cs = bcs ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if ( after.size() == 0 ) {
        if ( before.size() == 1 && cs ) {
            QChar ch = before.at( 0 );

            // Test when isShared() is true
            QString s1 = string;
            s1.remove( ch );
            QCOMPARE(s1, result);

            // Test again with isShared() is false
            QString s4 = string;
            s4.begin(); // Detach
            s4.remove( ch );
            QCOMPARE(s4, result);

#ifndef QT_NO_CAST_FROM_ASCII
            // Testing remove(QLatin1Char) isn't ambiguous
            if ( QChar(ch.toLatin1()) == ch ) {
                QString s2 = string;
                s2.remove(ch.toLatin1());
                QCOMPARE(s2, result);
            }
#endif
        }

        // Test when needsDetach() is true
        QString s3 = string;
        s3.remove( before, cs );
        QCOMPARE(s3, result);

        QString s5 = string;
        s5.begin(); // Detach so needsDetach() is false
        s5.remove( before, cs );
        QCOMPARE(s5, result);

        if (QtPrivate::isLatin1(before)) {
            QString s6 = string;
            s6.remove( QLatin1String(before.toLatin1()), cs );
            QCOMPARE(s6, result);
        }
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
        << u"alpha"_s << u"a+"_s << u""_s << u"lph"_s;
    QTest::newRow("banana:s/^.a//")
        << u"banana"_s << u"^.a"_s << u""_s << u"nana"_s;
    QTest::newRow("<empty>:s/^.a//")
        << u""_s << u"^.a"_s << u""_s << u""_s;
    // The null-vs-empty distinction in after is only relevant to repplace_regexp(), but
    // include both cases here to keep after's "empty here, non-empty there" rule simple.
    QTest::newRow("<empty>:s/^.a/<null>/")
        << u""_s << u"^.a"_s << QString() << u""_s;
    QTest::newRow("<null>:s/^.a//") << QString() << u"^.a"_s << u""_s << QString();
    QTest::newRow("<null>s/.a/<null>/") << QString() << u"^.a"_s << QString() << QString();
    QTest::newRow("invalid")
        << u""_s << u"invalid regex\\"_s << u""_s << u""_s;
}

void tst_QString::remove_regexp()
{
    static const QRegularExpression ignoreMessagePattern(
        u"^QString::replace\\(\\): called on an invalid QRegularExpression object"_s
    );

    QFETCH( QString, string );
    QFETCH( QString, regexp );
    QTEST(QString(), "after"); // non-empty replacement text tests should go in replace_regexp_data()

    QRegularExpression regularExpression(regexp);
    // remove() delegates to replace(), which produces this warning:
    if (!regularExpression.isValid())
        QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    string.remove(regularExpression);
    QTEST(string, "result");
}
#endif

void tst_QString::remove_extra()
{
    {
        QString quickFox = "The quick brown fox jumps over the lazy dog. "
                           "The lazy dog jumps over the quick brown fox."_L1;
        QString s1 = quickFox;
        QVERIFY(s1.data_ptr().needsDetach());
        s1.remove(s1);
        QVERIFY(s1.isEmpty());
        QVERIFY(!quickFox.isEmpty());

        QVERIFY(!quickFox.data_ptr().needsDetach());
        quickFox.remove(quickFox);
        QVERIFY(quickFox.isEmpty());
    }

    {
        QString s = u"BCDEFGHJK"_s;
        QString s1 = s;
        s1.insert(0, u'A');  // detaches
        s1.erase(s1.cbegin());
        QCOMPARE(s1, s);
    }

    {
        QString s = u"Clock"_s;
        s.removeFirst();
        QCOMPARE(s, u"lock");
        s.removeLast();
        QCOMPARE(s, u"loc");
        s.removeAt(s.indexOf(u'o'));
        QCOMPARE(s, u"lc");
        s.clear();
        // No crash on empty strings
        s.removeFirst();
        s.removeLast();
        s.removeAt(2);
    }
}

void tst_QString::erase_single_arg()
{
    QString s = u"abcdefg"_s;
    auto it = s.erase(s.cbegin());
    QCOMPARE_EQ(s, u"bcdefg");
    QCOMPARE(it, s.cbegin());

    it = s.erase(std::prev(s.end()));
    QCOMPARE_EQ(s, u"bcdef");
    QCOMPARE(it, s.cend());

    it = s.erase(std::find(s.begin(), s.end(), QChar(u'd')));
    QCOMPARE(it, s.begin() + 2);
}

void tst_QString::erase()
{
    QString str = u"abcdefg"_s;

    QString s = str;
    auto it = s.erase(s.begin(), s.end());
    QCOMPARE_EQ(s, u"");
    QCOMPARE(it, s.end());

    s = str;
    it = s.erase(std::prev(s.end()));
    QCOMPARE_EQ(s, u"abcdef");
    QCOMPARE(it, s.end());

    it = s.erase(s.begin() + 2, s.end());
    QCOMPARE_EQ(s, u"ab");
    QCOMPARE(it, s.end());

    it = s.erase(s.begin(), s.begin() + 1);
    QCOMPARE_EQ(s, u"b");
    QCOMPARE(it, s.begin());

    {
        QString s1 = QLatin1String("house");
        QString copy = s1;
        // erase() should return an iterator, not const_iterator
        auto it = s1.erase(s1.cbegin(), s1.cbegin());
        *it = QLatin1Char('m');
        QCOMPARE(s1, u"mouse");
        QCOMPARE(copy, u"house");
    }
}

void tst_QString::toNum_base_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<int>("expected");

    QTest::newRow("FF") << u"FF"_s << 16 << 255;
    QTest::newRow("0xFF") << u"0xFF"_s << 16 << 255;
    QTest::newRow("77") << u"77"_s << 8 << 63;
    QTest::newRow("077") << u"077"_s << 8 << 63;

    QTest::newRow("0xFF - deduced base") << u"0xFF"_s << 0 << 255;
    QTest::newRow("077 - deduced base") << u"077"_s << 0 << 63;
    QTest::newRow("255 - deduced base") << u"255"_s << 0 << 255;

    QTest::newRow(" FF") << u" FF"_s << 16 << 255;
    QTest::newRow(" 0xFF") << u" 0xFF"_s << 16 << 255;
    QTest::newRow(" 77") << u" 77"_s << 8 << 63;
    QTest::newRow(" 077") << u" 077"_s << 8 << 63;

    QTest::newRow(" 0xFF - deduced base") << u" 0xFF"_s << 0 << 255;
    QTest::newRow(" 077 - deduced base") << u" 077"_s << 0 << 63;
    QTest::newRow(" 255 - deduced base") << u" 255"_s << 0 << 255;

    QTest::newRow("\tFF\t") << u"\tFF\t"_s << 16 << 255;
    QTest::newRow("\t0xFF  ") << u"\t0xFF  "_s << 16 << 255;
    QTest::newRow("   77   ") << u"   77   "_s << 8 << 63;
    QTest::newRow("77  ") << u"77  "_s << 8 << 63;
}

void tst_QString::toNum_base()
{
    QFETCH(QString, str);
    QFETCH(int, base);
    QFETCH(int, expected);

    bool ok = false;
    QCOMPARE(str.toInt(&ok, base), expected);
    QVERIFY(ok);

    QCOMPARE(str.toUInt(&ok, base), uint(expected));
    QVERIFY(ok);

    QCOMPARE(str.toShort(&ok, base), expected);
    QVERIFY(ok);

    QCOMPARE(str.toUShort(&ok, base), expected);
    QVERIFY(ok);

    QCOMPARE(str.toLong(&ok, base), expected);
    QVERIFY(ok);

    QCOMPARE(str.toULong(&ok, base), ulong(expected));
    QVERIFY(ok);

    QCOMPARE(str.toLongLong(&ok, base), expected);
    QVERIFY(ok);

    QCOMPARE(str.toULongLong(&ok, base), qulonglong(expected));
    QVERIFY(ok);
}

void tst_QString::toNum_base_neg_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<int>("expected");

    QTest::newRow("-FE") << u"-FE"_s << 16 << -254;
    QTest::newRow("-0xFE") << u"-0xFE"_s << 16 << -254;
    QTest::newRow("-77") << u"-77"_s << 8 << -63;
    QTest::newRow("-077") << u"-077"_s << 8 << -63;

    QTest::newRow("-0xFE - deduced base") << u"-0xFE"_s << 0 << -254;
    QTest::newRow("-077 - deduced base") << u"-077"_s << 0 << -63;
    QTest::newRow("-254 - deduced base") << u"-254"_s << 0 << -254;
}

void tst_QString::toNum_base_neg()
{
    QFETCH(QString, str);
    QFETCH(int, base);
    QFETCH(int, expected);

    bool ok = false;
    QCOMPARE(str.toInt(&ok, base), expected);
    QVERIFY(ok);

    QCOMPARE(str.toShort(&ok, base), expected);
    QVERIFY(ok);

    QCOMPARE(str.toLong(&ok, base), expected);
    QVERIFY(ok);

    QCOMPARE(str.toLongLong(&ok, base), expected);
    QVERIFY(ok);
}

void tst_QString::toNum_Bad()
{
    QString a;
    bool ok = false;

    QString(u"32768"_s).toShort(&ok);
    QVERIFY(!ok);

    QString(u"-32769"_s).toShort(&ok);
    QVERIFY(!ok);

    QString(u"65536"_s).toUShort(&ok);
    QVERIFY(!ok);

    QString(u"2147483648"_s).toInt(&ok);
    QVERIFY(!ok);

    QString(u"-2147483649"_s).toInt(&ok);
    QVERIFY(!ok);

    QString(u"4294967296"_s).toUInt(&ok);
    QVERIFY(!ok);

    if (sizeof(long) == 4) {
        QString(u"2147483648"_s).toLong(&ok);
        QVERIFY(!ok);

        QString(u"-2147483649"_s).toLong(&ok);
        QVERIFY(!ok);

        QString(u"4294967296"_s).toULong(&ok);
        QVERIFY(!ok);
    }

    QString(u"9223372036854775808"_s).toLongLong(&ok);
    QVERIFY(!ok);

    QString(u"-9223372036854775809"_s).toLongLong(&ok);
    QVERIFY(!ok);

    QString(u"18446744073709551616"_s).toULongLong(&ok);
    QVERIFY(!ok);

    QString(u"-1"_s).toUShort(&ok);
    QVERIFY(!ok);

    QString(u"-1"_s).toUInt(&ok);
    QVERIFY(!ok);

    QString(u"-1"_s).toULong(&ok);
    QVERIFY(!ok);

    QString(u"-1"_s).toULongLong(&ok);
    QVERIFY(!ok);
}

void tst_QString::toNum_BadAll_data()
{
    QTest::addColumn<QString>("str");

    QTest::newRow("empty") << u""_s;
    QTest::newRow("space") << u" "_s;
    QTest::newRow("dot") << u"."_s;
    QTest::newRow("dash") << u"-"_s;
    QTest::newRow("hello") << u"hello"_s;
    QTest::newRow("1.2.3") << u"1.2.3"_s;
    QTest::newRow("0x0x0x") << u"0x0x0x"_s;
    QTest::newRow("123-^~<") << u"123-^~<"_s;
    QTest::newRow("123ThisIsNotANumber") << u"123ThisIsNotANumber"_s;
}

void tst_QString::toNum_BadAll()
{
    QFETCH(QString, str);
    bool ok = false;

    str.toShort(&ok);
    QVERIFY(!ok);

    str.toUShort(&ok);
    QVERIFY(!ok);

    str.toInt(&ok);
    QVERIFY(!ok);

    str.toUInt(&ok);
    QVERIFY(!ok);

    str.toLong(&ok);
    QVERIFY(!ok);

    str.toULong(&ok);
    QVERIFY(!ok);

    str.toLongLong(&ok);
    QVERIFY(!ok);

    str.toULongLong(&ok);
    QVERIFY(!ok);

    str.toFloat(&ok);
    QVERIFY(!ok);

    str.toDouble(&ok);
    QVERIFY(!ok);
}

void tst_QString::toNum()
{
#if defined (Q_OS_WIN) && defined (Q_CC_MSVC)
#define TEST_TO_INT(num, func) \
    a = QLatin1StringView(#num); \
    QVERIFY2(a.func(&ok) == num ## i64 && ok, "Failed: num=" #num ", func=" #func);
#else
#define TEST_TO_INT(num, func) \
    a = QLatin1StringView(#num); \
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
    a = QLatin1StringView(#num); \
    QVERIFY2(a.func(&ok) == num ## i64 && ok, "Failed: num=" #num ", func=" #func);
#else
#define TEST_TO_UINT(num, func) \
    a = QLatin1StringView(#num); \
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

    a = u"FF"_s;
    a.toULongLong(&ok, 10);
    QVERIFY(!ok);

    a = u"FF"_s;
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

    a = u""_s;
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = u"COMPARE"_s;
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = u"123"_s;
    QCOMPARE(a.toUShort(),(ushort)123);
    QCOMPARE(a.toUShort(&ok),(ushort)123);
    QVERIFY(ok);

    a = u"123A"_s;
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = u"1234567"_s;
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = u"aaa123aaa"_s;
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = u"aaa123"_s;
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = u"123aaa"_s;
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = u"32767"_s;
    QCOMPARE(a.toUShort(),(ushort)32767);
    QCOMPARE(a.toUShort(&ok),(ushort)32767);
    QVERIFY(ok);

    a = u"-32767"_s;
    QCOMPARE(a.toUShort(),(ushort)0);
    QCOMPARE(a.toUShort(&ok),(ushort)0);
    QVERIFY(!ok);

    a = u"65535"_s;
    QCOMPARE(a.toUShort(),(ushort)65535);
    QCOMPARE(a.toUShort(&ok),(ushort)65535);
    QVERIFY(ok);

    if (sizeof(short) == 2) {
        a = u"65536"_s;
        QCOMPARE(a.toUShort(),(ushort)0);
        QCOMPARE(a.toUShort(&ok),(ushort)0);
        QVERIFY(!ok);

        a = u"123456"_s;
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

    a = u""_s;
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = u"COMPARE"_s;
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = u"123"_s;
    QCOMPARE(a.toShort(),(short)123);
    QCOMPARE(a.toShort(&ok),(short)123);
    QVERIFY(ok);

    a = u"123A"_s;
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = u"1234567"_s;
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = u"aaa123aaa"_s;
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = u"aaa123"_s;
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = u"123aaa"_s;
    QCOMPARE(a.toShort(),(short)0);
    QCOMPARE(a.toShort(&ok),(short)0);
    QVERIFY(!ok);

    a = u"32767"_s;
    QCOMPARE(a.toShort(),(short)32767);
    QCOMPARE(a.toShort(&ok),(short)32767);
    QVERIFY(ok);

    a = u"-32767"_s;
    QCOMPARE(a.toShort(),(short)-32767);
    QCOMPARE(a.toShort(&ok),(short)-32767);
    QVERIFY(ok);

    a = u"-32768"_s;
    QCOMPARE(a.toShort(),(short)-32768);
    QCOMPARE(a.toShort(&ok),(short)-32768);
    QVERIFY(ok);

    if (sizeof(short) == 2) {
        a = u"32768"_s;
        QCOMPARE(a.toShort(),(short)0);
        QCOMPARE(a.toShort(&ok),(short)0);
        QVERIFY(!ok);

        a = u"-32769"_s;
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

    a = u""_s;
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = u"COMPARE"_s;
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = u"123"_s;
    QCOMPARE(a.toInt(),123);
    QCOMPARE(a.toInt(&ok),123);
    QVERIFY(ok);

    a = u"123A"_s;
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = u"1234567"_s;
    QCOMPARE(a.toInt(),1234567);
    QCOMPARE(a.toInt(&ok),1234567);
    QVERIFY(ok);

    a = u"12345678901234"_s;
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = u"3234567890"_s;
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = u"aaa12345aaa"_s;
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = u"aaa12345"_s;
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = u"12345aaa"_s;
    QCOMPARE(a.toInt(),0);
    QCOMPARE(a.toInt(&ok),0);
    QVERIFY(!ok);

    a = u"2147483647"_s; // 2**31 - 1
    QCOMPARE(a.toInt(),2147483647);
    QCOMPARE(a.toInt(&ok),2147483647);
    QVERIFY(ok);

    if (sizeof(int) == 4) {
        a = u"-2147483647"_s; // -(2**31 - 1)
        QCOMPARE(a.toInt(),-2147483647);
        QCOMPARE(a.toInt(&ok),-2147483647);
        QVERIFY(ok);

        a = u"2147483648"_s; // 2**31
        QCOMPARE(a.toInt(),0);
        QCOMPARE(a.toInt(&ok),0);
        QVERIFY(!ok);

        a = u"-2147483648"_s; // -2**31
        QCOMPARE(a.toInt(),-2147483647 - 1);
        QCOMPARE(a.toInt(&ok),-2147483647 - 1);
        QVERIFY(ok);

        a = u"2147483649"_s; // 2**31 + 1
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

    a = u"3234567890"_s;
    QCOMPARE(a.toUInt(&ok),3234567890u);
    QVERIFY(ok);

    a = u"-50"_s;
    QCOMPARE(a.toUInt(),0u);
    QCOMPARE(a.toUInt(&ok),0u);
    QVERIFY(!ok);

    a = u"4294967295"_s; // 2**32 - 1
    QCOMPARE(a.toUInt(),4294967295u);
    QCOMPARE(a.toUInt(&ok),4294967295u);
    QVERIFY(ok);

    if (sizeof(int) == 4) {
        a = u"4294967296"_s; // 2**32
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
    QTest::newRow( "empty" ) << u""_s << 10 << 0UL << false;
    QTest::newRow( "ulong1" ) << u"3234567890"_s << 10 << 3234567890UL << true;
    QTest::newRow( "ulong2" ) << u"fFFfFfFf"_s << 16 << 0xFFFFFFFFUL << true;
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
    QTest::newRow("empty") << u""_s << 10 << 0L << false;
    QTest::newRow("normal") << u"7fFFfFFf"_s << 16 << 0x7fFFfFFfL << true;
    QTest::newRow("long_max") << u"2147483647"_s << 10 << 2147483647L << true;
    if (sizeof(long) == 4) {
        QTest::newRow("long_max+1") << u"2147483648"_s << 10 << 0L << false;
        QTest::newRow("long_min-1") << u"-80000001"_s << 16 << 0L << false;
    }
    QTest::newRow("negative") << u"-7fffffff"_s << 16 << -0x7fffffffL << true;
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
    bool ok = true;

    QCOMPARE(str.toULongLong(), Q_UINT64_C(0));
    QCOMPARE(str.toULongLong(&ok), Q_UINT64_C(0));
    QVERIFY(!ok);

    str = u"18446744073709551615"_s; // ULLONG_MAX
    QCOMPARE( str.toULongLong( 0 ), Q_UINT64_C(18446744073709551615) );
    QCOMPARE( str.toULongLong( &ok ), Q_UINT64_C(18446744073709551615) );
    QVERIFY( ok );

    str = u"18446744073709551616"_s; // ULLONG_MAX + 1
    QCOMPARE( str.toULongLong( 0 ), Q_UINT64_C(0) );
    QCOMPARE( str.toULongLong( &ok ), Q_UINT64_C(0) );
    QVERIFY( !ok );

    str = u"-150"_s;
    QCOMPARE( str.toULongLong( 0 ), Q_UINT64_C(0) );
    QCOMPARE( str.toULongLong( &ok ), Q_UINT64_C(0) );
    QVERIFY( !ok );

    // Check limits round-trip in every base:
    using ULL = std::numeric_limits<qulonglong>;
    for (int b = 0; b <= 36; ++b) {
        if (b == 1) // 0 and 2 through 36 are valid bases
            ++b;
        QCOMPARE(QString::number(ULL::max(), b ? b : 10).toULongLong(&ok, b), ULL::max());
        QVERIFY(ok);
    }
}

void tst_QString::toLongLong()
{
    QString str;
    bool ok;

    QCOMPARE(str.toLongLong(0), Q_INT64_C(0));
    QCOMPARE(str.toLongLong(&ok), Q_INT64_C(0));
    QVERIFY(!ok);

    str = u"9223372036854775807"_s; // LLONG_MAX
    QCOMPARE( str.toLongLong( 0 ), Q_INT64_C(9223372036854775807) );
    QCOMPARE( str.toLongLong( &ok ), Q_INT64_C(9223372036854775807) );
    QVERIFY( ok );

    str = u"-9223372036854775808"_s; // LLONG_MIN
    QCOMPARE( str.toLongLong( 0 ),
             -Q_INT64_C(9223372036854775807) - Q_INT64_C(1) );
    QCOMPARE( str.toLongLong( &ok ),
             -Q_INT64_C(9223372036854775807) - Q_INT64_C(1) );
    QVERIFY( ok );

    str = u"aaaa9223372036854775807aaaa"_s;
    QCOMPARE( str.toLongLong( 0 ), Q_INT64_C(0) );
    QCOMPARE( str.toLongLong( &ok ), Q_INT64_C(0) );
    QVERIFY( !ok );

    str = u"9223372036854775807aaaa"_s;
    QCOMPARE( str.toLongLong( 0 ), Q_INT64_C(0) );
    QCOMPARE( str.toLongLong( &ok ), Q_INT64_C(0) );
    QVERIFY( !ok );

    str = u"aaaa9223372036854775807"_s;
    QCOMPARE( str.toLongLong( 0 ), Q_INT64_C(0) );
    QCOMPARE( str.toLongLong( &ok ), Q_INT64_C(0) );
    QVERIFY( !ok );

    static char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (int i = 0; i < 36; ++i) {
        for (int j = 0; j < 36; ++j) {
            for (int k = 0; k < 36; ++k) {
                QString str;
                str += QLatin1Char(digits[i]);
                str += QLatin1Char(digits[j]);
                str += QLatin1Char(digits[k]);
                qlonglong value = (((i * 36) + j) * 36) + k;
                QVERIFY(str.toLongLong(0, 36) == value);
            }
        }
    }

    // Check bounds.
    // First in every base, with no prefix:
    using LL = std::numeric_limits<qlonglong>;
    for (int b = 0; b <= 36; ++b) {
        if (b == 1) // 0 and 2 through 36 are valid bases
            ++b;
        QCOMPARE(QString::number(LL::max(), b ? b : 10).toLongLong(&ok, b), LL::max());
        QVERIFY(ok);
        QCOMPARE(QString::number(LL::min(), b ? b : 10).toLongLong(&ok, b), LL::min());
        QVERIFY(ok);
    }

    // Then in base 16 or 0 with 0x prefix:
    auto big = QString::number(LL::min(), 16);
    big.insert(1, u"0x"); // after the minus sign
    big.prepend(u"\t\r\n\f\v ");
    QCOMPARE(big.toLongLong(&ok, 16), LL::min());
    QVERIFY(ok);
    QCOMPARE(big.toLongLong(&ok, 0), LL::min());
    QVERIFY(ok);
    big = QString::number(LL::max(), 16);
    big.prepend(u"\t\r\n\f\v 0x");
    QCOMPARE(big.toLongLong(&ok, 16), LL::max());
    QVERIFY(ok);
    QCOMPARE(big.toLongLong(&ok, 0), LL::max());
    QVERIFY(ok);
    big.insert(6, u'+');
    QCOMPARE(big.toLongLong(&ok, 16), LL::max());
    QVERIFY(ok);
    QCOMPARE(big.toLongLong(&ok, 0), LL::max());
    QVERIFY(ok);

    // Next octal:
    big = QString::number(LL::min(), 8);
    big.insert(1, u'0'); // after the minus sign
    big.prepend(u"\t\r\n\f\v ");
    QCOMPARE(big.toLongLong(&ok, 8), LL::min());
    QVERIFY(ok);
    QCOMPARE(big.toLongLong(&ok, 0), LL::min());
    QVERIFY(ok);
    big = QString::number(LL::max(), 8);
    big.prepend(u"\t\r\n\f\v 0");
    QCOMPARE(big.toLongLong(&ok, 8), LL::max());
    QVERIFY(ok);
    QCOMPARE(big.toLongLong(&ok, 0), LL::max());
    QVERIFY(ok);
    big.insert(6, u'+');
    QCOMPARE(big.toLongLong(&ok, 8), LL::max());
    QVERIFY(ok);
    QCOMPARE(big.toLongLong(&ok, 0), LL::max());
    QVERIFY(ok);

    // Finally decimal for base 0:
    big = QString::number(LL::min(), 10);
    big.prepend(u"\t\r\n\f\v ");
    QCOMPARE(big.toLongLong(&ok, 0), LL::min());
    QVERIFY(ok);
    big = QString::number(LL::max(), 10);
    big.prepend(u"\t\r\n\f\v ");
    QCOMPARE(big.toLongLong(&ok, 0), LL::max());
    QVERIFY(ok);
    big.insert(6, u'+');
    QCOMPARE(big.toLongLong(&ok, 0), LL::max());
    QVERIFY(ok);
}

////////////////////////////////////////////////////////////////////////////

void tst_QString::toFloat()
{
    QString a;
    bool ok;

    QCOMPARE(a.toFloat(), 0.0f);
    QCOMPARE(a.toFloat(&ok), 0.0f);
    QVERIFY(!ok);

    a = u"0.000000000931322574615478515625"_s;
    QCOMPARE(a.toFloat(&ok),(float)(0.000000000931322574615478515625));
    QVERIFY(ok);
}

void tst_QString::toDouble_data()
{
    QTest::addColumn<QString>("str" );
    QTest::addColumn<double>("result" );
    QTest::addColumn<bool>("result_ok" );

    QTest::newRow("null") << QString() << 0.0 << false;
    QTest::newRow("empty") << u""_s << 0.0 << false;

    QTest::newRow("ok00") << u"0.000000000931322574615478515625"_s << 0.000000000931322574615478515625 << true;
    QTest::newRow("ok01") << u" 123.45"_s << 123.45 << true;

    QTest::newRow("ok02") << u"0.1e10"_s << 0.1e10 << true;
    QTest::newRow("ok03") << u"0.1e-10"_s << 0.1e-10 << true;

    QTest::newRow("ok04") << u"1e10"_s << 1.0e10 << true;
    QTest::newRow("ok05") << u"1e+10"_s << 1.0e10 << true;
    QTest::newRow("ok06") << u"1e-10"_s << 1.0e-10 << true;

    QTest::newRow("ok07") << u" 1e10"_s << 1.0e10 << true;
    QTest::newRow("ok08") << u"  1e+10"_s << 1.0e10 << true;
    QTest::newRow("ok09") << u"   1e-10"_s << 1.0e-10 << true;

    QTest::newRow("ok10") << u"1."_s << 1.0 << true;
    QTest::newRow("ok11") << u".1"_s << 0.1 << true;
    QTest::newRow("ok12") << u"1.2345"_s << 1.2345 << true;
    QTest::newRow("ok13") << u"12345.6"_s << 12345.6 << true;
    QTest::newRow("double-e+") << u"1.2345e+01"_s << 12.345 << true;
    QTest::newRow("double-E+") << u"1.2345E+01"_s << 12.345 << true;

    QTest::newRow("wrong00") << u"123.45 "_s << 123.45 << true;
    QTest::newRow("wrong01") << u" 123.45 "_s << 123.45 << true;

    QTest::newRow("wrong02") << u"aa123.45aa"_s << 0.0 << false;
    QTest::newRow("wrong03") << u"123.45aa"_s << 0.0 << false;
    QTest::newRow("wrong04") << u"123erf"_s << 0.0 << false;

    QTest::newRow("wrong05") << u"abc"_s << 0.0 << false;
    QTest::newRow( "wrong06" ) << QString() << 0.0 << false;
    QTest::newRow("wrong07") << u""_s << 0.0 << false;
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
    QCOMPARE(a.setNum(2147483647L), u"2147483647"); // 32 bit LONG_MAX
    QCOMPARE(a.setNum(-2147483647L), u"-2147483647"); // LONG_MIN + 1
    QCOMPARE(a.setNum(-2147483647L-1L), u"-2147483648"); // LONG_MIN
    QCOMPARE(a.setNum(1.23), u"1.23");
    QCOMPARE(a.setNum(1.234567), u"1.23457");
#if defined(LONG_MAX) && defined(LLONG_MAX) && LONG_MAX == LLONG_MAX
    // LONG_MAX and LONG_MIN on 64 bit systems
    QCOMPARE(a.setNum(9223372036854775807L), u"9223372036854775807");
    QCOMPARE(a.setNum(-9223372036854775807L-1L), u"-9223372036854775808");
    QCOMPARE(a.setNum(18446744073709551615UL), u"18446744073709551615");
#endif
    QCOMPARE(a.setNum(Q_INT64_C(123)), u"123");
    // 2^40 == 1099511627776
    QCOMPARE(a.setNum(Q_INT64_C(-1099511627776)), u"-1099511627776");
    QCOMPARE(a.setNum(Q_UINT64_C(1099511627776)), u"1099511627776");
    QCOMPARE(a.setNum(Q_INT64_C(9223372036854775807)), // LLONG_MAX
            u"9223372036854775807");
    QCOMPARE(a.setNum(-Q_INT64_C(9223372036854775807) - Q_INT64_C(1)),
            u"-9223372036854775808");
    QCOMPARE(a.setNum(Q_UINT64_C(18446744073709551615)), // ULLONG_MAX
            u"18446744073709551615");
    QCOMPARE(a.setNum(0.000000000931322574615478515625), u"9.31323e-10");

//  QCOMPARE(a.setNum(0.000000000931322574615478515625,'g',30),(QString)"9.31322574615478515625e-010");
//  QCOMPARE(a.setNum(0.000000000931322574615478515625,'f',30),(QString)"0.00000000093132257461547852");
}

void tst_QString::startsWith()
{
    QString a;

    QVERIFY(!a.startsWith(u'A'));
    QVERIFY(!a.startsWith(u"AB"_s));
    {
        CREATE_VIEW(u"AB"_s);
        QVERIFY(!a.startsWith(view));
    }
    QVERIFY(!a.isDetached());

    a = u"AB"_s;
    QVERIFY(a.startsWith(u"A"));
    QVERIFY(a.startsWith(u"AB"_s));
    QVERIFY(!a.startsWith(u"C"));
    QVERIFY(!a.startsWith(u"ABCDEF"_s));
    QVERIFY(a.startsWith(u""_s));
    QVERIFY( a.startsWith(QString()) );
    QVERIFY(a.startsWith(u'A'));
    QVERIFY(a.startsWith(QLatin1Char('A')));
    QVERIFY(a.startsWith(QChar(u'A')));
    QVERIFY(!a.startsWith(u'C'));
    QVERIFY( !a.startsWith(QChar()) );
    QVERIFY( !a.startsWith(QLatin1Char(0)) );

    QVERIFY( a.startsWith(QLatin1String("A")) );
    QVERIFY( a.startsWith(QLatin1String("AB")) );
    QVERIFY( !a.startsWith(QLatin1String("C")) );
    QVERIFY( !a.startsWith(QLatin1String("ABCDEF")) );
    QVERIFY( a.startsWith(QLatin1String("")) );
    QVERIFY( a.startsWith(QLatin1String(nullptr)) );

    QVERIFY(a.startsWith(u"A"_s, Qt::CaseSensitive));
    QVERIFY(a.startsWith(u"A"_s, Qt::CaseInsensitive));
    QVERIFY(!a.startsWith(u"a"_s, Qt::CaseSensitive));
    QVERIFY(a.startsWith(u"a"_s, Qt::CaseInsensitive));
    QVERIFY(!a.startsWith(u"aB"_s, Qt::CaseSensitive));
    QVERIFY(a.startsWith(u"aB"_s, Qt::CaseInsensitive));
    QVERIFY(!a.startsWith(u"C"_s, Qt::CaseSensitive));
    QVERIFY(!a.startsWith(u"C"_s, Qt::CaseInsensitive));
    QVERIFY(!a.startsWith(u"c"_s, Qt::CaseSensitive));
    QVERIFY(!a.startsWith(u"c"_s, Qt::CaseInsensitive));
    QVERIFY(!a.startsWith(u"abcdef"_s, Qt::CaseInsensitive));
    QVERIFY(a.startsWith(u""_s, Qt::CaseInsensitive));
    QVERIFY( a.startsWith(QString(), Qt::CaseInsensitive) );
    QVERIFY(a.startsWith(u'a', Qt::CaseInsensitive));
    QVERIFY(a.startsWith(u'A', Qt::CaseInsensitive));
    QVERIFY( a.startsWith(QLatin1Char('a'), Qt::CaseInsensitive) );
    QVERIFY(a.startsWith(QChar(u'a'), Qt::CaseInsensitive));
    QVERIFY(!a.startsWith(u'c', Qt::CaseInsensitive));
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
    QVERIFY( a.startsWith(QLatin1String(nullptr), Qt::CaseInsensitive) );
    QVERIFY(a.startsWith(u'A', Qt::CaseSensitive));
    QVERIFY(a.startsWith(QLatin1Char('A'), Qt::CaseSensitive));
    QVERIFY(a.startsWith(QChar(u'A'), Qt::CaseSensitive));
    QVERIFY(!a.startsWith(u'a', Qt::CaseSensitive));
    QVERIFY( !a.startsWith(QChar(), Qt::CaseSensitive) );
    QVERIFY( !a.startsWith(QLatin1Char(0), Qt::CaseSensitive) );

#define TEST_VIEW_STARTS_WITH(string, yes) { CREATE_VIEW(string); QCOMPARE(a.startsWith(view), yes); }
    TEST_VIEW_STARTS_WITH(u"A"_s, true);
    TEST_VIEW_STARTS_WITH(u"AB"_s, true);
    TEST_VIEW_STARTS_WITH(u"C"_s, false);
    TEST_VIEW_STARTS_WITH(u"ABCDEF"_s, false);
#undef TEST_VIEW_STARTS_WITH

    a = u""_s;
    QVERIFY(a.startsWith(u""_s));
    QVERIFY( a.startsWith(QString()) );
    QVERIFY(!a.startsWith(u"ABC"_s));

    QVERIFY( a.startsWith(QLatin1String("")) );
    QVERIFY( a.startsWith(QLatin1String(nullptr)) );
    QVERIFY( !a.startsWith(QLatin1String("ABC")) );

    QVERIFY( !a.startsWith(QLatin1Char(0)) );
    QVERIFY( !a.startsWith(QLatin1Char('x')) );
    QVERIFY( !a.startsWith(QChar()) );

    a = QString();
    QVERIFY( !a.startsWith(u""_s) );
    QVERIFY( a.startsWith(QString()) );
    QVERIFY(!a.startsWith(u"ABC"_s));

    QVERIFY( !a.startsWith(QLatin1String("")) );
    QVERIFY( a.startsWith(QLatin1String(nullptr)) );
    QVERIFY( !a.startsWith(QLatin1String("ABC")) );

    QVERIFY( !a.startsWith(QLatin1Char(0)) );
    QVERIFY( !a.startsWith(QLatin1Char('x')) );
    QVERIFY( !a.startsWith(QChar()) );

    // this test is independent of encoding
    a = u'é';
    QVERIFY(a.startsWith(u"é"_s));
    QVERIFY(!a.startsWith(u"á"_s));

    // this one is dependent of encoding
    QVERIFY(a.startsWith(u"É"_s, Qt::CaseInsensitive));
}

void tst_QString::endsWith()
{
    QString a;

    QVERIFY(!a.endsWith(u'A'));
    QVERIFY(!a.endsWith(u"AB"_s));
    {
        CREATE_VIEW(u"AB"_s);
        QVERIFY(!a.endsWith(view));
    }
    QVERIFY(!a.isDetached());

    a = u"AB"_s;
    QVERIFY( a.endsWith(u"B"_s) );
    QVERIFY( a.endsWith(u"AB"_s) );
    QVERIFY( !a.endsWith(u"C"_s) );
    QVERIFY( !a.endsWith(u"ABCDEF"_s) );
    QVERIFY( a.endsWith(u""_s) );
    QVERIFY( a.endsWith(QString()) );
    QVERIFY( a.endsWith(u'B') );
    QVERIFY( a.endsWith(QLatin1Char('B')) );
    QVERIFY( a.endsWith(QChar(u'B')) );
    QVERIFY( !a.endsWith(u'C') );
    QVERIFY( !a.endsWith(QChar()) );
    QVERIFY( !a.endsWith(QLatin1Char(0)) );

    QVERIFY( a.endsWith(QLatin1String("B")) );
    QVERIFY( a.endsWith(QLatin1String("AB")) );
    QVERIFY( !a.endsWith(QLatin1String("C")) );
    QVERIFY( !a.endsWith(QLatin1String("ABCDEF")) );
    QVERIFY( a.endsWith(QLatin1String("")) );
    QVERIFY( a.endsWith(QLatin1String(nullptr)) );

    QVERIFY( a.endsWith(u"B"_s, Qt::CaseSensitive) );
    QVERIFY( a.endsWith(u"B", Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(u"b", Qt::CaseSensitive) );
    QVERIFY( a.endsWith(u"b"_s, Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(u"aB"_s, Qt::CaseSensitive) );
    QVERIFY( a.endsWith(u"aB"_s, Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(u"C"_s, Qt::CaseSensitive) );
    QVERIFY( !a.endsWith(u"C"_s, Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(u"c"_s, Qt::CaseSensitive) );
    QVERIFY( !a.endsWith(u"c"_s, Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(u"abcdef"_s, Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(u""_s, Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(QString(), Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(u'b', Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(u'B', Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(QLatin1Char('b'), Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(QChar(u'b'), Qt::CaseInsensitive) );
    QVERIFY( !a.endsWith(u'c', Qt::CaseInsensitive) );
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
    QVERIFY( a.endsWith(QLatin1String(nullptr), Qt::CaseInsensitive) );
    QVERIFY( a.endsWith(u'B', Qt::CaseSensitive) );
    QVERIFY( a.endsWith(QLatin1Char('B'), Qt::CaseSensitive) );
    QVERIFY( a.endsWith(QChar(u'B'), Qt::CaseSensitive) );
    QVERIFY( !a.endsWith(u'b', Qt::CaseSensitive) );
    QVERIFY( !a.endsWith(QChar(), Qt::CaseSensitive) );
    QVERIFY( !a.endsWith(QLatin1Char(0), Qt::CaseSensitive) );

#define TEST_VIEW_ENDS_WITH(string, yes) { CREATE_VIEW(string); QCOMPARE(a.endsWith(view), yes); }
    TEST_VIEW_ENDS_WITH(QLatin1String("B"), true);
    TEST_VIEW_ENDS_WITH(QLatin1String("AB"), true);
    TEST_VIEW_ENDS_WITH(QLatin1String("C"), false);
    TEST_VIEW_ENDS_WITH(QLatin1String("ABCDEF"), false);
    TEST_VIEW_ENDS_WITH(QLatin1String(""), true);
    TEST_VIEW_ENDS_WITH(QLatin1String(nullptr), true);
#undef TEST_VIEW_ENDS_WITH

    a = u""_s;
    QVERIFY( a.endsWith(u""_s) );
    QVERIFY( a.endsWith(QString()) );
    QVERIFY( !a.endsWith(u"ABC"_s) );
    QVERIFY( !a.endsWith(QLatin1Char(0)) );
    QVERIFY( !a.endsWith(QLatin1Char('x')) );
    QVERIFY( !a.endsWith(QChar()) );

    QVERIFY( a.endsWith(QLatin1String("")) );
    QVERIFY( a.endsWith(QLatin1String(nullptr)) );
    QVERIFY( !a.endsWith(QLatin1String("ABC")) );

    a = QString();
    QVERIFY( !a.endsWith(u""_s) );
    QVERIFY( a.endsWith(QString()) );
    QVERIFY( !a.endsWith(u"ABC"_s) );

    QVERIFY( !a.endsWith(QLatin1String("")) );
    QVERIFY( a.endsWith(QLatin1String(nullptr)) );
    QVERIFY( !a.endsWith(QLatin1String("ABC")) );

    QVERIFY( !a.endsWith(QLatin1Char(0)) );
    QVERIFY( !a.endsWith(QLatin1Char('x')) );
    QVERIFY( !a.endsWith(QChar()) );

    // this test is independent of encoding
    a = u'é';
    QVERIFY(a.endsWith(u"é"_s));
    QVERIFY(!a.endsWith(u"á"_s));

    // this one is dependent of encoding
    QVERIFY(a.endsWith(u"É"_s, Qt::CaseInsensitive));
}

void tst_QString::check_QDataStream()
{
    QString a;
    QByteArray ar;
    {
        QDataStream out(&ar,QIODevice::WriteOnly);
        out << u"COMPARE Text"_s;
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
        out << u"This is COMPARE Text"_s;
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
        a = u""_s;
        QTextStream ts(&a);
        // invalid Utf8
        ts << "pi \261= " << 3.125;
        QCOMPARE(a, QString::fromUtf16(u"pi \xfffd= 3.125"));
    }
    {
        a = u""_s;
        QTextStream ts(&a);
        // valid Utf8
        ts << "pi ø= " << 3.125;
        QCOMPARE(a, QString::fromUtf16(u"pi ø= 3.125"));
    }
    {
        a = u"123 456"_s;
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

    // This actually tests the recycling of the shared data object
    QString::DataPointer csd = cstr.data_ptr();
    cstr.setRawData(ptr2, 1);
    QEXPECT_FAIL("", "This is currently not working: QTBUG-94450.", Continue);
    QVERIFY(cstr.isDetached());
    QVERIFY(cstr.constData() == ptr2);
    QVERIFY(cstr == QString(ptr2, 1));
    QEXPECT_FAIL("", "This is currently not working: QTBUG-94450.", Continue);
    QVERIFY(cstr.data_ptr() == csd);

    // This tests the discarding of the shared data object
    cstr = QString::fromUtf8("foo");
    QVERIFY(cstr.isDetached());
    QVERIFY(cstr.constData() != ptr2);

    // Another test of the fallback
    csd = cstr.data_ptr();
    cstr.setRawData(ptr2, 1);
    QEXPECT_FAIL("", "This is currently not working: QTBUG-94450.", Continue);
    QVERIFY(cstr.isDetached());
    QVERIFY(cstr.constData() == ptr2);
    QVERIFY(cstr == QString(ptr2, 1));
    QVERIFY(cstr.data_ptr() != csd);
}

void tst_QString::nullTerminated()
{
    const QChar ptr[] = { u'ሴ', u'ʎ', u'\0' };

    QTest::ThrowOnFailEnabler thrower;

    auto check = [ptr] (const QString &r) {
        QVERIFY(r.constData() != ptr);
        QCOMPARE(r.constData()[0], ptr[0]);
        QCOMPARE(r.constData()[1], ptr[1]);
        QCOMPARE(r.constData()[2], u'\0');
        QCOMPARE(r.size(), 2);
    };

    {
        QString str = QString::fromRawData(ptr, 2);
        QCOMPARE(str.constData(), ptr);
        QCOMPARE(str.constData()[0], ptr[0]);
        QCOMPARE(str.constData()[1], ptr[1]);
        QCOMPARE(str.size(), 2);

        check(str.nullTerminated());
        check(QString::fromRawData(ptr, 2).nullTerminated()); // rvalue
    }

    {
        QString str = QString::fromRawData(ptr, 2);
        QCOMPARE(str.constData(), ptr);
        QCOMPARE(str.constData()[0], ptr[0]);
        QCOMPARE(str.constData()[1], ptr[1]);
        QCOMPARE(str.size(), 2);

        check(str.nullTerminate());
    }
}

void tst_QString::setUnicode()
{
    const QChar ptr[] = { u'ሴ', QChar(0x0000) };
    const char16_t utf16[] = { u'ሴ', 0x0000 };

    QTest::ThrowOnFailEnabler throwOnFail;

    auto doTest = [](const auto ptr, QString &str) mutable {
        // make sure that the data was copied
        QCOMPARE_NE(str.constData(), reinterpret_cast<const QChar *>(ptr));
        QVERIFY(str.isDetached());
        QCOMPARE(str, QString(reinterpret_cast<const QChar *>(ptr), 1));

        // make sure that the string is resized, even if the data is nullptr
        str = u"test"_s;
        QCOMPARE(str.size(), 4);
        str.setUnicode(nullptr, 1);
        QCOMPARE(str.size(), 1);
        QCOMPARE(str, u"t");
    };

    {
        QString str;
        QVERIFY(!str.isDetached());
        str.setUnicode(ptr, 1);
        doTest(ptr, str);
        str.setUnicode(nullptr, 0);
    }

    {
        QString str;
        QVERIFY(!str.isDetached());
        str.setUnicode(utf16, 1);
        doTest(utf16, str);
        str.setUnicode(nullptr, 0);
    }

    {
        QString str;
        QVERIFY(!str.isDetached());
        str.setUtf16(utf16, 1);
        doTest(utf16, str);
        str.setUtf16(nullptr, 0);
    }
}

void tst_QString::fromStdString()
{
    QVERIFY(QString::fromStdString(std::string()).isEmpty());
    std::string stroustrup = "foo";
    QString eng = QString::fromStdString( stroustrup );
    QCOMPARE( eng, u"foo"_s );
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

    QString emptyStr(u""_s);
    QVERIFY(emptyStr.toStdString().empty());
    QVERIFY(!emptyStr.isDetached());

    QString nord = u"foo"_s;
    std::string stroustrup1 = nord.toStdString();
    QVERIFY( qstrcmp(stroustrup1.c_str(), "foo") == 0 );

    // For now, most QString constructors are also broken with respect
    // to embedded null characters, had to find one that works...
    const char16_t utf16[] = u"Embedded\0null\0character!";
    const size_t size = std::size(utf16) - 1; // - 1, null terminator of the string literal
    QString qtnull(reinterpret_cast<const QChar *>(utf16), size);

    std::string stdnull = qtnull.toStdString();
    QCOMPARE(int(stdnull.size()), qtnull.size());

    std::u16string stdu16null = qtnull.toStdU16String();
    QCOMPARE(int(stdu16null.size()), qtnull.size());
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

    QTest::newRow("str0") << QByteArray("abcdefgh") << u"abcdefgh"_s << -1;
    QTest::newRow("str0-len") << QByteArray("abcdefgh") << u"abc"_s << 3;
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
    str += u" some text"_s;
    QTest::newRow("str3") << QByteArray("\342\202\254 some text") << str << -1;

    str = QChar(0x20ac);
    str += u" some "_s;
    QTest::newRow("str3-len") << QByteArray("\342\202\254 some text") << str << 9;

    // test that QString::fromUtf8 suppresses an initial BOM, but not a ZWNBSP
    str = u"hello"_s;
    QByteArray bom("\357\273\277");
    QTest::newRow("bom0") << bom << QString() << 3;
    QTest::newRow("bom1") << bom + "hello" << str << -1;
    QTest::newRow("bom+zwnbsp0") << bom + bom << QString(QChar(0xfeff)) << -1;
    QTest::newRow("bom+zwnbsp1") << bom + "hello" + bom << str + QChar(0xfeff) << -1;

    str = u"hello"_s;
    str += QChar::ReplacementCharacter;
    str += QChar(0x68);
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += QChar(0x61);
    str += QChar::ReplacementCharacter;
    QTest::newRow("invalid utf8") << QByteArray("hello\344h\344\344\366\344a\304") << str << -1;
    QTest::newRow("invalid utf8-len") << QByteArray("hello\344h\344\344\366\344a\304") << u"hello"_s << 5;

    str = u"Prohl"_s;
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += u"e"_s;
    str += QChar::ReplacementCharacter;
    str += u" plugin"_s;
    str += QChar::ReplacementCharacter;
    str += u" Netscape"_s;

    QTest::newRow("invalid utf8 2") << QByteArray("Prohl\355\276e\350 plugin\371 Netscape") << str << -1;
    QTest::newRow("invalid utf8-len 2") << QByteArray("Prohl\355\276e\350 plugin\371 Netscape") << u""_s << 0;

    QTest::newRow("null-1") << QByteArray() << QString() << -1;
    QTest::newRow("null0") << QByteArray() << QString() << 0;
    QTest::newRow("empty-1") << QByteArray("\0abcd", 5) << QString() << -1;
    QTest::newRow("empty5") << QByteArray("\0abcd", 5) << QString::fromLatin1("\0abcd", 5) << 5;
    QTest::newRow("other-1") << QByteArray("ab\0cd", 5) << QString::fromLatin1("ab") << -1;
    QTest::newRow("other5") << QByteArray("ab\0cd", 5) << QString::fromLatin1("ab\0cd", 5) << 5;

    str = u"Old Italic: "_s;
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
        longQString += u"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"_s;
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

    QCOMPARE(QString::fromLocal8Bit(local8Bit.isNull() ? 0 : local8Bit.data(), len).size(),
            result.size());
    QCOMPARE(QString::fromLocal8Bit(local8Bit.isNull() ? 0 : local8Bit.data(), len), result);
}

void tst_QString::local8Bit_data()
{
    QTest::addColumn<QString>("local8Bit");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("nullString") << QString() << QByteArray();
    QTest::newRow("emptyString") << u""_s << QByteArray("");
    QTest::newRow("string") << u"test"_s << QByteArray("test");

    QByteArray longQByteArray;
    QString longQString;

    for (int l=0;l<111;l++) {
        longQByteArray = longQByteArray + "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        longQString += u"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"_s;
    }

    QTest::newRow("longString") << longQString << longQByteArray;
    QTest::newRow("someNonAlphaChars") << u"d:/this/is/a/test.h"_s << QByteArray("d:/this/is/a/test.h");
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
        const QChar malformed[] = { u'A', QChar(0xd800), u'B', u'\0' };
        const char expected[] = "A";
        QTest::newRow("LoneHighSurrogate")
            << QString(malformed, sizeof(malformed) / sizeof(QChar))
            // Don't include the terminating '\0' of expected:
            << QByteArray(expected, sizeof(expected) / sizeof(char) - 1);
    }
    {
        const QChar malformed[] = { u'A', QChar(0xdc00), u'B', u'\0' };
        const char expected[] = "A";
        QTest::newRow("LoneLowSurrogate")
            << QString(malformed, sizeof(malformed) / sizeof(QChar))
            << QByteArray(expected, sizeof(expected) / sizeof(char) - 1);
    }
    {
        const QChar malformed[] = { u'A', QChar(0xd800), QChar(0xd801), u'B', u'\0' };
        const char expected[] = "A";
        QTest::newRow("DoubleHighSurrogate")
            << QString(malformed, sizeof(malformed) / sizeof(QChar))
            << QByteArray(expected, sizeof(expected) / sizeof(char) - 1);
    }
    {
        const QChar malformed[] = { u'A', QChar(0xdc00), QChar(0xdc01), u'B', u'\0' };
        const char expected[] = "A";
        QTest::newRow("DoubleLowSurrogate")
            << QString(malformed, sizeof(malformed) / sizeof(QChar))
            << QByteArray(expected, sizeof(expected) / sizeof(char) - 1);
    }
    {
        const QChar malformed[] = { u'A', QChar(0xdc00), QChar(0xd800), u'B', u'\0' };
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
    QCOMPARE(latin1.size(), unicode.size());

    auto roundtripTest = [&]() {
        // fromLatin1
        QString fromLatin1 = QString::fromLatin1(latin1, latin1.length());
        QCOMPARE(fromLatin1.length(), unicode.length());
        QCOMPARE(fromLatin1, unicode);

        // and back:
        QByteArray toLatin1 = unicode.toLatin1();
        QCOMPARE(toLatin1.length(), latin1.length());
        QCOMPARE(toLatin1, latin1);
    };

    roundtripTest();

    if (latin1.isEmpty())
        return;

    if (QTest::currentTestFailed()) QFAIL("failed");
    while (latin1.length() < 16) {
        latin1 += latin1;
        unicode += unicode;
    }
    roundtripTest();

    // double again (length will be > 32)
    if (QTest::currentTestFailed()) QFAIL("failed");
    latin1 += latin1;
    unicode += unicode;
    roundtripTest();

    // double again (length will be > 64)
    if (QTest::currentTestFailed()) QFAIL("failed");
    latin1 += latin1;
    unicode += unicode;
    roundtripTest();

    if (QTest::currentTestFailed()) QFAIL("failed");
    latin1 += latin1;
    unicode += unicode;
    roundtripTest();
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
    QCOMPARE(latin1.size(), unicodesrc.size());
    QCOMPARE(latin1.isNull(), unicodedst.isNull());
    QCOMPARE(latin1.isEmpty(), unicodedst.isEmpty());
    QCOMPARE(latin1.size(), unicodedst.size());

    if (!latin1.isEmpty())
        while (latin1.size() < 128) {
            latin1 += latin1;
            unicodesrc += unicodesrc;
            unicodedst += unicodedst;
        }

    // toLatin1
    QCOMPARE(unicodesrc.toLatin1().size(), latin1.size());
    QCOMPARE(unicodesrc.toLatin1(), latin1);

    // and back:
    QCOMPARE(QString::fromLatin1(latin1, latin1.size()).size(), unicodedst.size());
    QCOMPARE(QString::fromLatin1(latin1, latin1.size()), unicodedst);

    // try the rvalue version of toLatin1()
    QString s = unicodesrc;
    QCOMPARE(std::move(s).toLatin1(), latin1);

    // and verify that the moved-from object can still be used
    s = u"foo"_s;
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
    QCOMPARE(s, u"Hello Unicode World");

    s = QString::fromUcs4(str1);
    QCOMPARE(s, u"Hello Unicode World");

    s = QString::fromUcs4(str1, 5);
    QCOMPARE(s, u"Hello");

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

    TransientDefaultLocale transient(QLocale(u"de_DE"));

    QString s3;
    QString s4(u"[%0]"_s);
    QString s5(u"[%1]"_s);
    QString s6(u"[%3]"_s);
    QString s7(u"[%9]"_s);
    QString s8(u"[%0 %1]"_s);
    QString s9(u"[%0 %3]"_s);
    QString s10(u"[%1 %2 %3]"_s);
    QString s11(u"[%9 %3 %0]"_s);
    QString s12(u"[%9 %1 %3 %9 %0 %8]"_s);
    QString s13(u"%1% %x%c%2 %d%2-%"_s);
    QString s14(u"%1%2%3"_s);

    const QString null;
    const QString empty(u""_s);
    const QString foo(u"foo"_s);
    const QString bar(u"bar"_s);

    Q_ASSERT(null.isNull());
    Q_ASSERT(!empty.isNull());
    QCOMPARE(s4.arg(null), "[]"_L1);
    QCOMPARE(s4.arg(empty), "[]"_L1);
    QCOMPARE(s4.arg(QStringView()), "[]"_L1);
    QCOMPARE(s4.arg(QStringView(u"")), "[]"_L1);
    QCOMPARE(s4.arg(u8""), "[]"_L1);

    QCOMPARE(s4.arg(foo), "[foo]"_L1);
    QCOMPARE( s5.arg(QLatin1String("foo")), QLatin1String("[foo]") );
    QCOMPARE( s6.arg(u"foo"), QLatin1String("[foo]") );
    QCOMPARE(s7.arg(foo), "[foo]"_L1);
    QCOMPARE(s8.arg(foo), "[foo %1]"_L1);
    QCOMPARE(s8.arg(foo).arg(bar), "[foo bar]"_L1);
    QCOMPARE(s8.arg(foo, bar), "[foo bar]"_L1);
    QCOMPARE(s9.arg(foo), "[foo %3]"_L1);
    QCOMPARE(s9.arg(foo).arg(bar), "[foo bar]"_L1);
    QCOMPARE(s9.arg(foo, bar), "[foo bar]"_L1);
    QCOMPARE(s10.arg(foo), "[foo %2 %3]"_L1);
    QCOMPARE(s10.arg(foo).arg(bar), "[foo bar %3]"_L1);
    QCOMPARE(s10.arg(foo, bar), "[foo bar %3]"_L1);
    QCOMPARE(s10.arg(foo).arg(bar).arg(u"baz"_s), "[foo bar baz]"_L1);
    QCOMPARE(s10.arg(foo, bar, u"baz"_s), "[foo bar baz]"_L1);
    QCOMPARE(s11.arg(foo), "[%9 %3 foo]"_L1);
    QCOMPARE(s11.arg(foo).arg(bar), "[%9 bar foo]"_L1);
    QCOMPARE(s11.arg(foo, bar), "[%9 bar foo]"_L1);
    QCOMPARE(s11.arg(foo).arg(bar).arg(u"baz"_s), "[baz bar foo]"_L1);
    QCOMPARE(s11.arg(foo, bar, u"baz"_s), "[baz bar foo]"_L1);
    QCOMPARE( s12.arg(u"a"_s).arg(u"b"_s).arg(u"c"_s).arg(u"d"_s).arg(u"e"_s),
             QLatin1String("[e b c e a d]") );
    QCOMPARE(s12.arg(u"a"_s, u"b"_s, u"c"_s, u"d"_s).arg(u"e"_s), "[e b c e a d]"_L1);
    QCOMPARE(s12.arg(u"a"_s).arg(u"b"_s, u"c"_s, u"d"_s, u"e"_s), "[e b c e a d]"_L1);
    QCOMPARE( s13.arg(u"alpha"_s).arg(u"beta"_s),
             QLatin1String("alpha% %x%cbeta %dbeta-%") );
    QCOMPARE(s13.arg(u"alpha"_s, u"beta"_s), "alpha% %x%cbeta %dbeta-%"_L1);
    QCOMPARE(s14.arg(u"a"_s, u"b"_s, u"c"_s), "abc"_L1);
    QCOMPARE(s8.arg(u"%1"_s).arg(foo), "[foo foo]"_L1);
    QCOMPARE(s8.arg(u"%1"_s, foo), "[%1 foo]"_L1);
    QCOMPARE(s4.arg(foo, 2), "[foo]"_L1);
    QCOMPARE(s4.arg(foo, -2), "[foo]"_L1);
    QCOMPARE(s4.arg(foo, 10), "[       foo]"_L1);
    QCOMPARE(s4.arg(foo, -10), "[foo       ]"_L1);

    QString firstName(u"James"_s);
    QString lastName(u"Bond"_s);
    QString fullName = QString(u"My name is %2, %1 %2"_s).arg(firstName).arg(lastName);
    QCOMPARE(fullName, QLatin1String("My name is Bond, James Bond"));

    // ### Qt 7: clean this up, leave just the #else branch
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
    static const QRegularExpression nonAsciiArgWarning("QString::arg\\(\\): the replacement \".*\" contains non-ASCII digits");
    QTest::ignoreMessage(QtWarningMsg, nonAsciiArgWarning);
    QCOMPARE( QString("%¹").arg("foo"), QString("foo") );
    QTest::ignoreMessage(QtWarningMsg, nonAsciiArgWarning);
    QCOMPARE( QString("%¹%1").arg("foo"), QString("foofoo") );
    QTest::ignoreMessage(QtWarningMsg, nonAsciiArgWarning);
    QCOMPARE( QString("%1²").arg("E=mc"), QString("E=mc") );
    QTest::ignoreMessage(QtWarningMsg, nonAsciiArgWarning);
    QTest::ignoreMessage(QtWarningMsg, nonAsciiArgWarning);
    QCOMPARE( QString("%1²%2").arg("a").arg("b"), QString("ba") );
    QTest::ignoreMessage(QtWarningMsg, nonAsciiArgWarning);
    QTest::ignoreMessage(QtWarningMsg, nonAsciiArgWarning);
    QTest::ignoreMessage(QtWarningMsg, nonAsciiArgWarning);
    QCOMPARE( QString("%¹%1²%2").arg("a").arg("b"), QString("a%1²b") );
    QTest::ignoreMessage(QtWarningMsg, nonAsciiArgWarning);
    QTest::ignoreMessage(QtWarningMsg, nonAsciiArgWarning);
    QCOMPARE( QString("%2²%1").arg("a").arg("b"), QString("ba") );
#else
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"%¹\", \"foo\"");
    QCOMPARE(u"%¹"_s.arg(foo), u"%¹");
    QCOMPARE(u"%¹%1"_s.arg(foo), u"%¹foo");
    QCOMPARE(u"%1²"_s.arg(u"E=mc"_s), u"E=mc²");
    QCOMPARE(u"%1²%2"_s.arg(u"a"_s).arg(u"b"_s), u"a²b");
    QCOMPARE(u"%¹%1²%2"_s.arg(u"a"_s).arg(u"b"_s), u"%¹a²b");
    QCOMPARE(u"%2²%1"_s.arg(u"a"_s).arg(u"b"_s), u"b²a");
#endif

    // number overloads
    QCOMPARE( s4.arg(0), QLatin1String("[0]") );
    QCOMPARE( s4.arg(-1), QLatin1String("[-1]") );
    QCOMPARE( s4.arg(0U), QLatin1String("[0]"));
    QCOMPARE( s4.arg(qint8(-128)), QLatin1String("[-128]")); // signed char
    QCOMPARE( s4.arg(quint8(255)), QLatin1String("[255]"));  // unsigned char
    QCOMPARE( s4.arg(short(-4200)), QLatin1String("[-4200]"));
    QCOMPARE( s4.arg(ushort(42000)), QLatin1String("[42000]"));
    QCOMPARE( s4.arg(4294967295UL), QLatin1String("[4294967295]") ); // ULONG_MAX 32
    QCOMPARE( s4.arg(Q_INT64_C(9223372036854775807)), // LLONG_MAX
             QLatin1String("[9223372036854775807]") );
    QCOMPARE( s4.arg(Q_UINT64_C(9223372036854775808)), // LLONG_MAX + 1
             QLatin1String("[9223372036854775808]") );

    // (unscoped) enums
    enum Enum {
        Foo1, Foo2,           // reproducer for QTBUG-131906,
        RangeExtended = -667, // but w/o the UB of out-of-range values
    };
    QCOMPARE(s4.arg(Enum(-666)), QLatin1String("[-666]"));
    enum : int { FooS = -1 };
    enum : uint { FooU = 1 };
    QCOMPARE(s4.arg(FooS), QLatin1String("[-1]"));
    QCOMPARE(s4.arg(FooU), QLatin1String("[1]"));

    // FP overloads
    QCOMPARE(s4.arg(2.25), QLatin1String("[2.25]"));
    QCOMPARE(s4.arg(3.75f), QLatin1String("[3.75]"));
    QCOMPARE(s4.arg(qfloat16{4.125f}), QLatin1String("[4.125]"));

    // char-ish overloads
    QCOMPARE(s4.arg('\xE4'), QStringView(u"[ä]"));
    QCOMPARE(s4.arg(u'ø'), QStringView(u"[ø]"));
    QCOMPARE(QLatin1String("[%1]").arg(L'ø'), QStringView(u"[ø]"));
    QCOMPARE(s4.arg(L'ø'), QStringView(u"[ø]"));
#ifndef __cpp_char8_t
#ifndef QT_NO_CAST_FROM_ASCII
    QCOMPARE(QLatin1String("[%1]").arg(u8'a'), QLatin1String("[a]"));
#endif
#endif
    QCOMPARE(s4.arg(u8'a'), QLatin1String("[a]"));

    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"\", \"foo\"");
    QCOMPARE(QString().arg(foo), QString());
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"\", 0");
    QCOMPARE( QString().arg(0), QString() );
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"\", 0");
    QCOMPARE( QString().arg(0U), QString() );
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"\", 0");
    QCOMPARE( QString().arg(0.0), QString() );
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"\", 0");
    QCOMPARE(QString(u""_s).arg(0), u""_s);
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \" \", 0");
    QCOMPARE(QString(u" "_s).arg(0), " "_L1);
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"%\", 0");
    QCOMPARE(QString(u"%"_s).arg(0), "%"_L1);
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"%%\", 0");
    QCOMPARE(QString(u"%%"_s).arg(0), "%%"_L1);
    QTest::ignoreMessage(QtWarningMsg, "QString::arg: Argument missing: \"%%%\", 0");
    QCOMPARE(QString(u"%%%"_s).arg(0), "%%%"_L1);
    QCOMPARE(QString(u"%%%1%%%2"_s).arg(foo).arg(bar), "%%foo%%bar"_L1);

    QCOMPARE(u"%1"_s.arg(null, 3), "   "_L1);
    QCOMPARE(u"%1"_s.arg(empty, 3), "   "_L1);
    QCOMPARE(u"%1"_s.arg(QStringView(), 3), "   "_L1);
    QCOMPARE(u"%1"_s.arg(QStringView(u""), 3), "   "_L1);
    QCOMPARE(u"%1%1"_s.arg(null), ""_L1);
    QCOMPARE(u"%2%1"_s.arg(empty), "%2"_L1);
    QCOMPARE(u"%2%1"_s.arg(QStringView()), "%2"_L1);
    QCOMPARE(u"%2%1"_s.arg(QStringView(u"")), "%2"_L1);
    QCOMPARE(u"%1"_s.arg(u"hello"_s, -10), "hello     "_L1);
    QCOMPARE(u"%1"_s.arg(QUtf8StringView{u8"ä"}, -3), u"ä  ");
    QCOMPARE(u"%1"_s.arg("hello"_L1, -5), "hello"_L1);
    QCOMPARE(u"%1"_s.arg(u"hello", -2), "hello"_L1);
    QCOMPARE(u"%1"_s.arg(u"hello"_s, 0), "hello"_L1);
    QCOMPARE(u"%1"_s.arg("hello"_L1, 2), "hello"_L1);
    QCOMPARE(u"%1"_s.arg(u"hello", 5), "hello"_L1);
    QCOMPARE(u"%1"_s.arg(u"hello"_s, 10), "     hello"_L1);
    QCOMPARE(u"%1"_s.arg(QUtf8StringView{u8"ä"}, 3), u"  ä");
    QCOMPARE(u"%1%1"_s.arg(u"hello"_s), "hellohello"_L1);
    QCOMPARE(u"%2%1"_s.arg(u"hello"_s), "%2hello"_L1);

    QCOMPARE( QString(u"%2 %L1"_s).arg(12345.6789).arg(12345.6789),
             QLatin1String("12345.7 12.345,7") );
    QCOMPARE( QString(u"[%2] [%L1]"_s).arg(12345.6789, 9).arg(12345.6789, 9),
              QLatin1String("[  12345.7] [ 12.345,7]") );
    QCOMPARE( QString(u"[%2] [%L1]"_s).arg(12345.6789, 9, 'g', 7).arg(12345.6789, 9, 'g', 7),
              QLatin1String("[ 12345.68] [12.345,68]") );
    QCOMPARE( QString(u"[%2] [%L1]"_s).arg(12345.6789, 10, 'g', 7, QLatin1Char('0')).arg(12345.6789, 10, 'g', 7, QLatin1Char('0')),
              QLatin1String("[0012345.68] [012.345,68]") );

    QCOMPARE( QString(u"%2 %L1"_s).arg(123456789).arg(123456789),
             QLatin1String("123456789 123.456.789") );
    QCOMPARE( QString(u"[%2] [%L1]"_s).arg(123456789, 12).arg(123456789, 12),
              QLatin1String("[   123456789] [ 123.456.789]") );
    QCOMPARE( QString(u"[%2] [%L1]"_s).arg(123456789, 13, 10, QLatin1Char('0')).arg(123456789, 12, 10, QLatin1Char('0')),
              QLatin1String("[000123456789] [00123.456.789]") );
    QCOMPARE( QString(u"[%2] [%L1]"_s).arg(123456789, 13, 16, QLatin1Char('0')).arg(123456789, 12, 16, QLatin1Char('0')),
              QLatin1String("[0000075bcd15] [00000075bcd15]") );

    QCOMPARE( QString(u"%L2 %L1 %3"_s).arg(12345.7).arg(123456789).arg('c'),
             QLatin1String("123.456.789 12.345,7 c") );

    // multi-digit replacement
    QString input(u"%%%L0 %1 %02 %3 %4 %5 %L6 %7 %8 %%% %090 %10 %11 %L12 %14 %L9888 %9999 %%%%%%%L"_s);
    input = input.arg(u"A"_s).arg(u"B"_s).arg(u"C"_s)
                 .arg(u"D"_s).arg(u"E"_s).arg(u"f"_s)
                 .arg(u"g"_s).arg(u"h"_s).arg(u"i"_s).arg(u"j"_s)
                 .arg(u"k"_s).arg(u"l"_s).arg(u"m"_s)
                 .arg(u"n"_s).arg(u"o"_s).arg(u"p"_s);

    QCOMPARE(input, QLatin1String("%%A B C D E f g h i %%% j0 k l m n o88 p99 %%%%%%%L"));

    QString str(u"%1 %2 %3 %4 %5 %6 %7 %8 %9 foo %10 %11 bar"_s);
    str = str.arg(u"one"_s, u"2"_s, u"3"_s, u"4"_s, u"5"_s, u"6"_s, u"7"_s, u"8"_s, u"9"_s);
    str = str.arg(u"ahoy"_s, u"there"_s);
    QCOMPARE(str, "one 2 3 4 5 6 7 8 9 foo ahoy there bar"_L1);

    // Make sure the single- and multi-arg expand the same sequences: at most
    // two digits. The sequence below has four replacements: %01, %10 (twice),
    // %11, and %12.
    QString str2 = u"%100 %101 %110 %12 %0100"_s;
    QLatin1StringView str2expected = "B0 B1 C0 D A00"_L1;
    QCOMPARE(str2.arg(QChar(u'A')).arg(QChar(u'B')).arg(QChar(u'C')).arg(QChar(u'D')), str2expected);
    QCOMPARE(str2.arg(QChar(u'A'), QChar(u'B')).arg(QChar(u'C')).arg(QChar(u'D')), str2expected);
    QCOMPARE(str2.arg(QChar(u'A'), QChar(u'B'), QChar(u'C')).arg(QChar(u'D')), str2expected);
    QCOMPARE(str2.arg(QChar(u'A'), QChar(u'B'), QChar(u'C'), QChar(u'D')), str2expected);

    QCOMPARE(u"%1"_s.arg(-1, 3, 10, QChar(u'0')), "-01"_L1);
    QCOMPARE(u"%1"_s.arg(-100, 3, 10, QChar(u'0')), "-100"_L1);
    QCOMPARE(u"%1"_s.arg(-1, 3, 10, QChar(u' ')), " -1"_L1);
    QCOMPARE(u"%1"_s.arg(-100, 3, 10, QChar(u' ')), "-100"_L1);
    QCOMPARE(u"%1"_s.arg(1U, 3, 10, QChar(u' ')), "  1"_L1);
    QCOMPARE(u"%1"_s.arg(1000U, 3, 10, QChar(u' ')), "1000"_L1);
    QCOMPARE(u"%1"_s.arg(-1, 3, 10, QChar(u'x')), "x-1"_L1);
    QCOMPARE(u"%1"_s.arg(-100, 3, 10, QChar(u'x')), "-100"_L1);
    QCOMPARE(u"%1"_s.arg(1U, 3, 10, QChar(u'x')), "xx1"_L1);
    QCOMPARE(u"%1"_s.arg(1000U, 3, 10, QChar(u'x')), "1000"_L1);

    QCOMPARE(u"%1"_s.arg(-1., 3, 'g', -1, QChar(u'0')), "-01"_L1);
    QCOMPARE(u"%1"_s.arg(-100., 3, 'g', -1, QChar(u'0')), "-100"_L1);
    QCOMPARE(u"%1"_s.arg(-1., 3, 'g', -1, QChar(u' ')), " -1"_L1);
    QCOMPARE(u"%1"_s.arg(-100., 3, 'g', -1, QChar(u' ')), "-100"_L1);
    QCOMPARE(u"%1"_s.arg(1., 3, 'g', -1, QChar(u'x')), "xx1"_L1);
    QCOMPARE(u"%1"_s.arg(1000., 3, 'g', -1, QChar(u'x')), "1000"_L1);
    QCOMPARE(u"%1"_s.arg(-1., 3, 'g', -1, QChar(u'x')), "x-1"_L1);
    QCOMPARE(u"%1"_s.arg(-100., 3, 'g', -1, QChar(u'x')), "-100"_L1);

    transient.revise(QLocale(u"ar"_s));
    QCOMPARE(u"%L1"_s.arg(12345.6789, 10, 'g', 7, QLatin1Char('0')),
             u"\u0660\u0661\u0662\u066c\u0663\u0664\u0665\u066b\u0666\u0668"); // "٠١٢٬٣٤٥٫٦٨"
    QCOMPARE(u"%L1"_s.arg(123456789, 13, 10, QLatin1Char('0')),
             u"\u0660\u0660\u0661\u0662\u0663\u066c\u0664\u0665\u0666\u066c\u0667\u0668\u0669"); // ٠٠١٢٣٬٤٥٦٬٧٨٩
}

template <typename S, typename...Ts>
using arg_compile_test = decltype(std::declval<S>().arg(std::declval<Ts>()...));
template <typename S, typename...Ts>
constexpr bool arg_compiles_v = qxp::is_detected_v<arg_compile_test, S, Ts...>;

void tst_QString::arg_negative_tests()
{
    static_assert(!arg_compiles_v<QString&, QObject*>);
    // QLatin1StringView::arg() is unconstrained...
    // static_assert(!arg_compiles_v<QLatin1StringView&, QObject*>);

    // integral type called like an FP overload:
    static_assert(!arg_compiles_v<QString&, int, int, char, int, QChar>);
    static_assert(!arg_compiles_v<QString&, long, int, char, int, char16_t>);

    // strong enums don't match:
    enum class Strong {};
    static_assert(!arg_compiles_v<QString&, Strong>);
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
}

void tst_QString::number_double_data()
{
    QTest::addColumn<double>("value");
    QTest::addColumn<char>("format");
    QTest::addColumn<int>("precision");
    QTest::addColumn<QString>("expected");

    // This function is implemented in ../shared/test_number_shared.h
    add_number_double_shared_data([](NumberDoubleTestData datum) {
        const char *title =
                !datum.optTitle.isEmpty() ? datum.optTitle.data() : datum.expected.data();
        QTest::addRow("%s, format '%c', precision %d", title, datum.f, datum.p)
                << datum.d << datum.f << datum.p << datum.expected.toString();
        if (datum.f != 'f') { // Also test uppercase format
            datum.f = QtMiscUtils::toAsciiUpper(datum.f);
            QString upper = datum.expected.toString().toUpper();
            QString upperTitle = QString::fromLatin1(title);
            if (!datum.optTitle.isEmpty())
                upperTitle += u", uppercase"_s;
            else
                upperTitle = upperTitle.toUpper();
            QTest::addRow("%s, format '%c', precision %d", qPrintable(upper), datum.f, datum.p)
                    << datum.d << datum.f << datum.p << upper;
        }
    });
}

void tst_QString::number_double()
{
    QFETCH(double, value);
    QFETCH(char, format);
    QFETCH(int, precision);
    QT_IGNORE_DEPRECATIONS(constexpr bool has_denorm = std::numeric_limits<double>::has_denorm != std::denorm_present;)
    if constexpr (has_denorm) {
        if (::qstrcmp(QTest::currentDataTag(), "Very small number, very high precision, format 'f', precision 350") == 0) {
            QSKIP("Skipping 'denorm' as this type lacks denormals on this system");
        }
    }
    QTEST(QString::number(value, format, precision), "expected");
}

void tst_QString::number_base_data()
{
    QTest::addColumn<qlonglong>("n");
    QTest::addColumn<int>("base");
    QTest::addColumn<QString>("expected");

    QTest::newRow("base 10, positive") << 12346LL << 10 << u"12346"_s;
    QTest::newRow("base  2, positive") << 12346LL <<  2 << u"11000000111010"_s;
    QTest::newRow("base  8, positive") << 12346LL <<  8 << u"30072"_s;
    QTest::newRow("base 16, positive") << 12346LL << 16 << u"303a"_s;
    QTest::newRow("base 17, positive") << 12346LL << 17 << u"28c4"_s;
    QTest::newRow("base 36, positive") << 2181789482LL << 36 << u"102zbje"_s;

    QTest::newRow("base 10, negative") << -12346LL << 10 << u"-12346"_s;
    QTest::newRow("base  2, negative") << -12346LL <<  2 << u"-11000000111010"_s;
    QTest::newRow("base  8, negative") << -12346LL <<  8 << u"-30072"_s;
    QTest::newRow("base 16, negative") << -12346LL << 16 << u"-303a"_s;
    QTest::newRow("base 17, negative") << -12346LL << 17 << u"-28c4"_s;
    QTest::newRow("base 36, negative") << -2181789482LL << 36 << u"-102zbje"_s;

    QTest::newRow("base  2, minus 1") << -1LL << 2 << u"-1"_s;

    QTest::newRow("largeint, base 10, positive")
            << 123456789012LL << 10 << u"123456789012"_s;
    QTest::newRow("largeint, base  2, positive")
            << 123456789012LL <<  2 << u"1110010111110100110010001101000010100"_s;
    QTest::newRow("largeint, base  8, positive")
            << 123456789012LL <<  8 << u"1627646215024"_s;
    QTest::newRow("largeint, base 16, positive")
            << 123456789012LL << 16 << u"1cbe991a14"_s;
    QTest::newRow("largeint, base 17, positive")
            << 123456789012LL << 17 << u"10bec2b629"_s;

    QTest::newRow("largeint, base 10, negative")
            << -123456789012LL << 10 << u"-123456789012"_s;
    QTest::newRow("largeint, base  2, negative")
            << -123456789012LL <<  2 << u"-1110010111110100110010001101000010100"_s;
    QTest::newRow("largeint, base  8, negative")
            << -123456789012LL <<  8 << u"-1627646215024"_s;
    QTest::newRow("largeint, base 16, negative")
            << -123456789012LL << 16 << u"-1cbe991a14"_s;
    QTest::newRow("largeint, base 17, negative")
            << -123456789012LL << 17 << u"-10bec2b629"_s;
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
    QCOMPARE(u"%1"_s.arg(micro), expect);
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

    QTest::newRow("null") << QString() << u","_s << 0 << -1 << int(QString::SectionDefault)
                          << QString() << false;
    QTest::newRow("empty") << u""_s << u","_s << 0 << -1 << int(QString::SectionDefault) << u""_s
                           << false;
    QTest::newRow("data0") << u"forename,middlename,surname,phone"_s << u","_s << 2 << 2
                           << int(QString::SectionDefault) << u"surname"_s << false;
    QTest::newRow("data1") << u"/usr/local/bin/myapp"_s << u"/"_s << 3 << 4
                           << int(QString::SectionDefault) << u"bin/myapp"_s << false;
    QTest::newRow("data2") << u"/usr/local/bin/myapp"_s << u"/"_s << 3 << 3
                           << int(QString::SectionSkipEmpty) << u"myapp"_s << false;
    QTest::newRow("data3") << u"forename**middlename**surname**phone"_s << u"**"_s << 2 << 2
                           << int(QString::SectionDefault) << u"surname"_s << false;
    QTest::newRow("data4") << u"forename**middlename**surname**phone"_s << u"**"_s << -3 << -2
                           << int(QString::SectionDefault) << u"middlename**surname"_s << false;
    QTest::newRow("data5") << u"##Datt######wollen######wir######mal######sehen##"_s << u"#"_s << 0
                           << 0 << int(QString::SectionSkipEmpty) << u"Datt"_s << false;
    QTest::newRow("data6") << u"##Datt######wollen######wir######mal######sehen##"_s << u"#"_s << 1
                           << 1 << int(QString::SectionSkipEmpty) << u"wollen"_s << false;
    QTest::newRow("data7") << u"##Datt######wollen######wir######mal######sehen##"_s << u"#"_s << 2
                           << 2 << int(QString::SectionSkipEmpty) << u"wir"_s << false;
    QTest::newRow("data8") << u"##Datt######wollen######wir######mal######sehen##"_s << u"#"_s << 3
                           << 3 << int(QString::SectionSkipEmpty) << u"mal"_s << false;
    QTest::newRow("data9") << u"##Datt######wollen######wir######mal######sehen##"_s << u"#"_s << 4
                           << 4 << int(QString::SectionSkipEmpty) << u"sehen"_s << false;
    // not fixed for 3.1
    QTest::newRow("data10") << u"a/b/c/d"_s << u"/"_s << 1 << -1
                            << int(QString::SectionIncludeLeadingSep | QString::SectionIncludeTrailingSep)
                            << u"/b/c/d"_s << false;
    QTest::newRow("data11") << u"aoLoboLocolod"_s << u"olo"_s << -1 << -1
                            << int(QString::SectionCaseInsensitiveSeps) << u"d"_s << false;
    QTest::newRow("data12") << u"F0"_s << u"F"_s << 0 << 0 << int(QString::SectionSkipEmpty)
                            << u"0"_s << false;
    QTest::newRow("foo1") << u"foo;foo;"_s << u";"_s << 0 << 0
                          << int(QString::SectionIncludeLeadingSep) << u"foo"_s << false;
    QTest::newRow("foo2") << u"foo;foo;"_s << u";"_s << 1 << 1
                          << int(QString::SectionIncludeLeadingSep) << u";foo"_s << false;
    QTest::newRow("foo3") << u"foo;foo;"_s << u";"_s << 2 << 2
                          << int(QString::SectionIncludeLeadingSep) << u";"_s << false;
    QTest::newRow("foo1rx") << u"foo;foo;"_s << u";"_s << 0 << 0
                            << int(QString::SectionIncludeLeadingSep) << u"foo"_s << true;
    QTest::newRow("foo2rx") << u"foo;foo;"_s << u";"_s << 1 << 1
                            << int(QString::SectionIncludeLeadingSep) << u";foo"_s << true;
    QTest::newRow("foo3rx") << u"foo;foo;"_s << u";"_s << 2 << 2
                            << int(QString::SectionIncludeLeadingSep) << u";"_s << true;

    QTest::newRow("qmake_path") << u"/Users/sam/troll/qt4.0/src/corelib/QtCore_debug.xcode/"_s
                                << u"/"_s << 0 << -2 << int(QString::SectionDefault)
                                << u"/Users/sam/troll/qt4.0/src/corelib/QtCore_debug.xcode"_s
                                << false;
    QTest::newRow("qmake_pathrx") << u"/Users/sam/troll/qt4.0/src/corelib/QtCore_debug.xcode/"_s
                                  << u"/"_s << 0 << -2 << int(QString::SectionDefault)
                                  << u"/Users/sam/troll/qt4.0/src/corelib/QtCore_debug.xcode"_s
                                  << true;
    QTest::newRow("data13") << u"||2|3|||"_s << u"|"_s << 0 << 1
                            << int(QString::SectionIncludeLeadingSep | QString::SectionIncludeTrailingSep)
                            << u"||"_s << false;
    QTest::newRow("data14") << u"||2|3|||"_s << u"\\|"_s << 0 << 1
                            << int(QString::SectionIncludeLeadingSep | QString::SectionIncludeTrailingSep)
                            << u"||"_s << true;
    QTest::newRow("data15") << u"|1|2|"_s << u"|"_s << 0 << 1
                            << int(QString::SectionIncludeLeadingSep | QString::SectionIncludeTrailingSep)
                            << u"|1|"_s << false;
    QTest::newRow("data16") << u"|1|2|"_s << u"\\|"_s << 0 << 1
                            << int(QString::SectionIncludeLeadingSep | QString::SectionIncludeTrailingSep)
                            << u"|1|"_s << true;
    QTest::newRow("normal1") << u"o1o2o"_s
                            << u"o"_s << 0 << 0
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << u"o"_s << false;
    QTest::newRow("normal2") << u"o1o2o"_s
                            << u"o"_s << 1 << 1
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << u"o1o"_s << false;
    QTest::newRow("normal3") << u"o1o2o"_s
                            << u"o"_s << 2 << 2
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << u"o2o"_s << false;
    QTest::newRow("normal4") << u"o1o2o"_s
                            << u"o"_s << 2 << 3
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << u"o2o"_s << false;
    QTest::newRow("normal5") << u"o1o2o"_s
                            << u"o"_s << 1 << 2
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << u"o1o2o"_s << false;
    QTest::newRow("range1") << u"o1o2o"_s
                            << u"o"_s << -5 << -5
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString() << false;
    QTest::newRow("range2") << u"oo1o2o"_s
                            << u"o"_s << -5 << 1
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep
                                   |QString::SectionSkipEmpty)
                            << u"oo1o2o"_s << false;
    QTest::newRow("range3") << u"o1o2o"_s
                            << u"o"_s << 2 << 1
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString() << false;
    QTest::newRow("range4") << u"o1o2o"_s
                            << u"o"_s << 4 << 4
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                            << QString() << false;
    QTest::newRow("range5") << u"o1oo2o"_s
                            << u"o"_s << -2 << -1
                            << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep
                                   |QString::SectionSkipEmpty)
                            << u"o1oo2o"_s << false;
    QTest::newRow("rx1") << u"o1o2o"_s
                        << u"[a-z]"_s << 0 << 0
                        << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                        << u"o"_s << true;
    QTest::newRow("rx2") << u"o1o2o"_s
                        << u"[a-z]"_s << 1 << 1
                        << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                        << u"o1o"_s << true;
    QTest::newRow("rx3") << u"o1o2o"_s
                        << u"[a-z]"_s << 2 << 2
                        << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                        << u"o2o"_s << true;
    QTest::newRow("rx4") << u"o1o2o"_s
                        << u"[a-z]"_s << 2 << 3
                        << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                        << u"o2o"_s << true;
    QTest::newRow("rx5") << u"o1o2o"_s
                        << u"[a-z]"_s << 1 << 2
                        << int(QString::SectionIncludeLeadingSep|QString::SectionIncludeTrailingSep)
                        << u"o1o2o"_s << true;
    QTest::newRow("data17") << u"This is a story, a small story"_s
                        << u"\\b"_s << 3 << 3
                        << int(QString::SectionDefault)
                        << u"is"_s << true;
    QTest::newRow("data18") << u"99.0 42.3"_s
                        << u"\\s*[AaBb]\\s*"_s << 1 << 1
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

    QVERIFY( QString() == u""_s );
    QVERIFY( u""_s == QString() );

    QVERIFY( QString(u""_s) == u""_s );
    QVERIFY( u""_s == QString(u""_s) );

    QVERIFY(QString() == nullptr);
    QVERIFY(nullptr == QString());

    QVERIFY(QString(u""_s) == nullptr);
    QVERIFY(nullptr == QString(u""_s));

    QVERIFY( QString().size() == 0 );

    QVERIFY(u""_s.size() == 0);

    QVERIFY(QString() == u""_s);
    QVERIFY( QString(u""_s) == QString() );
}

void tst_QString::operator_smaller()
{
    QString null;
    QString empty(u""_s);
    QString foo(u"foo"_s);
    [[maybe_unused]]
    const char *nullC = nullptr;
    [[maybe_unused]]
    const char *emptyC = "";

    QVERIFY( !(null < QString()) );
    QVERIFY( !(null > QString()) );

    QVERIFY(!(empty < u""_s));
    QVERIFY(!(empty > u""_s));

    QVERIFY( !(null < empty) );
    QVERIFY( !(null > empty) );

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    QVERIFY( !(nullC < empty) );
    QVERIFY( !(nullC > empty) );

    QVERIFY( !(null < emptyC) );
    QVERIFY( !(null > emptyC) );
#endif

    QVERIFY( null < foo );
    QVERIFY( !(null > foo) );
    QVERIFY( foo > null );
    QVERIFY( !(foo < null) );

    QVERIFY( empty < foo );
    QVERIFY( !(empty > foo) );
    QVERIFY( foo > empty );
    QVERIFY( !(foo < empty) );

    QVERIFY( !(null < QLatin1String(nullptr)) );
    QVERIFY( !(null > QLatin1String(nullptr)) );
    QVERIFY( !(null < QLatin1String("")) );
    QVERIFY( !(null > QLatin1String("")) );

    QVERIFY( !(null < QLatin1String("")) );
    QVERIFY( !(null > QLatin1String("")) );
    QVERIFY( !(empty < QLatin1String("")) );
    QVERIFY( !(empty > QLatin1String("")) );

    QVERIFY( !(QLatin1String(nullptr) < null) );
    QVERIFY( !(QLatin1String(nullptr) > null) );
    QVERIFY( !(QLatin1String("") < null) );
    QVERIFY( !(QLatin1String("") > null) );

    QVERIFY( !(QLatin1String(nullptr) < empty) );
    QVERIFY( !(QLatin1String(nullptr) > empty) );
    QVERIFY( !(QLatin1String("") < empty) );
    QVERIFY( !(QLatin1String("") > empty) );

    QVERIFY( QLatin1String(nullptr) < foo );
    QVERIFY( !(QLatin1String(nullptr) > foo) );
    QVERIFY( QLatin1String("") < foo );
    QVERIFY( !(QLatin1String("") > foo) );

    QVERIFY( foo > QLatin1String(nullptr) );
    QVERIFY( !(foo < QLatin1String(nullptr)) );
    QVERIFY( foo > QLatin1String("") );
    QVERIFY( !(foo < QLatin1String("")) );

    QVERIFY( QLatin1String(nullptr) == empty);
    QVERIFY( QLatin1String(nullptr) == null);
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
    QCOMPARE_LT(foo, QString::fromUtf8("\xc3\xa9"));

#ifndef QT_NO_CAST_FROM_ASCII
    QVERIFY( foo < "\xc3\xa9" );
#endif

    QCOMPARE_LT(QString(u"a"_s), QString(u"b"_s));
    QCOMPARE_LE(QString(u"a"_s), QString(u"b"_s));
    QCOMPARE_LE(QString(u"a"_s), QString(u"a"_s));
    QCOMPARE_EQ(QString(u"a"_s), QString(u"a"_s));
    QCOMPARE_GE(QString(u"a"_s), QString(u"a"_s));
    QCOMPARE_GE(QString(u"b"_s), QString(u"a"_s));
    QCOMPARE_GT(QString(u"b"_s), QString(u"a"_s));

#ifndef QT_NO_CAST_FROM_ASCII
    QVERIFY(QString("a") < "b");
    QVERIFY(QString("a") <= "b");
    QVERIFY(QString("a") <= "a");
    QVERIFY(QString("a") == "a");
    QVERIFY(QString("a") >= "a");
    QVERIFY(QString("b") >= "a");
    QVERIFY(QString("b") > "a");

    QCOMPARE_LT("a", QString(u"b"_s));
    QCOMPARE_LE("a", QString(u"b"_s));
    QCOMPARE_LE("a", QString(u"a"_s));
    QCOMPARE_EQ("a", QString(u"a"_s));
    QCOMPARE_GE("a", QString(u"a"_s));
    QCOMPARE_GE("b", QString(u"a"_s));
    QCOMPARE_GT("b", QString(u"a"_s));
#endif

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    QVERIFY(QString("a") < QByteArray("b"));
    QVERIFY(QString("a") <= QByteArray("b"));
    QVERIFY(QString("a") <= QByteArray("a"));
    QVERIFY(QString("a") == QByteArray("a"));
    QVERIFY(QString("a") >= QByteArray("a"));
    QVERIFY(QString("b") >= QByteArray("a"));
    QVERIFY(QString("b") > QByteArray("a"));
#endif

    QVERIFY(QLatin1String("a") < QString(u"b"_s));
    QVERIFY(QLatin1String("a") <= QString(u"b"_s));
    QVERIFY(QLatin1String("a") <= QString(u"a"_s));
    QVERIFY(QLatin1String("a") == QString(u"a"_s));
    QVERIFY(QLatin1String("a") >= QString(u"a"_s));
    QVERIFY(QLatin1String("b") >= QString(u"a"_s));
    QVERIFY(QLatin1String("b") > QString(u"a"_s));

    QVERIFY(QString(u"a"_s) < QLatin1String("b"));
    QVERIFY(QString(u"a"_s) <= QLatin1String("b"));
    QVERIFY(QString(u"a"_s) <= QLatin1String("a"));
    QVERIFY(QString(u"a"_s) == QLatin1String("a"));
    QVERIFY(QString(u"a"_s) >= QLatin1String("a"));
    QVERIFY(QString(u"b"_s) >= QLatin1String("a"));
    QVERIFY(QString(u"b"_s) > QLatin1String("a"));

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
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
#endif
}

void tst_QString::integer_conversion_data()
{
    QTest::addColumn<QString>("num_str");
    QTest::addColumn<int>("base");
    QTest::addColumn<bool>("good");
    QTest::addColumn<qlonglong>("num");

    QTest::newRow("C empty 0")      << u""_s         << 0  << false << qlonglong(0);
    QTest::newRow("C empty 8")      << u""_s         << 8  << false << qlonglong(0);
    QTest::newRow("C empty 10")     << u""_s         << 10 << false << qlonglong(0);
    QTest::newRow("C empty 16")     << u""_s         << 16 << false << qlonglong(0);

    QTest::newRow("C null 0")       << QString()           << 0  << false << (qlonglong)0;
    QTest::newRow("C null 8")       << QString()           << 8  << false << (qlonglong)0;
    QTest::newRow("C null 10")      << QString()           << 10 << false << (qlonglong)0;
    QTest::newRow("C null 16")      << QString()           << 16 << false << (qlonglong)0;

    QTest::newRow("C   -0xf 0")     << u"  -0xf"_s   << 0  << true  << qlonglong(-15);
    QTest::newRow("C -0xf   0")     << u"-0xf  "_s   << 0  << true  << qlonglong(-15);
    QTest::newRow("C \t0xf\t 0")    << u"\t0xf\t"_s  << 0  << true  << qlonglong(15);
    QTest::newRow("C   -010 0")     << u"  -010"_s   << 0  << true  << qlonglong(-8);
    QTest::newRow("C 010   0")      << u"010  "_s    << 0  << true  << qlonglong(8);
    QTest::newRow("C \t-010\t 0")   << u"\t-010\t"_s << 0  << true  << qlonglong(-8);
    QTest::newRow("C   123 10")     << u"  123"_s    << 10 << true  << qlonglong(123);
    QTest::newRow("C 123   10")     << u"123  "_s    << 10 << true  << qlonglong(123);
    QTest::newRow("C \t123\t 10")   << u"\t123\t"_s  << 10 << true  << qlonglong(123);
    QTest::newRow("C   -0xf 16")    << u"  -0xf"_s   << 16 << true  << qlonglong(-15);
    QTest::newRow("C -0xf   16")    << u"-0xf  "_s   << 16 << true  << qlonglong(-15);
    QTest::newRow("C \t0xf\t 16")   << u"\t0xf\t"_s  << 16 << true  << qlonglong(15);

    QTest::newRow("C -0 0")         << u"-0"_s       << 0   << true << qlonglong(0);
    QTest::newRow("C -0 8")         << u"-0"_s       << 8   << true << qlonglong(0);
    QTest::newRow("C -0 10")        << u"-0"_s       << 10  << true << qlonglong(0);
    QTest::newRow("C -0 16")        << u"-0"_s       << 16  << true << qlonglong(0);

    QTest::newRow("C 1.234 10")     << u"1.234"_s    << 10 << false << qlonglong(0);
    QTest::newRow("C 1,234 10")     << u"1,234"_s    << 10 << false << qlonglong(0);

    QTest::newRow("C 0x 0")         << u"0x"_s       << 0  << false << qlonglong(0);
    QTest::newRow("C 0x 16")        << u"0x"_s       << 16 << false << qlonglong(0);

    QTest::newRow("C 10 0")         << u"10"_s       << 0  << true  << qlonglong(10);
    QTest::newRow("C 010 0")        << u"010"_s      << 0  << true  << qlonglong(8);
    QTest::newRow("C 0x10 0")       << u"0x10"_s     << 0  << true  << qlonglong(16);
    QTest::newRow("C 10 8")         << u"10"_s       << 8  << true  << qlonglong(8);
    QTest::newRow("C 010 8")        << u"010"_s      << 8  << true  << qlonglong(8);
    QTest::newRow("C 0x10 8")       << u"0x10"_s     << 8  << false << qlonglong(0);
    QTest::newRow("C 10 10")        << u"10"_s       << 10 << true  << qlonglong(10);
    QTest::newRow("C 010 10")       << u"010"_s      << 10 << true  << qlonglong(10);
    QTest::newRow("C 0x10 10")      << u"0x10"_s     << 10 << false << qlonglong(0);
    QTest::newRow("C 10 16")        << u"10"_s       << 16 << true  << qlonglong(16);
    QTest::newRow("C 010 16")       << u"010"_s      << 16 << true  << qlonglong(16);
    QTest::newRow("C 0x10 16")      << u"0x10"_s     << 16 << true  << qlonglong(16);

    QTest::newRow("C -10 0")        << u"-10"_s      << 0  << true  << qlonglong(-10);
    QTest::newRow("C -010 0")       << u"-010"_s     << 0  << true  << qlonglong(-8);
    QTest::newRow("C -0x10 0")      << u"-0x10"_s    << 0  << true  << qlonglong(-16);
    QTest::newRow("C -10 8")        << u"-10"_s      << 8  << true  << qlonglong(-8);
    QTest::newRow("C -010 8")       << u"-010"_s     << 8  << true  << qlonglong(-8);
    QTest::newRow("C -0x10 8")      << u"-0x10"_s    << 8  << false << qlonglong(0);
    QTest::newRow("C -10 10")       << u"-10"_s      << 10 << true  << qlonglong(-10);
    QTest::newRow("C -010 10")      << u"-010"_s     << 10 << true  << qlonglong(-10);
    QTest::newRow("C -0x10 10")     << u"-0x10"_s    << 10 << false << qlonglong(0);
    QTest::newRow("C -10 16")       << u"-10"_s      << 16 << true  << qlonglong(-16);
    QTest::newRow("C -010 16")      << u"-010"_s     << 16 << true  << qlonglong(-16);
    QTest::newRow("C -0x10 16")     << u"-0x10"_s    << 16 << true  << qlonglong(-16);

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

    QTest::newRow("C 1")             << u"1"_s          << true  << 1.0;
    QTest::newRow("C 1.0")           << u"1.0"_s        << true  << 1.0;
    QTest::newRow("C 1.234")         << u"1.234"_s      << true  << 1.234;
    QTest::newRow("C 1.234e-10")     << u"1.234e-10"_s  << true  << 1.234e-10;
    QTest::newRow("C 1.234E10")      << u"1.234E10"_s   << true  << 1.234e10;
    QTest::newRow("C 1e10")          << u"1e10"_s       << true  << 1.0e10;

    // The bad...

    QTest::newRow("C null")          << QString()       << false << 0.0;
    QTest::newRow("C empty")         << u""_s           << false << 0.0;
    QTest::newRow("C space")         << u" "_s          << false << 0.0;
    QTest::newRow("C spaces")        << u"  "_s         << false << 0.0;
    QTest::newRow("C .")             << u"."_s          << false << 0.0;
    QTest::newRow("C 1e")            << u"1e"_s         << false << 0.0;
    QTest::newRow("C 1,")            << u"1,"_s         << false << 0.0;
    QTest::newRow("C 1,0")           << u"1,0"_s        << false << 0.0;
    QTest::newRow("C 1,000")         << u"1,000"_s      << false << 0.0;
    QTest::newRow("C 1e1.0")         << u"1e1.0"_s      << false << 0.0;
    QTest::newRow("C 1e+")           << u"1e+"_s        << false << 0.0;
    QTest::newRow("C 1e-")           << u"1e-"_s        << false << 0.0;
    QTest::newRow("de_DE 1,0")       << u"1,0"_s        << false << 0.0;
    QTest::newRow("de_DE 1,234")     << u"1,234"_s      << false << 0.0;
    QTest::newRow("de_DE 1,234e-10") << u"1,234e-10"_s  << false << 0.0;
    QTest::newRow("de_DE 1,234E10")  << u"1,234E10"_s   << false << 0.0;

    // And the ugly...

    QTest::newRow("C .1")            << u".1"_s         << true  << 0.1;
    QTest::newRow("C -.1")           << u"-.1"_s        << true  << -0.1;
    QTest::newRow("C 1.")            << u"1."_s         << true  << 1.0;
    QTest::newRow("C 1.E10")         << u"1.E10"_s      << true  << 1.0e10;
    QTest::newRow("C 1e+10")         << u"1e+10"_s      << true  << 1.0e+10;
    QTest::newRow("C   1")           << u"  1"_s        << true  << 1.0;
    QTest::newRow("C 1  ")           << u"1  "_s        << true  << 1.0;

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
        QCOMPARE(s, QLatin1String(data->expected));
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

    QString s = u"0123456789"_s;

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

    QString s(u"1234"_s);
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

    QTest::newRow("1") << u"a,b,c"_s << u","_s << QStringList{u"a"_s, u"b"_s, u"c"_s};
    QTest::newRow("2") << u"-rw-r--r--  1 0  0  519240 Jul  9  2002 bigfile"_s << u" "_s
                       << QStringList{ u"-rw-r--r--"_s, u""_s, u"1"_s,      u"0"_s,      u""_s,
                                       u"0"_s,          u""_s, u"519240"_s, u"Jul"_s,    u""_s,
                                       u"9"_s,          u""_s, u"2002"_s,   u"bigfile"_s };
    QTest::newRow("one-empty") << u""_s << u" "_s << QStringList{u""_s};
    QTest::newRow("two-empty") << u" "_s << u" "_s << QStringList{u""_s, u""_s};
    QTest::newRow("three-empty") << u"  "_s << u" "_s << QStringList{u""_s, u""_s, u""_s};

    QTest::newRow("all-empty") << u""_s << u""_s << QStringList{u""_s, u""_s};
    QTest::newRow("sep-empty") << u"abc"_s << u""_s << QStringList{u""_s, u"a"_s, u"b"_s, u"c"_s, u""_s};
    QTest::newRow("null-empty") << QString() << u" "_s << QStringList{u""_s};
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

    result.removeAll(u""_s);
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
                            << QStringList{u"Some"_s, u"text"_s, u"with"_s, u"strange"_s, u"whitespace."_s };

    QTest::newRow("data02") << "This time, a normal English sentence."
                            << "\\W+"
                            << QStringList{u"This"_s, u"time"_s, u"a"_s, u"normal"_s, u"English"_s, u"sentence"_s, u""_s};

    QTest::newRow("data03") << "Now: this sentence fragment."
                            << "\\b"
                            << QStringList{u""_s, u"Now"_s, u": "_s, u"this"_s, u" "_s, u"sentence"_s, u" "_s, u"fragment"_s, u"."_s};
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

// Test that rvalue strings (e.g. temporaries) are kept alive in
// QRegularExpression-related APIs
void tst_QString::regularexpression_lifetime()
{
    const auto getString = [] {
        // deliberately heap-allocated
        return QString(QLatin1String("the quick brown fox jumps over the lazy dog"));
    };

    QRegularExpression re(u"\\w{5}"_s);

    {
        QString s = getString();
        QRegularExpressionMatch match;
        const bool contains = std::move(s).contains(re, &match);
        s.fill(u'X'); // NOLINT(bugprone-use-after-move)
        QVERIFY(contains);
        QCOMPARE(match.capturedView(), u"quick");
    }

    {
        QString s = getString();
        QRegularExpressionMatch match;
        const auto index = std::move(s).indexOf(re, 0, &match);
        s.fill(u'X'); // NOLINT(bugprone-use-after-move)
        QCOMPARE(index, 4);
        QCOMPARE(match.capturedView(), u"quick");
    }

    {
        QString s = getString();
        QRegularExpressionMatch match;
        const auto lastIndex = std::move(s).lastIndexOf(re, &match);
        s.fill(u'X'); // NOLINT(bugprone-use-after-move)
        QCOMPARE(lastIndex, 20);
        QCOMPARE(match.capturedView(), u"jumps");
    }
}
#endif

void tst_QString::fromUtf16_data()
{
    QTest::addColumn<QString>("ucs2");
    QTest::addColumn<QString>("res");
    QTest::addColumn<int>("len");

    QTest::newRow("str0") << u"abcdefgh"_s << u"abcdefgh"_s << -1;
    QTest::newRow("str0-len") << u"abcdefgh"_s << u"abc"_s << 3;
}

#if QT_DEPRECATED_SINCE(6, 0)
void tst_QString::fromUtf16()
{
    QFETCH(QString, ucs2);
    QFETCH(QString, res);
    QFETCH(int, len);
    QT_IGNORE_DEPRECATIONS(QCOMPARE(QString::fromUtf16(ucs2.utf16(), len), res);)
}
#endif // QT_DEPRECATED_SINCE(6, 0)

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

    QString emptyStr(u""_s);
    QVERIFY(emptyStr.toStdU16String().empty());
    QVERIFY(emptyStr.toStdU32String().empty());
    QVERIFY(!emptyStr.isDetached());

    QString s1, s2;
    static const std::u16string u16str1(u"Hello Unicode World");
    static const std::u32string u32str1(U"Hello Unicode World");
    s1 = QString::fromStdU16String(u16str1);
    s2 = QString::fromStdU32String(u32str1);
    QCOMPARE(s1, u"Hello Unicode World");
    QCOMPARE(s1, s2);

    QCOMPARE(s2.toStdU16String(), u16str1);
    QCOMPARE(s1.toStdU32String(), u32str1);

    s1 = QString::fromStdU32String(std::u32string(U"\u221212\U000020AC\U00010000"));
    QCOMPARE(s1, QString::fromUtf8("\342\210\222" "12" "\342\202\254" "\360\220\200\200"));
}

void tst_QString::latin1String()
{
    QString s(u"Hello"_s);

    QVERIFY(s == QLatin1String("Hello"));
    QVERIFY(s != QLatin1String("Hello World"));
    QVERIFY(s < QLatin1String("Helloa"));
    QVERIFY(!(s > QLatin1String("Helloa")));
    QVERIFY(s > QLatin1String("Helln"));
    QVERIFY(s > QLatin1String("Hell"));
    QVERIFY(!(s < QLatin1String("Helln")));
    QVERIFY(!(s < QLatin1String("Hell")));
}

void tst_QString::isInf_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<bool>("expected_ok");
    QTest::addColumn<bool>("expected_is_inf");

    QTest::newRow("inf") << u"inf"_s << true << true;
    QTest::newRow("INF") << u"INF"_s << true << true;
    QTest::newRow("inf  ") << u"inf  "_s << true << true;
    QTest::newRow("+inf") << u"+inf"_s << true << true;
    QTest::newRow("\t +INF") << u"\t +INF"_s << true << true;
    QTest::newRow("\t INF") << u"\t INF"_s << true << true;
    QTest::newRow("inF  ") << u"inF  "_s << true << true;
    QTest::newRow("+iNf") << u"+iNf"_s << true << true;
    QTest::newRow("INFe-10") << u"INFe-10"_s << false << false;
    QTest::newRow("0xINF") << u"0xINF"_s << false << false;
    QTest::newRow("- INF") << u"- INF"_s << false << false;
    QTest::newRow("+ INF") << u"+ INF"_s << false << false;
    QTest::newRow("-- INF") << u"-- INF"_s << false << false;
    QTest::newRow("inf0") << u"inf0"_s << false << false;
    QTest::newRow("--INF") << u"--INF"_s << false << false;
    QTest::newRow("++INF") << u"++INF"_s << false << false;
    QTest::newRow("INF++") << u"INF++"_s << false << false;
    QTest::newRow("INF--") << u"INF--"_s << false << false;
    QTest::newRow("INF +") << u"INF +"_s << false << false;
    QTest::newRow("INF -") << u"INF -"_s << false << false;
    QTest::newRow("0INF") << u"0INF"_s << false << false;
}

void tst_QString::isInf()
{
    QFETCH(QString, str);
    QFETCH(bool, expected_ok);
    QFETCH(bool, expected_is_inf);

    bool ok = false;
    double dbl = str.toDouble(&ok);
    QVERIFY(ok == expected_ok);
    QVERIFY(qIsInf(dbl) == expected_is_inf);
}

void tst_QString::isNan_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<bool>("expected_ok");
    QTest::addColumn<bool>("expected_is_nan");

    QTest::newRow("nan") << u"nan"_s << true << true;
    QTest::newRow("NAN") << u"NAN"_s << true << true;
    QTest::newRow("nan  ") << u"nan  "_s << true << true;
    QTest::newRow("\t NAN") << u"\t NAN"_s << true << true;
    QTest::newRow("\t NAN  ") << u"\t NAN  "_s << true << true;
    QTest::newRow("-nan") << u"-nan"_s << false << false;
    QTest::newRow("+NAN") << u"+NAN"_s << false << false;
    QTest::newRow("NaN") << u"NaN"_s << true << true;
    QTest::newRow("nAn") << u"nAn"_s << true << true;
    QTest::newRow("NANe-10") << u"NANe-10"_s << false << false;
    QTest::newRow("0xNAN") << u"0xNAN"_s << false << false;
    QTest::newRow("0NAN") << u"0NAN"_s << false << false;
}

void tst_QString::isNan()
{
    QFETCH(QString, str);
    QFETCH(bool, expected_ok);
    QFETCH(bool, expected_is_nan);

    bool ok = false;
    double dbl = str.toDouble(&ok);
    QVERIFY(ok == expected_ok);
    QVERIFY(qIsNaN(dbl) == expected_is_nan);
}

void tst_QString::nanAndInf()
{
    bool ok;
    double d;

    d = u"-INF"_s.toDouble(&ok);
    QVERIFY(ok);
    QVERIFY(d == -qInf());

    u"INF"_s.toLong(&ok);
    QVERIFY(!ok);

    u"INF"_s.toLong(&ok, 36);
    QVERIFY(ok);

    u"INF0"_s.toLong(&ok, 36);
    QVERIFY(ok);

    u"0INF0"_s.toLong(&ok, 36);
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
    QCOMPARE(form.arg(qInf(), 5, 'f', 3, u'0'), u"00inf");
    QCOMPARE(form.arg(qInf(), -5, 'f', 3, u'0'), u"inf00");
    QCOMPARE(form.arg(-qInf(), 6, 'f', 3, u'0'), u"00-inf");
    QCOMPARE(form.arg(-qInf(), -6, 'f', 3, u'0'), u"-inf00");
    QCOMPARE(form.arg(qQNaN(), -5, 'f', 3, u'0'), u"nan00");
    QCOMPARE(form.arg(qInf(), 5, 'F', 3, u'0'), u"00INF");
    QCOMPARE(form.arg(qInf(), -5, 'F', 3, u'0'), u"INF00");
    QCOMPARE(form.arg(-qInf(), 6, 'F', 3, u'0'), u"00-INF");
    QCOMPARE(form.arg(-qInf(), -6, 'F', 3, u'0'), u"-INF00");
    QCOMPARE(form.arg(qQNaN(), -5, 'F', 3, u'0'), u"NAN00");
    QCOMPARE(form.arg(qInf(), 5, 'e', 3, u'0'), u"00inf");
    QCOMPARE(form.arg(qInf(), -5, 'e', 3, u'0'), u"inf00");
    QCOMPARE(form.arg(-qInf(), 6, 'e', 3, u'0'), u"00-inf");
    QCOMPARE(form.arg(-qInf(), -6, 'e', 3, u'0'), u"-inf00");
    QCOMPARE(form.arg(qQNaN(), -5, 'e', 3, u'0'), u"nan00");
    QCOMPARE(form.arg(qInf(), 5, 'E', 3, u'0'), u"00INF");
    QCOMPARE(form.arg(qInf(), -5, 'E', 3, u'0'), u"INF00");
    QCOMPARE(form.arg(-qInf(), 6, 'E', 3, u'0'), u"00-INF");
    QCOMPARE(form.arg(-qInf(), -6, 'E', 3, u'0'), u"-INF00");
    QCOMPARE(form.arg(qQNaN(), -5, 'E', 3, u'0'), u"NAN00");
    QCOMPARE(form.arg(qInf(), 5, 'g', 3, u'0'), u"00inf");
    QCOMPARE(form.arg(qInf(), -5, 'g', 3, u'0'), u"inf00");
    QCOMPARE(form.arg(-qInf(), 6, 'g', 3, u'0'), u"00-inf");
    QCOMPARE(form.arg(-qInf(), -6, 'g', 3, u'0'), u"-inf00");
    QCOMPARE(form.arg(qQNaN(), -5, 'g', 3, u'0'), u"nan00");
    QCOMPARE(form.arg(qInf(), 5, 'G', 3, u'0'), u"00INF");
    QCOMPARE(form.arg(qInf(), -5, 'G', 3, u'0'), u"INF00");
    QCOMPARE(form.arg(-qInf(), 6, 'G', 3, u'0'), u"00-INF");
    QCOMPARE(form.arg(-qInf(), -6, 'G', 3, u'0'), u"-INF00");
    QCOMPARE(form.arg(qQNaN(), -5, 'G', 3, u'0'), u"NAN00");
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
        << DataList{QVariant(int(5)), QVariant(u"f"_s), QVariant(int(0))}
        << IntList{3, 2, 5} << u"abc"_s << u"aa5bfcccc0"_s;

    QTest::newRow("str1")
        << QStringLiteral("%3.%1.%3.%2")
        << DataList{QVariant(int(5)), QVariant(u"foo"_s), QVariant(qulonglong(INT_MAX))}
        << IntList{10, 2, 5} << u"0 c"_s << u"2147483647.0000000005.2147483647.foo"_s;

    QTest::newRow("str2")
        << QStringLiteral("%9 og poteter")
        << DataList{QVariant(u"fisk"_s)} << IntList{100} << u"f"_s
        << u"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
           "fffffffffffffffffffffffffffffffffffffisk og poteter"_s;

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
    QCOMPARE(replaceValues.size(), fillChars.size());
    QCOMPARE(replaceValues.size(), widths.size());

    QString actual = pattern;
    for (int i=0; i<replaceValues.size(); ++i) {
        const QVariant &var = replaceValues.at(i);
        const int width = widths.at(i);
        const QChar fillChar = fillChars.at(i);
        switch (var.typeId()) {
        case QMetaType::QString:
            actual = actual.arg(var.toString(), width, fillChar);
            break;
        case QMetaType::Int:
            actual = actual.arg(var.toInt(), width, base, fillChar);
            break;
        case QMetaType::UInt:
            actual = actual.arg(var.toUInt(), width, base, fillChar);
            break;
        case QMetaType::Double:
            actual = actual.arg(var.toDouble(), width, fmt, prec, fillChar);
            break;
        case QMetaType::LongLong:
            actual = actual.arg(var.toLongLong(), width, base, fillChar);
            break;
        case QMetaType::ULongLong:
            actual = actual.arg(var.toULongLong(), width, base, fillChar);
            break;
        default:
            QVERIFY(0);
            break;
        }
    }

    QCOMPARE(actual, expected);
}

void tst_QString::comparisonCompiles()
{
    QTestPrivate::testAllComparisonOperatorsCompile<QString>();
    QTestPrivate::testAllComparisonOperatorsCompile<QString, std::nullptr_t>();
    QTestPrivate::testAllComparisonOperatorsCompile<QString, QChar>();
    QTestPrivate::testAllComparisonOperatorsCompile<QString, QLatin1StringView>();
    QTestPrivate::testAllComparisonOperatorsCompile<QString, const char16_t *>();
    QTestPrivate::testAllComparisonOperatorsCompile<QString, QStringView>();
    QTestPrivate::testAllComparisonOperatorsCompile<QString, QUtf8StringView>();
#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    QTestPrivate::testAllComparisonOperatorsCompile<QString, QByteArrayView>();
    QTestPrivate::testAllComparisonOperatorsCompile<QString, QByteArray>();
    QTestPrivate::testAllComparisonOperatorsCompile<QString, const char *>();
#endif
#ifdef __cpp_char8_t
    QTestPrivate::testAllComparisonOperatorsCompile<QString, const char8_t *>();
#endif
}

void tst_QString::compare_data()
{
    QTest::addColumn<QString>("s1");
    QTest::addColumn<QString>("s2");
    QTest::addColumn<int>("csr"); // case sensitive result
    QTest::addColumn<int>("cir"); // case insensitive result

    // null strings
    QTest::newRow("null-null") << QString() << QString() << 0 << 0;
    QTest::newRow("text-null") << u"a"_s << QString() << 1 << 1;
    QTest::newRow("null-text") << QString() << u"a"_s << -1 << -1;
    QTest::newRow("null-empty") << QString() << u""_s << 0 << 0;
    QTest::newRow("empty-null") << u""_s << QString() << 0 << 0;

    // empty strings
    QTest::newRow("data0") << u""_s << u""_s << 0 << 0;
    QTest::newRow("data1") << u"a"_s << u""_s << 1 << 1;
    QTest::newRow("data2") << u""_s << u"a"_s << -1 << -1;

    // equal length
    QTest::newRow("data3") << u"abc"_s << u"abc"_s << 0 << 0;
    QTest::newRow("data4") << u"abC"_s << u"abc"_s << -1 << 0;
    QTest::newRow("data5") << u"abc"_s << u"abC"_s << 1 << 0;

    // different length
    QTest::newRow("data6") << u"abcdef"_s << u"abc"_s << 1 << 1;
    QTest::newRow("data7") << u"abCdef"_s << u"abc"_s << -1 << 1;
    QTest::newRow("data8") << u"abc"_s << u"abcdef"_s << -1 << -1;

    QString upper;
    upper += QChar(QChar::highSurrogate(0x10400));
    upper += QChar(QChar::lowSurrogate(0x10400));
    QString lower;
    lower += QChar(QChar::highSurrogate(0x10428));
    lower += QChar(QChar::lowSurrogate(0x10428));
    QTest::newRow("data9") << upper << lower << -1 << 0;

    // embedded nulls
    QLatin1String onenull("", 1);
    QTest::newRow("data10") << QString(onenull) << QString(onenull) << 0 << 0;
    QTest::newRow("data11") << QString(onenull) << u""_s << 1 << 1;
    QTest::newRow("data12") << u""_s << QString(onenull) << -1 << -1;
    QTest::newRow("data13") << QString::fromLatin1("ab\0c", 4) << QString(QLatin1String("ab\0c", 4)) << 0 << 0;
    QTest::newRow("data14") << QString(QLatin1String("ab\0c", 4)) << u"abc"_s << -1 << -1;
    QTest::newRow("data15") << u"abc"_s << QString(QLatin1String("ab\0c", 4)) << 1 << 1;
    QTest::newRow("data16") << u"abc"_s << QString(QLatin1String("abc", 4)) << -1 << -1;

    // All tests below (generated by the 3 for-loops) are meant to exercise the vectorized versions
    // of ucstrncmp.

    QString in1, in2;
    for (int i = 0; i < 70; ++i) {
        in1 += QString::number(i % 10);
        in2 += QString::number((70 - i + 1) % 10);
    }
    Q_ASSERT(in1.size() == in2.size());
    Q_ASSERT(in1 != in2);
    Q_ASSERT(in1.at(0) < in2.at(0));
    for (int i = 0; i < in1.size(); ++i) {
        Q_ASSERT(in1.at(i) != in2.at(i));
    }

    for (int i = 1; i <= 65; ++i) {
        QString inp1 = in1.left(i);
        QString inp2 = in2.left(i);
        QTest::addRow("all-different-%d", i) << inp1 << inp2 << -1 << -1;
    }

    for (int i = 1; i <= 65; ++i) {
        QString start(i - 1, u'a');

        QString in = start + QLatin1Char('a');
        QTest::addRow("all-same-%d", i) << in << in << 0 << 0;

        QString in2 = start + QLatin1Char('b');
        QTest::addRow("last-different-%d", i) << in << in2 << -1 << -1;
    }

    for (int i = 0; i < 16; ++i) {
        QString in1(16, u'a');
        QString in2 = in1;
        in2[i] = u'b';
        QTest::addRow("all-same-except-char-%d", i) << in1 << in2 << -1 << -1;
    }

    // some non-US-ASCII comparisons
    QChar smallA = u'a';
    QChar smallAWithAcute = u'á';
    QChar capitalAWithAcute = u'Á';
    QChar nbsp = u'\u00a0';
    for (int i = 1; i <= 65; ++i) {
        QString padding(i - 1, u' ');
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
    for (int i = 0; i < s.size(); ++i)
        if (s.at(i).unicode() > 0xff)
            return false;
    return true;
}

static inline int sign(int x)
{
    return x == 0 ? 0 : (x < 0 ? -1 : 1);
}

void tst_QString::compare()
{
    QFETCH(QString, s1);
    QFETCH(QString, s2);
    QFETCH(int, csr);
    QFETCH(int, cir);

    QByteArray s1_8 = s1.toUtf8();
    QByteArray s2_8 = s2.toUtf8();
    QByteArray s1_1 = s1.toLatin1();
    QByteArray s2_1 = s2.toLatin1();
    QLatin1String l1s1(s1_1);
    QLatin1String l1s2(s2_1);

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

    if (csr == 0) {
        QVERIFY(qHash(s1) == qHash(s2));
        QVERIFY(s1 == s2);
        QVERIFY(!(s1 != s2));
    } else {
        QVERIFY(s1 != s2);
        QVERIFY(!(s1 == s2));
    }

    if (!cir)
        QCOMPARE(s1.toCaseFolded(), s2.toCaseFolded());

    if (isLatin(s2)) {
        QVERIFY(QtPrivate::isLatin1(s2));
        QCOMPARE(sign(QString::compare(s1, l1s2)), csr);
        QCOMPARE(sign(QString::compare(s1, l1s2, Qt::CaseInsensitive)), cir);

        // ensure it doesn't compare past the explicit size
        QByteArray l1 = s2.toLatin1();
        l1 += "x";
        QLatin1String l1str(l1.constData(), l1.size() - 1);
        QCOMPARE(sign(QString::compare(s1, l1str)), csr);
        QCOMPARE(sign(QString::compare(s1, l1str, Qt::CaseInsensitive)), cir);
    }

    if (isLatin(s1)) {
        QVERIFY(QtPrivate::isLatin1(s1));
        QCOMPARE(sign(QString::compare(l1s1, s2)), csr);
        QCOMPARE(sign(QString::compare(l1s1, s2, Qt::CaseInsensitive)), cir);
    }

    if (isLatin(s1) && isLatin(s2)) {
        QCOMPARE(sign(QtPrivate::compareStrings(l1s1, l1s2)), csr);
        QCOMPARE(sign(QtPrivate::compareStrings(l1s1, l1s2, Qt::CaseInsensitive)), cir);
        QCOMPARE(l1s1 == l1s2, csr == 0);
        QCOMPARE(l1s1 < l1s2, csr < 0);
        QCOMPARE(l1s1 > l1s2, csr > 0);
    }
}

void tst_QString::comparisonMacros_data()
{
    compare_data();
}

void tst_QString::comparisonMacros()
{
    QFETCH(const QString, s1);
    QFETCH(const QString, s2);
    QFETCH(int, csr);

    const Qt::strong_ordering expectedOrdering = [&csr] {
        if (csr > 0)
            return Qt::strong_ordering::greater;
        else if (csr < 0)
            return Qt::strong_ordering::less;
        return Qt::strong_ordering::equal;
    }();

    QT_TEST_ALL_COMPARISON_OPS(s1, s2, expectedOrdering);

    const QStringView s2sv(s2);
    QT_TEST_ALL_COMPARISON_OPS(s1, s2sv, expectedOrdering);

    if (!s2.contains(QChar(u'\0'))) {
        const char16_t *utfData = reinterpret_cast<const char16_t*>(s2.constData());
        QT_TEST_ALL_COMPARISON_OPS(s1, utfData, expectedOrdering);
    }

    if (s2.size() == 1) {
        const QChar ch = s2.front();
        QT_TEST_ALL_COMPARISON_OPS(s1, ch, expectedOrdering);
    }

    if (isLatin(s2)) {
        QByteArray ba = s2.toLatin1();
        const QLatin1StringView l1s2{ba};
        QT_TEST_ALL_COMPARISON_OPS(s1, l1s2, expectedOrdering);
    }

    const QByteArray u8s2 = s2.toUtf8();

    const QUtf8StringView u8s2sv(u8s2.data(), u8s2.size());
    QT_TEST_ALL_COMPARISON_OPS(s1, u8s2sv, expectedOrdering);

#ifdef __cpp_char8_t
    if (!s2.contains(QChar(u'\0'))) {
        const char8_t *char8data = reinterpret_cast<const char8_t*>(u8s2.constData());
        QT_TEST_ALL_COMPARISON_OPS(s1, char8data, expectedOrdering);
    }
#endif // __cpp_char8_t

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
    QT_TEST_ALL_COMPARISON_OPS(s1, u8s2, expectedOrdering);
    const QByteArrayView u8s2view{u8s2.begin(), u8s2.size()};
    QT_TEST_ALL_COMPARISON_OPS(s1, u8s2view, expectedOrdering);
    if (!s2.contains(QChar(u'\0'))) {
        const char *u8data = u8s2.constData();
        QT_TEST_ALL_COMPARISON_OPS(s1, u8data, expectedOrdering);
    }
#endif // !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
}

void tst_QString::resize()
{
    QString s;
    s.resize(11, u' ');
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
    QString buffer(u"hello world"_s);

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

    s += u"hello world"_s;

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
    s += u"hello world"_s;
    s.resize(0);
    QVERIFY(s.capacity() == 100);

    // reserve() can't be used to truncate data
    s.fill(u'x', 100);
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
    QString str(u"*%L1*%L2*"_s);

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

    QVERIFY(str.size() == 4);
    QCOMPARE(str.capacity(), 0);
    QVERIFY(str == QLatin1String("abcd"));
    QVERIFY(!str.data_ptr()->isMutable());

    const QChar *s = str.constData();
    QString str2 = str;
    QVERIFY(str2.constData() == s);
    QCOMPARE(str2.capacity(), 0);

    // detach on non const access
    QVERIFY(str.data() != s);
    QVERIFY(str.capacity() >= str.size());

    QVERIFY(str2.constData() == s);
    QVERIFY(str2.data() != s);
    QVERIFY(str2.capacity() >= str2.size());
}

void tst_QString::userDefinedLiterals()
{
    {
        using namespace Qt::StringLiterals;
        QString str = u"abcd"_s;

        QVERIFY(str.size() == 4);
        QCOMPARE(str.capacity(), 0);
        QVERIFY(str == QLatin1String("abcd"));
        QVERIFY(!str.data_ptr()->isMutable());

        const QChar *s = str.constData();
        QString str2 = str;
        QVERIFY(str2.constData() == s);
        QCOMPARE(str2.capacity(), 0);

        // detach on non const access
        QVERIFY(str.data() != s);
        QVERIFY(str.capacity() >= str.size());

        QVERIFY(str2.constData() == s);
        QVERIFY(str2.data() != s);
        QVERIFY(str2.capacity() >= str2.size());
    }

#if QT_DEPRECATED_SINCE(6, 8)
    {
        QT_IGNORE_DEPRECATIONS(QString str = u"abcd"_qs;)

        QVERIFY(str.size() == 4);
        QCOMPARE(str.capacity(), 0);
        QVERIFY(str == QLatin1String("abcd"));
        QVERIFY(!str.data_ptr()->isMutable());

        const QChar *s = str.constData();
        QString str2 = str;
        QVERIFY(str2.constData() == s);
        QCOMPARE(str2.capacity(), 0);

        // detach on non const access
        QVERIFY(str.data() != s);
        QVERIFY(str.capacity() >= str.size());

        QVERIFY(str2.constData() == s);
        QVERIFY(str2.data() != s);
        QVERIFY(str2.capacity() >= str2.size());
    }
#endif // QT_DEPRECATED_SINCE(6, 8)
}

#if !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)
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
#endif // !defined(QT_RESTRICTED_CAST_FROM_ASCII) && !defined(QT_NO_CAST_FROM_ASCII)

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
    QTest::newRow("empty") << u""_s << u""_s;
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

    QString null = QLatin1String(nullptr);
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

    // assign to non-null QString with enough capacity:
    s.clear();
    s.squeeze();
    s.reserve(3);
    s = QLatin1String("foo");
    const int capacity = s.capacity();
    QCOMPARE(s.capacity(), 3);
    s = sp;
    QCOMPARE(s, QString(sp));
    QCOMPARE(s.capacity(), capacity);

    // assign to shared QString (enough capacity, but can't use):
    s = QLatin1String("foo");
    QString s2 = s;
    s = sp;
    QCOMPARE(s, QString(sp));

    // assign to empty QString:
    s = QString(u""_s);
    s.detach();
    QCOMPARE(s.capacity(), 0);
    s = sp;
    QCOMPARE(s, QString(sp));
}

void tst_QString::isRightToLeft_data()
{
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<bool>("rtl");

    QTest::newRow("null") << QString() << false;
    QTest::newRow("empty") << u""_s << false;

    QTest::newRow("numbers-only") << u"12345"_s << false;
    QTest::newRow("latin1-only") << u"hello"_s << false;
    QTest::newRow("numbers-latin1") << (u"12345"_s + u"hello"_s) << false;

    static const char16_t unicode1[] = { 0x627, 0x627 };
    QTest::newRow("arabic-only") << QString::fromUtf16(unicode1, 2) << true;
    QTest::newRow("numbers-arabic") << (u"12345"_s + QString::fromUtf16(unicode1, 2)) << true;
    QTest::newRow("numbers-latin1-arabic") << (u"12345"_s + u"hello"_s + QString::fromUtf16(unicode1, 2)) << false;
    QTest::newRow("numbers-arabic-latin1") << (u"12345"_s + QString::fromUtf16(unicode1, 2) + u"hello"_s) << true;

    static const char16_t unicode2[] = { QChar::highSurrogate(0xE01DAu), QChar::lowSurrogate(0xE01DAu), QChar::highSurrogate(0x2F800u), QChar::lowSurrogate(0x2F800u) };
    QTest::newRow("surrogates-VS-CJK") << QString::fromUtf16(unicode2, 4) << false;

    static const char16_t unicode3[] = { QChar::highSurrogate(0x10800u), QChar::lowSurrogate(0x10800u), QChar::highSurrogate(0x10805u), QChar::lowSurrogate(0x10805u) };
    QTest::newRow("surrogates-cypriot") << QString::fromUtf16(unicode3, 4) << true;

    QTest::newRow("lre") << (u"12345"_s + QChar(0x202a) + u"9"_s + QChar(0x202c)) << false;
    QTest::newRow("rle") << (u"12345"_s + QChar(0x202b) + u"9"_s + QChar(0x202c)) << false;
    QTest::newRow("r in lre") << (u"12345"_s + QChar(0x202a) + QString::fromUtf16(unicode1, 2) + QChar(0x202c) + u"a"_s) << true;
    QTest::newRow("l in lre") << (u"12345"_s + QChar(0x202a) + u"a"_s + QChar(0x202c) + QString::fromUtf16(unicode1, 2)) << false;
    QTest::newRow("r in rle") << (u"12345"_s + QChar(0x202b) + QString::fromUtf16(unicode1, 2) + QChar(0x202c) + u"a"_s) << true;
    QTest::newRow("l in rle") << (u"12345"_s + QChar(0x202b) + u"a"_s + QChar(0x202c) + QString::fromUtf16(unicode1, 2)) << false;

    QTest::newRow("lro") << (u"12345"_s + QChar(0x202d) + u"9"_s + QChar(0x202c)) << false;
    QTest::newRow("rlo") << (u"12345"_s + QChar(0x202e) + u"9"_s + QChar(0x202c)) << false;
    QTest::newRow("r in lro") << (u"12345"_s + QChar(0x202d) + QString::fromUtf16(unicode1, 2) + QChar(0x202c) + u"a"_s) << true;
    QTest::newRow("l in lro") << (u"12345"_s + QChar(0x202d) + u"a"_s + QChar(0x202c) + QString::fromUtf16(unicode1, 2)) << false;
    QTest::newRow("r in rlo") << (u"12345"_s + QChar(0x202e) + QString::fromUtf16(unicode1, 2) + QChar(0x202c) + u"a"_s) << true;
    QTest::newRow("l in rlo") << (u"12345"_s + QChar(0x202e) + u"a"_s + QChar(0x202c) + QString::fromUtf16(unicode1, 2)) << false;

    QTest::newRow("lri") << (u"12345"_s + QChar(0x2066) + u"a"_s + QChar(0x2069) + QString::fromUtf16(unicode1, 2)) << true;
    QTest::newRow("rli") << (u"12345"_s + QChar(0x2067) + QString::fromUtf16(unicode1, 2) + QChar(0x2069) + u"a"_s) << false;
    QTest::newRow("fsi1") << (u"12345"_s + QChar(0x2068) + u"a"_s + QChar(0x2069) + QString::fromUtf16(unicode1, 2)) << true;
    QTest::newRow("fsi2") << (u"12345"_s + QChar(0x2068) + QString::fromUtf16(unicode1, 2) + QChar(0x2069) + u"a"_s) << false;
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
    QTest::addRow("valid-%02d", row++) << u""_s << true;
    QTest::addRow("valid-%02d", row++) << u"abc def"_s << true;
    QTest::addRow("valid-%02d", row++) << u"àbç"_s << true;
    QTest::addRow("valid-%02d", row++) << u"ßẞ"_s << true;
    QTest::addRow("valid-%02d", row++) << u"𝐀𝐁𝐂abc𝐃𝐄𝐅def"_s << true;
    QTest::addRow("valid-%02d", row++) << u"abc𝐀𝐁𝐂def𝐃𝐄𝐅"_s << true;
    QTest::addRow("valid-%02d", row++) << QString(u"abc"_s + QChar(0x0000) + u"def"_s) << true;
    QTest::addRow("valid-%02d", row++) << QString(u"abc"_s + QChar(0xFFFF) + u"def"_s) << true;
    // check that BOM presence doesn't make any difference
    QTest::addRow("valid-%02d", row++) << (QString() + QChar(0xFEFF) + u"abc𝐀𝐁𝐂def𝐃𝐄𝐅"_s) << true;
    QTest::addRow("valid-%02d", row++) << (QString() + QChar(0xFFFE) + u"abc𝐀𝐁𝐂def𝐃𝐄𝐅"_s) << true;

    row = 0;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QChar(0xD800)) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + u"abc"_s + QChar(0xD800)) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QChar(0xD800) + u"def"_s) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + u"abc"_s + QChar(0xD800) + u"def"_s) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QChar(0xD800) + QChar(0xD800)) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + u"abc"_s + QChar(0xD800) + QChar(0xD800)) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + QChar(0xD800) + QChar(0xD800) + u"def"_s) << false;
    QTest::addRow("stray-high-%02d", row++) << (QString() + u"abc"_s + QChar(0xD800) + QChar(0xD800) + u"def"_s) << false;

    row = 0;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QChar(0xDC00)) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + u"abc"_s + QChar(0xDC00)) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QChar(0xDC00) + u"def"_s) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + u"abc"_s + QChar(0xDC00) + u"def"_s) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QChar(0xDC00) + QChar(0xDC00)) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + u"abc"_s + QChar(0xDC00) + QChar(0xDC00)) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + QChar(0xDC00) + QChar(0xDC00) + u"def"_s) << false;
    QTest::addRow("stray-low-%02d", row++) << (QString() + u"abc"_s + QChar(0xDC00) + QChar(0xDC00) + u"def"_s) << false;
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

    s = QString::fromUtf8("abc"); // detached
    const QChar *dataConstPtr = s.constData();
    QVERIFY(dataConstPtr != constPtr);

    const char16_t *char16Ptr = reinterpret_cast<const char16_t *>(s.utf16());

    QString s1 = s;
    QCOMPARE(s1.constData(), dataConstPtr);
    QCOMPARE(s1.unicode(), s.unicode());

    QChar *s1Ptr = s1.data(); // detaches here, because the string is not empty
    QVERIFY(s1Ptr != dataConstPtr);
    QVERIFY(s1.unicode() != s.unicode());

    *s1Ptr = u'd';
    QCOMPARE(s1, u"dbc");

    // utf pointer is valid while the string is not changed
    QCOMPARE(QString::fromUtf16(char16Ptr), s);
}

void tst_QString::clear()
{
    QString s;
    s.clear();
    QVERIFY(s.isEmpty());
    QVERIFY(!s.isDetached());

    s = u"some tests string"_s;
    QVERIFY(!s.isEmpty());

    s.clear();
    QVERIFY(s.isEmpty());
}

void tst_QString::first()
{
    QString a;

    QVERIFY(a.first(0).isEmpty());
    QVERIFY(!a.isDetached());

    a = u"ABCDEFGHIEfGEFG"_s; // 15 chars

    // lvalue
    QCOMPARE(a.first(5), u"ABCDE");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, not detached
    QCOMPARE(QString(a).first(5), u"ABCDE");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, detached
    QCOMPARE(detached(a).first(5), u"ABCDE");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");
}

void tst_QString::last()
{
    QString a;

    QVERIFY(a.last(0).isEmpty());
    QVERIFY(!a.isDetached());

    a = u"ABCDEFGHIEfGEFG"_s; // 15 chars

    // lvalue
    QCOMPARE(a.last(10), u"FGHIEfGEFG");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, not detached
    QCOMPARE(QString(a).last(10), u"FGHIEfGEFG");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, detached
    QCOMPARE(detached(a).last(10), u"FGHIEfGEFG");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");
}

void tst_QString::sliced()
{
    QString a;

    QVERIFY(a.sliced(0).isEmpty());
    QVERIFY(a.sliced(0, 0).isEmpty());
    QVERIFY(!a.isDetached());

    a = u"ABCDEFGHIEfGEFG"_s; // 15 chars

    // lvalue
    QCOMPARE(a.sliced(5), u"FGHIEfGEFG");
    QCOMPARE(a.sliced(5, 3), u"FGH");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, not detached
    QCOMPARE(QString(a).sliced(5), u"FGHIEfGEFG");
    QCOMPARE(QString(a).sliced(5, 3), u"FGH");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, detached
    QCOMPARE(detached(a).sliced(5), u"FGHIEfGEFG");
    QCOMPARE(detached(a).sliced(5, 3), u"FGH");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");
}

void tst_QString::slice()
{
    QString a;

    a.slice(0);
    QVERIFY(a.isEmpty());
    QVERIFY(a.isNull());
    a.slice(0, 0);
    QVERIFY(a.isEmpty());
    QVERIFY(a.isNull());

    a = u"Five pineapples"_s;

    a.slice(5);
    QCOMPARE_EQ(a, u"pineapples");

    a.slice(4, 3);
    QCOMPARE_EQ(a, u"app");

    a.slice(a.size());
    QVERIFY(a.isEmpty());
}

void tst_QString::chopped()
{
    QString a;

    QVERIFY(a.chopped(0).isEmpty());
    QVERIFY(!a.isDetached());

    a = u"ABCDEFGHIEfGEFG"_s; // 15 chars

    // lvalue
    QCOMPARE(a.chopped(10), u"ABCDE");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, not detached
    QCOMPARE(QString(a).chopped(10), u"ABCDE");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");

    // rvalue, detached
    QCOMPARE(detached(a).chopped(10), u"ABCDE");
    QCOMPARE(a, u"ABCDEFGHIEfGEFG");
}

void tst_QString::removeIf()
{
    QString a;

    auto pred = [](const QChar &c) { return c.isLower(); };
    a.removeIf(pred);
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    // Test when the string is not shared
    a = "aABbcCDd"_L1;
    QVERIFY(!a.data_ptr()->needsDetach());
    a.removeIf(pred);
    QCOMPARE(a, u"ABCD");

    // Test when the string is shared
    a = "aABbcCDd"_L1;
    QString b = a;
    QVERIFY(a.data_ptr()->needsDetach());
    a.removeIf(pred);
    QCOMPARE(a, u"ABCD");
    QCOMPARE(b, "aABbcCDd"_L1);

    auto removeA = [](const char c) { return c == 'a' || c == 'A'; };

    a = "aBcAbCa"_L1; // Not shared
    QCOMPARE(a.removeIf(removeA), u"BcbC");

    a = "aBcAbCa"_L1;
    b = a; // Shared
    QCOMPARE(a.removeIf(removeA), u"BcbC");
}

void tst_QString::std_stringview_conversion()
{
    static_assert(std::is_convertible_v<QString, std::u16string_view>);

    QString s;
    std::u16string_view sv(s);
    QCOMPARE(sv, std::u16string_view());

    s = u""_s;
    sv = s;
    QCOMPARE(s.size(), 0);
    QCOMPARE(sv.size(), size_t(0));
    QCOMPARE(sv, std::u16string_view());

    s = u"Hello"_s;
    sv = s;
    QCOMPARE(sv, std::u16string_view(u"Hello"));

    s = u"Hello\0world"_s;
    sv = s;
    QCOMPARE(s.size(), 11);
    QCOMPARE(sv.size(), size_t(11));
    QCOMPARE(sv, std::u16string_view(u"Hello\0world", 11));
}

// QString's collation order is only supported during the lifetime as QCoreApplication
QTEST_GUILESS_MAIN(tst_QString)

#include "tst_qstring.moc"
