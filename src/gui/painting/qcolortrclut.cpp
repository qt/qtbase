// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcolortrclut_p.h"
#include "qcolortransferfunction_p.h"
#include "qcolortransfertable_p.h"
#include <qmath.h>

QT_BEGIN_NAMESPACE
std::shared_ptr<QColorTrcLut> QColorTrcLut::create()
{
    struct Access : QColorTrcLut {};
    return std::make_shared<Access>();
}

std::shared_ptr<QColorTrcLut> QColorTrcLut::fromGamma(qreal gamma)
{
    auto cp = create();

    for (int i = 0; i <= (255 * 16); ++i) {
        cp->m_toLinear[i] = ushort(qRound(qPow(i / qreal(255 * 16), gamma) * (255 * 256)));
        cp->m_fromLinear[i] = ushort(qRound(qPow(i / qreal(255 * 16), qreal(1) / gamma) * (255 * 256)));
    }

    return cp;
}

std::shared_ptr<QColorTrcLut> QColorTrcLut::fromTransferFunction(const QColorTransferFunction &fun)
{
    auto cp = create();
    QColorTransferFunction inv = fun.inverted();

    for (int i = 0; i <= (255 * 16); ++i) {
        cp->m_toLinear[i] = ushort(qRound(fun.apply(i / qreal(255 * 16)) * (255 * 256)));
        cp->m_fromLinear[i] = ushort(qRound(inv.apply(i / qreal(255 * 16)) * (255 * 256)));
    }

    return cp;
}

std::shared_ptr<QColorTrcLut> QColorTrcLut::fromTransferTable(const QColorTransferTable &table)
{
    auto cp = create();

    float minInverse = 0.0f;
    for (int i = 0; i <= (255 * 16); ++i) {
        cp->m_toLinear[i] = ushort(qBound(0, qRound(table.apply(i / qreal(255 * 16)) * (255 * 256)), 65280));
        minInverse = table.applyInverse(i / qreal(255 * 16), minInverse);
        cp->m_fromLinear[i] = ushort(qBound(0, qRound(minInverse * (255 * 256)), 65280));
    }

    return cp;
}

QT_END_NAMESPACE
