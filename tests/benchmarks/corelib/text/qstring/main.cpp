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

    // Parsing:
    void toLongLong_data();
    void toLongLong();
    void toULongLong_data();
    void toULongLong();
    void toDouble_data();
    void toDouble();

private:
    void section_data_impl(bool includeRegExOnly = true);
    template <typename RX> void section_impl();
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
        s.toUpper();
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
        s.toLower();
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
        s.toCaseFolded();
    }
}

template <typename Integer>
void toWholeCommon_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("base");
    QTest::addColumn<bool>("good");
    QTest::addColumn<Integer>("expected");

    QTest::newRow("empty") << QStringLiteral("") << 10 << false << Integer(0ull);
    QTest::newRow("0") << QStringLiteral("0") << 10 << true << Integer(0ull);
    QTest::newRow("1234") << QStringLiteral("1234") << 10 << true << Integer(1234ull);
    QTest::newRow("1,234") << QStringLiteral("1,234") << 10 << false << Integer(0ull);
    QTest::newRow("123456789")
            << QStringLiteral("123456789") << 10 << true << Integer(123456789ull);
    QTest::newRow("bad1dea, base 16")
            << QStringLiteral("bad1dea") << 16 << true << Integer(0xBAD1DEAull);
    QTest::newRow("bad1dea, base 10") << QStringLiteral("bad1dea") << 10 << false << Integer(0ull);
    QTest::newRow("42, base 13") << QStringLiteral("42") << 13 << true << Integer(6ull * 9ull);
    QTest::newRow("242, base 8") << QStringLiteral("242") << 8 << true << Integer(0242ull);
    QTest::newRow("495, base 8") << QStringLiteral("495") << 8 << false << Integer(0ull);
    QTest::newRow("101101, base 2")
            << QStringLiteral("101101") << 2 << true << Integer(0b101101ull);
    QTest::newRow("ad, base 30") << QStringLiteral("ad") << 30 << true << Integer(313ull);
}

void tst_QString::toLongLong_data()
{
    toWholeCommon_data<qlonglong>();

    QTest::newRow("-1234") << QStringLiteral("-1234") << 10 << true << -1234ll;
    QTest::newRow("-123456789") << QStringLiteral("-123456789") << 10 << true << -123456789ll;
    QTest::newRow("-bad1dea, base 16") << QStringLiteral("-bad1dea") << 16 << true << -0xBAD1DEAll;
    QTest::newRow("-242, base 8") << QStringLiteral("-242") << 8 << true << -0242ll;
    QTest::newRow("-101101, base 2") << QStringLiteral("-101101") << 2 << true << -0b101101ll;
    QTest::newRow("-ad, base 30") << QStringLiteral("-ad") << 30 << true << -313ll;

    QTest::newRow("qlonglong-max")
            << QStringLiteral("9223372036854775807") << 10 << true
            << std::numeric_limits<qlonglong>::max();
    QTest::newRow("qlonglong-min")
            << QStringLiteral("-9223372036854775808") << 10 << true
            << std::numeric_limits<qlonglong>::min();
    QTest::newRow("qlonglong-max, base 2")
            << QString(63, u'1') << 2 << true << std::numeric_limits<qlonglong>::max();
    QTest::newRow("qlonglong-min, base 2")
            << (QStringLiteral("-1") + QString(63, u'0')) << 2 << true
            << std::numeric_limits<qlonglong>::min();
    QTest::newRow("qlonglong-max, base 16")
            << (QChar(u'7') + QString(15, u'f')) << 16 << true
            << std::numeric_limits<qlonglong>::max();
    QTest::newRow("qlonglong-min, base 16")
            << (QStringLiteral("-8") + QString(15, u'0')) << 16 << true
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
            << QStringLiteral("9223372036854775808") << 10 << true
            << (qulonglong(std::numeric_limits<qlonglong>::max()) + 1);
    QTest::newRow("qulonglong-max")
            << QStringLiteral("18446744073709551615") << 10 << true
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

    QTest::newRow("empty") << QStringLiteral("") << false << 0.0;
    QTest::newRow("0") << QStringLiteral("0") << true << 0.0;
    QTest::newRow("0.12340") << QStringLiteral("0.12340") << true << 0.12340;
    QTest::newRow("-0.12340") << QStringLiteral("-0.12340") << true << -0.12340;
    QTest::newRow("epsilon")
        << QStringLiteral("2.220446049e-16") << true << std::numeric_limits<double>::epsilon();
    QTest::newRow("1.0e-4") << QStringLiteral("1.0e-4") << true << 1.0e-4;
    QTest::newRow("1.0e+4") << QStringLiteral("1.0e+4") << true << 1.0e+4;
    QTest::newRow("10.e+3") << QStringLiteral("10.e+3") << true << 1.0e+4;
    QTest::newRow("10e+3.") << QStringLiteral("10e+3.") << false << 0.0;
    QTest::newRow("1e4") << QStringLiteral("1e4") << true << 1.0e+4;
    QTest::newRow("1.0e-8") << QStringLiteral("1.0e-8") << true << 1.0e-8;
    QTest::newRow("1.0e+8") << QStringLiteral("1.0e+8") << true << 1.0e+8;

    // NaN and infinity:
    QTest::newRow("nan") << QStringLiteral("nan") << true << qQNaN();
    QTest::newRow("NaN") << QStringLiteral("NaN") << true << qQNaN();
    QTest::newRow("-nan") << QStringLiteral("-nan") << false << 0.0;
    QTest::newRow("+nan") << QStringLiteral("+nan") << false << 0.0;
    QTest::newRow("inf") << QStringLiteral("inf") << true << qInf();
    QTest::newRow("Inf") << QStringLiteral("Inf") << true << qInf();
    QTest::newRow("+inf") << QStringLiteral("+inf") << true << qInf();
    QTest::newRow("-inf") << QStringLiteral("-inf") << true << -qInf();
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

QTEST_APPLESS_MAIN(tst_QString)

#include "main.moc"
