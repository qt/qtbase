// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QStringList>
#include <QByteArray>
#include <QLatin1StringView>
#include <QFile>
#include <QTest>
#include <limits>

using namespace Qt::StringLiterals;

class tst_QString: public QObject
{
    Q_OBJECT
public:
    tst_QString();
private slots:
    void section_regexp_data() { section_data_impl(); }
    void section_regularexpression_data() { section_data_impl(); }
    void section_regularexpression() { section_impl<QRegularExpression>(); }
    void section_string_data() { section_data_impl(false); }
    void section_string() { section_impl<QString>(); }

    void toUpper_data();
    void toUpper();
    void toLower_data();
    void toLower();
    void toCaseFolded_data();
    void toCaseFolded();

    // Serializing:
    void number_qlonglong_data();
    void number_qlonglong() { number_impl<qlonglong>(); }
    void number_qulonglong_data();
    void number_qulonglong() { number_impl<qulonglong>(); }

    void number_double_data();
    void number_double();

    // Parsing:
    void toLongLong_data();
    void toLongLong();
    void toULongLong_data();
    void toULongLong();
    void toDouble_data();
    void toDouble();

    // operator=(~)
#if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
    void operator_assign_BA() { operator_assign<QByteArray>(); }
    void operator_assign_BA_data() { operator_assign_data(); }
    void operator_assign_char() { operator_assign<const char*>(); };
    void operator_assign_char_data() { operator_assign_data();}
#endif
    void operator_assign_L1SV() { operator_assign<QLatin1StringView>(); }
    void operator_assign_L1SV_data() { operator_assign_data(); }

private:
    void section_data_impl(bool includeRegExOnly = true);
    template <typename RX> void section_impl();
    template <typename Integer> void number_impl();
    template <typename T> void operator_assign();
    void operator_assign_data();
};

tst_QString::tst_QString()
{
}

void tst_QString::section_data_impl(bool includeRegExOnly)
{
    QTest::addColumn<QString>("s");
    QTest::addColumn<QString>("sep");
    QTest::addColumn<bool>("isRegExp");

    QTest::newRow("IPv4") << QStringLiteral("192.168.0.1") << QStringLiteral(".") << false;
    QTest::newRow("IPv6") << QStringLiteral("2001:0db8:85a3:0000:0000:8a2e:0370:7334") << QStringLiteral(":") << false;
    if (includeRegExOnly) {
        QTest::newRow("IPv6-reversed-roles") << QStringLiteral("2001:0db8:85a3:0000:0000:8a2e:0370:7334") << QStringLiteral("\\d+") << true;
        QTest::newRow("IPv6-complex") << QStringLiteral("2001:0db8:85a3:0000:0000:8a2e:0370:7334") << QStringLiteral("(\\d+):\\1") << true;
    }
}

template <typename RX>
inline QString escape(const QString &s)
{ return RX::escape(s); }

template <>
inline QString escape<QString>(const QString &s)
{ return s; }

template <typename RX>
inline void optimize(RX &) {}

template <>
inline void optimize(QRegularExpression &rx)
{ rx.optimize(); }

template <typename RX>
void tst_QString::section_impl()
{
    QFETCH(QString, s);
    QFETCH(QString, sep);
    QFETCH(bool, isRegExp);

    RX rx(isRegExp ? sep : escape<RX>(sep));
    optimize(rx);
    for (int i = 0; i < 20; ++i)
        (void) s.count(rx); // make (s, rx) hot

    QBENCHMARK {
        const QString result = s.section(rx, 0, 16);
        Q_UNUSED(result);
    }
}

void tst_QString::toUpper_data()
{
    QTest::addColumn<QString>("s");

    QString lowerLatin1(300, QChar('a'));
    QString upperLatin1(300, QChar('A'));

    QString lowerDeseret;
    {
        QString pattern;
        pattern += QChar(QChar::highSurrogate(0x10428));
        pattern += QChar(QChar::lowSurrogate(0x10428));
        for (int i = 0; i < 300 / pattern.size(); ++i)
            lowerDeseret += pattern;
    }
    QString upperDeseret;
    {
        QString pattern;
        pattern += QChar(QChar::highSurrogate(0x10400));
        pattern += QChar(QChar::lowSurrogate(0x10400));
        for (int i = 0; i < 300 / pattern.size(); ++i)
            upperDeseret += pattern;
    }

    QString lowerLigature(600, QChar(0xFB03));

    QTest::newRow("600<a>") << (lowerLatin1 + lowerLatin1);
    QTest::newRow("600<A>") << (upperLatin1 + upperLatin1);

    QTest::newRow("300<a>+300<A>") << (lowerLatin1 + upperLatin1);
    QTest::newRow("300<A>+300<a>") << (upperLatin1 + lowerLatin1);

    QTest::newRow("300<10428>") << (lowerDeseret + lowerDeseret);
    QTest::newRow("300<10400>") << (upperDeseret + upperDeseret);

    QTest::newRow("150<10428>+150<10400>") << (lowerDeseret + upperDeseret);
    QTest::newRow("150<10400>+150<10428>") << (upperDeseret + lowerDeseret);

    QTest::newRow("300a+150<10400>") << (lowerLatin1 + upperDeseret);
    QTest::newRow("300a+150<10428>") << (lowerLatin1 + lowerDeseret);
    QTest::newRow("300A+150<10400>") << (upperLatin1 + upperDeseret);
    QTest::newRow("300A+150<10428>") << (upperLatin1 + lowerDeseret);

    QTest::newRow("600<FB03> (ligature)") << lowerLigature;

    QString fullRange;
    for (char32_t i = 0; i <= QChar::LastValidCodePoint; ++i)
        fullRange.append(QChar::fromUcs4(i));

    QTest::newRow("full-range") << fullRange;
}

void tst_QString::toUpper()
{
    QFETCH(QString, s);

    QBENCHMARK {
        [[maybe_unused]] auto r = s.toUpper();
    }
}

void tst_QString::toLower_data()
{
    toUpper_data();
}

void tst_QString::toLower()
{
    QFETCH(QString, s);

    QBENCHMARK {
        [[maybe_unused]] auto r = s.toLower();
    }
}

void tst_QString::toCaseFolded_data()
{
    toUpper_data();
}

void tst_QString::toCaseFolded()
{
    QFETCH(QString, s);

    QBENCHMARK {
        [[maybe_unused]] auto r = s.toCaseFolded();
    }
}

template <typename Integer>
void tst_QString::number_impl()
{
    QFETCH(Integer, number);
    QFETCH(int, base);
    QFETCH(QString, expected);

    QString actual;
    QBENCHMARK {
        actual = QString::number(number, base);
    }
    QCOMPARE(actual, expected);
}

template <typename Integer>
void number_integer_common()
{
    QTest::addColumn<Integer>("number");
    QTest::addColumn<int>("base");
    QTest::addColumn<QString>("expected");

    QTest::newRow("0") << Integer(0ull) << 10 << QStringLiteral("0");
    QTest::newRow("1234") << Integer(1234ull) << 10 << QStringLiteral("1234");
    QTest::newRow("123456789") << Integer(123456789ull) << 10 << QStringLiteral("123456789");
    QTest::newRow("bad1dea, base 16") << Integer(0xBAD1DEAull) << 16 << QStringLiteral("bad1dea");
    QTest::newRow("242, base 8") << Integer(0242ull) << 8 << QStringLiteral("242");
    QTest::newRow("101101, base 2") << Integer(0b101101ull) << 2 << QStringLiteral("101101");
    QTest::newRow("ad, base 30") << Integer(313ull) << 30 << QStringLiteral("ad");
}

void tst_QString::number_qlonglong_data()
{
    number_integer_common<qlonglong>();

    QTest::newRow("-1234") << -1234ll << 10 << QStringLiteral("-1234");
    QTest::newRow("-123456789") << -123456789ll << 10 << QStringLiteral("-123456789");
    QTest::newRow("-bad1dea, base 16") << -0xBAD1DEAll << 16 << QStringLiteral("-bad1dea");
    QTest::newRow("-242, base 8") << -0242ll << 8 << QStringLiteral("-242");
    QTest::newRow("-101101, base 2") << -0b101101ll << 2 << QStringLiteral("-101101");
    QTest::newRow("-ad, base 30") << -313ll << 30 << QStringLiteral("-ad");

    QTest::newRow("qlonglong-max")
            << std::numeric_limits<qlonglong>::max() << 10 << QStringLiteral("9223372036854775807");
    QTest::newRow("qlonglong-min")
            << std::numeric_limits<qlonglong>::min() << 10
            << QStringLiteral("-9223372036854775808");
    QTest::newRow("qlonglong-max, base 2")
            << std::numeric_limits<qlonglong>::max() << 2 << QString(63, u'1');
    QTest::newRow("qlonglong-min, base 2") << std::numeric_limits<qlonglong>::min() << 2
                                           << (QStringLiteral("-1") + QString(63, u'0'));
    QTest::newRow("qlonglong-max, base 16")
            << std::numeric_limits<qlonglong>::max() << 16 << (QChar(u'7') + QString(15, u'f'));
    QTest::newRow("qlonglong-min, base 16") << std::numeric_limits<qlonglong>::min() << 16
                                            << (QStringLiteral("-8") + QString(15, u'0'));
}

void tst_QString::number_qulonglong_data()
{
    number_integer_common<qulonglong>();

    QTest::newRow("qlonglong-max + 1")
            << (qulonglong(std::numeric_limits<qlonglong>::max()) + 1) << 10
            << QStringLiteral("9223372036854775808");
    QTest::newRow("qulonglong-max")
            << std::numeric_limits<qulonglong>::max() << 10
            << QStringLiteral("18446744073709551615");
    QTest::newRow("qulonglong-max, base 2")
            << std::numeric_limits<qulonglong>::max() << 2 << QString(64, u'1');
    QTest::newRow("qulonglong-max, base 16")
            << std::numeric_limits<qulonglong>::max() << 16 << QString(16, u'f');
}

void tst_QString::number_double_data()
{
    QTest::addColumn<double>("number");
    QTest::addColumn<char>("format");
    QTest::addColumn<int>("precision");
    QTest::addColumn<QString>("expected");

    struct
    {
        double d;
        char f;
        int p;
        QString expected;
    } data[] = {
        { 0.0, 'f', 0, QStringLiteral("0") },
        { 0.0001, 'f', 0, QStringLiteral("0") },
        { 0.1234, 'f', 5, QStringLiteral("0.12340") },
        { -0.1234, 'f', 5, QStringLiteral("-0.12340") },
        { 0.5 + qSqrt(1.25), 'f', 15, QStringLiteral("1.618033988749895") },
        { std::numeric_limits<double>::epsilon(), 'g', 10, QStringLiteral("2.220446049e-16") },
        { 0.0001, 'E', 1, QStringLiteral("1.0E-04") },
        { 1e8, 'E', 1, QStringLiteral("1.0E+08") },
        { -1e8, 'E', 1, QStringLiteral("-1.0E+08") },
    };

    for (auto &datum : data) {
        QTest::addRow("%s, format '%c', precision %d", qPrintable(datum.expected), datum.f,
                      datum.p)
                << datum.d << datum.f << datum.p << datum.expected;
    }
}

void tst_QString::number_double()
{
    QFETCH(double, number);
    QFETCH(char, format);
    QFETCH(int, precision);
    QFETCH(QString, expected);

    QString actual;
    QBENCHMARK {
        actual = QString::number(number, format, precision);
    }
    QCOMPARE(actual, expected);
}

template <typename Integer>
void toWholeCommon_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("base");
    QTest::addColumn<bool>("good");
    QTest::addColumn<Integer>("expected");

    QTest::newRow("empty") << u""_s << 10 << false << Integer(0ull);
    QTest::newRow("0") << u"0"_s << 10 << true << Integer(0ull);
    QTest::newRow("1234") << u"1234"_s << 10 << true << Integer(1234ull);
    QTest::newRow("1,234") << u"1,234"_s << 10 << false << Integer(0ull);
    QTest::newRow("123456789")
            << u"123456789"_s << 10 << true << Integer(123456789ull);
    QTest::newRow("bad1dea, base 16")
            << u"bad1dea"_s << 16 << true << Integer(0xBAD1DEAull);
    QTest::newRow("bad1dea, base 10") << u"bad1dea"_s << 10 << false << Integer(0ull);
    QTest::newRow("42, base 13") << u"42"_s << 13 << true << Integer(6ull * 9ull);
    QTest::newRow("242, base 8") << u"242"_s << 8 << true << Integer(0242ull);
    QTest::newRow("495, base 8") << u"495"_s << 8 << false << Integer(0ull);
    QTest::newRow("101101, base 2")
            << u"101101"_s << 2 << true << Integer(0b101101ull);
    QTest::newRow("ad, base 30") << u"ad"_s << 30 << true << Integer(313ull);
}

void tst_QString::toLongLong_data()
{
    toWholeCommon_data<qlonglong>();

    QTest::newRow("-1234") << u"-1234"_s << 10 << true << -1234ll;
    QTest::newRow("-123456789") << u"-123456789"_s << 10 << true << -123456789ll;
    QTest::newRow("-bad1dea, base 16") << u"-bad1dea"_s << 16 << true << -0xBAD1DEAll;
    QTest::newRow("-242, base 8") << u"-242"_s << 8 << true << -0242ll;
    QTest::newRow("-101101, base 2") << u"-101101"_s << 2 << true << -0b101101ll;
    QTest::newRow("-ad, base 30") << u"-ad"_s << 30 << true << -313ll;

    QTest::newRow("qlonglong-max")
            << u"9223372036854775807"_s << 10 << true
            << std::numeric_limits<qlonglong>::max();
    QTest::newRow("qlonglong-min")
            << u"-9223372036854775808"_s << 10 << true
            << std::numeric_limits<qlonglong>::min();
    QTest::newRow("qlonglong-max, base 2")
            << QString(63, u'1') << 2 << true << std::numeric_limits<qlonglong>::max();
    QTest::newRow("qlonglong-min, base 2")
            << (u"-1"_s + QString(63, u'0')) << 2 << true
            << std::numeric_limits<qlonglong>::min();
    QTest::newRow("qlonglong-max, base 16")
            << (QChar(u'7') + QString(15, u'f')) << 16 << true
            << std::numeric_limits<qlonglong>::max();
    QTest::newRow("qlonglong-min, base 16")
            << (u"-8"_s + QString(15, u'0')) << 16 << true
            << std::numeric_limits<qlonglong>::min();
}

void tst_QString::toLongLong()
{
    QFETCH(QString, text);
    QFETCH(int, base);
    QFETCH(bool, good);
    QFETCH(qlonglong, expected);

    qlonglong actual = expected;
    bool ok = false;
    QBENCHMARK {
        actual = text.toLongLong(&ok, base);
    }
    QCOMPARE(ok, good);
    QCOMPARE(actual, expected);
}

void tst_QString::toULongLong_data()
{
    toWholeCommon_data<qulonglong>();

    QTest::newRow("qlonglong-max + 1")
            << u"9223372036854775808"_s << 10 << true
            << (qulonglong(std::numeric_limits<qlonglong>::max()) + 1);
    QTest::newRow("qulonglong-max")
            << u"18446744073709551615"_s << 10 << true
            << std::numeric_limits<qulonglong>::max();
    QTest::newRow("qulonglong-max, base 2")
            << QString(64, u'1') << 2 << true << std::numeric_limits<qulonglong>::max();
    QTest::newRow("qulonglong-max, base 16")
            << QString(16, u'f') << 16 << true << std::numeric_limits<qulonglong>::max();
}

void tst_QString::toULongLong()
{
    QFETCH(QString, text);
    QFETCH(int, base);
    QFETCH(bool, good);
    QFETCH(qulonglong, expected);

    qulonglong actual = expected;
    bool ok = false;
    QBENCHMARK {
        actual = text.toULongLong(&ok, base);
    }
    QCOMPARE(ok, good);
    QCOMPARE(actual, expected);
}

void tst_QString::toDouble_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("good");
    QTest::addColumn<double>("expected");

    QTest::newRow("empty") << u""_s << false << 0.0;
    QTest::newRow("0") << u"0"_s << true << 0.0;
    QTest::newRow("0.12340") << u"0.12340"_s << true << 0.12340;
    QTest::newRow("-0.12340") << u"-0.12340"_s << true << -0.12340;
    QTest::newRow("epsilon")
        << u"2.220446049e-16"_s << true << std::numeric_limits<double>::epsilon();
    QTest::newRow("1.0e-4") << u"1.0e-4"_s << true << 1.0e-4;
    QTest::newRow("1.0e+4") << u"1.0e+4"_s << true << 1.0e+4;
    QTest::newRow("10.e+3") << u"10.e+3"_s << true << 1.0e+4;
    QTest::newRow("10e+3.") << u"10e+3."_s << false << 0.0;
    QTest::newRow("1e4") << u"1e4"_s << true << 1.0e+4;
    QTest::newRow("1.0e-8") << u"1.0e-8"_s << true << 1.0e-8;
    QTest::newRow("1.0e+8") << u"1.0e+8"_s << true << 1.0e+8;

    // NaN and infinity:
    QTest::newRow("nan") << u"nan"_s << true << qQNaN();
    QTest::newRow("NaN") << u"NaN"_s << true << qQNaN();
    QTest::newRow("-nan") << u"-nan"_s << false << 0.0;
    QTest::newRow("+nan") << u"+nan"_s << false << 0.0;
    QTest::newRow("inf") << u"inf"_s << true << qInf();
    QTest::newRow("Inf") << u"Inf"_s << true << qInf();
    QTest::newRow("+inf") << u"+inf"_s << true << qInf();
    QTest::newRow("-inf") << u"-inf"_s << true << -qInf();
}

void tst_QString::toDouble()
{
    QFETCH(QString, text);
    QFETCH(bool, good);
    QFETCH(double, expected);

    double actual = expected;
    bool ok = false;
    QBENCHMARK {
        actual = text.toDouble(&ok);
    }
    QCOMPARE(ok, good);
    QCOMPARE(actual, expected);
}

template <typename T> void tst_QString::operator_assign()
{
    QFETCH(QByteArray, data);
    QString str(data.size(), Qt::Uninitialized);

    T tdata;
    if constexpr (std::is_same_v<T, const char*>) {
        tdata = data.constData();
    } else if constexpr (std::is_same_v<T, QLatin1String>) {
        tdata = T(data.constData(), data.size());
    } else {
        tdata = T(data.constData(), data.size());
        tdata.detach();
    }

    QBENCHMARK {
        str.operator=(tdata);
    }
}

void tst_QString::operator_assign_data()
{
    QTest::addColumn<QByteArray>("data");

    QByteArray data;
    data.fill('a', 5);
    QTest::newRow("length: 5") << data;
    data.fill('b', 10);
    QTest::newRow("length: 10") << data;
    data.fill('c', 20);
    QTest::newRow("length: 20") << data;
    data.fill('d', 50);
    QTest::newRow("length: 50") << data;
    data.fill('e', 100);
    QTest::newRow("length: 100") << data;
    data.fill('f', 500);
    QTest::newRow("length: 500") << data;
    data.fill('g', 1'000);
    QTest::newRow("length: 1'000") << data;
}

QTEST_APPLESS_MAIN(tst_QString)

#include "tst_bench_qstring.moc"
