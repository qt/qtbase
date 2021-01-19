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
*/

template<typename Int> constexpr Int qDiv(Int a, unsigned b)
{ return (a - (a < 0 ? int(b - 1) : 0)) / int(b); }

template<typename Int> constexpr Int qMod(Int a, unsigned b)
{ return a - qDiv(a, b) * b; }

} // QRoundingDown

QT_END_NAMESPACE

#endif // QCALENDARMATH_P_H
