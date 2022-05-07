/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QMATH_P_H
#define QMATH_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qmath.h>
#include <qtransform.h>

QT_BEGIN_NAMESPACE

static const qreal Q_PI   = qreal(M_PI);     // pi
static const qreal Q_MM_PER_INCH = 25.4;

inline QRect qt_mapFillRect(const QRectF &rect, const QTransform &xf)
{
    // Only for xf <= scaling or 90 degree rotations
    Q_ASSERT(xf.type() <= QTransform::TxScale
             || (xf.type() == QTransform::TxRotate && qFuzzyIsNull(xf.m11()) && qFuzzyIsNull(xf.m22())));
    // Transform the corners instead of the rect to avoid hitting numerical accuracy limit
    // when transforming topleft and size separately and adding afterwards,
    // as that can sometimes be slightly off around the .5 point, leading to wrong rounding
    QPoint pt1 = xf.map(rect.topLeft()).toPoint();
    QPoint pt2 = xf.map(rect.bottomRight()).toPoint();
    // Normalize and adjust for the QRect vs. QRectF bottomright
    return QRect::span(pt1, pt2).adjusted(0, 0, -1, -1);
}

QT_END_NAMESPACE

#endif // QMATH_P_H
