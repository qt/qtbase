// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
