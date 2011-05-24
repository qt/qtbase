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

#ifndef QGLFRAMEBUFFEROBJECT_H
#define QGLFRAMEBUFFEROBJECT_H

#include <QtOpenGL/qgl.h>
#include <QtGui/qpaintdevice.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(OpenGL)

class QGLFramebufferObjectPrivate;
class QGLFramebufferObjectFormat;

class Q_OPENGL_EXPORT QGLFramebufferObject : public QPaintDevice
{
    Q_DECLARE_PRIVATE(QGLFramebufferObject)
public:
    enum Attachment {
        NoAttachment,
        CombinedDepthStencil,
        Depth
    };

    QGLFramebufferObject(const QSize &size, GLenum target = GL_TEXTURE_2D);
    QGLFramebufferObject(int width, int height, GLenum target = GL_TEXTURE_2D);
#if !defined(QT_OPENGL_ES) || defined(Q_QDOC)
    QGLFramebufferObject(const QSize &size, Attachment attachment,
                         GLenum target = GL_TEXTURE_2D, GLenum internal_format = GL_RGBA8);
    QGLFramebufferObject(int width, int height, Attachment attachment,
                         GLenum target = GL_TEXTURE_2D, GLenum internal_format = GL_RGBA8);
#else
    QGLFramebufferObject(const QSize &size, Attachment attachment,
                         GLenum target = GL_TEXTURE_2D, GLenum internal_format = GL_RGBA);
    QGLFramebufferObject(int width, int height, Attachment attachment,
                         GLenum target = GL_TEXTURE_2D, GLenum internal_format = GL_RGBA);
#endif

    QGLFramebufferObject(const QSize &size, const QGLFramebufferObjectFormat &format);
    QGLFramebufferObject(int width, int height, const QGLFramebufferObjectFormat &format);

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
    QGLFramebufferObject(const QSize &size, QMacCompatGLenum target = GL_TEXTURE_2D);
    QGLFramebufferObject(int width, int height, QMacCompatGLenum target = GL_TEXTURE_2D);

    QGLFramebufferObject(const QSize &size, Attachment attachment,
                         QMacCompatGLenum target = GL_TEXTURE_2D, QMacCompatGLenum internal_format = GL_RGBA8);
    QGLFramebufferObject(int width, int height, Attachment attachment,
                         QMacCompatGLenum target = GL_TEXTURE_2D, QMacCompatGLenum internal_format = GL_RGBA8);
#endif

    virtual ~QGLFramebufferObject();

    QGLFramebufferObjectFormat format() const;

    bool isValid() const;
    bool isBound() const;
    bool bind();
    bool release();

    GLuint texture() const;
    QSize size() const;
    QImage toImage() const;
    Attachment attachment() const;

    QPaintEngine *paintEngine() const;
    GLuint handle() const;

    static bool bindDefault();

    static bool hasOpenGLFramebufferObjects();

    void drawTexture(const QRectF &target, GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);
    void drawTexture(const QPointF &point, GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);
#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
    void drawTexture(const QRectF &target, QMacCompatGLuint textureId, QMacCompatGLenum textureTarget = GL_TEXTURE_2D);
    void drawTexture(const QPointF &point, QMacCompatGLuint textureId, QMacCompatGLenum textureTarget = GL_TEXTURE_2D);
#endif

    static bool hasOpenGLFramebufferBlit();
    static void blitFramebuffer(QGLFramebufferObject *target, const QRect &targetRect,
                                QGLFramebufferObject *source, const QRect &sourceRect,
                                GLbitfield buffers = GL_COLOR_BUFFER_BIT,
                                GLenum filter = GL_NEAREST);

protected:
    int metric(PaintDeviceMetric metric) const;
    int devType() const { return QInternal::FramebufferObject; }

private:
    Q_DISABLE_COPY(QGLFramebufferObject)
    QScopedPointer<QGLFramebufferObjectPrivate> d_ptr;
    friend class QGLPaintDevice;
    friend class QGLFBOGLPaintDevice;
};

class QGLFramebufferObjectFormatPrivate;
class Q_OPENGL_EXPORT QGLFramebufferObjectFormat
{
public:
    QGLFramebufferObjectFormat();
    QGLFramebufferObjectFormat(const QGLFramebufferObjectFormat &other);
    QGLFramebufferObjectFormat &operator=(const QGLFramebufferObjectFormat &other);
    ~QGLFramebufferObjectFormat();

    void setSamples(int samples);
    int samples() const;

    void setMipmap(bool enabled);
    bool mipmap() const;

    void setAttachment(QGLFramebufferObject::Attachment attachment);
    QGLFramebufferObject::Attachment attachment() const;

    void setTextureTarget(GLenum target);
    GLenum textureTarget() const;

    void setInternalTextureFormat(GLenum internalTextureFormat);
    GLenum internalTextureFormat() const;

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
    void setTextureTarget(QMacCompatGLenum target);
    void setInternalTextureFormat(QMacCompatGLenum internalTextureFormat);
#endif

    bool operator==(const QGLFramebufferObjectFormat& other) const;
    bool operator!=(const QGLFramebufferObjectFormat& other) const;

private:
    QGLFramebufferObjectFormatPrivate *d;

    void detach();
};

QT_END_NAMESPACE

QT_END_HEADER
#endif // QGLFRAMEBUFFEROBJECT_H
