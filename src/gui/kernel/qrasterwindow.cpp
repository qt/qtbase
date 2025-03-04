// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    void handleResizeEvent() override
    {
        Q_Q(QRasterWindow);
        if (backingstore->size() != q->size())
            markWindowAsDirty();
    }

    void beginPaint(const QRegion &region) override
    {
        Q_Q(QRasterWindow);
        const QSize size = q->size();
        if (backingstore->size() != size)
            backingstore->resize(size);

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

void QRasterWindow::resizeEvent(QResizeEvent *)
{
}

QT_END_NAMESPACE

#include "moc_qrasterwindow.cpp"
