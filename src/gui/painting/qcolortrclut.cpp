/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
