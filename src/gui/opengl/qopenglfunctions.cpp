/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <QtCore/qloggingcategory.h>

#ifdef Q_OS_IOS
#include <dlfcn.h>
#endif

#ifndef GL_FRAMEBUFFER_SRGB_CAPABLE_EXT
#define GL_FRAMEBUFFER_SRGB_CAPABLE_EXT   0x8DBA
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcGLES3, "qt.opengl.es3")

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

    void invalidateResource() Q_DECL_OVERRIDE
    {
        m_features = -1;
        m_extensions = -1;
    }

    void freeResource(QOpenGLContext *) Q_DECL_OVERRIDE
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
        qWarning() << "QOpenGLFunctions created with non-current context";
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
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBindTexture.xml}{glBindTexture()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glBlendFunc(GLenum sfactor, GLenum dfactor)

    Convenience function that calls glBlendFunc(\a sfactor, \a dfactor).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBlendFunc.xml}{glBlendFunc()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glClear(GLbitfield mask)

    Convenience function that calls glClear(\a mask).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glClear.xml}{glClear()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)

    Convenience function that calls glClearColor(\a red, \a green, \a blue, \a alpha).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glClearColor.xml}{glClearColor()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glClearStencil(GLint s)

    Convenience function that calls glClearStencil(\a s).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glClearStencil.xml}{glClearStencil()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)

    Convenience function that calls glColorMask(\a red, \a green, \a blue, \a alpha).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glColorMask.xml}{glColorMask()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)

    Convenience function that calls glCopyTexImage2D(\a target, \a level, \a internalformat, \a x, \a y, \a width, \a height, \a border).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glCopyTexImage2D.xml}{glCopyTexImage2D()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)

    Convenience function that calls glCopyTexSubImage2D(\a target, \a level, \a xoffset, \a yoffset, \a x, \a y, \a width, \a height).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glCopyTexSubImage2D.xml}{glCopyTexSubImage2D()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glCullFace(GLenum mode)

    Convenience function that calls glCullFace(\a mode).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glCullFace.xml}{glCullFace()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDeleteTextures(GLsizei n, const GLuint* textures)

    Convenience function that calls glDeleteTextures(\a n, \a textures).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDeleteTextures.xml}{glDeleteTextures()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDepthFunc(GLenum func)

    Convenience function that calls glDepthFunc(\a func).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDepthFunc.xml}{glDepthFunc()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDepthMask(GLboolean flag)

    Convenience function that calls glDepthMask(\a flag).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDepthMask.xml}{glDepthMask()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDisable(GLenum cap)

    Convenience function that calls glDisable(\a cap).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDisable.xml}{glDisable()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDrawArrays(GLenum mode, GLint first, GLsizei count)

    Convenience function that calls glDrawArrays(\a mode, \a first, \a count).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDrawArrays.xml}{glDrawArrays()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)

    Convenience function that calls glDrawElements(\a mode, \a count, \a type, \a indices).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDrawElements.xml}{glDrawElements()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glEnable(GLenum cap)

    Convenience function that calls glEnable(\a cap).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glEnable.xml}{glEnable()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glFinish()

    Convenience function that calls glFinish().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glFinish.xml}{glFinish()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glFlush()

    Convenience function that calls glFlush().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glFlush.xml}{glFlush()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glFrontFace(GLenum mode)

    Convenience function that calls glFrontFace(\a mode).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glFrontFace.xml}{glFrontFace()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGenTextures(GLsizei n, GLuint* textures)

    Convenience function that calls glGenTextures(\a n, \a textures).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGenTextures.xml}{glGenTextures()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGetBooleanv(GLenum pname, GLboolean* params)

    Convenience function that calls glGetBooleanv(\a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetBooleanv.xml}{glGetBooleanv()}.

    \since 5.3
*/

/*!
    \fn GLenum QOpenGLFunctions::glGetError()

    Convenience function that calls glGetError().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetError.xml}{glGetError()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGetFloatv(GLenum pname, GLfloat* params)

    Convenience function that calls glGetFloatv(\a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetFloatv.xml}{glGetFloatv()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGetIntegerv(GLenum pname, GLint* params)

    Convenience function that calls glGetIntegerv(\a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetIntegerv.xml}{glGetIntegerv()}.

    \since 5.3
*/

/*!
    \fn const GLubyte *QOpenGLFunctions::glGetString(GLenum name)

    Convenience function that calls glGetString(\a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetString.xml}{glGetString()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)

    Convenience function that calls glGetTexParameterfv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetTexParameterfv.xml}{glGetTexParameterfv()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetTexParameteriv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetTexParameteriv.xml}{glGetTexParameteriv()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glHint(GLenum target, GLenum mode)

    Convenience function that calls glHint(\a target, \a mode).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glHint.xml}{glHint()}.

    \since 5.3
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsEnabled(GLenum cap)

    Convenience function that calls glIsEnabled(\a cap).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glIsEnabled.xml}{glIsEnabled()}.

    \since 5.3
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsTexture(GLuint texture)

    Convenience function that calls glIsTexture(\a texture).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glIsTexture.xml}{glIsTexture()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glLineWidth(GLfloat width)

    Convenience function that calls glLineWidth(\a width).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glLineWidth.xml}{glLineWidth()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glPixelStorei(GLenum pname, GLint param)

    Convenience function that calls glPixelStorei(\a pname, \a param).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glPixelStorei.xml}{glPixelStorei()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glPolygonOffset(GLfloat factor, GLfloat units)

    Convenience function that calls glPolygonOffset(\a factor, \a units).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glPolygonOffset.xml}{glPolygonOffset()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)

    Convenience function that calls glReadPixels(\a x, \a y, \a width, \a height, \a format, \a type, \a pixels).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glReadPixels.xml}{glReadPixels()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glScissor(GLint x, GLint y, GLsizei width, GLsizei height)

    Convenience function that calls glScissor(\a x, \a y, \a width, \a height).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glScissor.xml}{glScissor()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glStencilFunc(GLenum func, GLint ref, GLuint mask)

    Convenience function that calls glStencilFunc(\a func, \a ref, \a mask).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glStencilFunc.xml}{glStencilFunc()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glStencilMask(GLuint mask)

    Convenience function that calls glStencilMask(\a mask).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glStencilMask.xml}{glStencilMask()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)

    Convenience function that calls glStencilOp(\a fail, \a zfail, \a zpass).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glStencilOp.xml}{glStencilOp()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)

    Convenience function that calls glTexImage2D(\a target, \a level, \a internalformat, \a width, \a height, \a border, \a format, \a type, \a pixels).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glTexImage2D.xml}{glTexImage2D()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexParameterf(GLenum target, GLenum pname, GLfloat param)

    Convenience function that calls glTexParameterf(\a target, \a pname, \a param).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glTexParameterf.xml}{glTexParameterf()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)

    Convenience function that calls glTexParameterfv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glTexParameterfv.xml}{glTexParameterfv()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexParameteri(GLenum target, GLenum pname, GLint param)

    Convenience function that calls glTexParameteri(\a target, \a pname, \a param).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glTexParameteri.xml}{glTexParameteri()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexParameteriv(GLenum target, GLenum pname, const GLint* params)

    Convenience function that calls glTexParameteriv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glTexParameteriv.xml}{glTexParameteriv()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)

    Convenience function that calls glTexSubImage2D(\a target, \a level, \a xoffset, \a yoffset, \a width, \a height, \a format, \a type, \a pixels).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glTexSubImage2D.xml}{glTexSubImage2D()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glViewport(GLint x, GLint y, GLsizei width, GLsizei height)

    Convenience function that calls glViewport(\a x, \a y, \a width, \a height).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glViewport.xml}{glViewport()}.

    \since 5.3
*/

/*!
    \fn void QOpenGLFunctions::glActiveTexture(GLenum texture)

    Convenience function that calls glActiveTexture(\a texture).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glActiveTexture.xml}{glActiveTexture()}.
*/

/*!
    \fn void QOpenGLFunctions::glAttachShader(GLuint program, GLuint shader)

    Convenience function that calls glAttachShader(\a program, \a shader).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glAttachShader.xml}{glAttachShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glBindAttribLocation(GLuint program, GLuint index, const char* name)

    Convenience function that calls glBindAttribLocation(\a program, \a index, \a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBindAttribLocation.xml}{glBindAttribLocation()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glBindBuffer(GLenum target, GLuint buffer)

    Convenience function that calls glBindBuffer(\a target, \a buffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBindBuffer.xml}{glBindBuffer()}.
*/

/*!
    \fn void QOpenGLFunctions::glBindFramebuffer(GLenum target, GLuint framebuffer)

    Convenience function that calls glBindFramebuffer(\a target, \a framebuffer).

    Note that Qt will translate a \a framebuffer argument of 0 to the currently
    bound QOpenGLContext's defaultFramebufferObject().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBindFramebuffer.xml}{glBindFramebuffer()}.
*/

/*!
    \fn void QOpenGLFunctions::glBindRenderbuffer(GLenum target, GLuint renderbuffer)

    Convenience function that calls glBindRenderbuffer(\a target, \a renderbuffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBindRenderbuffer.xml}{glBindRenderbuffer()}.
*/

/*!
    \fn void QOpenGLFunctions::glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)

    Convenience function that calls glBlendColor(\a red, \a green, \a blue, \a alpha).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBlendColor.xml}{glBlendColor()}.
*/

/*!
    \fn void QOpenGLFunctions::glBlendEquation(GLenum mode)

    Convenience function that calls glBlendEquation(\a mode).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBlendEquation.xml}{glBlendEquation()}.
*/

/*!
    \fn void QOpenGLFunctions::glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)

    Convenience function that calls glBlendEquationSeparate(\a modeRGB, \a modeAlpha).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBlendEquationSeparate.xml}{glBlendEquationSeparate()}.
*/

/*!
    \fn void QOpenGLFunctions::glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)

    Convenience function that calls glBlendFuncSeparate(\a srcRGB, \a dstRGB, \a srcAlpha, \a dstAlpha).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBlendFuncSeparate.xml}{glBlendFuncSeparate()}.
*/

/*!
    \fn void QOpenGLFunctions::glBufferData(GLenum target, qopengl_GLsizeiptr size, const void* data, GLenum usage)

    Convenience function that calls glBufferData(\a target, \a size, \a data, \a usage).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBufferData.xml}{glBufferData()}.
*/

/*!
    \fn void QOpenGLFunctions::glBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, const void* data)

    Convenience function that calls glBufferSubData(\a target, \a offset, \a size, \a data).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glBufferSubData.xml}{glBufferSubData()}.
*/

/*!
    \fn GLenum QOpenGLFunctions::glCheckFramebufferStatus(GLenum target)

    Convenience function that calls glCheckFramebufferStatus(\a target).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glCheckFramebufferStatus.xml}{glCheckFramebufferStatus()}.
*/

/*!
    \fn void QOpenGLFunctions::glClearDepthf(GLclampf depth)

    Convenience function that calls glClearDepth(\a depth) on
    desktop OpenGL systems and glClearDepthf(\a depth) on
    embedded OpenGL ES systems.

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glClearDepthf.xml}{glClearDepthf()}.
*/

/*!
    \fn void QOpenGLFunctions::glCompileShader(GLuint shader)

    Convenience function that calls glCompileShader(\a shader).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glCompileShader.xml}{glCompileShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)

    Convenience function that calls glCompressedTexImage2D(\a target, \a level, \a internalformat, \a width, \a height, \a border, \a imageSize, \a data).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glCompressedTexImage2D.xml}{glCompressedTexImage2D()}.
*/

/*!
    \fn void QOpenGLFunctions::glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)

    Convenience function that calls glCompressedTexSubImage2D(\a target, \a level, \a xoffset, \a yoffset, \a width, \a height, \a format, \a imageSize, \a data).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glCompressedTexSubImage2D.xml}{glCompressedTexSubImage2D()}.
*/

/*!
    \fn GLuint QOpenGLFunctions::glCreateProgram()

    Convenience function that calls glCreateProgram().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glCreateProgram.xml}{glCreateProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn GLuint QOpenGLFunctions::glCreateShader(GLenum type)

    Convenience function that calls glCreateShader(\a type).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glCreateShader.xml}{glCreateShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glDeleteBuffers(GLsizei n, const GLuint* buffers)

    Convenience function that calls glDeleteBuffers(\a n, \a buffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDeleteBuffers.xml}{glDeleteBuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)

    Convenience function that calls glDeleteFramebuffers(\a n, \a framebuffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDeleteFramebuffers.xml}{glDeleteFramebuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glDeleteProgram(GLuint program)

    Convenience function that calls glDeleteProgram(\a program).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDeleteProgram.xml}{glDeleteProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)

    Convenience function that calls glDeleteRenderbuffers(\a n, \a renderbuffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDeleteRenderbuffers.xml}{glDeleteRenderbuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glDeleteShader(GLuint shader)

    Convenience function that calls glDeleteShader(\a shader).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDeleteShader.xml}{glDeleteShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glDepthRangef(GLclampf zNear, GLclampf zFar)

    Convenience function that calls glDepthRange(\a zNear, \a zFar) on
    desktop OpenGL systems and glDepthRangef(\a zNear, \a zFar) on
    embedded OpenGL ES systems.

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDepthRangef.xml}{glDepthRangef()}.
*/

/*!
    \fn void QOpenGLFunctions::glDetachShader(GLuint program, GLuint shader)

    Convenience function that calls glDetachShader(\a program, \a shader).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDetachShader.xml}{glDetachShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glDisableVertexAttribArray(GLuint index)

    Convenience function that calls glDisableVertexAttribArray(\a index).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glDisableVertexAttribArray.xml}{glDisableVertexAttribArray()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glEnableVertexAttribArray(GLuint index)

    Convenience function that calls glEnableVertexAttribArray(\a index).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glEnableVertexAttribArray.xml}{glEnableVertexAttribArray()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)

    Convenience function that calls glFramebufferRenderbuffer(\a target, \a attachment, \a renderbuffertarget, \a renderbuffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glFramebufferRenderbuffer.xml}{glFramebufferRenderbuffer()}.
*/

/*!
    \fn void QOpenGLFunctions::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)

    Convenience function that calls glFramebufferTexture2D(\a target, \a attachment, \a textarget, \a texture, \a level).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glFramebufferTexture2D.xml}{glFramebufferTexture2D()}.
*/

/*!
    \fn void QOpenGLFunctions::glGenBuffers(GLsizei n, GLuint* buffers)

    Convenience function that calls glGenBuffers(\a n, \a buffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGenBuffers.xml}{glGenBuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glGenerateMipmap(GLenum target)

    Convenience function that calls glGenerateMipmap(\a target).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGenerateMipmap.xml}{glGenerateMipmap()}.
*/

/*!
    \fn void QOpenGLFunctions::glGenFramebuffers(GLsizei n, GLuint* framebuffers)

    Convenience function that calls glGenFramebuffers(\a n, \a framebuffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGenFramebuffers.xml}{glGenFramebuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)

    Convenience function that calls glGenRenderbuffers(\a n, \a renderbuffers).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGenRenderbuffers.xml}{glGenRenderbuffers()}.
*/

/*!
    \fn void QOpenGLFunctions::glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)

    Convenience function that calls glGetActiveAttrib(\a program, \a index, \a bufsize, \a length, \a size, \a type, \a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetActiveAttrib.xml}{glGetActiveAttrib()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)

    Convenience function that calls glGetActiveUniform(\a program, \a index, \a bufsize, \a length, \a size, \a type, \a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetActiveUniform.xml}{glGetActiveUniform()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)

    Convenience function that calls glGetAttachedShaders(\a program, \a maxcount, \a count, \a shaders).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetAttachedShaders.xml}{glGetAttachedShaders()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn GLint QOpenGLFunctions::glGetAttribLocation(GLuint program, const char* name)

    Convenience function that calls glGetAttribLocation(\a program, \a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetAttribLocation.xml}{glGetAttribLocation()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetBufferParameteriv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetBufferParameteriv.xml}{glGetBufferParameteriv()}.
*/

/*!
    \fn void QOpenGLFunctions::glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)

    Convenience function that calls glGetFramebufferAttachmentParameteriv(\a target, \a attachment, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetFramebufferAttachmentParameteriv.xml}{glGetFramebufferAttachmentParameteriv()}.
*/

/*!
    \fn void QOpenGLFunctions::glGetProgramiv(GLuint program, GLenum pname, GLint* params)

    Convenience function that calls glGetProgramiv(\a program, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetProgramiv.xml}{glGetProgramiv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog)

    Convenience function that calls glGetProgramInfoLog(\a program, \a bufsize, \a length, \a infolog).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetProgramInfoLog.xml}{glGetProgramInfoLog()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetRenderbufferParameteriv(\a target, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetRenderbufferParameteriv.xml}{glGetRenderbufferParameteriv()}.
*/

/*!
    \fn void QOpenGLFunctions::glGetShaderiv(GLuint shader, GLenum pname, GLint* params)

    Convenience function that calls glGetShaderiv(\a shader, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetShaderiv.xml}{glGetShaderiv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog)

    Convenience function that calls glGetShaderInfoLog(\a shader, \a bufsize, \a length, \a infolog).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetShaderInfoLog.xml}{glGetShaderInfoLog()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)

    Convenience function that calls glGetShaderPrecisionFormat(\a shadertype, \a precisiontype, \a range, \a precision).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetShaderPrecisionFormat.xml}{glGetShaderPrecisionFormat()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source)

    Convenience function that calls glGetShaderSource(\a shader, \a bufsize, \a length, \a source).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetShaderSource.xml}{glGetShaderSource()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetUniformfv(GLuint program, GLint location, GLfloat* params)

    Convenience function that calls glGetUniformfv(\a program, \a location, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetUniformfv.xml}{glGetUniformfv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetUniformiv(GLuint program, GLint location, GLint* params)

    Convenience function that calls glGetUniformiv(\a program, \a location, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetUniformiv.xml}{glGetUniformiv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn GLint QOpenGLFunctions::glGetUniformLocation(GLuint program, const char* name)

    Convenience function that calls glGetUniformLocation(\a program, \a name).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetUniformLocation.xml}{glGetUniformLocation()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)

    Convenience function that calls glGetVertexAttribfv(\a index, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetVertexAttribfv.xml}{glGetVertexAttribfv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)

    Convenience function that calls glGetVertexAttribiv(\a index, \a pname, \a params).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetVertexAttribiv.xml}{glGetVertexAttribiv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer)

    Convenience function that calls glGetVertexAttribPointerv(\a index, \a pname, \a pointer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glGetVertexAttribPointerv.xml}{glGetVertexAttribPointerv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsBuffer(GLuint buffer)

    Convenience function that calls glIsBuffer(\a buffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glIsBuffer.xml}{glIsBuffer()}.
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsFramebuffer(GLuint framebuffer)

    Convenience function that calls glIsFramebuffer(\a framebuffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glIsFramebuffer.xml}{glIsFramebuffer()}.
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsProgram(GLuint program)

    Convenience function that calls glIsProgram(\a program).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glIsProgram.xml}{glIsProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsRenderbuffer(GLuint renderbuffer)

    Convenience function that calls glIsRenderbuffer(\a renderbuffer).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glIsRenderbuffer.xml}{glIsRenderbuffer()}.
*/

/*!
    \fn GLboolean QOpenGLFunctions::glIsShader(GLuint shader)

    Convenience function that calls glIsShader(\a shader).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glIsShader.xml}{glIsShader()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glLinkProgram(GLuint program)

    Convenience function that calls glLinkProgram(\a program).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glLinkProgram.xml}{glLinkProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glReleaseShaderCompiler()

    Convenience function that calls glReleaseShaderCompiler().

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glReleaseShaderCompiler.xml}{glReleaseShaderCompiler()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)

    Convenience function that calls glRenderbufferStorage(\a target, \a internalformat, \a width, \a height).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glRenderbufferStorage.xml}{glRenderbufferStorage()}.
*/

/*!
    \fn void QOpenGLFunctions::glSampleCoverage(GLclampf value, GLboolean invert)

    Convenience function that calls glSampleCoverage(\a value, \a invert).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glSampleCoverage.xml}{glSampleCoverage()}.
*/

/*!
    \fn void QOpenGLFunctions::glShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length)

    Convenience function that calls glShaderBinary(\a n, \a shaders, \a binaryformat, \a binary, \a length).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glShaderBinary.xml}{glShaderBinary()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length)

    Convenience function that calls glShaderSource(\a shader, \a count, \a string, \a length).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glShaderSource.xml}{glShaderSource()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)

    Convenience function that calls glStencilFuncSeparate(\a face, \a func, \a ref, \a mask).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glStencilFuncSeparate.xml}{glStencilFuncSeparate()}.
*/

/*!
    \fn void QOpenGLFunctions::glStencilMaskSeparate(GLenum face, GLuint mask)

    Convenience function that calls glStencilMaskSeparate(\a face, \a mask).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glStencilMaskSeparate.xml}{glStencilMaskSeparate()}.
*/

/*!
    \fn void QOpenGLFunctions::glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)

    Convenience function that calls glStencilOpSeparate(\a face, \a fail, \a zfail, \a zpass).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glStencilOpSeparate.xml}{glStencilOpSeparate()}.
*/

/*!
    \fn void QOpenGLFunctions::glUniform1f(GLint location, GLfloat x)

    Convenience function that calls glUniform1f(\a location, \a x).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform1f.xml}{glUniform1f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform1fv(GLint location, GLsizei count, const GLfloat* v)

    Convenience function that calls glUniform1fv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform1fv.xml}{glUniform1fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform1i(GLint location, GLint x)

    Convenience function that calls glUniform1i(\a location, \a x).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform1i.xml}{glUniform1i()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform1iv(GLint location, GLsizei count, const GLint* v)

    Convenience function that calls glUniform1iv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform1iv.xml}{glUniform1iv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform2f(GLint location, GLfloat x, GLfloat y)

    Convenience function that calls glUniform2f(\a location, \a x, \a y).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform2f.xml}{glUniform2f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform2fv(GLint location, GLsizei count, const GLfloat* v)

    Convenience function that calls glUniform2fv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform2fv.xml}{glUniform2fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform2i(GLint location, GLint x, GLint y)

    Convenience function that calls glUniform2i(\a location, \a x, \a y).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform2i.xml}{glUniform2i()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform2iv(GLint location, GLsizei count, const GLint* v)

    Convenience function that calls glUniform2iv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform2iv.xml}{glUniform2iv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)

    Convenience function that calls glUniform3f(\a location, \a x, \a y, \a z).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform3f.xml}{glUniform3f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform3fv(GLint location, GLsizei count, const GLfloat* v)

    Convenience function that calls glUniform3fv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform3fv.xml}{glUniform3fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform3i(GLint location, GLint x, GLint y, GLint z)

    Convenience function that calls glUniform3i(\a location, \a x, \a y, \a z).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform3i.xml}{glUniform3i()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform3iv(GLint location, GLsizei count, const GLint* v)

    Convenience function that calls glUniform3iv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform3iv.xml}{glUniform3iv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)

    Convenience function that calls glUniform4f(\a location, \a x, \a y, \a z, \a w).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform4f.xml}{glUniform4f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform4fv(GLint location, GLsizei count, const GLfloat* v)

    Convenience function that calls glUniform4fv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform4fv.xml}{glUniform4fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)

    Convenience function that calls glUniform4i(\a location, \a x, \a y, \a z, \a w).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform4i.xml}{glUniform4i()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniform4iv(GLint location, GLsizei count, const GLint* v)

    Convenience function that calls glUniform4iv(\a location, \a count, \a v).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniform4iv.xml}{glUniform4iv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)

    Convenience function that calls glUniformMatrix2fv(\a location, \a count, \a transpose, \a value).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniformMatrix2fv.xml}{glUniformMatrix2fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)

    Convenience function that calls glUniformMatrix3fv(\a location, \a count, \a transpose, \a value).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniformMatrix3fv.xml}{glUniformMatrix3fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)

    Convenience function that calls glUniformMatrix4fv(\a location, \a count, \a transpose, \a value).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUniformMatrix4fv.xml}{glUniformMatrix4fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glUseProgram(GLuint program)

    Convenience function that calls glUseProgram(\a program).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glUseProgram.xml}{glUseProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glValidateProgram(GLuint program)

    Convenience function that calls glValidateProgram(\a program).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glValidateProgram.xml}{glValidateProgram()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib1f(GLuint indx, GLfloat x)

    Convenience function that calls glVertexAttrib1f(\a indx, \a x).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glVertexAttrib1f.xml}{glVertexAttrib1f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib1fv(GLuint indx, const GLfloat* values)

    Convenience function that calls glVertexAttrib1fv(\a indx, \a values).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glVertexAttrib1fv.xml}{glVertexAttrib1fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)

    Convenience function that calls glVertexAttrib2f(\a indx, \a x, \a y).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glVertexAttrib2f.xml}{glVertexAttrib2f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib2fv(GLuint indx, const GLfloat* values)

    Convenience function that calls glVertexAttrib2fv(\a indx, \a values).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glVertexAttrib2fv.xml}{glVertexAttrib2fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)

    Convenience function that calls glVertexAttrib3f(\a indx, \a x, \a y, \a z).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glVertexAttrib3f.xml}{glVertexAttrib3f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib3fv(GLuint indx, const GLfloat* values)

    Convenience function that calls glVertexAttrib3fv(\a indx, \a values).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glVertexAttrib3fv.xml}{glVertexAttrib3fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)

    Convenience function that calls glVertexAttrib4f(\a indx, \a x, \a y, \a z, \a w).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glVertexAttrib4f.xml}{glVertexAttrib4f()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttrib4fv(GLuint indx, const GLfloat* values)

    Convenience function that calls glVertexAttrib4fv(\a indx, \a values).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glVertexAttrib4fv.xml}{glVertexAttrib4fv()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn void QOpenGLFunctions::glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)

    Convenience function that calls glVertexAttribPointer(\a indx, \a size, \a type, \a normalized, \a stride, \a ptr).

    For more information, see the OpenGL ES 2.0 documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man/glVertexAttribPointer.xml}{glVertexAttribPointer()}.

    This convenience function will do nothing on OpenGL ES 1.x systems.
*/

/*!
    \fn bool QOpenGLFunctions::isInitialized(const QOpenGLFunctionsPrivate *d)
    \internal
*/

namespace {

enum ResolvePolicy
{
    ResolveOES = 0x1,
    ResolveEXT = 0x2,
    ResolveANGLE = 0x4,
    ResolveNV = 0x8
};

template <typename Base, typename FuncType, int Policy, typename ReturnType>
class Resolver
{
public:
    Resolver(FuncType Base::*func, FuncType fallback, const char *name, const char *alternateName = 0)
        : funcPointerName(func)
        , fallbackFuncPointer(fallback)
        , funcName(name)
        , alternateFuncName(alternateName)
    {
    }

    ReturnType operator()();

    template <typename P1>
    ReturnType operator()(P1 p1);

    template <typename P1, typename P2>
    ReturnType operator()(P1 p1, P2 p2);

    template <typename P1, typename P2, typename P3>
    ReturnType operator()(P1 p1, P2 p2, P3 p3);

    template <typename P1, typename P2, typename P3, typename P4>
    ReturnType operator()(P1 p1, P2 p2, P3 p3, P4 p4);

    template <typename P1, typename P2, typename P3, typename P4, typename P5>
    ReturnType operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
    ReturnType operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
    ReturnType operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
    ReturnType operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
    ReturnType operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10>
    ReturnType operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11>
    ReturnType operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11);

private:
    FuncType Base::*funcPointerName;
    FuncType fallbackFuncPointer;
    QByteArray funcName;
    QByteArray alternateFuncName;
};

template <typename Base, typename FuncType, int Policy>
class Resolver<Base, FuncType, Policy, void>
{
public:
    Resolver(FuncType Base::*func, FuncType fallback, const char *name, const char *alternateName = 0)
        : funcPointerName(func)
        , fallbackFuncPointer(fallback)
        , funcName(name)
        , alternateFuncName(alternateName)
    {
    }

    void operator()();

    template <typename P1>
    void operator()(P1 p1);

    template <typename P1, typename P2>
    void operator()(P1 p1, P2 p2);

    template <typename P1, typename P2, typename P3>
    void operator()(P1 p1, P2 p2, P3 p3);

    template <typename P1, typename P2, typename P3, typename P4>
    void operator()(P1 p1, P2 p2, P3 p3, P4 p4);

    template <typename P1, typename P2, typename P3, typename P4, typename P5>
    void operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
    void operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
    void operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
    void operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
    void operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10>
    void operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10);

    template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11>
    void operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11);

private:
    FuncType Base::*funcPointerName;
    FuncType fallbackFuncPointer;
    QByteArray funcName;
    QByteArray alternateFuncName;
};

#define RESOLVER_COMMON \
    QOpenGLContext *context = QOpenGLContext::currentContext(); \
    Base *funcs = qt_gl_functions(context); \
 \
    FuncType old = funcs->*funcPointerName; \
 \
    funcs->*funcPointerName = (FuncType)context->getProcAddress(funcName); \
 \
    if ((Policy & ResolveOES) && !(funcs->*funcPointerName)) \
        funcs->*funcPointerName = (FuncType)context->getProcAddress(funcName + "OES"); \
 \
    if (!(funcs->*funcPointerName)) \
        funcs->*funcPointerName = (FuncType)context->getProcAddress(funcName + "ARB"); \
 \
    if ((Policy & ResolveEXT) && !(funcs->*funcPointerName)) \
        funcs->*funcPointerName = (FuncType)context->getProcAddress(funcName + "EXT"); \
 \
    if ((Policy & ResolveANGLE) && !(funcs->*funcPointerName)) \
        funcs->*funcPointerName = (FuncType)context->getProcAddress(funcName + "ANGLE"); \
 \
    if ((Policy & ResolveNV) && !(funcs->*funcPointerName)) \
        funcs->*funcPointerName = (FuncType)context->getProcAddress(funcName + "NV"); \
 \
    if (!alternateFuncName.isEmpty() && !(funcs->*funcPointerName)) { \
        funcs->*funcPointerName = (FuncType)context->getProcAddress(alternateFuncName); \
 \
        if ((Policy & ResolveOES) && !(funcs->*funcPointerName)) \
            funcs->*funcPointerName = (FuncType)context->getProcAddress(alternateFuncName + "OES"); \
 \
        if (!(funcs->*funcPointerName)) \
            funcs->*funcPointerName = (FuncType)context->getProcAddress(alternateFuncName + "ARB"); \
 \
        if ((Policy & ResolveEXT) && !(funcs->*funcPointerName)) \
            funcs->*funcPointerName = (FuncType)context->getProcAddress(alternateFuncName + "EXT"); \
 \
        if ((Policy & ResolveANGLE) && !(funcs->*funcPointerName)) \
            funcs->*funcPointerName = (FuncType)context->getProcAddress(funcName + "ANGLE"); \
 \
        if ((Policy & ResolveNV) && !(funcs->*funcPointerName)) \
            funcs->*funcPointerName = (FuncType)context->getProcAddress(funcName + "NV"); \
    }

#define RESOLVER_COMMON_NON_VOID \
    RESOLVER_COMMON \
 \
    if (!(funcs->*funcPointerName)) { \
        if (fallbackFuncPointer) { \
            funcs->*funcPointerName = fallbackFuncPointer; \
        } else { \
            funcs->*funcPointerName = old; \
            return ReturnType(); \
        } \
    }

#define RESOLVER_COMMON_VOID \
    RESOLVER_COMMON \
 \
    if (!(funcs->*funcPointerName)) { \
        if (fallbackFuncPointer) { \
            funcs->*funcPointerName = fallbackFuncPointer; \
        } else { \
            funcs->*funcPointerName = old; \
            return; \
        } \
    }

template <typename Base, typename FuncType, int Policy, typename ReturnType>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()()
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)();
}

template <typename Base, typename FuncType, int Policy, typename ReturnType> template <typename P1>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()(P1 p1)
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)(p1);
}

template <typename Base, typename FuncType, int Policy, typename ReturnType> template <typename P1, typename P2>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()(P1 p1, P2 p2)
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)(p1, p2);
}

template <typename Base, typename FuncType, int Policy, typename ReturnType> template <typename P1, typename P2, typename P3>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()(P1 p1, P2 p2, P3 p3)
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)(p1, p2, p3);
}

template <typename Base, typename FuncType, int Policy, typename ReturnType> template <typename P1, typename P2, typename P3, typename P4>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()(P1 p1, P2 p2, P3 p3, P4 p4)
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)(p1, p2, p3, p4);
}

template <typename Base, typename FuncType, int Policy, typename ReturnType> template <typename P1, typename P2, typename P3, typename P4, typename P5>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5)
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)(p1, p2, p3, p4, p5);
}

template <typename Base, typename FuncType, int Policy, typename ReturnType> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6)
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6);
}

template <typename Base, typename FuncType, int Policy, typename ReturnType> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7)
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6, p7);
}

template <typename Base, typename FuncType, int Policy, typename ReturnType> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8)
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6, p7, p8);
}

template <typename Base, typename FuncType, int Policy, typename ReturnType> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9)
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6, p7, p8, p9);
}

template <typename Base, typename FuncType, int Policy, typename ReturnType> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10>
ReturnType Resolver<Base, FuncType, Policy, ReturnType>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10)
{
    RESOLVER_COMMON_NON_VOID

    return (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
}

template <typename Base, typename FuncType, int Policy>
void Resolver<Base, FuncType, Policy, void>::operator()()
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)();
}

template <typename Base, typename FuncType, int Policy> template <typename P1>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1);
}

template <typename Base, typename FuncType, int Policy> template <typename P1, typename P2>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1, P2 p2)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1, p2);
}

template <typename Base, typename FuncType, int Policy> template <typename P1, typename P2, typename P3>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1, P2 p2, P3 p3)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1, p2, p3);
}

template <typename Base, typename FuncType, int Policy> template <typename P1, typename P2, typename P3, typename P4>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1, P2 p2, P3 p3, P4 p4)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1, p2, p3, p4);
}

template <typename Base, typename FuncType, int Policy> template <typename P1, typename P2, typename P3, typename P4, typename P5>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1, p2, p3, p4, p5);
}

template <typename Base, typename FuncType, int Policy> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6);
}

template <typename Base, typename FuncType, int Policy> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6, p7);
}

template <typename Base, typename FuncType, int Policy> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6, p7, p8);
}

template <typename Base, typename FuncType, int Policy> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6, p7, p8, p9);
}

template <typename Base, typename FuncType, int Policy> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
}

template <typename Base, typename FuncType, int Policy> template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11>
void Resolver<Base, FuncType, Policy, void>::operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11)
{
    RESOLVER_COMMON_VOID

    (funcs->*funcPointerName)(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
}

template <typename ReturnType, int Policy, typename Base, typename FuncType>
Resolver<Base, FuncType, Policy, ReturnType> functionResolverWithFallback(FuncType Base::*func, FuncType fallback, const char *name, const char *alternate = 0)
{
    return Resolver<Base, FuncType, Policy, ReturnType>(func, fallback, name, alternate);
}

template <typename ReturnType, int Policy, typename Base, typename FuncType>
Resolver<Base, FuncType, Policy, ReturnType> functionResolver(FuncType Base::*func, const char *name, const char *alternate = 0)
{
    return Resolver<Base, FuncType, Policy, ReturnType>(func, 0, name, alternate);
}

} // namespace

#define RESOLVE_FUNC(RETURN_TYPE, POLICY, NAME) \
    return functionResolver<RETURN_TYPE, POLICY>(&QOpenGLExtensionsPrivate::NAME, "gl" #NAME)

#define RESOLVE_FUNC_VOID(POLICY, NAME) \
    functionResolver<void, POLICY>(&QOpenGLExtensionsPrivate::NAME, "gl" #NAME)

#define RESOLVE_FUNC_SPECIAL(RETURN_TYPE, POLICY, NAME) \
    return functionResolverWithFallback<RETURN_TYPE, POLICY>(&QOpenGLExtensionsPrivate::NAME, qopenglfSpecial##NAME, "gl" #NAME)

#define RESOLVE_FUNC_SPECIAL_VOID(POLICY, NAME) \
    functionResolverWithFallback<void, POLICY>(&QOpenGLExtensionsPrivate::NAME, qopenglfSpecial##NAME, "gl" #NAME)

#define RESOLVE_FUNC_WITH_ALTERNATE(RETURN_TYPE, POLICY, NAME, ALTERNATE) \
    return functionResolver<RETURN_TYPE, POLICY>(&QOpenGLExtensionsPrivate::NAME, "gl" #NAME, "gl" #ALTERNATE)

#define RESOLVE_FUNC_VOID_WITH_ALTERNATE(POLICY, NAME, ALTERNATE) \
    functionResolver<void, POLICY>(&QOpenGLExtensionsPrivate::NAME, "gl" #NAME, "gl" #ALTERNATE)

#ifndef QT_OPENGL_ES_2

// GLES2 + OpenGL1 common subset. These are normally not resolvable,
// but the underlying platform code may hide this limitation.

static void QOPENGLF_APIENTRY qopenglfResolveBindTexture(GLenum target, GLuint texture)
{
    RESOLVE_FUNC_VOID(0, BindTexture)(target, texture);
}

static void QOPENGLF_APIENTRY qopenglfResolveBlendFunc(GLenum sfactor, GLenum dfactor)
{
    RESOLVE_FUNC_VOID(0, BlendFunc)(sfactor, dfactor);
}

static void QOPENGLF_APIENTRY qopenglfResolveClear(GLbitfield mask)
{
    RESOLVE_FUNC_VOID(0, Clear)(mask);
}

static void QOPENGLF_APIENTRY qopenglfResolveClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    RESOLVE_FUNC_VOID(0, ClearColor)(red, green, blue, alpha);
}

static void QOPENGLF_APIENTRY qopenglfResolveClearDepthf(GLclampf depth)
{
    if (QOpenGLContext::currentContext()->isOpenGLES()) {
        RESOLVE_FUNC_VOID(0, ClearDepthf)(depth);
    } else {
        RESOLVE_FUNC_VOID(0, ClearDepth)((GLdouble) depth);
    }
}

static void QOPENGLF_APIENTRY qopenglfResolveClearStencil(GLint s)
{
    RESOLVE_FUNC_VOID(0, ClearStencil)(s);
}

static void QOPENGLF_APIENTRY qopenglfResolveColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    RESOLVE_FUNC_VOID(0, ColorMask)(red, green, blue, alpha);
}

static void QOPENGLF_APIENTRY qopenglfResolveCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    RESOLVE_FUNC_VOID(0, CopyTexImage2D)(target, level, internalformat, x, y, width, height, border);
}

static void QOPENGLF_APIENTRY qopenglfResolveCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    RESOLVE_FUNC_VOID(0, CopyTexSubImage2D)(target, level, xoffset, yoffset, x, y, width, height);
}

static void QOPENGLF_APIENTRY qopenglfResolveCullFace(GLenum mode)
{
    RESOLVE_FUNC_VOID(0, CullFace)(mode);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteTextures(GLsizei n, const GLuint* textures)
{
    RESOLVE_FUNC_VOID(0, DeleteTextures)(n, textures);
}

static void QOPENGLF_APIENTRY qopenglfResolveDepthFunc(GLenum func)
{
    RESOLVE_FUNC_VOID(0, DepthFunc)(func);
}

static void QOPENGLF_APIENTRY qopenglfResolveDepthMask(GLboolean flag)
{
    RESOLVE_FUNC_VOID(0, DepthMask)(flag);
}

static void QOPENGLF_APIENTRY qopenglfResolveDepthRangef(GLclampf zNear, GLclampf zFar)
{
    if (QOpenGLContext::currentContext()->isOpenGLES()) {
        RESOLVE_FUNC_VOID(0, DepthRangef)(zNear, zFar);
    } else {
        RESOLVE_FUNC_VOID(0, DepthRange)((GLdouble) zNear, (GLdouble) zFar);
    }
}

static void QOPENGLF_APIENTRY qopenglfResolveDisable(GLenum cap)
{
    RESOLVE_FUNC_VOID(0, Disable)(cap);
}

static void QOPENGLF_APIENTRY qopenglfResolveDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    RESOLVE_FUNC_VOID(0, DrawArrays)(mode, first, count);
}

static void QOPENGLF_APIENTRY qopenglfResolveDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
    RESOLVE_FUNC_VOID(0, DrawElements)(mode, count, type, indices);
}

static void QOPENGLF_APIENTRY qopenglfResolveEnable(GLenum cap)
{
    RESOLVE_FUNC_VOID(0, Enable)(cap);
}

static void QOPENGLF_APIENTRY qopenglfResolveFinish()
{
    RESOLVE_FUNC_VOID(0, Finish)();
}

static void QOPENGLF_APIENTRY qopenglfResolveFlush()
{
    RESOLVE_FUNC_VOID(0, Flush)();
}

static void QOPENGLF_APIENTRY qopenglfResolveFrontFace(GLenum mode)
{
    RESOLVE_FUNC_VOID(0, FrontFace)(mode);
}

static void QOPENGLF_APIENTRY qopenglfResolveGenTextures(GLsizei n, GLuint* textures)
{
    RESOLVE_FUNC_VOID(0, GenTextures)(n, textures);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetBooleanv(GLenum pname, GLboolean* params)
{
    RESOLVE_FUNC_VOID(0, GetBooleanv)(pname, params);
}

static GLenum QOPENGLF_APIENTRY qopenglfResolveGetError()
{
    RESOLVE_FUNC(GLenum, 0, GetError)();
}

static void QOPENGLF_APIENTRY qopenglfResolveGetFloatv(GLenum pname, GLfloat* params)
{
    RESOLVE_FUNC_VOID(0, GetFloatv)(pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetIntegerv(GLenum pname, GLint* params)
{
    RESOLVE_FUNC_VOID(0, GetIntegerv)(pname, params);
}

static const GLubyte * QOPENGLF_APIENTRY qopenglfResolveGetString(GLenum name)
{
    RESOLVE_FUNC(const GLubyte *, 0, GetString)(name);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
    RESOLVE_FUNC_VOID(0, GetTexParameterfv)(target, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
    RESOLVE_FUNC_VOID(0, GetTexParameteriv)(target, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveHint(GLenum target, GLenum mode)
{
    RESOLVE_FUNC_VOID(0, Hint)(target, mode);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsEnabled(GLenum cap)
{
    RESOLVE_FUNC(GLboolean, 0, IsEnabled)(cap);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsTexture(GLuint texture)
{
    RESOLVE_FUNC(GLboolean, 0, IsTexture)(texture);
}

static void QOPENGLF_APIENTRY qopenglfResolveLineWidth(GLfloat width)
{
    RESOLVE_FUNC_VOID(0, LineWidth)(width);
}

static void QOPENGLF_APIENTRY qopenglfResolvePixelStorei(GLenum pname, GLint param)
{
    RESOLVE_FUNC_VOID(0, PixelStorei)(pname, param);
}

static void QOPENGLF_APIENTRY qopenglfResolvePolygonOffset(GLfloat factor, GLfloat units)
{
    RESOLVE_FUNC_VOID(0, PolygonOffset)(factor, units);
}

static void QOPENGLF_APIENTRY qopenglfResolveReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
    RESOLVE_FUNC_VOID(0, ReadPixels)(x, y, width, height, format, type, pixels);
}

static void QOPENGLF_APIENTRY qopenglfResolveScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    RESOLVE_FUNC_VOID(0, Scissor)(x, y, width, height);
}

static void QOPENGLF_APIENTRY qopenglfResolveStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    RESOLVE_FUNC_VOID(0, StencilFunc)(func, ref, mask);
}

static void QOPENGLF_APIENTRY qopenglfResolveStencilMask(GLuint mask)
{
    RESOLVE_FUNC_VOID(0, StencilMask)(mask);
}

static void QOPENGLF_APIENTRY qopenglfResolveStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    RESOLVE_FUNC_VOID(0, StencilOp)(fail, zfail, zpass);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
    RESOLVE_FUNC_VOID(0, TexImage2D)(target, level, internalformat, width, height, border, format, type, pixels);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    RESOLVE_FUNC_VOID(0, TexParameterf)(target, pname, param);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
    RESOLVE_FUNC_VOID(0, TexParameterfv)(target, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexParameteri(GLenum target, GLenum pname, GLint param)
{
    RESOLVE_FUNC_VOID(0, TexParameteri)(target, pname, param);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
    RESOLVE_FUNC_VOID(0, TexParameteriv)(target, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
    RESOLVE_FUNC_VOID(0, TexSubImage2D)(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

static void QOPENGLF_APIENTRY qopenglfResolveViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    RESOLVE_FUNC_VOID(0, Viewport)(x, y, width, height);
}

// GL(ES)2

static void QOPENGLF_APIENTRY qopenglfResolveActiveTexture(GLenum texture)
{
    RESOLVE_FUNC_VOID(0, ActiveTexture)(texture);
}

static void QOPENGLF_APIENTRY qopenglfResolveAttachShader(GLuint program, GLuint shader)
{
    RESOLVE_FUNC_VOID_WITH_ALTERNATE(0, AttachShader, AttachObject)(program, shader);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindAttribLocation(GLuint program, GLuint index, const char* name)
{
    RESOLVE_FUNC_VOID(0, BindAttribLocation)(program, index, name);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindBuffer(GLenum target, GLuint buffer)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, BindBuffer)(target, buffer);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindFramebuffer(GLenum target, GLuint framebuffer)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, BindFramebuffer)(target, framebuffer);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, BindRenderbuffer)(target, renderbuffer);
}

static void QOPENGLF_APIENTRY qopenglfResolveBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, BlendColor)(red, green, blue, alpha);
}

static void QOPENGLF_APIENTRY qopenglfResolveBlendEquation(GLenum mode)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, BlendEquation)(mode);
}

static void QOPENGLF_APIENTRY qopenglfResolveBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, BlendEquationSeparate)(modeRGB, modeAlpha);
}

static void QOPENGLF_APIENTRY qopenglfResolveBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, BlendFuncSeparate)(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

static void QOPENGLF_APIENTRY qopenglfResolveBufferData(GLenum target, qopengl_GLsizeiptr size, const void* data, GLenum usage)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, BufferData)(target, size, data, usage);
}

static void QOPENGLF_APIENTRY qopenglfResolveBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, const void* data)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, BufferSubData)(target, offset, size, data);
}

static GLenum QOPENGLF_APIENTRY qopenglfResolveCheckFramebufferStatus(GLenum target)
{
    RESOLVE_FUNC(GLenum, ResolveOES | ResolveEXT, CheckFramebufferStatus)(target);
}

static void QOPENGLF_APIENTRY qopenglfResolveCompileShader(GLuint shader)
{
    RESOLVE_FUNC_VOID(0, CompileShader)(shader);
}

static void QOPENGLF_APIENTRY qopenglfResolveCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, CompressedTexImage2D)(target, level, internalformat, width, height, border, imageSize, data);
}

static void QOPENGLF_APIENTRY qopenglfResolveCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, CompressedTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

static GLuint QOPENGLF_APIENTRY qopenglfResolveCreateProgram()
{
    RESOLVE_FUNC_WITH_ALTERNATE(GLuint, 0, CreateProgram, CreateProgramObject)();
}

static GLuint QOPENGLF_APIENTRY qopenglfResolveCreateShader(GLenum type)
{
    RESOLVE_FUNC_WITH_ALTERNATE(GLuint, 0, CreateShader, CreateShaderObject)(type);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteBuffers(GLsizei n, const GLuint* buffers)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, DeleteBuffers)(n, buffers);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, DeleteFramebuffers)(n, framebuffers);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteProgram(GLuint program)
{
    RESOLVE_FUNC_VOID(0, DeleteProgram)(program);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, DeleteRenderbuffers)(n, renderbuffers);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteShader(GLuint shader)
{
    RESOLVE_FUNC_VOID_WITH_ALTERNATE(0, DeleteShader, DeleteObject)(shader);
}

static void QOPENGLF_APIENTRY qopenglfResolveDetachShader(GLuint program, GLuint shader)
{
    RESOLVE_FUNC_VOID_WITH_ALTERNATE(0, DetachShader, DetachObject)(program, shader);
}

static void QOPENGLF_APIENTRY qopenglfResolveDisableVertexAttribArray(GLuint index)
{
    RESOLVE_FUNC_VOID(0, DisableVertexAttribArray)(index);
}

static void QOPENGLF_APIENTRY qopenglfResolveEnableVertexAttribArray(GLuint index)
{
    RESOLVE_FUNC_VOID(0, EnableVertexAttribArray)(index);
}

static void QOPENGLF_APIENTRY qopenglfResolveFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, FramebufferRenderbuffer)(target, attachment, renderbuffertarget, renderbuffer);
}

static void QOPENGLF_APIENTRY qopenglfResolveFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, FramebufferTexture2D)(target, attachment, textarget, texture, level);
}

static void QOPENGLF_APIENTRY qopenglfResolveGenBuffers(GLsizei n, GLuint* buffers)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, GenBuffers)(n, buffers);
}

static void QOPENGLF_APIENTRY qopenglfResolveGenerateMipmap(GLenum target)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, GenerateMipmap)(target);
}

static void QOPENGLF_APIENTRY qopenglfResolveGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, GenFramebuffers)(n, framebuffers);
}

static void QOPENGLF_APIENTRY qopenglfResolveGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, GenRenderbuffers)(n, renderbuffers);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
    RESOLVE_FUNC_VOID(0, GetActiveAttrib)(program, index, bufsize, length, size, type, name);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
    RESOLVE_FUNC_VOID(0, GetActiveUniform)(program, index, bufsize, length, size, type, name);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
    RESOLVE_FUNC_VOID_WITH_ALTERNATE(0, GetAttachedShaders, GetAttachedObjects)(program, maxcount, count, shaders);
}

static GLint QOPENGLF_APIENTRY qopenglfResolveGetAttribLocation(GLuint program, const char* name)
{
    RESOLVE_FUNC(GLint, 0, GetAttribLocation)(program, name);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, GetBufferParameteriv)(target, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, GetFramebufferAttachmentParameteriv)(target, attachment, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
    RESOLVE_FUNC_VOID_WITH_ALTERNATE(0, GetProgramiv, GetObjectParameteriv)(program, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog)
{
    RESOLVE_FUNC_VOID_WITH_ALTERNATE(0, GetProgramInfoLog, GetInfoLog)(program, bufsize, length, infolog);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, GetRenderbufferParameteriv)(target, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
    RESOLVE_FUNC_VOID_WITH_ALTERNATE(0, GetShaderiv, GetObjectParameteriv)(shader, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog)
{
    RESOLVE_FUNC_VOID_WITH_ALTERNATE(0, GetShaderInfoLog, GetInfoLog)(shader, bufsize, length, infolog);
}

static void QOPENGLF_APIENTRY qopenglfSpecialGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
    Q_UNUSED(shadertype);
    Q_UNUSED(precisiontype);
    range[0] = range[1] = precision[0] = 0;
}

static void QOPENGLF_APIENTRY qopenglfResolveGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
    RESOLVE_FUNC_SPECIAL_VOID(ResolveOES | ResolveEXT, GetShaderPrecisionFormat)(shadertype, precisiontype, range, precision);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source)
{
    RESOLVE_FUNC_VOID(0, GetShaderSource)(shader, bufsize, length, source);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
    RESOLVE_FUNC_VOID(0, GetUniformfv)(program, location, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetUniformiv(GLuint program, GLint location, GLint* params)
{
    RESOLVE_FUNC_VOID(0, GetUniformiv)(program, location, params);
}

static GLint QOPENGLF_APIENTRY qopenglfResolveGetUniformLocation(GLuint program, const char* name)
{
    RESOLVE_FUNC(GLint, 0, GetUniformLocation)(program, name);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
    RESOLVE_FUNC_VOID(0, GetVertexAttribfv)(index, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
    RESOLVE_FUNC_VOID(0, GetVertexAttribiv)(index, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer)
{
    RESOLVE_FUNC_VOID(0, GetVertexAttribPointerv)(index, pname, pointer);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsBuffer(GLuint buffer)
{
    RESOLVE_FUNC(GLboolean, ResolveOES | ResolveEXT, IsBuffer)(buffer);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsFramebuffer(GLuint framebuffer)
{
    RESOLVE_FUNC(GLboolean, ResolveOES | ResolveEXT, IsFramebuffer)(framebuffer);
}

static GLboolean QOPENGLF_APIENTRY qopenglfSpecialIsProgram(GLuint program)
{
    return program != 0;
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsProgram(GLuint program)
{
    RESOLVE_FUNC_SPECIAL(GLboolean, 0, IsProgram)(program);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsRenderbuffer(GLuint renderbuffer)
{
    RESOLVE_FUNC(GLboolean, ResolveOES | ResolveEXT, IsRenderbuffer)(renderbuffer);
}

static GLboolean QOPENGLF_APIENTRY qopenglfSpecialIsShader(GLuint shader)
{
    return shader != 0;
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsShader(GLuint shader)
{
    RESOLVE_FUNC_SPECIAL(GLboolean, 0, IsShader)(shader);
}

static void QOPENGLF_APIENTRY qopenglfResolveLinkProgram(GLuint program)
{
    RESOLVE_FUNC_VOID(0, LinkProgram)(program);
}

static void QOPENGLF_APIENTRY qopenglfSpecialReleaseShaderCompiler()
{
}

static void QOPENGLF_APIENTRY qopenglfResolveReleaseShaderCompiler()
{
    RESOLVE_FUNC_SPECIAL_VOID(0, ReleaseShaderCompiler)();
}

static void QOPENGLF_APIENTRY qopenglfResolveRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, RenderbufferStorage)(target, internalformat, width, height);
}

static void QOPENGLF_APIENTRY qopenglfResolveSampleCoverage(GLclampf value, GLboolean invert)
{
    RESOLVE_FUNC_VOID(ResolveOES | ResolveEXT, SampleCoverage)(value, invert);
}

static void QOPENGLF_APIENTRY qopenglfResolveShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length)
{
    RESOLVE_FUNC_VOID(0, ShaderBinary)(n, shaders, binaryformat, binary, length);
}

static void QOPENGLF_APIENTRY qopenglfResolveShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length)
{
    RESOLVE_FUNC_VOID(0, ShaderSource)(shader, count, string, length);
}

static void QOPENGLF_APIENTRY qopenglfResolveStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
    RESOLVE_FUNC_VOID(ResolveEXT, StencilFuncSeparate)(face, func, ref, mask);
}

static void QOPENGLF_APIENTRY qopenglfResolveStencilMaskSeparate(GLenum face, GLuint mask)
{
    RESOLVE_FUNC_VOID(ResolveEXT, StencilMaskSeparate)(face, mask);
}

static void QOPENGLF_APIENTRY qopenglfResolveStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
    RESOLVE_FUNC_VOID(ResolveEXT, StencilOpSeparate)(face, fail, zfail, zpass);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform1f(GLint location, GLfloat x)
{
    RESOLVE_FUNC_VOID(0, Uniform1f)(location, x);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    RESOLVE_FUNC_VOID(0, Uniform1fv)(location, count, v);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform1i(GLint location, GLint x)
{
    RESOLVE_FUNC_VOID(0, Uniform1i)(location, x);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform1iv(GLint location, GLsizei count, const GLint* v)
{
    RESOLVE_FUNC_VOID(0, Uniform1iv)(location, count, v);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform2f(GLint location, GLfloat x, GLfloat y)
{
    RESOLVE_FUNC_VOID(0, Uniform2f)(location, x, y);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
    RESOLVE_FUNC_VOID(0, Uniform2fv)(location, count, v);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform2i(GLint location, GLint x, GLint y)
{
    RESOLVE_FUNC_VOID(0, Uniform2i)(location, x, y);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform2iv(GLint location, GLsizei count, const GLint* v)
{
    RESOLVE_FUNC_VOID(0, Uniform2iv)(location, count, v);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
    RESOLVE_FUNC_VOID(0, Uniform3f)(location, x, y, z);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
    RESOLVE_FUNC_VOID(0, Uniform3fv)(location, count, v);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform3i(GLint location, GLint x, GLint y, GLint z)
{
    RESOLVE_FUNC_VOID(0, Uniform3i)(location, x, y, z);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform3iv(GLint location, GLsizei count, const GLint* v)
{
    RESOLVE_FUNC_VOID(0, Uniform3iv)(location, count, v);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    RESOLVE_FUNC_VOID(0, Uniform4f)(location, x, y, z, w);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
    RESOLVE_FUNC_VOID(0, Uniform4fv)(location, count, v);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
    RESOLVE_FUNC_VOID(0, Uniform4i)(location, x, y, z, w);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform4iv(GLint location, GLsizei count, const GLint* v)
{
    RESOLVE_FUNC_VOID(0, Uniform4iv)(location, count, v);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    RESOLVE_FUNC_VOID(0, UniformMatrix2fv)(location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    RESOLVE_FUNC_VOID(0, UniformMatrix3fv)(location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    RESOLVE_FUNC_VOID(0, UniformMatrix4fv)(location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUseProgram(GLuint program)
{
    RESOLVE_FUNC_VOID(0, UseProgram)(program);
}

static void QOPENGLF_APIENTRY qopenglfResolveValidateProgram(GLuint program)
{
    RESOLVE_FUNC_VOID(0, ValidateProgram)(program);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttrib1f(GLuint indx, GLfloat x)
{
    RESOLVE_FUNC_VOID(0, VertexAttrib1f)(indx, x);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttrib1fv(GLuint indx, const GLfloat* values)
{
    RESOLVE_FUNC_VOID(0, VertexAttrib1fv)(indx, values);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
{
    RESOLVE_FUNC_VOID(0, VertexAttrib2f)(indx, x, y);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttrib2fv(GLuint indx, const GLfloat* values)
{
    RESOLVE_FUNC_VOID(0, VertexAttrib2fv)(indx, values);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
    RESOLVE_FUNC_VOID(0, VertexAttrib3f)(indx, x, y, z);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttrib3fv(GLuint indx, const GLfloat* values)
{
    RESOLVE_FUNC_VOID(0, VertexAttrib3fv)(indx, values);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    RESOLVE_FUNC_VOID(0, VertexAttrib4f)(indx, x, y, z, w);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttrib4fv(GLuint indx, const GLfloat* values)
{
    RESOLVE_FUNC_VOID(0, VertexAttrib4fv)(indx, values);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)
{
    RESOLVE_FUNC_VOID(0, VertexAttribPointer)(indx, size, type, normalized, stride, ptr);
}

#endif // !QT_OPENGL_ES_2

// Extensions not standard in any ES version

static GLvoid *QOPENGLF_APIENTRY qopenglfResolveMapBuffer(GLenum target, GLenum access)
{
    // It is possible that GL_OES_map_buffer is present, but then having to
    // differentiate between glUnmapBufferOES and glUnmapBuffer causes extra
    // headache. QOpenGLBuffer::map() will handle this automatically, while direct
    // calls are better off with migrating to the standard glMapBufferRange.
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (ctx->isOpenGLES() && ctx->format().majorVersion() >= 3) {
        qWarning("QOpenGLFunctions: glMapBuffer is not available in OpenGL ES 3.0 and up. Use glMapBufferRange instead.");
        return 0;
    } else {
        RESOLVE_FUNC(GLvoid *, ResolveOES, MapBuffer)(target, access);
    }
}

static void QOPENGLF_APIENTRY qopenglfResolveGetBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, GLvoid *data)
{
    RESOLVE_FUNC_VOID(ResolveEXT, GetBufferSubData)
        (target, offset, size, data);
}

static void QOPENGLF_APIENTRY qopenglfResolveDiscardFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments)
{
    RESOLVE_FUNC_VOID(ResolveEXT, DiscardFramebuffer)(target, numAttachments, attachments);
}

#if !defined(QT_OPENGL_ES_2) && !defined(QT_OPENGL_DYNAMIC)
// Special translation functions for ES-specific calls on desktop GL

static void QOPENGLF_APIENTRY qopenglfTranslateClearDepthf(GLclampf depth)
{
    ::glClearDepth(depth);
}

static void QOPENGLF_APIENTRY qopenglfTranslateDepthRangef(GLclampf zNear, GLclampf zFar)
{
    ::glDepthRange(zNear, zFar);
}
#endif // !ES && !DYNAMIC

QOpenGLFunctionsPrivate::QOpenGLFunctionsPrivate(QOpenGLContext *)
{
    /* Assign a pointer to an above defined static function
     * which on first call resolves the function from the current
     * context, assigns it to the member variable and executes it
     * (see Resolver template) */
#ifndef QT_OPENGL_ES_2
    // The GL1 functions may not be queriable via getProcAddress().
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::AllGLFunctionsQueryable)) {
        // The platform plugin supports resolving these.
        BindTexture = qopenglfResolveBindTexture;
        BlendFunc = qopenglfResolveBlendFunc;
        Clear = qopenglfResolveClear;
        ClearColor = qopenglfResolveClearColor;
        ClearDepthf = qopenglfResolveClearDepthf;
        ClearStencil = qopenglfResolveClearStencil;
        ColorMask = qopenglfResolveColorMask;
        CopyTexImage2D = qopenglfResolveCopyTexImage2D;
        CopyTexSubImage2D = qopenglfResolveCopyTexSubImage2D;
        CullFace = qopenglfResolveCullFace;
        DeleteTextures = qopenglfResolveDeleteTextures;
        DepthFunc = qopenglfResolveDepthFunc;
        DepthMask = qopenglfResolveDepthMask;
        DepthRangef = qopenglfResolveDepthRangef;
        Disable = qopenglfResolveDisable;
        DrawArrays = qopenglfResolveDrawArrays;
        DrawElements = qopenglfResolveDrawElements;
        Enable = qopenglfResolveEnable;
        Finish = qopenglfResolveFinish;
        Flush = qopenglfResolveFlush;
        FrontFace = qopenglfResolveFrontFace;
        GenTextures = qopenglfResolveGenTextures;
        GetBooleanv = qopenglfResolveGetBooleanv;
        GetError = qopenglfResolveGetError;
        GetFloatv = qopenglfResolveGetFloatv;
        GetIntegerv = qopenglfResolveGetIntegerv;
        GetString = qopenglfResolveGetString;
        GetTexParameterfv = qopenglfResolveGetTexParameterfv;
        GetTexParameteriv = qopenglfResolveGetTexParameteriv;
        Hint = qopenglfResolveHint;
        IsEnabled = qopenglfResolveIsEnabled;
        IsTexture = qopenglfResolveIsTexture;
        LineWidth = qopenglfResolveLineWidth;
        PixelStorei = qopenglfResolvePixelStorei;
        PolygonOffset = qopenglfResolvePolygonOffset;
        ReadPixels = qopenglfResolveReadPixels;
        Scissor = qopenglfResolveScissor;
        StencilFunc = qopenglfResolveStencilFunc;
        StencilMask = qopenglfResolveStencilMask;
        StencilOp = qopenglfResolveStencilOp;
        TexImage2D = qopenglfResolveTexImage2D;
        TexParameterf = qopenglfResolveTexParameterf;
        TexParameterfv = qopenglfResolveTexParameterfv;
        TexParameteri = qopenglfResolveTexParameteri;
        TexParameteriv = qopenglfResolveTexParameteriv;
        TexSubImage2D = qopenglfResolveTexSubImage2D;
        Viewport = qopenglfResolveViewport;
    } else {
#ifndef QT_OPENGL_DYNAMIC
        // Use the functions directly. This requires linking QtGui to an OpenGL implementation.
        BindTexture = ::glBindTexture;
        BlendFunc = ::glBlendFunc;
        Clear = ::glClear;
        ClearColor = ::glClearColor;
        ClearDepthf = qopenglfTranslateClearDepthf;
        ClearStencil = ::glClearStencil;
        ColorMask = ::glColorMask;
        CopyTexImage2D = ::glCopyTexImage2D;
        CopyTexSubImage2D = ::glCopyTexSubImage2D;
        CullFace = ::glCullFace;
        DeleteTextures = ::glDeleteTextures;
        DepthFunc = ::glDepthFunc;
        DepthMask = ::glDepthMask;
        DepthRangef = qopenglfTranslateDepthRangef;
        Disable = ::glDisable;
        DrawArrays = ::glDrawArrays;
        DrawElements = ::glDrawElements;
        Enable = ::glEnable;
        Finish = ::glFinish;
        Flush = ::glFlush;
        FrontFace = ::glFrontFace;
        GenTextures = ::glGenTextures;
        GetBooleanv = ::glGetBooleanv;
        GetError = ::glGetError;
        GetFloatv = ::glGetFloatv;
        GetIntegerv = ::glGetIntegerv;
        GetString = ::glGetString;
        GetTexParameterfv = ::glGetTexParameterfv;
        GetTexParameteriv = ::glGetTexParameteriv;
        Hint = ::glHint;
        IsEnabled = ::glIsEnabled;
        IsTexture = ::glIsTexture;
        LineWidth = ::glLineWidth;
        PixelStorei = ::glPixelStorei;
        PolygonOffset = ::glPolygonOffset;
        ReadPixels = ::glReadPixels;
        Scissor = ::glScissor;
        StencilFunc = ::glStencilFunc;
        StencilMask = ::glStencilMask;
        StencilOp = ::glStencilOp;
#if defined(Q_OS_OSX) && MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
        TexImage2D = reinterpret_cast<void (*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *)>(glTexImage2D);
#else
        TexImage2D = glTexImage2D;
#endif
        TexParameterf = ::glTexParameterf;
        TexParameterfv = ::glTexParameterfv;
        TexParameteri = ::glTexParameteri;
        TexParameteriv = ::glTexParameteriv;
        TexSubImage2D = ::glTexSubImage2D;
        Viewport = ::glViewport;
#else // QT_OPENGL_DYNAMIC
        // This should not happen.
        qFatal("QOpenGLFunctions: Dynamic OpenGL builds do not support platforms with insufficient function resolving capabilities");
#endif
    }

    ActiveTexture = qopenglfResolveActiveTexture;
    AttachShader = qopenglfResolveAttachShader;
    BindAttribLocation = qopenglfResolveBindAttribLocation;
    BindBuffer = qopenglfResolveBindBuffer;
    BindFramebuffer = qopenglfResolveBindFramebuffer;
    BindRenderbuffer = qopenglfResolveBindRenderbuffer;
    BlendColor = qopenglfResolveBlendColor;
    BlendEquation = qopenglfResolveBlendEquation;
    BlendEquationSeparate = qopenglfResolveBlendEquationSeparate;
    BlendFuncSeparate = qopenglfResolveBlendFuncSeparate;
    BufferData = qopenglfResolveBufferData;
    BufferSubData = qopenglfResolveBufferSubData;
    CheckFramebufferStatus = qopenglfResolveCheckFramebufferStatus;
    CompileShader = qopenglfResolveCompileShader;
    CompressedTexImage2D = qopenglfResolveCompressedTexImage2D;
    CompressedTexSubImage2D = qopenglfResolveCompressedTexSubImage2D;
    CreateProgram = qopenglfResolveCreateProgram;
    CreateShader = qopenglfResolveCreateShader;
    DeleteBuffers = qopenglfResolveDeleteBuffers;
    DeleteFramebuffers = qopenglfResolveDeleteFramebuffers;
    DeleteProgram = qopenglfResolveDeleteProgram;
    DeleteRenderbuffers = qopenglfResolveDeleteRenderbuffers;
    DeleteShader = qopenglfResolveDeleteShader;
    DetachShader = qopenglfResolveDetachShader;
    DisableVertexAttribArray = qopenglfResolveDisableVertexAttribArray;
    EnableVertexAttribArray = qopenglfResolveEnableVertexAttribArray;
    FramebufferRenderbuffer = qopenglfResolveFramebufferRenderbuffer;
    FramebufferTexture2D = qopenglfResolveFramebufferTexture2D;
    GenBuffers = qopenglfResolveGenBuffers;
    GenerateMipmap = qopenglfResolveGenerateMipmap;
    GenFramebuffers = qopenglfResolveGenFramebuffers;
    GenRenderbuffers = qopenglfResolveGenRenderbuffers;
    GetActiveAttrib = qopenglfResolveGetActiveAttrib;
    GetActiveUniform = qopenglfResolveGetActiveUniform;
    GetAttachedShaders = qopenglfResolveGetAttachedShaders;
    GetAttribLocation = qopenglfResolveGetAttribLocation;
    GetBufferParameteriv = qopenglfResolveGetBufferParameteriv;
    GetFramebufferAttachmentParameteriv = qopenglfResolveGetFramebufferAttachmentParameteriv;
    GetProgramiv = qopenglfResolveGetProgramiv;
    GetProgramInfoLog = qopenglfResolveGetProgramInfoLog;
    GetRenderbufferParameteriv = qopenglfResolveGetRenderbufferParameteriv;
    GetShaderiv = qopenglfResolveGetShaderiv;
    GetShaderInfoLog = qopenglfResolveGetShaderInfoLog;
    GetShaderPrecisionFormat = qopenglfResolveGetShaderPrecisionFormat;
    GetShaderSource = qopenglfResolveGetShaderSource;
    GetUniformfv = qopenglfResolveGetUniformfv;
    GetUniformiv = qopenglfResolveGetUniformiv;
    GetUniformLocation = qopenglfResolveGetUniformLocation;
    GetVertexAttribfv = qopenglfResolveGetVertexAttribfv;
    GetVertexAttribiv = qopenglfResolveGetVertexAttribiv;
    GetVertexAttribPointerv = qopenglfResolveGetVertexAttribPointerv;
    IsBuffer = qopenglfResolveIsBuffer;
    IsFramebuffer = qopenglfResolveIsFramebuffer;
    IsProgram = qopenglfResolveIsProgram;
    IsRenderbuffer = qopenglfResolveIsRenderbuffer;
    IsShader = qopenglfResolveIsShader;
    LinkProgram = qopenglfResolveLinkProgram;
    ReleaseShaderCompiler = qopenglfResolveReleaseShaderCompiler;
    RenderbufferStorage = qopenglfResolveRenderbufferStorage;
    SampleCoverage = qopenglfResolveSampleCoverage;
    ShaderBinary = qopenglfResolveShaderBinary;
    ShaderSource = qopenglfResolveShaderSource;
    StencilFuncSeparate = qopenglfResolveStencilFuncSeparate;
    StencilMaskSeparate = qopenglfResolveStencilMaskSeparate;
    StencilOpSeparate = qopenglfResolveStencilOpSeparate;
    Uniform1f = qopenglfResolveUniform1f;
    Uniform1fv = qopenglfResolveUniform1fv;
    Uniform1i = qopenglfResolveUniform1i;
    Uniform1iv = qopenglfResolveUniform1iv;
    Uniform2f = qopenglfResolveUniform2f;
    Uniform2fv = qopenglfResolveUniform2fv;
    Uniform2i = qopenglfResolveUniform2i;
    Uniform2iv = qopenglfResolveUniform2iv;
    Uniform3f = qopenglfResolveUniform3f;
    Uniform3fv = qopenglfResolveUniform3fv;
    Uniform3i = qopenglfResolveUniform3i;
    Uniform3iv = qopenglfResolveUniform3iv;
    Uniform4f = qopenglfResolveUniform4f;
    Uniform4fv = qopenglfResolveUniform4fv;
    Uniform4i = qopenglfResolveUniform4i;
    Uniform4iv = qopenglfResolveUniform4iv;
    UniformMatrix2fv = qopenglfResolveUniformMatrix2fv;
    UniformMatrix3fv = qopenglfResolveUniformMatrix3fv;
    UniformMatrix4fv = qopenglfResolveUniformMatrix4fv;
    UseProgram = qopenglfResolveUseProgram;
    ValidateProgram = qopenglfResolveValidateProgram;
    VertexAttrib1f = qopenglfResolveVertexAttrib1f;
    VertexAttrib1fv = qopenglfResolveVertexAttrib1fv;
    VertexAttrib2f = qopenglfResolveVertexAttrib2f;
    VertexAttrib2fv = qopenglfResolveVertexAttrib2fv;
    VertexAttrib3f = qopenglfResolveVertexAttrib3f;
    VertexAttrib3fv = qopenglfResolveVertexAttrib3fv;
    VertexAttrib4f = qopenglfResolveVertexAttrib4f;
    VertexAttrib4fv = qopenglfResolveVertexAttrib4fv;
    VertexAttribPointer = qopenglfResolveVertexAttribPointer;
#endif // !QT_OPENGL_ES_2
}

/*!
    \class QOpenGLExtraFunctions
    \brief The QOpenGLExtraFunctions class provides cross-platform access to the OpenGL ES 3.0 and 3.1 API.
    \since 5.6
    \ingroup painting-3D
    \inmodule QtGui

    This subclass of QOpenGLFunctions includes the OpenGL ES 3.0 and 3.1
    functions. These will only work when an OpenGL ES 3.0 or 3.1 context, or an
    OpenGL context of a version containing the functions in question either in
    core or as extension, is in use. This allows developing GLES 3.0 and 3.1
    applications in a cross-platform manner: development can happen on a desktop
    platform with OpenGL 3.x or 4.x, deploying to a real GLES 3.1 device later
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
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBeginQuery.xml}{glBeginQuery()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBeginTransformFeedback(GLenum primitiveMode)

    Convenience function that calls glBeginTransformFeedback(\a primitiveMode).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBeginTransformFeedback.xml}{glBeginTransformFeedback()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindBufferBase(GLenum target, GLuint index, GLuint buffer)

    Convenience function that calls glBindBufferBase(\a target, \a index, \a buffer).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBindBufferBase.xml}{glBindBufferBase()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)

    Convenience function that calls glBindBufferRange(\a target, \a index, \a buffer, \a offset, \a size).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBindBufferRange.xml}{glBindBufferRange()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindSampler(GLuint unit, GLuint sampler)

    Convenience function that calls glBindSampler(\a unit, \a sampler).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBindSampler.xml}{glBindSampler()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindTransformFeedback(GLenum target, GLuint id)

    Convenience function that calls glBindTransformFeedback(\a target, \a id).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBindTransformFeedback.xml}{glBindTransformFeedback()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindVertexArray(GLuint array)

    Convenience function that calls glBindVertexArray(\a array).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBindVertexArray.xml}{glBindVertexArray()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)

    Convenience function that calls glBlitFramebuffer(\a srcX0, \a srcY0, \a srcX1, \a srcY1, \a dstX0, \a dstY0, \a dstX1, \a dstY1, \a mask, \a filter).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBlitFramebuffer.xml}{glBlitFramebuffer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)

    Convenience function that calls glClearBufferfi(\a buffer, \a drawbuffer, \a depth, \a stencil).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glClearBufferfi.xml}{glClearBufferfi()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat * value)

    Convenience function that calls glClearBufferfv(\a buffer, \a drawbuffer, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glClearBufferfv.xml}{glClearBufferfv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint * value)

    Convenience function that calls glClearBufferiv(\a buffer, \a drawbuffer, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glClearBufferiv.xml}{glClearBufferiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint * value)

    Convenience function that calls glClearBufferuiv(\a buffer, \a drawbuffer, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glClearBufferuiv.xml}{glClearBufferuiv()}.
*/

/*!
    \fn GLenum QOpenGLExtraFunctions::glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)

    Convenience function that calls glClientWaitSync(\a sync, \a flags, \a timeout).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glClientWaitSync.xml}{glClientWaitSync()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data)

    Convenience function that calls glCompressedTexImage3D(\a target, \a level, \a internalformat, \a width, \a height, \a depth, \a border, \a imageSize, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glCompressedTexImage3D.xml}{glCompressedTexImage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void * data)

    Convenience function that calls glCompressedTexSubImage3D(\a target, \a level, \a xoffset, \a yoffset, \a zoffset, \a width, \a height, \a depth, \a format, \a imageSize, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glCompressedTexSubImage3D.xml}{glCompressedTexSubImage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)

    Convenience function that calls glCopyBufferSubData(\a readTarget, \a writeTarget, \a readOffset, \a writeOffset, \a size).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glCopyBufferSubData.xml}{glCopyBufferSubData()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)

    Convenience function that calls glCopyTexSubImage3D(\a target, \a level, \a xoffset, \a yoffset, \a zoffset, \a x, \a y, \a width, \a height).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glCopyTexSubImage3D.xml}{glCopyTexSubImage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteQueries(GLsizei n, const GLuint * ids)

    Convenience function that calls glDeleteQueries(\a n, \a ids).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDeleteQueries.xml}{glDeleteQueries()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteSamplers(GLsizei count, const GLuint * samplers)

    Convenience function that calls glDeleteSamplers(\a count, \a samplers).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDeleteSamplers.xml}{glDeleteSamplers()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteSync(GLsync sync)

    Convenience function that calls glDeleteSync(\a sync).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDeleteSync.xml}{glDeleteSync()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteTransformFeedbacks(GLsizei n, const GLuint * ids)

    Convenience function that calls glDeleteTransformFeedbacks(\a n, \a ids).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDeleteTransformFeedbacks.xml}{glDeleteTransformFeedbacks()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteVertexArrays(GLsizei n, const GLuint * arrays)

    Convenience function that calls glDeleteVertexArrays(\a n, \a arrays).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDeleteVertexArrays.xml}{glDeleteVertexArrays()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)

    Convenience function that calls glDrawArraysInstanced(\a mode, \a first, \a count, \a instancecount).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDrawArraysInstanced.xml}{glDrawArraysInstanced()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawBuffers(GLsizei n, const GLenum * bufs)

    Convenience function that calls glDrawBuffers(\a n, \a bufs).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDrawBuffers.xml}{glDrawBuffers()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount)

    Convenience function that calls glDrawElementsInstanced(\a mode, \a count, \a type, \a indices, \a instancecount).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDrawElementsInstanced.xml}{glDrawElementsInstanced()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void * indices)

    Convenience function that calls glDrawRangeElements(\a mode, \a start, \a end, \a count, \a type, \a indices).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDrawRangeElements.xml}{glDrawRangeElements()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glEndQuery(GLenum target)

    Convenience function that calls glEndQuery(\a target).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glEndQuery.xml}{glEndQuery()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glEndTransformFeedback(void)

    Convenience function that calls glEndTransformFeedback().

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glEndTransformFeedback.xml}{glEndTransformFeedback()}.
*/

/*!
    \fn GLsync QOpenGLExtraFunctions::glFenceSync(GLenum condition, GLbitfield flags)

    Convenience function that calls glFenceSync(\a condition, \a flags).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glFenceSync.xml}{glFenceSync()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)

    Convenience function that calls glFlushMappedBufferRange(\a target, \a offset, \a length).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glFlushMappedBufferRange.xml}{glFlushMappedBufferRange()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)

    Convenience function that calls glFramebufferTextureLayer(\a target, \a attachment, \a texture, \a level, \a layer).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glFramebufferTextureLayer.xml}{glFramebufferTextureLayer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGenQueries(GLsizei n, GLuint* ids)

    Convenience function that calls glGenQueries(\a n, \a ids).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGenQueries.xml}{glGenQueries()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGenSamplers(GLsizei count, GLuint* samplers)

    Convenience function that calls glGenSamplers(\a count, \a samplers).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGenSamplers.xml}{glGenSamplers()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGenTransformFeedbacks(GLsizei n, GLuint* ids)

    Convenience function that calls glGenTransformFeedbacks(\a n, \a ids).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGenTransformFeedbacks.xml}{glGenTransformFeedbacks()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGenVertexArrays(GLsizei n, GLuint* arrays)

    Convenience function that calls glGenVertexArrays(\a n, \a arrays).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGenVertexArrays.xml}{glGenVertexArrays()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName)

    Convenience function that calls glGetActiveUniformBlockName(\a program, \a uniformBlockIndex, \a bufSize, \a length, \a uniformBlockName).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetActiveUniformBlockName.xml}{glGetActiveUniformBlockName()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params)

    Convenience function that calls glGetActiveUniformBlockiv(\a program, \a uniformBlockIndex, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetActiveUniformBlockiv.xml}{glGetActiveUniformBlockiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint* params)

    Convenience function that calls glGetActiveUniformsiv(\a program, \a uniformCount, \a uniformIndices, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetActiveUniformsiv.xml}{glGetActiveUniformsiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64* params)

    Convenience function that calls glGetBufferParameteri64v(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetBufferParameteri64v.xml}{glGetBufferParameteri64v()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetBufferPointerv(GLenum target, GLenum pname, void ** params)

    Convenience function that calls glGetBufferPointerv(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetBufferPointerv.xml}{glGetBufferPointerv()}.
*/

/*!
    \fn GLint QOpenGLExtraFunctions::glGetFragDataLocation(GLuint program, const GLchar * name)

    Convenience function that calls glGetFragDataLocation(\a program, \a name).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetFragDataLocation.xml}{glGetFragDataLocation()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetInteger64i_v(GLenum target, GLuint index, GLint64* data)

    Convenience function that calls glGetInteger64i_v(\a target, \a index, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetInteger64i_v.xml}{glGetInteger64i_v()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetInteger64v(GLenum pname, GLint64* data)

    Convenience function that calls glGetInteger64v(\a pname, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetInteger64v.xml}{glGetInteger64v()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetIntegeri_v(GLenum target, GLuint index, GLint* data)

    Convenience function that calls glGetIntegeri_v(\a target, \a index, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetIntegeri_v.xml}{glGetIntegeri_v()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint* params)

    Convenience function that calls glGetInternalformativ(\a target, \a internalformat, \a pname, \a bufSize, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetInternalformativ.xml}{glGetInternalformativ()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void * binary)

    Convenience function that calls glGetProgramBinary(\a program, \a bufSize, \a length, \a binaryFormat, \a binary).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetProgramBinary.xml}{glGetProgramBinary()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params)

    Convenience function that calls glGetQueryObjectuiv(\a id, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetQueryObjectuiv.xml}{glGetQueryObjectuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetQueryiv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetQueryiv(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetQueryiv.xml}{glGetQueryiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat* params)

    Convenience function that calls glGetSamplerParameterfv(\a sampler, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetSamplerParameterfv.xml}{glGetSamplerParameterfv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params)

    Convenience function that calls glGetSamplerParameteriv(\a sampler, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetSamplerParameteriv.xml}{glGetSamplerParameteriv()}.
*/

/*!
    \fn const GLubyte * QOpenGLExtraFunctions::glGetStringi(GLenum name, GLuint index)

    Convenience function that calls glGetStringi(\a name, \a index).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetStringi.xml}{glGetStringi()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values)

    Convenience function that calls glGetSynciv(\a sync, \a pname, \a bufSize, \a length, \a values).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetSynciv.xml}{glGetSynciv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name)

    Convenience function that calls glGetTransformFeedbackVarying(\a program, \a index, \a bufSize, \a length, \a size, \a type, \a name).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetTransformFeedbackVarying.xml}{glGetTransformFeedbackVarying()}.
*/

/*!
    \fn GLuint QOpenGLExtraFunctions::glGetUniformBlockIndex(GLuint program, const GLchar * uniformBlockName)

    Convenience function that calls glGetUniformBlockIndex(\a program, \a uniformBlockName).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetUniformBlockIndex.xml}{glGetUniformBlockIndex()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const* uniformNames, GLuint* uniformIndices)

    Convenience function that calls glGetUniformIndices(\a program, \a uniformCount, \a uniformNames, \a uniformIndices).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetUniformIndices.xml}{glGetUniformIndices()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetUniformuiv(GLuint program, GLint location, GLuint* params)

    Convenience function that calls glGetUniformuiv(\a program, \a location, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetUniformuiv.xml}{glGetUniformuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetVertexAttribIiv(GLuint index, GLenum pname, GLint* params)

    Convenience function that calls glGetVertexAttribIiv(\a index, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetVertexAttribIiv.xml}{glGetVertexAttribIiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint* params)

    Convenience function that calls glGetVertexAttribIuiv(\a index, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetVertexAttribIuiv.xml}{glGetVertexAttribIuiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments)

    Convenience function that calls glInvalidateFramebuffer(\a target, \a numAttachments, \a attachments).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glInvalidateFramebuffer.xml}{glInvalidateFramebuffer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height)

    Convenience function that calls glInvalidateSubFramebuffer(\a target, \a numAttachments, \a attachments, \a x, \a y, \a width, \a height).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glInvalidateSubFramebuffer.xml}{glInvalidateSubFramebuffer()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsQuery(GLuint id)

    Convenience function that calls glIsQuery(\a id).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glIsQuery.xml}{glIsQuery()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsSampler(GLuint sampler)

    Convenience function that calls glIsSampler(\a sampler).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glIsSampler.xml}{glIsSampler()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsSync(GLsync sync)

    Convenience function that calls glIsSync(\a sync).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glIsSync.xml}{glIsSync()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsTransformFeedback(GLuint id)

    Convenience function that calls glIsTransformFeedback(\a id).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glIsTransformFeedback.xml}{glIsTransformFeedback()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsVertexArray(GLuint array)

    Convenience function that calls glIsVertexArray(\a array).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glIsVertexArray.xml}{glIsVertexArray()}.
*/

/*!
    \fn void * QOpenGLExtraFunctions::glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)

    Convenience function that calls glMapBufferRange(\a target, \a offset, \a length, \a access).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glMapBufferRange.xml}{glMapBufferRange()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glPauseTransformFeedback(void)

    Convenience function that calls glPauseTransformFeedback().

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glPauseTransformFeedback.xml}{glPauseTransformFeedback()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramBinary(GLuint program, GLenum binaryFormat, const void * binary, GLsizei length)

    Convenience function that calls glProgramBinary(\a program, \a binaryFormat, \a binary, \a length).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramBinary.xml}{glProgramBinary()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramParameteri(GLuint program, GLenum pname, GLint value)

    Convenience function that calls glProgramParameteri(\a program, \a pname, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramParameteri.xml}{glProgramParameteri()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glReadBuffer(GLenum src)

    Convenience function that calls glReadBuffer(\a src).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glReadBuffer.xml}{glReadBuffer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)

    Convenience function that calls glRenderbufferStorageMultisample(\a target, \a samples, \a internalformat, \a width, \a height).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glRenderbufferStorageMultisample.xml}{glRenderbufferStorageMultisample()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glResumeTransformFeedback(void)

    Convenience function that calls glResumeTransformFeedback().

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glResumeTransformFeedback.xml}{glResumeTransformFeedback()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)

    Convenience function that calls glSamplerParameterf(\a sampler, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glSamplerParameterf.xml}{glSamplerParameterf()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat * param)

    Convenience function that calls glSamplerParameterfv(\a sampler, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glSamplerParameterfv.xml}{glSamplerParameterfv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSamplerParameteri(GLuint sampler, GLenum pname, GLint param)

    Convenience function that calls glSamplerParameteri(\a sampler, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glSamplerParameteri.xml}{glSamplerParameteri()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint * param)

    Convenience function that calls glSamplerParameteriv(\a sampler, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glSamplerParameteriv.xml}{glSamplerParameteriv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels)

    Convenience function that calls glTexImage3D(\a target, \a level, \a internalformat, \a width, \a height, \a depth, \a border, \a format, \a type, \a pixels).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glTexImage3D.xml}{glTexImage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)

    Convenience function that calls glTexStorage2D(\a target, \a levels, \a internalformat, \a width, \a height).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glTexStorage2D.xml}{glTexStorage2D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)

    Convenience function that calls glTexStorage3D(\a target, \a levels, \a internalformat, \a width, \a height, \a depth).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glTexStorage3D.xml}{glTexStorage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels)

    Convenience function that calls glTexSubImage3D(\a target, \a level, \a xoffset, \a yoffset, \a zoffset, \a width, \a height, \a depth, \a format, \a type, \a pixels).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glTexSubImage3D.xml}{glTexSubImage3D()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const* varyings, GLenum bufferMode)

    Convenience function that calls glTransformFeedbackVaryings(\a program, \a count, \a varyings, \a bufferMode).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glTransformFeedbackVaryings.xml}{glTransformFeedbackVaryings()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform1ui(GLint location, GLuint v0)

    Convenience function that calls glUniform1ui(\a location, \a v0).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniform1ui.xml}{glUniform1ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform1uiv(GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glUniform1uiv(\a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniform1uiv.xml}{glUniform1uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform2ui(GLint location, GLuint v0, GLuint v1)

    Convenience function that calls glUniform2ui(\a location, \a v0, \a v1).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniform2ui.xml}{glUniform2ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform2uiv(GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glUniform2uiv(\a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniform2uiv.xml}{glUniform2uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)

    Convenience function that calls glUniform3ui(\a location, \a v0, \a v1, \a v2).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniform3ui.xml}{glUniform3ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform3uiv(GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glUniform3uiv(\a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniform3uiv.xml}{glUniform3uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)

    Convenience function that calls glUniform4ui(\a location, \a v0, \a v1, \a v2, \a v3).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniform4ui.xml}{glUniform4ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniform4uiv(GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glUniform4uiv(\a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniform4uiv.xml}{glUniform4uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)

    Convenience function that calls glUniformBlockBinding(\a program, \a uniformBlockIndex, \a uniformBlockBinding).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniformBlockBinding.xml}{glUniformBlockBinding()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix2x3fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniformMatrix2x3fv.xml}{glUniformMatrix2x3fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix2x4fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniformMatrix2x4fv.xml}{glUniformMatrix2x4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix3x2fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniformMatrix3x2fv.xml}{glUniformMatrix3x2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix3x4fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniformMatrix3x4fv.xml}{glUniformMatrix3x4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix4x2fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniformMatrix4x2fv.xml}{glUniformMatrix4x2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glUniformMatrix4x3fv(\a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUniformMatrix4x3fv.xml}{glUniformMatrix4x3fv()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glUnmapBuffer(GLenum target)

    Convenience function that calls glUnmapBuffer(\a target).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUnmapBuffer.xml}{glUnmapBuffer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribDivisor(GLuint index, GLuint divisor)

    Convenience function that calls glVertexAttribDivisor(\a index, \a divisor).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glVertexAttribDivisor.xml}{glVertexAttribDivisor()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w)

    Convenience function that calls glVertexAttribI4i(\a index, \a x, \a y, \a z, \a w).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glVertexAttribI4i.xml}{glVertexAttribI4i()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribI4iv(GLuint index, const GLint * v)

    Convenience function that calls glVertexAttribI4iv(\a index, \a v).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glVertexAttribI4iv.xml}{glVertexAttribI4iv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)

    Convenience function that calls glVertexAttribI4ui(\a index, \a x, \a y, \a z, \a w).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glVertexAttribI4ui.xml}{glVertexAttribI4ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribI4uiv(GLuint index, const GLuint * v)

    Convenience function that calls glVertexAttribI4uiv(\a index, \a v).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glVertexAttribI4uiv.xml}{glVertexAttribI4uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer)

    Convenience function that calls glVertexAttribIPointer(\a index, \a size, \a type, \a stride, \a pointer).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glVertexAttribIPointer.xml}{glVertexAttribIPointer()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)

    Convenience function that calls glWaitSync(\a sync, \a flags, \a timeout).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glWaitSync.xml}{glWaitSync()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glActiveShaderProgram(GLuint pipeline, GLuint program)

    Convenience function that calls glActiveShaderProgram(\a pipeline, \a program).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glActiveShaderProgram.xml}{glActiveShaderProgram()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)

    Convenience function that calls glBindImageTexture(\a unit, \a texture, \a level, \a layered, \a layer, \a access, \a format).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBindImageTexture.xml}{glBindImageTexture()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindProgramPipeline(GLuint pipeline)

    Convenience function that calls glBindProgramPipeline(\a pipeline).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBindProgramPipeline.xml}{glBindProgramPipeline()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)

    Convenience function that calls glBindVertexBuffer(\a bindingindex, \a buffer, \a offset, \a stride).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glBindVertexBuffer.xml}{glBindVertexBuffer()}.
*/

/*!
    \fn GLuint QOpenGLExtraFunctions::glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const* strings)

    Convenience function that calls glCreateShaderProgramv(\a type, \a count, \a strings).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glCreateShaderProgramv.xml}{glCreateShaderProgramv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDeleteProgramPipelines(GLsizei n, const GLuint * pipelines)

    Convenience function that calls glDeleteProgramPipelines(\a n, \a pipelines).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDeleteProgramPipelines.xml}{glDeleteProgramPipelines()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)

    Convenience function that calls glDispatchCompute(\a num_groups_x, \a num_groups_y, \a num_groups_z).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDispatchCompute.xml}{glDispatchCompute()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDispatchComputeIndirect(GLintptr indirect)

    Convenience function that calls glDispatchComputeIndirect(\a indirect).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDispatchComputeIndirect.xml}{glDispatchComputeIndirect()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawArraysIndirect(GLenum mode, const void * indirect)

    Convenience function that calls glDrawArraysIndirect(\a mode, \a indirect).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDrawArraysIndirect.xml}{glDrawArraysIndirect()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect)

    Convenience function that calls glDrawElementsIndirect(\a mode, \a type, \a indirect).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glDrawElementsIndirect.xml}{glDrawElementsIndirect()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glFramebufferParameteri(GLenum target, GLenum pname, GLint param)

    Convenience function that calls glFramebufferParameteri(\a target, \a pname, \a param).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glFramebufferParameteri.xml}{glFramebufferParameteri()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGenProgramPipelines(GLsizei n, GLuint* pipelines)

    Convenience function that calls glGenProgramPipelines(\a n, \a pipelines).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGenProgramPipelines.xml}{glGenProgramPipelines()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetBooleani_v(GLenum target, GLuint index, GLboolean* data)

    Convenience function that calls glGetBooleani_v(\a target, \a index, \a data).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetBooleani_v.xml}{glGetBooleani_v()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint* params)

    Convenience function that calls glGetFramebufferParameteriv(\a target, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetFramebufferParameteriv.xml}{glGetFramebufferParameteriv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetMultisamplefv(GLenum pname, GLuint index, GLfloat* val)

    Convenience function that calls glGetMultisamplefv(\a pname, \a index, \a val).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetMultisamplefv.xml}{glGetMultisamplefv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params)

    Convenience function that calls glGetProgramInterfaceiv(\a program, \a programInterface, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetProgramInterfaceiv.xml}{glGetProgramInterfaceiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei* length, GLchar* infoLog)

    Convenience function that calls glGetProgramPipelineInfoLog(\a pipeline, \a bufSize, \a length, \a infoLog).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetProgramPipelineInfoLog.xml}{glGetProgramPipelineInfoLog()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint* params)

    Convenience function that calls glGetProgramPipelineiv(\a pipeline, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetProgramPipelineiv.xml}{glGetProgramPipelineiv()}.
*/

/*!
    \fn GLuint QOpenGLExtraFunctions::glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar * name)

    Convenience function that calls glGetProgramResourceIndex(\a program, \a programInterface, \a name).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetProgramResourceIndex.xml}{glGetProgramResourceIndex()}.
*/

/*!
    \fn GLint QOpenGLExtraFunctions::glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar * name)

    Convenience function that calls glGetProgramResourceLocation(\a program, \a programInterface, \a name).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetProgramResourceLocation.xml}{glGetProgramResourceLocation()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar* name)

    Convenience function that calls glGetProgramResourceName(\a program, \a programInterface, \a index, \a bufSize, \a length, \a name).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetProgramResourceName.xml}{glGetProgramResourceName()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei bufSize, GLsizei* length, GLint* params)

    Convenience function that calls glGetProgramResourceiv(\a program, \a programInterface, \a index, \a propCount, \a props, \a bufSize, \a length, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetProgramResourceiv.xml}{glGetProgramResourceiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat* params)

    Convenience function that calls glGetTexLevelParameterfv(\a target, \a level, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetTexLevelParameterfv.xml}{glGetTexLevelParameterfv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params)

    Convenience function that calls glGetTexLevelParameteriv(\a target, \a level, \a pname, \a params).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glGetTexLevelParameteriv.xml}{glGetTexLevelParameteriv()}.
*/

/*!
    \fn GLboolean QOpenGLExtraFunctions::glIsProgramPipeline(GLuint pipeline)

    Convenience function that calls glIsProgramPipeline(\a pipeline).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glIsProgramPipeline.xml}{glIsProgramPipeline()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glMemoryBarrier(GLbitfield barriers)

    Convenience function that calls glMemoryBarrier(\a barriers).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glMemoryBarrier.xml}{glMemoryBarrier()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glMemoryBarrierByRegion(GLbitfield barriers)

    Convenience function that calls glMemoryBarrierByRegion(\a barriers).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glMemoryBarrierByRegion.xml}{glMemoryBarrierByRegion()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1f(GLuint program, GLint location, GLfloat v0)

    Convenience function that calls glProgramUniform1f(\a program, \a location, \a v0).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform1f.xml}{glProgramUniform1f()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)

    Convenience function that calls glProgramUniform1fv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform1fv.xml}{glProgramUniform1fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1i(GLuint program, GLint location, GLint v0)

    Convenience function that calls glProgramUniform1i(\a program, \a location, \a v0).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform1i.xml}{glProgramUniform1i()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint * value)

    Convenience function that calls glProgramUniform1iv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform1iv.xml}{glProgramUniform1iv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1ui(GLuint program, GLint location, GLuint v0)

    Convenience function that calls glProgramUniform1ui(\a program, \a location, \a v0).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform1ui.xml}{glProgramUniform1ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glProgramUniform1uiv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform1uiv.xml}{glProgramUniform1uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1)

    Convenience function that calls glProgramUniform2f(\a program, \a location, \a v0, \a v1).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform2f.xml}{glProgramUniform2f()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)

    Convenience function that calls glProgramUniform2fv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform2fv.xml}{glProgramUniform2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1)

    Convenience function that calls glProgramUniform2i(\a program, \a location, \a v0, \a v1).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform2i.xml}{glProgramUniform2i()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint * value)

    Convenience function that calls glProgramUniform2iv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform2iv.xml}{glProgramUniform2iv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1)

    Convenience function that calls glProgramUniform2ui(\a program, \a location, \a v0, \a v1).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform2ui.xml}{glProgramUniform2ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glProgramUniform2uiv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform2uiv.xml}{glProgramUniform2uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2)

    Convenience function that calls glProgramUniform3f(\a program, \a location, \a v0, \a v1, \a v2).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform3f.xml}{glProgramUniform3f()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)

    Convenience function that calls glProgramUniform3fv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform3fv.xml}{glProgramUniform3fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2)

    Convenience function that calls glProgramUniform3i(\a program, \a location, \a v0, \a v1, \a v2).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform3i.xml}{glProgramUniform3i()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint * value)

    Convenience function that calls glProgramUniform3iv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform3iv.xml}{glProgramUniform3iv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2)

    Convenience function that calls glProgramUniform3ui(\a program, \a location, \a v0, \a v1, \a v2).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform3ui.xml}{glProgramUniform3ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glProgramUniform3uiv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform3uiv.xml}{glProgramUniform3uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)

    Convenience function that calls glProgramUniform4f(\a program, \a location, \a v0, \a v1, \a v2, \a v3).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform4f.xml}{glProgramUniform4f()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)

    Convenience function that calls glProgramUniform4fv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform4fv.xml}{glProgramUniform4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3)

    Convenience function that calls glProgramUniform4i(\a program, \a location, \a v0, \a v1, \a v2, \a v3).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform4i.xml}{glProgramUniform4i()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint * value)

    Convenience function that calls glProgramUniform4iv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform4iv.xml}{glProgramUniform4iv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)

    Convenience function that calls glProgramUniform4ui(\a program, \a location, \a v0, \a v1, \a v2, \a v3).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform4ui.xml}{glProgramUniform4ui()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)

    Convenience function that calls glProgramUniform4uiv(\a program, \a location, \a count, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniform4uiv.xml}{glProgramUniform4uiv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix2fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniformMatrix2fv.xml}{glProgramUniformMatrix2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix2x3fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniformMatrix2x3fv.xml}{glProgramUniformMatrix2x3fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix2x4fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniformMatrix2x4fv.xml}{glProgramUniformMatrix2x4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix3fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniformMatrix3fv.xml}{glProgramUniformMatrix3fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix3x2fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniformMatrix3x2fv.xml}{glProgramUniformMatrix3x2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix3x4fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniformMatrix3x4fv.xml}{glProgramUniformMatrix3x4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix4fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniformMatrix4fv.xml}{glProgramUniformMatrix4fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix4x2fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniformMatrix4x2fv.xml}{glProgramUniformMatrix4x2fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)

    Convenience function that calls glProgramUniformMatrix4x3fv(\a program, \a location, \a count, \a transpose, \a value).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glProgramUniformMatrix4x3fv.xml}{glProgramUniformMatrix4x3fv()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glSampleMaski(GLuint maskNumber, GLbitfield mask)

    Convenience function that calls glSampleMaski(\a maskNumber, \a mask).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glSampleMaski.xml}{glSampleMaski()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)

    Convenience function that calls glTexStorage2DMultisample(\a target, \a samples, \a internalformat, \a width, \a height, \a fixedsamplelocations).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glTexStorage2DMultisample.xml}{glTexStorage2DMultisample()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program)

    Convenience function that calls glUseProgramStages(\a pipeline, \a stages, \a program).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glUseProgramStages.xml}{glUseProgramStages()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glValidateProgramPipeline(GLuint pipeline)

    Convenience function that calls glValidateProgramPipeline(\a pipeline).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glValidateProgramPipeline.xml}{glValidateProgramPipeline()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribBinding(GLuint attribindex, GLuint bindingindex)

    Convenience function that calls glVertexAttribBinding(\a attribindex, \a bindingindex).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glVertexAttribBinding.xml}{glVertexAttribBinding()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset)

    Convenience function that calls glVertexAttribFormat(\a attribindex, \a size, \a type, \a normalized, \a relativeoffset).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glVertexAttribFormat.xml}{glVertexAttribFormat()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset)

    Convenience function that calls glVertexAttribIFormat(\a attribindex, \a size, \a type, \a relativeoffset).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glVertexAttribIFormat.xml}{glVertexAttribIFormat()}.
*/

/*!
    \fn void QOpenGLExtraFunctions::glVertexBindingDivisor(GLuint bindingindex, GLuint divisor)

    Convenience function that calls glVertexBindingDivisor(\a bindingindex, \a divisor).

    This function is only available in OpenGL ES 3.x, or OpenGL 3.x or 4.x contexts. When running
    with plain OpenGL, the function is only usable when the given profile and version contains the
    function either in core or as an extension.

    For more information, see the OpenGL ES 3.x documentation for
    \l{http://www.khronos.org/opengles/sdk/docs/man31/glVertexBindingDivisor.xml}{glVertexBindingDivisor()}.
*/

/*!
    \fn bool QOpenGLExtraFunctions::isInitialized(const QOpenGLExtraFunctionsPrivate *d)
    \internal
*/

// Functions part of the OpenGL ES 3.0+ standard need special handling. These, just like
// the 2.0 functions, are not guaranteed to be resolvable via eglGetProcAddress or
// similar. (we cannot count on EGL_KHR_(client_)get_all_proc_addresses being available)

// Calling them directly is, unlike the 2.0 functions, not feasible because one may build
// the binaries on a GLES3-capable system and then deploy on a GLES2-only system that does
// not have these symbols, and vice versa. Until ES3 becomes universally available, they
// have to be dlsym'ed.

Q_GLOBAL_STATIC(QOpenGLES3Helper, qgles3Helper)

bool QOpenGLES3Helper::init()
{
#ifdef QT_NO_LIBRARY
    return false;
#elif !defined(Q_OS_IOS)
# ifdef Q_OS_WIN
#  ifndef QT_DEBUG
    m_gl.setFileName(QStringLiteral("libGLESv2"));
#  else
    m_gl.setFileName(QStringLiteral("libGLESv2d"));
#  endif
# else
#  ifdef Q_OS_ANDROID
    m_gl.setFileName(QStringLiteral("GLESv2"));
#  else
    m_gl.setFileNameAndVersion(QStringLiteral("GLESv2"), 2);
#  endif
# endif // Q_OS_WIN
    return m_gl.load();
#else
    return true;
#endif // Q_OS_IOS
}

QFunctionPointer QOpenGLES3Helper::resolve(const char *name)
{
#ifdef Q_OS_IOS
    return QFunctionPointer(dlsym(RTLD_DEFAULT, name));
#elif !defined(QT_NO_LIBRARY)
    return m_gl.resolve(name);
#else
    Q_UNUSED(name);
    return 0;
#endif
}

QOpenGLES3Helper::QOpenGLES3Helper()
{
    m_supportedVersion = qMakePair(2, 0);

    if (init()) {
        const QPair<int, int> contextVersion = QOpenGLContext::currentContext()->format().version();

        qCDebug(lcGLES3, "Resolving OpenGL ES 3.0 entry points");

        BeginQuery = (void (QOPENGLF_APIENTRYP) (GLenum, GLuint)) resolve("glBeginQuery");
        BeginTransformFeedback = (void (QOPENGLF_APIENTRYP) (GLenum)) resolve("glBeginTransformFeedback");
        BindBufferBase = (void (QOPENGLF_APIENTRYP) (GLenum, GLuint, GLuint)) resolve("glBindBufferBase");
        BindBufferRange = (void (QOPENGLF_APIENTRYP) (GLenum, GLuint, GLuint, GLintptr, GLsizeiptr)) resolve("glBindBufferRange");
        BindSampler = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint)) resolve("glBindSampler");
        BindTransformFeedback = (void (QOPENGLF_APIENTRYP) (GLenum, GLuint)) resolve("glBindTransformFeedback");
        BindVertexArray = (void (QOPENGLF_APIENTRYP) (GLuint)) resolve("glBindVertexArray");
        BlitFramebuffer = (void (QOPENGLF_APIENTRYP) (GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum)) resolve("glBlitFramebuffer");
        ClearBufferfi = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, GLfloat, GLint)) resolve("glClearBufferfi");
        ClearBufferfv = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, const GLfloat *)) resolve("glClearBufferfv");
        ClearBufferiv = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, const GLint *)) resolve("glClearBufferiv");
        ClearBufferuiv = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, const GLuint *)) resolve("glClearBufferuiv");
        ClientWaitSync = (GLenum (QOPENGLF_APIENTRYP) (GLsync, GLbitfield, GLuint64)) resolve("glClientWaitSync");
        CompressedTexImage3D = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const void *)) resolve("glCompressedTexImage3D");
        CompressedTexSubImage3D = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const void *)) resolve("glCompressedTexSubImage3D");
        CopyBufferSubData = (void (QOPENGLF_APIENTRYP) (GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr)) resolve("glCopyBufferSubData");
        CopyTexSubImage3D = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)) resolve("glCopyTexSubImage3D");
        DeleteQueries = (void (QOPENGLF_APIENTRYP) (GLsizei, const GLuint *)) resolve("glDeleteQueries");
        DeleteSamplers = (void (QOPENGLF_APIENTRYP) (GLsizei, const GLuint *)) resolve("glDeleteSamplers");
        DeleteSync = (void (QOPENGLF_APIENTRYP) (GLsync)) resolve("glDeleteSync");
        DeleteTransformFeedbacks = (void (QOPENGLF_APIENTRYP) (GLsizei, const GLuint *)) resolve("glDeleteTransformFeedbacks");
        DeleteVertexArrays = (void (QOPENGLF_APIENTRYP) (GLsizei, const GLuint *)) resolve("glDeleteVertexArrays");
        DrawArraysInstanced = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, GLsizei, GLsizei)) resolve("glDrawArraysInstanced");
        DrawBuffers = (void (QOPENGLF_APIENTRYP) (GLsizei, const GLenum *)) resolve("glDrawBuffers");
        DrawElementsInstanced = (void (QOPENGLF_APIENTRYP) (GLenum, GLsizei, GLenum, const void *, GLsizei)) resolve("glDrawElementsInstanced");
        DrawRangeElements = (void (QOPENGLF_APIENTRYP) (GLenum, GLuint, GLuint, GLsizei, GLenum, const void *)) resolve("glDrawRangeElements");
        EndQuery = (void (QOPENGLF_APIENTRYP) (GLenum)) resolve("glEndQuery");
        EndTransformFeedback = (void (QOPENGLF_APIENTRYP) ()) resolve("glEndTransformFeedback");
        FenceSync = (GLsync (QOPENGLF_APIENTRYP) (GLenum, GLbitfield)) resolve("glFenceSync");
        FlushMappedBufferRange = (void (QOPENGLF_APIENTRYP) (GLenum, GLintptr, GLsizeiptr)) resolve("glFlushMappedBufferRange");
        FramebufferTextureLayer = (void (QOPENGLF_APIENTRYP) (GLenum, GLenum, GLuint, GLint, GLint)) resolve("glFramebufferTextureLayer");
        GenQueries = (void (QOPENGLF_APIENTRYP) (GLsizei, GLuint*)) resolve("glGenQueries");
        GenSamplers = (void (QOPENGLF_APIENTRYP) (GLsizei, GLuint*)) resolve("glGenSamplers");
        GenTransformFeedbacks = (void (QOPENGLF_APIENTRYP) (GLsizei, GLuint*)) resolve("glGenTransformFeedbacks");
        GenVertexArrays = (void (QOPENGLF_APIENTRYP) (GLsizei, GLuint*)) resolve("glGenVertexArrays");
        GetActiveUniformBlockName = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint, GLsizei, GLsizei*, GLchar*)) resolve("glGetActiveUniformBlockName");
        GetActiveUniformBlockiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint, GLenum, GLint*)) resolve("glGetActiveUniformBlockiv");
        GetActiveUniformsiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLsizei, const GLuint *, GLenum, GLint*)) resolve("glGetActiveUniformsiv");
        GetBufferParameteri64v = (void (QOPENGLF_APIENTRYP) (GLenum, GLenum, GLint64*)) resolve("glGetBufferParameteri64v");
        GetBufferPointerv = (void (QOPENGLF_APIENTRYP) (GLenum, GLenum, void **)) resolve("glGetBufferPointerv");
        GetFragDataLocation = (GLint (QOPENGLF_APIENTRYP) (GLuint, const GLchar *)) resolve("glGetFragDataLocation");
        GetInteger64i_v = (void (QOPENGLF_APIENTRYP) (GLenum, GLuint, GLint64*)) resolve("glGetInteger64i_v");
        GetInteger64v = (void (QOPENGLF_APIENTRYP) (GLenum, GLint64*)) resolve("glGetInteger64v");
        GetIntegeri_v = (void (QOPENGLF_APIENTRYP) (GLenum, GLuint, GLint*)) resolve("glGetIntegeri_v");
        GetInternalformativ = (void (QOPENGLF_APIENTRYP) (GLenum, GLenum, GLenum, GLsizei, GLint*)) resolve("glGetInternalformativ");
        GetProgramBinary = (void (QOPENGLF_APIENTRYP) (GLuint, GLsizei, GLsizei*, GLenum*, void *)) resolve("glGetProgramBinary");
        GetQueryObjectuiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLuint*)) resolve("glGetQueryObjectuiv");
        GetQueryiv = (void (QOPENGLF_APIENTRYP) (GLenum, GLenum, GLint*)) resolve("glGetQueryiv");
        GetSamplerParameterfv = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLfloat*)) resolve("glGetSamplerParameterfv");
        GetSamplerParameteriv = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLint*)) resolve("glGetSamplerParameteriv");
        GetStringi = (const GLubyte * (QOPENGLF_APIENTRYP) (GLenum, GLuint)) resolve("glGetStringi");
        GetSynciv = (void (QOPENGLF_APIENTRYP) (GLsync, GLenum, GLsizei, GLsizei*, GLint*)) resolve("glGetSynciv");
        GetTransformFeedbackVarying = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint, GLsizei, GLsizei*, GLsizei*, GLenum*, GLchar*)) resolve("glGetTransformFeedbackVarying");
        GetUniformBlockIndex = (GLuint (QOPENGLF_APIENTRYP) (GLuint, const GLchar *)) resolve("glGetUniformBlockIndex");
        GetUniformIndices = (void (QOPENGLF_APIENTRYP) (GLuint, GLsizei, const GLchar *const*, GLuint*)) resolve("glGetUniformIndices");
        GetUniformuiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLuint*)) resolve("glGetUniformuiv");
        GetVertexAttribIiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLint*)) resolve("glGetVertexAttribIiv");
        GetVertexAttribIuiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLuint*)) resolve("glGetVertexAttribIuiv");
        InvalidateFramebuffer = (void (QOPENGLF_APIENTRYP) (GLenum, GLsizei, const GLenum *)) resolve("glInvalidateFramebuffer");
        InvalidateSubFramebuffer = (void (QOPENGLF_APIENTRYP) (GLenum, GLsizei, const GLenum *, GLint, GLint, GLsizei, GLsizei)) resolve("glInvalidateSubFramebuffer");
        IsQuery = (GLboolean (QOPENGLF_APIENTRYP) (GLuint)) resolve("glIsQuery");
        IsSampler = (GLboolean (QOPENGLF_APIENTRYP) (GLuint)) resolve("glIsSampler");
        IsSync = (GLboolean (QOPENGLF_APIENTRYP) (GLsync)) resolve("glIsSync");
        IsTransformFeedback = (GLboolean (QOPENGLF_APIENTRYP) (GLuint)) resolve("glIsTransformFeedback");
        IsVertexArray = (GLboolean (QOPENGLF_APIENTRYP) (GLuint)) resolve("glIsVertexArray");
        MapBufferRange = (void * (QOPENGLF_APIENTRYP) (GLenum, GLintptr, GLsizeiptr, GLbitfield)) resolve("glMapBufferRange");
        PauseTransformFeedback = (void (QOPENGLF_APIENTRYP) ()) resolve("glPauseTransformFeedback");
        ProgramBinary = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, const void *, GLsizei)) resolve("glProgramBinary");
        ProgramParameteri = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLint)) resolve("glProgramParameteri");
        ReadBuffer = (void (QOPENGLF_APIENTRYP) (GLenum)) resolve("glReadBuffer");
        RenderbufferStorageMultisample = (void (QOPENGLF_APIENTRYP) (GLenum, GLsizei, GLenum, GLsizei, GLsizei)) resolve("glRenderbufferStorageMultisample");
        ResumeTransformFeedback = (void (QOPENGLF_APIENTRYP) ()) resolve("glResumeTransformFeedback");
        SamplerParameterf = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLfloat)) resolve("glSamplerParameterf");
        SamplerParameterfv = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, const GLfloat *)) resolve("glSamplerParameterfv");
        SamplerParameteri = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLint)) resolve("glSamplerParameteri");
        SamplerParameteriv = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, const GLint *)) resolve("glSamplerParameteriv");
        TexImage3D = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *)) resolve("glTexImage3D");
        TexStorage2D = (void (QOPENGLF_APIENTRYP) (GLenum, GLsizei, GLenum, GLsizei, GLsizei)) resolve("glTexStorage2D");
        TexStorage3D = (void (QOPENGLF_APIENTRYP) (GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei)) resolve("glTexStorage3D");
        TexSubImage3D = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void *)) resolve("glTexSubImage3D");
        TransformFeedbackVaryings = (void (QOPENGLF_APIENTRYP) (GLuint, GLsizei, const GLchar *const*, GLenum)) resolve("glTransformFeedbackVaryings");
        Uniform1ui = (void (QOPENGLF_APIENTRYP) (GLint, GLuint)) resolve("glUniform1ui");
        Uniform1uiv = (void (QOPENGLF_APIENTRYP) (GLint, GLsizei, const GLuint *)) resolve("glUniform1uiv");
        Uniform2ui = (void (QOPENGLF_APIENTRYP) (GLint, GLuint, GLuint)) resolve("glUniform2ui");
        Uniform2uiv = (void (QOPENGLF_APIENTRYP) (GLint, GLsizei, const GLuint *)) resolve("glUniform2uiv");
        Uniform3ui = (void (QOPENGLF_APIENTRYP) (GLint, GLuint, GLuint, GLuint)) resolve("glUniform3ui");
        Uniform3uiv = (void (QOPENGLF_APIENTRYP) (GLint, GLsizei, const GLuint *)) resolve("glUniform3uiv");
        Uniform4ui = (void (QOPENGLF_APIENTRYP) (GLint, GLuint, GLuint, GLuint, GLuint)) resolve("glUniform4ui");
        Uniform4uiv = (void (QOPENGLF_APIENTRYP) (GLint, GLsizei, const GLuint *)) resolve("glUniform4uiv");
        UniformBlockBinding = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint, GLuint)) resolve("glUniformBlockBinding");
        UniformMatrix2x3fv = (void (QOPENGLF_APIENTRYP) (GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glUniformMatrix2x3fv");
        UniformMatrix2x4fv = (void (QOPENGLF_APIENTRYP) (GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glUniformMatrix2x4fv");
        UniformMatrix3x2fv = (void (QOPENGLF_APIENTRYP) (GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glUniformMatrix3x2fv");
        UniformMatrix3x4fv = (void (QOPENGLF_APIENTRYP) (GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glUniformMatrix3x4fv");
        UniformMatrix4x2fv = (void (QOPENGLF_APIENTRYP) (GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glUniformMatrix4x2fv");
        UniformMatrix4x3fv = (void (QOPENGLF_APIENTRYP) (GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glUniformMatrix4x3fv");
        UnmapBuffer = (GLboolean (QOPENGLF_APIENTRYP) (GLenum)) resolve("glUnmapBuffer");
        VertexAttribDivisor = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint)) resolve("glVertexAttribDivisor");
        VertexAttribI4i = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLint, GLint, GLint)) resolve("glVertexAttribI4i");
        VertexAttribI4iv = (void (QOPENGLF_APIENTRYP) (GLuint, const GLint *)) resolve("glVertexAttribI4iv");
        VertexAttribI4ui = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint, GLuint, GLuint, GLuint)) resolve("glVertexAttribI4ui");
        VertexAttribI4uiv = (void (QOPENGLF_APIENTRYP) (GLuint, const GLuint *)) resolve("glVertexAttribI4uiv");
        VertexAttribIPointer = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLenum, GLsizei, const void *)) resolve("glVertexAttribIPointer");
        WaitSync = (void (QOPENGLF_APIENTRYP) (GLsync, GLbitfield, GLuint64)) resolve("glWaitSync");

        if (!BeginQuery || !BlitFramebuffer || !GenTransformFeedbacks || !GenVertexArrays || !MapBufferRange
            || !RenderbufferStorageMultisample || !TexStorage2D || !WaitSync) {
            qWarning("OpenGL ES 3.0 entry points not found. This is odd because the driver returned a context of version %d.%d",
                     contextVersion.first, contextVersion.second);
            return;
        }
        m_supportedVersion = qMakePair(3, 0);

        if (contextVersion >= qMakePair(3, 1)) {
            qCDebug(lcGLES3, "Resolving OpenGL ES 3.1 entry points");

            ActiveShaderProgram = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint)) resolve("glActiveShaderProgram");
            BindImageTexture = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint, GLint, GLboolean, GLint, GLenum, GLenum)) resolve("glBindImageTexture");
            BindProgramPipeline = (void (QOPENGLF_APIENTRYP) (GLuint)) resolve("glBindProgramPipeline");
            BindVertexBuffer = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint, GLintptr, GLsizei)) resolve("glBindVertexBuffer");
            CreateShaderProgramv = (GLuint (QOPENGLF_APIENTRYP) (GLenum, GLsizei, const GLchar *const*)) resolve("glCreateShaderProgramv");
            DeleteProgramPipelines = (void (QOPENGLF_APIENTRYP) (GLsizei, const GLuint *)) resolve("glDeleteProgramPipelines");
            DispatchCompute = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint, GLuint)) resolve("glDispatchCompute");
            DispatchComputeIndirect = (void (QOPENGLF_APIENTRYP) (GLintptr)) resolve("glDispatchComputeIndirect");
            DrawArraysIndirect = (void (QOPENGLF_APIENTRYP) (GLenum, const void *)) resolve("glDrawArraysIndirect");
            DrawElementsIndirect = (void (QOPENGLF_APIENTRYP) (GLenum, GLenum, const void *)) resolve("glDrawElementsIndirect");
            FramebufferParameteri = (void (QOPENGLF_APIENTRYP) (GLenum, GLenum, GLint)) resolve("glFramebufferParameteri");
            GenProgramPipelines = (void (QOPENGLF_APIENTRYP) (GLsizei, GLuint*)) resolve("glGenProgramPipelines");
            GetBooleani_v = (void (QOPENGLF_APIENTRYP) (GLenum, GLuint, GLboolean*)) resolve("glGetBooleani_v");
            GetFramebufferParameteriv = (void (QOPENGLF_APIENTRYP) (GLenum, GLenum, GLint*)) resolve("glGetFramebufferParameteriv");
            GetMultisamplefv = (void (QOPENGLF_APIENTRYP) (GLenum, GLuint, GLfloat*)) resolve("glGetMultisamplefv");
            GetProgramInterfaceiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLenum, GLint*)) resolve("glGetProgramInterfaceiv");
            GetProgramPipelineInfoLog = (void (QOPENGLF_APIENTRYP) (GLuint, GLsizei, GLsizei*, GLchar*)) resolve("glGetProgramPipelineInfoLog");
            GetProgramPipelineiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLint*)) resolve("glGetProgramPipelineiv");
            GetProgramResourceIndex = (GLuint (QOPENGLF_APIENTRYP) (GLuint, GLenum, const GLchar *)) resolve("glGetProgramResourceIndex");
            GetProgramResourceLocation = (GLint (QOPENGLF_APIENTRYP) (GLuint, GLenum, const GLchar *)) resolve("glGetProgramResourceLocation");
            GetProgramResourceName = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLuint, GLsizei, GLsizei*, GLchar*)) resolve("glGetProgramResourceName");
            GetProgramResourceiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLenum, GLuint, GLsizei, const GLenum *, GLsizei, GLsizei*, GLint*)) resolve("glGetProgramResourceiv");
            GetTexLevelParameterfv = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, GLenum, GLfloat*)) resolve("glGetTexLevelParameterfv");
            GetTexLevelParameteriv = (void (QOPENGLF_APIENTRYP) (GLenum, GLint, GLenum, GLint*)) resolve("glGetTexLevelParameteriv");
            IsProgramPipeline = (GLboolean (QOPENGLF_APIENTRYP) (GLuint)) resolve("glIsProgramPipeline");
            MemoryBarrierFunc = (void (QOPENGLF_APIENTRYP) (GLbitfield)) resolve("glMemoryBarrier");
            MemoryBarrierByRegion = (void (QOPENGLF_APIENTRYP) (GLbitfield)) resolve("glMemoryBarrierByRegion");
            ProgramUniform1f = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLfloat)) resolve("glProgramUniform1f");
            ProgramUniform1fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLfloat *)) resolve("glProgramUniform1fv");
            ProgramUniform1i = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLint)) resolve("glProgramUniform1i");
            ProgramUniform1iv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLint *)) resolve("glProgramUniform1iv");
            ProgramUniform1ui = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLuint)) resolve("glProgramUniform1ui");
            ProgramUniform1uiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLuint *)) resolve("glProgramUniform1uiv");
            ProgramUniform2f = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLfloat, GLfloat)) resolve("glProgramUniform2f");
            ProgramUniform2fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLfloat *)) resolve("glProgramUniform2fv");
            ProgramUniform2i = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLint, GLint)) resolve("glProgramUniform2i");
            ProgramUniform2iv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLint *)) resolve("glProgramUniform2iv");
            ProgramUniform2ui = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLuint, GLuint)) resolve("glProgramUniform2ui");
            ProgramUniform2uiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLuint *)) resolve("glProgramUniform2uiv");
            ProgramUniform3f = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLfloat, GLfloat, GLfloat)) resolve("glProgramUniform3f");
            ProgramUniform3fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLfloat *)) resolve("glProgramUniform3fv");
            ProgramUniform3i = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLint, GLint, GLint)) resolve("glProgramUniform3i");
            ProgramUniform3iv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLint *)) resolve("glProgramUniform3iv");
            ProgramUniform3ui = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLuint, GLuint, GLuint)) resolve("glProgramUniform3ui");
            ProgramUniform3uiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLuint *)) resolve("glProgramUniform3uiv");
            ProgramUniform4f = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLfloat, GLfloat, GLfloat, GLfloat)) resolve("glProgramUniform4f");
            ProgramUniform4fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLfloat *)) resolve("glProgramUniform4fv");
            ProgramUniform4i = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLint, GLint, GLint, GLint)) resolve("glProgramUniform4i");
            ProgramUniform4iv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLint *)) resolve("glProgramUniform4iv");
            ProgramUniform4ui = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLuint, GLuint, GLuint, GLuint)) resolve("glProgramUniform4ui");
            ProgramUniform4uiv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, const GLuint *)) resolve("glProgramUniform4uiv");
            ProgramUniformMatrix2fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glProgramUniformMatrix2fv");
            ProgramUniformMatrix2x3fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glProgramUniformMatrix2x3fv");
            ProgramUniformMatrix2x4fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glProgramUniformMatrix2x4fv");
            ProgramUniformMatrix3fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glProgramUniformMatrix3fv");
            ProgramUniformMatrix3x2fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glProgramUniformMatrix3x2fv");
            ProgramUniformMatrix3x4fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glProgramUniformMatrix3x4fv");
            ProgramUniformMatrix4fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glProgramUniformMatrix4fv");
            ProgramUniformMatrix4x2fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glProgramUniformMatrix4x2fv");
            ProgramUniformMatrix4x3fv = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLsizei, GLboolean, const GLfloat *)) resolve("glProgramUniformMatrix4x3fv");
            SampleMaski = (void (QOPENGLF_APIENTRYP) (GLuint, GLbitfield)) resolve("glSampleMaski");
            TexStorage2DMultisample = (void (QOPENGLF_APIENTRYP) (GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLboolean)) resolve("glTexStorage2DMultisample");
            UseProgramStages = (void (QOPENGLF_APIENTRYP) (GLuint, GLbitfield, GLuint)) resolve("glUseProgramStages");
            ValidateProgramPipeline = (void (QOPENGLF_APIENTRYP) (GLuint)) resolve("glValidateProgramPipeline");
            VertexAttribBinding = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint)) resolve("glVertexAttribBinding");
            VertexAttribFormat = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLenum, GLboolean, GLuint)) resolve("glVertexAttribFormat");
            VertexAttribIFormat = (void (QOPENGLF_APIENTRYP) (GLuint, GLint, GLenum, GLuint)) resolve("glVertexAttribIFormat");
            VertexBindingDivisor = (void (QOPENGLF_APIENTRYP) (GLuint, GLuint)) resolve("glVertexBindingDivisor");

            if (!ActiveShaderProgram || !BindImageTexture || !DispatchCompute || !DrawArraysIndirect
                || !GenProgramPipelines || !MemoryBarrierFunc) {
                qWarning("OpenGL ES 3.1 entry points not found. This is odd because the driver returned a context of version %d.%d",
                         contextVersion.first, contextVersion.second);
                return;
            }
            m_supportedVersion = qMakePair(3, 1);
        }
    } else {
        qFatal("Failed to load libGLESv2");
    }
}

// GLES 3.0 and 3.1

// Checks for true OpenGL ES 3.x. OpenGL with GL_ARB_ES3_compatibility
// does not count because there the plain resolvers work anyhow.
static inline bool isES3(int minor)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();

    const bool libMatches = QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES;
    const bool contextMatches = ctx->isOpenGLES() && ctx->format().version() >= qMakePair(3, minor);

    // Resolving happens whenever qgles3Helper() is called first. So do it only
    // when the driver gives a 3.0+ context.
    if (libMatches && contextMatches)
        return qgles3Helper()->supportedVersion() >= qMakePair(3, minor);

    return false;
}

// Go through the dlsym-based helper for real ES 3, resolve using
// wglGetProcAddress or similar when on plain OpenGL.

static void QOPENGLF_APIENTRY qopenglfResolveBeginQuery(GLenum target, GLuint id)
{
    if (isES3(0))
        qgles3Helper()->BeginQuery(target, id);
    else
        RESOLVE_FUNC_VOID(0, BeginQuery)(target, id);
}

static void QOPENGLF_APIENTRY qopenglfResolveBeginTransformFeedback(GLenum primitiveMode)
{
    if (isES3(0))
        qgles3Helper()->BeginTransformFeedback(primitiveMode);
    else
        RESOLVE_FUNC_VOID(0, BeginTransformFeedback)(primitiveMode);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
    if (isES3(0))
        qgles3Helper()->BindBufferBase(target, index, buffer);
    else
        RESOLVE_FUNC_VOID(0, BindBufferBase)(target, index, buffer);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
    if (isES3(0))
        qgles3Helper()->BindBufferRange(target, index, buffer, offset, size);
    else
        RESOLVE_FUNC_VOID(0, BindBufferRange)(target, index, buffer, offset, size);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindSampler(GLuint unit, GLuint sampler)
{
    if (isES3(0))
        qgles3Helper()->BindSampler(unit, sampler);
    else
        RESOLVE_FUNC_VOID(0, BindSampler)(unit, sampler);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindTransformFeedback(GLenum target, GLuint id)
{
    if (isES3(0))
        qgles3Helper()->BindTransformFeedback(target, id);
    else
        RESOLVE_FUNC_VOID(0, BindTransformFeedback)(target, id);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindVertexArray(GLuint array)
{
    if (isES3(0))
        qgles3Helper()->BindVertexArray(array);
    else
        RESOLVE_FUNC_VOID(0, BindVertexArray)(array);
}

static void QOPENGLF_APIENTRY qopenglfResolveBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    if (isES3(0))
        qgles3Helper()->BlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    else
        RESOLVE_FUNC_VOID(ResolveEXT | ResolveANGLE | ResolveNV, BlitFramebuffer)
            (srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

static void QOPENGLF_APIENTRY qopenglfResolveClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
    if (isES3(0))
        qgles3Helper()->ClearBufferfi(buffer, drawbuffer, depth, stencil);
    else
        RESOLVE_FUNC_VOID(0, ClearBufferfi)(buffer, drawbuffer, depth, stencil);
}

static void QOPENGLF_APIENTRY qopenglfResolveClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat * value)
{
    if (isES3(0))
        qgles3Helper()->ClearBufferfv(buffer, drawbuffer, value);
    else
        RESOLVE_FUNC_VOID(0, ClearBufferfv)(buffer, drawbuffer, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint * value)
{
    if (isES3(0))
        qgles3Helper()->ClearBufferiv(buffer, drawbuffer, value);
    else
        RESOLVE_FUNC_VOID(0, ClearBufferiv)(buffer, drawbuffer, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint * value)
{
    if (isES3(0))
        qgles3Helper()->ClearBufferuiv(buffer, drawbuffer, value);
    else
        RESOLVE_FUNC_VOID(0, ClearBufferuiv)(buffer, drawbuffer, value);
}

static GLenum QOPENGLF_APIENTRY qopenglfResolveClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    if (isES3(0))
        return qgles3Helper()->ClientWaitSync(sync, flags, timeout);
    else
        RESOLVE_FUNC(GLenum, 0, ClientWaitSync)(sync, flags, timeout);
}

static void QOPENGLF_APIENTRY qopenglfResolveCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data)
{
    if (isES3(0))
        qgles3Helper()->CompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
    else
        RESOLVE_FUNC_VOID(0, CompressedTexImage3D)(target, level, internalformat, width, height, depth, border, imageSize, data);
}

static void QOPENGLF_APIENTRY qopenglfResolveCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void * data)
{
    if (isES3(0))
        qgles3Helper()->CompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    else
        RESOLVE_FUNC_VOID(0, CompressedTexSubImage3D)(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}

static void QOPENGLF_APIENTRY qopenglfResolveCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
    if (isES3(0))
        qgles3Helper()->CopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
    else
        RESOLVE_FUNC_VOID(0, CopyBufferSubData)(readTarget, writeTarget, readOffset, writeOffset, size);
}

static void QOPENGLF_APIENTRY qopenglfResolveCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (isES3(0))
        qgles3Helper()->CopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
    else
        RESOLVE_FUNC_VOID(0, CopyTexSubImage3D)(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteQueries(GLsizei n, const GLuint * ids)
{
    if (isES3(0))
        qgles3Helper()->DeleteQueries(n, ids);
    else
        RESOLVE_FUNC_VOID(0, DeleteQueries)(n, ids);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteSamplers(GLsizei count, const GLuint * samplers)
{
    if (isES3(0))
        qgles3Helper()->DeleteSamplers(count, samplers);
    else
        RESOLVE_FUNC_VOID(0, DeleteSamplers)(count, samplers);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteSync(GLsync sync)
{
    if (isES3(0))
        qgles3Helper()->DeleteSync(sync);
    else
        RESOLVE_FUNC_VOID(0, DeleteSync)(sync);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteTransformFeedbacks(GLsizei n, const GLuint * ids)
{
    if (isES3(0))
        qgles3Helper()->DeleteTransformFeedbacks(n, ids);
    else
        RESOLVE_FUNC_VOID(0, DeleteTransformFeedbacks)(n, ids);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteVertexArrays(GLsizei n, const GLuint * arrays)
{
    if (isES3(0))
        qgles3Helper()->DeleteVertexArrays(n, arrays);
    else
        RESOLVE_FUNC_VOID(0, DeleteVertexArrays)(n, arrays);
}

static void QOPENGLF_APIENTRY qopenglfResolveDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
    if (isES3(0))
        qgles3Helper()->DrawArraysInstanced(mode, first, count, instancecount);
    else
        RESOLVE_FUNC_VOID(0, DrawArraysInstanced)(mode, first, count, instancecount);
}

static void QOPENGLF_APIENTRY qopenglfResolveDrawBuffers(GLsizei n, const GLenum * bufs)
{
    if (isES3(0))
        qgles3Helper()->DrawBuffers(n, bufs);
    else
        RESOLVE_FUNC_VOID(0, DrawBuffers)(n, bufs);
}

static void QOPENGLF_APIENTRY qopenglfResolveDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount)
{
    if (isES3(0))
        qgles3Helper()->DrawElementsInstanced(mode, count, type, indices, instancecount);
    else
        RESOLVE_FUNC_VOID(0, DrawElementsInstanced)(mode, count, type, indices, instancecount);
}

static void QOPENGLF_APIENTRY qopenglfResolveDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void * indices)
{
    if (isES3(0))
        qgles3Helper()->DrawRangeElements(mode, start, end, count, type, indices);
    else
        RESOLVE_FUNC_VOID(0, DrawRangeElements)(mode, start, end, count, type, indices);
}

static void QOPENGLF_APIENTRY qopenglfResolveEndQuery(GLenum target)
{
    if (isES3(0))
        qgles3Helper()->EndQuery(target);
    else
        RESOLVE_FUNC_VOID(0, EndQuery)(target);
}

static void QOPENGLF_APIENTRY qopenglfResolveEndTransformFeedback()
{
    if (isES3(0))
        qgles3Helper()->EndTransformFeedback();
    else
        RESOLVE_FUNC_VOID(0, EndTransformFeedback)();
}

static GLsync QOPENGLF_APIENTRY qopenglfResolveFenceSync(GLenum condition, GLbitfield flags)
{
    if (isES3(0))
        return qgles3Helper()->FenceSync(condition, flags);
    else
        RESOLVE_FUNC(GLsync, 0, FenceSync)(condition, flags);
}

static void QOPENGLF_APIENTRY qopenglfResolveFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
{
    if (isES3(0))
        qgles3Helper()->FlushMappedBufferRange(target, offset, length);
    else
        RESOLVE_FUNC_VOID(0, FlushMappedBufferRange)(target, offset, length);
}

static void QOPENGLF_APIENTRY qopenglfResolveFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    if (isES3(0))
        qgles3Helper()->FramebufferTextureLayer(target, attachment, texture, level, layer);
    else
        RESOLVE_FUNC_VOID(0, FramebufferTextureLayer)(target, attachment, texture, level, layer);
}

static void QOPENGLF_APIENTRY qopenglfResolveGenQueries(GLsizei n, GLuint* ids)
{
    if (isES3(0))
        qgles3Helper()->GenQueries(n, ids);
    else
        RESOLVE_FUNC_VOID(0, GenQueries)(n, ids);
}

static void QOPENGLF_APIENTRY qopenglfResolveGenSamplers(GLsizei count, GLuint* samplers)
{
    if (isES3(0))
        qgles3Helper()->GenSamplers(count, samplers);
    else
        RESOLVE_FUNC_VOID(0, GenSamplers)(count, samplers);
}

static void QOPENGLF_APIENTRY qopenglfResolveGenTransformFeedbacks(GLsizei n, GLuint* ids)
{
    if (isES3(0))
        qgles3Helper()->GenTransformFeedbacks(n, ids);
    else
        RESOLVE_FUNC_VOID(0, GenTransformFeedbacks)(n, ids);
}

static void QOPENGLF_APIENTRY qopenglfResolveGenVertexArrays(GLsizei n, GLuint* arrays)
{
    if (isES3(0))
        qgles3Helper()->GenVertexArrays(n, arrays);
    else
        RESOLVE_FUNC_VOID(0, GenVertexArrays)(n, arrays);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName)
{
    if (isES3(0))
        qgles3Helper()->GetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
    else
        RESOLVE_FUNC_VOID(0, GetActiveUniformBlockName)(program, uniformBlockIndex, bufSize, length, uniformBlockName);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params)
{
    if (isES3(0))
        qgles3Helper()->GetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetActiveUniformBlockiv)(program, uniformBlockIndex, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint* params)
{
    if (isES3(0))
        qgles3Helper()->GetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetActiveUniformsiv)(program, uniformCount, uniformIndices, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetBufferParameteri64v(GLenum target, GLenum pname, GLint64* params)
{
    if (isES3(0))
        qgles3Helper()->GetBufferParameteri64v(target, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetBufferParameteri64v)(target, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetBufferPointerv(GLenum target, GLenum pname, void ** params)
{
    if (isES3(0))
        qgles3Helper()->GetBufferPointerv(target, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetBufferPointerv)(target, pname, params);
}

static GLint QOPENGLF_APIENTRY qopenglfResolveGetFragDataLocation(GLuint program, const GLchar * name)
{
    if (isES3(0))
        return qgles3Helper()->GetFragDataLocation(program, name);
    else
        RESOLVE_FUNC(GLint, 0, GetFragDataLocation)(program, name);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetInteger64i_v(GLenum target, GLuint index, GLint64* data)
{
    if (isES3(0))
        qgles3Helper()->GetInteger64i_v(target, index, data);
    else
        RESOLVE_FUNC_VOID(0, GetInteger64i_v)(target, index, data);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetInteger64v(GLenum pname, GLint64* data)
{
    if (isES3(0))
        qgles3Helper()->GetInteger64v(pname, data);
    else
        RESOLVE_FUNC_VOID(0, GetInteger64v)(pname, data);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetIntegeri_v(GLenum target, GLuint index, GLint* data)
{
    if (isES3(0))
        qgles3Helper()->GetIntegeri_v(target, index, data);
    else
        RESOLVE_FUNC_VOID(0, GetIntegeri_v)(target, index, data);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint* params)
{
    if (isES3(0))
        qgles3Helper()->GetInternalformativ(target, internalformat, pname, bufSize, params);
    else
        RESOLVE_FUNC_VOID(0, GetInternalformativ)(target, internalformat, pname, bufSize, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void * binary)
{
    if (isES3(0))
        qgles3Helper()->GetProgramBinary(program, bufSize, length, binaryFormat, binary);
    else
        RESOLVE_FUNC_VOID(0, GetProgramBinary)(program, bufSize, length, binaryFormat, binary);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params)
{
    if (isES3(0))
        qgles3Helper()->GetQueryObjectuiv(id, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetQueryObjectuiv)(id, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetQueryiv(GLenum target, GLenum pname, GLint* params)
{
    if (isES3(0))
        qgles3Helper()->GetQueryiv(target, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetQueryiv)(target, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat* params)
{
    if (isES3(0))
        qgles3Helper()->GetSamplerParameterfv(sampler, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetSamplerParameterfv)(sampler, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params)
{
    if (isES3(0))
        qgles3Helper()->GetSamplerParameteriv(sampler, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetSamplerParameteriv)(sampler, pname, params);
}

static const GLubyte * QOPENGLF_APIENTRY qopenglfResolveGetStringi(GLenum name, GLuint index)
{
    if (isES3(0))
        return qgles3Helper()->GetStringi(name, index);
    else
        RESOLVE_FUNC(const GLubyte *, 0, GetStringi)(name, index);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values)
{
    if (isES3(0))
        qgles3Helper()->GetSynciv(sync, pname, bufSize, length, values);
    else
        RESOLVE_FUNC_VOID(0, GetSynciv)(sync, pname, bufSize, length, values);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name)
{
    if (isES3(0))
        qgles3Helper()->GetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
    else
        RESOLVE_FUNC_VOID(0, GetTransformFeedbackVarying)(program, index, bufSize, length, size, type, name);
}

static GLuint QOPENGLF_APIENTRY qopenglfResolveGetUniformBlockIndex(GLuint program, const GLchar * uniformBlockName)
{
    if (isES3(0))
        return qgles3Helper()->GetUniformBlockIndex(program, uniformBlockName);
    else
        RESOLVE_FUNC(GLuint, 0, GetUniformBlockIndex)(program, uniformBlockName);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const* uniformNames, GLuint* uniformIndices)
{
    if (isES3(0))
        qgles3Helper()->GetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
    else
        RESOLVE_FUNC_VOID(0, GetUniformIndices)(program, uniformCount, uniformNames, uniformIndices);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetUniformuiv(GLuint program, GLint location, GLuint* params)
{
    if (isES3(0))
        qgles3Helper()->GetUniformuiv(program, location, params);
    else
        RESOLVE_FUNC_VOID(0, GetUniformuiv)(program, location, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetVertexAttribIiv(GLuint index, GLenum pname, GLint* params)
{
    if (isES3(0))
        qgles3Helper()->GetVertexAttribIiv(index, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetVertexAttribIiv)(index, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint* params)
{
    if (isES3(0))
        qgles3Helper()->GetVertexAttribIuiv(index, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetVertexAttribIuiv)(index, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments)
{
    if (isES3(0))
        qgles3Helper()->InvalidateFramebuffer(target, numAttachments, attachments);
    else
        RESOLVE_FUNC_VOID(0, InvalidateFramebuffer)(target, numAttachments, attachments);
}

static void QOPENGLF_APIENTRY qopenglfResolveInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (isES3(0))
        qgles3Helper()->InvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
    else
        RESOLVE_FUNC_VOID(0, InvalidateSubFramebuffer)(target, numAttachments, attachments, x, y, width, height);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsQuery(GLuint id)
{
    if (isES3(0))
        return qgles3Helper()->IsQuery(id);
    else
        RESOLVE_FUNC(GLboolean, 0, IsQuery)(id);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsSampler(GLuint sampler)
{
    if (isES3(0))
        return qgles3Helper()->IsSampler(sampler);
    else
        RESOLVE_FUNC(GLboolean, 0, IsSampler)(sampler);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsSync(GLsync sync)
{
    if (isES3(0))
        return qgles3Helper()->IsSync(sync);
    else
        RESOLVE_FUNC(GLboolean, 0, IsSync)(sync);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsTransformFeedback(GLuint id)
{
    if (isES3(0))
        return qgles3Helper()->IsTransformFeedback(id);
    else
        RESOLVE_FUNC(GLboolean, 0, IsTransformFeedback)(id);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsVertexArray(GLuint array)
{
    if (isES3(0))
        return qgles3Helper()->IsVertexArray(array);
    else
        RESOLVE_FUNC(GLboolean, 0, IsVertexArray)(array);
}

static void * QOPENGLF_APIENTRY qopenglfResolveMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    if (isES3(0))
        return qgles3Helper()->MapBufferRange(target, offset, length, access);
    else
        RESOLVE_FUNC(void *, 0, MapBufferRange)(target, offset, length, access);
}

static void QOPENGLF_APIENTRY qopenglfResolvePauseTransformFeedback()
{
    if (isES3(0))
        qgles3Helper()->PauseTransformFeedback();
    else
        RESOLVE_FUNC_VOID(0, PauseTransformFeedback)();
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramBinary(GLuint program, GLenum binaryFormat, const void * binary, GLsizei length)
{
    if (isES3(0))
        qgles3Helper()->ProgramBinary(program, binaryFormat, binary, length);
    else
        RESOLVE_FUNC_VOID(0, ProgramBinary)(program, binaryFormat, binary, length);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramParameteri(GLuint program, GLenum pname, GLint value)
{
    if (isES3(0))
        qgles3Helper()->ProgramParameteri(program, pname, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramParameteri)(program, pname, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveReadBuffer(GLenum src)
{
    if (isES3(0))
        qgles3Helper()->ReadBuffer(src);
    else
        RESOLVE_FUNC_VOID(0, ReadBuffer)(src);
}

static void QOPENGLF_APIENTRY qopenglfResolveRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    if (isES3(0))
        qgles3Helper()->RenderbufferStorageMultisample(target, samples, internalformat, width, height);
    else
        RESOLVE_FUNC_VOID(ResolveEXT | ResolveANGLE | ResolveNV, RenderbufferStorageMultisample)
            (target, samples, internalformat, width, height);
}

static void QOPENGLF_APIENTRY qopenglfResolveResumeTransformFeedback()
{
    if (isES3(0))
        qgles3Helper()->ResumeTransformFeedback();
    else
        RESOLVE_FUNC_VOID(0, ResumeTransformFeedback)();
}

static void QOPENGLF_APIENTRY qopenglfResolveSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)
{
    if (isES3(0))
        qgles3Helper()->SamplerParameterf(sampler, pname, param);
    else
        RESOLVE_FUNC_VOID(0, SamplerParameterf)(sampler, pname, param);
}

static void QOPENGLF_APIENTRY qopenglfResolveSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat * param)
{
    if (isES3(0))
        qgles3Helper()->SamplerParameterfv(sampler, pname, param);
    else
        RESOLVE_FUNC_VOID(0, SamplerParameterfv)(sampler, pname, param);
}

static void QOPENGLF_APIENTRY qopenglfResolveSamplerParameteri(GLuint sampler, GLenum pname, GLint param)
{
    if (isES3(0))
        qgles3Helper()->SamplerParameteri(sampler, pname, param);
    else
        RESOLVE_FUNC_VOID(0, SamplerParameteri)(sampler, pname, param);
}

static void QOPENGLF_APIENTRY qopenglfResolveSamplerParameteriv(GLuint sampler, GLenum pname, const GLint * param)
{
    if (isES3(0))
        qgles3Helper()->SamplerParameteriv(sampler, pname, param);
    else
        RESOLVE_FUNC_VOID(0, SamplerParameteriv)(sampler, pname, param);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels)
{
    if (isES3(0))
        qgles3Helper()->TexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
    else
        RESOLVE_FUNC_VOID(0, TexImage3D)(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    if (isES3(0))
        qgles3Helper()->TexStorage2D(target, levels, internalformat, width, height);
    else
        RESOLVE_FUNC_VOID(0, TexStorage2D)(target, levels, internalformat, width, height);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    if (isES3(0))
        qgles3Helper()->TexStorage3D(target, levels, internalformat, width, height, depth);
    else
        RESOLVE_FUNC_VOID(0, TexStorage3D)(target, levels, internalformat, width, height, depth);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels)
{
    if (isES3(0))
        qgles3Helper()->TexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    else
        RESOLVE_FUNC_VOID(0, TexSubImage3D)(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

static void QOPENGLF_APIENTRY qopenglfResolveTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const* varyings, GLenum bufferMode)
{
    if (isES3(0))
        qgles3Helper()->TransformFeedbackVaryings(program, count, varyings, bufferMode);
    else
        RESOLVE_FUNC_VOID(0, TransformFeedbackVaryings)(program, count, varyings, bufferMode);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform1ui(GLint location, GLuint v0)
{
    if (isES3(0))
        qgles3Helper()->Uniform1ui(location, v0);
    else
        RESOLVE_FUNC_VOID(0, Uniform1ui)(location, v0);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform1uiv(GLint location, GLsizei count, const GLuint * value)
{
    if (isES3(0))
        qgles3Helper()->Uniform1uiv(location, count, value);
    else
        RESOLVE_FUNC_VOID(0, Uniform1uiv)(location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform2ui(GLint location, GLuint v0, GLuint v1)
{
    if (isES3(0))
        qgles3Helper()->Uniform2ui(location, v0, v1);
    else
        RESOLVE_FUNC_VOID(0, Uniform2ui)(location, v0, v1);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform2uiv(GLint location, GLsizei count, const GLuint * value)
{
    if (isES3(0))
        qgles3Helper()->Uniform2uiv(location, count, value);
    else
        RESOLVE_FUNC_VOID(0, Uniform2uiv)(location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)
{
    if (isES3(0))
        qgles3Helper()->Uniform3ui(location, v0, v1, v2);
    else
        RESOLVE_FUNC_VOID(0, Uniform3ui)(location, v0, v1, v2);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform3uiv(GLint location, GLsizei count, const GLuint * value)
{
    if (isES3(0))
        qgles3Helper()->Uniform3uiv(location, count, value);
    else
        RESOLVE_FUNC_VOID(0, Uniform3uiv)(location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
    if (isES3(0))
        qgles3Helper()->Uniform4ui(location, v0, v1, v2, v3);
    else
        RESOLVE_FUNC_VOID(0, Uniform4ui)(location, v0, v1, v2, v3);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniform4uiv(GLint location, GLsizei count, const GLuint * value)
{
    if (isES3(0))
        qgles3Helper()->Uniform4uiv(location, count, value);
    else
        RESOLVE_FUNC_VOID(0, Uniform4uiv)(location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    if (isES3(0))
        qgles3Helper()->UniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
    else
        RESOLVE_FUNC_VOID(0, UniformBlockBinding)(program, uniformBlockIndex, uniformBlockBinding);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(0))
        qgles3Helper()->UniformMatrix2x3fv(location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, UniformMatrix2x3fv)(location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(0))
        qgles3Helper()->UniformMatrix2x4fv(location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, UniformMatrix2x4fv)(location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(0))
        qgles3Helper()->UniformMatrix3x2fv(location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, UniformMatrix3x2fv)(location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(0))
        qgles3Helper()->UniformMatrix3x4fv(location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, UniformMatrix3x4fv)(location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(0))
        qgles3Helper()->UniformMatrix4x2fv(location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, UniformMatrix4x2fv)(location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(0))
        qgles3Helper()->UniformMatrix4x3fv(location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, UniformMatrix4x3fv)(location, count, transpose, value);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveUnmapBuffer(GLenum target)
{
    if (isES3(0))
        return qgles3Helper()->UnmapBuffer(target);
    else
        RESOLVE_FUNC(GLboolean, ResolveOES, UnmapBuffer)(target);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttribDivisor(GLuint index, GLuint divisor)
{
    if (isES3(0))
        qgles3Helper()->VertexAttribDivisor(index, divisor);
    else
        RESOLVE_FUNC_VOID(0, VertexAttribDivisor)(index, divisor);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w)
{
    if (isES3(0))
        qgles3Helper()->VertexAttribI4i(index, x, y, z, w);
    else
        RESOLVE_FUNC_VOID(0, VertexAttribI4i)(index, x, y, z, w);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttribI4iv(GLuint index, const GLint * v)
{
    if (isES3(0))
        qgles3Helper()->VertexAttribI4iv(index, v);
    else
        RESOLVE_FUNC_VOID(0, VertexAttribI4iv)(index, v);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)
{
    if (isES3(0))
        qgles3Helper()->VertexAttribI4ui(index, x, y, z, w);
    else
        RESOLVE_FUNC_VOID(0, VertexAttribI4ui)(index, x, y, z, w);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttribI4uiv(GLuint index, const GLuint * v)
{
    if (isES3(0))
        qgles3Helper()->VertexAttribI4uiv(index, v);
    else
        RESOLVE_FUNC_VOID(0, VertexAttribI4uiv)(index, v);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer)
{
    if (isES3(0))
        qgles3Helper()->VertexAttribIPointer(index, size, type, stride, pointer);
    else
        RESOLVE_FUNC_VOID(0, VertexAttribIPointer)(index, size, type, stride, pointer);
}

static void QOPENGLF_APIENTRY qopenglfResolveWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    if (isES3(0))
        qgles3Helper()->WaitSync(sync, flags, timeout);
    else
        RESOLVE_FUNC_VOID(0, WaitSync)(sync, flags, timeout);
}

static void QOPENGLF_APIENTRY qopenglfResolveActiveShaderProgram(GLuint pipeline, GLuint program)
{
    if (isES3(1))
        qgles3Helper()->ActiveShaderProgram(pipeline, program);
    else
        RESOLVE_FUNC_VOID(0, ActiveShaderProgram)(pipeline, program);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
{
    if (isES3(1))
        qgles3Helper()->BindImageTexture(unit, texture, level, layered, layer, access, format);
    else
        RESOLVE_FUNC_VOID(0, BindImageTexture)(unit, texture, level, layered, layer, access, format);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindProgramPipeline(GLuint pipeline)
{
    if (isES3(1))
        qgles3Helper()->BindProgramPipeline(pipeline);
    else
        RESOLVE_FUNC_VOID(0, BindProgramPipeline)(pipeline);
}

static void QOPENGLF_APIENTRY qopenglfResolveBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
{
    if (isES3(1))
        qgles3Helper()->BindVertexBuffer(bindingindex, buffer, offset, stride);
    else
        RESOLVE_FUNC_VOID(0, BindVertexBuffer)(bindingindex, buffer, offset, stride);
}

static GLuint QOPENGLF_APIENTRY qopenglfResolveCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const* strings)
{
    if (isES3(1))
        return qgles3Helper()->CreateShaderProgramv(type, count, strings);
    else
        RESOLVE_FUNC(GLuint, 0, CreateShaderProgramv)(type, count, strings);
}

static void QOPENGLF_APIENTRY qopenglfResolveDeleteProgramPipelines(GLsizei n, const GLuint * pipelines)
{
    if (isES3(1))
        qgles3Helper()->DeleteProgramPipelines(n, pipelines);
    else
        RESOLVE_FUNC_VOID(0, DeleteProgramPipelines)(n, pipelines);
}

static void QOPENGLF_APIENTRY qopenglfResolveDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)
{
    if (isES3(1))
        qgles3Helper()->DispatchCompute(num_groups_x, num_groups_y, num_groups_z);
    else
        RESOLVE_FUNC_VOID(0, DispatchCompute)(num_groups_x, num_groups_y, num_groups_z);
}

static void QOPENGLF_APIENTRY qopenglfResolveDispatchComputeIndirect(GLintptr indirect)
{
    if (isES3(1))
        qgles3Helper()->DispatchComputeIndirect(indirect);
    else
        RESOLVE_FUNC_VOID(0, DispatchComputeIndirect)(indirect);
}

static void QOPENGLF_APIENTRY qopenglfResolveDrawArraysIndirect(GLenum mode, const void * indirect)
{
    if (isES3(1))
        qgles3Helper()->DrawArraysIndirect(mode, indirect);
    else
        RESOLVE_FUNC_VOID(0, DrawArraysIndirect)(mode, indirect);
}

static void QOPENGLF_APIENTRY qopenglfResolveDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect)
{
    if (isES3(1))
        qgles3Helper()->DrawElementsIndirect(mode, type, indirect);
    else
        RESOLVE_FUNC_VOID(0, DrawElementsIndirect)(mode, type, indirect);
}

static void QOPENGLF_APIENTRY qopenglfResolveFramebufferParameteri(GLenum target, GLenum pname, GLint param)
{
    if (isES3(1))
        qgles3Helper()->FramebufferParameteri(target, pname, param);
    else
        RESOLVE_FUNC_VOID(0, FramebufferParameteri)(target, pname, param);
}

static void QOPENGLF_APIENTRY qopenglfResolveGenProgramPipelines(GLsizei n, GLuint* pipelines)
{
    if (isES3(1))
        qgles3Helper()->GenProgramPipelines(n, pipelines);
    else
        RESOLVE_FUNC_VOID(0, GenProgramPipelines)(n, pipelines);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetBooleani_v(GLenum target, GLuint index, GLboolean* data)
{
    if (isES3(1))
        qgles3Helper()->GetBooleani_v(target, index, data);
    else
        RESOLVE_FUNC_VOID(0, GetBooleani_v)(target, index, data);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetFramebufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    if (isES3(1))
        qgles3Helper()->GetFramebufferParameteriv(target, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetFramebufferParameteriv)(target, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetMultisamplefv(GLenum pname, GLuint index, GLfloat* val)
{
    if (isES3(1))
        qgles3Helper()->GetMultisamplefv(pname, index, val);
    else
        RESOLVE_FUNC_VOID(0, GetMultisamplefv)(pname, index, val);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params)
{
    if (isES3(1))
        qgles3Helper()->GetProgramInterfaceiv(program, programInterface, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetProgramInterfaceiv)(program, programInterface, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
{
    if (isES3(1))
        qgles3Helper()->GetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog);
    else
        RESOLVE_FUNC_VOID(0, GetProgramPipelineInfoLog)(pipeline, bufSize, length, infoLog);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint* params)
{
    if (isES3(1))
        qgles3Helper()->GetProgramPipelineiv(pipeline, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetProgramPipelineiv)(pipeline, pname, params);
}

static GLuint QOPENGLF_APIENTRY qopenglfResolveGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar * name)
{
    if (isES3(1))
        return qgles3Helper()->GetProgramResourceIndex(program, programInterface, name);
    else
        RESOLVE_FUNC(GLuint, 0, GetProgramResourceIndex)(program, programInterface, name);
}

static GLint QOPENGLF_APIENTRY qopenglfResolveGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar * name)
{
    if (isES3(1))
        return qgles3Helper()->GetProgramResourceLocation(program, programInterface, name);
    else
        RESOLVE_FUNC(GLint, 0, GetProgramResourceLocation)(program, programInterface, name);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar* name)
{
    if (isES3(1))
        qgles3Helper()->GetProgramResourceName(program, programInterface, index, bufSize, length, name);
    else
        RESOLVE_FUNC_VOID(0, GetProgramResourceName)(program, programInterface, index, bufSize, length, name);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei bufSize, GLsizei* length, GLint* params)
{
    if (isES3(1))
        qgles3Helper()->GetProgramResourceiv(program, programInterface, index, propCount, props, bufSize, length, params);
    else
        RESOLVE_FUNC_VOID(0, GetProgramResourceiv)(program, programInterface, index, propCount, props, bufSize, length, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat* params)
{
    if (isES3(1))
        qgles3Helper()->GetTexLevelParameterfv(target, level, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetTexLevelParameterfv)(target, level, pname, params);
}

static void QOPENGLF_APIENTRY qopenglfResolveGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params)
{
    if (isES3(1))
        qgles3Helper()->GetTexLevelParameteriv(target, level, pname, params);
    else
        RESOLVE_FUNC_VOID(0, GetTexLevelParameteriv)(target, level, pname, params);
}

static GLboolean QOPENGLF_APIENTRY qopenglfResolveIsProgramPipeline(GLuint pipeline)
{
    if (isES3(1))
        return qgles3Helper()->IsProgramPipeline(pipeline);
    else
        RESOLVE_FUNC(GLboolean, 0, IsProgramPipeline)(pipeline);
}

static void QOPENGLF_APIENTRY qopenglfResolveMemoryBarrier(GLbitfield barriers)
{
    if (isES3(1))
        qgles3Helper()->MemoryBarrierFunc(barriers);
    else
        RESOLVE_FUNC_VOID(0, MemoryBarrierFunc)(barriers);
}

static void QOPENGLF_APIENTRY qopenglfResolveMemoryBarrierByRegion(GLbitfield barriers)
{
    if (isES3(1))
        qgles3Helper()->MemoryBarrierByRegion(barriers);
    else
        RESOLVE_FUNC_VOID(0, MemoryBarrierByRegion)(barriers);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform1f(GLuint program, GLint location, GLfloat v0)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform1f(program, location, v0);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform1f)(program, location, v0);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform1fv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform1fv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform1i(GLuint program, GLint location, GLint v0)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform1i(program, location, v0);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform1i)(program, location, v0);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform1iv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform1iv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform1ui(GLuint program, GLint location, GLuint v0)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform1ui(program, location, v0);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform1ui)(program, location, v0);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform1uiv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform1uiv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform2f(program, location, v0, v1);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform2f)(program, location, v0, v1);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform2fv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform2fv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform2i(program, location, v0, v1);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform2i)(program, location, v0, v1);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform2iv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform2iv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform2ui(program, location, v0, v1);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform2ui)(program, location, v0, v1);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform2uiv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform2uiv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform3f(program, location, v0, v1, v2);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform3f)(program, location, v0, v1, v2);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform3fv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform3fv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform3i(program, location, v0, v1, v2);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform3i)(program, location, v0, v1, v2);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform3iv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform3iv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform3ui(program, location, v0, v1, v2);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform3ui)(program, location, v0, v1, v2);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform3uiv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform3uiv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform4f(program, location, v0, v1, v2, v3);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform4f)(program, location, v0, v1, v2, v3);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform4fv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform4fv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform4i(program, location, v0, v1, v2, v3);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform4i)(program, location, v0, v1, v2, v3);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform4iv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform4iv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform4ui(program, location, v0, v1, v2, v3);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform4ui)(program, location, v0, v1, v2, v3);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniform4uiv(program, location, count, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniform4uiv)(program, location, count, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniformMatrix2fv(program, location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniformMatrix2fv)(program, location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniformMatrix2x3fv(program, location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniformMatrix2x3fv)(program, location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniformMatrix2x4fv(program, location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniformMatrix2x4fv)(program, location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniformMatrix3fv(program, location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniformMatrix3fv)(program, location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniformMatrix3x2fv(program, location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniformMatrix3x2fv)(program, location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniformMatrix3x4fv(program, location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniformMatrix3x4fv)(program, location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniformMatrix4fv(program, location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniformMatrix4fv)(program, location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniformMatrix4x2fv(program, location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniformMatrix4x2fv)(program, location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    if (isES3(1))
        qgles3Helper()->ProgramUniformMatrix4x3fv(program, location, count, transpose, value);
    else
        RESOLVE_FUNC_VOID(0, ProgramUniformMatrix4x3fv)(program, location, count, transpose, value);
}

static void QOPENGLF_APIENTRY qopenglfResolveSampleMaski(GLuint maskNumber, GLbitfield mask)
{
    if (isES3(1))
        qgles3Helper()->SampleMaski(maskNumber, mask);
    else
        RESOLVE_FUNC_VOID(0, SampleMaski)(maskNumber, mask);
}

static void QOPENGLF_APIENTRY qopenglfResolveTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
{
    if (isES3(1))
        qgles3Helper()->TexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
    else
        RESOLVE_FUNC_VOID(0, TexStorage2DMultisample)(target, samples, internalformat, width, height, fixedsamplelocations);
}

static void QOPENGLF_APIENTRY qopenglfResolveUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program)
{
    if (isES3(1))
        qgles3Helper()->UseProgramStages(pipeline, stages, program);
    else
        RESOLVE_FUNC_VOID(0, UseProgramStages)(pipeline, stages, program);
}

static void QOPENGLF_APIENTRY qopenglfResolveValidateProgramPipeline(GLuint pipeline)
{
    if (isES3(1))
        qgles3Helper()->ValidateProgramPipeline(pipeline);
    else
        RESOLVE_FUNC_VOID(0, ValidateProgramPipeline)(pipeline);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttribBinding(GLuint attribindex, GLuint bindingindex)
{
    if (isES3(1))
        qgles3Helper()->VertexAttribBinding(attribindex, bindingindex);
    else
        RESOLVE_FUNC_VOID(0, VertexAttribBinding)(attribindex, bindingindex);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset)
{
    if (isES3(1))
        qgles3Helper()->VertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
    else
        RESOLVE_FUNC_VOID(0, VertexAttribFormat)(attribindex, size, type, normalized, relativeoffset);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset)
{
    if (isES3(1))
        qgles3Helper()->VertexAttribIFormat(attribindex, size, type, relativeoffset);
    else
        RESOLVE_FUNC_VOID(0, VertexAttribIFormat)(attribindex, size, type, relativeoffset);
}

static void QOPENGLF_APIENTRY qopenglfResolveVertexBindingDivisor(GLuint bindingindex, GLuint divisor)
{
    if (isES3(1))
        qgles3Helper()->VertexBindingDivisor(bindingindex, divisor);
    else
        RESOLVE_FUNC_VOID(0, VertexBindingDivisor)(bindingindex, divisor);
}

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
    ReadBuffer = qopenglfResolveReadBuffer;
    DrawRangeElements = qopenglfResolveDrawRangeElements;
    TexImage3D = qopenglfResolveTexImage3D;
    TexSubImage3D = qopenglfResolveTexSubImage3D;
    CopyTexSubImage3D = qopenglfResolveCopyTexSubImage3D;
    CompressedTexImage3D = qopenglfResolveCompressedTexImage3D;
    CompressedTexSubImage3D = qopenglfResolveCompressedTexSubImage3D;
    GenQueries = qopenglfResolveGenQueries;
    DeleteQueries = qopenglfResolveDeleteQueries;
    IsQuery = qopenglfResolveIsQuery;
    BeginQuery = qopenglfResolveBeginQuery;
    EndQuery = qopenglfResolveEndQuery;
    GetQueryiv = qopenglfResolveGetQueryiv;
    GetQueryObjectuiv = qopenglfResolveGetQueryObjectuiv;
    UnmapBuffer = qopenglfResolveUnmapBuffer;
    GetBufferPointerv = qopenglfResolveGetBufferPointerv;
    DrawBuffers = qopenglfResolveDrawBuffers;
    UniformMatrix2x3fv = qopenglfResolveUniformMatrix2x3fv;
    UniformMatrix3x2fv = qopenglfResolveUniformMatrix3x2fv;
    UniformMatrix2x4fv = qopenglfResolveUniformMatrix2x4fv;
    UniformMatrix4x2fv = qopenglfResolveUniformMatrix4x2fv;
    UniformMatrix3x4fv = qopenglfResolveUniformMatrix3x4fv;
    UniformMatrix4x3fv = qopenglfResolveUniformMatrix4x3fv;
    BlitFramebuffer = qopenglfResolveBlitFramebuffer;
    RenderbufferStorageMultisample = qopenglfResolveRenderbufferStorageMultisample;
    FramebufferTextureLayer = qopenglfResolveFramebufferTextureLayer;
    MapBufferRange = qopenglfResolveMapBufferRange;
    FlushMappedBufferRange = qopenglfResolveFlushMappedBufferRange;
    BindVertexArray = qopenglfResolveBindVertexArray;
    DeleteVertexArrays = qopenglfResolveDeleteVertexArrays;
    GenVertexArrays = qopenglfResolveGenVertexArrays;
    IsVertexArray = qopenglfResolveIsVertexArray;
    GetIntegeri_v = qopenglfResolveGetIntegeri_v;
    BeginTransformFeedback = qopenglfResolveBeginTransformFeedback;
    EndTransformFeedback = qopenglfResolveEndTransformFeedback;
    BindBufferRange = qopenglfResolveBindBufferRange;
    BindBufferBase = qopenglfResolveBindBufferBase;
    TransformFeedbackVaryings = qopenglfResolveTransformFeedbackVaryings;
    GetTransformFeedbackVarying = qopenglfResolveGetTransformFeedbackVarying;
    VertexAttribIPointer = qopenglfResolveVertexAttribIPointer;
    GetVertexAttribIiv = qopenglfResolveGetVertexAttribIiv;
    GetVertexAttribIuiv = qopenglfResolveGetVertexAttribIuiv;
    VertexAttribI4i = qopenglfResolveVertexAttribI4i;
    VertexAttribI4ui = qopenglfResolveVertexAttribI4ui;
    VertexAttribI4iv = qopenglfResolveVertexAttribI4iv;
    VertexAttribI4uiv = qopenglfResolveVertexAttribI4uiv;
    GetUniformuiv = qopenglfResolveGetUniformuiv;
    GetFragDataLocation = qopenglfResolveGetFragDataLocation;
    Uniform1ui = qopenglfResolveUniform1ui;
    Uniform2ui = qopenglfResolveUniform2ui;
    Uniform3ui = qopenglfResolveUniform3ui;
    Uniform4ui = qopenglfResolveUniform4ui;
    Uniform1uiv = qopenglfResolveUniform1uiv;
    Uniform2uiv = qopenglfResolveUniform2uiv;
    Uniform3uiv = qopenglfResolveUniform3uiv;
    Uniform4uiv = qopenglfResolveUniform4uiv;
    ClearBufferiv = qopenglfResolveClearBufferiv;
    ClearBufferuiv = qopenglfResolveClearBufferuiv;
    ClearBufferfv = qopenglfResolveClearBufferfv;
    ClearBufferfi = qopenglfResolveClearBufferfi;
    GetStringi = qopenglfResolveGetStringi;
    CopyBufferSubData = qopenglfResolveCopyBufferSubData;
    GetUniformIndices = qopenglfResolveGetUniformIndices;
    GetActiveUniformsiv = qopenglfResolveGetActiveUniformsiv;
    GetUniformBlockIndex = qopenglfResolveGetUniformBlockIndex;
    GetActiveUniformBlockiv = qopenglfResolveGetActiveUniformBlockiv;
    GetActiveUniformBlockName = qopenglfResolveGetActiveUniformBlockName;
    UniformBlockBinding = qopenglfResolveUniformBlockBinding;
    DrawArraysInstanced = qopenglfResolveDrawArraysInstanced;
    DrawElementsInstanced = qopenglfResolveDrawElementsInstanced;
    FenceSync = qopenglfResolveFenceSync;
    IsSync = qopenglfResolveIsSync;
    DeleteSync = qopenglfResolveDeleteSync;
    ClientWaitSync = qopenglfResolveClientWaitSync;
    WaitSync = qopenglfResolveWaitSync;
    GetInteger64v = qopenglfResolveGetInteger64v;
    GetSynciv = qopenglfResolveGetSynciv;
    GetInteger64i_v = qopenglfResolveGetInteger64i_v;
    GetBufferParameteri64v = qopenglfResolveGetBufferParameteri64v;
    GenSamplers = qopenglfResolveGenSamplers;
    DeleteSamplers = qopenglfResolveDeleteSamplers;
    IsSampler = qopenglfResolveIsSampler;
    BindSampler = qopenglfResolveBindSampler;
    SamplerParameteri = qopenglfResolveSamplerParameteri;
    SamplerParameteriv = qopenglfResolveSamplerParameteriv;
    SamplerParameterf = qopenglfResolveSamplerParameterf;
    SamplerParameterfv = qopenglfResolveSamplerParameterfv;
    GetSamplerParameteriv = qopenglfResolveGetSamplerParameteriv;
    GetSamplerParameterfv = qopenglfResolveGetSamplerParameterfv;
    VertexAttribDivisor = qopenglfResolveVertexAttribDivisor;
    BindTransformFeedback = qopenglfResolveBindTransformFeedback;
    DeleteTransformFeedbacks = qopenglfResolveDeleteTransformFeedbacks;
    GenTransformFeedbacks = qopenglfResolveGenTransformFeedbacks;
    IsTransformFeedback = qopenglfResolveIsTransformFeedback;
    PauseTransformFeedback = qopenglfResolvePauseTransformFeedback;
    ResumeTransformFeedback = qopenglfResolveResumeTransformFeedback;
    GetProgramBinary = qopenglfResolveGetProgramBinary;
    ProgramBinary = qopenglfResolveProgramBinary;
    ProgramParameteri = qopenglfResolveProgramParameteri;
    InvalidateFramebuffer = qopenglfResolveInvalidateFramebuffer;
    InvalidateSubFramebuffer = qopenglfResolveInvalidateSubFramebuffer;
    TexStorage2D = qopenglfResolveTexStorage2D;
    TexStorage3D = qopenglfResolveTexStorage3D;
    GetInternalformativ = qopenglfResolveGetInternalformativ;

    DispatchCompute = qopenglfResolveDispatchCompute;
    DispatchComputeIndirect = qopenglfResolveDispatchComputeIndirect;
    DrawArraysIndirect = qopenglfResolveDrawArraysIndirect;
    DrawElementsIndirect = qopenglfResolveDrawElementsIndirect;
    FramebufferParameteri = qopenglfResolveFramebufferParameteri;
    GetFramebufferParameteriv = qopenglfResolveGetFramebufferParameteriv;
    GetProgramInterfaceiv = qopenglfResolveGetProgramInterfaceiv;
    GetProgramResourceIndex = qopenglfResolveGetProgramResourceIndex;
    GetProgramResourceName = qopenglfResolveGetProgramResourceName;
    GetProgramResourceiv = qopenglfResolveGetProgramResourceiv;
    GetProgramResourceLocation = qopenglfResolveGetProgramResourceLocation;
    UseProgramStages = qopenglfResolveUseProgramStages;
    ActiveShaderProgram = qopenglfResolveActiveShaderProgram;
    CreateShaderProgramv = qopenglfResolveCreateShaderProgramv;
    BindProgramPipeline = qopenglfResolveBindProgramPipeline;
    DeleteProgramPipelines = qopenglfResolveDeleteProgramPipelines;
    GenProgramPipelines = qopenglfResolveGenProgramPipelines;
    IsProgramPipeline = qopenglfResolveIsProgramPipeline;
    GetProgramPipelineiv = qopenglfResolveGetProgramPipelineiv;
    ProgramUniform1i = qopenglfResolveProgramUniform1i;
    ProgramUniform2i = qopenglfResolveProgramUniform2i;
    ProgramUniform3i = qopenglfResolveProgramUniform3i;
    ProgramUniform4i = qopenglfResolveProgramUniform4i;
    ProgramUniform1ui = qopenglfResolveProgramUniform1ui;
    ProgramUniform2ui = qopenglfResolveProgramUniform2ui;
    ProgramUniform3ui = qopenglfResolveProgramUniform3ui;
    ProgramUniform4ui = qopenglfResolveProgramUniform4ui;
    ProgramUniform1f = qopenglfResolveProgramUniform1f;
    ProgramUniform2f = qopenglfResolveProgramUniform2f;
    ProgramUniform3f = qopenglfResolveProgramUniform3f;
    ProgramUniform4f = qopenglfResolveProgramUniform4f;
    ProgramUniform1iv = qopenglfResolveProgramUniform1iv;
    ProgramUniform2iv = qopenglfResolveProgramUniform2iv;
    ProgramUniform3iv = qopenglfResolveProgramUniform3iv;
    ProgramUniform4iv = qopenglfResolveProgramUniform4iv;
    ProgramUniform1uiv = qopenglfResolveProgramUniform1uiv;
    ProgramUniform2uiv = qopenglfResolveProgramUniform2uiv;
    ProgramUniform3uiv = qopenglfResolveProgramUniform3uiv;
    ProgramUniform4uiv = qopenglfResolveProgramUniform4uiv;
    ProgramUniform1fv = qopenglfResolveProgramUniform1fv;
    ProgramUniform2fv = qopenglfResolveProgramUniform2fv;
    ProgramUniform3fv = qopenglfResolveProgramUniform3fv;
    ProgramUniform4fv = qopenglfResolveProgramUniform4fv;
    ProgramUniformMatrix2fv = qopenglfResolveProgramUniformMatrix2fv;
    ProgramUniformMatrix3fv = qopenglfResolveProgramUniformMatrix3fv;
    ProgramUniformMatrix4fv = qopenglfResolveProgramUniformMatrix4fv;
    ProgramUniformMatrix2x3fv = qopenglfResolveProgramUniformMatrix2x3fv;
    ProgramUniformMatrix3x2fv = qopenglfResolveProgramUniformMatrix3x2fv;
    ProgramUniformMatrix2x4fv = qopenglfResolveProgramUniformMatrix2x4fv;
    ProgramUniformMatrix4x2fv = qopenglfResolveProgramUniformMatrix4x2fv;
    ProgramUniformMatrix3x4fv = qopenglfResolveProgramUniformMatrix3x4fv;
    ProgramUniformMatrix4x3fv = qopenglfResolveProgramUniformMatrix4x3fv;
    ValidateProgramPipeline = qopenglfResolveValidateProgramPipeline;
    GetProgramPipelineInfoLog = qopenglfResolveGetProgramPipelineInfoLog;
    BindImageTexture = qopenglfResolveBindImageTexture;
    GetBooleani_v = qopenglfResolveGetBooleani_v;
    MemoryBarrierFunc = qopenglfResolveMemoryBarrier;
    MemoryBarrierByRegion = qopenglfResolveMemoryBarrierByRegion;
    TexStorage2DMultisample = qopenglfResolveTexStorage2DMultisample;
    GetMultisamplefv = qopenglfResolveGetMultisamplefv;
    SampleMaski = qopenglfResolveSampleMaski;
    GetTexLevelParameteriv = qopenglfResolveGetTexLevelParameteriv;
    GetTexLevelParameterfv = qopenglfResolveGetTexLevelParameterfv;
    BindVertexBuffer = qopenglfResolveBindVertexBuffer;
    VertexAttribFormat = qopenglfResolveVertexAttribFormat;
    VertexAttribIFormat = qopenglfResolveVertexAttribIFormat;
    VertexAttribBinding = qopenglfResolveVertexAttribBinding;
    VertexBindingDivisor = qopenglfResolveVertexBindingDivisor;
}

QOpenGLExtensionsPrivate::QOpenGLExtensionsPrivate(QOpenGLContext *ctx)
    : QOpenGLExtraFunctionsPrivate(ctx),
      flushVendorChecked(false)
{
    MapBuffer = qopenglfResolveMapBuffer;
    GetBufferSubData = qopenglfResolveGetBufferSubData;
    DiscardFramebuffer = qopenglfResolveDiscardFramebuffer;
}

QOpenGLES3Helper *QOpenGLExtensions::gles3Helper()
{
    return qgles3Helper();
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
