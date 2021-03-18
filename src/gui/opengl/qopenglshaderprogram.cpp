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

#include "qopenglshaderprogram.h"
#include "qopenglprogrambinarycache_p.h"
#include "qopenglextrafunctions.h"
#include "private/qopenglcontext_p.h"
#include <QtCore/private/qobject_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qvector.h>
#include <QtCore/qloggingcategory.h>
#include <QtGui/qtransform.h>
#include <QtGui/QColor>
#include <QtGui/QSurfaceFormat>

#if !defined(QT_OPENGL_ES_2)
#include <QtGui/qopenglfunctions_4_0_core.h>
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLShaderProgram
    \brief The QOpenGLShaderProgram class allows OpenGL shader programs to be linked and used.
    \since 5.0
    \ingroup painting-3D
    \inmodule QtGui

    \section1 Introduction

    This class supports shader programs written in the OpenGL Shading
    Language (GLSL) and in the OpenGL/ES Shading Language (GLSL/ES).

    QOpenGLShader and QOpenGLShaderProgram shelter the programmer from the details of
    compiling and linking vertex and fragment shaders.

    The following example creates a vertex shader program using the
    supplied source \c{code}.  Once compiled and linked, the shader
    program is activated in the current QOpenGLContext by calling
    QOpenGLShaderProgram::bind():

    \snippet code/src_gui_qopenglshaderprogram.cpp 0

    \section1 Writing Portable Shaders

    Shader programs can be difficult to reuse across OpenGL implementations
    because of varying levels of support for standard vertex attributes and
    uniform variables.  In particular, GLSL/ES lacks all of the
    standard variables that are present on desktop OpenGL systems:
    \c{gl_Vertex}, \c{gl_Normal}, \c{gl_Color}, and so on.  Desktop OpenGL
    lacks the variable qualifiers \c{highp}, \c{mediump}, and \c{lowp}.

    The QOpenGLShaderProgram class makes the process of writing portable shaders
    easier by prefixing all shader programs with the following lines on
    desktop OpenGL:

    \code
    #define highp
    #define mediump
    #define lowp
    \endcode

    This makes it possible to run most GLSL/ES shader programs
    on desktop systems.  The programmer should restrict themselves
    to just features that are present in GLSL/ES, and avoid
    standard variable names that only work on the desktop.

    \section1 Simple Shader Example

    \snippet code/src_gui_qopenglshaderprogram.cpp 1

    With the above shader program active, we can draw a green triangle
    as follows:

    \snippet code/src_gui_qopenglshaderprogram.cpp 2

    \section1 Binary Shaders and Programs

    Binary shaders may be specified using \c{glShaderBinary()} on
    the return value from QOpenGLShader::shaderId().  The QOpenGLShader instance
    containing the binary can then be added to the shader program with
    addShader() and linked in the usual fashion with link().

    Binary programs may be specified using \c{glProgramBinaryOES()}
    on the return value from programId().  Then the application should
    call link(), which will notice that the program has already been
    specified and linked, allowing other operations to be performed
    on the shader program. The shader program's id can be explicitly
    created using the create() function.

    \section2 Caching Program Binaries

    As of Qt 5.9, support for caching program binaries on disk is built in. To
    enable this, switch to using addCacheableShaderFromSourceCode() and
    addCacheableShaderFromSourceFile(). With an OpenGL ES 3.x context or support
    for \c{GL_ARB_get_program_binary}, this will transparently cache program
    binaries under QStandardPaths::GenericCacheLocation or
    QStandardPaths::CacheLocation. When support is not available, calling the
    cacheable function variants is equivalent to the normal ones.

    \note Some drivers do not have any binary formats available, even though
    they advertise the extension or offer OpenGL ES 3.0. In this case program
    binary support will be disabled.

    \sa QOpenGLShader
*/

/*!
    \class QOpenGLShader
    \brief The QOpenGLShader class allows OpenGL shaders to be compiled.
    \since 5.0
    \ingroup painting-3D
    \inmodule QtGui

    This class supports shaders written in the OpenGL Shading Language (GLSL)
    and in the OpenGL/ES Shading Language (GLSL/ES).

    QOpenGLShader and QOpenGLShaderProgram shelter the programmer from the details of
    compiling and linking vertex and fragment shaders.

    \sa QOpenGLShaderProgram
*/

/*!
    \enum QOpenGLShader::ShaderTypeBit
    This enum specifies the type of QOpenGLShader that is being created.

    \value Vertex Vertex shader written in the OpenGL Shading Language (GLSL).
    \value Fragment Fragment shader written in the OpenGL Shading Language (GLSL).
    \value Geometry Geometry shaders written in the OpenGL Shading Language (GLSL)
           (requires OpenGL >= 3.2 or OpenGL ES >= 3.2).
    \value TessellationControl Tessellation control shaders written in the OpenGL
           shading language (GLSL) (requires OpenGL >= 4.0 or OpenGL ES >= 3.2).
    \value TessellationEvaluation Tessellation evaluation shaders written in the OpenGL
           shading language (GLSL) (requires OpenGL >= 4.0 or OpenGL ES >= 3.2).
    \value Compute Compute shaders written in the OpenGL shading language (GLSL)
           (requires OpenGL >= 4.3 or OpenGL ES >= 3.1).
*/

Q_DECLARE_LOGGING_CATEGORY(lcOpenGLProgramDiskCache)

// For GLES 3.1/3.2
#ifndef GL_GEOMETRY_SHADER
#define GL_GEOMETRY_SHADER         0x8DD9
#endif
#ifndef GL_TESS_CONTROL_SHADER
#define GL_TESS_CONTROL_SHADER     0x8E88
#endif
#ifndef GL_TESS_EVALUATION_SHADER
#define GL_TESS_EVALUATION_SHADER  0x8E87
#endif
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER          0x91B9
#endif
#ifndef GL_MAX_GEOMETRY_OUTPUT_VERTICES
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES          0x8DE0
#endif
#ifndef GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS  0x8DE1
#endif
#ifndef GL_PATCH_VERTICES
#define GL_PATCH_VERTICES  0x8E72
#endif
#ifndef GL_PATCH_DEFAULT_OUTER_LEVEL
#define GL_PATCH_DEFAULT_OUTER_LEVEL  0x8E74
#endif
#ifndef GL_PATCH_DEFAULT_INNER_LEVEL
#define GL_PATCH_DEFAULT_INNER_LEVEL  0x8E73
#endif

#ifndef QT_OPENGL_ES_2
static inline bool isFormatGLES(const QSurfaceFormat &f)
{
    return (f.renderableType() == QSurfaceFormat::OpenGLES);
}
#endif

static inline bool supportsGeometry(const QSurfaceFormat &f)
{
    return f.version() >= qMakePair(3, 2);
}

static inline bool supportsCompute(const QSurfaceFormat &f)
{
#ifndef QT_OPENGL_ES_2
    if (!isFormatGLES(f))
        return f.version() >= qMakePair(4, 3);
    else
        return f.version() >= qMakePair(3, 1);
#else
    return f.version() >= qMakePair(3, 1);
#endif
}

static inline bool supportsTessellation(const QSurfaceFormat &f)
{
#ifndef QT_OPENGL_ES_2
    if (!isFormatGLES(f))
        return f.version() >= qMakePair(4, 0);
    else
        return f.version() >= qMakePair(3, 2);
#else
    return f.version() >= qMakePair(3, 2);
#endif
}

class QOpenGLShaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLShader)
public:
    QOpenGLShaderPrivate(QOpenGLContext *ctx, QOpenGLShader::ShaderType type)
        : shaderGuard(nullptr)
        , shaderType(type)
        , compiled(false)
        , glfuncs(new QOpenGLExtraFunctions(ctx))
        , supportsGeometryShaders(false)
        , supportsTessellationShaders(false)
        , supportsComputeShaders(false)
    {
        if (shaderType & QOpenGLShader::Geometry)
            supportsGeometryShaders = supportsGeometry(ctx->format());
        else if (shaderType & (QOpenGLShader::TessellationControl | QOpenGLShader::TessellationEvaluation))
            supportsTessellationShaders = supportsTessellation(ctx->format());
        else if (shaderType & QOpenGLShader::Compute)
            supportsComputeShaders = supportsCompute(ctx->format());
    }
    ~QOpenGLShaderPrivate();

    QOpenGLSharedResourceGuard *shaderGuard;
    QOpenGLShader::ShaderType shaderType;
    bool compiled;
    QString log;

    QOpenGLExtraFunctions *glfuncs;

    // Support for geometry shaders
    bool supportsGeometryShaders;
    // Support for tessellation shaders
    bool supportsTessellationShaders;
    // Support for compute shaders
    bool supportsComputeShaders;


    bool create();
    bool compile(QOpenGLShader *q);
    void deleteShader();
};

namespace {
    void freeShaderFunc(QOpenGLFunctions *funcs, GLuint id)
    {
        funcs->glDeleteShader(id);
    }
}

QOpenGLShaderPrivate::~QOpenGLShaderPrivate()
{
    delete glfuncs;
    if (shaderGuard)
        shaderGuard->free();
}

bool QOpenGLShaderPrivate::create()
{
    QOpenGLContext *context = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
    if (!context)
        return false;
    GLuint shader = 0;
    if (shaderType == QOpenGLShader::Vertex) {
        shader = glfuncs->glCreateShader(GL_VERTEX_SHADER);
    } else if (shaderType == QOpenGLShader::Geometry && supportsGeometryShaders) {
        shader = glfuncs->glCreateShader(GL_GEOMETRY_SHADER);
    } else if (shaderType == QOpenGLShader::TessellationControl && supportsTessellationShaders) {
        shader = glfuncs->glCreateShader(GL_TESS_CONTROL_SHADER);
    } else if (shaderType == QOpenGLShader::TessellationEvaluation && supportsTessellationShaders) {
        shader = glfuncs->glCreateShader(GL_TESS_EVALUATION_SHADER);
    } else if (shaderType == QOpenGLShader::Compute && supportsComputeShaders) {
        shader = glfuncs->glCreateShader(GL_COMPUTE_SHADER);
    } else if (shaderType == QOpenGLShader::Fragment) {
        shader = glfuncs->glCreateShader(GL_FRAGMENT_SHADER);
    }
    if (!shader) {
        qWarning("QOpenGLShader: could not create shader");
        return false;
    }
    shaderGuard = new QOpenGLSharedResourceGuard(context, shader, freeShaderFunc);
    return true;
}

bool QOpenGLShaderPrivate::compile(QOpenGLShader *q)
{
    GLuint shader = shaderGuard ? shaderGuard->id() : 0;
    if (!shader)
        return false;

    // Try to compile shader
    glfuncs->glCompileShader(shader);
    GLint value = 0;

    // Get compilation status
    glfuncs->glGetShaderiv(shader, GL_COMPILE_STATUS, &value);
    compiled = (value != 0);

    if (!compiled) {
        // Compilation failed, try to provide some information about the failure
        QString name = q->objectName();

        const char *types[] = {
            "Fragment",
            "Vertex",
            "Geometry",
            "Tessellation Control",
            "Tessellation Evaluation",
            "Compute",
            ""
        };

        const char *type = types[6];
        switch (shaderType) {
        case QOpenGLShader::Fragment:
            type = types[0]; break;
        case QOpenGLShader::Vertex:
            type = types[1]; break;
        case QOpenGLShader::Geometry:
            type = types[2]; break;
        case QOpenGLShader::TessellationControl:
            type = types[3]; break;
        case QOpenGLShader::TessellationEvaluation:
            type = types[4]; break;
        case QOpenGLShader::Compute:
            type = types[5]; break;
        }

        // Get info and source code lengths
        GLint infoLogLength = 0;
        GLint sourceCodeLength = 0;
        char *logBuffer = nullptr;
        char *sourceCodeBuffer = nullptr;

        // Get the compilation info log
        glfuncs->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        if (infoLogLength > 1) {
            GLint temp;
            logBuffer = new char [infoLogLength];
            glfuncs->glGetShaderInfoLog(shader, infoLogLength, &temp, logBuffer);
        }

        // Get the source code
        glfuncs->glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &sourceCodeLength);

        if (sourceCodeLength > 1) {
            GLint temp;
            sourceCodeBuffer = new char [sourceCodeLength];
            glfuncs->glGetShaderSource(shader, sourceCodeLength, &temp, sourceCodeBuffer);
        }

        if (logBuffer)
            log = QString::fromLatin1(logBuffer);
        else
            log = QLatin1String("failed");

        if (name.isEmpty())
            qWarning("QOpenGLShader::compile(%s): %s", type, qPrintable(log));
        else
            qWarning("QOpenGLShader::compile(%s)[%s]: %s", type, qPrintable(name), qPrintable(log));

        // Dump the source code if we got it
        if (sourceCodeBuffer) {
            qWarning("*** Problematic %s shader source code ***\n"
                     "%ls\n"
                     "***",
                     type, qUtf16Printable(QString::fromLatin1(sourceCodeBuffer)));
        }

        // Cleanup
        delete [] logBuffer;
        delete [] sourceCodeBuffer;
    }

    return compiled;
}

void QOpenGLShaderPrivate::deleteShader()
{
    if (shaderGuard) {
        shaderGuard->free();
        shaderGuard = nullptr;
    }
}

/*!
    Constructs a new QOpenGLShader object of the specified \a type
    and attaches it to \a parent.  If shader programs are not supported,
    QOpenGLShaderProgram::hasOpenGLShaderPrograms() will return false.

    This constructor is normally followed by a call to compileSourceCode()
    or compileSourceFile().

    The shader will be associated with the current QOpenGLContext.

    \sa compileSourceCode(), compileSourceFile()
*/
QOpenGLShader::QOpenGLShader(QOpenGLShader::ShaderType type, QObject *parent)
    : QObject(*new QOpenGLShaderPrivate(QOpenGLContext::currentContext(), type), parent)
{
    Q_D(QOpenGLShader);
    d->create();
}

/*!
    Deletes this shader.  If the shader has been attached to a
    QOpenGLShaderProgram object, then the actual shader will stay around
    until the QOpenGLShaderProgram is destroyed.
*/
QOpenGLShader::~QOpenGLShader()
{
}

/*!
    Returns the type of this shader.
*/
QOpenGLShader::ShaderType QOpenGLShader::shaderType() const
{
    Q_D(const QOpenGLShader);
    return d->shaderType;
}

static const char qualifierDefines[] =
    "#define lowp\n"
    "#define mediump\n"
    "#define highp\n";

#if defined(QT_OPENGL_ES) && !defined(QT_OPENGL_FORCE_SHADER_DEFINES)
// The "highp" qualifier doesn't exist in fragment shaders
// on all ES platforms.  When it doesn't exist, use "mediump".
#define QOpenGL_REDEFINE_HIGHP 1
static const char redefineHighp[] =
    "#ifndef GL_FRAGMENT_PRECISION_HIGH\n"
    "#define highp mediump\n"
    "#endif\n";
#endif

// Boiler-plate header to have the layout attributes available we need later
static const char blendEquationAdvancedHeader[] =
    "#ifdef GL_KHR_blend_equation_advanced\n"
    "#extension GL_ARB_fragment_coord_conventions : enable\n"
    "#extension GL_KHR_blend_equation_advanced : enable\n"
    "#endif\n";

struct QVersionDirectivePosition
{
    Q_DECL_CONSTEXPR QVersionDirectivePosition(int position = 0, int line = -1)
        : position(position)
        , line(line)
    {
    }

    Q_DECL_CONSTEXPR bool hasPosition() const
    {
        return position > 0;
    }

    const int position;
    const int line;
};

static QVersionDirectivePosition findVersionDirectivePosition(const char *source)
{
    Q_ASSERT(source);

    // According to the GLSL spec the #version directive must not be
    // preceded by anything but whitespace and comments.
    // In order to not get confused by #version directives within a
    // multiline comment, we need to do some minimal comment parsing
    // while searching for the directive.
    enum {
        Normal,
        StartOfLine,
        PreprocessorDirective,
        CommentStarting,
        MultiLineComment,
        SingleLineComment,
        CommentEnding
    } state = StartOfLine;

    const char *c = source;
    while (*c) {
        switch (state) {
        case PreprocessorDirective:
            if (*c == ' ' || *c == '\t')
                break;
            if (!strncmp(c, "version", strlen("version"))) {
                // Found version directive
                c += strlen("version");
                while (*c && *c != '\n')
                    ++c;
                int splitPosition = c - source + 1;
                int linePosition = int(std::count(source, c, '\n')) + 1;
                return QVersionDirectivePosition(splitPosition, linePosition);
            } else if (*c == '/')
                state = CommentStarting;
            else if (*c == '\n')
                state = StartOfLine;
            else
                state = Normal;
            break;
        case StartOfLine:
            if (*c == ' ' || *c == '\t')
                break;
            else if (*c == '#') {
                state = PreprocessorDirective;
                break;
            }
            state = Normal;
            Q_FALLTHROUGH();
        case Normal:
            if (*c == '/')
                state = CommentStarting;
            else if (*c == '\n')
                state = StartOfLine;
            break;
        case CommentStarting:
            if (*c == '*')
                state = MultiLineComment;
            else if (*c == '/')
                state = SingleLineComment;
            else
                state = Normal;
            break;
        case MultiLineComment:
            if (*c == '*')
                state = CommentEnding;
            break;
        case SingleLineComment:
            if (*c == '\n')
                state = Normal;
            break;
        case CommentEnding:
            if (*c == '/')
                state = Normal;
            else if (*c != QLatin1Char('*'))
                state = MultiLineComment;
            break;
        }
        ++c;
    }

    return QVersionDirectivePosition(0, 1);
}

/*!
    Sets the \a source code for this shader and compiles it.
    Returns \c true if the source was successfully compiled, false otherwise.

    \sa compileSourceFile()
*/
bool QOpenGLShader::compileSourceCode(const char *source)
{
    Q_D(QOpenGLShader);
    // This method breaks the shader code into two parts:
    // 1. Up to and including an optional #version directive.
    // 2. The rest.
    // If a #version directive exists, qualifierDefines and redefineHighp
    // are inserted after. Otherwise they are inserted right at the start.
    // In both cases a #line directive is appended in order to compensate
    // for line number changes in case of compiler errors.

    if (d->shaderGuard && d->shaderGuard->id() && source) {
        const QVersionDirectivePosition versionDirectivePosition = findVersionDirectivePosition(source);

        QVarLengthArray<const char *, 5> sourceChunks;
        QVarLengthArray<GLint, 5> sourceChunkLengths;
        QOpenGLContext *ctx = QOpenGLContext::currentContext();

        if (versionDirectivePosition.hasPosition()) {
            // Append source up to and including the #version directive
            sourceChunks.append(source);
            sourceChunkLengths.append(GLint(versionDirectivePosition.position));
        } else {
            // QTBUG-55733: Intel on Windows with Compatibility profile requires a #version always
            if (ctx->format().profile() == QSurfaceFormat::CompatibilityProfile) {
                const char *vendor = reinterpret_cast<const char *>(ctx->functions()->glGetString(GL_VENDOR));
                if (vendor && !strcmp(vendor, "Intel")) {
                    static const char version110[] = "#version 110\n";
                    sourceChunks.append(version110);
                    sourceChunkLengths.append(GLint(sizeof(version110)) - 1);
                }
            }
        }
        if (d->shaderType == Fragment) {
            sourceChunks.append(blendEquationAdvancedHeader);
            sourceChunkLengths.append(GLint(sizeof(blendEquationAdvancedHeader) - 1));
        }

        // The precision qualifiers are useful on OpenGL/ES systems,
        // but usually not present on desktop systems.
        const QSurfaceFormat currentSurfaceFormat = ctx->format();
        QOpenGLContextPrivate *ctx_d = QOpenGLContextPrivate::get(QOpenGLContext::currentContext());
        if (currentSurfaceFormat.renderableType() == QSurfaceFormat::OpenGL
                || ctx_d->workaround_missingPrecisionQualifiers
#ifdef QT_OPENGL_FORCE_SHADER_DEFINES
                || true
#endif
                ) {
            sourceChunks.append(qualifierDefines);
            sourceChunkLengths.append(GLint(sizeof(qualifierDefines) - 1));
        }

#ifdef QOpenGL_REDEFINE_HIGHP
        if (d->shaderType == Fragment && !ctx_d->workaround_missingPrecisionQualifiers
            && QOpenGLContext::currentContext()->isOpenGLES()) {
            sourceChunks.append(redefineHighp);
            sourceChunkLengths.append(GLint(sizeof(redefineHighp) - 1));
        }
#endif

        QByteArray lineDirective;
        // #line is rejected by some drivers:
        // "2.1 Mesa 8.1-devel (git-48a3d4e)" or "MESA 2.1 Mesa 8.1-devel"
        const char *version = reinterpret_cast<const char *>(ctx->functions()->glGetString(GL_VERSION));
        if (!version || !strstr(version, "2.1 Mesa 8")) {
            // Append #line directive in order to compensate for text insertion
            lineDirective = QStringLiteral("#line %1\n").arg(versionDirectivePosition.line).toUtf8();
            sourceChunks.append(lineDirective.constData());
            sourceChunkLengths.append(GLint(lineDirective.length()));
        }

        // Append rest of shader code
        sourceChunks.append(source + versionDirectivePosition.position);
        sourceChunkLengths.append(GLint(qstrlen(source + versionDirectivePosition.position)));

        d->glfuncs->glShaderSource(d->shaderGuard->id(), sourceChunks.size(), sourceChunks.data(), sourceChunkLengths.data());
        return d->compile(this);
    } else {
        return false;
    }
}

/*!
    \overload

    Sets the \a source code for this shader and compiles it.
    Returns \c true if the source was successfully compiled, false otherwise.

    \sa compileSourceFile()
*/
bool QOpenGLShader::compileSourceCode(const QByteArray& source)
{
    return compileSourceCode(source.constData());
}

/*!
    \overload

    Sets the \a source code for this shader and compiles it.
    Returns \c true if the source was successfully compiled, false otherwise.

    \sa compileSourceFile()
*/
bool QOpenGLShader::compileSourceCode(const QString& source)
{
    return compileSourceCode(source.toLatin1().constData());
}

/*!
    Sets the source code for this shader to the contents of \a fileName
    and compiles it.  Returns \c true if the file could be opened and the
    source compiled, false otherwise.

    \sa compileSourceCode()
*/
bool QOpenGLShader::compileSourceFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "QOpenGLShader: Unable to open file" << fileName;
        return false;
    }

    QByteArray contents = file.readAll();
    return compileSourceCode(contents.constData());
}

/*!
    Returns the source code for this shader.

    \sa compileSourceCode()
*/
QByteArray QOpenGLShader::sourceCode() const
{
    Q_D(const QOpenGLShader);
    GLuint shader = d->shaderGuard ? d->shaderGuard->id() : 0;
    if (!shader)
        return QByteArray();
    GLint size = 0;
    d->glfuncs->glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &size);
    if (size <= 0)
        return QByteArray();
    GLint len = 0;
    char *source = new char [size];
    d->glfuncs->glGetShaderSource(shader, size, &len, source);
    QByteArray src(source);
    delete [] source;
    return src;
}

/*!
    Returns \c true if this shader has been compiled; false otherwise.

    \sa compileSourceCode(), compileSourceFile()
*/
bool QOpenGLShader::isCompiled() const
{
    Q_D(const QOpenGLShader);
    return d->compiled;
}

/*!
    Returns the errors and warnings that occurred during the last compile.

    \sa compileSourceCode(), compileSourceFile()
*/
QString QOpenGLShader::log() const
{
    Q_D(const QOpenGLShader);
    return d->log;
}

/*!
    Returns the OpenGL identifier associated with this shader.

    \sa QOpenGLShaderProgram::programId()
*/
GLuint QOpenGLShader::shaderId() const
{
    Q_D(const QOpenGLShader);
    return d->shaderGuard ? d->shaderGuard->id() : 0;
}

class QOpenGLShaderProgramPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLShaderProgram)
public:
    QOpenGLShaderProgramPrivate()
        : programGuard(nullptr)
        , linked(false)
        , inited(false)
        , removingShaders(false)
        , glfuncs(new QOpenGLExtraFunctions)
#ifndef QT_OPENGL_ES_2
        , tessellationFuncs(nullptr)
#endif
        , linkBinaryRecursion(false)
    {
    }
    ~QOpenGLShaderProgramPrivate();

    QOpenGLSharedResourceGuard *programGuard;
    bool linked;
    bool inited;
    bool removingShaders;

    QString log;
    QList<QOpenGLShader *> shaders;
    QList<QOpenGLShader *> anonShaders;

    QOpenGLExtraFunctions *glfuncs;
#ifndef QT_OPENGL_ES_2
    // for tessellation features not in GLES 3.2
    QOpenGLFunctions_4_0_Core *tessellationFuncs;
#endif

    bool hasShader(QOpenGLShader::ShaderType type) const;

    QOpenGLProgramBinaryCache::ProgramDesc binaryProgram;
    bool isCacheDisabled() const;
    bool compileCacheable();
    bool linkBinary();

    bool linkBinaryRecursion;
};

namespace {
    void freeProgramFunc(QOpenGLFunctions *funcs, GLuint id)
    {
        funcs->glDeleteProgram(id);
    }
}


QOpenGLShaderProgramPrivate::~QOpenGLShaderProgramPrivate()
{
    delete glfuncs;
    if (programGuard)
        programGuard->free();
}

bool QOpenGLShaderProgramPrivate::hasShader(QOpenGLShader::ShaderType type) const
{
    for (QOpenGLShader *shader : shaders) {
        if (shader->shaderType() == type)
            return true;
    }
    return false;
}

/*!
    Constructs a new shader program and attaches it to \a parent.
    The program will be invalid until addShader() is called.

    The shader program will be associated with the current QOpenGLContext.

    \sa addShader()
*/
QOpenGLShaderProgram::QOpenGLShaderProgram(QObject *parent)
    : QObject(*new QOpenGLShaderProgramPrivate, parent)
{
}

/*!
    Deletes this shader program.
*/
QOpenGLShaderProgram::~QOpenGLShaderProgram()
{
}

/*!
    Requests the shader program's id to be created immediately. Returns \c true
    if successful; \c false otherwise.

    This function is primarily useful when combining QOpenGLShaderProgram
    with other OpenGL functions that operate directly on the shader
    program id, like \c {GL_OES_get_program_binary}.

    When the shader program is used normally, the shader program's id will
    be created on demand.

    \sa programId()

    \since 5.3
 */
bool QOpenGLShaderProgram::create()
{
    return init();
}

bool QOpenGLShaderProgram::init()
{
    Q_D(QOpenGLShaderProgram);
    if ((d->programGuard && d->programGuard->id()) || d->inited)
        return true;
    d->inited = true;
    QOpenGLContext *context = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
    if (!context)
        return false;
    d->glfuncs->initializeOpenGLFunctions();

#ifndef QT_OPENGL_ES_2
    if (!context->isOpenGLES() && context->format().version() >= qMakePair(4, 0)) {
        d->tessellationFuncs = context->versionFunctions<QOpenGLFunctions_4_0_Core>();
        d->tessellationFuncs->initializeOpenGLFunctions();
    }
#endif

    GLuint program = d->glfuncs->glCreateProgram();
    if (!program) {
        qWarning("QOpenGLShaderProgram: could not create shader program");
        return false;
    }
    if (d->programGuard)
        delete d->programGuard;
    d->programGuard = new QOpenGLSharedResourceGuard(context, program, freeProgramFunc);
    return true;
}

/*!
    Adds a compiled \a shader to this shader program.  Returns \c true
    if the shader could be added, or false otherwise.

    Ownership of the \a shader object remains with the caller.
    It will not be deleted when this QOpenGLShaderProgram instance
    is deleted.  This allows the caller to add the same shader
    to multiple shader programs.

    \sa addShaderFromSourceCode(), addShaderFromSourceFile()
    \sa removeShader(), link(), removeAllShaders()
*/
bool QOpenGLShaderProgram::addShader(QOpenGLShader *shader)
{
    Q_D(QOpenGLShaderProgram);
    if (!init())
        return false;
    if (d->shaders.contains(shader))
        return true;    // Already added to this shader program.
    if (d->programGuard && d->programGuard->id() && shader) {
        if (!shader->d_func()->shaderGuard || !shader->d_func()->shaderGuard->id())
            return false;
        if (d->programGuard->group() != shader->d_func()->shaderGuard->group()) {
            qWarning("QOpenGLShaderProgram::addShader: Program and shader are not associated with same context.");
            return false;
        }
        d->glfuncs->glAttachShader(d->programGuard->id(), shader->d_func()->shaderGuard->id());
        d->linked = false;  // Program needs to be relinked.
        d->shaders.append(shader);
        connect(shader, SIGNAL(destroyed()), this, SLOT(shaderDestroyed()));
        return true;
    } else {
        return false;
    }
}

/*!
    Compiles \a source as a shader of the specified \a type and
    adds it to this shader program.  Returns \c true if compilation
    was successful, false otherwise.  The compilation errors
    and warnings will be made available via log().

    This function is intended to be a short-cut for quickly
    adding vertex and fragment shaders to a shader program without
    creating an instance of QOpenGLShader first.

    \sa addShader(), addShaderFromSourceFile()
    \sa removeShader(), link(), log(), removeAllShaders()
*/
bool QOpenGLShaderProgram::addShaderFromSourceCode(QOpenGLShader::ShaderType type, const char *source)
{
    Q_D(QOpenGLShaderProgram);
    if (!init())
        return false;
    QOpenGLShader *shader = new QOpenGLShader(type, this);
    if (!shader->compileSourceCode(source)) {
        d->log = shader->log();
        delete shader;
        return false;
    }
    d->anonShaders.append(shader);
    return addShader(shader);
}

/*!
    \overload

    Compiles \a source as a shader of the specified \a type and
    adds it to this shader program.  Returns \c true if compilation
    was successful, false otherwise.  The compilation errors
    and warnings will be made available via log().

    This function is intended to be a short-cut for quickly
    adding vertex and fragment shaders to a shader program without
    creating an instance of QOpenGLShader first.

    \sa addShader(), addShaderFromSourceFile()
    \sa removeShader(), link(), log(), removeAllShaders()
*/
bool QOpenGLShaderProgram::addShaderFromSourceCode(QOpenGLShader::ShaderType type, const QByteArray& source)
{
    return addShaderFromSourceCode(type, source.constData());
}

/*!
    \overload

    Compiles \a source as a shader of the specified \a type and
    adds it to this shader program.  Returns \c true if compilation
    was successful, false otherwise.  The compilation errors
    and warnings will be made available via log().

    This function is intended to be a short-cut for quickly
    adding vertex and fragment shaders to a shader program without
    creating an instance of QOpenGLShader first.

    \sa addShader(), addShaderFromSourceFile()
    \sa removeShader(), link(), log(), removeAllShaders()
*/
bool QOpenGLShaderProgram::addShaderFromSourceCode(QOpenGLShader::ShaderType type, const QString& source)
{
    return addShaderFromSourceCode(type, source.toLatin1().constData());
}

/*!
    Compiles the contents of \a fileName as a shader of the specified
    \a type and adds it to this shader program.  Returns \c true if
    compilation was successful, false otherwise.  The compilation errors
    and warnings will be made available via log().

    This function is intended to be a short-cut for quickly
    adding vertex and fragment shaders to a shader program without
    creating an instance of QOpenGLShader first.

    \sa addShader(), addShaderFromSourceCode()
*/
bool QOpenGLShaderProgram::addShaderFromSourceFile
    (QOpenGLShader::ShaderType type, const QString& fileName)
{
    Q_D(QOpenGLShaderProgram);
    if (!init())
        return false;
    QOpenGLShader *shader = new QOpenGLShader(type, this);
    if (!shader->compileSourceFile(fileName)) {
        d->log = shader->log();
        delete shader;
        return false;
    }
    d->anonShaders.append(shader);
    return addShader(shader);
}

/*!
    Registers the shader of the specified \a type and \a source to this
    program. Unlike addShaderFromSourceCode(), this function does not perform
    compilation. Compilation is deferred to link(), and may not happen at all,
    because link() may potentially use a program binary from Qt's shader disk
    cache. This will typically lead to a significant increase in performance.

    \return true if the shader has been registered or, in the non-cached case,
    compiled successfully; false if there was an error. The compilation error
    messages can be retrieved via log().

    When the disk cache is disabled, via Qt::AA_DisableShaderDiskCache for
    example, or the OpenGL context has no support for context binaries, calling
    this function is equivalent to addShaderFromSourceCode().

    \since 5.9
    \sa addShaderFromSourceCode(), addCacheableShaderFromSourceFile()
 */
bool QOpenGLShaderProgram::addCacheableShaderFromSourceCode(QOpenGLShader::ShaderType type, const char *source)
{
    Q_D(QOpenGLShaderProgram);
    if (!init())
        return false;
    if (d->isCacheDisabled())
        return addShaderFromSourceCode(type, source);

    return addCacheableShaderFromSourceCode(type, QByteArray(source));
}

static inline QShader::Stage qt_shaderTypeToStage(QOpenGLShader::ShaderType type)
{
    switch (type) {
    case QOpenGLShader::Vertex:
        return QShader::VertexStage;
    case QOpenGLShader::Fragment:
        return QShader::FragmentStage;
    case QOpenGLShader::Geometry:
        return QShader::GeometryStage;
    case QOpenGLShader::TessellationControl:
        return QShader::TessellationControlStage;
    case QOpenGLShader::TessellationEvaluation:
        return QShader::TessellationEvaluationStage;
    case QOpenGLShader::Compute:
        return QShader::ComputeStage;
    }
    return QShader::VertexStage;
}

static inline QOpenGLShader::ShaderType qt_shaderStageToType(QShader::Stage stage)
{
    switch (stage) {
    case QShader::VertexStage:
        return QOpenGLShader::Vertex;
    case QShader::TessellationControlStage:
        return QOpenGLShader::TessellationControl;
    case QShader::TessellationEvaluationStage:
        return QOpenGLShader::TessellationEvaluation;
    case QShader::GeometryStage:
        return QOpenGLShader::Geometry;
    case QShader::FragmentStage:
        return QOpenGLShader::Fragment;
    case QShader::ComputeStage:
        return QOpenGLShader::Compute;
    }
    return QOpenGLShader::Vertex;
}

/*!
    \overload

    Registers the shader of the specified \a type and \a source to this
    program. Unlike addShaderFromSourceCode(), this function does not perform
    compilation. Compilation is deferred to link(), and may not happen at all,
    because link() may potentially use a program binary from Qt's shader disk
    cache. This will typically lead to a significant increase in performance.

    \return true if the shader has been registered or, in the non-cached case,
    compiled successfully; false if there was an error. The compilation error
    messages can be retrieved via log().

    When the disk cache is disabled, via Qt::AA_DisableShaderDiskCache for
    example, or the OpenGL context has no support for context binaries, calling
    this function is equivalent to addShaderFromSourceCode().

    \since 5.9
    \sa addShaderFromSourceCode(), addCacheableShaderFromSourceFile()
 */
bool QOpenGLShaderProgram::addCacheableShaderFromSourceCode(QOpenGLShader::ShaderType type, const QByteArray &source)
{
    Q_D(QOpenGLShaderProgram);
    if (!init())
        return false;
    if (d->isCacheDisabled())
        return addShaderFromSourceCode(type, source);

    d->binaryProgram.shaders.append(QOpenGLProgramBinaryCache::ShaderDesc(qt_shaderTypeToStage(type), source));
    return true;
}

/*!
    \overload

    Registers the shader of the specified \a type and \a source to this
    program. Unlike addShaderFromSourceCode(), this function does not perform
    compilation. Compilation is deferred to link(), and may not happen at all,
    because link() may potentially use a program binary from Qt's shader disk
    cache. This will typically lead to a significant increase in performance.

    When the disk cache is disabled, via Qt::AA_DisableShaderDiskCache for
    example, or the OpenGL context has no support for context binaries, calling
    this function is equivalent to addShaderFromSourceCode().

    \since 5.9
    \sa addShaderFromSourceCode(), addCacheableShaderFromSourceFile()
 */
bool QOpenGLShaderProgram::addCacheableShaderFromSourceCode(QOpenGLShader::ShaderType type, const QString &source)
{
    Q_D(QOpenGLShaderProgram);
    if (!init())
        return false;
    if (d->isCacheDisabled())
        return addShaderFromSourceCode(type, source);

    return addCacheableShaderFromSourceCode(type, source.toUtf8().constData());
}

/*!
    Registers the shader of the specified \a type and \a fileName to this
    program. Unlike addShaderFromSourceFile(), this function does not perform
    compilation. Compilation is deferred to link(), and may not happen at all,
    because link() may potentially use a program binary from Qt's shader disk
    cache. This will typically lead to a significant increase in performance.

    \return true if the file has been read successfully, false if the file could
    not be opened or the normal, non-cached compilation of the shader has
    failed. The compilation error messages can be retrieved via log().

    When the disk cache is disabled, via Qt::AA_DisableShaderDiskCache for
    example, or the OpenGL context has no support for context binaries, calling
    this function is equivalent to addShaderFromSourceFile().

    \since 5.9
    \sa addShaderFromSourceFile(), addCacheableShaderFromSourceCode()
 */
bool QOpenGLShaderProgram::addCacheableShaderFromSourceFile(QOpenGLShader::ShaderType type, const QString &fileName)
{
    Q_D(QOpenGLShaderProgram);
    if (!init())
        return false;
    if (d->isCacheDisabled())
        return addShaderFromSourceFile(type, fileName);

    QOpenGLProgramBinaryCache::ShaderDesc shader(qt_shaderTypeToStage(type));
    // NB! It could be tempting to defer reading the file contents and just
    // hash the filename as the cache key, perhaps combined with last-modified
    // timestamp checks. However, this would raise a number of issues (no
    // timestamps for files in the resource system; preference for global, not
    // per-application cache items (where filenames may clash); resource-based
    // shaders from libraries like Qt Quick; etc.), so just avoid it.
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        shader.source = f.readAll();
        f.close();
    } else {
        qWarning("QOpenGLShaderProgram: Unable to open file %s", qPrintable(fileName));
        return false;
    }
    d->binaryProgram.shaders.append(shader);
    return true;
}

/*!
    Removes \a shader from this shader program.  The object is not deleted.

    The shader program must be valid in the current QOpenGLContext.

    \sa addShader(), link(), removeAllShaders()
*/
void QOpenGLShaderProgram::removeShader(QOpenGLShader *shader)
{
    Q_D(QOpenGLShaderProgram);
    if (d->programGuard && d->programGuard->id()
        && shader && shader->d_func()->shaderGuard)
    {
        d->glfuncs->glDetachShader(d->programGuard->id(), shader->d_func()->shaderGuard->id());
    }
    d->linked = false;  // Program needs to be relinked.
    if (shader) {
        d->shaders.removeAll(shader);
        d->anonShaders.removeAll(shader);
        disconnect(shader, SIGNAL(destroyed()), this, SLOT(shaderDestroyed()));
    }
}

/*!
    Returns a list of all shaders that have been added to this shader
    program using addShader().

    \sa addShader(), removeShader()
*/
QList<QOpenGLShader *> QOpenGLShaderProgram::shaders() const
{
    Q_D(const QOpenGLShaderProgram);
    return d->shaders;
}

/*!
    Removes all of the shaders that were added to this program previously.
    The QOpenGLShader objects for the shaders will not be deleted if they
    were constructed externally.  QOpenGLShader objects that are constructed
    internally by QOpenGLShaderProgram will be deleted.

    \sa addShader(), removeShader()
*/
void QOpenGLShaderProgram::removeAllShaders()
{
    Q_D(QOpenGLShaderProgram);
    d->removingShaders = true;
    for (QOpenGLShader *shader : qAsConst(d->shaders)) {
        if (d->programGuard && d->programGuard->id()
            && shader && shader->d_func()->shaderGuard)
        {
            d->glfuncs->glDetachShader(d->programGuard->id(), shader->d_func()->shaderGuard->id());
        }
    }
    // Delete shader objects that were created anonymously.
    qDeleteAll(d->anonShaders);
    d->shaders.clear();
    d->anonShaders.clear();
    d->binaryProgram = QOpenGLProgramBinaryCache::ProgramDesc();
    d->linked = false;  // Program needs to be relinked.
    d->removingShaders = false;
}

/*!
    Links together the shaders that were added to this program with
    addShader().  Returns \c true if the link was successful or
    false otherwise.  If the link failed, the error messages can
    be retrieved with log().

    Subclasses can override this function to initialize attributes
    and uniform variables for use in specific shader programs.

    If the shader program was already linked, calling this
    function again will force it to be re-linked.

    When shaders were added to this program via
    addCacheableShaderFromSourceCode() or addCacheableShaderFromSourceFile(),
    program binaries are supported, and a cached binary is available on disk,
    actual compilation and linking are skipped. Instead, link() will initialize
    the program with the binary blob via glProgramBinary(). If there is no
    cached version of the program or it was generated with a different driver
    version, the shaders will be compiled from source and the program will get
    linked normally. This allows seamless upgrading of the graphics drivers,
    without having to worry about potentially incompatible binary formats.

    \sa addShader(), log()
*/
bool QOpenGLShaderProgram::link()
{
    Q_D(QOpenGLShaderProgram);
    GLuint program = d->programGuard ? d->programGuard->id() : 0;
    if (!program)
        return false;

    if (!d->linkBinaryRecursion && d->shaders.isEmpty() && !d->binaryProgram.shaders.isEmpty())
        return d->linkBinary();

    GLint value;
    if (d->shaders.isEmpty()) {
        // If there are no explicit shaders, then it is possible that the
        // application added a program binary with glProgramBinaryOES(), or
        // otherwise populated the shaders itself. This is also the case when
        // we are recursively called back from linkBinary() after a successful
        // glProgramBinary(). Check to see if the program is already linked and
        // bail out if so.
        value = 0;
        d->glfuncs->glGetProgramiv(program, GL_LINK_STATUS, &value);
        d->linked = (value != 0);
        if (d->linked)
            return true;
    }

    d->glfuncs->glLinkProgram(program);
    value = 0;
    d->glfuncs->glGetProgramiv(program, GL_LINK_STATUS, &value);
    d->linked = (value != 0);
    value = 0;
    d->glfuncs->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &value);
    d->log = QString();
    if (value > 1) {
        char *logbuf = new char [value];
        GLint len;
        d->glfuncs->glGetProgramInfoLog(program, value, &len, logbuf);
        d->log = QString::fromLatin1(logbuf);
        if (!d->linked && !d->linkBinaryRecursion) {
            QString name = objectName();
            if (name.isEmpty())
                qWarning("QOpenGLShader::link: %ls", qUtf16Printable(d->log));
            else
                qWarning("QOpenGLShader::link[%ls]: %ls", qUtf16Printable(name), qUtf16Printable(d->log));
        }
        delete [] logbuf;
    }
    return d->linked;
}

/*!
    Returns \c true if this shader program has been linked; false otherwise.

    \sa link()
*/
bool QOpenGLShaderProgram::isLinked() const
{
    Q_D(const QOpenGLShaderProgram);
    return d->linked;
}

/*!
    Returns the errors and warnings that occurred during the last link()
    or addShader() with explicitly specified source code.

    \sa link()
*/
QString QOpenGLShaderProgram::log() const
{
    Q_D(const QOpenGLShaderProgram);
    return d->log;
}

/*!
    Binds this shader program to the active QOpenGLContext and makes
    it the current shader program.  Any previously bound shader program
    is released.  This is equivalent to calling \c{glUseProgram()} on
    programId().  Returns \c true if the program was successfully bound;
    false otherwise.  If the shader program has not yet been linked,
    or it needs to be re-linked, this function will call link().

    \sa link(), release()
*/
bool QOpenGLShaderProgram::bind()
{
    Q_D(QOpenGLShaderProgram);
    GLuint program = d->programGuard ? d->programGuard->id() : 0;
    if (!program)
        return false;
    if (!d->linked && !link())
        return false;
#ifndef QT_NO_DEBUG
    if (d->programGuard->group() != QOpenGLContextGroup::currentContextGroup()) {
        qWarning("QOpenGLShaderProgram::bind: program is not valid in the current context.");
        return false;
    }
#endif
    d->glfuncs->glUseProgram(program);
    return true;
}

/*!
    Releases the active shader program from the current QOpenGLContext.
    This is equivalent to calling \c{glUseProgram(0)}.

    \sa bind()
*/
void QOpenGLShaderProgram::release()
{
    Q_D(QOpenGLShaderProgram);
#ifndef QT_NO_DEBUG
    if (d->programGuard && d->programGuard->group() != QOpenGLContextGroup::currentContextGroup())
        qWarning("QOpenGLShaderProgram::release: program is not valid in the current context.");
#endif
    d->glfuncs->glUseProgram(0);
}

/*!
    Returns the OpenGL identifier associated with this shader program.

    \sa QOpenGLShader::shaderId()
*/
GLuint QOpenGLShaderProgram::programId() const
{
    Q_D(const QOpenGLShaderProgram);
    GLuint id = d->programGuard ? d->programGuard->id() : 0;
    if (id)
        return id;

    // Create the identifier if we don't have one yet.  This is for
    // applications that want to create the attached shader configuration
    // themselves, particularly those using program binaries.
    if (!const_cast<QOpenGLShaderProgram *>(this)->init())
        return 0;
    return d->programGuard ? d->programGuard->id() : 0;
}

/*!
    Binds the attribute \a name to the specified \a location.  This
    function can be called before or after the program has been linked.
    Any attributes that have not been explicitly bound when the program
    is linked will be assigned locations automatically.

    When this function is called after the program has been linked,
    the program will need to be relinked for the change to take effect.

    \sa attributeLocation()
*/
void QOpenGLShaderProgram::bindAttributeLocation(const char *name, int location)
{
    Q_D(QOpenGLShaderProgram);
    if (!init() || !d->programGuard || !d->programGuard->id())
        return;
    d->glfuncs->glBindAttribLocation(d->programGuard->id(), location, name);
    d->linked = false;  // Program needs to be relinked.
}

/*!
    \overload

    Binds the attribute \a name to the specified \a location.  This
    function can be called before or after the program has been linked.
    Any attributes that have not been explicitly bound when the program
    is linked will be assigned locations automatically.

    When this function is called after the program has been linked,
    the program will need to be relinked for the change to take effect.

    \sa attributeLocation()
*/
void QOpenGLShaderProgram::bindAttributeLocation(const QByteArray& name, int location)
{
    bindAttributeLocation(name.constData(), location);
}

/*!
    \overload

    Binds the attribute \a name to the specified \a location.  This
    function can be called before or after the program has been linked.
    Any attributes that have not been explicitly bound when the program
    is linked will be assigned locations automatically.

    When this function is called after the program has been linked,
    the program will need to be relinked for the change to take effect.

    \sa attributeLocation()
*/
void QOpenGLShaderProgram::bindAttributeLocation(const QString& name, int location)
{
    bindAttributeLocation(name.toLatin1().constData(), location);
}

/*!
    Returns the location of the attribute \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    attribute for this shader program.

    \sa uniformLocation(), bindAttributeLocation()
*/
int QOpenGLShaderProgram::attributeLocation(const char *name) const
{
    Q_D(const QOpenGLShaderProgram);
    if (d->linked && d->programGuard && d->programGuard->id()) {
        return d->glfuncs->glGetAttribLocation(d->programGuard->id(), name);
    } else {
        qWarning("QOpenGLShaderProgram::attributeLocation(%s): shader program is not linked", name);
        return -1;
    }
}

/*!
    \overload

    Returns the location of the attribute \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    attribute for this shader program.

    \sa uniformLocation(), bindAttributeLocation()
*/
int QOpenGLShaderProgram::attributeLocation(const QByteArray& name) const
{
    return attributeLocation(name.constData());
}

/*!
    \overload

    Returns the location of the attribute \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    attribute for this shader program.

    \sa uniformLocation(), bindAttributeLocation()
*/
int QOpenGLShaderProgram::attributeLocation(const QString& name) const
{
    return attributeLocation(name.toLatin1().constData());
}

/*!
    Sets the attribute at \a location in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, GLfloat value)
{
    Q_D(QOpenGLShaderProgram);
    if (location != -1)
        d->glfuncs->glVertexAttrib1fv(location, &value);
}

/*!
    \overload

    Sets the attribute called \a name in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, GLfloat value)
{
    setAttributeValue(attributeLocation(name), value);
}

/*!
    Sets the attribute at \a location in the current context to
    the 2D vector (\a x, \a y).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, GLfloat x, GLfloat y)
{
    Q_D(QOpenGLShaderProgram);
    if (location != -1) {
        GLfloat values[2] = {x, y};
        d->glfuncs->glVertexAttrib2fv(location, values);
    }
}

/*!
    \overload

    Sets the attribute called \a name in the current context to
    the 2D vector (\a x, \a y).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, GLfloat x, GLfloat y)
{
    setAttributeValue(attributeLocation(name), x, y);
}

/*!
    Sets the attribute at \a location in the current context to
    the 3D vector (\a x, \a y, \a z).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
        (int location, GLfloat x, GLfloat y, GLfloat z)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[3] = {x, y, z};
        d->glfuncs->glVertexAttrib3fv(location, values);
    }
}

/*!
    \overload

    Sets the attribute called \a name in the current context to
    the 3D vector (\a x, \a y, \a z).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
        (const char *name, GLfloat x, GLfloat y, GLfloat z)
{
    setAttributeValue(attributeLocation(name), x, y, z);
}

/*!
    Sets the attribute at \a location in the current context to
    the 4D vector (\a x, \a y, \a z, \a w).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
        (int location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    Q_D(QOpenGLShaderProgram);
    if (location != -1) {
        GLfloat values[4] = {x, y, z, w};
        d->glfuncs->glVertexAttrib4fv(location, values);
    }
}

/*!
    \overload

    Sets the attribute called \a name in the current context to
    the 4D vector (\a x, \a y, \a z, \a w).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
        (const char *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    setAttributeValue(attributeLocation(name), x, y, z, w);
}

/*!
    Sets the attribute at \a location in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, const QVector2D& value)
{
    Q_D(QOpenGLShaderProgram);
    if (location != -1)
        d->glfuncs->glVertexAttrib2fv(location, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the attribute called \a name in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, const QVector2D& value)
{
    setAttributeValue(attributeLocation(name), value);
}

/*!
    Sets the attribute at \a location in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, const QVector3D& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glVertexAttrib3fv(location, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the attribute called \a name in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, const QVector3D& value)
{
    setAttributeValue(attributeLocation(name), value);
}

/*!
    Sets the attribute at \a location in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, const QVector4D& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glVertexAttrib4fv(location, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the attribute called \a name in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, const QVector4D& value)
{
    setAttributeValue(attributeLocation(name), value);
}

/*!
    Sets the attribute at \a location in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, const QColor& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(value.redF()), GLfloat(value.greenF()),
                             GLfloat(value.blueF()), GLfloat(value.alphaF())};
        d->glfuncs->glVertexAttrib4fv(location, values);
    }
}

/*!
    \overload

    Sets the attribute called \a name in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, const QColor& value)
{
    setAttributeValue(attributeLocation(name), value);
}

/*!
    Sets the attribute at \a location in the current context to the
    contents of \a values, which contains \a columns elements, each
    consisting of \a rows elements.  The \a rows value should be
    1, 2, 3, or 4.  This function is typically used to set matrix
    values and column vectors.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
    (int location, const GLfloat *values, int columns, int rows)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (rows < 1 || rows > 4) {
        qWarning("QOpenGLShaderProgram::setAttributeValue: rows %d not supported", rows);
        return;
    }
    if (location != -1) {
        while (columns-- > 0) {
            if (rows == 1)
                d->glfuncs->glVertexAttrib1fv(location, values);
            else if (rows == 2)
                d->glfuncs->glVertexAttrib2fv(location, values);
            else if (rows == 3)
                d->glfuncs->glVertexAttrib3fv(location, values);
            else
                d->glfuncs->glVertexAttrib4fv(location, values);
            values += rows;
            ++location;
        }
    }
}

/*!
    \overload

    Sets the attribute called \a name in the current context to the
    contents of \a values, which contains \a columns elements, each
    consisting of \a rows elements.  The \a rows value should be
    1, 2, 3, or 4.  This function is typically used to set matrix
    values and column vectors.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
    (const char *name, const GLfloat *values, int columns, int rows)
{
    setAttributeValue(attributeLocation(name), values, columns, rows);
}

/*!
    Sets an array of vertex \a values on the attribute at \a location
    in this shader program.  The \a tupleSize indicates the number of
    components per vertex (1, 2, 3, or 4), and the \a stride indicates
    the number of bytes between vertices.  A default \a stride value
    of zero indicates that the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
    (int location, const GLfloat *values, int tupleSize, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, tupleSize, GL_FLOAT, GL_FALSE,
                              stride, values);
    }
}

/*!
    Sets an array of 2D vertex \a values on the attribute at \a location
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (int location, const QVector2D *values, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE,
                              stride, values);
    }
}

/*!
    Sets an array of 3D vertex \a values on the attribute at \a location
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (int location, const QVector3D *values, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE,
                              stride, values);
    }
}

/*!
    Sets an array of 4D vertex \a values on the attribute at \a location
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (int location, const QVector4D *values, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE,
                              stride, values);
    }
}

/*!
    Sets an array of vertex \a values on the attribute at \a location
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The \a type indicates the type of elements in the \a values array,
    usually \c{GL_FLOAT}, \c{GL_UNSIGNED_BYTE}, etc.  The \a tupleSize
    indicates the number of components per vertex: 1, 2, 3, or 4.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    The setAttributeBuffer() function can be used to set the attribute
    array to an offset within a vertex buffer.

    \note Normalization will be enabled. If this is not desired, call
    glVertexAttribPointer directly through QOpenGLFunctions.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray(), setAttributeBuffer()
*/
void QOpenGLShaderProgram::setAttributeArray
    (int location, GLenum type, const void *values, int tupleSize, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, tupleSize, type, GL_TRUE,
                              stride, values);
    }
}

/*!
    \overload

    Sets an array of vertex \a values on the attribute called \a name
    in this shader program.  The \a tupleSize indicates the number of
    components per vertex (1, 2, 3, or 4), and the \a stride indicates
    the number of bytes between vertices.  A default \a stride value
    of zero indicates that the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on \a name.  Otherwise the value specified with setAttributeValue()
    for \a name will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
    (const char *name, const GLfloat *values, int tupleSize, int stride)
{
    setAttributeArray(attributeLocation(name), values, tupleSize, stride);
}

/*!
    \overload

    Sets an array of 2D vertex \a values on the attribute called \a name
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on \a name.  Otherwise the value specified with setAttributeValue()
    for \a name will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (const char *name, const QVector2D *values, int stride)
{
    setAttributeArray(attributeLocation(name), values, stride);
}

/*!
    \overload

    Sets an array of 3D vertex \a values on the attribute called \a name
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on \a name.  Otherwise the value specified with setAttributeValue()
    for \a name will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (const char *name, const QVector3D *values, int stride)
{
    setAttributeArray(attributeLocation(name), values, stride);
}

/*!
    \overload

    Sets an array of 4D vertex \a values on the attribute called \a name
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on \a name.  Otherwise the value specified with setAttributeValue()
    for \a name will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (const char *name, const QVector4D *values, int stride)
{
    setAttributeArray(attributeLocation(name), values, stride);
}

/*!
    \overload

    Sets an array of vertex \a values on the attribute called \a name
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The \a type indicates the type of elements in the \a values array,
    usually \c{GL_FLOAT}, \c{GL_UNSIGNED_BYTE}, etc.  The \a tupleSize
    indicates the number of components per vertex: 1, 2, 3, or 4.

    The array will become active when enableAttributeArray() is called
    on the \a name.  Otherwise the value specified with
    setAttributeValue() for \a name will be used.

    The setAttributeBuffer() function can be used to set the attribute
    array to an offset within a vertex buffer.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray(), setAttributeBuffer()
*/
void QOpenGLShaderProgram::setAttributeArray
    (const char *name, GLenum type, const void *values, int tupleSize, int stride)
{
    setAttributeArray(attributeLocation(name), type, values, tupleSize, stride);
}

/*!
    Sets an array of vertex values on the attribute at \a location in
    this shader program, starting at a specific \a offset in the
    currently bound vertex buffer.  The \a stride indicates the number
    of bytes between vertices.  A default \a stride value of zero
    indicates that the vertices are densely packed in the value array.

    The \a type indicates the type of elements in the vertex value
    array, usually \c{GL_FLOAT}, \c{GL_UNSIGNED_BYTE}, etc.  The \a
    tupleSize indicates the number of components per vertex: 1, 2, 3,
    or 4.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    \note Normalization will be enabled. If this is not desired, call
    glVertexAttribPointer directly through QOpenGLFunctions.

    \sa setAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeBuffer
    (int location, GLenum type, int offset, int tupleSize, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, tupleSize, type, GL_TRUE, stride,
                              reinterpret_cast<const void *>(qintptr(offset)));
    }
}

/*!
    \overload

    Sets an array of vertex values on the attribute called \a name
    in this shader program, starting at a specific \a offset in the
    currently bound vertex buffer.  The \a stride indicates the number
    of bytes between vertices.  A default \a stride value of zero
    indicates that the vertices are densely packed in the value array.

    The \a type indicates the type of elements in the vertex value
    array, usually \c{GL_FLOAT}, \c{GL_UNSIGNED_BYTE}, etc.  The \a
    tupleSize indicates the number of components per vertex: 1, 2, 3,
    or 4.

    The array will become active when enableAttributeArray() is called
    on the \a name.  Otherwise the value specified with
    setAttributeValue() for \a name will be used.

    \sa setAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeBuffer
    (const char *name, GLenum type, int offset, int tupleSize, int stride)
{
    setAttributeBuffer(attributeLocation(name), type, offset, tupleSize, stride);
}

/*!
    Enables the vertex array at \a location in this shader program
    so that the value set by setAttributeArray() on \a location
    will be used by the shader program.

    \sa disableAttributeArray(), setAttributeArray(), setAttributeValue()
    \sa setUniformValue()
*/
void QOpenGLShaderProgram::enableAttributeArray(int location)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glEnableVertexAttribArray(location);
}

/*!
    \overload

    Enables the vertex array called \a name in this shader program
    so that the value set by setAttributeArray() on \a name
    will be used by the shader program.

    \sa disableAttributeArray(), setAttributeArray(), setAttributeValue()
    \sa setUniformValue()
*/
void QOpenGLShaderProgram::enableAttributeArray(const char *name)
{
    enableAttributeArray(attributeLocation(name));
}

/*!
    Disables the vertex array at \a location in this shader program
    that was enabled by a previous call to enableAttributeArray().

    \sa enableAttributeArray(), setAttributeArray(), setAttributeValue()
    \sa setUniformValue()
*/
void QOpenGLShaderProgram::disableAttributeArray(int location)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glDisableVertexAttribArray(location);
}

/*!
    \overload

    Disables the vertex array called \a name in this shader program
    that was enabled by a previous call to enableAttributeArray().

    \sa enableAttributeArray(), setAttributeArray(), setAttributeValue()
    \sa setUniformValue()
*/
void QOpenGLShaderProgram::disableAttributeArray(const char *name)
{
    disableAttributeArray(attributeLocation(name));
}

/*!
    Returns the location of the uniform variable \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    uniform variable for this shader program.

    \sa attributeLocation()
*/
int QOpenGLShaderProgram::uniformLocation(const char *name) const
{
    Q_D(const QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (d->linked && d->programGuard && d->programGuard->id()) {
        return d->glfuncs->glGetUniformLocation(d->programGuard->id(), name);
    } else {
        qWarning("QOpenGLShaderProgram::uniformLocation(%s): shader program is not linked", name);
        return -1;
    }
}

/*!
    \overload

    Returns the location of the uniform variable \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    uniform variable for this shader program.

    \sa attributeLocation()
*/
int QOpenGLShaderProgram::uniformLocation(const QByteArray& name) const
{
    return uniformLocation(name.constData());
}

/*!
    \overload

    Returns the location of the uniform variable \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    uniform variable for this shader program.

    \sa attributeLocation()
*/
int QOpenGLShaderProgram::uniformLocation(const QString& name) const
{
    return uniformLocation(name.toLatin1().constData());
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, GLfloat value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform1fv(location, 1, &value);
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, GLfloat value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, GLint value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform1i(location, value);
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, GLint value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.
    This function should be used when setting sampler values.

    \note This function is not aware of unsigned int support in modern OpenGL
    versions and therefore treats \a value as a GLint and calls glUniform1i.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, GLuint value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform1i(location, value);
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.  This function should be used when setting sampler values.

    \note This function is not aware of unsigned int support in modern OpenGL
    versions and therefore treats \a value as a GLint and calls glUniform1i.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, GLuint value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the 2D vector (\a x, \a y).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, GLfloat x, GLfloat y)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[2] = {x, y};
        d->glfuncs->glUniform2fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context to
    the 2D vector (\a x, \a y).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, GLfloat x, GLfloat y)
{
    setUniformValue(uniformLocation(name), x, y);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the 3D vector (\a x, \a y, \a z).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue
        (int location, GLfloat x, GLfloat y, GLfloat z)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[3] = {x, y, z};
        d->glfuncs->glUniform3fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context to
    the 3D vector (\a x, \a y, \a z).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue
        (const char *name, GLfloat x, GLfloat y, GLfloat z)
{
    setUniformValue(uniformLocation(name), x, y, z);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the 4D vector (\a x, \a y, \a z, \a w).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue
        (int location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {x, y, z, w};
        d->glfuncs->glUniform4fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context to
    the 4D vector (\a x, \a y, \a z, \a w).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue
        (const char *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    setUniformValue(uniformLocation(name), x, y, z, w);
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QVector2D& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform2fv(location, 1, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QVector2D& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QVector3D& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform3fv(location, 1, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QVector3D& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QVector4D& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform4fv(location, 1, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QVector4D& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the red, green, blue, and alpha components of \a color.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QColor& color)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(color.redF()), GLfloat(color.greenF()),
                             GLfloat(color.blueF()), GLfloat(color.alphaF())};
        d->glfuncs->glUniform4fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context to
    the red, green, blue, and alpha components of \a color.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QColor& color)
{
    setUniformValue(uniformLocation(name), color);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the x and y coordinates of \a point.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QPoint& point)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(point.x()), GLfloat(point.y())};
        d->glfuncs->glUniform2fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable associated with \a name in the current
    context to the x and y coordinates of \a point.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QPoint& point)
{
    setUniformValue(uniformLocation(name), point);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the x and y coordinates of \a point.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QPointF& point)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(point.x()), GLfloat(point.y())};
        d->glfuncs->glUniform2fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable associated with \a name in the current
    context to the x and y coordinates of \a point.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QPointF& point)
{
    setUniformValue(uniformLocation(name), point);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the width and height of the given \a size.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QSize& size)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(size.width()), GLfloat(size.height())};
        d->glfuncs->glUniform2fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable associated with \a name in the current
    context to the width and height of the given \a size.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QSize& size)
{
    setUniformValue(uniformLocation(name), size);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the width and height of the given \a size.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QSizeF& size)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(size.width()), GLfloat(size.height())};
        d->glfuncs->glUniform2fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable associated with \a name in the current
    context to the width and height of the given \a size.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QSizeF& size)
{
    setUniformValue(uniformLocation(name), size);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 2x2 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix2x2& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniformMatrix2fv(location, 1, GL_FALSE, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 2x2 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix2x2& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 2x3 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat2x3, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec3.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix2x3& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform3fv(location, 2, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 2x3 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat2x3, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec3.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix2x3& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 2x4 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat2x4, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec4.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix2x4& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform4fv(location, 2, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 2x4 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat2x4, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec4.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix2x4& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 3x2 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat3x2, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec2.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix3x2& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform2fv(location, 3, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 3x2 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat3x2, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec2.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix3x2& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 3x3 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix3x3& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniformMatrix3fv(location, 1, GL_FALSE, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 3x3 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix3x3& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 3x4 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat3x4, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec4.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix3x4& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform4fv(location, 3, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 3x4 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat3x4, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec4.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix3x4& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 4x2 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat4x2, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec2.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix4x2& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform2fv(location, 4, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 4x2 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat4x2, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec2.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix4x2& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 4x3 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat4x3, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec3.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix4x3& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform3fv(location, 4, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 4x3 matrix \a value.

    \note This function is not aware of non square matrix support,
    that is, GLSL types like mat4x3, that is present in modern OpenGL
    versions. Instead, it treats the uniform as an array of vec3.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix4x3& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 4x4 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix4x4& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniformMatrix4fv(location, 1, GL_FALSE, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 4x4 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix4x4& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    \overload

    Sets the uniform variable at \a location in the current context
    to a 2x2 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const GLfloat value[2][2])
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniformMatrix2fv(location, 1, GL_FALSE, value[0]);
}

/*!
    \overload

    Sets the uniform variable at \a location in the current context
    to a 3x3 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const GLfloat value[3][3])
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniformMatrix3fv(location, 1, GL_FALSE, value[0]);
}

/*!
    \overload

    Sets the uniform variable at \a location in the current context
    to a 4x4 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const GLfloat value[4][4])
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniformMatrix4fv(location, 1, GL_FALSE, value[0]);
}


/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 2x2 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const GLfloat value[2][2])
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 3x3 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const GLfloat value[3][3])
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 4x4 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const GLfloat value[4][4])
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to a
    3x3 transformation matrix \a value that is specified as a QTransform value.

    To set a QTransform value as a 4x4 matrix in a shader, use
    \c{setUniformValue(location, QMatrix4x4(value))}.
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QTransform& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat mat[3][3] = {
            {GLfloat(value.m11()), GLfloat(value.m12()), GLfloat(value.m13())},
            {GLfloat(value.m21()), GLfloat(value.m22()), GLfloat(value.m23())},
            {GLfloat(value.m31()), GLfloat(value.m32()), GLfloat(value.m33())}
        };
        d->glfuncs->glUniformMatrix3fv(location, 1, GL_FALSE, mat[0]);
    }
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context to a
    3x3 transformation matrix \a value that is specified as a QTransform value.

    To set a QTransform value as a 4x4 matrix in a shader, use
    \c{setUniformValue(name, QMatrix4x4(value))}.
*/
void QOpenGLShaderProgram::setUniformValue
        (const char *name, const QTransform& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const GLint *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform1iv(location, count, values);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray
        (const char *name, const GLint *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count elements of \a values.  This overload
    should be used when setting an array of sampler values.

    \note This function is not aware of unsigned int support in modern OpenGL
    versions and therefore treats \a values as a GLint and calls glUniform1iv.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const GLuint *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform1iv(location, count, reinterpret_cast<const GLint *>(values));
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count elements of \a values.  This overload
    should be used when setting an array of sampler values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray
        (const char *name, const GLuint *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count elements of \a values.  Each element
    has \a tupleSize components.  The \a tupleSize must be 1, 2, 3, or 4.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const GLfloat *values, int count, int tupleSize)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        if (tupleSize == 1)
            d->glfuncs->glUniform1fv(location, count, values);
        else if (tupleSize == 2)
            d->glfuncs->glUniform2fv(location, count, values);
        else if (tupleSize == 3)
            d->glfuncs->glUniform3fv(location, count, values);
        else if (tupleSize == 4)
            d->glfuncs->glUniform4fv(location, count, values);
        else
            qWarning("QOpenGLShaderProgram::setUniformValue: size %d not supported", tupleSize);
    }
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count elements of \a values.  Each element
    has \a tupleSize components.  The \a tupleSize must be 1, 2, 3, or 4.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray
        (const char *name, const GLfloat *values, int count, int tupleSize)
{
    setUniformValueArray(uniformLocation(name), values, count, tupleSize);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 2D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QVector2D *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform2fv(location, count, reinterpret_cast<const GLfloat *>(values));
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 2D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QVector2D *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 3D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QVector3D *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform3fv(location, count, reinterpret_cast<const GLfloat *>(values));
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 3D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QVector3D *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 4D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QVector4D *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform4fv(location, count, reinterpret_cast<const GLfloat *>(values));
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 4D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QVector4D *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

// We have to repack matrix arrays from qreal to GLfloat.
#define setUniformMatrixArray(func,location,values,count,type,cols,rows) \
    if (location == -1 || count <= 0) \
        return; \
    if (sizeof(type) == sizeof(GLfloat) * cols * rows) { \
        func(location, count, GL_FALSE, \
             reinterpret_cast<const GLfloat *>(values[0].constData())); \
    } else { \
        QVarLengthArray<GLfloat> temp(cols * rows * count); \
        for (int index = 0; index < count; ++index) { \
            for (int index2 = 0; index2 < (cols * rows); ++index2) { \
                temp.data()[cols * rows * index + index2] = \
                    values[index].constData()[index2]; \
            } \
        } \
        func(location, count, GL_FALSE, temp.constData()); \
    }
#define setUniformGenericMatrixArray(colfunc,location,values,count,type,cols,rows) \
    if (location == -1 || count <= 0) \
        return; \
    if (sizeof(type) == sizeof(GLfloat) * cols * rows) { \
        const GLfloat *data = reinterpret_cast<const GLfloat *> \
            (values[0].constData());  \
        colfunc(location, count * cols, data); \
    } else { \
        QVarLengthArray<GLfloat> temp(cols * rows * count); \
        for (int index = 0; index < count; ++index) { \
            for (int index2 = 0; index2 < (cols * rows); ++index2) { \
                temp.data()[cols * rows * index + index2] = \
                    values[index].constData()[index2]; \
            } \
        } \
        colfunc(location, count * cols, temp.constData()); \
    }

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 2x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix2x2 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformMatrixArray
        (d->glfuncs->glUniformMatrix2fv, location, values, count, QMatrix2x2, 2, 2);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 2x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix2x2 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 2x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix2x3 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform3fv, location, values, count,
         QMatrix2x3, 2, 3);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 2x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix2x3 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 2x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix2x4 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform4fv, location, values, count,
         QMatrix2x4, 2, 4);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 2x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix2x4 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 3x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix3x2 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform2fv, location, values, count,
         QMatrix3x2, 3, 2);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 3x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix3x2 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 3x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix3x3 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformMatrixArray
        (d->glfuncs->glUniformMatrix3fv, location, values, count, QMatrix3x3, 3, 3);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 3x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix3x3 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 3x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix3x4 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform4fv, location, values, count,
         QMatrix3x4, 3, 4);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 3x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix3x4 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 4x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix4x2 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform2fv, location, values, count,
         QMatrix4x2, 4, 2);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 4x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix4x2 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 4x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix4x3 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform3fv, location, values, count,
         QMatrix4x3, 4, 3);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 4x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix4x3 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 4x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix4x4 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformMatrixArray
        (d->glfuncs->glUniformMatrix4fv, location, values, count, QMatrix4x4, 4, 4);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 4x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix4x4 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Returns the hardware limit for how many vertices a geometry shader
    can output.
*/
int QOpenGLShaderProgram::maxGeometryOutputVertices() const
{
    GLint n = 0;
    Q_D(const QOpenGLShaderProgram);
    d->glfuncs->glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &n);
    return n;
}

/*!
    Use this function to specify to OpenGL the number of vertices in
    a patch to \a count. A patch is a custom OpenGL primitive whose interpretation
    is entirely defined by the tessellation shader stages. Therefore, calling
    this function only makes sense when using a QOpenGLShaderProgram
    containing tessellation stage shaders. When using OpenGL tessellation,
    the only primitive that can be rendered with \c{glDraw*()} functions is
    \c{GL_PATCHES}.

    This is equivalent to calling glPatchParameteri(GL_PATCH_VERTICES, count).

    \note This modifies global OpenGL state and is not specific to this
    QOpenGLShaderProgram instance. You should call this in your render
    function when needed, as QOpenGLShaderProgram will not apply this for
    you. This is purely a convenience function.

    \sa patchVertexCount()
*/
void QOpenGLShaderProgram::setPatchVertexCount(int count)
{
    Q_D(QOpenGLShaderProgram);
    d->glfuncs->glPatchParameteri(GL_PATCH_VERTICES, count);
}

/*!
    Returns the number of vertices per-patch to be used when rendering.

    \note This returns the global OpenGL state value. It is not specific to
    this QOpenGLShaderProgram instance.

    \sa setPatchVertexCount()
*/
int QOpenGLShaderProgram::patchVertexCount() const
{
    int patchVertices = 0;
    Q_D(const QOpenGLShaderProgram);
    d->glfuncs->glGetIntegerv(GL_PATCH_VERTICES, &patchVertices);
    return patchVertices;
}

/*!
    Sets the default outer tessellation levels to be used by the tessellation
    primitive generator in the event that the tessellation control shader
    does not output them to \a levels. For more details on OpenGL and Tessellation
    shaders see \l{OpenGL Tessellation Shaders}.

    The \a levels argument should be a QVector consisting of 4 floats. Not all
    of the values make sense for all tessellation modes. If you specify a vector with
    fewer than 4 elements, the remaining elements will be given a default value of 1.

    \note This modifies global OpenGL state and is not specific to this
    QOpenGLShaderProgram instance. You should call this in your render
    function when needed, as QOpenGLShaderProgram will not apply this for
    you. This is purely a convenience function.

    \note This function is only available with OpenGL >= 4.0 and is not supported
    with OpenGL ES 3.2.

    \sa defaultOuterTessellationLevels(), setDefaultInnerTessellationLevels()
*/
void QOpenGLShaderProgram::setDefaultOuterTessellationLevels(const QVector<float> &levels)
{
#ifndef QT_OPENGL_ES_2
    Q_D(QOpenGLShaderProgram);
    if (d->tessellationFuncs) {
        QVector<float> tessLevels = levels;

        // Ensure we have the required 4 outer tessellation levels
        // Use default of 1 for missing entries (same as spec)
        const int argCount = 4;
        if (tessLevels.size() < argCount) {
            tessLevels.reserve(argCount);
            for (int i = tessLevels.size(); i < argCount; ++i)
                tessLevels.append(1.0f);
        }
        d->tessellationFuncs->glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, tessLevels.data());
    }
#else
    Q_UNUSED(levels);
#endif
}

/*!
    Returns the default outer tessellation levels to be used by the tessellation
    primitive generator in the event that the tessellation control shader
    does not output them. For more details on OpenGL and Tessellation shaders see
    \l{OpenGL Tessellation Shaders}.

    Returns a QVector of floats describing the outer tessellation levels. The vector
    will always have four elements but not all of them make sense for every mode
    of tessellation.

    \note This returns the global OpenGL state value. It is not specific to
    this QOpenGLShaderProgram instance.

    \note This function is only supported with OpenGL >= 4.0 and will not
    return valid results with OpenGL ES 3.2.

    \sa setDefaultOuterTessellationLevels(), defaultInnerTessellationLevels()
*/
QVector<float> QOpenGLShaderProgram::defaultOuterTessellationLevels() const
{
#ifndef QT_OPENGL_ES_2
    QVector<float> tessLevels(4, 1.0f);
    Q_D(const QOpenGLShaderProgram);
    if (d->tessellationFuncs)
        d->tessellationFuncs->glGetFloatv(GL_PATCH_DEFAULT_OUTER_LEVEL, tessLevels.data());
    return tessLevels;
#else
    return QVector<float>();
#endif
}

/*!
    Sets the default outer tessellation levels to be used by the tessellation
    primitive generator in the event that the tessellation control shader
    does not output them to \a levels. For more details on OpenGL and Tessellation shaders see
    \l{OpenGL Tessellation Shaders}.

    The \a levels argument should be a QVector consisting of 2 floats. Not all
    of the values make sense for all tessellation modes. If you specify a vector with
    fewer than 2 elements, the remaining elements will be given a default value of 1.

    \note This modifies global OpenGL state and is not specific to this
    QOpenGLShaderProgram instance. You should call this in your render
    function when needed, as QOpenGLShaderProgram will not apply this for
    you. This is purely a convenience function.

    \note This function is only available with OpenGL >= 4.0 and is not supported
    with OpenGL ES 3.2.

    \sa defaultInnerTessellationLevels(), setDefaultOuterTessellationLevels()
*/
void QOpenGLShaderProgram::setDefaultInnerTessellationLevels(const QVector<float> &levels)
{
#ifndef QT_OPENGL_ES_2
    Q_D(QOpenGLShaderProgram);
    if (d->tessellationFuncs) {
        QVector<float> tessLevels = levels;

        // Ensure we have the required 2 inner tessellation levels
        // Use default of 1 for missing entries (same as spec)
        const int argCount = 2;
        if (tessLevels.size() < argCount) {
            tessLevels.reserve(argCount);
            for (int i = tessLevels.size(); i < argCount; ++i)
                tessLevels.append(1.0f);
        }
        d->tessellationFuncs->glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, tessLevels.data());
    }
#else
    Q_UNUSED(levels);
#endif
}

/*!
    Returns the default inner tessellation levels to be used by the tessellation
    primitive generator in the event that the tessellation control shader
    does not output them. For more details on OpenGL and Tessellation shaders see
    \l{OpenGL Tessellation Shaders}.

    Returns a QVector of floats describing the inner tessellation levels. The vector
    will always have two elements but not all of them make sense for every mode
    of tessellation.

    \note This returns the global OpenGL state value. It is not specific to
    this QOpenGLShaderProgram instance.

    \note This function is only supported with OpenGL >= 4.0 and will not
    return valid results with OpenGL ES 3.2.

    \sa setDefaultInnerTessellationLevels(), defaultOuterTessellationLevels()
*/
QVector<float> QOpenGLShaderProgram::defaultInnerTessellationLevels() const
{
#ifndef QT_OPENGL_ES_2
    QVector<float> tessLevels(2, 1.0f);
    Q_D(const QOpenGLShaderProgram);
    if (d->tessellationFuncs)
        d->tessellationFuncs->glGetFloatv(GL_PATCH_DEFAULT_INNER_LEVEL, tessLevels.data());
    return tessLevels;
#else
    return QVector<float>();
#endif
}


/*!
    Returns \c true if shader programs written in the OpenGL Shading
    Language (GLSL) are supported on this system; false otherwise.

    The \a context is used to resolve the GLSL extensions.
    If \a context is \nullptr, then QOpenGLContext::currentContext()
    is used.
*/
bool QOpenGLShaderProgram::hasOpenGLShaderPrograms(QOpenGLContext *context)
{
    if (!context)
        context = QOpenGLContext::currentContext();
    if (!context)
        return false;
    return QOpenGLFunctions(context).hasOpenGLFeature(QOpenGLFunctions::Shaders);
}

/*!
    \internal
*/
void QOpenGLShaderProgram::shaderDestroyed()
{
    Q_D(QOpenGLShaderProgram);
    QOpenGLShader *shader = qobject_cast<QOpenGLShader *>(sender());
    if (shader && !d->removingShaders)
        removeShader(shader);
}

/*!
    Returns \c true if shader programs of type \a type are supported on
    this system; false otherwise.

    The \a context is used to resolve the GLSL extensions.
    If \a context is \nullptr, then QOpenGLContext::currentContext()
    is used.
*/
bool QOpenGLShader::hasOpenGLShaders(ShaderType type, QOpenGLContext *context)
{
    if (!context)
        context = QOpenGLContext::currentContext();
    if (!context)
        return false;

    if ((type & ~(Geometry | Vertex | Fragment | TessellationControl | TessellationEvaluation | Compute)) || type == 0)
        return false;

    if (type & QOpenGLShader::Geometry)
        return supportsGeometry(context->format());
    else if (type & (QOpenGLShader::TessellationControl | QOpenGLShader::TessellationEvaluation))
        return supportsTessellation(context->format());
    else if (type & QOpenGLShader::Compute)
        return supportsCompute(context->format());

    // Unconditional support of vertex and fragment shaders implicitly assumes
    // a minimum OpenGL version of 2.0
    return true;
}

bool QOpenGLShaderProgramPrivate::isCacheDisabled() const
{
    static QOpenGLProgramBinarySupportCheckWrapper binSupportCheck;
    return !binSupportCheck.get(QOpenGLContext::currentContext())->isSupported();
}

bool QOpenGLShaderProgramPrivate::compileCacheable()
{
    Q_Q(QOpenGLShaderProgram);
    for (const QOpenGLProgramBinaryCache::ShaderDesc &shader : qAsConst(binaryProgram.shaders)) {
        QScopedPointer<QOpenGLShader> s(new QOpenGLShader(qt_shaderStageToType(shader.stage), q));
        if (!s->compileSourceCode(shader.source)) {
            log = s->log();
            return false;
        }
        anonShaders.append(s.take());
        if (!q->addShader(anonShaders.last()))
            return false;
    }
    return true;
}

bool QOpenGLShaderProgramPrivate::linkBinary()
{
    static QOpenGLProgramBinaryCache binCache;

    Q_Q(QOpenGLShaderProgram);

    const QByteArray cacheKey = binaryProgram.cacheKey();
    if (lcOpenGLProgramDiskCache().isEnabled(QtDebugMsg))
        qCDebug(lcOpenGLProgramDiskCache, "program with %d shaders, cache key %s",
                binaryProgram.shaders.count(), cacheKey.constData());

    bool needsCompile = true;
    if (binCache.load(cacheKey, q->programId())) {
        qCDebug(lcOpenGLProgramDiskCache, "Program binary received from cache");
        needsCompile = false;
    }

    bool needsSave = false;
    if (needsCompile) {
        qCDebug(lcOpenGLProgramDiskCache, "Program binary not in cache, compiling");
        if (compileCacheable())
            needsSave = true;
        else
            return false;
    }

    linkBinaryRecursion = true;
    bool ok = q->link();
    linkBinaryRecursion = false;
    if (ok && needsSave)
        binCache.save(cacheKey, q->programId());

    return ok;
}

QT_END_NAMESPACE
