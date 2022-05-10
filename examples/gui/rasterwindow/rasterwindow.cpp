// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "rasterwindow.h"

//! [1]
RasterWindow::RasterWindow(QWindow *parent)
    : QWindow(parent)
    , m_backingStore(new QBackingStore(this))
{
    setGeometry(100, 100, 300, 200);
}
//! [1]


//! [7]
bool RasterWindow::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        renderNow();
        return true;
    }
    return QWindow::event(event);
}
//! [7]

//! [6]
void RasterWindow::renderLater()
{
    requestUpdate();
}
//! [6]


//! [5]
void RasterWindow::resizeEvent(QResizeEvent *resizeEvent)
{
    m_backingStore->resize(resizeEvent->size());
}
//! [5]

//! [2]
void RasterWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed())
        renderNow();
}
//! [2]


//! [3]
void RasterWindow::renderNow()
{
    if (!isExposed())
        return;

    QRect rect(0, 0, width(), height());
    m_backingStore->beginPaint(rect);

    QPaintDevice *device = m_backingStore->paintDevice();
    QPainter painter(device);

    painter.fillRect(0, 0, width(), height(), QGradient::NightFade);
    render(&painter);
    painter.end();

    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}
//! [3]

//! [4]
void RasterWindow::render(QPainter *painter)
{
    painter->drawText(QRectF(0, 0, width(), height()), Qt::AlignCenter, QStringLiteral("QWindow"));
}
//! [4]
