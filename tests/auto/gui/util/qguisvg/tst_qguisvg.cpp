// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>

#include <QList>
#include <QString>
#include <QtTypes>

#include <private/qguisvg_p.h>

#include <cmath>
#include <limits>
#include <utility>

class tst_QGuiSvg : public QObject
{
    Q_OBJECT

public:
    tst_QGuiSvg() = default;
    virtual ~tst_QGuiSvg() = default;

private slots:
    void trimmed_data();
    void trimmed();
    void testToDouble_data();
    void testToDouble();
};

void tst_QGuiSvg::trimmed_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QString>("outTiny");

    QTest::newRow("empty") << "" << "" << "";
    QTest::newRow("space") << " " << "" << "";
    QTest::newRow("0-0") << "a" << "a" << "a";
    QTest::newRow("0-1") << "a " << "a" << "a";
    QTest::newRow("0-2") << "a  " << "a" << "a";
    QTest::newRow("1-0") << " a" << "a" << "a";
    QTest::newRow("1-1") << " a " << "a" << "a";
    QTest::newRow("1-2") << " a  " << "a" << "a";
    QTest::newRow("2-0") << "  a" << "a" << "a";
    QTest::newRow("2-1") << "  a " << "a" << "a";
    QTest::newRow("2-2") << "  a  " << "a" << "a";
    QTest::newRow("mid") << "x y" << "x y" << "x y";

    // spaces according to SVG
    QTest::newRow("ht") << "\tb\t" << "b" << "b";
    QTest::newRow("lf") << "\nb\n" << "b" << "b";
    QTest::newRow("ff") << "\fb\f" << "b" << "\fb\f";
    QTest::newRow("cr") << "\rb\r" << "b" << "b";
    QTest::newRow("sp") << " b " << "b" << "b";

    // spaces according to QChar but not SVG
    QTest::newRow("vt") << "\va\v" << "\va\v" << "\va\v";
    QTest::newRow("nel") << "\u0085a\u0085" << "\u0085a\u0085" << "\u0085a\u0085";
    QTest::newRow("nbsp") << "\u00A0a\u00A0" << "\u00A0a\u00A0" << "\u00A0a\u00A0";
}

void tst_QGuiSvg::trimmed()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QString, outTiny);

    QCOMPARE(QGuiSvg::trimmed(in, false), out);
    QCOMPARE(QGuiSvg::trimmed(in, true), outTiny);
}

void tst_QGuiSvg::testToDouble_data()
{
    // make sure we can use NAN (no fast-math)
    Q_ASSERT(NAN != NAN);
    Q_ASSERT(qreal(NAN) != qreal(NAN));

    QTest::addColumn<QString>("numString");
    QTest::addColumn<qreal>("value");

    using S = std::pair<QString, qreal>;

    const QList<S> signs = { { "", 1. }, { "+", 1. }, { "-", -1. }, { "$", NAN } };
    const QList<S> digits = {
        { "0", 0. },       { "00", 0. },    { "1", 1. },     { "01", 1. },
        { "23", 23. },     { "0023", 23. }, { "456", 456. }, { "7890", 7890. },
        { "9999", 9999. }, { "4x4", NAN },  { "acdc", NAN }, { "ACDC", NAN }
    };

    QList<S> integer = {
        { "-32768", -32768. },
        { "32767", 32767. },
        { "+32767", 32767. },
    };
    integer.reserve(integer.size() + signs.size() * digits.size());
    for (const auto &sign : signs) {
        for (const auto &digitPart : digits) {
            // TODO: Test empty string which is not a valid integer
            integer.push_back({ sign.first + digitPart.first,
                                sign.second * digitPart.second });
        }
    }

    QList<S> decimal_number = integer;
    // limits of conforming SVG Tiny 1.2 content
    decimal_number.push_back({ "-32767.9999", -32767.9999 });
    decimal_number.push_back({ "32767.9999", 32767.9999 });
    decimal_number.push_back({ "+32767.9999", 32767.9999 });
    decimal_number.reserve(decimal_number.size()
                           + signs.size() * digits.size() * (digits.size() + 1));
    // TODO: Test decimal point without following digit which
    //       is not a valid decimal number: QTBUG-143993
    for (const auto &sign : signs) {
        for (const auto &fractDigits : digits) {
            const qreal fractPart =
                    fractDigits.second * std::pow(10., -fractDigits.first.toLatin1().length());
            decimal_number.push_back({ sign.first + "." + fractDigits.first,
                                       sign.second * fractPart });
            for (const auto &wholeDigits : digits) {
                decimal_number.push_back({
                    sign.first + wholeDigits.first + "." + fractDigits.first,
                    sign.second * (wholeDigits.second + fractPart)
                });
            }
        }
    }

    // TODO: Test 'E' or 'e' without following digit which is not a valid scientific number
    constexpr qsizetype exponentCount = 2;
    constexpr QChar exponentChars[exponentCount]{ 'E', 'e' };
    // current implementation's limits
    QList<S> scientific_number = {
        { "-3.4028235e38", -3.4028235e38 },
        { "3.4028235e38", 3.4028235e38 },
        { "+3.4028235e38", 3.4028235e38 },
    };
    scientific_number.reserve(scientific_number.size()
                              + integer.size() * decimal_number.size() * exponentCount);
    for (const auto &exponent : std::as_const(integer)) {
        // The current implementation handles values up to about +/-3.4e(+/-38) mathematically
        // correct, see above. The following check avoids values exceeding these limits because they
        // were known to fail anyway.
        // I don't completely rely on the numeric_limits tested below because the current
        // implementation returns zero for strings like "0e9999" which is correct math but might
        // not be valid svg.
        if ((exponent.second < -38 || 38 < exponent.second))
            continue;
        for (const auto &mantissa : std::as_const(decimal_number)) {
            const qreal value = mantissa.second * std::pow(10., exponent.second);
            if (value < std::numeric_limits<float>::lowest()
                || std::numeric_limits<float>::max() < value)
                continue;
            for (auto e : exponentChars)
                scientific_number.push_back({ mantissa.first + e + exponent.first,
                                              value });
        }
    }
    const auto number{ decimal_number + scientific_number };
    for (const auto &n : number)
        QTest::newRow(n.first.toStdString().c_str()) << n.first << n.second;
}

void tst_QGuiSvg::testToDouble()
{
    QFETCH(QString, numString);
    QFETCH(qreal, value);

    bool ok = false;
    const qreal actual = QGuiSvg::toDouble(numString, &ok);
    QCOMPARE(ok, !std::isnan(value));
    if (ok)
        QCOMPARE(actual, value);
}

QTEST_MAIN(tst_QGuiSvg)
#include "tst_qguisvg.moc"
