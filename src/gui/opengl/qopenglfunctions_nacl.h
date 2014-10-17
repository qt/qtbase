/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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


//
// Note! this header is designed to be directly included from
// qglfunctions.h and should not be used on its own.
//

inline void QOpenGLFunctions::glBindTexture(GLenum target, GLuint texture)
{
// TODO: remove ifdef and only have "GLES2"
#ifdef QT_OPENGL_ES_2
    ::GLES2BindTexture(target, texture);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindTexture(target, texture);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendFunc(GLenum sfactor, GLenum dfactor)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2BlendFunc(sfactor, dfactor);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendFunc(sfactor, dfactor);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glClear(GLbitfield mask)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Clear(mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Clear(mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2ClearColor(red, green, blue, alpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ClearColor(red, green, blue, alpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glClearStencil(GLint s)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2ClearStencil(s);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ClearStencil(s);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2ColorMask(red, green, blue, alpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ColorMask(red, green, blue, alpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2CopyTexImage2D(target, level, internalformat, x, y, width,height, border);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CopyTexImage2D(target, level, internalformat, x, y, width,height, border);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCullFace(GLenum mode)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2CullFace(mode);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CullFace(mode);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteTextures(GLsizei n, const GLuint* textures)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DeleteTextures(n, textures);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteTextures(n, textures);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDepthFunc(GLenum func)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DepthFunc(func);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DepthFunc(func);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDepthMask(GLboolean flag)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DepthMask(flag);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DepthMask(flag);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDisable(GLenum cap)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Disable(cap);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Disable(cap);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DrawArrays(mode, first, count);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DrawArrays(mode, first, count);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DrawElements(mode, count, type, indices);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DrawElements(mode, count, type, indices);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glEnable(GLenum cap)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Enable(cap);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Enable(cap);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFinish()
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Finish();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Finish();
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFlush()
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Flush();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Flush();
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFrontFace(GLenum mode)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2FrontFace(mode);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->FrontFace(mode);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenTextures(GLsizei n, GLuint* textures)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GenTextures(n, textures);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenTextures(n, textures);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetBooleanv(GLenum pname, GLboolean* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetBooleanv(pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetBooleanv(pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLenum QOpenGLFunctions::glGetError()
{
#ifdef QT_OPENGL_ES_2
    GLenum result = ::GLES2GetError();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLenum result = d_ptr->GetError();
#endif
    return result;
}

inline void QOpenGLFunctions::glGetFloatv(GLenum pname, GLfloat* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetFloatv(pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetFloatv(pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetIntegerv(GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetIntegerv(pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetIntegerv(pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline const GLubyte *QOpenGLFunctions::glGetString(GLenum name)
{
#ifdef QT_OPENGL_ES_2
    const GLubyte *result = ::GLES2GetString(name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    const GLubyte *result = d_ptr->GetString(name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetTexParameterfv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetTexParameterfv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetTexParameteriv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetTexParameteriv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glHint(GLenum target, GLenum mode)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Hint(target, mode);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Hint(target, mode);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLboolean QOpenGLFunctions::glIsEnabled(GLenum cap)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::GLES2IsEnabled(cap);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsEnabled(cap);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsTexture(GLuint texture)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::GLES2IsTexture(texture);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsTexture(texture);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glLineWidth(GLfloat width)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2LineWidth(width);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->LineWidth(width);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glPixelStorei(GLenum pname, GLint param)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2PixelStorei(pname, param);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->PixelStorei(pname, param);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glPolygonOffset(GLfloat factor, GLfloat units)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2PolygonOffset(factor, units);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->PolygonOffset(factor, units);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2ReadPixels(x, y, width, height, format, type, pixels);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ReadPixels(x, y, width, height, format, type, pixels);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Scissor(x, y, width, height);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Scissor(x, y, width, height);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2StencilFunc(func, ref, mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilFunc(func, ref, mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilMask(GLuint mask)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2StencilMask(mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilMask(mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2StencilOp(fail, zfail, zpass);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilOp(fail, zfail, zpass);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2TexImage2D(target, level, internalformat, width,height, border, format, type, pixels);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexImage2D(target, level, internalformat, width,height, border, format, type, pixels);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2TexParameterf(target, pname, param);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexParameterf(target, pname, param);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2TexParameterfv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexParameterfv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexParameteri(GLenum target, GLenum pname, GLint param)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2TexParameteri(target, pname, param);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexParameteri(target, pname, param);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2TexParameteriv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexParameteriv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2TexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Viewport(x, y, width, height);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Viewport(x, y, width, height);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

// GL(ES)2

inline void QOpenGLFunctions::glActiveTexture(GLenum texture)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2ActiveTexture(texture);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ActiveTexture(texture);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glAttachShader(GLuint program, GLuint shader)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2AttachShader(program, shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->AttachShader(program, shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindAttribLocation(GLuint program, GLuint index, const char* name)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2BindAttribLocation(program, index, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindAttribLocation(program, index, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindBuffer(GLenum target, GLuint buffer)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2BindBuffer(target, buffer);
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
#ifdef QT_OPENGL_ES_2
    ::GLES2BindFramebuffer(target, framebuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindFramebuffer(target, framebuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2BindRenderbuffer(target, renderbuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindRenderbuffer(target, renderbuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2BlendColor(red, green, blue, alpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendColor(red, green, blue, alpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendEquation(GLenum mode)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2BlendEquation(mode);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendEquation(mode);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2BlendEquationSeparate(modeRGB, modeAlpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendEquationSeparate(modeRGB, modeAlpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2BlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBufferData(GLenum target, qopengl_GLsizeiptr size, const void* data, GLenum usage)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2BufferData(target, size, data, usage);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BufferData(target, size, data, usage);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, const void* data)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2BufferSubData(target, offset, size, data);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BufferSubData(target, offset, size, data);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLenum QOpenGLFunctions::glCheckFramebufferStatus(GLenum target)
{
#ifdef QT_OPENGL_ES_2
    GLenum result = ::GLES2CheckFramebufferStatus(target);
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
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ClearDepthf(depth);
#else
    ::GLES2ClearDepthf(depth);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCompileShader(GLuint shader)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2CompileShader(shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CompileShader(shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2CompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2CompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLuint QOpenGLFunctions::glCreateProgram()
{
#ifdef QT_OPENGL_ES_2
    GLuint result = ::GLES2CreateProgram();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLuint result = d_ptr->CreateProgram();
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLuint QOpenGLFunctions::glCreateShader(GLenum type)
{
#ifdef QT_OPENGL_ES_2
    GLuint result = ::GLES2CreateShader(type);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLuint result = d_ptr->CreateShader(type);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DeleteBuffers(n, buffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteBuffers(n, buffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DeleteFramebuffers(n, framebuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteFramebuffers(n, framebuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteProgram(GLuint program)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DeleteProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DeleteRenderbuffers(n, renderbuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteRenderbuffers(n, renderbuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteShader(GLuint shader)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DeleteShader(shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteShader(shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDepthRangef(GLclampf zNear, GLclampf zFar)
{
#ifndef QT_OPENGL_ES
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DepthRangef(zNear, zFar);
#else
    ::GLES2DepthRangef(zNear, zFar);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDetachShader(GLuint program, GLuint shader)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DetachShader(program, shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DetachShader(program, shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDisableVertexAttribArray(GLuint index)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2DisableVertexAttribArray(index);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DisableVertexAttribArray(index);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glEnableVertexAttribArray(GLuint index)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2EnableVertexAttribArray(index);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->EnableVertexAttribArray(index);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2FramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->FramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2FramebufferTexture2D(target, attachment, textarget, texture, level);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->FramebufferTexture2D(target, attachment, textarget, texture, level);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenBuffers(GLsizei n, GLuint* buffers)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GenBuffers(n, buffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenBuffers(n, buffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenerateMipmap(GLenum target)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GenerateMipmap(target);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenerateMipmap(target);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GenFramebuffers(n, framebuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenFramebuffers(n, framebuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GenRenderbuffers(n, renderbuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenRenderbuffers(n, renderbuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetActiveAttrib(program, index, bufsize, length, size, type, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetActiveAttrib(program, index, bufsize, length, size, type, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetActiveUniform(program, index, bufsize, length, size, type, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetActiveUniform(program, index, bufsize, length, size, type, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetAttachedShaders(program, maxcount, count, shaders);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetAttachedShaders(program, maxcount, count, shaders);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLint QOpenGLFunctions::glGetAttribLocation(GLuint program, const char* name)
{
#ifdef QT_OPENGL_ES_2
    GLint result = ::GLES2GetAttribLocation(program, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLint result = d_ptr->GetAttribLocation(program, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetBufferParameteriv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetBufferParameteriv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetFramebufferAttachmentParameteriv(target, attachment, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetFramebufferAttachmentParameteriv(target, attachment, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetProgramiv(program, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetProgramiv(program, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetProgramInfoLog(program, bufsize, length, infolog);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetProgramInfoLog(program, bufsize, length, infolog);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetRenderbufferParameteriv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetRenderbufferParameteriv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetShaderiv(shader, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderiv(shader, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetShaderInfoLog(shader, bufsize, length, infolog);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderInfoLog(shader, bufsize, length, infolog);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetShaderSource(shader, bufsize, length, source);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderSource(shader, bufsize, length, source);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetUniformfv(program, location, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetUniformfv(program, location, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetUniformiv(GLuint program, GLint location, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetUniformiv(program, location, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetUniformiv(program, location, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLint QOpenGLFunctions::glGetUniformLocation(GLuint program, const char* name)
{
#ifdef QT_OPENGL_ES_2
    GLint result = ::GLES2GetUniformLocation(program, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLint result = d_ptr->GetUniformLocation(program, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetVertexAttribfv(index, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetVertexAttribfv(index, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetVertexAttribiv(index, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetVertexAttribiv(index, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2GetVertexAttribPointerv(index, pname, pointer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetVertexAttribPointerv(index, pname, pointer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLboolean QOpenGLFunctions::glIsBuffer(GLuint buffer)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::GLES2IsBuffer(buffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsBuffer(buffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsFramebuffer(GLuint framebuffer)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::GLES2IsFramebuffer(framebuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsFramebuffer(framebuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsProgram(GLuint program)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::GLES2IsProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsRenderbuffer(GLuint renderbuffer)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::GLES2IsRenderbuffer(renderbuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsRenderbuffer(renderbuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsShader(GLuint shader)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::GLES2IsShader(shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsShader(shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glLinkProgram(GLuint program)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2LinkProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->LinkProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glReleaseShaderCompiler()
{
#ifdef QT_OPENGL_ES_2
    ::GLES2ReleaseShaderCompiler();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ReleaseShaderCompiler();
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2RenderbufferStorage(target, internalformat, width, height);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->RenderbufferStorage(target, internalformat, width, height);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glSampleCoverage(GLclampf value, GLboolean invert)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2SampleCoverage(value, invert);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->SampleCoverage(value, invert);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2ShaderBinary(n, shaders, binaryformat, binary, length);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ShaderBinary(n, shaders, binaryformat, binary, length);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2ShaderSource(shader, count, string, length);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ShaderSource(shader, count, string, length);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2StencilFuncSeparate(face, func, ref, mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilFuncSeparate(face, func, ref, mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilMaskSeparate(GLenum face, GLuint mask)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2StencilMaskSeparate(face, mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilMaskSeparate(face, mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2StencilOpSeparate(face, fail, zfail, zpass);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilOpSeparate(face, fail, zfail, zpass);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1f(GLint location, GLfloat x)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform1f(location, x);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1f(location, x);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform1fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1i(GLint location, GLint x)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform1i(location, x);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1i(location, x);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1iv(GLint location, GLsizei count, const GLint* v)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform1iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2f(GLint location, GLfloat x, GLfloat y)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform2f(location, x, y);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2f(location, x, y);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform2fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2i(GLint location, GLint x, GLint y)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform2i(location, x, y);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2i(location, x, y);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2iv(GLint location, GLsizei count, const GLint* v)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform2iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform3f(location, x, y, z);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3f(location, x, y, z);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform3fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3i(GLint location, GLint x, GLint y, GLint z)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform3i(location, x, y, z);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3i(location, x, y, z);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3iv(GLint location, GLsizei count, const GLint* v)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform3iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform4f(location, x, y, z, w);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4f(location, x, y, z, w);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform4fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform4i(location, x, y, z, w);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4i(location, x, y, z, w);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4iv(GLint location, GLsizei count, const GLint* v)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2Uniform4iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2UniformMatrix2fv(location, count, transpose, value);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UniformMatrix2fv(location, count, transpose, value);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2UniformMatrix3fv(location, count, transpose, value);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UniformMatrix3fv(location, count, transpose, value);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2UniformMatrix4fv(location, count, transpose, value);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UniformMatrix4fv(location, count, transpose, value);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUseProgram(GLuint program)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2UseProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UseProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glValidateProgram(GLuint program)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2ValidateProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ValidateProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib1f(GLuint indx, GLfloat x)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2VertexAttrib1f(indx, x);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib1f(indx, x);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib1fv(GLuint indx, const GLfloat* values)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2VertexAttrib1fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib1fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2VertexAttrib2f(indx, x, y);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib2f(indx, x, y);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib2fv(GLuint indx, const GLfloat* values)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2VertexAttrib2fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib2fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2VertexAttrib3f(indx, x, y, z);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib3f(indx, x, y, z);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib3fv(GLuint indx, const GLfloat* values)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2VertexAttrib3fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib3fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2VertexAttrib4f(indx, x, y, z, w);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib4f(indx, x, y, z, w);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib4fv(GLuint indx, const GLfloat* values)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2VertexAttrib4fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib4fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)
{
#ifdef QT_OPENGL_ES_2
    ::GLES2VertexAttribPointer(indx, size, type, normalized, stride, ptr);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttribPointer(indx, size, type, normalized, stride, ptr);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}