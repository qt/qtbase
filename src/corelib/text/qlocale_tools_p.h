// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLOCALE_TOOLS_P_H
#define QLOCALE_TOOLS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "qlocale_p.h"
#include "qstring.h"

QT_BEGIN_NAMESPACE

enum StrayCharacterMode {
    TrailingJunkProhibited,
    TrailingJunkAllowed,
    WhitespacesAllowed
};

template <typename T> struct QSimpleParsedNumber
{
    T result;
    // When used < 0, -used is how much was used, but it was an error.
    qsizetype used;
    bool ok() const { return used > 0; }
};

// API note: this function can't process a number with more than 2.1 billion digits
[[nodiscard]] QSimpleParsedNumber<double>
qt_asciiToDouble(const char *num, qsizetype numLen,
                 StrayCharacterMode strayCharMode = TrailingJunkProhibited);
void qt_doubleToAscii(double d, QLocaleData::DoubleForm form, int precision,
                      char *buf, qsizetype bufSize,
                      bool &sign, int &length, int &decpt);

[[nodiscard]] QString qulltoBasicLatin(qulonglong l, int base, bool negative);
[[nodiscard]] QString qulltoa(qulonglong l, int base, const QStringView zero);
[[nodiscard]] Q_CORE_EXPORT QString qdtoa(qreal d, int *decpt, int *sign);
[[nodiscard]] QString qdtoBasicLatin(double d, QLocaleData::DoubleForm form,
                                     int precision, bool uppercase);
[[nodiscard]] QByteArray qdtoAscii(double d, QLocaleData::DoubleForm form,
                                   int precision, bool uppercase);

[[nodiscard]] constexpr inline bool isZero(double d)
{
    return d == 0; // Amusingly, compilers do not grumble.
}

// Enough space for the digits before the decimal separator:
[[nodiscard]] inline int wholePartSpace(double d)
{
    Q_ASSERT(d >= 0); // caller should call qAbs() if needed
    // Optimize for numbers between -512k and 512k - otherwise, use the
    // maximum number of digits in the whole number part of a double:
    return d > (1 << 19) ? std::numeric_limits<double>::max_exponent10 + 1 : 6;
}

// Returns code-point of same kind (UCS2 or UCS4) as zero; digit is 0 through 9
template <typename UcsInt>
[[nodiscard]] inline UcsInt unicodeForDigit(uint digit, UcsInt zero)
{
    // Must match QLocaleData::numericToCLocale()'s digit-digestion.
    Q_ASSERT(digit < 10);
    if (!digit)
        return zero;

    // See QTBUG-85409: Suzhou's digits are U+3007, U+3021, ..., U+3029
    if (zero == u'\u3007')
        return u'\u3020' + digit;
    // In util/locale_database/ldml.py, LocaleScanner.numericData() asserts no
    // other number system in CLDR has discontinuous digits.

    return zero + digit;
}

[[nodiscard]] Q_CORE_EXPORT double qstrntod(const char *s00, qsizetype len,
                                            char const **se, bool *ok);
[[nodiscard]] inline double qstrtod(const char *s00, char const **se, bool *ok)
{
    qsizetype len = qsizetype(strlen(s00));
    return qstrntod(s00, len, se, ok);
}

[[nodiscard]] QSimpleParsedNumber<qlonglong> qstrntoll(const char *nptr, qsizetype size, int base);
[[nodiscard]] QSimpleParsedNumber<qulonglong> qstrntoull(const char *nptr, qsizetype size, int base);

QT_END_NAMESPACE

#endif
