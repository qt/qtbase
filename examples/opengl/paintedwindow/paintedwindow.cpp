/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

    m_animation = new QPropertyAnimation(this, "rotation");
    m_animation->setStartValue(qreal(0));
    m_animation->setEndValue(qreal(1));
    m_animation->setDuration(500);

    setOrientation(QGuiApplication::primaryScreen()->primaryOrientation());
    m_rotation = 0;

    m_targetOrientation = orientation();
    m_nextTargetOrientation = Qt::UnknownOrientation;

    connect(screen(), SIGNAL(currentOrientationChanged(Qt::ScreenOrientation)), this, SLOT(orientationChanged(Qt::ScreenOrientation)));
    connect(m_animation, SIGNAL(finished()), this, SLOT(rotationDone()));
    connect(this, SIGNAL(rotationChanged(qreal)), this, SLOT(paint()));
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
    switch (o) {
    case Qt::LandscapeOrientation:
        orientationChanged(Qt::PortraitOrientation);
        break;
    case Qt::PortraitOrientation:
        orientationChanged(Qt::InvertedLandscapeOrientation);
        break;
    case Qt::InvertedLandscapeOrientation:
        orientationChanged(Qt::InvertedPortraitOrientation);
        break;
    case Qt::InvertedPortraitOrientation:
        orientationChanged(Qt::LandscapeOrientation);
        break;
    default:
        Q_ASSERT(false);
    }

    paint();
}

void PaintedWindow::orientationChanged(Qt::ScreenOrientation newOrientation)
{
    if (orientation() == newOrientation)
        return;

    if (m_animation->state() == QAbstractAnimation::Running) {
        m_nextTargetOrientation = newOrientation;
        return;
    }

    Qt::ScreenOrientation screenOrientation = screen()->primaryOrientation();

    QRect rect(0, 0, width(), height());

    m_prevImage = QImage(width(), height(), QImage::Format_ARGB32_Premultiplied);
    m_nextImage = QImage(width(), height(), QImage::Format_ARGB32_Premultiplied);
    m_prevImage.fill(0);
    m_nextImage.fill(0);

    QPainter p;
    p.begin(&m_prevImage);
    p.setTransform(QScreen::transformBetween(orientation(), screenOrientation, rect));
    paint(&p, QScreen::mapBetween(orientation(), screenOrientation, rect));
    p.end();

    p.begin(&m_nextImage);
    p.setTransform(QScreen::transformBetween(newOrientation, screenOrientation, rect));
    paint(&p, QScreen::mapBetween(newOrientation, screenOrientation, rect));
    p.end();

    m_deltaRotation = QScreen::angleBetween(newOrientation, orientation());
    if (m_deltaRotation > 180)
        m_deltaRotation = 180 - m_deltaRotation;

    m_targetOrientation = newOrientation;
    m_animation->start();
}

void PaintedWindow::rotationDone()
{
    setOrientation(m_targetOrientation);
    if (m_nextTargetOrientation != Qt::UnknownOrientation) {
        Q_ASSERT(m_animation->state() != QAbstractAnimation::Running);
        orientationChanged(m_nextTargetOrientation);
        m_nextTargetOrientation = Qt::UnknownOrientation;
    }
}

void PaintedWindow::setRotation(qreal r)
{
    if (r != m_rotation) {
        m_rotation = r;
        emit rotationChanged(r);
    }
}

void PaintedWindow::paint()
{
    m_context->makeCurrent(this);

    Qt::ScreenOrientation screenOrientation = screen()->primaryOrientation();
    Qt::ScreenOrientation appOrientation = orientation();

    QRect rect(0, 0, width(), height());

    QOpenGLPaintDevice device(size());
    QPainter painter(&device);

    QPainterPath path;
    path.addEllipse(rect);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect, Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.fillPath(path, Qt::blue);

    if (orientation() != m_targetOrientation) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.save();
        painter.translate(width() / 2, height() / 2);
        painter.rotate(m_deltaRotation * m_rotation);
        painter.translate(-width() / 2, -height() / 2);
        painter.drawImage(0, 0, m_prevImage);
        painter.restore();
        painter.translate(width() / 2, height() / 2);
        painter.rotate(m_deltaRotation * m_rotation - m_deltaRotation);
        painter.translate(-width() / 2, -height() / 2);
        painter.setOpacity(m_rotation);
        painter.drawImage(0, 0, m_nextImage);
    } else {
        QRect mapped = QScreen::mapBetween(appOrientation, screenOrientation, rect);

        painter.setTransform(QScreen::transformBetween(appOrientation, screenOrientation, rect));
        paint(&painter, mapped);
        painter.end();
    }

    m_context->swapBuffers(this);
}

void PaintedWindow::paint(QPainter *painter, const QRect &rect)
{
    painter->setRenderHint(QPainter::Antialiasing);
    QFont font;
    font.setPixelSize(64);
    painter->setFont(font);
    painter->drawText(rect, Qt::AlignCenter, "Hello");
}
