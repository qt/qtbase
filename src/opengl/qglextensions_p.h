/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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


// OpenGL constants

#ifndef FRAMEBUFFER_SRGB_CAPABLE_EXT
#define FRAMEBUFFER_SRGB_CAPABLE_EXT 0x8DBA
#endif

#ifndef FRAMEBUFFER_SRGB_EXT
#define FRAMEBUFFER_SRGB_EXT 0x8DB9
#endif

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER                   0x8892
#endif

#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW                    0x88E4
#endif

/* NV_texture_rectangle */
#ifndef GL_NV_texture_rectangle
#define GL_TEXTURE_RECTANGLE_NV           0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_NV   0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_NV     0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_NV  0x84F8
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#ifndef GL_RGB16
#define GL_RGB16 0x8054
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_IBM_texture_mirrored_repeat
#define GL_MIRRORED_REPEAT_IBM            0x8370
#endif

#ifndef GL_SGIS_generate_mipmap
#define GL_GENERATE_MIPMAP_SGIS           0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS      0x8192
#endif

// ARB_fragment_program extension protos
#ifndef GL_FRAGMENT_PROGRAM_ARB
#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER_ARB
#define GL_PIXEL_UNPACK_BUFFER_ARB 0x88EC
#endif

#ifndef GL_WRITE_ONLY_ARB
#define GL_WRITE_ONLY_ARB 0x88B9
#endif

#ifndef GL_STREAM_DRAW_ARB
#define GL_STREAM_DRAW_ARB 0x88E0
#endif

// Stencil wrap and two-side defines
#ifndef GL_STENCIL_TEST_TWO_SIDE_EXT
#define GL_STENCIL_TEST_TWO_SIDE_EXT 0x8910
#endif
#ifndef GL_INCR_WRAP_EXT
#define GL_INCR_WRAP_EXT 0x8507
#endif
#ifndef GL_DECR_WRAP_EXT
#define GL_DECR_WRAP_EXT 0x8508
#endif

#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif

#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif

#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16 0x81A5
#endif

#ifndef GL_DEPTH_COMPONENT24_OES
#define GL_DEPTH_COMPONENT24_OES 0x81A6
#endif

#ifndef GL_EXT_framebuffer_object
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT                    0x0506
#define GL_MAX_RENDERBUFFER_SIZE_EXT                            0x84E8
#define GL_FRAMEBUFFER_BINDING_EXT                              0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT                             0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT               0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT               0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT             0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT     0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT        0x8CD4
#define GL_FRAMEBUFFER_COMPLETE_EXT                             0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT                0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT        0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT      0x8CD8
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT                0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT                   0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT               0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT               0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                          0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS_EXT                            0x8CDF
#define GL_COLOR_ATTACHMENT0_EXT                                0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT                                0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT                                0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT                                0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT                                0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT                                0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT                                0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT                                0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT                                0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT                                0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT                               0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT                               0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT                               0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT                               0x8CED
#define GL_COLOR_ATTACHMENT14_EXT                               0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT                               0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT                                 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT                               0x8D20
#define GL_FRAMEBUFFER_EXT                                      0x8D40
#define GL_RENDERBUFFER_EXT                                     0x8D41
#define GL_RENDERBUFFER_WIDTH_EXT                               0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT                              0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT                     0x8D44
#define GL_STENCIL_INDEX_EXT                                    0x8D45
#define GL_STENCIL_INDEX1_EXT                                   0x8D46
#define GL_STENCIL_INDEX4_EXT                                   0x8D47
#define GL_STENCIL_INDEX8_EXT                                   0x8D48
#define GL_STENCIL_INDEX16_EXT                                  0x8D49
#define GL_RENDERBUFFER_RED_SIZE_EXT                            0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT                          0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT                           0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT                          0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT                          0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT                        0x8D55
#endif

// GL_EXT_framebuffer_blit
#ifndef GL_READ_FRAMEBUFFER_EXT
#define GL_READ_FRAMEBUFFER_EXT                                 0x8CA8
#endif

// GL_EXT_framebuffer_multisample
#ifndef GL_RENDERBUFFER_SAMPLES_EXT
#define GL_RENDERBUFFER_SAMPLES_EXT                             0x8CAB
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT               0x8D56
#endif

#ifndef GL_MAX_SAMPLES_EXT
#define GL_MAX_SAMPLES_EXT                                      0x8D57
#endif

#ifndef GL_DRAW_FRAMEBUFFER_EXT
#define GL_DRAW_FRAMEBUFFER_EXT                                 0x8CA9
#endif

#ifndef GL_EXT_packed_depth_stencil
#define GL_DEPTH_STENCIL_EXT                                    0x84F9
#define GL_UNSIGNED_INT_24_8_EXT                                0x84FA
#define GL_DEPTH24_STENCIL8_EXT                                 0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT                             0x88F1
#endif

// ### hm. should be part of the GL 1.2 spec..
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE                  0x812F
#endif

#ifndef GL_VERSION_1_2
#define GL_PACK_SKIP_IMAGES               0x806B
#define GL_PACK_IMAGE_HEIGHT              0x806C
#define GL_UNPACK_SKIP_IMAGES             0x806D
#define GL_UNPACK_IMAGE_HEIGHT            0x806E
#endif

#ifndef GL_VERSION_1_4
#define GL_CONSTANT_COLOR 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_INCR_WRAP 0x8507
#define GL_DECR_WRAP 0x8508
#endif

#ifndef GL_VERSION_1_5
#define GL_ARRAY_BUFFER                                         0x8892
#define GL_ELEMENT_ARRAY_BUFFER                                 0x8893
#define GL_STREAM_DRAW                                          0x88E0
#define GL_STREAM_READ                                          0x88E1
#define GL_STREAM_COPY                                          0x88E2
#define GL_STATIC_DRAW                                          0x88E4
#define GL_STATIC_READ                                          0x88E5
#define GL_STATIC_COPY                                          0x88E6
#define GL_DYNAMIC_DRAW                                         0x88E8
#define GL_DYNAMIC_READ                                         0x88E9
#define GL_DYNAMIC_COPY                                         0x88EA
#endif

#ifndef GL_VERSION_2_0
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_FLOAT_VEC2 0x8B50
#define GL_FLOAT_VEC3 0x8B51
#define GL_FLOAT_VEC4 0x8B52
#define GL_INT_VEC2 0x8B53
#define GL_INT_VEC3 0x8B54
#define GL_INT_VEC4 0x8B55
#define GL_BOOL 0x8B56
#define GL_BOOL_VEC2 0x8B57
#define GL_BOOL_VEC3 0x8B58
#define GL_BOOL_VEC4 0x8B59
#define GL_FLOAT_MAT2 0x8B5A
#define GL_FLOAT_MAT3 0x8B5B
#define GL_FLOAT_MAT4 0x8B5C
#define GL_SAMPLER_1D 0x8B5D
#define GL_SAMPLER_2D 0x8B5E
#define GL_SAMPLER_3D 0x8B5F
#define GL_SAMPLER_CUBE 0x8B60
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_ACTIVE_UNIFORMS 0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH 0x8B87
#define GL_ACTIVE_ATTRIBUTES 0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH 0x8B8A
#endif

// Geometry shader defines
#ifndef GL_GEOMETRY_SHADER_EXT
#  define GL_GEOMETRY_SHADER_EXT 0x8DD9
#  define GL_GEOMETRY_VERTICES_OUT_EXT 0x8DDA
#  define GL_GEOMETRY_INPUT_TYPE_EXT 0x8DDB
#  define GL_GEOMETRY_OUTPUT_TYPE_EXT 0x8DDC
#  define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT 0x8C29
#  define GL_MAX_GEOMETRY_VARYING_COMPONENTS_EXT 0x8DDD
#  define GL_MAX_VERTEX_VARYING_COMPONENTS_EXT 0x8DDE
#  define GL_MAX_VARYING_COMPONENTS_EXT 0x8B4B
#  define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT 0x8DDF
#  define GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT 0x8DE0
#  define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT 0x8DE1
#  define GL_LINES_ADJACENCY_EXT 0xA
#  define GL_LINE_STRIP_ADJACENCY_EXT 0xB
#  define GL_TRIANGLES_ADJACENCY_EXT 0xC
#  define GL_TRIANGLE_STRIP_ADJACENCY_EXT 0xD
#  define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT 0x8DA8
#  define GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT 0x8DA9
#  define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT 0x8DA7
#  define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER_EXT 0x8CD4
#  define GL_PROGRAM_POINT_SIZE_EXT 0x8642
#endif

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
