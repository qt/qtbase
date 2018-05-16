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

#include "qopenglfunctions.h"
#include "qopenglextrafunctions.h"
#include "qopenglextensions_p.h"
#include "qdebug.h"
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/private/qopengl_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#ifdef Q_OS_INTEGRITY
#include <EGL/egl.h>
#endif

#ifndef GL_FRAMEBUFFER_SRGB_CAPABLE_EXT
#define GL_FRAMEBUFFER_SRGB_CAPABLE_EXT   0x8DBA
#endif

QT_BEGIN_NAMESPACE

#define QT_OPENGL_COUNT_FUNCTIONS(ret, name, args) +1
#define QT_OPENGL_FUNCTION_NAMES(ret, name, args) \
    "gl"#name"\0"
#define QT_OPENGL_FLAGS(ret, name, args) \
    0,
#define QT_OPENGL_IMPLEMENT(CLASS, FUNCTIONS) \
void CLASS::init(QOpenGLContext *context) \
{ \
    const char *names = FUNCTIONS(QT_OPENGL_FUNCTION_NAMES); \
    const char *name = names; \
    for (int i = 0; i < FUNCTIONS(QT_OPENGL_COUNT_FUNCTIONS); ++i) { \
        functions[i] = QT_PREPEND_NAMESPACE(getProcAddress(context, name)); \
        name += strlen(name) + 1; \
    } \
}

/*!
    \class QOpenGLFunctions
    \brief The QOpenGLFunctions class provides cross-platform access to the OpenGL ES 2.0 API.
    \since 5.0
    \ingroup painting-3D
    \inmodule QtGui

    OpenGL ES 2.0 defines a subset of the OpenGL specification that is
    common across many desktop and embedded OpenGL implementations.
    However, it can be difficult to use the functions from that subset
    because they need to be resolved manually on desktop systems.

    QOpenGLFunctions provides a guaranteed API that is available on all
    OpenGL systems and takes care of function resolution on systems
    that need it.  The recommended way to use QOpenGLFunctions is by
    direct inheritance:

    \code
    class MyGLWindow : public QWindow, protected QOpenGLFunctions
    {
        Q_OBJECT
    public:
        MyGLWindow(QScreen *screen = 0);

    protected:
        void initializeGL();
        void paintGL();

        QOpenGLContext *m_context;
    };

    MyGLWindow(QScreen *screen)
      : QWindow(screen), QOpenGLWidget(parent)
    {
        setSurfaceType(OpenGLSurface);
        create();

        // Create an OpenGL context
        m_context = new QOpenGLContext;
        m_context->create();

        // Setup scene and render it
        initializeGL();
        paintGL();
    }

    void MyGLWindow::initializeGL()
    {
        m_context->makeCurrent(this);
        initializeOpenGLFunctions();
    }
    \endcode

    The \c{paintGL()} function can then use any of the OpenGL ES 2.0
    functions without explicit resolution, such as glActiveTexture()
    in the following example:

    \code
    void MyGLWindow::paintGL()
    {
        m_context->makeCurrent(this);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureId);
        ...
        m_context->swapBuffers(this);
        m_context->doneCurrent();
    }
    \endcode

    QOpenGLFunctions can also be used directly for ad-hoc invocation
    of OpenGL ES 2.0 functions on all platforms:

    \code
    QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
    glFuncs.glActiveTexture(GL_TEXTURE1);
    \endcode

    An alternative approach is to query the context's associated
    QOpenGLFunctions instance. This is somewhat faster than the previous
    approach due to avoiding the creation of a new instance, but the difference
    is fairly small since the internal data structures are shared, and function
    resolving happens only once for a given context, regardless of the number of
    QOpenGLFunctions instances initialized for it.

    \code
    QOpenGLFunctions *glFuncs = QOpenGLContext::currentContext()->functions();
    glFuncs->glActiveTexture(GL_TEXTURE1);
    \endcode

    QOpenGLFunctions provides wrappers for all OpenGL ES 2.0
    functions, including the common subset of OpenGL 1.x and ES
    2.0. While such functions, for example glClear() or
    glDrawArrays(), can be called also directly, as long as the
    application links to the platform-specific OpenGL library, calling
    them via QOpenGLFunctions enables the possibility of dynamically
    loading the OpenGL implementation.

    The hasOpenGLFeature() and openGLFeatures() functions can be used
    to determine if the OpenGL implementation has a major OpenGL ES 2.0
    feature.  For example, the following checks if non power of two
    textures are available:

    \code
    QOpenGLFunctions funcs(QOpenGLContext::currentContext());
    bool npot = funcs.hasOpenGLFeature(QOpenGLFunctions::NPOTTextures);
    \endcode

    \sa QOpenGLContext, QSurfaceFormat
*/

/*!
    \enum QOpenGLFunctions::OpenGLFeature
    This enum defines OpenGL and OpenGL ES features whose presence
    may depend on the implementation.

    \value Multitexture glActiveTexture() function is available.
    \value Shaders Shader functions are available.
    \value Buffers Vertex and index buffer functions are available.
    \value Framebuffers Framebuffer object functions are available.
    \value BlendColor glBlendColor() is available.
    \value BlendEquation glBlendEquation() is available.
    \value BlendEquationSeparate glBlendEquationSeparate() is available.
    \value BlendFuncSeparate glBlendFuncSeparate() is available.
    \value BlendSubtract Blend subtract mode is available.
    \value CompressedTextures Compressed texture functions are available.
    \value Multisample glSampleCoverage() function is available.
    \value StencilSeparate Separate stencil functions are available.
    \value NPOTTextures Non power of two textures are available.
    \value NPOTTextureRepeat Non power of two textures can use GL_REPEAT as wrap parameter.
    \value FixedFunctionPipeline The fixed function pipeline is available.
    \value TextureRGFormats The GL_RED and GL_RG texture formats are available.
    \value MultipleRenderTargets Multiple color attachments to framebuffer objects are available.
*/

// Hidden private fields for additional extension data.
struct QOpenGLFunctionsPrivateEx : public QOpenGLExtensionsPrivate, public QOpenGLSharedResource
{
    QOpenGLFunctionsPrivateEx(QOpenGLContext *context)
        : QOpenGLExtensionsPrivate(context)
        , QOpenGLSharedResource(context->shareGroup())
        , m_features(-1)
        , m_extensions(-1)
    {}

    void invalidateResource() override
    {
        m_features = -1;
        m_extensions = -1;
    }

    void freeResource(QOpenGLContext *) override
    {
        // no gl resources to free
    }

    int m_features;
    int m_extensions;
};

Q_GLOBAL_STATIC(QOpenGLMultiGroupSharedResource, qt_gl_functions_resource)

static QOpenGLFunctionsPrivateEx *qt_gl_functions(QOpenGLContext *context = 0)
{
    if (!context)
        context = QOpenGLContext::currentContext();
    Q_ASSERT(context);
    QOpenGLFunctionsPrivateEx *funcs =
        qt_gl_functions_resource()->value<QOpenGLFunctionsPrivateEx>(context);
    return funcs;
}

/*!
    Constructs a default function resolver. The resolver cannot
    be used until initializeOpenGLFunctions() is called to specify
    the context.

    \sa initializeOpenGLFunctions()
*/
QOpenGLFunctions::QOpenGLFunctions()
    : d_ptr(0)
{
}

/*!
    Constructs a function resolver for \a context.  If \a context
    is null, then the resolver will be created for the current QOpenGLContext.

    The context or another context in the group must be current.

    An object constructed in this way can only be used with \a context
    and other contexts that share with it.  Use initializeOpenGLFunctions()
    to change the object's context association.

    \sa initializeOpenGLFunctions()
*/
QOpenGLFunctions::QOpenGLFunctions(QOpenGLContext *context)
    : d_ptr(0)
{
    if (context && QOpenGLContextGroup::currentContextGroup() == context->shareGroup())
        d_ptr = qt_gl_functions(context);
    else
        qWarning("QOpenGLFunctions created with non-current context");
}

QOpenGLExtensions::QOpenGLExtensions()
{
}

QOpenGLExtensions::QOpenGLExtensions(QOpenGLContext *context)
    : QOpenGLExtraFunctions(context)
{
}

/*!
    \fn QOpenGLFunctions::~QOpenGLFunctions()

    Destroys this function resolver.
*/

static int qt_gl_resolve_features()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (ctx->isOpenGLES()) {
        // OpenGL ES
        int features = QOpenGLFunctions::Multitexture |
            QOpenGLFunctions::Shaders |
            QOpenGLFunctions::Buffers |
            QOpenGLFunctions::Framebuffers |
            QOpenGLFunctions::BlendColor |
            QOpenGLFunctions::BlendEquation |
            QOpenGLFunctions::BlendEquationSeparate |
            QOpenGLFunctions::BlendFuncSeparate |
            QOpenGLFunctions::BlendSubtract |
            QOpenGLFunctions::CompressedTextures |
            QOpenGLFunctions::Multisample |
            QOpenGLFunctions::StencilSeparate;
        QOpenGLExtensionMatcher extensions;
        if (extensions.match("GL_IMG_texture_npot"))
            features |= QOpenGLFunctions::NPOTTextures;
        if (extensions.match("GL_OES_texture_npot"))
            features |= QOpenGLFunctions::NPOTTextures |
                QOpenGLFunctions::NPOTTextureRepeat;
        if (ctx->format().majorVersion() >= 3 || extensions.match("GL_EXT_texture_rg")) {
            // Mesa's GLES implementation (as of 10.6.0) is unable to handle this, even though it provides 3.0.
            const char *renderer = reinterpret_cast<const char *>(ctx->functions()->glGetString(GL_RENDERER));
            if (!(renderer && strstr(renderer, "Mesa")))
                features |= QOpenGLFunctions::TextureRGFormats;
        }
        if (ctx->format().majorVersion() >= 3)
            features |= QOpenGLFunctions::MultipleRenderTargets;
        return features;
    } else {
        // OpenGL
        int features = QOpenGLFunctions::TextureRGFormats;
        QSurfaceFormat format = QOpenGLContext::currentContext()->format();
        QOpenGLExtensionMatcher extensions;

        if (format.majorVersion() >= 3)
            features |= QOpenGLFunctions::Framebuffers | QOpenGLFunctions::MultipleRenderTargets;
        else if (extensions.match("GL_EXT_framebuffer_object") || extensions.match("GL_ARB_framebuffer_object"))
            features |= QOpenGLFunctions::Framebuffers | QOpenGLFunctions::MultipleRenderTargets;

        if (format.majorVersion() >= 2) {
            features |= QOpenGLFunctions::BlendColor |
                QOpenGLFunctions::BlendEquation |
                QOpenGLFunctions::BlendSubtract |
                QOpenGLFunctions::Multitexture |
                QOpenGLFunctions::CompressedTextures |
                QOpenGLFunctions::Multisample |
                QOpenGLFunctions::BlendFuncSeparate |
                QOpenGLFunctions::Buffers |
                QOpenGLFunctions::Shaders |
                QOpenGLFunctions::StencilSeparate |
                QOpenGLFunctions::BlendEquationSeparate |
                QOpenGLFunctions::NPOTTextures |
                QOpenGLFunctions::NPOTTextureRepeat;
        } else {
            // Recognize features by extension name.
            if (extensions.match("GL_ARB_multitexture"))
                features |= QOpenGLFunctions::Multitexture;
            if (extensions.match("GL_ARB_shader_objects"))
                features |= QOpenGLFunctions::Shaders;
            if (extensions.match("GL_EXT_blend_color"))
                features |= QOpenGLFunctions::BlendColor;
            if (extensions.match("GL_EXT_blend_equation_separate"))
                features |= QOpenGLFunctions::BlendEquationSeparate;
            if (extensions.match("GL_EXT_blend_subtract"))
                features |= QOpenGLFunctions::BlendSubtract;
            if (extensions.match("GL_EXT_blend_func_separate"))
                features |= QOpenGLFunctions::BlendFuncSeparate;
            if (extensions.match("GL_ARB_texture_compression"))
                features |= QOpenGLFunctions::CompressedTextures;
            if (extensions.match("GL_ARB_multisample"))
                features |= QOpenGLFunctions::Multisample;
            if (extensions.match("GL_ARB_texture_non_power_of_two"))
                features |= QOpenGLFunctions::NPOTTextures |
                    QOpenGLFunctions::NPOTTextureRepeat;
        }

        const QPair<int, int> version = format.version();
        if (version < qMakePair(3, 0)
            || (version == qMakePair(3, 0) && format.testOption(QSurfaceFormat::DeprecatedFunctions))
            || (version == qMakePair(3, 1) && extensions.match("GL_ARB_compatibility"))
            || (version >= qMakePair(3, 2) && format.profile() == QSurfaceFormat::CompatibilityProfile)) {
            features |= QOpenGLFunctions::FixedFunctionPipeline;
        }
        return features;
    }
}

static int qt_gl_resolve_extensions()
{
    int extensions = 0;
    QOpenGLExtensionMatcher extensionMatcher;
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QSurfaceFormat format = ctx->format();

    if (extensionMatcher.match("GL_EXT_bgra"))
        extensions |= QOpenGLExtensions::BGRATextureFormat;
    if (extensionMatcher.match("GL_ARB_texture_rectangle"))
        extensions |= QOpenGLExtensions::TextureRectangle;
    if (extensionMatcher.match("GL_ARB_texture_compression"))
        extensions |= QOpenGLExtensions::TextureCompression;
    if (extensionMatcher.match("GL_EXT_texture_compression_s3tc"))
        extensions |= QOpenGLExtensions::DDSTextureCompression;
    if (extensionMatcher.match("GL_OES_compressed_ETC1_RGB8_texture"))
        extensions |= QOpenGLExtensions::ETC1TextureCompression;
    if (extensionMatcher.match("GL_IMG_texture_compression_pvrtc"))
        extensions |= QOpenGLExtensions::PVRTCTextureCompression;
    if (extensionMatcher.match("GL_ARB_texture_mirrored_repeat"))
        extensions |= QOpenGLExtensions::MirroredRepeat;
    if (extensionMatcher.match("GL_EXT_stencil_two_side"))
        extensions |= QOpenGLExtensions::StencilTwoSide;
    if (extensionMatcher.match("GL_EXT_stencil_wrap"))
        extensions |= QOpenGLExtensions::StencilWrap;
    if (extensionMatcher.match("GL_NV_float_buffer"))
        extensions |= QOpenGLExtensions::NVFloatBuffer;
    if (extensionMatcher.match("GL_ARB_pixel_buffer_object"))
        extensions |= QOpenGLExtensions::PixelBufferObject;

    if (ctx->isOpenGLES()) {
        if (format.majorVersion() >= 2)
            extensions |= QOpenGLExtensions::GenerateMipmap;

        if (format.majorVersion() >= 3) {
            extensions |= QOpenGLExtensions::PackedDepthStencil
                | QOpenGLExtensions::Depth24
                | QOpenGLExtensions::ElementIndexUint
                | QOpenGLExtensions::MapBufferRange
                | QOpenGLExtensions::FramebufferBlit
                | QOpenGLExtensions::FramebufferMultisample
                | QOpenGLExtensions::Sized8Formats;
        } else {
            // Recognize features by extension name.
            if (extensionMatcher.match("GL_OES_packed_depth_stencil"))
                extensions |= QOpenGLExtensions::PackedDepthStencil;
            if (extensionMatcher.match("GL_OES_depth24"))
                extensions |= QOpenGLExtensions::Depth24;
            if (extensionMatcher.match("GL_ANGLE_framebuffer_blit"))
                extensions |= QOpenGLExtensions::FramebufferBlit;
            if (extensionMatcher.match("GL_ANGLE_framebuffer_multisample"))
                extensions |= QOpenGLExtensions::FramebufferMultisample;
            if (extensionMatcher.match("GL_NV_framebuffer_blit"))
                extensions |= QOpenGLExtensions::FramebufferBlit;
            if (extensionMatcher.match("GL_NV_framebuffer_multisample"))
                extensions |= QOpenGLExtensions::FramebufferMultisample;
            if (extensionMatcher.match("GL_OES_rgb8_rgba8"))
                extensions |= QOpenGLExtensions::Sized8Formats;
        }

        if (extensionMatcher.match("GL_OES_mapbuffer"))
            extensions |= QOpenGLExtensions::MapBuffer;
        if (extensionMatcher.match("GL_OES_element_index_uint"))
            extensions |= QOpenGLExtensions::ElementIndexUint;
        // We don't match GL_APPLE_texture_format_BGRA8888 here because it has different semantics.
        if (extensionMatcher.match("GL_IMG_texture_format_BGRA8888") || extensionMatcher.match("GL_EXT_texture_format_BGRA8888"))
            extensions |= QOpenGLExtensions::BGRATextureFormat;
        if (extensionMatcher.match("GL_EXT_discard_framebuffer"))
            extensions |= QOpenGLExtensions::DiscardFramebuffer;
        if (extensionMatcher.match("GL_EXT_texture_norm16"))
            extensions |= QOpenGLExtensions::Sized16Formats;
    } else {
        extensions |= QOpenGLExtensions::ElementIndexUint
            | QOpenGLExtensions::MapBuffer
            | QOpenGLExtensions::Sized16Formats;

        if (format.version() >= qMakePair(1, 2))
            extensions |= QOpenGLExtensions::BGRATextureFormat;

        if (format.version() >= qMakePair(1, 4) || extensionMatcher.match("GL_SGIS_generate_mipmap"))
            extensions |= QOpenGLExtensions::GenerateMipmap;

        if (format.majorVersion() >= 3 || extensionMatcher.match("GL_ARB_framebuffer_object")) {
            extensions |= QOpenGLExtensions::FramebufferMultisample
                | QOpenGLExtensions::FramebufferBlit
                | QOpenGLExtensions::PackedDepthStencil
                | QOpenGLExtensions::Sized8Formats;
        } else {
            // Recognize features by extension name.
            if (extensionMatcher.match("GL_EXT_framebuffer_multisample"))
                extensions |= QOpenGLExtensions::FramebufferMultisample;
            if (extensionMatcher.match("GL_EXT_framebuffer_blit"))
                extensions |= QOpenGLExtensions::FramebufferBlit;
            if (extensionMatcher.match("GL_EXT_packed_depth_stencil"))
                extensions |= QOpenGLExtensions::PackedDepthStencil;
        }

        if (format.version() >= qMakePair(3, 2) || extensionMatcher.match("GL_ARB_geometry_shader4"))
            extensions |= QOpenGLExtensions::GeometryShaders;

        if (extensionMatcher.match("GL_ARB_map_buffer_range"))
            extensions |= QOpenGLExtensions::MapBufferRange;

        if (extensionMatcher.match("GL_EXT_framebuffer_sRGB")) {
            GLboolean srgbCapableFramebuffers = false;
            ctx->functions()->glGetBooleanv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &srgbCapableFramebuffers);
            if (srgbCapableFramebuffers)
                extensions |= QOpenGLExtensions::SRGBFrameBuffer;
        }
    }

    return extensions;
}

/*!
    Returns the set of features that are present on this system's
    OpenGL implementation.

    It is assumed that the QOpenGLContext associated with this function
    resolver is current.

    \sa hasOpenGLFeature()
*/
QOpenGLFunctions::OpenGLFeatures QOpenGLFunctions::openGLFeatures() const
{
    QOpenGLFunctionsPrivateEx *d = static_cast<QOpenGLFunctionsPrivateEx *>(d_ptr);
    if (!d)
        return 0;
    if (d->m_features == -1)
        d->m_features = qt_gl_resolve_features();
    return QOpenGLFunctions::OpenGLFeatures(d->m_features);
}

/*!
    Returns \c true if \a feature is present on this system's OpenGL
    implementation; false otherwise.

    It is assumed that the QOpenGLContext associated with this function
    resolver is current.

    \sa openGLFeatures()
*/
bool QOpenGLFunctions::hasOpenGLFeature(QOpenGLFunctions::OpenGLFeature feature) const
{
    QOpenGLFunctionsPrivateEx *d = static_cast<QOpenGLFunctionsPrivateEx *>(d_ptr);
    if (!d)
        return false;
    if (d->m_features == -1)
        d->m_features = qt_gl_resolve_features();
    return (d->m_features & int(feature)) != 0;
}

/*!
    Returns the set of extensions that are present on this system's
    OpenGL implementation.

    It is assumed that the QOpenGLContext associated with this extension
    resolver is current.

    \sa hasOpenGLExtensions()
*/
QOpenGLExtensions::OpenGLExtensions QOpenGLExtensions::openGLExtensions()
{
    QOpenGLFunctionsPrivateEx *d = static_cast<QOpenGLFunctionsPrivateEx *>(d_ptr);
    if (!d)
        return 0;
    if (d->m_extensions == -1)
        d->m_extensions = qt_gl_resolve_extensions();
    return QOpenGLExtensions::OpenGLExtensions(d->m_extensions);
}

/*!
    Returns \c true if \a extension is present on this system's OpenGL
    implementation; false otherwise.

    It is assumed that the QOpenGLContext associated with this extension
    resolver is current.

    \sa openGLFeatures()
*/
bool QOpenGLExtensions::hasOpenGLExtension(QOpenGLExtensions::OpenGLExtension extension) const
{
    QOpenGLFunctionsPrivateEx *d = static_cast<QOpenGLFunctionsPrivateEx *>(d_ptr);
    if (!d)
        return false;
    if (d->m_extensions == -1)
        d->m_extensions = qt_gl_resolve_extensions();
    return (d->m_extensions & int(extension)) != 0;
}

/*!
    \fn void QOpenGLFunctions::initializeGLFunctions()
    \obsolete

    Use initializeOpenGLFunctions() instead.
*/

/*!
    Initializes OpenGL function resolution for the current context.

    After calling this function, the QOpenGLFunctions object can only be
    used with the current context and other contexts that share with it.
    Call initializeOpenGLFunctions() again to change the object's context
    association.
*/
void QOpenGLFunctions::initializeOpenGLFunctions()
{
    d_ptr = qt_gl_functions();
}

/*!
    \fn void QOpenGLFunctions::glBindTexture(GLenum target, GLuint texture)

    Convenience function that calls glBindTexture(\a target, \a texture).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindTexture.xhtml}{glBindTexture()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glBlendFunc(GLenum sfactor, GLenum dfactor)

    Convenience function that calls glBlendFunc(\a sfactor, \a dfactor).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlendFunc.xhtml}{glBlendFunc()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glClear(GLbitfield mask)

    Convenience function that calls glClear(\a mask).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glClear.xhtml}{glClear()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)

    Convenience function that calls glClearColor(\a red, \a green, \a blue, \a alpha).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glClearColor.xhtml}{glClearColor()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glClearStencil(GLint s)

    Convenience function that calls glClearStencil(\a s).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glClearStencil.xhtml}{glClearStencil()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)

    Convenience function that calls glColorMask(\a red, \a green, \a blue, \a alpha).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glColorMask.xhtml}{glColorMask()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)

    Convenience function that calls glCopyTexImage2D(\a target, \a level, \a internalformat, \a x, \a y, \a width, \a height, \a border).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCopyTexImage2D.xhtml}{glCopyTexImage2D()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)

    Convenience function that calls glCopyTexSubImage2D(\a target, \a level, \a xoffset, \a yoffset, \a x, \a y, \a width, \a height).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCopyTexSubImage2D.xhtml}{glCopyTexSubImage2D()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glCullFace(GLenum mode)

    Convenience function that calls glCullFace(\a mode).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCullFace.xhtml}{glCullFace()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDeleteTextures(GLsizei n, const GLuint* textures)

    Convenience function that calls glDeleteTextures(\a n, \a textures).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteTextures.xhtml}{glDeleteTextures()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDepthFunc(GLenum func)

    Convenience function that calls glDepthFunc(\a func).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDepthFunc.xhtml}{glDepthFunc()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDepthMask(GLboolean flag)

    Convenience function that calls glDepthMask(\a flag).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDepthMask.xhtml}{glDepthMask()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDisable(GLenum cap)

    Convenience function that calls glDisable(\a cap).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDisable.xhtml}{glDisable()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDrawArrays(GLenum mode, GLint first, GLsizei count)

    Convenience function that calls glDrawArrays(\a mode, \a first, \a count).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawArrays.xhtml}{glDrawArrays()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)

    Convenience function that calls glDrawElements(\a mode, \a count, \a type, \a indices).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawElements.xhtml}{glDrawElements()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glEnable(GLenum cap)

    Convenience function that calls glEnable(\a cap).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glEnable.xhtml}{glEnable()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glFinish()

    Convenience function that calls glFinish().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glFinish.xhtml}{glFinish()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glFlush()

    Convenience function that calls glFlush().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glFlush.xhtml}{glFlush()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glFrontFace(GLenum mode)

    Convenience function that calls glFrontFace(\a mode).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glFrontFace.xhtml}{glFrontFace()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGenTextures(GLsizei n, GLuint* textures)

    Convenience function that calls glGenTextures(\a n, \a textures).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGenTextures.xhtml}{glGenTextures()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGetBooleanv(GLenum pname, GLboolean* params)

    Convenience function that calls glGetBooleanv(\a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGet.xhtml}{glGetBooleanv()}.

    \since 5.3
*/

/*!
    \fn GLenum QOpenGLFunctions::glGetError()

    Convenience function that calls glGetError().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetError.xhtml}{glGetError()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGetFloatv(GLenum pname, GLfloat* params)

    Convenience function that calls glGetFloatv(\a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGet.xhtml}{glGetFloatv()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGetIntegerv(GLenum pname, GLint* params)

    Convenience function that calls glGetIntegerv(\a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGet.xhtml}{glGetIntegerv()}.

    \since 5.3
*/

/*!
    \fn const GLubyte *QOpenGLFunctions::glGetString(GLenum name)

    Convenience function that calls glGetString(\a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetString.xhtml}{glGetString()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)

    Convenience function that calls glGetTexParameterfv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetTexParameter.xhtml}{glGetTexParameterfv()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetTexParameteriv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetTexParameter.xhtml}{glGetTexParameteriv()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glHint(GLenum target, GLenum mode)

    Convenience function that calls glHint(\a target, \a mode).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glHint.xhtml}{glHint()}.

    \since 5.3
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsEnabled(GLenum cap)

    Convenience function that calls glIsEnabled(\a cap).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsEnabled.xhtml}{glIsEnabled()}.

    \since 5.3
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsTexture(GLuint texture)

    Convenience function that calls glIsTexture(\a texture).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsTexture.xhtml}{glIsTexture()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glLineWidth(GLfloat width)

    Convenience function that calls glLineWidth(\a width).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glLineWidth.xhtml}{glLineWidth()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glPixelStorei(GLenum pname, GLint param)

    Convenience function that calls glPixelStorei(\a pname, \a param).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glPixelStorei.xhtml}{glPixelStorei()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glPolygonOffset(GLfloat factor, GLfloat units)

    Convenience function that calls glPolygonOffset(\a factor, \a units).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glPolygonOffset.xhtml}{glPolygonOffset()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)

    Convenience function that calls glReadPixels(\a x, \a y, \a width, \a height, \a format, \a type, \a pixels).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glReadPixels.xhtml}{glReadPixels()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glScissor(GLint x, GLint y, GLsizei width, GLsizei height)

    Convenience function that calls glScissor(\a x, \a y, \a width, \a height).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glScissor.xhtml}{glScissor()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glStencilFunc(GLenum func, GLint ref, GLuint mask)

    Convenience function that calls glStencilFunc(\a func, \a ref, \a mask).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glStencilFunc.xhtml}{glStencilFunc()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glStencilMask(GLuint mask)

    Convenience function that calls glStencilMask(\a mask).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glStencilMask.xhtml}{glStencilMask()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)

    Convenience function that calls glStencilOp(\a fail, \a zfail, \a zpass).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glStencilOp.xhtml}{glStencilOp()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)

    Convenience function that calls glTexImage2D(\a target, \a level, \a internalformat, \a width, \a height, \a border, \a format, \a type, \a pixels).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexImage2D.xhtml}{glTexImage2D()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexParameterf(GLenum target, GLenum pname, GLfloat param)

    Convenience function that calls glTexParameterf(\a target, \a pname, \a param).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexParameter.xhtml}{glTexParameterf()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)

    Convenience function that calls glTexParameterfv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexParameter.xhtml}{glTexParameterfv()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexParameteri(GLenum target, GLenum pname, GLint param)

    Convenience function that calls glTexParameteri(\a target, \a pname, \a param).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexParameter.xhtml}{glTexParameteri()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexParameteriv(GLenum target, GLenum pname, const GLint* params)

    Convenience function that calls glTexParameteriv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexParameter.xhtml}{glTexParameteriv()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)

    Convenience function that calls glTexSubImage2D(\a target, \a level, \a xoffset, \a yoffset, \a width, \a height, \a format, \a type, \a pixels).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexSubImage2D.xhtml}{glTexSubImage2D()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glViewport(GLint x, GLint y, GLsizei width, GLsizei height)

    Convenience function that calls glViewport(\a x, \a y, \a width, \a height).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glViewport.xhtml}{glViewport()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glActiveTexture(GLenum texture)

    Convenience function that calls glActiveTexture(\a texture).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glActiveTexture.xhtml}{glActiveTexture()}.
*/

/*!
    \fn void QOpenGLFunctions::glAttachShader(GLuint program, GLuint shader)

    Convenience function that calls glAttachShader(\a program, \a shader).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glAttachShader.xhtml}{glAttachShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glBindAttribLocation(GLuint program, GLuint index, const char* name)

    Convenience function that calls glBindAttribLocation(\a program, \a index, \a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindAttribLocation.xhtml}{glBindAttribLocation()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glBindBuffer(GLenum target, GLuint buffer)

    Convenience function that calls glBindBuffer(\a target, \a buffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindBuffer.xhtml}{glBindBuffer()}.
*/

/*!
    \fn void QOpenGLFunctions::glBindFramebuffer(GLenum target, GLuint framebuffer)

    Convenience function that calls glBindFramebuffer(\a target, \a framebuffer).

    Note that Qt will translate a \a framebuffer argument of 0 to the currently
    bound QOpenGLContext's defaultFramebufferObject().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindFramebuffer.xhtml}{glBindFramebuffer()}.
*/

/*!
    \fn void QOpenGLFunctions::glBindRenderbuffer(GLenum target, GLuint renderbuffer)

    Convenience function that calls glBindRenderbuffer(\a target, \a renderbuffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindRenderbuffer.xhtml}{glBindRenderbuffer()}.
*/

/*!
    \fn void QOpenGLFunctions::glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)

    Convenience function that calls glBlendColor(\a red, \a green, \a blue, \a alpha).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlendColor.xhtml}{glBlendColor()}.
*/

/*!
    \fn void QOpenGLFunctions::glBlendEquation(GLenum mode)

    Convenience function that calls glBlendEquation(\a mode).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlendEquation.xhtml}{glBlendEquation()}.
*/

/*!
    \fn void QOpenGLFunctions::glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)

    Convenience function that calls glBlendEquationSeparate(\a modeRGB, \a modeAlpha).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlendEquationSeparate.xhtml}{glBlendEquationSeparate()}.
*/

/*!
    \fn void QOpenGLFunctions::glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)

    Convenience function that calls glBlendFuncSeparate(\a srcRGB, \a dstRGB, \a srcAlpha, \a dstAlpha).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlendFuncSeparate.xhtml}{glBlendFuncSeparate()}.
*/

/*!
    \fn void QOpenGLFunctions::glBufferData(GLenum target, qopengl_GLsizeiptr size, const void* data, GLenum usage)

    Convenience function that calls glBufferData(\a target, \a size, \a data, \a usage).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBufferData.xhtml}{glBufferData()}.
*/

/*!
    \fn void QOpenGLFunctions::glBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, const void* data)

    Convenience function that calls glBufferSubData(\a target, \a offset, \a size, \a data).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBufferSubData.xhtml}{glBufferSubData()}.
*/

/*!
    \fn GLenum QOpenGLFunctions::glCheckFramebufferStatus(GLenum target)

    Convenience function that calls glCheckFramebufferStatus(\a target).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCheckFramebufferStatus.xhtml}{glCheckFramebufferStatus()}.
*/

/*!
    \fn void QOpenGLFunctions::glClearDepthf(GLclampf depth)

    Convenience function that calls glClearDepth(\a depth) on
    desktop OpenGL systems and glClearDepthf(\a depth) on
    embedded OpenGL ES systems.

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glClearDepthf.xhtml}{glClearDepthf()}.
*/

/*!
    \fn void QOpenGLFunctions::glCompileShader(GLuint shader)

    Convenience function that calls glCompileShader(\a shader).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCompileShader.xhtml}{glCompileShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)

    Convenience function that calls glCompressedTexImage2D(\a target, \a level, \a internalformat, \a width, \a height, \a border, \a imageSize, \a data).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCompressedTexImage2D.xhtml}{glCompressedTexImage2D()}.
*/

/*!
    \fn void QOpenGLFunctions::glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)

    Convenience function that calls glCompressedTexSubImage2D(\a target, \a level, \a xoffset, \a yoffset, \a width, \a height, \a format, \a imageSize, \a data).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCompressedTexSubImage2D.xhtml}{glCompressedTexSubImage2D()}.
*/

/*!
    \fn GLuint QOpenGLFunctions::glCreateProgram()

    Convenience function that calls glCreateProgram().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCreateProgram.xhtml}{glCreateProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn GLuint QOpenGLFunctions::glCreateShader(GLenum type)

    Convenience function that calls glCreateShader(\a type).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCreateShader.xhtml}{glCreateShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glDeleteBuffers(GLsizei n, const GLuint* buffers)

    Convenience function that calls glDeleteBuffers(\a n, \a buffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteBuffers.xhtml}{glDeleteBuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)

    Convenience function that calls glDeleteFramebuffers(\a n, \a framebuffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteFramebuffers.xhtml}{glDeleteFramebuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glDeleteProgram(GLuint program)

    Convenience function that calls glDeleteProgram(\a program).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteProgram.xhtml}{glDeleteProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)

    Convenience function that calls glDeleteRenderbuffers(\a n, \a renderbuffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteRenderbuffers.xhtml}{glDeleteRenderbuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glDeleteShader(GLuint shader)

    Convenience function that calls glDeleteShader(\a shader).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteShader.xhtml}{glDeleteShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glDepthRangef(GLclampf zNear, GLclampf zFar)

    Convenience function that calls glDepthRange(\a zNear, \a zFar) on
    desktop OpenGL systems and glDepthRangef(\a zNear, \a zFar) on
    embedded OpenGL ES systems.

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDepthRangef.xhtml}{glDepthRangef()}.
*/

/*!
    \fn void QOpenGLFunctions::glDetachShader(GLuint program, GLuint shader)

    Convenience function that calls glDetachShader(\a program, \a shader).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDetachShader.xhtml}{glDetachShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glDisableVertexAttribArray(GLuint index)

    Convenience function that calls glDisableVertexAttribArray(\a index).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es2.0/html/glDisableVertexAttribArray.xhtml}{glDisableVertexAttribArray()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glEnableVertexAttribArray(GLuint index)

    Convenience function that calls glEnableVertexAttribArray(\a index).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glEnableVertexAttribArray.xhtml}{glEnableVertexAttribArray()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)

    Convenience function that calls glFramebufferRenderbuffer(\a target, \a attachment, \a renderbuffertarget, \a renderbuffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glFramebufferRenderbuffer.xhtml}{glFramebufferRenderbuffer()}.
*/

/*!
    \fn void QOpenGLFunctions::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)

    Convenience function that calls glFramebufferTexture2D(\a target, \a attachment, \a textarget, \a texture, \a level).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glFramebufferTexture2D.xhtml}{glFramebufferTexture2D()}.
*/

/*!
    \fn void QOpenGLFunctions::glGenBuffers(GLsizei n, GLuint* buffers)

    Convenience function that calls glGenBuffers(\a n, \a buffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGenBuffers.xhtml}{glGenBuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glGenerateMipmap(GLenum target)

    Convenience function that calls glGenerateMipmap(\a target).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGenerateMipmap.xhtml}{glGenerateMipmap()}.
*/

/*!
    \fn void QOpenGLFunctions::glGenFramebuffers(GLsizei n, GLuint* framebuffers)

    Convenience function that calls glGenFramebuffers(\a n, \a framebuffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGenFramebuffers.xhtml}{glGenFramebuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)

    Convenience function that calls glGenRenderbuffers(\a n, \a renderbuffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGenRenderbuffers.xhtml}{glGenRenderbuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)

    Convenience function that calls glGetActiveAttrib(\a program, \a index, \a bufsize, \a length, \a size, \a type, \a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetActiveAttrib.xhtml}{glGetActiveAttrib()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)

    Convenience function that calls glGetActiveUniform(\a program, \a index, \a bufsize, \a length, \a size, \a type, \a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetActiveUniform.xhtml}{glGetActiveUniform()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)

    Convenience function that calls glGetAttachedShaders(\a program, \a maxcount, \a count, \a shaders).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetAttachedShaders.xhtml}{glGetAttachedShaders()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn GLint QOpenGLFunctions::glGetAttribLocation(GLuint program, const char* name)

    Convenience function that calls glGetAttribLocation(\a program, \a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetAttribLocation.xhtml}{glGetAttribLocation()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetBufferParameteriv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetBufferParameteriv.xhtml}{glGetBufferParameteriv()}.
*/

/*!
    \fn void QOpenGLFunctions::glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)

    Convenience function that calls glGetFramebufferAttachmentParameteriv(\a target, \a attachment, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetFramebufferAttachmentParameteriv.xhtml}{glGetFramebufferAttachmentParameteriv()}.
*/

/*!
    \fn void QOpenGLFunctions::glGetProgramiv(GLuint program, GLenum pname, GLint* params)

    Convenience function that calls glGetProgramiv(\a program, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetProgramiv.xhtml}{glGetProgramiv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog)

    Convenience function that calls glGetProgramInfoLog(\a program, \a bufsize, \a length, \a infolog).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetProgramInfoLog.xhtml}{glGetProgramInfoLog()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetRenderbufferParameteriv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetRenderbufferParameteriv.xhtml}{glGetRenderbufferParameteriv()}.
*/

/*!
    \fn void QOpenGLFunctions::glGetShaderiv(GLuint shader, GLenum pname, GLint* params)

    Convenience function that calls glGetShaderiv(\a shader, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetShaderiv.xhtml}{glGetShaderiv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog)

    Convenience function that calls glGetShaderInfoLog(\a shader, \a bufsize, \a length, \a infolog).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetShaderInfoLog.xhtml}{glGetShaderInfoLog()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)

    Convenience function that calls glGetShaderPrecisionFormat(\a shadertype, \a precisiontype, \a range, \a precision).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetShaderPrecisionFormat.xhtml}{glGetShaderPrecisionFormat()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source)

    Convenience function that calls glGetShaderSource(\a shader, \a bufsize, \a length, \a source).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetShaderSource.xhtml}{glGetShaderSource()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetUniformfv(GLuint program, GLint location, GLfloat* params)

    Convenience function that calls glGetUniformfv(\a program, \a location, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetUniform.xhtml}{glGetUniformfv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetUniformiv(GLuint program, GLint location, GLint* params)

    Convenience function that calls glGetUniformiv(\a program, \a location, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetUniform.xhtml}{glGetUniformiv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn GLint QOpenGLFunctions::glGetUniformLocation(GLuint program, const char* name)

    Convenience function that calls glGetUniformLocation(\a program, \a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetUniformLocation.xhtml}{glGetUniformLocation()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)

    Convenience function that calls glGetVertexAttribfv(\a index, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetVertexAttribfv.xhtml}{glGetVertexAttribfv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)

    Convenience function that calls glGetVertexAttribiv(\a index, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetVertexAttribiv.xhtml}{glGetVertexAttribiv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer)

    Convenience function that calls glGetVertexAttribPointerv(\a index, \a pname, \a pointer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetVertexAttribPointerv.xhtml}{glGetVertexAttribPointerv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsBuffer(GLuint buffer)

    Convenience function that calls glIsBuffer(\a buffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsBuffer.xhtml}{glIsBuffer()}.
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsFramebuffer(GLuint framebuffer)

    Convenience function that calls glIsFramebuffer(\a framebuffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsFramebuffer.xhtml}{glIsFramebuffer()}.
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsProgram(GLuint program)

    Convenience function that calls glIsProgram(\a program).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsProgram.xhtml}{glIsProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsRenderbuffer(GLuint renderbuffer)

    Convenience function that calls glIsRenderbuffer(\a renderbuffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsRenderbuffer.xhtml}{glIsRenderbuffer()}.
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsShader(GLuint shader)

    Convenience function that calls glIsShader(\a shader).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsShader.xhtml}{glIsShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glLinkProgram(GLuint program)

    Convenience function that calls glLinkProgram(\a program).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glLinkProgram.xhtml}{glLinkProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glReleaseShaderCompiler()

    Convenience function that calls glReleaseShaderCompiler().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glReleaseShaderCompiler.xhtml}{glReleaseShaderCompiler()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)

    Convenience function that calls glRenderbufferStorage(\a target, \a internalformat, \a width, \a height).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glRenderbufferStorage.xhtml}{glRenderbufferStorage()}.
*/

/*!
    \fn void QOpenGLFunctions::glSampleCoverage(GLclampf value, GLboolean invert)

    Convenience function that calls glSampleCoverage(\a value, \a invert).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glSampleCoverage.xhtml}{glSampleCoverage()}.
*/

/*!
    \fn void QOpenGLFunctions::glShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length)

    Convenience function that calls glShaderBinary(\a n, \a shaders, \a binaryformat, \a binary, \a length).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glShaderBinary.xhtml}{glShaderBinary()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length)

    Convenience function that calls glShaderSource(\a shader, \a count, \a string, \a length).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glShaderSource.xhtml}{glShaderSource()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)

    Convenience function that calls glStencilFuncSeparate(\a face, \a func, \a ref, \a mask).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glStencilFuncSeparate.xhtml}{glStencilFuncSeparate()}.
*/

/*!
    \fn void QOpenGLFunctions::glStencilMaskSeparate(GLenum face, GLuint mask)

    Convenience function that calls glStencilMaskSeparate(\a face, \a mask).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glStencilMaskSeparate.xhtml}{glStencilMaskSeparate()}.
*/

/*!
    \fn void QOpenGLFunctions::glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)

    Convenience function that calls glStencilOpSeparate(\a face, \a fail, \a zfail, \a zpass).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glStencilOpSeparate.xhtml}{glStencilOpSeparate()}.
*/

/*!
    \fn void QOpenGLFunctions::glUniform1f(GLint location, GLfloat x)

    Convenience function that calls glUniform1f(\a location, \a x).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform1f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform1fv(GLint location, GLsizei count, const GLfloat* v)

    Convenience function that calls glUniform1fv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform1fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform1i(GLint location, GLint x)

    Convenience function that calls glUniform1i(\a location, \a x).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform1i()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform1iv(GLint location, GLsizei count, const GLint* v)

    Convenience function that calls glUniform1iv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform1iv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform2f(GLint location, GLfloat x, GLfloat y)

    Convenience function that calls glUniform2f(\a location, \a x, \a y).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform2f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform2fv(GLint location, GLsizei count, const GLfloat* v)

    Convenience function that calls glUniform2fv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform2fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform2i(GLint location, GLint x, GLint y)

    Convenience function that calls glUniform2i(\a location, \a x, \a y).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform2i()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform2iv(GLint location, GLsizei count, const GLint* v)

    Convenience function that calls glUniform2iv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform2iv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)

    Convenience function that calls glUniform3f(\a location, \a x, \a y, \a z).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform3f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform3fv(GLint location, GLsizei count, const GLfloat* v)

    Convenience function that calls glUniform3fv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform3fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform3i(GLint location, GLint x, GLint y, GLint z)

    Convenience function that calls glUniform3i(\a location, \a x, \a y, \a z).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform3i()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform3iv(GLint location, GLsizei count, const GLint* v)

    Convenience function that calls glUniform3iv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform3iv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)

    Convenience function that calls glUniform4f(\a location, \a x, \a y, \a z, \a w).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform4f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform4fv(GLint location, GLsizei count, const GLfloat* v)

    Convenience function that calls glUniform4fv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform4fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)

    Convenience function that calls glUniform4i(\a location, \a x, \a y, \a z, \a w).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform4i()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform4iv(GLint location, GLsizei count, const GLint* v)

    Convenience function that calls glUniform4iv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform4iv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)

    Convenience function that calls glUniformMatrix2fv(\a location, \a count, \a transpose, \a value).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniformMatrix2fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)

    Convenience function that calls glUniformMatrix3fv(\a location, \a count, \a transpose, \a value).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniformMatrix3fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)

    Convenience function that calls glUniformMatrix4fv(\a location, \a count, \a transpose, \a value).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniformMatrix4fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUseProgram(GLuint program)

    Convenience function that calls glUseProgram(\a program).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUseProgram.xhtml}{glUseProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glValidateProgram(GLuint program)

    Convenience function that calls glValidateProgram(\a program).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glValidateProgram.xhtml}{glValidateProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib1f(GLuint indx, GLfloat x)

    Convenience function that calls glVertexAttrib1f(\a indx, \a x).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttrib1f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib1fv(GLuint indx, const GLfloat* values)

    Convenience function that calls glVertexAttrib1fv(\a indx, \a values).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttrib1fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)

    Convenience function that calls glVertexAttrib2f(\a indx, \a x, \a y).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttrib2f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib2fv(GLuint indx, const GLfloat* values)

    Convenience function that calls glVertexAttrib2fv(\a indx, \a values).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttrib2fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)

    Convenience function that calls glVertexAttrib3f(\a indx, \a x, \a y, \a z).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttrib3f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib3fv(GLuint indx, const GLfloat* values)

    Convenience function that calls glVertexAttrib3fv(\a indx, \a values).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttrib3fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)

    Convenience function that calls glVertexAttrib4f(\a indx, \a x, \a y, \a z, \a w).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttrib4f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib4fv(GLuint indx, const GLfloat* values)

    Convenience function that calls glVertexAttrib4fv(\a indx, \a values).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttrib4fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)

    Convenience function that calls glVertexAttribPointer(\a indx, \a size, \a type, \a normalized, \a stride, \a ptr).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttribPointer.xhtml}{glVertexAttribPointer()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn bool QOpenGLFunctions::isInitialized(const QOpenGLFunctionsPrivate *d)
    \internal
*/

namespace {

// this function tries hard to get the opengl function we're looking for by also
// trying to resolve it with some of the common extensions if the generic name
// can't be found.
static QFunctionPointer getProcAddress(QOpenGLContext *context, const char *funcName)
{
    QFunctionPointer function = context->getProcAddress(funcName);

    static const struct {
        const char *name;
        int len; // includes trailing \0
    } extensions[] = {
        { "ARB", 4 },
        { "OES", 4 },
        { "EXT", 4 },
        { "ANGLE", 6 },
        { "NV", 3 },
    };

    if (!function) {
        char fn[512];
        size_t size = strlen(funcName);
        Q_ASSERT(size < 500);
        memcpy(fn, funcName, size);
        char *ext = fn + size;

        for (const auto &e : extensions) {
            memcpy(ext, e.name, e.len);
            function = context->getProcAddress(fn);
            if (function)
                break;
        }
    }

    return function;
}

template <typename Func>
Func resolve(QOpenGLContext *context, const char *name, Func)
{
    return reinterpret_cast<Func>(getProcAddress(context, name));
}

}

#define RESOLVE(name) \
    resolve(context, "gl"#name, name)

#ifndef QT_OPENGL_ES_2

// some fallback functions
static void QOPENGLF_APIENTRY qopenglfSpecialClearDepthf(GLclampf depth)
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    QOpenGLFunctionsPrivate *funcs = qt_gl_functions(context);
    funcs->f.ClearDepth((GLdouble) depth);
}

static void QOPENGLF_APIENTRY qopenglfSpecialDepthRangef(GLclampf zNear, GLclampf zFar)
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    QOpenGLFunctionsPrivate *funcs = qt_gl_functions(context);
    funcs->f.DepthRange((GLdouble) zNear, (GLdouble) zFar);
}

static void QOPENGLF_APIENTRY qopenglfSpecialGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
    Q_UNUSED(shadertype);
    Q_UNUSED(precisiontype);
    range[0] = range[1] = precision[0] = 0;
}

static GLboolean QOPENGLF_APIENTRY qopenglfSpecialIsProgram(GLuint program)
{
    return program != 0;
}

static GLboolean QOPENGLF_APIENTRY qopenglfSpecialIsShader(GLuint shader)
{
    return shader != 0;
}

static void QOPENGLF_APIENTRY qopenglfSpecialReleaseShaderCompiler()
{
}

#endif // !QT_OPENGL_ES_2


QOpenGLFunctionsPrivate::QOpenGLFunctionsPrivate(QOpenGLContext *c)
{
    init(c);

#ifndef QT_OPENGL_ES_2
    // setup fallbacks in case some methods couldn't get resolved
    bool es = QOpenGLContext::currentContext()->isOpenGLES();
    if (!f.ClearDepthf || !es)
        f.ClearDepthf = qopenglfSpecialClearDepthf;
    if (!f.DepthRangef || !es)
        f.DepthRangef = qopenglfSpecialDepthRangef;
    if (!f.GetShaderPrecisionFormat)
        f.GetShaderPrecisionFormat = qopenglfSpecialGetShaderPrecisionFormat;
    if (!f.IsProgram)
        f.IsProgram = qopenglfSpecialIsProgram;
    if (!f.IsShader)
        f.IsShader = qopenglfSpecialIsShader;
    if (!f.ReleaseShaderCompiler)
        f.ReleaseShaderCompiler = qopenglfSpecialReleaseShaderCompiler;
#endif
}


QT_OPENGL_IMPLEMENT(QOpenGLFunctionsPrivate, QT_OPENGL_FUNCTIONS)

/*!
    \class QOpenGLExtraFunctions
    \brief The QOpenGLExtraFunctions class provides cross-platform access to the OpenGL ES 3.0, 3.1 and 3.2 API.
    \since 5.6
    \ingroup painting-3D
    \inmodule QtGui

    This subclass of QOpenGLFunctions includes the OpenGL ES 3.0, 3.1 and 3.2
    functions. These will only work when an OpenGL ES 3.x context, or an
    OpenGL context of a version containing the functions in question either in
    core or as extension, is in use. This allows developing GLES 3.x
    applications in a cross-platform manner: development can happen on a desktop
    platform with OpenGL 3.x or 4.x, deploying to a true GLES 3.x device later
    on will require no or minimal changes to the application.

    \note This class is different from the versioned OpenGL wrappers, for
    instance QOpenGLFunctions_3_2_Core. The versioned function wrappers target a
    given version and profile of OpenGL. They are therefore not suitable for
    cross-OpenGL-OpenGLES development.
 */

/*!
    \fn void QOpenGLExtraFunctions::glBeginQuery(GLenum target, GLuint id)

    Convenience function that calls glBeginQuery(\a target, \a id).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBeginQuery.xhtml}{glBeginQuery()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBeginTransformFeedback(GLenum primitiveMode)

    Convenience function that calls glBeginTransformFeedback(\a primitiveMode).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBeginTransformFeedback.xhtml}{glBeginTransformFeedback()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindBufferBase(GLenum target, GLuint index, GLuint buffer)

    Convenience function that calls glBindBufferBase(\a target, \a index, \a buffer).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindBufferBase.xhtml}{glBindBufferBase()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)

    Convenience function that calls glBindBufferRange(\a target, \a index, \a buffer, \a offset, \a size).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindBufferRange.xhtml}{glBindBufferRange()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindSampler(GLuint unit, GLuint sampler)

    Convenience function that calls glBindSampler(\a unit, \a sampler).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindSampler.xhtml}{glBindSampler()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindTransformFeedback(GLenum target, GLuint id)

    Convenience function that calls glBindTransformFeedback(\a target, \a id).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindTransformFeedback.xhtml}{glBindTransformFeedback()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindVertexArray(GLuint array)

    Convenience function that calls glBindVertexArray(\a array).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindVertexArray.xhtml}{glBindVertexArray()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)

    Convenience function that calls glBlitFramebuffer(\a srcX0, \a srcY0, \a srcX1, \a srcY1, \a dstX0, \a dstY0, \a dstX1, \a dstY1, \a mask, \a filter).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlitFramebuffer.xhtml}{glBlitFramebuffer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)

    Convenience function that calls glClearBufferfi(\a buffer, \a drawbuffer, \a depth, \a stencil).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glClearBuffer.xhtml}{glClearBufferfi()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat * value)

    Convenience function that calls glClearBufferfv(\a buffer, \a drawbuffer, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glClearBuffer.xhtml}{glClearBufferfv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint * value)

    Convenience function that calls glClearBufferiv(\a buffer, \a drawbuffer, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glClearBuffer.xhtml}{glClearBufferiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint * value)

    Convenience function that calls glClearBufferuiv(\a buffer, \a drawbuffer, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glClearBuffer.xhtml}{glClearBufferuiv()}.
*/

/*!
    \fn GLenum QOpenGLExtraFunctions::glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)

    Convenience function that calls glClientWaitSync(\a sync, \a flags, \a timeout).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glClientWaitSync.xhtml}{glClientWaitSync()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data)

    Convenience function that calls glCompressedTexImage3D(\a target, \a level, \a internalformat, \a width, \a height, \a depth, \a border, \a imageSize, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCompressedTexImage3D.xhtml}{glCompressedTexImage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void * data)

    Convenience function that calls glCompressedTexSubImage3D(\a target, \a level, \a xoffset, \a yoffset, \a zoffset, \a width, \a height, \a depth, \a format, \a imageSize, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCompressedTexSubImage3D.xhtml}{glCompressedTexSubImage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)

    Convenience function that calls glCopyBufferSubData(\a readTarget, \a writeTarget, \a readOffset, \a writeOffset, \a size).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCopyBufferSubData.xhtml}{glCopyBufferSubData()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)

    Convenience function that calls glCopyTexSubImage3D(\a target, \a level, \a xoffset, \a yoffset, \a zoffset, \a x, \a y, \a width, \a height).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCopyTexSubImage3D.xhtml}{glCopyTexSubImage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteQueries(GLsizei n, const GLuint * ids)

    Convenience function that calls glDeleteQueries(\a n, \a ids).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteQueries.xhtml}{glDeleteQueries()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteSamplers(GLsizei count, const GLuint * samplers)

    Convenience function that calls glDeleteSamplers(\a count, \a samplers).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteSamplers.xhtml}{glDeleteSamplers()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteSync(GLsync sync)

    Convenience function that calls glDeleteSync(\a sync).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteSync.xhtml}{glDeleteSync()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteTransformFeedbacks(GLsizei n, const GLuint * ids)

    Convenience function that calls glDeleteTransformFeedbacks(\a n, \a ids).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteTransformFeedbacks.xhtml}{glDeleteTransformFeedbacks()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteVertexArrays(GLsizei n, const GLuint * arrays)

    Convenience function that calls glDeleteVertexArrays(\a n, \a arrays).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteVertexArrays.xhtml}{glDeleteVertexArrays()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)

    Convenience function that calls glDrawArraysInstanced(\a mode, \a first, \a count, \a instancecount).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawArraysInstanced.xhtml}{glDrawArraysInstanced()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawBuffers(GLsizei n, const GLenum * bufs)

    Convenience function that calls glDrawBuffers(\a n, \a bufs).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawBuffers.xhtml}{glDrawBuffers()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount)

    Convenience function that calls glDrawElementsInstanced(\a mode, \a count, \a type, \a indices, \a instancecount).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawElementsInstanced.xhtml}{glDrawElementsInstanced()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void * indices)

    Convenience function that calls glDrawRangeElements(\a mode, \a start, \a end, \a count, \a type, \a indices).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawRangeElements.xhtml}{glDrawRangeElements()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glEndQuery(GLenum target)

    Convenience function that calls glEndQuery(\a target).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glEndQuery.xhtml}{glEndQuery()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glEndTransformFeedback()

    Convenience function that calls glEndTransformFeedback().

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glEndTransformFeedback.xhtml}{glEndTransformFeedback()}.
*/

/*!
    \fn GLsync QOpenGLExtraFunctions::glFenceSync(GLenum condition, GLbitfield flags)

    Convenience function that calls glFenceSync(\a condition, \a flags).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glFenceSync.xhtml}{glFenceSync()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)

    Convenience function that calls glFlushMappedBufferRange(\a target, \a offset, \a length).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glFlushMappedBufferRange.xhtml}{glFlushMappedBufferRange()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)

    Convenience function that calls glFramebufferTextureLayer(\a target, \a attachment, \a texture, \a level, \a layer).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glFramebufferTextureLayer.xhtml}{glFramebufferTextureLayer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGenQueries(GLsizei n, GLuint* ids)

    Convenience function that calls glGenQueries(\a n, \a ids).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGenQueries.xhtml}{glGenQueries()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGenSamplers(GLsizei count, GLuint* samplers)

    Convenience function that calls glGenSamplers(\a count, \a samplers).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGenSamplers.xhtml}{glGenSamplers()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGenTransformFeedbacks(GLsizei n, GLuint* ids)

    Convenience function that calls glGenTransformFeedbacks(\a n, \a ids).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGenTransformFeedbacks.xhtml}{glGenTransformFeedbacks()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGenVertexArrays(GLsizei n, GLuint* arrays)

    Convenience function that calls glGenVertexArrays(\a n, \a arrays).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGenVertexArrays.xhtml}{glGenVertexArrays()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName)

    Convenience function that calls glGetActiveUniformBlockName(\a program, \a uniformBlockIndex, \a bufSize, \a length, \a uniformBlockName).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetActiveUniformBlockName.xhtml}{glGetActiveUniformBlockName()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params)

    Convenience function that calls glGetActiveUniformBlockiv(\a program, \a uniformBlockIndex, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetActiveUniformBlockiv.xhtml}{glGetActiveUniformBlockiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint* params)

    Convenience function that calls glGetActiveUniformsiv(\a program, \a uniformCount, \a uniformIndices, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetActiveUniformsiv.xhtml}{glGetActiveUniformsiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64* params)

    Convenience function that calls glGetBufferParameteri64v(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetBufferParameter.xhtml}{glGetBufferParameteri64v()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetBufferPointerv(GLenum target, GLenum pname, void ** params)

    Convenience function that calls glGetBufferPointerv(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetBufferPointerv.xhtml}{glGetBufferPointerv()}.
*/

/*!
    \fn GLint QOpenGLExtraFunctions::glGetFragDataLocation(GLuint program, const GLchar * name)

    Convenience function that calls glGetFragDataLocation(\a program, \a name).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetFragDataLocation.xhtml}{glGetFragDataLocation()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetInteger64i_v(GLenum target, GLuint index, GLint64* data)

    Convenience function that calls glGetInteger64i_v(\a target, \a index, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGet.xhtml}{glGetInteger64i_v()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetInteger64v(GLenum pname, GLint64* data)

    Convenience function that calls glGetInteger64v(\a pname, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGet.xhtml}{glGetInteger64v()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetIntegeri_v(GLenum target, GLuint index, GLint* data)

    Convenience function that calls glGetIntegeri_v(\a target, \a index, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGet.xhtml}{glGetIntegeri_v()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint* params)

    Convenience function that calls glGetInternalformativ(\a target, \a internalformat, \a pname, \a bufSize, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetInternalformativ.xhtml}{glGetInternalformativ()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void * binary)

    Convenience function that calls glGetProgramBinary(\a program, \a bufSize, \a length, \a binaryFormat, \a binary).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetProgramBinary.xhtml}{glGetProgramBinary()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params)

    Convenience function that calls glGetQueryObjectuiv(\a id, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetQueryObjectuiv.xhtml}{glGetQueryObjectuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetQueryiv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetQueryiv(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetQueryiv.xhtml}{glGetQueryiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat* params)

    Convenience function that calls glGetSamplerParameterfv(\a sampler, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetSamplerParameter.xhtml}{glGetSamplerParameterfv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params)

    Convenience function that calls glGetSamplerParameteriv(\a sampler, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetSamplerParameter.xhtml}{glGetSamplerParameteriv()}.
*/

/*!
    \fn const GLubyte * QOpenGLExtraFunctions::glGetStringi(GLenum name, GLuint index)

    Convenience function that calls glGetStringi(\a name, \a index).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetString.xhtml}{glGetStringi()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values)

    Convenience function that calls glGetSynciv(\a sync, \a pname, \a bufSize, \a length, \a values).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetSynciv.xhtml}{glGetSynciv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name)

    Convenience function that calls glGetTransformFeedbackVarying(\a program, \a index, \a bufSize, \a length, \a size, \a type, \a name).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetTransformFeedbackVarying.xhtml}{glGetTransformFeedbackVarying()}.
*/

/*!
    \fn GLuint QOpenGLExtraFunctions::glGetUniformBlockIndex(GLuint program, const GLchar * uniformBlockName)

    Convenience function that calls glGetUniformBlockIndex(\a program, \a uniformBlockName).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetUniformBlockIndex.xhtml}{glGetUniformBlockIndex()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const* uniformNames, GLuint* uniformIndices)

    Convenience function that calls glGetUniformIndices(\a program, \a uniformCount, \a uniformNames, \a uniformIndices).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetUniformIndices.xhtml}{glGetUniformIndices()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetUniformuiv(GLuint program, GLint location, GLuint* params)

    Convenience function that calls glGetUniformuiv(\a program, \a location, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetUniform.xhtml}{glGetUniformuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetVertexAttribIiv(GLuint index, GLenum pname, GLint* params)

    Convenience function that calls glGetVertexAttribIiv(\a index, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetVertexAttrib.xhtml}{glGetVertexAttribIiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint* params)

    Convenience function that calls glGetVertexAttribIuiv(\a index, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetVertexAttrib.xhtml}{glGetVertexAttribIuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments)

    Convenience function that calls glInvalidateFramebuffer(\a target, \a numAttachments, \a attachments).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glInvalidateFramebuffer.xhtml}{glInvalidateFramebuffer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height)

    Convenience function that calls glInvalidateSubFramebuffer(\a target, \a numAttachments, \a attachments, \a x, \a y, \a width, \a height).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glInvalidateSubFramebuffer.xhtml}{glInvalidateSubFramebuffer()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsQuery(GLuint id)

    Convenience function that calls glIsQuery(\a id).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsQuery.xhtml}{glIsQuery()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsSampler(GLuint sampler)

    Convenience function that calls glIsSampler(\a sampler).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsSampler.xhtml}{glIsSampler()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsSync(GLsync sync)

    Convenience function that calls glIsSync(\a sync).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsSync.xhtml}{glIsSync()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsTransformFeedback(GLuint id)

    Convenience function that calls glIsTransformFeedback(\a id).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsTransformFeedback.xhtml}{glIsTransformFeedback()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsVertexArray(GLuint array)

    Convenience function that calls glIsVertexArray(\a array).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsVertexArray.xhtml}{glIsVertexArray()}.
*/

/*!
    \fn void * QOpenGLExtraFunctions::glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)

    Convenience function that calls glMapBufferRange(\a target, \a offset, \a length, \a access).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glMapBufferRange.xhtml}{glMapBufferRange()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glPauseTransformFeedback()

    Convenience function that calls glPauseTransformFeedback().

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glPauseTransformFeedback.xhtml}{glPauseTransformFeedback()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramBinary(GLuint program, GLenum binaryFormat, const void * binary, GLsizei length)

    Convenience function that calls glProgramBinary(\a program, \a binaryFormat, \a binary, \a length).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramBinary.xhtml}{glProgramBinary()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramParameteri(GLuint program, GLenum pname, GLint value)

    Convenience function that calls glProgramParameteri(\a program, \a pname, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramParameteri.xhtml}{glProgramParameteri()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glReadBuffer(GLenum src)

    Convenience function that calls glReadBuffer(\a src).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glReadBuffer.xhtml}{glReadBuffer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)

    Convenience function that calls glRenderbufferStorageMultisample(\a target, \a samples, \a internalformat, \a width, \a height).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glRenderbufferStorageMultisample.xhtml}{glRenderbufferStorageMultisample()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glResumeTransformFeedback()

    Convenience function that calls glResumeTransformFeedback().

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glResumeTransformFeedback.xhtml}{glResumeTransformFeedback()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)

    Convenience function that calls glSamplerParameterf(\a sampler, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glSamplerParameter.xhtml}{glSamplerParameterf()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat * param)

    Convenience function that calls glSamplerParameterfv(\a sampler, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glSamplerParameter.xhtml}{glSamplerParameterfv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSamplerParameteri(GLuint sampler, GLenum pname, GLint param)

    Convenience function that calls glSamplerParameteri(\a sampler, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glSamplerParameter.xhtml}{glSamplerParameteri()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint * param)

    Convenience function that calls glSamplerParameteriv(\a sampler, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glSamplerParameter.xhtml}{glSamplerParameteriv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels)

    Convenience function that calls glTexImage3D(\a target, \a level, \a internalformat, \a width, \a height, \a depth, \a border, \a format, \a type, \a pixels).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexImage3D.xhtml}{glTexImage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)

    Convenience function that calls glTexStorage2D(\a target, \a levels, \a internalformat, \a width, \a height).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexStorage2D.xhtml}{glTexStorage2D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)

    Convenience function that calls glTexStorage3D(\a target, \a levels, \a internalformat, \a width, \a height, \a depth).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexStorage3D.xhtml}{glTexStorage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels)

    Convenience function that calls glTexSubImage3D(\a target, \a level, \a xoffset, \a yoffset, \a zoffset, \a width, \a height, \a depth, \a format, \a type, \a pixels).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexSubImage3D.xhtml}{glTexSubImage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const* varyings, GLenum bufferMode)

    Convenience function that calls glTransformFeedbackVaryings(\a program, \a count, \a varyings, \a bufferMode).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTransformFeedbackVaryings.xhtml}{glTransformFeedbackVaryings()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform1ui(GLint location, GLuint v0)

    Convenience function that calls glUniform1ui(\a location, \a v0).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform1ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform1uiv(GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glUniform1uiv(\a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform1uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform2ui(GLint location, GLuint v0, GLuint v1)

    Convenience function that calls glUniform2ui(\a location, \a v0, \a v1).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform2ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform2uiv(GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glUniform2uiv(\a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform2uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)

    Convenience function that calls glUniform3ui(\a location, \a v0, \a v1, \a v2).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform3ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform3uiv(GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glUniform3uiv(\a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform3uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)

    Convenience function that calls glUniform4ui(\a location, \a v0, \a v1, \a v2, \a v3).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform4ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform4uiv(GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glUniform4uiv(\a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniform4uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)

    Convenience function that calls glUniformBlockBinding(\a program, \a uniformBlockIndex, \a uniformBlockBinding).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniformBlockBinding.xhtml}{glUniformBlockBinding()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix2x3fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniformMatrix2x3fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix2x4fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniformMatrix2x4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix3x2fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniformMatrix3x2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix3x4fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniformMatrix3x4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix4x2fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniformMatrix4x2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix4x3fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUniform.xhtml}{glUniformMatrix4x3fv()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glUnmapBuffer(GLenum target)

    Convenience function that calls glUnmapBuffer(\a target).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUnmapBuffer.xhtml}{glUnmapBuffer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribDivisor(GLuint index, GLuint divisor)

    Convenience function that calls glVertexAttribDivisor(\a index, \a divisor).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttribDivisor.xhtml}{glVertexAttribDivisor()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w)

    Convenience function that calls glVertexAttribI4i(\a index, \a x, \a y, \a z, \a w).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttribI4i()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribI4iv(GLuint index, const GLint * v)

    Convenience function that calls glVertexAttribI4iv(\a index, \a v).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttribI4iv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)

    Convenience function that calls glVertexAttribI4ui(\a index, \a x, \a y, \a z, \a w).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttribI4ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribI4uiv(GLuint index, const GLuint * v)

    Convenience function that calls glVertexAttribI4uiv(\a index, \a v).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttrib.xhtml}{glVertexAttribI4uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer)

    Convenience function that calls glVertexAttribIPointer(\a index, \a size, \a type, \a stride, \a pointer).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml}{glVertexAttribIPointer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)

    Convenience function that calls glWaitSync(\a sync, \a flags, \a timeout).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glWaitSync.xhtml}{glWaitSync()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glActiveShaderProgram(GLuint pipeline, GLuint program)

    Convenience function that calls glActiveShaderProgram(\a pipeline, \a program).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glActiveShaderProgram.xhtml}{glActiveShaderProgram()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)

    Convenience function that calls glBindImageTexture(\a unit, \a texture, \a level, \a layered, \a layer, \a access, \a format).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindImageTexture.xhtml}{glBindImageTexture()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindProgramPipeline(GLuint pipeline)

    Convenience function that calls glBindProgramPipeline(\a pipeline).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindProgramPipeline.xhtml}{glBindProgramPipeline()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)

    Convenience function that calls glBindVertexBuffer(\a bindingindex, \a buffer, \a offset, \a stride).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBindVertexBuffer.xhtml}{glBindVertexBuffer()}.
*/

/*!
    \fn GLuint QOpenGLExtraFunctions::glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const* strings)

    Convenience function that calls glCreateShaderProgramv(\a type, \a count, \a strings).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCreateShaderProgramv.xhtml}{glCreateShaderProgramv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteProgramPipelines(GLsizei n, const GLuint * pipelines)

    Convenience function that calls glDeleteProgramPipelines(\a n, \a pipelines).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDeleteProgramPipelines.xhtml}{glDeleteProgramPipelines()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)

    Convenience function that calls glDispatchCompute(\a num_groups_x, \a num_groups_y, \a num_groups_z).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDispatchCompute.xhtml}{glDispatchCompute()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDispatchComputeIndirect(GLintptr indirect)

    Convenience function that calls glDispatchComputeIndirect(\a indirect).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDispatchComputeIndirect.xhtml}{glDispatchComputeIndirect()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawArraysIndirect(GLenum mode, const void * indirect)

    Convenience function that calls glDrawArraysIndirect(\a mode, \a indirect).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawArraysIndirect.xhtml}{glDrawArraysIndirect()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect)

    Convenience function that calls glDrawElementsIndirect(\a mode, \a type, \a indirect).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawElementsIndirect.xhtml}{glDrawElementsIndirect()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glFramebufferParameteri(GLenum target, GLenum pname, GLint param)

    Convenience function that calls glFramebufferParameteri(\a target, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glFramebufferParameteri.xhtml}{glFramebufferParameteri()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGenProgramPipelines(GLsizei n, GLuint* pipelines)

    Convenience function that calls glGenProgramPipelines(\a n, \a pipelines).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGenProgramPipelines.xhtml}{glGenProgramPipelines()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetBooleani_v(GLenum target, GLuint index, GLboolean* data)

    Convenience function that calls glGetBooleani_v(\a target, \a index, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGet.xhtml}{glGetBooleani_v()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetFramebufferParameteriv(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetFramebufferParameteriv.xhtml}{glGetFramebufferParameteriv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetMultisamplefv(GLenum pname, GLuint index, GLfloat* val)

    Convenience function that calls glGetMultisamplefv(\a pname, \a index, \a val).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetMultisamplefv.xhtml}{glGetMultisamplefv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params)

    Convenience function that calls glGetProgramInterfaceiv(\a program, \a programInterface, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetProgramInterface.xhtml}{glGetProgramInterfaceiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei* length, GLchar* infoLog)

    Convenience function that calls glGetProgramPipelineInfoLog(\a pipeline, \a bufSize, \a length, \a infoLog).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetProgramPipelineInfoLog.xhtml}{glGetProgramPipelineInfoLog()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint* params)

    Convenience function that calls glGetProgramPipelineiv(\a pipeline, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetProgramPipeline.xhtml}{glGetProgramPipelineiv()}.
*/

/*!
    \fn GLuint QOpenGLExtraFunctions::glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar * name)

    Convenience function that calls glGetProgramResourceIndex(\a program, \a programInterface, \a name).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetProgramResourceIndex.xhtml}{glGetProgramResourceIndex()}.
*/

/*!
    \fn GLint QOpenGLExtraFunctions::glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar * name)

    Convenience function that calls glGetProgramResourceLocation(\a program, \a programInterface, \a name).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetProgramResourceLocation.xhtml}{glGetProgramResourceLocation()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar* name)

    Convenience function that calls glGetProgramResourceName(\a program, \a programInterface, \a index, \a bufSize, \a length, \a name).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetProgramResourceName.xhtml}{glGetProgramResourceName()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei bufSize, GLsizei* length, GLint* params)

    Convenience function that calls glGetProgramResourceiv(\a program, \a programInterface, \a index, \a propCount, \a props, \a bufSize, \a length, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetProgramResource.xhtml}{glGetProgramResourceiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat* params)

    Convenience function that calls glGetTexLevelParameterfv(\a target, \a level, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetTexLevelParameter.xhtml}{glGetTexLevelParameterfv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params)

    Convenience function that calls glGetTexLevelParameteriv(\a target, \a level, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetTexLevelParameter.xhtml}{glGetTexLevelParameteriv()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsProgramPipeline(GLuint pipeline)

    Convenience function that calls glIsProgramPipeline(\a pipeline).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsProgramPipeline.xhtml}{glIsProgramPipeline()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glMemoryBarrier(GLbitfield barriers)

    Convenience function that calls glMemoryBarrier(\a barriers).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3.1/html/glMemoryBarrier.xhtml}{glMemoryBarrier()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glMemoryBarrierByRegion(GLbitfield barriers)

    Convenience function that calls glMemoryBarrierByRegion(\a barriers).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3.1/html/glMemoryBarrier.xhtml}{glMemoryBarrierByRegion()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1f(GLuint program, GLint location, GLfloat v0)

    Convenience function that calls glProgramUniform1f(\a program, \a location, \a v0).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform1f()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)

    Convenience function that calls glProgramUniform1fv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform1fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1i(GLuint program, GLint location, GLint v0)

    Convenience function that calls glProgramUniform1i(\a program, \a location, \a v0).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform1i()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint * value)

    Convenience function that calls glProgramUniform1iv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform1iv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1ui(GLuint program, GLint location, GLuint v0)

    Convenience function that calls glProgramUniform1ui(\a program, \a location, \a v0).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform1ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glProgramUniform1uiv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform1uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1)

    Convenience function that calls glProgramUniform2f(\a program, \a location, \a v0, \a v1).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform2f()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)

    Convenience function that calls glProgramUniform2fv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1)

    Convenience function that calls glProgramUniform2i(\a program, \a location, \a v0, \a v1).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform2i()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint * value)

    Convenience function that calls glProgramUniform2iv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform2iv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1)

    Convenience function that calls glProgramUniform2ui(\a program, \a location, \a v0, \a v1).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform2ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glProgramUniform2uiv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform2uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2)

    Convenience function that calls glProgramUniform3f(\a program, \a location, \a v0, \a v1, \a v2).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform3f()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)

    Convenience function that calls glProgramUniform3fv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform3fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2)

    Convenience function that calls glProgramUniform3i(\a program, \a location, \a v0, \a v1, \a v2).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform3i()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint * value)

    Convenience function that calls glProgramUniform3iv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform3iv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2)

    Convenience function that calls glProgramUniform3ui(\a program, \a location, \a v0, \a v1, \a v2).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform3ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glProgramUniform3uiv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform3uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)

    Convenience function that calls glProgramUniform4f(\a program, \a location, \a v0, \a v1, \a v2, \a v3).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform4f()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)

    Convenience function that calls glProgramUniform4fv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3)

    Convenience function that calls glProgramUniform4i(\a program, \a location, \a v0, \a v1, \a v2, \a v3).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform4i()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint * value)

    Convenience function that calls glProgramUniform4iv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform4iv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)

    Convenience function that calls glProgramUniform4ui(\a program, \a location, \a v0, \a v1, \a v2, \a v3).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform4ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glProgramUniform4uiv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniform4uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix2fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniformMatrix2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix2x3fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniformMatrix2x3fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix2x4fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniformMatrix2x4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix3fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniformMatrix3fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix3x2fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniformMatrix3x2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix3x4fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniformMatrix3x4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix4fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniformMatrix4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix4x2fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniformMatrix4x2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix4x3fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glProgramUniform.xhtml}{glProgramUniformMatrix4x3fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSampleMaski(GLuint maskNumber, GLbitfield mask)

    Convenience function that calls glSampleMaski(\a maskNumber, \a mask).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glSampleMaski.xhtml}{glSampleMaski()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)

    Convenience function that calls glTexStorage2DMultisample(\a target, \a samples, \a internalformat, \a width, \a height, \a fixedsamplelocations).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexStorage2DMultisample.xhtml}{glTexStorage2DMultisample()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program)

    Convenience function that calls glUseProgramStages(\a pipeline, \a stages, \a program).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glUseProgramStages.xhtml}{glUseProgramStages()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glValidateProgramPipeline(GLuint pipeline)

    Convenience function that calls glValidateProgramPipeline(\a pipeline).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glValidateProgramPipeline.xhtml}{glValidateProgramPipeline()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribBinding(GLuint attribindex, GLuint bindingindex)

    Convenience function that calls glVertexAttribBinding(\a attribindex, \a bindingindex).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttribBinding.xhtml}{glVertexAttribBinding()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset)

    Convenience function that calls glVertexAttribFormat(\a attribindex, \a size, \a type, \a normalized, \a relativeoffset).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttribFormat.xhtml}{glVertexAttribFormat()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset)

    Convenience function that calls glVertexAttribIFormat(\a attribindex, \a size, \a type, \a relativeoffset).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttribIFormat.xhtml}{glVertexAttribIFormat()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexBindingDivisor(GLuint bindingindex, GLuint divisor)

    Convenience function that calls glVertexBindingDivisor(\a bindingindex, \a divisor).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexBindingDivisor.xhtml}{glVertexBindingDivisor()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBlendBarrier(void)

    Convenience function that calls glBlendBarrier().

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlendBarrier.xhtml}{glBlendBarrier()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha)

    Convenience function that calls glBlendEquationSeparatei(\a buf, \a modeRGB, \a modeAlpha).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlendEquationSeparate.xhtml}{glBlendEquationSeparatei()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBlendEquationi(GLuint buf, GLenum mode)

    Convenience function that calls glBlendEquationi(\a buf, \a mode).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlendEquationi.xhtml}{glBlendEquationi()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)

    Convenience function that calls glBlendFuncSeparatei(\a buf, \a srcRGB, \a dstRGB, \a srcAlpha, \a dstAlpha).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlendFuncSeparate.xhtml}{glBlendFuncSeparatei()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBlendFunci(GLuint buf, GLenum src, GLenum dst)

    Convenience function that calls glBlendFunci(\a buf, \a src, \a dst).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBlendFunci.xhtml}{glBlendFunci()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a)

    Convenience function that calls glColorMaski(\a index, \a r, \a g, \a b, \a a).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glColorMask.xhtml}{glColorMaski()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth)

    Convenience function that calls glCopyImageSubData(\a srcName, \a srcTarget, \a srcLevel, \a srcX, \a srcY, \a srcZ, \a dstName, \a dstTarget, \a dstLevel, \a dstX, \a dstY, \a dstZ, \a srcWidth, \a srcHeight, \a srcDepth).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCopyImageSubData.xhtml}{glCopyImageSubData()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDebugMessageCallback(GLDEBUGPROC callback, const void * userParam)

    Convenience function that calls glDebugMessageCallback(\a callback, \a userParam).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDebugMessageCallback.xhtml}{glDebugMessageCallback()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint * ids, GLboolean enabled)

    Convenience function that calls glDebugMessageControl(\a source, \a type, \a severity, \a count, \a ids, \a enabled).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDebugMessageControl.xhtml}{glDebugMessageContro()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * buf)

    Convenience function that calls glDebugMessageInsert(\a source, \a type, \a id, \a severity, \a length, \a buf).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDebugMessageInsert.xhtml}{glDebugMessageInsert()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDisablei(GLenum target, GLuint index)

    Convenience function that calls glDisablei(\a target, \a index).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glEnable.xhtml}{glDisablei()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void * indices, GLint basevertex)

    Convenience function that calls glDrawElementsBaseVertex(\a mode, \a count, \a type, \a indices, \a basevertex).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawElementsBaseVertex.xhtml}{glDrawElementsBaseVerte()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount, GLint basevertex)

    Convenience function that calls glDrawElementsInstancedBaseVertex(\a mode, \a count, \a type, \a indices, \a instancecount, \a basevertex).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawElementsInstancedBaseVertex.xhtml}{glDrawElementsInstancedBaseVerte()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void * indices, GLint basevertex)

    Convenience function that calls glDrawRangeElementsBaseVertex(\a mode, \a start, \a end, \a count, \a type, \a indices, \a basevertex).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawRangeElementsBaseVertex.xhtml}{glDrawRangeElementsBaseVerte()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glEnablei(GLenum target, GLuint index)

    Convenience function that calls glEnablei(\a target, \a index).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glEnablei.xhtml}{glEnablei()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level)

    Convenience function that calls glFramebufferTexture(\a target, \a attachment, \a texture, \a level).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glFramebufferTexture.xhtml}{glFramebufferTexture()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum* sources, GLenum* types, GLuint* ids, GLenum* severities, GLsizei* lengths, GLchar* messageLog)

    Convenience function that calls glGetDebugMessageLog(\a count, \a bufSize, \a sources, \a types, \a ids, \a severities, \a lengths, \a messageLog).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetDebugMessageLog.xhtml}{glGetDebugMessageLog()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetGraphicsResetStatus(void)

    Convenience function that calls glGetGraphicsResetStatus().

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetGraphicsResetStatus.xhtml}{glGetGraphicsResetStatus()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei* length, GLchar* label)

    Convenience function that calls glGetObjectLabel(\a identifier, \a name, \a bufSize, \a length, \a label).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetObjectLabel.xhtml}{glGetObjectLabe()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetObjectPtrLabel(const void * ptr, GLsizei bufSize, GLsizei* length, GLchar* label)

    Convenience function that calls glGetObjectPtrLabel(\a ptr, \a bufSize, \a length, \a label).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetObjectPtrLabel.xhtml}{glGetObjectPtrLabe()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetPointerv(GLenum pname, void ** params)

    Convenience function that calls glGetPointerv(\a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetPointerv.xhtml}{glGetPointerv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint* params)

    Convenience function that calls glGetSamplerParameterIiv(\a sampler, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetSamplerParameter.xhtml}{glGetSamplerParameterIiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint* params)

    Convenience function that calls glGetSamplerParameterIuiv(\a sampler, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetSamplerParameter.xhtml}{glGetSamplerParameterIuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetTexParameterIiv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetTexParameterIiv(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetTexParameter.xhtml}{glGetTexParameterIiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint* params)

    Convenience function that calls glGetTexParameterIuiv(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetTexParameter.xhtml}{glGetTexParameterIuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat* params)

    Convenience function that calls glGetnUniformfv(\a program, \a location, \a bufSize, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetUniform.xhtml}{glGetnUniformfv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint* params)

    Convenience function that calls glGetnUniformiv(\a program, \a location, \a bufSize, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetUniform.xhtml}{glGetnUniformiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint* params)

    Convenience function that calls glGetnUniformuiv(\a program, \a location, \a bufSize, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetUniform.xhtml}{glGetnUniformuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glIsEnabledi(GLenum target, GLuint index)

    Convenience function that calls glIsEnabledi(\a target, \a index).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glIsEnabled.xhtml}{glIsEnabledi()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glMinSampleShading(GLfloat value)

    Convenience function that calls glMinSampleShading(\a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glMinSampleShading.xhtml}{glMinSampleShading()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar * label)

    Convenience function that calls glObjectLabel(\a identifier, \a name, \a length, \a label).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glObjectLabel.xhtml}{glObjectLabe()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glObjectPtrLabel(const void * ptr, GLsizei length, const GLchar * label)

    Convenience function that calls glObjectPtrLabel(\a ptr, \a length, \a label).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glObjectPtrLabel.xhtml}{glObjectPtrLabe()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glPatchParameteri(GLenum pname, GLint value)

    Convenience function that calls glPatchParameteri(\a pname, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glPatchParameteri.xhtml}{glPatchParameteri()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glPopDebugGroup(void)

    Convenience function that calls glPopDebugGroup().

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glPopDebugGroup.xhtml}{glPopDebugGroup()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glPrimitiveBoundingBox(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW)

    Convenience function that calls glPrimitiveBoundingBox(\a minX, \a minY, \a minZ, \a minW, \a maxX, \a maxY, \a maxZ, \a maxW).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glPrimitiveBoundingBox.xhtml}{glPrimitiveBoundingBo()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar * message)

    Convenience function that calls glPushDebugGroup(\a source, \a id, \a length, \a message).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glPushDebugGroup.xhtml}{glPushDebugGroup()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void * data)

    Convenience function that calls glReadnPixels(\a x, \a y, \a width, \a height, \a format, \a type, \a bufSize, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glReadPixels.xhtml}{glReadnPixels()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint * param)

    Convenience function that calls glSamplerParameterIiv(\a sampler, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glSamplerParameter.xhtml}{glSamplerParameterIiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint * param)

    Convenience function that calls glSamplerParameterIuiv(\a sampler, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glSamplerParameter.xhtml}{glSamplerParameterIuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer)

    Convenience function that calls glTexBuffer(\a target, \a internalformat, \a buffer).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexBuffer.xhtml}{glTexBuffer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size)

    Convenience function that calls glTexBufferRange(\a target, \a internalformat, \a buffer, \a offset, \a size).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexBufferRange.xhtml}{glTexBufferRange()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexParameterIiv(GLenum target, GLenum pname, const GLint * params)

    Convenience function that calls glTexParameterIiv(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexParameter.xhtml}{glTexParameterIiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexParameterIuiv(GLenum target, GLenum pname, const GLuint * params)

    Convenience function that calls glTexParameterIuiv(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexParameter.xhtml}{glTexParameterIuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations)

    Convenience function that calls glTexStorage3DMultisample(\a target, \a samples, \a internalformat, \a width, \a height, \a depth, \a fixedsamplelocations).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.2 documentation for
    \l{https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexStorage3DMultisample.xhtml}{glTexStorage3DMultisample()}.
*/

/*!
    \fn bool QOpenGLExtraFunctions::isInitialized(const QOpenGLExtraFunctionsPrivate *d)
    \internal
*/


/*!
    Constructs a default function resolver. The resolver cannot be used until
    \l {QOpenGLFunctions::}{initializeOpenGLFunctions()} is called to specify
    the context.
*/
QOpenGLExtraFunctions::QOpenGLExtraFunctions()
{
}

/*!
    Constructs a function resolver for context. If \a context is null, then
    the resolver will be created for the current QOpenGLContext.

    The context or another context in the group must be current.

    An object constructed in this way can only be used with context and other
    contexts that share with it. Use \l {QOpenGLFunctions::}
    {initializeOpenGLFunctions()} to change the object's context association.
*/
QOpenGLExtraFunctions::QOpenGLExtraFunctions(QOpenGLContext *context)
    : QOpenGLFunctions(context)
{
}

QOpenGLExtraFunctionsPrivate::QOpenGLExtraFunctionsPrivate(QOpenGLContext *ctx)
    : QOpenGLFunctionsPrivate(ctx)
{
    init(ctx);
}

QT_OPENGL_IMPLEMENT(QOpenGLExtraFunctionsPrivate, QT_OPENGL_EXTRA_FUNCTIONS)

QOpenGLExtensionsPrivate::QOpenGLExtensionsPrivate(QOpenGLContext *ctx)
    : QOpenGLExtraFunctionsPrivate(ctx),
      flushVendorChecked(false)
{
    QOpenGLContext *context = QOpenGLContext::currentContext();

    MapBuffer = RESOLVE(MapBuffer);
    GetBufferSubData = RESOLVE(GetBufferSubData);
    DiscardFramebuffer = RESOLVE(DiscardFramebuffer);
 }

void QOpenGLExtensions::flushShared()
{
    Q_D(QOpenGLExtensions);

    if (!d->flushVendorChecked) {
        d->flushVendorChecked = true;
        // It is not quite clear if glFlush() is sufficient to synchronize access to
        // resources between sharing contexts in the same thread. On most platforms this
        // is enough (e.g. iOS explicitly documents it), while certain drivers only work
        // properly when doing glFinish().
        d->flushIsSufficientToSyncContexts = false; // default to false, not guaranteed by the spec
        const char *vendor = (const char *) glGetString(GL_VENDOR);
        if (vendor) {
            static const char *const flushEnough[] = { "Apple", "ATI", "Intel", "NVIDIA" };
            for (size_t i = 0; i < sizeof(flushEnough) / sizeof(const char *); ++i) {
                if (strstr(vendor, flushEnough[i])) {
                    d->flushIsSufficientToSyncContexts = true;
                    break;
                }
            }
        }
    }

    if (d->flushIsSufficientToSyncContexts)
        glFlush();
    else
        glFinish();
}

QT_END_NAMESPACE
