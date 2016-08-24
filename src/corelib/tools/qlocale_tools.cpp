/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlocale_tools_p.h"
#include "qdoublescanprint_p.h"
#include "qlocale_p.h"
#include "qstring.h"

#include <private/qnumeric_p.h>

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#if defined(Q_OS_LINUX) && !defined(__UCLIBC__)
#    include <fenv.h>
#endif

// Sizes as defined by the ISO C99 standard - fallback
#ifndef LLONG_MAX
#   define LLONG_MAX Q_INT64_C(0x7fffffffffffffff)
#endif
#ifndef LLONG_MIN
#   define LLONG_MIN (-LLONG_MAX - Q_INT64_C(1))
#endif
#ifndef ULLONG_MAX
#   define ULLONG_MAX Q_UINT64_C(0xffffffffffffffff)
#endif

QT_BEGIN_NAMESPACE

#include "../../3rdparty/freebsd/strtoull.c"
#include "../../3rdparty/freebsd/strtoll.c"

QT_CLOCALE_HOLDER

void doubleToAscii(double d, QLocaleData::DoubleForm form, int precision, char *buf, int bufSize,
                   bool &sign, int &length, int &decpt)
{
    if (bufSize == 0) {
        decpt = 0;
        sign = d < 0;
        length = 0;
        return;
    }

    // Detect special numbers (nan, +/-inf)
    // We cannot use the high-level API of libdouble-conversion as we need to apply locale-specific
    // formatting, such as decimal points, thousands-separators, etc. Because of this, we have to
    // check for infinity and NaN before calling DoubleToAscii.
    if (qt_is_inf(d)) {
        sign = d < 0;
        if (bufSize >= 3) {
            buf[0] = 'i';
            buf[1] = 'n';
            buf[2] = 'f';
            length = 3;
        } else {
            length = 0;
        }
        return;
    } else if (qt_is_nan(d)) {
        if (bufSize >= 3) {
            buf[0] = 'n';
            buf[1] = 'a';
            buf[2] = 'n';
            length = 3;
        } else {
            length = 0;
        }
        return;
    }

    if (form == QLocaleData::DFSignificantDigits && precision == 0)
        precision = 1; // 0 significant digits is silently converted to 1

#if !defined(QT_NO_DOUBLECONVERSION) && !defined(QT_BOOTSTRAPPED)
    // one digit before the decimal dot, counts as significant digit for DoubleToStringConverter
    if (form == QLocaleData::DFExponent && precision >= 0)
        ++precision;

    double_conversion::DoubleToStringConverter::DtoaMode mode;
    if (precision == QLocale::FloatingPointShortest) {
        mode = double_conversion::DoubleToStringConverter::SHORTEST;
    } else if (form == QLocaleData::DFSignificantDigits || form == QLocaleData::DFExponent) {
        mode = double_conversion::DoubleToStringConverter::PRECISION;
    } else {
        mode = double_conversion::DoubleToStringConverter::FIXED;
    }
    double_conversion::DoubleToStringConverter::DoubleToAscii(d, mode, precision, buf, bufSize,
                                                              &sign, &length, &decpt);
#else // QT_NO_DOUBLECONVERSION || QT_BOOTSTRAPPED

    // Cut the precision at 999, to fit it into the format string. We can't get more than 17
    // significant digits, so anything after that is mostly noise. You do get closer to the "middle"
    // of the range covered by the given double with more digits, so to a degree it does make sense
    // to honor higher precisions. We define that at more than 999 digits that is not the case.
    if (precision > 999)
        precision = 999;
    else if (precision == QLocale::FloatingPointShortest)
        precision = QLocaleData::DoubleMaxSignificant; // "shortest" mode not supported by snprintf

    if (isZero(d)) {
        // Negative zero is expected as simple "0", not "-0". We cannot do d < 0, though.
        sign = false;
        buf[0] = '0';
        length = 1;
        decpt = 1;
        return;
    } else if (d < 0) {
        sign = true;
        d = -d;
    } else {
        sign = false;
    }

    const int formatLength = 7; // '%', '.', 3 digits precision, 'f', '\0'
    char format[formatLength];
    format[formatLength - 1] = '\0';
    format[0] = '%';
    format[1] = '.';
    format[2] = char((precision / 100) % 10) + '0';
    format[3] = char((precision / 10) % 10)  + '0';
    format[4] = char(precision % 10)  + '0';
    int extraChars;
    switch (form) {
    case QLocaleData::DFDecimal:
        format[formatLength - 2] = 'f';
        // <anything> '.' <precision> '\0' - optimize for numbers smaller than 512k
        extraChars = (d > (1 << 19) ? QLocaleData::DoubleMaxDigitsBeforeDecimal : 6) + 2;
        break;
    case QLocaleData::DFExponent:
        format[formatLength - 2] = 'e';
        // '.', 'e', '-', <exponent> '\0'
        extraChars = 7;
        break;
    case QLocaleData::DFSignificantDigits:
        format[formatLength - 2] = 'g';

        // either the same as in the 'e' case, or '.' and '\0'
        // precision covers part before '.'
        extraChars = 7;
        break;
    default:
        Q_UNREACHABLE();
    }

    QVarLengthArray<char> target(precision + extraChars);

    length = qDoubleSnprintf(target.data(), target.size(), QT_CLOCALE, format, d);
    int firstSignificant = 0;
    int decptInTarget = length;

    // Find the first significant digit (not 0), and note any '.' we encounter.
    // There is no '-' at the front of target because we made sure d > 0 above.
    while (firstSignificant < length) {
        if (target[firstSignificant] == '.')
            decptInTarget = firstSignificant;
        else if (target[firstSignificant] != '0')
            break;
        ++firstSignificant;
    }

    // If no '.' found so far, search the rest of the target buffer for it.
    if (decptInTarget == length)
        decptInTarget = std::find(target.data() + firstSignificant, target.data() + length, '.') -
                target.data();

    int eSign = length;
    if (form != QLocaleData::DFDecimal) {
        // In 'e' or 'g' form, look for the 'e'.
        eSign = std::find(target.data() + firstSignificant, target.data() + length, 'e') -
                target.data();

        if (eSign < length) {
            // If 'e' is found, the final decimal point is determined by the number after 'e'.
            // Mind that the final decimal point, decpt, is the offset of the decimal point from the
            // start of the resulting string in buf. It may be negative or larger than bufSize, in
            // which case the missing digits are zeroes. In the 'e' case decptInTarget is always 1,
            // as variants of snprintf always generate numbers with one digit before the '.' then.
            // This is why the final decimal point is offset by 1, relative to the number after 'e'.
            bool ok;
            const char *endptr;
            decpt = qstrtoll(target.data() + eSign + 1, &endptr, 10, &ok) + 1;
            Q_ASSERT(ok);
            Q_ASSERT(endptr - target.data() <= length);
        } else {
            // No 'e' found, so it's the 'f' form. Variants of snprintf generate numbers with
            // potentially multiple digits before the '.', but without decimal exponent then. So we
            // get the final decimal point from the position of the '.'. The '.' itself takes up one
            // character. We adjust by 1 below if that gets in the way.
            decpt = decptInTarget - firstSignificant;
        }
    } else {
        // In 'f' form, there can not be an 'e', so it's enough to look for the '.'
        // (and possibly adjust by 1 below)
        decpt = decptInTarget - firstSignificant;
    }

    // Move the actual digits from the snprintf target to the actual buffer.
    if (decptInTarget > firstSignificant) {
        // First move the digits before the '.', if any
        int lengthBeforeDecpt = decptInTarget - firstSignificant;
        memcpy(buf, target.data() + firstSignificant, qMin(lengthBeforeDecpt, bufSize));
        if (eSign > decptInTarget && lengthBeforeDecpt < bufSize) {
            // Then move any remaining digits, until 'e'
            memcpy(buf + lengthBeforeDecpt, target.data() + decptInTarget + 1,
                   qMin(eSign - decptInTarget - 1, bufSize - lengthBeforeDecpt));
            // The final length of the output is the distance between the first significant digit
            // and 'e' minus 1, for the '.', except if the buffer is smaller.
            length = qMin(eSign - firstSignificant - 1, bufSize);
        } else {
            // 'e' was before the decpt or things didn't fit. Don't subtract the '.' from the length.
            length = qMin(eSign - firstSignificant, bufSize);
        }
    } else {
        if (eSign > firstSignificant) {
            // If there are any significant digits at all, they are all after the '.' now.
            // Just copy them straight away.
            memcpy(buf, target.data() + firstSignificant, qMin(eSign - firstSignificant, bufSize));

            // The decimal point was before the first significant digit, so we were one off above.
            // Consider 0.1 - buf will be just '1', and decpt should be 0. But
            // "decptInTarget - firstSignificant" will yield -1.
            ++decpt;
            length = qMin(eSign - firstSignificant, bufSize);
        } else {
            // No significant digits means the number is just 0.
            buf[0] = '0';
            length = 1;
            decpt = 1;
        }
    }
#endif // QT_NO_DOUBLECONVERSION || QT_BOOTSTRAPPED
    while (length > 1 && buf[length - 1] == '0') // drop trailing zeroes
        --length;
}

double asciiToDouble(const char *num, int numLen, bool &ok, int &processed,
                     TrailingJunkMode trailingJunkMode)
{
    if (*num == '\0') {
        ok = false;
        processed = 0;
        return 0.0;
    }

    ok = true;

    // We have to catch NaN before because we need NaN as marker for "garbage" in the
    // libdouble-conversion case and, in contrast to libdouble-conversion or sscanf, we don't allow
    // "-nan" or "+nan"
    if (qstrcmp(num, "nan") == 0) {
        processed = 3;
        return qt_snan();
    } else if ((num[0] == '-' || num[0] == '+') && qstrcmp(num + 1, "nan") == 0) {
        processed = 0;
        ok = false;
        return 0.0;
    }

    // Infinity values are implementation defined in the sscanf case. In the libdouble-conversion
    // case we need infinity as overflow marker.
    if (qstrcmp(num, "+inf") == 0) {
        processed = 4;
        return qt_inf();
    } else if (qstrcmp(num, "inf") == 0) {
        processed = 3;
        return qt_inf();
    } else if (qstrcmp(num, "-inf") == 0) {
        processed = 4;
        return -qt_inf();
    }

    double d = 0.0;
#if !defined(QT_NO_DOUBLECONVERSION) && !defined(QT_BOOTSTRAPPED)
    int conv_flags = (trailingJunkMode == TrailingJunkAllowed) ?
                double_conversion::StringToDoubleConverter::ALLOW_TRAILING_JUNK :
                double_conversion::StringToDoubleConverter::NO_FLAGS;
    double_conversion::StringToDoubleConverter conv(conv_flags, 0.0, qt_snan(), 0, 0);
    d = conv.StringToDouble(num, numLen, &processed);

    if (!qIsFinite(d)) {
        ok = false;
        if (qIsNaN(d)) {
            // Garbage found. We don't accept it and return 0.
            processed = 0;
            return 0.0;
        } else {
            // Overflow. That's not OK, but we still return infinity.
            return d;
        }
    }
#else
    if (qDoubleSscanf(num, QT_CLOCALE, "%lf%n", &d, &processed) < 1)
        processed = 0;

    if ((trailingJunkMode == TrailingJunkProhibited && processed != numLen) || qIsNaN(d)) {
        // Implementation defined nan symbol or garbage found. We don't accept it.
        processed = 0;
        ok = false;
        return 0.0;
    }

    if (!qIsFinite(d)) {
        // Overflow. Check for implementation-defined infinity symbols and reject them.
        // We assume that any infinity symbol has to contain a character that cannot be part of a
        // "normal" number (that is 0-9, ., -, +, e).
        ok = false;
        for (int i = 0; i < processed; ++i) {
            char c = num[i];
            if ((c < '0' || c > '9') && c != '.' && c != '-' && c != '+' && c != 'e') {
                // Garbage found
                processed = 0;
                return 0.0;
            }
        }
        return d;
    }
#endif // !defined(QT_NO_DOUBLECONVERSION) && !defined(QT_BOOTSTRAPPED)

    // Otherwise we would have gotten NaN or sorted it out above.
    Q_ASSERT(trailingJunkMode == TrailingJunkAllowed || processed == numLen);

    // Check if underflow has occurred.
    if (isZero(d)) {
        for (int i = 0; i < processed; ++i) {
            if (num[i] >= '1' && num[i] <= '9') {
                // if a digit before any 'e' is not 0, then a non-zero number was intended.
                ok = false;
                return 0.0;
            } else if (num[i] == 'e') {
                break;
            }
        }
    }
    return d;
}

unsigned long long
qstrtoull(const char * nptr, const char **endptr, int base, bool *ok)
{
    // strtoull accepts negative numbers. We don't.
    // Use a different variable so we pass the original nptr to strtoul
    // (we need that so endptr may be nptr in case of failure)
    const char *begin = nptr;
    while (ascii_isspace(*begin))
        ++begin;
    if (*begin == '-') {
        *ok = false;
        return 0;
    }

    *ok = true;
    errno = 0;
    char *endptr2 = 0;
    unsigned long long result = qt_strtoull(nptr, &endptr2, base);
    if (endptr)
        *endptr = endptr2;
    if ((result == 0 || result == std::numeric_limits<unsigned long long>::max())
            && (errno || endptr2 == nptr)) {
        *ok = false;
        return 0;
    }
    return result;
}

long long
qstrtoll(const char * nptr, const char **endptr, int base, bool *ok)
{
    *ok = true;
    errno = 0;
    char *endptr2 = 0;
    long long result = qt_strtoll(nptr, &endptr2, base);
    if (endptr)
        *endptr = endptr2;
    if ((result == 0 || result == std::numeric_limits<long long>::min()
         || result == std::numeric_limits<long long>::max())
            && (errno || nptr == endptr2)) {
        *ok = false;
        return 0;
    }
    return result;
}

QString qulltoa(qulonglong l, int base, const QChar _zero)
{
    ushort buff[65]; // length of MAX_ULLONG in base 2
    ushort *p = buff + 65;

    if (base != 10 || _zero.unicode() == '0') {
        while (l != 0) {
            int c = l % base;

            --p;

            if (c < 10)
                *p = '0' + c;
            else
                *p = c - 10 + 'a';

            l /= base;
        }
    }
    else {
        while (l != 0) {
            int c = l % base;

            *(--p) = _zero.unicode() + c;

            l /= base;
        }
    }

    return QString(reinterpret_cast<QChar *>(p), 65 - (p - buff));
}

QString qlltoa(qlonglong l, int base, const QChar zero)
{
    return qulltoa(l < 0 ? -l : l, base, zero);
}

QString &decimalForm(QChar zero, QChar decimal, QChar group,
                     QString &digits, int decpt, int precision,
                     PrecisionMode pm,
                     bool always_show_decpt,
                     bool thousands_group)
{
    if (decpt < 0) {
        for (int i = 0; i < -decpt; ++i)
            digits.prepend(zero);
        decpt = 0;
    }
    else if (decpt > digits.length()) {
        for (int i = digits.length(); i < decpt; ++i)
            digits.append(zero);
    }

    if (pm == PMDecimalDigits) {
        uint decimal_digits = digits.length() - decpt;
        for (int i = decimal_digits; i < precision; ++i)
            digits.append(zero);
    }
    else if (pm == PMSignificantDigits) {
        for (int i = digits.length(); i < precision; ++i)
            digits.append(zero);
    }
    else { // pm == PMChopTrailingZeros
    }

    if (always_show_decpt || decpt < digits.length())
        digits.insert(decpt, decimal);

    if (thousands_group) {
        for (int i = decpt - 3; i > 0; i -= 3)
            digits.insert(i, group);
    }

    if (decpt == 0)
        digits.prepend(zero);

    return digits;
}

QString &exponentForm(QChar zero, QChar decimal, QChar exponential,
                      QChar group, QChar plus, QChar minus,
                      QString &digits, int decpt, int precision,
                      PrecisionMode pm,
                      bool always_show_decpt,
                      bool leading_zero_in_exponent)
{
    int exp = decpt - 1;

    if (pm == PMDecimalDigits) {
        for (int i = digits.length(); i < precision + 1; ++i)
            digits.append(zero);
    }
    else if (pm == PMSignificantDigits) {
        for (int i = digits.length(); i < precision; ++i)
            digits.append(zero);
    }
    else { // pm == PMChopTrailingZeros
    }

    if (always_show_decpt || digits.length() > 1)
        digits.insert(1, decimal);

    digits.append(exponential);
    digits.append(QLocaleData::longLongToString(zero, group, plus, minus,
                   exp, leading_zero_in_exponent ? 2 : 1, 10, -1, QLocaleData::AlwaysShowSign));

    return digits;
}

double qstrtod(const char *s00, const char **se, bool *ok)
{
    const int len = static_cast<int>(strlen(s00));
    Q_ASSERT(len >= 0);
    return qstrntod(s00, len, se, ok);
}

/*!
  \internal

  Converts the initial portion of the string pointed to by \a s00 to a double, using the 'C' locale.
 */
double qstrntod(const char *s00, int len, const char **se, bool *ok)
{
    int processed = 0;
    bool nonNullOk = false;
    double d = asciiToDouble(s00, len, nonNullOk, processed, TrailingJunkAllowed);
    if (se)
        *se = s00 + processed;
    if (ok)
        *ok = nonNullOk;
    return d;
}

QString qdtoa(qreal d, int *decpt, int *sign)
{
    bool nonNullSign = false;
    int nonNullDecpt = 0;
    int length = 0;

    // Some versions of libdouble-conversion like an extra digit, probably for '\0'
    char result[QLocaleData::DoubleMaxSignificant + 1];
    doubleToAscii(d, QLocaleData::DFSignificantDigits, QLocale::FloatingPointShortest, result,
                  QLocaleData::DoubleMaxSignificant + 1, nonNullSign, length, nonNullDecpt);

    if (sign)
        *sign = nonNullSign ? 1 : 0;
    if (decpt)
        *decpt = nonNullDecpt;

    return QLatin1String(result, length);
}

QT_END_NAMESPACE
