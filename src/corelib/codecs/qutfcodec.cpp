/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qutfcodec_p.h"
#include "qlist.h"
#include "qendian.h"
#include "qchar.h"

#include "private/qsimd_p.h"
#include "private/qstringiterator_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(textcodec)

QUtf8Codec::~QUtf8Codec()
{
}

QByteArray QUtf8Codec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    ConverterState s(QStringConverter::Flag::Stateless);
    if (!state)
        state = &s;
    return QUtf8::convertFromUnicode(uc, len, state);
}

void QUtf8Codec::convertToUnicode(QString *target, const char *chars, int len, ConverterState *state) const
{
    ConverterState s(QStringConverter::Flag::Stateless);
    if (!state)
        state = &s;
    *target += QUtf8::convertToUnicode(chars, len, state);
}

QString QUtf8Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
{
    ConverterState s(QStringConverter::Flag::Stateless);
    if (!state)
        state = &s;
    return QUtf8::convertToUnicode(chars, len, state);
}

QByteArray QUtf8Codec::name() const
{
    return "UTF-8";
}

int QUtf8Codec::mibEnum() const
{
    return 106;
}

QUtf16Codec::~QUtf16Codec()
{
}

QByteArray QUtf16Codec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    ConverterState s(QStringConverter::Flag::Stateless);
    if (!state)
        state = &s;
    return QUtf16::convertFromUnicode(uc, len, state, e);
}

QString QUtf16Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
{
    ConverterState s(QStringConverter::Flag::Stateless);
    if (!state)
        state = &s;
    return QUtf16::convertToUnicode(chars, len, state, e);
}

int QUtf16Codec::mibEnum() const
{
    return 1015;
}

QByteArray QUtf16Codec::name() const
{
    return "UTF-16";
}

QList<QByteArray> QUtf16Codec::aliases() const
{
    return QList<QByteArray>();
}

int QUtf16BECodec::mibEnum() const
{
    return 1013;
}

QByteArray QUtf16BECodec::name() const
{
    return "UTF-16BE";
}

QList<QByteArray> QUtf16BECodec::aliases() const
{
    QList<QByteArray> list;
    return list;
}

int QUtf16LECodec::mibEnum() const
{
    return 1014;
}

QByteArray QUtf16LECodec::name() const
{
    return "UTF-16LE";
}

QList<QByteArray> QUtf16LECodec::aliases() const
{
    QList<QByteArray> list;
    return list;
}

QUtf32Codec::~QUtf32Codec()
{
}

QByteArray QUtf32Codec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    ConverterState s(QStringConverter::Flag::Stateless);
    if (!state)
        state = &s;
    return QUtf32::convertFromUnicode(uc, len, state, e);
}

QString QUtf32Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
{
    ConverterState s(QStringConverter::Flag::Stateless);
    if (!state)
        state = &s;
    return QUtf32::convertToUnicode(chars, len, state, e);
}

int QUtf32Codec::mibEnum() const
{
    return 1017;
}

QByteArray QUtf32Codec::name() const
{
    return "UTF-32";
}

QList<QByteArray> QUtf32Codec::aliases() const
{
    QList<QByteArray> list;
    return list;
}

int QUtf32BECodec::mibEnum() const
{
    return 1018;
}

QByteArray QUtf32BECodec::name() const
{
    return "UTF-32BE";
}

QList<QByteArray> QUtf32BECodec::aliases() const
{
    QList<QByteArray> list;
    return list;
}

int QUtf32LECodec::mibEnum() const
{
    return 1019;
}

QByteArray QUtf32LECodec::name() const
{
    return "UTF-32LE";
}

QList<QByteArray> QUtf32LECodec::aliases() const
{
    QList<QByteArray> list;
    return list;
}

#endif // textcodec

QT_END_NAMESPACE
