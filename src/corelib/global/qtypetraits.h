/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#include "QtCore/qglobal.h"

#ifndef QTYPETRAITS_H
#define QTYPETRAITS_H

QT_BEGIN_NAMESPACE

namespace QtPrivate {

//
// Define QIsUnsignedEnum, QIsSignedEnum -
// std::is_signed, std::is_unsigned does not work for enum's
//

// a metafunction to invert an integral_constant:
template <typename T>
struct not_
    : std::integral_constant<bool, !T::value> {};

// Checks whether a type is unsigned (T must be convertible to unsigned int):
template <typename T>
struct QIsUnsignedEnum
    : std::integral_constant<bool, (T(0) < T(-1))> {};

// Checks whether a type is signed (T must be convertible to int):
template <typename T>
struct QIsSignedEnum
    : not_< QIsUnsignedEnum<T> > {};

Q_STATIC_ASSERT(( QIsUnsignedEnum<quint8>::value));
Q_STATIC_ASSERT((!QIsUnsignedEnum<qint8>::value));

Q_STATIC_ASSERT((!QIsSignedEnum<quint8>::value));
Q_STATIC_ASSERT(( QIsSignedEnum<qint8>::value));

Q_STATIC_ASSERT(( QIsUnsignedEnum<quint16>::value));
Q_STATIC_ASSERT((!QIsUnsignedEnum<qint16>::value));

Q_STATIC_ASSERT((!QIsSignedEnum<quint16>::value));
Q_STATIC_ASSERT(( QIsSignedEnum<qint16>::value));

Q_STATIC_ASSERT(( QIsUnsignedEnum<quint32>::value));
Q_STATIC_ASSERT((!QIsUnsignedEnum<qint32>::value));

Q_STATIC_ASSERT((!QIsSignedEnum<quint32>::value));
Q_STATIC_ASSERT(( QIsSignedEnum<qint32>::value));

Q_STATIC_ASSERT(( QIsUnsignedEnum<quint64>::value));
Q_STATIC_ASSERT((!QIsUnsignedEnum<qint64>::value));

Q_STATIC_ASSERT((!QIsSignedEnum<quint64>::value));
Q_STATIC_ASSERT(( QIsSignedEnum<qint64>::value));

} // namespace QtPrivate

QT_END_NAMESPACE
#endif  // QTYPETRAITS_H
