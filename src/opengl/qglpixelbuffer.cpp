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

/*!
    \class QGLPixelBuffer
    \brief The QGLPixelBuffer class encapsulates an OpenGL pbuffer.
    \since 4.1

    \ingroup painting-3D

    Rendering into a pbuffer is normally done using full hardware
    acceleration. This can be significantly faster than rendering
    into a QPixmap.

    There are three approaches to using this class:

    \list 1
    \o \bold{We can draw into the pbuffer and convert it to a QImage
       using toImage().} This is normally much faster than calling
       QGLWidget::renderPixmap().

    \o \bold{We can draw into the pbuffer and copy the contents into
       an OpenGL texture using updateDynamicTexture().} This allows
       us to create dynamic textures and works on all systems
       with pbuffer support.

    \o \bold{On systems that support it, we can bind the pbuffer to
       an OpenGL texture.} The texture is then updated automatically
       when the pbuffer contents change, eliminating the need for
       additional copy operations. This is supported only on Windows
       and Mac OS X systems that provide the \c render_texture
       extension. Note that under Windows, a multi-sampled pbuffer
       can't be used in conjunction with the \c render_texture
       extension. If a multi-sampled pbuffer is requested under
       Windows, the \c render_texture extension is turned off for that
       pbuffer.


    \endlist


    \section1 Threading

    As of Qt 4.8, it's possible to render into a QGLPixelBuffer using
    a QPainter in a separate thread. Note that OpenGL 2.0 or OpenGL ES
    2.0 is required for this to work. Also, under X11, it's necessary
    to set the Qt::AA_X11InitThreads application attribute.

    Pbuffers are provided by the OpenGL \c pbuffer extension; call
    hasOpenGLPbuffer() to find out if the system provides pbuffers.

    \sa {opengl/pbuffers}{Pbuffers Example}
*/

#include <QtCore/qglobal.h>

#if !defined(QT_OPENGL_ES_1)
#include <private/qpaintengineex_opengl2_p.h>
#endif

#include <qglpixelbuffer.h>
#include <private/qglpixelbuffer_p.h>
#include <private/qfont_p.h>
#include <qimage.h>

#ifndef QT_OPENGL_ES_2
#include <private/qpaintengine_opengl_p.h>
#endif

QT_BEGIN_NAMESPACE

#if !defined(QT_OPENGL_ES_2)
extern void qgl_cleanup_glyph_cache(QGLContext *);
#else
void qgl_cleanup_glyph_cache(QGLContext *) {}
#endif

extern QImage qt_gl_read_framebuffer(const QSize&, bool, bool);


QGLContext* QGLPBufferGLPaintDevice::context() const
{
    return pbuf->d_func()->qctx;
}

void QGLPBufferGLPaintDevice::endPaint() {
    glFlush();
    QGLPaintDevice::endPaint();
}

void QGLPBufferGLPaintDevice::setPBuffer(QGLPixelBuffer* pb)
{
    pbuf = pb;
}

void QGLPixelBufferPrivate::common_init(const QSize &size, const QGLFormat &format, QGLWidget *shareWidget)
{
    Q_Q(QGLPixelBuffer);
    if(init(size, format, shareWidget)) {
        req_size = size;
        req_format = format;
        req_shareWidget = shareWidget;
        invalid = false;
        qctx = new QGLContext(format);
        qctx->d_func()->sharing = (shareWidget != 0);
        if (shareWidget != 0 && shareWidget->d_func()->glcx) {
            QGLContextGroup::addShare(qctx, shareWidget->d_func()->glcx);
            shareWidget->d_func()->glcx->d_func()->sharing = true;
        }

        glDevice.setPBuffer(q);
        qctx->d_func()->paintDevice = q;
        qctx->d_func()->valid = true;
#if defined(Q_WS_WIN) && !defined(QT_OPENGL_ES)
        qctx->d_func()->dc = dc;
        qctx->d_func()->rc = ctx;
#elif (defined(Q_WS_X11) && defined(QT_NO_EGL))
        qctx->d_func()->cx = ctx;
        qctx->d_func()->pbuf = (void *) pbuf;
        qctx->d_func()->vi = 0;
#elif defined(Q_WS_MAC)
        qctx->d_func()->cx = ctx;
        qctx->d_func()->vi = 0;
#elif !defined(QT_NO_EGL)
        qctx->d_func()->eglContext = ctx;
        qctx->d_func()->eglSurface = pbuf;
#endif
    }
}

/*!
    Constructs an OpenGL pbuffer of the given \a size. If no \a
    format is specified, the \l{QGLFormat::defaultFormat()}{default
    format} is used. If the \a shareWidget parameter points to a
    valid QGLWidget, the pbuffer will share its context with \a
    shareWidget.

    If you intend to bind this pbuffer as a dynamic texture, the width
    and height components of \c size must be powers of two (e.g., 512
    x 128).

    \sa size(), format()
*/
QGLPixelBuffer::QGLPixelBuffer(const QSize &size, const QGLFormat &format, QGLWidget *shareWidget)
    : d_ptr(new QGLPixelBufferPrivate(this))
{
    Q_D(QGLPixelBuffer);
    d->common_init(size, format, shareWidget);
}


/*! \overload

    Constructs an OpenGL pbuffer with the \a width and \a height. If
    no \a format is specified, the
    \l{QGLFormat::defaultFormat()}{default format} is used. If the \a
    shareWidget parameter points to a valid QGLWidget, the pbuffer
    will share its context with \a shareWidget.

    If you intend to bind this pbuffer as a dynamic texture, the width
    and height components of \c size must be powers of two (e.g., 512
    x 128).

    \sa size(), format()
*/
QGLPixelBuffer::QGLPixelBuffer(int width, int height, const QGLFormat &format, QGLWidget *shareWidget)
    : d_ptr(new QGLPixelBufferPrivate(this))
{
    Q_D(QGLPixelBuffer);
    d->common_init(QSize(width, height), format, shareWidget);
}


/*! \fn QGLPixelBuffer::~QGLPixelBuffer()

    Destroys the pbuffer and frees any allocated resources.
*/
QGLPixelBuffer::~QGLPixelBuffer()
{
    Q_D(QGLPixelBuffer);

    // defined in qpaintengine_opengl.cpp
    QGLContext *current = const_cast<QGLContext *>(QGLContext::currentContext());
    if (current != d->qctx)
        makeCurrent();
    qgl_cleanup_glyph_cache(d->qctx);
    d->cleanup();
    delete d->qctx;
    if (current && current != d->qctx)
        current->makeCurrent();
}

/*! \fn bool QGLPixelBuffer::makeCurrent()

    Makes this pbuffer the current OpenGL rendering context. Returns
    true on success; otherwise returns false.

    \sa QGLContext::makeCurrent(), doneCurrent()
*/

bool QGLPixelBuffer::makeCurrent()
{
    Q_D(QGLPixelBuffer);
    if (d->invalid)
        return false;
    d->qctx->makeCurrent();
    return true;
}

/*! \fn bool QGLPixelBuffer::doneCurrent()

    Makes no context the current OpenGL context. Returns true on
    success; otherwise returns false.
*/

bool QGLPixelBuffer::doneCurrent()
{
    Q_D(QGLPixelBuffer);
    if (d->invalid)
        return false;
    d->qctx->doneCurrent();
    return true;
}

/*!
    Generates and binds a 2D GL texture that is the same size as the
    pbuffer, and returns the texture's ID. This can be used in
    conjunction with bindToDynamicTexture() and
    updateDynamicTexture().

    \sa size()
*/

#if (defined(Q_WS_X11) || defined(Q_WS_WIN)) && defined(QT_NO_EGL)
GLuint QGLPixelBuffer::generateDynamicTexture() const
{
    Q_D(const QGLPixelBuffer);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, d->req_size.width(), d->req_size.height(), 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return texture;
}
#endif

/*! \fn bool QGLPixelBuffer::bindToDynamicTexture(GLuint texture_id)

    Binds the texture specified by \a texture_id to this pbuffer.
    Returns true on success; otherwise returns false.

    The texture must be of the same size and format as the pbuffer.

    To unbind the texture, call releaseFromDynamicTexture(). While
    the texture is bound, it is updated automatically when the
    pbuffer contents change, eliminating the need for additional copy
    operations.

    Example:

    \snippet doc/src/snippets/code/src_opengl_qglpixelbuffer.cpp 0

    \warning This function uses the \c {render_texture} extension,
    which is currently not supported under X11. An alternative that
    works on all systems (including X11) is to manually copy the
    pbuffer contents to a texture using updateDynamicTexture().

    \warning For the bindToDynamicTexture() call to succeed on the
    Mac OS X, the pbuffer needs a shared context, i.e. the
    QGLPixelBuffer must be created with a share widget.

    \sa generateDynamicTexture(), releaseFromDynamicTexture()
*/

/*! \fn void QGLPixelBuffer::releaseFromDynamicTexture()

    Releases the pbuffer from any previously bound texture.

    \sa bindToDynamicTexture()
*/

/*! \fn bool QGLPixelBuffer::hasOpenGLPbuffers()

    Returns true if the OpenGL \c pbuffer extension is present on
    this system; otherwise returns false.
*/

/*!
    Copies the pbuffer contents into the texture specified with \a
    texture_id.

    The texture must be of the same size and format as the pbuffer.

    Example:

    \snippet doc/src/snippets/code/src_opengl_qglpixelbuffer.cpp 1

    An alternative on Windows and Mac OS X systems that support the
    \c render_texture extension is to use bindToDynamicTexture() to
    get dynamic updates of the texture.

    \sa generateDynamicTexture(), bindToDynamicTexture()
*/
void QGLPixelBuffer::updateDynamicTexture(GLuint texture_id) const
{
    Q_D(const QGLPixelBuffer);
    if (d->invalid)
        return;
    glBindTexture(GL_TEXTURE_2D, texture_id);
#ifndef QT_OPENGL_ES
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, d->req_size.width(), d->req_size.height(), 0);
#else
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, d->req_size.width(), d->req_size.height(), 0);
#endif
}

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
void QGLPixelBuffer::updateDynamicTexture(QMacCompatGLuint texture_id) const
{
    updateDynamicTexture(GLuint(texture_id));
}
#endif

/*!
    Returns the size of the pbuffer.
*/
QSize QGLPixelBuffer::size() const
{
    Q_D(const QGLPixelBuffer);
    return d->req_size;
}

/*!
    Returns the contents of the pbuffer as a QImage.
*/
QImage QGLPixelBuffer::toImage() const
{
    Q_D(const QGLPixelBuffer);
    if (d->invalid)
        return QImage();

    const_cast<QGLPixelBuffer *>(this)->makeCurrent();
    return qt_gl_read_framebuffer(d->req_size, d->format.alpha(), true);
}

/*!
    Returns the native pbuffer handle.
*/
Qt::HANDLE QGLPixelBuffer::handle() const
{
    Q_D(const QGLPixelBuffer);
    if (d->invalid)
        return 0;
    return (Qt::HANDLE) d->pbuf;
}

/*!
    Returns true if this pbuffer is valid; otherwise returns false.
*/
bool QGLPixelBuffer::isValid() const
{
    Q_D(const QGLPixelBuffer);
    return !d->invalid;
}

#if !defined(QT_OPENGL_ES_1)
Q_GLOBAL_STATIC(QGLEngineThreadStorage<QGL2PaintEngineEx>, qt_buffer_2_engine)
#endif

#ifndef QT_OPENGL_ES_2
Q_GLOBAL_STATIC(QGLEngineThreadStorage<QOpenGLPaintEngine>, qt_buffer_engine)
#endif

/*! \reimp */
QPaintEngine *QGLPixelBuffer::paintEngine() const
{
#if defined(QT_OPENGL_ES_1)
    return qt_buffer_engine()->engine();
#elif defined(QT_OPENGL_ES_2)
    return qt_buffer_2_engine()->engine();
#else
    if (qt_gl_preferGL2Engine())
        return qt_buffer_2_engine()->engine();
    else
        return qt_buffer_engine()->engine();
#endif
}

/*! \reimp */
int QGLPixelBuffer::metric(PaintDeviceMetric metric) const
{
    Q_D(const QGLPixelBuffer);

    float dpmx = qt_defaultDpiX()*100./2.54;
    float dpmy = qt_defaultDpiY()*100./2.54;
    int w = d->req_size.width();
    int h = d->req_size.height();
    switch (metric) {
    case PdmWidth:
        return w;

    case PdmHeight:
        return h;

    case PdmWidthMM:
        return qRound(w * 1000 / dpmx);

    case PdmHeightMM:
        return qRound(h * 1000 / dpmy);

    case PdmNumColors:
        return 0;

    case PdmDepth:
        return 32;//d->depth;

    case PdmDpiX:
        return qRound(dpmx * 0.0254);

    case PdmDpiY:
        return qRound(dpmy * 0.0254);

    case PdmPhysicalDpiX:
        return qRound(dpmx * 0.0254);

    case PdmPhysicalDpiY:
        return qRound(dpmy * 0.0254);

    default:
        qWarning("QGLPixelBuffer::metric(), Unhandled metric type: %d\n", metric);
        break;
    }
    return 0;
}

/*!
    Generates and binds a 2D GL texture to the current context, based
    on \a image. The generated texture id is returned and can be used
    in later glBindTexture() calls.

    The \a target parameter specifies the texture target.

    Equivalent to calling QGLContext::bindTexture().

    \sa deleteTexture()
*/
GLuint QGLPixelBuffer::bindTexture(const QImage &image, GLenum target)
{
    Q_D(QGLPixelBuffer);
#ifndef QT_OPENGL_ES
    return d->qctx->bindTexture(image, target, GLint(GL_RGBA8));
#else
    return d->qctx->bindTexture(image, target, GL_RGBA);
#endif
}

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
/*! \internal */
GLuint QGLPixelBuffer::bindTexture(const QImage &image, QMacCompatGLenum target)
{
    Q_D(QGLPixelBuffer);
    return d->qctx->bindTexture(image, target, QMacCompatGLint(GL_RGBA8));
}
#endif

/*! \overload

    Generates and binds a 2D GL texture based on \a pixmap.

    Equivalent to calling QGLContext::bindTexture().

    \sa deleteTexture()
*/
GLuint QGLPixelBuffer::bindTexture(const QPixmap &pixmap, GLenum target)
{
    Q_D(QGLPixelBuffer);
#ifndef QT_OPENGL_ES
    return d->qctx->bindTexture(pixmap, target, GLint(GL_RGBA8));
#else
    return d->qctx->bindTexture(pixmap, target, GL_RGBA);
#endif
}

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
/*! \internal */
GLuint QGLPixelBuffer::bindTexture(const QPixmap &pixmap, QMacCompatGLenum target)
{
    Q_D(QGLPixelBuffer);
    return d->qctx->bindTexture(pixmap, target, QMacCompatGLint(GL_RGBA8));
}
#endif

/*! \overload

    Reads the DirectDrawSurface (DDS) compressed file \a fileName and
    generates a 2D GL texture from it.

    Equivalent to calling QGLContext::bindTexture().

    \sa deleteTexture()
*/
GLuint QGLPixelBuffer::bindTexture(const QString &fileName)
{
    Q_D(QGLPixelBuffer);
    return d->qctx->bindTexture(fileName);
}

/*!
    Removes the texture identified by \a texture_id from the texture cache.

    Equivalent to calling QGLContext::deleteTexture().
 */
void QGLPixelBuffer::deleteTexture(GLuint texture_id)
{
    Q_D(QGLPixelBuffer);
    d->qctx->deleteTexture(texture_id);
}

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
/*! \internal */
void QGLPixelBuffer::deleteTexture(QMacCompatGLuint texture_id)
{
    Q_D(QGLPixelBuffer);
    d->qctx->deleteTexture(texture_id);
}
#endif

/*!
    \since 4.4

    Draws the given texture, \a textureId, to the given target rectangle,
    \a target, in OpenGL model space. The \a textureTarget should be a 2D
    texture target.

    Equivalent to the corresponding QGLContext::drawTexture().
*/
void QGLPixelBuffer::drawTexture(const QRectF &target, GLuint textureId, GLenum textureTarget)
{
    Q_D(QGLPixelBuffer);
    d->qctx->drawTexture(target, textureId, textureTarget);
}

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
/*! \internal */
void QGLPixelBuffer::drawTexture(const QRectF &target, QMacCompatGLuint textureId, QMacCompatGLenum textureTarget)
{
    Q_D(QGLPixelBuffer);
    d->qctx->drawTexture(target, textureId, textureTarget);
}
#endif

/*!
    \since 4.4

    Draws the given texture, \a textureId, at the given \a point in OpenGL model
    space. The textureTarget parameter should be a 2D texture target.

    Equivalent to the corresponding QGLContext::drawTexture().
*/
void QGLPixelBuffer::drawTexture(const QPointF &point, GLuint textureId, GLenum textureTarget)
{
    Q_D(QGLPixelBuffer);
    d->qctx->drawTexture(point, textureId, textureTarget);
}

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
/*! \internal */
void QGLPixelBuffer::drawTexture(const QPointF &point, QMacCompatGLuint textureId, QMacCompatGLenum textureTarget)
{
    Q_D(QGLPixelBuffer);
    d->qctx->drawTexture(point, textureId, textureTarget);
}
#endif

/*!
    Returns the format of the pbuffer. The format may be different
    from the one that was requested.
*/
QGLFormat QGLPixelBuffer::format() const
{
    Q_D(const QGLPixelBuffer);
    return d->format;
}

/*! \fn int QGLPixelBuffer::devType() const
    \internal
*/

QT_END_NAMESPACE
