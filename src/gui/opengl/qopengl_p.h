/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QOPENGL_P_H
#define QOPENGL_P_H

#include <qopengl.h>
#include <private/qopenglcontext_p.h>

#include <qthreadstorage.h>
#include <qcache.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QOpenGLExtensionMatcher
{
public:
    QOpenGLExtensionMatcher(const char *str);
    QOpenGLExtensionMatcher();

    bool match(const char *str) const {
        int str_length = qstrlen(str);

        Q_ASSERT(str);
        Q_ASSERT(str_length > 0);
        Q_ASSERT(str[str_length-1] != ' ');

        for (int i = 0; i < m_offsets.size(); ++i) {
            const char *extension = m_extensions.constData() + m_offsets.at(i);
            if (qstrncmp(extension, str, str_length) == 0 && extension[str_length] == ' ')
                return true;
        }
        return false;
    }

private:
    void init(const char *str);

    QByteArray m_extensions;
    QVector<int> m_offsets;
};

class QOpenGLTexture : public QOpenGLSharedResource {
public:
    QOpenGLTexture(QOpenGLContext *ctx, GLuint id, bool inverted)
        : QOpenGLSharedResource(ctx->shareGroup())
        , m_id(id)
        , m_inverted(inverted)
    {
    }

    GLuint id() const { return m_id; }
    bool invertedY() const { return m_inverted; }

protected:
    void invalidateResource()
    {
        m_id = 0;
    }

    void freeResource(QOpenGLContext *)
    {
        glDeleteTextures(1, &m_id);
    }

private:
    GLuint m_id;
    bool m_inverted;
};

struct QOpenGLTextureCacheKey {
    qint64 key;
    QOpenGLContextGroup *group;
};

inline bool operator==(const QOpenGLTextureCacheKey &a, const QOpenGLTextureCacheKey &b)
{
    return a.key == b.key && a.group == b.group;
}

inline uint qHash(const QOpenGLTextureCacheKey &key)
{
    return qHash(key.key) ^ qHash(key.group);
}

class QPlatformPixmap;

class QOpenGLTextureCache {
public:
    QOpenGLTextureCache();
    ~QOpenGLTextureCache();

    void insert(QOpenGLContext *ctx, qint64 key, QOpenGLTexture *texture, int cost);
    void remove(qint64 key);
    inline int size();
    inline void setMaxCost(int newMax);
    inline int maxCost();
    inline QOpenGLTexture* getTexture(QOpenGLContext *ctx, qint64 key);

    bool remove(QOpenGLContext *ctx, GLuint textureId);
    void removeContextTextures(QOpenGLContext *ctx);
    static QOpenGLTextureCache *instance();
    static void cleanupTexturesForCacheKey(qint64 cacheKey);
    static void cleanupTexturesForPixampData(QPlatformPixmap* pixmap);
    static void cleanupBeforePixmapDestruction(QPlatformPixmap* pixmap);

private:
    QCache<QOpenGLTextureCacheKey, QOpenGLTexture> m_cache;
    QReadWriteLock m_lock;
};

int QOpenGLTextureCache::size() {
    QReadLocker locker(&m_lock);
    return m_cache.size();
}

void QOpenGLTextureCache::setMaxCost(int newMax)
{
    QWriteLocker locker(&m_lock);
    m_cache.setMaxCost(newMax);
}

int QOpenGLTextureCache::maxCost()
{
    QReadLocker locker(&m_lock);
    return m_cache.maxCost();
}

QOpenGLTexture* QOpenGLTextureCache::getTexture(QOpenGLContext *ctx, qint64 key)
{
    QReadLocker locker(&m_lock);
    const QOpenGLTextureCacheKey cacheKey = { key, ctx->shareGroup() };
    return m_cache.object(cacheKey);
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QOPENGL_H
