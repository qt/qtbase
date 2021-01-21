/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QOPENGLTEXTURECACHE_P_H
#define QOPENGLTEXTURECACHE_P_H

#include <QtGui/private/qtguiglobal_p.h>
#include <QHash>
#include <QObject>
#include <QCache>
#include <private/qopenglcontext_p.h>
#include <private/qopengltextureuploader_p.h>
#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

class QOpenGLCachedTexture;

class Q_GUI_EXPORT QOpenGLTextureCache : public QOpenGLSharedResource
{
public:
    static QOpenGLTextureCache *cacheForContext(QOpenGLContext *context);

    QOpenGLTextureCache(QOpenGLContext *);
    ~QOpenGLTextureCache();

    GLuint bindTexture(QOpenGLContext *context, const QPixmap &pixmap,
                       QOpenGLTextureUploader::BindOptions options = QOpenGLTextureUploader::PremultipliedAlphaBindOption);
    GLuint bindTexture(QOpenGLContext *context, const QImage &image,
                       QOpenGLTextureUploader::BindOptions options = QOpenGLTextureUploader::PremultipliedAlphaBindOption);

    void invalidate(qint64 key);

    void invalidateResource() override;
    void freeResource(QOpenGLContext *ctx) override;

private:
    GLuint bindTexture(QOpenGLContext *context, qint64 key, const QImage &image, QOpenGLTextureUploader::BindOptions options);

    QMutex m_mutex;
    QCache<quint64, QOpenGLCachedTexture> m_cache;
};

class QOpenGLCachedTexture
{
public:
    QOpenGLCachedTexture(GLuint id, QOpenGLTextureUploader::BindOptions options, QOpenGLContext *context);
    ~QOpenGLCachedTexture() { m_resource->free(); }

    GLuint id() const { return m_resource->id(); }
    QOpenGLTextureUploader::BindOptions options() const { return m_options; }

private:
    QOpenGLSharedResourceGuard *m_resource;
    QOpenGLTextureUploader::BindOptions m_options;
};

QT_END_NAMESPACE

#endif

