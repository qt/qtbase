#include "paintedwindow.h"

#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QTimer>

#include <qmath.h>

PaintedWindow::PaintedWindow()
{
    QSurfaceFormat format;
    format.setStencilBufferSize(8);
    format.setSamples(4);

    setSurfaceType(QWindow::OpenGLSurface);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setFormat(format);

    create();

    m_context = new QOpenGLContext(this);
    m_context->setFormat(format);
    m_context->create();
}

void PaintedWindow::resizeEvent(QResizeEvent *)
{
    paint();
}

void PaintedWindow::exposeEvent(QExposeEvent *)
{
    paint();
}

void PaintedWindow::paint()
{
    m_context->makeCurrent(this);

    QPainterPath path;
    path.addEllipse(0, 0, width(), height());

    QOpenGLPaintDevice device(size());

    QPainter painter(&device);
    painter.fillRect(0, 0, width(), height(), Qt::white);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillPath(path, Qt::blue);
    painter.end();

    m_context->swapBuffers(this);
}
