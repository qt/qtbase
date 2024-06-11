// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcolortrclut_p.h"
#include "qcolortransferfunction_p.h"
#include "qcolortransfergeneric_p.h"
#include "qcolortransfertable_p.h"
#include "qcolortrc_p.h"
#include <qmath.h>

QT_BEGIN_NAMESPACE
std::shared_ptr<QColorTrcLut> QColorTrcLut::create()
{
    struct Access : QColorTrcLut {};
    return std::make_shared<Access>();
}

std::shared_ptr<QColorTrcLut> QColorTrcLut::fromGamma(float gamma, Direction dir)
{
    auto cp = create();
    cp->setFromGamma(gamma, dir);
    return cp;
}

std::shared_ptr<QColorTrcLut> QColorTrcLut::fromTrc(const QColorTrc &trc, Direction dir)
{
    if (!trc.isValid())
        return nullptr;
    auto cp = create();
    cp->setFromTrc(trc, dir);
    return cp;
}

void QColorTrcLut::setFromGamma(float gamma, Direction dir)
{
    constexpr float iRes = 1.f / float(Resolution);
    if (dir & ToLinear) {
        if (!m_toLinear)
            m_toLinear.reset(new ushort[Resolution + 1]);
        for (int i = 0; i <= Resolution; ++i) {
            const int val = qRound(qPow(i * iRes, gamma) * (255 * 256));
            m_toLinear[i] = qBound(0, val, 65280);
        }
    }

    if (dir & FromLinear) {
        const float iGamma = 1.f / gamma;
        if (!m_fromLinear)
            m_fromLinear.reset(new ushort[Resolution + 1]);
        for (int i = 0; i <= Resolution; ++i)
            m_fromLinear[i] = ushort(qRound(qBound(0.f, qPow(i * iRes, iGamma), 1.f) * (255 * 256)));
    }
}

void QColorTrcLut::setFromTransferFunction(const QColorTransferFunction &fun, Direction dir)
{
    constexpr float iRes = 1.f / float(Resolution);
    if (dir & ToLinear) {
        if (!m_toLinear)
            m_toLinear.reset(new ushort[Resolution + 1]);
        for (int i = 0; i <= Resolution; ++i) {
            const int val = qRound(fun.apply(i * iRes)* (255 * 256));
            if (val > 65280 && i < m_unclampedToLinear)
                m_unclampedToLinear = i;
            m_toLinear[i] = qBound(0, val, 65280);
        }
    }

    if (dir & FromLinear) {
        if (!m_fromLinear)
            m_fromLinear.reset(new ushort[Resolution + 1]);
        QColorTransferFunction inv = fun.inverted();
        for (int i = 0; i <= Resolution; ++i)
            m_fromLinear[i] = ushort(qRound(qBound(0.f, inv.apply(i * iRes), 1.f) * (255 * 256)));
    }
}

void QColorTrcLut::setFromTransferGenericFunction(const QColorTransferGenericFunction &fun, Direction dir)
{
    constexpr float iRes = 1.f / float(Resolution);
    if (dir & ToLinear) {
        if (!m_toLinear)
            m_toLinear.reset(new ushort[Resolution + 1]);
        for (int i = 0; i <= Resolution; ++i) {
            const int val = qRound(fun.apply(i * iRes) * (255 * 256));
            if (val > 65280 && i < m_unclampedToLinear)
                m_unclampedToLinear = i;
            m_toLinear[i] = qBound(0, val, 65280);
        }
    }

    if (dir & FromLinear) {
        if (!m_fromLinear)
            m_fromLinear.reset(new ushort[Resolution + 1]);
        for (int i = 0; i <= Resolution; ++i)
            m_fromLinear[i] = ushort(qRound(qBound(0.f, fun.applyInverse(i * iRes), 1.f) * (255 * 256)));
    }
}

void QColorTrcLut::setFromTransferTable(const QColorTransferTable &table, Direction dir)
{
    constexpr float iRes = 1.f / float(Resolution);
    if (dir & ToLinear) {
        if (!m_toLinear)
            m_toLinear.reset(new ushort[Resolution + 1]);
        for (int i = 0; i <= Resolution; ++i)
            m_toLinear[i] = ushort(qRound(table.apply(i * iRes) * (255 * 256)));
    }

    if (dir & FromLinear) {
        if (!m_fromLinear)
            m_fromLinear.reset(new ushort[Resolution + 1]);
        float minInverse = 0.0f;
        for (int i = 0; i <= Resolution; ++i) {
            minInverse = table.applyInverse(i * iRes, minInverse);
            m_fromLinear[i] = ushort(qRound(minInverse * (255 * 256)));
        }
    }
}

void QColorTrcLut::setFromTrc(const QColorTrc &trc, Direction dir)
{
    switch (trc.m_type) {
    case QColorTrc::Type::ParameterizedFunction:
        return setFromTransferFunction(trc.fun(), dir);
    case QColorTrc::Type::Table:
        return setFromTransferTable(trc.table(), dir);
    case QColorTrc::Type::GenericFunction:
        return setFromTransferGenericFunction(trc.hdr(), dir);
    case QColorTrc::Type::Uninitialized:
        break;
    }
}

QT_END_NAMESPACE
