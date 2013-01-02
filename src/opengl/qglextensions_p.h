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

#ifndef QGL_EXTENSIONS_P_H
#define QGL_EXTENSIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt OpenGL classes.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

// extension prototypes
# ifndef APIENTRYP
#   ifdef APIENTRY
#     define APIENTRYP APIENTRY *
#   else
#     define APIENTRY
#     define APIENTRYP *
#   endif
# endif

#include <QtCore/qglobal.h>

#ifndef GL_ARB_vertex_buffer_object
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
#endif

#ifndef GL_VERSION_2_0
typedef char GLchar;
#endif

// ARB_vertex_buffer_object
typedef void (APIENTRY *_glBindBuffer) (GLenum, GLuint);
typedef void (APIENTRY *_glDeleteBuffers) (GLsizei, const GLuint *);
typedef void (APIENTRY *_glGenBuffers) (GLsizei, GLuint *);
typedef void (APIENTRY *_glBufferData) (GLenum, GLsizeiptrARB, const GLvoid *, GLenum);
typedef void (APIENTRY *_glBufferSubData) (GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *);
typedef void (APIENTRY *_glGetBufferSubData) (GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *);
typedef void (APIENTRY *_glGetBufferParameteriv) (GLenum, GLenum, GLint *);
typedef GLvoid* (APIENTRY *_glMapBufferARB) (GLenum, GLenum);
typedef GLboolean (APIENTRY *_glUnmapBufferARB) (GLenum);
// We can call the buffer functions directly in OpenGL/ES 1.1 or higher,
// but all other platforms need to resolve the extensions.
#if defined(QT_OPENGL_ES)
#if defined(GL_OES_VERSION_1_0) && !defined(GL_OES_VERSION_1_1)
#define QGL_RESOLVE_BUFFER_FUNCS 1
#endif
#else
#define QGL_RESOLVE_BUFFER_FUNCS 1
#endif

// ARB_fragment_program
typedef void (APIENTRY *_glProgramStringARB) (GLenum, GLenum, GLsizei, const GLvoid *);
typedef void (APIENTRY *_glBindProgramARB) (GLenum, GLuint);
typedef void (APIENTRY *_glDeleteProgramsARB) (GLsizei, const GLuint *);
typedef void (APIENTRY *_glGenProgramsARB) (GLsizei, GLuint *);
typedef void (APIENTRY *_glProgramLocalParameter4fvARB) (GLenum, GLuint, const GLfloat *);

// GLSL
typedef GLuint (APIENTRY *_glCreateShader) (GLenum);
typedef void (APIENTRY *_glShaderSource) (GLuint, GLsizei, const char **, const GLint *);
typedef void (APIENTRY *_glShaderBinary) (GLint, const GLuint*, GLenum, const void*, GLint);
typedef void (APIENTRY *_glCompileShader) (GLuint);
typedef void (APIENTRY *_glDeleteShader) (GLuint);
typedef GLboolean (APIENTRY *_glIsShader) (GLuint);

typedef GLuint (APIENTRY *_glCreateProgram) ();
typedef void (APIENTRY *_glAttachShader) (GLuint, GLuint);
typedef void (APIENTRY *_glDetachShader) (GLuint, GLuint);
typedef void (APIENTRY *_glLinkProgram) (GLuint);
typedef void (APIENTRY *_glUseProgram) (GLuint);
typedef void (APIENTRY *_glDeleteProgram) (GLuint);
typedef GLboolean (APIENTRY *_glIsProgram) (GLuint);

typedef void (APIENTRY *_glGetShaderInfoLog) (GLuint, GLsizei, GLsizei *, char *);
typedef void (APIENTRY *_glGetShaderiv) (GLuint, GLenum, GLint *);
typedef void (APIENTRY *_glGetShaderSource) (GLuint, GLsizei, GLsizei *, char *);
typedef void (APIENTRY *_glGetProgramiv) (GLuint, GLenum, GLint *);
typedef void (APIENTRY *_glGetProgramInfoLog) (GLuint, GLsizei, GLsizei *, char *);

typedef GLuint (APIENTRY *_glGetUniformLocation) (GLuint, const char*);
typedef void (APIENTRY *_glUniform4fv) (GLint, GLsizei, const GLfloat *);
typedef void (APIENTRY *_glUniform3fv) (GLint, GLsizei, const GLfloat *);
typedef void (APIENTRY *_glUniform2fv) (GLint, GLsizei, const GLfloat *);
typedef void (APIENTRY *_glUniform1fv) (GLint, GLsizei, const GLfloat *);
typedef void (APIENTRY *_glUniform1i) (GLint, GLint);
typedef void (APIENTRY *_glUniform1iv) (GLint, GLsizei, const GLint *);
typedef void (APIENTRY *_glUniformMatrix2fv) (GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY *_glUniformMatrix3fv) (GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY *_glUniformMatrix4fv) (GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY *_glUniformMatrix2x3fv) (GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY *_glUniformMatrix2x4fv) (GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY *_glUniformMatrix3x2fv) (GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY *_glUniformMatrix3x4fv) (GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY *_glUniformMatrix4x2fv) (GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY *_glUniformMatrix4x3fv) (GLint, GLsizei, GLboolean, const GLfloat *);

typedef void (APIENTRY *_glBindAttribLocation) (GLuint, GLuint, const char *);
typedef GLint (APIENTRY *_glGetAttribLocation) (GLuint, const char *);
typedef void (APIENTRY *_glVertexAttrib1fv) (GLuint, const GLfloat *);
typedef void (APIENTRY *_glVertexAttrib2fv) (GLuint, const GLfloat *);
typedef void (APIENTRY *_glVertexAttrib3fv) (GLuint, const GLfloat *);
typedef void (APIENTRY *_glVertexAttrib4fv) (GLuint, const GLfloat *);
typedef void (APIENTRY *_glVertexAttribPointer) (GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
typedef void (APIENTRY *_glDisableVertexAttribArray) (GLuint);
typedef void (APIENTRY *_glEnableVertexAttribArray) (GLuint);

typedef void (APIENTRY *_glGetProgramBinaryOES) (GLuint, GLsizei, GLsizei *, GLenum *, void *);
typedef void (APIENTRY *_glProgramBinaryOES) (GLuint, GLenum, const void *, GLint);


typedef void (APIENTRY *_glMultiTexCoord4f) (GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *_glActiveStencilFaceEXT) (GLenum );

// Needed for GL2 engine:
typedef void (APIENTRY *_glStencilOpSeparate) (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
typedef void (APIENTRY *_glActiveTexture) (GLenum);
typedef void (APIENTRY *_glBlendColor) (GLclampf, GLclampf, GLclampf, GLclampf);


// EXT_GL_framebuffer_object
typedef GLboolean (APIENTRY *_glIsRenderbuffer) (GLuint renderbuffer);
typedef void (APIENTRY *_glBindRenderbuffer) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRY *_glDeleteRenderbuffers) (GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY *_glGenRenderbuffers) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY *_glRenderbufferStorage) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY *_glGetRenderbufferParameteriv) (GLenum target, GLenum pname, GLint *params);
typedef GLboolean (APIENTRY *_glIsFramebuffer) (GLuint framebuffer);
typedef void (APIENTRY *_glBindFramebuffer) (GLenum target, GLuint framebuffer);
typedef void (APIENTRY *_glDeleteFramebuffers) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *_glGenFramebuffers) (GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRY *_glCheckFramebufferStatus) (GLenum target);
typedef void (APIENTRY *_glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget,
                                                           GLuint texture, GLint level);
typedef void (APIENTRY *_glFramebufferRenderbuffer) (GLenum target, GLenum attachment, GLenum renderbuffertarget,
                                                              GLuint renderbuffer);
typedef void (APIENTRY *_glGetFramebufferAttachmentParameteriv) (GLenum target, GLenum attachment, GLenum pname,
                                                                          GLint *params);
typedef void (APIENTRY *_glGenerateMipmap) (GLenum target);

// EXT_GL_framebuffer_blit
typedef void (APIENTRY *_glBlitFramebufferEXT) (int srcX0, int srcY0, int srcX1, int srcY1,
                                                int dstX0, int dstY0, int dstX1, int dstY1,
                                                GLbitfield mask, GLenum filter);

// EXT_GL_framebuffer_multisample
typedef void (APIENTRY *_glRenderbufferStorageMultisampleEXT) (GLenum target, GLsizei samples,
                                                               GLenum internalformat, GLsizei width, GLsizei height);

// GL_EXT_geometry_shader4
typedef void (APIENTRY *_glProgramParameteriEXT)(GLuint program, GLenum pname, GLint value);
typedef void (APIENTRY *_glFramebufferTextureEXT)(GLenum target, GLenum attachment,
                                                  GLuint texture, GLint level);
typedef void (APIENTRY *_glFramebufferTextureLayerEXT)(GLenum target, GLenum attachment,
                                                       GLuint texture, GLint level, GLint layer);
typedef void (APIENTRY *_glFramebufferTextureFaceEXT)(GLenum target, GLenum attachment,
                                                      GLuint texture, GLint level, GLenum face);

// ARB_texture_compression
typedef void (APIENTRY *_glCompressedTexImage2DARB) (GLenum, GLint, GLenum, GLsizei,
                                                     GLsizei, GLint, GLsizei, const GLvoid *);
QT_BEGIN_NAMESPACE

struct QGLExtensionFuncs
{
    QGLExtensionFuncs() {
#if !defined(QT_OPENGL_ES_2)
        qt_glProgramStringARB = 0;
        qt_glBindProgramARB = 0;
        qt_glDeleteProgramsARB = 0;
        qt_glGenProgramsARB = 0;
        qt_glProgramLocalParameter4fvARB = 0;

        // GLSL
        qt_glCreateShader = 0;
        qt_glShaderSource = 0;
        qt_glShaderBinary = 0;
        qt_glCompileShader = 0;
        qt_glDeleteShader = 0;
        qt_glIsShader = 0;

        qt_glCreateProgram = 0;
        qt_glAttachShader = 0;
        qt_glDetachShader = 0;
        qt_glLinkProgram = 0;
        qt_glUseProgram = 0;
        qt_glDeleteProgram = 0;
        qt_glIsProgram = 0;

        qt_glGetShaderInfoLog = 0;
        qt_glGetShaderiv = 0;
        qt_glGetShaderSource = 0;
        qt_glGetProgramiv = 0;
        qt_glGetProgramInfoLog = 0;

        qt_glGetUniformLocation = 0;
        qt_glUniform4fv = 0;
        qt_glUniform3fv = 0;
        qt_glUniform2fv = 0;
        qt_glUniform1fv = 0;
        qt_glUniform1i = 0;
        qt_glUniform1iv = 0;
        qt_glUniformMatrix2fv = 0;
        qt_glUniformMatrix3fv = 0;
        qt_glUniformMatrix4fv = 0;
        qt_glUniformMatrix2x3fv = 0;
        qt_glUniformMatrix2x4fv = 0;
        qt_glUniformMatrix3x2fv = 0;
        qt_glUniformMatrix3x4fv = 0;
        qt_glUniformMatrix4x2fv = 0;
        qt_glUniformMatrix4x3fv = 0;

        qt_glBindAttribLocation = 0;
        qt_glGetAttribLocation = 0;
        qt_glVertexAttrib1fv = 0;
        qt_glVertexAttrib2fv = 0;
        qt_glVertexAttrib3fv = 0;
        qt_glVertexAttrib4fv = 0;
        qt_glVertexAttribPointer = 0;
        qt_glDisableVertexAttribArray = 0;
        qt_glEnableVertexAttribArray = 0;

        // Extras for GL2 engine:
        qt_glActiveTexture = 0;
        qt_glStencilOpSeparate = 0;
        qt_glBlendColor = 0;

        qt_glActiveStencilFaceEXT = 0;
        qt_glMultiTexCoord4f = 0;
#else
        qt_glslResolved = false;

        qt_glGetProgramBinaryOES = 0;
        qt_glProgramBinaryOES = 0;
#endif

        // FBOs
#if !defined(QT_OPENGL_ES_2)
        qt_glIsRenderbuffer = 0;
        qt_glBindRenderbuffer = 0;
        qt_glDeleteRenderbuffers = 0;
        qt_glGenRenderbuffers = 0;
        qt_glRenderbufferStorage = 0;
        qt_glGetRenderbufferParameteriv = 0;
        qt_glIsFramebuffer = 0;
        qt_glBindFramebuffer = 0;
        qt_glDeleteFramebuffers = 0;
        qt_glGenFramebuffers = 0;
        qt_glCheckFramebufferStatus = 0;
        qt_glFramebufferTexture2D = 0;
        qt_glFramebufferRenderbuffer = 0;
        qt_glGetFramebufferAttachmentParameteriv = 0;
        qt_glGenerateMipmap = 0;
#endif
        qt_glBlitFramebufferEXT = 0;
        qt_glRenderbufferStorageMultisampleEXT = 0;

        // Buffer objects:
#if defined(QGL_RESOLVE_BUFFER_FUNCS)
        qt_glBindBuffer = 0;
        qt_glDeleteBuffers = 0;
        qt_glGenBuffers = 0;
        qt_glBufferData = 0;
        qt_glBufferSubData = 0;
        qt_glGetBufferSubData = 0;
        qt_glGetBufferParameteriv = 0;
#endif
        qt_glMapBufferARB = 0;
        qt_glUnmapBufferARB = 0;

        qt_glProgramParameteriEXT = 0;
        qt_glFramebufferTextureEXT = 0;
        qt_glFramebufferTextureLayerEXT = 0;
        qt_glFramebufferTextureFaceEXT = 0;
#if !defined(QT_OPENGL_ES)
        // Texture compression
        qt_glCompressedTexImage2DARB = 0;
#endif
    }


#if !defined(QT_OPENGL_ES_2)
    _glProgramStringARB qt_glProgramStringARB;
    _glBindProgramARB qt_glBindProgramARB;
    _glDeleteProgramsARB qt_glDeleteProgramsARB;
    _glGenProgramsARB qt_glGenProgramsARB;
    _glProgramLocalParameter4fvARB qt_glProgramLocalParameter4fvARB;

    // GLSL definitions
    _glCreateShader qt_glCreateShader;
    _glShaderSource qt_glShaderSource;
    _glShaderBinary qt_glShaderBinary;
    _glCompileShader qt_glCompileShader;
    _glDeleteShader qt_glDeleteShader;
    _glIsShader qt_glIsShader;

    _glCreateProgram qt_glCreateProgram;
    _glAttachShader qt_glAttachShader;
    _glDetachShader qt_glDetachShader;
    _glLinkProgram qt_glLinkProgram;
    _glUseProgram qt_glUseProgram;
    _glDeleteProgram qt_glDeleteProgram;
    _glIsProgram qt_glIsProgram;

    _glGetShaderInfoLog qt_glGetShaderInfoLog;
    _glGetShaderiv qt_glGetShaderiv;
    _glGetShaderSource qt_glGetShaderSource;
    _glGetProgramiv qt_glGetProgramiv;
    _glGetProgramInfoLog qt_glGetProgramInfoLog;

    _glGetUniformLocation qt_glGetUniformLocation;
    _glUniform4fv qt_glUniform4fv;
    _glUniform3fv qt_glUniform3fv;
    _glUniform2fv qt_glUniform2fv;
    _glUniform1fv qt_glUniform1fv;
    _glUniform1i qt_glUniform1i;
    _glUniform1iv qt_glUniform1iv;
    _glUniformMatrix2fv qt_glUniformMatrix2fv;
    _glUniformMatrix3fv qt_glUniformMatrix3fv;
    _glUniformMatrix4fv qt_glUniformMatrix4fv;
    _glUniformMatrix2x3fv qt_glUniformMatrix2x3fv;
    _glUniformMatrix2x4fv qt_glUniformMatrix2x4fv;
    _glUniformMatrix3x2fv qt_glUniformMatrix3x2fv;
    _glUniformMatrix3x4fv qt_glUniformMatrix3x4fv;
    _glUniformMatrix4x2fv qt_glUniformMatrix4x2fv;
    _glUniformMatrix4x3fv qt_glUniformMatrix4x3fv;

    _glBindAttribLocation qt_glBindAttribLocation;
    _glGetAttribLocation qt_glGetAttribLocation;
    _glVertexAttrib1fv qt_glVertexAttrib1fv;
    _glVertexAttrib2fv qt_glVertexAttrib2fv;
    _glVertexAttrib3fv qt_glVertexAttrib3fv;
    _glVertexAttrib4fv qt_glVertexAttrib4fv;
    _glVertexAttribPointer qt_glVertexAttribPointer;
    _glDisableVertexAttribArray qt_glDisableVertexAttribArray;
    _glEnableVertexAttribArray qt_glEnableVertexAttribArray;

#else
    bool qt_glslResolved;

    _glGetProgramBinaryOES qt_glGetProgramBinaryOES;
    _glProgramBinaryOES qt_glProgramBinaryOES;
#endif

    _glActiveStencilFaceEXT qt_glActiveStencilFaceEXT;
    _glMultiTexCoord4f qt_glMultiTexCoord4f;

#if !defined(QT_OPENGL_ES_2)
    // Extras needed for GL2 engine:
    _glActiveTexture qt_glActiveTexture;
    _glStencilOpSeparate qt_glStencilOpSeparate;
    _glBlendColor qt_glBlendColor;

#endif

    // FBOs
#if !defined(QT_OPENGL_ES_2)
    _glIsRenderbuffer qt_glIsRenderbuffer;
    _glBindRenderbuffer qt_glBindRenderbuffer;
    _glDeleteRenderbuffers qt_glDeleteRenderbuffers;
    _glGenRenderbuffers qt_glGenRenderbuffers;
    _glRenderbufferStorage qt_glRenderbufferStorage;
    _glGetRenderbufferParameteriv qt_glGetRenderbufferParameteriv;
    _glIsFramebuffer qt_glIsFramebuffer;
    _glBindFramebuffer qt_glBindFramebuffer;
    _glDeleteFramebuffers qt_glDeleteFramebuffers;
    _glGenFramebuffers qt_glGenFramebuffers;
    _glCheckFramebufferStatus qt_glCheckFramebufferStatus;
    _glFramebufferTexture2D qt_glFramebufferTexture2D;
    _glFramebufferRenderbuffer qt_glFramebufferRenderbuffer;
    _glGetFramebufferAttachmentParameteriv qt_glGetFramebufferAttachmentParameteriv;
    _glGenerateMipmap qt_glGenerateMipmap;
#endif
    _glBlitFramebufferEXT qt_glBlitFramebufferEXT;
    _glRenderbufferStorageMultisampleEXT qt_glRenderbufferStorageMultisampleEXT;

    // Buffer objects
#if defined(QGL_RESOLVE_BUFFER_FUNCS)
    _glBindBuffer qt_glBindBuffer;
    _glDeleteBuffers qt_glDeleteBuffers;
    _glGenBuffers qt_glGenBuffers;
    _glBufferData qt_glBufferData;
    _glBufferSubData qt_glBufferSubData;
    _glGetBufferSubData qt_glGetBufferSubData;
    _glGetBufferParameteriv qt_glGetBufferParameteriv;
#endif
    _glMapBufferARB qt_glMapBufferARB;
    _glUnmapBufferARB qt_glUnmapBufferARB;

    // Geometry shaders...
    _glProgramParameteriEXT qt_glProgramParameteriEXT;
    _glFramebufferTextureEXT qt_glFramebufferTextureEXT;
    _glFramebufferTextureLayerEXT qt_glFramebufferTextureLayerEXT;
    _glFramebufferTextureFaceEXT qt_glFramebufferTextureFaceEXT;
#if !defined(QT_OPENGL_ES)
    // Texture compression
    _glCompressedTexImage2DARB qt_glCompressedTexImage2DARB;
#endif
};


#if !defined(QT_OPENGL_ES_2)
#define glProgramStringARB QGLContextPrivate::extensionFuncs(ctx).qt_glProgramStringARB
#define glBindProgramARB QGLContextPrivate::extensionFuncs(ctx).qt_glBindProgramARB
#define glDeleteProgramsARB QGLContextPrivate::extensionFuncs(ctx).qt_glDeleteProgramsARB
#define glGenProgramsARB QGLContextPrivate::extensionFuncs(ctx).qt_glGenProgramsARB
#define glProgramLocalParameter4fvARB QGLContextPrivate::extensionFuncs(ctx).qt_glProgramLocalParameter4fvARB

#define glActiveStencilFaceEXT QGLContextPrivate::extensionFuncs(ctx).qt_glActiveStencilFaceEXT

#define glMultiTexCoord4f QGLContextPrivate::extensionFuncs(ctx).qt_glMultiTexCoord4f

#define glActiveTexture QGLContextPrivate::extensionFuncs(ctx).qt_glActiveTexture
#endif // !defined(QT_OPENGL_ES_2)


// FBOs
#if !defined(QT_OPENGL_ES_2)
#define glIsRenderbuffer QGLContextPrivate::extensionFuncs(ctx).qt_glIsRenderbuffer
#define glBindRenderbuffer QGLContextPrivate::extensionFuncs(ctx).qt_glBindRenderbuffer
#define glDeleteRenderbuffers QGLContextPrivate::extensionFuncs(ctx).qt_glDeleteRenderbuffers
#define glGenRenderbuffers QGLContextPrivate::extensionFuncs(ctx).qt_glGenRenderbuffers
#define glRenderbufferStorage QGLContextPrivate::extensionFuncs(ctx).qt_glRenderbufferStorage
#define glGetRenderbufferParameteriv QGLContextPrivate::extensionFuncs(ctx).qt_glGetRenderbufferParameteriv
#define glIsFramebuffer QGLContextPrivate::extensionFuncs(ctx).qt_glIsFramebuffer
#define glBindFramebuffer QGLContextPrivate::extensionFuncs(ctx).qt_glBindFramebuffer
#define glDeleteFramebuffers QGLContextPrivate::extensionFuncs(ctx).qt_glDeleteFramebuffers
#define glGenFramebuffers QGLContextPrivate::extensionFuncs(ctx).qt_glGenFramebuffers
#define glCheckFramebufferStatus QGLContextPrivate::extensionFuncs(ctx).qt_glCheckFramebufferStatus
#define glFramebufferTexture2D QGLContextPrivate::extensionFuncs(ctx).qt_glFramebufferTexture2D
#define glFramebufferRenderbuffer QGLContextPrivate::extensionFuncs(ctx).qt_glFramebufferRenderbuffer
#define glGetFramebufferAttachmentParameteriv QGLContextPrivate::extensionFuncs(ctx).qt_glGetFramebufferAttachmentParameteriv
#define glGenerateMipmap QGLContextPrivate::extensionFuncs(ctx).qt_glGenerateMipmap
#endif // QT_OPENGL_ES_2
#define glBlitFramebufferEXT QGLContextPrivate::extensionFuncs(ctx).qt_glBlitFramebufferEXT
#define glRenderbufferStorageMultisampleEXT QGLContextPrivate::extensionFuncs(ctx).qt_glRenderbufferStorageMultisampleEXT


// Buffer objects
#if defined(QGL_RESOLVE_BUFFER_FUNCS)
#define glBindBuffer QGLContextPrivate::extensionFuncs(ctx).qt_glBindBuffer
#define glDeleteBuffers QGLContextPrivate::extensionFuncs(ctx).qt_glDeleteBuffers
#define glGenBuffers QGLContextPrivate::extensionFuncs(ctx).qt_glGenBuffers
#define glBufferData QGLContextPrivate::extensionFuncs(ctx).qt_glBufferData
#define glBufferSubData QGLContextPrivate::extensionFuncs(ctx).qt_glBufferSubData
#define glGetBufferSubData QGLContextPrivate::extensionFuncs(ctx).qt_glGetBufferSubData
#define glGetBufferParameteriv QGLContextPrivate::extensionFuncs(ctx).qt_glGetBufferParameteriv
#endif
#define glMapBufferARB QGLContextPrivate::extensionFuncs(ctx).qt_glMapBufferARB
#define glUnmapBufferARB QGLContextPrivate::extensionFuncs(ctx).qt_glUnmapBufferARB


// GLSL
#if !defined(QT_OPENGL_ES_2)

#define glCreateShader QGLContextPrivate::extensionFuncs(ctx).qt_glCreateShader
#define glShaderSource QGLContextPrivate::extensionFuncs(ctx).qt_glShaderSource
#define glShaderBinary QGLContextPrivate::extensionFuncs(ctx).qt_glShaderBinary
#define glCompileShader QGLContextPrivate::extensionFuncs(ctx).qt_glCompileShader
#define glDeleteShader QGLContextPrivate::extensionFuncs(ctx).qt_glDeleteShader
#define glIsShader QGLContextPrivate::extensionFuncs(ctx).qt_glIsShader

#define glCreateProgram QGLContextPrivate::extensionFuncs(ctx).qt_glCreateProgram
#define glAttachShader QGLContextPrivate::extensionFuncs(ctx).qt_glAttachShader
#define glDetachShader QGLContextPrivate::extensionFuncs(ctx).qt_glDetachShader
#define glLinkProgram QGLContextPrivate::extensionFuncs(ctx).qt_glLinkProgram
#define glUseProgram QGLContextPrivate::extensionFuncs(ctx).qt_glUseProgram
#define glDeleteProgram QGLContextPrivate::extensionFuncs(ctx).qt_glDeleteProgram
#define glIsProgram QGLContextPrivate::extensionFuncs(ctx).qt_glIsProgram

#define glGetShaderInfoLog QGLContextPrivate::extensionFuncs(ctx).qt_glGetShaderInfoLog
#define glGetShaderiv QGLContextPrivate::extensionFuncs(ctx).qt_glGetShaderiv
#define glGetShaderSource QGLContextPrivate::extensionFuncs(ctx).qt_glGetShaderSource
#define glGetProgramiv QGLContextPrivate::extensionFuncs(ctx).qt_glGetProgramiv
#define glGetProgramInfoLog QGLContextPrivate::extensionFuncs(ctx).qt_glGetProgramInfoLog

#define glGetUniformLocation QGLContextPrivate::extensionFuncs(ctx).qt_glGetUniformLocation
#define glUniform4fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniform4fv
#define glUniform3fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniform3fv
#define glUniform2fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniform2fv
#define glUniform1fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniform1fv
#define glUniform1i QGLContextPrivate::extensionFuncs(ctx).qt_glUniform1i
#define glUniform1iv QGLContextPrivate::extensionFuncs(ctx).qt_glUniform1iv
#define glUniformMatrix2fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniformMatrix2fv
#define glUniformMatrix3fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniformMatrix3fv
#define glUniformMatrix4fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniformMatrix4fv
#define glUniformMatrix2x3fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniformMatrix2x3fv
#define glUniformMatrix2x4fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniformMatrix2x4fv
#define glUniformMatrix3x2fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniformMatrix3x2fv
#define glUniformMatrix3x4fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniformMatrix3x4fv
#define glUniformMatrix4x2fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniformMatrix4x2fv
#define glUniformMatrix4x3fv QGLContextPrivate::extensionFuncs(ctx).qt_glUniformMatrix4x3fv

#define glBindAttribLocation QGLContextPrivate::extensionFuncs(ctx).qt_glBindAttribLocation
#define glGetAttribLocation QGLContextPrivate::extensionFuncs(ctx).qt_glGetAttribLocation
#define glVertexAttrib1fv QGLContextPrivate::extensionFuncs(ctx).qt_glVertexAttrib1fv
#define glVertexAttrib2fv QGLContextPrivate::extensionFuncs(ctx).qt_glVertexAttrib2fv
#define glVertexAttrib3fv QGLContextPrivate::extensionFuncs(ctx).qt_glVertexAttrib3fv
#define glVertexAttrib4fv QGLContextPrivate::extensionFuncs(ctx).qt_glVertexAttrib4fv
#define glVertexAttribPointer QGLContextPrivate::extensionFuncs(ctx).qt_glVertexAttribPointer
#define glDisableVertexAttribArray QGLContextPrivate::extensionFuncs(ctx).qt_glDisableVertexAttribArray
#define glEnableVertexAttribArray QGLContextPrivate::extensionFuncs(ctx).qt_glEnableVertexAttribArray

#else // QT_OPENGL_ES_2

#define glGetProgramBinaryOES QGLContextPrivate::extensionFuncs(ctx).qt_glGetProgramBinaryOES
#define glProgramBinaryOES QGLContextPrivate::extensionFuncs(ctx).qt_glProgramBinaryOES

#endif // QT_OPENGL_ES_2


#if !defined(QT_OPENGL_ES_2)
#define glStencilOpSeparate QGLContextPrivate::extensionFuncs(ctx).qt_glStencilOpSeparate
#define glBlendColor QGLContextPrivate::extensionFuncs(ctx).qt_glBlendColor
#endif

#if defined(QT_OPENGL_ES_2)
#define glClearDepth glClearDepthf
#endif

#define glProgramParameteriEXT QGLContextPrivate::extensionFuncs(ctx).qt_glProgramParameteriEXT
#define glFramebufferTextureEXT QGLContextPrivate::extensionFuncs(ctx).qt_glFramebufferTextureEXT
#define glFramebufferTextureLayerEXT QGLContextPrivate::extensionFuncs(ctx).qt_glFramebufferTextureLayerEXT
#define glFramebufferTextureFaceEXT QGLContextPrivate::extensionFuncs(ctx).qt_glFramebufferTextureFaceEXT

#if !defined(QT_OPENGL_ES)
#define glCompressedTexImage2D QGLContextPrivate::extensionFuncs(ctx).qt_glCompressedTexImage2DARB
#endif

extern bool qt_resolve_framebufferobject_extensions(QGLContext *ctx);
bool qt_resolve_buffer_extensions(QGLContext *ctx);

bool qt_resolve_version_1_3_functions(QGLContext *ctx);
bool qt_resolve_version_2_0_functions(QGLContext *ctx);
bool qt_resolve_stencil_face_extension(QGLContext *ctx);
bool qt_resolve_frag_program_extensions(QGLContext *ctx);

bool qt_resolve_glsl_extensions(QGLContext *ctx);

QT_END_NAMESPACE

#endif // QGL_EXTENSIONS_P_H
