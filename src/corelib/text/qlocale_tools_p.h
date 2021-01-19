/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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

double qt_asciiToDouble(const char *num, int numLen, bool &ok, int &processed,
                        StrayCharacterMode strayCharMode = TrailingJunkProhibited);
void qt_doubleToAscii(double d, QLocaleData::DoubleForm form, int precision, char *buf, int bufSize,
                      bool &sign, int &length, int &decpt);

QString qulltoa(qulonglong l, int base, const QChar _zero);
Q_CORE_EXPORT QString qdtoa(qreal d, int *decpt, int *sign);

enum PrecisionMode {
    PMDecimalDigits =             0x01,
    PMSignificantDigits =   0x02,
    PMChopTrailingZeros =   0x03
};

QString &decimalForm(QChar zero, QChar decimal, QChar group,
                     QString &digits, int decpt, int precision,
                     PrecisionMode pm,
                     bool always_show_decpt,
                     bool thousands_group);
QString &exponentForm(QChar zero, QChar decimal, QChar exponential,
                      QChar group, QChar plus, QChar minus,
                      QString &digits, int decpt, int precision,
                      PrecisionMode pm,
                      bool always_show_decpt,
                      bool leading_zero_in_exponent);

inline bool isZero(double d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return !(ch[0] & 0x7F || ch[1] || ch[2] || ch[3] || ch[4] || ch[5] || ch[6] || ch[7]);
    } else {
        return !(ch[7] & 0x7F || ch[6] || ch[5] || ch[4] || ch[3] || ch[2] || ch[1] || ch[0]);
    }
}

Q_CORE_EXPORT double qstrtod(const char *s00, char const **se, bool *ok);
Q_CORE_EXPORT double qstrntod(const char *s00, int len, char const **se, bool *ok);
qlonglong qstrtoll(const char *nptr, const char **endptr, int base, bool *ok);
qulonglong qstrtoull(const char *nptr, const char **endptr, int base, bool *ok);

QT_END_NAMESPACE

#endif
