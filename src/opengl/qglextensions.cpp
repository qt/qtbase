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

#include "qgl_p.h"
#include <qglframebufferobject.h>

QT_BEGIN_NAMESPACE

static QFunctionPointer qt_gl_getProcAddress_search
    (QGLContext *ctx, const char *name1, const char *name2,
     const char *name3, const char *name4)
{
    QFunctionPointer addr = ctx->getProcAddress(QLatin1String(name1));
    if (addr)
        return addr;

    addr = ctx->getProcAddress(QLatin1String(name2));
    if (addr)
        return addr;

    addr = ctx->getProcAddress(QLatin1String(name3));
    if (addr)
        return addr;

    if (name4)
        return ctx->getProcAddress(QLatin1String(name4));

    return 0;
}

// Search for an extension function starting with the most likely
// function suffix first, and then trying the other variations.
#if defined(QT_OPENGL_ES)
#define qt_gl_getProcAddress(ctx,name) \
    qt_gl_getProcAddress_search((ctx), name, name "OES", name "EXT", name "ARB")
#define qt_gl_getProcAddressEXT(ctx,name) \
    qt_gl_getProcAddress_search((ctx), name "OES", name, name "EXT", name "ARB")
#define qt_gl_getProcAddressARB(ctx,name) \
    qt_gl_getProcAddress_search((ctx), name "OES", name, name "ARB", name "EXT")
#define qt_gl_getProcAddressOES(ctx,name) \
    qt_gl_getProcAddress_search((ctx), name "OES", name, name "EXT", name "ARB")
#else
#define qt_gl_getProcAddress(ctx,name) \
    qt_gl_getProcAddress_search((ctx), name, name "ARB", name "EXT", 0)
#define qt_gl_getProcAddressEXT(ctx,name) \
    qt_gl_getProcAddress_search((ctx), name "EXT", name, name "ARB", 0)
#define qt_gl_getProcAddressARB(ctx,name) \
    qt_gl_getProcAddress_search((ctx), name "ARB", name, name "EXT", 0)
#define qt_gl_getProcAddressOES(ctx,name) \
    qt_gl_getProcAddress_search((ctx), name "OES", name, name "EXT", name "ARB")
#endif

bool qt_resolve_framebufferobject_extensions(QGLContext *ctx)
{
#if defined(QT_OPENGL_ES_2)
    static bool have_resolved = false;
    if (have_resolved)
        return true;
    have_resolved = true;
#else
    if (glIsRenderbuffer != 0)
        return true;
#endif

    if (ctx == 0) {
        qWarning("QGLFramebufferObject: Unable to resolve framebuffer object extensions -"
                 " make sure there is a current context when creating the framebuffer object.");
        return false;
    }


    glBlitFramebufferEXT = (_glBlitFramebufferEXT) qt_gl_getProcAddressEXT(ctx, "glBlitFramebuffer");
    glRenderbufferStorageMultisampleEXT =
        (_glRenderbufferStorageMultisampleEXT) qt_gl_getProcAddressEXT(ctx, "glRenderbufferStorageMultisample");

#if !defined(QT_OPENGL_ES_2)
    glIsRenderbuffer = (_glIsRenderbuffer) qt_gl_getProcAddressEXT(ctx, "glIsRenderbuffer");
    if (!glIsRenderbuffer)
        return false;   // Not much point searching for anything else.
    glBindRenderbuffer = (_glBindRenderbuffer) qt_gl_getProcAddressEXT(ctx, "glBindRenderbuffer");
    glDeleteRenderbuffers = (_glDeleteRenderbuffers) qt_gl_getProcAddressEXT(ctx, "glDeleteRenderbuffers");
    glGenRenderbuffers = (_glGenRenderbuffers) qt_gl_getProcAddressEXT(ctx, "glGenRenderbuffers");
    glRenderbufferStorage = (_glRenderbufferStorage) qt_gl_getProcAddressEXT(ctx, "glRenderbufferStorage");
    glGetRenderbufferParameteriv =
        (_glGetRenderbufferParameteriv) qt_gl_getProcAddressEXT(ctx, "glGetRenderbufferParameteriv");
    glIsFramebuffer = (_glIsFramebuffer) qt_gl_getProcAddressEXT(ctx, "glIsFramebuffer");
    glBindFramebuffer = (_glBindFramebuffer) qt_gl_getProcAddressEXT(ctx, "glBindFramebuffer");
    glDeleteFramebuffers = (_glDeleteFramebuffers) qt_gl_getProcAddressEXT(ctx, "glDeleteFramebuffers");
    glGenFramebuffers = (_glGenFramebuffers) qt_gl_getProcAddressEXT(ctx, "glGenFramebuffers");
    glCheckFramebufferStatus = (_glCheckFramebufferStatus) qt_gl_getProcAddressEXT(ctx, "glCheckFramebufferStatus");
    glFramebufferTexture2D = (_glFramebufferTexture2D) qt_gl_getProcAddressEXT(ctx, "glFramebufferTexture2D");
    glFramebufferRenderbuffer = (_glFramebufferRenderbuffer) qt_gl_getProcAddressEXT(ctx, "glFramebufferRenderbuffer");
    glGetFramebufferAttachmentParameteriv =
        (_glGetFramebufferAttachmentParameteriv) qt_gl_getProcAddressEXT(ctx, "glGetFramebufferAttachmentParameteriv");
    glGenerateMipmap = (_glGenerateMipmap) qt_gl_getProcAddressEXT(ctx, "glGenerateMipmap");

    return glIsRenderbuffer != 0;
#else
    return true;
#endif
}

#if !defined(QT_OPENGL_ES_2)
bool qt_resolve_version_1_3_functions(QGLContext *ctx)
{
    if (glMultiTexCoord4f != 0)
        return true;

    QGLContext cx(QGLFormat::defaultFormat());
    glMultiTexCoord4f = (_glMultiTexCoord4f) ctx->getProcAddress(QLatin1String("glMultiTexCoord4f"));

    glActiveTexture = (_glActiveTexture) ctx->getProcAddress(QLatin1String("glActiveTexture"));
    return glMultiTexCoord4f && glActiveTexture;
}
#endif

#if !defined(QT_OPENGL_ES_2)
bool qt_resolve_stencil_face_extension(QGLContext *ctx)
{
    if (glActiveStencilFaceEXT != 0)
        return true;

    QGLContext cx(QGLFormat::defaultFormat());
    glActiveStencilFaceEXT = (_glActiveStencilFaceEXT) ctx->getProcAddress(QLatin1String("glActiveStencilFaceEXT"));

    return glActiveStencilFaceEXT;
}
#endif


#if !defined(QT_OPENGL_ES_2)
bool qt_resolve_frag_program_extensions(QGLContext *ctx)
{
    if (glProgramStringARB != 0)
        return true;

    // ARB_fragment_program
    glProgramStringARB = (_glProgramStringARB) ctx->getProcAddress(QLatin1String("glProgramStringARB"));
    glBindProgramARB = (_glBindProgramARB) ctx->getProcAddress(QLatin1String("glBindProgramARB"));
    glDeleteProgramsARB = (_glDeleteProgramsARB) ctx->getProcAddress(QLatin1String("glDeleteProgramsARB"));
    glGenProgramsARB = (_glGenProgramsARB) ctx->getProcAddress(QLatin1String("glGenProgramsARB"));
    glProgramLocalParameter4fvARB = (_glProgramLocalParameter4fvARB) ctx->getProcAddress(QLatin1String("glProgramLocalParameter4fvARB"));

    return glProgramStringARB
        && glBindProgramARB
        && glDeleteProgramsARB
        && glGenProgramsARB
        && glProgramLocalParameter4fvARB;
}
#endif


bool qt_resolve_buffer_extensions(QGLContext *ctx)
{
#if defined(QGL_RESOLVE_BUFFER_FUNCS)
    if (glBindBuffer && glDeleteBuffers && glGenBuffers && glBufferData
            && glBufferSubData && glGetBufferParameteriv)
        return true;
#endif

#if defined(QGL_RESOLVE_BUFFER_FUNCS)
    glBindBuffer = (_glBindBuffer) qt_gl_getProcAddressARB(ctx, "glBindBuffer");
    glDeleteBuffers = (_glDeleteBuffers) qt_gl_getProcAddressARB(ctx, "glDeleteBuffers");
    glGenBuffers = (_glGenBuffers) qt_gl_getProcAddressARB(ctx, "glGenBuffers");
    glBufferData = (_glBufferData) qt_gl_getProcAddressARB(ctx, "glBufferData");
    glBufferSubData = (_glBufferSubData) qt_gl_getProcAddressARB(ctx, "glBufferSubData");
    glGetBufferSubData = (_glGetBufferSubData) qt_gl_getProcAddressARB(ctx, "glGetBufferSubData");
    glGetBufferParameteriv = (_glGetBufferParameteriv) qt_gl_getProcAddressARB(ctx, "glGetBufferParameteriv");
#endif
    glMapBufferARB = (_glMapBufferARB) qt_gl_getProcAddressARB(ctx, "glMapBuffer");
    glUnmapBufferARB = (_glUnmapBufferARB) qt_gl_getProcAddressARB(ctx, "glUnmapBuffer");

#if defined(QGL_RESOLVE_BUFFER_FUNCS)
    return glBindBuffer
        && glDeleteBuffers
        && glGenBuffers
        && glBufferData
        && glBufferSubData
        && glGetBufferParameteriv;
        // glGetBufferSubData() is optional
#else
    return true;
#endif
}

bool qt_resolve_glsl_extensions(QGLContext *ctx)
{

#if defined(QT_OPENGL_ES_2)
    // The GLSL shader functions are always present in OpenGL/ES 2.0.
    // The only exceptions are glGetProgramBinaryOES and glProgramBinaryOES.
    if (!QGLContextPrivate::extensionFuncs(ctx).qt_glslResolved) {
        glGetProgramBinaryOES = (_glGetProgramBinaryOES) ctx->getProcAddress(QLatin1String("glGetProgramBinaryOES"));
        glProgramBinaryOES = (_glProgramBinaryOES) ctx->getProcAddress(QLatin1String("glProgramBinaryOES"));
        QGLContextPrivate::extensionFuncs(ctx).qt_glslResolved = true;
    }
    return true;
#else
    if (glCreateShader)
        return true;

    // Geometry shaders are optional...
    glProgramParameteriEXT = (_glProgramParameteriEXT) ctx->getProcAddress(QLatin1String("glProgramParameteriEXT"));
    glFramebufferTextureEXT = (_glFramebufferTextureEXT) ctx->getProcAddress(QLatin1String("glFramebufferTextureEXT"));
    glFramebufferTextureLayerEXT = (_glFramebufferTextureLayerEXT) ctx->getProcAddress(QLatin1String("glFramebufferTextureLayerEXT"));
    glFramebufferTextureFaceEXT = (_glFramebufferTextureFaceEXT) ctx->getProcAddress(QLatin1String("glFramebufferTextureFaceEXT"));

    // Must at least have the FragmentShader extension to continue.
    if (!(QGLExtensions::glExtensions() & QGLExtensions::FragmentShader))
        return false;

    glCreateShader = (_glCreateShader) ctx->getProcAddress(QLatin1String("glCreateShader"));
    if (glCreateShader) {
        glShaderSource = (_glShaderSource) ctx->getProcAddress(QLatin1String("glShaderSource"));
        glShaderBinary = (_glShaderBinary) ctx->getProcAddress(QLatin1String("glShaderBinary"));
        glCompileShader = (_glCompileShader) ctx->getProcAddress(QLatin1String("glCompileShader"));
        glDeleteShader = (_glDeleteShader) ctx->getProcAddress(QLatin1String("glDeleteShader"));
        glIsShader = (_glIsShader) ctx->getProcAddress(QLatin1String("glIsShader"));

        glCreateProgram = (_glCreateProgram) ctx->getProcAddress(QLatin1String("glCreateProgram"));
        glAttachShader = (_glAttachShader) ctx->getProcAddress(QLatin1String("glAttachShader"));
        glDetachShader = (_glDetachShader) ctx->getProcAddress(QLatin1String("glDetachShader"));
        glLinkProgram = (_glLinkProgram) ctx->getProcAddress(QLatin1String("glLinkProgram"));
        glUseProgram = (_glUseProgram) ctx->getProcAddress(QLatin1String("glUseProgram"));
        glDeleteProgram = (_glDeleteProgram) ctx->getProcAddress(QLatin1String("glDeleteProgram"));
        glIsProgram = (_glIsProgram) ctx->getProcAddress(QLatin1String("glIsProgram"));

        glGetShaderInfoLog = (_glGetShaderInfoLog) ctx->getProcAddress(QLatin1String("glGetShaderInfoLog"));
        glGetShaderiv = (_glGetShaderiv) ctx->getProcAddress(QLatin1String("glGetShaderiv"));
        glGetShaderSource = (_glGetShaderSource) ctx->getProcAddress(QLatin1String("glGetShaderSource"));
        glGetProgramiv = (_glGetProgramiv) ctx->getProcAddress(QLatin1String("glGetProgramiv"));
        glGetProgramInfoLog = (_glGetProgramInfoLog) ctx->getProcAddress(QLatin1String("glGetProgramInfoLog"));

        glGetUniformLocation = (_glGetUniformLocation) ctx->getProcAddress(QLatin1String("glGetUniformLocation"));
        glUniform4fv = (_glUniform4fv) ctx->getProcAddress(QLatin1String("glUniform4fv"));
        glUniform3fv = (_glUniform3fv) ctx->getProcAddress(QLatin1String("glUniform3fv"));
        glUniform2fv = (_glUniform2fv) ctx->getProcAddress(QLatin1String("glUniform2fv"));
        glUniform1fv = (_glUniform1fv) ctx->getProcAddress(QLatin1String("glUniform1fv"));
        glUniform1i = (_glUniform1i) ctx->getProcAddress(QLatin1String("glUniform1i"));
        glUniform1iv = (_glUniform1iv) ctx->getProcAddress(QLatin1String("glUniform1iv"));
        glUniformMatrix2fv = (_glUniformMatrix2fv) ctx->getProcAddress(QLatin1String("glUniformMatrix2fv"));
        glUniformMatrix3fv = (_glUniformMatrix3fv) ctx->getProcAddress(QLatin1String("glUniformMatrix3fv"));
        glUniformMatrix4fv = (_glUniformMatrix4fv) ctx->getProcAddress(QLatin1String("glUniformMatrix4fv"));
        glUniformMatrix2x3fv = (_glUniformMatrix2x3fv) ctx->getProcAddress(QLatin1String("glUniformMatrix2x3fv"));
        glUniformMatrix2x4fv = (_glUniformMatrix2x4fv) ctx->getProcAddress(QLatin1String("glUniformMatrix2x4fv"));
        glUniformMatrix3x2fv = (_glUniformMatrix3x2fv) ctx->getProcAddress(QLatin1String("glUniformMatrix3x2fv"));
        glUniformMatrix3x4fv = (_glUniformMatrix3x4fv) ctx->getProcAddress(QLatin1String("glUniformMatrix3x4fv"));
        glUniformMatrix4x2fv = (_glUniformMatrix4x2fv) ctx->getProcAddress(QLatin1String("glUniformMatrix4x2fv"));
        glUniformMatrix4x3fv = (_glUniformMatrix4x3fv) ctx->getProcAddress(QLatin1String("glUniformMatrix4x3fv"));

        glBindAttribLocation = (_glBindAttribLocation) ctx->getProcAddress(QLatin1String("glBindAttribLocation"));
        glGetAttribLocation = (_glGetAttribLocation) ctx->getProcAddress(QLatin1String("glGetAttribLocation"));
        glVertexAttrib1fv = (_glVertexAttrib1fv) ctx->getProcAddress(QLatin1String("glVertexAttrib1fv"));
        glVertexAttrib2fv = (_glVertexAttrib2fv) ctx->getProcAddress(QLatin1String("glVertexAttrib2fv"));
        glVertexAttrib3fv = (_glVertexAttrib3fv) ctx->getProcAddress(QLatin1String("glVertexAttrib3fv"));
        glVertexAttrib4fv = (_glVertexAttrib4fv) ctx->getProcAddress(QLatin1String("glVertexAttrib4fv"));
        glVertexAttribPointer = (_glVertexAttribPointer) ctx->getProcAddress(QLatin1String("glVertexAttribPointer"));
        glDisableVertexAttribArray = (_glDisableVertexAttribArray) ctx->getProcAddress(QLatin1String("glDisableVertexAttribArray"));
        glEnableVertexAttribArray = (_glEnableVertexAttribArray) ctx->getProcAddress(QLatin1String("glEnableVertexAttribArray"));

    } else {
        // We may not have the standard shader functions, but we might
        // have the older ARB functions instead.
        glCreateShader = (_glCreateShader) ctx->getProcAddress(QLatin1String("glCreateShaderObjectARB"));
        glShaderSource = (_glShaderSource) ctx->getProcAddress(QLatin1String("glShaderSourceARB"));
        glShaderBinary = (_glShaderBinary) ctx->getProcAddress(QLatin1String("glShaderBinaryARB"));
        glCompileShader = (_glCompileShader) ctx->getProcAddress(QLatin1String("glCompileShaderARB"));
        glDeleteShader = (_glDeleteShader) ctx->getProcAddress(QLatin1String("glDeleteObjectARB"));
        glIsShader = 0;

        glCreateProgram = (_glCreateProgram) ctx->getProcAddress(QLatin1String("glCreateProgramObjectARB"));
        glAttachShader = (_glAttachShader) ctx->getProcAddress(QLatin1String("glAttachObjectARB"));
        glDetachShader = (_glDetachShader) ctx->getProcAddress(QLatin1String("glDetachObjectARB"));
        glLinkProgram = (_glLinkProgram) ctx->getProcAddress(QLatin1String("glLinkProgramARB"));
        glUseProgram = (_glUseProgram) ctx->getProcAddress(QLatin1String("glUseProgramObjectARB"));
        glDeleteProgram = (_glDeleteProgram) ctx->getProcAddress(QLatin1String("glDeleteObjectARB"));
        glIsProgram = 0;

        glGetShaderInfoLog = (_glGetShaderInfoLog) ctx->getProcAddress(QLatin1String("glGetInfoLogARB"));
        glGetShaderiv = (_glGetShaderiv) ctx->getProcAddress(QLatin1String("glGetObjectParameterivARB"));
        glGetShaderSource = (_glGetShaderSource) ctx->getProcAddress(QLatin1String("glGetShaderSourceARB"));
        glGetProgramiv = (_glGetProgramiv) ctx->getProcAddress(QLatin1String("glGetObjectParameterivARB"));
        glGetProgramInfoLog = (_glGetProgramInfoLog) ctx->getProcAddress(QLatin1String("glGetInfoLogARB"));

        glGetUniformLocation = (_glGetUniformLocation) ctx->getProcAddress(QLatin1String("glGetUniformLocationARB"));
        glUniform4fv = (_glUniform4fv) ctx->getProcAddress(QLatin1String("glUniform4fvARB"));
        glUniform3fv = (_glUniform3fv) ctx->getProcAddress(QLatin1String("glUniform3fvARB"));
        glUniform2fv = (_glUniform2fv) ctx->getProcAddress(QLatin1String("glUniform2fvARB"));
        glUniform1fv = (_glUniform1fv) ctx->getProcAddress(QLatin1String("glUniform1fvARB"));
        glUniform1i = (_glUniform1i) ctx->getProcAddress(QLatin1String("glUniform1iARB"));
        glUniform1iv = (_glUniform1iv) ctx->getProcAddress(QLatin1String("glUniform1ivARB"));
        glUniformMatrix2fv = (_glUniformMatrix2fv) ctx->getProcAddress(QLatin1String("glUniformMatrix2fvARB"));
        glUniformMatrix3fv = (_glUniformMatrix3fv) ctx->getProcAddress(QLatin1String("glUniformMatrix3fvARB"));
        glUniformMatrix4fv = (_glUniformMatrix4fv) ctx->getProcAddress(QLatin1String("glUniformMatrix4fvARB"));
        glUniformMatrix2x3fv = (_glUniformMatrix2x3fv) ctx->getProcAddress(QLatin1String("glUniformMatrix2x3fvARB"));
        glUniformMatrix2x4fv = (_glUniformMatrix2x4fv) ctx->getProcAddress(QLatin1String("glUniformMatrix2x4fvARB"));
        glUniformMatrix3x2fv = (_glUniformMatrix3x2fv) ctx->getProcAddress(QLatin1String("glUniformMatrix3x2fvARB"));
        glUniformMatrix3x4fv = (_glUniformMatrix3x4fv) ctx->getProcAddress(QLatin1String("glUniformMatrix3x4fvARB"));
        glUniformMatrix4x2fv = (_glUniformMatrix4x2fv) ctx->getProcAddress(QLatin1String("glUniformMatrix4x2fvARB"));
        glUniformMatrix4x3fv = (_glUniformMatrix4x3fv) ctx->getProcAddress(QLatin1String("glUniformMatrix4x3fvARB"));

        glBindAttribLocation = (_glBindAttribLocation) ctx->getProcAddress(QLatin1String("glBindAttribLocationARB"));
        glGetAttribLocation = (_glGetAttribLocation) ctx->getProcAddress(QLatin1String("glGetAttribLocationARB"));
        glVertexAttrib1fv = (_glVertexAttrib1fv) ctx->getProcAddress(QLatin1String("glVertexAttrib1fvARB"));
        glVertexAttrib2fv = (_glVertexAttrib2fv) ctx->getProcAddress(QLatin1String("glVertexAttrib2fvARB"));
        glVertexAttrib3fv = (_glVertexAttrib3fv) ctx->getProcAddress(QLatin1String("glVertexAttrib3fvARB"));
        glVertexAttrib4fv = (_glVertexAttrib4fv) ctx->getProcAddress(QLatin1String("glVertexAttrib4fvARB"));
        glVertexAttribPointer = (_glVertexAttribPointer) ctx->getProcAddress(QLatin1String("glVertexAttribPointerARB"));
        glDisableVertexAttribArray = (_glDisableVertexAttribArray) ctx->getProcAddress(QLatin1String("glDisableVertexAttribArrayARB"));
        glEnableVertexAttribArray = (_glEnableVertexAttribArray) ctx->getProcAddress(QLatin1String("glEnableVertexAttribArrayARB"));
    }

    // Note: glShaderBinary(), glIsShader(), glIsProgram(), and
    // glUniformMatrixNxMfv() are optional, but all other functions
    // are required.

    return glCreateShader &&
        glShaderSource &&
        glCompileShader &&
        glDeleteProgram &&
        glCreateProgram &&
        glAttachShader &&
        glDetachShader &&
        glLinkProgram &&
        glUseProgram &&
        glGetShaderInfoLog &&
        glGetShaderiv &&
        glGetShaderSource &&
        glGetProgramiv &&
        glGetProgramInfoLog &&
        glGetUniformLocation &&
        glUniform1fv &&
        glUniform2fv &&
        glUniform3fv &&
        glUniform4fv &&
        glUniform1i &&
        glUniform1iv &&
        glUniformMatrix2fv &&
        glUniformMatrix3fv &&
        glUniformMatrix4fv &&
        glBindAttribLocation &&
        glGetAttribLocation &&
        glVertexAttrib1fv &&
        glVertexAttrib2fv &&
        glVertexAttrib3fv &&
        glVertexAttrib4fv &&
        glVertexAttribPointer &&
        glDisableVertexAttribArray &&
        glEnableVertexAttribArray;
#endif
}

#if !defined(QT_OPENGL_ES_2)
bool qt_resolve_version_2_0_functions(QGLContext *ctx)
{
    bool gl2supported = true;
    if (!qt_resolve_glsl_extensions(ctx))
        gl2supported = false;

    if (!qt_resolve_version_1_3_functions(ctx))
        gl2supported = false;

    if (glStencilOpSeparate)
        return gl2supported;

    glBlendColor = (_glBlendColor) ctx->getProcAddress(QLatin1String("glBlendColor"));
    glStencilOpSeparate = (_glStencilOpSeparate) ctx->getProcAddress(QLatin1String("glStencilOpSeparate"));
    if (!glBlendColor || !glStencilOpSeparate)
        gl2supported = false;

    return gl2supported;
}
#endif


QT_END_NAMESPACE
