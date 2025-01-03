// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSDIRECT2DHELPERS_H
#define QWINDOWSDIRECT2DHELPERS_H

#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>
#include <QtGui/qcolor.h>
#include <QtGui/qtransform.h>

#ifdef Q_CC_MINGW
#  include <qt_windows.h>
#  include <d2d1.h>
#  include <d2d1helper.h>
#  include <d2dbasetypes.h>
#  include <d2d1_1.h>
#endif // Q_CC_MINGW
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

    return D2D1::SizeU(UINT32(qRound(qsize.width())),
                       UINT32(qRound(qsize.height())));
}

inline D2D1_SIZE_U to_d2d_size_u(const QSize &qsize)
{
    return D2D1::SizeU(UINT32(qsize.width()),
                       UINT32(qsize.height()));
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
