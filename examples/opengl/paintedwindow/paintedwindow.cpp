/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
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
**
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
    setFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setFormat(format);

    create();

    m_context = new QOpenGLContext(this);
    m_context->setFormat(format);
    m_context->create();

    m_animation = new QPropertyAnimation(this, "rotation");
    m_animation->setStartValue(qreal(0));
    m_animation->setEndValue(qreal(1));
    m_animation->setDuration(500);

    QRect screenGeometry = screen()->availableGeometry();

    QPoint center = screenGeometry.center();
    QRect windowRect = screen()->isLandscape(screen()->orientation()) ? QRect(0, 0, 640, 480) : QRect(0, 0, 480, 640);
    setGeometry(QRect(center - windowRect.center(), windowRect.size()));

    m_rotation = 0;

    reportContentOrientationChange(screen()->orientation());

    m_targetOrientation = contentOrientation();
    m_nextTargetOrientation = Qt::PrimaryOrientation;

    connect(screen(), &QScreen::orientationChanged, this, &PaintedWindow::orientationChanged);
    connect(m_animation, &QAbstractAnimation::finished, this, &PaintedWindow::rotationDone);
    typedef void (PaintedWindow::*PaintedWindowVoidSlot)();
    connect(this, &PaintedWindow::rotationChanged,
            this, static_cast<PaintedWindowVoidSlot>(&PaintedWindow::paint));
}

void PaintedWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed())
        paint();
}

void PaintedWindow::mousePressEvent(QMouseEvent *)
{
    Qt::ScreenOrientation o = contentOrientation();
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
    if (contentOrientation() == newOrientation)
        return;

    if (m_animation->state() == QAbstractAnimation::Running) {
        m_nextTargetOrientation = newOrientation;
        return;
    }

    QRect rect(0, 0, width(), height());

    m_prevImage = QImage(width(), height(), QImage::Format_ARGB32_Premultiplied);
    m_nextImage = QImage(width(), height(), QImage::Format_ARGB32_Premultiplied);
    m_prevImage.fill(0);
    m_nextImage.fill(0);

    QPainter p;
    p.begin(&m_prevImage);
    p.setTransform(screen()->transformBetween(contentOrientation(), screen()->orientation(), rect));
    paint(&p, screen()->mapBetween(contentOrientation(), screen()->orientation(), rect));
    p.end();

    p.begin(&m_nextImage);
    p.setTransform(screen()->transformBetween(newOrientation, screen()->orientation(), rect));
    paint(&p, screen()->mapBetween(newOrientation, screen()->orientation(), rect));
    p.end();

    m_deltaRotation = screen()->angleBetween(newOrientation, contentOrientation());
    if (m_deltaRotation > 180)
        m_deltaRotation = 180 - m_deltaRotation;

    m_targetOrientation = newOrientation;
    m_animation->start();
}

void PaintedWindow::rotationDone()
{
    reportContentOrientationChange(m_targetOrientation);
    if (m_nextTargetOrientation != Qt::PrimaryOrientation) {
        Q_ASSERT(m_animation->state() != QAbstractAnimation::Running);
        orientationChanged(m_nextTargetOrientation);
        m_nextTargetOrientation = Qt::PrimaryOrientation;
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

    QRect rect(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());

    QOpenGLPaintDevice device(size() * devicePixelRatio());
    QPainter painter(&device);

    QPainterPath path;
    path.addEllipse(rect);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect, Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.fillPath(path, Qt::blue);

    if (contentOrientation() != m_targetOrientation) {
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
        QRect mapped = screen()->mapBetween(contentOrientation(), screen()->orientation(), rect);

        painter.setTransform(screen()->transformBetween(contentOrientation(), screen()->orientation(), rect));
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
