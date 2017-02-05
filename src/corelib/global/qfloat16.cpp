/****************************************************************************
**
** Copyright (C) 2016 by Southwest Research Institute (R)
** Contact: http://www.qt-project.org/legal
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

#include "qfloat16_p.h"

QT_BEGIN_NAMESPACE

/*! \headerfile <QFloat16>

    This header file provides support for half-precision (16-bit) floating
    point data with the class \c qfloat16.  It is fully compliant with IEEE
    754 as a storage type.  This implies that any arithmetic operation on a
    \c qfloat16 instance results in the value first being converted to a
    \c float.  This conversion to and from \c float is performed by hardware
    when possible, but on processors that do not natively support half-precision,
    the conversion is performed through a sequence of lookup table operations.

    \c qfloat16 should be treated as if it were a POD (plain old data) type.
    Consequently, none of the supported operations need any elaboration beyond
    stating that it supports all arithmetic operators incident to floating point
    types.

    \since 5.9
*/

Q_STATIC_ASSERT_X(sizeof(float) == sizeof(quint32),
                  "qfloat16 assumes that floats are 32 bits wide");

// There are a few corner cases regarding denormals where GHS compiler is relying
// hardware behavior that is not IEC 559 compliant. Therefore the compiler
// reports std::numeric_limits<float>::is_iec559 as false. This is all right
// according to our needs.

#if !defined(Q_CC_GHS)
Q_STATIC_ASSERT_X(std::numeric_limits<float>::is_iec559,
                  "Only works with IEEE 754 floating point");
#endif

Q_STATIC_ASSERT_X(std::numeric_limits<float>::has_infinity &&
                  std::numeric_limits<float>::has_quiet_NaN &&
                  std::numeric_limits<float>::has_signaling_NaN,
                  "Only works with IEEE 754 floating point");

/*!
    Returns true if the \c qfloat16 \a {f} is equivalent to infinity.
    \relates <QFloat16>

    \sa qIsInf
*/
Q_REQUIRED_RESULT bool qIsInf(qfloat16 f) Q_DECL_NOTHROW { return qt_is_inf(f); }

/*!
    Returns true if the \c qfloat16 \a {f} is not a number (NaN).
    \relates <QFloat16>

    \sa qIsNaN
*/
Q_REQUIRED_RESULT bool qIsNaN(qfloat16 f) Q_DECL_NOTHROW { return qt_is_nan(f); }

/*!
    Returns true if the \c qfloat16 \a {f} is a finite number.
    \relates <QFloat16>

    \sa qIsFinite
*/
Q_REQUIRED_RESULT bool qIsFinite(qfloat16 f) Q_DECL_NOTHROW { return qt_is_finite(f); }

/*! \fn int qRound(qfloat16 value)
    \relates <QFloat16>

    Rounds \a value to the nearest integer.

    \sa qRound
*/

/*! \fn qint64 qRound64(qfloat16 value)
    \relates <QFloat16>

    Rounds \a value to the nearest 64-bit integer.

    \sa qRound64
*/

/*! \fn bool qFuzzyCompare(qfloat16 p1, qfloat16 p2)
    \relates <QFloat16>

    Compares the floating point value \a p1 and \a p2 and
    returns \c true if they are considered equal, otherwise \c false.

    The two numbers are compared in a relative way, where the
    exactness is stronger the smaller the numbers are.
 */

QT_END_NAMESPACE
