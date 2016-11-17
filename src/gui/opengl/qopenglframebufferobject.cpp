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

#include "qopenglframebufferobject.h"
#include "qopenglframebufferobject_p.h"

#include <qdebug.h>
#include <private/qopengl_p.h>
#include <private/qopenglcontext_p.h>
#include <private/qopenglextensions_p.h>
#include <private/qfont_p.h>

#include <qwindow.h>
#include <qlibrary.h>
#include <qimage.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG
#define QT_RESET_GLERROR()                                \
{                                                         \
    while (QOpenGLContext::currentContext()->functions()->glGetError() != GL_NO_ERROR) {} \
}
#define QT_CHECK_GLERROR()                                \
{                                                         \
    GLenum err = QOpenGLContext::currentContext()->functions()->glGetError(); \
    if (err != GL_NO_ERROR) {                             \
        qDebug("[%s line %d] OpenGL Error: %d",           \
               __FILE__, __LINE__, (int)err);             \
    }                                                     \
}
#else
#define QT_RESET_GLERROR() {}
#define QT_CHECK_GLERROR() {}
#endif

#ifndef GL_MAX_SAMPLES
#define GL_MAX_SAMPLES 0x8D57
#endif

#ifndef GL_RENDERBUFFER_SAMPLES
#define GL_RENDERBUFFER_SAMPLES 0x8CAB
#endif

#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif

#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif

#ifndef GL_DEPTH_COMPONENT24_OES
#define GL_DEPTH_COMPONENT24_OES 0x81A6
#endif

#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif

#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif

#ifndef GL_RGB8
#define GL_RGB8                           0x8051
#endif

#ifndef GL_RGB10
#define GL_RGB10                          0x8052
#endif

#ifndef GL_RGBA8
#define GL_RGBA8                          0x8058
#endif

#ifndef GL_RGB10_A2
#define GL_RGB10_A2                       0x8059
#endif

#ifndef GL_BGRA
#define GL_BGRA                           0x80E1
#endif

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
#endif

#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#endif


/*!
    \class QOpenGLFramebufferObjectFormat
    \brief The QOpenGLFramebufferObjectFormat class specifies the format of an OpenGL
    framebuffer object.
    \inmodule QtGui

    \since 5.0

    \ingroup painting-3D

    A framebuffer object has several characteristics:
    \list
    \li \l{setSamples()}{Number of samples per pixels.}
    \li \l{setAttachment()}{Depth and/or stencil attachments.}
    \li \l{setTextureTarget()}{Texture target.}
    \li \l{setInternalTextureFormat()}{Internal texture format.}
    \endlist

    Note that the desired attachments or number of samples per pixels might not
    be supported by the hardware driver. Call QOpenGLFramebufferObject::format()
    after creating a QOpenGLFramebufferObject to find the exact format that was
    used to create the frame buffer object.

    \sa QOpenGLFramebufferObject
*/

/*!
    \internal
*/
void QOpenGLFramebufferObjectFormat::detach()
{
    if (d->ref.load() != 1) {
        QOpenGLFramebufferObjectFormatPrivate *newd
            = new QOpenGLFramebufferObjectFormatPrivate(d);
        if (!d->ref.deref())
            delete d;
        d = newd;
    }
}

/*!
    Creates a QOpenGLFramebufferObjectFormat object for specifying
    the format of an OpenGL framebuffer object.

    By default the format specifies a non-multisample framebuffer object with no
    depth/stencil attachments, texture target \c GL_TEXTURE_2D, and internal format \c GL_RGBA8.
    On OpenGL/ES systems, the default internal format is \c GL_RGBA.

    \sa samples(), attachment(), internalTextureFormat()
*/

QOpenGLFramebufferObjectFormat::QOpenGLFramebufferObjectFormat()
{
    d = new QOpenGLFramebufferObjectFormatPrivate;
}

/*!
    Constructs a copy of \a other.
*/

QOpenGLFramebufferObjectFormat::QOpenGLFramebufferObjectFormat(const QOpenGLFramebufferObjectFormat &other)
{
    d = other.d;
    d->ref.ref();
}

/*!
    Assigns \a other to this object.
*/

QOpenGLFramebufferObjectFormat &QOpenGLFramebufferObjectFormat::operator=(const QOpenGLFramebufferObjectFormat &other)
{
    if (d != other.d) {
        other.d->ref.ref();
        if (!d->ref.deref())
            delete d;
        d = other.d;
    }
    return *this;
}

/*!
    Destroys the QOpenGLFramebufferObjectFormat.
*/
QOpenGLFramebufferObjectFormat::~QOpenGLFramebufferObjectFormat()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    Sets the number of samples per pixel for a multisample framebuffer object
    to \a samples.  The default sample count of 0 represents a regular
    non-multisample framebuffer object.

    If the desired amount of samples per pixel is not supported by the hardware
    then the maximum number of samples per pixel will be used. Note that
    multisample framebuffer objects can not be bound as textures. Also, the
    \c{GL_EXT_framebuffer_multisample} extension is required to create a
    framebuffer with more than one sample per pixel.

    \sa samples()
*/
void QOpenGLFramebufferObjectFormat::setSamples(int samples)
{
    detach();
    d->samples = samples;
}

/*!
    Returns the number of samples per pixel if a framebuffer object
    is a multisample framebuffer object. Otherwise, returns 0.
    The default value is 0.

    \sa setSamples()
*/
int QOpenGLFramebufferObjectFormat::samples() const
{
    return d->samples;
}

/*!
    Enables mipmapping if \a enabled is true; otherwise disables it.

    Mipmapping is disabled by default.

    If mipmapping is enabled, additional memory will be allocated for
    the mipmap levels. The mipmap levels can be updated by binding the
    texture and calling glGenerateMipmap(). Mipmapping cannot be enabled
    for multisampled framebuffer objects.

    \sa mipmap(), QOpenGLFramebufferObject::texture()
*/
void QOpenGLFramebufferObjectFormat::setMipmap(bool enabled)
{
    detach();
    d->mipmap = enabled;
}

/*!
    Returns \c true if mipmapping is enabled.

    \sa setMipmap()
*/
bool QOpenGLFramebufferObjectFormat::mipmap() const
{
    return d->mipmap;
}

/*!
    Sets the attachment configuration of a framebuffer object to \a attachment.

    \sa attachment()
*/
void QOpenGLFramebufferObjectFormat::setAttachment(QOpenGLFramebufferObject::Attachment attachment)
{
    detach();
    d->attachment = attachment;
}

/*!
    Returns the configuration of the depth and stencil buffers attached to
    a framebuffer object.  The default is QOpenGLFramebufferObject::NoAttachment.

    \sa setAttachment()
*/
QOpenGLFramebufferObject::Attachment QOpenGLFramebufferObjectFormat::attachment() const
{
    return d->attachment;
}

/*!
    Sets the texture target of the texture attached to a framebuffer object to
    \a target. Ignored for multisample framebuffer objects.

    \sa textureTarget(), samples()
*/
void QOpenGLFramebufferObjectFormat::setTextureTarget(GLenum target)
{
    detach();
    d->target = target;
}

/*!
    Returns the texture target of the texture attached to a framebuffer object.
    Ignored for multisample framebuffer objects.  The default is
    \c GL_TEXTURE_2D.

    \sa setTextureTarget(), samples()
*/
GLenum QOpenGLFramebufferObjectFormat::textureTarget() const
{
    return d->target;
}

/*!
    Sets the internal format of a framebuffer object's texture or
    multisample framebuffer object's color buffer to
    \a internalTextureFormat.

    \sa internalTextureFormat()
*/
void QOpenGLFramebufferObjectFormat::setInternalTextureFormat(GLenum internalTextureFormat)
{
    detach();
    d->internal_format = internalTextureFormat;
}

/*!
    Returns the internal format of a framebuffer object's texture or
    multisample framebuffer object's color buffer.  The default is
    \c GL_RGBA8 on desktop OpenGL systems, and \c GL_RGBA on
    OpenGL/ES systems.

    \sa setInternalTextureFormat()
*/
GLenum QOpenGLFramebufferObjectFormat::internalTextureFormat() const
{
    return d->internal_format;
}

/*!
    Returns \c true if all the options of this framebuffer object format
    are the same as \a other; otherwise returns \c false.
*/
bool QOpenGLFramebufferObjectFormat::operator==(const QOpenGLFramebufferObjectFormat& other) const
{
    if (d == other.d)
        return true;
    else
        return d->equals(other.d);
}

/*!
    Returns \c false if all the options of this framebuffer object format
    are the same as \a other; otherwise returns \c true.
*/
bool QOpenGLFramebufferObjectFormat::operator!=(const QOpenGLFramebufferObjectFormat& other) const
{
    return !(*this == other);
}

bool QOpenGLFramebufferObjectPrivate::checkFramebufferStatus(QOpenGLContext *ctx) const
{
    if (!ctx)
        return false;   // Context no longer exists.
    GLenum status = ctx->functions()->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status) {
    case GL_NO_ERROR:
    case GL_FRAMEBUFFER_COMPLETE:
        return true;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        qDebug("QOpenGLFramebufferObject: Unsupported framebuffer format.");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        qDebug("QOpenGLFramebufferObject: Framebuffer incomplete attachment.");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, missing attachment.");
        break;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT
    case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT:
        qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, duplicate attachment.");
        break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, attached images must have same dimensions.");
        break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
        qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, attached images must have same format.");
        break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, missing draw buffer.");
        break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, missing read buffer.");
        break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, attachments must have same number of samples per pixel.");
        break;
#endif
    default:
        qDebug() <<"QOpenGLFramebufferObject: An undefined error has occurred: "<< status;
        break;
    }
    return false;
}

namespace
{
    void freeFramebufferFunc(QOpenGLFunctions *funcs, GLuint id)
    {
        funcs->glDeleteFramebuffers(1, &id);
    }

    void freeRenderbufferFunc(QOpenGLFunctions *funcs, GLuint id)
    {
        funcs->glDeleteRenderbuffers(1, &id);
    }

    void freeTextureFunc(QOpenGLFunctions *funcs, GLuint id)
    {
        funcs->glDeleteTextures(1, &id);
    }
}

void QOpenGLFramebufferObjectPrivate::init(QOpenGLFramebufferObject *, const QSize &size,
                                           QOpenGLFramebufferObject::Attachment attachment,
                                           GLenum texture_target, GLenum internal_format,
                                           GLint samples, bool mipmap)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();

    funcs.initializeOpenGLFunctions();

    if (!funcs.hasOpenGLFeature(QOpenGLFunctions::Framebuffers))
        return;

    // Fall back to using a normal non-msaa FBO if we don't have support for MSAA
    if (!funcs.hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample)
            || !funcs.hasOpenGLExtension(QOpenGLExtensions::FramebufferBlit)) {
        samples = 0;
    } else if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
        GLint maxSamples;
        funcs.glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        samples = qBound(0, int(samples), int(maxSamples));
    }

    colorAttachments.append(ColorAttachment(size, internal_format));

    dsSize = size;

    samples = qMax(0, samples);
    requestedSamples = samples;

    target = texture_target;

    QT_RESET_GLERROR(); // reset error state
    GLuint fbo = 0;

    funcs.glGenFramebuffers(1, &fbo);
    funcs.glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    QOpenGLContextPrivate::get(ctx)->qgl_current_fbo_invalid = true;

    QT_CHECK_GLERROR();

    format.setTextureTarget(target);
    format.setInternalTextureFormat(internal_format);
    format.setMipmap(mipmap);

    if (samples == 0)
        initTexture(0);
    else
        initColorBuffer(0, &samples);

    format.setSamples(int(samples));

    initDepthStencilAttachments(ctx, attachment);

    if (valid)
        fbo_guard = new QOpenGLSharedResourceGuard(ctx, fbo, freeFramebufferFunc);
    else
        funcs.glDeleteFramebuffers(1, &fbo);

    QT_CHECK_GLERROR();
}

void QOpenGLFramebufferObjectPrivate::initTexture(int idx)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    GLuint texture = 0;

    funcs.glGenTextures(1, &texture);
    funcs.glBindTexture(target, texture);

    funcs.glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    funcs.glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    funcs.glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    funcs.glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    ColorAttachment &color(colorAttachments[idx]);

    GLuint pixelType = GL_UNSIGNED_BYTE;
    if (color.internalFormat == GL_RGB10_A2 || color.internalFormat == GL_RGB10)
        pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;

    funcs.glTexImage2D(target, 0, color.internalFormat, color.size.width(), color.size.height(), 0,
                       GL_RGBA, pixelType, NULL);
    if (format.mipmap()) {
        int width = color.size.width();
        int height = color.size.height();
        int level = 0;
        while (width > 1 || height > 1) {
            width = qMax(1, width >> 1);
            height = qMax(1, height >> 1);
            ++level;
            funcs.glTexImage2D(target, level, color.internalFormat, width, height, 0,
                               GL_RGBA, pixelType, NULL);
        }
    }
    funcs.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx,
                                 target, texture, 0);

    QT_CHECK_GLERROR();
    funcs.glBindTexture(target, 0);
    valid = checkFramebufferStatus(ctx);
    if (valid) {
        color.guard = new QOpenGLSharedResourceGuard(ctx, texture, freeTextureFunc);
    } else {
        funcs.glDeleteTextures(1, &texture);
    }
}

void QOpenGLFramebufferObjectPrivate::initColorBuffer(int idx, GLint *samples)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    GLuint color_buffer = 0;

    ColorAttachment &color(colorAttachments[idx]);

    GLenum storageFormat = color.internalFormat;
    // ES requires a sized format. The older desktop extension does not. Correct the format on ES.
    if (ctx->isOpenGLES() && color.internalFormat == GL_RGBA) {
        if (funcs.hasOpenGLExtension(QOpenGLExtensions::Sized8Formats))
            storageFormat = GL_RGBA8;
        else
            storageFormat = GL_RGBA4;
    }

    funcs.glGenRenderbuffers(1, &color_buffer);
    funcs.glBindRenderbuffer(GL_RENDERBUFFER, color_buffer);
    funcs.glRenderbufferStorageMultisample(GL_RENDERBUFFER, *samples, storageFormat, color.size.width(), color.size.height());
    funcs.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx,
                                    GL_RENDERBUFFER, color_buffer);

    QT_CHECK_GLERROR();
    valid = checkFramebufferStatus(ctx);
    if (valid) {
        // Query the actual number of samples. This can be greater than the requested
        // value since the typically supported values are 0, 4, 8, ..., and the
        // requests are mapped to the next supported value.
        funcs.glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, samples);
        color.guard = new QOpenGLSharedResourceGuard(ctx, color_buffer, freeRenderbufferFunc);
    } else {
        funcs.glDeleteRenderbuffers(1, &color_buffer);
    }
}

void QOpenGLFramebufferObjectPrivate::initDepthStencilAttachments(QOpenGLContext *ctx,
                                                                  QOpenGLFramebufferObject::Attachment attachment)
{
    // Use the same sample count for all attachments. format.samples() already contains
    // the actual number of samples for the color attachment and is not suitable. Use
    // requestedSamples instead.
    const int samples = requestedSamples;

    // free existing attachments
    if (depth_buffer_guard) {
        funcs.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        depth_buffer_guard->free();
    }
    if (stencil_buffer_guard) {
        funcs.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
        if (stencil_buffer_guard != depth_buffer_guard)
            stencil_buffer_guard->free();
    }

    depth_buffer_guard = 0;
    stencil_buffer_guard = 0;

    GLuint depth_buffer = 0;
    GLuint stencil_buffer = 0;

    // In practice, a combined depth-stencil buffer is supported by all desktop platforms, while a
    // separate stencil buffer is not. On embedded devices however, a combined depth-stencil buffer
    // might not be supported while separate buffers are, according to QTBUG-12861.

    if (attachment == QOpenGLFramebufferObject::CombinedDepthStencil
        && funcs.hasOpenGLExtension(QOpenGLExtensions::PackedDepthStencil))
    {
        // depth and stencil buffer needs another extension
        funcs.glGenRenderbuffers(1, &depth_buffer);
        funcs.glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
        Q_ASSERT(funcs.glIsRenderbuffer(depth_buffer));
        if (samples != 0 && funcs.hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample))
            funcs.glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
                GL_DEPTH24_STENCIL8, dsSize.width(), dsSize.height());
        else
            funcs.glRenderbufferStorage(GL_RENDERBUFFER,
                GL_DEPTH24_STENCIL8, dsSize.width(), dsSize.height());

        stencil_buffer = depth_buffer;
        funcs.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                     GL_RENDERBUFFER, depth_buffer);
        funcs.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                     GL_RENDERBUFFER, stencil_buffer);

        valid = checkFramebufferStatus(ctx);
        if (!valid) {
            funcs.glDeleteRenderbuffers(1, &depth_buffer);
            stencil_buffer = depth_buffer = 0;
        }
    }

    if (depth_buffer == 0 && (attachment == QOpenGLFramebufferObject::CombinedDepthStencil
        || (attachment == QOpenGLFramebufferObject::Depth)))
    {
        funcs.glGenRenderbuffers(1, &depth_buffer);
        funcs.glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
        Q_ASSERT(funcs.glIsRenderbuffer(depth_buffer));
        if (samples != 0 && funcs.hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample)) {
            if (ctx->isOpenGLES()) {
                if (funcs.hasOpenGLExtension(QOpenGLExtensions::Depth24))
                    funcs.glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
                                                           GL_DEPTH_COMPONENT24, dsSize.width(), dsSize.height());
                else
                    funcs.glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
                                                           GL_DEPTH_COMPONENT16, dsSize.width(), dsSize.height());
            } else {
                funcs.glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
                                                       GL_DEPTH_COMPONENT, dsSize.width(), dsSize.height());
            }
        } else {
            if (ctx->isOpenGLES()) {
                if (funcs.hasOpenGLExtension(QOpenGLExtensions::Depth24)) {
                    funcs.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                                                dsSize.width(), dsSize.height());
                } else {
                    funcs.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                                                dsSize.width(), dsSize.height());
                }
            } else {
                funcs.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, dsSize.width(), dsSize.height());
            }
        }
        funcs.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                     GL_RENDERBUFFER, depth_buffer);
        valid = checkFramebufferStatus(ctx);
        if (!valid) {
            funcs.glDeleteRenderbuffers(1, &depth_buffer);
            depth_buffer = 0;
        }
    }

    if (stencil_buffer == 0 && (attachment == QOpenGLFramebufferObject::CombinedDepthStencil)) {
        funcs.glGenRenderbuffers(1, &stencil_buffer);
        funcs.glBindRenderbuffer(GL_RENDERBUFFER, stencil_buffer);
        Q_ASSERT(funcs.glIsRenderbuffer(stencil_buffer));

#ifdef QT_OPENGL_ES
        GLenum storage = GL_STENCIL_INDEX8;
#else
        GLenum storage = ctx->isOpenGLES() ? GL_STENCIL_INDEX8 : GL_STENCIL_INDEX;
#endif

        if (samples != 0 && funcs.hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample))
            funcs.glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, storage, dsSize.width(), dsSize.height());
        else
            funcs.glRenderbufferStorage(GL_RENDERBUFFER, storage, dsSize.width(), dsSize.height());

        funcs.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, stencil_buffer);
        valid = checkFramebufferStatus(ctx);
        if (!valid) {
            funcs.glDeleteRenderbuffers(1, &stencil_buffer);
            stencil_buffer = 0;
        }
    }

    // The FBO might have become valid after removing the depth or stencil buffer.
    valid = checkFramebufferStatus(ctx);

    if (depth_buffer && stencil_buffer) {
        fbo_attachment = QOpenGLFramebufferObject::CombinedDepthStencil;
    } else if (depth_buffer) {
        fbo_attachment = QOpenGLFramebufferObject::Depth;
    } else {
        fbo_attachment = QOpenGLFramebufferObject::NoAttachment;
    }

    if (valid) {
        if (depth_buffer)
            depth_buffer_guard = new QOpenGLSharedResourceGuard(ctx, depth_buffer, freeRenderbufferFunc);
        if (stencil_buffer) {
            if (stencil_buffer == depth_buffer)
                stencil_buffer_guard = depth_buffer_guard;
            else
                stencil_buffer_guard = new QOpenGLSharedResourceGuard(ctx, stencil_buffer, freeRenderbufferFunc);
        }
    } else {
        if (depth_buffer)
            funcs.glDeleteRenderbuffers(1, &depth_buffer);
        if (stencil_buffer && depth_buffer != stencil_buffer)
            funcs.glDeleteRenderbuffers(1, &stencil_buffer);
    }
    QT_CHECK_GLERROR();

    format.setAttachment(fbo_attachment);
}

/*!
    \class QOpenGLFramebufferObject
    \brief The QOpenGLFramebufferObject class encapsulates an OpenGL framebuffer object.
    \since 5.0
    \inmodule QtGui

    \ingroup painting-3D

    The QOpenGLFramebufferObject class encapsulates an OpenGL framebuffer
    object, defined by the \c{GL_EXT_framebuffer_object} extension. It provides
    a rendering surface that can be painted on with a QPainter with the help of
    QOpenGLPaintDevice, or rendered to using native OpenGL calls. This surface
    can be bound and used as a regular texture in your own OpenGL drawing code.
    By default, the QOpenGLFramebufferObject class generates a 2D OpenGL
    texture (using the \c{GL_TEXTURE_2D} target), which is used as the internal
    rendering target.

    \b{It is important to have a current OpenGL context when creating a
    QOpenGLFramebufferObject, otherwise initialization will fail.}

    Create the QOpenGLFrameBufferObject instance with the CombinedDepthStencil
    attachment if you want QPainter to render correctly. Note that you need to
    create a QOpenGLFramebufferObject with more than one sample per pixel for
    primitives to be antialiased when drawing using a QPainter. To create a
    multisample framebuffer object you should use one of the constructors that
    take a QOpenGLFramebufferObjectFormat parameter, and set the
    QOpenGLFramebufferObjectFormat::samples() property to a non-zero value.

    For multisample framebuffer objects a color render buffer is created,
    otherwise a texture with the specified texture target is created.
    The color render buffer or texture will have the specified internal
    format, and will be bound to the \c GL_COLOR_ATTACHMENT0
    attachment in the framebuffer object.

    Multiple render targets are also supported, in case the OpenGL
    implementation supports this. Here there will be multiple textures (or, in
    case of multisampling, renderbuffers) present and each of them will get
    attached to \c GL_COLOR_ATTACHMENT0, \c 1, \c 2, ...

    If you want to use a framebuffer object with multisampling enabled
    as a texture, you first need to copy from it to a regular framebuffer
    object using QOpenGLContext::blitFramebuffer().

    It is possible to draw into a QOpenGLFramebufferObject using QPainter and
    QOpenGLPaintDevice in a separate thread.
*/


/*!
    \enum QOpenGLFramebufferObject::Attachment

    This enum type is used to configure the depth and stencil buffers
    attached to the framebuffer object when it is created.

    \value NoAttachment         No attachment is added to the framebuffer object. Note that the
                                OpenGL depth and stencil tests won't work when rendering to a
                                framebuffer object without any depth or stencil buffers.
                                This is the default value.

    \value CombinedDepthStencil If the \c GL_EXT_packed_depth_stencil extension is present,
                                a combined depth and stencil buffer is attached.
                                If the extension is not present, only a depth buffer is attached.

    \value Depth                A depth buffer is attached to the framebuffer object.

    \sa attachment()
*/

static inline GLenum effectiveInternalFormat(GLenum internalFormat)
{
    if (!internalFormat)
#ifdef QT_OPENGL_ES_2
        internalFormat = GL_RGBA;
#else
        internalFormat = QOpenGLContext::currentContext()->isOpenGLES() ? GL_RGBA : GL_RGBA8;
#endif
    return internalFormat;
}

/*! \fn QOpenGLFramebufferObject::QOpenGLFramebufferObject(const QSize &size, GLenum target)

    Constructs an OpenGL framebuffer object and binds a 2D OpenGL texture
    to the buffer of the size \a size. The texture is bound to the
    \c GL_COLOR_ATTACHMENT0 target in the framebuffer object.

    The \a target parameter is used to specify the OpenGL texture
    target. The default target is \c GL_TEXTURE_2D. Keep in mind that
    \c GL_TEXTURE_2D textures must have a power of 2 width and height
    (e.g. 256x512), unless you are using OpenGL 2.0 or higher.

    By default, no depth and stencil buffers are attached. This behavior
    can be toggled using one of the overloaded constructors.

    The default internal texture format is \c GL_RGBA8 for desktop
    OpenGL, and \c GL_RGBA for OpenGL/ES.

    It is important that you have a current OpenGL context set when
    creating the QOpenGLFramebufferObject, otherwise the initialization
    will fail.

    \sa size(), texture(), attachment()
*/

QOpenGLFramebufferObject::QOpenGLFramebufferObject(const QSize &size, GLenum target)
    : d_ptr(new QOpenGLFramebufferObjectPrivate)
{
    Q_D(QOpenGLFramebufferObject);
    d->init(this, size, NoAttachment, target, effectiveInternalFormat(0));
}

/*! \overload

    Constructs an OpenGL framebuffer object and binds a 2D OpenGL texture
    to the buffer of the given \a width and \a height.

    \sa size(), texture()
*/
QOpenGLFramebufferObject::QOpenGLFramebufferObject(int width, int height, GLenum target)
    : QOpenGLFramebufferObject(QSize(width, height), target)
{
}

/*! \overload

    Constructs an OpenGL framebuffer object of the given \a size based on the
    supplied \a format.
*/

QOpenGLFramebufferObject::QOpenGLFramebufferObject(const QSize &size, const QOpenGLFramebufferObjectFormat &format)
    : d_ptr(new QOpenGLFramebufferObjectPrivate)
{
    Q_D(QOpenGLFramebufferObject);
    d->init(this, size, format.attachment(), format.textureTarget(), format.internalTextureFormat(),
            format.samples(), format.mipmap());
}

/*! \overload

    Constructs an OpenGL framebuffer object of the given \a width and \a height
    based on the supplied \a format.
*/

QOpenGLFramebufferObject::QOpenGLFramebufferObject(int width, int height, const QOpenGLFramebufferObjectFormat &format)
    : QOpenGLFramebufferObject(QSize(width, height), format)
{
}

/*! \overload

    Constructs an OpenGL framebuffer object and binds a texture to the
    buffer of the given \a width and \a height.

    The \a attachment parameter describes the depth/stencil buffer
    configuration, \a target the texture target and \a internalFormat
    the internal texture format. The default texture target is \c
    GL_TEXTURE_2D, while the default internal format is \c GL_RGBA8
    for desktop OpenGL and \c GL_RGBA for OpenGL/ES.

    \sa size(), texture(), attachment()
*/
QOpenGLFramebufferObject::QOpenGLFramebufferObject(int width, int height, Attachment attachment,
                                           GLenum target, GLenum internalFormat)
    : d_ptr(new QOpenGLFramebufferObjectPrivate)
{
    Q_D(QOpenGLFramebufferObject);
    d->init(this, QSize(width, height), attachment, target, effectiveInternalFormat(internalFormat));
}

/*! \overload

    Constructs an OpenGL framebuffer object and binds a texture to the
    buffer of the given \a size.

    The \a attachment parameter describes the depth/stencil buffer
    configuration, \a target the texture target and \a internalFormat
    the internal texture format. The default texture target is \c
    GL_TEXTURE_2D, while the default internal format is \c GL_RGBA8
    for desktop OpenGL and \c GL_RGBA for OpenGL/ES.

    \sa size(), texture(), attachment()
*/
QOpenGLFramebufferObject::QOpenGLFramebufferObject(const QSize &size, Attachment attachment,
                                           GLenum target, GLenum internalFormat)
    : d_ptr(new QOpenGLFramebufferObjectPrivate)
{
    Q_D(QOpenGLFramebufferObject);
    d->init(this, size, attachment, target, effectiveInternalFormat(internalFormat));
}

/*!
    \fn QOpenGLFramebufferObject::~QOpenGLFramebufferObject()

    Destroys the framebuffer object and frees any allocated resources.
*/
QOpenGLFramebufferObject::~QOpenGLFramebufferObject()
{
    Q_D(QOpenGLFramebufferObject);
    if (isBound())
        release();

    for (const auto &color : qAsConst(d->colorAttachments)) {
        if (color.guard)
            color.guard->free();
    }
    d->colorAttachments.clear();

    if (d->depth_buffer_guard)
        d->depth_buffer_guard->free();
    if (d->stencil_buffer_guard && d->stencil_buffer_guard != d->depth_buffer_guard)
        d->stencil_buffer_guard->free();
    if (d->fbo_guard)
        d->fbo_guard->free();

    QOpenGLContextPrivate *contextPrv = QOpenGLContextPrivate::get(QOpenGLContext::currentContext());
    if (contextPrv && contextPrv->qgl_current_fbo == this) {
        contextPrv->qgl_current_fbo_invalid = true;
        contextPrv->qgl_current_fbo = Q_NULLPTR;
    }
}

/*!
    Creates and attaches an additional texture or renderbuffer of \a size width
    and height.

    There is always an attachment at GL_COLOR_ATTACHMENT0. Call this function
    to set up additional attachments at GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2, ...

    When \a internalFormat is not \c 0, it specifies the internal format of the
    texture or renderbuffer. Otherwise a default of GL_RGBA or GL_RGBA8 is
    used.

    \note This is only functional when multiple render targets are supported by
    the OpenGL implementation. When that is not the case, the function will not
    add any additional color attachments. Call
    QOpenGLFunctions::hasOpenGLFeature() with
    QOpenGLFunctions::MultipleRenderTargets at runtime to check if MRT is
    supported.

    \note The internal format of the color attachments may differ but there may
    be limitations on the supported combinations, depending on the drivers.

    \note The size of the color attachments may differ but rendering is limited
    to the area that fits all the attachments, according to the OpenGL
    specification. Some drivers may not be fully conformant in this respect,
    however.

    \since 5.6
 */
void QOpenGLFramebufferObject::addColorAttachment(const QSize &size, GLenum internalFormat)
{
    Q_D(QOpenGLFramebufferObject);

    if (!QOpenGLContext::currentContext()->functions()->hasOpenGLFeature(QOpenGLFunctions::MultipleRenderTargets)) {
        qWarning("Multiple render targets not supported, ignoring extra color attachment request");
        return;
    }

    QOpenGLFramebufferObjectPrivate::ColorAttachment color(size, effectiveInternalFormat(internalFormat));
    d->colorAttachments.append(color);
    const int idx = d->colorAttachments.count() - 1;

    if (d->requestedSamples == 0) {
        d->initTexture(idx);
    } else {
        GLint samples = d->requestedSamples;
        d->initColorBuffer(idx, &samples);
    }
}

/*! \overload

    Creates and attaches an additional texture or renderbuffer of size \a width and \a height.

    When \a internalFormat is not \c 0, it specifies the internal format of the texture or
    renderbuffer. Otherwise a default of GL_RGBA or GL_RGBA8 is used.

    \since 5.6
 */
void QOpenGLFramebufferObject::addColorAttachment(int width, int height, GLenum internalFormat)
{
    addColorAttachment(QSize(width, height), internalFormat);
}

/*!
    \fn bool QOpenGLFramebufferObject::isValid() const

    Returns \c true if the framebuffer object is valid.

    The framebuffer can become invalid if the initialization process
    fails, the user attaches an invalid buffer to the framebuffer
    object, or a non-power of two width/height is specified as the
    texture size if the texture target is \c{GL_TEXTURE_2D}.
    The non-power of two limitation does not apply if the OpenGL version
    is 2.0 or higher, or if the GL_ARB_texture_non_power_of_two extension
    is present.

    The framebuffer can also become invalid if the QOpenGLContext that
    the framebuffer was created within is destroyed and there are
    no other shared contexts that can take over ownership of the
    framebuffer.
*/
bool QOpenGLFramebufferObject::isValid() const
{
    Q_D(const QOpenGLFramebufferObject);
    return d->valid && d->fbo_guard && d->fbo_guard->id();
}

/*!
    \fn bool QOpenGLFramebufferObject::bind()

    Switches rendering from the default, windowing system provided
    framebuffer to this framebuffer object.
    Returns \c true upon success, false otherwise.

    \note If takeTexture() was called, a new texture is created and associated
    with the framebuffer object. This is potentially expensive and changes the
    context state (the currently bound texture).

    \sa release()
*/
bool QOpenGLFramebufferObject::bind()
{
    if (!isValid())
        return false;
    Q_D(QOpenGLFramebufferObject);
    QOpenGLContext *current = QOpenGLContext::currentContext();
    if (!current)
        return false;
#ifdef QT_DEBUG
    if (current->shareGroup() != d->fbo_guard->group())
        qWarning("QOpenGLFramebufferObject::bind() called from incompatible context");
#endif

    d->funcs.glBindFramebuffer(GL_FRAMEBUFFER, d->fbo());

    QOpenGLContextPrivate::get(current)->qgl_current_fbo_invalid = true;
    QOpenGLContextPrivate::get(current)->qgl_current_fbo = this;

    if (d->format.samples() == 0) {
        // Create new textures to replace the ones stolen via takeTexture().
        for (int i = 0; i < d->colorAttachments.count(); ++i) {
            if (!d->colorAttachments.at(i).guard)
                d->initTexture(i);
        }
    }

    return d->valid;
}

/*!
    \fn bool QOpenGLFramebufferObject::release()

    Switches rendering back to the default, windowing system provided
    framebuffer.
    Returns \c true upon success, false otherwise.

    \sa bind()
*/
bool QOpenGLFramebufferObject::release()
{
    if (!isValid())
        return false;

    QOpenGLContext *current = QOpenGLContext::currentContext();
    if (!current)
        return false;

    Q_D(QOpenGLFramebufferObject);
#ifdef QT_DEBUG
    if (current->shareGroup() != d->fbo_guard->group())
        qWarning("QOpenGLFramebufferObject::release() called from incompatible context");
#endif

    if (current) {
        d->funcs.glBindFramebuffer(GL_FRAMEBUFFER, current->defaultFramebufferObject());

        QOpenGLContextPrivate *contextPrv = QOpenGLContextPrivate::get(current);
        contextPrv->qgl_current_fbo_invalid = true;
        contextPrv->qgl_current_fbo = Q_NULLPTR;
    }

    return true;
}

/*!
    \fn GLuint QOpenGLFramebufferObject::texture() const

    Returns the texture id for the texture attached as the default
    rendering target in this framebuffer object. This texture id can
    be bound as a normal texture in your own OpenGL code.

    If a multisample framebuffer object is used then the value returned
    from this function will be invalid.

    When multiple textures are attached, the return value is the ID of
    the first one.

    \sa takeTexture(), textures()
*/
GLuint QOpenGLFramebufferObject::texture() const
{
    Q_D(const QOpenGLFramebufferObject);
    return d->colorAttachments[0].guard ? d->colorAttachments[0].guard->id() : 0;
}

/*!
    Returns the texture id for all attached textures.

    If a multisample framebuffer object is used, then an empty vector is returned.

    \since 5.6

    \sa takeTexture(), texture()
*/
QVector<GLuint> QOpenGLFramebufferObject::textures() const
{
    Q_D(const QOpenGLFramebufferObject);
    QVector<GLuint> ids;
    if (d->format.samples() != 0)
        return ids;
    ids.reserve(d->colorAttachments.count());
    for (const auto &color : d->colorAttachments)
        ids.append(color.guard ? color.guard->id() : 0);
    return ids;
}

/*!
   \fn GLuint QOpenGLFramebufferObject::takeTexture()

   Returns the texture id for the texture attached to this framebuffer
   object. The ownership of the texture is transferred to the caller.

   If the framebuffer object is currently bound, an implicit release()
   will be done. During the next call to bind() a new texture will be
   created.

   If a multisample framebuffer object is used, then there is no
   texture and the return value from this function will be invalid.
   Similarly, incomplete framebuffer objects will also return 0.

   \since 5.3

   \sa texture(), bind(), release()
 */
GLuint QOpenGLFramebufferObject::takeTexture()
{
    return takeTexture(0);
}

/*! \overload

   Returns the texture id for the texture attached to the color attachment of
   index \a colorAttachmentIndex of this framebuffer object. The ownership of
   the texture is transferred to the caller.

   When \a colorAttachmentIndex is \c 0, the behavior is identical to the
   parameter-less variant of this function.

   If the framebuffer object is currently bound, an implicit release()
   will be done. During the next call to bind() a new texture will be
   created.

   If a multisample framebuffer object is used, then there is no
   texture and the return value from this function will be invalid.
   Similarly, incomplete framebuffer objects will also return 0.

   \since 5.6
 */
GLuint QOpenGLFramebufferObject::takeTexture(int colorAttachmentIndex)
{
    Q_D(QOpenGLFramebufferObject);
    GLuint id = 0;
    if (isValid() && d->format.samples() == 0 && d->colorAttachments.count() > colorAttachmentIndex) {
        QOpenGLContext *current = QOpenGLContext::currentContext();
        if (current && current->shareGroup() == d->fbo_guard->group() && isBound())
            release();
        auto &guard = d->colorAttachments[colorAttachmentIndex].guard;
        id = guard ? guard->id() : 0;
        // Do not call free() on texture_guard, just null it out.
        // This way the texture will not be deleted when the guard is destroyed.
        guard = 0;
    }
    return id;
}

/*!
    \return the size of the color and depth/stencil attachments attached to
    this framebuffer object.
*/
QSize QOpenGLFramebufferObject::size() const
{
    Q_D(const QOpenGLFramebufferObject);
    return d->dsSize;
}

/*!
    \return the sizes of all color attachments attached to this framebuffer
    object.

    \since 5.6
*/
QVector<QSize> QOpenGLFramebufferObject::sizes() const
{
    Q_D(const QOpenGLFramebufferObject);
    QVector<QSize> sz;
    sz.reserve(d->colorAttachments.size());
    for (const auto &color : d->colorAttachments)
        sz.append(color.size);
    return sz;
}

/*!
    \fn int QOpenGLFramebufferObject::width() const

    Returns the width of the framebuffer object attachments.
*/

/*!
    \fn int QOpenGLFramebufferObject::height() const

    Returns the height of the framebuffer object attachments.
*/

/*!
    Returns the format of this framebuffer object.
*/
QOpenGLFramebufferObjectFormat QOpenGLFramebufferObject::format() const
{
    Q_D(const QOpenGLFramebufferObject);
    return d->format;
}

static inline QImage qt_gl_read_framebuffer_rgba8(const QSize &size, bool include_alpha, QOpenGLContext *context)
{
    QOpenGLFunctions *funcs = context->functions();
    const int w = size.width();
    const int h = size.height();
    bool isOpenGL12orBetter = !context->isOpenGLES() && (context->format().majorVersion() >= 2 || context->format().minorVersion() >= 2);
    if (isOpenGL12orBetter) {
        QImage img(size, include_alpha ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);
        funcs->glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, img.bits());
        return img;
    }

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    // Without GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA only makes sense on little endian.
    const bool has_bgra_ext = context->isOpenGLES()
                              ? context->hasExtension(QByteArrayLiteral("GL_EXT_read_format_bgra"))
                              : context->hasExtension(QByteArrayLiteral("GL_EXT_bgra"));

#ifndef Q_OS_IOS
    const char *renderer = reinterpret_cast<const char *>(funcs->glGetString(GL_RENDERER));
    const char *ver = reinterpret_cast<const char *>(funcs->glGetString(GL_VERSION));

    // Blacklist GPU chipsets that have problems with their BGRA support.
    const bool blackListed = (qstrcmp(renderer, "PowerVR Rogue G6200") == 0
                             && ::strstr(ver, "1.3") != 0) ||
                             (qstrcmp(renderer, "Mali-T760") == 0
                             && ::strstr(ver, "3.1") != 0) ||
                             (qstrcmp(renderer, "Mali-T720") == 0
                             && ::strstr(ver, "3.1") != 0) ||
                             qstrcmp(renderer, "PowerVR SGX 554") == 0;
#else
    const bool blackListed = true;
#endif
    const bool supports_bgra = has_bgra_ext && !blackListed;

    if (supports_bgra) {
        QImage img(size, include_alpha ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);
        funcs->glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, img.bits());
        return img;
    }
#endif
    QImage rgbaImage(size, include_alpha ? QImage::Format_RGBA8888_Premultiplied : QImage::Format_RGBX8888);
    funcs->glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgbaImage.bits());
    return rgbaImage;
}

static inline QImage qt_gl_read_framebuffer_rgb10a2(const QSize &size, bool include_alpha, QOpenGLContext *context)
{
    // We assume OpenGL 1.2+ or ES 3.0+ here.
    QImage img(size, include_alpha ? QImage::Format_A2BGR30_Premultiplied : QImage::Format_BGR30);
    context->functions()->glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, img.bits());
    return img;
}

static QImage qt_gl_read_framebuffer(const QSize &size, GLenum internal_format, bool include_alpha, bool flip)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QOpenGLFunctions *funcs = ctx->functions();
    while (funcs->glGetError());

    switch (internal_format) {
    case GL_RGB:
    case GL_RGB8:
        return qt_gl_read_framebuffer_rgba8(size, false, ctx).mirrored(false, flip);
    case GL_RGB10:
        return qt_gl_read_framebuffer_rgb10a2(size, false, ctx).mirrored(false, flip);
    case GL_RGB10_A2:
        return qt_gl_read_framebuffer_rgb10a2(size, include_alpha, ctx).mirrored(false, flip);
    case GL_RGBA:
    case GL_RGBA8:
    default:
        return qt_gl_read_framebuffer_rgba8(size, include_alpha, ctx).mirrored(false, flip);
    }

    Q_UNREACHABLE();
    return QImage();
}

Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha)
{
    return qt_gl_read_framebuffer(size, alpha_format ? GL_RGBA : GL_RGB, include_alpha, true);
}

/*!
    \fn QImage QOpenGLFramebufferObject::toImage(bool flipped) const

    Returns the contents of this framebuffer object as a QImage.

    If \a flipped is true the image is flipped from OpenGL coordinates to raster coordinates.
    If used together with QOpenGLPaintDevice, \a flipped should be the opposite of the value
    of QOpenGLPaintDevice::paintFlipped().

    The returned image has a format of premultiplied ARGB32 or RGB32. The latter is used
    only when internalTextureFormat() is set to \c GL_RGB.

    If the rendering in the framebuffer was not done with premultiplied alpha in mind,
    create a wrapper QImage with a non-premultiplied format. This is necessary before
    performing operations like QImage::save() because otherwise the image data would get
    unpremultiplied, even though it was not premultiplied in the first place. To create
    such a wrapper without performing a copy of the pixel data, do the following:

    \code
    QImage fboImage(fbo.toImage());
    QImage image(fboImage.constBits(), fboImage.width(), fboImage.height(), QImage::Format_ARGB32);
    \endcode

    Since Qt 5.2 the function will fall back to premultiplied RGBA8888 or RGBx8888 when
    reading to (A)RGB32 is not supported. Since 5.4 an A2BGR30 image is returned if the
    internal format is RGB10_A2.

    For multisampled framebuffer objects the samples are resolved using the
    \c{GL_EXT_framebuffer_blit} extension. If the extension is not available, the contents
    of the returned image is undefined.

    For singlesampled framebuffers the contents is retrieved via \c glReadPixels. This is
    a potentially expensive and inefficient operation. Therefore it is recommended that
    this function is used as seldom as possible.

    \sa QOpenGLPaintDevice::paintFlipped()
*/

QImage QOpenGLFramebufferObject::toImage(bool flipped) const
{
    return toImage(flipped, 0);
}

/*!
    \fn QImage QOpenGLFramebufferObject::toImage() const
    \overload

    Returns the contents of this framebuffer object as a QImage. This method flips
    the image from OpenGL coordinates to raster coordinates.
*/
// ### Qt 6: Remove this method and make it a default argument instead.
QImage QOpenGLFramebufferObject::toImage() const
{
    return toImage(true, 0);
}

/*! \overload

    Returns the contents of the color attachment of index \a
    colorAttachmentIndex of this framebuffer object as a QImage. This method
    flips the image from OpenGL coordinates to raster coordinates when \a
    flipped is set to \c true.

    \note This overload is only fully functional when multiple render targets are
    supported by the OpenGL implementation. When that is not the case, only one
    color attachment will be set up.

    \since 5.6
*/
QImage QOpenGLFramebufferObject::toImage(bool flipped, int colorAttachmentIndex) const
{
    Q_D(const QOpenGLFramebufferObject);
    if (!d->valid)
        return QImage();

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning("QOpenGLFramebufferObject::toImage() called without a current context");
        return QImage();
    }

    if (d->colorAttachments.count() <= colorAttachmentIndex) {
        qWarning("QOpenGLFramebufferObject::toImage() called for missing color attachment");
        return QImage();
    }

    GLuint prevFbo = 0;
    ctx->functions()->glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *) &prevFbo);

    if (prevFbo != d->fbo())
        const_cast<QOpenGLFramebufferObject *>(this)->bind();

    QImage image;
    QOpenGLExtraFunctions *extraFuncs = ctx->extraFunctions();
    // qt_gl_read_framebuffer doesn't work on a multisample FBO
    if (format().samples() != 0) {
        QRect rect(QPoint(0, 0), size());
        if (extraFuncs->hasOpenGLFeature(QOpenGLFunctions::MultipleRenderTargets)) {
            QOpenGLFramebufferObject temp(d->colorAttachments[colorAttachmentIndex].size, QOpenGLFramebufferObjectFormat());
            blitFramebuffer(&temp, rect, const_cast<QOpenGLFramebufferObject *>(this), rect,
                            GL_COLOR_BUFFER_BIT, GL_NEAREST,
                            colorAttachmentIndex, 0);
            image = temp.toImage(flipped);
        } else {
            QOpenGLFramebufferObject temp(size(), QOpenGLFramebufferObjectFormat());
            blitFramebuffer(&temp, rect, const_cast<QOpenGLFramebufferObject *>(this), rect);
            image = temp.toImage(flipped);
        }
    } else {
        if (extraFuncs->hasOpenGLFeature(QOpenGLFunctions::MultipleRenderTargets)) {
            extraFuncs->glReadBuffer(GL_COLOR_ATTACHMENT0 + colorAttachmentIndex);
            image = qt_gl_read_framebuffer(d->colorAttachments[colorAttachmentIndex].size,
                                           d->colorAttachments[colorAttachmentIndex].internalFormat,
                                           true, flipped);
            extraFuncs->glReadBuffer(GL_COLOR_ATTACHMENT0);
        } else {
            image = qt_gl_read_framebuffer(d->colorAttachments[0].size,
                                           d->colorAttachments[0].internalFormat,
                                           true, flipped);
        }
    }

    if (prevFbo != d->fbo())
        ctx->functions()->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);

    return image;
}

/*!
    \fn bool QOpenGLFramebufferObject::bindDefault()

    Switches rendering back to the default, windowing system provided
    framebuffer.
    Returns \c true upon success, false otherwise.

    \sa bind(), release()
*/
bool QOpenGLFramebufferObject::bindDefault()
{
    QOpenGLContext *ctx = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());

    if (ctx) {
        ctx->functions()->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
        QOpenGLContextPrivate::get(ctx)->qgl_current_fbo_invalid = true;
        QOpenGLContextPrivate::get(ctx)->qgl_current_fbo = Q_NULLPTR;
    }
#ifdef QT_DEBUG
    else
        qWarning("QOpenGLFramebufferObject::bindDefault() called without current context.");
#endif

    return ctx != 0;
}

/*!
    \fn bool QOpenGLFramebufferObject::hasOpenGLFramebufferObjects()

    Returns \c true if the OpenGL \c{GL_EXT_framebuffer_object} extension
    is present on this system; otherwise returns \c false.
*/
bool QOpenGLFramebufferObject::hasOpenGLFramebufferObjects()
{
    return QOpenGLContext::currentContext()->functions()->hasOpenGLFeature(QOpenGLFunctions::Framebuffers);
}

/*!
    \fn GLuint QOpenGLFramebufferObject::handle() const

    Returns the OpenGL framebuffer object handle for this framebuffer
    object (returned by the \c{glGenFrameBuffersEXT()} function). This
    handle can be used to attach new images or buffers to the
    framebuffer. The user is responsible for cleaning up and
    destroying these objects.
*/
GLuint QOpenGLFramebufferObject::handle() const
{
    Q_D(const QOpenGLFramebufferObject);
    return d->fbo();
}

/*!
    Returns the status of the depth and stencil buffers attached to
    this framebuffer object.
*/

QOpenGLFramebufferObject::Attachment QOpenGLFramebufferObject::attachment() const
{
    Q_D(const QOpenGLFramebufferObject);
    if (d->valid)
        return d->fbo_attachment;
    return NoAttachment;
}

/*!
    Sets the attachments of the framebuffer object to \a attachment.

    This can be used to free or reattach the depth and stencil buffer
    attachments as needed.

    \note This function alters the current framebuffer binding.
 */
void QOpenGLFramebufferObject::setAttachment(QOpenGLFramebufferObject::Attachment attachment)
{
    Q_D(QOpenGLFramebufferObject);
    if (attachment == d->fbo_attachment || !isValid())
        return;
    QOpenGLContext *current = QOpenGLContext::currentContext();
    if (!current)
        return;
#ifdef QT_DEBUG
    if (current->shareGroup() != d->fbo_guard->group())
        qWarning("QOpenGLFramebufferObject::setAttachment() called from incompatible context");
#endif
    d->funcs.glBindFramebuffer(GL_FRAMEBUFFER, d->fbo());
    QOpenGLContextPrivate::get(current)->qgl_current_fbo_invalid = true;
    d->initDepthStencilAttachments(current, attachment);
}

/*!
    Returns \c true if the framebuffer object is currently bound to the current context,
    otherwise false is returned.
*/
bool QOpenGLFramebufferObject::isBound() const
{
    Q_D(const QOpenGLFramebufferObject);
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx)
        return false;
    GLint fbo = 0;
    ctx->functions()->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    return GLuint(fbo) == d->fbo();
}

/*!
    \fn bool QOpenGLFramebufferObject::hasOpenGLFramebufferBlit()

    Returns \c true if the OpenGL \c{GL_EXT_framebuffer_blit} extension
    is present on this system; otherwise returns \c false.

    \sa blitFramebuffer()
*/
bool QOpenGLFramebufferObject::hasOpenGLFramebufferBlit()
{
    return QOpenGLExtensions(QOpenGLContext::currentContext()).hasOpenGLExtension(QOpenGLExtensions::FramebufferBlit);
}


/*!
    \overload

    Convenience overload to blit between two framebuffer objects.
*/
void QOpenGLFramebufferObject::blitFramebuffer(QOpenGLFramebufferObject *target,
                                               QOpenGLFramebufferObject *source,
                                               GLbitfield buffers, GLenum filter)
{
    if (!target && !source)
        return;

    QSize targetSize;
    QSize sourceSize;

    if (target)
        targetSize = target->size();
    if (source)
        sourceSize = source->size();

    if (targetSize.isEmpty())
        targetSize = sourceSize;
    else if (sourceSize.isEmpty())
        sourceSize = targetSize;

    blitFramebuffer(target, QRect(QPoint(0, 0), targetSize),
                    source, QRect(QPoint(0, 0), sourceSize),
                    buffers, filter);
}

/*! \overload
 *
    Convenience overload to blit between two framebuffer objects.
*/
void QOpenGLFramebufferObject::blitFramebuffer(QOpenGLFramebufferObject *target, const QRect &targetRect,
                                               QOpenGLFramebufferObject *source, const QRect &sourceRect,
                                               GLbitfield buffers,
                                               GLenum filter)
{
    blitFramebuffer(target, targetRect, source, sourceRect, buffers, filter, 0, 0);
}

/*!
    \enum QOpenGLFramebufferObject::FramebufferRestorePolicy
    \since 5.7

    This enum type is used to configure the behavior related to restoring
    framebuffer bindings when calling blitFramebuffer().

    \value DontRestoreFramebufferBinding        Do not restore the previous framebuffer binding.
                                                The caller is responsible for tracking and setting
                                                the framebuffer binding as needed.

    \value RestoreFramebufferBindingToDefault   After the blit operation, bind the default
                                                framebuffer.

    \value RestoreFrameBufferBinding            Restore the previously bound framebuffer. This is
                                                potentially expensive because of the need to
                                                query the currently bound framebuffer.

    \sa blitFramebuffer()
*/

/*!
    \since 5.7

    Blits from the \a sourceRect rectangle in the \a source framebuffer
    object to the \a targetRect rectangle in the \a target framebuffer object.

    If \a source or \a target is 0, the default framebuffer will be used
    instead of a framebuffer object as source or target respectively.

    This function will have no effect unless hasOpenGLFramebufferBlit() returns
    true.

    The \a buffers parameter should be a mask consisting of any combination of
    \c GL_COLOR_BUFFER_BIT, \c GL_DEPTH_BUFFER_BIT, and
    \c GL_STENCIL_BUFFER_BIT.  Any buffer type that is not present both
    in the source and target buffers is ignored.

    The \a sourceRect and \a targetRect rectangles may have different sizes;
    in this case \a buffers should not contain \c GL_DEPTH_BUFFER_BIT or
    \c GL_STENCIL_BUFFER_BIT. The \a filter parameter should be set to
    \c GL_LINEAR or \c GL_NEAREST, and specifies whether linear or nearest
    interpolation should be used when scaling is performed.

    If \a source equals \a target a copy is performed within the same buffer.
    Results are undefined if the source and target rectangles overlap and
    have different sizes. The sizes must also be the same if any of the
    framebuffer objects are multisample framebuffers.

    \note The scissor test will restrict the blit area if enabled.

    When multiple render targets are in use, \a readColorAttachmentIndex and \a
    drawColorAttachmentIndex specify the index of the color attachments in the
    source and destination framebuffers.

    The \a restorePolicy determines if the framebuffer that was bound prior to
    calling this function should be restored, or if the default framebuffer
    should be bound before returning, of if the caller is responsible for
    tracking and setting the bound framebuffer. Restoring the previous
    framebuffer can be relatively expensive due to the call to \c{glGetIntegerv}
    which on some OpenGL drivers may imply a pipeline stall.

    \sa hasOpenGLFramebufferBlit()
*/
void QOpenGLFramebufferObject::blitFramebuffer(QOpenGLFramebufferObject *target, const QRect &targetRect,
                                               QOpenGLFramebufferObject *source, const QRect &sourceRect,
                                               GLbitfield buffers,
                                               GLenum filter,
                                               int readColorAttachmentIndex,
                                               int drawColorAttachmentIndex,
                                               QOpenGLFramebufferObject::FramebufferRestorePolicy restorePolicy)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx)
        return;

    QOpenGLExtensions extensions(ctx);
    if (!extensions.hasOpenGLExtension(QOpenGLExtensions::FramebufferBlit))
        return;

    GLuint prevFbo = 0;
    if (restorePolicy == RestoreFrameBufferBinding)
        ctx->functions()->glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *) &prevFbo);

    const int sx0 = sourceRect.left();
    const int sx1 = sourceRect.left() + sourceRect.width();
    const int sy0 = sourceRect.top();
    const int sy1 = sourceRect.top() + sourceRect.height();

    const int tx0 = targetRect.left();
    const int tx1 = targetRect.left() + targetRect.width();
    const int ty0 = targetRect.top();
    const int ty1 = targetRect.top() + targetRect.height();

    const GLuint defaultFboId = ctx->defaultFramebufferObject();

    extensions.glBindFramebuffer(GL_READ_FRAMEBUFFER, source ? source->handle() : defaultFboId);
    extensions.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target ? target->handle() : defaultFboId);

    const bool supportsMRT = extensions.hasOpenGLFeature(QOpenGLFunctions::MultipleRenderTargets);
    if (supportsMRT) {
        extensions.glReadBuffer(GL_COLOR_ATTACHMENT0 + readColorAttachmentIndex);
        if (target) {
            GLenum drawBuf = GL_COLOR_ATTACHMENT0 + drawColorAttachmentIndex;
            extensions.glDrawBuffers(1, &drawBuf);
        }
    }

    extensions.glBlitFramebuffer(sx0, sy0, sx1, sy1,
                                 tx0, ty0, tx1, ty1,
                                 buffers, filter);

    if (supportsMRT)
        extensions.glReadBuffer(GL_COLOR_ATTACHMENT0);

    switch (restorePolicy) {
    case RestoreFrameBufferBinding:
        ctx->functions()->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo); // sets both READ and DRAW
        break;

    case RestoreFramebufferBindingToDefault:
        ctx->functions()->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject()); // sets both READ and DRAW
        break;

    case DontRestoreFramebufferBinding:
        break;
    }
}

/*!
   \overload

    Convenience overload to blit between two framebuffer objects and
    to restore the previous framebuffer binding. Equivalent to calling
    blitFramebuffer(target, targetRect, source, sourceRect, buffers, filter,
    readColorAttachmentIndex, drawColorAttachmentIndex,
    RestoreFrameBufferBinding).
*/
void QOpenGLFramebufferObject::blitFramebuffer(QOpenGLFramebufferObject *target, const QRect &targetRect,
                                               QOpenGLFramebufferObject *source, const QRect &sourceRect,
                                               GLbitfield buffers,
                                               GLenum filter,
                                               int readColorAttachmentIndex,
                                               int drawColorAttachmentIndex)
{
    blitFramebuffer(target, targetRect, source, sourceRect,
                    buffers, filter,
                    readColorAttachmentIndex,
                    drawColorAttachmentIndex,
                    RestoreFrameBufferBinding);
}

QT_END_NAMESPACE
