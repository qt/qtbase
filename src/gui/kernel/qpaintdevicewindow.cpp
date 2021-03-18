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

#include "qpaintdevicewindow_p.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

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
    Q_UNUSED(exposeEvent);
    Q_D(QPaintDeviceWindow);
    if (isExposed()) {
        d->markWindowAsDirty();
        // Do not rely on exposeEvent->region() as it has some issues for the
        // time being, namely that it is sometimes in local coordinates,
        // sometimes relative to the parent, depending on the platform plugin.
        // We require local coords here.
        d->doFlush(QRect(QPoint(0, 0), size()));
    }
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
