// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rasterwindow.h"

RasterWindow::RasterWindow(QWindow *parent)
    : QWindow(parent)
    , m_update_pending(false)
{
    winId();

    m_backingStore = new QBackingStore(this);
    create();

    setGeometry(100, 100, 300, 200);
}


bool RasterWindow::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        m_update_pending = false;
        renderNow();
        return true;
    }
    return QWindow::event(event);
}

void RasterWindow::renderLater()
{
    if (!m_update_pending) {
        m_update_pending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

void RasterWindow::resizeEvent(QResizeEvent *resizeEvent)
{
    m_backingStore->resize(resizeEvent->size());
    if (isExposed())
        renderNow();
}

void RasterWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed()) {
        renderNow();
    }
}

void RasterWindow::renderNow()
{
    if (!isExposed())
        return;

    QRect rect(0, 0, width(), height());
    m_backingStore->beginPaint(rect);

    QPaintDevice *device = m_backingStore->paintDevice();
    QPainter painter(device);

    painter.fillRect(0, 0, width(), height(), Qt::white);
    render(&painter);

    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}

void RasterWindow::render(QPainter *painter)
{
    painter->drawText(QRectF(0, 0, width(), height()), Qt::AlignCenter, QStringLiteral("QWindow"));
}
