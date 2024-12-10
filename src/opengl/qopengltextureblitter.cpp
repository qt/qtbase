// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopengltextureblitter.h"

#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtOpenGL/QOpenGLVertexArrayObject>
#include <QtOpenGL/QOpenGLBuffer>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLExtraFunctions>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES           0x8D65
#endif
#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE              0x84F5
#endif
#ifndef GL_TEXTURE_WIDTH
#define GL_TEXTURE_WIDTH                  0x1000
#endif
#ifndef GL_TEXTURE_HEIGHT
#define GL_TEXTURE_HEIGHT                 0x1001
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLTextureBlitter
    \brief The QOpenGLTextureBlitter class provides a convenient way to draw textured quads via OpenGL.
    \since 5.8
    \ingroup painting-3D
    \inmodule QtOpenGL

    Drawing textured quads, in order to get the contents of a texture
    onto the screen, is a common operation when developing 2D user
    interfaces. QOpenGLTextureBlitter provides a convenience class to
    avoid repeating vertex data, shader sources, buffer and program
    management and matrix calculations.

    For example, a QOpenGLWidget subclass can do the following to draw
    the contents rendered into a framebuffer at the pixel position \c{(x, y)}:

    \code
    void OpenGLWidget::initializeGL()
    {
        m_blitter.create();
        m_fbo = new QOpenGLFramebufferObject(size);
    }

    void OpenGLWidget::paintGL()
    {
        m_fbo->bind();
        // update offscreen content
        m_fbo->release();

        m_blitter.bind();
        const QRect targetRect(QPoint(x, y), m_fbo->size());
        const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(targetRect, QRect(QPoint(0, 0), m_fbo->size()));
        m_blitter.blit(m_fbo->texture(), target, QOpenGLTextureBlitter::OriginBottomLeft);
        m_blitter.release();
    }
    \endcode

    The blitter implements GLSL shaders both for GLSL 1.00 (suitable
    for OpenGL (ES) 2.x and compatibility profiles of newer OpenGL
    versions) and version 150 (suitable for core profile contexts with
    OpenGL 3.2 and newer).
 */

static const char vertex_shader150[] =
    "#version 150 core\n"
    "in vec3 vertexCoord;"
    "in vec2 textureCoord;"
    "out vec2 uv;"
    "uniform mat4 vertexTransform;"
    "uniform mat3 textureTransform;"
    "void main() {"
    "   uv = (textureTransform * vec3(textureCoord,1.0)).xy;"
    "   gl_Position = vertexTransform * vec4(vertexCoord,1.0);"
    "}";

static const char fragment_shader150[] =
    "#version 150 core\n"
    "in vec2 uv;"
    "out vec4 fragcolor;"
    "uniform sampler2D textureSampler;"
    "uniform bool swizzle;"
    "uniform float opacity;"
    "void main() {"
    "   vec4 tmpFragColor = texture(textureSampler, uv);"
    "   tmpFragColor.a *= opacity;"
    "   fragcolor = swizzle ? tmpFragColor.bgra : tmpFragColor;"
    "}";

static const char vertex_shader[] =
    "attribute highp vec3 vertexCoord;"
    "attribute highp vec2 textureCoord;"
    "varying highp vec2 uv;"
    "uniform highp mat4 vertexTransform;"
    "uniform highp mat3 textureTransform;"
    "void main() {"
    "   uv = (textureTransform * vec3(textureCoord,1.0)).xy;"
    "   gl_Position = vertexTransform * vec4(vertexCoord,1.0);"
    "}";

static const char fragment_shader[] =
    "varying highp vec2 uv;"
    "uniform sampler2D textureSampler;"
    "uniform bool swizzle;"
    "uniform highp float opacity;"
    "void main() {"
    "   highp vec4 tmpFragColor = texture2D(textureSampler,uv);"
    "   tmpFragColor.a *= opacity;"
    "   gl_FragColor = swizzle ? tmpFragColor.bgra : tmpFragColor;"
    "}";

static const char fragment_shader_external_oes[] =
    "#extension GL_OES_EGL_image_external : require\n"
    "varying highp vec2 uv;"
    "uniform samplerExternalOES textureSampler;\n"
    "uniform bool swizzle;"
    "uniform highp float opacity;"
    "void main() {"
    "   highp vec4 tmpFragColor = texture2D(textureSampler, uv);"
    "   tmpFragColor.a *= opacity;"
    "   gl_FragColor = swizzle ? tmpFragColor.bgra : tmpFragColor;"
    "}";

static const char fragment_shader_rectangle[] =
    "varying highp vec2 uv;"
    "uniform sampler2DRect textureSampler;"
    "uniform bool swizzle;"
    "uniform highp float opacity;"
    "void main() {"
    "   highp vec4 tmpFragColor = texture2DRect(textureSampler,uv);"
    "   tmpFragColor.a *= opacity;"
    "   gl_FragColor = swizzle ? tmpFragColor.bgra : tmpFragColor;"
    "}";

static const char fragment_shader150_rectangle[] =
    "#version 150 core\n"
    "in vec2 uv;"
    "out vec4 fragcolor;"
    "uniform sampler2DRect textureSampler;"
    "uniform bool swizzle;"
    "uniform float opacity;"
    "void main() {"
    "   vec4 tmpFragColor = texture(textureSampler, uv);"
    "   tmpFragColor.a *= opacity;"
    "   fragcolor = swizzle ? tmpFragColor.bgra : tmpFragColor;"
    "}";

static const GLfloat vertex_buffer_data[] = {
        -1,-1, 0,
        -1, 1, 0,
         1,-1, 0,
        -1, 1, 0,
         1,-1, 0,
         1, 1, 0
};

static const GLfloat texture_buffer_data[] = {
        0, 0,
        0, 1,
        1, 0,
        0, 1,
        1, 0,
        1, 1
};

class QBlitterTextureBinder
{
public:
    explicit QBlitterTextureBinder(GLenum target, GLuint textureId) : m_target(target)
    {
        QOpenGLContext::currentContext()->functions()->glBindTexture(m_target, textureId);
    }
    ~QBlitterTextureBinder()
    {
        QOpenGLContext::currentContext()->functions()->glBindTexture(m_target, 0);
    }

private:
    GLenum m_target;
};

class QOpenGLTextureBlitterPrivate
{
public:
    enum TextureMatrixUniform {
        User,
        Identity,
        IdentityFlipped
    };

    enum ProgramIndex {
        TEXTURE_2D,
        TEXTURE_EXTERNAL_OES,
        TEXTURE_RECTANGLE
    };

    QOpenGLTextureBlitterPrivate(QOpenGLTextureBlitter *q_ptr) :
        q(q_ptr),
        swizzle(false),
        opacity(1.0f),
        vao(new QOpenGLVertexArrayObject),
        currentTarget(TEXTURE_2D)
    { }

    bool buildProgram(ProgramIndex idx, const char *vs, const char *fs);
    bool ensureProgram(ProgramIndex idx);

    void blit(GLuint texture, const QMatrix4x4 &targetTransform, const QMatrix3x3 &sourceTransform);
    void blit(GLuint texture, const QMatrix4x4 &targetTransform, QOpenGLTextureBlitter::Origin origin);

    QMatrix3x3 toTextureCoordinates(const QMatrix3x3 &sourceTransform) const;

    bool prepareProgram(const QMatrix4x4 &vertexTransform);

    QOpenGLTextureBlitter *q;
    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer textureBuffer;
    struct Program {
        Program() :
            vertexCoordAttribPos(0),
            vertexTransformUniformPos(0),
            textureCoordAttribPos(0),
            textureTransformUniformPos(0),
            swizzleUniformPos(0),
            opacityUniformPos(0),
            swizzle(false),
            opacity(0.0f),
            textureMatrixUniformState(User)
        { }
        QScopedPointer<QOpenGLShaderProgram> glProgram;
        GLuint vertexCoordAttribPos;
        GLuint vertexTransformUniformPos;
        GLuint textureCoordAttribPos;
        GLuint textureTransformUniformPos;
        GLuint swizzleUniformPos;
        GLuint opacityUniformPos;
        bool swizzle;
        float opacity;
        TextureMatrixUniform textureMatrixUniformState;
    } programs[3];
    bool swizzle;
    float opacity;
    QScopedPointer<QOpenGLVertexArrayObject> vao;
    GLenum currentTarget;
};

static inline QOpenGLTextureBlitterPrivate::ProgramIndex targetToProgramIndex(GLenum target)
{
    switch (target) {
    case GL_TEXTURE_2D:
        return QOpenGLTextureBlitterPrivate::TEXTURE_2D;
    case GL_TEXTURE_EXTERNAL_OES:
        return QOpenGLTextureBlitterPrivate::TEXTURE_EXTERNAL_OES;
    case GL_TEXTURE_RECTANGLE:
        return QOpenGLTextureBlitterPrivate::TEXTURE_RECTANGLE;
    default:
        qWarning("Unsupported texture target 0x%x", target);
        return QOpenGLTextureBlitterPrivate::TEXTURE_2D;
    }
}

bool QOpenGLTextureBlitterPrivate::prepareProgram(const QMatrix4x4 &vertexTransform)
{
    ProgramIndex programIndex = targetToProgramIndex(currentTarget);
    if (!ensureProgram(programIndex))
        return false;

    Program *program = &programs[programIndex];

    vertexBuffer.bind();
    program->glProgram->setAttributeBuffer(program->vertexCoordAttribPos, GL_FLOAT, 0, 3, 0);
    program->glProgram->enableAttributeArray(program->vertexCoordAttribPos);
    vertexBuffer.release();

    program->glProgram->setUniformValue(program->vertexTransformUniformPos, vertexTransform);

    textureBuffer.bind();
    program->glProgram->setAttributeBuffer(program->textureCoordAttribPos, GL_FLOAT, 0, 2, 0);
    program->glProgram->enableAttributeArray(program->textureCoordAttribPos);
    textureBuffer.release();

    if (swizzle != program->swizzle) {
        program->glProgram->setUniformValue(program->swizzleUniformPos, swizzle);
        program->swizzle = swizzle;
    }

    if (opacity != program->opacity) {
        program->glProgram->setUniformValue(program->opacityUniformPos, opacity);
        program->opacity = opacity;
    }

    return true;
}

QMatrix3x3 QOpenGLTextureBlitterPrivate::toTextureCoordinates(const QMatrix3x3 &sourceTransform) const
{
    if (currentTarget == GL_TEXTURE_RECTANGLE) {
        // Non-normalized coordinates
        QMatrix4x4 textureTransform(sourceTransform);
        if (auto *glFunctions = QOpenGLContext::currentContext()->extraFunctions()) {
            int width, height;
            glFunctions->glGetTexLevelParameteriv(currentTarget, 0, GL_TEXTURE_WIDTH, &width);
            glFunctions->glGetTexLevelParameteriv(currentTarget, 0, GL_TEXTURE_HEIGHT, &height);
            textureTransform.scale(width, height);
        }
        return textureTransform.toGenericMatrix<3, 3>();
   }

    return sourceTransform; // Normalized coordinates
}

void QOpenGLTextureBlitterPrivate::blit(GLuint texture,
                                        const QMatrix4x4 &targetTransform,
                                        const QMatrix3x3 &sourceTransform)
{
    QBlitterTextureBinder binder(currentTarget, texture);
    if (!prepareProgram(targetTransform))
        return;

    Program *program = &programs[targetToProgramIndex(currentTarget)];

    const QMatrix3x3 textureTransform = toTextureCoordinates(sourceTransform);
    program->glProgram->setUniformValue(program->textureTransformUniformPos, textureTransform);
    program->textureMatrixUniformState = User;

    QOpenGLContext::currentContext()->functions()->glDrawArrays(GL_TRIANGLES, 0, 6);
}

void QOpenGLTextureBlitterPrivate::blit(GLuint texture,
                                        const QMatrix4x4 &targetTransform,
                                        QOpenGLTextureBlitter::Origin origin)
{
    QBlitterTextureBinder binder(currentTarget, texture);
    if (!prepareProgram(targetTransform))
        return;

    Program *program = &programs[targetToProgramIndex(currentTarget)];

    if (origin == QOpenGLTextureBlitter::OriginTopLeft) {
        if (program->textureMatrixUniformState != IdentityFlipped) {
            QMatrix3x3 sourceTransform;
            sourceTransform(1,1) = -1;
            sourceTransform(1,2) = 1;
            const QMatrix3x3 textureTransform = toTextureCoordinates(sourceTransform);
            program->glProgram->setUniformValue(program->textureTransformUniformPos, textureTransform);
            program->textureMatrixUniformState = IdentityFlipped;
        }
    } else if (program->textureMatrixUniformState != Identity) {
        const QMatrix3x3 textureTransform = toTextureCoordinates(QMatrix3x3());
        program->glProgram->setUniformValue(program->textureTransformUniformPos, textureTransform);
        program->textureMatrixUniformState = Identity;
    }

    QOpenGLContext::currentContext()->functions()->glDrawArrays(GL_TRIANGLES, 0, 6);
}

bool QOpenGLTextureBlitterPrivate::buildProgram(ProgramIndex idx, const char *vs, const char *fs)
{
    Program *p = &programs[idx];

    p->glProgram.reset(new QOpenGLShaderProgram);

    p->glProgram->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vs);
    p->glProgram->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fs);
    p->glProgram->link();
    if (!p->glProgram->isLinked()) {
        qWarning() << "Could not link shader program:\n" << p->glProgram->log();
        return false;
    }

    p->glProgram->bind();

    p->vertexCoordAttribPos = p->glProgram->attributeLocation("vertexCoord");
    p->vertexTransformUniformPos = p->glProgram->uniformLocation("vertexTransform");
    p->textureCoordAttribPos = p->glProgram->attributeLocation("textureCoord");
    p->textureTransformUniformPos = p->glProgram->uniformLocation("textureTransform");
    p->swizzleUniformPos = p->glProgram->uniformLocation("swizzle");
    p->opacityUniformPos = p->glProgram->uniformLocation("opacity");

    p->glProgram->setUniformValue(p->swizzleUniformPos, false);

    // minmize state left set after a create()
    p->glProgram->release();

    return true;
}

bool QOpenGLTextureBlitterPrivate::ensureProgram(ProgramIndex idx)
{
    if (programs[idx].glProgram)
        return true;

    QOpenGLContext *currentContext = QOpenGLContext::currentContext();
    if (!currentContext)
        return false;

    QSurfaceFormat format = currentContext->format();
    if (format.profile() == QSurfaceFormat::CoreProfile && format.version() >= std::pair(3,2)) {
        if (idx == QOpenGLTextureBlitterPrivate::TEXTURE_RECTANGLE && q->supportsRectangleTarget()) {
            if (!buildProgram(idx, vertex_shader150, fragment_shader150_rectangle))
                return false;
        }
    } else {
        if (idx == QOpenGLTextureBlitterPrivate::TEXTURE_RECTANGLE && q->supportsRectangleTarget()) {
            if (!buildProgram(idx, vertex_shader, fragment_shader_rectangle))
                return false;
        }
        if (idx == QOpenGLTextureBlitterPrivate::TEXTURE_EXTERNAL_OES && q->supportsExternalOESTarget()) {
            if (!buildProgram(idx, vertex_shader, fragment_shader_external_oes))
                return false;
        }
    }

    return !programs[idx].glProgram.isNull();
}

/*!
    Constructs a new QOpenGLTextureBlitter instance.

    \note no graphics resources are initialized in the
    constructor. This makes it safe to place plain
    QOpenGLTextureBlitter members into classes because the actual
    initialization that depends on the OpenGL context happens only in
    create().
 */
QOpenGLTextureBlitter::QOpenGLTextureBlitter()
    : d_ptr(new QOpenGLTextureBlitterPrivate(this))
{
}

/*!
    Destructs the instance.

    \note When the OpenGL context - or a context sharing resources
    with it - that was current when calling create() is not current,
    graphics resources will not be released. Therefore, it is
    recommended to call destroy() manually instead of relying on the
    destructor to perform OpenGL resource cleanup.
 */
QOpenGLTextureBlitter::~QOpenGLTextureBlitter()
{
    destroy();
}

/*!
    Initializes the graphics resources used by the blitter.

    \return \c true if successful, \c false if there was a
    failure. Failures can occur when there is no OpenGL context
    current on the current thread, or when shader compilation fails
    for some reason.

    \sa isCreated(), destroy()
 */
bool QOpenGLTextureBlitter::create()
{
    QOpenGLContext *currentContext = QOpenGLContext::currentContext();
    if (!currentContext)
        return false;

    Q_D(QOpenGLTextureBlitter);

    if (d->programs[QOpenGLTextureBlitterPrivate::TEXTURE_2D].glProgram)
        return true;

    QSurfaceFormat format = currentContext->format();
    // Build the most common, 2D texture shader variant.
    // The other special ones are deferred and compiled only when first needed.
    if (format.profile() == QSurfaceFormat::CoreProfile && format.version() >= std::pair(3,2)) {
        if (!d->buildProgram(QOpenGLTextureBlitterPrivate::TEXTURE_2D, vertex_shader150, fragment_shader150))
            return false;
    } else {
        if (!d->buildProgram(QOpenGLTextureBlitterPrivate::TEXTURE_2D, vertex_shader, fragment_shader))
            return false;
    }

    // Create and bind the VAO, if supported.
    QOpenGLVertexArrayObject::Binder vaoBinder(d->vao.data());

    d->vertexBuffer.create();
    d->vertexBuffer.bind();
    d->vertexBuffer.allocate(vertex_buffer_data, sizeof(vertex_buffer_data));
    d->vertexBuffer.release();

    d->textureBuffer.create();
    d->textureBuffer.bind();
    d->textureBuffer.allocate(texture_buffer_data, sizeof(texture_buffer_data));
    d->textureBuffer.release();

    return true;
}

/*!
    \return \c true if create() was called and succeeded. \c false otherwise.

    \sa create(), destroy()
 */
bool QOpenGLTextureBlitter::isCreated() const
{
    Q_D(const QOpenGLTextureBlitter);
    return !d->programs[QOpenGLTextureBlitterPrivate::TEXTURE_2D].glProgram.isNull();
}

/*!
    Frees all graphics resources held by the blitter. Assumes that
    the OpenGL context, or another context sharing resources with it,
    that was current on the thread when invoking create() is current.

    The function has no effect when the blitter is not in created state.

    \sa create()
 */
void QOpenGLTextureBlitter::destroy()
{
    if (!isCreated())
        return;
    Q_D(QOpenGLTextureBlitter);
    d->programs[QOpenGLTextureBlitterPrivate::TEXTURE_2D].glProgram.reset();
    d->programs[QOpenGLTextureBlitterPrivate::TEXTURE_EXTERNAL_OES].glProgram.reset();
    d->programs[QOpenGLTextureBlitterPrivate::TEXTURE_RECTANGLE].glProgram.reset();
    d->vertexBuffer.destroy();
    d->textureBuffer.destroy();
    d->vao.reset();
}

/*!
    \return \c true when bind() accepts \c GL_TEXTURE_EXTERNAL_OES as
    its target argument.

    \sa bind(), blit()
 */
bool QOpenGLTextureBlitter::supportsExternalOESTarget() const
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    return ctx && ctx->isOpenGLES() && ctx->hasExtension("GL_OES_EGL_image_external");
}

/*!
    \return \c true when bind() accepts \c GL_TEXTURE_RECTANGLE as
    its target argument.

    \sa bind(), blit()
 */
bool QOpenGLTextureBlitter::supportsRectangleTarget() const
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx || ctx->isOpenGLES())
        return false;

    if (ctx->hasExtension("GL_ARB_texture_rectangle"))
        return true;

    if (ctx->hasExtension("GL_EXT_texture_rectangle"))
        return true;

    QSurfaceFormat f = ctx->format();
    const auto version = std::pair(f.majorVersion(), f.minorVersion());
    if (version >= std::pair(3, 1))
        return true;

    return false;
}

/*!
    Binds the graphics resources used by the blitter. This must be
    called before calling blit(). Code modifying the OpenGL state
    should be avoided between the call to bind() and blit() because
    otherwise conflicts may arise.

    \a target is the texture target for the source texture and must be
    either \c GL_TEXTURE_2D, \c GL_TEXTURE_RECTANGLE, or \c GL_OES_EGL_image_external.

    \sa release(), blit()
 */
void QOpenGLTextureBlitter::bind(GLenum target)
{
    Q_D(QOpenGLTextureBlitter);

    if (d->vao->isCreated())
        d->vao->bind();

    d->currentTarget = target;
    QOpenGLTextureBlitterPrivate::ProgramIndex programIndex = targetToProgramIndex(target);
    if (!d->ensureProgram(programIndex))
        return;

    QOpenGLTextureBlitterPrivate::Program *p = &d->programs[programIndex];
    p->glProgram->bind();

    d->vertexBuffer.bind();
    p->glProgram->setAttributeBuffer(p->vertexCoordAttribPos, GL_FLOAT, 0, 3, 0);
    p->glProgram->enableAttributeArray(p->vertexCoordAttribPos);
    d->vertexBuffer.release();

    d->textureBuffer.bind();
    p->glProgram->setAttributeBuffer(p->textureCoordAttribPos, GL_FLOAT, 0, 2, 0);
    p->glProgram->enableAttributeArray(p->textureCoordAttribPos);
    d->textureBuffer.release();
}

/*!
    Unbinds the graphics resources used by the blitter.

    \sa bind()
 */
void QOpenGLTextureBlitter::release()
{
    Q_D(QOpenGLTextureBlitter);
    QOpenGLTextureBlitterPrivate::Program *p = &d->programs[targetToProgramIndex(d->currentTarget)];
    if (p->glProgram)
        p->glProgram->release();
    if (d->vao->isCreated())
        d->vao->release();
}

/*!
    Sets whether swizzling is enabled for the red and blue color channels to
    \a swizzle. An BGRA to RGBA conversion (occurring in the shader on
    the GPU, instead of a slow CPU-side transformation) can be useful
    when the source texture contains data from a QImage with a format
    like QImage::Format_ARGB32 which maps to BGRA on little endian
    systems.

    By default the red-blue swizzle is disabled since this is what a
    texture attached to an framebuffer object or a texture based on a
    byte ordered QImage format (like QImage::Format_RGBA8888) needs.
 */
void QOpenGLTextureBlitter::setRedBlueSwizzle(bool swizzle)
{
    Q_D(QOpenGLTextureBlitter);
    d->swizzle = swizzle;
}

/*!
    Changes the opacity to \a opacity. The default opacity is 1.0.

    \note the blitter does not alter the blend state. It is up to the
    caller of blit() to ensure the correct blend settings are active.

 */
void QOpenGLTextureBlitter::setOpacity(float opacity)
{
    Q_D(QOpenGLTextureBlitter);
    d->opacity = opacity;
}

/*!
    \enum QOpenGLTextureBlitter::Origin

    \value OriginBottomLeft Indicates that the data in the texture
    follows the OpenGL convention of coordinate systems, meaning Y is
    running from bottom to top.

    \value OriginTopLeft Indicates that the data in the texture has Y
    running from top to bottom, which is typical with regular,
    unflipped image data.

    \sa blit()
 */

/*!
    Performs the blit with the source texture \a texture.

    \a targetTransform specifies the transformation applied. This is
    usually generated by the targetTransform() helper function.

    \a sourceOrigin specifies if the image data needs flipping. When
    \a texture corresponds to a texture attached to an FBO pass
    OriginBottomLeft. On the other hand, when \a texture is based on
    unflipped image data, pass OriginTopLeft. This is more efficient
    than using QImage::flipped().

    \sa targetTransform(), Origin, bind()
 */
void QOpenGLTextureBlitter::blit(GLuint texture,
                                 const QMatrix4x4 &targetTransform,
                                 Origin sourceOrigin)
{
    Q_D(QOpenGLTextureBlitter);
    d->blit(texture, targetTransform, sourceOrigin);
}

/*!
    Performs the blit with the source texture \a texture.

    \a targetTransform specifies the transformation applied. This is
    usually generated by the targetTransform() helper function.

    \a sourceTransform specifies the transformation applied to the
    source. This allows using only a sub-rect of the source
    texture. This is usually generated by the sourceTransform() helper
    function.

    \sa sourceTransform(), targetTransform(), Origin, bind()
 */
void QOpenGLTextureBlitter::blit(GLuint texture,
                                 const QMatrix4x4 &targetTransform,
                                 const QMatrix3x3 &sourceTransform)
{
    Q_D(QOpenGLTextureBlitter);
    d->blit(texture, targetTransform, sourceTransform);
}

/*!
    Calculates a target transform suitable for blit().

    \a target is the target rectangle in pixels. \a viewport describes
    the source dimensions and will in most cases be set to (0, 0,
    image width, image height).

    For unscaled output the size of \a target and \a viewport should
    match.

    \sa blit()
 */
QMatrix4x4 QOpenGLTextureBlitter::targetTransform(const QRectF &target,
                                                  const QRect &viewport)
{
    qreal x_scale = target.width() / viewport.width();
    qreal y_scale = target.height() / viewport.height();

    const QPointF relative_to_viewport = target.topLeft() - viewport.topLeft();
    qreal x_translate = x_scale - 1 + ((relative_to_viewport.x() / viewport.width()) * 2);
    qreal y_translate = -y_scale + 1 - ((relative_to_viewport.y() / viewport.height()) * 2);

    QMatrix4x4 matrix;
    matrix(0,3) = x_translate;
    matrix(1,3) = y_translate;

    matrix(0,0) = x_scale;
    matrix(1,1) = y_scale;

    return matrix;
}

/*!
    Calculates a 3x3 matrix suitable as the input to blit(). This is
    used when only a part of the texture is to be used in the blit.

    \a subTexture is the desired source rectangle in pixels, \a
    textureSize is the full width and height of the texture data.  \a
    origin specifies the orientation of the image data when it comes
    to the Y axis.

    \sa blit(), Origin
 */
QMatrix3x3 QOpenGLTextureBlitter::sourceTransform(const QRectF &subTexture,
                                                  const QSize &textureSize,
                                                  Origin origin)
{
    qreal x_scale = subTexture.width() / textureSize.width();
    qreal y_scale = subTexture.height() / textureSize.height();

    const QPointF topLeft = subTexture.topLeft();
    qreal x_translate = topLeft.x() / textureSize.width();
    qreal y_translate = topLeft.y() / textureSize.height();

    if (origin == OriginTopLeft) {
        y_scale = -y_scale;
        y_translate = 1 - y_translate;
    }

    QMatrix3x3 matrix;
    matrix(0,2) = x_translate;
    matrix(1,2) = y_translate;

    matrix(0,0) = x_scale;
    matrix(1,1) = y_scale;

    return matrix;
}

QT_END_NAMESPACE
