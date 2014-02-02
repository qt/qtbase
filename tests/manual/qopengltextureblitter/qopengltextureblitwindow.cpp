/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopengltextureblitwindow.h"

#include <QtGui/QPainter>
#include <QtGui/QOpenGLTexture>
#include <QtGui/QMatrix4x4>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

QOpenGLTextureBlitWindow::QOpenGLTextureBlitWindow()
    : QWindow()
    , m_context(new QOpenGLContext(this))
{
    resize(500,500);
    setSurfaceType(OpenGLSurface);
    QSurfaceFormat surfaceFormat = format();
    if (QCoreApplication::arguments().contains(QStringLiteral("-coreprofile"))) {
        surfaceFormat.setVersion(3,2);
        surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    }

    setFormat(surfaceFormat);
    create();
    m_context->setFormat(surfaceFormat);
    m_context->create();

    m_context->makeCurrent(this);

    m_blitter.create();
}

void QOpenGLTextureBlitWindow::render()
{
    m_context->makeCurrent(this);

    QRect viewport(0,0,dWidth(),dHeight());
    glViewport(0,0,dWidth(), dHeight());

    glClearColor(0.f, .6f, .0f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QOpenGLTexture texture(m_image);
    texture.create();

    QRectF topLeft(QPointF(0,0), QPointF(dWidth()/2.0, dHeight()/2.0));
    QRectF topRight(QPointF(dWidth()/2.0,0), QPointF(dWidth(), dHeight()/2.0));
    QRectF bottomLeft(QPointF(0, dHeight()/2.0), QPointF(dWidth() /2.0, dHeight()));
    QRectF bottomRight(QPoint(dWidth()/2.0, dHeight()/2.0), QPointF(dWidth(), dHeight()));

    QOpenGLTextureBlitter::Origin topLeftOrigin = QOpenGLTextureBlitter::OriginTopLeft;
    QOpenGLTextureBlitter::Origin bottomLeftOrigin = QOpenGLTextureBlitter::OriginBottomLeft;
    QMatrix4x4 topRightVertexFlipped = QOpenGLTextureBlitter::targetTransform(topRight, viewport, topLeftOrigin);
    QMatrix4x4 bottomLeftVertex = QOpenGLTextureBlitter::targetTransform(bottomLeft, viewport, topLeftOrigin);
    QMatrix4x4 bottomRightVertexFlipped = QOpenGLTextureBlitter::targetTransform(bottomRight, viewport, topLeftOrigin);

    QMatrix3x3 texTopLeft = QOpenGLTextureBlitter::sourceTransform(topLeft, m_image.size(), topLeftOrigin);
    QMatrix3x3 texTopRight = QOpenGLTextureBlitter::sourceTransform(topRight, m_image.size(), topLeftOrigin);
    QMatrix3x3 texBottomLeft = QOpenGLTextureBlitter::sourceTransform(bottomLeft, m_image.size(), topLeftOrigin);
    QMatrix3x3 texBottomRight = QOpenGLTextureBlitter::sourceTransform(bottomRight, m_image.size(), bottomLeftOrigin);

    QSizeF subSize(topLeft.width()/2, topLeft.height()/2);
    QRectF subTopLeft(topLeft.topLeft(), subSize);
    QRectF subTopRight(QPointF(topLeft.topLeft().x() + topLeft.width() / 2, topLeft.topLeft().y()),subSize);
    QRectF subBottomLeft(QPointF(topLeft.topLeft().x(), topLeft.topLeft().y() + topLeft.height() / 2), subSize);
    QRectF subBottomRight(QPointF(topLeft.topLeft().x() + topLeft.width() / 2, topLeft.topLeft().y() + topLeft.height() / 2), subSize);

    QMatrix4x4 subTopLeftVertex = QOpenGLTextureBlitter::targetTransform(subTopLeft, viewport, topLeftOrigin);
    QMatrix4x4 subTopRightVertex = QOpenGLTextureBlitter::targetTransform(subTopRight, viewport, topLeftOrigin);
    QMatrix4x4 subBottomLeftVertex = QOpenGLTextureBlitter::targetTransform(subBottomLeft, viewport, topLeftOrigin);
    QMatrix4x4 subBottomRightVertex = QOpenGLTextureBlitter::targetTransform(subBottomRight, viewport, topLeftOrigin);

    m_blitter.bind();
    m_blitter.blit(texture.textureId(), subTopLeftVertex, texBottomRight);
    m_blitter.blit(texture.textureId(), subTopRightVertex, texBottomLeft);
    m_blitter.blit(texture.textureId(), subBottomLeftVertex, texTopRight);
    m_blitter.blit(texture.textureId(), subBottomRightVertex, texTopLeft);

    m_blitter.blit(texture.textureId(), topRightVertexFlipped, topLeftOrigin);
    m_blitter.blit(texture.textureId(), bottomLeftVertex, bottomLeftOrigin);

    m_blitter.setSwizzleRB(true);
    m_blitter.blit(texture.textureId(), bottomRightVertexFlipped, texTopLeft);
    m_blitter.setSwizzleRB(false);
    m_blitter.release();

    m_context->swapBuffers(this);
}


void QOpenGLTextureBlitWindow::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);
    render();
}

void QOpenGLTextureBlitWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_image = QImage(size() * devicePixelRatio(), QImage::Format_ARGB32_Premultiplied);

    m_image.fill(Qt::gray);

    QPainter p(&m_image);

    QPen pen(Qt::red);
    pen.setWidth(5);
    p.setPen(pen);

    QFont font = p.font();
    font.setPixelSize(qMin(m_image.height(), m_image.width()) / 20);
    p.setFont(font);

    int dx = dWidth() / 5;
    int dy = dHeight() / 5;
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            QRect textRect(x * dx, y*dy, dx,dy);
            QString text = QString("[%1,%2]").arg(x).arg(y);
            p.drawText(textRect,text);
        }
    }

    p.drawRect(QRectF(2.5,2.5,dWidth() - 5, dHeight() - 5));

}

