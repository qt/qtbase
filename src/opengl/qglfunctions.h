/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#ifndef QGLFUNCTIONS_H
#define QGLFUNCTIONS_H

#ifdef __GLEW_H__
#if defined(Q_CC_GNU)
#warning qglfunctions.h is not compatible with GLEW, GLEW defines will be undefined
#warning To use GLEW with Qt, do not include <QtOpenGL> or <QGLFunctions> after glew.h
#endif
#endif

#include <QtOpenGL/qgl.h>
#include <QtGui/qopenglcontext.h>

QT_BEGIN_NAMESPACE


// Types that aren't defined in all system's gl.h files.
typedef ptrdiff_t qgl_GLintptr;
typedef ptrdiff_t qgl_GLsizeiptr;

#if defined(APIENTRY) && !defined(QGLF_APIENTRY)
#   define QGLF_APIENTRY APIENTRY
#elif defined(GL_APIENTRY) && !defined(QGLF_APIENTRY)
#   define QGLF_APIENTRY GL_APIENTRY
#endif

# ifndef QGLF_APIENTRYP
#   ifdef QGLF_APIENTRY
#     define QGLF_APIENTRYP QGLF_APIENTRY *
#   else
#     define QGLF_APIENTRY
#     define QGLF_APIENTRYP *
#   endif
# endif

struct QGLFunctionsPrivate;

// Undefine any macros from GLEW, qglextensions_p.h, etc that
// may interfere with the definition of QGLFunctions.
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

class Q_OPENGL_EXPORT QGLFunctions
{
public:
    QGLFunctions();
    explicit QGLFunctions(const QGLContext *context);
    ~QGLFunctions() {}

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
        NPOTTextures          = 0x1000
    };
    Q_DECLARE_FLAGS(OpenGLFeatures, OpenGLFeature)

    QGLFunctions::OpenGLFeatures openGLFeatures() const;
    bool hasOpenGLFeature(QGLFunctions::OpenGLFeature feature) const;

    void initializeGLFunctions(const QGLContext *context = 0);

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
    void glBufferData(GLenum target, qgl_GLsizeiptr size, const void* data, GLenum usage);
    void glBufferSubData(GLenum target, qgl_GLintptr offset, qgl_GLsizeiptr size, const void* data);
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
    int glGetAttribLocation(GLuint program, const char* name);
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
    int glGetUniformLocation(GLuint program, const char* name);
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

private:
    QGLFunctionsPrivate *d_ptr;
    static bool isInitialized(const QGLFunctionsPrivate *d) { return d != 0; }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGLFunctions::OpenGLFeatures)

struct QGLFunctionsPrivate
{
    QGLFunctionsPrivate(const QGLContext *context = 0);

#ifndef QT_OPENGL_ES_2
    void (QGLF_APIENTRYP activeTexture)(GLenum texture);
    void (QGLF_APIENTRYP attachShader)(GLuint program, GLuint shader);
    void (QGLF_APIENTRYP bindAttribLocation)(GLuint program, GLuint index, const char* name);
    void (QGLF_APIENTRYP bindBuffer)(GLenum target, GLuint buffer);
    void (QGLF_APIENTRYP bindFramebuffer)(GLenum target, GLuint framebuffer);
    void (QGLF_APIENTRYP bindRenderbuffer)(GLenum target, GLuint renderbuffer);
    void (QGLF_APIENTRYP blendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void (QGLF_APIENTRYP blendEquation)(GLenum mode);
    void (QGLF_APIENTRYP blendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
    void (QGLF_APIENTRYP blendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    void (QGLF_APIENTRYP bufferData)(GLenum target, qgl_GLsizeiptr size, const void* data, GLenum usage);
    void (QGLF_APIENTRYP bufferSubData)(GLenum target, qgl_GLintptr offset, qgl_GLsizeiptr size, const void* data);
    GLenum (QGLF_APIENTRYP checkFramebufferStatus)(GLenum target);
    void (QGLF_APIENTRYP compileShader)(GLuint shader);
    void (QGLF_APIENTRYP compressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data);
    void (QGLF_APIENTRYP compressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data);
    GLuint (QGLF_APIENTRYP createProgram)();
    GLuint (QGLF_APIENTRYP createShader)(GLenum type);
    void (QGLF_APIENTRYP deleteBuffers)(GLsizei n, const GLuint* buffers);
    void (QGLF_APIENTRYP deleteFramebuffers)(GLsizei n, const GLuint* framebuffers);
    void (QGLF_APIENTRYP deleteProgram)(GLuint program);
    void (QGLF_APIENTRYP deleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers);
    void (QGLF_APIENTRYP deleteShader)(GLuint shader);
    void (QGLF_APIENTRYP detachShader)(GLuint program, GLuint shader);
    void (QGLF_APIENTRYP disableVertexAttribArray)(GLuint index);
    void (QGLF_APIENTRYP enableVertexAttribArray)(GLuint index);
    void (QGLF_APIENTRYP framebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void (QGLF_APIENTRYP framebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void (QGLF_APIENTRYP genBuffers)(GLsizei n, GLuint* buffers);
    void (QGLF_APIENTRYP generateMipmap)(GLenum target);
    void (QGLF_APIENTRYP genFramebuffers)(GLsizei n, GLuint* framebuffers);
    void (QGLF_APIENTRYP genRenderbuffers)(GLsizei n, GLuint* renderbuffers);
    void (QGLF_APIENTRYP getActiveAttrib)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
    void (QGLF_APIENTRYP getActiveUniform)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
    void (QGLF_APIENTRYP getAttachedShaders)(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
    int (QGLF_APIENTRYP getAttribLocation)(GLuint program, const char* name);
    void (QGLF_APIENTRYP getBufferParameteriv)(GLenum target, GLenum pname, GLint* params);
    void (QGLF_APIENTRYP getFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
    void (QGLF_APIENTRYP getProgramiv)(GLuint program, GLenum pname, GLint* params);
    void (QGLF_APIENTRYP getProgramInfoLog)(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog);
    void (QGLF_APIENTRYP getRenderbufferParameteriv)(GLenum target, GLenum pname, GLint* params);
    void (QGLF_APIENTRYP getShaderiv)(GLuint shader, GLenum pname, GLint* params);
    void (QGLF_APIENTRYP getShaderInfoLog)(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog);
    void (QGLF_APIENTRYP getShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
    void (QGLF_APIENTRYP getShaderSource)(GLuint shader, GLsizei bufsize, GLsizei* length, char* source);
    void (QGLF_APIENTRYP getUniformfv)(GLuint program, GLint location, GLfloat* params);
    void (QGLF_APIENTRYP getUniformiv)(GLuint program, GLint location, GLint* params);
    int (QGLF_APIENTRYP getUniformLocation)(GLuint program, const char* name);
    void (QGLF_APIENTRYP getVertexAttribfv)(GLuint index, GLenum pname, GLfloat* params);
    void (QGLF_APIENTRYP getVertexAttribiv)(GLuint index, GLenum pname, GLint* params);
    void (QGLF_APIENTRYP getVertexAttribPointerv)(GLuint index, GLenum pname, void** pointer);
    GLboolean (QGLF_APIENTRYP isBuffer)(GLuint buffer);
    GLboolean (QGLF_APIENTRYP isFramebuffer)(GLuint framebuffer);
    GLboolean (QGLF_APIENTRYP isProgram)(GLuint program);
    GLboolean (QGLF_APIENTRYP isRenderbuffer)(GLuint renderbuffer);
    GLboolean (QGLF_APIENTRYP isShader)(GLuint shader);
    void (QGLF_APIENTRYP linkProgram)(GLuint program);
    void (QGLF_APIENTRYP releaseShaderCompiler)();
    void (QGLF_APIENTRYP renderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void (QGLF_APIENTRYP sampleCoverage)(GLclampf value, GLboolean invert);
    void (QGLF_APIENTRYP shaderBinary)(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length);
    void (QGLF_APIENTRYP shaderSource)(GLuint shader, GLsizei count, const char** string, const GLint* length);
    void (QGLF_APIENTRYP stencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
    void (QGLF_APIENTRYP stencilMaskSeparate)(GLenum face, GLuint mask);
    void (QGLF_APIENTRYP stencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
    void (QGLF_APIENTRYP uniform1f)(GLint location, GLfloat x);
    void (QGLF_APIENTRYP uniform1fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QGLF_APIENTRYP uniform1i)(GLint location, GLint x);
    void (QGLF_APIENTRYP uniform1iv)(GLint location, GLsizei count, const GLint* v);
    void (QGLF_APIENTRYP uniform2f)(GLint location, GLfloat x, GLfloat y);
    void (QGLF_APIENTRYP uniform2fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QGLF_APIENTRYP uniform2i)(GLint location, GLint x, GLint y);
    void (QGLF_APIENTRYP uniform2iv)(GLint location, GLsizei count, const GLint* v);
    void (QGLF_APIENTRYP uniform3f)(GLint location, GLfloat x, GLfloat y, GLfloat z);
    void (QGLF_APIENTRYP uniform3fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QGLF_APIENTRYP uniform3i)(GLint location, GLint x, GLint y, GLint z);
    void (QGLF_APIENTRYP uniform3iv)(GLint location, GLsizei count, const GLint* v);
    void (QGLF_APIENTRYP uniform4f)(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (QGLF_APIENTRYP uniform4fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QGLF_APIENTRYP uniform4i)(GLint location, GLint x, GLint y, GLint z, GLint w);
    void (QGLF_APIENTRYP uniform4iv)(GLint location, GLsizei count, const GLint* v);
    void (QGLF_APIENTRYP uniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (QGLF_APIENTRYP uniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (QGLF_APIENTRYP uniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (QGLF_APIENTRYP useProgram)(GLuint program);
    void (QGLF_APIENTRYP validateProgram)(GLuint program);
    void (QGLF_APIENTRYP vertexAttrib1f)(GLuint indx, GLfloat x);
    void (QGLF_APIENTRYP vertexAttrib1fv)(GLuint indx, const GLfloat* values);
    void (QGLF_APIENTRYP vertexAttrib2f)(GLuint indx, GLfloat x, GLfloat y);
    void (QGLF_APIENTRYP vertexAttrib2fv)(GLuint indx, const GLfloat* values);
    void (QGLF_APIENTRYP vertexAttrib3f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
    void (QGLF_APIENTRYP vertexAttrib3fv)(GLuint indx, const GLfloat* values);
    void (QGLF_APIENTRYP vertexAttrib4f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (QGLF_APIENTRYP vertexAttrib4fv)(GLuint indx, const GLfloat* values);
    void (QGLF_APIENTRYP vertexAttribPointer)(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr);
#endif
};

inline void QGLFunctions::glActiveTexture(GLenum texture)
{
#if defined(QT_OPENGL_ES_2)
    ::glActiveTexture(texture);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->activeTexture(texture);
#endif
}

inline void QGLFunctions::glAttachShader(GLuint program, GLuint shader)
{
#if defined(QT_OPENGL_ES_2)
    ::glAttachShader(program, shader);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->attachShader(program, shader);
#endif
}

inline void QGLFunctions::glBindAttribLocation(GLuint program, GLuint index, const char* name)
{
#if defined(QT_OPENGL_ES_2)
    ::glBindAttribLocation(program, index, name);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->bindAttribLocation(program, index, name);
#endif
}

inline void QGLFunctions::glBindBuffer(GLenum target, GLuint buffer)
{
#if defined(QT_OPENGL_ES_2)
    ::glBindBuffer(target, buffer);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->bindBuffer(target, buffer);
#endif
}

inline void QGLFunctions::glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    if (framebuffer == 0)
        framebuffer = QOpenGLContext::currentContext()->defaultFramebufferObject();
#if defined(QT_OPENGL_ES_2)
    ::glBindFramebuffer(target, framebuffer);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->bindFramebuffer(target, framebuffer);
#endif
}

inline void QGLFunctions::glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
#if defined(QT_OPENGL_ES_2)
    ::glBindRenderbuffer(target, renderbuffer);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->bindRenderbuffer(target, renderbuffer);
#endif
}

inline void QGLFunctions::glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
#if defined(QT_OPENGL_ES_2)
    ::glBlendColor(red, green, blue, alpha);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->blendColor(red, green, blue, alpha);
#endif
}

inline void QGLFunctions::glBlendEquation(GLenum mode)
{
#if defined(QT_OPENGL_ES_2)
    ::glBlendEquation(mode);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->blendEquation(mode);
#endif
}

inline void QGLFunctions::glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
#if defined(QT_OPENGL_ES_2)
    ::glBlendEquationSeparate(modeRGB, modeAlpha);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->blendEquationSeparate(modeRGB, modeAlpha);
#endif
}

inline void QGLFunctions::glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
#if defined(QT_OPENGL_ES_2)
    ::glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
#endif
}

inline void QGLFunctions::glBufferData(GLenum target, qgl_GLsizeiptr size, const void* data, GLenum usage)
{
#if defined(QT_OPENGL_ES_2)
    ::glBufferData(target, size, data, usage);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->bufferData(target, size, data, usage);
#endif
}

inline void QGLFunctions::glBufferSubData(GLenum target, qgl_GLintptr offset, qgl_GLsizeiptr size, const void* data)
{
#if defined(QT_OPENGL_ES_2)
    ::glBufferSubData(target, offset, size, data);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->bufferSubData(target, offset, size, data);
#endif
}

inline GLenum QGLFunctions::glCheckFramebufferStatus(GLenum target)
{
#if defined(QT_OPENGL_ES_2)
    return ::glCheckFramebufferStatus(target);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    return d_ptr->checkFramebufferStatus(target);
#endif
}

inline void QGLFunctions::glClearDepthf(GLclampf depth)
{
#ifndef QT_OPENGL_ES
    ::glClearDepth(depth);
#else
    ::glClearDepthf(depth);
#endif
}

inline void QGLFunctions::glCompileShader(GLuint shader)
{
#if defined(QT_OPENGL_ES_2)
    ::glCompileShader(shader);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->compileShader(shader);
#endif
}

inline void QGLFunctions::glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
#if defined(QT_OPENGL_ES_2)
    ::glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->compressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
#endif
}

inline void QGLFunctions::glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)
{
#if defined(QT_OPENGL_ES_2)
    ::glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->compressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
#endif
}

inline GLuint QGLFunctions::glCreateProgram()
{
#if defined(QT_OPENGL_ES_2)
    return ::glCreateProgram();
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    return d_ptr->createProgram();
#endif
}

inline GLuint QGLFunctions::glCreateShader(GLenum type)
{
#if defined(QT_OPENGL_ES_2)
    return ::glCreateShader(type);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    return d_ptr->createShader(type);
#endif
}

inline void QGLFunctions::glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glDeleteBuffers(n, buffers);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->deleteBuffers(n, buffers);
#endif
}

inline void QGLFunctions::glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glDeleteFramebuffers(n, framebuffers);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->deleteFramebuffers(n, framebuffers);
#endif
}

inline void QGLFunctions::glDeleteProgram(GLuint program)
{
#if defined(QT_OPENGL_ES_2)
    ::glDeleteProgram(program);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->deleteProgram(program);
#endif
}

inline void QGLFunctions::glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glDeleteRenderbuffers(n, renderbuffers);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->deleteRenderbuffers(n, renderbuffers);
#endif
}

inline void QGLFunctions::glDeleteShader(GLuint shader)
{
#if defined(QT_OPENGL_ES_2)
    ::glDeleteShader(shader);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->deleteShader(shader);
#endif
}

inline void QGLFunctions::glDepthRangef(GLclampf zNear, GLclampf zFar)
{
#ifndef QT_OPENGL_ES
    ::glDepthRange(zNear, zFar);
#else
    ::glDepthRangef(zNear, zFar);
#endif
}

inline void QGLFunctions::glDetachShader(GLuint program, GLuint shader)
{
#if defined(QT_OPENGL_ES_2)
    ::glDetachShader(program, shader);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->detachShader(program, shader);
#endif
}

inline void QGLFunctions::glDisableVertexAttribArray(GLuint index)
{
#if defined(QT_OPENGL_ES_2)
    ::glDisableVertexAttribArray(index);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->disableVertexAttribArray(index);
#endif
}

inline void QGLFunctions::glEnableVertexAttribArray(GLuint index)
{
#if defined(QT_OPENGL_ES_2)
    ::glEnableVertexAttribArray(index);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->enableVertexAttribArray(index);
#endif
}

inline void QGLFunctions::glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
#if defined(QT_OPENGL_ES_2)
    ::glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->framebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
#endif
}

inline void QGLFunctions::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
#if defined(QT_OPENGL_ES_2)
    ::glFramebufferTexture2D(target, attachment, textarget, texture, level);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->framebufferTexture2D(target, attachment, textarget, texture, level);
#endif
}

inline void QGLFunctions::glGenBuffers(GLsizei n, GLuint* buffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glGenBuffers(n, buffers);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->genBuffers(n, buffers);
#endif
}

inline void QGLFunctions::glGenerateMipmap(GLenum target)
{
#if defined(QT_OPENGL_ES_2)
    ::glGenerateMipmap(target);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->generateMipmap(target);
#endif
}

inline void QGLFunctions::glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glGenFramebuffers(n, framebuffers);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->genFramebuffers(n, framebuffers);
#endif
}

inline void QGLFunctions::glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
#if defined(QT_OPENGL_ES_2)
    ::glGenRenderbuffers(n, renderbuffers);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->genRenderbuffers(n, renderbuffers);
#endif
}

inline void QGLFunctions::glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetActiveAttrib(program, index, bufsize, length, size, type, name);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getActiveAttrib(program, index, bufsize, length, size, type, name);
#endif
}

inline void QGLFunctions::glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetActiveUniform(program, index, bufsize, length, size, type, name);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getActiveUniform(program, index, bufsize, length, size, type, name);
#endif
}

inline void QGLFunctions::glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetAttachedShaders(program, maxcount, count, shaders);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getAttachedShaders(program, maxcount, count, shaders);
#endif
}

inline int QGLFunctions::glGetAttribLocation(GLuint program, const char* name)
{
#if defined(QT_OPENGL_ES_2)
    return ::glGetAttribLocation(program, name);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    return d_ptr->getAttribLocation(program, name);
#endif
}

inline void QGLFunctions::glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetBufferParameteriv(target, pname, params);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getBufferParameteriv(target, pname, params);
#endif
}

inline void QGLFunctions::glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getFramebufferAttachmentParameteriv(target, attachment, pname, params);
#endif
}

inline void QGLFunctions::glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetProgramiv(program, pname, params);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getProgramiv(program, pname, params);
#endif
}

inline void QGLFunctions::glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetProgramInfoLog(program, bufsize, length, infolog);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getProgramInfoLog(program, bufsize, length, infolog);
#endif
}

inline void QGLFunctions::glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetRenderbufferParameteriv(target, pname, params);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getRenderbufferParameteriv(target, pname, params);
#endif
}

inline void QGLFunctions::glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetShaderiv(shader, pname, params);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getShaderiv(shader, pname, params);
#endif
}

inline void QGLFunctions::glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetShaderInfoLog(shader, bufsize, length, infolog);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getShaderInfoLog(shader, bufsize, length, infolog);
#endif
}

inline void QGLFunctions::glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getShaderPrecisionFormat(shadertype, precisiontype, range, precision);
#endif
}

inline void QGLFunctions::glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetShaderSource(shader, bufsize, length, source);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getShaderSource(shader, bufsize, length, source);
#endif
}

inline void QGLFunctions::glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetUniformfv(program, location, params);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getUniformfv(program, location, params);
#endif
}

inline void QGLFunctions::glGetUniformiv(GLuint program, GLint location, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetUniformiv(program, location, params);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getUniformiv(program, location, params);
#endif
}

inline int QGLFunctions::glGetUniformLocation(GLuint program, const char* name)
{
#if defined(QT_OPENGL_ES_2)
    return ::glGetUniformLocation(program, name);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    return d_ptr->getUniformLocation(program, name);
#endif
}

inline void QGLFunctions::glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetVertexAttribfv(index, pname, params);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getVertexAttribfv(index, pname, params);
#endif
}

inline void QGLFunctions::glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetVertexAttribiv(index, pname, params);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getVertexAttribiv(index, pname, params);
#endif
}

inline void QGLFunctions::glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer)
{
#if defined(QT_OPENGL_ES_2)
    ::glGetVertexAttribPointerv(index, pname, pointer);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->getVertexAttribPointerv(index, pname, pointer);
#endif
}

inline GLboolean QGLFunctions::glIsBuffer(GLuint buffer)
{
#if defined(QT_OPENGL_ES_2)
    return ::glIsBuffer(buffer);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    return d_ptr->isBuffer(buffer);
#endif
}

inline GLboolean QGLFunctions::glIsFramebuffer(GLuint framebuffer)
{
#if defined(QT_OPENGL_ES_2)
    return ::glIsFramebuffer(framebuffer);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    return d_ptr->isFramebuffer(framebuffer);
#endif
}

inline GLboolean QGLFunctions::glIsProgram(GLuint program)
{
#if defined(QT_OPENGL_ES_2)
    return ::glIsProgram(program);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    return d_ptr->isProgram(program);
#endif
}

inline GLboolean QGLFunctions::glIsRenderbuffer(GLuint renderbuffer)
{
#if defined(QT_OPENGL_ES_2)
    return ::glIsRenderbuffer(renderbuffer);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    return d_ptr->isRenderbuffer(renderbuffer);
#endif
}

inline GLboolean QGLFunctions::glIsShader(GLuint shader)
{
#if defined(QT_OPENGL_ES_2)
    return ::glIsShader(shader);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    return d_ptr->isShader(shader);
#endif
}

inline void QGLFunctions::glLinkProgram(GLuint program)
{
#if defined(QT_OPENGL_ES_2)
    ::glLinkProgram(program);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->linkProgram(program);
#endif
}

inline void QGLFunctions::glReleaseShaderCompiler()
{
#if defined(QT_OPENGL_ES_2)
    ::glReleaseShaderCompiler();
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->releaseShaderCompiler();
#endif
}

inline void QGLFunctions::glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
#if defined(QT_OPENGL_ES_2)
    ::glRenderbufferStorage(target, internalformat, width, height);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->renderbufferStorage(target, internalformat, width, height);
#endif
}

inline void QGLFunctions::glSampleCoverage(GLclampf value, GLboolean invert)
{
#if defined(QT_OPENGL_ES_2)
    ::glSampleCoverage(value, invert);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->sampleCoverage(value, invert);
#endif
}

inline void QGLFunctions::glShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length)
{
#if defined(QT_OPENGL_ES_2)
    ::glShaderBinary(n, shaders, binaryformat, binary, length);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->shaderBinary(n, shaders, binaryformat, binary, length);
#endif
}

inline void QGLFunctions::glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length)
{
#if defined(QT_OPENGL_ES_2)
    ::glShaderSource(shader, count, string, length);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->shaderSource(shader, count, string, length);
#endif
}

inline void QGLFunctions::glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
#if defined(QT_OPENGL_ES_2)
    ::glStencilFuncSeparate(face, func, ref, mask);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->stencilFuncSeparate(face, func, ref, mask);
#endif
}

inline void QGLFunctions::glStencilMaskSeparate(GLenum face, GLuint mask)
{
#if defined(QT_OPENGL_ES_2)
    ::glStencilMaskSeparate(face, mask);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->stencilMaskSeparate(face, mask);
#endif
}

inline void QGLFunctions::glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
#if defined(QT_OPENGL_ES_2)
    ::glStencilOpSeparate(face, fail, zfail, zpass);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->stencilOpSeparate(face, fail, zfail, zpass);
#endif
}

inline void QGLFunctions::glUniform1f(GLint location, GLfloat x)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform1f(location, x);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform1f(location, x);
#endif
}

inline void QGLFunctions::glUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform1fv(location, count, v);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform1fv(location, count, v);
#endif
}

inline void QGLFunctions::glUniform1i(GLint location, GLint x)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform1i(location, x);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform1i(location, x);
#endif
}

inline void QGLFunctions::glUniform1iv(GLint location, GLsizei count, const GLint* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform1iv(location, count, v);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform1iv(location, count, v);
#endif
}

inline void QGLFunctions::glUniform2f(GLint location, GLfloat x, GLfloat y)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform2f(location, x, y);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform2f(location, x, y);
#endif
}

inline void QGLFunctions::glUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform2fv(location, count, v);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform2fv(location, count, v);
#endif
}

inline void QGLFunctions::glUniform2i(GLint location, GLint x, GLint y)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform2i(location, x, y);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform2i(location, x, y);
#endif
}

inline void QGLFunctions::glUniform2iv(GLint location, GLsizei count, const GLint* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform2iv(location, count, v);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform2iv(location, count, v);
#endif
}

inline void QGLFunctions::glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform3f(location, x, y, z);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform3f(location, x, y, z);
#endif
}

inline void QGLFunctions::glUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform3fv(location, count, v);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform3fv(location, count, v);
#endif
}

inline void QGLFunctions::glUniform3i(GLint location, GLint x, GLint y, GLint z)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform3i(location, x, y, z);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform3i(location, x, y, z);
#endif
}

inline void QGLFunctions::glUniform3iv(GLint location, GLsizei count, const GLint* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform3iv(location, count, v);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform3iv(location, count, v);
#endif
}

inline void QGLFunctions::glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform4f(location, x, y, z, w);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform4f(location, x, y, z, w);
#endif
}

inline void QGLFunctions::glUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform4fv(location, count, v);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform4fv(location, count, v);
#endif
}

inline void QGLFunctions::glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform4i(location, x, y, z, w);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform4i(location, x, y, z, w);
#endif
}

inline void QGLFunctions::glUniform4iv(GLint location, GLsizei count, const GLint* v)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniform4iv(location, count, v);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniform4iv(location, count, v);
#endif
}

inline void QGLFunctions::glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniformMatrix2fv(location, count, transpose, value);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniformMatrix2fv(location, count, transpose, value);
#endif
}

inline void QGLFunctions::glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniformMatrix3fv(location, count, transpose, value);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniformMatrix3fv(location, count, transpose, value);
#endif
}

inline void QGLFunctions::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#if defined(QT_OPENGL_ES_2)
    ::glUniformMatrix4fv(location, count, transpose, value);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->uniformMatrix4fv(location, count, transpose, value);
#endif
}

inline void QGLFunctions::glUseProgram(GLuint program)
{
#if defined(QT_OPENGL_ES_2)
    ::glUseProgram(program);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->useProgram(program);
#endif
}

inline void QGLFunctions::glValidateProgram(GLuint program)
{
#if defined(QT_OPENGL_ES_2)
    ::glValidateProgram(program);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->validateProgram(program);
#endif
}

inline void QGLFunctions::glVertexAttrib1f(GLuint indx, GLfloat x)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib1f(indx, x);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->vertexAttrib1f(indx, x);
#endif
}

inline void QGLFunctions::glVertexAttrib1fv(GLuint indx, const GLfloat* values)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib1fv(indx, values);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->vertexAttrib1fv(indx, values);
#endif
}

inline void QGLFunctions::glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib2f(indx, x, y);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->vertexAttrib2f(indx, x, y);
#endif
}

inline void QGLFunctions::glVertexAttrib2fv(GLuint indx, const GLfloat* values)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib2fv(indx, values);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->vertexAttrib2fv(indx, values);
#endif
}

inline void QGLFunctions::glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib3f(indx, x, y, z);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->vertexAttrib3f(indx, x, y, z);
#endif
}

inline void QGLFunctions::glVertexAttrib3fv(GLuint indx, const GLfloat* values)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib3fv(indx, values);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->vertexAttrib3fv(indx, values);
#endif
}

inline void QGLFunctions::glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib4f(indx, x, y, z, w);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->vertexAttrib4f(indx, x, y, z, w);
#endif
}

inline void QGLFunctions::glVertexAttrib4fv(GLuint indx, const GLfloat* values)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttrib4fv(indx, values);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->vertexAttrib4fv(indx, values);
#endif
}

inline void QGLFunctions::glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)
{
#if defined(QT_OPENGL_ES_2)
    ::glVertexAttribPointer(indx, size, type, normalized, stride, ptr);
#else
    Q_ASSERT(QGLFunctions::isInitialized(d_ptr));
    d_ptr->vertexAttribPointer(indx, size, type, normalized, stride, ptr);
#endif
}

#ifndef GL_ACTIVE_ATTRIBUTE_MAX_LENGTH
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH 0x8B8A
#endif
#ifndef GL_ACTIVE_ATTRIBUTES
#define GL_ACTIVE_ATTRIBUTES 0x8B89
#endif
#ifndef GL_ACTIVE_TEXTURE
#define GL_ACTIVE_TEXTURE 0x84E0
#endif
#ifndef GL_ACTIVE_UNIFORM_MAX_LENGTH
#define GL_ACTIVE_UNIFORM_MAX_LENGTH 0x8B87
#endif
#ifndef GL_ACTIVE_UNIFORMS
#define GL_ACTIVE_UNIFORMS 0x8B86
#endif
#ifndef GL_ALIASED_LINE_WIDTH_RANGE
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#endif
#ifndef GL_ALIASED_POINT_SIZE_RANGE
#define GL_ALIASED_POINT_SIZE_RANGE 0x846D
#endif
#ifndef GL_ALPHA
#define GL_ALPHA 0x1906
#endif
#ifndef GL_ALPHA_BITS
#define GL_ALPHA_BITS 0x0D55
#endif
#ifndef GL_ALWAYS
#define GL_ALWAYS 0x0207
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ARRAY_BUFFER_BINDING
#define GL_ARRAY_BUFFER_BINDING 0x8894
#endif
#ifndef GL_ATTACHED_SHADERS
#define GL_ATTACHED_SHADERS 0x8B85
#endif
#ifndef GL_BACK
#define GL_BACK 0x0405
#endif
#ifndef GL_BLEND
#define GL_BLEND 0x0BE2
#endif
#ifndef GL_BLEND_COLOR
#define GL_BLEND_COLOR 0x8005
#endif
#ifndef GL_BLEND_DST_ALPHA
#define GL_BLEND_DST_ALPHA 0x80CA
#endif
#ifndef GL_BLEND_DST_RGB
#define GL_BLEND_DST_RGB 0x80C8
#endif
#ifndef GL_BLEND_EQUATION
#define GL_BLEND_EQUATION 0x8009
#endif
#ifndef GL_BLEND_EQUATION_ALPHA
#define GL_BLEND_EQUATION_ALPHA 0x883D
#endif
#ifndef GL_BLEND_EQUATION_RGB
#define GL_BLEND_EQUATION_RGB 0x8009
#endif
#ifndef GL_BLEND_SRC_ALPHA
#define GL_BLEND_SRC_ALPHA 0x80CB
#endif
#ifndef GL_BLEND_SRC_RGB
#define GL_BLEND_SRC_RGB 0x80C9
#endif
#ifndef GL_BLUE_BITS
#define GL_BLUE_BITS 0x0D54
#endif
#ifndef GL_BOOL
#define GL_BOOL 0x8B56
#endif
#ifndef GL_BOOL_VEC2
#define GL_BOOL_VEC2 0x8B57
#endif
#ifndef GL_BOOL_VEC3
#define GL_BOOL_VEC3 0x8B58
#endif
#ifndef GL_BOOL_VEC4
#define GL_BOOL_VEC4 0x8B59
#endif
#ifndef GL_BUFFER_SIZE
#define GL_BUFFER_SIZE 0x8764
#endif
#ifndef GL_BUFFER_USAGE
#define GL_BUFFER_USAGE 0x8765
#endif
#ifndef GL_BYTE
#define GL_BYTE 0x1400
#endif
#ifndef GL_CCW
#define GL_CCW 0x0901
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_COLOR_CLEAR_VALUE
#define GL_COLOR_CLEAR_VALUE 0x0C22
#endif
#ifndef GL_COLOR_WRITEMASK
#define GL_COLOR_WRITEMASK 0x0C23
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_COMPRESSED_TEXTURE_FORMATS
#define GL_COMPRESSED_TEXTURE_FORMATS 0x86A3
#endif
#ifndef GL_CONSTANT_ALPHA
#define GL_CONSTANT_ALPHA 0x8003
#endif
#ifndef GL_CONSTANT_COLOR
#define GL_CONSTANT_COLOR 0x8001
#endif
#ifndef GL_CULL_FACE
#define GL_CULL_FACE 0x0B44
#endif
#ifndef GL_CULL_FACE_MODE
#define GL_CULL_FACE_MODE 0x0B45
#endif
#ifndef GL_CURRENT_PROGRAM
#define GL_CURRENT_PROGRAM 0x8B8D
#endif
#ifndef GL_CURRENT_VERTEX_ATTRIB
#define GL_CURRENT_VERTEX_ATTRIB 0x8626
#endif
#ifndef GL_CW
#define GL_CW 0x0900
#endif
#ifndef GL_DECR
#define GL_DECR 0x1E03
#endif
#ifndef GL_DECR_WRAP
#define GL_DECR_WRAP 0x8508
#endif
#ifndef GL_DELETE_STATUS
#define GL_DELETE_STATUS 0x8B80
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT 0x8D00
#endif
#ifndef GL_DEPTH_BITS
#define GL_DEPTH_BITS 0x0D56
#endif
#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_DEPTH_CLEAR_VALUE
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#endif
#ifndef GL_DEPTH_COMPONENT
#define GL_DEPTH_COMPONENT 0x1902
#endif
#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16 0x81A5
#endif
#ifndef GL_DEPTH_FUNC
#define GL_DEPTH_FUNC 0x0B74
#endif
#ifndef GL_DEPTH_RANGE
#define GL_DEPTH_RANGE 0x0B70
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif
#ifndef GL_DEPTH_WRITEMASK
#define GL_DEPTH_WRITEMASK 0x0B72
#endif
#ifndef GL_DITHER
#define GL_DITHER 0x0BD0
#endif
#ifndef GL_DONT_CARE
#define GL_DONT_CARE 0x1100
#endif
#ifndef GL_DST_ALPHA
#define GL_DST_ALPHA 0x0304
#endif
#ifndef GL_DST_COLOR
#define GL_DST_COLOR 0x0306
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER_BINDING
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#endif
#ifndef GL_EQUAL
#define GL_EQUAL 0x0202
#endif
#ifndef GL_EXTENSIONS
#define GL_EXTENSIONS 0x1F03
#endif
#ifndef GL_FALSE
#define GL_FALSE 0
#endif
#ifndef GL_FASTEST
#define GL_FASTEST 0x1101
#endif
#ifndef GL_FIXED
#define GL_FIXED 0x140C
#endif
#ifndef GL_FLOAT
#define GL_FLOAT 0x1406
#endif
#ifndef GL_FLOAT_MAT2
#define GL_FLOAT_MAT2 0x8B5A
#endif
#ifndef GL_FLOAT_MAT3
#define GL_FLOAT_MAT3 0x8B5B
#endif
#ifndef GL_FLOAT_MAT4
#define GL_FLOAT_MAT4 0x8B5C
#endif
#ifndef GL_FLOAT_VEC2
#define GL_FLOAT_VEC2 0x8B50
#endif
#ifndef GL_FLOAT_VEC3
#define GL_FLOAT_VEC3 0x8B51
#endif
#ifndef GL_FLOAT_VEC4
#define GL_FLOAT_VEC4 0x8B52
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#endif
#ifndef GL_FRAMEBUFFER_UNSUPPORTED
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#endif
#ifndef GL_FRONT
#define GL_FRONT 0x0404
#endif
#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK 0x0408
#endif
#ifndef GL_FRONT_FACE
#define GL_FRONT_FACE 0x0B46
#endif
#ifndef GL_FUNC_ADD
#define GL_FUNC_ADD 0x8006
#endif
#ifndef GL_FUNC_REVERSE_SUBTRACT
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#endif
#ifndef GL_FUNC_SUBTRACT
#define GL_FUNC_SUBTRACT 0x800A
#endif
#ifndef GL_GENERATE_MIPMAP_HINT
#define GL_GENERATE_MIPMAP_HINT 0x8192
#endif
#ifndef GL_GEQUAL
#define GL_GEQUAL 0x0206
#endif
#ifndef GL_GREATER
#define GL_GREATER 0x0204
#endif
#ifndef GL_GREEN_BITS
#define GL_GREEN_BITS 0x0D53
#endif
#ifndef GL_HIGH_FLOAT
#define GL_HIGH_FLOAT 0x8DF2
#endif
#ifndef GL_HIGH_INT
#define GL_HIGH_INT 0x8DF5
#endif
#ifndef GL_IMPLEMENTATION_COLOR_READ_FORMAT
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT 0x8B9B
#endif
#ifndef GL_IMPLEMENTATION_COLOR_READ_TYPE
#define GL_IMPLEMENTATION_COLOR_READ_TYPE 0x8B9A
#endif
#ifndef GL_INCR
#define GL_INCR 0x1E02
#endif
#ifndef GL_INCR_WRAP
#define GL_INCR_WRAP 0x8507
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_INT
#define GL_INT 0x1404
#endif
#ifndef GL_INT_VEC2
#define GL_INT_VEC2 0x8B53
#endif
#ifndef GL_INT_VEC3
#define GL_INT_VEC3 0x8B54
#endif
#ifndef GL_INT_VEC4
#define GL_INT_VEC4 0x8B55
#endif
#ifndef GL_INVALID_ENUM
#define GL_INVALID_ENUM 0x0500
#endif
#ifndef GL_INVALID_FRAMEBUFFER_OPERATION
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#endif
#ifndef GL_INVALID_OPERATION
#define GL_INVALID_OPERATION 0x0502
#endif
#ifndef GL_INVALID_VALUE
#define GL_INVALID_VALUE 0x0501
#endif
#ifndef GL_INVERT
#define GL_INVERT 0x150A
#endif
#ifndef GL_KEEP
#define GL_KEEP 0x1E00
#endif
#ifndef GL_LEQUAL
#define GL_LEQUAL 0x0203
#endif
#ifndef GL_LESS
#define GL_LESS 0x0201
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif
#ifndef GL_LINEAR_MIPMAP_NEAREST
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#endif
#ifndef GL_LINE_LOOP
#define GL_LINE_LOOP 0x0002
#endif
#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0003
#endif
#ifndef GL_LINE_WIDTH
#define GL_LINE_WIDTH 0x0B21
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_LOW_FLOAT
#define GL_LOW_FLOAT 0x8DF0
#endif
#ifndef GL_LOW_INT
#define GL_LOW_INT 0x8DF3
#endif
#ifndef GL_LUMINANCE
#define GL_LUMINANCE 0x1909
#endif
#ifndef GL_LUMINANCE_ALPHA
#define GL_LUMINANCE_ALPHA 0x190A
#endif
#ifndef GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#endif
#ifndef GL_MAX_CUBE_MAP_TEXTURE_SIZE
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
#endif
#ifndef GL_MAX_FRAGMENT_UNIFORM_VECTORS
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS 0x8DFD
#endif
#ifndef GL_MAX_RENDERBUFFER_SIZE
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#endif
#ifndef GL_MAX_TEXTURE_IMAGE_UNITS
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#endif
#ifndef GL_MAX_TEXTURE_SIZE
#define GL_MAX_TEXTURE_SIZE 0x0D33
#endif
#ifndef GL_MAX_VARYING_VECTORS
#define GL_MAX_VARYING_VECTORS 0x8DFC
#endif
#ifndef GL_MAX_VERTEX_ATTRIBS
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#endif
#ifndef GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#endif
#ifndef GL_MAX_VERTEX_UNIFORM_VECTORS
#define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
#endif
#ifndef GL_MAX_VIEWPORT_DIMS
#define GL_MAX_VIEWPORT_DIMS 0x0D3A
#endif
#ifndef GL_MEDIUM_FLOAT
#define GL_MEDIUM_FLOAT 0x8DF1
#endif
#ifndef GL_MEDIUM_INT
#define GL_MEDIUM_INT 0x8DF4
#endif
#ifndef GL_MIRRORED_REPEAT
#define GL_MIRRORED_REPEAT 0x8370
#endif
#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif
#ifndef GL_NEAREST_MIPMAP_LINEAR
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#endif
#ifndef GL_NEAREST_MIPMAP_NEAREST
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#endif
#ifndef GL_NEVER
#define GL_NEVER 0x0200
#endif
#ifndef GL_NICEST
#define GL_NICEST 0x1102
#endif
#ifndef GL_NO_ERROR
#define GL_NO_ERROR 0
#endif
#ifndef GL_NONE
#define GL_NONE 0
#endif
#ifndef GL_NOTEQUAL
#define GL_NOTEQUAL 0x0205
#endif
#ifndef GL_NUM_COMPRESSED_TEXTURE_FORMATS
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#endif
#ifndef GL_NUM_SHADER_BINARY_FORMATS
#define GL_NUM_SHADER_BINARY_FORMATS 0x8DF9
#endif
#ifndef GL_ONE
#define GL_ONE 1
#endif
#ifndef GL_ONE_MINUS_CONSTANT_ALPHA
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#endif
#ifndef GL_ONE_MINUS_CONSTANT_COLOR
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#endif
#ifndef GL_ONE_MINUS_DST_ALPHA
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#endif
#ifndef GL_ONE_MINUS_DST_COLOR
#define GL_ONE_MINUS_DST_COLOR 0x0307
#endif
#ifndef GL_ONE_MINUS_SRC_ALPHA
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif
#ifndef GL_OUT_OF_MEMORY
#define GL_OUT_OF_MEMORY 0x0505
#endif
#ifndef GL_PACK_ALIGNMENT
#define GL_PACK_ALIGNMENT 0x0D05
#endif
#ifndef GL_POINTS
#define GL_POINTS 0x0000
#endif
#ifndef GL_POLYGON_OFFSET_FACTOR
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#endif
#ifndef GL_POLYGON_OFFSET_FILL
#define GL_POLYGON_OFFSET_FILL 0x8037
#endif
#ifndef GL_POLYGON_OFFSET_UNITS
#define GL_POLYGON_OFFSET_UNITS 0x2A00
#endif
#ifndef GL_RED_BITS
#define GL_RED_BITS 0x0D52
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif
#ifndef GL_RENDERBUFFER_ALPHA_SIZE
#define GL_RENDERBUFFER_ALPHA_SIZE 0x8D53
#endif
#ifndef GL_RENDERBUFFER_BINDING
#define GL_RENDERBUFFER_BINDING 0x8CA7
#endif
#ifndef GL_RENDERBUFFER_BLUE_SIZE
#define GL_RENDERBUFFER_BLUE_SIZE 0x8D52
#endif
#ifndef GL_RENDERBUFFER_DEPTH_SIZE
#define GL_RENDERBUFFER_DEPTH_SIZE 0x8D54
#endif
#ifndef GL_RENDERBUFFER_GREEN_SIZE
#define GL_RENDERBUFFER_GREEN_SIZE 0x8D51
#endif
#ifndef GL_RENDERBUFFER_HEIGHT
#define GL_RENDERBUFFER_HEIGHT 0x8D43
#endif
#ifndef GL_RENDERBUFFER_INTERNAL_FORMAT
#define GL_RENDERBUFFER_INTERNAL_FORMAT 0x8D44
#endif
#ifndef GL_RENDERBUFFER_RED_SIZE
#define GL_RENDERBUFFER_RED_SIZE 0x8D50
#endif
#ifndef GL_RENDERBUFFER_STENCIL_SIZE
#define GL_RENDERBUFFER_STENCIL_SIZE 0x8D55
#endif
#ifndef GL_RENDERBUFFER_WIDTH
#define GL_RENDERBUFFER_WIDTH 0x8D42
#endif
#ifndef GL_RENDERER
#define GL_RENDERER 0x1F01
#endif
#ifndef GL_REPEAT
#define GL_REPEAT 0x2901
#endif
#ifndef GL_REPLACE
#define GL_REPLACE 0x1E01
#endif
#ifndef GL_RGB
#define GL_RGB 0x1907
#endif
#ifndef GL_RGB565
#define GL_RGB565 0x8D62
#endif
#ifndef GL_RGB5_A1
#define GL_RGB5_A1 0x8057
#endif
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_RGBA4
#define GL_RGBA4 0x8056
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_SAMPLE_ALPHA_TO_COVERAGE
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#endif
#ifndef GL_SAMPLE_BUFFERS
#define GL_SAMPLE_BUFFERS 0x80A8
#endif
#ifndef GL_SAMPLE_COVERAGE
#define GL_SAMPLE_COVERAGE 0x80A0
#endif
#ifndef GL_SAMPLE_COVERAGE_INVERT
#define GL_SAMPLE_COVERAGE_INVERT 0x80AB
#endif
#ifndef GL_SAMPLE_COVERAGE_VALUE
#define GL_SAMPLE_COVERAGE_VALUE 0x80AA
#endif
#ifndef GL_SAMPLER_2D
#define GL_SAMPLER_2D 0x8B5E
#endif
#ifndef GL_SAMPLER_CUBE
#define GL_SAMPLER_CUBE 0x8B60
#endif
#ifndef GL_SAMPLES
#define GL_SAMPLES 0x80A9
#endif
#ifndef GL_SCISSOR_BOX
#define GL_SCISSOR_BOX 0x0C10
#endif
#ifndef GL_SCISSOR_TEST
#define GL_SCISSOR_TEST 0x0C11
#endif
#ifndef GL_SHADER_BINARY_FORMATS
#define GL_SHADER_BINARY_FORMATS 0x8DF8
#endif
#ifndef GL_SHADER_COMPILER
#define GL_SHADER_COMPILER 0x8DFA
#endif
#ifndef GL_SHADER_SOURCE_LENGTH
#define GL_SHADER_SOURCE_LENGTH 0x8B88
#endif
#ifndef GL_SHADER_TYPE
#define GL_SHADER_TYPE 0x8B4F
#endif
#ifndef GL_SHADING_LANGUAGE_VERSION
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#endif
#ifndef GL_SHORT
#define GL_SHORT 0x1402
#endif
#ifndef GL_SRC_ALPHA
#define GL_SRC_ALPHA 0x0302
#endif
#ifndef GL_SRC_ALPHA_SATURATE
#define GL_SRC_ALPHA_SATURATE 0x0308
#endif
#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR 0x0300
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_STENCIL_ATTACHMENT
#define GL_STENCIL_ATTACHMENT 0x8D20
#endif
#ifndef GL_STENCIL_BACK_FAIL
#define GL_STENCIL_BACK_FAIL 0x8801
#endif
#ifndef GL_STENCIL_BACK_FUNC
#define GL_STENCIL_BACK_FUNC 0x8800
#endif
#ifndef GL_STENCIL_BACK_PASS_DEPTH_FAIL
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL 0x8802
#endif
#ifndef GL_STENCIL_BACK_PASS_DEPTH_PASS
#define GL_STENCIL_BACK_PASS_DEPTH_PASS 0x8803
#endif
#ifndef GL_STENCIL_BACK_REF
#define GL_STENCIL_BACK_REF 0x8CA3
#endif
#ifndef GL_STENCIL_BACK_VALUE_MASK
#define GL_STENCIL_BACK_VALUE_MASK 0x8CA4
#endif
#ifndef GL_STENCIL_BACK_WRITEMASK
#define GL_STENCIL_BACK_WRITEMASK 0x8CA5
#endif
#ifndef GL_STENCIL_BITS
#define GL_STENCIL_BITS 0x0D57
#endif
#ifndef GL_STENCIL_BUFFER_BIT
#define GL_STENCIL_BUFFER_BIT 0x00000400
#endif
#ifndef GL_STENCIL_CLEAR_VALUE
#define GL_STENCIL_CLEAR_VALUE 0x0B91
#endif
#ifndef GL_STENCIL_FAIL
#define GL_STENCIL_FAIL 0x0B94
#endif
#ifndef GL_STENCIL_FUNC
#define GL_STENCIL_FUNC 0x0B92
#endif
#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX 0x1901
#endif
#ifndef GL_STENCIL_INDEX8
#define GL_STENCIL_INDEX8 0x8D48
#endif
#ifndef GL_STENCIL_PASS_DEPTH_FAIL
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#endif
#ifndef GL_STENCIL_PASS_DEPTH_PASS
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#endif
#ifndef GL_STENCIL_REF
#define GL_STENCIL_REF 0x0B97
#endif
#ifndef GL_STENCIL_TEST
#define GL_STENCIL_TEST 0x0B90
#endif
#ifndef GL_STENCIL_VALUE_MASK
#define GL_STENCIL_VALUE_MASK 0x0B93
#endif
#ifndef GL_STENCIL_WRITEMASK
#define GL_STENCIL_WRITEMASK 0x0B98
#endif
#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW 0x88E0
#endif
#ifndef GL_SUBPIXEL_BITS
#define GL_SUBPIXEL_BITS 0x0D50
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE
#define GL_TEXTURE 0x1702
#endif
#ifndef GL_TEXTURE10
#define GL_TEXTURE10 0x84CA
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE11
#define GL_TEXTURE11 0x84CB
#endif
#ifndef GL_TEXTURE12
#define GL_TEXTURE12 0x84CC
#endif
#ifndef GL_TEXTURE13
#define GL_TEXTURE13 0x84CD
#endif
#ifndef GL_TEXTURE14
#define GL_TEXTURE14 0x84CE
#endif
#ifndef GL_TEXTURE15
#define GL_TEXTURE15 0x84CF
#endif
#ifndef GL_TEXTURE16
#define GL_TEXTURE16 0x84D0
#endif
#ifndef GL_TEXTURE17
#define GL_TEXTURE17 0x84D1
#endif
#ifndef GL_TEXTURE18
#define GL_TEXTURE18 0x84D2
#endif
#ifndef GL_TEXTURE19
#define GL_TEXTURE19 0x84D3
#endif
#ifndef GL_TEXTURE20
#define GL_TEXTURE20 0x84D4
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif
#ifndef GL_TEXTURE21
#define GL_TEXTURE21 0x84D5
#endif
#ifndef GL_TEXTURE22
#define GL_TEXTURE22 0x84D6
#endif
#ifndef GL_TEXTURE23
#define GL_TEXTURE23 0x84D7
#endif
#ifndef GL_TEXTURE24
#define GL_TEXTURE24 0x84D8
#endif
#ifndef GL_TEXTURE25
#define GL_TEXTURE25 0x84D9
#endif
#ifndef GL_TEXTURE26
#define GL_TEXTURE26 0x84DA
#endif
#ifndef GL_TEXTURE27
#define GL_TEXTURE27 0x84DB
#endif
#ifndef GL_TEXTURE28
#define GL_TEXTURE28 0x84DC
#endif
#ifndef GL_TEXTURE29
#define GL_TEXTURE29 0x84DD
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_TEXTURE30
#define GL_TEXTURE30 0x84DE
#endif
#ifndef GL_TEXTURE3
#define GL_TEXTURE3 0x84C3
#endif
#ifndef GL_TEXTURE31
#define GL_TEXTURE31 0x84DF
#endif
#ifndef GL_TEXTURE4
#define GL_TEXTURE4 0x84C4
#endif
#ifndef GL_TEXTURE5
#define GL_TEXTURE5 0x84C5
#endif
#ifndef GL_TEXTURE6
#define GL_TEXTURE6 0x84C6
#endif
#ifndef GL_TEXTURE7
#define GL_TEXTURE7 0x84C7
#endif
#ifndef GL_TEXTURE8
#define GL_TEXTURE8 0x84C8
#endif
#ifndef GL_TEXTURE9
#define GL_TEXTURE9 0x84C9
#endif
#ifndef GL_TEXTURE_BINDING_2D
#define GL_TEXTURE_BINDING_2D 0x8069
#endif
#ifndef GL_TEXTURE_BINDING_CUBE_MAP
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514
#endif
#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP 0x8513
#endif
#ifndef GL_TEXTURE_CUBE_MAP_NEGATIVE_X
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#endif
#ifndef GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#endif
#ifndef GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_X
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_Y
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_Z
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif
#ifndef GL_TRUE
#define GL_TRUE 1
#endif
#ifndef GL_UNPACK_ALIGNMENT
#define GL_UNPACK_ALIGNMENT 0x0CF5
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_UNSIGNED_SHORT
#define GL_UNSIGNED_SHORT 0x1403
#endif
#ifndef GL_UNSIGNED_SHORT_4_4_4_4
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#endif
#ifndef GL_UNSIGNED_SHORT_5_5_5_1
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#endif
#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif
#ifndef GL_VALIDATE_STATUS
#define GL_VALIDATE_STATUS 0x8B83
#endif
#ifndef GL_VENDOR
#define GL_VENDOR 0x1F00
#endif
#ifndef GL_VERSION
#define GL_VERSION 0x1F02
#endif
#ifndef GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#endif
#ifndef GL_VERTEX_ATTRIB_ARRAY_ENABLED
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED 0x8622
#endif
#ifndef GL_VERTEX_ATTRIB_ARRAY_NORMALIZED
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED 0x886A
#endif
#ifndef GL_VERTEX_ATTRIB_ARRAY_POINTER
#define GL_VERTEX_ATTRIB_ARRAY_POINTER 0x8645
#endif
#ifndef GL_VERTEX_ATTRIB_ARRAY_SIZE
#define GL_VERTEX_ATTRIB_ARRAY_SIZE 0x8623
#endif
#ifndef GL_VERTEX_ATTRIB_ARRAY_STRIDE
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE 0x8624
#endif
#ifndef GL_VERTEX_ATTRIB_ARRAY_TYPE
#define GL_VERTEX_ATTRIB_ARRAY_TYPE 0x8625
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_VIEWPORT
#define GL_VIEWPORT 0x0BA2
#endif
#ifndef GL_ZERO
#define GL_ZERO 0
#endif

QT_END_NAMESPACE

#endif
