// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"

#include <private/qguiapplication_p.h>

#include <QBackingStore>
#include <QPainter>

static int colorIndexId = 0;

QColor colorTable[] =
{
    QColor("#f09f8f"),
    QColor("#a2bff2"),
    QColor("#c0ef8f")
};

Window::Window(QScreen *screen)
    : QWindow(screen)
    , m_backgroundColorIndex(colorIndexId++)
{
    initialize();
}

Window::Window(QWindow *parent)
    : QWindow(parent)
    , m_backgroundColorIndex(colorIndexId++)
{
    initialize();
}

void Window::initialize()
{
    if (parent())
        setGeometry(QRect(160, 120, 320, 240));
    else {
        setFlags(flags() | Qt::WindowTitleHint | Qt::WindowSystemMenuHint
                       | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
        const QSize baseSize = QSize(640, 480);
        setGeometry(QRect(geometry().topLeft(), baseSize));

        setSizeIncrement(QSize(10, 10));
        setBaseSize(baseSize);
        setMinimumSize(QSize(240, 160));
        setMaximumSize(QSize(800, 600));
    }

    create();
    m_backingStore = new QBackingStore(this);

    m_image = QImage(geometry().size(), QImage::Format_RGB32);
    m_image.fill(colorTable[m_backgroundColorIndex % (sizeof(colorTable) / sizeof(colorTable[0]))].rgba());

    m_lastPos = QPoint(-1, -1);
    m_renderTimer = 0;
}

void Window::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->position().toPoint();
}

void Window::mouseMoveEvent(QMouseEvent *event)
{
    if (m_lastPos != QPoint(-1, -1)) {
        QPainter p(&m_image);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawLine(m_lastPos, event->position().toPoint());
        m_lastPos = event->position().toPoint();

        scheduleRender();
    }
}

void Window::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_lastPos != QPoint(-1, -1)) {
        QPainter p(&m_image);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawLine(m_lastPos, event->position().toPoint());
        m_lastPos = QPoint(-1, -1);

        scheduleRender();
    }
}

void Window::exposeEvent(QExposeEvent *)
{
    scheduleRender();
}

void Window::resizeEvent(QResizeEvent *)
{
    QImage old = m_image;

    int width = qMax(geometry().width(), old.width());
    int height = qMax(geometry().height(), old.height());

    if (width > old.width() || height > old.height()) {
        m_image = QImage(width, height, QImage::Format_RGB32);
        m_image.fill(colorTable[(m_backgroundColorIndex) % (sizeof(colorTable) / sizeof(colorTable[0]))].rgba());

        QPainter p(&m_image);
        p.drawImage(0, 0, old);
    }
    scheduleRender();
}

void Window::keyPressEvent(QKeyEvent *event)
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

void Window::scheduleRender()
{
    if (!m_renderTimer)
        m_renderTimer = startTimer(1);
}

void Window::timerEvent(QTimerEvent *)
{
    if (isExposed())
        render();
    killTimer(m_renderTimer);
    m_renderTimer = 0;
}

void Window::render()
{
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


