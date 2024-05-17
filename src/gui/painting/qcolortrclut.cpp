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

std::shared_ptr<QColorTrcLut> QColorTrcLut::fromGamma(qreal gamma, Direction dir)
{
    auto cp = create();
    cp->setFromGamma(gamma, dir);
    return cp;
}

std::shared_ptr<QColorTrcLut> QColorTrcLut::fromTransferFunction(const QColorTransferFunction &fun, Direction dir)
{
    auto cp = create();
    cp->setFromTransferFunction(fun, dir);
    return cp;
}

std::shared_ptr<QColorTrcLut> QColorTrcLut::fromTransferTable(const QColorTransferTable &table, Direction dir)
{
    auto cp = create();
    cp->setFromTransferTable(table, dir);
    return cp;
}

void QColorTrcLut::setFromGamma(qreal gamma, Direction dir)
{
    if (dir & ToLinear) {
        if (!m_toLinear)
            m_toLinear.reset(new ushort[Resolution + 1]);
        for (int i = 0; i <= Resolution; ++i)
            m_toLinear[i] = ushort(qRound(qPow(i / qreal(Resolution), gamma) * (255 * 256)));
    }

    if (dir & FromLinear) {
        if (!m_fromLinear)
            m_fromLinear.reset(new ushort[Resolution + 1]);
        for (int i = 0; i <= Resolution; ++i)
            m_fromLinear[i] = ushort(qRound(qPow(i / qreal(Resolution), qreal(1) / gamma) * (255 * 256)));
    }
}

void QColorTrcLut::setFromTransferFunction(const QColorTransferFunction &fun, Direction dir)
{
    if (dir & ToLinear) {
        if (!m_toLinear)
            m_toLinear.reset(new ushort[Resolution + 1]);
        for (int i = 0; i <= Resolution; ++i)
            m_toLinear[i] = ushort(qRound(fun.apply(i / qreal(Resolution)) * (255 * 256)));
    }

    if (dir & FromLinear) {
        if (!m_fromLinear)
            m_fromLinear.reset(new ushort[Resolution + 1]);
        QColorTransferFunction inv = fun.inverted();
        for (int i = 0; i <= Resolution; ++i)
            m_fromLinear[i] = ushort(qRound(inv.apply(i / qreal(Resolution)) * (255 * 256)));
    }
}

void QColorTrcLut::setFromTransferTable(const QColorTransferTable &table, Direction dir)
{
    if (dir & ToLinear) {
        if (!m_toLinear)
            m_toLinear.reset(new ushort[Resolution + 1]);
        for (int i = 0; i <= Resolution; ++i)
            m_toLinear[i] = ushort(qBound(0, qRound(table.apply(i / qreal(Resolution)) * (255 * 256)), 65280));
    }

    if (dir & FromLinear) {
        if (!m_fromLinear)
            m_fromLinear.reset(new ushort[Resolution + 1]);
        float minInverse = 0.0f;
        for (int i = 0; i <= Resolution; ++i) {
            minInverse = table.applyInverse(i / qreal(Resolution), minInverse);
            m_fromLinear[i] = ushort(qBound(0, qRound(minInverse * (255 * 256)), 65280));
        }
    }
}

QT_END_NAMESPACE
