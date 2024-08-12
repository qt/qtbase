// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Do not include anything in this file. We are being #included in the unnamed namespace
// with a bunch of defines that may break other legitimate code.

#define LITERAL "some literal"
#define LITERAL_LEN (sizeof(LITERAL)-1)
#define LITERAL_EXTRA "some literal" "EXTRA"

// "some literal", but replacing all vowels by their umlauted UTF-8 string :)
#define UTF8_LITERAL "s\xc3\xb6m\xc3\xab l\xc3\xaft\xc3\xabr\xc3\xa4l"
#define UTF8_LITERAL_LEN (sizeof(UTF8_LITERAL)-1)
#define UTF8_LITERAL_EXTRA "s\xc3\xb6m\xc3\xab l\xc3\xaft\xc3\xabr\xc3\xa4l" "EXTRA"

// "some literal", but replacing all vocals by their umlauted UTF-8 string :)
#define UNICODE_LITERAL u"s\u00f6m\u00eb l\u00eft\u00ebr\u00e4l"
#define UNICODE_LITERAL_LEN ((sizeof(UNICODE_LITERAL) - 1) / 2)
#define UNICODE_LITERAL_EXTRA u"s\u00f6m\u00eb l\u00eft\u00ebr\u00e4l" "EXTRA"

#ifndef P
# error You need to define P
#endif

//fix for gcc4.0: if the operator+ does not exist without QT_USE_FAST_OPERATOR_PLUS
#ifndef QT_USE_FAST_CONCATENATION
#define Q %
#else
#define Q P
#endif

template <typename T> QString toQString(const T &t);

template <> QString toQString(const QString &s) { return s; }
template <> QString toQString(const QStringView &v) { return v.toString(); }
template <> QString toQString(const QLatin1String &l) { return l; }
template <> QString toQString(const QLatin1Char &l) { return QChar(l); }
template <> QString toQString(const QChar &c) { return c; }
template <> QString toQString(const QChar::SpecialCharacter &c) { return QChar(c); }
template <> QString toQString(char16_t * const &p) { return QStringView(p).toString(); }
template <size_t N> QString toQString(const char16_t (&a)[N]) { return QStringView(a).toString(); }
template <> QString toQString(const char16_t &c) { return QChar(c); }

template <typename T> QByteArray toQByteArray(const T &t);

template <> QByteArray toQByteArray(const QByteArray &b) { return b; }
template <> QByteArray toQByteArray(const QByteArrayView &bav) { return bav.toByteArray(); }
template <> QByteArray toQByteArray(char * const &p) { return p; }
template <size_t N> QByteArray toQByteArray(const char (&a)[N]) { return a; }
template <> QByteArray toQByteArray(const char &c) { return QByteArray(&c, 1); }

template <typename String, typename Separator>
void checkItWorksWithFreeSpaceAtBegin(const String &chunk, const Separator &separator)
{
    // GIVEN: a String with freeSpaceAtBegin() and less than chunk.size() freeSpaceAtEnd()
    String str;

    int prepends = 0;
    const int max_prepends = 10;
    while (str.data_ptr().freeSpaceAtBegin() < chunk.size() && prepends++ < max_prepends)
        str.prepend(chunk);
    QVERIFY(prepends < max_prepends);

    int appends = 0;
    const int max_appends = 100;
    while (str.data_ptr().freeSpaceAtEnd() >= chunk.size() && appends++ < max_appends)
        str.append(chunk);
    QVERIFY(appends < max_appends);

    QVERIFY(str.capacity() - str.size() >= chunk.size());
    QVERIFY(str.data_ptr().freeSpaceAtEnd() < chunk.size());

    // WHEN: adding a QStringBuilder expression which exceeds freeSpaceAtEnd()
    str += separator P chunk;

    // THEN: it doesn't crash (QTBUG-99330)
    const String expected = chunk.repeated(prepends + appends) + separator + chunk;
    QCOMPARE(str, expected);
}

template <typename String>
void checkNullVsEmpty(const String &empty)
{
    String a;
    String b;
    QVERIFY(a.isNull());
    QVERIFY(b.isNull());
    String result = a P b;
    QVERIFY(result.isNull());

    String d = empty;
    QVERIFY(d.isEmpty());
    QVERIFY(!d.isNull());
    result = a P d;
    QVERIFY(result.isEmpty());
    QVERIFY(!result.isNull());

    result = a P a P a;
    QVERIFY(result.isNull());
}

namespace CheckAuto {
// T is cvref-qualified, using universal reference deduction rules.
template <typename T> struct Helper;

// These specializations forward to the non-const ones, and add const on top.
template <typename T> struct Helper<const T>
{
    static const T create() { return Helper<T>::create(); }
    static const T createNull() { return Helper<T>::createNull(); }
};
template <typename T> struct Helper<const T &>
{
    static const T &create() { return Helper<T &>::create(); }
    static const T &createNull() { return Helper<T &>::createNull(); }
};

template <> struct Helper<QString>
{
    static QString create() { return QString::fromUtf8("QString rvalue"); }
    static QString createNull() { return QString(); }
};

template <> struct Helper<QString &>
{
    static QString &create() { static QString s = QString::fromUtf8("QString lvalue"); return s; }
    static QString &createNull() { static QString s; return s; }
};

template <> struct Helper<QStringView>
{
    static QStringView create() { return QStringView(u"QStringView rvalue"); }
    static QStringView createNull() { return QStringView(); }
};

template <> struct Helper<QStringView &>
{
    static QStringView &create() { static QStringView s = u"QStringView lvalue"; return s; }
    static QStringView &createNull() { static QStringView s; return s; }
};

template <> struct Helper<QByteArray>
{
    static QByteArray create() { return QByteArray("QByteArray rvalue"); }
    static QByteArray createNull() { return QByteArray(); }
};

template <> struct Helper<QByteArray &>
{
    static QByteArray &create() { static QByteArray ba = QByteArray("QByteArray lvalue"); return ba; }
    static QByteArray &createNull() { static QByteArray ba; return ba; }
};

template <> struct Helper<QByteArrayView>
{
    static QByteArrayView create() { return QByteArrayView("QByteArrayView rvalue"); }
    static QByteArrayView createNull() { return QByteArrayView(); }
};

template <> struct Helper<QByteArrayView &>
{
    static QByteArrayView &create() { static QByteArrayView ba = "QByteArrayView lvalue"; return ba; }
    static QByteArrayView &createNull() { static QByteArrayView ba; return ba; }
};

template <> struct Helper<const char *>
{
    static const char *create() { return "const char * rvalue"; }
    static const char *createNull() { return ""; }
};

template <> struct Helper<const char *&>
{
    static const char *&create() { static const char *s = "const char * lvalue"; return s; }
    static const char *&createNull() { static const char *s = ""; return s; }
};

template <typename String1, typename String2, typename Result>
void checkAutoImpl3()
{
    {
        auto result = Helper<String1>::create() P Helper<String2>::create();
        Result expected = result;
        QCOMPARE(result, expected);
    }
    {
        auto result = Helper<String2>::create() P Helper<String1>::create();
        Result expected = result;
        QCOMPARE(result, expected);
    }
    {
        auto result = Helper<String1>::create() P Helper<String2>::create() P Helper<String1>::create();
        Result expected = result;
        QCOMPARE(result, expected);
    }
    {
        auto result = Helper<String2>::create() P Helper<String1>::create() P Helper<String2>::create();
        Result expected = result;
        QCOMPARE(result, expected);
    }
    {
        auto result = Helper<String1>::createNull() P Helper<String2>::create();
        Result expected = result;
        QCOMPARE(result, expected);
    }
    {
        auto result = Helper<String1>::createNull() P Helper<String2>::createNull();
        Result expected = result;
        QCOMPARE(result, expected);
    }
}

template <typename String1, typename String2, typename Result>
void checkAutoImpl2()
{
    checkAutoImpl3<String1  , String2  , Result>();
    checkAutoImpl3<String1 &, String2  , Result>();
    checkAutoImpl3<String1  , String2 &, Result>();
    checkAutoImpl3<String1 &, String2 &, Result>();
}

template <typename String1, typename String2, typename Result>
void checkAutoImpl()
{
    checkAutoImpl2<      String1,       String2, Result>();
    checkAutoImpl2<const String1,       String2, Result>();
    checkAutoImpl2<      String1, const String2, Result>();
    checkAutoImpl2<const String1, const String2, Result>();
}

} // namespace CheckAuto

void checkAuto()
{
    CheckAuto::checkAutoImpl<QString, QString, QString>();
    CheckAuto::checkAutoImpl<QString, QStringView, QString>();

    CheckAuto::checkAutoImpl<QByteArray, QByteArray, QByteArray>();
    CheckAuto::checkAutoImpl<QByteArray, const char *, QByteArray>();
    CheckAuto::checkAutoImpl<QByteArray, QByteArrayView, QByteArray>();

#ifndef QT_NO_CAST_FROM_ASCII
    CheckAuto::checkAutoImpl<QString, const char *, QString>();
    CheckAuto::checkAutoImpl<QString, QByteArray, QString>();
#endif
}

void runScenario()
{
    // this code is latin1. TODO: replace it with the utf8 block below, once
    // strings default to utf8.
    QLatin1String l1string(LITERAL);
    QString string(l1string);
    QStringView stringview = QStringView{ string }.mid(2, 10);
    QLatin1Char lchar('c');
    QChar qchar(lchar);
    QChar::SpecialCharacter special(QChar::Nbsp);
    char16_t u16char = UNICODE_LITERAL[0];
    char16_t u16chararray[] = { u's', 0xF6, u'm', 0xEB, u' ', u'l', 0xEF, u't', 0xEB, u'r', 0xE4, u'l', 0x00 };
    QCOMPARE(QStringView(u16chararray), QStringView(UNICODE_LITERAL));
    char16_t *u16charstar = u16chararray;

#define CHECK(QorP, a1, a2) \
    do { \
        QCOMPARE(QString(a1 QorP a2), toQString(a1).append(toQString(a2))); \
        QCOMPARE(QString(a2 QorP a1), toQString(a2).append(toQString(a1))); \
    } while (0)

    CHECK(P, l1string, l1string);
    CHECK(P, l1string, string);
    CHECK(P, l1string, QString(string));
    CHECK(Q, l1string, string);
    CHECK(Q, l1string, QString(string));

    CHECK(Q, l1string, stringview);
    CHECK(P, l1string, lchar);
    CHECK(P, l1string, qchar);
    CHECK(P, l1string, special);
    CHECK(P, l1string, QStringLiteral(LITERAL));
    CHECK(Q, l1string, u16char);
    CHECK(Q, l1string, u16chararray);
    CHECK(Q, l1string, u16charstar);

    CHECK(P, string, string);
    CHECK(P, string, QString(string));
    CHECK(P, QString(string), QString(string));
    CHECK(Q, string, string);
    CHECK(Q, string, QString(string));
    CHECK(Q, QString(string), QString(string));

    CHECK(P, string, stringview);
    CHECK(P, QString(string), stringview);
    CHECK(Q, string, stringview);
    CHECK(Q, QString(string), stringview);

    CHECK(P, string, lchar);
    CHECK(P, QString(string), lchar);
    CHECK(Q, string, lchar);
    CHECK(Q, QString(string), lchar);

    CHECK(P, string, qchar);
    CHECK(P, QString(string), qchar);
    CHECK(P, string, special);
    CHECK(P, QString(string), special);
    CHECK(P, string, QStringLiteral(LITERAL));
    CHECK(P, QString(string), QStringLiteral(LITERAL));
    CHECK(Q, string, qchar);
    CHECK(Q, QString(string), qchar);
    CHECK(Q, string, special);
    CHECK(Q, QString(string), special);
    CHECK(Q, string, QStringLiteral(LITERAL));
    CHECK(Q, QString(string), QStringLiteral(LITERAL));

    CHECK(Q, string, u16char);
    CHECK(Q, QString(string), u16char);
    CHECK(Q, string, u16chararray);
    CHECK(Q, QString(string), u16chararray);
    CHECK(Q, string, u16charstar);
    CHECK(Q, QString(string), u16charstar);

    CHECK(Q, stringview, stringview);
    CHECK(Q, stringview, lchar);
    CHECK(Q, stringview, qchar);
    CHECK(Q, stringview, special);
    CHECK(P, stringview, QStringLiteral(LITERAL));
    CHECK(Q, stringview, QStringLiteral(LITERAL));
    CHECK(Q, stringview, u16char);
    CHECK(Q, stringview, u16chararray);
    CHECK(Q, stringview, u16charstar);

#if defined(QT_USE_QSTRINGBUILDER)
    CHECK(P, lchar, lchar);
    CHECK(P, lchar, special);
#endif
    CHECK(P, lchar, qchar);
    CHECK(P, lchar, QStringLiteral(LITERAL));
    CHECK(Q, lchar, u16char);
    CHECK(Q, lchar, u16chararray);
    CHECK(Q, lchar, u16charstar);

#if defined(QT_USE_QSTRINGBUILDER)
    CHECK(P, qchar, qchar);
#endif
    CHECK(P, qchar, special);
    CHECK(P, qchar, QStringLiteral(LITERAL));
    CHECK(Q, qchar, u16char);
    CHECK(Q, qchar, u16chararray);
    CHECK(Q, qchar, u16charstar);

#if defined(QT_USE_QSTRINGBUILDER)
    CHECK(P, special, special);
#endif
    CHECK(P, special, QStringLiteral(LITERAL));
    CHECK(Q, special, u16char);
    CHECK(Q, special, u16chararray);
    CHECK(Q, special, u16charstar);

    CHECK(P, QStringLiteral(LITERAL), QStringLiteral(LITERAL));
    CHECK(Q, QStringLiteral(LITERAL), u16char);
    CHECK(Q, QStringLiteral(LITERAL), u16chararray);
    CHECK(Q, QStringLiteral(LITERAL), u16charstar);

    // CHECK(Q, u16char, u16char);             // BUILTIN <-> BUILTIN cat't be overloaded
    // CHECK(Q, u16char, u16chararray);
    // CHECK(Q, u16char, u16charstar);

    // CHECK(Q, u16chararray, u16chararray);   // BUILTIN <-> BUILTIN cat't be overloaded
    // CHECK(Q, u16chararray, u16charstar);

    // CHECK(Q, u16charstar, u16charstar);     // BUILTIN <-> BUILTIN cat't be overloaded

#undef CHECK

#define CHECK(QorP, a1, a2) \
    do { \
        QCOMPARE(QByteArray(a1 QorP a2), toQByteArray(a1).append(toQByteArray(a2))); \
        QCOMPARE(QByteArray(a2 QorP a1), toQByteArray(a2).append(toQByteArray(a1))); \
    } while (0)

    QByteArray bytearray = stringview.toUtf8();
    QByteArrayView baview = QByteArrayView(bytearray).mid(0, bytearray.size() - 2);
    char *charstar = bytearray.data();
    char chararray[3] = { 'H', 'i', '\0' };
    const char constchararray[3] = { 'H', 'i', '\0' };
    char achar = 'a';
    char embedded_NULs[16] = { 'H', 'i' };
    const char const_embedded_NULs[16] = { 'H', 'i' };

    CHECK(P, bytearray, bytearray);
    CHECK(P, QByteArray(bytearray), bytearray);
    CHECK(P, QByteArray(bytearray), QByteArray(bytearray));
    CHECK(P, bytearray, baview);
    CHECK(P, QByteArray(bytearray), baview);
    CHECK(P, bytearray, charstar);
    CHECK(Q, bytearray, bytearray);
    CHECK(Q, QByteArray(bytearray), bytearray);
    CHECK(Q, QByteArray(bytearray), QByteArray(bytearray));
    CHECK(Q, bytearray, baview);
    CHECK(Q, QByteArray(bytearray), baview);
    CHECK(Q, bytearray, charstar);

#ifndef Q_CC_MSVC // see QTBUG-65359
    CHECK(P, bytearray, chararray);
#else
    Q_UNUSED(chararray);
#endif
    CHECK(P, bytearray, constchararray);
    CHECK(P, bytearray, achar);
    CHECK(Q, bytearray, constchararray);
    CHECK(Q, bytearray, achar);
    CHECK(Q, bytearray, embedded_NULs);
    CHECK(Q, bytearray, const_embedded_NULs);

    CHECK(Q, baview, bytearray);
    CHECK(Q, baview, charstar);
    CHECK(Q, baview, chararray);
    CHECK(Q, baview, constchararray);
    CHECK(Q, baview, achar);
    CHECK(Q, baview, embedded_NULs);
    CHECK(Q, baview, const_embedded_NULs);

    // Check QString/QByteArray consistency when appending const char[] with embedded NULs:
    {
        const QByteArray ba = baview Q embedded_NULs;
        QEXPECT_FAIL("", "QTBUG-117321", Continue);
        QCOMPARE(ba.size(), baview.size() + q20::ssize(embedded_NULs) - 1);

#ifndef QT_NO_CAST_FROM_ASCII
        const auto l1s = QLatin1StringView{baview}; // l1string != baview

        const QString s = l1s Q embedded_NULs;
        QCOMPARE(s.size(), l1s.size() + q20::ssize(embedded_NULs) - 1);

        QEXPECT_FAIL("", "QTBUG-117321", Continue);
        QCOMPARE(s, ba);
#endif
    }

    //CHECK(Q, charstar, charstar);     // BUILTIN <-> BUILTIN cat't be overloaded
    //CHECK(Q, charstar, chararray);
    //CHECK(Q, charstar, achar);

    //CHECK(Q, chararray, chararray);   // BUILTIN <-> BUILTIN cat't be overloaded
    //CHECK(Q, chararray, achar);

    //CHECK(Q, achar, achar);           // BUILTIN <-> BUILTIN cat't be overloaded

#undef CHECK

    QString r2(QLatin1String(LITERAL LITERAL));
    QString r3 = QString::fromUtf8(UTF8_LITERAL UTF8_LITERAL);
    QString r;

    // self-assignment:
    r = stringview.toString();
    r = lchar + r;
#ifdef QT_USE_QSTRINGBUILDER
    QCOMPARE(r, QString(lchar P stringview));
#endif

    r = QStringLiteral(UNICODE_LITERAL);
    r = r Q QStringLiteral(UNICODE_LITERAL);
    QCOMPARE(r, r3);

#ifndef QT_NO_CAST_FROM_ASCII
    r = string P LITERAL;
    QCOMPARE(r, r2);
    r = LITERAL P string;
    QCOMPARE(r, r2);

    QByteArray ba = QByteArray(LITERAL);
    r = ba P string;
    QCOMPARE(r, r2);
    r = string P ba;
    QCOMPARE(r, r2);

    r = string P QByteArrayLiteral(LITERAL);
    QCOMPARE(r, r2);
    r = QByteArrayLiteral(LITERAL) P string;
    QCOMPARE(r, r2);

    r = ba P l1string;
    QCOMPARE(r, r2);
    r = l1string P ba;
    QCOMPARE(r, r2);

    r = ba P QLatin1String(l1string);
    QCOMPARE(r, r2);
    r = QLatin1String(l1string) P std::as_const(ba);
    QCOMPARE(r, r2);

    static const char badata[] = LITERAL_EXTRA;
    ba = QByteArray::fromRawData(badata, LITERAL_LEN);
    r = ba P string;
    QCOMPARE(r, r2);
    r = string P ba;
    QCOMPARE(r, r2);

    string = QString::fromUtf8(UTF8_LITERAL);
    ba = UTF8_LITERAL;

    r = string P UTF8_LITERAL;
    QCOMPARE(r.size(), r3.size());
    QCOMPARE(r, r3);
    r = UTF8_LITERAL P string;
    QCOMPARE(r, r3);
    r = ba P string;
    QCOMPARE(r, r3);
    r = string P ba;
    QCOMPARE(r, r3);

    ba = QByteArray::fromRawData(UTF8_LITERAL_EXTRA, UTF8_LITERAL_LEN);
    r = ba P string;
    QCOMPARE(r, r3);
    r = string P ba;
    QCOMPARE(r, r3);

    ba = QByteArray(); // empty
    r = ba P string;
    QCOMPARE(r, string);
    r = string P ba;
    QCOMPARE(r, string);

    const char *zero = nullptr;
    r = string P zero;
    QCOMPARE(r, string);
    r = zero P string;
    QCOMPARE(r, string);
#endif

    string = QString::fromLatin1(LITERAL);
    QCOMPARE(QByteArray(qPrintable(string P string)), QByteArray(string.toLatin1() + string.toLatin1()));



    //QByteArray
    {
        QByteArray ba = LITERAL;
        QByteArray superba = ba P ba P LITERAL;
        QCOMPARE(superba, QByteArray(LITERAL LITERAL LITERAL));

        ba = QByteArrayLiteral(LITERAL);
        QCOMPARE(ba, QByteArray(LITERAL));
        superba = ba P QByteArrayLiteral(LITERAL) P LITERAL;
        QCOMPARE(superba, QByteArray(LITERAL LITERAL LITERAL));

        QByteArray testWith0 = ba P "test\0with\0zero" P ba;
        QCOMPARE(testWith0, QByteArray(LITERAL "test" LITERAL));

        QByteArray ba2 = ba P '\0' + LITERAL;
        QCOMPARE(ba2, QByteArray(LITERAL "\0" LITERAL, ba.size()*2+1));

        const char *mmh = "test\0foo";
        QCOMPARE(QByteArray(ba P mmh P ba), testWith0);

        QByteArray raw = QByteArray::fromRawData(UTF8_LITERAL_EXTRA, UTF8_LITERAL_LEN);
        QByteArray r = "hello" P raw;
        QByteArray r2 = "hello" UTF8_LITERAL;
        QCOMPARE(r, r2);
        r2 = QByteArray("hello\0") P UTF8_LITERAL;
        QCOMPARE(r, r2);

        const char *zero = nullptr;
        r = ba P zero;
        QCOMPARE(r, ba);
        r = zero P ba;
        QCOMPARE(r, ba);

#ifdef QT_USE_QSTRINGBUILDER
        QByteArrayView qbav = LITERAL;
        superba = qbav P qbav P LITERAL;
        QCOMPARE(superba, QByteArray(LITERAL LITERAL LITERAL));
#endif
    }

    //operator QString  +=
    {
        QString str = QString::fromUtf8(UTF8_LITERAL);
        str +=  QLatin1String(LITERAL) P str;
        QCOMPARE(str, QString::fromUtf8(UTF8_LITERAL LITERAL UTF8_LITERAL));
#ifndef QT_NO_CAST_FROM_ASCII
        str = (QString::fromUtf8(UTF8_LITERAL) += QLatin1String(LITERAL) P UTF8_LITERAL);
        QCOMPARE(str, QString::fromUtf8(UTF8_LITERAL LITERAL UTF8_LITERAL));
#endif

        QString str2 = QString::fromUtf8(UTF8_LITERAL);
        QString str2_e = QString::fromUtf8(UTF8_LITERAL);
        const char * nullData = 0;
        str2 += QLatin1String(nullData) P str2;
        str2_e += QLatin1String("") P str2_e;
        QCOMPARE(str2, str2_e);
    }

    checkItWorksWithFreeSpaceAtBegin(QString::fromUtf8(UTF8_LITERAL),
                                 #ifdef QT_NO_CAST_FROM_ASCII
                                     QLatin1String("1234")
                                 #else
                                     "1234"
                                 #endif
                                     );
    if (QTest::currentTestFailed())
        return;

    //operator QByteArray  +=
    {
        QByteArray ba = UTF8_LITERAL;
        ba +=  QByteArray(LITERAL) P UTF8_LITERAL;
        QCOMPARE(ba, QByteArray(UTF8_LITERAL LITERAL UTF8_LITERAL));
        ba += LITERAL P QByteArray::fromRawData(UTF8_LITERAL_EXTRA, UTF8_LITERAL_LEN);
        QCOMPARE(ba, QByteArray(UTF8_LITERAL LITERAL UTF8_LITERAL LITERAL UTF8_LITERAL));
        QByteArray withZero = QByteArray(LITERAL "\0" LITERAL, LITERAL_LEN*2+1);
        QByteArray ba2 = withZero;
        ba2 += ba2 P withZero;
        QCOMPARE(ba2, QByteArray(withZero + withZero + withZero));
    }

    // null vs. empty
    checkNullVsEmpty(QStringLiteral(""));
    checkNullVsEmpty(QByteArrayLiteral(""));

    // auto
    checkAuto();

    checkItWorksWithFreeSpaceAtBegin(QByteArray(UTF8_LITERAL), "1234");
    if (QTest::currentTestFailed())
        return;
}
