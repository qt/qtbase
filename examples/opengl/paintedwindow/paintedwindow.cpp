/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "paintedwindow.h"

#include <QGuiApplication>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QScreen>
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

void PaintedWindow::mousePressEvent(QMouseEvent *)
{
    Qt::ScreenOrientation o = orientation();
    if (o == Qt::UnknownOrientation)
        o = QGuiApplication::primaryScreen()->primaryOrientation();

    switch (o) {
    case Qt::LandscapeOrientation:
        setOrientation(Qt::PortraitOrientation);
        break;
    case Qt::PortraitOrientation:
        setOrientation(Qt::InvertedLandscapeOrientation);
        break;
    case Qt::InvertedLandscapeOrientation:
        setOrientation(Qt::InvertedPortraitOrientation);
        break;
    case Qt::InvertedPortraitOrientation:
        setOrientation(Qt::LandscapeOrientation);
        break;
    default:
        Q_ASSERT(false);
    }

    paint();
}

void PaintedWindow::paint()
{
    m_context->makeCurrent(this);

    QOpenGLPaintDevice device(size());

    Qt::ScreenOrientation screenOrientation = QGuiApplication::primaryScreen()->primaryOrientation();
    Qt::ScreenOrientation appOrientation = orientation();

    QRect rect(0, 0, width(), height());
    QRect mapped = QScreen::mapBetween(appOrientation, screenOrientation, rect);

    QPainterPath path;
    path.addEllipse(mapped);

    QPainter painter(&device);
    painter.setTransform(QScreen::transformBetween(appOrientation, screenOrientation, rect));
    painter.fillRect(mapped, Qt::white);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillPath(path, Qt::blue);
    QFont font;
    font.setPixelSize(64);
    painter.setFont(font);
    painter.drawText(mapped, Qt::AlignCenter, "Hello");
    painter.end();

    m_context->swapBuffers(this);
}
