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

#include "qpixmap.h"
#include "qglframebufferobject.h"

#include <private/qpaintengine_raster_p.h>

#include "qpixmapdata_gl_p.h"

#include <private/qgl_p.h>
#include <private/qdrawhelper_p.h>
#include <private/qimage_p.h>
#include <private/qfont_p.h>

#include <private/qpaintengineex_opengl2_p.h>

#include <qdesktopwidget.h>
#include <qfile.h>
#include <qimagereader.h>
#include <qbuffer.h>

QT_BEGIN_NAMESPACE

Q_OPENGL_EXPORT extern const QGLContext* qt_gl_share_context();

/*!
    \class QGLFramebufferObjectPool
    \since 4.6

    \brief The QGLFramebufferObject class provides a pool of framebuffer
    objects for offscreen rendering purposes.

    When requesting an FBO of a given size and format, an FBO of the same
    format and a size at least as big as the requested size will be returned.

    \internal
*/

static inline int areaDiff(const QSize &size, const QGLFramebufferObject *fbo)
{
    return qAbs(size.width() * size.height() - fbo->width() * fbo->height());
}

extern int qt_next_power_of_two(int v);

static inline QSize maybeRoundToNextPowerOfTwo(const QSize &sz)
{
#ifdef QT_OPENGL_ES_2
    QSize rounded(qt_next_power_of_two(sz.width()), qt_next_power_of_two(sz.height()));
    if (rounded.width() * rounded.height() < 1.20 * sz.width() * sz.height())
        return rounded;
#endif
    return sz;
}


QGLFramebufferObject *QGLFramebufferObjectPool::acquire(const QSize &requestSize, const QGLFramebufferObjectFormat &requestFormat, bool strictSize)
{
    QGLFramebufferObject *chosen = 0;
    QGLFramebufferObject *candidate = 0;
    for (int i = 0; !chosen && i < m_fbos.size(); ++i) {
        QGLFramebufferObject *fbo = m_fbos.at(i);

        if (strictSize) {
            if (fbo->size() == requestSize && fbo->format() == requestFormat) {
                chosen = fbo;
                break;
            } else {
                continue;
            }
        }

        if (fbo->format() == requestFormat) {
            // choose the fbo with a matching format and the closest size
            if (!candidate || areaDiff(requestSize, candidate) > areaDiff(requestSize, fbo))
                candidate = fbo;
        }

        if (candidate) {
            m_fbos.removeOne(candidate);

            const QSize fboSize = candidate->size();
            QSize sz = fboSize;

            if (sz.width() < requestSize.width())
                sz.setWidth(qMax(requestSize.width(), qRound(sz.width() * 1.5)));
            if (sz.height() < requestSize.height())
                sz.setHeight(qMax(requestSize.height(), qRound(sz.height() * 1.5)));

            // wasting too much space?
            if (sz.width() * sz.height() > requestSize.width() * requestSize.height() * 4)
                sz = requestSize;

            if (sz != fboSize) {
                delete candidate;
                candidate = new QGLFramebufferObject(maybeRoundToNextPowerOfTwo(sz), requestFormat);
            }

            chosen = candidate;
        }
    }

    if (!chosen) {
        if (strictSize)
            chosen = new QGLFramebufferObject(requestSize, requestFormat);
        else
            chosen = new QGLFramebufferObject(maybeRoundToNextPowerOfTwo(requestSize), requestFormat);
    }

    if (!chosen->isValid()) {
        delete chosen;
        chosen = 0;
    }

    return chosen;
}

void QGLFramebufferObjectPool::release(QGLFramebufferObject *fbo)
{
    if (fbo)
        m_fbos << fbo;
}


QPaintEngine* QGLPixmapGLPaintDevice::paintEngine() const
{
    return data->paintEngine();
}

void QGLPixmapGLPaintDevice::beginPaint()
{
    if (!data->isValid())
        return;

    // QGLPaintDevice::beginPaint will store the current binding and replace
    // it with m_thisFBO:
    m_thisFBO = data->m_renderFbo->handle();
    QGLPaintDevice::beginPaint();

    Q_ASSERT(data->paintEngine()->type() == QPaintEngine::OpenGL2);

    // QPixmap::fill() is deferred until now, where we actually need to do the fill:
    if (data->needsFill()) {
        const QColor &c = data->fillColor();
        float alpha = c.alphaF();
        glDisable(GL_SCISSOR_TEST);
        glClearColor(c.redF() * alpha, c.greenF() * alpha, c.blueF() * alpha, alpha);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    else if (!data->isUninitialized()) {
        // If the pixmap (GL Texture) has valid content (it has been
        // uploaded from an image or rendered into before), we need to
        // copy it from the texture to the render FBO.

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);

#if !defined(QT_OPENGL_ES_2)
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, data->width(), data->height(), 0, -999999, 999999);
#endif

        glViewport(0, 0, data->width(), data->height());

        // Pass false to bind so it doesn't copy the FBO into the texture!
        context()->drawTexture(QRect(0, 0, data->width(), data->height()), data->bind(false));
    }
}

void QGLPixmapGLPaintDevice::endPaint()
{
    if (!data->isValid())
        return;

    data->copyBackFromRenderFbo(false);

    // Base's endPaint will restore the previous FBO binding
    QGLPaintDevice::endPaint();

    qgl_fbo_pool()->release(data->m_renderFbo);
    data->m_renderFbo = 0;
}

QGLContext* QGLPixmapGLPaintDevice::context() const
{
    data->ensureCreated();
    return data->m_ctx;
}

QSize QGLPixmapGLPaintDevice::size() const
{
    return data->size();
}

bool QGLPixmapGLPaintDevice::alphaRequested() const
{
    return data->m_hasAlpha;
}

void QGLPixmapGLPaintDevice::setPixmapData(QGLPixmapData* d)
{
    data = d;
}

static int qt_gl_pixmap_serial = 0;

QGLPixmapData::QGLPixmapData(PixelType type)
    : QPixmapData(type, OpenGLClass)
    , m_renderFbo(0)
    , m_engine(0)
    , m_ctx(0)
    , m_dirty(false)
    , m_hasFillColor(false)
    , m_hasAlpha(false)
{
    setSerialNumber(++qt_gl_pixmap_serial);
    m_glDevice.setPixmapData(this);
}

QGLPixmapData::~QGLPixmapData()
{
    const QGLContext *shareContext = qt_gl_share_context();
    if (!shareContext)
        return;

    delete m_engine;

    if (m_texture.id) {
        QGLShareContextScope ctx(shareContext);
        glDeleteTextures(1, &m_texture.id);
    }
}

QPixmapData *QGLPixmapData::createCompatiblePixmapData() const
{
    return new QGLPixmapData(pixelType());
}

bool QGLPixmapData::isValid() const
{
    return w > 0 && h > 0;
}

bool QGLPixmapData::isValidContext(const QGLContext *ctx) const
{
    if (ctx == m_ctx)
        return true;

    const QGLContext *share_ctx = qt_gl_share_context();
    return ctx == share_ctx || QGLContext::areSharing(ctx, share_ctx);
}

void QGLPixmapData::resize(int width, int height)
{
    if (width == w && height == h)
        return;

    if (width <= 0 || height <= 0) {
        width = 0;
        height = 0;
    }

    w = width;
    h = height;
    is_null = (w <= 0 || h <= 0);
    d = pixelType() == QPixmapData::PixmapType ? 32 : 1;

    if (m_texture.id) {
        QGLShareContextScope ctx(qt_gl_share_context());
        glDeleteTextures(1, &m_texture.id);
        m_texture.id = 0;
    }

    m_source = QImage();
    m_dirty = isValid();
    setSerialNumber(++qt_gl_pixmap_serial);
}

void QGLPixmapData::ensureCreated() const
{
    if (!m_dirty)
        return;

    m_dirty = false;

    QGLShareContextScope ctx(qt_gl_share_context());
    m_ctx = ctx;

    const GLenum internal_format = m_hasAlpha ? GL_RGBA : GL_RGB;
#ifdef QT_OPENGL_ES_2
    const GLenum external_format = internal_format;
#else
    const GLenum external_format = qt_gl_preferredTextureFormat();
#endif
    const GLenum target = GL_TEXTURE_2D;

    if (!m_texture.id) {
        glGenTextures(1, &m_texture.id);
        glBindTexture(target, m_texture.id);
        glTexImage2D(target, 0, internal_format, w, h, 0, external_format, GL_UNSIGNED_BYTE, 0);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    if (!m_source.isNull()) {
        if (external_format == GL_RGB) {
            const QImage tx = m_source.convertToFormat(QImage::Format_RGB888).mirrored(false, true);

            glBindTexture(target, m_texture.id);
            glTexSubImage2D(target, 0, 0, 0, w, h, external_format,
                            GL_UNSIGNED_BYTE, tx.bits());
        } else {
            const QImage tx = ctx->d_func()->convertToGLFormat(m_source, true, external_format);

            glBindTexture(target, m_texture.id);
            glTexSubImage2D(target, 0, 0, 0, w, h, external_format,
                            GL_UNSIGNED_BYTE, tx.bits());
        }

        if (useFramebufferObjects())
            m_source = QImage();
    }

    m_texture.options &= ~QGLContext::MemoryManagedBindOption;
}

void QGLPixmapData::fromImage(const QImage &image,
                              Qt::ImageConversionFlags flags)
{
    QImage img = image;
    createPixmapForImage(img, flags, false);
}

void QGLPixmapData::fromImageReader(QImageReader *imageReader,
                                 Qt::ImageConversionFlags flags)
{
    QImage image = imageReader->read();
    if (image.isNull())
        return;

    createPixmapForImage(image, flags, true);
}

bool QGLPixmapData::fromFile(const QString &filename, const char *format,
                             Qt::ImageConversionFlags flags)
{
    if (pixelType() == QPixmapData::BitmapType)
        return QPixmapData::fromFile(filename, format, flags);
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.peek(64);
        bool alpha;
        if (m_texture.canBindCompressedTexture
                (data.constData(), data.size(), format, &alpha)) {
            resize(0, 0);
            data = file.readAll();
            file.close();
            QGLShareContextScope ctx(qt_gl_share_context());
            QSize size = m_texture.bindCompressedTexture
                (data.constData(), data.size(), format);
            if (!size.isEmpty()) {
                w = size.width();
                h = size.height();
                is_null = false;
                d = 32;
                m_hasAlpha = alpha;
                m_source = QImage();
                m_dirty = isValid();
                return true;
            }
            return false;
        }
    }

    QImage image = QImageReader(filename, format).read();
    if (image.isNull())
        return false;

    createPixmapForImage(image, flags, true);

    return !isNull();
}

bool QGLPixmapData::fromData(const uchar *buffer, uint len, const char *format,
                             Qt::ImageConversionFlags flags)
{
    bool alpha;
    const char *buf = reinterpret_cast<const char *>(buffer);
    if (m_texture.canBindCompressedTexture(buf, int(len), format, &alpha)) {
        resize(0, 0);
        QGLShareContextScope ctx(qt_gl_share_context());
        QSize size = m_texture.bindCompressedTexture(buf, int(len), format);
        if (!size.isEmpty()) {
            w = size.width();
            h = size.height();
            is_null = false;
            d = 32;
            m_hasAlpha = alpha;
            m_source = QImage();
            m_dirty = isValid();
            return true;
        }
    }

    QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(buffer), len);
    QBuffer b(&a);
    b.open(QIODevice::ReadOnly);
    QImage image = QImageReader(&b, format).read();
    if (image.isNull())
        return false;

    createPixmapForImage(image, flags, true);

    return !isNull();
}

/*!
    out-of-place conversion (inPlace == false) will always detach()
 */
void QGLPixmapData::createPixmapForImage(QImage &image, Qt::ImageConversionFlags flags, bool inPlace)
{
    if (image.size() == QSize(w, h))
        setSerialNumber(++qt_gl_pixmap_serial);

    resize(image.width(), image.height());

    if (pixelType() == BitmapType) {
        m_source = image.convertToFormat(QImage::Format_MonoLSB);

    } else {
        QImage::Format format = QImage::Format_RGB32;
        if (qApp->desktop()->depth() == 16)
            format = QImage::Format_RGB16;

        if (image.hasAlphaChannel()
            && ((flags & Qt::NoOpaqueDetection)
                || const_cast<QImage &>(image).data_ptr()->checkForAlphaPixels()))
            format = QImage::Format_ARGB32_Premultiplied;;

        if (inPlace && image.data_ptr()->convertInPlace(format, flags)) {
            m_source = image;
        } else {
            m_source = image.convertToFormat(format);

            // convertToFormat won't detach the image if format stays the same.
            if (image.format() == format)
                m_source.detach();
        }
    }

    m_dirty = true;
    m_hasFillColor = false;

    m_hasAlpha = m_source.hasAlphaChannel();
    w = image.width();
    h = image.height();
    is_null = (w <= 0 || h <= 0);
    d = m_source.depth();

    if (m_texture.id) {
        QGLShareContextScope ctx(qt_gl_share_context());
        glDeleteTextures(1, &m_texture.id);
        m_texture.id = 0;
    }
}

bool QGLPixmapData::scroll(int dx, int dy, const QRect &rect)
{
    Q_UNUSED(dx);
    Q_UNUSED(dy);
    Q_UNUSED(rect);
    return false;
}

void QGLPixmapData::copy(const QPixmapData *data, const QRect &rect)
{
    if (data->classId() != QPixmapData::OpenGLClass || !static_cast<const QGLPixmapData *>(data)->useFramebufferObjects()) {
        QPixmapData::copy(data, rect);
        return;
    }

    const QGLPixmapData *other = static_cast<const QGLPixmapData *>(data);
    if (other->m_renderFbo) {
        QGLShareContextScope ctx(qt_gl_share_context());

        resize(rect.width(), rect.height());
        m_hasAlpha = other->m_hasAlpha;
        ensureCreated();

        if (!ctx->d_ptr->fbo)
            glGenFramebuffers(1, &ctx->d_ptr->fbo);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, ctx->d_ptr->fbo);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                GL_TEXTURE_2D, m_texture.id, 0);

        if (!other->m_renderFbo->isBound())
            glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, other->m_renderFbo->handle());

        glDisable(GL_SCISSOR_TEST);
        if (ctx->d_ptr->active_engine && ctx->d_ptr->active_engine->type() == QPaintEngine::OpenGL2)
            static_cast<QGL2PaintEngineEx *>(ctx->d_ptr->active_engine)->invalidateState();

        glBlitFramebufferEXT(rect.x(), rect.y(), rect.x() + rect.width(), rect.y() + rect.height(),
                0, 0, w, h,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER_EXT, ctx->d_ptr->current_fbo);
    } else {
        QPixmapData::copy(data, rect);
    }
}

void QGLPixmapData::fill(const QColor &color)
{
    if (!isValid())
        return;

    bool hasAlpha = color.alpha() != 255;
    if (hasAlpha && !m_hasAlpha) {
        if (m_texture.id) {
            glDeleteTextures(1, &m_texture.id);
            m_texture.id = 0;
            m_dirty = true;
        }
        m_hasAlpha = color.alpha() != 255;
    }

    if (useFramebufferObjects()) {
        m_source = QImage();
        m_hasFillColor = true;
        m_fillColor = color;
    } else {

        if (m_source.isNull()) {
            m_fillColor = color;
            m_hasFillColor = true;

        } else if (m_source.depth() == 32) {
            m_source.fill(PREMUL(color.rgba()));

        } else if (m_source.depth() == 1) {
            if (color == Qt::color1)
                m_source.fill(1);
            else
                m_source.fill(0);
        }
    }
}

bool QGLPixmapData::hasAlphaChannel() const
{
    return m_hasAlpha;
}

QImage QGLPixmapData::fillImage(const QColor &color) const
{
    QImage img;
    if (pixelType() == BitmapType) {
        img = QImage(w, h, QImage::Format_MonoLSB);

        img.setColorCount(2);
        img.setColor(0, QColor(Qt::color0).rgba());
        img.setColor(1, QColor(Qt::color1).rgba());

        if (color == Qt::color1)
            img.fill(1);
        else
            img.fill(0);
    } else {
        img = QImage(w, h,
                m_hasAlpha
                ? QImage::Format_ARGB32_Premultiplied
                : QImage::Format_RGB32);
        img.fill(PREMUL(color.rgba()));
    }
    return img;
}

extern QImage qt_gl_read_texture(const QSize &size, bool alpha_format, bool include_alpha);

QImage QGLPixmapData::toImage() const
{
    if (!isValid())
        return QImage();

    if (m_renderFbo) {
        copyBackFromRenderFbo(true);
    } else if (!m_source.isNull()) {
        QImageData *data = const_cast<QImage &>(m_source).data_ptr();
        if (data->paintEngine && data->paintEngine->isActive()
            && data->paintEngine->paintDevice() == &m_source)
        {
            return m_source.copy();
        }
        return m_source;
    } else if (m_dirty || m_hasFillColor) {
        return fillImage(m_fillColor);
    } else {
        ensureCreated();
    }

    QGLShareContextScope ctx(qt_gl_share_context());
    glBindTexture(GL_TEXTURE_2D, m_texture.id);
    return qt_gl_read_texture(QSize(w, h), true, true);
}

struct TextureBuffer
{
    QGLFramebufferObject *fbo;
    QGL2PaintEngineEx *engine;
};

Q_GLOBAL_STATIC(QGLFramebufferObjectPool, _qgl_fbo_pool)
QGLFramebufferObjectPool* qgl_fbo_pool()
{
    return _qgl_fbo_pool();
}

void QGLPixmapData::copyBackFromRenderFbo(bool keepCurrentFboBound) const
{
    if (!isValid())
        return;

    m_hasFillColor = false;

    const QGLContext *share_ctx = qt_gl_share_context();
    QGLShareContextScope ctx(share_ctx);

    ensureCreated();

    if (!ctx->d_ptr->fbo)
        glGenFramebuffers(1, &ctx->d_ptr->fbo);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, ctx->d_ptr->fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
        GL_TEXTURE_2D, m_texture.id, 0);

    const int x0 = 0;
    const int x1 = w;
    const int y0 = 0;
    const int y1 = h;

    if (!m_renderFbo->isBound())
        glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, m_renderFbo->handle());

    glDisable(GL_SCISSOR_TEST);

    glBlitFramebufferEXT(x0, y0, x1, y1,
            x0, y0, x1, y1,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST);

    if (keepCurrentFboBound) {
        glBindFramebuffer(GL_FRAMEBUFFER_EXT, ctx->d_ptr->current_fbo);
    } else {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, m_renderFbo->handle());
        ctx->d_ptr->current_fbo = m_renderFbo->handle();
    }
}

bool QGLPixmapData::useFramebufferObjects() const
{
    return QGLFramebufferObject::hasOpenGLFramebufferObjects()
           && QGLFramebufferObject::hasOpenGLFramebufferBlit()
           && qt_gl_preferGL2Engine()
           && (w * h > 32*32); // avoid overhead of FBOs for small pixmaps
}

QPaintEngine* QGLPixmapData::paintEngine() const
{
    if (!isValid())
        return 0;

    if (m_renderFbo)
        return m_engine;

    if (useFramebufferObjects()) {
        extern QGLWidget* qt_gl_share_widget();

        if (!QGLContext::currentContext())
            const_cast<QGLContext *>(qt_gl_share_context())->makeCurrent();
        QGLShareContextScope ctx(qt_gl_share_context());

        QGLFramebufferObjectFormat format;
        format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        format.setInternalTextureFormat(GLenum(m_hasAlpha ? GL_RGBA : GL_RGB));

        m_renderFbo = qgl_fbo_pool()->acquire(size(), format);

        if (m_renderFbo) {
            if (!m_engine)
                m_engine = new QGL2PaintEngineEx;
            return m_engine;
        }

        qWarning() << "Failed to create pixmap texture buffer of size " << size() << ", falling back to raster paint engine";
    }

    m_dirty = true;
    if (m_source.size() != size())
        m_source = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    if (m_hasFillColor) {
        m_source.fill(PREMUL(m_fillColor.rgba()));
        m_hasFillColor = false;
    }
    return m_source.paintEngine();
}

extern QRgb qt_gl_convertToGLFormat(QRgb src_pixel, GLenum texture_format);

// If copyBack is true, bind will copy the contents of the render
// FBO to the texture (which is not bound to the texture, as it's
// a multisample FBO).
GLuint QGLPixmapData::bind(bool copyBack) const
{
    if (m_renderFbo && copyBack) {
        copyBackFromRenderFbo(true);
    } else {
        ensureCreated();
    }

    GLuint id = m_texture.id;
    glBindTexture(GL_TEXTURE_2D, id);

    if (m_hasFillColor) {
        if (!useFramebufferObjects()) {
            m_source = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
            m_source.fill(PREMUL(m_fillColor.rgba()));
        }

        m_hasFillColor = false;

        GLenum format = qt_gl_preferredTextureFormat();
        QImage tx(w, h, QImage::Format_ARGB32_Premultiplied);
        tx.fill(qt_gl_convertToGLFormat(m_fillColor.rgba(), format));
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, GL_UNSIGNED_BYTE, tx.bits());
    }

    return id;
}

QGLTexture* QGLPixmapData::texture() const
{
    return &m_texture;
}

int QGLPixmapData::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    if (w == 0)
        return 0;

    switch (metric) {
    case QPaintDevice::PdmWidth:
        return w;
    case QPaintDevice::PdmHeight:
        return h;
    case QPaintDevice::PdmNumColors:
        return 0;
    case QPaintDevice::PdmDepth:
        return d;
    case QPaintDevice::PdmWidthMM:
        return qRound(w * 25.4 / qt_defaultDpiX());
    case QPaintDevice::PdmHeightMM:
        return qRound(h * 25.4 / qt_defaultDpiY());
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmPhysicalDpiX:
        return qt_defaultDpiX();
    case QPaintDevice::PdmDpiY:
    case QPaintDevice::PdmPhysicalDpiY:
        return qt_defaultDpiY();
    default:
        qWarning("QGLPixmapData::metric(): Invalid metric");
        return 0;
    }
}

QGLPaintDevice *QGLPixmapData::glDevice() const
{
    return &m_glDevice;
}

QT_END_NAMESPACE
