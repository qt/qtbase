// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlocale_tools_p.h"
#include "qdoublescanprint_p.h"
#include "qlocale_p.h"
#include "qstring.h"

#include <private/qtools_p.h>
#include <private/qnumeric_p.h>

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <limits>
#include <charconv>

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

using namespace QtMiscUtils;

QT_CLOCALE_HOLDER

void qt_doubleToAscii(double d, QLocaleData::DoubleForm form, int precision,
                      char *buf, qsizetype bufSize,
                      bool &sign, int &length, int &decpt)
{
    if (bufSize == 0) {
        decpt = 0;
        sign = d < 0;
        length = 0;
        return;
    }

    // Detect special numbers (nan, +/-inf)
    // We cannot use the high-level API of libdouble-conversion as we need to
    // apply locale-specific formatting, such as decimal points, grouping
    // separators, etc. Because of this, we have to check for infinity and NaN
    // before calling DoubleToAscii.
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
    // libDoubleConversion is limited to 32-bit lengths. It's ok to cap the buffer size,
    // though, because the library will never write 2GiB of chars as output
    // (the length out-parameter is just an int, too).
    const auto boundedBufferSize = static_cast<int>((std::min)(bufSize, qsizetype(INT_MAX)));
    double_conversion::DoubleToStringConverter::DoubleToAscii(d, mode, precision, buf,
                                                              boundedBufferSize,
                                                              &sign, &length, &decpt);
#else // QT_NO_DOUBLECONVERSION || QT_BOOTSTRAPPED

    // Cut the precision at 999, to fit it into the format string. We can't get more than 17
    // significant digits, so anything after that is mostly noise. You do get closer to the "middle"
    // of the range covered by the given double with more digits, so to a degree it does make sense
    // to honor higher precisions. We define that at more than 999 digits that is not the case.
    if (precision > 999)
        precision = 999;
    else if (precision == QLocale::FloatingPointShortest)
        precision = std::numeric_limits<double>::max_digits10; // snprintf lacks "shortest" mode

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
        // <anything> '.' <precision> '\0'
        extraChars = wholePartSpace(d) + 2;
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
            auto r = qstrntoll(target.data() + eSign + 1, length - eSign - 1, 10);
            decpt = r.result + 1;
            Q_ASSERT(r.ok());
            Q_ASSERT(r.used + eSign + 1 <= length);
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

QSimpleParsedNumber<double> qt_asciiToDouble(const char *num, qsizetype numLen,
                                             StrayCharacterMode strayCharMode)
{
    if (numLen <= 0)
        return {};

    // We have to catch NaN before because we need NaN as marker for "garbage" in the
    // libdouble-conversion case and, in contrast to libdouble-conversion or sscanf, we don't allow
    // "-nan" or "+nan"
    if (char c = *num; numLen >= 3
            && (c == '-' || c == '+' || c == 'I' || c == 'i' || c == 'N' || c == 'n')) {
        bool negative = (c == '-');
        bool hasSign = negative || (c == '+');
        qptrdiff offset = 0;
        if (hasSign) {
            offset = 1;
            c = num[offset];
        }

        if (c > '9') {
            auto lowered = [](char c) {
                // this will mangle non-letters, but none can become a letter
                return c | 0x20;
            };

            // Found a non-digit, so this MUST be either "inf", "+inf", "-inf"
            // or "nan". Anything else is an invalid parse and we don't need to
            // feed it to the converter below.
            if (numLen != offset + 3)
                return {};

            c = lowered(c);
            char c2 = lowered(num[offset + 1]);
            char c3 = lowered(num[offset + 2]);
            if (c == 'i' && c2 == 'n' && c3 == 'f')
                return { negative ? -qt_inf() : qt_inf(), offset + 3 };
            else if (c == 'n' && c2 == 'a' && c3 == 'n' && !hasSign)
                return { qt_qnan(), 3 };
            return {};
        }
    }

    double d = 0.0;
    int processed;
#if !defined(QT_NO_DOUBLECONVERSION) && !defined(QT_BOOTSTRAPPED)
    int conv_flags = double_conversion::StringToDoubleConverter::NO_FLAGS;
    if (strayCharMode == TrailingJunkAllowed) {
        conv_flags = double_conversion::StringToDoubleConverter::ALLOW_TRAILING_JUNK;
    } else if (strayCharMode == WhitespacesAllowed) {
        conv_flags = double_conversion::StringToDoubleConverter::ALLOW_LEADING_SPACES
                | double_conversion::StringToDoubleConverter::ALLOW_TRAILING_SPACES;
    }
    double_conversion::StringToDoubleConverter conv(conv_flags, 0.0, qt_qnan(), nullptr, nullptr);
    if (int(numLen) != numLen) {
        // a number over 2 GB in length is silly, just assume it isn't valid
        return {};
    } else {
        d = conv.StringToDouble(num, int(numLen), &processed);
    }

    if (!qIsFinite(d)) {
        if (qIsNaN(d)) {
            // Garbage found. We don't accept it and return 0.
            return {};
        } else {
            // Overflow. That's not OK, but we still return infinity.
            return { d, -processed };
        }
    }
#else
    // ::digits10 is 19, but ::max() is 18'446'744'073'709'551'615ULL - go, figure...
    constexpr auto maxDigitsForULongLong = 1 + std::numeric_limits<unsigned long long>::digits10;
    // need to ensure that we don't read more than numLen of input:
    char fmt[1 + maxDigitsForULongLong + 4 + 1];
    qsnprintf(fmt, sizeof fmt, "%s%llu%s", "%", static_cast<unsigned long long>(numLen), "lf%n");

    if (qDoubleSscanf(num, QT_CLOCALE, fmt, &d, &processed) < 1)
        processed = 0;

    if ((strayCharMode == TrailingJunkProhibited && processed != numLen) || qIsNaN(d)) {
        // Implementation defined nan symbol or garbage found. We don't accept it.
        return {};
    }

    if (!qIsFinite(d)) {
        // Overflow. Check for implementation-defined infinity symbols and reject them.
        // We assume that any infinity symbol has to contain a character that cannot be part of a
        // "normal" number (that is 0-9, ., -, +, e).
        for (int i = 0; i < processed; ++i) {
            char c = num[i];
            if ((c < '0' || c > '9') && c != '.' && c != '-' && c != '+' && c != 'e' && c != 'E') {
                // Garbage found
                return {};
            }
        }
        return { d, -processed };
    }
#endif // !defined(QT_NO_DOUBLECONVERSION) && !defined(QT_BOOTSTRAPPED)

    // Otherwise we would have gotten NaN or sorted it out above.
    Q_ASSERT(strayCharMode == TrailingJunkAllowed || processed == numLen);

    // Check if underflow has occurred.
    if (isZero(d)) {
        for (int i = 0; i < processed; ++i) {
            if (num[i] >= '1' && num[i] <= '9') {
                // if a digit before any 'e' is not 0, then a non-zero number was intended.
                return {d, -processed};
            } else if (num[i] == 'e' || num[i] == 'E') {
                break;
            }
        }
    }
    return { d, processed };
}

/* Detect base if 0 and, if base is hex or bin, skip over 0x/0b prefixes */
static auto scanPrefix(const char *p, const char *stop, int base)
{
    struct R
    {
        const char *next;
        int base;
    };
    if (p < stop && isAsciiDigit(*p)) {
        if (*p == '0') {
            const char *x_or_b = p + 1;
            if (x_or_b < stop) {
                switch (*x_or_b) {
                case 'b':
                case 'B':
                    if (base == 0)
                        base = 2;
                    if (base == 2)
                        p += 2;
                    return R{p, base};
                case 'x':
                case 'X':
                    if (base == 0)
                        base = 16;
                    if (base == 16)
                        p += 2;
                    return R{p, base};
                }
            }
            if (base == 0)
                base = 8;
        } else if (base == 0) {
            base = 10;
        }
        Q_ASSERT(base);
    }
    return R{p, base};
}

static bool isDigitForBase(char d, int base)
{
    if (d < '0')
        return false;
    if (d - '0' < qMin(base, 10))
        return true;
    if (base > 10) {
        d |= 0x20; // tolower
        return d >= 'a' && d < 'a' + base - 10;
    }
    return false;
}

QSimpleParsedNumber<qulonglong> qstrntoull(const char *begin, qsizetype size, int base)
{
    const char *p = begin, *const stop = begin + size;
    while (p < stop && ascii_isspace(*p))
        ++p;
    unsigned long long result = 0;
    if (p >= stop || *p == '-')
        return { };
    const auto prefix = scanPrefix(*p == '+' ? p + 1 : p, stop, base);
    if (!prefix.base || prefix.next >= stop)
        return { };

    const auto res = std::from_chars(prefix.next, stop, result, prefix.base);
    if (res.ec != std::errc{})
        return { };
    return { result, res.ptr == prefix.next ? 0 : res.ptr - begin };
}

QSimpleParsedNumber<qlonglong> qstrntoll(const char *begin, qsizetype size, int base)
{
    const char *p = begin, *const stop = begin + size;
    while (p < stop && ascii_isspace(*p))
        ++p;
    // Frustratingly, std::from_chars() doesn't cope with a 0x prefix that might
    // be between the sign and digits, so we have to handle that for it, which
    // means we can't use its ability to read LLONG_MIN directly; see below.
    const bool negate = p < stop && *p == '-';
    if (negate || (p < stop && *p == '+'))
        ++p;

    const auto prefix = scanPrefix(p, stop, base);
    // Must check for digit, as from_chars() will accept a sign, which would be
    // a second sign, that we should reject.
    if (!prefix.base || prefix.next >= stop || !isDigitForBase(*prefix.next, prefix.base))
        return { };

    long long result = 0;
    auto res = std::from_chars(prefix.next, stop, result, prefix.base);
    if (negate && res.ec == std::errc::result_out_of_range) {
        // Maybe LLONG_MIN:
        unsigned long long check = 0;
        res = std::from_chars(prefix.next, stop, check, prefix.base);
        if (res.ec == std::errc{} && check + std::numeric_limits<long long>::min() == 0)
            return { std::numeric_limits<long long>::min(), res.ptr - begin };
        return { };
    }
    if (res.ec != std::errc{})
        return { };
    return { negate ? -result : result, res.ptr - begin };
}

template <typename Char>
static Q_ALWAYS_INLINE void qulltoString_helper(qulonglong number, int base, Char *&p)
{
    // Performance-optimized code. Compiler can generate faster code when base is known.
    switch (base) {
#define BIG_BASE_LOOP(b)                                  \
    do {                                                  \
        const int r = number % b;                         \
        *--p = Char((r < 10 ? '0' : 'a' - 10) + r); \
        number /= b;                                      \
    } while (number)
#ifndef __OPTIMIZE_SIZE__
#    define SMALL_BASE_LOOP(b)             \
        do {                               \
            *--p = Char('0' + number % b); \
            number /= b;                   \
        } while (number)

    case 2: SMALL_BASE_LOOP(2); break;
    case 8: SMALL_BASE_LOOP(8); break;
    case 10: SMALL_BASE_LOOP(10); break;
    case 16: BIG_BASE_LOOP(16); break;
#undef SMALL_BASE_LOOP
#endif
    default: BIG_BASE_LOOP(base); break;
#undef BIG_BASE_LOOP
    }
}

// This is technically "qulonglong to ascii", but that name's taken
QString qulltoBasicLatin(qulonglong number, int base, bool negative)
{
    if (number == 0)
        return QStringLiteral("0");
    // Length of MIN_LLONG with the sign in front is 65; we never need surrogate pairs.
    // We do not need a terminator.
    const unsigned maxlen = 65;
    static_assert(CHAR_BIT * sizeof(number) + 1 <= maxlen);
    char16_t buff[maxlen];
    char16_t *const end = buff + maxlen, *p = end;

    qulltoString_helper<char16_t>(number, base, p);
    if (negative)
        *--p = u'-';

    return QString(reinterpret_cast<QChar *>(p), end - p);
}

QString qulltoa(qulonglong number, int base, const QStringView zero)
{
    // Length of MAX_ULLONG in base 2 is 64; and we may need a surrogate pair
    // per digit. We do not need a terminator.
    const unsigned maxlen = 128;
    static_assert(CHAR_BIT * sizeof(number) <= maxlen);
    char16_t buff[maxlen];
    char16_t *const end = buff + maxlen, *p = end;

    if (base != 10 || zero == u"0") {
        qulltoString_helper<char16_t>(number, base, p);
    } else if (zero.size() && !zero.at(0).isSurrogate()) {
        const char16_t zeroUcs2 = zero.at(0).unicode();
        while (number != 0) {
            *(--p) = unicodeForDigit(number % base, zeroUcs2);

            number /= base;
        }
    } else if (zero.size() == 2 && zero.at(0).isHighSurrogate()) {
        const char32_t zeroUcs4 = QChar::surrogateToUcs4(zero.at(0), zero.at(1));
        while (number != 0) {
            const char32_t digit = unicodeForDigit(number % base, zeroUcs4);

            *(--p) = QChar::lowSurrogate(digit);
            *(--p) = QChar::highSurrogate(digit);

            number /= base;
        }
    } else { // zero should always be either a non-surrogate or a surrogate pair:
        Q_UNREACHABLE_RETURN(QString());
    }

    return QString(reinterpret_cast<QChar *>(p), end - p);
}

/*!
  \internal

  Converts the initial portion of the string pointed to by \a s00 to a double,
  using the 'C' locale. The function sets the pointer pointed to by \a se to
  point to the character past the last character converted.
 */
double qstrntod(const char *s00, qsizetype len, const char **se, bool *ok)
{
    auto r = qt_asciiToDouble(s00, len, TrailingJunkAllowed);
    if (se)
        *se = s00 + (r.used < 0 ? -r.used : r.used);
    if (ok)
        *ok = r.ok();
    return r.result;
}

QString qdtoa(qreal d, int *decpt, int *sign)
{
    bool nonNullSign = false;
    int nonNullDecpt = 0;
    int length = 0;

    // Some versions of libdouble-conversion like an extra digit, probably for '\0'
    constexpr qsizetype digits = std::numeric_limits<double>::max_digits10 + 1;
    char result[digits];
    qt_doubleToAscii(d, QLocaleData::DFSignificantDigits, QLocale::FloatingPointShortest,
                     result, digits, nonNullSign, length, nonNullDecpt);

    if (sign)
        *sign = nonNullSign ? 1 : 0;
    if (decpt)
        *decpt = nonNullDecpt;

    return QLatin1StringView(result, length);
}

static QLocaleData::DoubleForm resolveFormat(int precision, int decpt, qsizetype length)
{
    bool useDecimal;
    if (precision == QLocale::FloatingPointShortest) {
        // Find out which representation is shorter.
        // Set bias to everything added to exponent form but not
        // decimal, minus the converse.

        // Exponent adds separator, sign and two exponents:
        int bias = 2 + 2;
        if (length <= decpt && length > 1)
            ++bias;
        else if (length == 1 && decpt <= 0)
            --bias;

        // When 0 < decpt <= length, the forms have equal digit
        // counts, plus things bias has taken into account;
        // otherwise decimal form's digit count is right-padded with
        // zeros to decpt, when decpt is positive, otherwise it's
        // left-padded with 1 - decpt zeros.
        if (decpt <= 0)
            useDecimal = 1 - decpt <= bias;
        else if (decpt <= length)
            useDecimal = true;
        else
            useDecimal = decpt <= length + bias;
    } else {
        // X == decpt - 1, POSIX's P; -4 <= X < P iff -4 < decpt <= P
        Q_ASSERT(precision >= 0);
        useDecimal = decpt > -4 && decpt <= (precision ? precision : 1);
    }
    return useDecimal ? QLocaleData::DFDecimal : QLocaleData::DFExponent;
}

static constexpr int digits(int number)
{
    Q_ASSERT(number >= 0);
    if (Q_LIKELY(number < 1000))
        return number < 10 ? 1 : number < 100 ? 2 : 3;
    int i = 3;
    for (number /= 1000; number; number /= 10)
        ++i;
    return i;
}

// Used generically for both QString and QByteArray
template <typename T>
static T dtoString(double d, QLocaleData::DoubleForm form, int precision, bool uppercase)
{
    // Undocumented: aside from F.P.Shortest, precision < 0 is treated as
    // default, 6 - same as printf().
    if (precision != QLocale::FloatingPointShortest && precision < 0)
        precision = 6;

    using D = std::numeric_limits<double>;
    // 1 is for the null-terminator
    constexpr int MaxDigits = 1 + qMax(D::max_exponent10, D::digits10 - D::min_exponent10);

    // "maxDigits" above is a reasonable estimate, though we may need more due to extra precision
    int bufSize = 1;
    if (precision == QLocale::FloatingPointShortest)
        bufSize += D::max_digits10;
    else if (form == QLocaleData::DFDecimal && qIsFinite(d))
        bufSize += wholePartSpace(qAbs(d)) + precision;
    else // Add extra digit due to different interpretations of precision.
        bufSize += qMax(2, precision) + 1; // Must also be big enough for "nan" or "inf"

    // Reserve `MaxDigits` on the stack, which is a reasonable estimate;
    // but we may need more due to extra precision, which we cannot know at compile-time.
    QVarLengthArray<char, MaxDigits> buffer(bufSize);
    bool negative = false;
    int length = 0;
    int decpt = 0;
    qt_doubleToAscii(d, form, precision, buffer.data(), buffer.size(), negative, length, decpt);
    QLatin1StringView view(buffer.data(), length);
    const bool succinct = form == QLocaleData::DFSignificantDigits;
    qsizetype total = (negative ? 1 : 0) + length;
    if (qIsFinite(d)) {
        if (succinct)
            form = resolveFormat(precision, decpt, view.size());

        switch (form) {
        case QLocaleData::DFExponent:
            total += 3; // (.e+) The '.' may not be needed, but we would only overestimate by 1 char
            // Exponents: we guarantee at least 2
            total += std::max(2, digits(std::abs(decpt - 1)));
            // "length - 1" because one of the digits will always be before the decimal point
            if (int extraPrecision = precision - (length - 1); extraPrecision > 0 && !succinct)
                total += extraPrecision; // some requested zero-padding
            break;
        case QLocaleData::DFDecimal:
            if (decpt <= 0) // leading "0." and zeros
                total += 2 - decpt;
            else if (decpt < length) // just the dot
                total += 1;
            else // trailing zeros (and no dot, unless we require extra precision):
                total += decpt - length;

            if (precision > 0 && !succinct) {
                // May need trailing zeros to satisfy precision:
                if (decpt < length)
                    total += std::max(0, precision - length + decpt);
                else // and a dot to separate them:
                    total += 1 + precision;
            }
            break;
        case QLocaleData::DFSignificantDigits:
            Q_UNREACHABLE(); // Handled earlier
        }
    }

    constexpr bool IsQString = std::is_same_v<T, QString>;
    using Char = std::conditional_t<IsQString, char16_t, char>;

    T result;
    result.reserve(total);

    if (negative && !isZero(d)) // We don't return "-0"
        result.append(Char('-'));
    if (!qIsFinite(d)) {
        result.append(view);
        if (uppercase)
            result = std::move(result).toUpper();
    } else {
        switch (form) {
        case QLocaleData::DFExponent: {
            result.append(view.first(1));
            view = view.sliced(1);
            if (!view.isEmpty() || (!succinct && precision > 0)) {
                result.append(Char('.'));
                result.append(view);
                if (qsizetype pad = precision - view.size(); !succinct && pad > 0) {
                    for (int i = 0; i < pad; ++i)
                        result.append(Char('0'));
                }
            }
            int exponent = decpt - 1;
            result.append(Char(uppercase ? 'E' : 'e'));
            result.append(Char(exponent < 0 ? '-' : '+'));
            exponent = std::abs(exponent);
            Q_ASSUME(exponent <= D::max_exponent10 + D::max_digits10);
            int exponentDigits = digits(exponent);
            // C's printf guarantees a two-digit exponent, and so do we:
            if (exponentDigits == 1)
                result.append(Char('0'));
            result.resize(result.size() + exponentDigits);
            auto location = reinterpret_cast<Char *>(result.end());
            qulltoString_helper<Char>(exponent, 10, location);
            break;
        }
        case QLocaleData::DFDecimal:
            if (decpt < 0) {
                if constexpr (IsQString)
                    result.append(u"0.0");
                else
                    result.append("0.0");
                while (++decpt < 0)
                    result.append(Char('0'));
                result.append(view);
                if (!succinct) {
                    auto numDecimals = result.size() - 2 - (negative ? 1 : 0);
                    for (qsizetype i = numDecimals; i < precision; ++i)
                        result.append(Char('0'));
                }
            } else {
                if (decpt > view.size()) {
                    result.append(view);
                    const int sign = negative ? 1 : 0;
                    while (result.size() - sign < decpt)
                        result.append(Char('0'));
                    view = {};
                } else if (decpt) {
                    result.append(view.first(decpt));
                    view = view.sliced(decpt);
                } else {
                    result.append(Char('0'));
                }
                if (!view.isEmpty() || (!succinct && view.size() < precision)) {
                    result.append(Char('.'));
                    result.append(view);
                    if (!succinct) {
                        for (qsizetype i = view.size(); i < precision; ++i)
                            result.append(Char('0'));
                    }
                }
            }
            break;
        case QLocaleData::DFSignificantDigits:
            Q_UNREACHABLE(); // taken care of earlier
            break;
        }
    }
    Q_ASSERT(total >= result.size()); // No reallocations are needed
    return result;
}

QString qdtoBasicLatin(double d, QLocaleData::DoubleForm form, int precision, bool uppercase)
{
    return dtoString<QString>(d, form, precision, uppercase);
}

QByteArray qdtoAscii(double d, QLocaleData::DoubleForm form, int precision, bool uppercase)
{
    return dtoString<QByteArray>(d, form, precision, uppercase);
}

QT_END_NAMESPACE
