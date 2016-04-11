/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSDIRECT2DHELPERS_H
#define QWINDOWSDIRECT2DHELPERS_H

#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtCore/QPointF>
#include <QtGui/QColor>
#include <QtGui/QTransform>

#include <d2d1_1helper.h>

QT_BEGIN_NAMESPACE

inline D2D1_RECT_U to_d2d_rect_u(const QRect &qrect)
{
    return D2D1::RectU(qrect.x(), qrect.y(), qrect.x() + qrect.width(), qrect.y() + qrect.height());
}

inline D2D1_RECT_F to_d2d_rect_f(const QRectF &qrect)
{
    return D2D1::RectF(qrect.x(), qrect.y(), qrect.x() + qrect.width(), qrect.y() + qrect.height());
}

inline D2D1_SIZE_U to_d2d_size_u(const QSizeF &qsize)
{
    return D2D1::SizeU(qsize.width(), qsize.height());
}

inline D2D1_SIZE_U to_d2d_size_u(const QSize &qsize)
{
    return D2D1::SizeU(qsize.width(), qsize.height());
}

inline D2D1_POINT_2F to_d2d_point_2f(const QPointF &qpoint)
{
    return D2D1::Point2F(qpoint.x(), qpoint.y());
}

inline D2D1::ColorF to_d2d_color_f(const QColor &c)
{
    return D2D1::ColorF(c.redF(), c.greenF(), c.blueF(), c.alphaF());
}

inline D2D1_MATRIX_3X2_F to_d2d_matrix_3x2_f(const QTransform &transform)
{
    return D2D1::Matrix3x2F(transform.m11(), transform.m12(),
                            transform.m21(), transform.m22(),
                            transform.m31(), transform.m32());
}

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DHELPERS_H
