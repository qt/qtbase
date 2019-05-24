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

#ifndef QWINDOWSDIRECT2DBITMAP_H
#define QWINDOWSDIRECT2DBITMAP_H

#include <QtCore/qnamespace.h>
#include <QtCore/qrect.h>
#include <QtCore/qscopedpointer.h>

struct ID2D1DeviceContext;
struct ID2D1Bitmap1;

QT_BEGIN_NAMESPACE

class QWindowsDirect2DDeviceContext;
class QWindowsDirect2DBitmapPrivate;

class QImage;
class QSize;
class QColor;

class QWindowsDirect2DBitmap
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DBitmap)
    Q_DISABLE_COPY_MOVE(QWindowsDirect2DBitmap)
public:
    QWindowsDirect2DBitmap();
    QWindowsDirect2DBitmap(ID2D1Bitmap1 *bitmap, ID2D1DeviceContext *dc);
    ~QWindowsDirect2DBitmap();

    bool resize(int width, int height);
    bool fromImage(const QImage &image, Qt::ImageConversionFlags flags);

    ID2D1Bitmap1* bitmap() const;
    QWindowsDirect2DDeviceContext* deviceContext() const;

    void fill(const QColor &color);
    QImage toImage(const QRect &rect = QRect());

    QSize size() const;

private:
    QScopedPointer<QWindowsDirect2DBitmapPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DBITMAP_H
