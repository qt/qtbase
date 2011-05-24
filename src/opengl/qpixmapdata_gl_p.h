/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#ifndef QPIXMAPDATA_GL_P_H
#define QPIXMAPDATA_GL_P_H

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

#include "qgl_p.h"
#include "qgl.h"

#include "private/qpixmapdata_p.h"
#include "private/qglpaintdevice_p.h"

#ifdef Q_OS_SYMBIAN
#include "private/qvolatileimage_p.h"
#endif

QT_BEGIN_NAMESPACE

class QPaintEngine;
class QGLFramebufferObject;
class QGLFramebufferObjectFormat;
class QGLPixmapData;

#ifdef QGL_USE_TEXTURE_POOL
void qt_gl_register_pixmap(QGLPixmapData *pd);
void qt_gl_unregister_pixmap(QGLPixmapData *pd);
void qt_gl_hibernate_pixmaps();
#endif

#ifdef Q_OS_SYMBIAN
class QNativeImageHandleProvider;
#endif

class QGLFramebufferObjectPool
{
public:
    QGLFramebufferObject *acquire(const QSize &size, const QGLFramebufferObjectFormat &format, bool strictSize = false);
    void release(QGLFramebufferObject *fbo);

private:
    QList<QGLFramebufferObject *> m_fbos;
};

QGLFramebufferObjectPool* qgl_fbo_pool();


class QGLPixmapGLPaintDevice : public QGLPaintDevice
{
public:
    QPaintEngine* paintEngine() const;

    void beginPaint();
    void endPaint();
    QGLContext* context() const;
    QSize size() const;
    bool alphaRequested() const;

    void setPixmapData(QGLPixmapData*);
private:
    QGLPixmapData *data;
};


class Q_OPENGL_EXPORT QGLPixmapData : public QPixmapData
{
public:
    QGLPixmapData(PixelType type);
    ~QGLPixmapData();

    QPixmapData *createCompatiblePixmapData() const;

    // Re-implemented from QPixmapData:
    void resize(int width, int height);
    void fromImage(const QImage &image, Qt::ImageConversionFlags flags);
    void fromImageReader(QImageReader *imageReader,
                          Qt::ImageConversionFlags flags);
    bool fromFile(const QString &filename, const char *format,
                  Qt::ImageConversionFlags flags);
    bool fromData(const uchar *buffer, uint len, const char *format,
                  Qt::ImageConversionFlags flags);
    void copy(const QPixmapData *data, const QRect &rect);
    bool scroll(int dx, int dy, const QRect &rect);
    void fill(const QColor &color);
    bool hasAlphaChannel() const;
    QImage toImage() const;
    QPaintEngine *paintEngine() const;
    int metric(QPaintDevice::PaintDeviceMetric metric) const;

    // For accessing as a target:
    QGLPaintDevice *glDevice() const;

    // For accessing as a source:
    bool isValidContext(const QGLContext *ctx) const;
    GLuint bind(bool copyBack = true) const;
    QGLTexture *texture() const;

#ifdef QGL_USE_TEXTURE_POOL
    void destroyTexture();
    // Detach this image from the image pool.
    void detachTextureFromPool();
    // Release the GL resources associated with this pixmap and copy
    // the pixmap's contents out of the GPU back into main memory.
    // The GL resource will be automatically recreated the next time
    // ensureCreated() is called.  Does nothing if the pixmap cannot be
    // hibernated for some reason (e.g. texture is shared with another
    // process via a SgImage).
    void hibernate();
    // Called when the QGLTexturePool wants to reclaim this pixmap's
    // texture objects to reuse storage.
    void reclaimTexture();
    void forceToImage();
#endif

#ifdef Q_OS_SYMBIAN
    QImage::Format idealFormat(QImage &image, Qt::ImageConversionFlags flags);
    void* toNativeType(NativeType type);
    void fromNativeType(void* pixmap, NativeType type);
    bool initFromNativeImageHandle(void *handle, const QString &type);
    void createFromNativeImageHandleProvider();
    void releaseNativeImageHandle();
#endif

private:
    bool isValid() const;

    void ensureCreated() const;

    bool isUninitialized() const { return m_dirty && m_source.isNull(); }

    bool needsFill() const { return m_hasFillColor; }
    QColor fillColor() const { return m_fillColor; }



    QGLPixmapData(const QGLPixmapData &other);
    QGLPixmapData &operator=(const QGLPixmapData &other);

    void copyBackFromRenderFbo(bool keepCurrentFboBound) const;
    QSize size() const { return QSize(w, h); }

    bool useFramebufferObjects() const;

    QImage fillImage(const QColor &color) const;

    void createPixmapForImage(QImage &image, Qt::ImageConversionFlags flags, bool inPlace);

    mutable QGLFramebufferObject *m_renderFbo;
    mutable QPaintEngine *m_engine;
    mutable QGLContext *m_ctx;
#ifdef Q_OS_SYMBIAN
    mutable QVolatileImage m_source;
    mutable QNativeImageHandleProvider *nativeImageHandleProvider;
    void *nativeImageHandle;
    QString nativeImageType;
#else
    mutable QImage m_source;
#endif
    mutable QGLTexture m_texture;

    // the texture is not in sync with the source image
    mutable bool m_dirty;

    // fill has been called and no painting has been done, so the pixmap is
    // represented by a single fill color
    mutable QColor m_fillColor;
    mutable bool m_hasFillColor;

    mutable bool m_hasAlpha;

    mutable QGLPixmapGLPaintDevice m_glDevice;

#ifdef QGL_USE_TEXTURE_POOL
    QGLPixmapData *nextLRU;
    QGLPixmapData *prevLRU;
    mutable bool inLRU;
    mutable bool failedToAlloc;
    mutable bool inTexturePool;

    QGLPixmapData *next;
    QGLPixmapData *prev;

    friend class QGLTexturePool;

    friend void qt_gl_register_pixmap(QGLPixmapData *pd);
    friend void qt_gl_unregister_pixmap(QGLPixmapData *pd);
    friend void qt_gl_hibernate_pixmaps();
#endif

    friend class QGLPixmapGLPaintDevice;
    friend class QMeeGoPixmapData;
    friend class QMeeGoLivePixmapData;
};

QT_END_NAMESPACE

#endif // QPIXMAPDATA_GL_P_H


