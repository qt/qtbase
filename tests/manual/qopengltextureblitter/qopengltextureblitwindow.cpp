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
    texture.setWrapMode(QOpenGLTexture::ClampToEdge);
    texture.create();

    QOpenGLTexture texture_mirrored(m_image_mirrord);
    texture_mirrored.setWrapMode(QOpenGLTexture::ClampToEdge);
    texture_mirrored.create();

    QRectF topLeftOriginTopLeft(QPointF(0,0), QPointF(dWidth()/2.0, dHeight()/2.0));
    QRectF topRightOriginTopLeft(QPointF(dWidth()/2.0,0), QPointF(dWidth(), dHeight()/2.0));
    QRectF bottomLeftOriginTopLeft(QPointF(0, dHeight()/2.0), QPointF(dWidth() /2.0, dHeight()));
    QRectF bottomRightOriginTopLeft(QPoint(dWidth()/2.0, dHeight()/2.0), QPointF(dWidth(), dHeight()));

    QRectF topLeftOriginBottomLeft = bottomLeftOriginTopLeft; Q_UNUSED(topLeftOriginBottomLeft);
    QRectF topRightOriginBottomLeft = bottomRightOriginTopLeft; Q_UNUSED(topRightOriginBottomLeft);
    QRectF bottomLeftOriginBottomLeft = topLeftOriginTopLeft;
    QRectF bottomRightOriginBottomLeft = topRightOriginTopLeft;

    QOpenGLTextureBlitter::Origin topLeftOrigin = QOpenGLTextureBlitter::OriginTopLeft;
    QOpenGLTextureBlitter::Origin bottomLeftOrigin = QOpenGLTextureBlitter::OriginBottomLeft;

    QMatrix4x4 topRightOriginTopLeftVertex = QOpenGLTextureBlitter::targetTransform(topRightOriginTopLeft, viewport);
    QMatrix4x4 bottomLeftOriginTopLeftVertex = QOpenGLTextureBlitter::targetTransform(bottomLeftOriginTopLeft, viewport);
    QMatrix4x4 bottomRightOriginTopLeftVertex = QOpenGLTextureBlitter::targetTransform(bottomRightOriginTopLeft, viewport);

    QMatrix3x3 texTopLeftOriginTopLeft = QOpenGLTextureBlitter::sourceTransform(topLeftOriginTopLeft, m_image.size(), topLeftOrigin);
    QMatrix3x3 texTopRightOriginBottomLeft = QOpenGLTextureBlitter::sourceTransform(topRightOriginBottomLeft, m_image.size(), bottomLeftOrigin);
    QMatrix3x3 texBottomLeftOriginBottomLeft = QOpenGLTextureBlitter::sourceTransform(bottomLeftOriginBottomLeft, m_image.size(), bottomLeftOrigin);
    QMatrix3x3 texBottomRightOriginBottomLeft = QOpenGLTextureBlitter::sourceTransform(bottomRightOriginBottomLeft, m_image.size(), bottomLeftOrigin);

    QSizeF subSize(topLeftOriginTopLeft.width()/2, topLeftOriginTopLeft.height()/2);
    QRectF subTopLeftOriginTopLeft(topLeftOriginTopLeft.topLeft(), subSize);
    QRectF subTopRightOriginTopLeft(QPointF(topLeftOriginTopLeft.topLeft().x() + topLeftOriginTopLeft.width() / 2,
                               topLeftOriginTopLeft.topLeft().y()), subSize);
    QRectF subBottomLeftOriginTopLeft(QPointF(topLeftOriginTopLeft.topLeft().x(),
                                 topLeftOriginTopLeft.topLeft().y() + topLeftOriginTopLeft.height() / 2), subSize);
    QRectF subBottomRightOriginTopLeft(QPointF(topLeftOriginTopLeft.topLeft().x() + topLeftOriginTopLeft.width() / 2, 
                                  topLeftOriginTopLeft.topLeft().y() + topLeftOriginTopLeft.height() / 2), subSize);

    QMatrix4x4 subTopLeftOriginTopLeftVertex = QOpenGLTextureBlitter::targetTransform(subTopLeftOriginTopLeft, viewport);
    QMatrix4x4 subTopRightOriginTopLeftVertex = QOpenGLTextureBlitter::targetTransform(subTopRightOriginTopLeft, viewport);
    QMatrix4x4 subBottomLeftOriginTopLeftVertex = QOpenGLTextureBlitter::targetTransform(subBottomLeftOriginTopLeft, viewport);
    QMatrix4x4 subBottomRightOriginTopLeftVertex = QOpenGLTextureBlitter::targetTransform(subBottomRightOriginTopLeft, viewport);

    m_blitter.bind();
    m_blitter.blit(texture_mirrored.textureId(), subTopLeftOriginTopLeftVertex, texBottomRightOriginBottomLeft);
    m_blitter.blit(texture_mirrored.textureId(), subTopRightOriginTopLeftVertex, texBottomLeftOriginBottomLeft);
    m_blitter.blit(texture.textureId(), subBottomLeftOriginTopLeftVertex, texTopRightOriginBottomLeft);
    m_blitter.blit(texture.textureId(), subBottomRightOriginTopLeftVertex, texTopLeftOriginTopLeft);

    m_blitter.blit(texture.textureId(), topRightOriginTopLeftVertex, topLeftOrigin);
    m_blitter.blit(texture_mirrored.textureId(), bottomLeftOriginTopLeftVertex, topLeftOrigin);

    m_blitter.setSwizzleRB(true);
    m_blitter.blit(texture.textureId(), bottomRightOriginTopLeftVertex, texTopLeftOriginTopLeft);
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

    m_image_mirrord = m_image.mirrored(false,true);
}

