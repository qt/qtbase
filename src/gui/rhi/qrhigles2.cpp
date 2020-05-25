/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qrhigles2_p_p.h"
#include <QWindow>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QtGui/private/qopenglextensions_p.h>
#include <QtGui/private/qopenglprogrambinarycache_p.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

/*
  OpenGL backend. Binding vertex attribute locations and decomposing uniform
  buffers into uniforms are handled transparently to the application via the
  reflection data (QShaderDescription). Real uniform buffers are never used,
  regardless of the GLSL version. Textures and buffers feature no special
  logic, it's all just glTexSubImage2D and glBufferSubData (with "dynamic"
  buffers set to GL_DYNAMIC_DRAW). The swapchain and the associated
  renderbuffer for depth-stencil will be dummies since we have no control over
  the underlying buffers here. While the baseline here is plain GLES 2.0, some
  modern GL(ES) features like multisample renderbuffers, blits, and compute are
  used when available. Also functional with core profile contexts.
*/

/*!
    \class QRhiGles2InitParams
    \internal
    \inmodule QtGui
    \brief OpenGL specific initialization parameters.

    An OpenGL-based QRhi needs an already created QOffscreenSurface at minimum.
    Additionally, while optional, it is recommended that the QWindow the first
    QRhiSwapChain will target is passed in as well.

    \badcode
        QOffscreenSurface *fallbackSurface = QRhiGles2InitParams::newFallbackSurface();
        QRhiGles2InitParams params;
        params.fallbackSurface = fallbackSurface;
        params.window = window;
        rhi = QRhi::create(QRhi::OpenGLES2, &params);
    \endcode

    By default QRhi creates a QOpenGLContext on its own. This approach works
    well in most cases, included threaded scenarios, where there is a dedicated
    QRhi for each rendering thread. As there will be a QOpenGLContext for each
    QRhi, the OpenGL context requirements (a context can only be current on one
    thread) are satisfied. The implicitly created context is destroyed
    automatically together with the QRhi.

    The QSurfaceFormat for the context is specified in \l format. The
    constructor sets this to QSurfaceFormat::defaultFormat() so applications
    that use QSurfaceFormat::setDefaultFormat() do not need to set the format
    again.

    \note The depth and stencil buffer sizes are set automatically to 24 and 8
    when no size was explicitly set for these buffers in \l format. As there
    are possible adjustments to \l format, applications can use
    adjustedFormat() to query the effective format that is passed to
    QOpenGLContext::setFormat() internally.

    A QOffscreenSurface has to be specified in \l fallbackSurface. In order to
    prevent mistakes in threaded situations, this is never created
    automatically by the QRhi since, like QWindow, QOffscreenSurface can only
    be created on the gui/main thread.

    As a convenience, applications can use newFallbackSurface() which creates
    and returns a QOffscreenSurface that is compatible with the QOpenGLContext
    that is going to be created by the QRhi afterwards. Note that the ownership
    of the returned QOffscreenSurface is transferred to the caller and the QRhi
    will not destroy it.

    \note QRhiSwapChain can only target QWindow instances that have their
    surface type set to QSurface::OpenGLSurface.

    \note \l window is optional. It is recommended to specify it whenever
    possible, in order to avoid problems on multi-adapter and multi-screen
    systems. When \l window is not set, the very first
    QOpenGLContext::makeCurrent() happens with \l fallbackSurface which may be
    an invisible window on some platforms (for example, Windows) and that may
    trigger unexpected problems in some cases.

    \section2 Working with existing OpenGL contexts

    When interoperating with another graphics engine, it may be necessary to
    get a QRhi instance that uses the same OpenGL context. This can be achieved
    by passing a pointer to a QRhiGles2NativeHandles to QRhi::create(). The
    \l{QRhiGles2NativeHandles::context}{context} must be set to a non-null
    value.

    An alternative approach is to create a QOpenGLContext that
    \l{QOpenGLContext::setShareContext()}{shares resources} with the other
    engine's context and passing in that context via QRhiGles2NativeHandles.

    The QRhi does not take ownership of the QOpenGLContext passed in via
    QRhiGles2NativeHandles.
 */

/*!
    \class QRhiGles2NativeHandles
    \internal
    \inmodule QtGui
    \brief Holds the OpenGL context used by the QRhi.
 */

#ifndef GL_BGRA
#define GL_BGRA                           0x80E1
#endif

#ifndef GL_R8
#define GL_R8                             0x8229
#endif

#ifndef GL_R16
#define GL_R16                            0x822A
#endif

#ifndef GL_RED
#define GL_RED                            0x1903
#endif

#ifndef GL_RGBA8
#define GL_RGBA8                          0x8058
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F                        0x8814
#endif

#ifndef GL_RGBA16F
#define GL_RGBA16F                        0x881A
#endif

#ifndef GL_R16F
#define GL_R16F                           0x822D
#endif

#ifndef GL_R32F
#define GL_R32F                           0x822E
#endif

#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT                     0x140B
#endif

#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16              0x81A5
#endif

#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24              0x81A6
#endif

#ifndef GL_DEPTH_COMPONENT32F
#define GL_DEPTH_COMPONENT32F             0x8CAC
#endif

#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX                  0x1901
#endif

#ifndef GL_STENCIL_INDEX8
#define GL_STENCIL_INDEX8                 0x8D48
#endif

#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8               0x88F0
#endif

#ifndef GL_DEPTH_STENCIL_ATTACHMENT
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#endif

#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL                  0x84F9
#endif

#ifndef GL_PRIMITIVE_RESTART_FIXED_INDEX
#define GL_PRIMITIVE_RESTART_FIXED_INDEX  0x8D69
#endif

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER               0x8CA8
#endif

#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#endif

#ifndef GL_MAX_DRAW_BUFFERS
#define GL_MAX_DRAW_BUFFERS               0x8824
#endif

#ifndef GL_TEXTURE_COMPARE_MODE
#define GL_TEXTURE_COMPARE_MODE           0x884C
#endif

#ifndef GL_COMPARE_REF_TO_TEXTURE
#define GL_COMPARE_REF_TO_TEXTURE         0x884E
#endif

#ifndef GL_TEXTURE_COMPARE_FUNC
#define GL_TEXTURE_COMPARE_FUNC           0x884D
#endif

#ifndef GL_MAX_SAMPLES
#define GL_MAX_SAMPLES                    0x8D57
#endif

#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER          0x90D2
#endif

#ifndef GL_READ_ONLY
#define GL_READ_ONLY                      0x88B8
#endif

#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY                     0x88B9
#endif

#ifndef GL_READ_WRITE
#define GL_READ_WRITE                     0x88BA
#endif

#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER                 0x91B9
#endif

#ifndef GL_ALL_BARRIER_BITS
#define GL_ALL_BARRIER_BITS               0xFFFFFFFF
#endif

#ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#endif

#ifndef GL_SHADER_STORAGE_BARRIER_BIT
#define GL_SHADER_STORAGE_BARRIER_BIT     0x00002000
#endif

#ifndef GL_VERTEX_PROGRAM_POINT_SIZE
#define GL_VERTEX_PROGRAM_POINT_SIZE      0x8642
#endif

#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE                   0x8861
#endif

#ifndef GL_MAP_READ_BIT
#define GL_MAP_READ_BIT                   0x0001
#endif

Q_DECLARE_LOGGING_CATEGORY(lcOpenGLProgramDiskCache)

/*!
    Constructs a new QRhiGles2InitParams.

    \l format is set to QSurfaceFormat::defaultFormat().
 */
QRhiGles2InitParams::QRhiGles2InitParams()
{
    format = QSurfaceFormat::defaultFormat();
}

/*!
    \return the QSurfaceFormat that will be set on the QOpenGLContext before
    calling QOpenGLContext::create(). This format is based on \a format, but
    may be adjusted. Applicable only when QRhi creates the context.
    Applications are advised to set this format on their QWindow in order to
    avoid potential BAD_MATCH failures.
 */
QSurfaceFormat QRhiGles2InitParams::adjustedFormat(const QSurfaceFormat &format)
{
    QSurfaceFormat fmt = format;

    if (fmt.depthBufferSize() == -1)
        fmt.setDepthBufferSize(24);
    if (fmt.stencilBufferSize() == -1)
        fmt.setStencilBufferSize(8);

    return fmt;
}

/*!
    \return a new QOffscreenSurface that can be used with a QRhi by passing it
    via a QRhiGles2InitParams.

    \a format is adjusted as appropriate in order to avoid having problems
    afterwards due to an incompatible context and surface.

    \note This function must only be called on the gui/main thread.

    \note It is the application's responsibility to destroy the returned
    QOffscreenSurface on the gui/main thread once the associated QRhi has been
    destroyed. The QRhi will not destroy the QOffscreenSurface.
 */
QOffscreenSurface *QRhiGles2InitParams::newFallbackSurface(const QSurfaceFormat &format)
{
    QSurfaceFormat fmt = adjustedFormat(format);

    // To resolve all fields in the format as much as possible, create a context.
    // This may be heavy, but allows avoiding BAD_MATCH on some systems.
    QOpenGLContext tempContext;
    tempContext.setFormat(fmt);
    if (tempContext.create())
        fmt = tempContext.format();
    else
        qWarning("QRhiGles2: Failed to create temporary context");

    QOffscreenSurface *s = new QOffscreenSurface;
    s->setFormat(fmt);
    s->create();

    return s;
}

QRhiGles2::QRhiGles2(QRhiGles2InitParams *params, QRhiGles2NativeHandles *importDevice)
    : ofr(this)
{
    requestedFormat = QRhiGles2InitParams::adjustedFormat(params->format);
    fallbackSurface = params->fallbackSurface;
    maybeWindow = params->window; // may be null

    importedContext = importDevice != nullptr;
    if (importedContext) {
        ctx = importDevice->context;
        if (!ctx) {
            qWarning("No OpenGL context given, cannot import");
            importedContext = false;
        }
    }
}

bool QRhiGles2::ensureContext(QSurface *surface) const
{
    bool nativeWindowGone = false;
    if (surface && surface->surfaceClass() == QSurface::Window && !surface->surfaceHandle()) {
        surface = fallbackSurface;
        nativeWindowGone = true;
    }

    if (!surface)
        surface = fallbackSurface;

    if (needsMakeCurrent)
        needsMakeCurrent = false;
    else if (!nativeWindowGone && QOpenGLContext::currentContext() == ctx && (surface == fallbackSurface || ctx->surface() == surface))
        return true;

    if (!ctx->makeCurrent(surface)) {
        if (ctx->isValid()) {
            qWarning("QRhiGles2: Failed to make context current. Expect bad things to happen.");
        } else {
            qWarning("QRhiGles2: Context is lost.");
            contextLost = true;
        }
        return false;
    }

    return true;
}

bool QRhiGles2::create(QRhi::Flags flags)
{
    Q_UNUSED(flags);
    Q_ASSERT(fallbackSurface);

    if (!importedContext) {
        ctx = new QOpenGLContext;
        ctx->setFormat(requestedFormat);
        if (!ctx->create()) {
            qWarning("QRhiGles2: Failed to create context");
            delete ctx;
            ctx = nullptr;
            return false;
        }
        qCDebug(QRHI_LOG_INFO) << "Created OpenGL context" << ctx->format();
    }

    if (!ensureContext(maybeWindow ? maybeWindow : fallbackSurface)) // see 'window' discussion in QRhiGles2InitParams comments
        return false;

    f = static_cast<QOpenGLExtensions *>(ctx->extraFunctions());

    const char *vendor = reinterpret_cast<const char *>(f->glGetString(GL_VENDOR));
    const char *renderer = reinterpret_cast<const char *>(f->glGetString(GL_RENDERER));
    const char *version = reinterpret_cast<const char *>(f->glGetString(GL_VERSION));
    if (vendor && renderer && version)
        qCDebug(QRHI_LOG_INFO, "OpenGL VENDOR: %s RENDERER: %s VERSION: %s", vendor, renderer, version);

    const QSurfaceFormat actualFormat = ctx->format();

    caps.ctxMajor = actualFormat.majorVersion();
    caps.ctxMinor = actualFormat.minorVersion();

    GLint n = 0;
    f->glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &n);
    supportedCompressedFormats.resize(n);
    if (n > 0)
        f->glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, supportedCompressedFormats.data());

    f->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.maxTextureSize);

    if (caps.ctxMajor >= 3 || actualFormat.renderableType() == QSurfaceFormat::OpenGL) {
        f->glGetIntegerv(GL_MAX_DRAW_BUFFERS, &caps.maxDrawBuffers);
        f->glGetIntegerv(GL_MAX_SAMPLES, &caps.maxSamples);
        caps.maxSamples = qMax(1, caps.maxSamples);
    } else {
        caps.maxDrawBuffers = 1;
        caps.maxSamples = 1;
    }

    caps.msaaRenderBuffer = f->hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample)
            && f->hasOpenGLExtension(QOpenGLExtensions::FramebufferBlit);

    caps.npotTextureFull = f->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures)
            && f->hasOpenGLFeature(QOpenGLFunctions::NPOTTextureRepeat);

    caps.gles = actualFormat.renderableType() == QSurfaceFormat::OpenGLES;
    if (caps.gles)
        caps.fixedIndexPrimitiveRestart = caps.ctxMajor >= 3; // ES 3.0
    else
        caps.fixedIndexPrimitiveRestart = caps.ctxMajor > 4 || (caps.ctxMajor == 4 && caps.ctxMinor >= 3); // 4.3

    if (caps.fixedIndexPrimitiveRestart)
        f->glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

    caps.bgraExternalFormat = f->hasOpenGLExtension(QOpenGLExtensions::BGRATextureFormat);
    caps.bgraInternalFormat = caps.bgraExternalFormat && caps.gles;
    caps.r8Format = f->hasOpenGLFeature(QOpenGLFunctions::TextureRGFormats);
    caps.r16Format = f->hasOpenGLExtension(QOpenGLExtensions::Sized16Formats);
    caps.floatFormats = caps.ctxMajor >= 3; // 3.0 or ES 3.0
    caps.depthTexture = caps.ctxMajor >= 3; // 3.0 or ES 3.0
    caps.packedDepthStencil = f->hasOpenGLExtension(QOpenGLExtensions::PackedDepthStencil);
#ifdef Q_OS_WASM
    caps.needsDepthStencilCombinedAttach = true;
#else
    caps.needsDepthStencilCombinedAttach = false;
#endif
    caps.srgbCapableDefaultFramebuffer = f->hasOpenGLExtension(QOpenGLExtensions::SRGBFrameBuffer);
    caps.coreProfile = actualFormat.profile() == QSurfaceFormat::CoreProfile;

    if (caps.gles)
        caps.uniformBuffers = caps.ctxMajor >= 3; // ES 3.0
    else
        caps.uniformBuffers = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 1); // 3.1

    caps.elementIndexUint = f->hasOpenGLExtension(QOpenGLExtensions::ElementIndexUint);
    caps.depth24 = f->hasOpenGLExtension(QOpenGLExtensions::Depth24);
    caps.rgba8Format = f->hasOpenGLExtension(QOpenGLExtensions::Sized8Formats);

    if (caps.gles)
        caps.instancing = caps.ctxMajor >= 3; // ES 3.0
    else
        caps.instancing = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 3); // 3.3

    caps.baseVertex = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 2); // 3.2 or ES 3.2

    if (caps.gles)
        caps.compute = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 1); // ES 3.1
    else
        caps.compute = caps.ctxMajor > 4 || (caps.ctxMajor == 4 && caps.ctxMinor >= 3); // 4.3

    if (caps.gles)
        caps.textureCompareMode = caps.ctxMajor >= 3; // ES 3.0
    else
        caps.textureCompareMode = true;

    // proper as in ES 3.0 (glMapBufferRange), not the old glMapBuffer
    // extension(s) (which is not in ES 3.0...messy)
    caps.properMapBuffer = f->hasOpenGLExtension(QOpenGLExtensions::MapBufferRange);

    if (caps.gles)
        caps.nonBaseLevelFramebufferTexture = caps.ctxMajor >= 3; // ES 3.0
    else
        caps.nonBaseLevelFramebufferTexture = true;

    caps.texelFetch = caps.ctxMajor >= 3; // 3.0 or ES 3.0

    if (!caps.gles) {
        f->glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        f->glEnable(GL_POINT_SPRITE);
    } // else (with gles) these are always on

    nativeHandlesStruct.context = ctx;

    contextLost = false;

    return true;
}

void QRhiGles2::destroy()
{
    if (!f)
        return;

    ensureContext();
    executeDeferredReleases();

    if (vao) {
        f->glDeleteVertexArrays(1, &vao);
        vao = 0;
    }

    for (uint shader : m_shaderCache)
        f->glDeleteShader(shader);
    m_shaderCache.clear();

    if (!importedContext) {
        delete ctx;
        ctx = nullptr;
    }

    f = nullptr;
}

void QRhiGles2::executeDeferredReleases()
{
    for (int i = releaseQueue.count() - 1; i >= 0; --i) {
        const QRhiGles2::DeferredReleaseEntry &e(releaseQueue[i]);
        switch (e.type) {
        case QRhiGles2::DeferredReleaseEntry::Buffer:
            f->glDeleteBuffers(1, &e.buffer.buffer);
            break;
        case QRhiGles2::DeferredReleaseEntry::Pipeline:
            f->glDeleteProgram(e.pipeline.program);
            break;
        case QRhiGles2::DeferredReleaseEntry::Texture:
            f->glDeleteTextures(1, &e.texture.texture);
            break;
        case QRhiGles2::DeferredReleaseEntry::RenderBuffer:
            f->glDeleteRenderbuffers(1, &e.renderbuffer.renderbuffer);
            f->glDeleteRenderbuffers(1, &e.renderbuffer.renderbuffer2);
            break;
        case QRhiGles2::DeferredReleaseEntry::TextureRenderTarget:
            f->glDeleteFramebuffers(1, &e.textureRenderTarget.framebuffer);
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
        releaseQueue.removeAt(i);
    }
}

QVector<int> QRhiGles2::supportedSampleCounts() const
{
    if (supportedSampleCountList.isEmpty()) {
        // 1, 2, 4, 8, ...
        for (int i = 1; i <= caps.maxSamples; i *= 2)
            supportedSampleCountList.append(i);
    }
    return supportedSampleCountList;
}

int QRhiGles2::effectiveSampleCount(int sampleCount) const
{
    // Stay compatible with QSurfaceFormat and friends where samples == 0 means the same as 1.
    const int s = qBound(1, sampleCount, 64);
    if (!supportedSampleCounts().contains(s)) {
        qWarning("Attempted to set unsupported sample count %d", sampleCount);
        return 1;
    }
    return s;
}

QRhiSwapChain *QRhiGles2::createSwapChain()
{
    return new QGles2SwapChain(this);
}

QRhiBuffer *QRhiGles2::createBuffer(QRhiBuffer::Type type, QRhiBuffer::UsageFlags usage, int size)
{
    return new QGles2Buffer(this, type, usage, size);
}

int QRhiGles2::ubufAlignment() const
{
    // No real uniform buffers are used so no need to pretend there is any
    // alignment requirement.
    return 1;
}

bool QRhiGles2::isYUpInFramebuffer() const
{
    return true;
}

bool QRhiGles2::isYUpInNDC() const
{
    return true;
}

bool QRhiGles2::isClipDepthZeroToOne() const
{
    return false;
}

QMatrix4x4 QRhiGles2::clipSpaceCorrMatrix() const
{
    return QMatrix4x4(); // identity
}

static inline GLenum toGlCompressedTextureFormat(QRhiTexture::Format format, QRhiTexture::Flags flags)
{
    const bool srgb = flags.testFlag(QRhiTexture::sRGB);
    switch (format) {
    case QRhiTexture::BC1:
        return srgb ? 0x8C4C : 0x83F0;
    case QRhiTexture::BC2:
        return srgb ? 0x8C4E : 0x83F2;
    case QRhiTexture::BC3:
        return srgb ? 0x8C4F : 0x83F3;

    case QRhiTexture::ETC2_RGB8:
        return srgb ? 0x9275 : 0x9274;
    case QRhiTexture::ETC2_RGB8A1:
        return srgb ? 0x9277 : 0x9276;
    case QRhiTexture::ETC2_RGBA8:
        return srgb ? 0x9279 : 0x9278;

    case QRhiTexture::ASTC_4x4:
        return srgb ? 0x93D0 : 0x93B0;
    case QRhiTexture::ASTC_5x4:
        return srgb ? 0x93D1 : 0x93B1;
    case QRhiTexture::ASTC_5x5:
        return srgb ? 0x93D2 : 0x93B2;
    case QRhiTexture::ASTC_6x5:
        return srgb ? 0x93D3 : 0x93B3;
    case QRhiTexture::ASTC_6x6:
        return srgb ? 0x93D4 : 0x93B4;
    case QRhiTexture::ASTC_8x5:
        return srgb ? 0x93D5 : 0x93B5;
    case QRhiTexture::ASTC_8x6:
        return srgb ? 0x93D6 : 0x93B6;
    case QRhiTexture::ASTC_8x8:
        return srgb ? 0x93D7 : 0x93B7;
    case QRhiTexture::ASTC_10x5:
        return srgb ? 0x93D8 : 0x93B8;
    case QRhiTexture::ASTC_10x6:
        return srgb ? 0x93D9 : 0x93B9;
    case QRhiTexture::ASTC_10x8:
        return srgb ? 0x93DA : 0x93BA;
    case QRhiTexture::ASTC_10x10:
        return srgb ? 0x93DB : 0x93BB;
    case QRhiTexture::ASTC_12x10:
        return srgb ? 0x93DC : 0x93BC;
    case QRhiTexture::ASTC_12x12:
        return srgb ? 0x93DD : 0x93BD;

    default:
        return 0; // this is reachable, just return an invalid format
    }
}

bool QRhiGles2::isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const
{
    if (isCompressedFormat(format))
        return supportedCompressedFormats.contains(GLint(toGlCompressedTextureFormat(format, flags)));

    switch (format) {
    case QRhiTexture::D16:
    case QRhiTexture::D32F:
        return caps.depthTexture;

    case QRhiTexture::BGRA8:
        return caps.bgraExternalFormat;

    case QRhiTexture::R8:
        return caps.r8Format;

    case QRhiTexture::R16:
        return caps.r16Format;

    case QRhiTexture::RGBA16F:
    case QRhiTexture::RGBA32F:
        return caps.floatFormats;

    case QRhiTexture::R16F:
    case QRhiTexture::R32F:
        return caps.floatFormats;

    default:
        break;
    }

    return true;
}

bool QRhiGles2::isFeatureSupported(QRhi::Feature feature) const
{
    switch (feature) {
    case QRhi::MultisampleTexture:
        return false;
    case QRhi::MultisampleRenderBuffer:
        return caps.msaaRenderBuffer;
    case QRhi::DebugMarkers:
        return false;
    case QRhi::Timestamps:
        return false;
    case QRhi::Instancing:
        return caps.instancing;
    case QRhi::CustomInstanceStepRate:
        return false;
    case QRhi::PrimitiveRestart:
        return caps.fixedIndexPrimitiveRestart;
    case QRhi::NonDynamicUniformBuffers:
        return true;
    case QRhi::NonFourAlignedEffectiveIndexBufferOffset:
        return true;
    case QRhi::NPOTTextureRepeat:
        return caps.npotTextureFull;
    case QRhi::RedOrAlpha8IsRed:
        return caps.coreProfile;
    case QRhi::ElementIndexUint:
        return caps.elementIndexUint;
    case QRhi::Compute:
        return caps.compute;
    case QRhi::WideLines:
        return true;
    case QRhi::VertexShaderPointSize:
        return true;
    case QRhi::BaseVertex:
        return caps.baseVertex;
    case QRhi::BaseInstance:
        return false; // not in ES 3.2, so won't bother
    case QRhi::TriangleFanTopology:
        return true;
    case QRhi::ReadBackNonUniformBuffer:
        return !caps.gles || caps.properMapBuffer;
    case QRhi::ReadBackNonBaseMipLevel:
        return caps.nonBaseLevelFramebufferTexture;
    case QRhi::TexelFetch:
        return caps.texelFetch;
    default:
        Q_UNREACHABLE();
        return false;
    }
}

int QRhiGles2::resourceLimit(QRhi::ResourceLimit limit) const
{
    switch (limit) {
    case QRhi::TextureSizeMin:
        return 1;
    case QRhi::TextureSizeMax:
        return caps.maxTextureSize;
    case QRhi::MaxColorAttachments:
        return caps.maxDrawBuffers;
    case QRhi::FramesInFlight:
        // From our perspective. What the GL impl does internally is another
        // question, but that's out of our hands and does not concern us here.
        return 1;
    case QRhi::MaxAsyncReadbackFrames:
        return 1;
    default:
        Q_UNREACHABLE();
        return 0;
    }
}

const QRhiNativeHandles *QRhiGles2::nativeHandles()
{
    return &nativeHandlesStruct;
}

void QRhiGles2::sendVMemStatsToProfiler()
{
    // nothing to do here
}

bool QRhiGles2::makeThreadLocalNativeContextCurrent()
{
    if (inFrame && !ofr.active)
        return ensureContext(currentSwapChain->surface);
    else
        return ensureContext();
}

void QRhiGles2::releaseCachedResources()
{
    if (!ensureContext())
        return;

    for (uint shader : m_shaderCache)
        f->glDeleteShader(shader);

    m_shaderCache.clear();
}

bool QRhiGles2::isDeviceLost() const
{
    return contextLost;
}

QRhiRenderBuffer *QRhiGles2::createRenderBuffer(QRhiRenderBuffer::Type type, const QSize &pixelSize,
                                                int sampleCount, QRhiRenderBuffer::Flags flags)
{
    return new QGles2RenderBuffer(this, type, pixelSize, sampleCount, flags);
}

QRhiTexture *QRhiGles2::createTexture(QRhiTexture::Format format, const QSize &pixelSize,
                                      int sampleCount, QRhiTexture::Flags flags)
{
    return new QGles2Texture(this, format, pixelSize, sampleCount, flags);
}

QRhiSampler *QRhiGles2::createSampler(QRhiSampler::Filter magFilter, QRhiSampler::Filter minFilter,
                                      QRhiSampler::Filter mipmapMode,
                                      QRhiSampler::AddressMode u, QRhiSampler::AddressMode v, QRhiSampler::AddressMode w)
{
    return new QGles2Sampler(this, magFilter, minFilter, mipmapMode, u, v, w);
}

QRhiTextureRenderTarget *QRhiGles2::createTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                              QRhiTextureRenderTarget::Flags flags)
{
    return new QGles2TextureRenderTarget(this, desc, flags);
}

QRhiGraphicsPipeline *QRhiGles2::createGraphicsPipeline()
{
    return new QGles2GraphicsPipeline(this);
}

QRhiShaderResourceBindings *QRhiGles2::createShaderResourceBindings()
{
    return new QGles2ShaderResourceBindings(this);
}

QRhiComputePipeline *QRhiGles2::createComputePipeline()
{
    return new QGles2ComputePipeline(this);
}

void QRhiGles2::setGraphicsPipeline(QRhiCommandBuffer *cb, QRhiGraphicsPipeline *ps)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);
    QGles2GraphicsPipeline *psD = QRHI_RES(QGles2GraphicsPipeline, ps);
    const bool pipelineChanged = cbD->currentGraphicsPipeline != ps || cbD->currentPipelineGeneration != psD->generation;

    if (pipelineChanged) {
        cbD->currentGraphicsPipeline = ps;
        cbD->currentComputePipeline = nullptr;
        cbD->currentPipelineGeneration = psD->generation;

        QGles2CommandBuffer::Command cmd;
        cmd.cmd = QGles2CommandBuffer::Command::BindGraphicsPipeline;
        cmd.args.bindGraphicsPipeline.ps = ps;
        cbD->commands.append(cmd);
    }
}

void QRhiGles2::setShaderResources(QRhiCommandBuffer *cb, QRhiShaderResourceBindings *srb,
                                   int dynamicOffsetCount,
                                   const QRhiCommandBuffer::DynamicOffset *dynamicOffsets)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass != QGles2CommandBuffer::NoPass);
    QGles2GraphicsPipeline *gfxPsD = QRHI_RES(QGles2GraphicsPipeline, cbD->currentGraphicsPipeline);
    QGles2ComputePipeline *compPsD = QRHI_RES(QGles2ComputePipeline, cbD->currentComputePipeline);

    if (!srb) {
        if (gfxPsD)
            srb = gfxPsD->m_shaderResourceBindings;
        else
            srb = compPsD->m_shaderResourceBindings;
    }

    QRhiPassResourceTracker &passResTracker(cbD->passResTrackers[cbD->currentPassResTrackerIndex]);
    QGles2ShaderResourceBindings *srbD = QRHI_RES(QGles2ShaderResourceBindings, srb);
    bool hasDynamicOffsetInSrb = false;
    for (int i = 0, ie = srbD->m_bindings.count(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = srbD->m_bindings.at(i).data();
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
            // no BufUniformRead / AccessUniform because no real uniform buffers are used
            if (b->u.ubuf.hasDynamicOffset)
                hasDynamicOffsetInSrb = true;
            break;
        case QRhiShaderResourceBinding::SampledTexture:
            for (int elem = 0; elem < b->u.stex.count; ++elem) {
                trackedRegisterTexture(&passResTracker,
                                       QRHI_RES(QGles2Texture, b->u.stex.texSamplers[elem].tex),
                                       QRhiPassResourceTracker::TexSample,
                                       QRhiPassResourceTracker::toPassTrackerTextureStage(b->stage));
            }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
        {
            QGles2Texture *texD = QRHI_RES(QGles2Texture, b->u.simage.tex);
            QRhiPassResourceTracker::TextureAccess access;
            if (b->type == QRhiShaderResourceBinding::ImageLoad)
                access = QRhiPassResourceTracker::TexStorageLoad;
            else if (b->type == QRhiShaderResourceBinding::ImageStore)
                access = QRhiPassResourceTracker::TexStorageStore;
            else
                access = QRhiPassResourceTracker::TexStorageLoadStore;
            trackedRegisterTexture(&passResTracker, texD, access,
                                   QRhiPassResourceTracker::toPassTrackerTextureStage(b->stage));
        }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
        case QRhiShaderResourceBinding::BufferStore:
        case QRhiShaderResourceBinding::BufferLoadStore:
        {
            QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, b->u.sbuf.buf);
            QRhiPassResourceTracker::BufferAccess access;
            if (b->type == QRhiShaderResourceBinding::BufferLoad)
                access = QRhiPassResourceTracker::BufStorageLoad;
            else if (b->type == QRhiShaderResourceBinding::BufferStore)
                access = QRhiPassResourceTracker::BufStorageStore;
            else
                access = QRhiPassResourceTracker::BufStorageLoadStore;
            trackedRegisterBuffer(&passResTracker, bufD, access,
                                  QRhiPassResourceTracker::toPassTrackerBufferStage(b->stage));
        }
            break;
        default:
            break;
        }
    }

    const bool srbChanged = gfxPsD ? (cbD->currentGraphicsSrb != srb) : (cbD->currentComputeSrb != srb);
    const bool srbRebuilt = cbD->currentSrbGeneration != srbD->generation;

    if (srbChanged || srbRebuilt || hasDynamicOffsetInSrb) {
        if (gfxPsD) {
            cbD->currentGraphicsSrb = srb;
            cbD->currentComputeSrb = nullptr;
        } else {
            cbD->currentGraphicsSrb = nullptr;
            cbD->currentComputeSrb = srb;
        }
        cbD->currentSrbGeneration = srbD->generation;

        QGles2CommandBuffer::Command cmd;
        cmd.cmd = QGles2CommandBuffer::Command::BindShaderResources;
        cmd.args.bindShaderResources.maybeGraphicsPs = gfxPsD;
        cmd.args.bindShaderResources.maybeComputePs = compPsD;
        cmd.args.bindShaderResources.srb = srb;
        cmd.args.bindShaderResources.dynamicOffsetCount = 0;
        if (hasDynamicOffsetInSrb) {
            if (dynamicOffsetCount < QGles2CommandBuffer::Command::MAX_UBUF_BINDINGS) {
                cmd.args.bindShaderResources.dynamicOffsetCount = dynamicOffsetCount;
                uint *p = cmd.args.bindShaderResources.dynamicOffsetPairs;
                for (int i = 0; i < dynamicOffsetCount; ++i) {
                    const QRhiCommandBuffer::DynamicOffset &dynOfs(dynamicOffsets[i]);
                    *p++ = uint(dynOfs.first);
                    *p++ = dynOfs.second;
                }
            } else {
                qWarning("Too many dynamic offsets (%d, max is %d)",
                         dynamicOffsetCount, QGles2CommandBuffer::Command::MAX_UBUF_BINDINGS);
            }
        }
        cbD->commands.append(cmd);
    }
}

void QRhiGles2::setVertexInput(QRhiCommandBuffer *cb,
                               int startBinding, int bindingCount, const QRhiCommandBuffer::VertexInput *bindings,
                               QRhiBuffer *indexBuf, quint32 indexOffset, QRhiCommandBuffer::IndexFormat indexFormat)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);
    QRhiPassResourceTracker &passResTracker(cbD->passResTrackers[cbD->currentPassResTrackerIndex]);

    for (int i = 0; i < bindingCount; ++i) {
        QRhiBuffer *buf = bindings[i].first;
        quint32 ofs = bindings[i].second;
        QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, buf);
        Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::VertexBuffer));

        QGles2CommandBuffer::Command cmd;
        cmd.cmd = QGles2CommandBuffer::Command::BindVertexBuffer;
        cmd.args.bindVertexBuffer.ps = cbD->currentGraphicsPipeline;
        cmd.args.bindVertexBuffer.buffer = bufD->buffer;
        cmd.args.bindVertexBuffer.offset = ofs;
        cmd.args.bindVertexBuffer.binding = startBinding + i;
        cbD->commands.append(cmd);

        trackedRegisterBuffer(&passResTracker, bufD, QRhiPassResourceTracker::BufVertexInput,
                              QRhiPassResourceTracker::BufVertexInputStage);
    }

    if (indexBuf) {
        QGles2Buffer *ibufD = QRHI_RES(QGles2Buffer, indexBuf);
        Q_ASSERT(ibufD->m_usage.testFlag(QRhiBuffer::IndexBuffer));

        QGles2CommandBuffer::Command cmd;
        cmd.cmd = QGles2CommandBuffer::Command::BindIndexBuffer;
        cmd.args.bindIndexBuffer.buffer = ibufD->buffer;
        cmd.args.bindIndexBuffer.offset = indexOffset;
        cmd.args.bindIndexBuffer.type = indexFormat == QRhiCommandBuffer::IndexUInt16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
        cbD->commands.append(cmd);

        trackedRegisterBuffer(&passResTracker, ibufD, QRhiPassResourceTracker::BufIndexRead,
                              QRhiPassResourceTracker::BufVertexInputStage);
    }
}

void QRhiGles2::setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    QGles2CommandBuffer::Command cmd;
    cmd.cmd = QGles2CommandBuffer::Command::Viewport;
    const std::array<float, 4> r = viewport.viewport();
    // A negative width or height is an error. A negative x or y is not.
    if (r[2] < 0.0f || r[3] < 0.0f)
        return;

    cmd.args.viewport.x = r[0];
    cmd.args.viewport.y = r[1];
    cmd.args.viewport.w = r[2];
    cmd.args.viewport.h = r[3];
    cmd.args.viewport.d0 = viewport.minDepth();
    cmd.args.viewport.d1 = viewport.maxDepth();
    cbD->commands.append(cmd);
}

void QRhiGles2::setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    QGles2CommandBuffer::Command cmd;
    cmd.cmd = QGles2CommandBuffer::Command::Scissor;
    const std::array<int, 4> r = scissor.scissor();
    // A negative width or height is an error. A negative x or y is not.
    if (r[2] < 0 || r[3] < 0)
        return;

    cmd.args.scissor.x = r[0];
    cmd.args.scissor.y = r[1];
    cmd.args.scissor.w = r[2];
    cmd.args.scissor.h = r[3];
    cbD->commands.append(cmd);
}

void QRhiGles2::setBlendConstants(QRhiCommandBuffer *cb, const QColor &c)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    QGles2CommandBuffer::Command cmd;
    cmd.cmd = QGles2CommandBuffer::Command::BlendConstants;
    cmd.args.blendConstants.r = float(c.redF());
    cmd.args.blendConstants.g = float(c.greenF());
    cmd.args.blendConstants.b = float(c.blueF());
    cmd.args.blendConstants.a = float(c.alphaF());
    cbD->commands.append(cmd);
}

void QRhiGles2::setStencilRef(QRhiCommandBuffer *cb, quint32 refValue)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    QGles2CommandBuffer::Command cmd;
    cmd.cmd = QGles2CommandBuffer::Command::StencilRef;
    cmd.args.stencilRef.ref = refValue;
    cmd.args.stencilRef.ps = cbD->currentGraphicsPipeline;
    cbD->commands.append(cmd);
}

void QRhiGles2::draw(QRhiCommandBuffer *cb, quint32 vertexCount,
                     quint32 instanceCount, quint32 firstVertex, quint32 firstInstance)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    QGles2CommandBuffer::Command cmd;
    cmd.cmd = QGles2CommandBuffer::Command::Draw;
    cmd.args.draw.ps = cbD->currentGraphicsPipeline;
    cmd.args.draw.vertexCount = vertexCount;
    cmd.args.draw.firstVertex = firstVertex;
    cmd.args.draw.instanceCount = instanceCount;
    cmd.args.draw.baseInstance = firstInstance;
    cbD->commands.append(cmd);
}

void QRhiGles2::drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                            quint32 instanceCount, quint32 firstIndex, qint32 vertexOffset, quint32 firstInstance)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    QGles2CommandBuffer::Command cmd;
    cmd.cmd = QGles2CommandBuffer::Command::DrawIndexed;
    cmd.args.drawIndexed.ps = cbD->currentGraphicsPipeline;
    cmd.args.drawIndexed.indexCount = indexCount;
    cmd.args.drawIndexed.firstIndex = firstIndex;
    cmd.args.drawIndexed.instanceCount = instanceCount;
    cmd.args.drawIndexed.baseInstance = firstInstance;
    cmd.args.drawIndexed.baseVertex = vertexOffset;
    cbD->commands.append(cmd);
}

void QRhiGles2::debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name)
{
    if (!debugMarkers)
        return;

    Q_UNUSED(cb);
    Q_UNUSED(name);
}

void QRhiGles2::debugMarkEnd(QRhiCommandBuffer *cb)
{
    if (!debugMarkers)
        return;

    Q_UNUSED(cb);
}

void QRhiGles2::debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg)
{
    if (!debugMarkers)
        return;

    Q_UNUSED(cb);
    Q_UNUSED(msg);
}

const QRhiNativeHandles *QRhiGles2::nativeHandles(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
    return nullptr;
}

static void addBoundaryCommand(QGles2CommandBuffer *cbD, QGles2CommandBuffer::Command::Cmd type)
{
    QGles2CommandBuffer::Command cmd;
    cmd.cmd = type;
    cbD->commands.append(cmd);
}

void QRhiGles2::beginExternal(QRhiCommandBuffer *cb)
{
    if (ofr.active) {
        Q_ASSERT(!currentSwapChain);
        if (!ensureContext())
            return;
    } else {
        Q_ASSERT(currentSwapChain);
        if (!ensureContext(currentSwapChain->surface))
            return;
    }

    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);

    if (cbD->recordingPass == QGles2CommandBuffer::ComputePass
            && !cbD->computePassState.writtenResources.isEmpty())
    {
        QGles2CommandBuffer::Command cmd;
        cmd.cmd = QGles2CommandBuffer::Command::Barrier;
        cmd.args.barrier.barriers = GL_ALL_BARRIER_BITS;
        cbD->commands.append(cmd);
    }

    executeCommandBuffer(cbD);

    cbD->resetCommands();

    if (vao)
        f->glBindVertexArray(0);
}

void QRhiGles2::endExternal(QRhiCommandBuffer *cb)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->commands.isEmpty() && cbD->currentPassResTrackerIndex == -1);

    cbD->resetCachedState();

    if (cbD->recordingPass != QGles2CommandBuffer::NoPass) {
        // Commands that come after this point need a resource tracker and also
        // a BarriersForPass command enqueued. (the ones we had from
        // beginPass() are now gone since beginExternal() processed all that
        // due to calling executeCommandBuffer()).
        enqueueBarriersForPass(cbD);
    }

    addBoundaryCommand(cbD, QGles2CommandBuffer::Command::ResetFrame);

    if (cbD->currentTarget)
        enqueueBindFramebuffer(cbD->currentTarget, cbD);
}

QRhi::FrameOpResult QRhiGles2::beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);

    QGles2SwapChain *swapChainD = QRHI_RES(QGles2SwapChain, swapChain);
    if (!ensureContext(swapChainD->surface))
        return contextLost ? QRhi::FrameOpDeviceLost : QRhi::FrameOpError;

    currentSwapChain = swapChainD;

    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();
    QRHI_PROF_F(beginSwapChainFrame(swapChain));

    executeDeferredReleases();
    swapChainD->cb.resetState();

    addBoundaryCommand(&swapChainD->cb, QGles2CommandBuffer::Command::BeginFrame);

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiGles2::endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags)
{
    QGles2SwapChain *swapChainD = QRHI_RES(QGles2SwapChain, swapChain);
    Q_ASSERT(currentSwapChain == swapChainD);

    addBoundaryCommand(&swapChainD->cb, QGles2CommandBuffer::Command::EndFrame);

    if (!ensureContext(swapChainD->surface))
        return contextLost ? QRhi::FrameOpDeviceLost : QRhi::FrameOpError;

    executeCommandBuffer(&swapChainD->cb);

    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();
    // this must be done before the swap
    QRHI_PROF_F(endSwapChainFrame(swapChain, swapChainD->frameCount + 1));

    if (swapChainD->surface && !flags.testFlag(QRhi::SkipPresent)) {
        ctx->swapBuffers(swapChainD->surface);
        needsMakeCurrent = true;
    } else {
        f->glFlush();
    }

    swapChainD->frameCount += 1;
    currentSwapChain = nullptr;
    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiGles2::beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);
    if (!ensureContext())
        return contextLost ? QRhi::FrameOpDeviceLost : QRhi::FrameOpError;

    ofr.active = true;

    executeDeferredReleases();
    ofr.cbWrapper.resetState();

    addBoundaryCommand(&ofr.cbWrapper, QGles2CommandBuffer::Command::BeginFrame);
    *cb = &ofr.cbWrapper;

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiGles2::endOffscreenFrame(QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    Q_ASSERT(ofr.active);
    ofr.active = false;

    addBoundaryCommand(&ofr.cbWrapper, QGles2CommandBuffer::Command::EndFrame);

    if (!ensureContext())
        return contextLost ? QRhi::FrameOpDeviceLost : QRhi::FrameOpError;

    executeCommandBuffer(&ofr.cbWrapper);

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiGles2::finish()
{
    if (inFrame) {
        if (ofr.active) {
            Q_ASSERT(!currentSwapChain);
            Q_ASSERT(ofr.cbWrapper.recordingPass == QGles2CommandBuffer::NoPass);
            if (!ensureContext())
                return contextLost ? QRhi::FrameOpDeviceLost : QRhi::FrameOpError;
            executeCommandBuffer(&ofr.cbWrapper);
            ofr.cbWrapper.resetCommands();
        } else {
            Q_ASSERT(currentSwapChain);
            Q_ASSERT(currentSwapChain->cb.recordingPass == QGles2CommandBuffer::NoPass);
            if (!ensureContext(currentSwapChain->surface))
                return contextLost ? QRhi::FrameOpDeviceLost : QRhi::FrameOpError;
            executeCommandBuffer(&currentSwapChain->cb);
            currentSwapChain->cb.resetCommands();
        }
    }
    return QRhi::FrameOpSuccess;
}

static bool bufferAccessIsWrite(QGles2Buffer::Access access)
{
    return access == QGles2Buffer::AccessStorageWrite
        || access == QGles2Buffer::AccessStorageReadWrite
        || access == QGles2Buffer::AccessUpdate;
}

static bool textureAccessIsWrite(QGles2Texture::Access access)
{
    return access == QGles2Texture::AccessStorageWrite
        || access == QGles2Texture::AccessStorageReadWrite
        || access == QGles2Texture::AccessUpdate
        || access == QGles2Texture::AccessFramebuffer;
}

void QRhiGles2::trackedBufferBarrier(QGles2CommandBuffer *cbD, QGles2Buffer *bufD, QGles2Buffer::Access access)
{
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::NoPass); // this is for resource updates only
    const QGles2Buffer::Access prevAccess = bufD->usageState.access;
    if (access == prevAccess)
        return;

    if (bufferAccessIsWrite(prevAccess)) {
        // Generating the minimal barrier set is way too complicated to do
        // correctly (prevAccess is overwritten so we won't have proper
        // tracking across multiple passes) so setting all barrier bits will do
        // for now.
        QGles2CommandBuffer::Command cmd;
        cmd.cmd = QGles2CommandBuffer::Command::Barrier;
        cmd.args.barrier.barriers = GL_ALL_BARRIER_BITS;
        cbD->commands.append(cmd);
    }

    bufD->usageState.access = access;
}

void QRhiGles2::trackedImageBarrier(QGles2CommandBuffer *cbD, QGles2Texture *texD, QGles2Texture::Access access)
{
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::NoPass); // this is for resource updates only
    const QGles2Texture::Access prevAccess = texD->usageState.access;
    if (access == prevAccess)
        return;

    if (textureAccessIsWrite(prevAccess)) {
        QGles2CommandBuffer::Command cmd;
        cmd.cmd = QGles2CommandBuffer::Command::Barrier;
        cmd.args.barrier.barriers = GL_ALL_BARRIER_BITS;
        cbD->commands.append(cmd);
    }

    texD->usageState.access = access;
}

void QRhiGles2::enqueueSubresUpload(QGles2Texture *texD, QGles2CommandBuffer *cbD,
                                    int layer, int level, const QRhiTextureSubresourceUploadDescription &subresDesc)
{
    trackedImageBarrier(cbD, texD, QGles2Texture::AccessUpdate);
    const bool isCompressed = isCompressedFormat(texD->m_format);
    const bool isCubeMap = texD->m_flags.testFlag(QRhiTexture::CubeMap);
    const GLenum faceTargetBase = isCubeMap ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : texD->target;
    const QPoint dp = subresDesc.destinationTopLeft();
    const QByteArray rawData = subresDesc.data();
    if (!subresDesc.image().isNull()) {
        QImage img = subresDesc.image();
        QSize size = img.size();
        QGles2CommandBuffer::Command cmd;
        cmd.cmd = QGles2CommandBuffer::Command::SubImage;
        if (!subresDesc.sourceSize().isEmpty() || !subresDesc.sourceTopLeft().isNull()) {
            const QPoint sp = subresDesc.sourceTopLeft();
            if (!subresDesc.sourceSize().isEmpty())
                size = subresDesc.sourceSize();
            img = img.copy(sp.x(), sp.y(), size.width(), size.height());
        }
        cmd.args.subImage.target = texD->target;
        cmd.args.subImage.texture = texD->texture;
        cmd.args.subImage.faceTarget = faceTargetBase + uint(layer);
        cmd.args.subImage.level = level;
        cmd.args.subImage.dx = dp.x();
        cmd.args.subImage.dy = dp.y();
        cmd.args.subImage.w = size.width();
        cmd.args.subImage.h = size.height();
        cmd.args.subImage.glformat = texD->glformat;
        cmd.args.subImage.gltype = texD->gltype;
        cmd.args.subImage.rowStartAlign = 4;
        cmd.args.subImage.data = cbD->retainImage(img);
        cbD->commands.append(cmd);
    } else if (!rawData.isEmpty() && isCompressed) {
        const QSize size = subresDesc.sourceSize().isEmpty() ? q->sizeForMipLevel(level, texD->m_pixelSize)
                                                             : subresDesc.sourceSize();
        if (texD->specified) {
            QGles2CommandBuffer::Command cmd;
            cmd.cmd = QGles2CommandBuffer::Command::CompressedSubImage;
            cmd.args.compressedSubImage.target = texD->target;
            cmd.args.compressedSubImage.texture = texD->texture;
            cmd.args.compressedSubImage.faceTarget = faceTargetBase + uint(layer);
            cmd.args.compressedSubImage.level = level;
            cmd.args.compressedSubImage.dx = dp.x();
            cmd.args.compressedSubImage.dy = dp.y();
            cmd.args.compressedSubImage.w = size.width();
            cmd.args.compressedSubImage.h = size.height();
            cmd.args.compressedSubImage.glintformat = texD->glintformat;
            cmd.args.compressedSubImage.size = rawData.size();
            cmd.args.compressedSubImage.data = cbD->retainData(rawData);
            cbD->commands.append(cmd);
        } else {
            QGles2CommandBuffer::Command cmd;
            cmd.cmd = QGles2CommandBuffer::Command::CompressedImage;
            cmd.args.compressedImage.target = texD->target;
            cmd.args.compressedImage.texture = texD->texture;
            cmd.args.compressedImage.faceTarget = faceTargetBase + uint(layer);
            cmd.args.compressedImage.level = level;
            cmd.args.compressedImage.glintformat = texD->glintformat;
            cmd.args.compressedImage.w = size.width();
            cmd.args.compressedImage.h = size.height();
            cmd.args.compressedImage.size = rawData.size();
            cmd.args.compressedImage.data = cbD->retainData(rawData);
            cbD->commands.append(cmd);
        }
    } else if (!rawData.isEmpty()) {
        const QSize size = subresDesc.sourceSize().isEmpty() ? q->sizeForMipLevel(level, texD->m_pixelSize)
                                                             : subresDesc.sourceSize();
        quint32 bpl = 0;
        textureFormatInfo(texD->m_format, size, &bpl, nullptr);
        QGles2CommandBuffer::Command cmd;
        cmd.cmd = QGles2CommandBuffer::Command::SubImage;
        cmd.args.subImage.target = texD->target;
        cmd.args.subImage.texture = texD->texture;
        cmd.args.subImage.faceTarget = faceTargetBase + uint(layer);
        cmd.args.subImage.level = level;
        cmd.args.subImage.dx = dp.x();
        cmd.args.subImage.dy = dp.y();
        cmd.args.subImage.w = size.width();
        cmd.args.subImage.h = size.height();
        cmd.args.subImage.glformat = texD->glformat;
        cmd.args.subImage.gltype = texD->gltype;
        // Default unpack alignment (row start aligment
        // requirement) is 4. QImage guarantees 4 byte aligned
        // row starts, but our raw data here does not.
        cmd.args.subImage.rowStartAlign = (bpl & 3) ? 1 : 4;
        cmd.args.subImage.data = cbD->retainData(rawData);
        cbD->commands.append(cmd);
    } else {
        qWarning("Invalid texture upload for %p layer=%d mip=%d", texD, layer, level);
    }
}

void QRhiGles2::enqueueResourceUpdates(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    QRhiResourceUpdateBatchPrivate *ud = QRhiResourceUpdateBatchPrivate::get(resourceUpdates);

    for (const QRhiResourceUpdateBatchPrivate::BufferOp &u : ud->bufferOps) {
        if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::DynamicUpdate) {
            QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, u.buf);
            Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
            if (bufD->m_usage.testFlag(QRhiBuffer::UniformBuffer)) {
                memcpy(bufD->ubuf.data() + u.offset, u.data.constData(), size_t(u.data.size()));
            } else {
                trackedBufferBarrier(cbD, bufD, QGles2Buffer::AccessUpdate);
                QGles2CommandBuffer::Command cmd;
                cmd.cmd = QGles2CommandBuffer::Command::BufferSubData;
                cmd.args.bufferSubData.target = bufD->targetForDataOps;
                cmd.args.bufferSubData.buffer = bufD->buffer;
                cmd.args.bufferSubData.offset = u.offset;
                cmd.args.bufferSubData.size = u.data.size();
                cmd.args.bufferSubData.data = cbD->retainData(u.data);
                cbD->commands.append(cmd);
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::StaticUpload) {
            QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, u.buf);
            Q_ASSERT(bufD->m_type != QRhiBuffer::Dynamic);
            Q_ASSERT(u.offset + u.data.size() <= bufD->m_size);
            if (bufD->m_usage.testFlag(QRhiBuffer::UniformBuffer)) {
                memcpy(bufD->ubuf.data() + u.offset, u.data.constData(), size_t(u.data.size()));
            } else {
                trackedBufferBarrier(cbD, bufD, QGles2Buffer::AccessUpdate);
                QGles2CommandBuffer::Command cmd;
                cmd.cmd = QGles2CommandBuffer::Command::BufferSubData;
                cmd.args.bufferSubData.target = bufD->targetForDataOps;
                cmd.args.bufferSubData.buffer = bufD->buffer;
                cmd.args.bufferSubData.offset = u.offset;
                cmd.args.bufferSubData.size = u.data.size();
                cmd.args.bufferSubData.data = cbD->retainData(u.data);
                cbD->commands.append(cmd);
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::Read) {
            QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, u.buf);
            if (bufD->m_usage.testFlag(QRhiBuffer::UniformBuffer)) {
                u.result->data.resize(u.readSize);
                memcpy(u.result->data.data(), bufD->ubuf.constData() + u.offset, size_t(u.readSize));
                if (u.result->completed)
                    u.result->completed();
            } else {
                QGles2CommandBuffer::Command cmd;
                cmd.cmd = QGles2CommandBuffer::Command::GetBufferSubData;
                cmd.args.getBufferSubData.result = u.result;
                cmd.args.getBufferSubData.target = bufD->targetForDataOps;
                cmd.args.getBufferSubData.buffer = bufD->buffer;
                cmd.args.getBufferSubData.offset = u.offset;
                cmd.args.getBufferSubData.size = u.readSize;
                cbD->commands.append(cmd);
            }
        }
    }

    for (const QRhiResourceUpdateBatchPrivate::TextureOp &u : ud->textureOps) {
        if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Upload) {
            QGles2Texture *texD = QRHI_RES(QGles2Texture, u.dst);
            for (int layer = 0; layer < QRhi::MAX_LAYERS; ++layer) {
                for (int level = 0; level < QRhi::MAX_LEVELS; ++level) {
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : qAsConst(u.subresDesc[layer][level]))
                        enqueueSubresUpload(texD, cbD, layer, level, subresDesc);
                }
            }
            texD->specified = true;
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Copy) {
            Q_ASSERT(u.src && u.dst);
            QGles2Texture *srcD = QRHI_RES(QGles2Texture, u.src);
            QGles2Texture *dstD = QRHI_RES(QGles2Texture, u.dst);

            trackedImageBarrier(cbD, srcD, QGles2Texture::AccessRead);
            trackedImageBarrier(cbD, dstD, QGles2Texture::AccessUpdate);

            const QSize mipSize = q->sizeForMipLevel(u.desc.sourceLevel(), srcD->m_pixelSize);
            const QSize copySize = u.desc.pixelSize().isEmpty() ? mipSize : u.desc.pixelSize();
            // do not translate coordinates, even if sp is bottom-left from gl's pov
            const QPoint sp = u.desc.sourceTopLeft();
            const QPoint dp = u.desc.destinationTopLeft();

            const GLenum srcFaceTargetBase = srcD->m_flags.testFlag(QRhiTexture::CubeMap)
                    ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : srcD->target;
            const GLenum dstFaceTargetBase = dstD->m_flags.testFlag(QRhiTexture::CubeMap)
                    ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : dstD->target;

            QGles2CommandBuffer::Command cmd;
            cmd.cmd = QGles2CommandBuffer::Command::CopyTex;

            cmd.args.copyTex.srcFaceTarget = srcFaceTargetBase + uint(u.desc.sourceLayer());
            cmd.args.copyTex.srcTexture = srcD->texture;
            cmd.args.copyTex.srcLevel = u.desc.sourceLevel();
            cmd.args.copyTex.srcX = sp.x();
            cmd.args.copyTex.srcY = sp.y();

            cmd.args.copyTex.dstTarget = dstD->target;
            cmd.args.copyTex.dstTexture = dstD->texture;
            cmd.args.copyTex.dstFaceTarget = dstFaceTargetBase + uint(u.desc.destinationLayer());
            cmd.args.copyTex.dstLevel = u.desc.destinationLevel();
            cmd.args.copyTex.dstX = dp.x();
            cmd.args.copyTex.dstY = dp.y();

            cmd.args.copyTex.w = copySize.width();
            cmd.args.copyTex.h = copySize.height();

            cbD->commands.append(cmd);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Read) {
            QGles2CommandBuffer::Command cmd;
            cmd.cmd = QGles2CommandBuffer::Command::ReadPixels;
            cmd.args.readPixels.result = u.result;
            QGles2Texture *texD = QRHI_RES(QGles2Texture, u.rb.texture());
            if (texD)
                trackedImageBarrier(cbD, texD, QGles2Texture::AccessRead);
            cmd.args.readPixels.texture = texD ? texD->texture : 0;
            if (texD) {
                const QSize readImageSize = q->sizeForMipLevel(u.rb.level(), texD->m_pixelSize);
                cmd.args.readPixels.w = readImageSize.width();
                cmd.args.readPixels.h = readImageSize.height();
                cmd.args.readPixels.format = texD->m_format;
                const GLenum faceTargetBase = texD->m_flags.testFlag(QRhiTexture::CubeMap)
                        ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : texD->target;
                cmd.args.readPixels.readTarget = faceTargetBase + uint(u.rb.layer());
                cmd.args.readPixels.level = u.rb.level();
            }
            cbD->commands.append(cmd);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::GenMips) {
            QGles2Texture *texD = QRHI_RES(QGles2Texture, u.dst);
            trackedImageBarrier(cbD, texD, QGles2Texture::AccessFramebuffer);
            QGles2CommandBuffer::Command cmd;
            cmd.cmd = QGles2CommandBuffer::Command::GenMip;
            cmd.args.genMip.target = texD->target;
            cmd.args.genMip.texture = texD->texture;
            cbD->commands.append(cmd);
        }
    }

    ud->free();
}

static inline GLenum toGlTopology(QRhiGraphicsPipeline::Topology t)
{
    switch (t) {
    case QRhiGraphicsPipeline::Triangles:
        return GL_TRIANGLES;
    case QRhiGraphicsPipeline::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    case QRhiGraphicsPipeline::TriangleFan:
        return GL_TRIANGLE_FAN;
    case QRhiGraphicsPipeline::Lines:
        return GL_LINES;
    case QRhiGraphicsPipeline::LineStrip:
        return GL_LINE_STRIP;
    case QRhiGraphicsPipeline::Points:
        return GL_POINTS;
    default:
        Q_UNREACHABLE();
        return GL_TRIANGLES;
    }
}

static inline GLenum toGlCullMode(QRhiGraphicsPipeline::CullMode c)
{
    switch (c) {
    case QRhiGraphicsPipeline::Front:
        return GL_FRONT;
    case QRhiGraphicsPipeline::Back:
        return GL_BACK;
    default:
        Q_UNREACHABLE();
        return GL_BACK;
    }
}

static inline GLenum toGlFrontFace(QRhiGraphicsPipeline::FrontFace f)
{
    switch (f) {
    case QRhiGraphicsPipeline::CCW:
        return GL_CCW;
    case QRhiGraphicsPipeline::CW:
        return GL_CW;
    default:
        Q_UNREACHABLE();
        return GL_CCW;
    }
}

static inline GLenum toGlBlendFactor(QRhiGraphicsPipeline::BlendFactor f)
{
    switch (f) {
    case QRhiGraphicsPipeline::Zero:
        return GL_ZERO;
    case QRhiGraphicsPipeline::One:
        return GL_ONE;
    case QRhiGraphicsPipeline::SrcColor:
        return GL_SRC_COLOR;
    case QRhiGraphicsPipeline::OneMinusSrcColor:
        return GL_ONE_MINUS_SRC_COLOR;
    case QRhiGraphicsPipeline::DstColor:
        return GL_DST_COLOR;
    case QRhiGraphicsPipeline::OneMinusDstColor:
        return GL_ONE_MINUS_DST_COLOR;
    case QRhiGraphicsPipeline::SrcAlpha:
        return GL_SRC_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    case QRhiGraphicsPipeline::DstAlpha:
        return GL_DST_ALPHA;
    case QRhiGraphicsPipeline::OneMinusDstAlpha:
        return GL_ONE_MINUS_DST_ALPHA;
    case QRhiGraphicsPipeline::ConstantColor:
        return GL_CONSTANT_COLOR;
    case QRhiGraphicsPipeline::OneMinusConstantColor:
        return GL_ONE_MINUS_CONSTANT_COLOR;
    case QRhiGraphicsPipeline::ConstantAlpha:
        return GL_CONSTANT_ALPHA;
    case QRhiGraphicsPipeline::OneMinusConstantAlpha:
        return GL_ONE_MINUS_CONSTANT_ALPHA;
    case QRhiGraphicsPipeline::SrcAlphaSaturate:
        return GL_SRC_ALPHA_SATURATE;
    case QRhiGraphicsPipeline::Src1Color:
    case QRhiGraphicsPipeline::OneMinusSrc1Color:
    case QRhiGraphicsPipeline::Src1Alpha:
    case QRhiGraphicsPipeline::OneMinusSrc1Alpha:
        qWarning("Unsupported blend factor %d", f);
        return GL_ZERO;
    default:
        Q_UNREACHABLE();
        return GL_ZERO;
    }
}

static inline GLenum toGlBlendOp(QRhiGraphicsPipeline::BlendOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::Add:
        return GL_FUNC_ADD;
    case QRhiGraphicsPipeline::Subtract:
        return GL_FUNC_SUBTRACT;
    case QRhiGraphicsPipeline::ReverseSubtract:
        return GL_FUNC_REVERSE_SUBTRACT;
    case QRhiGraphicsPipeline::Min:
        return GL_MIN;
    case QRhiGraphicsPipeline::Max:
        return GL_MAX;
    default:
        Q_UNREACHABLE();
        return GL_FUNC_ADD;
    }
}

static inline GLenum toGlCompareOp(QRhiGraphicsPipeline::CompareOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::Never:
        return GL_NEVER;
    case QRhiGraphicsPipeline::Less:
        return GL_LESS;
    case QRhiGraphicsPipeline::Equal:
        return GL_EQUAL;
    case QRhiGraphicsPipeline::LessOrEqual:
        return GL_LEQUAL;
    case QRhiGraphicsPipeline::Greater:
        return GL_GREATER;
    case QRhiGraphicsPipeline::NotEqual:
        return GL_NOTEQUAL;
    case QRhiGraphicsPipeline::GreaterOrEqual:
        return GL_GEQUAL;
    case QRhiGraphicsPipeline::Always:
        return GL_ALWAYS;
    default:
        Q_UNREACHABLE();
        return GL_ALWAYS;
    }
}

static inline GLenum toGlStencilOp(QRhiGraphicsPipeline::StencilOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::StencilZero:
        return GL_ZERO;
    case QRhiGraphicsPipeline::Keep:
        return GL_KEEP;
    case QRhiGraphicsPipeline::Replace:
        return GL_REPLACE;
    case QRhiGraphicsPipeline::IncrementAndClamp:
        return GL_INCR;
    case QRhiGraphicsPipeline::DecrementAndClamp:
        return GL_DECR;
    case QRhiGraphicsPipeline::Invert:
        return GL_INVERT;
    case QRhiGraphicsPipeline::IncrementAndWrap:
        return GL_INCR_WRAP;
    case QRhiGraphicsPipeline::DecrementAndWrap:
        return GL_DECR_WRAP;
    default:
        Q_UNREACHABLE();
        return GL_KEEP;
    }
}

static inline GLenum toGlMinFilter(QRhiSampler::Filter f, QRhiSampler::Filter m)
{
    switch (f) {
    case QRhiSampler::Nearest:
        if (m == QRhiSampler::None)
            return GL_NEAREST;
        else
            return m == QRhiSampler::Nearest ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR;
    case QRhiSampler::Linear:
        if (m == QRhiSampler::None)
            return GL_LINEAR;
        else
            return m == QRhiSampler::Nearest ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
    default:
        Q_UNREACHABLE();
        return GL_LINEAR;
    }
}

static inline GLenum toGlMagFilter(QRhiSampler::Filter f)
{
    switch (f) {
    case QRhiSampler::Nearest:
        return GL_NEAREST;
    case QRhiSampler::Linear:
        return GL_LINEAR;
    default:
        Q_UNREACHABLE();
        return GL_LINEAR;
    }
}

static inline GLenum toGlWrapMode(QRhiSampler::AddressMode m)
{
    switch (m) {
    case QRhiSampler::Repeat:
        return GL_REPEAT;
    case QRhiSampler::ClampToEdge:
        return GL_CLAMP_TO_EDGE;
    case QRhiSampler::Mirror:
        return GL_MIRRORED_REPEAT;
    default:
        Q_UNREACHABLE();
        return GL_CLAMP_TO_EDGE;
    }
}

static inline GLenum toGlTextureCompareFunc(QRhiSampler::CompareOp op)
{
    switch (op) {
    case QRhiSampler::Never:
        return GL_NEVER;
    case QRhiSampler::Less:
        return GL_LESS;
    case QRhiSampler::Equal:
        return GL_EQUAL;
    case QRhiSampler::LessOrEqual:
        return GL_LEQUAL;
    case QRhiSampler::Greater:
        return GL_GREATER;
    case QRhiSampler::NotEqual:
        return GL_NOTEQUAL;
    case QRhiSampler::GreaterOrEqual:
        return GL_GEQUAL;
    case QRhiSampler::Always:
        return GL_ALWAYS;
    default:
        Q_UNREACHABLE();
        return GL_NEVER;
    }
}

static inline QGles2Buffer::Access toGlAccess(QRhiPassResourceTracker::BufferAccess access)
{
    switch (access) {
    case QRhiPassResourceTracker::BufVertexInput:
        return QGles2Buffer::AccessVertex;
    case QRhiPassResourceTracker::BufIndexRead:
        return QGles2Buffer::AccessIndex;
    case QRhiPassResourceTracker::BufUniformRead:
        return QGles2Buffer::AccessUniform;
    case QRhiPassResourceTracker::BufStorageLoad:
        return QGles2Buffer::AccessStorageRead;
    case QRhiPassResourceTracker::BufStorageStore:
        return QGles2Buffer::AccessStorageWrite;
    case QRhiPassResourceTracker::BufStorageLoadStore:
        return QGles2Buffer::AccessStorageReadWrite;
    default:
        Q_UNREACHABLE();
        break;
    }
    return QGles2Buffer::AccessNone;
}

static inline QRhiPassResourceTracker::UsageState toPassTrackerUsageState(const QGles2Buffer::UsageState &bufUsage)
{
    QRhiPassResourceTracker::UsageState u;
    u.layout = 0; // N/A
    u.access = bufUsage.access;
    u.stage = 0; // N/A
    return u;
}

static inline QGles2Texture::Access toGlAccess(QRhiPassResourceTracker::TextureAccess access)
{
    switch (access) {
    case QRhiPassResourceTracker::TexSample:
        return QGles2Texture::AccessSample;
    case QRhiPassResourceTracker::TexColorOutput:
        return QGles2Texture::AccessFramebuffer;
    case QRhiPassResourceTracker::TexDepthOutput:
        return QGles2Texture::AccessFramebuffer;
    case QRhiPassResourceTracker::TexStorageLoad:
        return QGles2Texture::AccessStorageRead;
    case QRhiPassResourceTracker::TexStorageStore:
        return QGles2Texture::AccessStorageWrite;
    case QRhiPassResourceTracker::TexStorageLoadStore:
        return QGles2Texture::AccessStorageReadWrite;
    default:
        Q_UNREACHABLE();
        break;
    }
    return QGles2Texture::AccessNone;
}

static inline QRhiPassResourceTracker::UsageState toPassTrackerUsageState(const QGles2Texture::UsageState &texUsage)
{
    QRhiPassResourceTracker::UsageState u;
    u.layout = 0; // N/A
    u.access = texUsage.access;
    u.stage = 0; // N/A
    return u;
}

void QRhiGles2::trackedRegisterBuffer(QRhiPassResourceTracker *passResTracker,
                                      QGles2Buffer *bufD,
                                      QRhiPassResourceTracker::BufferAccess access,
                                      QRhiPassResourceTracker::BufferStage stage)
{
    QGles2Buffer::UsageState &u(bufD->usageState);
    passResTracker->registerBuffer(bufD, 0, &access, &stage, toPassTrackerUsageState(u));
    u.access = toGlAccess(access);
}

void QRhiGles2::trackedRegisterTexture(QRhiPassResourceTracker *passResTracker,
                                       QGles2Texture *texD,
                                       QRhiPassResourceTracker::TextureAccess access,
                                       QRhiPassResourceTracker::TextureStage stage)
{
    QGles2Texture::UsageState &u(texD->usageState);
    passResTracker->registerTexture(texD, &access, &stage, toPassTrackerUsageState(u));
    u.access = toGlAccess(access);
}

void QRhiGles2::executeCommandBuffer(QRhiCommandBuffer *cb)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    GLenum indexType = GL_UNSIGNED_SHORT;
    quint32 indexStride = sizeof(quint16);
    quint32 indexOffset = 0;
    GLuint currentArrayBuffer = 0;
    static const int TRACKED_ATTRIB_COUNT = 16;
    bool enabledAttribArrays[TRACKED_ATTRIB_COUNT];
    memset(enabledAttribArrays, 0, sizeof(enabledAttribArrays));

    for (const QGles2CommandBuffer::Command &cmd : qAsConst(cbD->commands)) {
        switch (cmd.cmd) {
        case QGles2CommandBuffer::Command::BeginFrame:
            if (caps.coreProfile) {
                if (!vao)
                    f->glGenVertexArrays(1, &vao);
                f->glBindVertexArray(vao);
            }
            break;
        case QGles2CommandBuffer::Command::EndFrame:
            if (vao)
                f->glBindVertexArray(0);
            break;
        case QGles2CommandBuffer::Command::ResetFrame:
            if (vao)
                f->glBindVertexArray(vao);
            break;
        case QGles2CommandBuffer::Command::Viewport:
            f->glViewport(GLint(cmd.args.viewport.x), GLint(cmd.args.viewport.y), GLsizei(cmd.args.viewport.w), GLsizei(cmd.args.viewport.h));
            f->glDepthRangef(cmd.args.viewport.d0, cmd.args.viewport.d1);
            break;
        case QGles2CommandBuffer::Command::Scissor:
            f->glScissor(cmd.args.scissor.x, cmd.args.scissor.y, cmd.args.scissor.w, cmd.args.scissor.h);
            break;
        case QGles2CommandBuffer::Command::BlendConstants:
            f->glBlendColor(cmd.args.blendConstants.r, cmd.args.blendConstants.g, cmd.args.blendConstants.b, cmd.args.blendConstants.a);
            break;
        case QGles2CommandBuffer::Command::StencilRef:
        {
            QGles2GraphicsPipeline *psD = QRHI_RES(QGles2GraphicsPipeline, cmd.args.stencilRef.ps);
            if (psD) {
                const GLint ref = GLint(cmd.args.stencilRef.ref);
                f->glStencilFuncSeparate(GL_FRONT, toGlCompareOp(psD->m_stencilFront.compareOp), ref, psD->m_stencilReadMask);
                f->glStencilFuncSeparate(GL_BACK, toGlCompareOp(psD->m_stencilBack.compareOp), ref, psD->m_stencilReadMask);
                cbD->graphicsPassState.dynamic.stencilRef = ref;
            } else {
                qWarning("No graphics pipeline active for setStencilRef; ignored");
            }
        }
            break;
        case QGles2CommandBuffer::Command::BindVertexBuffer:
        {
            QGles2GraphicsPipeline *psD = QRHI_RES(QGles2GraphicsPipeline, cmd.args.bindVertexBuffer.ps);
            if (psD) {
                for (auto it = psD->m_vertexInputLayout.cbeginAttributes(), itEnd = psD->m_vertexInputLayout.cendAttributes();
                     it != itEnd; ++it)
                {
                    const int bindingIdx = it->binding();
                    if (bindingIdx != cmd.args.bindVertexBuffer.binding)
                        continue;

                    if (cmd.args.bindVertexBuffer.buffer != currentArrayBuffer) {
                        currentArrayBuffer = cmd.args.bindVertexBuffer.buffer;
                        // we do not support more than one vertex buffer
                        f->glBindBuffer(GL_ARRAY_BUFFER, currentArrayBuffer);
                    }

                    const QRhiVertexInputBinding *inputBinding = psD->m_vertexInputLayout.bindingAt(bindingIdx);
                    const int stride = int(inputBinding->stride());
                    int size = 1;
                    GLenum type = GL_FLOAT;
                    bool normalize = false;
                    switch (it->format()) {
                    case QRhiVertexInputAttribute::Float4:
                        type = GL_FLOAT;
                        size = 4;
                        break;
                    case QRhiVertexInputAttribute::Float3:
                        type = GL_FLOAT;
                        size = 3;
                        break;
                    case QRhiVertexInputAttribute::Float2:
                        type = GL_FLOAT;
                        size = 2;
                        break;
                    case QRhiVertexInputAttribute::Float:
                        type = GL_FLOAT;
                        size = 1;
                        break;
                    case QRhiVertexInputAttribute::UNormByte4:
                        type = GL_UNSIGNED_BYTE;
                        normalize = true;
                        size = 4;
                        break;
                    case QRhiVertexInputAttribute::UNormByte2:
                        type = GL_UNSIGNED_BYTE;
                        normalize = true;
                        size = 2;
                        break;
                    case QRhiVertexInputAttribute::UNormByte:
                        type = GL_UNSIGNED_BYTE;
                        normalize = true;
                        size = 1;
                        break;
                    default:
                        break;
                    }

                    const int locationIdx = it->location();
                    quint32 ofs = it->offset() + cmd.args.bindVertexBuffer.offset;
                    f->glVertexAttribPointer(GLuint(locationIdx), size, type, normalize, stride,
                                             reinterpret_cast<const GLvoid *>(quintptr(ofs)));
                    if (locationIdx >= TRACKED_ATTRIB_COUNT || !enabledAttribArrays[locationIdx]) {
                        if (locationIdx < TRACKED_ATTRIB_COUNT)
                            enabledAttribArrays[locationIdx] = true;
                        f->glEnableVertexAttribArray(GLuint(locationIdx));
                    }
                    if (inputBinding->classification() == QRhiVertexInputBinding::PerInstance && caps.instancing)
                        f->glVertexAttribDivisor(GLuint(locationIdx), GLuint(inputBinding->instanceStepRate()));
                }
            } else {
                qWarning("No graphics pipeline active for setVertexInput; ignored");
            }
        }
            break;
        case QGles2CommandBuffer::Command::BindIndexBuffer:
            indexType = cmd.args.bindIndexBuffer.type;
            indexStride = indexType == GL_UNSIGNED_SHORT ? sizeof(quint16) : sizeof(quint32);
            indexOffset = cmd.args.bindIndexBuffer.offset;
            f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cmd.args.bindIndexBuffer.buffer);
            break;
        case QGles2CommandBuffer::Command::Draw:
        {
            QGles2GraphicsPipeline *psD = QRHI_RES(QGles2GraphicsPipeline, cmd.args.draw.ps);
            if (psD) {
                if (cmd.args.draw.instanceCount == 1 || !caps.instancing) {
                    f->glDrawArrays(psD->drawMode, GLint(cmd.args.draw.firstVertex), GLsizei(cmd.args.draw.vertexCount));
                } else {
                    f->glDrawArraysInstanced(psD->drawMode, GLint(cmd.args.draw.firstVertex), GLsizei(cmd.args.draw.vertexCount),
                                             GLsizei(cmd.args.draw.instanceCount));
                }
            } else {
                qWarning("No graphics pipeline active for draw; ignored");
            }
        }
            break;
        case QGles2CommandBuffer::Command::DrawIndexed:
        {
            QGles2GraphicsPipeline *psD = QRHI_RES(QGles2GraphicsPipeline, cmd.args.drawIndexed.ps);
            if (psD) {
                const GLvoid *ofs = reinterpret_cast<const GLvoid *>(
                            quintptr(cmd.args.drawIndexed.firstIndex * indexStride + indexOffset));
                if (cmd.args.drawIndexed.instanceCount == 1 || !caps.instancing) {
                    if (cmd.args.drawIndexed.baseVertex != 0 && caps.baseVertex) {
                        f->glDrawElementsBaseVertex(psD->drawMode,
                                                    GLsizei(cmd.args.drawIndexed.indexCount),
                                                    indexType,
                                                    ofs,
                                                    cmd.args.drawIndexed.baseVertex);
                    } else {
                        f->glDrawElements(psD->drawMode,
                                          GLsizei(cmd.args.drawIndexed.indexCount),
                                          indexType,
                                          ofs);
                    }
                } else {
                    if (cmd.args.drawIndexed.baseVertex != 0 && caps.baseVertex) {
                        f->glDrawElementsInstancedBaseVertex(psD->drawMode,
                                                             GLsizei(cmd.args.drawIndexed.indexCount),
                                                             indexType,
                                                             ofs,
                                                             GLsizei(cmd.args.drawIndexed.instanceCount),
                                                             cmd.args.drawIndexed.baseVertex);
                    } else {
                        f->glDrawElementsInstanced(psD->drawMode,
                                                   GLsizei(cmd.args.drawIndexed.indexCount),
                                                   indexType,
                                                   ofs,
                                                   GLsizei(cmd.args.drawIndexed.instanceCount));
                    }
                }
            } else {
                qWarning("No graphics pipeline active for drawIndexed; ignored");
            }
        }
            break;
        case QGles2CommandBuffer::Command::BindGraphicsPipeline:
            executeBindGraphicsPipeline(cbD, QRHI_RES(QGles2GraphicsPipeline, cmd.args.bindGraphicsPipeline.ps));
            break;
        case QGles2CommandBuffer::Command::BindShaderResources:
            bindShaderResources(cmd.args.bindShaderResources.maybeGraphicsPs,
                                cmd.args.bindShaderResources.maybeComputePs,
                                cmd.args.bindShaderResources.srb,
                                cmd.args.bindShaderResources.dynamicOffsetPairs,
                                cmd.args.bindShaderResources.dynamicOffsetCount);
            break;
        case QGles2CommandBuffer::Command::BindFramebuffer:
            if (cmd.args.bindFramebuffer.fbo) {
                f->glBindFramebuffer(GL_FRAMEBUFFER, cmd.args.bindFramebuffer.fbo);
                if (caps.maxDrawBuffers > 1) {
                    const int colorAttCount = cmd.args.bindFramebuffer.colorAttCount;
                    QVarLengthArray<GLenum, 8> bufs;
                    for (int i = 0; i < colorAttCount; ++i)
                        bufs.append(GL_COLOR_ATTACHMENT0 + uint(i));
                    f->glDrawBuffers(colorAttCount, bufs.constData());
                }
            } else {
                f->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
                if (caps.maxDrawBuffers > 1) {
                    GLenum bufs = GL_BACK;
                    f->glDrawBuffers(1, &bufs);
                }
            }
            if (caps.srgbCapableDefaultFramebuffer) {
                if (cmd.args.bindFramebuffer.srgb)
                    f->glEnable(GL_FRAMEBUFFER_SRGB);
                else
                    f->glDisable(GL_FRAMEBUFFER_SRGB);
            }
            break;
        case QGles2CommandBuffer::Command::Clear:
            f->glDisable(GL_SCISSOR_TEST);
            if (cmd.args.clear.mask & GL_COLOR_BUFFER_BIT) {
                f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                f->glClearColor(cmd.args.clear.c[0], cmd.args.clear.c[1], cmd.args.clear.c[2], cmd.args.clear.c[3]);
            }
            if (cmd.args.clear.mask & GL_DEPTH_BUFFER_BIT) {
                f->glDepthMask(GL_TRUE);
                f->glClearDepthf(cmd.args.clear.d);
            }
            if (cmd.args.clear.mask & GL_STENCIL_BUFFER_BIT)
                f->glClearStencil(GLint(cmd.args.clear.s));
            f->glClear(cmd.args.clear.mask);
            cbD->graphicsPassState.reset(); // altered depth/color write, invalidate in order to avoid confusing the state tracking
            break;
        case QGles2CommandBuffer::Command::BufferSubData:
            f->glBindBuffer(cmd.args.bufferSubData.target, cmd.args.bufferSubData.buffer);
            f->glBufferSubData(cmd.args.bufferSubData.target, cmd.args.bufferSubData.offset, cmd.args.bufferSubData.size,
                               cmd.args.bufferSubData.data);
            break;
        case QGles2CommandBuffer::Command::GetBufferSubData:
        {
            QRhiBufferReadbackResult *result = cmd.args.getBufferSubData.result;
            f->glBindBuffer(cmd.args.getBufferSubData.target, cmd.args.getBufferSubData.buffer);
            if (caps.gles) {
                if (caps.properMapBuffer) {
                    void *p = f->glMapBufferRange(cmd.args.getBufferSubData.target,
                                                  cmd.args.getBufferSubData.offset,
                                                  cmd.args.getBufferSubData.size,
                                                  GL_MAP_READ_BIT);
                    if (p) {
                        result->data.resize(cmd.args.getBufferSubData.size);
                        memcpy(result->data.data(), p, size_t(cmd.args.getBufferSubData.size));
                        f->glUnmapBuffer(cmd.args.getBufferSubData.target);
                    }
                }
            } else {
                result->data.resize(cmd.args.getBufferSubData.size);
                f->glGetBufferSubData(cmd.args.getBufferSubData.target,
                                      cmd.args.getBufferSubData.offset,
                                      cmd.args.getBufferSubData.size,
                                      result->data.data());
            }
            if (result->completed)
                result->completed();
        }
            break;
        case QGles2CommandBuffer::Command::CopyTex:
        {
            GLuint fbo;
            f->glGenFramebuffers(1, &fbo);
            f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                      cmd.args.copyTex.srcFaceTarget, cmd.args.copyTex.srcTexture, cmd.args.copyTex.srcLevel);
            f->glBindTexture(cmd.args.copyTex.dstTarget, cmd.args.copyTex.dstTexture);
            f->glCopyTexSubImage2D(cmd.args.copyTex.dstFaceTarget, cmd.args.copyTex.dstLevel,
                                   cmd.args.copyTex.dstX, cmd.args.copyTex.dstY,
                                   cmd.args.copyTex.srcX, cmd.args.copyTex.srcY,
                                   cmd.args.copyTex.w, cmd.args.copyTex.h);
            f->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
            f->glDeleteFramebuffers(1, &fbo);
        }
            break;
        case QGles2CommandBuffer::Command::ReadPixels:
        {
            QRhiReadbackResult *result = cmd.args.readPixels.result;
            GLuint tex = cmd.args.readPixels.texture;
            GLuint fbo = 0;
            int mipLevel = 0;
            if (tex) {
                result->pixelSize = QSize(cmd.args.readPixels.w, cmd.args.readPixels.h);
                result->format = cmd.args.readPixels.format;
                mipLevel = cmd.args.readPixels.level;
                if (mipLevel == 0 || caps.nonBaseLevelFramebufferTexture) {
                    f->glGenFramebuffers(1, &fbo);
                    f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                    f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                              cmd.args.readPixels.readTarget, cmd.args.readPixels.texture, mipLevel);
                }
            } else {
                result->pixelSize = currentSwapChain->pixelSize;
                result->format = QRhiTexture::RGBA8;
                // readPixels handles multisample resolving implicitly
            }
            result->data.resize(result->pixelSize.width() * result->pixelSize.height() * 4);
            if (mipLevel == 0 || caps.nonBaseLevelFramebufferTexture) {
                // With GLES (2.0?) GL_RGBA is the only mandated readback format, so stick with it.
                f->glReadPixels(0, 0, result->pixelSize.width(), result->pixelSize.height(),
                                GL_RGBA, GL_UNSIGNED_BYTE,
                                result->data.data());
            } else {
                result->data.fill('\0');
            }
            if (fbo) {
                f->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
                f->glDeleteFramebuffers(1, &fbo);
            }
            if (result->completed)
                result->completed();
        }
            break;
        case QGles2CommandBuffer::Command::SubImage:
            f->glBindTexture(cmd.args.subImage.target, cmd.args.subImage.texture);
            if (cmd.args.subImage.rowStartAlign != 4)
                f->glPixelStorei(GL_UNPACK_ALIGNMENT, cmd.args.subImage.rowStartAlign);
            f->glTexSubImage2D(cmd.args.subImage.faceTarget, cmd.args.subImage.level,
                               cmd.args.subImage.dx, cmd.args.subImage.dy,
                               cmd.args.subImage.w, cmd.args.subImage.h,
                               cmd.args.subImage.glformat, cmd.args.subImage.gltype,
                               cmd.args.subImage.data);
            if (cmd.args.subImage.rowStartAlign != 4)
                f->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            break;
        case QGles2CommandBuffer::Command::CompressedImage:
            f->glBindTexture(cmd.args.compressedImage.target, cmd.args.compressedImage.texture);
            f->glCompressedTexImage2D(cmd.args.compressedImage.faceTarget, cmd.args.compressedImage.level,
                                      cmd.args.compressedImage.glintformat,
                                      cmd.args.compressedImage.w, cmd.args.compressedImage.h, 0,
                                      cmd.args.compressedImage.size, cmd.args.compressedImage.data);
            break;
        case QGles2CommandBuffer::Command::CompressedSubImage:
            f->glBindTexture(cmd.args.compressedSubImage.target, cmd.args.compressedSubImage.texture);
            f->glCompressedTexSubImage2D(cmd.args.compressedSubImage.faceTarget, cmd.args.compressedSubImage.level,
                                         cmd.args.compressedSubImage.dx, cmd.args.compressedSubImage.dy,
                                         cmd.args.compressedSubImage.w, cmd.args.compressedSubImage.h,
                                         cmd.args.compressedSubImage.glintformat,
                                         cmd.args.compressedSubImage.size, cmd.args.compressedSubImage.data);
            break;
        case QGles2CommandBuffer::Command::BlitFromRenderbuffer:
        {
            GLuint fbo[2];
            f->glGenFramebuffers(2, fbo);
            f->glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo[0]);
            f->glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                         GL_RENDERBUFFER, cmd.args.blitFromRb.renderbuffer);
            f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo[1]);

            f->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cmd.args.blitFromRb.target,
                                      cmd.args.blitFromRb.texture, cmd.args.blitFromRb.dstLevel);
            f->glBlitFramebuffer(0, 0, cmd.args.blitFromRb.w, cmd.args.blitFromRb.h,
                                 0, 0, cmd.args.blitFromRb.w, cmd.args.blitFromRb.h,
                                 GL_COLOR_BUFFER_BIT,
                                 GL_LINEAR);
            f->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
        }
            break;
        case QGles2CommandBuffer::Command::GenMip:
            f->glBindTexture(cmd.args.genMip.target, cmd.args.genMip.texture);
            f->glGenerateMipmap(cmd.args.genMip.target);
            break;
        case QGles2CommandBuffer::Command::BindComputePipeline:
        {
            QGles2ComputePipeline *psD = QRHI_RES(QGles2ComputePipeline, cmd.args.bindComputePipeline.ps);
            f->glUseProgram(psD->program);
        }
            break;
        case QGles2CommandBuffer::Command::Dispatch:
            f->glDispatchCompute(cmd.args.dispatch.x, cmd.args.dispatch.y, cmd.args.dispatch.z);
            break;
        case QGles2CommandBuffer::Command::BarriersForPass:
        {
            GLbitfield barriers = 0;
            QRhiPassResourceTracker &tracker(cbD->passResTrackers[cmd.args.barriersForPass.trackerIndex]);
            // we only care about after-write, not any other accesses, and
            // cannot tell if something was written in a shader several passes
            // ago: now the previously written resource may be used with an
            // access that was not in the previous passes, result in a missing
            // barrier in theory. Hence setting all barrier bits whenever
            // something previously written is used for the first time in a
            // subsequent pass.
            for (auto it = tracker.cbeginBuffers(), itEnd = tracker.cendBuffers(); it != itEnd; ++it) {
                QGles2Buffer::Access accessBeforePass = QGles2Buffer::Access(it->stateAtPassBegin.access);
                if (bufferAccessIsWrite(accessBeforePass))
                    barriers |= GL_ALL_BARRIER_BITS;
            }
            for (auto it = tracker.cbeginTextures(), itEnd = tracker.cendTextures(); it != itEnd; ++it) {
                QGles2Texture::Access accessBeforePass = QGles2Texture::Access(it->stateAtPassBegin.access);
                if (textureAccessIsWrite(accessBeforePass))
                    barriers |= GL_ALL_BARRIER_BITS;
            }
            if (barriers && caps.compute)
                f->glMemoryBarrier(barriers);
        }
            break;
        case QGles2CommandBuffer::Command::Barrier:
            if (caps.compute)
                f->glMemoryBarrier(cmd.args.barrier.barriers);
            break;
        default:
            break;
        }
    }
}

void QRhiGles2::executeBindGraphicsPipeline(QGles2CommandBuffer *cbD, QGles2GraphicsPipeline *psD)
{
    QGles2CommandBuffer::GraphicsPassState &state(cbD->graphicsPassState);
    const bool forceUpdate = !state.valid;
    state.valid = true;

    const bool scissor = psD->m_flags.testFlag(QRhiGraphicsPipeline::UsesScissor);
    if (forceUpdate || scissor != state.scissor) {
        state.scissor = scissor;
        if (scissor)
            f->glEnable(GL_SCISSOR_TEST);
        else
            f->glDisable(GL_SCISSOR_TEST);
    }

    const bool cullFace = psD->m_cullMode != QRhiGraphicsPipeline::None;
    const GLenum cullMode = cullFace ? toGlCullMode(psD->m_cullMode) : GL_NONE;
    if (forceUpdate || cullFace != state.cullFace || cullMode != state.cullMode) {
        state.cullFace = cullFace;
        state.cullMode = cullMode;
        if (cullFace) {
            f->glEnable(GL_CULL_FACE);
            f->glCullFace(cullMode);
        } else {
            f->glDisable(GL_CULL_FACE);
        }
    }

    const GLenum frontFace = toGlFrontFace(psD->m_frontFace);
    if (forceUpdate || frontFace != state.frontFace) {
        state.frontFace = frontFace;
        f->glFrontFace(frontFace);
    }

    if (!psD->m_targetBlends.isEmpty()) {
        // We do not have MRT support here, meaning all targets use the blend
        // params from the first one. This is technically incorrect, even if
        // nothing in Qt relies on it. However, considering that
        // glBlendFuncSeparatei is only available in GL 4.0+ and GLES 3.2+, we
        // may just live with this for now because no point in bothering if it
        // won't be usable on many GLES (3.1 or 3.0) systems.
        const QRhiGraphicsPipeline::TargetBlend &targetBlend(psD->m_targetBlends.first());

        const QGles2CommandBuffer::GraphicsPassState::ColorMask colorMask = {
            targetBlend.colorWrite.testFlag(QRhiGraphicsPipeline::R),
            targetBlend.colorWrite.testFlag(QRhiGraphicsPipeline::G),
            targetBlend.colorWrite.testFlag(QRhiGraphicsPipeline::B),
            targetBlend.colorWrite.testFlag(QRhiGraphicsPipeline::A)
        };
        if (forceUpdate || colorMask != state.colorMask) {
            state.colorMask = colorMask;
            f->glColorMask(colorMask.r, colorMask.g, colorMask.b, colorMask.a);
        }

        const bool blendEnabled = targetBlend.enable;
        const QGles2CommandBuffer::GraphicsPassState::Blend blend = {
            toGlBlendFactor(targetBlend.srcColor),
            toGlBlendFactor(targetBlend.dstColor),
            toGlBlendFactor(targetBlend.srcAlpha),
            toGlBlendFactor(targetBlend.dstAlpha),
            toGlBlendOp(targetBlend.opColor),
            toGlBlendOp(targetBlend.opAlpha)
        };
        if (forceUpdate || blendEnabled != state.blendEnabled || (blendEnabled && blend != state.blend)) {
            state.blendEnabled = blendEnabled;
            if (blendEnabled) {
                state.blend = blend;
                f->glEnable(GL_BLEND);
                f->glBlendFuncSeparate(blend.srcColor, blend.dstColor, blend.srcAlpha, blend.dstAlpha);
                f->glBlendEquationSeparate(blend.opColor, blend.opAlpha);
            } else {
                f->glDisable(GL_BLEND);
            }
        }
    } else {
        const QGles2CommandBuffer::GraphicsPassState::ColorMask colorMask = { true, true, true, true };
        if (forceUpdate || colorMask != state.colorMask) {
            state.colorMask = colorMask;
            f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
        const bool blendEnabled = false;
        if (forceUpdate || blendEnabled != state.blendEnabled) {
            state.blendEnabled = blendEnabled;
            f->glDisable(GL_BLEND);
        }
    }

    const bool depthTest = psD->m_depthTest;
    if (forceUpdate || depthTest != state.depthTest) {
        state.depthTest = depthTest;
        if (depthTest)
            f->glEnable(GL_DEPTH_TEST);
        else
            f->glDisable(GL_DEPTH_TEST);
    }

    const bool depthWrite = psD->m_depthWrite;
    if (forceUpdate || depthWrite != state.depthWrite) {
        state.depthWrite = depthWrite;
        f->glDepthMask(depthWrite);
    }

    const GLenum depthFunc = toGlCompareOp(psD->m_depthOp);
    if (forceUpdate || depthFunc != state.depthFunc) {
        state.depthFunc = depthFunc;
        f->glDepthFunc(depthFunc);
    }

    const bool stencilTest = psD->m_stencilTest;
    const GLuint stencilReadMask = psD->m_stencilReadMask;
    const GLuint stencilWriteMask = psD->m_stencilWriteMask;
    const QGles2CommandBuffer::GraphicsPassState::StencilFace stencilFront = {
        toGlCompareOp(psD->m_stencilFront.compareOp),
        toGlStencilOp(psD->m_stencilFront.failOp),
        toGlStencilOp(psD->m_stencilFront.depthFailOp),
        toGlStencilOp(psD->m_stencilFront.passOp)
    };
    const QGles2CommandBuffer::GraphicsPassState::StencilFace stencilBack = {
        toGlCompareOp(psD->m_stencilBack.compareOp),
        toGlStencilOp(psD->m_stencilBack.failOp),
        toGlStencilOp(psD->m_stencilBack.depthFailOp),
        toGlStencilOp(psD->m_stencilBack.passOp)
    };
    if (forceUpdate || stencilTest != state.stencilTest
            || (stencilTest
                && (stencilReadMask != state.stencilReadMask || stencilWriteMask != state.stencilWriteMask
                    || stencilFront != state.stencil[0] || stencilBack != state.stencil[1])))
    {
        state.stencilTest = stencilTest;
        if (stencilTest) {
            state.stencilReadMask = stencilReadMask;
            state.stencilWriteMask = stencilWriteMask;
            state.stencil[0] = stencilFront;
            state.stencil[1] = stencilBack;

            f->glEnable(GL_STENCIL_TEST);

            f->glStencilFuncSeparate(GL_FRONT, stencilFront.func, state.dynamic.stencilRef, stencilReadMask);
            f->glStencilOpSeparate(GL_FRONT, stencilFront.failOp, stencilFront.zfailOp, stencilFront.zpassOp);
            f->glStencilMaskSeparate(GL_FRONT, stencilWriteMask);

            f->glStencilFuncSeparate(GL_BACK, stencilBack.func, state.dynamic.stencilRef, stencilReadMask);
            f->glStencilOpSeparate(GL_BACK, stencilBack.failOp, stencilBack.zfailOp, stencilBack.zpassOp);
            f->glStencilMaskSeparate(GL_BACK, stencilWriteMask);
        } else {
            f->glDisable(GL_STENCIL_TEST);
        }
    }

    const bool polyOffsetFill = psD->m_depthBias != 0 || !qFuzzyIsNull(psD->m_slopeScaledDepthBias);
    const float polyOffsetFactor = psD->m_slopeScaledDepthBias;
    const float polyOffsetUnits = psD->m_depthBias;
    if (forceUpdate || state.polyOffsetFill != polyOffsetFill
            || polyOffsetFactor != state.polyOffsetFactor || polyOffsetUnits != state.polyOffsetUnits)
    {
        state.polyOffsetFill = polyOffsetFill;
        state.polyOffsetFactor = polyOffsetFactor;
        state.polyOffsetUnits = polyOffsetUnits;
        if (polyOffsetFill) {
            f->glPolygonOffset(polyOffsetFactor, polyOffsetUnits);
            f->glEnable(GL_POLYGON_OFFSET_FILL);
        } else {
            f->glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    if (psD->m_topology == QRhiGraphicsPipeline::Lines || psD->m_topology == QRhiGraphicsPipeline::LineStrip) {
        const float lineWidth = psD->m_lineWidth;
        if (forceUpdate || lineWidth != state.lineWidth) {
            state.lineWidth = lineWidth;
            f->glLineWidth(lineWidth);
        }
    }

    f->glUseProgram(psD->program);
}

static inline void qrhi_std140_to_packed(float *dst, int vecSize, int elemCount, const void *src)
{
    const float *p = reinterpret_cast<const float *>(src);
    for (int i = 0; i < elemCount; ++i) {
        for (int j = 0; j < vecSize; ++j)
            dst[vecSize * i + j] = *p++;
        p += 4 - vecSize;
    }
}

void QRhiGles2::bindShaderResources(QRhiGraphicsPipeline *maybeGraphicsPs, QRhiComputePipeline *maybeComputePs,
                                    QRhiShaderResourceBindings *srb,
                                    const uint *dynOfsPairs, int dynOfsCount)
{
    QGles2ShaderResourceBindings *srbD = QRHI_RES(QGles2ShaderResourceBindings, srb);
    int texUnit = 0;
    QVarLengthArray<float, 256> packedFloatArray;

    for (int i = 0, ie = srbD->m_bindings.count(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = srbD->m_bindings.at(i).data();

        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            int viewOffset = b->u.ubuf.offset;
            if (dynOfsCount) {
                for (int j = 0; j < dynOfsCount; ++j) {
                    if (dynOfsPairs[2 * j] == uint(b->binding)) {
                        viewOffset = int(dynOfsPairs[2 * j + 1]);
                        break;
                    }
                }
            }
            QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, b->u.ubuf.buf);
            const QByteArray bufView = QByteArray::fromRawData(bufD->ubuf.constData() + viewOffset,
                                                               b->u.ubuf.maybeSize ? b->u.ubuf.maybeSize : bufD->m_size);
            QVector<QGles2UniformDescription> &uniforms(maybeGraphicsPs ? QRHI_RES(QGles2GraphicsPipeline, maybeGraphicsPs)->uniforms
                                                                        : QRHI_RES(QGles2ComputePipeline, maybeComputePs)->uniforms);
            for (QGles2UniformDescription &uniform : uniforms) {
                if (uniform.binding == b->binding) {
                    // in a uniform buffer everything is at least 4 byte aligned
                    // so this should not cause unaligned reads
                    const void *src = bufView.constData() + uniform.offset;

                    if (uniform.arrayDim > 0
                            && uniform.type != QShaderDescription::Float
                            && uniform.type != QShaderDescription::Vec2
                            && uniform.type != QShaderDescription::Vec3
                            && uniform.type != QShaderDescription::Vec4)
                    {
                        qWarning("Uniform with buffer binding %d, buffer offset %d, type %d is an array, "
                                 "but arrays are only supported for float, vec2, vec3, and vec4. "
                                 "Only the first element will be set.",
                                 uniform.binding, uniform.offset, uniform.type);
                    }

                    // Our input is an std140 layout uniform block. See
                    // "Standard Uniform Block Layout" in section 7.6.2.2 of
                    // the OpenGL spec. This has some peculiar alignment
                    // requirements, which is not what glUniform* wants. Hence
                    // the unpacking/repacking for arrays and certain types.

                    switch (uniform.type) {
                    case QShaderDescription::Float:
                    {
                        const int elemCount = uniform.arrayDim;
                        if (elemCount < 1) {
                            f->glUniform1f(uniform.glslLocation, *reinterpret_cast<const float *>(src));
                        } else {
                            // input is 16 bytes per element as per std140, have to convert to packed
                            packedFloatArray.resize(elemCount);
                            qrhi_std140_to_packed(packedFloatArray.data(), 1, elemCount, src);
                            f->glUniform1fv(uniform.glslLocation, elemCount, packedFloatArray.constData());
                        }
                    }
                        break;
                    case QShaderDescription::Vec2:
                    {
                        const int elemCount = uniform.arrayDim;
                        if (elemCount < 1) {
                            f->glUniform2fv(uniform.glslLocation, 1, reinterpret_cast<const float *>(src));
                        } else {
                            packedFloatArray.resize(elemCount * 2);
                            qrhi_std140_to_packed(packedFloatArray.data(), 2, elemCount, src);
                            f->glUniform2fv(uniform.glslLocation, elemCount, packedFloatArray.constData());
                        }
                    }
                        break;
                    case QShaderDescription::Vec3:
                    {
                        const int elemCount = uniform.arrayDim;
                        if (elemCount < 1) {
                            f->glUniform3fv(uniform.glslLocation, 1, reinterpret_cast<const float *>(src));
                        } else {
                            packedFloatArray.resize(elemCount * 3);
                            qrhi_std140_to_packed(packedFloatArray.data(), 3, elemCount, src);
                            f->glUniform3fv(uniform.glslLocation, elemCount, packedFloatArray.constData());
                        }
                    }
                        break;
                    case QShaderDescription::Vec4:
                        f->glUniform4fv(uniform.glslLocation, qMax(1, uniform.arrayDim), reinterpret_cast<const float *>(src));
                        break;
                    case QShaderDescription::Mat2:
                        f->glUniformMatrix2fv(uniform.glslLocation, 1, GL_FALSE, reinterpret_cast<const float *>(src));
                        break;
                    case QShaderDescription::Mat3:
                    {
                        // 4 floats per column (or row, if row-major)
                        float mat[9];
                        const float *srcMat = reinterpret_cast<const float *>(src);
                        memcpy(mat, srcMat, 3 * sizeof(float));
                        memcpy(mat + 3, srcMat + 4, 3 * sizeof(float));
                        memcpy(mat + 6, srcMat + 8, 3 * sizeof(float));
                        f->glUniformMatrix3fv(uniform.glslLocation, 1, GL_FALSE, mat);
                    }
                        break;
                    case QShaderDescription::Mat4:
                        f->glUniformMatrix4fv(uniform.glslLocation, 1, GL_FALSE, reinterpret_cast<const float *>(src));
                        break;
                    case QShaderDescription::Int:
                        f->glUniform1i(uniform.glslLocation, *reinterpret_cast<const qint32 *>(src));
                        break;
                    case QShaderDescription::Int2:
                        f->glUniform2iv(uniform.glslLocation, 1, reinterpret_cast<const qint32 *>(src));
                        break;
                    case QShaderDescription::Int3:
                        f->glUniform3iv(uniform.glslLocation, 1, reinterpret_cast<const qint32 *>(src));
                        break;
                    case QShaderDescription::Int4:
                        f->glUniform4iv(uniform.glslLocation, 1, reinterpret_cast<const qint32 *>(src));
                        break;
                    case QShaderDescription::Uint:
                        f->glUniform1ui(uniform.glslLocation, *reinterpret_cast<const quint32 *>(src));
                        break;
                    case QShaderDescription::Uint2:
                        f->glUniform2uiv(uniform.glslLocation, 1, reinterpret_cast<const quint32 *>(src));
                        break;
                    case QShaderDescription::Uint3:
                        f->glUniform3uiv(uniform.glslLocation, 1, reinterpret_cast<const quint32 *>(src));
                        break;
                    case QShaderDescription::Uint4:
                        f->glUniform4uiv(uniform.glslLocation, 1, reinterpret_cast<const quint32 *>(src));
                        break;
                    case QShaderDescription::Bool: // a glsl bool is 4 bytes, like (u)int
                        f->glUniform1i(uniform.glslLocation, *reinterpret_cast<const qint32 *>(src));
                        break;
                    case QShaderDescription::Bool2:
                        f->glUniform2iv(uniform.glslLocation, 1, reinterpret_cast<const qint32 *>(src));
                        break;
                    case QShaderDescription::Bool3:
                        f->glUniform3iv(uniform.glslLocation, 1, reinterpret_cast<const qint32 *>(src));
                        break;
                    case QShaderDescription::Bool4:
                        f->glUniform4iv(uniform.glslLocation, 1, reinterpret_cast<const qint32 *>(src));
                        break;
                    default:
                        qWarning("Uniform with buffer binding %d, buffer offset %d has unsupported type %d",
                                 uniform.binding, uniform.offset, uniform.type);
                        break;
                    }
                }
            }
        }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
        {
            QVector<QGles2SamplerDescription> &samplers(maybeGraphicsPs ? QRHI_RES(QGles2GraphicsPipeline, maybeGraphicsPs)->samplers
                                                                        : QRHI_RES(QGles2ComputePipeline, maybeComputePs)->samplers);
            for (int elem = 0; elem < b->u.stex.count; ++elem) {
                QGles2Texture *texD = QRHI_RES(QGles2Texture, b->u.stex.texSamplers[elem].tex);
                QGles2Sampler *samplerD = QRHI_RES(QGles2Sampler, b->u.stex.texSamplers[elem].sampler);
                for (QGles2SamplerDescription &sampler : samplers) {
                    if (sampler.binding == b->binding) {
                        f->glActiveTexture(GL_TEXTURE0 + uint(texUnit));
                        f->glBindTexture(texD->target, texD->texture);

                        if (texD->samplerState != samplerD->d) {
                            f->glTexParameteri(texD->target, GL_TEXTURE_MIN_FILTER, GLint(samplerD->d.glminfilter));
                            f->glTexParameteri(texD->target, GL_TEXTURE_MAG_FILTER, GLint(samplerD->d.glmagfilter));
                            f->glTexParameteri(texD->target, GL_TEXTURE_WRAP_S, GLint(samplerD->d.glwraps));
                            f->glTexParameteri(texD->target, GL_TEXTURE_WRAP_T, GLint(samplerD->d.glwrapt));
                            // 3D textures not supported by GLES 2.0 or by us atm...
                            //f->glTexParameteri(texD->target, GL_TEXTURE_WRAP_R, samplerD->d.glwrapr);
                            if (caps.textureCompareMode) {
                                if (samplerD->d.gltexcomparefunc != GL_NEVER) {
                                    f->glTexParameteri(texD->target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                                    f->glTexParameteri(texD->target, GL_TEXTURE_COMPARE_FUNC, GLint(samplerD->d.gltexcomparefunc));
                                } else {
                                    f->glTexParameteri(texD->target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
                                }
                            }
                            texD->samplerState = samplerD->d;
                        }

                        f->glUniform1i(sampler.glslLocation + elem, texUnit);
                        ++texUnit;
                    }
                }
            }
        }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
        {
            QGles2Texture *texD = QRHI_RES(QGles2Texture, b->u.simage.tex);
            const bool layered = texD->m_flags.testFlag(QRhiTexture::CubeMap);
            GLenum access = GL_READ_WRITE;
            if (b->type == QRhiShaderResourceBinding::ImageLoad)
                access = GL_READ_ONLY;
            else if (b->type == QRhiShaderResourceBinding::ImageStore)
                access = GL_WRITE_ONLY;
            f->glBindImageTexture(GLuint(b->binding), texD->texture,
                                  b->u.simage.level, layered, 0,
                                  access, texD->glsizedintformat);
        }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
        case QRhiShaderResourceBinding::BufferStore:
        case QRhiShaderResourceBinding::BufferLoadStore:
        {
            QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, b->u.sbuf.buf);
            if (b->u.sbuf.offset == 0 && b->u.sbuf.maybeSize == 0)
                f->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GLuint(b->binding), bufD->buffer);
            else
                f->glBindBufferRange(GL_SHADER_STORAGE_BUFFER, GLuint(b->binding), bufD->buffer,
                                     b->u.sbuf.offset, b->u.sbuf.maybeSize ? b->u.sbuf.maybeSize : bufD->m_size);
        }
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    if (texUnit > 1)
        f->glActiveTexture(GL_TEXTURE0);
}

void QRhiGles2::resourceUpdate(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_ASSERT(QRHI_RES(QGles2CommandBuffer, cb)->recordingPass == QGles2CommandBuffer::NoPass);

    enqueueResourceUpdates(cb, resourceUpdates);
}

QGles2RenderTargetData *QRhiGles2::enqueueBindFramebuffer(QRhiRenderTarget *rt, QGles2CommandBuffer *cbD,
                                                          bool *wantsColorClear, bool *wantsDsClear)
{
    QGles2RenderTargetData *rtD = nullptr;
    QRhiPassResourceTracker &passResTracker(cbD->passResTrackers[cbD->currentPassResTrackerIndex]);

    QGles2CommandBuffer::Command fbCmd;
    fbCmd.cmd = QGles2CommandBuffer::Command::BindFramebuffer;

    switch (rt->resourceType()) {
    case QRhiResource::RenderTarget:
        rtD = &QRHI_RES(QGles2ReferenceRenderTarget, rt)->d;
        if (wantsColorClear)
            *wantsColorClear = true;
        if (wantsDsClear)
            *wantsDsClear = true;
        fbCmd.args.bindFramebuffer.fbo = 0;
        fbCmd.args.bindFramebuffer.colorAttCount = 1;
        break;
    case QRhiResource::TextureRenderTarget:
    {
        QGles2TextureRenderTarget *rtTex = QRHI_RES(QGles2TextureRenderTarget, rt);
        rtD = &rtTex->d;
        if (wantsColorClear)
            *wantsColorClear = !rtTex->m_flags.testFlag(QRhiTextureRenderTarget::PreserveColorContents);
        if (wantsDsClear)
            *wantsDsClear = !rtTex->m_flags.testFlag(QRhiTextureRenderTarget::PreserveDepthStencilContents);
        fbCmd.args.bindFramebuffer.fbo = rtTex->framebuffer;
        fbCmd.args.bindFramebuffer.colorAttCount = rtD->colorAttCount;

        for (auto it = rtTex->m_desc.cbeginColorAttachments(), itEnd = rtTex->m_desc.cendColorAttachments();
             it != itEnd; ++it)
        {
            const QRhiColorAttachment &colorAtt(*it);
            QGles2Texture *texD = QRHI_RES(QGles2Texture, colorAtt.texture());
            QGles2Texture *resolveTexD = QRHI_RES(QGles2Texture, colorAtt.resolveTexture());
            if (texD) {
                trackedRegisterTexture(&passResTracker, texD,
                                       QRhiPassResourceTracker::TexColorOutput,
                                       QRhiPassResourceTracker::TexColorOutputStage);
            }
            if (resolveTexD) {
                trackedRegisterTexture(&passResTracker, resolveTexD,
                                       QRhiPassResourceTracker::TexColorOutput,
                                       QRhiPassResourceTracker::TexColorOutputStage);
            }
            // renderbuffers cannot be written in shaders (no image store) so
            // they do not matter here
        }
        if (rtTex->m_desc.depthTexture()) {
            trackedRegisterTexture(&passResTracker, QRHI_RES(QGles2Texture, rtTex->m_desc.depthTexture()),
                                   QRhiPassResourceTracker::TexDepthOutput,
                                   QRhiPassResourceTracker::TexDepthOutputStage);
        }
    }
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    fbCmd.args.bindFramebuffer.srgb = rtD->srgbUpdateAndBlend;
    cbD->commands.append(fbCmd);

    return rtD;
}

void QRhiGles2::enqueueBarriersForPass(QGles2CommandBuffer *cbD)
{
    cbD->passResTrackers.append(QRhiPassResourceTracker());
    cbD->currentPassResTrackerIndex = cbD->passResTrackers.count() - 1;
    QGles2CommandBuffer::Command cmd;
    cmd.cmd = QGles2CommandBuffer::Command::BarriersForPass;
    cmd.args.barriersForPass.trackerIndex = cbD->currentPassResTrackerIndex;
    cbD->commands.append(cmd);
}

void QRhiGles2::beginPass(QRhiCommandBuffer *cb,
                          QRhiRenderTarget *rt,
                          const QColor &colorClearValue,
                          const QRhiDepthStencilClearValue &depthStencilClearValue,
                          QRhiResourceUpdateBatch *resourceUpdates)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);

    // Get a new resource tracker. Then add a command that will generate
    // glMemoryBarrier() calls based on that tracker when submitted.
    enqueueBarriersForPass(cbD);

    bool wantsColorClear, wantsDsClear;
    QGles2RenderTargetData *rtD = enqueueBindFramebuffer(rt, cbD, &wantsColorClear, &wantsDsClear);

    QGles2CommandBuffer::Command clearCmd;
    clearCmd.cmd = QGles2CommandBuffer::Command::Clear;
    clearCmd.args.clear.mask = 0;
    if (rtD->colorAttCount && wantsColorClear)
        clearCmd.args.clear.mask |= GL_COLOR_BUFFER_BIT;
    if (rtD->dsAttCount && wantsDsClear)
        clearCmd.args.clear.mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    clearCmd.args.clear.c[0] = float(colorClearValue.redF());
    clearCmd.args.clear.c[1] = float(colorClearValue.greenF());
    clearCmd.args.clear.c[2] = float(colorClearValue.blueF());
    clearCmd.args.clear.c[3] = float(colorClearValue.alphaF());
    clearCmd.args.clear.d = depthStencilClearValue.depthClearValue();
    clearCmd.args.clear.s = depthStencilClearValue.stencilClearValue();
    cbD->commands.append(clearCmd);

    cbD->recordingPass = QGles2CommandBuffer::RenderPass;
    cbD->currentTarget = rt;

    cbD->resetCachedState();
}

void QRhiGles2::endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    if (cbD->currentTarget->resourceType() == QRhiResource::TextureRenderTarget) {
        QGles2TextureRenderTarget *rtTex = QRHI_RES(QGles2TextureRenderTarget, cbD->currentTarget);
        if (rtTex->m_desc.cbeginColorAttachments() != rtTex->m_desc.cendColorAttachments()) {
            // handle only 1 color attachment and only (msaa) renderbuffer
            const QRhiColorAttachment &colorAtt(*rtTex->m_desc.cbeginColorAttachments());
            if (colorAtt.resolveTexture()) {
                Q_ASSERT(colorAtt.renderBuffer());
                QGles2RenderBuffer *rbD = QRHI_RES(QGles2RenderBuffer, colorAtt.renderBuffer());
                const QSize size = colorAtt.resolveTexture()->pixelSize();
                if (rbD->pixelSize() != size) {
                    qWarning("Resolve source (%dx%d) and target (%dx%d) size does not match",
                             rbD->pixelSize().width(), rbD->pixelSize().height(), size.width(), size.height());
                }
                QGles2CommandBuffer::Command cmd;
                cmd.cmd = QGles2CommandBuffer::Command::BlitFromRenderbuffer;
                cmd.args.blitFromRb.renderbuffer = rbD->renderbuffer;
                cmd.args.blitFromRb.w = size.width();
                cmd.args.blitFromRb.h = size.height();
                QGles2Texture *colorTexD = QRHI_RES(QGles2Texture, colorAtt.resolveTexture());
                const GLenum faceTargetBase = colorTexD->m_flags.testFlag(QRhiTexture::CubeMap) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X
                                                                                                : colorTexD->target;
                cmd.args.blitFromRb.target = faceTargetBase + uint(colorAtt.resolveLayer());
                cmd.args.blitFromRb.texture = colorTexD->texture;
                cmd.args.blitFromRb.dstLevel = colorAtt.resolveLevel();
                cbD->commands.append(cmd);
            }
        }
    }

    cbD->recordingPass = QGles2CommandBuffer::NoPass;
    cbD->currentTarget = nullptr;

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);
}

void QRhiGles2::beginComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);

    enqueueBarriersForPass(cbD);

    cbD->recordingPass = QGles2CommandBuffer::ComputePass;

    cbD->resetCachedState();
}

void QRhiGles2::endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::ComputePass);

    cbD->recordingPass = QGles2CommandBuffer::NoPass;

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);
}

void QRhiGles2::setComputePipeline(QRhiCommandBuffer *cb, QRhiComputePipeline *ps)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::ComputePass);
    QGles2ComputePipeline *psD = QRHI_RES(QGles2ComputePipeline, ps);
    const bool pipelineChanged = cbD->currentComputePipeline != ps || cbD->currentPipelineGeneration != psD->generation;

    if (pipelineChanged) {
        cbD->currentGraphicsPipeline = nullptr;
        cbD->currentComputePipeline = ps;
        cbD->currentPipelineGeneration = psD->generation;

        QGles2CommandBuffer::Command cmd;
        cmd.cmd = QGles2CommandBuffer::Command::BindComputePipeline;
        cmd.args.bindComputePipeline.ps = ps;
        cbD->commands.append(cmd);
    }
}

template<typename T>
inline void qrhigl_accumulateComputeResource(T *writtenResources, QRhiResource *resource,
                                             QRhiShaderResourceBinding::Type bindingType,
                                             int loadTypeVal, int storeTypeVal, int loadStoreTypeVal)
{
    int access = 0;
    if (bindingType == loadTypeVal) {
        access = QGles2CommandBuffer::ComputePassState::Read;
    } else {
        access = QGles2CommandBuffer::ComputePassState::Write;
        if (bindingType == loadStoreTypeVal)
            access |= QGles2CommandBuffer::ComputePassState::Read;
    }
    auto it = writtenResources->find(resource);
    if (it != writtenResources->end())
        it->first |= access;
    else if (bindingType == storeTypeVal || bindingType == loadStoreTypeVal)
        writtenResources->insert(resource, { access, true });
}

void QRhiGles2::dispatch(QRhiCommandBuffer *cb, int x, int y, int z)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::ComputePass);

    if (cbD->currentComputeSrb) {
        GLbitfield barriers = 0;

        // The key in the writtenResources map indicates that the resource was
        // written in a previous dispatch, whereas the value accumulates the
        // access mask in the current one.
        for (auto &accessAndIsNewFlag : cbD->computePassState.writtenResources)
            accessAndIsNewFlag = { 0, false };

        QGles2ShaderResourceBindings *srbD = QRHI_RES(QGles2ShaderResourceBindings, cbD->currentComputeSrb);
        const int bindingCount = srbD->m_bindings.count();
        for (int i = 0; i < bindingCount; ++i) {
            const QRhiShaderResourceBinding::Data *b = srbD->m_bindings.at(i).data();
            switch (b->type) {
            case QRhiShaderResourceBinding::ImageLoad:
            case QRhiShaderResourceBinding::ImageStore:
            case QRhiShaderResourceBinding::ImageLoadStore:
                qrhigl_accumulateComputeResource(&cbD->computePassState.writtenResources,
                                                 b->u.simage.tex,
                                                 b->type,
                                                 QRhiShaderResourceBinding::ImageLoad,
                                                 QRhiShaderResourceBinding::ImageStore,
                                                 QRhiShaderResourceBinding::ImageLoadStore);
                break;
            case QRhiShaderResourceBinding::BufferLoad:
            case QRhiShaderResourceBinding::BufferStore:
            case QRhiShaderResourceBinding::BufferLoadStore:
                qrhigl_accumulateComputeResource(&cbD->computePassState.writtenResources,
                                                 b->u.sbuf.buf,
                                                 b->type,
                                                 QRhiShaderResourceBinding::BufferLoad,
                                                 QRhiShaderResourceBinding::BufferStore,
                                                 QRhiShaderResourceBinding::BufferLoadStore);
                break;
            default:
                break;
            }
        }

        for (auto it = cbD->computePassState.writtenResources.begin(); it != cbD->computePassState.writtenResources.end(); ) {
            const int accessInThisDispatch = it->first;
            const bool isNewInThisDispatch = it->second;
            if (accessInThisDispatch && !isNewInThisDispatch) {
                if (it.key()->resourceType() == QRhiResource::Texture)
                    barriers |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
                else
                    barriers |= GL_SHADER_STORAGE_BARRIER_BIT;
            }
            // Anything that was previously written, but is only read now, can be
            // removed from the written list (because that previous write got a
            // corresponding barrier now).
            if (accessInThisDispatch == QGles2CommandBuffer::ComputePassState::Read)
                it = cbD->computePassState.writtenResources.erase(it);
            else
                ++it;
        }

        if (barriers) {
            QGles2CommandBuffer::Command cmd;
            cmd.cmd = QGles2CommandBuffer::Command::Barrier;
            cmd.args.barrier.barriers = barriers;
            cbD->commands.append(cmd);
        }
    }

    QGles2CommandBuffer::Command cmd;
    cmd.cmd = QGles2CommandBuffer::Command::Dispatch;
    cmd.args.dispatch.x = GLuint(x);
    cmd.args.dispatch.y = GLuint(y);
    cmd.args.dispatch.z = GLuint(z);
    cbD->commands.append(cmd);
}

static inline GLenum toGlShaderType(QRhiShaderStage::Type type)
{
    switch (type) {
    case QRhiShaderStage::Vertex:
        return GL_VERTEX_SHADER;
    case QRhiShaderStage::Fragment:
        return GL_FRAGMENT_SHADER;
    case QRhiShaderStage::Compute:
        return GL_COMPUTE_SHADER;
    default:
        Q_UNREACHABLE();
        return GL_VERTEX_SHADER;
    }
}

QByteArray QRhiGles2::shaderSource(const QRhiShaderStage &shaderStage, int *glslVersion)
{
    const QShader bakedShader = shaderStage.shader();
    QVector<int> versionsToTry;
    QByteArray source;
    if (caps.gles) {
        if (caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 2)) {
            versionsToTry << 320 << 310 << 300 << 100;
        } else if (caps.ctxMajor == 3 && caps.ctxMinor == 1) {
            versionsToTry << 310 << 300 << 100;
        } else if (caps.ctxMajor == 3 && caps.ctxMinor == 0) {
            versionsToTry << 300 << 100;
        } else {
            versionsToTry << 100;
        }
        for (int v : versionsToTry) {
            QShaderVersion ver(v, QShaderVersion::GlslEs);
            source = bakedShader.shader({ QShader::GlslShader, ver, shaderStage.shaderVariant() }).shader();
            if (!source.isEmpty()) {
                if (glslVersion)
                    *glslVersion = v;
                break;
            }
        }
    } else {
        if (caps.ctxMajor > 4 || (caps.ctxMajor == 4 && caps.ctxMinor >= 6)) {
            versionsToTry << 460 << 450 << 440 << 430 << 420 << 410 << 400 << 330 << 150;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 5) {
            versionsToTry << 450 << 440 << 430 << 420 << 410 << 400 << 330 << 150;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 4) {
            versionsToTry << 440 << 430 << 420 << 410 << 400 << 330 << 150;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 3) {
            versionsToTry << 430 << 420 << 410 << 400 << 330 << 150;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 2) {
            versionsToTry << 420 << 410 << 400 << 330 << 150;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 1) {
            versionsToTry << 410 << 400 << 330 << 150;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 0) {
            versionsToTry << 400 << 330 << 150;
        } else if (caps.ctxMajor == 3 && caps.ctxMinor == 3) {
            versionsToTry << 330 << 150;
        } else if (caps.ctxMajor == 3 && caps.ctxMinor == 2) {
            versionsToTry << 150;
        }
        if (!caps.coreProfile)
            versionsToTry << 120;
        for (int v : versionsToTry) {
            source = bakedShader.shader({ QShader::GlslShader, v, shaderStage.shaderVariant() }).shader();
            if (!source.isEmpty()) {
                if (glslVersion)
                    *glslVersion = v;
                break;
            }
        }
    }
    if (source.isEmpty()) {
        qWarning() << "No GLSL shader code found (versions tried: " << versionsToTry
                   << ") in baked shader" << bakedShader;
    }
    return source;
}

bool QRhiGles2::compileShader(GLuint program, const QRhiShaderStage &shaderStage, int *glslVersion)
{
    const QByteArray source = shaderSource(shaderStage, glslVersion);
    if (source.isEmpty())
        return false;

    GLuint shader;
    auto cacheIt = m_shaderCache.constFind(shaderStage);
    if (cacheIt != m_shaderCache.constEnd()) {
        shader = *cacheIt;
    } else {
        shader = f->glCreateShader(toGlShaderType(shaderStage.type()));
        const char *srcStr = source.constData();
        const GLint srcLength = source.count();
        f->glShaderSource(shader, 1, &srcStr, &srcLength);
        f->glCompileShader(shader);
        GLint compiled = 0;
        f->glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLogLength = 0;
            f->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
            QByteArray log;
            if (infoLogLength > 1) {
                GLsizei length = 0;
                log.resize(infoLogLength);
                f->glGetShaderInfoLog(shader, infoLogLength, &length, log.data());
            }
            qWarning("Failed to compile shader: %s\nSource was:\n%s", log.constData(), source.constData());
            return false;
        }
        if (m_shaderCache.count() >= MAX_SHADER_CACHE_ENTRIES) {
            // Use the simplest strategy: too many cached shaders -> drop them all.
            for (uint shader : m_shaderCache)
                f->glDeleteShader(shader); // does not actually get released yet when attached to a not-yet-released program
            m_shaderCache.clear();
        }
        m_shaderCache.insert(shaderStage, shader);
    }

    f->glAttachShader(program, shader);

    return true;
}

bool QRhiGles2::linkProgram(GLuint program)
{
    f->glLinkProgram(program);
    GLint linked = 0;
    f->glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLogLength = 0;
        f->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        QByteArray log;
        if (infoLogLength > 1) {
            GLsizei length = 0;
            log.resize(infoLogLength);
            f->glGetProgramInfoLog(program, infoLogLength, &length, log.data());
        }
        qWarning("Failed to link shader program: %s", log.constData());
        return false;
    }
    return true;
}

void QRhiGles2::registerUniformIfActive(const QShaderDescription::BlockVariable &var,
                                        const QByteArray &namePrefix,
                                        int binding,
                                        int baseOffset,
                                        GLuint program,
                                        QVector<QGles2UniformDescription> *dst)
{
    if (var.type == QShaderDescription::Struct) {
        qWarning("Nested structs are not supported at the moment. '%s' ignored.",
                 qPrintable(var.name));
        return;
    }
    QGles2UniformDescription uniform;
    uniform.type = var.type;
    const QByteArray name = namePrefix + var.name.toUtf8();
    uniform.glslLocation = f->glGetUniformLocation(program, name.constData());
    if (uniform.glslLocation >= 0) {
        if (var.arrayDims.count() > 1) {
            qWarning("Array '%s' has more than one dimension. This is not supported.",
                     qPrintable(var.name));
            return;
        }
        uniform.binding = binding;
        uniform.offset = uint(baseOffset + var.offset);
        uniform.size = var.size;
        uniform.arrayDim = var.arrayDims.isEmpty() ? 0 : var.arrayDims.first();
        dst->append(uniform);
    }
}

void QRhiGles2::gatherUniforms(GLuint program,
                               const QShaderDescription::UniformBlock &ub,
                               QVector<QGles2UniformDescription> *dst)
{
    QByteArray prefix = ub.structName.toUtf8() + '.';
    for (const QShaderDescription::BlockVariable &blockMember : ub.members) {
        if (blockMember.type == QShaderDescription::Struct) {
            QByteArray structPrefix = prefix + blockMember.name.toUtf8();

            const int baseOffset = blockMember.offset;
            if (blockMember.arrayDims.isEmpty()) {
                for (const QShaderDescription::BlockVariable &structMember : blockMember.structMembers)
                    registerUniformIfActive(structMember, structPrefix, ub.binding, baseOffset, program, dst);
            } else {
                if (blockMember.arrayDims.count() > 1) {
                    qWarning("Array of struct '%s' has more than one dimension. Only the first dimension is used.",
                             qPrintable(blockMember.name));
                }
                const int dim = blockMember.arrayDims.first();
                const int elemSize = blockMember.size / dim;
                int elemOffset = baseOffset;
                for (int di = 0; di < dim; ++di) {
                    const QByteArray arrayPrefix = structPrefix + '[' + QByteArray::number(di) + ']' + '.';
                    for (const QShaderDescription::BlockVariable &structMember : blockMember.structMembers)
                        registerUniformIfActive(structMember, arrayPrefix, ub.binding, elemOffset, program, dst);
                    elemOffset += elemSize;
                }
            }
        } else {
            registerUniformIfActive(blockMember, prefix, ub.binding, 0, program, dst);
        }
    }
}

void QRhiGles2::gatherSamplers(GLuint program, const QShaderDescription::InOutVariable &v,
                               QVector<QGles2SamplerDescription> *dst)
{
    QGles2SamplerDescription sampler;
    const QByteArray name = v.name.toUtf8();
    sampler.glslLocation = f->glGetUniformLocation(program, name.constData());
    if (sampler.glslLocation >= 0) {
        sampler.binding = v.binding;
        dst->append(sampler);
    }
}

bool QRhiGles2::isProgramBinaryDiskCacheEnabled() const
{
    static QOpenGLProgramBinarySupportCheckWrapper checker;
    return checker.get(ctx)->isSupported();
}

Q_GLOBAL_STATIC(QOpenGLProgramBinaryCache, qrhi_programBinaryCache);

static inline QShader::Stage toShaderStage(QRhiShaderStage::Type type)
{
    switch (type) {
    case QRhiShaderStage::Vertex:
        return QShader::VertexStage;
    case QRhiShaderStage::Fragment:
        return QShader::FragmentStage;
    case QRhiShaderStage::Compute:
        return QShader::ComputeStage;
    default:
        Q_UNREACHABLE();
        return QShader::VertexStage;
    }
}

QRhiGles2::DiskCacheResult QRhiGles2::tryLoadFromDiskCache(const QRhiShaderStage *stages, int stageCount,
                                                           GLuint program, QByteArray *cacheKey)
{
    QRhiGles2::DiskCacheResult result = QRhiGles2::DiskCacheMiss;
    QByteArray diskCacheKey;

    if (isProgramBinaryDiskCacheEnabled()) {
        QOpenGLProgramBinaryCache::ProgramDesc binaryProgram;
        for (int i = 0; i < stageCount; ++i) {
            const QRhiShaderStage &stage(stages[i]);
            const QByteArray source = shaderSource(stage, nullptr);
            if (source.isEmpty())
                return QRhiGles2::DiskCacheError;
            binaryProgram.shaders.append(QOpenGLProgramBinaryCache::ShaderDesc(toShaderStage(stage.type()), source));
        }

        diskCacheKey = binaryProgram.cacheKey();
        if (qrhi_programBinaryCache()->load(diskCacheKey, program)) {
            qCDebug(lcOpenGLProgramDiskCache, "Program binary received from cache, program %u, key %s",
                    program, diskCacheKey.constData());
            result = QRhiGles2::DiskCacheHit;
        }
    }

    if (cacheKey)
        *cacheKey = diskCacheKey;

    return result;
}

void QRhiGles2::trySaveToDiskCache(GLuint program, const QByteArray &cacheKey)
{
    if (isProgramBinaryDiskCacheEnabled()) {
        qCDebug(lcOpenGLProgramDiskCache, "Saving program binary, program %u, key %s",
                program, cacheKey.constData());
        qrhi_programBinaryCache()->save(cacheKey, program);
    }
}

QGles2Buffer::QGles2Buffer(QRhiImplementation *rhi, Type type, UsageFlags usage, int size)
    : QRhiBuffer(rhi, type, usage, size)
{
}

QGles2Buffer::~QGles2Buffer()
{
    release();
}

void QGles2Buffer::release()
{
    if (!buffer)
        return;

    QRhiGles2::DeferredReleaseEntry e;
    e.type = QRhiGles2::DeferredReleaseEntry::Buffer;

    e.buffer.buffer = buffer;

    buffer = 0;

    QRHI_RES_RHI(QRhiGles2);
    rhiD->releaseQueue.append(e);
    QRHI_PROF;
    QRHI_PROF_F(releaseBuffer(this));
    rhiD->unregisterResource(this);
}

bool QGles2Buffer::build()
{
    if (buffer)
        release();

    QRHI_RES_RHI(QRhiGles2);
    QRHI_PROF;

    const int nonZeroSize = m_size <= 0 ? 256 : m_size;

    if (m_usage.testFlag(QRhiBuffer::UniformBuffer)) {
        if (int(m_usage) != QRhiBuffer::UniformBuffer) {
            qWarning("Uniform buffer: multiple usages specified, this is not supported by the OpenGL backend");
            return false;
        }
        ubuf.resize(nonZeroSize);
        QRHI_PROF_F(newBuffer(this, uint(nonZeroSize), 0, 1));
        return true;
    }

    if (!rhiD->ensureContext())
        return false;

    targetForDataOps = GL_ARRAY_BUFFER;
    if (m_usage.testFlag(QRhiBuffer::IndexBuffer))
        targetForDataOps = GL_ELEMENT_ARRAY_BUFFER;
    else if (m_usage.testFlag(QRhiBuffer::StorageBuffer))
        targetForDataOps = GL_SHADER_STORAGE_BUFFER;

    rhiD->f->glGenBuffers(1, &buffer);
    rhiD->f->glBindBuffer(targetForDataOps, buffer);
    rhiD->f->glBufferData(targetForDataOps, nonZeroSize, nullptr, m_type == Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    usageState.access = AccessNone;

    QRHI_PROF_F(newBuffer(this, uint(nonZeroSize), 1, 0));
    rhiD->registerResource(this);
    return true;
}

QRhiBuffer::NativeBuffer QGles2Buffer::nativeBuffer()
{
    if (m_usage.testFlag(QRhiBuffer::UniformBuffer))
        return { {}, 0 };

    return { { &buffer }, 1 };
}

QGles2RenderBuffer::QGles2RenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                                       int sampleCount, QRhiRenderBuffer::Flags flags)
    : QRhiRenderBuffer(rhi, type, pixelSize, sampleCount, flags)
{
}

QGles2RenderBuffer::~QGles2RenderBuffer()
{
    release();
}

void QGles2RenderBuffer::release()
{
    if (!renderbuffer)
        return;

    QRhiGles2::DeferredReleaseEntry e;
    e.type = QRhiGles2::DeferredReleaseEntry::RenderBuffer;

    e.renderbuffer.renderbuffer = renderbuffer;
    e.renderbuffer.renderbuffer2 = stencilRenderbuffer;

    renderbuffer = 0;
    stencilRenderbuffer = 0;

    QRHI_RES_RHI(QRhiGles2);
    rhiD->releaseQueue.append(e);
    QRHI_PROF;
    QRHI_PROF_F(releaseRenderBuffer(this));
    rhiD->unregisterResource(this);
}

bool QGles2RenderBuffer::build()
{
    if (renderbuffer)
        release();

    QRHI_RES_RHI(QRhiGles2);
    QRHI_PROF;
    samples = rhiD->effectiveSampleCount(m_sampleCount);

    if (m_flags.testFlag(UsedWithSwapChainOnly)) {
        if (m_type == DepthStencil) {
            QRHI_PROF_F(newRenderBuffer(this, false, true, samples));
            return true;
        }

        qWarning("RenderBuffer: UsedWithSwapChainOnly is meaningless in combination with Color");
    }

    if (!rhiD->ensureContext())
        return false;

    rhiD->f->glGenRenderbuffers(1, &renderbuffer);
    rhiD->f->glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);

    const QSize size = m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize;

    switch (m_type) {
    case QRhiRenderBuffer::DepthStencil:
        if (rhiD->caps.msaaRenderBuffer && samples > 1) {
            rhiD->f->glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8,
                                                      size.width(), size.height());
            stencilRenderbuffer = 0;
        } else if (rhiD->caps.packedDepthStencil || rhiD->caps.needsDepthStencilCombinedAttach) {
            const GLenum storage = rhiD->caps.needsDepthStencilCombinedAttach ? GL_DEPTH_STENCIL : GL_DEPTH24_STENCIL8;
            rhiD->f->glRenderbufferStorage(GL_RENDERBUFFER, storage,
                                           size.width(), size.height());
            stencilRenderbuffer = 0;
        } else {
            GLenum depthStorage = GL_DEPTH_COMPONENT;
            if (rhiD->caps.gles) {
                if (rhiD->caps.depth24)
                    depthStorage = GL_DEPTH_COMPONENT24;
                else
                    depthStorage = GL_DEPTH_COMPONENT16; // plain ES 2.0 only has this
            }
            const GLenum stencilStorage = rhiD->caps.gles ? GL_STENCIL_INDEX8 : GL_STENCIL_INDEX;
            rhiD->f->glRenderbufferStorage(GL_RENDERBUFFER, depthStorage,
                                           size.width(), size.height());
            rhiD->f->glGenRenderbuffers(1, &stencilRenderbuffer);
            rhiD->f->glBindRenderbuffer(GL_RENDERBUFFER, stencilRenderbuffer);
            rhiD->f->glRenderbufferStorage(GL_RENDERBUFFER, stencilStorage,
                                           size.width(), size.height());
        }
        QRHI_PROF_F(newRenderBuffer(this, false, false, samples));
        break;
    case QRhiRenderBuffer::Color:
        if (rhiD->caps.msaaRenderBuffer && samples > 1)
            rhiD->f->glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8,
                                                      size.width(), size.height());
        else
            rhiD->f->glRenderbufferStorage(GL_RENDERBUFFER, rhiD->caps.rgba8Format ? GL_RGBA8 : GL_RGBA4,
                                           size.width(), size.height());
        QRHI_PROF_F(newRenderBuffer(this, false, false, samples));
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    rhiD->registerResource(this);
    return true;
}

QRhiTexture::Format QGles2RenderBuffer::backingFormat() const
{
    return m_type == Color ? QRhiTexture::RGBA8 : QRhiTexture::UnknownFormat;
}

QGles2Texture::QGles2Texture(QRhiImplementation *rhi, Format format, const QSize &pixelSize,
                             int sampleCount, Flags flags)
    : QRhiTexture(rhi, format, pixelSize, sampleCount, flags)
{
}

QGles2Texture::~QGles2Texture()
{
    release();
}

void QGles2Texture::release()
{
    if (!texture)
        return;

    QRhiGles2::DeferredReleaseEntry e;
    e.type = QRhiGles2::DeferredReleaseEntry::Texture;

    e.texture.texture = texture;

    texture = 0;
    specified = false;

    QRHI_RES_RHI(QRhiGles2);
    if (owns)
        rhiD->releaseQueue.append(e);
    QRHI_PROF;
    QRHI_PROF_F(releaseTexture(this));
    rhiD->unregisterResource(this);
}

bool QGles2Texture::prepareBuild(QSize *adjustedSize)
{
    if (texture)
        release();

    QRHI_RES_RHI(QRhiGles2);
    if (!rhiD->ensureContext())
        return false;

    const QSize size = m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize;

    const bool isCube = m_flags.testFlag(CubeMap);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);
    const bool isCompressed = rhiD->isCompressedFormat(m_format);

    target = isCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    mipLevelCount = hasMipMaps ? rhiD->q->mipLevelsForSize(size) : 1;
    gltype = GL_UNSIGNED_BYTE;

    if (isCompressed) {
        if (m_flags.testFlag(UsedWithLoadStore)) {
            qWarning("Compressed texture cannot be used with image load/store");
            return false;
        }
        glintformat = toGlCompressedTextureFormat(m_format, m_flags);
        if (!glintformat) {
            qWarning("Compressed format %d not mappable to GL compressed format", m_format);
            return false;
        }
        glsizedintformat = glintformat;
        glformat = GL_RGBA;
    } else {
        switch (m_format) {
        case QRhiTexture::RGBA8:
            glintformat = GL_RGBA;
            glsizedintformat = rhiD->caps.rgba8Format ? GL_RGBA8 : GL_RGBA;
            glformat = GL_RGBA;
            break;
        case QRhiTexture::BGRA8:
            glintformat = rhiD->caps.bgraInternalFormat ? GL_BGRA : GL_RGBA;
            glsizedintformat = rhiD->caps.rgba8Format ? GL_RGBA8 : GL_RGBA;
            glformat = GL_BGRA;
            break;
        case QRhiTexture::R16:
            glintformat = GL_R16;
            glsizedintformat = glintformat;
            glformat = GL_RED;
            gltype = GL_UNSIGNED_SHORT;
            break;
        case QRhiTexture::R8:
            glintformat = GL_R8;
            glsizedintformat = glintformat;
            glformat = GL_RED;
            break;
        case QRhiTexture::RED_OR_ALPHA8:
            glintformat = rhiD->caps.coreProfile ? GL_R8 : GL_ALPHA;
            glsizedintformat = glintformat;
            glformat = rhiD->caps.coreProfile ? GL_RED : GL_ALPHA;
            break;
        case QRhiTexture::RGBA16F:
            glintformat = GL_RGBA16F;
            glsizedintformat = glintformat;
            glformat = GL_RGBA;
            gltype = GL_HALF_FLOAT;
            break;
        case QRhiTexture::RGBA32F:
            glintformat = GL_RGBA32F;
            glsizedintformat = glintformat;
            glformat = GL_RGBA;
            gltype = GL_FLOAT;
            break;
        case QRhiTexture::R16F:
            glintformat = GL_R16F;
            glsizedintformat = glintformat;
            glformat = GL_RED;
            gltype = GL_HALF_FLOAT;
            break;
        case QRhiTexture::R32F:
            glintformat = GL_R32F;
            glsizedintformat = glintformat;
            glformat = GL_RED;
            gltype = GL_FLOAT;
            break;
        case QRhiTexture::D16:
            glintformat = GL_DEPTH_COMPONENT16;
            glsizedintformat = glintformat;
            glformat = GL_DEPTH_COMPONENT;
            gltype = GL_UNSIGNED_SHORT;
            break;
        case QRhiTexture::D32F:
            glintformat = GL_DEPTH_COMPONENT32F;
            glsizedintformat = glintformat;
            glformat = GL_DEPTH_COMPONENT;
            gltype = GL_FLOAT;
            break;
        default:
            Q_UNREACHABLE();
            glintformat = GL_RGBA;
            glsizedintformat = rhiD->caps.rgba8Format ? GL_RGBA8 : GL_RGBA;
            glformat = GL_RGBA;
            break;
        }
    }

    samplerState = QGles2SamplerData();

    usageState.access = AccessNone;

    if (adjustedSize)
        *adjustedSize = size;

    return true;
}

bool QGles2Texture::build()
{
    QSize size;
    if (!prepareBuild(&size))
        return false;

    QRHI_RES_RHI(QRhiGles2);
    rhiD->f->glGenTextures(1, &texture);

    const bool isCube = m_flags.testFlag(CubeMap);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);
    const bool isCompressed = rhiD->isCompressedFormat(m_format);
    if (!isCompressed) {
        rhiD->f->glBindTexture(target, texture);
        if (!m_flags.testFlag(UsedWithLoadStore)) {
            if (hasMipMaps || isCube) {
                const GLenum faceTargetBase = isCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : target;
                for (int layer = 0, layerCount = isCube ? 6 : 1; layer != layerCount; ++layer) {
                    for (int level = 0; level != mipLevelCount; ++level) {
                        const QSize mipSize = rhiD->q->sizeForMipLevel(level, size);
                        rhiD->f->glTexImage2D(faceTargetBase + uint(layer), level, GLint(glintformat),
                                              mipSize.width(), mipSize.height(), 0,
                                              glformat, gltype, nullptr);
                    }
                }
            } else {
                rhiD->f->glTexImage2D(target, 0, GLint(glintformat), size.width(), size.height(),
                                      0, glformat, gltype, nullptr);
            }
        } else {
            // Must be specified with immutable storage functions otherwise
            // bindImageTexture may fail. Also, the internal format must be a
            // sized format here.
            rhiD->f->glTexStorage2D(target, mipLevelCount, glsizedintformat, size.width(), size.height());
        }
        specified = true;
    } else {
        // Cannot use glCompressedTexImage2D without valid data, so defer.
        // Compressed textures will not be used as render targets so this is
        // not an issue.
        specified = false;
    }

    QRHI_PROF;
    QRHI_PROF_F(newTexture(this, true, mipLevelCount, isCube ? 6 : 1, 1));

    owns = true;

    generation += 1;
    rhiD->registerResource(this);
    return true;
}

bool QGles2Texture::buildFrom(QRhiTexture::NativeTexture src)
{
    const uint *textureId = static_cast<const uint *>(src.object);
    if (!textureId || !*textureId)
        return false;

    if (!prepareBuild())
        return false;

    texture = *textureId;
    specified = true;

    QRHI_RES_RHI(QRhiGles2);
    QRHI_PROF;
    QRHI_PROF_F(newTexture(this, false, mipLevelCount, m_flags.testFlag(CubeMap) ? 6 : 1, 1));

    owns = false;

    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::NativeTexture QGles2Texture::nativeTexture()
{
    return {&texture, 0};
}

QGles2Sampler::QGles2Sampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                             AddressMode u, AddressMode v, AddressMode w)
    : QRhiSampler(rhi, magFilter, minFilter, mipmapMode, u, v, w)
{
}

QGles2Sampler::~QGles2Sampler()
{
    release();
}

void QGles2Sampler::release()
{
    // nothing to do here
}

bool QGles2Sampler::build()
{
    d.glminfilter = toGlMinFilter(m_minFilter, m_mipmapMode);
    d.glmagfilter = toGlMagFilter(m_magFilter);
    d.glwraps = toGlWrapMode(m_addressU);
    d.glwrapt = toGlWrapMode(m_addressV);
    d.glwrapr = toGlWrapMode(m_addressW);
    d.gltexcomparefunc = toGlTextureCompareFunc(m_compareOp);

    generation += 1;
    return true;
}

// dummy, no Vulkan-style RenderPass+Framebuffer concept here
QGles2RenderPassDescriptor::QGles2RenderPassDescriptor(QRhiImplementation *rhi)
    : QRhiRenderPassDescriptor(rhi)
{
}

QGles2RenderPassDescriptor::~QGles2RenderPassDescriptor()
{
    release();
}

void QGles2RenderPassDescriptor::release()
{
    // nothing to do here
}

bool QGles2RenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const
{
    Q_UNUSED(other);
    return true;
}

QGles2ReferenceRenderTarget::QGles2ReferenceRenderTarget(QRhiImplementation *rhi)
    : QRhiRenderTarget(rhi),
      d(rhi)
{
}

QGles2ReferenceRenderTarget::~QGles2ReferenceRenderTarget()
{
    release();
}

void QGles2ReferenceRenderTarget::release()
{
    // nothing to do here
}

QSize QGles2ReferenceRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QGles2ReferenceRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QGles2ReferenceRenderTarget::sampleCount() const
{
    return d.sampleCount;
}

QGles2TextureRenderTarget::QGles2TextureRenderTarget(QRhiImplementation *rhi,
                                                     const QRhiTextureRenderTargetDescription &desc,
                                                     Flags flags)
    : QRhiTextureRenderTarget(rhi, desc, flags),
      d(rhi)
{
}

QGles2TextureRenderTarget::~QGles2TextureRenderTarget()
{
    release();
}

void QGles2TextureRenderTarget::release()
{
    if (!framebuffer)
        return;

    QRhiGles2::DeferredReleaseEntry e;
    e.type = QRhiGles2::DeferredReleaseEntry::TextureRenderTarget;

    e.textureRenderTarget.framebuffer = framebuffer;

    framebuffer = 0;

    QRHI_RES_RHI(QRhiGles2);
    rhiD->releaseQueue.append(e);

    rhiD->unregisterResource(this);
}

QRhiRenderPassDescriptor *QGles2TextureRenderTarget::newCompatibleRenderPassDescriptor()
{
    return new QGles2RenderPassDescriptor(m_rhi);
}

bool QGles2TextureRenderTarget::build()
{
    QRHI_RES_RHI(QRhiGles2);

    if (framebuffer)
        release();

    const bool hasColorAttachments = m_desc.cbeginColorAttachments() != m_desc.cendColorAttachments();
    Q_ASSERT(hasColorAttachments || m_desc.depthTexture());
    Q_ASSERT(!m_desc.depthStencilBuffer() || !m_desc.depthTexture());
    const bool hasDepthStencil = m_desc.depthStencilBuffer() || m_desc.depthTexture();

    if (hasColorAttachments) {
        const int count = m_desc.cendColorAttachments() - m_desc.cbeginColorAttachments();
        if (count > rhiD->caps.maxDrawBuffers) {
            qWarning("QGles2TextureRenderTarget: Too many color attachments (%d, max is %d)",
                     count, rhiD->caps.maxDrawBuffers);
        }
    }
    if (m_desc.depthTexture() && !rhiD->caps.depthTexture)
        qWarning("QGles2TextureRenderTarget: Depth texture is not supported and will be ignored");

    if (!rhiD->ensureContext())
        return false;

    rhiD->f->glGenFramebuffers(1, &framebuffer);
    rhiD->f->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    d.colorAttCount = 0;
    int attIndex = 0;
    for (auto it = m_desc.cbeginColorAttachments(), itEnd = m_desc.cendColorAttachments(); it != itEnd; ++it, ++attIndex) {
        d.colorAttCount += 1;
        const QRhiColorAttachment &colorAtt(*it);
        QRhiTexture *texture = colorAtt.texture();
        QRhiRenderBuffer *renderBuffer = colorAtt.renderBuffer();
        Q_ASSERT(texture || renderBuffer);
        if (texture) {
            QGles2Texture *texD = QRHI_RES(QGles2Texture, texture);
            Q_ASSERT(texD->texture && texD->specified);
            const GLenum faceTargetBase = texD->flags().testFlag(QRhiTexture::CubeMap) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : texD->target;
            rhiD->f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + uint(attIndex), faceTargetBase + uint(colorAtt.layer()),
                                            texD->texture, colorAtt.level());
            if (attIndex == 0) {
                d.pixelSize = texD->pixelSize();
                d.sampleCount = 1;
            }
        } else if (renderBuffer) {
            QGles2RenderBuffer *rbD = QRHI_RES(QGles2RenderBuffer, renderBuffer);
            rhiD->f->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + uint(attIndex), GL_RENDERBUFFER, rbD->renderbuffer);
            if (attIndex == 0) {
                d.pixelSize = rbD->pixelSize();
                d.sampleCount = rbD->samples;
            }
        }
    }

    if (hasDepthStencil) {
        if (m_desc.depthStencilBuffer()) {
            QGles2RenderBuffer *depthRbD = QRHI_RES(QGles2RenderBuffer, m_desc.depthStencilBuffer());
            if (rhiD->caps.needsDepthStencilCombinedAttach) {
                rhiD->f->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                                   depthRbD->renderbuffer);
            } else {
                rhiD->f->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                                   depthRbD->renderbuffer);
                if (depthRbD->stencilRenderbuffer)
                    rhiD->f->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                                       depthRbD->stencilRenderbuffer);
                else // packed
                    rhiD->f->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                                       depthRbD->renderbuffer);
            }
            if (d.colorAttCount == 0) {
                d.pixelSize = depthRbD->pixelSize();
                d.sampleCount = depthRbD->samples;
            }
        } else {
            QGles2Texture *depthTexD = QRHI_RES(QGles2Texture, m_desc.depthTexture());
            rhiD->f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexD->texture, 0);
            if (d.colorAttCount == 0) {
                d.pixelSize = depthTexD->pixelSize();
                d.sampleCount = 1;
            }
        }
        d.dsAttCount = 1;
    } else {
        d.dsAttCount = 0;
    }

    d.dpr = 1;
    d.rp = QRHI_RES(QGles2RenderPassDescriptor, m_renderPassDesc);

    GLenum status = rhiD->f->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_NO_ERROR && status != GL_FRAMEBUFFER_COMPLETE) {
        qWarning("Framebuffer incomplete: 0x%x", status);
        return false;
    }

    rhiD->registerResource(this);
    return true;
}

QSize QGles2TextureRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QGles2TextureRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QGles2TextureRenderTarget::sampleCount() const
{
    return d.sampleCount;
}

QGles2ShaderResourceBindings::QGles2ShaderResourceBindings(QRhiImplementation *rhi)
    : QRhiShaderResourceBindings(rhi)
{
}

QGles2ShaderResourceBindings::~QGles2ShaderResourceBindings()
{
    release();
}

void QGles2ShaderResourceBindings::release()
{
    // nothing to do here
}

bool QGles2ShaderResourceBindings::build()
{
    generation += 1;
    return true;
}

QGles2GraphicsPipeline::QGles2GraphicsPipeline(QRhiImplementation *rhi)
    : QRhiGraphicsPipeline(rhi)
{
}

QGles2GraphicsPipeline::~QGles2GraphicsPipeline()
{
    release();
}

void QGles2GraphicsPipeline::release()
{
    if (!program)
        return;

    QRhiGles2::DeferredReleaseEntry e;
    e.type = QRhiGles2::DeferredReleaseEntry::Pipeline;

    e.pipeline.program = program;

    program = 0;
    uniforms.clear();
    samplers.clear();

    QRHI_RES_RHI(QRhiGles2);
    rhiD->releaseQueue.append(e);

    rhiD->unregisterResource(this);
}

bool QGles2GraphicsPipeline::build()
{
    QRHI_RES_RHI(QRhiGles2);

    if (program)
        release();

    if (!rhiD->ensureContext())
        return false;

    if (!rhiD->sanityCheckGraphicsPipeline(this))
        return false;

    drawMode = toGlTopology(m_topology);

    program = rhiD->f->glCreateProgram();

    QByteArray diskCacheKey;
    QRhiGles2::DiskCacheResult diskCacheResult = rhiD->tryLoadFromDiskCache(m_shaderStages.constData(),
                                                                            m_shaderStages.count(),
                                                                            program,
                                                                            &diskCacheKey);
    if (diskCacheResult == QRhiGles2::DiskCacheError)
        return false;

    const bool needsCompile = diskCacheResult == QRhiGles2::DiskCacheMiss;

    QShaderDescription vsDesc;
    QShaderDescription fsDesc;
    for (const QRhiShaderStage &shaderStage : qAsConst(m_shaderStages)) {
        const bool isVertex = shaderStage.type() == QRhiShaderStage::Vertex;
        const bool isFragment = shaderStage.type() == QRhiShaderStage::Fragment;
        if (isVertex) {
            if (needsCompile && !rhiD->compileShader(program, shaderStage, nullptr))
                return false;
            vsDesc = shaderStage.shader().description();
        } else if (isFragment) {
            if (needsCompile && !rhiD->compileShader(program, shaderStage, nullptr))
                return false;
            fsDesc = shaderStage.shader().description();
        }
    }

    for (auto inVar : vsDesc.inputVariables()) {
        const QByteArray name = inVar.name.toUtf8();
        rhiD->f->glBindAttribLocation(program, GLuint(inVar.location), name.constData());
    }

    if (needsCompile && !rhiD->linkProgram(program))
        return false;

    if (needsCompile)
        rhiD->trySaveToDiskCache(program, diskCacheKey);

    for (const QShaderDescription::UniformBlock &ub : vsDesc.uniformBlocks())
        rhiD->gatherUniforms(program, ub, &uniforms);

    for (const QShaderDescription::UniformBlock &ub : fsDesc.uniformBlocks())
        rhiD->gatherUniforms(program, ub, &uniforms);

    for (const QShaderDescription::InOutVariable &v : vsDesc.combinedImageSamplers())
        rhiD->gatherSamplers(program, v, &samplers);

    for (const QShaderDescription::InOutVariable &v : fsDesc.combinedImageSamplers())
        rhiD->gatherSamplers(program, v, &samplers);

    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QGles2ComputePipeline::QGles2ComputePipeline(QRhiImplementation *rhi)
    : QRhiComputePipeline(rhi)
{
}

QGles2ComputePipeline::~QGles2ComputePipeline()
{
    release();
}

void QGles2ComputePipeline::release()
{
    if (!program)
        return;

    QRhiGles2::DeferredReleaseEntry e;
    e.type = QRhiGles2::DeferredReleaseEntry::Pipeline;

    e.pipeline.program = program;

    program = 0;
    uniforms.clear();
    samplers.clear();

    QRHI_RES_RHI(QRhiGles2);
    rhiD->releaseQueue.append(e);

    rhiD->unregisterResource(this);
}

bool QGles2ComputePipeline::build()
{
    QRHI_RES_RHI(QRhiGles2);

    if (program)
        release();

    if (!rhiD->ensureContext())
        return false;

    program = rhiD->f->glCreateProgram();
    QShaderDescription csDesc;

    QByteArray diskCacheKey;
    QRhiGles2::DiskCacheResult diskCacheResult = rhiD->tryLoadFromDiskCache(&m_shaderStage, 1, program, &diskCacheKey);
    if (diskCacheResult == QRhiGles2::DiskCacheError)
        return false;

    const bool needsCompile = diskCacheResult == QRhiGles2::DiskCacheMiss;

    if (needsCompile && !rhiD->compileShader(program, m_shaderStage, nullptr))
        return false;

    csDesc = m_shaderStage.shader().description();

    if (needsCompile && !rhiD->linkProgram(program))
        return false;

    if (needsCompile)
        rhiD->trySaveToDiskCache(program, diskCacheKey);

    for (const QShaderDescription::UniformBlock &ub : csDesc.uniformBlocks())
        rhiD->gatherUniforms(program, ub, &uniforms);
    for (const QShaderDescription::InOutVariable &v : csDesc.combinedImageSamplers())
        rhiD->gatherSamplers(program, v, &samplers);

    // storage images and buffers need no special steps here

    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QGles2CommandBuffer::QGles2CommandBuffer(QRhiImplementation *rhi)
    : QRhiCommandBuffer(rhi)
{
    resetState();
}

QGles2CommandBuffer::~QGles2CommandBuffer()
{
    release();
}

void QGles2CommandBuffer::release()
{
    // nothing to do here
}

QGles2SwapChain::QGles2SwapChain(QRhiImplementation *rhi)
    : QRhiSwapChain(rhi),
      rt(rhi),
      cb(rhi)
{
}

QGles2SwapChain::~QGles2SwapChain()
{
    release();
}

void QGles2SwapChain::release()
{
    QRHI_PROF;
    QRHI_PROF_F(releaseSwapChain(this));
}

QRhiCommandBuffer *QGles2SwapChain::currentFrameCommandBuffer()
{
    return &cb;
}

QRhiRenderTarget *QGles2SwapChain::currentFrameRenderTarget()
{
    return &rt;
}

QSize QGles2SwapChain::surfacePixelSize()
{
    Q_ASSERT(m_window);
    return m_window->size() * m_window->devicePixelRatio();
}

QRhiRenderPassDescriptor *QGles2SwapChain::newCompatibleRenderPassDescriptor()
{
    return new QGles2RenderPassDescriptor(m_rhi);
}

bool QGles2SwapChain::buildOrResize()
{
    surface = m_window;
    m_currentPixelSize = surfacePixelSize();
    pixelSize = m_currentPixelSize;

    if (m_depthStencil && m_depthStencil->flags().testFlag(QRhiRenderBuffer::UsedWithSwapChainOnly)
            && m_depthStencil->pixelSize() != pixelSize)
    {
        m_depthStencil->setPixelSize(pixelSize);
        m_depthStencil->build();
    }

    rt.d.rp = QRHI_RES(QGles2RenderPassDescriptor, m_renderPassDesc);
    rt.d.pixelSize = pixelSize;
    rt.d.dpr = float(m_window->devicePixelRatio());
    rt.d.sampleCount = qBound(1, m_sampleCount, 64);
    rt.d.colorAttCount = 1;
    rt.d.dsAttCount = m_depthStencil ? 1 : 0;
    rt.d.srgbUpdateAndBlend = m_flags.testFlag(QRhiSwapChain::sRGB);

    frameCount = 0;

    QRHI_PROF;
    // make something up
    QRHI_PROF_F(resizeSwapChain(this, 2, m_sampleCount > 1 ? 2 : 0, m_sampleCount));

    return true;
}

QT_END_NAMESPACE
