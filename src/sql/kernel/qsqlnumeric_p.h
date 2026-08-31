// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSQLNUMERIC_P_H
#define QSQLNUMERIC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtSql/private/qtsqlglobal_p.h>
#include <QtCore/private/qlocale_tools_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qbytearrayview.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>

#include <limits>
#include <type_traits>

/*
    Scaled-integer numeric helpers for SQL drivers.

    Firebird and InterBase store NUMERIC/DECIMAL as an integer of 16 to 128
    bits together with a scale: value = stored * 10^scale (scale <= 0). These
    helpers convert such integers to and from QVariant according to
    QSql::NumericalPrecisionPolicy and parse/format the exact decimal text that
    HighPrecision and the bind path use — without ever going through double,
    which would silently drop digits above 2^53.

    128-bit support piggybacks on QtCore's private qinternalint128 alias
    (qint128 where the compiler has __int128, std::_Signed128 on MSVC >= 19.30).
    QSQL_HAS_INT128 tells a driver whether that type exists; the driver combines
    it with its own client-library condition. Two rules keep the 128-bit
    instantiations correct on MSVC, whose std::_Signed128 converts to narrower
    integers by keeping the low bits and has no conversion to double: never
    narrow without sqlFitsIn(), and never negate (format, then strip the sign).
*/
#if defined(QT_SUPPORTS_INT128) || defined(QT_USE_MSVC_INT128)
#  define QSQL_HAS_INT128
#endif

QT_BEGIN_NAMESPACE

namespace QtPrivate {

#ifdef QSQL_HAS_INT128
template <typename T>
inline constexpr bool sqlIsInt128 = std::is_same_v<T, qinternalint128>;
#else
template <typename T>
inline constexpr bool sqlIsInt128 = false;
#endif

/*
    10^n as T (n >= 0). Scale magnitudes are bounded by the column's precision
    (<= 38 for INT128, <= 18 for INT64), so this never overflows for the type
    backing the column it is used with.
*/
template <typename T>
T sqlIntPow10(int n)
{
    T r = 1;
    for (int i = 0; i < n; ++i)
        r *= 10;
    return r;
}

/*
    Decimal text of an integer of any width. The 128-bit type has no
    QString::number overload; qint128toBasicLatin formats it (correctly for the
    minimum value, too).
*/
template <typename T>
QString sqlIntegerToString(T val)
{
#ifdef QSQL_HAS_INT128
    if constexpr (sqlIsInt128<T>)
        return qint128toBasicLatin(val);
    else
#endif
        return QString::number(static_cast<qint64>(val));
}

/*
    True if val is representable as Narrow. The comparison happens in T, so the
    result is exact even when a cast would silently truncate.
*/
template <typename Narrow, typename T>
bool sqlFitsIn(T val)
{
    return val >= T((std::numeric_limits<Narrow>::min)())
        && val <= T((std::numeric_limits<Narrow>::max)());
}

/*
    Format a scaled integer as an exact decimal string.
    e.g. val=12345, scale=-2 -> "123.45"; val=12, scale=2 -> "1200"
    The integer is formatted once and the zero padding and the decimal point
    are inserted in place behind the sign, which never moves: no negation (so
    the minimum of T formats correctly) and no left/mid/concatenation chain on
    this per-row path.
*/
template <typename T>
QString sqlScaledToString(T val, int scale)
{
    QString number = sqlIntegerToString(val);
    if (scale == 0)
        return number;
    if (scale > 0) {
        // value = stored * 10^scale -> an integer with `scale` trailing zeros
        // (but 0 * 10^n is "0", not "000")
        if (val != 0)
            number.resize(number.size() + scale, u'0');
        return number;
    }
    const qsizetype signLen = val < 0 ? 1 : 0;
    const qsizetype absScale = -scale;
    const qsizetype digits = number.size() - signLen;
    // Fewer digits than decimals: "5" at scale -3 needs "0.005"
    const qsizetype leadingZeros = absScale >= digits ? absScale - digits + 1 : 0;
    number.reserve(number.size() + leadingZeros + 1);
    if (leadingZeros)
        number.insert(signLen, QString(leadingZeros, u'0'));
    number.insert(number.size() - absScale, u'.');
    return number;
}

/*
    The double nearest to val * 10^scale, for every width of T.

    When val is below 2^53 in magnitude and |scale| <= 22, both operands are
    exact doubles (10^k is exactly representable up to 10^22), so the single
    IEEE division or multiplication is correctly rounded. Multiplying by an
    inexact 10^-k, as a double-based conversion naively would, is not:
    998 * 0.1 gives 99.80000000000001. Anything wider goes through the exact
    decimal text, which toDouble() rounds correctly as well (and which is the
    only route for 128-bit values on MSVC, whose std::_Signed128 has no
    conversion to double). Both routes agree, so a NUMERIC value yields the
    same double whether the engine backs it with 16, 32, 64 or 128 bits.
*/
template <typename T>
double sqlScaledToDouble(T val, int scale)
{
    static constexpr double pow10[] = {
        1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
        1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22,
    };
    constexpr qint64 exactLimit = qint64(1) << 53;
    using Wide = std::conditional_t<sqlIsInt128<T>, T, qint64>;
    const Wide wide = static_cast<Wide>(val);
    if (wide > Wide(-exactLimit) && wide < Wide(exactLimit) && scale >= -22 && scale <= 22) {
        const double d = static_cast<double>(static_cast<qint64>(wide));
        return scale < 0 ? d / pow10[-scale] : d * pow10[scale];
    }
    return sqlScaledToString(val, scale).toDouble();
}

/*
    Return a scaled integer as the QVariant dictated by the precision policy:
    HighPrecision -> exact decimal QString; LowPrecisionDouble -> the nearest
    double (sqlScaledToDouble); LowPrecisionInt32/Int64 -> the integer part,
    computed with integer arithmetic (truncating toward zero) so large values
    keep their precision. scale == 0 returns a sub-128-bit value as is, with
    the usual integral promotion (qint16/qint32 -> Int, qint64 -> LongLong).

    An integer part that does not fit the requested LowPrecisionInt target is
    returned as a double instead of being wrapped, for every width; *overflow
    is set so the driver can log it.
*/
template <typename T>
QVariant sqlApplyScale(T val, int scale, QSql::NumericalPrecisionPolicy policy,
                       bool *overflow = nullptr)
{
    if (overflow)
        *overflow = false;
    // The integer policies compute in qint64 below 128 bits, in T itself at 128.
    using Wide = std::conditional_t<sqlIsInt128<T>, T, qint64>;
    const auto asDouble = [&] { return sqlScaledToDouble(val, scale); };

    if constexpr (!sqlIsInt128<T>) {
        if (scale == 0)
            return QVariant(val);
    }
    switch (policy) {
    case QSql::LowPrecisionInt32:
    case QSql::LowPrecisionInt64: {
        const Wide wide = static_cast<Wide>(val);
        const Wide scaled = scale < 0 ? wide / sqlIntPow10<Wide>(-scale)
                                      : wide * sqlIntPow10<Wide>(scale);
        if (policy == QSql::LowPrecisionInt32 && sqlFitsIn<qint32>(scaled))
            return QVariant(static_cast<qint32>(scaled));
        if (policy == QSql::LowPrecisionInt64 && sqlFitsIn<qint64>(scaled))
            return QVariant(static_cast<qint64>(scaled));
        if (overflow)
            *overflow = true;
        Q_FALLTHROUGH();
    }
    case QSql::LowPrecisionDouble:
        return QVariant(asDouble());
    case QSql::HighPrecision:
        return QVariant(sqlScaledToString(val, scale));
    }
    return QVariant(asDouble());
}

/*
    Report sqlApplyScale's overflow - the integer part did not fit the
    LowPrecisionInt target, so a double was returned - on the driver's logging
    category, passed as its category function so qCWarning's enablement check
    applies. value is the exact decimal text, as sqlScaledToString formats it.
    The one message text keeps the drivers' output identical.
*/
template <typename Category>
void sqlWarnLowPrecisionOverflow(Category &&category, const QString &value,
                                 QSql::NumericalPrecisionPolicy policy)
{
    qCWarning(category, "INT128 value %s exceeds the LowPrecisionInt%d range; returning a double",
              qUtf8Printable(value), policy == QSql::LowPrecisionInt32 ? 32 : 64);
}

/*
    Rewrite a decimal in exponent notation, as QByteArray::number(d, 'g', ...)
    produces it ("1.5e-19", "-2.5e+05"), as the same digits in plain notation
    ("0.00000000000000000015", "-250000") by moving the decimal point. No digit
    is dropped or invented. Returns false if the text is not of that form.
*/
inline bool sqlExpandExponent(QByteArrayView in, QByteArray *out)
{
    qsizetype ePos = in.indexOf('e');
    if (ePos < 0)
        ePos = in.indexOf('E');
    if (ePos < 0)
        return false;
    bool ok = false;
    const int exponent = in.sliced(ePos + 1).toInt(&ok); // "+20", "-05"
    if (!ok)
        return false;

    qsizetype pos = 0;
    bool negative = false;
    if (pos < ePos && (in[pos] == '+' || in[pos] == '-'))
        negative = (in[pos++] == '-');
    QByteArray digits;       // the mantissa's digits, point removed
    qsizetype intDigits = -1; // how many of them precede the point
    for (; pos < ePos; ++pos) {
        const char c = in[pos];
        if (c == '.') {
            if (intDigits >= 0)
                return false;
            intDigits = digits.size();
        } else if (c >= '0' && c <= '9') {
            digits.append(c);
        } else {
            return false;
        }
    }
    if (digits.isEmpty())
        return false;
    if (intDigits < 0)
        intDigits = digits.size();

    // Where the point lands within digits once the exponent is applied
    const qsizetype point = intDigits + exponent;
    const QByteArrayView digitsView(digits);
    QByteArray result;
    result.reserve(digits.size() + qAbs(exponent) + 3);
    if (negative)
        result.append('-');
    if (point <= 0) {
        result.append("0.");
        result.append(-point, '0');
        result.append(digitsView);
    } else if (point >= digits.size()) {
        result.append(digitsView);
        result.append(point - digits.size(), '0');
    } else {
        result.append(digitsView.first(point));
        result.append('.');
        result.append(digitsView.sliced(point));
    }
    *out = result;
    return true;
}

/*
    Variant types the encoders treat as plain integers: they scale with one
    checked multiply where the target type allows (sqlScaledFromVariant), and
    their decimal text is their value - a Bool is "1" or "0", not "true".
*/
inline bool sqlIsIntegralVariant(int typeId)
{
    return typeId == QMetaType::Int || typeId == QMetaType::UInt
        || typeId == QMetaType::LongLong || typeId == QMetaType::ULongLong
        || typeId == QMetaType::Bool;
}

/*
    The two bind failures, worded identically by every encoder (the native
    128-bit codec and Firebird's string interface alike): an integral value
    the column cannot hold, and text that is malformed or does not fit.
*/
inline QString sqlOutOfRangeError(const QVariant &val, int scale)
{
    return QString::fromLatin1("Value '%1' out of range for NUMERIC/DECIMAL with scale %2")
            .arg(val.toString(), QString::number(scale));
}

inline QString sqlCannotEncodeError(const QByteArray &text, int scale)
{
    return QString::fromLatin1("Cannot encode '%1' as NUMERIC/DECIMAL with scale %2")
            .arg(QString::fromLatin1(text), QString::number(scale));
}

/*
    Canonical decimal text of a value bound to a scaled numeric parameter.
    Integer and string variants are already plain C-locale decimals; a Bool
    becomes "1" or "0" (QVariant::toByteArray() would say "true"). A
    floating-point variant's text is its shortest round-trip form, which
    QVariant::toByteArray() renders in exponent notation for large and small
    magnitudes; the plain-decimal grammar below rejects exponents, so that form
    is expanded digit for digit rather than re-formatted with a fixed
    precision, which would drop the digits beyond it.
*/
inline QByteArray sqlNumericInputString(const QVariant &val)
{
    const int t = val.typeId();
    if (t == QMetaType::Bool)
        return val.toBool() ? QByteArray("1") : QByteArray("0");
    QByteArray s = val.toByteArray();
    if (t == QMetaType::Double || t == QMetaType::Float) {
        QByteArray plain;
        if (sqlExpandExponent(s, &plain))
            s = plain;
    }
    return s;
}

/*
    Split a plain decimal string into the parts of the scaled integer a column
    stores: shift the decimal point by -scale digits (right for the usual
    negative scale) and report the sign, the digits of the resulting integer
    part and whether the first dropped digit rounds it away from zero. Accepts
    an optional sign, digits and at most one decimal point, with surrounding
    whitespace; anything else (including exponent notation) is rejected.
    Each output pointer may be null; with all three null only the grammar is
    checked.
*/
inline bool sqlSplitScaledDecimal(QByteArrayView decimal, int scale, bool *negativeOut,
                                  QByteArray *intPartOut, bool *roundUpOut)
{
    const QByteArrayView s = decimal.trimmed();
    qsizetype pos = 0;
    bool negative = false;
    if (pos < s.size() && (s[pos] == '+' || s[pos] == '-'))
        negative = (s[pos++] == '-');

    QByteArray intPart, fracPart;
    bool dot = false, anyDigit = false;
    for (; pos < s.size(); ++pos) {
        const char c = s[pos];
        if (c == '.') {
            if (dot)
                return false;
            dot = true;
        } else if (c >= '0' && c <= '9') {
            (dot ? fracPart : intPart).append(c);
            anyDigit = true;
        } else {
            return false;
        }
    }
    if (!anyDigit)
        return false;
    if (!negativeOut && !intPartOut && !roundUpOut)
        return true; // validation only

    // Shift the decimal point right by -scale digits (left for positive scale).
    const int shift = -scale;
    if (shift >= 0) {
        while (fracPart.size() < shift)
            fracPart.append('0');
        intPart += fracPart.left(shift);
        fracPart.remove(0, shift);
    } else {
        const qsizetype k = -shift;
        while (intPart.size() < k)
            intPart.prepend('0');
        fracPart.prepend(intPart.right(k));
        intPart.chop(k);
    }

    // Round half away from zero on the first remaining fractional digit.
    if (roundUpOut)
        *roundUpOut = !fracPart.isEmpty() && fracPart[0] >= '5';
    if (negativeOut)
        *negativeOut = negative;
    if (intPartOut)
        *intPartOut = intPart;
    return true;
}

/*
    True if the string satisfies sqlDecimalToScaled's grammar. Lets a driver
    that hands the text to its client library reject exactly what the native
    parser rejects.
*/
inline bool sqlIsPlainDecimal(QByteArrayView decimal)
{
    return sqlSplitScaledDecimal(decimal, 0, nullptr, nullptr, nullptr);
}

/*
    Convert a plain decimal string into the scaled integer of type T, rounding
    half away from zero and detecting overflow. The digits accumulate toward
    the sign — acc*10 - d for negatives — so the minimum of T is reachable and
    T is never negated. Each step is bounds-checked before it happens; `/`
    truncates toward zero for every T used here, so the bounds are exact.
*/
template <typename T>
bool sqlDecimalToScaled(QByteArrayView decimal, int scale, T *out)
{
    bool negative = false, roundUp = false;
    QByteArray intPart;
    if (!sqlSplitScaledDecimal(decimal, scale, &negative, &intPart, &roundUp))
        return false;

    const T lo = (std::numeric_limits<T>::min)();
    const T hi = (std::numeric_limits<T>::max)();
    T acc = 0;
    for (const char c : intPart) {
        const int d = c - '0';
        if (negative) {
            if (acc < (lo + d) / 10)
                return false; // overflow
            acc = acc * 10 - d;
        } else {
            if (acc > (hi - d) / 10)
                return false; // overflow
            acc = acc * 10 + d;
        }
    }
    if (roundUp) {
        if (acc == (negative ? lo : hi))
            return false; // overflow
        acc += negative ? -1 : 1;
    }
    *out = acc;
    return true;
}

/*
    Encode a bound value as the scaled integer of type T for a column with the
    given scale (0 for a plain integer column). Integral variants scale with
    one checked multiply; everything else goes through its canonical decimal
    string, so strings and doubles are encoded exactly — never via a double
    round trip. On failure returns false and, if errorText is given, describes
    the problem.
*/
template <typename T>
bool sqlScaledFromVariant(const QVariant &val, int scale, T *out, QString *errorText)
{
    const auto outOfRange = [&] {
        if (errorText)
            *errorText = sqlOutOfRangeError(val, scale);
        return false;
    };

    const int t = val.typeId();
    // 10^digits10 is the largest power of ten T can hold.
    if (sqlIsIntegralVariant(t) && scale <= 0 && -scale <= std::numeric_limits<T>::digits10) {
        const T lo = (std::numeric_limits<T>::min)();
        const T hi = (std::numeric_limits<T>::max)();
        T v = 0;
        if (t == QMetaType::ULongLong) {
            const quint64 u = val.toULongLong();
            if constexpr (sizeof(T) <= sizeof(quint64)) {
                if (u > static_cast<quint64>(hi))
                    return outOfRange();
            }
            v = T(u);
        } else {
            const qint64 ll = val.toLongLong();
            if constexpr (sizeof(T) < sizeof(qint64)) {
                // reject before narrowing - T(ll) would silently wrap
                if (!sqlFitsIn<T>(ll))
                    return outOfRange();
            }
            v = T(ll);
        }
        const T factor = sqlIntPow10<T>(-scale);
        if (v < 0 ? v < lo / factor : v > hi / factor)
            return outOfRange();
        *out = v * factor;
        return true;
    }

    const QByteArray strVal = sqlNumericInputString(val);
    if (!sqlDecimalToScaled<T>(strVal, scale, out)) {
        if (errorText)
            *errorText = sqlCannotEncodeError(strVal, scale);
        return false;
    }
    return true;
}

#ifdef QSQL_HAS_INT128
/*
    Assemble a 128-bit integer from its two 64-bit words and split it again.
    Engines transmit INT128 as a low and a high word (Firebird's FB_I128), so
    drivers copy those words and never dereference a 16-byte-aligned type in
    a message buffer; going word-wise also keeps the result independent of the
    host's byte order.
*/
inline qinternalint128 sqlInt128FromWords(quint64 low, quint64 high)
{
    qinternaluint128 u = high;
    u <<= 64;
    u |= low;
    return static_cast<qinternalint128>(u);
}

inline void sqlInt128ToWords(qinternalint128 value, quint64 *low, quint64 *high)
{
    const qinternaluint128 u = static_cast<qinternaluint128>(value);
    *low = static_cast<quint64>(u);
    *high = static_cast<quint64>(u >> 64);
}
#endif // QSQL_HAS_INT128

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QSQLNUMERIC_P_H
