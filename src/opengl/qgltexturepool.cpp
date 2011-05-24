/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgltexturepool_p.h"
#include "qpixmapdata_gl_p.h"

QT_BEGIN_NAMESPACE

Q_OPENGL_EXPORT extern QGLWidget* qt_gl_share_widget();

static QGLTexturePool *qt_gl_texture_pool = 0;

class QGLTexturePoolPrivate
{
public:
    QGLTexturePoolPrivate() : lruFirst(0), lruLast(0) {}

    QGLPixmapData *lruFirst;
    QGLPixmapData *lruLast;
};

QGLTexturePool::QGLTexturePool()
    : d_ptr(new QGLTexturePoolPrivate())
{
}

QGLTexturePool::~QGLTexturePool()
{
}

QGLTexturePool *QGLTexturePool::instance()
{
    if (!qt_gl_texture_pool)
        qt_gl_texture_pool = new QGLTexturePool();
    return qt_gl_texture_pool;
}

GLuint QGLTexturePool::createTextureForPixmap(GLenum target,
                                            GLint level,
                                            GLint internalformat,
                                            GLsizei width,
                                            GLsizei height,
                                            GLenum format,
                                            GLenum type,
                                            QGLPixmapData *data)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(target, texture);
    do {
        glTexImage2D(target, level, internalformat, width, height, 0, format, type, 0);
        GLenum error = glGetError();
        if (error == GL_NO_ERROR) {
            if (data)
                moveToHeadOfLRU(data);
            return texture;
        } else if (error != GL_OUT_OF_MEMORY) {
            qWarning("QGLTexturePool: cannot create temporary texture because of invalid params");
            return 0;
        }
    } while (reclaimSpace(internalformat, width, height, format, type, data));
    qWarning("QGLTexturePool: cannot reclaim sufficient space for a %dx%d pixmap",
             width, height);
    return 0;
}

bool QGLTexturePool::createPermanentTexture(GLuint texture,
                                            GLenum target,
                                            GLint level,
                                            GLint internalformat,
                                            GLsizei width,
                                            GLsizei height,
                                            GLenum format,
                                            GLenum type,
                                            const GLvoid *data)
{
    glBindTexture(target, texture);
    do {
        glTexImage2D(target, level, internalformat, width, height, 0, format, type, data);

        GLenum error = glGetError();
        if (error == GL_NO_ERROR) {
            return true;
        } else if (error != GL_OUT_OF_MEMORY) {
            qWarning("QGLTexturePool: cannot create permanent texture because of invalid params");
            return false;
        }
    } while (reclaimSpace(internalformat, width, height, format, type, 0));
    qWarning("QGLTexturePool: cannot reclaim sufficient space for a %dx%d pixmap",
             width, height);
    return 0;
}

void QGLTexturePool::releaseTexture(QGLPixmapData *data, GLuint texture)
{
    // Very simple strategy at the moment: just destroy the texture.
    if (data)
        removeFromLRU(data);

    QGLWidget *shareWidget = qt_gl_share_widget();
    if (shareWidget) {
        QGLShareContextScope ctx(shareWidget->context());
        glDeleteTextures(1, &texture);
    }
}

void QGLTexturePool::useTexture(QGLPixmapData *data)
{
    moveToHeadOfLRU(data);
}

void QGLTexturePool::detachTexture(QGLPixmapData *data)
{
    removeFromLRU(data);
}

bool QGLTexturePool::reclaimSpace(GLint internalformat,
                                    GLsizei width,
                                    GLsizei height,
                                    GLenum format,
                                    GLenum type,
                                    QGLPixmapData *data)
{
    Q_UNUSED(internalformat);   // For future use in picking the best texture to eject.
    Q_UNUSED(width);
    Q_UNUSED(height);
    Q_UNUSED(format);
    Q_UNUSED(type);

    bool succeeded = false;
    bool wasInLRU = false;
    if (data) {
        wasInLRU = data->inLRU;
        moveToHeadOfLRU(data);
    }

    QGLPixmapData *lrudata = pixmapLRU();
    if (lrudata && lrudata != data) {
        lrudata->reclaimTexture();
        succeeded = true;
    }

    if (data && !wasInLRU)
        removeFromLRU(data);

    return succeeded;
}

void QGLTexturePool::hibernate()
{
    Q_D(QGLTexturePool);
    QGLPixmapData *pd = d->lruLast;
    while (pd) {
        QGLPixmapData *prevLRU = pd->prevLRU;
        pd->inTexturePool = false;
        pd->inLRU = false;
        pd->nextLRU = 0;
        pd->prevLRU = 0;
        pd->hibernate();
        pd = prevLRU;
    }
    d->lruFirst = 0;
    d->lruLast = 0;
}

void QGLTexturePool::moveToHeadOfLRU(QGLPixmapData *data)
{
    Q_D(QGLTexturePool);
    if (data->inLRU) {
        if (!data->prevLRU)
            return;     // Already at the head of the list.
        removeFromLRU(data);
    }
    data->inLRU = true;
    data->nextLRU = d->lruFirst;
    data->prevLRU = 0;
    if (d->lruFirst)
        d->lruFirst->prevLRU = data;
    else
        d->lruLast = data;
    d->lruFirst = data;
}

void QGLTexturePool::removeFromLRU(QGLPixmapData *data)
{
    Q_D(QGLTexturePool);
    if (!data->inLRU)
        return;
    if (data->nextLRU)
        data->nextLRU->prevLRU = data->prevLRU;
    else
        d->lruLast = data->prevLRU;
    if (data->prevLRU)
        data->prevLRU->nextLRU = data->nextLRU;
    else
        d->lruFirst = data->nextLRU;
    data->inLRU = false;
}

QGLPixmapData *QGLTexturePool::pixmapLRU()
{
    Q_D(QGLTexturePool);
    return d->lruLast;
}

QT_END_NAMESPACE
