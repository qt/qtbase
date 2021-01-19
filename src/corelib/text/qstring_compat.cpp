/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
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

#if defined(QSTRING_H) || defined(QBYTEARRAY_H)
#  error "This file cannot be compiled with pre-compiled headers"
#endif
#define QT_COMPILING_QSTRING_COMPAT_CPP

#include "qbytearray.h"
#include "qstring.h"

QT_BEGIN_NAMESPACE

// all these implementations must be the same as the inline versions in qstring.h
QString QString::trimmed() const
{
    return trimmed_helper(*this);
}

QString QString::simplified() const
{
    return simplified_helper(*this);
}

QString QString::toLower() const
{
    return toLower_helper(*this);
}

QString QString::toCaseFolded() const
{
    return toCaseFolded_helper(*this);
}

QString QString::toUpper() const
{
    return toUpper_helper(*this);
}

QByteArray QString::toLatin1() const
{
    return toLatin1_helper(*this);
}

QByteArray QString::toLocal8Bit() const
{
    return toLocal8Bit_helper(isNull() ? nullptr : constData(), size());
}

QByteArray QString::toUtf8() const
{
    return toUtf8_helper(*this);
}

// ditto, for qbytearray.h (because we're lazy)
QByteArray QByteArray::toLower() const
{
    return toLower_helper(*this);
}

QByteArray QByteArray::toUpper() const
{
    return toUpper_helper(*this);
}

QByteArray QByteArray::trimmed() const
{
    return trimmed_helper(*this);
}

QByteArray QByteArray::simplified() const
{
    return simplified_helper(*this);
}

QT_END_NAMESPACE
