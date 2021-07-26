/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <QStringList>
#include <QFile>
#include <QTest>
#include <limits>

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

    void number_qlonglong_data();
    void number_qlonglong() { number_impl<qlonglong>(); }
    void number_qulonglong_data();
    void number_qulonglong() { number_impl<qulonglong>(); }

    void number_double_data();
    void number_double();

private:
    void section_data_impl(bool includeRegExOnly = true);
    template <typename RX> void section_impl();
    template <typename Integer> void number_impl();
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
    QTest::addColumn<qlonglong>("number");
    QTest::addColumn<int>("base");
    QTest::addColumn<QString>("expected");

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
    QTest::addColumn<qulonglong>("number");
    QTest::addColumn<int>("base");
    QTest::addColumn<QString>("expected");

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

QTEST_APPLESS_MAIN(tst_QString)

#include "tst_bench_qstring.moc"
