// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpaintdevicewindow_p.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

QPaintDeviceWindowPrivate::QPaintDeviceWindowPrivate()
    = default;

QPaintDeviceWindowPrivate::~QPaintDeviceWindowPrivate()
    = default;

/*!
    \class QPaintDeviceWindow
    \inmodule QtGui
    \since 5.4
    \brief Convenience subclass of QWindow that is also a QPaintDevice.

    QPaintDeviceWindow is like a regular QWindow, with the added functionality
    of being a paint device too. Whenever the content needs to be updated,
    the virtual paintEvent() function is called. Subclasses, that reimplement
    this function, can then simply open a QPainter on the window.

    \note This class cannot directly be used in applications. It rather serves
    as a base for subclasses like QOpenGLWindow.

    \sa QOpenGLWindow
*/

/*!
    Marks the entire window as dirty and schedules a repaint.

    \note Subsequent calls to this function before the next paint
    event will get ignored.

    \note For non-exposed windows the update is deferred until the
    window becomes exposed again.
*/
void QPaintDeviceWindow::update()
{
    update(QRect(QPoint(0,0), size()));
}

/*!
    Marks the \a rect of the window as dirty and schedules a repaint.

    \note Subsequent calls to this function before the next paint
    event will get ignored, but \a rect is added to the region to update.

    \note For non-exposed windows the update is deferred until the
    window becomes exposed again.
*/
void QPaintDeviceWindow::update(const QRect &rect)
{
    Q_D(QPaintDeviceWindow);
    d->dirtyRegion += rect;
    if (isExposed())
        requestUpdate();
}

/*!
    Marks the \a region of the window as dirty and schedules a repaint.

    \note Subsequent calls to this function before the next paint
    event will get ignored, but \a region is added to the region to update.

    \note For non-exposed windows the update is deferred until the
    window becomes exposed again.
*/
void QPaintDeviceWindow::update(const QRegion &region)
{
    Q_D(QPaintDeviceWindow);
    d->dirtyRegion += region;
    if (isExposed())
        requestUpdate();
}

/*!
    Handles paint events passed in the \a event parameter.

    The default implementation does nothing. Reimplement this function to
    perform painting. If necessary, the dirty area is retrievable from
    the \a event.
*/
void QPaintDeviceWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // Do nothing
}

/*!
  \internal
 */
int QPaintDeviceWindow::metric(PaintDeviceMetric metric) const
{
    QScreen *screen = this->screen();
    if (!screen && QGuiApplication::primaryScreen())
        screen = QGuiApplication::primaryScreen();

    switch (metric) {
    case PdmWidth:
        return width();
    case PdmWidthMM:
        if (screen)
            return width() * screen->physicalSize().width() / screen->geometry().width();
        break;
    case PdmHeight:
        return height();
    case PdmHeightMM:
        if (screen)
            return height() * screen->physicalSize().height() / screen->geometry().height();
        break;
    case PdmDpiX:
        if (screen)
            return qRound(screen->logicalDotsPerInchX());
        break;
    case PdmDpiY:
        if (screen)
            return qRound(screen->logicalDotsPerInchY());
        break;
    case PdmPhysicalDpiX:
        if (screen)
            return qRound(screen->physicalDotsPerInchX());
        break;
    case PdmPhysicalDpiY:
        if (screen)
            return qRound(screen->physicalDotsPerInchY());
        break;
    case PdmDevicePixelRatio:
        return int(QWindow::devicePixelRatio());
        break;
    case PdmDevicePixelRatioScaled:
        return int(QWindow::devicePixelRatio() * devicePixelRatioFScale());
        break;
    case PdmDevicePixelRatioF_EncodedA:
        Q_FALLTHROUGH();
    case PdmDevicePixelRatioF_EncodedB:
        return QPaintDevice::encodeMetricF(metric, QWindow::devicePixelRatio());
        break;
    default:
        break;
    }

    return QPaintDevice::metric(metric);
}

/*!
  \internal
 */
void QPaintDeviceWindow::exposeEvent(QExposeEvent *exposeEvent)
{
    QWindow::exposeEvent(exposeEvent);
}

/*!
  \internal
 */
bool QPaintDeviceWindow::event(QEvent *event)
{
    Q_D(QPaintDeviceWindow);

    if (event->type() == QEvent::UpdateRequest) {
        if (handle()) // platform window may be gone when the window is closed during app exit
            d->handleUpdateEvent();
        return true;
    } else if (event->type() == QEvent::Paint) {
        d->markWindowAsDirty();
        // Do not rely on exposeEvent->region() as it has some issues for the
        // time being, namely that it is sometimes in local coordinates,
        // sometimes relative to the parent, depending on the platform plugin.
        // We require local coords here.
        auto region = QRect(QPoint(0, 0), size());
        d->doFlush(region); // Will end up calling paintEvent
        return true;
    } else if (event->type() == QEvent::Resize) {
        d->handleResizeEvent();
    }

    return QWindow::event(event);
}

/*!
  \internal
 */
QPaintDeviceWindow::QPaintDeviceWindow(QPaintDeviceWindowPrivate &dd, QWindow *parent)
    : QWindow(dd, parent)
{
}

/*!
  \internal
 */
QPaintEngine *QPaintDeviceWindow::paintEngine() const
{
    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qpaintdevicewindow.cpp"
