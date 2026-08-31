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

using namespace Qt::Literals::StringLiterals;

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

    void testParsePath_data();
    void testParsePath();
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

void tst_QGuiSvg::testParsePath_data()
{
    // Test data for SVG path specification as defined by the Path Data page
    // https://www.w3.org/TR/SVG/paths.html#PathData
    // This means :
    // - An extra pair of coordinates after a moveto are treated as lineto
    // - Parsing a path stops at the first invalid command reached, and the
    //   path before this command is considered valid.
    // - Extra parameters of a command are dropped and parsing stops.

    QTest::addColumn<QString>("pathString");
    QTest::addColumn<QPainterPath>("refPath");

    {
        QString path = "Q 10 20 30 40"_L1;
        QPainterPath refPath;
        refPath.quadTo(QPointF(10, 20), QPointF(30, 40));

        QTest::newRow("Q1") << path << refPath;
    }

    {
        QString path = "Q 10 20 30 40 50 60"_L1;
        QPainterPath refPath;
        refPath.quadTo(QPointF(10, 20), QPointF(30, 40));

        QTest::newRow("Q2") << path << refPath;
    }

    {
        QString path = "M 10 20 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(30, 40);

        QTest::newRow("M1") << path << refPath;
    }

    {
        QString path = "M 10 20 30 40 50"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(30, 40);

        QTest::newRow("M2") << path << refPath;
    }

    {
        QString path = "m 10 20 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(40, 60);

        QTest::newRow("m1") << path << refPath;
    }

    {
        QString path = "m 10 20 30 40 50"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(40, 60);

        QTest::newRow("m2") << path << refPath;
    }

    {
        QString path = "M 10 20 L 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(30, 40);

        QTest::newRow("L1") << path << refPath;
    }

    {
        QString path = "M 10 20 L 30 40 50"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(30, 40);

        QTest::newRow("L2") << path << refPath;
    }

    {
        QString path = "M 10 20 l 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(40, 60);

        QTest::newRow("l1") << path << refPath;
    }

    {
        QString path = "M 10 20 l 30 40 50"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(40, 60);

        QTest::newRow("l2") << path << refPath;
    }

    {
        QString path = "M 10 20 H 30"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(30, 20);

        QTest::newRow("H1") << path << refPath;
    }

    {
        QString path = "M 10 20 H 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(30, 20);
        refPath.lineTo(40, 20);

        QTest::newRow("H2") << path << refPath;
    }

    {
        QString path = "M 10 20 h 30"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(40, 20);

        QTest::newRow("h1") << path << refPath;
    }

    {
        QString path = "M 10 20 h 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(40, 20);
        refPath.lineTo(80, 20);

        QTest::newRow("h2") << path << refPath;
    }

    {
        QString path = "M 10 20 V 30"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(10, 30);

        QTest::newRow("V1") << path << refPath;
    }

    {
        QString path = "M 10 20 V 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(10, 30);
        refPath.lineTo(10, 40);

        QTest::newRow("V2") << path << refPath;
    }

    {
        QString path = "M 10 20 v 30"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(10, 50);

        QTest::newRow("v1") << path << refPath;
    }

    {
        QString path = "M 10 20 v 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(10, 50);
        refPath.lineTo(10, 90);

        QTest::newRow("v2") << path << refPath;
    }

    {
        QString path = "C 10 20 30 40 50 60"_L1;
        QPainterPath refPath;
        refPath.cubicTo(QPointF(10, 20), QPointF(30, 40), QPointF(50, 60));

        QTest::newRow("C1") << path << refPath;
    }

    {
        QString path = "C 10 20 30 40 50 60 70 80"_L1;
        QPainterPath refPath;
        refPath.cubicTo(QPointF(10, 20), QPointF(30, 40), QPointF(50, 60));

        QTest::newRow("C2") << path << refPath;
    }

    {
        QString path = "M 5 5 c 10 20 30 40 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.cubicTo(QPointF(15, 25), QPointF(35, 45), QPointF(55, 65));

        QTest::newRow("c1") << path << refPath;
    }

    {
        QString path = "M 5 5 c 10 20 30 40 50 60 70 80"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.cubicTo(QPointF(15, 25), QPointF(35, 45), QPointF(55, 65));

        QTest::newRow("c2") << path << refPath;
    }

    {
        QString path = "M 5 5 S 30 40 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.cubicTo(QPointF(5, 5), QPointF(30, 40), QPointF(50, 60));

        QTest::newRow("S1") << path << refPath;
    }

    {
        QString path = "M 5 5 S 30 40 50 60 70"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.cubicTo(QPointF(5, 5), QPointF(30, 40), QPointF(50, 60));

        QTest::newRow("S2") << path << refPath;
    }

    {
        QString path = "M 5 5 s 30 40 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.cubicTo(QPointF(5, 5), QPointF(35, 45), QPointF(55, 65));

        QTest::newRow("s1") << path << refPath;
    }

    {
        QString path = "M 5 5 s 30 40 50 60 70"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.cubicTo(QPointF(5, 5), QPointF(35, 45), QPointF(55, 65));

        QTest::newRow("s2") << path << refPath;
    }

    {
        QString path = "M 5 5 q 10 20 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.quadTo(QPointF(15, 25), QPointF(35, 45));

        QTest::newRow("q1") << path << refPath;
    }

    {
        QString path = "M 5 5 q 10 20 30 40 50"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.quadTo(QPointF(15, 25), QPointF(35, 45));

        QTest::newRow("q2") << path << refPath;
    }

    {
        QString path = "M 5 5 T 10 20"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.quadTo(QPointF(5, 5), QPointF(10, 20));

        QTest::newRow("T1") << path << refPath;
    }

    {
        QString path = "M 5 5 T 10 20 30"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.quadTo(QPointF(5, 5), QPointF(10, 20));

        QTest::newRow("T2") << path << refPath;
    }

    {
        QString path = "M 5 5 t 10 20"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.quadTo(QPointF(5, 5), QPointF(15, 25));

        QTest::newRow("t1") << path << refPath;
    }

    {
        QString path = "M 5 5 t 10 20 30"_L1;
        QPainterPath refPath;
        refPath.moveTo(5, 5);
        refPath.quadTo(QPointF(5, 5), QPointF(15, 25));

        QTest::newRow("t2") << path << refPath;
    }

    // A zero radius turns the arc into a straight line segment, see
    // https://www.w3.org/TR/SVG/paths.html#ArcOutOfRangeParameters
    {
        QString path = "M 10 20 A 0 0 0 0 0 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(30, 40);

        QTest::newRow("A1") << path << refPath;
    }

    {
        QString path = "M 10 20 A 0 0 0 0 0 30 40 50"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(30, 40);

        QTest::newRow("A2") << path << refPath;
    }

    {
        QString path = "M 10 20 a 0 0 0 0 0 30 40"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(40, 60);

        QTest::newRow("a1") << path << refPath;
    }

    {
        QString path = "M 10 20 a 0 0 0 0 0 30 40 50"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(40, 60);

        QTest::newRow("a2") << path << refPath;
    }

    {
        QString path = "M 10 20 L 30 40 Z"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(30, 40);
        refPath.closeSubpath();

        QTest::newRow("Z1") << path << refPath;
    }

    {
        QString path = "M 10 20 L 30 40 Z 50"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);
        refPath.lineTo(30, 40);
        refPath.closeSubpath();

        QTest::newRow("Z2") << path << refPath;
    }

    {
        QString path = "M 10"_L1;
        QPainterPath refPath;

        QTest::newRow("M3") << path << refPath;
    }

    {
        QString path = "m 10"_L1;
        QPainterPath refPath;

        QTest::newRow("m3") << path << refPath;
    }

    {
        QString path = "L 10"_L1;
        QPainterPath refPath;

        QTest::newRow("L3") << path << refPath;
    }

    {
        QString path = "C 10 20 30 40 50"_L1;
        QPainterPath refPath;

        QTest::newRow("C3") << path << refPath;
    }

    {
        QString path = "S 10 20 30"_L1;
        QPainterPath refPath;

        QTest::newRow("S3") << path << refPath;
    }

    {
        QString path = "Q 10 20 30"_L1;
        QPainterPath refPath;

        QTest::newRow("Q3") << path << refPath;
    }

    {
        QString path = "T 10"_L1;
        QPainterPath refPath;

        QTest::newRow("T3") << path << refPath;
    }

    {
        QString path = "A 0 0 0 0 0 30"_L1;
        QPainterPath refPath;

        QTest::newRow("A3") << path << refPath;
    }

    {
        QString path = "M 10 20 30 L 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);

        QTest::newRow("M4") << path << refPath;
    }

    {
        QString path = "M 10 20 L 30 L 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);

        QTest::newRow("L4") << path << refPath;
    }

    {
        QString path = "M 10 20 l 30 l 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);

        QTest::newRow("l4") << path << refPath;
    }

    {
        QString path = "M 0 0 L 10 10 30 L 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(0, 0);
        refPath.lineTo(10, 10);

        QTest::newRow("L5") << path << refPath;
    }

    {
        QString path = "M 10 20 C 1 2 3 4 5 L 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);

        QTest::newRow("C4") << path << refPath;
    }

    {
        QString path = "M 10 20 S 1 2 3 L 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);

        QTest::newRow("S4") << path << refPath;
    }

    {
        QString path = "M 10 20 Q 1 2 3 L 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);

        QTest::newRow("Q4") << path << refPath;
    }

    {
        QString path = "M 10 20 T 1 L 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);

        QTest::newRow("T4") << path << refPath;
    }

    {
        QString path = "M 10 20 A 0 0 0 0 0 30 L 50 60"_L1;
        QPainterPath refPath;
        refPath.moveTo(10, 20);

        QTest::newRow("A4") << path << refPath;
    }
}

void tst_QGuiSvg::testParsePath()
{
    QFETCH(QString, pathString);
    QFETCH(const QPainterPath, refPath);

    auto path = QGuiSvg::parsePath(pathString);

    QCOMPARE(path.value(), refPath);
}

QTEST_MAIN(tst_QGuiSvg)
#include "tst_qguisvg.moc"
