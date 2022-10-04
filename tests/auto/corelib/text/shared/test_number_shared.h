// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <limits>
#include <QtCore/qstring.h>
#include <QtCore/qlocale.h>

struct NumberDoubleTestData
{
    double d;
    char f;
    int p;
    QLatin1String expected;
    QLatin1String optTitle = {}; // optional
    // Tests with same (f, p, expected) should use optTitle to avoid duplicate data tags.
};

template<typename Fun>
inline void add_number_double_shared_data(Fun addTestRowFunction)
{
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr double inf = std::numeric_limits<double>::infinity();
    const static NumberDoubleTestData data[] {
        { 0.0, 'f', 0, QLatin1String("0") },
        { 0.0, 'e', 0, QLatin1String("0e+00") },
        { 0.0, 'e', 1, QLatin1String("0.0e+00") },
        { 0.0001, 'f', 0, QLatin1String("0"), QLatin1String("0(.0001)") },
        { 0.1234, 'f', 5, QLatin1String("0.12340") },
        { -0.1234, 'f', 5, QLatin1String("-0.12340") },
        { 0.0000000314, 'f', 12, QLatin1String("0.000000031400") },
        { -0.0000000314, 'f', 12, QLatin1String("-0.000000031400") },
        { -100000, 'f', 15, QLatin1String("-100000.000000000000000") },
        { 0.5 + qSqrt(1.25), 'f', 15, QLatin1String("1.618033988749895") },
        { 0.5 + qSqrt(1.25), 'e', 15, QLatin1String("1.618033988749895e+00") },
        { 1.7976931348623157e+308, 'f', 120,
          QLatin1String(
                  "17976931348623157081452742373170435679807056752584499659891747680315726078002853"
                  "87605895586327668781715404589535143824642343213268894641827684675467035375169860"
                  "49910576551282076245490090389328944075868508455133942304583236903222948165808559"
                  "332123348274797826204144723168738177180919299881250404026184124858368."
                  "00000000000000000000000000000000000000000000000000000000000000000000000000000000"
                  "0000000000000000000000000000000000000000"),
          QLatin1String("Big number, high precision") },
        { 1.0, 'f', 350,
          QLatin1String(
                  "1.0000000000000000000000000000000000000000000000000000000000000000000000000"
                  "00000000000000000000000000000000000000000000000000000000000000000000000000"
                  "00000000000000000000000000000000000000000000000000000000000000000000000000"
                  "00000000000000000000000000000000000000000000000000000000000000000000000000"
                  "0000000000000000000000000000000000000000000000000000000"),
          QLatin1String("Very high precision 1") },
        { 1.0e-308, 'f', 350,
          QLatin1String("0."
                        "00000000000000000000000000000000000000000000000000000000000000000000000000"
                        "00000000000000000000000000000000000000000000000000000000000000000000000000"
                        "00000000000000000000000000000000000000000000000000000000000000000000000000"
                        "00000000000000000000000000000000000000000000000000000000000000000000000000"
                        "000000000000999999999999999909326625337248461995470489"),
          QLatin1String("Very small number, very high precision") },
        { std::numeric_limits<double>::epsilon(), 'g', 10, QLatin1String("2.220446049e-16") },
        { 0.0001, 'e', 1, QLatin1String("1.0e-04") },
        { 1e8, 'e', 1, QLatin1String("1.0e+08") },
        { -1e8, 'e', 1, QLatin1String("-1.0e+08") },
        { 1.1e-8, 'e', 6, QLatin1String("1.100000e-08") },
        { -1.1e-8, 'e', 6, QLatin1String("-1.100000e-08") },
        { 1.1e+8, 'e', 6, QLatin1String("1.100000e+08") },
        { -1.1e+8, 'e', 6, QLatin1String("-1.100000e+08") },
        { 100000, 'f', 0, QLatin1String("100000") },
        // Increasingly small fraction, test how/when 'g' switches to scientific notation:
        { 0.001, 'g', 6, QLatin1String("0.001") },
        { 0.0001, 'g', 6, QLatin1String("0.0001") },
        { 0.00001, 'g', 6, QLatin1String("1e-05") },
        { 0.000001, 'g', 6, QLatin1String("1e-06") },
        // FloatingPointShortest is relied upon by various facilities:
        { 1.0, 'g', QLocale::FloatingPointShortest, QLatin1String("1") },
        { 0.01, 'g', QLocale::FloatingPointShortest, QLatin1String("0.01") },
        { 123.456, 'g', QLocale::FloatingPointShortest, QLatin1String("123.456") },
        { 12.12, 'g', QLocale::FloatingPointShortest, QLatin1String("12.12") },
        { 0.000001, 'g', QLocale::FloatingPointShortest, QLatin1String("1e-06") },
        { 100000, 'g', QLocale::FloatingPointShortest, QLatin1String("1e+05") },
        // inf and nan testing:
        { inf, 'g', QLocale::FloatingPointShortest, QLatin1String("inf") },
        { -inf, 'g', QLocale::FloatingPointShortest, QLatin1String("-inf") },
        { nan, 'g', QLocale::FloatingPointShortest, QLatin1String("nan") },
        { inf, 'f', 15, QLatin1String("inf") },
        { -inf, 'f', 15, QLatin1String("-inf") },
        { nan, 'f', 15, QLatin1String("nan") },
        { inf, 'e', 2, QLatin1String("inf") },
        { -inf, 'e', 2, QLatin1String("-inf") },
        { nan, 'e', 2, QLatin1String("nan") },
        // Negative precision (except QLocale::F.P.Shortest) defaults to 6:
        { 0.001, 'f', -50, QLatin1String("0.001000") },
        { 0.0001, 'f', -62, QLatin1String("0.000100") },
        { 0.00001, 'f', -11, QLatin1String("0.000010") },
        { 0.000001, 'f', -41, QLatin1String("0.000001") },
        { 0.0000001, 'f', -21, QLatin1String("0.000000") },
        // Some rounding tests
        { 10.5, 'f', 0, QLatin1String("11") },
        { 12.05, 'f', 1, QLatin1String("12.1") },
        { 14.500000000000001, 'f', 0, QLatin1String("15") },
        { 16.5000000000000001, 'f', 0, QLatin1String("17") },
    };
    for (auto datum : data)
        addTestRowFunction(datum);
}
