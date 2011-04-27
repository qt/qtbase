/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qglpixelbuffer.h>
#include <qgl.h>
#include <private/qgl_p.h>

#include <private/qglpixelbuffer_p.h>

#include <qimage.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/* WGL_WGLEXT_PROTOTYPES */
typedef const char * (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);
typedef HPBUFFERARB (WINAPI * PFNWGLCREATEPBUFFERARBPROC) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
typedef HDC (WINAPI * PFNWGLGETPBUFFERDCARBPROC) (HPBUFFERARB hPbuffer);
typedef int (WINAPI * PFNWGLRELEASEPBUFFERDCARBPROC) (HPBUFFERARB hPbuffer, HDC hDC);
typedef BOOL (WINAPI * PFNWGLDESTROYPBUFFERARBPROC) (HPBUFFERARB hPbuffer);
typedef BOOL (WINAPI * PFNWGLQUERYPBUFFERARBPROC) (HPBUFFERARB hPbuffer, int iAttribute, int *piValue);
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBIVARBPROC) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBFVARBPROC) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef BOOL (WINAPI * PFNWGLBINDTEXIMAGEARBPROC) (HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL (WINAPI * PFNWGLRELEASETEXIMAGEARBPROC) (HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL (WINAPI * PFNWGLSETPBUFFERATTRIBARBPROC) (HPBUFFERARB hPbuffer, const int * piAttribList);

#ifndef WGL_ARB_pbuffer
#define WGL_DRAW_TO_PBUFFER_ARB        0x202D
#define WGL_MAX_PBUFFER_PIXELS_ARB     0x202E
#define WGL_MAX_PBUFFER_WIDTH_ARB      0x202F
#define WGL_MAX_PBUFFER_HEIGHT_ARB     0x2030
#define WGL_PBUFFER_LARGEST_ARB        0x2033
#define WGL_PBUFFER_WIDTH_ARB          0x2034
#define WGL_PBUFFER_HEIGHT_ARB         0x2035
#define WGL_PBUFFER_LOST_ARB           0x2036
#endif

#ifndef WGL_ARB_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_ARB   0x2000
#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_DRAW_TO_BITMAP_ARB         0x2002
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_NEED_PALETTE_ARB           0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB    0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB     0x2006
#define WGL_SWAP_METHOD_ARB            0x2007
#define WGL_NUMBER_OVERLAYS_ARB        0x2008
#define WGL_NUMBER_UNDERLAYS_ARB       0x2009
#define WGL_TRANSPARENT_ARB            0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB  0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB            0x200C
#define WGL_SHARE_STENCIL_ARB          0x200D
#define WGL_SHARE_ACCUM_ARB            0x200E
#define WGL_SUPPORT_GDI_ARB            0x200F
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_STEREO_ARB                 0x2012
#define WGL_PIXEL_TYPE_ARB             0x2013
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_RED_BITS_ARB               0x2015
#define WGL_RED_SHIFT_ARB              0x2016
#define WGL_GREEN_BITS_ARB             0x2017
#define WGL_GREEN_SHIFT_ARB            0x2018
#define WGL_BLUE_BITS_ARB              0x2019
#define WGL_BLUE_SHIFT_ARB             0x201A
#define WGL_ALPHA_BITS_ARB             0x201B
#define WGL_ALPHA_SHIFT_ARB            0x201C
#define WGL_ACCUM_BITS_ARB             0x201D
#define WGL_ACCUM_RED_BITS_ARB         0x201E
#define WGL_ACCUM_GREEN_BITS_ARB       0x201F
#define WGL_ACCUM_BLUE_BITS_ARB        0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB       0x2021
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_AUX_BUFFERS_ARB            0x2024
#define WGL_NO_ACCELERATION_ARB        0x2025
#define WGL_GENERIC_ACCELERATION_ARB   0x2026
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_SWAP_EXCHANGE_ARB          0x2028
#define WGL_SWAP_COPY_ARB              0x2029
#define WGL_SWAP_UNDEFINED_ARB         0x202A
#define WGL_TYPE_RGBA_ARB              0x202B
#define WGL_TYPE_COLORINDEX_ARB        0x202C
#endif

#ifndef WGL_ARB_render_texture
#define WGL_BIND_TO_TEXTURE_RGB_ARB        0x2070
#define WGL_BIND_TO_TEXTURE_RGBA_ARB       0x2071
#define WGL_TEXTURE_FORMAT_ARB             0x2072
#define WGL_TEXTURE_TARGET_ARB             0x2073
#define WGL_MIPMAP_TEXTURE_ARB             0x2074
#define WGL_TEXTURE_RGB_ARB                0x2075
#define WGL_TEXTURE_RGBA_ARB               0x2076
#define WGL_NO_TEXTURE_ARB                 0x2077
#define WGL_TEXTURE_CUBE_MAP_ARB           0x2078
#define WGL_TEXTURE_1D_ARB                 0x2079
#define WGL_TEXTURE_2D_ARB                 0x207A
#define WGL_MIPMAP_LEVEL_ARB               0x207B
#define WGL_CUBE_MAP_FACE_ARB              0x207C
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x207D
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x207E
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x207F
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x2080
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x2081
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x2082
#define WGL_FRONT_LEFT_ARB                 0x2083
#define WGL_FRONT_RIGHT_ARB                0x2084
#define WGL_BACK_LEFT_ARB                  0x2085
#define WGL_BACK_RIGHT_ARB                 0x2086
#define WGL_AUX0_ARB                       0x2087
#define WGL_AUX1_ARB                       0x2088
#define WGL_AUX2_ARB                       0x2089
#define WGL_AUX3_ARB                       0x208A
#define WGL_AUX4_ARB                       0x208B
#define WGL_AUX5_ARB                       0x208C
#define WGL_AUX6_ARB                       0x208D
#define WGL_AUX7_ARB                       0x208E
#define WGL_AUX8_ARB                       0x208F
#define WGL_AUX9_ARB                       0x2090
#endif

#ifndef WGL_FLOAT_COMPONENTS_NV
#define WGL_FLOAT_COMPONENTS_NV        0x20B0
#endif

#ifndef WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB         0x2041
#define WGL_SAMPLES_ARB                0x2042
#endif

#ifndef GL_SAMPLES_ARB
#define GL_SAMPLES_ARB 0x80A9
#endif

QGLFormat pfiToQGLFormat(HDC hdc, int pfi);

static void qt_format_to_attrib_list(bool has_render_texture, const QGLFormat &f, int attribs[])
{
    int i = 0;
    attribs[i++] = WGL_SUPPORT_OPENGL_ARB;
    attribs[i++] = TRUE;
    attribs[i++] = WGL_DRAW_TO_PBUFFER_ARB;
    attribs[i++] = TRUE;

    if (has_render_texture) {
        attribs[i++] = WGL_BIND_TO_TEXTURE_RGBA_ARB;
        attribs[i++] = TRUE;
    }

    attribs[i++] = WGL_COLOR_BITS_ARB;
    attribs[i++] = 32;
    attribs[i++] = WGL_DOUBLE_BUFFER_ARB;
    attribs[i++] = FALSE;

    if (f.stereo()) {
        attribs[i++] = WGL_STEREO_ARB;
        attribs[i++] = TRUE;
    }
    if (f.depth()) {
        attribs[i++] = WGL_DEPTH_BITS_ARB;
        attribs[i++] = f.depthBufferSize() == -1 ? 24 : f.depthBufferSize();
    }
    if (f.redBufferSize() != -1) {
        attribs[i++] = WGL_RED_BITS_ARB;
        attribs[i++] = f.redBufferSize();
    }
    if (f.greenBufferSize() != -1) {
        attribs[i++] = WGL_GREEN_BITS_ARB;
        attribs[i++] = f.greenBufferSize();
    }
    if (f.blueBufferSize() != -1) {
        attribs[i++] = WGL_BLUE_BITS_ARB;
        attribs[i++] = f.blueBufferSize();
    }
    if (f.alpha()) {
        attribs[i++] = WGL_ALPHA_BITS_ARB;
        attribs[i++] = f.alphaBufferSize() == -1 ? 8 : f.alphaBufferSize();
    }
    if (f.accum()) {
        attribs[i++] = WGL_ACCUM_BITS_ARB;
        attribs[i++] = f.accumBufferSize() == -1 ? 16 : f.accumBufferSize();
    }
    if (f.stencil()) {
        attribs[i++] = WGL_STENCIL_BITS_ARB;
        attribs[i++] = f.stencilBufferSize() == -1 ? 8 : f.stencilBufferSize();
    }
    if ((f.redBufferSize() > 8 || f.greenBufferSize() > 8
         || f.blueBufferSize() > 8 || f.alphaBufferSize() > 8)
        && (QGLExtensions::glExtensions() & QGLExtensions::NVFloatBuffer))
    {
        attribs[i++] = WGL_FLOAT_COMPONENTS_NV;
        attribs[i++] = TRUE;
    }
    if (f.sampleBuffers()) {
        attribs[i++] = WGL_SAMPLE_BUFFERS_ARB;
        attribs[i++] = 1;
        attribs[i++] = WGL_SAMPLES_ARB;
        attribs[i++] = f.samples() == -1 ? 16 : f.samples();
    }
    attribs[i] = 0;
}

bool QGLPixelBufferPrivate::init(const QSize &size, const QGLFormat &f, QGLWidget *shareWidget)
{
    QGLTemporaryContext tempContext;

    PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB =
        (PFNWGLCREATEPBUFFERARBPROC) wglGetProcAddress("wglCreatePbufferARB");
    PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB =
        (PFNWGLGETPBUFFERDCARBPROC) wglGetProcAddress("wglGetPbufferDCARB");
    PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB =
        (PFNWGLQUERYPBUFFERARBPROC) wglGetProcAddress("wglQueryPbufferARB");
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress("wglChoosePixelFormatARB");

    if (!wglCreatePbufferARB) // assumes that if one can be resolved, all of them can
        return false;

    dc = wglGetCurrentDC();
    Q_ASSERT(dc);
    has_render_texture = false;

    // sample buffers doesn't work in conjunction with the render_texture extension
    if (!f.sampleBuffers()) {
        PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
                (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");

        if (wglGetExtensionsStringARB) {
            QString extensions(QLatin1String(wglGetExtensionsStringARB(dc)));
            has_render_texture = extensions.contains(QLatin1String("WGL_ARB_render_texture"));
        }
    }

    int attribs[40];
    qt_format_to_attrib_list(has_render_texture, f, attribs);

    // Find pbuffer capable pixel format.
    unsigned int num_formats = 0;
    int pixel_format;
    wglChoosePixelFormatARB(dc, attribs, 0, 1, &pixel_format, &num_formats);

    // some GL implementations don't support pbuffers with accum
    // buffers, so try that before we give up
    if (num_formats == 0 && f.accum()) {
        QGLFormat tmp = f;
        tmp.setAccum(false);
        qt_format_to_attrib_list(has_render_texture, tmp, attribs);
        wglChoosePixelFormatARB(dc, attribs, 0, 1, &pixel_format, &num_formats);
    }

    if (num_formats == 0) {
        qWarning("QGLPixelBuffer: Unable to find a pixel format with pbuffer  - giving up.");
        return false;
    }
    format = pfiToQGLFormat(dc, pixel_format);

    // NB! The below ONLY works if the width/height are powers of 2.
    // Set some pBuffer attributes so that we can use this pBuffer as
    // a 2D RGBA texture target.
    int pb_attribs[] = {WGL_TEXTURE_FORMAT_ARB, WGL_TEXTURE_RGBA_ARB,
                        WGL_TEXTURE_TARGET_ARB, WGL_TEXTURE_2D_ARB, 0};

    pbuf = wglCreatePbufferARB(dc, pixel_format, size.width(), size.height(),
                               has_render_texture ? pb_attribs : 0);
    if (!pbuf) {
        // try again without the render_texture extension
        pbuf = wglCreatePbufferARB(dc, pixel_format, size.width(), size.height(), 0);
        has_render_texture = false;
        if (!pbuf) {
            qWarning("QGLPixelBuffer: Unable to create pbuffer [w=%d, h=%d] - giving up.", size.width(), size.height());
            return false;
        }
    }

    dc = wglGetPbufferDCARB(pbuf);
    ctx = wglCreateContext(dc);
    if (!dc || !ctx) {
        qWarning("QGLPixelBuffer: Unable to create pbuffer context - giving up.");
        return false;
    }

    // Explicitly disable the render_texture extension if we have a 
    // multi-sampled pbuffer context. This seems to be a problem only with 
    // ATI cards if multi-sampling is forced globally in the driver.
    wglMakeCurrent(dc, ctx);
    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES_ARB, &samples);
    if (has_render_texture && samples != 0)
        has_render_texture = false;

    HGLRC share_ctx = shareWidget ? shareWidget->d_func()->glcx->d_func()->rc : 0;
    if (share_ctx && !wglShareLists(share_ctx, ctx))
        qWarning("QGLPixelBuffer: Unable to share display lists - with share widget.");

    int width, height;
    wglQueryPbufferARB(pbuf, WGL_PBUFFER_WIDTH_ARB, &width);
    wglQueryPbufferARB(pbuf, WGL_PBUFFER_HEIGHT_ARB, &height);
    return true;
}

bool QGLPixelBufferPrivate::cleanup()
{
    PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB =
        (PFNWGLRELEASEPBUFFERDCARBPROC) wglGetProcAddress("wglReleasePbufferDCARB");
    PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB =
        (PFNWGLDESTROYPBUFFERARBPROC) wglGetProcAddress("wglDestroyPbufferARB");
    if (!invalid && wglReleasePbufferDCARB && wglDestroyPbufferARB) {
        wglReleasePbufferDCARB(pbuf, dc);
        wglDestroyPbufferARB(pbuf);
    }
    return true;
}

bool QGLPixelBuffer::bindToDynamicTexture(GLuint texture_id)
{
    Q_D(QGLPixelBuffer);
    if (d->invalid || !d->has_render_texture)
        return false;
    PFNWGLBINDTEXIMAGEARBPROC wglBindTexImageARB =
        (PFNWGLBINDTEXIMAGEARBPROC) wglGetProcAddress("wglBindTexImageARB");
    if (wglBindTexImageARB) {
        glBindTexture(GL_TEXTURE_2D, texture_id);
        return wglBindTexImageARB(d->pbuf, WGL_FRONT_LEFT_ARB);
    }
    return false;
}

void QGLPixelBuffer::releaseFromDynamicTexture()
{
    Q_D(QGLPixelBuffer);
    if (d->invalid || !d->has_render_texture)
        return;
    PFNWGLRELEASETEXIMAGEARBPROC wglReleaseTexImageARB =
        (PFNWGLRELEASETEXIMAGEARBPROC) wglGetProcAddress("wglReleaseTexImageARB");
    if (wglReleaseTexImageARB)
        wglReleaseTexImageARB(d->pbuf, WGL_FRONT_LEFT_ARB);
}

bool QGLPixelBuffer::hasOpenGLPbuffers()
{
    bool ret = false;
    QGLTemporaryContext *tmpContext = 0;
    if (!QGLContext::currentContext())
        tmpContext = new QGLTemporaryContext;
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
        (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");
    if (wglGetExtensionsStringARB) {
        QString extensions(QLatin1String(wglGetExtensionsStringARB(wglGetCurrentDC())));
        if (extensions.contains(QLatin1String("WGL_ARB_pbuffer"))
            && extensions.contains(QLatin1String("WGL_ARB_pixel_format"))) {
            ret = true;
        }
    }
    if (tmpContext)
        delete tmpContext;
    return ret;
}

QT_END_NAMESPACE
