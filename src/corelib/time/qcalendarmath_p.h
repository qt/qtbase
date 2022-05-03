/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef QCALENDARMATH_P_H
#define QCALENDARMATH_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of q*calendar.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QRoundingDown {
/*
  Division, rounding down (rather than towards zero).

  From C++11 onwards, integer division is defined to round towards zero, so we
  can rely on that when implementing this. This is only used with denominator b
  > 0, so we only have to treat negative numerator, a, specially.

  If a is a multiple of b, adding 1 before and subtracting it after dividing by
  b gets us to where we should be (albeit by an eccentric path), since the
  adding caused rounding up, undone by the subtracting. Otherwise, adding 1
  doesn't change the result of dividing by b; and we want one less than that
  result. This is equivalent to subtracting b - 1 and simply dividing, except
  when that subtraction would underflow.
*/

template<typename Int> constexpr Int qDiv(Int a, unsigned b)
{ return a < 0 ? (a + 1) / int(b) - 1 : a / int(b); }

template<typename Int> constexpr Int qMod(Int a, unsigned b)
{ return a - qDiv(a, b) * b; }

} // QRoundingDown

QT_END_NAMESPACE

#endif // QCALENDARMATH_P_H
