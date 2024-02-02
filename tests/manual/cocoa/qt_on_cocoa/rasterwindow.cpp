// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "rasterwindow.h"

//#include <private/qguiapplication_p.h>

#include <QBackingStore>
#include <QPainter>
#include <QtWidgets>

static int colorIndexId = 0;

QColor colorTable[] =
{
    QColor("#f09f8f"),
    QColor("#a2bff2"),
    QColor("#c0ef8f")
};

RasterWindow::RasterWindow(QRasterWindow *parent)
    : QRasterWindow(parent)
    , m_backgroundColorIndex(colorIndexId++)
{
    initialize();
}

void RasterWindow::initialize()
{
    create();
    m_backingStore = new QBackingStore(this);

    m_image = QImage(geometry().size(), QImage::Format_RGB32);
    m_image.fill(colorTable[m_backgroundColorIndex % (sizeof(colorTable) / sizeof(colorTable[0]))].rgba());

    m_lastPos = QPoint(-1, -1);
}

void RasterWindow::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
    unsetCursor();
}

void RasterWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_lastPos != QPoint(-1, -1)) {
        QPainter p(&m_image);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawLine(m_lastPos, event->pos());
        m_lastPos = event->pos();
    }

    scheduleRender();
}

void RasterWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_lastPos != QPoint(-1, -1)) {
        QPainter p(&m_image);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawLine(m_lastPos, event->pos());
        m_lastPos = QPoint(-1, -1);
    }

    scheduleRender();
}

void RasterWindow::exposeEvent(QExposeEvent *)
{
    render();
}

void RasterWindow::resizeEvent(QResizeEvent *)
{
    QImage old = m_image;

    //qDebug() << "RasterWindow::resizeEvent" << width << height;

    int width = qMax(geometry().width(), old.width());
    int height = qMax(geometry().height(), old.height());

    if (width > old.width() || height > old.height()) {
        m_image = QImage(width, height, QImage::Format_RGB32);
        m_image.fill(colorTable[(m_backgroundColorIndex) % (sizeof(colorTable) / sizeof(colorTable[0]))].rgba());

        QPainter p(&m_image);
        p.drawImage(0, 0, old);
    }

    render();
}

void RasterWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Backspace:
        m_text.chop(1);
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        m_text.append('\n');
        break;
    default:
        m_text.append(event->text());
        break;
    }
    scheduleRender();
}

void RasterWindow::scheduleRender()
{
    requestUpdate();
}

bool RasterWindow::event(QEvent *e)
{
    if (e->type() == QEvent::UpdateRequest)
        render();

    return QWindow::event(e);
}

void RasterWindow::render()
{
    if (!isExposed()) {
        qDebug() << "Skipping render, not exposed";
        return;
    }

    QRect rect(QPoint(), geometry().size());

    m_backingStore->resize(rect.size());

    m_backingStore->beginPaint(rect);

    QPaintDevice *device = m_backingStore->paintDevice();

    QPainter p(device);
    p.drawImage(0, 0, m_image);

    QFont font;
    font.setPixelSize(32);

    p.setFont(font);
    p.drawText(rect, 0, m_text);

    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}


