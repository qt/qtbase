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

#ifndef QOPENGLFRAMEBUFFEROBJECT_H
#define QOPENGLFRAMEBUFFEROBJECT_H

#include <QtGui/qtguiglobal.h>

#ifndef QT_NO_OPENGL

#include <QtGui/qopengl.h>
#include <QtGui/qpaintdevice.h>

#include <QtCore/qscopedpointer.h>

#if defined(Q_CLANG_QDOC)
#undef GLuint
typedef unsigned int GLuint;
#undef GLenum
typedef unsigned int GLenum;
#undef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#undef GLbitfield
typedef unsigned int GLbitfield;
#endif

QT_BEGIN_NAMESPACE

class QOpenGLFramebufferObjectPrivate;
class QOpenGLFramebufferObjectFormat;

class Q_GUI_EXPORT QOpenGLFramebufferObject
{
    Q_DECLARE_PRIVATE(QOpenGLFramebufferObject)
public:
    enum Attachment {
        NoAttachment,
        CombinedDepthStencil,
        Depth
    };

    explicit QOpenGLFramebufferObject(const QSize &size, GLenum target = GL_TEXTURE_2D);
    QOpenGLFramebufferObject(int width, int height, GLenum target = GL_TEXTURE_2D);

    QOpenGLFramebufferObject(const QSize &size, Attachment attachment,
                         GLenum target = GL_TEXTURE_2D, GLenum internalFormat = 0);
    QOpenGLFramebufferObject(int width, int height, Attachment attachment,
                         GLenum target = GL_TEXTURE_2D, GLenum internalFormat = 0);

    QOpenGLFramebufferObject(const QSize &size, const QOpenGLFramebufferObjectFormat &format);
    QOpenGLFramebufferObject(int width, int height, const QOpenGLFramebufferObjectFormat &format);

    virtual ~QOpenGLFramebufferObject();

    void addColorAttachment(const QSize &size, GLenum internalFormat = 0);
    void addColorAttachment(int width, int height, GLenum internalFormat = 0);

    QOpenGLFramebufferObjectFormat format() const;

    bool isValid() const;
    bool isBound() const;
    bool bind();
    bool release();

    int width() const { return size().width(); }
    int height() const { return size().height(); }

    GLuint texture() const;
    QVector<GLuint> textures() const;

    GLuint takeTexture();
    GLuint takeTexture(int colorAttachmentIndex);

    QSize size() const;
    QVector<QSize> sizes() const;

    QImage toImage() const;
    QImage toImage(bool flipped) const;
    QImage toImage(bool flipped, int colorAttachmentIndex) const;

    Attachment attachment() const;
    void setAttachment(Attachment attachment);

    GLuint handle() const;

    static bool bindDefault();

    static bool hasOpenGLFramebufferObjects();

    static bool hasOpenGLFramebufferBlit();

    enum FramebufferRestorePolicy {
        DontRestoreFramebufferBinding,
        RestoreFramebufferBindingToDefault,
        RestoreFrameBufferBinding
    };

    static void blitFramebuffer(QOpenGLFramebufferObject *target, const QRect &targetRect,
                                QOpenGLFramebufferObject *source, const QRect &sourceRect,
                                GLbitfield buffers,
                                GLenum filter,
                                int readColorAttachmentIndex,
                                int drawColorAttachmentIndex,
                                FramebufferRestorePolicy restorePolicy);
    static void blitFramebuffer(QOpenGLFramebufferObject *target, const QRect &targetRect,
                                QOpenGLFramebufferObject *source, const QRect &sourceRect,
                                GLbitfield buffers,
                                GLenum filter,
                                int readColorAttachmentIndex,
                                int drawColorAttachmentIndex);
    static void blitFramebuffer(QOpenGLFramebufferObject *target, const QRect &targetRect,
                                QOpenGLFramebufferObject *source, const QRect &sourceRect,
                                GLbitfield buffers = GL_COLOR_BUFFER_BIT,
                                GLenum filter = GL_NEAREST);
    static void blitFramebuffer(QOpenGLFramebufferObject *target,
                                QOpenGLFramebufferObject *source,
                                GLbitfield buffers = GL_COLOR_BUFFER_BIT,
                                GLenum filter = GL_NEAREST);

private:
    Q_DISABLE_COPY(QOpenGLFramebufferObject)
    QScopedPointer<QOpenGLFramebufferObjectPrivate> d_ptr;
    friend class QOpenGLPaintDevice;
    friend class QOpenGLFBOGLPaintDevice;
};

class QOpenGLFramebufferObjectFormatPrivate;
class Q_GUI_EXPORT QOpenGLFramebufferObjectFormat
{
public:
    QOpenGLFramebufferObjectFormat();
    QOpenGLFramebufferObjectFormat(const QOpenGLFramebufferObjectFormat &other);
    QOpenGLFramebufferObjectFormat &operator=(const QOpenGLFramebufferObjectFormat &other);
    ~QOpenGLFramebufferObjectFormat();

    void setSamples(int samples);
    int samples() const;

    void setMipmap(bool enabled);
    bool mipmap() const;

    void setAttachment(QOpenGLFramebufferObject::Attachment attachment);
    QOpenGLFramebufferObject::Attachment attachment() const;

    void setTextureTarget(GLenum target);
    GLenum textureTarget() const;

    void setInternalTextureFormat(GLenum internalTextureFormat);
    GLenum internalTextureFormat() const;

    bool operator==(const QOpenGLFramebufferObjectFormat& other) const;
    bool operator!=(const QOpenGLFramebufferObjectFormat& other) const;

private:
    QOpenGLFramebufferObjectFormatPrivate *d;

    void detach();
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLFRAMEBUFFEROBJECT_H
