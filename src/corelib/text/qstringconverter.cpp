/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2018 Intel Corporation.
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

#include <qstringconverter.h>
#include <private/qutfcodec_p.h>

QT_BEGIN_NAMESPACE

/*!
    \enum QStringConverter::Flag

    \value DefaultConversion  No flag is set.
    \value ConvertInvalidToNull  If this flag is set, each invalid input
                                 character is output as a null character.
    \value IgnoreHeader  Ignore any Unicode byte-order mark and don't generate any.

    \value Stateless Ignore possible converter states between different function calls
           to encode or decode strings.
*/


void QStringConverter::State::clear()
{
    if (clearFn)
        clearFn(this);
    state_data[0] = state_data[1] = state_data[2] = state_data[3] = 0;
    remainingChars = 0;
    invalidChars = 0;
}

static QChar *fromUtf8(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf8::convertToUnicode(in, length, state);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf8(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf8::convertFromUnicode(in.data(), in.length(), state);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf16(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf16::convertToUnicode(in, length, state);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf16(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf16::convertFromUnicode(in.data(), in.length(), state);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf16BE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf16::convertToUnicode(in, length, state, BigEndianness);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf16BE(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf16::convertFromUnicode(in.data(), in.length(), state, BigEndianness);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf16LE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf16::convertToUnicode(in, length, state, LittleEndianness);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf16LE(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf16::convertFromUnicode(in.data(), in.length(), state, LittleEndianness);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf32(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf32::convertToUnicode(in, length, state);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf32(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf32::convertFromUnicode(in.data(), in.length(), state);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf32BE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf32::convertToUnicode(in, length, state, BigEndianness);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf32BE(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf32::convertFromUnicode(in.data(), in.length(), state, BigEndianness);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static QChar *fromUtf32LE(QChar *out, const char *in, qsizetype length, QStringConverter::State *state)
{
    QString s = QUtf32::convertToUnicode(in, length, state, LittleEndianness);
    memcpy(out, s.constData(), s.length()*sizeof(QChar));
    return out + s.length();
}

static char *toUtf32LE(char *out, QStringView in, QStringConverter::State *state)
{
    QByteArray s = QUtf32::convertFromUnicode(in.data(), in.length(), state, LittleEndianness);
    memcpy(out, s.constData(), s.length());
    return out + s.length();
}

static qsizetype fromUtf8Len(qsizetype l) { return l + 1; }
static qsizetype toUtf8Len(qsizetype l) { return 3*(l + 1); }

static qsizetype fromUtf16Len(qsizetype l) { return l/2 + 2; }
static qsizetype toUtf16Len(qsizetype l) { return 2*(l + 1); }

static qsizetype fromUtf32Len(qsizetype l) { return l + 1; }
static qsizetype toUtf32Len(qsizetype l) { return 4*(l + 1); }

const QStringConverter::Interface QStringConverter::encodingInterfaces[QStringConverter::LastEncoding + 1] =
{
    { fromUtf8, fromUtf8Len, toUtf8, toUtf8Len },
    { fromUtf16, fromUtf16Len, toUtf16, toUtf16Len },
    { fromUtf16LE, fromUtf16Len, toUtf16LE, toUtf16Len },
    { fromUtf16BE, fromUtf16Len, toUtf16BE, toUtf16Len },
    { fromUtf32, fromUtf32Len, toUtf32, toUtf32Len },
    { fromUtf32LE, fromUtf32Len, toUtf32LE, toUtf32Len },
    { fromUtf32BE, fromUtf32Len, toUtf32BE, toUtf32Len }
};

QT_END_NAMESPACE
