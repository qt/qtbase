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

#include "qcolorprofile_p.h"
#include <qmath.h>

QT_BEGIN_NAMESPACE

QColorProfile *QColorProfile::fromGamma(qreal gamma)
{
    QColorProfile *cp = new QColorProfile;

    for (int i = 0; i <= (255 * 16); ++i) {
        cp->m_toLinear[i] = ushort(qRound(qPow(i / qreal(255 * 16), gamma) * (255 * 256)));
        cp->m_fromLinear[i] = ushort(qRound(qPow(i / qreal(255 * 16), qreal(1) / gamma) * (255 * 256)));
    }

    return cp;
}

static qreal srgbToLinear(qreal v)
{
    const qreal a = 0.055;
    if (v <= qreal(0.04045))
        return v / qreal(12.92);
    else
        return qPow((v + a) / (qreal(1) + a), qreal(2.4));
}

static qreal linearToSrgb(qreal v)
{
    const qreal a = 0.055;
    if (v <= qreal(0.0031308))
        return v * qreal(12.92);
    else
        return (qreal(1) + a) * qPow(v, qreal(1.0 / 2.4)) - a;
}

QColorProfile *QColorProfile::fromSRgb()
{
    QColorProfile *cp = new QColorProfile;

    for (int i = 0; i <= (255 * 16); ++i) {
        cp->m_toLinear[i] = ushort(qRound(srgbToLinear(i / qreal(255 * 16)) * (255 * 256)));
        cp->m_fromLinear[i] = ushort(qRound(linearToSrgb(i / qreal(255 * 16)) * (255 * 256)));
    }

    return cp;
}

QT_END_NAMESPACE
