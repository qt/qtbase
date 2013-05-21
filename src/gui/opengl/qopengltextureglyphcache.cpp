/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopengltextureglyphcache_p.h"
#include "qopenglpaintengine_p.h"
#include "private/qopenglengineshadersource_p.h"
#include "qopenglextensions_p.h"

QT_BEGIN_NAMESPACE


QBasicAtomicInt qopengltextureglyphcache_serial_number = Q_BASIC_ATOMIC_INITIALIZER(1);

QOpenGLTextureGlyphCache::QOpenGLTextureGlyphCache(QFontEngineGlyphCache::Type type, const QTransform &matrix)
    : QImageTextureGlyphCache(type, matrix)
    , m_textureResource(0)
    , pex(0)
    , m_blitProgram(0)
    , m_filterMode(Nearest)
    , m_serialNumber(qopengltextureglyphcache_serial_number.fetchAndAddRelaxed(1))
{
#ifdef QT_GL_TEXTURE_GLYPH_CACHE_DEBUG
    qDebug(" -> QOpenGLTextureGlyphCache() %p for context %p.", this, QOpenGLContext::currentContext());
#endif
    m_vertexCoordinateArray[0] = -1.0f;
    m_vertexCoordinateArray[1] = -1.0f;
    m_vertexCoordinateArray[2] =  1.0f;
    m_vertexCoordinateArray[3] = -1.0f;
    m_vertexCoordinateArray[4] =  1.0f;
    m_vertexCoordinateArray[5] =  1.0f;
    m_vertexCoordinateArray[6] = -1.0f;
    m_vertexCoordinateArray[7] =  1.0f;

    m_textureCoordinateArray[0] = 0.0f;
    m_textureCoordinateArray[1] = 0.0f;
    m_textureCoordinateArray[2] = 1.0f;
    m_textureCoordinateArray[3] = 0.0f;
    m_textureCoordinateArray[4] = 1.0f;
    m_textureCoordinateArray[5] = 1.0f;
    m_textureCoordinateArray[6] = 0.0f;
    m_textureCoordinateArray[7] = 1.0f;
}

QOpenGLTextureGlyphCache::~QOpenGLTextureGlyphCache()
{
#ifdef QT_GL_TEXTURE_GLYPH_CACHE_DEBUG
    qDebug(" -> ~QOpenGLTextureGlyphCache() %p.", this);
#endif
}

void QOpenGLTextureGlyphCache::createTextureData(int width, int height)
{
    QOpenGLContext *ctx = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
    if (ctx == 0) {
        qWarning("QOpenGLTextureGlyphCache::createTextureData: Called with no context");
        return;
    }

    // create in QImageTextureGlyphCache baseclass is meant to be called
    // only to create the initial image and does not preserve the content,
    // so we don't call when this function is called from resize.
    if (ctx->d_func()->workaround_brokenFBOReadBack && image().isNull())
        QImageTextureGlyphCache::createTextureData(width, height);

    // Make the lower glyph texture size 16 x 16.
    if (width < 16)
        width = 16;
    if (height < 16)
        height = 16;

    if (m_textureResource && !m_textureResource->m_texture) {
        delete m_textureResource;
        m_textureResource = 0;
    }

    if (!m_textureResource)
        m_textureResource = new QOpenGLGlyphTexture(ctx);

    glGenTextures(1, &m_textureResource->m_texture);
    glBindTexture(GL_TEXTURE_2D, m_textureResource->m_texture);

    m_textureResource->m_width = width;
    m_textureResource->m_height = height;

    if (m_type == QFontEngineGlyphCache::Raster_RGBMask || m_type == QFontEngineGlyphCache::Raster_ARGB) {
        QVarLengthArray<uchar> data(width * height * 4);
        for (int i = 0; i < data.size(); ++i)
            data[i] = 0;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
    } else {
        QVarLengthArray<uchar> data(width * height);
        for (int i = 0; i < data.size(); ++i)
            data[i] = 0;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, &data[0]);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_filterMode = Nearest;
}

void QOpenGLTextureGlyphCache::resizeTextureData(int width, int height)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (ctx == 0) {
        qWarning("QOpenGLTextureGlyphCache::resizeTextureData: Called with no context");
        return;
    }

    GLint oldFbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFbo);

    int oldWidth = m_textureResource->m_width;
    int oldHeight = m_textureResource->m_height;

    // Make the lower glyph texture size 16 x 16.
    if (width < 16)
        width = 16;
    if (height < 16)
        height = 16;

    GLuint oldTexture = m_textureResource->m_texture;
    createTextureData(width, height);

    if (ctx->d_func()->workaround_brokenFBOReadBack) {
        QImageTextureGlyphCache::resizeTextureData(width, height);
        Q_ASSERT(image().depth() == 8);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, oldHeight, GL_ALPHA, GL_UNSIGNED_BYTE, image().constBits());
        glDeleteTextures(1, &oldTexture);
        return;
    }

    // ### the QTextureGlyphCache API needs to be reworked to allow
    // ### resizeTextureData to fail

    QOpenGLFunctions funcs(ctx);

    funcs.glBindFramebuffer(GL_FRAMEBUFFER, m_textureResource->m_fbo);

    GLuint tmp_texture;
    glGenTextures(1, &tmp_texture);
    glBindTexture(GL_TEXTURE_2D, tmp_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, oldWidth, oldHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_filterMode = Nearest;
    glBindTexture(GL_TEXTURE_2D, 0);
    funcs.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, tmp_texture, 0);

    funcs.glActiveTexture(GL_TEXTURE0 + QT_IMAGE_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, oldTexture);

    if (pex != 0)
        pex->transferMode(BrushDrawingMode);

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, oldWidth, oldHeight);

    QOpenGLShaderProgram *blitProgram = 0;
    if (pex == 0) {
        if (m_blitProgram == 0) {
            m_blitProgram = new QOpenGLShaderProgram(ctx);

            {
                QString source;
                source.append(QLatin1String(qopenglslMainWithTexCoordsVertexShader));
                source.append(QLatin1String(qopenglslUntransformedPositionVertexShader));

                QOpenGLShader *vertexShader = new QOpenGLShader(QOpenGLShader::Vertex, m_blitProgram);
                vertexShader->compileSourceCode(source);

                m_blitProgram->addShader(vertexShader);
            }

            {
                QString source;
                source.append(QLatin1String(qopenglslMainFragmentShader));
                source.append(QLatin1String(qopenglslImageSrcFragmentShader));

                QOpenGLShader *fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, m_blitProgram);
                fragmentShader->compileSourceCode(source);

                m_blitProgram->addShader(fragmentShader);
            }

            m_blitProgram->bindAttributeLocation("vertexCoordsArray", QT_VERTEX_COORDS_ATTR);
            m_blitProgram->bindAttributeLocation("textureCoordArray", QT_TEXTURE_COORDS_ATTR);

            m_blitProgram->link();
        }

        funcs.glVertexAttribPointer(QT_VERTEX_COORDS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, m_vertexCoordinateArray);
        funcs.glVertexAttribPointer(QT_TEXTURE_COORDS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, m_textureCoordinateArray);

        m_blitProgram->bind();
        m_blitProgram->enableAttributeArray(int(QT_VERTEX_COORDS_ATTR));
        m_blitProgram->enableAttributeArray(int(QT_TEXTURE_COORDS_ATTR));
        m_blitProgram->disableAttributeArray(int(QT_OPACITY_ATTR));

        blitProgram = m_blitProgram;

    } else {
        pex->setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, m_vertexCoordinateArray);
        pex->setVertexAttributePointer(QT_TEXTURE_COORDS_ATTR, m_textureCoordinateArray);

        pex->shaderManager->useBlitProgram();
        blitProgram = pex->shaderManager->blitProgram();
    }

    blitProgram->setUniformValue("imageTexture", QT_IMAGE_TEXTURE_UNIT);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindTexture(GL_TEXTURE_2D, m_textureResource->m_texture);

    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, oldWidth, oldHeight);

    funcs.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_RENDERBUFFER, 0);
    glDeleteTextures(1, &tmp_texture);
    glDeleteTextures(1, &oldTexture);

    funcs.glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)oldFbo);

    if (pex != 0) {
        glViewport(0, 0, pex->width, pex->height);
        pex->updateClipScissorTest();
    } else {
        m_blitProgram->disableAttributeArray(int(QT_VERTEX_COORDS_ATTR));
        m_blitProgram->disableAttributeArray(int(QT_TEXTURE_COORDS_ATTR));
    }
}

void QOpenGLTextureGlyphCache::fillTexture(const Coord &c, glyph_t glyph, QFixed subPixelPosition)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (ctx == 0) {
        qWarning("QOpenGLTextureGlyphCache::fillTexture: Called with no context");
        return;
    }

    if (ctx->d_func()->workaround_brokenFBOReadBack) {
        QImageTextureGlyphCache::fillTexture(c, glyph, subPixelPosition);

        glBindTexture(GL_TEXTURE_2D, m_textureResource->m_texture);
        const QImage &texture = image();
        const uchar *bits = texture.constBits();
        bits += c.y * texture.bytesPerLine() + c.x;
        for (int i=0; i<c.h; ++i) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y + i, c.w, 1, GL_ALPHA, GL_UNSIGNED_BYTE, bits);
            bits += texture.bytesPerLine();
        }
        return;
    }

    QImage mask = textureMapForGlyph(glyph, subPixelPosition);
    const int maskWidth = mask.width();
    const int maskHeight = mask.height();

#if defined(QT_OPENGL_ES_2)
    QOpenGLExtensions extensions(ctx);
    bool hasBGRA = extensions.hasOpenGLExtension(QOpenGLExtensions::BGRATextureFormat);
#endif

    if (mask.format() == QImage::Format_Mono) {
        mask = mask.convertToFormat(QImage::Format_Indexed8);
        for (int y = 0; y < maskHeight; ++y) {
            uchar *src = (uchar *) mask.scanLine(y);
            for (int x = 0; x < maskWidth; ++x)
                src[x] = -src[x]; // convert 0 and 1 into 0 and 255
        }
    } else if (mask.depth() == 32) {
        if (mask.format() == QImage::Format_RGB32
            // We need to make the alpha component equal to the average of the RGB values.
            // This is needed when drawing sub-pixel antialiased text on translucent targets.
#if defined(QT_OPENGL_ES_2)
            || !hasBGRA // We need to reverse the bytes
#endif
            ) {
            for (int y = 0; y < maskHeight; ++y) {
                quint32 *src = (quint32 *) mask.scanLine(y);
                for (int x = 0; x < maskWidth; ++x) {
                    uchar r = src[x] >> 16;
                    uchar g = src[x] >> 8;
                    uchar b = src[x];
                    quint32 avg;
                    if (mask.format() == QImage::Format_RGB32)
                        avg = (quint32(r) + quint32(g) + quint32(b) + 1) / 3; // "+1" for rounding.
                    else // Format_ARGB_Premultiplied
                        avg = src[x] >> 24;

#if defined(QT_OPENGL_ES_2)
                    if (!hasBGRA) {
                        // Reverse bytes to match GL_RGBA
                        src[x] = (avg << 24) | (quint32(r) << 0) | (quint32(g) << 8) | (quint32(b) << 16);
                    } else
#endif
                        src[x] = (src[x] & 0x00ffffff) | (avg << 24);
                }
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, m_textureResource->m_texture);
    if (mask.depth() == 32) {
#if defined(QT_OPENGL_ES_2)
        glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y, maskWidth, maskHeight, hasBGRA ? GL_BGRA_EXT : GL_RGBA, GL_UNSIGNED_BYTE, mask.bits());
#else
        glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y, maskWidth, maskHeight, GL_BGRA, GL_UNSIGNED_BYTE, mask.bits());
#endif
    } else {
        // glTexSubImage2D() might cause some garbage to appear in the texture if the mask width is
        // not a multiple of four bytes. The bug appeared on a computer with 32-bit Windows Vista
        // and nVidia GeForce 8500GT. GL_UNPACK_ALIGNMENT is set to four bytes, 'mask' has a
        // multiple of four bytes per line, and most of the glyph shows up correctly in the
        // texture, which makes me think that this is a driver bug.
        // One workaround is to make sure the mask width is a multiple of four bytes, for instance
        // by converting it to a format with four bytes per pixel. Another is to copy one line at a
        // time.

#if 0
        if (!ctx->d_func()->workaround_brokenAlphaTexSubImage_init) {
            // don't know which driver versions exhibit this bug, so be conservative for now
            const QByteArray versionString(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
            glctx->d_func()->workaround_brokenAlphaTexSubImage = versionString.indexOf("NVIDIA") >= 0;
            glctx->d_func()->workaround_brokenAlphaTexSubImage_init = true;
        }
#endif

#if 0
        if (ctx->d_func()->workaround_brokenAlphaTexSubImage) {
            for (int i = 0; i < maskHeight; ++i)
                glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y + i, maskWidth, 1, GL_ALPHA, GL_UNSIGNED_BYTE, mask.scanLine(i));
        } else {
#endif
            glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y, maskWidth, maskHeight, GL_ALPHA, GL_UNSIGNED_BYTE, mask.bits());
//        }
    }
}

int QOpenGLTextureGlyphCache::glyphPadding() const
{
    return 1;
}

int QOpenGLTextureGlyphCache::maxTextureWidth() const
{
    QOpenGLContext *ctx = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
    if (ctx == 0)
        return QImageTextureGlyphCache::maxTextureWidth();
    else
        return ctx->d_func()->maxTextureSize();
}

int QOpenGLTextureGlyphCache::maxTextureHeight() const
{
    QOpenGLContext *ctx = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
    if (ctx == 0)
        return QImageTextureGlyphCache::maxTextureHeight();

    if (ctx->d_func()->workaround_brokenTexSubImage)
        return qMin(1024, ctx->d_func()->maxTextureSize());
    else
        return ctx->d_func()->maxTextureSize();
}

void QOpenGLTextureGlyphCache::clear()
{
    m_textureResource->free();
    m_textureResource = 0;

    m_w = 0;
    m_h = 0;
    m_cx = 0;
    m_cy = 0;
    m_currentRowHeight = 0;
    coords.clear();
}

QT_END_NAMESPACE
