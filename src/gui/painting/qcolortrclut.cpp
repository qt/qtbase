/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#include "qcolortrclut_p.h"
#include "qcolortransferfunction_p.h"
#include "qcolortransfertable_p.h"
#include <qmath.h>

QT_BEGIN_NAMESPACE

QColorTrcLut *QColorTrcLut::fromGamma(qreal gamma)
{
    QColorTrcLut *cp = new QColorTrcLut;

    for (int i = 0; i <= (255 * 16); ++i) {
        cp->m_toLinear[i] = ushort(qRound(qPow(i / qreal(255 * 16), gamma) * (255 * 256)));
        cp->m_fromLinear[i] = ushort(qRound(qPow(i / qreal(255 * 16), qreal(1) / gamma) * (255 * 256)));
    }

    return cp;
}

QColorTrcLut *QColorTrcLut::fromTransferFunction(const QColorTransferFunction &fun)
{
    QColorTrcLut *cp = new QColorTrcLut;
    QColorTransferFunction inv = fun.inverted();

    for (int i = 0; i <= (255 * 16); ++i) {
        cp->m_toLinear[i] = ushort(qRound(fun.apply(i / qreal(255 * 16)) * (255 * 256)));
        cp->m_fromLinear[i] = ushort(qRound(inv.apply(i / qreal(255 * 16)) * (255 * 256)));
    }

    return cp;
}

QColorTrcLut *QColorTrcLut::fromTransferTable(const QColorTransferTable &table)
{
    QColorTrcLut *cp = new QColorTrcLut;

    float minInverse = 0.0f;
    for (int i = 0; i <= (255 * 16); ++i) {
        cp->m_toLinear[i] = ushort(qBound(0, qRound(table.apply(i / qreal(255 * 16)) * (255 * 256)), 65280));
        minInverse = table.applyInverse(i / qreal(255 * 16), minInverse);
        cp->m_fromLinear[i] = ushort(qBound(0, qRound(minInverse * (255 * 256)), 65280));
    }

    return cp;
}

QT_END_NAMESPACE
