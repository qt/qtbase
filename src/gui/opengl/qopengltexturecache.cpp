/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopengltexturecache_p.h"
#include "qopengltextureuploader_p.h"
#include <qmath.h>
#include <qopenglfunctions.h>
#include <private/qimagepixmapcleanuphooks_p.h>
#include <qpa/qplatformpixmap.h>

#include <qtgui_tracepoints_p.h>

QT_BEGIN_NAMESPACE

class QOpenGLTextureCacheWrapper
{
public:
    QOpenGLTextureCacheWrapper()
    {
        QImagePixmapCleanupHooks::instance()->addPlatformPixmapModificationHook(cleanupTexturesForPixmapData);
        QImagePixmapCleanupHooks::instance()->addPlatformPixmapDestructionHook(cleanupTexturesForPixmapData);
        QImagePixmapCleanupHooks::instance()->addImageHook(cleanupTexturesForCacheKey);
    }

    ~QOpenGLTextureCacheWrapper()
    {
        QImagePixmapCleanupHooks::instance()->removePlatformPixmapModificationHook(cleanupTexturesForPixmapData);
        QImagePixmapCleanupHooks::instance()->removePlatformPixmapDestructionHook(cleanupTexturesForPixmapData);
        QImagePixmapCleanupHooks::instance()->removeImageHook(cleanupTexturesForCacheKey);
    }

    QOpenGLTextureCache *cacheForContext(QOpenGLContext *context) {
        QMutexLocker lock(&m_mutex);
        return m_resource.value<QOpenGLTextureCache>(context);
    }

    static void cleanupTexturesForCacheKey(qint64 key);
    static void cleanupTexturesForPixmapData(QPlatformPixmap *pmd);

private:
    QOpenGLMultiGroupSharedResource m_resource;
    QMutex m_mutex;
};

Q_GLOBAL_STATIC(QOpenGLTextureCacheWrapper, qt_texture_caches)

QOpenGLTextureCache *QOpenGLTextureCache::cacheForContext(QOpenGLContext *context)
{
    return qt_texture_caches()->cacheForContext(context);
}

void QOpenGLTextureCacheWrapper::cleanupTexturesForCacheKey(qint64 key)
{
    QList<QOpenGLSharedResource *> resources = qt_texture_caches()->m_resource.resources();
    for (QList<QOpenGLSharedResource *>::iterator it = resources.begin(); it != resources.end(); ++it)
        static_cast<QOpenGLTextureCache *>(*it)->invalidate(key);
}

void QOpenGLTextureCacheWrapper::cleanupTexturesForPixmapData(QPlatformPixmap *pmd)
{
    cleanupTexturesForCacheKey(pmd->cacheKey());
}

QOpenGLTextureCache::QOpenGLTextureCache(QOpenGLContext *ctx)
    : QOpenGLSharedResource(ctx->shareGroup())
    , m_cache(256 * 1024) // 256 MB cache
{
}

QOpenGLTextureCache::~QOpenGLTextureCache()
{
}

GLuint QOpenGLTextureCache::bindTexture(QOpenGLContext *context, const QPixmap &pixmap, QOpenGLTextureUploader::BindOptions options)
{
    if (pixmap.isNull())
        return 0;
    QMutexLocker locker(&m_mutex);
    qint64 key = pixmap.cacheKey();

    // A QPainter is active on the image - take the safe route and replace the texture.
    if (!pixmap.paintingActive()) {
        QOpenGLCachedTexture *entry = m_cache.object(key);
        if (entry && entry->options() == options) {
            context->functions()->glBindTexture(GL_TEXTURE_2D, entry->id());
            return entry->id();
        }
    }

    GLuint id = bindTexture(context, key, pixmap.toImage(), options);
    if (id > 0)
        QImagePixmapCleanupHooks::enableCleanupHooks(pixmap);

    return id;
}

GLuint QOpenGLTextureCache::bindTexture(QOpenGLContext *context, const QImage &image, QOpenGLTextureUploader::BindOptions options)
{
    if (image.isNull())
        return 0;
    QMutexLocker locker(&m_mutex);
    qint64 key = image.cacheKey();

    // A QPainter is active on the image - take the safe route and replace the texture.
    if (!image.paintingActive()) {
        QOpenGLCachedTexture *entry = m_cache.object(key);
        if (entry && entry->options() == options) {
            context->functions()->glBindTexture(GL_TEXTURE_2D, entry->id());
            return entry->id();
        }
    }

    QImage img = image;
    if (!context->functions()->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures))
        options |= QOpenGLTextureUploader::PowerOfTwoBindOption;

    GLuint id = bindTexture(context, key, img, options);
    if (id > 0)
        QImagePixmapCleanupHooks::enableCleanupHooks(image);

    return id;
}

GLuint QOpenGLTextureCache::bindTexture(QOpenGLContext *context, qint64 key, const QImage &image, QOpenGLTextureUploader::BindOptions options)
{
    Q_TRACE_SCOPE(QOpenGLTextureCache_bindTexture, context, key, image, options);

    GLuint id;
    QOpenGLFunctions *funcs = context->functions();
    funcs->glGenTextures(1, &id);
    funcs->glBindTexture(GL_TEXTURE_2D, id);

    int cost = QOpenGLTextureUploader::textureImage(GL_TEXTURE_2D, image, options);

    m_cache.insert(key, new QOpenGLCachedTexture(id, options, context), cost / 1024);

    return id;
}

void QOpenGLTextureCache::invalidate(qint64 key)
{
    QMutexLocker locker(&m_mutex);
    m_cache.remove(key);
}

void QOpenGLTextureCache::invalidateResource()
{
    m_cache.clear();
}

void QOpenGLTextureCache::freeResource(QOpenGLContext *)
{
    Q_ASSERT(false); // the texture cache lives until the context group disappears
}

static void freeTexture(QOpenGLFunctions *funcs, GLuint id)
{
    funcs->glDeleteTextures(1, &id);
}

QOpenGLCachedTexture::QOpenGLCachedTexture(GLuint id, QOpenGLTextureUploader::BindOptions options, QOpenGLContext *context) : m_options(options)
{
    m_resource = new QOpenGLSharedResourceGuard(context, id, freeTexture);
}

QT_END_NAMESPACE
