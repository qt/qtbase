/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

// API note: this function can't process a number with more than 2.1 billion digits
[[nodiscard]] double qt_asciiToDouble(const char *num, qsizetype numLen, bool &ok, int &processed,
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

[[nodiscard]] qlonglong qstrntoll(const char *nptr, qsizetype size, const char **endptr,
                                  int base, bool *ok);
[[nodiscard]] qulonglong qstrntoull(const char *nptr, qsizetype size, const char **endptr,
                                    int base, bool *ok);

QT_END_NAMESPACE

#endif
