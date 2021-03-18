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

#include "qrasterwindow.h"

#include <QtGui/private/qpaintdevicewindow_p.h>

#include <QtGui/QBackingStore>
#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

/*!
  \class QRasterWindow
  \inmodule QtGui
  \since 5.4
  \brief QRasterWindow is a convenience class for using QPainter on a QWindow.

  QRasterWindow is a QWindow with a raster-based, non-OpenGL surface. On top of
  the functionality offered by QWindow, QRasterWindow adds a virtual
  paintEvent() function and the possibility to open a QPainter on itself. The
  underlying paint engine will be the raster one, meaning that all drawing will
  happen on the CPU. For performing accelerated, OpenGL-based drawing, use
  QOpenGLWindow instead.

  Internally the class is thin wrapper for QWindow and QBackingStore
  and is very similar to the \l{Raster Window Example}{Raster Window
  Example} that uses these classes directly.

  \sa QPaintDeviceWindow::paintEvent(), QPaintDeviceWindow::update()
*/

class QRasterWindowPrivate : public QPaintDeviceWindowPrivate
{
    Q_DECLARE_PUBLIC(QRasterWindow)
public:
    void beginPaint(const QRegion &region) override
    {
        Q_Q(QRasterWindow);
        const QSize size = q->size();
        if (backingstore->size() != size) {
            backingstore->resize(size);
            markWindowAsDirty();
        }
        backingstore->beginPaint(region);
    }

    void endPaint() override
    {
        backingstore->endPaint();
    }

    void flush(const QRegion &region) override
    {
        Q_Q(QRasterWindow);
        backingstore->flush(region, q);
    }

    QScopedPointer<QBackingStore> backingstore;
};

/*!
  Constructs a new QRasterWindow with \a parent.
*/
QRasterWindow::QRasterWindow(QWindow *parent)
    : QPaintDeviceWindow(*(new QRasterWindowPrivate), parent)
{
    setSurfaceType(QSurface::RasterSurface);
    d_func()->backingstore.reset(new QBackingStore(this));
}

QRasterWindow::~QRasterWindow()
{
  Q_D(QRasterWindow);
  // Delete backingstore while window is still alive, as it
  // might need to reference the window in the process
  d->backingstore.reset(nullptr);
}

/*!
  \internal
*/
int QRasterWindow::metric(PaintDeviceMetric metric) const
{
    Q_D(const QRasterWindow);

    switch (metric) {
    case PdmDepth:
        return d->backingstore->paintDevice()->depth();
    default:
        break;
    }
    return QPaintDeviceWindow::metric(metric);
}

/*!
  \internal
*/
QPaintDevice *QRasterWindow::redirected(QPoint *) const
{
    Q_D(const QRasterWindow);
    return d->backingstore->paintDevice();
}

QT_END_NAMESPACE
