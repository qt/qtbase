// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhigles2_p.h"
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QtCore/qmap.h>
#include <QtGui/private/qopenglextensions_p.h>
#include <QtGui/private/qopenglprogrambinarycache_p.h>
#include <QtGui/private/qwindow_p.h>
#include <qpa/qplatformopenglcontext.h>
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
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief OpenGL specific initialization parameters.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    An OpenGL-based QRhi needs an already created QSurface that can be used in
    combination with QOpenGLContext. Most commonly, this is a QOffscreenSurface
    in practice. Additionally, while optional, it is recommended that the QWindow
    the first QRhiSwapChain will target is passed in as well.

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

    The QSurfaceFormat for the context is specified in \c format. The
    constructor sets this to QSurfaceFormat::defaultFormat() so applications
    that call QSurfaceFormat::setDefaultFormat() with the appropriate settings
    before the constructor runs will not need to change value of \c format.

    \note Remember to set the depth and stencil buffer sizes to 24 and 8 when
    the renderer relies on depth or stencil testing, either in the global
    default QSurfaceFormat, or, alternatively, separately in all the involved
    QSurfaceFormat instances: in \c format, the format argument passed to
    newFallbackSurface(), and on any QWindow that is used with the QRhi.

    A QSurface has to be specified in \c fallbackSurface. In order to prevent
    mistakes in threaded situations, this is never created automatically by the
    QRhi because, like QWindow, instances of QSurface subclasses can often be
    created on the gui/main thread only.

    As a convenience, applications can use newFallbackSurface() which creates
    and returns a QOffscreenSurface that is compatible with the QOpenGLContext
    that is going to be created by the QRhi afterwards. Note that the ownership
    of the returned QOffscreenSurface is transferred to the caller and the QRhi
    will not destroy it.

    \note With the OpenGL backend, QRhiSwapChain can only target QWindow
    instances that have their surface type set to QSurface::OpenGLSurface or
    QSurface::RasterGLSurface.

    \note \c window is optional. It is recommended to specify it whenever
    possible, in order to avoid problems on multi-adapter and multi-screen
    systems. When \c window is not set, the very first
    QOpenGLContext::makeCurrent() happens with \c fallbackSurface which may be
    an invisible window on some platforms (for example, Windows) and that may
    trigger unexpected problems in some cases.

    In case resource sharing with an existing QOpenGLContext is desired, \c
    shareContext can be set to an existing QOpenGLContext. Alternatively,
    Qt::AA_ShareOpenGLContexts is honored as well, when enabled.

    \section2 Working with existing OpenGL contexts

    When interoperating with another graphics engine, it may be necessary to
    get a QRhi instance that uses the same OpenGL context. This can be achieved
    by passing a pointer to a QRhiGles2NativeHandles to QRhi::create(). The
    \c{QRhiGles2NativeHandles::context} must be set to a non-null value then.

    An alternative approach is to create a QOpenGLContext that
    \l{QOpenGLContext::setShareContext()}{shares resources} with the other
    engine's context and passing in that context via QRhiGles2NativeHandles.

    The QRhi does not take ownership of the QOpenGLContext passed in via
    QRhiGles2NativeHandles.
 */

/*!
    \variable QRhiGles2InitParams::format

    The QSurfaceFormat, initialized to QSurfaceFormat::defaultFormat() by default.
*/

/*!
    \variable QRhiGles2InitParams::fallbackSurface

    A QSurface compatible with \l format. Typically a QOffscreenSurface.
    Providing this is mandatory. Be aware of the threading implications: a
    QOffscreenSurface, like QWindow, must only ever be created and destroyed on
    the main (gui) thread, even if the QRhi is created and operates on another
    thread.
*/

/*!
    \variable QRhiGles2InitParams::window

    Optional, but setting it is recommended when targeting a QWindow with the
    QRhi.
*/

/*!
    \variable QRhiGles2InitParams::shareContext

    Optional, the QOpenGLContext to share resource with. QRhi creates its own
    context, and setting this member to a valid QOpenGLContext leads to calling
    \l{QOpenGLContext::setShareContext()}{setShareContext()} with it.
*/

/*!
    \class QRhiGles2NativeHandles
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Holds the OpenGL context used by the QRhi.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiGles2NativeHandles::context
*/

#ifndef GL_BGRA
#define GL_BGRA                           0x80E1
#endif

#ifndef GL_R8
#define GL_R8                             0x8229
#endif

#ifndef GL_R8I
#define GL_R8I                            0x8231
#endif

#ifndef GL_R8UI
#define GL_R8UI                           0x8232
#endif

#ifndef GL_R32I
#define GL_R32I                           0x8235
#endif

#ifndef GL_R32UI
#define GL_R32UI                          0x8236
#endif

#ifndef GL_RG32I
#define GL_RG32I                          0x823B
#endif

#ifndef GL_RG32UI
#define GL_RG32UI                         0x823C
#endif

#ifndef GL_RGBA32I
#define GL_RGBA32I                        0x8D82
#endif

#ifndef GL_RGBA32UI
#define GL_RGBA32UI                       0x8D70
#endif

#ifndef GL_RG8
#define GL_RG8                            0x822B
#endif

#ifndef GL_RG
#define GL_RG                             0x8227
#endif

#ifndef GL_RG_INTEGER
#define GL_RG_INTEGER                     0x8228
#endif

#ifndef GL_R16
#define GL_R16                            0x822A
#endif

#ifndef GL_RG16
#define GL_RG16                           0x822C
#endif

#ifndef GL_RED
#define GL_RED                            0x1903
#endif

#ifndef GL_RED_INTEGER
#define GL_RED_INTEGER                    0x8D94
#endif

#ifndef GL_RGBA_INTEGER
#define GL_RGBA_INTEGER                   0x8D99
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

#ifndef GL_DEPTH32F_STENCIL8
#define GL_DEPTH32F_STENCIL8              0x8CAD
#endif

#ifndef GL_FLOAT_32_UNSIGNED_INT_24_8_REV
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
#endif

#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8              0x84FA
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
#define GL_FRAMEBUFFER_SRGB               0x8DB9
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

#ifndef GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001
#endif

#ifndef GL_ELEMENT_ARRAY_BARRIER_BIT
#define GL_ELEMENT_ARRAY_BARRIER_BIT       0x00000002
#endif

#ifndef GL_UNIFORM_BARRIER_BIT
#define GL_UNIFORM_BARRIER_BIT             0x00000004
#endif

#ifndef GL_BUFFER_UPDATE_BARRIER_BIT
#define GL_BUFFER_UPDATE_BARRIER_BIT       0x00000200
#endif

#ifndef GL_SHADER_STORAGE_BARRIER_BIT
#define GL_SHADER_STORAGE_BARRIER_BIT      0x00002000
#endif

#ifndef GL_TEXTURE_FETCH_BARRIER_BIT
#define GL_TEXTURE_FETCH_BARRIER_BIT       0x00000008
#endif

#ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#endif

#ifndef GL_PIXEL_BUFFER_BARRIER_BIT
#define GL_PIXEL_BUFFER_BARRIER_BIT        0x00000080
#endif

#ifndef GL_TEXTURE_UPDATE_BARRIER_BIT
#define GL_TEXTURE_UPDATE_BARRIER_BIT      0x00000100
#endif

#ifndef GL_FRAMEBUFFER_BARRIER_BIT
#define GL_FRAMEBUFFER_BARRIER_BIT         0x00000400
#endif

#ifndef GL_ALL_BARRIER_BITS
#define GL_ALL_BARRIER_BITS               0xFFFFFFFF
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

#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT                  0x0002
#endif

#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#endif

#ifndef GL_TEXTURE_2D_MULTISAMPLE
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100
#endif

#ifndef GL_TEXTURE_2D_MULTISAMPLE_ARRAY
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY   0x9102
#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES           0x8D65
#endif

#ifndef GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS 0x90EB
#endif

#ifndef GL_MAX_COMPUTE_WORK_GROUP_COUNT
#define GL_MAX_COMPUTE_WORK_GROUP_COUNT   0x91BE
#endif

#ifndef GL_MAX_COMPUTE_WORK_GROUP_SIZE
#define GL_MAX_COMPUTE_WORK_GROUP_SIZE    0x91BF
#endif

#ifndef GL_TEXTURE_CUBE_MAP_SEAMLESS
#define GL_TEXTURE_CUBE_MAP_SEAMLESS      0x884F
#endif

#ifndef GL_CONTEXT_LOST
#define GL_CONTEXT_LOST                   0x0507
#endif

#ifndef GL_PROGRAM_BINARY_LENGTH
#define GL_PROGRAM_BINARY_LENGTH          0x8741
#endif

#ifndef GL_NUM_PROGRAM_BINARY_FORMATS
#define GL_NUM_PROGRAM_BINARY_FORMATS     0x87FE
#endif

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#endif

#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D                     0x806F
#endif

#ifndef GL_TEXTURE_WRAP_R
#define GL_TEXTURE_WRAP_R                 0x8072
#endif

#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE              0x84F5
#endif

#ifndef GL_TEXTURE_2D_ARRAY
#define GL_TEXTURE_2D_ARRAY               0x8C1A
#endif

#ifndef GL_MAX_ARRAY_TEXTURE_LAYERS
#define GL_MAX_ARRAY_TEXTURE_LAYERS       0x88FF
#endif

#ifndef GL_MAX_VERTEX_UNIFORM_COMPONENTS
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS  0x8B4A
#endif

#ifndef GL_MAX_FRAGMENT_UNIFORM_COMPONENTS
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#endif

#ifndef GL_MAX_VERTEX_UNIFORM_VECTORS
#define GL_MAX_VERTEX_UNIFORM_VECTORS     0x8DFB
#endif

#ifndef GL_MAX_FRAGMENT_UNIFORM_VECTORS
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS   0x8DFD
#endif

#ifndef GL_RGB10_A2
#define GL_RGB10_A2                       0x8059
#endif

#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#endif

#ifndef GL_MAX_VARYING_COMPONENTS
#define GL_MAX_VARYING_COMPONENTS         0x8B4B
#endif

#ifndef GL_MAX_VARYING_FLOATS
#define GL_MAX_VARYING_FLOATS             0x8B4B
#endif

#ifndef GL_MAX_VARYING_VECTORS
#define GL_MAX_VARYING_VECTORS            0x8DFC
#endif

#ifndef GL_TESS_CONTROL_SHADER
#define GL_TESS_CONTROL_SHADER            0x8E88
#endif

#ifndef GL_TESS_EVALUATION_SHADER
#define GL_TESS_EVALUATION_SHADER         0x8E87
#endif

#ifndef GL_PATCH_VERTICES
#define GL_PATCH_VERTICES                 0x8E72
#endif

#ifndef GL_LINE
#define GL_LINE                           0x1B01
#endif

#ifndef GL_FILL
#define GL_FILL                           0x1B02
#endif

#ifndef GL_PATCHES
#define GL_PATCHES                        0x000E
#endif

#ifndef GL_GEOMETRY_SHADER
#define GL_GEOMETRY_SHADER                0x8DD9
#endif

#ifndef GL_BACK_LEFT
#define GL_BACK_LEFT                      0x0402
#endif

#ifndef GL_BACK_RIGHT
#define GL_BACK_RIGHT                     0x0403
#endif

#ifndef GL_TEXTURE_1D
#  define GL_TEXTURE_1D                   0x0DE0
#endif

#ifndef GL_TEXTURE_1D_ARRAY
#  define GL_TEXTURE_1D_ARRAY             0x8C18
#endif

#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT                     0x140B
#endif

#ifndef GL_MAX_VERTEX_OUTPUT_COMPONENTS
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS   0x9122
#endif

#ifndef GL_TIMESTAMP
#define GL_TIMESTAMP                      0x8E28
#endif

#ifndef GL_QUERY_RESULT
#define GL_QUERY_RESULT                   0x8866
#endif

#ifndef GL_QUERY_RESULT_AVAILABLE
#define GL_QUERY_RESULT_AVAILABLE         0x8867
#endif

#ifndef GL_BUFFER
#define GL_BUFFER                         0x82E0
#endif

#ifndef GL_PROGRAM
#define GL_PROGRAM                        0x82E2
#endif

/*!
    Constructs a new QRhiGles2InitParams.

    \l format is set to QSurfaceFormat::defaultFormat().
 */
QRhiGles2InitParams::QRhiGles2InitParams()
{
    format = QSurfaceFormat::defaultFormat();
}

/*!
    \return a new QOffscreenSurface that can be used with a QRhi by passing it
    via a QRhiGles2InitParams.

    When \a format is not specified, its default value is the global default
    format settable via QSurfaceFormat::setDefaultFormat().

    \a format is adjusted as appropriate in order to avoid having problems
    afterwards due to an incompatible context and surface.

    \note This function must only be called on the gui/main thread.

    \note It is the application's responsibility to destroy the returned
    QOffscreenSurface on the gui/main thread once the associated QRhi has been
    destroyed. The QRhi will not destroy the QOffscreenSurface.
 */
QOffscreenSurface *QRhiGles2InitParams::newFallbackSurface(const QSurfaceFormat &format)
{
    QSurfaceFormat fmt = format;

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
    requestedFormat = params->format;
    fallbackSurface = params->fallbackSurface;
    maybeWindow = params->window; // may be null
    maybeShareContext = params->shareContext; // may be null

    importedContext = importDevice != nullptr;
    if (importedContext) {
        ctx = importDevice->context;
        if (!ctx) {
            qWarning("No OpenGL context given, cannot import");
            importedContext = false;
        }
    }
}

static inline QSurface *currentSurfaceForCurrentContext(QOpenGLContext *ctx)
{
    if (QOpenGLContext::currentContext() != ctx)
        return nullptr;

    QSurface *currentSurface = ctx->surface();
    if (!currentSurface)
        return nullptr;

    if (currentSurface->surfaceClass() == QSurface::Window && !currentSurface->surfaceHandle())
        return nullptr;

    return currentSurface;
}

QSurface *QRhiGles2::evaluateFallbackSurface() const
{
    // With Apple's deprecated OpenGL support we need to minimize the usage of
    // QOffscreenSurface since delicate problems can pop up with
    // NSOpenGLContext and drawables.
#if defined(Q_OS_MACOS)
    return maybeWindow && maybeWindow->handle() ? static_cast<QSurface *>(maybeWindow) : fallbackSurface;
#else
    return fallbackSurface;
#endif
}

bool QRhiGles2::ensureContext(QSurface *surface) const
{
    if (!surface) {
        // null means any surface is good because not going to render
        if (currentSurfaceForCurrentContext(ctx))
            return true;
        // if the context is not already current with a valid surface, use our
        // fallback surface, but platform specific quirks may apply
        surface = evaluateFallbackSurface();
    } else if (surface->surfaceClass() == QSurface::Window && !surface->surfaceHandle()) {
        // the window is not usable anymore (no native window underneath), behave as if offscreen
        surface = evaluateFallbackSurface();
    } else if (!needsMakeCurrentDueToSwap && currentSurfaceForCurrentContext(ctx) == surface) {
        // bail out if the makeCurrent is not necessary
        return true;
    }
    needsMakeCurrentDueToSwap = false;

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

bool QRhiGles2::create(QRhi::Flags flags)
{
    Q_ASSERT(fallbackSurface);
    rhiFlags = flags;

    if (!importedContext) {
        ctx = new QOpenGLContext;
        ctx->setFormat(requestedFormat);
        if (maybeShareContext) {
            ctx->setShareContext(maybeShareContext);
            ctx->setScreen(maybeShareContext->screen());
        } else if (QOpenGLContext *shareContext = QOpenGLContext::globalShareContext()) {
            ctx->setShareContext(shareContext);
            ctx->setScreen(shareContext->screen());
        } else if (maybeWindow) {
            ctx->setScreen(maybeWindow->screen());
        }
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
    const QSurfaceFormat actualFormat = ctx->format();
    caps.gles = actualFormat.renderableType() == QSurfaceFormat::OpenGLES;

    if (!caps.gles) {
        glPolygonMode = reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLenum, GLenum)>(
                ctx->getProcAddress(QByteArrayLiteral("glPolygonMode")));

        glTexImage1D = reinterpret_cast<void(QOPENGLF_APIENTRYP)(
                GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const void *)>(
                ctx->getProcAddress(QByteArrayLiteral("glTexImage1D")));

        glTexStorage1D = reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLenum, GLint, GLenum, GLsizei)>(
                ctx->getProcAddress(QByteArrayLiteral("glTexStorage1D")));

        glTexSubImage1D = reinterpret_cast<void(QOPENGLF_APIENTRYP)(
                GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *)>(
                ctx->getProcAddress(QByteArrayLiteral("glTexSubImage1D")));

        glCopyTexSubImage1D = reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLenum, GLint, GLint, GLint,
                                                                        GLint, GLsizei)>(
                ctx->getProcAddress(QByteArrayLiteral("glCopyTexSubImage1D")));

        glCompressedTexImage1D = reinterpret_cast<void(QOPENGLF_APIENTRYP)(
                GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid *)>(
                ctx->getProcAddress(QByteArrayLiteral("glCompressedTexImage1D")));

        glCompressedTexSubImage1D = reinterpret_cast<void(QOPENGLF_APIENTRYP)(
                GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid *)>(
                ctx->getProcAddress(QByteArrayLiteral("glCompressedTexSubImage1D")));

        glFramebufferTexture1D =
                reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint)>(
                        ctx->getProcAddress(QByteArrayLiteral("glFramebufferTexture1D")));
    }

    const char *vendor = reinterpret_cast<const char *>(f->glGetString(GL_VENDOR));
    const char *renderer = reinterpret_cast<const char *>(f->glGetString(GL_RENDERER));
    const char *version = reinterpret_cast<const char *>(f->glGetString(GL_VERSION));
    if (vendor && renderer && version)
        qCDebug(QRHI_LOG_INFO, "OpenGL VENDOR: %s RENDERER: %s VERSION: %s", vendor, renderer, version);

    if (vendor) {
        driverInfoStruct.deviceName += QByteArray(vendor);
        driverInfoStruct.deviceName += ' ';
    }
    if (renderer) {
        driverInfoStruct.deviceName += QByteArray(renderer);
        driverInfoStruct.deviceName += ' ';
    }
    if (version)
        driverInfoStruct.deviceName += QByteArray(version);

    caps.ctxMajor = actualFormat.majorVersion();
    caps.ctxMinor = actualFormat.minorVersion();

    GLint n = 0;
    f->glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &n);
    if (n > 0) {
        QVarLengthArray<GLint, 16> compressedTextureFormats(n);
        f->glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, compressedTextureFormats.data());
        for (GLint format : compressedTextureFormats)
            supportedCompressedFormats.insert(format);

    }
    // The above looks nice, if only it worked always. With GLES the list we
    // query is likely the full list of compressed formats (mostly anything
    // that can be decoded). With OpenGL however the list is not required to
    // include all formats due to the way the spec is worded. For instance, we
    // cannot rely on ASTC formats being present in the list on non-ES. Some
    // drivers do include them (Intel, NVIDIA), some don't (Mesa). On the other
    // hand, relying on extension strings only is not ok: for example, Intel
    // reports GL_KHR_texture_compression_astc_ldr whereas NVIDIA doesn't. So
    // the only reasonable thing to do is to query the list always and then see
    // if there is something we can add - if not already in there.
    std::array<QRhiTexture::Flags, 2> textureVariantFlags;
    textureVariantFlags[0] = {};
    textureVariantFlags[1] = QRhiTexture::sRGB;
    if (f->hasOpenGLExtension(QOpenGLExtensions::DDSTextureCompression)) {
        for (QRhiTexture::Flags f : textureVariantFlags) {
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::BC1, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::BC2, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::BC3, f));
        }
    }
    if (f->hasOpenGLExtension(QOpenGLExtensions::ETC2TextureCompression)) {
        for (QRhiTexture::Flags f : textureVariantFlags) {
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ETC2_RGB8, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ETC2_RGB8A1, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ETC2_RGBA8, f));
        }
    }
    if (f->hasOpenGLExtension(QOpenGLExtensions::ASTCTextureCompression)) {
        for (QRhiTexture::Flags f : textureVariantFlags) {
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_4x4, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_5x4, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_5x5, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_6x5, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_6x6, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_8x5, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_8x6, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_8x8, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_10x5, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_10x8, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_10x10, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_12x10, f));
            supportedCompressedFormats.insert(toGlCompressedTextureFormat(QRhiTexture::ASTC_12x12, f));
        }
    }

    f->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.maxTextureSize);

    if (!caps.gles || caps.ctxMajor >= 3) {
        // non-ES or ES 3.0+
        f->glGetIntegerv(GL_MAX_DRAW_BUFFERS, &caps.maxDrawBuffers);
        caps.hasDrawBuffersFunc = true;
        f->glGetIntegerv(GL_MAX_SAMPLES, &caps.maxSamples);
        caps.maxSamples = qMax(1, caps.maxSamples);
    } else {
        // ES 2.0 / WebGL 1
        caps.maxDrawBuffers = 1;
        caps.hasDrawBuffersFunc = false;
        // This does not mean MSAA is not supported, just that we cannot query
        // the supported sample counts. Assume that 4x is always supported.
        caps.maxSamples = 4;
    }

    caps.msaaRenderBuffer = f->hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample)
            && f->hasOpenGLExtension(QOpenGLExtensions::FramebufferBlit);

    caps.npotTextureFull = f->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures)
            && f->hasOpenGLFeature(QOpenGLFunctions::NPOTTextureRepeat);

    if (caps.gles)
        caps.fixedIndexPrimitiveRestart = caps.ctxMajor >= 3; // ES 3.0
    else
        caps.fixedIndexPrimitiveRestart = caps.ctxMajor > 4 || (caps.ctxMajor == 4 && caps.ctxMinor >= 3); // 4.3

    if (caps.fixedIndexPrimitiveRestart) {
#ifdef Q_OS_WASM
        // WebGL 2 behaves as if GL_PRIMITIVE_RESTART_FIXED_INDEX was always
        // enabled (i.e. matching D3D/Metal), and the value cannot be passed to
        // glEnable, so skip the call.
#else
        f->glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
#endif
    }

    caps.bgraExternalFormat = f->hasOpenGLExtension(QOpenGLExtensions::BGRATextureFormat);
    caps.bgraInternalFormat = caps.bgraExternalFormat && caps.gles;
    caps.r8Format = f->hasOpenGLFeature(QOpenGLFunctions::TextureRGFormats);

    if (caps.gles)
        caps.r32uiFormat = (caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 1)) && caps.r8Format; // ES 3.1
    else
        caps.r32uiFormat = true;

    caps.r16Format = f->hasOpenGLExtension(QOpenGLExtensions::Sized16Formats);
    caps.floatFormats = caps.ctxMajor >= 3; // 3.0 or ES 3.0
    caps.rgb10Formats = caps.ctxMajor >= 3; // 3.0 or ES 3.0
    caps.depthTexture = caps.ctxMajor >= 3; // 3.0 or ES 3.0
    caps.packedDepthStencil = f->hasOpenGLExtension(QOpenGLExtensions::PackedDepthStencil);
#ifdef Q_OS_WASM
    caps.needsDepthStencilCombinedAttach = true;
#else
    caps.needsDepthStencilCombinedAttach = false;
#endif

    // QOpenGLExtensions::SRGBFrameBuffer is not useful here. We need to know if
    // controlling the sRGB-on-shader-write state is supported, not that if the
    // default framebuffer is sRGB-capable. And there are two different
    // extensions for desktop and ES.
    caps.srgbWriteControl = ctx->hasExtension("GL_EXT_framebuffer_sRGB") || ctx->hasExtension("GL_EXT_sRGB_write_control");

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

    if (caps.compute) {
        f->glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &caps.maxThreadsPerThreadGroup);
        GLint tgPerDim[3];
        f->glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &tgPerDim[0]);
        f->glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &tgPerDim[1]);
        f->glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &tgPerDim[2]);
        caps.maxThreadGroupsPerDimension = qMin(tgPerDim[0], qMin(tgPerDim[1], tgPerDim[2]));
        f->glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &caps.maxThreadGroupsX);
        f->glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &caps.maxThreadGroupsY);
        f->glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &caps.maxThreadGroupsZ);
    }

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
    caps.intAttributes = caps.ctxMajor >= 3; // 3.0 or ES 3.0
    caps.screenSpaceDerivatives = f->hasOpenGLExtension(QOpenGLExtensions::StandardDerivatives);

    if (caps.gles)
        caps.multisampledTexture = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 1); // ES 3.1
    else
        caps.multisampledTexture = caps.ctxMajor >= 3; // 3.0

    // Program binary support: only the core stuff, do not bother with the old
    // extensions like GL_OES_get_program_binary
    if (caps.gles)
        caps.programBinary = caps.ctxMajor >= 3; // ES 3.0
    else
        caps.programBinary = caps.ctxMajor > 4 || (caps.ctxMajor == 4 && caps.ctxMinor >= 1); // 4.1

    if (caps.programBinary) {
        GLint fmtCount = 0;
        f->glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &fmtCount);
        if (fmtCount < 1)
            caps.programBinary = false;
    }

    caps.texture3D = caps.ctxMajor >= 3; // 3.0

    if (caps.gles)
        caps.texture1D = false; // ES
    else
        caps.texture1D = glTexImage1D && (caps.ctxMajor >= 2); // 2.0

    if (caps.gles)
        caps.tessellation = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 2); // ES 3.2
    else
        caps.tessellation = caps.ctxMajor >= 4; // 4.0

    if (caps.gles)
        caps.geometryShader = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 2); // ES 3.2
    else
        caps.geometryShader = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 2); // 3.2

    if (caps.ctxMajor >= 3) { // 3.0 or ES 3.0
        GLint maxArraySize = 0;
        f->glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArraySize);
        caps.maxTextureArraySize = maxArraySize;
    } else {
        caps.maxTextureArraySize = 0;
    }

    // The ES 2.0 spec only has MAX_xxxx_VECTORS. ES 3.0 and up has both
    // *VECTORS and *COMPONENTS. OpenGL 2.0-4.0 only has MAX_xxxx_COMPONENTS.
    // 4.1 and above has both. What a mess.
    if (caps.gles) {
        GLint maxVertexUniformVectors = 0;
        f->glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniformVectors);
        GLint maxFragmentUniformVectors = 0;
        f->glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxFragmentUniformVectors);
        caps.maxUniformVectors = qMin(maxVertexUniformVectors, maxFragmentUniformVectors);
    } else {
        GLint maxVertexUniformComponents = 0;
        f->glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniformComponents);
        GLint maxFragmentUniformComponents = 0;
        f->glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragmentUniformComponents);
        caps.maxUniformVectors = qMin(maxVertexUniformComponents, maxFragmentUniformComponents) / 4;
    }

    f->glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &caps.maxVertexInputs);

    if (caps.gles) {
        f->glGetIntegerv(GL_MAX_VARYING_VECTORS, &caps.maxVertexOutputs);
    } else if (caps.ctxMajor >= 3) {
        GLint components = 0;
        f->glGetIntegerv(caps.coreProfile ? GL_MAX_VERTEX_OUTPUT_COMPONENTS : GL_MAX_VARYING_COMPONENTS, &components);
        caps.maxVertexOutputs = components / 4;
    } else {
        // OpenGL before 3.0 only has this, and not the same as
        // MAX_VARYING_COMPONENTS strictly speaking, but will do.
        GLint components = 0;
        f->glGetIntegerv(GL_MAX_VARYING_FLOATS, &components);
        if (components > 0)
            caps.maxVertexOutputs = components / 4;
    }

    if (!caps.gles) {
        f->glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        if (!caps.coreProfile)
            f->glEnable(GL_POINT_SPRITE);
    } // else (with gles) these are always on

    // Match D3D and others when it comes to seamless cubemap filtering.
    // ES 3.0+ has this always enabled. (hopefully)
    // ES 2.0 and GL < 3.2 will not have it.
    if (!caps.gles && (caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 2)))
        f->glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    caps.halfAttributes = f->hasOpenGLExtension(QOpenGLExtensions::HalfFloatVertex);

    // We always require GL_OVR_multiview2 for symmetry with other backends.
    caps.multiView = f->hasOpenGLExtension(QOpenGLExtensions::MultiView)
                     && f->hasOpenGLExtension(QOpenGLExtensions::MultiViewExtended);
    if (caps.multiView) {
        glFramebufferTextureMultiviewOVR =
            reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLenum, GLenum, GLuint, GLint, GLint, GLsizei)>(
                ctx->getProcAddress(QByteArrayLiteral("glFramebufferTextureMultiviewOVR")));
    }

    // Only do timestamp queries on OpenGL 3.3+.
    caps.timestamps = !caps.gles && (caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 3));
    if (caps.timestamps) {
        glQueryCounter = reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLuint, GLenum)>(
            ctx->getProcAddress(QByteArrayLiteral("glQueryCounter")));
        glGetQueryObjectui64v = reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLuint, GLenum, quint64 *)>(
            ctx->getProcAddress(QByteArrayLiteral("glGetQueryObjectui64v")));
        if (!glQueryCounter || !glGetQueryObjectui64v)
            caps.timestamps = false;
    }

    // glObjectLabel is available on OpenGL ES 3.2+ and OpenGL 4.3+
    if (caps.gles)
        caps.objectLabel = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 2);
    else
        caps.objectLabel = caps.ctxMajor > 4 || (caps.ctxMajor == 4 && caps.ctxMinor >= 3);
    if (caps.objectLabel) {
        glObjectLabel = reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLenum, GLuint, GLsizei, const GLchar *)>(
                            ctx->getProcAddress(QByteArrayLiteral("glObjectLabel")));
    }

    if (caps.gles) {
        // This is the third way to get multisample rendering with GLES. (1. is
        // multisample render buffer -> resolve to texture; 2. is multisample
        // texture with GLES 3.1; 3. is this, avoiding the explicit multisample
        // buffer and should be more efficient with tiled architectures.
        // Interesting also because 2. does not seem to work in practice on
        // devices such as the Quest 3)
        caps.glesMultisampleRenderToTexture = ctx->hasExtension("GL_EXT_multisampled_render_to_texture");
        if (caps.glesMultisampleRenderToTexture) {
            glFramebufferTexture2DMultisampleEXT = reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint, GLsizei)>(
                ctx->getProcAddress(QByteArrayLiteral("glFramebufferTexture2DMultisampleEXT")));
            glRenderbufferStorageMultisampleEXT = reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLenum, GLsizei, GLenum, GLsizei, GLsizei)>(
                ctx->getProcAddress(QByteArrayLiteral("glRenderbufferStorageMultisampleEXT")));
        }
        caps.glesMultiviewMultisampleRenderToTexture = ctx->hasExtension("GL_OVR_multiview_multisampled_render_to_texture");
        if (caps.glesMultiviewMultisampleRenderToTexture) {
            glFramebufferTextureMultisampleMultiviewOVR = reinterpret_cast<void(QOPENGLF_APIENTRYP)(GLenum, GLenum, GLuint, GLint, GLsizei, GLint, GLsizei)>(
                ctx->getProcAddress(QByteArrayLiteral("glFramebufferTextureMultisampleMultiviewOVR")));
        }
    } else {
        caps.glesMultisampleRenderToTexture = false;
        caps.glesMultiviewMultisampleRenderToTexture = false;
    }

    caps.unpackRowLength = !caps.gles || caps.ctxMajor >= 3;

    if (caps.gles)
        caps.perRenderTargetBlending = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 2);
    else
        caps.perRenderTargetBlending = caps.ctxMajor >= 4;

    if (caps.gles) {
        if (caps.ctxMajor == 3 && caps.ctxMinor < 2) {
            caps.sampleVariables = ctx->hasExtension("GL_OES_sample_variables");
        } else {
            caps.sampleVariables = caps.ctxMajor > 3 || (caps.ctxMajor == 3 && caps.ctxMinor >= 2);
        }
    } else {
        caps.sampleVariables = caps.ctxMajor >= 4;
    }

    nativeHandlesStruct.context = ctx;

    contextLost = false;

    return true;
}

void QRhiGles2::destroy()
{
    if (!f)
        return;

    if (ensureContext()) {
        executeDeferredReleases();

        if (ofr.tsQueries[0]) {
            f->glDeleteQueries(2, ofr.tsQueries);
            ofr.tsQueries[0] = ofr.tsQueries[1] = 0;
        }

        if (vao) {
            f->glDeleteVertexArrays(1, &vao);
            vao = 0;
        }

        for (uint shader : m_shaderCache)
            f->glDeleteShader(shader);
        m_shaderCache.clear();
    }

    if (!importedContext) {
        delete ctx;
        ctx = nullptr;
    }

    f = nullptr;
}

void QRhiGles2::executeDeferredReleases()
{
    for (int i = releaseQueue.size() - 1; i >= 0; --i) {
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
            f->glDeleteTextures(1, &e.textureRenderTarget.nonMsaaThrowawayDepthTexture);
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
        releaseQueue.removeAt(i);
    }
}

QList<int> QRhiGles2::supportedSampleCounts() const
{
    if (supportedSampleCountList.isEmpty()) {
        // 1, 2, 4, 8, ...
        for (int i = 1; i <= caps.maxSamples; i *= 2)
            supportedSampleCountList.append(i);
    }
    return supportedSampleCountList;
}

QList<QSize> QRhiGles2::supportedShadingRates(int sampleCount) const
{
    Q_UNUSED(sampleCount);
    return { QSize(1, 1) };
}

QRhiSwapChain *QRhiGles2::createSwapChain()
{
    return new QGles2SwapChain(this);
}

QRhiBuffer *QRhiGles2::createBuffer(QRhiBuffer::Type type, QRhiBuffer::UsageFlags usage, quint32 size)
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

static inline void toGlTextureFormat(QRhiTexture::Format format, const QRhiGles2::Caps &caps,
                                     GLenum *glintformat, GLenum *glsizedintformat,
                                     GLenum *glformat, GLenum *gltype)
{
    switch (format) {
    case QRhiTexture::RGBA8:
        *glintformat = GL_RGBA;
        *glsizedintformat = caps.rgba8Format ? GL_RGBA8 : GL_RGBA;
        *glformat = GL_RGBA;
        *gltype = GL_UNSIGNED_BYTE;
        break;
    case QRhiTexture::BGRA8:
        *glintformat = caps.bgraInternalFormat ? GL_BGRA : GL_RGBA;
        *glsizedintformat = caps.rgba8Format ? GL_RGBA8 : GL_RGBA;
        *glformat = GL_BGRA;
        *gltype = GL_UNSIGNED_BYTE;
        break;
    case QRhiTexture::R16:
        *glintformat = GL_R16;
        *glsizedintformat = *glintformat;
        *glformat = GL_RED;
        *gltype = GL_UNSIGNED_SHORT;
        break;
    case QRhiTexture::RG16:
        *glintformat = GL_RG16;
        *glsizedintformat = *glintformat;
        *glformat = GL_RG;
        *gltype = GL_UNSIGNED_SHORT;
        break;
    case QRhiTexture::R8:
        *glintformat = GL_R8;
        *glsizedintformat = *glintformat;
        *glformat = GL_RED;
        *gltype = GL_UNSIGNED_BYTE;
        break;
    case QRhiTexture::R8SI:
        *glintformat = GL_R8I;
        *glsizedintformat = *glintformat;
        *glformat = GL_RED_INTEGER;
        *gltype = GL_BYTE;
        break;
    case QRhiTexture::R8UI:
        *glintformat = GL_R8UI;
        *glsizedintformat = *glintformat;
        *glformat = GL_RED_INTEGER;
        *gltype = GL_UNSIGNED_BYTE;
        break;
    case QRhiTexture::RG8:
        *glintformat = GL_RG8;
        *glsizedintformat = *glintformat;
        *glformat = GL_RG;
        *gltype = GL_UNSIGNED_BYTE;
        break;
    case QRhiTexture::RED_OR_ALPHA8:
        *glintformat = caps.coreProfile ? GL_R8 : GL_ALPHA;
        *glsizedintformat = *glintformat;
        *glformat = caps.coreProfile ? GL_RED : GL_ALPHA;
        *gltype = GL_UNSIGNED_BYTE;
        break;
    case QRhiTexture::RGBA16F:
        *glintformat = GL_RGBA16F;
        *glsizedintformat = *glintformat;
        *glformat = GL_RGBA;
        *gltype = GL_HALF_FLOAT;
        break;
    case QRhiTexture::RGBA32F:
        *glintformat = GL_RGBA32F;
        *glsizedintformat = *glintformat;
        *glformat = GL_RGBA;
        *gltype = GL_FLOAT;
        break;
    case QRhiTexture::R16F:
        *glintformat = GL_R16F;
        *glsizedintformat = *glintformat;
        *glformat = GL_RED;
        *gltype = GL_HALF_FLOAT;
        break;
    case QRhiTexture::R32F:
        *glintformat = GL_R32F;
        *glsizedintformat = *glintformat;
        *glformat = GL_RED;
        *gltype = GL_FLOAT;
        break;
    case QRhiTexture::RGB10A2:
        *glintformat = GL_RGB10_A2;
        *glsizedintformat = *glintformat;
        *glformat = GL_RGBA;
        *gltype = GL_UNSIGNED_INT_2_10_10_10_REV;
        break;
    case QRhiTexture::R32SI:
        *glintformat = GL_R32I;
        *glsizedintformat = *glintformat;
        *glformat = GL_RED_INTEGER;
        *gltype = GL_INT;
        break;
    case QRhiTexture::R32UI:
        *glintformat = GL_R32UI;
        *glsizedintformat = *glintformat;
        *glformat = GL_RED_INTEGER;
        *gltype = GL_UNSIGNED_INT;
        break;
    case QRhiTexture::RG32SI:
        *glintformat = GL_RG32I;
        *glsizedintformat = *glintformat;
        *glformat = GL_RG_INTEGER;
        *gltype = GL_INT;
        break;
    case QRhiTexture::RG32UI:
        *glintformat = GL_RG32UI;
        *glsizedintformat = *glintformat;
        *glformat = GL_RG_INTEGER;
        *gltype = GL_UNSIGNED_INT;
        break;
    case QRhiTexture::RGBA32SI:
        *glintformat = GL_RGBA32I;
        *glsizedintformat = *glintformat;
        *glformat = GL_RGBA_INTEGER;
        *gltype = GL_INT;
        break;
    case QRhiTexture::RGBA32UI:
        *glintformat = GL_RGBA32UI;
        *glsizedintformat = *glintformat;
        *glformat = GL_RGBA_INTEGER;
        *gltype = GL_UNSIGNED_INT;
        break;
    case QRhiTexture::D16:
        *glintformat = GL_DEPTH_COMPONENT16;
        *glsizedintformat = *glintformat;
        *glformat = GL_DEPTH_COMPONENT;
        *gltype = GL_UNSIGNED_SHORT;
        break;
    case QRhiTexture::D24:
        *glintformat = GL_DEPTH_COMPONENT24;
        *glsizedintformat = *glintformat;
        *glformat = GL_DEPTH_COMPONENT;
        *gltype = GL_UNSIGNED_INT;
        break;
    case QRhiTexture::D24S8:
        *glintformat = GL_DEPTH24_STENCIL8;
        *glsizedintformat = *glintformat;
        *glformat = GL_DEPTH_STENCIL;
        *gltype = GL_UNSIGNED_INT_24_8;
        break;
    case QRhiTexture::D32F:
        *glintformat = GL_DEPTH_COMPONENT32F;
        *glsizedintformat = *glintformat;
        *glformat = GL_DEPTH_COMPONENT;
        *gltype = GL_FLOAT;
        break;
    case QRhiTexture::D32FS8:
        *glintformat = GL_DEPTH32F_STENCIL8;
        *glsizedintformat = *glintformat;
        *glformat = GL_DEPTH_STENCIL;
        *gltype = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        break;
    default:
        Q_UNREACHABLE();
        *glintformat = GL_RGBA;
        *glsizedintformat = caps.rgba8Format ? GL_RGBA8 : GL_RGBA;
        *glformat = GL_RGBA;
        *gltype = GL_UNSIGNED_BYTE;
        break;
    }
}

bool QRhiGles2::isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const
{
    if (isCompressedFormat(format))
        return supportedCompressedFormats.contains(GLint(toGlCompressedTextureFormat(format, flags)));

    switch (format) {
    case QRhiTexture::D16:
    case QRhiTexture::D32F:
    case QRhiTexture::D32FS8:
        return caps.depthTexture;

    case QRhiTexture::D24:
        return caps.depth24;

    case QRhiTexture::D24S8:
        return caps.depth24 && caps.packedDepthStencil;

    case QRhiTexture::BGRA8:
        return caps.bgraExternalFormat;

    case QRhiTexture::R8:
    case QRhiTexture::R8SI:
    case QRhiTexture::R8UI:
        return caps.r8Format;

    case QRhiTexture::R32SI:
    case QRhiTexture::R32UI:
    case QRhiTexture::RG32SI:
    case QRhiTexture::RG32UI:
    case QRhiTexture::RGBA32SI:
    case QRhiTexture::RGBA32UI:
        return caps.r32uiFormat;

    case QRhiTexture::RG8:
        return caps.r8Format;

    case QRhiTexture::R16:
        return caps.r16Format;

    case QRhiTexture::RG16:
        return caps.r16Format;

    case QRhiTexture::RGBA16F:
    case QRhiTexture::RGBA32F:
        return caps.floatFormats;

    case QRhiTexture::R16F:
    case QRhiTexture::R32F:
        return caps.floatFormats;

    case QRhiTexture::RGB10A2:
        return caps.rgb10Formats;

    default:
        break;
    }

    return true;
}

bool QRhiGles2::isFeatureSupported(QRhi::Feature feature) const
{
    switch (feature) {
    case QRhi::MultisampleTexture:
        return caps.multisampledTexture;
    case QRhi::MultisampleRenderBuffer:
        return caps.msaaRenderBuffer;
    case QRhi::DebugMarkers:
        return false;
    case QRhi::Timestamps:
        return caps.timestamps;
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
        return !caps.coreProfile;
    case QRhi::VertexShaderPointSize:
        return true;
    case QRhi::BaseVertex:
        return caps.baseVertex;
    case QRhi::BaseInstance:
        // The glDraw*BaseInstance variants of draw calls are not in GLES 3.2,
        // so won't bother, even though they would be available in GL 4.2+.
        // Furthermore, not supporting this avoids having to deal with the
        // gl_InstanceID vs gl_InstanceIndex mess in shaders and in the shader
        // transpiling pipeline.
        return false;
    case QRhi::TriangleFanTopology:
        return true;
    case QRhi::ReadBackNonUniformBuffer:
        return !caps.gles || caps.properMapBuffer;
    case QRhi::ReadBackNonBaseMipLevel:
        return caps.nonBaseLevelFramebufferTexture;
    case QRhi::TexelFetch:
        return caps.texelFetch;
    case QRhi::RenderToNonBaseMipLevel:
        return caps.nonBaseLevelFramebufferTexture;
    case QRhi::IntAttributes:
        return caps.intAttributes;
    case QRhi::ScreenSpaceDerivatives:
        return caps.screenSpaceDerivatives;
    case QRhi::ReadBackAnyTextureFormat:
        return false;
    case QRhi::PipelineCacheDataLoadSave:
        return caps.programBinary;
    case QRhi::ImageDataStride:
        return caps.unpackRowLength;
    case QRhi::RenderBufferImport:
        return true;
    case QRhi::ThreeDimensionalTextures:
        return caps.texture3D;
    case QRhi::RenderTo3DTextureSlice:
        return caps.texture3D;
    case QRhi::TextureArrays:
        return caps.maxTextureArraySize > 0;
    case QRhi::Tessellation:
        return caps.tessellation;
    case QRhi::GeometryShader:
        return caps.geometryShader;
    case QRhi::TextureArrayRange:
        return false;
    case QRhi::NonFillPolygonMode:
        return !caps.gles;
    case QRhi::OneDimensionalTextures:
        return caps.texture1D;
    case QRhi::OneDimensionalTextureMipmaps:
        return caps.texture1D;
    case QRhi::HalfAttributes:
        return caps.halfAttributes;
    case QRhi::RenderToOneDimensionalTexture:
        return caps.texture1D;
    case QRhi::ThreeDimensionalTextureMipmaps:
        return caps.texture3D;
    case QRhi::MultiView:
        return caps.multiView && caps.maxTextureArraySize > 0;
    case QRhi::TextureViewFormat:
        return false;
    case QRhi::ResolveDepthStencil:
        return true;
    case QRhi::VariableRateShading:
        return false;
    case QRhi::VariableRateShadingMap:
    case QRhi::VariableRateShadingMapWithTexture:
        return false;
    case QRhi::PerRenderTargetBlending:
        return caps.perRenderTargetBlending;
    case QRhi::SampleVariables:
        return caps.sampleVariables;
    default:
        Q_UNREACHABLE_RETURN(false);
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
    case QRhi::MaxThreadGroupsPerDimension:
        return caps.maxThreadGroupsPerDimension;
    case QRhi::MaxThreadsPerThreadGroup:
        return caps.maxThreadsPerThreadGroup;
    case QRhi::MaxThreadGroupX:
        return caps.maxThreadGroupsX;
    case QRhi::MaxThreadGroupY:
        return caps.maxThreadGroupsY;
    case QRhi::MaxThreadGroupZ:
        return caps.maxThreadGroupsZ;
    case QRhi::TextureArraySizeMax:
        return 2048;
    case QRhi::MaxUniformBufferRange:
        return int(qMin<qint64>(INT_MAX, caps.maxUniformVectors * qint64(16)));
    case QRhi::MaxVertexInputs:
        return caps.maxVertexInputs;
    case QRhi::MaxVertexOutputs:
        return caps.maxVertexOutputs;
    case QRhi::ShadingRateImageTileSize:
        return 0;
    default:
        Q_UNREACHABLE_RETURN(0);
    }
}

const QRhiNativeHandles *QRhiGles2::nativeHandles()
{
    return &nativeHandlesStruct;
}

QRhiDriverInfo QRhiGles2::driverInfo() const
{
    return driverInfoStruct;
}

QRhiStats QRhiGles2::statistics()
{
    QRhiStats result;
    result.totalPipelineCreationTime = totalPipelineCreationTime();
    return result;
}

bool QRhiGles2::makeThreadLocalNativeContextCurrent()
{
    if (inFrame && !ofr.active)
        return ensureContext(currentSwapChain->surface);
    else
        return ensureContext();
}

void QRhiGles2::setQueueSubmitParams(QRhiNativeHandles *)
{
    // not applicable
}

void QRhiGles2::releaseCachedResources()
{
    if (!ensureContext())
        return;

    for (uint shader : m_shaderCache)
        f->glDeleteShader(shader);

    m_shaderCache.clear();

    m_pipelineCache.clear();
}

bool QRhiGles2::isDeviceLost() const
{
    return contextLost;
}

struct QGles2PipelineCacheDataHeader
{
    quint32 rhiId;
    quint32 arch;
    quint32 programBinaryCount;
    quint32 dataSize;
    char driver[240];
};

QByteArray QRhiGles2::pipelineCacheData()
{
    Q_STATIC_ASSERT(sizeof(QGles2PipelineCacheDataHeader) == 256);

    if (m_pipelineCache.isEmpty())
        return QByteArray();

    QGles2PipelineCacheDataHeader header;
    memset(&header, 0, sizeof(header));
    header.rhiId = pipelineCacheRhiId();
    header.arch = quint32(sizeof(void*));
    header.programBinaryCount = m_pipelineCache.size();
    const size_t driverStrLen = qMin(sizeof(header.driver) - 1, size_t(driverInfoStruct.deviceName.size()));
    if (driverStrLen)
        memcpy(header.driver, driverInfoStruct.deviceName.constData(), driverStrLen);
    header.driver[driverStrLen] = '\0';

    const size_t dataOffset = sizeof(header);
    size_t dataSize = 0;
    for (auto it = m_pipelineCache.cbegin(), end = m_pipelineCache.cend(); it != end; ++it) {
        dataSize += sizeof(quint32) + it.key().size()
                  + sizeof(quint32) + it->data.size()
                  + sizeof(quint32);
    }

    QByteArray buf(dataOffset + dataSize, Qt::Uninitialized);
    char *p = buf.data() + dataOffset;
    for (auto it = m_pipelineCache.cbegin(), end = m_pipelineCache.cend(); it != end; ++it) {
        const QByteArray key = it.key();
        const QByteArray data = it->data;
        const quint32 format = it->format;

        quint32 i = key.size();
        memcpy(p, &i, 4);
        p += 4;
        memcpy(p, key.constData(), key.size());
        p += key.size();

        i = data.size();
        memcpy(p, &i, 4);
        p += 4;
        memcpy(p, data.constData(), data.size());
        p += data.size();

        memcpy(p, &format, 4);
        p += 4;
    }
    Q_ASSERT(p == buf.data() + dataOffset + dataSize);

    header.dataSize = quint32(dataSize);
    memcpy(buf.data(), &header, sizeof(header));

    return buf;
}

void QRhiGles2::setPipelineCacheData(const QByteArray &data)
{
    if (data.isEmpty())
        return;

    const size_t headerSize = sizeof(QGles2PipelineCacheDataHeader);
    if (data.size() < qsizetype(headerSize)) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: Invalid blob size (header incomplete)");
        return;
    }
    const size_t dataOffset = headerSize;
    QGles2PipelineCacheDataHeader header;
    memcpy(&header, data.constData(), headerSize);

    const quint32 rhiId = pipelineCacheRhiId();
    if (header.rhiId != rhiId) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: The data is for a different QRhi version or backend (%u, %u)",
                rhiId, header.rhiId);
        return;
    }
    const quint32 arch = quint32(sizeof(void*));
    if (header.arch != arch) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: Architecture does not match (%u, %u)",
                arch, header.arch);
        return;
    }
    if (header.programBinaryCount == 0)
        return;

    const size_t driverStrLen = qMin(sizeof(header.driver) - 1, size_t(driverInfoStruct.deviceName.size()));
    if (strncmp(header.driver, driverInfoStruct.deviceName.constData(), driverStrLen)) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: OpenGL vendor/renderer/version does not match");
        return;
    }

    if (data.size() < qsizetype(dataOffset + header.dataSize)) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: Invalid blob size (data incomplete)");
        return;
    }

    m_pipelineCache.clear();

    const char *p = data.constData() + dataOffset;
    for (quint32 i = 0; i < header.programBinaryCount; ++i) {
        quint32 len = 0;
        memcpy(&len, p, 4);
        p += 4;
        QByteArray key(len, Qt::Uninitialized);
        memcpy(key.data(), p, len);
        p += len;

        memcpy(&len, p, 4);
        p += 4;
        QByteArray data(len, Qt::Uninitialized);
        memcpy(data.data(), p, len);
        p += len;

        quint32 format;
        memcpy(&format, p, 4);
        p += 4;

        m_pipelineCache.insert(key, { format, data });
    }

    qCDebug(QRHI_LOG_INFO, "Seeded pipeline cache with %d program binaries", int(m_pipelineCache.size()));
}

QRhiRenderBuffer *QRhiGles2::createRenderBuffer(QRhiRenderBuffer::Type type, const QSize &pixelSize,
                                                int sampleCount, QRhiRenderBuffer::Flags flags,
                                                QRhiTexture::Format backingFormatHint)
{
    return new QGles2RenderBuffer(this, type, pixelSize, sampleCount, flags, backingFormatHint);
}

QRhiTexture *QRhiGles2::createTexture(QRhiTexture::Format format,
                                      const QSize &pixelSize, int depth, int arraySize,
                                      int sampleCount, QRhiTexture::Flags flags)
{
    return new QGles2Texture(this, format, pixelSize, depth, arraySize, sampleCount, flags);
}

QRhiSampler *QRhiGles2::createSampler(QRhiSampler::Filter magFilter, QRhiSampler::Filter minFilter,
                                      QRhiSampler::Filter mipmapMode,
                                      QRhiSampler::AddressMode u, QRhiSampler::AddressMode v, QRhiSampler::AddressMode w)
{
    return new QGles2Sampler(this, magFilter, minFilter, mipmapMode, u, v, w);
}

QRhiShadingRateMap *QRhiGles2::createShadingRateMap()
{
    return nullptr;
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
        if (psD->lastUsedInFrameNo != frameNo) {
            psD->lastUsedInFrameNo = frameNo;
            psD->currentSrb = nullptr;
            psD->currentSrbGeneration = 0;
        }

        QGles2CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QGles2CommandBuffer::Command::BindGraphicsPipeline;
        cmd.args.bindGraphicsPipeline.ps = ps;
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

    QGles2ShaderResourceBindings *srbD = QRHI_RES(QGles2ShaderResourceBindings, srb);
    if (cbD->passNeedsResourceTracking) {
        QRhiPassResourceTracker &passResTracker(cbD->passResTrackers[cbD->currentPassResTrackerIndex]);
        for (int i = 0, ie = srbD->m_bindings.size(); i != ie; ++i) {
            const QRhiShaderResourceBinding::Data *b = shaderResourceBindingData(srbD->m_bindings.at(i));
            switch (b->type) {
            case QRhiShaderResourceBinding::UniformBuffer:
                // no BufUniformRead / AccessUniform because no real uniform buffers are used
                break;
            case QRhiShaderResourceBinding::SampledTexture:
            case QRhiShaderResourceBinding::Texture:
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
    }

    bool srbChanged = gfxPsD ? (cbD->currentGraphicsSrb != srb) : (cbD->currentComputeSrb != srb);

    // The Command::BindShaderResources command generated below is what will
    // cause uniforms to be set (glUniformNxx). This needs some special
    // handling here in this backend without real uniform buffers, because,
    // like in other backends, we optimize out the setShaderResources when the
    // srb that was set before is attempted to be set again on the command
    // buffer, but that is incorrect if the same srb is now used with another
    // pipeline. (because that could mean a glUseProgram not followed by
    // up-to-date glUniform calls, i.e. with GL we have a strong dependency
    // between the pipeline (== program) and the srb, unlike other APIs) This
    // is the reason there is a second level of srb(+generation) tracking in
    // the pipeline objects.
    if (gfxPsD && (gfxPsD->currentSrb != srb || gfxPsD->currentSrbGeneration != srbD->generation)) {
        srbChanged = true;
        gfxPsD->currentSrb = srb;
        gfxPsD->currentSrbGeneration = srbD->generation;
    } else if (compPsD && (compPsD->currentSrb != srb || compPsD->currentSrbGeneration != srbD->generation)) {
        srbChanged = true;
        compPsD->currentSrb = srb;
        compPsD->currentSrbGeneration = srbD->generation;
    }

    if (srbChanged || cbD->currentSrbGeneration != srbD->generation || srbD->hasDynamicOffset) {
        if (gfxPsD) {
            cbD->currentGraphicsSrb = srb;
            cbD->currentComputeSrb = nullptr;
        } else {
            cbD->currentGraphicsSrb = nullptr;
            cbD->currentComputeSrb = srb;
        }
        cbD->currentSrbGeneration = srbD->generation;

        QGles2CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QGles2CommandBuffer::Command::BindShaderResources;
        cmd.args.bindShaderResources.maybeGraphicsPs = gfxPsD;
        cmd.args.bindShaderResources.maybeComputePs = compPsD;
        cmd.args.bindShaderResources.srb = srb;
        cmd.args.bindShaderResources.dynamicOffsetCount = 0;
        if (srbD->hasDynamicOffset) {
            if (dynamicOffsetCount < QGles2CommandBuffer::MAX_DYNAMIC_OFFSET_COUNT) {
                cmd.args.bindShaderResources.dynamicOffsetCount = dynamicOffsetCount;
                uint *p = cmd.args.bindShaderResources.dynamicOffsetPairs;
                for (int i = 0; i < dynamicOffsetCount; ++i) {
                    const QRhiCommandBuffer::DynamicOffset &dynOfs(dynamicOffsets[i]);
                    *p++ = uint(dynOfs.first);
                    *p++ = dynOfs.second;
                }
            } else {
                qWarning("Too many dynamic offsets (%d, max is %d)",
                         dynamicOffsetCount, QGles2CommandBuffer::MAX_DYNAMIC_OFFSET_COUNT);
            }
        }
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

        QGles2CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QGles2CommandBuffer::Command::BindVertexBuffer;
        cmd.args.bindVertexBuffer.ps = cbD->currentGraphicsPipeline;
        cmd.args.bindVertexBuffer.buffer = bufD->buffer;
        cmd.args.bindVertexBuffer.offset = ofs;
        cmd.args.bindVertexBuffer.binding = startBinding + i;

        if (cbD->passNeedsResourceTracking) {
            trackedRegisterBuffer(&passResTracker, bufD, QRhiPassResourceTracker::BufVertexInput,
                                  QRhiPassResourceTracker::BufVertexInputStage);
        }
    }

    if (indexBuf) {
        QGles2Buffer *ibufD = QRHI_RES(QGles2Buffer, indexBuf);
        Q_ASSERT(ibufD->m_usage.testFlag(QRhiBuffer::IndexBuffer));

        QGles2CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QGles2CommandBuffer::Command::BindIndexBuffer;
        cmd.args.bindIndexBuffer.buffer = ibufD->buffer;
        cmd.args.bindIndexBuffer.offset = indexOffset;
        cmd.args.bindIndexBuffer.type = indexFormat == QRhiCommandBuffer::IndexUInt16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

        if (cbD->passNeedsResourceTracking) {
            trackedRegisterBuffer(&passResTracker, ibufD, QRhiPassResourceTracker::BufIndexRead,
                                  QRhiPassResourceTracker::BufVertexInputStage);
        }
    }
}

void QRhiGles2::setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    const std::array<float, 4> r = viewport.viewport();
    // A negative width or height is an error. A negative x or y is not.
    if (r[2] < 0.0f || r[3] < 0.0f)
        return;

    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QGles2CommandBuffer::Command::Viewport;
    cmd.args.viewport.x = r[0];
    cmd.args.viewport.y = r[1];
    cmd.args.viewport.w = r[2];
    cmd.args.viewport.h = r[3];
    cmd.args.viewport.d0 = viewport.minDepth();
    cmd.args.viewport.d1 = viewport.maxDepth();
}

void QRhiGles2::setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    const std::array<int, 4> r = scissor.scissor();
    // A negative width or height is an error. A negative x or y is not.
    if (r[2] < 0 || r[3] < 0)
        return;

    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QGles2CommandBuffer::Command::Scissor;
    cmd.args.scissor.x = r[0];
    cmd.args.scissor.y = r[1];
    cmd.args.scissor.w = r[2];
    cmd.args.scissor.h = r[3];
}

void QRhiGles2::setBlendConstants(QRhiCommandBuffer *cb, const QColor &c)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QGles2CommandBuffer::Command::BlendConstants;
    cmd.args.blendConstants.r = c.redF();
    cmd.args.blendConstants.g = c.greenF();
    cmd.args.blendConstants.b = c.blueF();
    cmd.args.blendConstants.a = c.alphaF();
}

void QRhiGles2::setStencilRef(QRhiCommandBuffer *cb, quint32 refValue)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QGles2CommandBuffer::Command::StencilRef;
    cmd.args.stencilRef.ref = refValue;
    cmd.args.stencilRef.ps = cbD->currentGraphicsPipeline;
}

void QRhiGles2::setShadingRate(QRhiCommandBuffer *cb, const QSize &coarsePixelSize)
{
    Q_UNUSED(cb);
    Q_UNUSED(coarsePixelSize);
}

void QRhiGles2::draw(QRhiCommandBuffer *cb, quint32 vertexCount,
                     quint32 instanceCount, quint32 firstVertex, quint32 firstInstance)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QGles2CommandBuffer::Command::Draw;
    cmd.args.draw.ps = cbD->currentGraphicsPipeline;
    cmd.args.draw.vertexCount = vertexCount;
    cmd.args.draw.firstVertex = firstVertex;
    cmd.args.draw.instanceCount = instanceCount;
    cmd.args.draw.baseInstance = firstInstance;
}

void QRhiGles2::drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                            quint32 instanceCount, quint32 firstIndex, qint32 vertexOffset, quint32 firstInstance)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QGles2CommandBuffer::Command::DrawIndexed;
    cmd.args.drawIndexed.ps = cbD->currentGraphicsPipeline;
    cmd.args.drawIndexed.indexCount = indexCount;
    cmd.args.drawIndexed.firstIndex = firstIndex;
    cmd.args.drawIndexed.instanceCount = instanceCount;
    cmd.args.drawIndexed.baseInstance = firstInstance;
    cmd.args.drawIndexed.baseVertex = vertexOffset;
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

static inline void addBoundaryCommand(QGles2CommandBuffer *cbD, QGles2CommandBuffer::Command::Cmd type, GLuint tsQuery = 0)
{
    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = type;
    if (type == QGles2CommandBuffer::Command::BeginFrame)
        cmd.args.beginFrame.timestampQuery = tsQuery;
    else if (type == QGles2CommandBuffer::Command::EndFrame)
        cmd.args.endFrame.timestampQuery = tsQuery;
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
        QGles2CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QGles2CommandBuffer::Command::Barrier;
        cmd.args.barrier.barriers = GL_ALL_BARRIER_BITS;
    }

    executeCommandBuffer(cbD);

    cbD->resetCommands();

    if (vao) {
        f->glBindVertexArray(0);
    } else {
        f->glBindBuffer(GL_ARRAY_BUFFER, 0);
        f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        if (caps.compute)
            f->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
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

double QRhiGles2::lastCompletedGpuTime(QRhiCommandBuffer *cb)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    return cbD->lastGpuTime;
}

QRhi::FrameOpResult QRhiGles2::beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags)
{
    QGles2SwapChain *swapChainD = QRHI_RES(QGles2SwapChain, swapChain);
    if (!ensureContext(swapChainD->surface))
        return contextLost ? QRhi::FrameOpDeviceLost : QRhi::FrameOpError;

    ctx->handle()->beginFrame();

    currentSwapChain = swapChainD;

    executeDeferredReleases();
    swapChainD->cb.resetState();
    frameNo += 1;

    if (swapChainD->timestamps.active[swapChainD->currentTimestampPairIndex]) {
        double elapsedSec = 0;
        if (swapChainD->timestamps.tryQueryTimestamps(swapChainD->currentTimestampPairIndex, this, &elapsedSec))
            swapChainD->cb.lastGpuTime = elapsedSec;
    }

    GLuint tsStart = swapChainD->timestamps.query[swapChainD->currentTimestampPairIndex * 2];
    GLuint tsEnd = swapChainD->timestamps.query[swapChainD->currentTimestampPairIndex * 2 + 1];
    const bool recordTimestamps = tsStart && tsEnd && !swapChainD->timestamps.active[swapChainD->currentTimestampPairIndex];

    addBoundaryCommand(&swapChainD->cb, QGles2CommandBuffer::Command::BeginFrame, recordTimestamps ? tsStart : 0);

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiGles2::endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags)
{
    QGles2SwapChain *swapChainD = QRHI_RES(QGles2SwapChain, swapChain);
    Q_ASSERT(currentSwapChain == swapChainD);

    GLuint tsStart = swapChainD->timestamps.query[swapChainD->currentTimestampPairIndex * 2];
    GLuint tsEnd = swapChainD->timestamps.query[swapChainD->currentTimestampPairIndex * 2 + 1];
    const bool recordTimestamps = tsStart && tsEnd && !swapChainD->timestamps.active[swapChainD->currentTimestampPairIndex];
    if (recordTimestamps) {
        swapChainD->timestamps.active[swapChainD->currentTimestampPairIndex] = true;
        swapChainD->currentTimestampPairIndex = (swapChainD->currentTimestampPairIndex + 1) % QGles2SwapChainTimestamps::TIMESTAMP_PAIRS;
    }

    addBoundaryCommand(&swapChainD->cb, QGles2CommandBuffer::Command::EndFrame, recordTimestamps ? tsEnd : 0);

    if (!ensureContext(swapChainD->surface))
        return contextLost ? QRhi::FrameOpDeviceLost : QRhi::FrameOpError;

    executeCommandBuffer(&swapChainD->cb);

    if (swapChainD->surface && !flags.testFlag(QRhi::SkipPresent)) {
        ctx->swapBuffers(swapChainD->surface);
        needsMakeCurrentDueToSwap = true;
    } else {
        f->glFlush();
    }

    currentSwapChain = nullptr;

    ctx->handle()->endFrame();

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiGles2::beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags)
{
    if (!ensureContext())
        return contextLost ? QRhi::FrameOpDeviceLost : QRhi::FrameOpError;

    ofr.active = true;

    executeDeferredReleases();
    ofr.cbWrapper.resetState();

    if (rhiFlags.testFlag(QRhi::EnableTimestamps) && caps.timestamps) {
        if (!ofr.tsQueries[0])
            f->glGenQueries(2, ofr.tsQueries);
    }

    addBoundaryCommand(&ofr.cbWrapper, QGles2CommandBuffer::Command::BeginFrame, ofr.tsQueries[0]);
    *cb = &ofr.cbWrapper;

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiGles2::endOffscreenFrame(QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    Q_ASSERT(ofr.active);
    ofr.active = false;

    addBoundaryCommand(&ofr.cbWrapper, QGles2CommandBuffer::Command::EndFrame, ofr.tsQueries[1]);

    if (!ensureContext())
        return contextLost ? QRhi::FrameOpDeviceLost : QRhi::FrameOpError;

    executeCommandBuffer(&ofr.cbWrapper);

    // Just as endFrame() does a flush when skipping the swapBuffers(), do it
    // here as well. This has the added benefit of playing nice when rendering
    // to a texture from a context and then consuming that texture from
    // another, sharing context.
    f->glFlush();

    if (ofr.tsQueries[0]) {
        quint64 timestamps[2];
        glGetQueryObjectui64v(ofr.tsQueries[1], GL_QUERY_RESULT, &timestamps[1]);
        glGetQueryObjectui64v(ofr.tsQueries[0], GL_QUERY_RESULT, &timestamps[0]);
        if (timestamps[1] >= timestamps[0]) {
            const quint64 nanoseconds = timestamps[1] - timestamps[0];
            ofr.cbWrapper.lastGpuTime = nanoseconds / 1000000000.0; // seconds
        }
    }

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
        // Do an actual glFinish(). May seem superfluous, but this is what
        // matches most other backends e.g. Vulkan/Metal that do a heavyweight
        // wait-for-idle blocking in their finish(). More importantly, this
        // allows clients simply call finish() in threaded or shared context
        // situations where one explicitly needs to do a glFlush or Finish.
        f->glFinish();
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

static inline GLbitfield barriersForBuffer()
{
    return GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT
        | GL_ELEMENT_ARRAY_BARRIER_BIT
        | GL_UNIFORM_BARRIER_BIT
        | GL_BUFFER_UPDATE_BARRIER_BIT
        | GL_SHADER_STORAGE_BARRIER_BIT;
}

static inline GLbitfield barriersForTexture()
{
    return GL_TEXTURE_FETCH_BARRIER_BIT
        | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
        | GL_PIXEL_BUFFER_BARRIER_BIT
        | GL_TEXTURE_UPDATE_BARRIER_BIT
        | GL_FRAMEBUFFER_BARRIER_BIT;
}

void QRhiGles2::trackedBufferBarrier(QGles2CommandBuffer *cbD, QGles2Buffer *bufD, QGles2Buffer::Access access)
{
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::NoPass); // this is for resource updates only
    if (!bufD->m_usage.testFlag(QRhiBuffer::StorageBuffer))
        return;

    const QGles2Buffer::Access prevAccess = bufD->usageState.access;
    if (access == prevAccess)
        return;

    if (bufferAccessIsWrite(prevAccess)) {
        // Generating the minimal barrier set is way too complicated to do
        // correctly (prevAccess is overwritten so we won't have proper
        // tracking across multiple passes) so setting all barrier bits will do
        // for now.
        QGles2CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QGles2CommandBuffer::Command::Barrier;
        cmd.args.barrier.barriers = barriersForBuffer();
    }

    bufD->usageState.access = access;
}

void QRhiGles2::trackedImageBarrier(QGles2CommandBuffer *cbD, QGles2Texture *texD, QGles2Texture::Access access)
{
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::NoPass); // this is for resource updates only
    if (!texD->m_flags.testFlag(QRhiTexture::UsedWithLoadStore))
        return;

    const QGles2Texture::Access prevAccess = texD->usageState.access;
    if (access == prevAccess)
        return;

    if (textureAccessIsWrite(prevAccess)) {
        QGles2CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QGles2CommandBuffer::Command::Barrier;
        cmd.args.barrier.barriers = barriersForTexture();
    }

    texD->usageState.access = access;
}

void QRhiGles2::enqueueSubresUpload(QGles2Texture *texD, QGles2CommandBuffer *cbD,
                                    int layer, int level, const QRhiTextureSubresourceUploadDescription &subresDesc)
{
    trackedImageBarrier(cbD, texD, QGles2Texture::AccessUpdate);
    const bool isCompressed = isCompressedFormat(texD->m_format);
    const bool isCubeMap = texD->m_flags.testFlag(QRhiTexture::CubeMap);
    const bool is3D = texD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
    const bool is1D = texD->m_flags.testFlag(QRhiTexture::OneDimensional);
    const bool isArray = texD->m_flags.testFlag(QRhiTexture::TextureArray);
    const GLenum faceTargetBase = isCubeMap ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : texD->target;
    const GLenum effectiveTarget = faceTargetBase + (isCubeMap ? uint(layer) : 0u);
    const QPoint dp = subresDesc.destinationTopLeft();
    const QByteArray rawData = subresDesc.data();

    auto setCmdByNotCompressedData = [&](const void* data, QSize size, quint32 dataStride)
    {
        quint32 bytesPerLine = 0;
        quint32 bytesPerPixel = 0;
        textureFormatInfo(texD->m_format, size, &bytesPerLine, nullptr, &bytesPerPixel);

        QGles2CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QGles2CommandBuffer::Command::SubImage;
        cmd.args.subImage.target = texD->target;
        cmd.args.subImage.texture = texD->texture;
        cmd.args.subImage.faceTarget = effectiveTarget;
        cmd.args.subImage.level = level;
        cmd.args.subImage.dx = dp.x();
        cmd.args.subImage.dy = is1D && isArray ? layer : dp.y();
        cmd.args.subImage.dz = is3D || isArray ? layer : 0;
        cmd.args.subImage.w = size.width();
        cmd.args.subImage.h = size.height();
        cmd.args.subImage.glformat = texD->glformat;
        cmd.args.subImage.gltype = texD->gltype;

        if (dataStride == 0)
            dataStride = bytesPerLine;

        cmd.args.subImage.rowStartAlign = (dataStride & 3) ? 1 : 4;
        cmd.args.subImage.rowLength = caps.unpackRowLength ? (bytesPerPixel ? dataStride / bytesPerPixel : 0) : 0;

        cmd.args.subImage.data = data;
    };

    if (!subresDesc.image().isNull()) {
        QImage img = subresDesc.image();
        QSize size = img.size();
        if (!subresDesc.sourceSize().isEmpty() || !subresDesc.sourceTopLeft().isNull()) {
            const QPoint sp = subresDesc.sourceTopLeft();
            if (!subresDesc.sourceSize().isEmpty())
                size = subresDesc.sourceSize();

            if (caps.unpackRowLength) {
                cbD->retainImage(img);
                // create a non-owning wrapper for the subimage
                const uchar *data = img.constBits() + sp.y() * img.bytesPerLine() + sp.x() * (qMax(1, img.depth() / 8));
                img = QImage(data, size.width(), size.height(), img.bytesPerLine(), img.format());
            } else {
                img = img.copy(sp.x(), sp.y(), size.width(), size.height());
            }
        }

        setCmdByNotCompressedData(cbD->retainImage(img), size, img.bytesPerLine());
    } else if (!rawData.isEmpty() && isCompressed) {
        const int depth = qMax(1, texD->m_depth);
        const int arraySize = qMax(0, texD->m_arraySize);
        if ((texD->flags().testFlag(QRhiTexture::UsedAsCompressedAtlas) || is3D || isArray)
                && !texD->zeroInitialized)
        {
            // Create on first upload since glCompressedTexImage2D cannot take
            // nullptr data. We have a rule in the QRhi docs that the first
            // upload for a compressed texture must cover the entire image, but
            // that is clearly not ideal when building a texture atlas, or when
            // having a 3D texture with per-slice data.
            quint32 byteSize = 0;
            compressedFormatInfo(texD->m_format, texD->m_pixelSize, nullptr, &byteSize, nullptr);
            if (is3D)
                byteSize *= depth;
            if (isArray)
                byteSize *= arraySize;
            QByteArray zeroBuf(byteSize, 0);
            QGles2CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QGles2CommandBuffer::Command::CompressedImage;
            cmd.args.compressedImage.target = texD->target;
            cmd.args.compressedImage.texture = texD->texture;
            cmd.args.compressedImage.faceTarget = effectiveTarget;
            cmd.args.compressedImage.level = level;
            cmd.args.compressedImage.glintformat = texD->glintformat;
            cmd.args.compressedImage.w = texD->m_pixelSize.width();
            cmd.args.compressedImage.h = is1D && isArray ? arraySize : texD->m_pixelSize.height();
            cmd.args.compressedImage.depth = is3D ? depth : (isArray ? arraySize : 0);
            cmd.args.compressedImage.size = byteSize;
            cmd.args.compressedImage.data = cbD->retainData(zeroBuf);
            texD->zeroInitialized = true;
        }

        const QSize size = subresDesc.sourceSize().isEmpty() ? q->sizeForMipLevel(level, texD->m_pixelSize)
                                                             : subresDesc.sourceSize();
        if (texD->specified || texD->zeroInitialized) {
            QGles2CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QGles2CommandBuffer::Command::CompressedSubImage;
            cmd.args.compressedSubImage.target = texD->target;
            cmd.args.compressedSubImage.texture = texD->texture;
            cmd.args.compressedSubImage.faceTarget = effectiveTarget;
            cmd.args.compressedSubImage.level = level;
            cmd.args.compressedSubImage.dx = dp.x();
            cmd.args.compressedSubImage.dy = is1D && isArray ? layer : dp.y();
            cmd.args.compressedSubImage.dz = is3D || isArray ? layer : 0;
            cmd.args.compressedSubImage.w = size.width();
            cmd.args.compressedSubImage.h = size.height();
            cmd.args.compressedSubImage.glintformat = texD->glintformat;
            cmd.args.compressedSubImage.size = rawData.size();
            cmd.args.compressedSubImage.data = cbD->retainData(rawData);
        } else {
            QGles2CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QGles2CommandBuffer::Command::CompressedImage;
            cmd.args.compressedImage.target = texD->target;
            cmd.args.compressedImage.texture = texD->texture;
            cmd.args.compressedImage.faceTarget = effectiveTarget;
            cmd.args.compressedImage.level = level;
            cmd.args.compressedImage.glintformat = texD->glintformat;
            cmd.args.compressedImage.w = size.width();
            cmd.args.compressedImage.h = is1D && isArray ? arraySize : size.height();
            cmd.args.compressedImage.depth = is3D ? depth : (isArray ? arraySize : 0);
            cmd.args.compressedImage.size = rawData.size();
            cmd.args.compressedImage.data = cbD->retainData(rawData);
        }
    } else if (!rawData.isEmpty()) {
        const QSize size = subresDesc.sourceSize().isEmpty() ? q->sizeForMipLevel(level, texD->m_pixelSize)
                                                             : subresDesc.sourceSize();

        setCmdByNotCompressedData(cbD->retainData(rawData), size, subresDesc.dataStride());
    } else {
        qWarning("Invalid texture upload for %p layer=%d mip=%d", texD, layer, level);
    }
}

void QRhiGles2::enqueueResourceUpdates(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    QRhiResourceUpdateBatchPrivate *ud = QRhiResourceUpdateBatchPrivate::get(resourceUpdates);

    for (int opIdx = 0; opIdx < ud->activeBufferOpCount; ++opIdx) {
        const QRhiResourceUpdateBatchPrivate::BufferOp &u(ud->bufferOps[opIdx]);
        if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::DynamicUpdate) {
            QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, u.buf);
            Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
            if (bufD->m_usage.testFlag(QRhiBuffer::UniformBuffer)) {
                memcpy(bufD->data.data() + u.offset, u.data.constData(), size_t(u.data.size()));
            } else {
                trackedBufferBarrier(cbD, bufD, QGles2Buffer::AccessUpdate);
                QGles2CommandBuffer::Command &cmd(cbD->commands.get());
                cmd.cmd = QGles2CommandBuffer::Command::BufferSubData;
                cmd.args.bufferSubData.target = bufD->targetForDataOps;
                cmd.args.bufferSubData.buffer = bufD->buffer;
                cmd.args.bufferSubData.offset = u.offset;
                cmd.args.bufferSubData.size = u.data.size();
                cmd.args.bufferSubData.data = cbD->retainBufferData(u.data);
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::StaticUpload) {
            QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, u.buf);
            Q_ASSERT(bufD->m_type != QRhiBuffer::Dynamic);
            Q_ASSERT(u.offset + u.data.size() <= bufD->m_size);
            if (bufD->m_usage.testFlag(QRhiBuffer::UniformBuffer)) {
                memcpy(bufD->data.data() + u.offset, u.data.constData(), size_t(u.data.size()));
            } else {
                trackedBufferBarrier(cbD, bufD, QGles2Buffer::AccessUpdate);
                QGles2CommandBuffer::Command &cmd(cbD->commands.get());
                cmd.cmd = QGles2CommandBuffer::Command::BufferSubData;
                cmd.args.bufferSubData.target = bufD->targetForDataOps;
                cmd.args.bufferSubData.buffer = bufD->buffer;
                cmd.args.bufferSubData.offset = u.offset;
                cmd.args.bufferSubData.size = u.data.size();
                cmd.args.bufferSubData.data = cbD->retainBufferData(u.data);
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::Read) {
            QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, u.buf);
            if (bufD->m_usage.testFlag(QRhiBuffer::UniformBuffer)) {
                u.result->data.resize(u.readSize);
                memcpy(u.result->data.data(), bufD->data.constData() + u.offset, size_t(u.readSize));
                if (u.result->completed)
                    u.result->completed();
            } else {
                QGles2CommandBuffer::Command &cmd(cbD->commands.get());
                cmd.cmd = QGles2CommandBuffer::Command::GetBufferSubData;
                cmd.args.getBufferSubData.result = u.result;
                cmd.args.getBufferSubData.target = bufD->targetForDataOps;
                cmd.args.getBufferSubData.buffer = bufD->buffer;
                cmd.args.getBufferSubData.offset = u.offset;
                cmd.args.getBufferSubData.size = u.readSize;
            }
        }
    }

    for (int opIdx = 0; opIdx < ud->activeTextureOpCount; ++opIdx) {
        const QRhiResourceUpdateBatchPrivate::TextureOp &u(ud->textureOps[opIdx]);
        if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Upload) {
            QGles2Texture *texD = QRHI_RES(QGles2Texture, u.dst);
            for (int layer = 0, maxLayer = u.subresDesc.size(); layer < maxLayer; ++layer) {
                for (int level = 0; level < QRhi::MAX_MIP_LEVELS; ++level) {
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : std::as_const(u.subresDesc[layer][level]))
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

            QGles2CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QGles2CommandBuffer::Command::CopyTex;

            const bool srcHasZ = srcD->m_flags.testFlag(QRhiTexture::ThreeDimensional) || srcD->m_flags.testFlag(QRhiTexture::TextureArray);
            const bool dstHasZ = dstD->m_flags.testFlag(QRhiTexture::ThreeDimensional) || dstD->m_flags.testFlag(QRhiTexture::TextureArray);
            const bool dstIs1dArray = dstD->m_flags.testFlag(QRhiTexture::OneDimensional)
                    && dstD->m_flags.testFlag(QRhiTexture::TextureArray);

            cmd.args.copyTex.srcTarget = srcD->target;
            cmd.args.copyTex.srcFaceTarget = srcFaceTargetBase + (srcHasZ ? 0u : uint(u.desc.sourceLayer()));
            cmd.args.copyTex.srcTexture = srcD->texture;
            cmd.args.copyTex.srcLevel = u.desc.sourceLevel();
            cmd.args.copyTex.srcX = sp.x();
            cmd.args.copyTex.srcY = sp.y();
            cmd.args.copyTex.srcZ = srcHasZ ? u.desc.sourceLayer() : 0;

            cmd.args.copyTex.dstTarget = dstD->target;
            cmd.args.copyTex.dstFaceTarget = dstFaceTargetBase + (dstHasZ ? 0u : uint(u.desc.destinationLayer()));
            cmd.args.copyTex.dstTexture = dstD->texture;
            cmd.args.copyTex.dstLevel = u.desc.destinationLevel();
            cmd.args.copyTex.dstX = dp.x();
            cmd.args.copyTex.dstY = dstIs1dArray ? u.desc.destinationLayer() : dp.y();
            cmd.args.copyTex.dstZ = dstHasZ ? u.desc.destinationLayer() : 0;

            cmd.args.copyTex.w = copySize.width();
            cmd.args.copyTex.h = copySize.height();
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Read) {
            QGles2CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QGles2CommandBuffer::Command::ReadPixels;
            cmd.args.readPixels.result = u.result;
            QGles2Texture *texD = QRHI_RES(QGles2Texture, u.rb.texture());
            if (texD)
                trackedImageBarrier(cbD, texD, QGles2Texture::AccessRead);
            cmd.args.readPixels.texture = texD ? texD->texture : 0;
            cmd.args.readPixels.slice3D = -1;
            if (texD) {
                if (u.rb.rect().isValid()) {
                    cmd.args.readPixels.x = u.rb.rect().x();
                    cmd.args.readPixels.y = u.rb.rect().y();
                    cmd.args.readPixels.w = u.rb.rect().width();
                    cmd.args.readPixels.h = u.rb.rect().height();
                }
                else {
                    const QSize readImageSize = q->sizeForMipLevel(u.rb.level(), texD->m_pixelSize);
                    cmd.args.readPixels.x = 0;
                    cmd.args.readPixels.y = 0;
                    cmd.args.readPixels.w = readImageSize.width();
                    cmd.args.readPixels.h = readImageSize.height();
                }
                cmd.args.readPixels.format = texD->m_format;
                if (texD->m_flags.testFlag(QRhiTexture::ThreeDimensional)
                    || texD->m_flags.testFlag(QRhiTexture::TextureArray))
                {
                    cmd.args.readPixels.readTarget = texD->target;
                    cmd.args.readPixels.slice3D = u.rb.layer();
                } else {
                    const GLenum faceTargetBase = texD->m_flags.testFlag(QRhiTexture::CubeMap)
                            ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : texD->target;
                    cmd.args.readPixels.readTarget = faceTargetBase + uint(u.rb.layer());
                }
                cmd.args.readPixels.level = u.rb.level();
            }
            else { // swapchain
                if (u.rb.rect().isValid()) {
                    cmd.args.readPixels.x = u.rb.rect().x();
                    cmd.args.readPixels.y = u.rb.rect().y();
                    cmd.args.readPixels.w = u.rb.rect().width();
                    cmd.args.readPixels.h = u.rb.rect().height();
                }
                else {
                    cmd.args.readPixels.x = 0;
                    cmd.args.readPixels.y = 0;
                    cmd.args.readPixels.w = currentSwapChain->pixelSize.width();
                    cmd.args.readPixels.h = currentSwapChain->pixelSize.height();
                }
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::GenMips) {
            QGles2Texture *texD = QRHI_RES(QGles2Texture, u.dst);
            trackedImageBarrier(cbD, texD, QGles2Texture::AccessFramebuffer);
            QGles2CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QGles2CommandBuffer::Command::GenMip;
            cmd.args.genMip.target = texD->target;
            cmd.args.genMip.texture = texD->texture;
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
    case QRhiGraphicsPipeline::Patches:
        return GL_PATCHES;
    default:
        Q_UNREACHABLE_RETURN(GL_TRIANGLES);
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
        Q_UNREACHABLE_RETURN(GL_BACK);
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
        Q_UNREACHABLE_RETURN(GL_CCW);
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
        Q_UNREACHABLE_RETURN(GL_ZERO);
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
        Q_UNREACHABLE_RETURN(GL_FUNC_ADD);
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
        Q_UNREACHABLE_RETURN(GL_ALWAYS);
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
        Q_UNREACHABLE_RETURN(GL_KEEP);
    }
}

static inline GLenum toGlPolygonMode(QRhiGraphicsPipeline::PolygonMode mode)
{
    switch (mode) {
    case QRhiGraphicsPipeline::PolygonMode::Fill:
        return GL_FILL;
    case QRhiGraphicsPipeline::PolygonMode::Line:
        return GL_LINE;
    default:
        Q_UNREACHABLE_RETURN(GL_FILL);
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
        Q_UNREACHABLE_RETURN(GL_LINEAR);
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
        Q_UNREACHABLE_RETURN(GL_LINEAR);
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
        Q_UNREACHABLE_RETURN(GL_CLAMP_TO_EDGE);
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
        Q_UNREACHABLE_RETURN(GL_NEVER);
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

struct CommandBufferExecTrackedState
{
    GLenum indexType = GL_UNSIGNED_SHORT;
    quint32 indexStride = sizeof(quint16);
    quint32 indexOffset = 0;
    GLuint currentArrayBuffer = 0;
    GLuint currentElementArrayBuffer = 0;
    struct {
        QRhiGraphicsPipeline *ps = nullptr;
        GLuint buffer = 0;
        quint32 offset = 0;
        int binding = 0;
    } lastBindVertexBuffer;
    static const int TRACKED_ATTRIB_COUNT = 16;
    bool enabledAttribArrays[TRACKED_ATTRIB_COUNT] = {};
    bool nonzeroAttribDivisor[TRACKED_ATTRIB_COUNT] = {};
    bool instancedAttributesUsed = false;
    int maxUntrackedInstancedAttribute = 0;
};

// Helper that must be used in executeCommandBuffer() whenever changing the
// ARRAY or ELEMENT_ARRAY buffer binding outside of Command::BindVertexBuffer
// and Command::BindIndexBuffer.
static inline void bindVertexIndexBufferWithStateReset(CommandBufferExecTrackedState *state,
                                                       QOpenGLExtensions *f,
                                                       GLenum target,
                                                       GLuint buffer)
{
    state->currentArrayBuffer = 0;
    state->currentElementArrayBuffer = 0;
    state->lastBindVertexBuffer.buffer = 0;
    f->glBindBuffer(target, buffer);
}

void QRhiGles2::executeCommandBuffer(QRhiCommandBuffer *cb)
{
    CommandBufferExecTrackedState state;
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);

    for (auto it = cbD->commands.cbegin(), end = cbD->commands.cend(); it != end; ++it) {
        const QGles2CommandBuffer::Command &cmd(*it);
        switch (cmd.cmd) {
        case QGles2CommandBuffer::Command::BeginFrame:
            if (cmd.args.beginFrame.timestampQuery)
                glQueryCounter(cmd.args.beginFrame.timestampQuery, GL_TIMESTAMP);
            if (caps.coreProfile) {
                if (!vao)
                    f->glGenVertexArrays(1, &vao);
                f->glBindVertexArray(vao);
            }
            break;
        case QGles2CommandBuffer::Command::EndFrame:
            if (state.instancedAttributesUsed) {
                for (int i = 0; i < CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT; ++i) {
                    if (state.nonzeroAttribDivisor[i])
                         f->glVertexAttribDivisor(GLuint(i), 0);
                }
                for (int i = CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT; i <= state.maxUntrackedInstancedAttribute; ++i)
                    f->glVertexAttribDivisor(GLuint(i), 0);
                state.instancedAttributesUsed = false;
            }
#ifdef Q_OS_WASM
            for (int i = 0; i < CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT; ++i) {
                if (state.enabledAttribArrays[i]) {
                    f->glDisableVertexAttribArray(GLuint(i));
                    state.enabledAttribArrays[i] = false;
                }
            }
#endif
            if (vao)
                f->glBindVertexArray(0);
            if (cmd.args.endFrame.timestampQuery)
                glQueryCounter(cmd.args.endFrame.timestampQuery, GL_TIMESTAMP);
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
                if (state.lastBindVertexBuffer.ps == psD
                        && state.lastBindVertexBuffer.buffer == cmd.args.bindVertexBuffer.buffer
                        && state.lastBindVertexBuffer.offset == cmd.args.bindVertexBuffer.offset
                        && state.lastBindVertexBuffer.binding == cmd.args.bindVertexBuffer.binding)
                {
                    // The pipeline and so the vertex input layout is
                    // immutable, no point in issuing the exact same set of
                    // glVertexAttribPointer again and again for the same buffer.
                    break;
                }
                state.lastBindVertexBuffer.ps = psD;
                state.lastBindVertexBuffer.buffer = cmd.args.bindVertexBuffer.buffer;
                state.lastBindVertexBuffer.offset = cmd.args.bindVertexBuffer.offset;
                state.lastBindVertexBuffer.binding = cmd.args.bindVertexBuffer.binding;

                if (cmd.args.bindVertexBuffer.buffer != state.currentArrayBuffer) {
                    state.currentArrayBuffer = cmd.args.bindVertexBuffer.buffer;
                    // we do not support more than one vertex buffer
                    f->glBindBuffer(GL_ARRAY_BUFFER, state.currentArrayBuffer);
                }
                for (auto it = psD->m_vertexInputLayout.cbeginAttributes(), itEnd = psD->m_vertexInputLayout.cendAttributes();
                     it != itEnd; ++it)
                {
                    const int bindingIdx = it->binding();
                    if (bindingIdx != cmd.args.bindVertexBuffer.binding)
                        continue;

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
                    case QRhiVertexInputAttribute::UInt4:
                        type = GL_UNSIGNED_INT;
                        size = 4;
                        break;
                    case QRhiVertexInputAttribute::UInt3:
                        type = GL_UNSIGNED_INT;
                        size = 3;
                        break;
                    case QRhiVertexInputAttribute::UInt2:
                        type = GL_UNSIGNED_INT;
                        size = 2;
                        break;
                    case QRhiVertexInputAttribute::UInt:
                        type = GL_UNSIGNED_INT;
                        size = 1;
                        break;
                     case QRhiVertexInputAttribute::SInt4:
                        type = GL_INT;
                        size = 4;
                        break;
                    case QRhiVertexInputAttribute::SInt3:
                        type = GL_INT;
                        size = 3;
                        break;
                    case QRhiVertexInputAttribute::SInt2:
                        type = GL_INT;
                        size = 2;
                        break;
                    case QRhiVertexInputAttribute::SInt:
                        type = GL_INT;
                        size = 1;
                        break;
                    case QRhiVertexInputAttribute::Half4:
                        type = GL_HALF_FLOAT;
                        size = 4;
                        break;
                    case QRhiVertexInputAttribute::Half3:
                        type = GL_HALF_FLOAT;
                        size = 3;
                        break;
                    case QRhiVertexInputAttribute::Half2:
                        type = GL_HALF_FLOAT;
                        size = 2;
                        break;
                    case QRhiVertexInputAttribute::Half:
                        type = GL_HALF_FLOAT;
                        size = 1;
                        break;
                    case QRhiVertexInputAttribute::UShort4:
                        type = GL_UNSIGNED_SHORT;
                        size = 4;
                        break;
                    case QRhiVertexInputAttribute::UShort3:
                        type = GL_UNSIGNED_SHORT;
                        size = 3;
                        break;
                    case QRhiVertexInputAttribute::UShort2:
                        type = GL_UNSIGNED_SHORT;
                        size = 2;
                        break;
                    case QRhiVertexInputAttribute::UShort:
                        type = GL_UNSIGNED_SHORT;
                        size = 1;
                        break;
                    case QRhiVertexInputAttribute::SShort4:
                        type = GL_SHORT;
                        size = 4;
                        break;
                    case QRhiVertexInputAttribute::SShort3:
                        type = GL_SHORT;
                        size = 3;
                        break;
                    case QRhiVertexInputAttribute::SShort2:
                        type = GL_SHORT;
                        size = 2;
                        break;
                    case QRhiVertexInputAttribute::SShort:
                        type = GL_SHORT;
                        size = 1;
                        break;
                    default:
                        break;
                    }

                    const int locationIdx = it->location();
                    quint32 ofs = it->offset() + cmd.args.bindVertexBuffer.offset;
                    if (type == GL_UNSIGNED_INT || type == GL_INT) {
                        if (caps.intAttributes) {
                            f->glVertexAttribIPointer(GLuint(locationIdx), size, type, stride,
                                                      reinterpret_cast<const GLvoid *>(quintptr(ofs)));
                        } else {
                            qWarning("Current RHI backend does not support IntAttributes. Check supported features.");
                            // This is a trick to disable this attribute
                            if (locationIdx < CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT)
                                state.enabledAttribArrays[locationIdx] = true;
                        }
                    } else {
                        f->glVertexAttribPointer(GLuint(locationIdx), size, type, normalize, stride,
                                                 reinterpret_cast<const GLvoid *>(quintptr(ofs)));
                    }
                    if (locationIdx >= CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT || !state.enabledAttribArrays[locationIdx]) {
                        if (locationIdx < CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT)
                            state.enabledAttribArrays[locationIdx] = true;
                        f->glEnableVertexAttribArray(GLuint(locationIdx));
                    }
                    if (inputBinding->classification() == QRhiVertexInputBinding::PerInstance && caps.instancing) {
                        f->glVertexAttribDivisor(GLuint(locationIdx), inputBinding->instanceStepRate());
                        if (Q_LIKELY(locationIdx < CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT))
                            state.nonzeroAttribDivisor[locationIdx] = true;
                        else
                            state.maxUntrackedInstancedAttribute = qMax(state.maxUntrackedInstancedAttribute, locationIdx);
                        state.instancedAttributesUsed = true;
                    } else if ((locationIdx < CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT
                                && state.nonzeroAttribDivisor[locationIdx])
                               || Q_UNLIKELY(locationIdx >= CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT
                                             && locationIdx <= state.maxUntrackedInstancedAttribute))
                    {
                        f->glVertexAttribDivisor(GLuint(locationIdx), 0);
                        if (locationIdx < CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT)
                            state.nonzeroAttribDivisor[locationIdx] = false;
                    }
                }
            } else {
                qWarning("No graphics pipeline active for setVertexInput; ignored");
            }
        }
            break;
        case QGles2CommandBuffer::Command::BindIndexBuffer:
            state.indexType = cmd.args.bindIndexBuffer.type;
            state.indexStride = state.indexType == GL_UNSIGNED_SHORT ? sizeof(quint16) : sizeof(quint32);
            state.indexOffset = cmd.args.bindIndexBuffer.offset;
            if (state.currentElementArrayBuffer != cmd.args.bindIndexBuffer.buffer) {
                state.currentElementArrayBuffer = cmd.args.bindIndexBuffer.buffer;
                f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.currentElementArrayBuffer);
            }
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
                            quintptr(cmd.args.drawIndexed.firstIndex * state.indexStride + state.indexOffset));
                if (cmd.args.drawIndexed.instanceCount == 1 || !caps.instancing) {
                    if (cmd.args.drawIndexed.baseVertex != 0 && caps.baseVertex) {
                        f->glDrawElementsBaseVertex(psD->drawMode,
                                                    GLsizei(cmd.args.drawIndexed.indexCount),
                                                    state.indexType,
                                                    ofs,
                                                    cmd.args.drawIndexed.baseVertex);
                    } else {
                        f->glDrawElements(psD->drawMode,
                                          GLsizei(cmd.args.drawIndexed.indexCount),
                                          state.indexType,
                                          ofs);
                    }
                } else {
                    if (cmd.args.drawIndexed.baseVertex != 0 && caps.baseVertex) {
                        f->glDrawElementsInstancedBaseVertex(psD->drawMode,
                                                             GLsizei(cmd.args.drawIndexed.indexCount),
                                                             state.indexType,
                                                             ofs,
                                                             GLsizei(cmd.args.drawIndexed.instanceCount),
                                                             cmd.args.drawIndexed.baseVertex);
                    } else {
                        f->glDrawElementsInstanced(psD->drawMode,
                                                   GLsizei(cmd.args.drawIndexed.indexCount),
                                                   state.indexType,
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
            bindShaderResources(cbD,
                                cmd.args.bindShaderResources.maybeGraphicsPs,
                                cmd.args.bindShaderResources.maybeComputePs,
                                cmd.args.bindShaderResources.srb,
                                cmd.args.bindShaderResources.dynamicOffsetPairs,
                                cmd.args.bindShaderResources.dynamicOffsetCount);
            break;
        case QGles2CommandBuffer::Command::BindFramebuffer:
        {
            QVarLengthArray<GLenum, 8> bufs;
            GLuint fbo = cmd.args.bindFramebuffer.fbo;
            if (!fbo)
                fbo = ctx->defaultFramebufferObject();
            f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            if (fbo) {
                const int colorAttCount = cmd.args.bindFramebuffer.colorAttCount;
                bufs.append(colorAttCount > 0 ? GL_COLOR_ATTACHMENT0 : GL_NONE);
                if (caps.maxDrawBuffers > 1) {
                    for (int i = 1; i < colorAttCount; ++i)
                        bufs.append(GL_COLOR_ATTACHMENT0 + uint(i));
                }
            } else {
                if (cmd.args.bindFramebuffer.stereo && cmd.args.bindFramebuffer.stereoTarget == QRhiSwapChain::RightBuffer)
                    bufs.append(GL_BACK_RIGHT);
                else
                    bufs.append(caps.gles ? GL_BACK : GL_BACK_LEFT);
            }
            if (caps.hasDrawBuffersFunc)
                f->glDrawBuffers(bufs.count(), bufs.constData());
            if (caps.srgbWriteControl) {
                if (cmd.args.bindFramebuffer.srgb)
                    f->glEnable(GL_FRAMEBUFFER_SRGB);
                else
                    f->glDisable(GL_FRAMEBUFFER_SRGB);
            }
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
            if (cmd.args.clear.mask & GL_STENCIL_BUFFER_BIT) {
                f->glStencilMask(0xFF);
                f->glClearStencil(GLint(cmd.args.clear.s));
            }
            f->glClear(cmd.args.clear.mask);
            cbD->graphicsPassState.reset(); // altered depth/color write, invalidate in order to avoid confusing the state tracking
            break;
        case QGles2CommandBuffer::Command::BufferSubData:
            bindVertexIndexBufferWithStateReset(&state, f, cmd.args.bufferSubData.target, cmd.args.bufferSubData.buffer);
            f->glBufferSubData(cmd.args.bufferSubData.target, cmd.args.bufferSubData.offset, cmd.args.bufferSubData.size,
                               cmd.args.bufferSubData.data);
            break;
        case QGles2CommandBuffer::Command::GetBufferSubData:
        {
            QRhiReadbackResult *result = cmd.args.getBufferSubData.result;
            bindVertexIndexBufferWithStateReset(&state, f, cmd.args.getBufferSubData.target, cmd.args.getBufferSubData.buffer);
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
            if (cmd.args.copyTex.srcTarget == GL_TEXTURE_3D
                || cmd.args.copyTex.srcTarget == GL_TEXTURE_2D_ARRAY
                || cmd.args.copyTex.srcTarget == GL_TEXTURE_1D_ARRAY) {
                f->glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cmd.args.copyTex.srcTexture,
                                             cmd.args.copyTex.srcLevel, cmd.args.copyTex.srcZ);
            } else if (cmd.args.copyTex.srcTarget == GL_TEXTURE_1D) {
                glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       cmd.args.copyTex.srcTarget, cmd.args.copyTex.srcTexture,
                                       cmd.args.copyTex.srcLevel);
            } else {
                f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          cmd.args.copyTex.srcFaceTarget, cmd.args.copyTex.srcTexture, cmd.args.copyTex.srcLevel);
            }
            f->glBindTexture(cmd.args.copyTex.dstTarget, cmd.args.copyTex.dstTexture);
            if (cmd.args.copyTex.dstTarget == GL_TEXTURE_3D || cmd.args.copyTex.dstTarget == GL_TEXTURE_2D_ARRAY) {
                f->glCopyTexSubImage3D(cmd.args.copyTex.dstTarget, cmd.args.copyTex.dstLevel,
                                       cmd.args.copyTex.dstX, cmd.args.copyTex.dstY, cmd.args.copyTex.dstZ,
                                       cmd.args.copyTex.srcX, cmd.args.copyTex.srcY,
                                       cmd.args.copyTex.w, cmd.args.copyTex.h);
            } else if (cmd.args.copyTex.dstTarget == GL_TEXTURE_1D) {
                glCopyTexSubImage1D(cmd.args.copyTex.dstTarget, cmd.args.copyTex.dstLevel,
                                    cmd.args.copyTex.dstX, cmd.args.copyTex.srcX,
                                    cmd.args.copyTex.srcY, cmd.args.copyTex.w);
            } else {
                f->glCopyTexSubImage2D(cmd.args.copyTex.dstFaceTarget, cmd.args.copyTex.dstLevel,
                                       cmd.args.copyTex.dstX, cmd.args.copyTex.dstY,
                                       cmd.args.copyTex.srcX, cmd.args.copyTex.srcY,
                                       cmd.args.copyTex.w, cmd.args.copyTex.h);
            }
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
            result->pixelSize = QSize(cmd.args.readPixels.w, cmd.args.readPixels.h);
            if (tex) {
                result->format = cmd.args.readPixels.format;
                mipLevel = cmd.args.readPixels.level;
                if (mipLevel == 0 || caps.nonBaseLevelFramebufferTexture) {
                    f->glGenFramebuffers(1, &fbo);
                    f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                    if (cmd.args.readPixels.slice3D >= 0) {
                        f->glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                     tex, mipLevel, cmd.args.readPixels.slice3D);
                    } else if (cmd.args.readPixels.readTarget == GL_TEXTURE_1D) {
                        glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                               cmd.args.readPixels.readTarget, tex, mipLevel);
                    } else {
                        f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                  cmd.args.readPixels.readTarget, tex, mipLevel);
                    }
                }
            } else {
                result->format = QRhiTexture::RGBA8;
                // readPixels handles multisample resolving implicitly
            }
            const int x = cmd.args.readPixels.x;
            const int y = cmd.args.readPixels.y;
            const int w = cmd.args.readPixels.w;
            const int h = cmd.args.readPixels.h;
            if (mipLevel == 0 || caps.nonBaseLevelFramebufferTexture) {
                // With GLES, GL_RGBA is the only mandated readback format, so stick with it.
                // (and that's why we return false for the ReadBackAnyTextureFormat feature)
                if (result->format == QRhiTexture::R8 || result->format == QRhiTexture::RED_OR_ALPHA8) {
                    result->data.resizeForOverwrite(w * h);
                    QByteArray tmpBuf;
                    tmpBuf.resizeForOverwrite(w * h * 4);
                    f->glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, tmpBuf.data());
                    const quint8 *srcBase = reinterpret_cast<const quint8 *>(tmpBuf.constData());
                    quint8 *dstBase = reinterpret_cast<quint8 *>(result->data.data());
                    const int componentIndex = isFeatureSupported(QRhi::RedOrAlpha8IsRed) ? 0 : 3;
                    for (int y = 0; y < h; ++y) {
                        const quint8 *src = srcBase + y * w * 4;
                        quint8 *dst = dstBase + y * w;
                        int count = w;
                        while (count-- > 0) {
                            *dst++ = src[componentIndex];
                            src += 4;
                        }
                    }
                } else {
                    // For other formats try it because this can be relevant for some use cases;
                    // if it works, then fine, if not, there's nothing we can do.
                    [[maybe_unused]] GLenum glintformat;
                    [[maybe_unused]] GLenum glsizedintformat;
                    GLenum glformat;
                    GLenum gltype;
                    toGlTextureFormat(result->format, caps, &glintformat, &glsizedintformat, &glformat, &gltype);
                    quint32 byteSize;
                    textureFormatInfo(result->format, result->pixelSize, nullptr, &byteSize, nullptr);
                    result->data.resizeForOverwrite(byteSize);
                    f->glReadPixels(x, y, w, h, glformat, gltype, result->data.data());
                }
            } else {
                result->data.resizeForOverwrite(w * h * 4);
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
            if (cmd.args.subImage.rowLength != 0)
                f->glPixelStorei(GL_UNPACK_ROW_LENGTH, cmd.args.subImage.rowLength);
            if (cmd.args.subImage.target == GL_TEXTURE_3D || cmd.args.subImage.target == GL_TEXTURE_2D_ARRAY) {
                f->glTexSubImage3D(cmd.args.subImage.target, cmd.args.subImage.level,
                                   cmd.args.subImage.dx, cmd.args.subImage.dy, cmd.args.subImage.dz,
                                   cmd.args.subImage.w, cmd.args.subImage.h, 1,
                                   cmd.args.subImage.glformat, cmd.args.subImage.gltype,
                                   cmd.args.subImage.data);
            } else if (cmd.args.subImage.target == GL_TEXTURE_1D) {
                glTexSubImage1D(cmd.args.subImage.target, cmd.args.subImage.level,
                                cmd.args.subImage.dx, cmd.args.subImage.w,
                                cmd.args.subImage.glformat, cmd.args.subImage.gltype,
                                cmd.args.subImage.data);
            } else {
                f->glTexSubImage2D(cmd.args.subImage.faceTarget, cmd.args.subImage.level,
                                   cmd.args.subImage.dx, cmd.args.subImage.dy,
                                   cmd.args.subImage.w, cmd.args.subImage.h,
                                   cmd.args.subImage.glformat, cmd.args.subImage.gltype,
                                   cmd.args.subImage.data);
            }
            if (cmd.args.subImage.rowStartAlign != 4)
                f->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            if (cmd.args.subImage.rowLength != 0)
                f->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            break;
        case QGles2CommandBuffer::Command::CompressedImage:
            f->glBindTexture(cmd.args.compressedImage.target, cmd.args.compressedImage.texture);
            if (cmd.args.compressedImage.target == GL_TEXTURE_3D || cmd.args.compressedImage.target == GL_TEXTURE_2D_ARRAY) {
                f->glCompressedTexImage3D(cmd.args.compressedImage.target, cmd.args.compressedImage.level,
                                          cmd.args.compressedImage.glintformat,
                                          cmd.args.compressedImage.w, cmd.args.compressedImage.h, cmd.args.compressedImage.depth,
                                          0, cmd.args.compressedImage.size, cmd.args.compressedImage.data);
            } else if (cmd.args.compressedImage.target == GL_TEXTURE_1D) {
                glCompressedTexImage1D(
                        cmd.args.compressedImage.target, cmd.args.compressedImage.level,
                        cmd.args.compressedImage.glintformat, cmd.args.compressedImage.w, 0,
                        cmd.args.compressedImage.size, cmd.args.compressedImage.data);
            } else {
                f->glCompressedTexImage2D(cmd.args.compressedImage.faceTarget, cmd.args.compressedImage.level,
                                          cmd.args.compressedImage.glintformat,
                                          cmd.args.compressedImage.w, cmd.args.compressedImage.h,
                                          0, cmd.args.compressedImage.size, cmd.args.compressedImage.data);
            }
            break;
        case QGles2CommandBuffer::Command::CompressedSubImage:
            f->glBindTexture(cmd.args.compressedSubImage.target, cmd.args.compressedSubImage.texture);
            if (cmd.args.compressedSubImage.target == GL_TEXTURE_3D || cmd.args.compressedSubImage.target == GL_TEXTURE_2D_ARRAY) {
                f->glCompressedTexSubImage3D(cmd.args.compressedSubImage.target, cmd.args.compressedSubImage.level,
                                             cmd.args.compressedSubImage.dx, cmd.args.compressedSubImage.dy, cmd.args.compressedSubImage.dz,
                                             cmd.args.compressedSubImage.w, cmd.args.compressedSubImage.h, 1,
                                             cmd.args.compressedSubImage.glintformat,
                                             cmd.args.compressedSubImage.size, cmd.args.compressedSubImage.data);
            } else if (cmd.args.compressedImage.target == GL_TEXTURE_1D) {
                glCompressedTexSubImage1D(
                        cmd.args.compressedSubImage.target, cmd.args.compressedSubImage.level,
                        cmd.args.compressedSubImage.dx, cmd.args.compressedSubImage.w,
                        cmd.args.compressedSubImage.glintformat, cmd.args.compressedSubImage.size,
                        cmd.args.compressedSubImage.data);
            } else {
                f->glCompressedTexSubImage2D(cmd.args.compressedSubImage.faceTarget, cmd.args.compressedSubImage.level,
                                             cmd.args.compressedSubImage.dx, cmd.args.compressedSubImage.dy,
                                             cmd.args.compressedSubImage.w, cmd.args.compressedSubImage.h,
                                             cmd.args.compressedSubImage.glintformat,
                                             cmd.args.compressedSubImage.size, cmd.args.compressedSubImage.data);
            }
            break;
        case QGles2CommandBuffer::Command::BlitFromRenderbuffer:
        {
            // Altering the scissor state, so reset the stored state, although
            // not strictly required as long as blit is done in endPass() only.
            cbD->graphicsPassState.reset();
            f->glDisable(GL_SCISSOR_TEST);
            GLuint fbo[2];
            f->glGenFramebuffers(2, fbo);
            f->glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo[0]);
            const bool ds = cmd.args.blitFromRenderbuffer.isDepthStencil;
            if (ds) {
                f->glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                             GL_RENDERBUFFER, cmd.args.blitFromRenderbuffer.renderbuffer);
                f->glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                             GL_RENDERBUFFER, cmd.args.blitFromRenderbuffer.renderbuffer);
            } else {
                f->glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_RENDERBUFFER, cmd.args.blitFromRenderbuffer.renderbuffer);
            }
            f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo[1]);
            if (cmd.args.blitFromRenderbuffer.target == GL_TEXTURE_3D || cmd.args.blitFromRenderbuffer.target == GL_TEXTURE_2D_ARRAY) {
                if (ds) {
                    f->glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                cmd.args.blitFromRenderbuffer.dstTexture,
                                                cmd.args.blitFromRenderbuffer.dstLevel,
                                                cmd.args.blitFromRenderbuffer.dstLayer);
                    f->glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                                cmd.args.blitFromRenderbuffer.dstTexture,
                                                cmd.args.blitFromRenderbuffer.dstLevel,
                                                cmd.args.blitFromRenderbuffer.dstLayer);
                } else {
                    f->glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                cmd.args.blitFromRenderbuffer.dstTexture,
                                                cmd.args.blitFromRenderbuffer.dstLevel,
                                                cmd.args.blitFromRenderbuffer.dstLayer);
                }
            } else {
                if (ds) {
                    f->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cmd.args.blitFromRenderbuffer.target,
                                            cmd.args.blitFromRenderbuffer.dstTexture, cmd.args.blitFromRenderbuffer.dstLevel);
                    f->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, cmd.args.blitFromRenderbuffer.target,
                                            cmd.args.blitFromRenderbuffer.dstTexture, cmd.args.blitFromRenderbuffer.dstLevel);
                } else {
                    f->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cmd.args.blitFromRenderbuffer.target,
                                            cmd.args.blitFromRenderbuffer.dstTexture, cmd.args.blitFromRenderbuffer.dstLevel);
                }
            }
            f->glBlitFramebuffer(0, 0, cmd.args.blitFromRenderbuffer.w, cmd.args.blitFromRenderbuffer.h,
                                 0, 0, cmd.args.blitFromRenderbuffer.w, cmd.args.blitFromRenderbuffer.h,
                                 ds ? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT,
                                 GL_NEAREST); // Qt 5 used Nearest when resolving samples, stick to that
            f->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
            f->glDeleteFramebuffers(2, fbo);
        }
            break;
        case QGles2CommandBuffer::Command::BlitFromTexture:
        {
            // Altering the scissor state, so reset the stored state, although
            // not strictly required as long as blit is done in endPass() only.
            cbD->graphicsPassState.reset();
            f->glDisable(GL_SCISSOR_TEST);
            GLuint fbo[2];
            f->glGenFramebuffers(2, fbo);
            f->glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo[0]);
            const bool ds = cmd.args.blitFromTexture.isDepthStencil;
            if (cmd.args.blitFromTexture.srcTarget == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
                if (ds) {
                    f->glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                 cmd.args.blitFromTexture.srcTexture,
                                                 cmd.args.blitFromTexture.srcLevel,
                                                 cmd.args.blitFromTexture.srcLayer);
                    f->glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                                 cmd.args.blitFromTexture.srcTexture,
                                                 cmd.args.blitFromTexture.srcLevel,
                                                 cmd.args.blitFromTexture.srcLayer);
                } else {
                    f->glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                 cmd.args.blitFromTexture.srcTexture,
                                                 cmd.args.blitFromTexture.srcLevel,
                                                 cmd.args.blitFromTexture.srcLayer);
                }
            } else {
                if (ds) {
                    f->glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cmd.args.blitFromTexture.srcTarget,
                                              cmd.args.blitFromTexture.srcTexture, cmd.args.blitFromTexture.srcLevel);
                    f->glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, cmd.args.blitFromTexture.srcTarget,
                                              cmd.args.blitFromTexture.srcTexture, cmd.args.blitFromTexture.srcLevel);
                } else {
                    f->glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cmd.args.blitFromTexture.srcTarget,
                                              cmd.args.blitFromTexture.srcTexture, cmd.args.blitFromTexture.srcLevel);
                }
            }
            f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo[1]);
            if (cmd.args.blitFromTexture.dstTarget == GL_TEXTURE_3D || cmd.args.blitFromTexture.dstTarget == GL_TEXTURE_2D_ARRAY) {
                if (ds) {
                    f->glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                 cmd.args.blitFromTexture.dstTexture,
                                                 cmd.args.blitFromTexture.dstLevel,
                                                 cmd.args.blitFromTexture.dstLayer);
                    f->glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                                 cmd.args.blitFromTexture.dstTexture,
                                                 cmd.args.blitFromTexture.dstLevel,
                                                 cmd.args.blitFromTexture.dstLayer);
                } else {
                    f->glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                 cmd.args.blitFromTexture.dstTexture,
                                                 cmd.args.blitFromTexture.dstLevel,
                                                 cmd.args.blitFromTexture.dstLayer);
                }
            } else {
                if (ds) {
                    f->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cmd.args.blitFromTexture.dstTarget,
                                              cmd.args.blitFromTexture.dstTexture, cmd.args.blitFromTexture.dstLevel);
                    f->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, cmd.args.blitFromTexture.dstTarget,
                                              cmd.args.blitFromTexture.dstTexture, cmd.args.blitFromTexture.dstLevel);
                } else {
                    f->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cmd.args.blitFromTexture.dstTarget,
                                              cmd.args.blitFromTexture.dstTexture, cmd.args.blitFromTexture.dstLevel);
                }
            }
            f->glBlitFramebuffer(0, 0, cmd.args.blitFromTexture.w, cmd.args.blitFromTexture.h,
                                 0, 0, cmd.args.blitFromTexture.w, cmd.args.blitFromTexture.h,
                                 ds ? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT,
                                 GL_NEAREST); // Qt 5 used Nearest when resolving samples, stick to that
            f->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
            f->glDeleteFramebuffers(2, fbo);
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
            if (!caps.compute)
                break;
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
                    barriers |= barriersForBuffer();
            }
            for (auto it = tracker.cbeginTextures(), itEnd = tracker.cendTextures(); it != itEnd; ++it) {
                QGles2Texture::Access accessBeforePass = QGles2Texture::Access(it->stateAtPassBegin.access);
                if (textureAccessIsWrite(accessBeforePass))
                    barriers |= barriersForTexture();
            }
            if (barriers)
                f->glMemoryBarrier(barriers);
        }
            break;
        case QGles2CommandBuffer::Command::Barrier:
            if (caps.compute)
                f->glMemoryBarrier(cmd.args.barrier.barriers);
            break;
        case QGles2CommandBuffer::Command::InvalidateFramebuffer:
            if (caps.gles && caps.ctxMajor >= 3) {
                f->glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER,
                                           cmd.args.invalidateFramebuffer.attCount,
                                           cmd.args.invalidateFramebuffer.att);
            }
            break;
        default:
            break;
        }
    }
    if (state.instancedAttributesUsed) {
        for (int i = 0; i < CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT; ++i) {
            if (state.nonzeroAttribDivisor[i])
                 f->glVertexAttribDivisor(GLuint(i), 0);
        }
        for (int i = CommandBufferExecTrackedState::TRACKED_ATTRIB_COUNT; i <= state.maxUntrackedInstancedAttribute; ++i)
            f->glVertexAttribDivisor(GLuint(i), 0);
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

    const GLenum polygonMode = toGlPolygonMode(psD->m_polygonMode);
    if (glPolygonMode && (forceUpdate || polygonMode != state.polygonMode)) {
        state.polygonMode = polygonMode;
        glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
    }

    if (!psD->m_targetBlends.isEmpty()) {
        GLint buffer = 0;
        bool anyBlendEnabled = false;
        for (const auto targetBlend : psD->m_targetBlends) {
            const QGles2CommandBuffer::GraphicsPassState::ColorMask colorMask = {
                targetBlend.colorWrite.testFlag(QRhiGraphicsPipeline::R),
                targetBlend.colorWrite.testFlag(QRhiGraphicsPipeline::G),
                targetBlend.colorWrite.testFlag(QRhiGraphicsPipeline::B),
                targetBlend.colorWrite.testFlag(QRhiGraphicsPipeline::A)
            };
            if (forceUpdate || colorMask != state.colorMask[buffer]) {
                state.colorMask[buffer] = colorMask;
                if (caps.perRenderTargetBlending)
                    f->glColorMaski(buffer, colorMask.r, colorMask.g, colorMask.b, colorMask.a);
                else
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
            anyBlendEnabled |= blendEnabled;
            if (forceUpdate || blendEnabled != state.blendEnabled[buffer] || (blendEnabled && blend != state.blend[buffer])) {
                state.blendEnabled[buffer] = blendEnabled;
                if (blendEnabled) {
                    state.blend[buffer] = blend;
                    if (caps.perRenderTargetBlending) {
                        f->glBlendFuncSeparatei(buffer, blend.srcColor, blend.dstColor, blend.srcAlpha, blend.dstAlpha);
                        f->glBlendEquationSeparatei(buffer, blend.opColor, blend.opAlpha);
                    } else {
                        f->glBlendFuncSeparate(blend.srcColor, blend.dstColor, blend.srcAlpha, blend.dstAlpha);
                        f->glBlendEquationSeparate(blend.opColor, blend.opAlpha);
                    }
                }
            }
            buffer++;
            if (!caps.perRenderTargetBlending)
                break;
        }
        if (anyBlendEnabled)
            f->glEnable(GL_BLEND);
        else
            f->glDisable(GL_BLEND);
    } else {
        const QGles2CommandBuffer::GraphicsPassState::ColorMask colorMask = { true, true, true, true };
        if (forceUpdate || colorMask != state.colorMask[0]) {
            state.colorMask[0] = colorMask;
            f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
        const bool blendEnabled = false;
        if (forceUpdate || blendEnabled != state.blendEnabled[0]) {
            state.blendEnabled[0] = blendEnabled;
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

    if (psD->m_topology == QRhiGraphicsPipeline::Patches) {
        const int cpCount = psD->m_patchControlPointCount;
        if (forceUpdate || cpCount != state.cpCount) {
            state.cpCount = cpCount;
            f->glPatchParameteri(GL_PATCH_VERTICES, qMax(1, cpCount));
        }
    }

    f->glUseProgram(psD->program);
}

template <typename T>
static inline void qrhi_std140_to_packed(T *dst, int vecSize, int elemCount, const void *src)
{
    const T *p = reinterpret_cast<const T *>(src);
    for (int i = 0; i < elemCount; ++i) {
        for (int j = 0; j < vecSize; ++j)
            dst[vecSize * i + j] = *p++;
        p += 4 - vecSize;
    }
}

void QRhiGles2::bindCombinedSampler(QGles2CommandBuffer *cbD, QGles2Texture *texD, QGles2Sampler *samplerD,
                                    void *ps, uint psGeneration, int glslLocation,
                                    int *texUnit, bool *activeTexUnitAltered)
{
    const bool samplerStateValid = texD->samplerState == samplerD->d;
    const bool cachedStateInRange = *texUnit < 16;
    bool updateTextureBinding = true;
    if (samplerStateValid && cachedStateInRange) {
        // If we already encountered the same texture with
        // the same pipeline for this texture unit in the
        // current pass, then the shader program already
        // has the uniform set. As in a 3D scene one model
        // often has more than one associated texture map,
        // the savings here can become significant,
        // depending on the scene.
        if (cbD->textureUnitState[*texUnit].ps == ps
                && cbD->textureUnitState[*texUnit].psGeneration == psGeneration
                && cbD->textureUnitState[*texUnit].texture == texD->texture)
        {
            updateTextureBinding = false;
        }
    }
    if (updateTextureBinding) {
        f->glActiveTexture(GL_TEXTURE0 + uint(*texUnit));
        *activeTexUnitAltered = true;
        f->glBindTexture(texD->target, texD->texture);
        f->glUniform1i(glslLocation, *texUnit);
        if (cachedStateInRange) {
            cbD->textureUnitState[*texUnit].ps = ps;
            cbD->textureUnitState[*texUnit].psGeneration = psGeneration;
            cbD->textureUnitState[*texUnit].texture = texD->texture;
        }
    }
    ++(*texUnit);
    if (!samplerStateValid) {
        f->glTexParameteri(texD->target, GL_TEXTURE_MIN_FILTER, GLint(samplerD->d.glminfilter));
        f->glTexParameteri(texD->target, GL_TEXTURE_MAG_FILTER, GLint(samplerD->d.glmagfilter));
        f->glTexParameteri(texD->target, GL_TEXTURE_WRAP_S, GLint(samplerD->d.glwraps));
        f->glTexParameteri(texD->target, GL_TEXTURE_WRAP_T, GLint(samplerD->d.glwrapt));
        if (caps.texture3D && texD->target == GL_TEXTURE_3D)
            f->glTexParameteri(texD->target, GL_TEXTURE_WRAP_R, GLint(samplerD->d.glwrapr));
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
}

void QRhiGles2::bindShaderResources(QGles2CommandBuffer *cbD,
                                    QRhiGraphicsPipeline *maybeGraphicsPs, QRhiComputePipeline *maybeComputePs,
                                    QRhiShaderResourceBindings *srb,
                                    const uint *dynOfsPairs, int dynOfsCount)
{
    QGles2ShaderResourceBindings *srbD = QRHI_RES(QGles2ShaderResourceBindings, srb);
    int texUnit = 1; // start from unit 1, keep 0 for resource mgmt stuff to avoid clashes
    bool activeTexUnitAltered = false;
    QGles2UniformDescriptionVector &uniforms(maybeGraphicsPs ? QRHI_RES(QGles2GraphicsPipeline, maybeGraphicsPs)->uniforms
                                                             : QRHI_RES(QGles2ComputePipeline, maybeComputePs)->uniforms);
    QGles2UniformState *uniformState = maybeGraphicsPs ? QRHI_RES(QGles2GraphicsPipeline, maybeGraphicsPs)->uniformState
                                                       : QRHI_RES(QGles2ComputePipeline, maybeComputePs)->uniformState;
    m_scratch.separateTextureBindings.clear();
    m_scratch.separateSamplerBindings.clear();

    for (int i = 0, ie = srbD->m_bindings.size(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = shaderResourceBindingData(srbD->m_bindings.at(i));

        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            int viewOffset = b->u.ubuf.offset;
            for (int j = 0; j < dynOfsCount; ++j) {
                if (dynOfsPairs[2 * j] == uint(b->binding)) {
                    viewOffset = int(dynOfsPairs[2 * j + 1]);
                    break;
                }
            }
            QGles2Buffer *bufD = QRHI_RES(QGles2Buffer, b->u.ubuf.buf);
            const char *bufView = bufD->data.constData() + viewOffset;
            for (const QGles2UniformDescription &uniform : std::as_const(uniforms)) {
                if (uniform.binding == b->binding) {
                    // in a uniform buffer everything is at least 4 byte aligned
                    // so this should not cause unaligned reads
                    const void *src = bufView + uniform.offset;

#ifndef QT_NO_DEBUG
                    if (uniform.arrayDim > 0
                            && uniform.type != QShaderDescription::Float
                            && uniform.type != QShaderDescription::Vec2
                            && uniform.type != QShaderDescription::Vec3
                            && uniform.type != QShaderDescription::Vec4
                            && uniform.type != QShaderDescription::Int
                            && uniform.type != QShaderDescription::Int2
                            && uniform.type != QShaderDescription::Int3
                            && uniform.type != QShaderDescription::Int4
                            && uniform.type != QShaderDescription::Mat3
                            && uniform.type != QShaderDescription::Mat4)
                    {
                        qWarning("Uniform with buffer binding %d, buffer offset %d, type %d is an array, "
                                 "but arrays are only supported for float, vec2, vec3, vec4, int, "
                                 "ivec2, ivec3, ivec4, mat3 and mat4. "
                                 "Only the first element will be set.",
                                 uniform.binding, uniform.offset, uniform.type);
                    }
#endif

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
                            const float v = *reinterpret_cast<const float *>(src);
                            if (uniform.glslLocation <= QGles2UniformState::MAX_TRACKED_LOCATION) {
                                QGles2UniformState &thisUniformState(uniformState[uniform.glslLocation]);
                                if (thisUniformState.componentCount != 1 || thisUniformState.v[0] != v) {
                                    thisUniformState.componentCount = 1;
                                    thisUniformState.v[0] = v;
                                    f->glUniform1f(uniform.glslLocation, v);
                                }
                            } else {
                                f->glUniform1f(uniform.glslLocation, v);
                            }
                        } else {
                            // input is 16 bytes per element as per std140, have to convert to packed
                            m_scratch.packedArray.resize(elemCount);
                            qrhi_std140_to_packed(&m_scratch.packedArray.data()->f, 1, elemCount, src);
                            f->glUniform1fv(uniform.glslLocation, elemCount, &m_scratch.packedArray.constData()->f);
                        }
                    }
                        break;
                    case QShaderDescription::Vec2:
                    {
                        const int elemCount = uniform.arrayDim;
                        if (elemCount < 1) {
                            const float *v = reinterpret_cast<const float *>(src);
                            if (uniform.glslLocation <= QGles2UniformState::MAX_TRACKED_LOCATION) {
                                QGles2UniformState &thisUniformState(uniformState[uniform.glslLocation]);
                                if (thisUniformState.componentCount != 2
                                        || thisUniformState.v[0] != v[0]
                                        || thisUniformState.v[1] != v[1])
                                {
                                    thisUniformState.componentCount = 2;
                                    thisUniformState.v[0] = v[0];
                                    thisUniformState.v[1] = v[1];
                                    f->glUniform2fv(uniform.glslLocation, 1, v);
                                }
                            } else {
                                f->glUniform2fv(uniform.glslLocation, 1, v);
                            }
                        } else {
                            m_scratch.packedArray.resize(elemCount * 2);
                            qrhi_std140_to_packed(&m_scratch.packedArray.data()->f, 2, elemCount, src);
                            f->glUniform2fv(uniform.glslLocation, elemCount, &m_scratch.packedArray.constData()->f);
                        }
                    }
                        break;
                    case QShaderDescription::Vec3:
                    {
                        const int elemCount = uniform.arrayDim;
                        if (elemCount < 1) {
                            const float *v = reinterpret_cast<const float *>(src);
                            if (uniform.glslLocation <= QGles2UniformState::MAX_TRACKED_LOCATION) {
                                QGles2UniformState &thisUniformState(uniformState[uniform.glslLocation]);
                                if (thisUniformState.componentCount != 3
                                        || thisUniformState.v[0] != v[0]
                                        || thisUniformState.v[1] != v[1]
                                        || thisUniformState.v[2] != v[2])
                                {
                                    thisUniformState.componentCount = 3;
                                    thisUniformState.v[0] = v[0];
                                    thisUniformState.v[1] = v[1];
                                    thisUniformState.v[2] = v[2];
                                    f->glUniform3fv(uniform.glslLocation, 1, v);
                                }
                            } else {
                                f->glUniform3fv(uniform.glslLocation, 1, v);
                            }
                        } else {
                            m_scratch.packedArray.resize(elemCount * 3);
                            qrhi_std140_to_packed(&m_scratch.packedArray.data()->f, 3, elemCount, src);
                            f->glUniform3fv(uniform.glslLocation, elemCount, &m_scratch.packedArray.constData()->f);
                        }
                    }
                        break;
                    case QShaderDescription::Vec4:
                    {
                        const int elemCount = uniform.arrayDim;
                        if (elemCount < 1) {
                            const float *v = reinterpret_cast<const float *>(src);
                            if (uniform.glslLocation <= QGles2UniformState::MAX_TRACKED_LOCATION) {
                                QGles2UniformState &thisUniformState(uniformState[uniform.glslLocation]);
                                if (thisUniformState.componentCount != 4
                                        || thisUniformState.v[0] != v[0]
                                        || thisUniformState.v[1] != v[1]
                                        || thisUniformState.v[2] != v[2]
                                        || thisUniformState.v[3] != v[3])
                                {
                                    thisUniformState.componentCount = 4;
                                    thisUniformState.v[0] = v[0];
                                    thisUniformState.v[1] = v[1];
                                    thisUniformState.v[2] = v[2];
                                    thisUniformState.v[3] = v[3];
                                    f->glUniform4fv(uniform.glslLocation, 1, v);
                                }
                            } else {
                                f->glUniform4fv(uniform.glslLocation, 1, v);
                            }
                        } else {
                            f->glUniform4fv(uniform.glslLocation, elemCount, reinterpret_cast<const float *>(src));
                        }
                    }
                        break;
                    case QShaderDescription::Mat2:
                        f->glUniformMatrix2fv(uniform.glslLocation, 1, GL_FALSE, reinterpret_cast<const float *>(src));
                        break;
                    case QShaderDescription::Mat3:
                    {
                        const int elemCount = uniform.arrayDim;
                        if (elemCount < 1) {
                            // 4 floats per column (or row, if row-major)
                            float mat[9];
                            const float *srcMat = reinterpret_cast<const float *>(src);
                            memcpy(mat, srcMat, 3 * sizeof(float));
                            memcpy(mat + 3, srcMat + 4, 3 * sizeof(float));
                            memcpy(mat + 6, srcMat + 8, 3 * sizeof(float));
                            f->glUniformMatrix3fv(uniform.glslLocation, 1, GL_FALSE, mat);
                        } else {
                            m_scratch.packedArray.resize(elemCount * 9);
                            qrhi_std140_to_packed(&m_scratch.packedArray.data()->f, 3, elemCount * 3, src);
                            f->glUniformMatrix3fv(uniform.glslLocation, elemCount, GL_FALSE, &m_scratch.packedArray.constData()->f);
                        }
                    }
                        break;
                    case QShaderDescription::Mat4:
                        f->glUniformMatrix4fv(uniform.glslLocation, qMax(1, uniform.arrayDim), GL_FALSE, reinterpret_cast<const float *>(src));
                        break;
                    case QShaderDescription::Int:
                    {
                        const int elemCount = uniform.arrayDim;
                        if (elemCount < 1) {
                            f->glUniform1i(uniform.glslLocation, *reinterpret_cast<const qint32 *>(src));
                        } else {
                            m_scratch.packedArray.resize(elemCount);
                            qrhi_std140_to_packed(&m_scratch.packedArray.data()->i, 1, elemCount, src);
                            f->glUniform1iv(uniform.glslLocation, elemCount, &m_scratch.packedArray.constData()->i);
                        }
                    }
                        break;
                    case QShaderDescription::Int2:
                    {
                        const int elemCount = uniform.arrayDim;
                        if (elemCount < 1) {
                            f->glUniform2iv(uniform.glslLocation, 1, reinterpret_cast<const qint32 *>(src));
                        } else {
                            m_scratch.packedArray.resize(elemCount * 2);
                            qrhi_std140_to_packed(&m_scratch.packedArray.data()->i, 2, elemCount, src);
                            f->glUniform2iv(uniform.glslLocation, elemCount, &m_scratch.packedArray.constData()->i);
                        }
                    }
                        break;
                    case QShaderDescription::Int3:
                    {
                        const int elemCount = uniform.arrayDim;
                        if (elemCount < 1) {
                            f->glUniform3iv(uniform.glslLocation, 1, reinterpret_cast<const qint32 *>(src));
                        } else {
                            m_scratch.packedArray.resize(elemCount * 3);
                            qrhi_std140_to_packed(&m_scratch.packedArray.data()->i, 3, elemCount, src);
                            f->glUniform3iv(uniform.glslLocation, elemCount, &m_scratch.packedArray.constData()->i);
                        }
                    }
                        break;
                    case QShaderDescription::Int4:
                        f->glUniform4iv(uniform.glslLocation, qMax(1, uniform.arrayDim), reinterpret_cast<const qint32 *>(src));
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
            const QGles2SamplerDescriptionVector &samplers(maybeGraphicsPs ? QRHI_RES(QGles2GraphicsPipeline, maybeGraphicsPs)->samplers
                                                                           : QRHI_RES(QGles2ComputePipeline, maybeComputePs)->samplers);
            void *ps;
            uint psGeneration;
            if (maybeGraphicsPs) {
                ps = maybeGraphicsPs;
                psGeneration = QRHI_RES(QGles2GraphicsPipeline, maybeGraphicsPs)->generation;
            } else {
                ps = maybeComputePs;
                psGeneration = QRHI_RES(QGles2ComputePipeline, maybeComputePs)->generation;
            }
            for (int elem = 0; elem < b->u.stex.count; ++elem) {
                QGles2Texture *texD = QRHI_RES(QGles2Texture, b->u.stex.texSamplers[elem].tex);
                QGles2Sampler *samplerD = QRHI_RES(QGles2Sampler, b->u.stex.texSamplers[elem].sampler);
                for (const QGles2SamplerDescription &shaderSampler : samplers) {
                    if (shaderSampler.combinedBinding == b->binding) {
                        const int loc = shaderSampler.glslLocation + elem;
                        bindCombinedSampler(cbD, texD, samplerD, ps, psGeneration, loc, &texUnit, &activeTexUnitAltered);
                        break;
                    }
                }
            }
        }
            break;
        case QRhiShaderResourceBinding::Texture:
            for (int elem = 0; elem < b->u.stex.count; ++elem) {
                QGles2Texture *texD = QRHI_RES(QGles2Texture, b->u.stex.texSamplers[elem].tex);
                m_scratch.separateTextureBindings.append({ texD, b->binding, elem });
            }
            break;
        case QRhiShaderResourceBinding::Sampler:
        {
            QGles2Sampler *samplerD = QRHI_RES(QGles2Sampler, b->u.stex.texSamplers[0].sampler);
            m_scratch.separateSamplerBindings.append({ samplerD, b->binding });
        }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
        {
            QGles2Texture *texD = QRHI_RES(QGles2Texture, b->u.simage.tex);
            Q_ASSERT(texD->m_flags.testFlag(QRhiTexture::UsedWithLoadStore));
            // arrays, cubemaps, and 3D textures expose the whole texture with all layers/slices
            const bool layered = texD->m_flags.testFlag(QRhiTexture::CubeMap)
                                 || texD->m_flags.testFlag(QRhiTexture::ThreeDimensional)
                                 || texD->m_flags.testFlag(QRhiTexture::TextureArray);
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
            Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::StorageBuffer));
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

    if (!m_scratch.separateTextureBindings.isEmpty() || !m_scratch.separateSamplerBindings.isEmpty()) {
        const QGles2SamplerDescriptionVector &samplers(maybeGraphicsPs ? QRHI_RES(QGles2GraphicsPipeline, maybeGraphicsPs)->samplers
                                                                       : QRHI_RES(QGles2ComputePipeline, maybeComputePs)->samplers);
        void *ps;
        uint psGeneration;
        if (maybeGraphicsPs) {
            ps = maybeGraphicsPs;
            psGeneration = QRHI_RES(QGles2GraphicsPipeline, maybeGraphicsPs)->generation;
        } else {
            ps = maybeComputePs;
            psGeneration = QRHI_RES(QGles2ComputePipeline, maybeComputePs)->generation;
        }
        for (const QGles2SamplerDescription &shaderSampler : samplers) {
            if (shaderSampler.combinedBinding >= 0)
                continue;
            for (const Scratch::SeparateSampler &sepSampler : std::as_const(m_scratch.separateSamplerBindings)) {
                if (sepSampler.binding != shaderSampler.sbinding)
                    continue;
                for (const Scratch::SeparateTexture &sepTex : std::as_const(m_scratch.separateTextureBindings)) {
                    if (sepTex.binding != shaderSampler.tbinding)
                        continue;
                    const int loc = shaderSampler.glslLocation + sepTex.elem;
                    bindCombinedSampler(cbD, sepTex.texture, sepSampler.sampler, ps, psGeneration,
                                        loc, &texUnit, &activeTexUnitAltered);
                }
            }
        }
    }

    if (activeTexUnitAltered)
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

    QGles2CommandBuffer::Command &fbCmd(cbD->commands.get());
    fbCmd.cmd = QGles2CommandBuffer::Command::BindFramebuffer;

    static const bool doClearBuffers = qEnvironmentVariableIntValue("QT_GL_NO_CLEAR_BUFFERS") == 0;
    static const bool doClearColorBuffer = qEnvironmentVariableIntValue("QT_GL_NO_CLEAR_COLOR_BUFFER") == 0;

    switch (rt->resourceType()) {
    case QRhiResource::SwapChainRenderTarget:
        rtD = &QRHI_RES(QGles2SwapChainRenderTarget, rt)->d;
        if (wantsColorClear)
            *wantsColorClear = doClearBuffers && doClearColorBuffer;
        if (wantsDsClear)
            *wantsDsClear = doClearBuffers;
        fbCmd.args.bindFramebuffer.fbo = 0;
        fbCmd.args.bindFramebuffer.colorAttCount = 1;
        fbCmd.args.bindFramebuffer.stereo = rtD->stereoTarget.has_value();
        if (fbCmd.args.bindFramebuffer.stereo)
            fbCmd.args.bindFramebuffer.stereoTarget = rtD->stereoTarget.value();
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
        fbCmd.args.bindFramebuffer.stereo = false;

        for (auto it = rtTex->m_desc.cbeginColorAttachments(), itEnd = rtTex->m_desc.cendColorAttachments();
             it != itEnd; ++it)
        {
            const QRhiColorAttachment &colorAtt(*it);
            QGles2Texture *texD = QRHI_RES(QGles2Texture, colorAtt.texture());
            QGles2Texture *resolveTexD = QRHI_RES(QGles2Texture, colorAtt.resolveTexture());
            if (texD && cbD->passNeedsResourceTracking) {
                trackedRegisterTexture(&passResTracker, texD,
                                       QRhiPassResourceTracker::TexColorOutput,
                                       QRhiPassResourceTracker::TexColorOutputStage);
            }
            if (resolveTexD && cbD->passNeedsResourceTracking) {
                trackedRegisterTexture(&passResTracker, resolveTexD,
                                       QRhiPassResourceTracker::TexColorOutput,
                                       QRhiPassResourceTracker::TexColorOutputStage);
            }
            // renderbuffers cannot be written in shaders (no image store) so
            // they do not matter here
        }
        if (rtTex->m_desc.depthTexture() && cbD->passNeedsResourceTracking) {
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

    return rtD;
}

void QRhiGles2::enqueueBarriersForPass(QGles2CommandBuffer *cbD)
{
    cbD->passResTrackers.append(QRhiPassResourceTracker());
    cbD->currentPassResTrackerIndex = cbD->passResTrackers.size() - 1;
    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QGles2CommandBuffer::Command::BarriersForPass;
    cmd.args.barriersForPass.trackerIndex = cbD->currentPassResTrackerIndex;
}

void QRhiGles2::beginPass(QRhiCommandBuffer *cb,
                          QRhiRenderTarget *rt,
                          const QColor &colorClearValue,
                          const QRhiDepthStencilClearValue &depthStencilClearValue,
                          QRhiResourceUpdateBatch *resourceUpdates,
                          QRhiCommandBuffer::BeginPassFlags flags)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);

    // Get a new resource tracker. Then add a command that will generate
    // glMemoryBarrier() calls based on that tracker when submitted.
    enqueueBarriersForPass(cbD);

    if (rt->resourceType() == QRhiRenderTarget::TextureRenderTarget) {
        QGles2TextureRenderTarget *rtTex = QRHI_RES(QGles2TextureRenderTarget, rt);
        if (!QRhiRenderTargetAttachmentTracker::isUpToDate<QGles2Texture, QGles2RenderBuffer>(rtTex->description(), rtTex->d.currentResIdList))
            rtTex->create();
    }

    bool wantsColorClear, wantsDsClear;
    QGles2RenderTargetData *rtD = enqueueBindFramebuffer(rt, cbD, &wantsColorClear, &wantsDsClear);

    QGles2CommandBuffer::Command &clearCmd(cbD->commands.get());
    clearCmd.cmd = QGles2CommandBuffer::Command::Clear;
    clearCmd.args.clear.mask = 0;
    if (rtD->colorAttCount && wantsColorClear)
        clearCmd.args.clear.mask |= GL_COLOR_BUFFER_BIT;
    if (rtD->dsAttCount && wantsDsClear)
        clearCmd.args.clear.mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    clearCmd.args.clear.c[0] = colorClearValue.redF();
    clearCmd.args.clear.c[1] = colorClearValue.greenF();
    clearCmd.args.clear.c[2] = colorClearValue.blueF();
    clearCmd.args.clear.c[3] = colorClearValue.alphaF();
    clearCmd.args.clear.d = depthStencilClearValue.depthClearValue();
    clearCmd.args.clear.s = depthStencilClearValue.stencilClearValue();

    cbD->recordingPass = QGles2CommandBuffer::RenderPass;
    cbD->passNeedsResourceTracking = !flags.testFlag(QRhiCommandBuffer::DoNotTrackResourcesForCompute);
    cbD->currentTarget = rt;

    cbD->resetCachedState();
}

void QRhiGles2::endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QGles2CommandBuffer *cbD = QRHI_RES(QGles2CommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QGles2CommandBuffer::RenderPass);

    if (cbD->currentTarget->resourceType() == QRhiResource::TextureRenderTarget) {
        QGles2TextureRenderTarget *rtTex = QRHI_RES(QGles2TextureRenderTarget, cbD->currentTarget);
        for (auto it = rtTex->m_desc.cbeginColorAttachments(), itEnd = rtTex->m_desc.cendColorAttachments();
             it != itEnd; ++it)
        {
            const QRhiColorAttachment &colorAtt(*it);
            if (!colorAtt.resolveTexture())
                continue;

            QGles2Texture *resolveTexD = QRHI_RES(QGles2Texture, colorAtt.resolveTexture());
            const QSize size = resolveTexD->pixelSize();
            if (colorAtt.renderBuffer()) {
                QGles2RenderBuffer *rbD = QRHI_RES(QGles2RenderBuffer, colorAtt.renderBuffer());
                if (rbD->pixelSize() != size) {
                    qWarning("Resolve source (%dx%d) and target (%dx%d) size does not match",
                             rbD->pixelSize().width(), rbD->pixelSize().height(), size.width(), size.height());
                }
                if (caps.glesMultisampleRenderToTexture) {
                    // colorAtt.renderBuffer() is not actually used for anything if OpenGL ES'
                    // auto-resolving GL_EXT_multisampled_render_to_texture is used.
                } else {
                    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
                    cmd.cmd = QGles2CommandBuffer::Command::BlitFromRenderbuffer;
                    cmd.args.blitFromRenderbuffer.renderbuffer = rbD->renderbuffer;
                    cmd.args.blitFromRenderbuffer.w = size.width();
                    cmd.args.blitFromRenderbuffer.h = size.height();
                    if (resolveTexD->m_flags.testFlag(QRhiTexture::CubeMap))
                        cmd.args.blitFromRenderbuffer.target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + uint(colorAtt.resolveLayer());
                    else
                        cmd.args.blitFromRenderbuffer.target = resolveTexD->target;
                    cmd.args.blitFromRenderbuffer.dstTexture = resolveTexD->texture;
                    cmd.args.blitFromRenderbuffer.dstLevel = colorAtt.resolveLevel();
                    const bool hasZ = resolveTexD->m_flags.testFlag(QRhiTexture::ThreeDimensional)
                        || resolveTexD->m_flags.testFlag(QRhiTexture::TextureArray);
                    cmd.args.blitFromRenderbuffer.dstLayer = hasZ ? colorAtt.resolveLayer() : 0;
                    cmd.args.blitFromRenderbuffer.isDepthStencil = false;
                }
            } else if (caps.glesMultisampleRenderToTexture) {
                // Nothing to do, resolving into colorAtt.resolveTexture() is automatic,
                // colorAtt.texture() is in fact not used for anything.
            } else {
                Q_ASSERT(colorAtt.texture());
                QGles2Texture *texD = QRHI_RES(QGles2Texture, colorAtt.texture());
                if (texD->pixelSize() != size) {
                    qWarning("Resolve source (%dx%d) and target (%dx%d) size does not match",
                             texD->pixelSize().width(), texD->pixelSize().height(), size.width(), size.height());
                }
                const int resolveCount = colorAtt.multiViewCount() >= 2 ? colorAtt.multiViewCount() : 1;
                for (int resolveIdx = 0; resolveIdx < resolveCount; ++resolveIdx) {
                    const int srcLayer = colorAtt.layer() + resolveIdx;
                    const int dstLayer = colorAtt.resolveLayer() + resolveIdx;
                    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
                    cmd.cmd = QGles2CommandBuffer::Command::BlitFromTexture;
                    if (texD->m_flags.testFlag(QRhiTexture::CubeMap))
                        cmd.args.blitFromTexture.srcTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + uint(srcLayer);
                    else
                        cmd.args.blitFromTexture.srcTarget = texD->target;
                    cmd.args.blitFromTexture.srcTexture = texD->texture;
                    cmd.args.blitFromTexture.srcLevel = colorAtt.level();
                    cmd.args.blitFromTexture.srcLayer = 0;
                    if (texD->m_flags.testFlag(QRhiTexture::ThreeDimensional) || texD->m_flags.testFlag(QRhiTexture::TextureArray))
                        cmd.args.blitFromTexture.srcLayer = srcLayer;
                    cmd.args.blitFromTexture.w = size.width();
                    cmd.args.blitFromTexture.h = size.height();
                    if (resolveTexD->m_flags.testFlag(QRhiTexture::CubeMap))
                        cmd.args.blitFromTexture.dstTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + uint(dstLayer);
                    else
                        cmd.args.blitFromTexture.dstTarget = resolveTexD->target;
                    cmd.args.blitFromTexture.dstTexture = resolveTexD->texture;
                    cmd.args.blitFromTexture.dstLevel = colorAtt.resolveLevel();
                    cmd.args.blitFromTexture.dstLayer = 0;
                    if (resolveTexD->m_flags.testFlag(QRhiTexture::ThreeDimensional) || resolveTexD->m_flags.testFlag(QRhiTexture::TextureArray))
                        cmd.args.blitFromTexture.dstLayer = dstLayer;
                    cmd.args.blitFromTexture.isDepthStencil = false;
                }
            }
        }

        if (rtTex->m_desc.depthResolveTexture()) {
            QGles2Texture *depthResolveTexD = QRHI_RES(QGles2Texture, rtTex->m_desc.depthResolveTexture());
            const QSize size = depthResolveTexD->pixelSize();
            if (rtTex->m_desc.depthStencilBuffer()) {
                QGles2RenderBuffer *rbD = QRHI_RES(QGles2RenderBuffer, rtTex->m_desc.depthStencilBuffer());
                QGles2CommandBuffer::Command &cmd(cbD->commands.get());
                cmd.cmd = QGles2CommandBuffer::Command::BlitFromRenderbuffer;
                cmd.args.blitFromRenderbuffer.renderbuffer = rbD->renderbuffer;
                cmd.args.blitFromRenderbuffer.w = size.width();
                cmd.args.blitFromRenderbuffer.h = size.height();
                cmd.args.blitFromRenderbuffer.target = depthResolveTexD->target;
                cmd.args.blitFromRenderbuffer.dstTexture = depthResolveTexD->texture;
                cmd.args.blitFromRenderbuffer.dstLevel = 0;
                cmd.args.blitFromRenderbuffer.dstLayer = 0;
                cmd.args.blitFromRenderbuffer.isDepthStencil = true;
            } else if (caps.glesMultisampleRenderToTexture) {
                // Nothing to do, resolving into depthResolveTexture() is automatic.
            } else {
                QGles2Texture *depthTexD = QRHI_RES(QGles2Texture, rtTex->m_desc.depthTexture());
                const int resolveCount = depthTexD->arraySize() >= 2 ? depthTexD->arraySize() : 1;
                for (int resolveIdx = 0; resolveIdx < resolveCount; ++resolveIdx) {
                    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
                    cmd.cmd = QGles2CommandBuffer::Command::BlitFromTexture;
                    cmd.args.blitFromTexture.srcTarget = depthTexD->target;
                    cmd.args.blitFromTexture.srcTexture = depthTexD->texture;
                    cmd.args.blitFromTexture.srcLevel = 0;
                    cmd.args.blitFromTexture.srcLayer = resolveIdx;
                    cmd.args.blitFromTexture.w = size.width();
                    cmd.args.blitFromTexture.h = size.height();
                    cmd.args.blitFromTexture.dstTarget = depthResolveTexD->target;
                    cmd.args.blitFromTexture.dstTexture = depthResolveTexD->texture;
                    cmd.args.blitFromTexture.dstLevel = 0;
                    cmd.args.blitFromTexture.dstLayer = resolveIdx;
                    cmd.args.blitFromTexture.isDepthStencil = true;
                }
            }
        }

        const bool mayDiscardDepthStencil =
            (rtTex->m_desc.depthStencilBuffer()
             || (rtTex->m_desc.depthTexture() && rtTex->m_flags.testFlag(QRhiTextureRenderTarget::DoNotStoreDepthStencilContents)))
            && !rtTex->m_desc.depthResolveTexture();
        if (mayDiscardDepthStencil) {
            QGles2CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QGles2CommandBuffer::Command::InvalidateFramebuffer;
            if (caps.needsDepthStencilCombinedAttach) {
                cmd.args.invalidateFramebuffer.attCount = 1;
                cmd.args.invalidateFramebuffer.att[0] = GL_DEPTH_STENCIL_ATTACHMENT;
            } else {
                cmd.args.invalidateFramebuffer.attCount = 2;
                cmd.args.invalidateFramebuffer.att[0] = GL_DEPTH_ATTACHMENT;
                cmd.args.invalidateFramebuffer.att[1] = GL_STENCIL_ATTACHMENT;
            }
        }
    }

    cbD->recordingPass = QGles2CommandBuffer::NoPass;
    cbD->currentTarget = nullptr;

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);
}

void QRhiGles2::beginComputePass(QRhiCommandBuffer *cb,
                                 QRhiResourceUpdateBatch *resourceUpdates,
                                 QRhiCommandBuffer::BeginPassFlags)
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
        if (psD->lastUsedInFrameNo != frameNo) {
            psD->lastUsedInFrameNo = frameNo;
            psD->currentSrb = nullptr;
            psD->currentSrbGeneration = 0;
        }

        QGles2CommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QGles2CommandBuffer::Command::BindComputePipeline;
        cmd.args.bindComputePipeline.ps = ps;
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
        const int bindingCount = srbD->m_bindings.size();
        for (int i = 0; i < bindingCount; ++i) {
            const QRhiShaderResourceBinding::Data *b = shaderResourceBindingData(srbD->m_bindings.at(i));
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
            QGles2CommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QGles2CommandBuffer::Command::Barrier;
            cmd.args.barrier.barriers = barriers;
        }
    }

    QGles2CommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QGles2CommandBuffer::Command::Dispatch;
    cmd.args.dispatch.x = GLuint(x);
    cmd.args.dispatch.y = GLuint(y);
    cmd.args.dispatch.z = GLuint(z);
}

static inline GLenum toGlShaderType(QRhiShaderStage::Type type)
{
    switch (type) {
    case QRhiShaderStage::Vertex:
        return GL_VERTEX_SHADER;
    case QRhiShaderStage::TessellationControl:
        return GL_TESS_CONTROL_SHADER;
    case QRhiShaderStage::TessellationEvaluation:
        return GL_TESS_EVALUATION_SHADER;
    case QRhiShaderStage::Geometry:
        return GL_GEOMETRY_SHADER;
    case QRhiShaderStage::Fragment:
        return GL_FRAGMENT_SHADER;
    case QRhiShaderStage::Compute:
        return GL_COMPUTE_SHADER;
    default:
        Q_UNREACHABLE_RETURN(GL_VERTEX_SHADER);
    }
}

QByteArray QRhiGles2::shaderSource(const QRhiShaderStage &shaderStage, QShaderVersion *shaderVersion)
{
    const QShader bakedShader = shaderStage.shader();
    QList<int> versionsToTry;
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
                if (shaderVersion)
                    *shaderVersion = ver;
                break;
            }
        }
    } else {
        if (caps.ctxMajor > 4 || (caps.ctxMajor == 4 && caps.ctxMinor >= 6)) {
            versionsToTry << 460 << 450 << 440 << 430 << 420 << 410 << 400 << 330 << 150 << 140 << 130;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 5) {
            versionsToTry << 450 << 440 << 430 << 420 << 410 << 400 << 330 << 150 << 140 << 130;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 4) {
            versionsToTry << 440 << 430 << 420 << 410 << 400 << 330 << 150 << 140 << 130;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 3) {
            versionsToTry << 430 << 420 << 410 << 400 << 330 << 150 << 140 << 130;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 2) {
            versionsToTry << 420 << 410 << 400 << 330 << 150 << 140 << 130;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 1) {
            versionsToTry << 410 << 400 << 330 << 150 << 140 << 130;
        } else if (caps.ctxMajor == 4 && caps.ctxMinor == 0) {
            versionsToTry << 400 << 330 << 150 << 140 << 130;
        } else if (caps.ctxMajor == 3 && caps.ctxMinor == 3) {
            versionsToTry << 330 << 150 << 140 << 130;
        } else if (caps.ctxMajor == 3 && caps.ctxMinor == 2) {
            versionsToTry << 150 << 140 << 130;
        } else if (caps.ctxMajor == 3 && caps.ctxMinor == 1) {
            versionsToTry << 140 << 130;
        } else if (caps.ctxMajor == 3 && caps.ctxMinor == 0) {
            versionsToTry << 130;
        }
        if (!caps.coreProfile)
            versionsToTry << 120;
        for (int v : versionsToTry) {
            source = bakedShader.shader({ QShader::GlslShader, v, shaderStage.shaderVariant() }).shader();
            if (!source.isEmpty()) {
                if (shaderVersion)
                    *shaderVersion = v;
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

bool QRhiGles2::compileShader(GLuint program, const QRhiShaderStage &shaderStage, QShaderVersion *shaderVersion)
{
    const QByteArray source = shaderSource(shaderStage, shaderVersion);
    if (source.isEmpty())
        return false;

    GLuint shader;
    auto cacheIt = m_shaderCache.constFind(shaderStage);
    if (cacheIt != m_shaderCache.constEnd()) {
        shader = *cacheIt;
    } else {
        shader = f->glCreateShader(toGlShaderType(shaderStage.type()));
        const char *srcStr = source.constData();
        const GLint srcLength = source.size();
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
        if (m_shaderCache.size() >= MAX_SHADER_CACHE_ENTRIES) {
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
                                        ActiveUniformLocationTracker *activeUniformLocations,
                                        QGles2UniformDescriptionVector *dst)
{
    if (var.type == QShaderDescription::Struct) {
        qWarning("Nested structs are not supported at the moment. '%s' ignored.",
                 var.name.constData());
        return;
    }
    QGles2UniformDescription uniform;
    uniform.type = var.type;
    const QByteArray name = namePrefix + var.name;
    // Here we expect that the OpenGL implementation has proper active uniform
    // handling, meaning that a uniform that is declared but not accessed
    // elsewhere in the code is reported as -1 when querying the location. If
    // that is not the case, it won't break anything, but we'll generate
    // unnecessary glUniform* calls then.
    uniform.glslLocation = f->glGetUniformLocation(program, name.constData());
    if (uniform.glslLocation >= 0 && !activeUniformLocations->hasSeen(uniform.glslLocation)) {
        if (var.arrayDims.size() > 1) {
            qWarning("Array '%s' has more than one dimension. This is not supported.",
                     var.name.constData());
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
                               ActiveUniformLocationTracker *activeUniformLocations,
                               QGles2UniformDescriptionVector *dst)
{
    QByteArray prefix = ub.structName + '.';
    for (const QShaderDescription::BlockVariable &blockMember : ub.members) {
        if (blockMember.type == QShaderDescription::Struct) {
            QByteArray structPrefix = prefix + blockMember.name;

            const int baseOffset = blockMember.offset;
            if (blockMember.arrayDims.isEmpty()) {
                for (const QShaderDescription::BlockVariable &structMember : blockMember.structMembers)
                    registerUniformIfActive(structMember, structPrefix + ".", ub.binding,
                                            baseOffset, program, activeUniformLocations, dst);
            } else {
                if (blockMember.arrayDims.size() > 1) {
                    qWarning("Array of struct '%s' has more than one dimension. Only the first "
                             "dimension is used.",
                             blockMember.name.constData());
                }
                const int dim = blockMember.arrayDims.first();
                const int elemSize = blockMember.size / dim;
                int elemOffset = baseOffset;
                for (int di = 0; di < dim; ++di) {
                    const QByteArray arrayPrefix = structPrefix + '[' + QByteArray::number(di) + ']' + '.';
                    for (const QShaderDescription::BlockVariable &structMember : blockMember.structMembers)
                        registerUniformIfActive(structMember, arrayPrefix, ub.binding, elemOffset, program, activeUniformLocations, dst);
                    elemOffset += elemSize;
                }
            }
        } else {
            registerUniformIfActive(blockMember, prefix, ub.binding, 0, program, activeUniformLocations, dst);
        }
    }
}

void QRhiGles2::gatherSamplers(GLuint program,
                               const QShaderDescription::InOutVariable &v,
                               QGles2SamplerDescriptionVector *dst)
{
    QGles2SamplerDescription sampler;
    sampler.glslLocation = f->glGetUniformLocation(program, v.name.constData());
    if (sampler.glslLocation >= 0) {
        sampler.combinedBinding = v.binding;
        sampler.tbinding = -1;
        sampler.sbinding = -1;
        dst->append(sampler);
    }
}

void QRhiGles2::gatherGeneratedSamplers(GLuint program,
                                        const QShader::SeparateToCombinedImageSamplerMapping &mapping,
                                        QGles2SamplerDescriptionVector *dst)
{
    QGles2SamplerDescription sampler;
    sampler.glslLocation = f->glGetUniformLocation(program, mapping.combinedSamplerName.constData());
    if (sampler.glslLocation >= 0) {
        sampler.combinedBinding = -1;
        sampler.tbinding = mapping.textureBinding;
        sampler.sbinding = mapping.samplerBinding;
        dst->append(sampler);
    }
}

void QRhiGles2::sanityCheckVertexFragmentInterface(const QShaderDescription &vsDesc, const QShaderDescription &fsDesc)
{
    if (!vsDesc.isValid() || !fsDesc.isValid())
        return;

    // Print a warning if the fragment shader input for a given location uses a
    // name that does not match the vertex shader output at the same location.
    // This is not an error with any other API and not with GLSL >= 330 either,
    // but matters for older GLSL code that has no location qualifiers.
    for (const QShaderDescription::InOutVariable &outVar : vsDesc.outputVariables()) {
        for (const QShaderDescription::InOutVariable &inVar : fsDesc.inputVariables()) {
            if (inVar.location == outVar.location) {
                if (inVar.name != outVar.name) {
                    qWarning("Vertex output name '%s' does not match fragment input '%s'. "
                             "This should be avoided because it causes problems with older GLSL versions.",
                             outVar.name.constData(), inVar.name.constData());
                }
                break;
            }
        }
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
    case QRhiShaderStage::TessellationControl:
        return QShader::TessellationControlStage;
    case QRhiShaderStage::TessellationEvaluation:
        return QShader::TessellationEvaluationStage;
    case QRhiShaderStage::Geometry:
        return QShader::GeometryStage;
    case QRhiShaderStage::Fragment:
        return QShader::FragmentStage;
    case QRhiShaderStage::Compute:
        return QShader::ComputeStage;
    default:
        Q_UNREACHABLE_RETURN(QShader::VertexStage);
    }
}

QRhiGles2::ProgramCacheResult QRhiGles2::tryLoadFromDiskOrPipelineCache(const QRhiShaderStage *stages,
                                                                        int stageCount,
                                                                        GLuint program,
                                                                        const QVector<QShaderDescription::InOutVariable> &inputVars,
                                                                        QByteArray *cacheKey)
{
    Q_ASSERT(cacheKey);

    // the traditional QOpenGL disk cache since Qt 5.9
    const bool legacyDiskCacheEnabled = isProgramBinaryDiskCacheEnabled();

    // QRhi's own (set)PipelineCacheData()
    const bool pipelineCacheEnabled = caps.programBinary && !m_pipelineCache.isEmpty();

    // calculating the cache key based on the source code is common for both types of caches
    if (legacyDiskCacheEnabled || pipelineCacheEnabled) {
        QOpenGLProgramBinaryCache::ProgramDesc binaryProgram;
        for (int i = 0; i < stageCount; ++i) {
            const QRhiShaderStage &stage(stages[i]);
            QByteArray source = shaderSource(stage, nullptr);
            if (source.isEmpty())
                return QRhiGles2::ProgramCacheError;

            if (stage.type() == QRhiShaderStage::Vertex) {
                // Now add something to the key that indicates the vertex input locations.
                // A GLSL shader lower than 330 (150, 140, ...) will not have location
                // qualifiers. This means that the shader source code is the same
                // regardless of what locations inputVars contains. This becomes a problem
                // because we'll glBindAttribLocation the shader based on inputVars, but
                // that's only when compiling/linking when there was no cache hit. Picking
                // from the cache afterwards should take the input locations into account
                // since if inputVars has now different locations for the attributes, then
                // it is not ok to reuse a program binary that used different attribute
                // locations. For a lot of clients this would not be an issue since they
                // typically hardcode and use the same vertex locations on every run. Some
                // systems that dynamically generate shaders may end up with a non-stable
                // order (and so location numbers), however. This is sub-optimal because
                // it makes caching inefficient, and said clients should be fixed, but in
                // any case this should not break rendering. Hence including the locations
                // in the cache key.
                QMap<QByteArray, int> inputLocations; // sorted by key when iterating
                for (const QShaderDescription::InOutVariable &var : inputVars)
                    inputLocations.insert(var.name, var.location);
                source += QByteArrayLiteral("\n // "); // just to be nice; treated as an arbitrary string regardless
                for (auto it = inputLocations.cbegin(), end = inputLocations.cend(); it != end; ++it) {
                    source += it.key();
                    source += QByteArray::number(it.value());
                }
                source += QByteArrayLiteral("\n");
            }

            binaryProgram.shaders.append(QOpenGLProgramBinaryCache::ShaderDesc(toShaderStage(stage.type()), source));
        }

        *cacheKey = binaryProgram.cacheKey();

        // Try our pipeline cache simulation first, if it got seeded with
        // setPipelineCacheData and there's a hit, then no need to go to the
        // filesystem at all.
        if (pipelineCacheEnabled) {
            auto it = m_pipelineCache.constFind(*cacheKey);
            if (it != m_pipelineCache.constEnd()) {
                GLenum err;
                for ( ; ; ) {
                    err = f->glGetError();
                    if (err == GL_NO_ERROR || err == GL_CONTEXT_LOST)
                        break;
                }
                f->glProgramBinary(program, it->format, it->data.constData(), it->data.size());
                err = f->glGetError();
                if (err == GL_NO_ERROR) {
                    GLint linkStatus = 0;
                    f->glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
                    if (linkStatus == GL_TRUE)
                        return QRhiGles2::ProgramCacheHit;
                }
            }
        }

        if (legacyDiskCacheEnabled && qrhi_programBinaryCache()->load(*cacheKey, program)) {
            // use the logging category QOpenGLShaderProgram would
            qCDebug(lcOpenGLProgramDiskCache, "Program binary received from cache, program %u, key %s",
                    program, cacheKey->constData());
            return QRhiGles2::ProgramCacheHit;
        }
    }

    return QRhiGles2::ProgramCacheMiss;
}

void QRhiGles2::trySaveToDiskCache(GLuint program, const QByteArray &cacheKey)
{
    // This is only for the traditional QOpenGL disk cache since Qt 5.9.

    if (isProgramBinaryDiskCacheEnabled()) {
        // use the logging category QOpenGLShaderProgram would
        qCDebug(lcOpenGLProgramDiskCache, "Saving program binary, program %u, key %s",
                program, cacheKey.constData());
        qrhi_programBinaryCache()->save(cacheKey, program);
    }
}

void QRhiGles2::trySaveToPipelineCache(GLuint program, const QByteArray &cacheKey, bool force)
{
    // This handles our own simulated "pipeline cache". (specific to QRhi, not
    // shared with legacy QOpenGL* stuff)

    if (caps.programBinary && (force || !m_pipelineCache.contains(cacheKey))) {
        GLint blobSize = 0;
        f->glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &blobSize);
        QByteArray blob(blobSize, Qt::Uninitialized);
        GLint outSize = 0;
        GLenum binaryFormat = 0;
        f->glGetProgramBinary(program, blobSize, &outSize, &binaryFormat, blob.data());
        if (blobSize == outSize)
            m_pipelineCache.insert(cacheKey, { binaryFormat, blob });
    }
}

QGles2Buffer::QGles2Buffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size)
    : QRhiBuffer(rhi, type, usage, size)
{
}

QGles2Buffer::~QGles2Buffer()
{
    destroy();
}

void QGles2Buffer::destroy()
{
    data.clear();
    if (!buffer)
        return;

    QRhiGles2::DeferredReleaseEntry e;
    e.type = QRhiGles2::DeferredReleaseEntry::Buffer;

    e.buffer.buffer = buffer;
    buffer = 0;

    QRHI_RES_RHI(QRhiGles2);
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QGles2Buffer::create()
{
    if (buffer)
        destroy();

    QRHI_RES_RHI(QRhiGles2);

    nonZeroSize = m_size <= 0 ? 256 : m_size;

    if (m_usage.testFlag(QRhiBuffer::UniformBuffer)) {
        if (int(m_usage) != QRhiBuffer::UniformBuffer) {
            qWarning("Uniform buffer: multiple usages specified, this is not supported by the OpenGL backend");
            return false;
        }
        data.resize(nonZeroSize);
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

    if (rhiD->glObjectLabel)
        rhiD->glObjectLabel(GL_BUFFER, buffer, -1, m_objectName.constData());

    usageState.access = AccessNone;

    rhiD->registerResource(this);
    return true;
}

QRhiBuffer::NativeBuffer QGles2Buffer::nativeBuffer()
{
    if (m_usage.testFlag(QRhiBuffer::UniformBuffer))
        return { {}, 0 };

    return { { &buffer }, 1 };
}

char *QGles2Buffer::beginFullDynamicBufferUpdateForCurrentFrame()
{
    Q_ASSERT(m_type == Dynamic);
    if (!m_usage.testFlag(UniformBuffer)) {
        QRHI_RES_RHI(QRhiGles2);
        rhiD->f->glBindBuffer(targetForDataOps, buffer);
        if (rhiD->caps.properMapBuffer) {
            return static_cast<char *>(rhiD->f->glMapBufferRange(targetForDataOps, 0, nonZeroSize,
                                                                 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
        } else {
            // Need some storage for the data, use the otherwise unused 'data' member.
            if (data.isEmpty())
                data.resize(nonZeroSize);
        }
    }
    return data.data();
}

void QGles2Buffer::endFullDynamicBufferUpdateForCurrentFrame()
{
    if (!m_usage.testFlag(UniformBuffer)) {
        QRHI_RES_RHI(QRhiGles2);
        rhiD->f->glBindBuffer(targetForDataOps, buffer);
        if (rhiD->caps.properMapBuffer)
            rhiD->f->glUnmapBuffer(targetForDataOps);
        else
            rhiD->f->glBufferSubData(targetForDataOps, 0, nonZeroSize, data.data());
    }
}

void QGles2Buffer::fullDynamicBufferUpdateForCurrentFrame(const void *bufferData, quint32 size)
{
    const quint32 copySize = size > 0 ? size : m_size;
    if (!m_usage.testFlag(UniformBuffer)) {
        QRHI_RES_RHI(QRhiGles2);
        rhiD->f->glBindBuffer(targetForDataOps, buffer);
        rhiD->f->glBufferSubData(targetForDataOps, 0, copySize, bufferData);
    } else {
        memcpy(data.data(), bufferData, copySize);
    }
}

QGles2RenderBuffer::QGles2RenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                                       int sampleCount, QRhiRenderBuffer::Flags flags,
                                       QRhiTexture::Format backingFormatHint)
    : QRhiRenderBuffer(rhi, type, pixelSize, sampleCount, flags, backingFormatHint)
{
}

QGles2RenderBuffer::~QGles2RenderBuffer()
{
    destroy();
}

void QGles2RenderBuffer::destroy()
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
    if (rhiD) {
        if (owns)
            rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QGles2RenderBuffer::create()
{
    if (renderbuffer)
        destroy();

    QRHI_RES_RHI(QRhiGles2);
    samples = rhiD->effectiveSampleCount(m_sampleCount);

    if (m_flags.testFlag(UsedWithSwapChainOnly)) {
        if (m_type == DepthStencil)
            return true;

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
            if (rhiD->caps.glesMultisampleRenderToTexture) {
                // Must match the logic in QGles2TextureRenderTarget::create().
                // EXT and non-EXT are not the same thing.
                rhiD->glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8,
                                                          size.width(), size.height());
            } else {
                rhiD->f->glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8,
                                                          size.width(), size.height());
            }
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
        break;
    case QRhiRenderBuffer::Color:
    {
        GLenum internalFormat = GL_RGBA4; // ES 2.0
        if (rhiD->caps.rgba8Format) {
            internalFormat = GL_RGBA8;
            if (m_backingFormatHint != QRhiTexture::UnknownFormat) {
                GLenum glintformat, glformat, gltype;
                // only care about the sized internal format, the rest is not used here
                toGlTextureFormat(m_backingFormatHint, rhiD->caps,
                                  &glintformat, &internalFormat, &glformat, &gltype);
            }
        }
        if (rhiD->caps.msaaRenderBuffer && samples > 1) {
            rhiD->f->glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, internalFormat,
                                                      size.width(), size.height());
        } else {
            rhiD->f->glRenderbufferStorage(GL_RENDERBUFFER, internalFormat,
                                           size.width(), size.height());
        }
    }
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    if (rhiD->glObjectLabel)
        rhiD->glObjectLabel(GL_RENDERBUFFER, renderbuffer, -1, m_objectName.constData());

    owns = true;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

bool QGles2RenderBuffer::createFrom(NativeRenderBuffer src)
{
    if (!src.object)
        return false;

    if (renderbuffer)
        destroy();

    QRHI_RES_RHI(QRhiGles2);
    samples = rhiD->effectiveSampleCount(m_sampleCount);

    if (m_flags.testFlag(UsedWithSwapChainOnly))
        qWarning("RenderBuffer: UsedWithSwapChainOnly is meaningless when importing an existing native object");

    if (!rhiD->ensureContext())
        return false;

    renderbuffer = src.object;

    owns = false;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::Format QGles2RenderBuffer::backingFormat() const
{
    if (m_backingFormatHint != QRhiTexture::UnknownFormat)
        return m_backingFormatHint;
    else
        return m_type == Color ? QRhiTexture::RGBA8 : QRhiTexture::UnknownFormat;
}

QGles2Texture::QGles2Texture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
                             int arraySize, int sampleCount, Flags flags)
    : QRhiTexture(rhi, format, pixelSize, depth, arraySize, sampleCount, flags)
{
}

QGles2Texture::~QGles2Texture()
{
    destroy();
}

void QGles2Texture::destroy()
{
    if (!texture)
        return;

    QRhiGles2::DeferredReleaseEntry e;
    e.type = QRhiGles2::DeferredReleaseEntry::Texture;

    e.texture.texture = texture;

    texture = 0;
    specified = false;
    zeroInitialized = false;

    QRHI_RES_RHI(QRhiGles2);
    if (rhiD) {
        if (owns)
            rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QGles2Texture::prepareCreate(QSize *adjustedSize)
{
    if (texture)
        destroy();

    QRHI_RES_RHI(QRhiGles2);
    if (!rhiD->ensureContext())
        return false;

    const bool isCube = m_flags.testFlag(CubeMap);
    const bool isArray = m_flags.testFlag(QRhiTexture::TextureArray);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);
    const bool isCompressed = rhiD->isCompressedFormat(m_format);
    const bool is1D = m_flags.testFlag(OneDimensional);

    const QSize size = is1D ? QSize(qMax(1, m_pixelSize.width()), 1)
                            : (m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize);

    samples = rhiD->effectiveSampleCount(m_sampleCount);

    if (is3D && !rhiD->caps.texture3D) {
        qWarning("3D textures are not supported");
        return false;
    }
    if (isCube && is3D) {
        qWarning("Texture cannot be both cube and 3D");
        return false;
    }
    if (isArray && is3D) {
        qWarning("Texture cannot be both array and 3D");
        return false;
    }
    if (is1D && !rhiD->caps.texture1D) {
        qWarning("1D textures are not supported");
        return false;
    }
    if (is1D && is3D) {
        qWarning("Texture cannot be both 1D and 3D");
        return false;
    }
    if (is1D && isCube) {
        qWarning("Texture cannot be both 1D and cube");
        return false;
    }

    if (m_depth > 1 && !is3D) {
        qWarning("Texture cannot have a depth of %d when it is not 3D", m_depth);
        return false;
    }
    if (m_arraySize > 0 && !isArray) {
        qWarning("Texture cannot have an array size of %d when it is not an array", m_arraySize);
        return false;
    }
    if (m_arraySize < 1 && isArray) {
        qWarning("Texture is an array but array size is %d", m_arraySize);
        return false;
    }

    target = isCube             ? GL_TEXTURE_CUBE_MAP
            : samples > 1 ? (isArray ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_MULTISAMPLE)
                                : (is3D ? GL_TEXTURE_3D
                                        : (is1D ? (isArray ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D)
                                                : (isArray ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D)));

    if (m_flags.testFlag(ExternalOES))
        target = GL_TEXTURE_EXTERNAL_OES;
    else if (m_flags.testFlag(TextureRectangleGL))
        target = GL_TEXTURE_RECTANGLE;

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
        toGlTextureFormat(m_format, rhiD->caps,
                          &glintformat, &glsizedintformat, &glformat, &gltype);
    }

    samplerState = QGles2SamplerData();

    usageState.access = AccessNone;

    if (adjustedSize)
        *adjustedSize = size;

    return true;
}

bool QGles2Texture::create()
{
    QSize size;
    if (!prepareCreate(&size))
        return false;

    QRHI_RES_RHI(QRhiGles2);
    rhiD->f->glGenTextures(1, &texture);

    const bool isCube = m_flags.testFlag(CubeMap);
    const bool isArray = m_flags.testFlag(QRhiTexture::TextureArray);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);
    const bool isCompressed = rhiD->isCompressedFormat(m_format);
    const bool is1D = m_flags.testFlag(OneDimensional);

    if (!isCompressed) {
        rhiD->f->glBindTexture(target, texture);
        if (!m_flags.testFlag(UsedWithLoadStore)) {
            if (is1D) {
                for (int level = 0; level < mipLevelCount; ++level) {
                    const QSize mipSize = rhiD->q->sizeForMipLevel(level, size);
                    if (isArray)
                        rhiD->f->glTexImage2D(target, level, GLint(glintformat), mipSize.width(),
                                              qMax(0, m_arraySize), 0, glformat, gltype, nullptr);
                    else
                        rhiD->glTexImage1D(target, level, GLint(glintformat), mipSize.width(), 0,
                                           glformat, gltype, nullptr);
                }
            } else if (isArray) {
                const int layerCount = qMax(0, m_arraySize);
                if (hasMipMaps) {
                    for (int level = 0; level != mipLevelCount; ++level) {
                        const QSize mipSize = rhiD->q->sizeForMipLevel(level, size);
                        rhiD->f->glTexImage3D(target, level, GLint(glintformat), mipSize.width(), mipSize.height(), layerCount,
                                              0, glformat, gltype, nullptr);
                    }
                } else {
                    rhiD->f->glTexImage3D(target, 0, GLint(glintformat), size.width(), size.height(), layerCount,
                                          0, glformat, gltype, nullptr);
                }
            } else if (is3D) {
                if (hasMipMaps) {
                    const int depth = qMax(1, m_depth);
                    for (int level = 0; level != mipLevelCount; ++level) {
                        const QSize mipSize = rhiD->q->sizeForMipLevel(level, size);
                        const int mipDepth = rhiD->q->sizeForMipLevel(level, QSize(depth, depth)).width();
                        rhiD->f->glTexImage3D(target, level, GLint(glintformat), mipSize.width(), mipSize.height(), mipDepth,
                                              0, glformat, gltype, nullptr);
                    }
                } else {
                    rhiD->f->glTexImage3D(target, 0, GLint(glintformat), size.width(), size.height(), qMax(1, m_depth),
                                          0, glformat, gltype, nullptr);
                }
            } else if (hasMipMaps || isCube) {
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
                // 2D texture. For multisample textures the GLES 3.1
                // glStorage2DMultisample must be used for portability.
                if (samples > 1 && rhiD->caps.multisampledTexture) {
                    // internal format must be sized
                    rhiD->f->glTexStorage2DMultisample(target, samples, glsizedintformat,
                                                       size.width(), size.height(), GL_TRUE);
                } else {
                    rhiD->f->glTexImage2D(target, 0, GLint(glintformat), size.width(), size.height(),
                                          0, glformat, gltype, nullptr);
                }
            }
        } else {
            // Must be specified with immutable storage functions otherwise
            // bindImageTexture may fail. Also, the internal format must be a
            // sized format here.
            if (is1D && !isArray)
                rhiD->glTexStorage1D(target, mipLevelCount, glsizedintformat, size.width());
            else if (!is1D && (is3D || isArray))
                rhiD->f->glTexStorage3D(target, mipLevelCount, glsizedintformat, size.width(), size.height(),
                                        is3D ? qMax(1, m_depth) : qMax(0, m_arraySize));
            else if (samples > 1)
                rhiD->f->glTexStorage2DMultisample(target, samples, glsizedintformat,
                                                   size.width(), size.height(), GL_TRUE);
            else
                rhiD->f->glTexStorage2D(target, mipLevelCount, glsizedintformat, size.width(),
                                        is1D ? qMax(0, m_arraySize) : size.height());
        }
        // Make sure the min filter is set to something non-mipmap-based already
        // here, given the ridiculous default of GL. It is changed based on
        // the sampler later, but there could be cases when one pulls the native
        // object out via nativeTexture() right away.
        rhiD->f->glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        specified = true;
    } else {
        // Cannot use glCompressedTexImage2D without valid data, so defer.
        // Compressed textures will not be used as render targets so this is
        // not an issue.
        specified = false;
    }

    if (rhiD->glObjectLabel)
        rhiD->glObjectLabel(GL_TEXTURE, texture, -1, m_objectName.constData());

    owns = true;

    generation += 1;
    rhiD->registerResource(this);
    return true;
}

bool QGles2Texture::createFrom(QRhiTexture::NativeTexture src)
{
    const uint textureId = uint(src.object);
    if (textureId == 0)
        return false;

    if (!prepareCreate())
        return false;

    texture = textureId;
    specified = true;
    zeroInitialized = true;

    owns = false;

    generation += 1;
    QRHI_RES_RHI(QRhiGles2);
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::NativeTexture QGles2Texture::nativeTexture()
{
    return {texture, 0};
}

QGles2Sampler::QGles2Sampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                             AddressMode u, AddressMode v, AddressMode w)
    : QRhiSampler(rhi, magFilter, minFilter, mipmapMode, u, v, w)
{
}

QGles2Sampler::~QGles2Sampler()
{
    destroy();
}

void QGles2Sampler::destroy()
{
    QRHI_RES_RHI(QRhiGles2);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QGles2Sampler::create()
{
    d.glminfilter = toGlMinFilter(m_minFilter, m_mipmapMode);
    d.glmagfilter = toGlMagFilter(m_magFilter);
    d.glwraps = toGlWrapMode(m_addressU);
    d.glwrapt = toGlWrapMode(m_addressV);
    d.glwrapr = toGlWrapMode(m_addressW);
    d.gltexcomparefunc = toGlTextureCompareFunc(m_compareOp);

    generation += 1;
    QRHI_RES_RHI(QRhiGles2);
    rhiD->registerResource(this, false);
    return true;
}

// dummy, no Vulkan-style RenderPass+Framebuffer concept here
QGles2RenderPassDescriptor::QGles2RenderPassDescriptor(QRhiImplementation *rhi)
    : QRhiRenderPassDescriptor(rhi)
{
}

QGles2RenderPassDescriptor::~QGles2RenderPassDescriptor()
{
    destroy();
}

void QGles2RenderPassDescriptor::destroy()
{
    QRHI_RES_RHI(QRhiGles2);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QGles2RenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const
{
    Q_UNUSED(other);
    return true;
}

QRhiRenderPassDescriptor *QGles2RenderPassDescriptor::newCompatibleRenderPassDescriptor() const
{
    QGles2RenderPassDescriptor *rpD = new QGles2RenderPassDescriptor(m_rhi);
    QRHI_RES_RHI(QRhiGles2);
    rhiD->registerResource(rpD, false);
    return rpD;
}

QVector<quint32> QGles2RenderPassDescriptor::serializedFormat() const
{
    return {};
}

QGles2SwapChainRenderTarget::QGles2SwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain)
    : QRhiSwapChainRenderTarget(rhi, swapchain),
      d(rhi)
{
}

QGles2SwapChainRenderTarget::~QGles2SwapChainRenderTarget()
{
    destroy();
}

void QGles2SwapChainRenderTarget::destroy()
{
    // nothing to do here
}

QSize QGles2SwapChainRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QGles2SwapChainRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QGles2SwapChainRenderTarget::sampleCount() const
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
    destroy();
}

void QGles2TextureRenderTarget::destroy()
{
    if (!framebuffer)
        return;

    QRhiGles2::DeferredReleaseEntry e;
    e.type = QRhiGles2::DeferredReleaseEntry::TextureRenderTarget;

    e.textureRenderTarget.framebuffer = framebuffer;
    e.textureRenderTarget.nonMsaaThrowawayDepthTexture = nonMsaaThrowawayDepthTexture;

    framebuffer = 0;
    nonMsaaThrowawayDepthTexture = 0;

    QRHI_RES_RHI(QRhiGles2);
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

QRhiRenderPassDescriptor *QGles2TextureRenderTarget::newCompatibleRenderPassDescriptor()
{
    QGles2RenderPassDescriptor *rpD = new QGles2RenderPassDescriptor(m_rhi);
    QRHI_RES_RHI(QRhiGles2);
    rhiD->registerResource(rpD, false);
    return rpD;
}

bool QGles2TextureRenderTarget::create()
{
    QRHI_RES_RHI(QRhiGles2);

    if (framebuffer)
        destroy();

    const bool hasColorAttachments = m_desc.colorAttachmentCount() > 0;
    Q_ASSERT(hasColorAttachments || m_desc.depthTexture());
    Q_ASSERT(!m_desc.depthStencilBuffer() || !m_desc.depthTexture());
    const bool hasDepthStencil = m_desc.depthStencilBuffer() || m_desc.depthTexture();

    if (hasColorAttachments) {
        const int count = int(m_desc.colorAttachmentCount());
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
    int multiViewCount = 0;
    for (auto it = m_desc.cbeginColorAttachments(), itEnd = m_desc.cendColorAttachments(); it != itEnd; ++it, ++attIndex) {
        d.colorAttCount += 1;
        const QRhiColorAttachment &colorAtt(*it);
        QRhiTexture *texture = colorAtt.texture();
        QRhiRenderBuffer *renderBuffer = colorAtt.renderBuffer();
        Q_ASSERT(texture || renderBuffer);
        if (texture) {
            QGles2Texture *texD = QRHI_RES(QGles2Texture, texture);
            Q_ASSERT(texD->texture && texD->specified);
            if (texD->flags().testFlag(QRhiTexture::ThreeDimensional) || texD->flags().testFlag(QRhiTexture::TextureArray)) {
                if (colorAtt.multiViewCount() < 2) {
                    rhiD->f->glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + uint(attIndex), texD->texture,
                                                       colorAtt.level(), colorAtt.layer());
                } else {
                    multiViewCount = colorAtt.multiViewCount();
                    if (texD->samples > 1 && rhiD->caps.glesMultiviewMultisampleRenderToTexture && colorAtt.resolveTexture()) {
                        // Special path for GLES and GL_OVR_multiview_multisampled_render_to_texture:
                        // ignore the color attachment's (multisample) texture
                        // array and give the resolve texture array to GL. (no
                        // explicit resolving is needed by us later on)
                        QGles2Texture *resolveTexD = QRHI_RES(QGles2Texture, colorAtt.resolveTexture());
                        rhiD->glFramebufferTextureMultisampleMultiviewOVR(GL_FRAMEBUFFER,
                                                                          GL_COLOR_ATTACHMENT0 + uint(attIndex),
                                                                          resolveTexD->texture,
                                                                          colorAtt.resolveLevel(),
                                                                          texD->samples,
                                                                          colorAtt.resolveLayer(),
                                                                          multiViewCount);
                    } else {
                        rhiD->glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER,
                                                               GL_COLOR_ATTACHMENT0 + uint(attIndex),
                                                               texD->texture,
                                                               colorAtt.level(),
                                                               colorAtt.layer(),
                                                               multiViewCount);
                    }
                }
            } else if (texD->flags().testFlag(QRhiTexture::OneDimensional)) {
                rhiD->glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + uint(attIndex),
                                             texD->target + uint(colorAtt.layer()), texD->texture,
                                             colorAtt.level());
            } else {
                if (texD->samples > 1 && rhiD->caps.glesMultisampleRenderToTexture && colorAtt.resolveTexture()) {
                    // Special path for GLES and GL_EXT_multisampled_render_to_texture:
                    // ignore the color attachment's (multisample) texture and
                    // give the resolve texture to GL. (no explicit resolving is
                    // needed by us later on)
                    QGles2Texture *resolveTexD = QRHI_RES(QGles2Texture, colorAtt.resolveTexture());
                    const GLenum faceTargetBase = resolveTexD->flags().testFlag(QRhiTexture::CubeMap) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : resolveTexD->target;
                    rhiD->glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + uint(attIndex), faceTargetBase + uint(colorAtt.resolveLayer()),
                                                               resolveTexD->texture, colorAtt.level(), texD->samples);
                } else {
                    const GLenum faceTargetBase = texD->flags().testFlag(QRhiTexture::CubeMap) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : texD->target;
                    rhiD->f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + uint(attIndex), faceTargetBase + uint(colorAtt.layer()),
                                                    texD->texture, colorAtt.level());
                }
            }
            if (attIndex == 0) {
                d.pixelSize = rhiD->q->sizeForMipLevel(colorAtt.level(), texD->pixelSize());
                d.sampleCount = texD->samples;
            }
        } else if (renderBuffer) {
            QGles2RenderBuffer *rbD = QRHI_RES(QGles2RenderBuffer, renderBuffer);
            if (rbD->samples > 1 && rhiD->caps.glesMultisampleRenderToTexture && colorAtt.resolveTexture()) {
                // Special path for GLES and GL_EXT_multisampled_render_to_texture: ignore
                // the (multisample) renderbuffer and give the resolve texture to GL. (so
                // no explicit resolve; depending on GL implementation internals, this may
                // play nicer with tiled architectures)
                QGles2Texture *resolveTexD = QRHI_RES(QGles2Texture, colorAtt.resolveTexture());
                const GLenum faceTargetBase = resolveTexD->flags().testFlag(QRhiTexture::CubeMap) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : resolveTexD->target;
                rhiD->glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + uint(attIndex), faceTargetBase + uint(colorAtt.resolveLayer()),
                                                           resolveTexD->texture, colorAtt.level(), rbD->samples);
            } else {
                rhiD->f->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + uint(attIndex), GL_RENDERBUFFER, rbD->renderbuffer);
            }
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
                if (depthRbD->stencilRenderbuffer) {
                    rhiD->f->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                                       depthRbD->stencilRenderbuffer);
                } else {
                    // packed depth-stencil
                    rhiD->f->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                                       depthRbD->renderbuffer);
                }
            }
            if (d.colorAttCount == 0) {
                d.pixelSize = depthRbD->pixelSize();
                d.sampleCount = depthRbD->samples;
            }
        } else {
            QGles2Texture *depthTexD = QRHI_RES(QGles2Texture, m_desc.depthTexture());
            if (multiViewCount < 2) {
                if (depthTexD->samples > 1 && rhiD->caps.glesMultisampleRenderToTexture && m_desc.depthResolveTexture()) {
                    // Special path for GLES and
                    // GL_EXT_multisampled_render_to_texture, for depth-stencil.
                    // Relevant only when depthResolveTexture is set.
                    QGles2Texture *depthResolveTexD = QRHI_RES(QGles2Texture, m_desc.depthResolveTexture());
                    rhiD->glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthResolveTexD->target,
                                                               depthResolveTexD->texture, 0, depthTexD->samples);
                    if (rhiD->isStencilSupportingFormat(depthResolveTexD->format())) {
                        rhiD->glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, depthResolveTexD->target,
                                                                   depthResolveTexD->texture, 0, depthTexD->samples);
                    }
                } else {
                    rhiD->f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexD->target,
                                                    depthTexD->texture, 0);
                    if (rhiD->isStencilSupportingFormat(depthTexD->format())) {
                        rhiD->f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, depthTexD->target,
                                                        depthTexD->texture, 0);
                    }
                }
            } else {
                if (depthTexD->samples > 1 && rhiD->caps.glesMultiviewMultisampleRenderToTexture) {
                    // And so it turns out
                    // https://registry.khronos.org/OpenGL/extensions/OVR/OVR_multiview.txt
                    // does not work with multisample 2D texture arrays. (at least
                    // that's what Issue 30 in the extension spec seems to imply)
                    //
                    // There is https://registry.khronos.org/OpenGL/extensions/EXT/EXT_multiview_texture_multisample.txt
                    // that seems to resolve that, but that does not seem to
                    // work (or not available) on GLES devices such as the Quest 3.
                    //
                    // So instead, on GLES we can use the
                    // multisample-multiview-auto-resolving version (which in
                    // turn is not supported on desktop GL e.g. by NVIDIA), too
                    // bad we have a multisample depth texture array here as
                    // every other API out there requires that. So, in absence
                    // of a depthResolveTexture, create a temporary one ignoring
                    // what the user has already created.
                    //
                    if (!m_flags.testFlag(DoNotStoreDepthStencilContents) && !m_desc.depthResolveTexture()) {
                        qWarning("Attempted to create a multiview+multisample QRhiTextureRenderTarget, but DoNotStoreDepthStencilContents was not set."
                                 " This path has no choice but to behave as if DoNotStoreDepthStencilContents was set, because QRhi is forced to create"
                                 " a throwaway non-multisample depth texture here. Set the flag to silence this warning, or set a depthResolveTexture.");
                    }
                    if (m_desc.depthResolveTexture()) {
                        QGles2Texture *depthResolveTexD = QRHI_RES(QGles2Texture, m_desc.depthResolveTexture());
                        rhiD->glFramebufferTextureMultisampleMultiviewOVR(GL_FRAMEBUFFER,
                                                                          GL_DEPTH_ATTACHMENT,
                                                                          depthResolveTexD->texture,
                                                                          0,
                                                                          depthTexD->samples,
                                                                          0,
                                                                          multiViewCount);
                        if (rhiD->isStencilSupportingFormat(depthResolveTexD->format())) {
                            rhiD->glFramebufferTextureMultisampleMultiviewOVR(GL_FRAMEBUFFER,
                                                                              GL_STENCIL_ATTACHMENT,
                                                                              depthResolveTexD->texture,
                                                                              0,
                                                                              depthTexD->samples,
                                                                              0,
                                                                              multiViewCount);
                        }
                    } else {
                        if (!nonMsaaThrowawayDepthTexture) {
                            rhiD->f->glGenTextures(1, &nonMsaaThrowawayDepthTexture);
                            rhiD->f->glBindTexture(GL_TEXTURE_2D_ARRAY, nonMsaaThrowawayDepthTexture);
                            rhiD->f->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH24_STENCIL8,
                                                    depthTexD->pixelSize().width(), depthTexD->pixelSize().height(), multiViewCount);
                        }
                        rhiD->glFramebufferTextureMultisampleMultiviewOVR(GL_FRAMEBUFFER,
                                                                          GL_DEPTH_ATTACHMENT,
                                                                          nonMsaaThrowawayDepthTexture,
                                                                          0,
                                                                          depthTexD->samples,
                                                                          0,
                                                                          multiViewCount);
                        rhiD->glFramebufferTextureMultisampleMultiviewOVR(GL_FRAMEBUFFER,
                                                                          GL_STENCIL_ATTACHMENT,
                                                                          nonMsaaThrowawayDepthTexture,
                                                                          0,
                                                                          depthTexD->samples,
                                                                          0,
                                                                          multiViewCount);
                    }
                } else {
                    // The depth texture here must be an array with at least
                    // multiViewCount elements, and the format should be D24 or D32F
                    // for depth only, or D24S8 for depth and stencil.
                    rhiD->glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexD->texture,
                                                           0, 0, multiViewCount);
                    if (rhiD->isStencilSupportingFormat(depthTexD->format())) {
                        rhiD->glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, depthTexD->texture,
                                                               0, 0, multiViewCount);
                    }
                }
            }
            if (d.colorAttCount == 0) {
                d.pixelSize = depthTexD->pixelSize();
                d.sampleCount = depthTexD->samples;
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

    if (rhiD->glObjectLabel)
        rhiD->glObjectLabel(GL_FRAMEBUFFER, framebuffer, -1, m_objectName.constData());

    QRhiRenderTargetAttachmentTracker::updateResIdList<QGles2Texture, QGles2RenderBuffer>(m_desc, &d.currentResIdList);

    rhiD->registerResource(this);
    return true;
}

QSize QGles2TextureRenderTarget::pixelSize() const
{
    if (!QRhiRenderTargetAttachmentTracker::isUpToDate<QGles2Texture, QGles2RenderBuffer>(m_desc, d.currentResIdList))
        const_cast<QGles2TextureRenderTarget *>(this)->create();

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
    destroy();
}

void QGles2ShaderResourceBindings::destroy()
{
    QRHI_RES_RHI(QRhiGles2);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QGles2ShaderResourceBindings::create()
{
    QRHI_RES_RHI(QRhiGles2);
    if (!rhiD->sanityCheckShaderResourceBindings(this))
        return false;

    hasDynamicOffset = false;
    for (int i = 0, ie = m_bindings.size(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = QRhiImplementation::shaderResourceBindingData(m_bindings.at(i));
        if (b->type == QRhiShaderResourceBinding::UniformBuffer) {
            if (b->u.ubuf.hasDynamicOffset) {
                hasDynamicOffset = true;
                break;
            }
        }
    }

    rhiD->updateLayoutDesc(this);

    generation += 1;
    rhiD->registerResource(this, false);
    return true;
}

void QGles2ShaderResourceBindings::updateResources(UpdateFlags flags)
{
    Q_UNUSED(flags);
    generation += 1;
}

QGles2GraphicsPipeline::QGles2GraphicsPipeline(QRhiImplementation *rhi)
    : QRhiGraphicsPipeline(rhi)
{
}

QGles2GraphicsPipeline::~QGles2GraphicsPipeline()
{
    destroy();
}

void QGles2GraphicsPipeline::destroy()
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
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

static inline bool isGraphicsStage(const QRhiShaderStage &shaderStage)
{
    const QRhiShaderStage::Type t = shaderStage.type();
    return t == QRhiShaderStage::Vertex
            || t == QRhiShaderStage::TessellationControl
            || t == QRhiShaderStage::TessellationEvaluation
            || t == QRhiShaderStage::Geometry
            || t == QRhiShaderStage::Fragment;
}

bool QGles2GraphicsPipeline::create()
{
    QRHI_RES_RHI(QRhiGles2);

    if (program)
        destroy();

    if (!rhiD->ensureContext())
        return false;

    rhiD->pipelineCreationStart();
    if (!rhiD->sanityCheckGraphicsPipeline(this))
        return false;

    drawMode = toGlTopology(m_topology);

    program = rhiD->f->glCreateProgram();

    enum {
        VtxIdx = 0,
        TCIdx,
        TEIdx,
        GeomIdx,
        FragIdx,
        LastIdx
    };
    const auto descIdxForStage = [](const QRhiShaderStage &shaderStage) {
        switch (shaderStage.type()) {
        case QRhiShaderStage::Vertex:
            return VtxIdx;
        case QRhiShaderStage::TessellationControl:
            return TCIdx;
        case QRhiShaderStage::TessellationEvaluation:
            return TEIdx;
        case QRhiShaderStage::Geometry:
            return GeomIdx;
        case QRhiShaderStage::Fragment:
            return FragIdx;
        default:
            break;
        }
        Q_UNREACHABLE_RETURN(VtxIdx);
    };
    QShaderDescription desc[LastIdx];
    QShader::SeparateToCombinedImageSamplerMappingList samplerMappingList[LastIdx];
    bool vertexFragmentOnly = true;
    for (const QRhiShaderStage &shaderStage : std::as_const(m_shaderStages)) {
        if (isGraphicsStage(shaderStage)) {
            const int idx = descIdxForStage(shaderStage);
            if (idx != VtxIdx && idx != FragIdx)
                vertexFragmentOnly = false;
            QShader shader = shaderStage.shader();
            QShaderVersion shaderVersion;
            desc[idx] = shader.description();
            if (!rhiD->shaderSource(shaderStage, &shaderVersion).isEmpty()) {
                samplerMappingList[idx] = shader.separateToCombinedImageSamplerMappingList(
                            { QShader::GlslShader, shaderVersion, shaderStage.shaderVariant() });
            }
        }
    }

    QByteArray cacheKey;
    QRhiGles2::ProgramCacheResult cacheResult = rhiD->tryLoadFromDiskOrPipelineCache(m_shaderStages.constData(),
                                                                                     m_shaderStages.size(),
                                                                                     program,
                                                                                     desc[VtxIdx].inputVariables(),
                                                                                     &cacheKey);
    if (cacheResult == QRhiGles2::ProgramCacheError)
        return false;

    if (cacheResult == QRhiGles2::ProgramCacheMiss) {
        for (const QRhiShaderStage &shaderStage : std::as_const(m_shaderStages)) {
            if (isGraphicsStage(shaderStage)) {
                if (!rhiD->compileShader(program, shaderStage, nullptr))
                    return false;
            }
        }

        // important when GLSL <= 150 is used that does not have location qualifiers
        for (const QShaderDescription::InOutVariable &inVar : desc[VtxIdx].inputVariables())
            rhiD->f->glBindAttribLocation(program, GLuint(inVar.location), inVar.name);

        if (vertexFragmentOnly)
            rhiD->sanityCheckVertexFragmentInterface(desc[VtxIdx], desc[FragIdx]);

        if (!rhiD->linkProgram(program))
            return false;

        if (rhiD->rhiFlags.testFlag(QRhi::EnablePipelineCacheDataSave)) {
            // force replacing existing cache entry (if there is one, then
            // something is wrong with it, as there was no hit)
            rhiD->trySaveToPipelineCache(program, cacheKey, true);
        } else {
            // legacy QOpenGLShaderProgram style behavior: the "pipeline cache"
            // was not enabled, so instead store to the Qt 5 disk cache
            rhiD->trySaveToDiskCache(program, cacheKey);
        }
    } else {
        Q_ASSERT(cacheResult == QRhiGles2::ProgramCacheHit);
        if (rhiD->rhiFlags.testFlag(QRhi::EnablePipelineCacheDataSave)) {
            // just so that it ends up in the pipeline cache also when the hit was
            // from the disk cache
            rhiD->trySaveToPipelineCache(program, cacheKey);
        }
    }

    // Use the same work area for the vertex & fragment stages, thus ensuring
    // that we will not do superfluous glUniform calls for uniforms that are
    // present in both shaders.
    QRhiGles2::ActiveUniformLocationTracker activeUniformLocations;

    for (const QRhiShaderStage &shaderStage : std::as_const(m_shaderStages)) {
        if (isGraphicsStage(shaderStage)) {
            const int idx = descIdxForStage(shaderStage);
            for (const QShaderDescription::UniformBlock &ub : desc[idx].uniformBlocks())
                rhiD->gatherUniforms(program, ub, &activeUniformLocations, &uniforms);
            for (const QShaderDescription::InOutVariable &v : desc[idx].combinedImageSamplers())
                rhiD->gatherSamplers(program, v, &samplers);
            for (const QShader::SeparateToCombinedImageSamplerMapping &mapping : samplerMappingList[idx])
                rhiD->gatherGeneratedSamplers(program, mapping, &samplers);
        }
    }

    std::sort(uniforms.begin(), uniforms.end(),
              [](const QGles2UniformDescription &a, const QGles2UniformDescription &b)
    {
        return a.offset < b.offset;
    });

    memset(uniformState, 0, sizeof(uniformState));

    currentSrb = nullptr;
    currentSrbGeneration = 0;

    if (rhiD->glObjectLabel)
        rhiD->glObjectLabel(GL_PROGRAM, program, -1, m_objectName.constData());

    rhiD->pipelineCreationEnd();
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
    destroy();
}

void QGles2ComputePipeline::destroy()
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
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QGles2ComputePipeline::create()
{
    QRHI_RES_RHI(QRhiGles2);

    if (program)
        destroy();

    if (!rhiD->ensureContext())
        return false;

    rhiD->pipelineCreationStart();

    const QShaderDescription csDesc = m_shaderStage.shader().description();
    QShader::SeparateToCombinedImageSamplerMappingList csSamplerMappingList;
    QShaderVersion shaderVersion;
    if (!rhiD->shaderSource(m_shaderStage, &shaderVersion).isEmpty()) {
        csSamplerMappingList = m_shaderStage.shader().separateToCombinedImageSamplerMappingList(
                    { QShader::GlslShader, shaderVersion, m_shaderStage.shaderVariant() });
    }

    program = rhiD->f->glCreateProgram();

    QByteArray cacheKey;
    QRhiGles2::ProgramCacheResult cacheResult = rhiD->tryLoadFromDiskOrPipelineCache(&m_shaderStage, 1, program, {}, &cacheKey);
    if (cacheResult == QRhiGles2::ProgramCacheError)
        return false;

    if (cacheResult == QRhiGles2::ProgramCacheMiss) {
        if (!rhiD->compileShader(program, m_shaderStage, nullptr))
            return false;

        if (!rhiD->linkProgram(program))
            return false;

        if (rhiD->rhiFlags.testFlag(QRhi::EnablePipelineCacheDataSave)) {
            // force replacing existing cache entry (if there is one, then
            // something is wrong with it, as there was no hit)
            rhiD->trySaveToPipelineCache(program, cacheKey, true);
        } else {
            // legacy QOpenGLShaderProgram style behavior: the "pipeline cache"
            // was not enabled, so instead store to the Qt 5 disk cache
            rhiD->trySaveToDiskCache(program, cacheKey);
        }
    } else {
        Q_ASSERT(cacheResult == QRhiGles2::ProgramCacheHit);
        if (rhiD->rhiFlags.testFlag(QRhi::EnablePipelineCacheDataSave)) {
            // just so that it ends up in the pipeline cache also when the hit was
            // from the disk cache
            rhiD->trySaveToPipelineCache(program, cacheKey);
        }
    }

    QRhiGles2::ActiveUniformLocationTracker activeUniformLocations;
    for (const QShaderDescription::UniformBlock &ub : csDesc.uniformBlocks())
        rhiD->gatherUniforms(program, ub, &activeUniformLocations, &uniforms);
    for (const QShaderDescription::InOutVariable &v : csDesc.combinedImageSamplers())
        rhiD->gatherSamplers(program, v, &samplers);
    for (const QShader::SeparateToCombinedImageSamplerMapping &mapping : csSamplerMappingList)
        rhiD->gatherGeneratedSamplers(program, mapping, &samplers);

    // storage images and buffers need no special steps here

    memset(uniformState, 0, sizeof(uniformState));

    currentSrb = nullptr;
    currentSrbGeneration = 0;

    rhiD->pipelineCreationEnd();
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
    destroy();
}

void QGles2CommandBuffer::destroy()
{
    // nothing to do here
}

QGles2SwapChain::QGles2SwapChain(QRhiImplementation *rhi)
    : QRhiSwapChain(rhi),
      rt(rhi, this),
      rtLeft(rhi, this),
      rtRight(rhi, this),
      cb(rhi)
{
}

QGles2SwapChain::~QGles2SwapChain()
{
    destroy();
}

void QGles2SwapChain::destroy()
{
    QRHI_RES_RHI(QRhiGles2);
    if (rhiD)
        rhiD->unregisterResource(this);
}

QRhiCommandBuffer *QGles2SwapChain::currentFrameCommandBuffer()
{
    return &cb;
}

QRhiRenderTarget *QGles2SwapChain::currentFrameRenderTarget()
{
    return &rt;
}

QRhiRenderTarget *QGles2SwapChain::currentFrameRenderTarget(StereoTargetBuffer targetBuffer)
{
    if (targetBuffer == LeftBuffer)
        return rtLeft.d.isValid() ? &rtLeft : &rt;
    else if (targetBuffer == RightBuffer)
        return rtRight.d.isValid() ? &rtRight : &rt;
    else
        Q_UNREACHABLE_RETURN(nullptr);
}

QSize QGles2SwapChain::surfacePixelSize()
{
    Q_ASSERT(m_window);
    if (QPlatformWindow *platformWindow = m_window->handle())
        // Prefer using QPlatformWindow geometry and DPR in order to avoid
        // errors due to rounded QWindow geometry.
        return platformWindow->geometry().size() * platformWindow->devicePixelRatio();
    else
        return m_window->size() * m_window->devicePixelRatio();
}

bool QGles2SwapChain::isFormatSupported(Format f)
{
    return f == SDR;
}

QRhiRenderPassDescriptor *QGles2SwapChain::newCompatibleRenderPassDescriptor()
{
    QGles2RenderPassDescriptor *rpD = new QGles2RenderPassDescriptor(m_rhi);
    QRHI_RES_RHI(QRhiGles2);
    rhiD->registerResource(rpD, false);
    return rpD;
}

void QGles2SwapChain::initSwapChainRenderTarget(QGles2SwapChainRenderTarget *rt)
{
    rt->setRenderPassDescriptor(m_renderPassDesc); // for the public getter in QRhiRenderTarget
    rt->d.rp = QRHI_RES(QGles2RenderPassDescriptor, m_renderPassDesc);
    rt->d.pixelSize = pixelSize;
    rt->d.dpr = float(m_window->devicePixelRatio());
    rt->d.sampleCount = qBound(1, m_sampleCount, 64);
    rt->d.colorAttCount = 1;
    rt->d.dsAttCount = m_depthStencil ? 1 : 0;
    rt->d.srgbUpdateAndBlend = m_flags.testFlag(QRhiSwapChain::sRGB);
}

bool QGles2SwapChain::createOrResize()
{
    // can be called multiple times due to window resizes
    if (surface && surface != m_window)
        destroy();

    surface = m_window;
    m_currentPixelSize = surfacePixelSize();
    pixelSize = m_currentPixelSize;

    if (m_depthStencil && m_depthStencil->flags().testFlag(QRhiRenderBuffer::UsedWithSwapChainOnly)
            && m_depthStencil->pixelSize() != pixelSize)
    {
        m_depthStencil->setPixelSize(pixelSize);
        m_depthStencil->create();
    }

    initSwapChainRenderTarget(&rt);

    if (m_window->format().stereo()) {
        initSwapChainRenderTarget(&rtLeft);
        rtLeft.d.stereoTarget = QRhiSwapChain::LeftBuffer;
        initSwapChainRenderTarget(&rtRight);
        rtRight.d.stereoTarget = QRhiSwapChain::RightBuffer;
    }

    QRHI_RES_RHI(QRhiGles2);
    if (rhiD->rhiFlags.testFlag(QRhi::EnableTimestamps) && rhiD->caps.timestamps)
        timestamps.prepare(rhiD);

    // The only reason to register this fairly fake gl swapchain
    // object with no native resources underneath is to be able to
    // implement a safe destroy().
    rhiD->registerResource(this, false);

    return true;
}

void QGles2SwapChainTimestamps::prepare(QRhiGles2 *rhiD)
{
    if (!query[0])
        rhiD->f->glGenQueries(TIMESTAMP_PAIRS * 2, query);
}

void QGles2SwapChainTimestamps::destroy(QRhiGles2 *rhiD)
{
    rhiD->f->glDeleteQueries(TIMESTAMP_PAIRS * 2, query);
    memset(active, 0, sizeof(active));
    memset(query, 0, sizeof(query));
}

bool QGles2SwapChainTimestamps::tryQueryTimestamps(int pairIndex, QRhiGles2 *rhiD, double *elapsedSec)
{
    if (!active[pairIndex])
        return false;

    GLuint tsStart = query[pairIndex * 2];
    GLuint tsEnd = query[pairIndex * 2 + 1];

    GLuint ready = GL_FALSE;
    rhiD->f->glGetQueryObjectuiv(tsEnd, GL_QUERY_RESULT_AVAILABLE, &ready);

    if (!ready)
        return false;

    bool result = false;
    quint64 timestamps[2];
    rhiD->glGetQueryObjectui64v(tsStart, GL_QUERY_RESULT, &timestamps[0]);
    rhiD->glGetQueryObjectui64v(tsEnd, GL_QUERY_RESULT, &timestamps[1]);

    if (timestamps[1] >= timestamps[0]) {
        const quint64 nanoseconds = timestamps[1] - timestamps[0];
        *elapsedSec = nanoseconds / 1000000000.0;
        result = true;
    }

    active[pairIndex] = false;
    return result;
}

QT_END_NAMESPACE
