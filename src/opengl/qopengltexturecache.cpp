// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopengltexturecache_p.h"
#include <private/qopengltextureuploader_p.h>
#include <qmath.h>
#include <qopenglfunctions.h>
#include <private/qimagepixmapcleanuphooks_p.h>
#include <qpa/qplatformpixmap.h>

#include <qtopengl_tracepoints_p.h>

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

static quint64 cacheSize()
{
    bool ok = false;
    const int envCacheSize = qEnvironmentVariableIntValue("QT_OPENGL_TEXTURE_CACHE_SIZE", &ok);
    if (ok)
        return envCacheSize;

    return 1024 * 1024; // 1024 MB cache default
}

QOpenGLTextureCache::QOpenGLTextureCache(QOpenGLContext *ctx)
    : QOpenGLSharedResource(ctx->shareGroup())
    , m_cache(cacheSize())
{
}

QOpenGLTextureCache::~QOpenGLTextureCache()
{
}

QOpenGLTextureCache::BindResult QOpenGLTextureCache::bindTexture(QOpenGLContext *context,
                                                                 const QPixmap &pixmap,
                                                                 QOpenGLTextureUploader::BindOptions options)
{
    if (pixmap.isNull())
        return { 0, {} };
    QMutexLocker locker(&m_mutex);
    qint64 key = pixmap.cacheKey();

    // A QPainter is active on the image - take the safe route and replace the texture.
    if (!pixmap.paintingActive()) {
        QOpenGLCachedTexture *entry = m_cache.object(key);
        if (entry && entry->options() == options) {
            context->functions()->glBindTexture(GL_TEXTURE_2D, entry->id());
            return { entry->id(), {} };
        }
    }

    BindResult result = bindTexture(context, key, pixmap.toImage(), options);
    if (result.id > 0)
        QImagePixmapCleanupHooks::enableCleanupHooks(pixmap);

    return result;
}

QOpenGLTextureCache::BindResult QOpenGLTextureCache::bindTexture(QOpenGLContext *context,
                                                                 const QImage &image,
                                                                 QOpenGLTextureUploader::BindOptions options)
{
    if (image.isNull())
        return { 0, {} };
    QMutexLocker locker(&m_mutex);
    qint64 key = image.cacheKey();

    // A QPainter is active on the image - take the safe route and replace the texture.
    if (!image.paintingActive()) {
        QOpenGLCachedTexture *entry = m_cache.object(key);
        if (entry && entry->options() == options) {
            context->functions()->glBindTexture(GL_TEXTURE_2D, entry->id());
            return { entry->id(), {} };
        }
    }

    QImage img = image;
    if (!context->functions()->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures))
        options |= QOpenGLTextureUploader::PowerOfTwoBindOption;

    BindResult result = bindTexture(context, key, img, options);
    if (result.id > 0)
        QImagePixmapCleanupHooks::enableCleanupHooks(image);

    return result;
}

Q_TRACE_POINT(qtopengl, QOpenGLTextureCache_bindTexture_entry, QOpenGLContext *context, qint64 key, const unsigned char *image, int options);
Q_TRACE_POINT(qtopengl, QOpenGLTextureCache_bindTexture_exit);

QOpenGLTextureCache::BindResult QOpenGLTextureCache::bindTexture(QOpenGLContext *context,
                                                                 qint64 key,
                                                                 const QImage &image,
                                                                 QOpenGLTextureUploader::BindOptions options)
{
    Q_TRACE_SCOPE(QOpenGLTextureCache_bindTexture, context, key, image.bits(), options);

    GLuint id;
    QOpenGLFunctions *funcs = context->functions();
    funcs->glGenTextures(1, &id);
    funcs->glBindTexture(GL_TEXTURE_2D, id);

    int cost = QOpenGLTextureUploader::textureImage(GL_TEXTURE_2D, image, options);

    m_cache.insert(key, new QOpenGLCachedTexture(id, options, context), cost / 1024);

    return { id, BindResultFlag::NewTexture };
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
