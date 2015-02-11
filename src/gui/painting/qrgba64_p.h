/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QRGBA64_P_H
#define QRGBA64_P_H

#include <QtGui/qrgba64.h>
#include <QtGui/private/qdrawhelper_p.h>

QT_BEGIN_NAMESPACE

inline QRgba64 combineAlpha256(QRgba64 rgba64, uint alpha256)
{
    return QRgba64::fromRgba64(rgba64.red(), rgba64.green(), rgba64.blue(), (rgba64.alpha() * alpha256) >> 8);
}

inline QRgba64 multiplyAlpha256(QRgba64 rgba64, uint alpha256)
{
    return QRgba64::fromRgba64((rgba64.red()   * alpha256) >> 8,
                               (rgba64.green() * alpha256) >> 8,
                               (rgba64.blue()  * alpha256) >> 8,
                               (rgba64.alpha() * alpha256) >> 8);
}

inline QRgba64 multiplyAlpha255(QRgba64 rgba64, uint alpha255)
{
    return QRgba64::fromRgba64(qt_div_255(rgba64.red()   * alpha255),
                               qt_div_255(rgba64.green() * alpha255),
                               qt_div_255(rgba64.blue()  * alpha255),
                               qt_div_255(rgba64.alpha() * alpha255));
}

inline QRgba64 multiplyAlpha65535(QRgba64 rgba64, uint alpha65535)
{
    return QRgba64::fromRgba64(qt_div_65535(rgba64.red()   * alpha65535),
                               qt_div_65535(rgba64.green() * alpha65535),
                               qt_div_65535(rgba64.blue()  * alpha65535),
                               qt_div_65535(rgba64.alpha() * alpha65535));
}

inline QRgba64 interpolate256(QRgba64 x, uint alpha1, QRgba64 y, uint alpha2)
{
    return QRgba64::fromRgba64(multiplyAlpha256(x, alpha1) + multiplyAlpha256(y, alpha2));
}

inline QRgba64 interpolate255(QRgba64 x, uint alpha1, QRgba64 y, uint alpha2)
{
    return QRgba64::fromRgba64(multiplyAlpha255(x, alpha1) + multiplyAlpha255(y, alpha2));
}

QT_END_NAMESPACE

#endif // QRGBA64_P_H
