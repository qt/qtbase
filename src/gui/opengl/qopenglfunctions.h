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

#ifndef QOPENGLFUNCTIONS_H
#define QOPENGLFUNCTIONS_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_OPENGL

#ifdef __GLEW_H__
#if defined(Q_CC_GNU)
#warning qopenglfunctions.h is not compatible with GLEW, GLEW defines will be undefined
#warning To use GLEW with Qt, do not include <qopengl.h> or <QOpenGLFunctions> after glew.h
#endif
#endif

#include <QtGui/qopengl.h>
#include <QtGui/qopenglcontext.h>

//#define Q_ENABLE_OPENGL_FUNCTIONS_DEBUG

#ifdef Q_ENABLE_OPENGL_FUNCTIONS_DEBUG
#include <stdio.h>
#define Q_OPENGL_FUNCTIONS_DEBUG \
    GLenum error = glGetError(); \
    if (error != GL_NO_ERROR) { \
        unsigned clamped = qMin(unsigned(error - GL_INVALID_ENUM), 4U); \
        const char *errors[] = { "GL_INVALID_ENUM", "GL_INVALID_VALUE", "GL_INVALID_OPERATION", "Unknown" }; \
        printf("GL error at %s:%d: %s\n", __FILE__, __LINE__, errors[clamped]); \
        int *value = 0; \
        *value = 0; \
    }
#else
#define Q_OPENGL_FUNCTIONS_DEBUG
#endif

QT_BEGIN_NAMESPACE

struct QOpenGLFunctionsPrivate;

// Undefine any macros from GLEW, qopenglextensions_p.h, etc that
// may interfere with the definition of QOpenGLFunctions.
#undef glActiveTexture
#undef glAttachShader
#undef glBindAttribLocation
#undef glBindBuffer
#undef glBindFramebuffer
#undef glBindRenderbuffer
#undef glBlendColor
#undef glBlendEquation
#undef glBlendEquationSeparate
#undef glBlendFuncSeparate
#undef glBufferData
#undef glBufferSubData
#undef glCheckFramebufferStatus
#undef glClearDepthf
#undef glCompileShader
#undef glCompressedTexImage2D
#undef glCompressedTexSubImage2D
#undef glCreateProgram
#undef glCreateShader
#undef glDeleteBuffers
#undef glDeleteFramebuffers
#undef glDeleteProgram
#undef glDeleteRenderbuffers
#undef glDeleteShader
#undef glDepthRangef
#undef glDetachShader
#undef glDisableVertexAttribArray
#undef glEnableVertexAttribArray
#undef glFramebufferRenderbuffer
#undef glFramebufferTexture2D
#undef glGenBuffers
#undef glGenerateMipmap
#undef glGenFramebuffers
#undef glGenRenderbuffers
#undef glGetActiveAttrib
#undef glGetActiveUniform
#undef glGetAttachedShaders
#undef glGetAttribLocation
#undef glGetBufferParameteriv
#undef glGetFramebufferAttachmentParameteriv
#undef glGetProgramiv
#undef glGetProgramInfoLog
#undef glGetRenderbufferParameteriv
#undef glGetShaderiv
#undef glGetShaderInfoLog
#undef glGetShaderPrecisionFormat
#undef glGetShaderSource
#undef glGetUniformfv
#undef glGetUniformiv
#undef glGetUniformLocation
#undef glGetVertexAttribfv
#undef glGetVertexAttribiv
#undef glGetVertexAttribPointerv
#undef glIsBuffer
#undef glIsFramebuffer
#undef glIsProgram
#undef glIsRenderbuffer
#undef glIsShader
#undef glLinkProgram
#undef glReleaseShaderCompiler
#undef glRenderbufferStorage
#undef glSampleCoverage
#undef glShaderBinary
#undef glShaderSource
#undef glStencilFuncSeparate
#undef glStencilMaskSeparate
#undef glStencilOpSeparate
#undef glUniform1f
#undef glUniform1fv
#undef glUniform1i
#undef glUniform1iv
#undef glUniform2f
#undef glUniform2fv
#undef glUniform2i
#undef glUniform2iv
#undef glUniform3f
#undef glUniform3fv
#undef glUniform3i
#undef glUniform3iv
#undef glUniform4f
#undef glUniform4fv
#undef glUniform4i
#undef glUniform4iv
#undef glUniformMatrix2fv
#undef glUniformMatrix3fv
#undef glUniformMatrix4fv
#undef glUseProgram
#undef glValidateProgram
#undef glVertexAttrib1f
#undef glVertexAttrib1fv
#undef glVertexAttrib2f
#undef glVertexAttrib2fv
#undef glVertexAttrib3f
#undef glVertexAttrib3fv
#undef glVertexAttrib4f
#undef glVertexAttrib4fv
#undef glVertexAttribPointer

class Q_GUI_EXPORT QOpenGLFunctions
{
public:
    QOpenGLFunctions();
    explicit QOpenGLFunctions(QOpenGLContext *context);
    ~QOpenGLFunctions() {}

    enum OpenGLFeature
    {
        Multitexture          = 0x0001,
        Shaders               = 0x0002,
        Buffers               = 0x0004,
        Framebuffers          = 0x0008,
        BlendColor            = 0x0010,
        BlendEquation         = 0x0020,
        BlendEquationSeparate = 0x0040,
        BlendFuncSeparate     = 0x0080,
        BlendSubtract         = 0x0100,
        CompressedTextures    = 0x0200,
        Multisample           = 0x0400,
        StencilSeparate       = 0x0800,
        NPOTTextures          = 0x1000,
        NPOTTextureRepeat     = 0x2000
    };
    Q_DECLARE_FLAGS(OpenGLFeatures, OpenGLFeature)

    QOpenGLFunctions::OpenGLFeatures openGLFeatures() const;
    bool hasOpenGLFeature(QOpenGLFunctions::OpenGLFeature feature) const;

    void initializeOpenGLFunctions();

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED void initializeGLFunctions() { initializeOpenGLFunctions(); }
#endif

    void glActiveTexture(GLenum texture);
    void glAttachShader(GLuint program, GLuint shader);
    void glBindAttribLocation(GLuint program, GLuint index, const char* name);
    void glBindBuffer(GLenum target, GLuint buffer);
    void glBindFramebuffer(GLenum target, GLuint framebuffer);
    void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
    void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void glBlendEquation(GLenum mode);
    void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
    void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    void glBufferData(GLenum target, qopengl_GLsizeiptr size, const void* data, GLenum usage);
    void glBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, const void* data);
    GLenum glCheckFramebufferStatus(GLenum target);
    void glClearDepthf(GLclampf depth);
    void glCompileShader(GLuint shader);
    void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data);
    void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data);
    GLuint glCreateProgram();
    GLuint glCreateShader(GLenum type);
    void glDeleteBuffers(GLsizei n, const GLuint* buffers);
    void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
    void glDeleteProgram(GLuint program);
    void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
    void glDeleteShader(GLuint shader);
    void glDepthRangef(GLclampf zNear, GLclampf zFar);
    void glDetachShader(GLuint program, GLuint shader);
    void glDisableVertexAttribArray(GLuint index);
    void glEnableVertexAttribArray(GLuint index);
    void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void glGenBuffers(GLsizei n, GLuint* buffers);
    void glGenerateMipmap(GLenum target);
    void glGenFramebuffers(GLsizei n, GLuint* framebuffers);
    void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
    void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
    void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
    void glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
    GLint glGetAttribLocation(GLuint program, const char* name);
    void glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params);
    void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params);
    void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
    void glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog);
    void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params);
    void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
    void glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog);
    void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
    void glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source);
    void glGetUniformfv(GLuint program, GLint location, GLfloat* params);
    void glGetUniformiv(GLuint program, GLint location, GLint* params);
    GLint glGetUniformLocation(GLuint program, const char* name);
    void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params);
    void glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params);
    void glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer);
    GLboolean glIsBuffer(GLuint buffer);
    GLboolean glIsFramebuffer(GLuint framebuffer);
    GLboolean glIsProgram(GLuint program);
    GLboolean glIsRenderbuffer(GLuint renderbuffer);
    GLboolean glIsShader(GLuint shader);
    void glLinkProgram(GLuint program);
    void glReleaseShaderCompiler();
    void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void glSampleCoverage(GLclampf value, GLboolean invert);
    void glShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length);
    void glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length);
    void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
    void glStencilMaskSeparate(GLenum face, GLuint mask);
    void glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
    void glUniform1f(GLint location, GLfloat x);
    void glUniform1fv(GLint location, GLsizei count, const GLfloat* v);
    void glUniform1i(GLint location, GLint x);
    void glUniform1iv(GLint location, GLsizei count, const GLint* v);
    void glUniform2f(GLint location, GLfloat x, GLfloat y);
    void glUniform2fv(GLint location, GLsizei count, const GLfloat* v);
    void glUniform2i(GLint location, GLint x, GLint y);
    void glUniform2iv(GLint location, GLsizei count, const GLint* v);
    void glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z);
    void glUniform3fv(GLint location, GLsizei count, const GLfloat* v);
    void glUniform3i(GLint location, GLint x, GLint y, GLint z);
    void glUniform3iv(GLint location, GLsizei count, const GLint* v);
    void glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void glUniform4fv(GLint location, GLsizei count, const GLfloat* v);
    void glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w);
    void glUniform4iv(GLint location, GLsizei count, const GLint* v);
    void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void glUseProgram(GLuint program);
    void glValidateProgram(GLuint program);
    void glVertexAttrib1f(GLuint indx, GLfloat x);
    void glVertexAttrib1fv(GLuint indx, const GLfloat* values);
    void glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y);
    void glVertexAttrib2fv(GLuint indx, const GLfloat* values);
    void glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
    void glVertexAttrib3fv(GLuint indx, const GLfloat* values);
    void glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void glVertexAttrib4fv(GLuint indx, const GLfloat* values);
    void glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr);

protected:
    QOpenGLFunctionsPrivate *d_ptr;
    static bool isInitialized(const QOpenGLFunctionsPrivate *d) { return d != 0; }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QOpenGLFunctions::OpenGLFeatures)

struct QOpenGLFunctionsPrivate
{
    QOpenGLFunctionsPrivate(QOpenGLContext *ctx);

#ifndef QT_OPENGL_ES_2
    void (QOPENGLF_APIENTRYP ActiveTexture)(GLenum texture);
    void (QOPENGLF_APIENTRYP AttachShader)(GLuint program, GLuint shader);
    void (QOPENGLF_APIENTRYP BindAttribLocation)(GLuint program, GLuint index, const char* name);
    void (QOPENGLF_APIENTRYP BindBuffer)(GLenum target, GLuint buffer);
    void (QOPENGLF_APIENTRYP BindFramebuffer)(GLenum target, GLuint framebuffer);
    void (QOPENGLF_APIENTRYP BindRenderbuffer)(GLenum target, GLuint renderbuffer);
    void (QOPENGLF_APIENTRYP BlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void (QOPENGLF_APIENTRYP BlendEquation)(GLenum mode);
    void (QOPENGLF_APIENTRYP BlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
    void (QOPENGLF_APIENTRYP BlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    void (QOPENGLF_APIENTRYP BufferData)(GLenum target, qopengl_GLsizeiptr size, const void* data, GLenum usage);
    void (QOPENGLF_APIENTRYP BufferSubData)(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, const void* data);
    GLenum (QOPENGLF_APIENTRYP CheckFramebufferStatus)(GLenum target);
    void (QOPENGLF_APIENTRYP CompileShader)(GLuint shader);
    void (QOPENGLF_APIENTRYP CompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data);
    void (QOPENGLF_APIENTRYP CompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data);
    GLuint (QOPENGLF_APIENTRYP CreateProgram)();
    GLuint (QOPENGLF_APIENTRYP CreateShader)(GLenum type);
    void (QOPENGLF_APIENTRYP DeleteBuffers)(GLsizei n, const GLuint* buffers);
    void (QOPENGLF_APIENTRYP DeleteFramebuffers)(GLsizei n, const GLuint* framebuffers);
    void (QOPENGLF_APIENTRYP DeleteProgram)(GLuint program);
    void (QOPENGLF_APIENTRYP DeleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers);
    void (QOPENGLF_APIENTRYP DeleteShader)(GLuint shader);
    void (QOPENGLF_APIENTRYP DetachShader)(GLuint program, GLuint shader);
    void (QOPENGLF_APIENTRYP DisableVertexAttribArray)(GLuint index);
    void (QOPENGLF_APIENTRYP EnableVertexAttribArray)(GLuint index);
    void (QOPENGLF_APIENTRYP FramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void (QOPENGLF_APIENTRYP FramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void (QOPENGLF_APIENTRYP GenBuffers)(GLsizei n, GLuint* buffers);
    void (QOPENGLF_APIENTRYP GenerateMipmap)(GLenum target);
    void (QOPENGLF_APIENTRYP GenFramebuffers)(GLsizei n, GLuint* framebuffers);
    void (QOPENGLF_APIENTRYP GenRenderbuffers)(GLsizei n, GLuint* renderbuffers);
    void (QOPENGLF_APIENTRYP GetActiveAttrib)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
    void (QOPENGLF_APIENTRYP GetActiveUniform)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
    void (QOPENGLF_APIENTRYP GetAttachedShaders)(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
    GLint (QOPENGLF_APIENTRYP GetAttribLocation)(GLuint program, const char* name);
    void (QOPENGLF_APIENTRYP GetBufferParameteriv)(GLenum target, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetProgramiv)(GLuint program, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetProgramInfoLog)(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog);
    void (QOPENGLF_APIENTRYP GetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetShaderiv)(GLuint shader, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetShaderInfoLog)(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog);
    void (QOPENGLF_APIENTRYP GetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
    void (QOPENGLF_APIENTRYP GetShaderSource)(GLuint shader, GLsizei bufsize, GLsizei* length, char* source);
    void (QOPENGLF_APIENTRYP GetUniformfv)(GLuint program, GLint location, GLfloat* params);
    void (QOPENGLF_APIENTRYP GetUniformiv)(GLuint program, GLint location, GLint* params);
    GLint (QOPENGLF_APIENTRYP GetUniformLocation)(GLuint program, const char* name);
    void (QOPENGLF_APIENTRYP GetVertexAttribfv)(GLuint index, GLenum pname, GLfloat* params);
    void (QOPENGLF_APIENTRYP GetVertexAttribiv)(GLuint index, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetVertexAttribPointerv)(GLuint index, GLenum pname, void** pointer);
    GLboolean (QOPENGLF_APIENTRYP IsBuffer)(GLuint buffer);
    GLboolean (QOPENGLF_APIENTRYP IsFramebuffer)(GLuint framebuffer);
    GLboolean (QOPENGLF_APIENTRYP IsProgram)(GLuint program);
    GLboolean (QOPENGLF_APIENTRYP IsRenderbuffer)(GLuint renderbuffer);
    GLboolean (QOPENGLF_APIENTRYP IsShader)(GLuint shader);
    void (QOPENGLF_APIENTRYP LinkProgram)(GLuint program);
    void (QOPENGLF_APIENTRYP ReleaseShaderCompiler)();
    void (QOPENGLF_APIENTRYP RenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP SampleCoverage)(GLclampf value, GLboolean invert);
    void (QOPENGLF_APIENTRYP ShaderBinary)(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length);
    void (QOPENGLF_APIENTRYP ShaderSource)(GLuint shader, GLsizei count, const char** string, const GLint* length);
    void (QOPENGLF_APIENTRYP StencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
    void (QOPENGLF_APIENTRYP StencilMaskSeparate)(GLenum face, GLuint mask);
    void (QOPENGLF_APIENTRYP StencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
    void (QOPENGLF_APIENTRYP Uniform1f)(GLint location, GLfloat x);
    void (QOPENGLF_APIENTRYP Uniform1fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QOPENGLF_APIENTRYP Uniform1i)(GLint location, GLint x);
    void (QOPENGLF_APIENTRYP Uniform1iv)(GLint location, GLsizei count, const GLint* v);
    void (QOPENGLF_APIENTRYP Uniform2f)(GLint location, GLfloat x, GLfloat y);
    void (QOPENGLF_APIENTRYP Uniform2fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QOPENGLF_APIENTRYP Uniform2i)(GLint location, GLint x, GLint y);
    void (QOPENGLF_APIENTRYP Uniform2iv)(GLint location, GLsizei count, const GLint* v);
    void (QOPENGLF_APIENTRYP Uniform3f)(GLint location, GLfloat x, GLfloat y, GLfloat z);
    void (QOPENGLF_APIENTRYP Uniform3fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QOPENGLF_APIENTRYP Uniform3i)(GLint location, GLint x, GLint y, GLint z);
    void (QOPENGLF_APIENTRYP Uniform3iv)(GLint location, GLsizei count, const GLint* v);
    void (QOPENGLF_APIENTRYP Uniform4f)(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (QOPENGLF_APIENTRYP Uniform4fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QOPENGLF_APIENTRYP Uniform4i)(GLint location, GLint x, GLint y, GLint z, GLint w);
    void (QOPENGLF_APIENTRYP Uniform4iv)(GLint location, GLsizei count, const GLint* v);
    void (QOPENGLF_APIENTRYP UniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (QOPENGLF_APIENTRYP UniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (QOPENGLF_APIENTRYP UniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (QOPENGLF_APIENTRYP UseProgram)(GLuint program);
    void (QOPENGLF_APIENTRYP ValidateProgram)(GLuint program);
    void (QOPENGLF_APIENTRYP VertexAttrib1f)(GLuint indx, GLfloat x);
    void (QOPENGLF_APIENTRYP VertexAttrib1fv)(GLuint indx, const GLfloat* values);
    void (QOPENGLF_APIENTRYP VertexAttrib2f)(GLuint indx, GLfloat x, GLfloat y);
    void (QOPENGLF_APIENTRYP VertexAttrib2fv)(GLuint indx, const GLfloat* values);
    void (QOPENGLF_APIENTRYP VertexAttrib3f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
    void (QOPENGLF_APIENTRYP VertexAttrib3fv)(GLuint indx, const GLfloat* values);
    void (QOPENGLF_APIENTRYP VertexAttrib4f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (QOPENGLF_APIENTRYP VertexAttrib4fv)(GLuint indx, const GLfloat* values);
    void (QOPENGLF_APIENTRYP VertexAttribPointer)(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr);
#endif
};

inline void QOpenGLFunctions::glActiveTexture(GLenum texture)
{
#if defined(QT_OPENGL_ES_2)
    ::glActiveTexture(texture);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ActiveTexture(texture);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glAttachShader(GLuint program, GLuint shader)
{
#if defined(QT_OPENGL_ES_2)
    ::glAttachShader(program, shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->AttachShader(program, shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindAttribLocation(GLuint program, GLuint index, const char* name)
{
#if defined(QT_OPENGL_ES_2)
    ::glBindAttribLocation(program, index, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindAttribLocation(program, index, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindBuffer(GLenum target, GLuint buffer)
{
#if defined(QT_OPENGL_ES_2)
    ::glBindBuffer(target, buffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindBuffer(target, buffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    if (framebuffer == 0)
        framebuffer = QOpenGLContext::currentContext()->defaultFramebufferObject();
#if defined(QT_OPENGL_ES_2)
    ::glBindFramebuffer(target, framebuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindFramebuffer(target, framebuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
#if defined(QT_OPENGL_ES_2)
    ::glBindRenderbuffer(target, renderbuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindRenderbuffer(target, renderbuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
#if defined(QT_OPENGL_ES_2)
    ::glBlendColor(red, green, blue, alpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendColor(red, green, blue, alpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendEquation(GLenum mode)
{
#if defined(QT_OPENGL_ES_2)
    ::glBlendEquation(mode);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendEquation(mode);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
#if defined(QT_OPENGL_ES_2)
    ::glBlendEquationSeparate(modeRGB, modeAlpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendEquationSeparate(modeRGB, modeAlpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
#if defined(QT_OPENGL_ES_2)
    ::glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBufferData(GLenum target, qopengl_GLsizeiptr size, const void* data, GLenum usage)
{
#if defined(QT_OPENGL_ES_2)
    ::glBufferData(target, size, data, usage);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BufferData(target, size, data, usage);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, const void* data)
{
#if defined(QT_OPENGL_ES_2)
    ::glBufferSubData(target, offset, size, data);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BufferSubData(target, offset, size, data);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLenum QOpenGLFunctions::glCheckFramebufferStatus(GLenum target)
{
#if defined(QT_OPENGL_ES_2)
    GLenum result = ::glCheckFramebufferStatus(target);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLenum result = d_ptr->CheckFramebufferStatus(target);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glClearDepthf(GLclampf depth)
{
#ifndef QT_OPENGL_ES
    ::glClearDepth(depth);
#else
    ::glClearDepthf(depth);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCompileShader(GLuint shader)
{
#if defined(QT_OPENGL_ES_2)
    ::glCompileShader(shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CompileShader(shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
#if defined(QT_OPENGL_ES_2)
    ::glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)
{
#if defined(QT_OPENGL_ES_2)
    ::glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLuint QOpenGLFunctions::glCreateProgram()
{
#if defined(QT_OPENGL_ES_2)
    GLuint result = ::glCreateProgram();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLuint result = d_ptr->CreateProgram();
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLuint QOpenGLFunctions::glCreateShader(GLenum type)
{
#if defined(QT_OPENGL_ES_2)
    GLuint result = ::glCreateShader(type);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLuint result = d_ptr->CreateShader(type);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glDeleteBuffers(n, buffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteBuffers(n, buffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glDeleteFramebuffers(n, framebuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteFramebuffers(n, framebuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteProgram(GLuint program)
{
#if defined(QT_OPENGL_ES_2)
    ::glDeleteProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glDeleteRenderbuffers(n, renderbuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteRenderbuffers(n, renderbuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteShader(GLuint shader)
{
#if defined(QT_OPENGL_ES_2)
    ::glDeleteShader(shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteShader(shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDepthRangef(GLclampf zNear, GLclampf zFar)
{
#ifndef QT_OPENGL_ES
    ::glDepthRange(zNear, zFar);
#else
    ::glDepthRangef(zNear, zFar);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDetachShader(GLuint program, GLuint shader)
{
#if defined(QT_OPENGL_ES_2)
    ::glDetachShader(program, shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DetachShader(program, shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDisableVertexAttribArray(GLuint index)
{
#if defined(QT_OPENGL_ES_2)
    ::glDisableVertexAttribArray(index);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DisableVertexAttribArray(index);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glEnableVertexAttribArray(GLuint index)
{
#if defined(QT_OPENGL_ES_2)
    ::glEnableVertexAttribArray(index);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->EnableVertexAttribArray(index);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
#if defined(QT_OPENGL_ES_2)
    ::glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->FramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
#if defined(QT_OPENGL_ES_2)
    ::glFramebufferTexture2D(target, attachment, textarget, texture, level);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->FramebufferTexture2D(target, attachment, textarget, texture, level);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenBuffers(GLsizei n, GLuint* buffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glGenBuffers(n, buffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenBuffers(n, buffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenerateMipmap(GLenum target)
{
#if defined(QT_OPENGL_ES_2)
    ::glGenerateMipmap(target);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenerateMipmap(target);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glGenFramebuffers(n, framebuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenFramebuffers(n, framebuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glGenRenderbuffers(n, renderbuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenRenderbuffers(n, renderbuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetActiveAttrib(program, index, bufsize, length, size, type, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetActiveAttrib(program, index, bufsize, length, size, type, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetActiveUniform(program, index, bufsize, length, size, type, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetActiveUniform(program, index, bufsize, length, size, type, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetAttachedShaders(program, maxcount, count, shaders);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetAttachedShaders(program, maxcount, count, shaders);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLint QOpenGLFunctions::glGetAttribLocation(GLuint program, const char* name)
{
#if defined(QT_OPENGL_ES_2)
    GLint result = ::glGetAttribLocation(program, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLint result = d_ptr->GetAttribLocation(program, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetBufferParameteriv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetBufferParameteriv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetFramebufferAttachmentParameteriv(target, attachment, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetProgramiv(program, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetProgramiv(program, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetProgramInfoLog(program, bufsize, length, infolog);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetProgramInfoLog(program, bufsize, length, infolog);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetRenderbufferParameteriv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetRenderbufferParameteriv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetShaderiv(shader, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderiv(shader, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetShaderInfoLog(shader, bufsize, length, infolog);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderInfoLog(shader, bufsize, length, infolog);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetShaderSource(shader, bufsize, length, source);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderSource(shader, bufsize, length, source);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetUniformfv(program, location, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetUniformfv(program, location, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetUniformiv(GLuint program, GLint location, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetUniformiv(program, location, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetUniformiv(program, location, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLint QOpenGLFunctions::glGetUniformLocation(GLuint program, const char* name)
{
#if defined(QT_OPENGL_ES_2)
    GLint result = ::glGetUniformLocation(program, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLint result = d_ptr->GetUniformLocation(program, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetVertexAttribfv(index, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetVertexAttribfv(index, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetVertexAttribiv(index, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetVertexAttribiv(index, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetVertexAttribPointerv(index, pname, pointer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetVertexAttribPointerv(index, pname, pointer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLboolean QOpenGLFunctions::glIsBuffer(GLuint buffer)
{
#if defined(QT_OPENGL_ES_2)
    GLboolean result = ::glIsBuffer(buffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsBuffer(buffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsFramebuffer(GLuint framebuffer)
{
#if defined(QT_OPENGL_ES_2)
    GLboolean result = ::glIsFramebuffer(framebuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsFramebuffer(framebuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsProgram(GLuint program)
{
#if defined(QT_OPENGL_ES_2)
    GLboolean result = ::glIsProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsRenderbuffer(GLuint renderbuffer)
{
#if defined(QT_OPENGL_ES_2)
    GLboolean result = ::glIsRenderbuffer(renderbuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsRenderbuffer(renderbuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsShader(GLuint shader)
{
#if defined(QT_OPENGL_ES_2)
    GLboolean result = ::glIsShader(shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsShader(shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glLinkProgram(GLuint program)
{
#if defined(QT_OPENGL_ES_2)
    ::glLinkProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->LinkProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glReleaseShaderCompiler()
{
#if defined(QT_OPENGL_ES_2)
    ::glReleaseShaderCompiler();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ReleaseShaderCompiler();
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
#if defined(QT_OPENGL_ES_2)
    ::glRenderbufferStorage(target, internalformat, width, height);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->RenderbufferStorage(target, internalformat, width, height);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glSampleCoverage(GLclampf value, GLboolean invert)
{
#if defined(QT_OPENGL_ES_2)
    ::glSampleCoverage(value, invert);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->SampleCoverage(value, invert);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length)
{
#if defined(QT_OPENGL_ES_2)
    ::glShaderBinary(n, shaders, binaryformat, binary, length);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ShaderBinary(n, shaders, binaryformat, binary, length);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length)
{
#if defined(QT_OPENGL_ES_2)
    ::glShaderSource(shader, count, string, length);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ShaderSource(shader, count, string, length);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
#if defined(QT_OPENGL_ES_2)
    ::glStencilFuncSeparate(face, func, ref, mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilFuncSeparate(face, func, ref, mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilMaskSeparate(GLenum face, GLuint mask)
{
#if defined(QT_OPENGL_ES_2)
    ::glStencilMaskSeparate(face, mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilMaskSeparate(face, mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
#if defined(QT_OPENGL_ES_2)
    ::glStencilOpSeparate(face, fail, zfail, zpass);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilOpSeparate(face, fail, zfail, zpass);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1f(GLint location, GLfloat x)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform1f(location, x);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1f(location, x);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform1fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1i(GLint location, GLint x)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform1i(location, x);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1i(location, x);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1iv(GLint location, GLsizei count, const GLint* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform1iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2f(GLint location, GLfloat x, GLfloat y)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform2f(location, x, y);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2f(location, x, y);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform2fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2i(GLint location, GLint x, GLint y)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform2i(location, x, y);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2i(location, x, y);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2iv(GLint location, GLsizei count, const GLint* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform2iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform3f(location, x, y, z);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3f(location, x, y, z);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform3fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3i(GLint location, GLint x, GLint y, GLint z)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform3i(location, x, y, z);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3i(location, x, y, z);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3iv(GLint location, GLsizei count, const GLint* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform3iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform4f(location, x, y, z, w);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4f(location, x, y, z, w);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform4fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform4i(location, x, y, z, w);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4i(location, x, y, z, w);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4iv(GLint location, GLsizei count, const GLint* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform4iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniformMatrix2fv(location, count, transpose, value);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UniformMatrix2fv(location, count, transpose, value);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniformMatrix3fv(location, count, transpose, value);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UniformMatrix3fv(location, count, transpose, value);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniformMatrix4fv(location, count, transpose, value);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UniformMatrix4fv(location, count, transpose, value);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUseProgram(GLuint program)
{
#if defined(QT_OPENGL_ES_2)
    ::glUseProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UseProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glValidateProgram(GLuint program)
{
#if defined(QT_OPENGL_ES_2)
    ::glValidateProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ValidateProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib1f(GLuint indx, GLfloat x)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib1f(indx, x);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib1f(indx, x);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib1fv(GLuint indx, const GLfloat* values)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib1fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib1fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib2f(indx, x, y);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib2f(indx, x, y);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib2fv(GLuint indx, const GLfloat* values)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib2fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib2fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib3f(indx, x, y, z);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib3f(indx, x, y, z);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib3fv(GLuint indx, const GLfloat* values)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib3fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib3fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib4f(indx, x, y, z, w);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib4f(indx, x, y, z, w);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib4fv(GLuint indx, const GLfloat* values)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib4fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib4fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttribPointer(indx, size, type, normalized, stride, ptr);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttribPointer(indx, size, type, normalized, stride, ptr);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif
