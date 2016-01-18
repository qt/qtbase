/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef GLEXTENSIONS_H
#define GLEXTENSIONS_H

#include <QtOpenGL>

/*
Functions resolved:

glGenFramebuffersEXT
glGenRenderbuffersEXT
glBindRenderbufferEXT
glRenderbufferStorageEXT
glDeleteFramebuffersEXT
glDeleteRenderbuffersEXT
glBindFramebufferEXT
glFramebufferTexture2DEXT
glFramebufferRenderbufferEXT
glCheckFramebufferStatusEXT

glActiveTexture
glTexImage3D

glGenBuffers
glBindBuffer
glBufferData
glDeleteBuffers
glMapBuffer
glUnmapBuffer
*/

#ifndef Q_OS_MAC
# ifndef APIENTRYP
#   ifdef APIENTRY
#     define APIENTRYP APIENTRY *
#   else
#     define APIENTRY
#     define APIENTRYP *
#   endif
# endif
#else
# define APIENTRY
# define APIENTRYP *
#endif

#ifndef GL_VERSION_1_2
#define GL_TEXTURE_3D 0x806F
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_BGRA 0x80E1
#endif

#ifndef GL_VERSION_1_3
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
//#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
//#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
//#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
//#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
//#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#endif

#ifndef GL_ARB_vertex_buffer_object
typedef ptrdiff_t GLsizeiptrARB;
#endif

#ifndef GL_VERSION_1_5
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_READ_WRITE 0x88BA
#define GL_STATIC_DRAW 0x88E4
#endif

#ifndef GL_EXT_framebuffer_object
#define GL_RENDERBUFFER_EXT 0x8D41
#define GL_FRAMEBUFFER_EXT 0x8D40
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#endif

typedef void (APIENTRY *_glGenFramebuffersEXT) (GLsizei, GLuint *);
typedef void (APIENTRY *_glGenRenderbuffersEXT) (GLsizei, GLuint *);
typedef void (APIENTRY *_glBindRenderbufferEXT) (GLenum, GLuint);
typedef void (APIENTRY *_glRenderbufferStorageEXT) (GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY *_glDeleteFramebuffersEXT) (GLsizei, const GLuint*);
typedef void (APIENTRY *_glDeleteRenderbuffersEXT) (GLsizei, const GLuint*);
typedef void (APIENTRY *_glBindFramebufferEXT) (GLenum, GLuint);
typedef void (APIENTRY *_glFramebufferTexture2DEXT) (GLenum, GLenum, GLenum, GLuint, GLint);
typedef void (APIENTRY *_glFramebufferRenderbufferEXT) (GLenum, GLenum, GLenum, GLuint);
typedef GLenum (APIENTRY *_glCheckFramebufferStatusEXT) (GLenum);

typedef void (APIENTRY *_glActiveTexture) (GLenum);
typedef void (APIENTRY *_glTexImage3D) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);

typedef void (APIENTRY *_glGenBuffers) (GLsizei, GLuint *);
typedef void (APIENTRY *_glBindBuffer) (GLenum, GLuint);
typedef void (APIENTRY *_glBufferData) (GLenum, GLsizeiptrARB, const GLvoid *, GLenum);
typedef void (APIENTRY *_glDeleteBuffers) (GLsizei, const GLuint *);
typedef void *(APIENTRY *_glMapBuffer) (GLenum, GLenum);
typedef GLboolean (APIENTRY *_glUnmapBuffer) (GLenum);

struct GLExtensionFunctions
{
    bool resolve(const QGLContext *context);

    bool fboSupported();
    bool openGL15Supported(); // the rest: multi-texture, 3D-texture, vertex buffer objects

    _glGenFramebuffersEXT GenFramebuffersEXT;
    _glGenRenderbuffersEXT GenRenderbuffersEXT;
    _glBindRenderbufferEXT BindRenderbufferEXT;
    _glRenderbufferStorageEXT RenderbufferStorageEXT;
    _glDeleteFramebuffersEXT DeleteFramebuffersEXT;
    _glDeleteRenderbuffersEXT DeleteRenderbuffersEXT;
    _glBindFramebufferEXT BindFramebufferEXT;
    _glFramebufferTexture2DEXT FramebufferTexture2DEXT;
    _glFramebufferRenderbufferEXT FramebufferRenderbufferEXT;
    _glCheckFramebufferStatusEXT CheckFramebufferStatusEXT;

    _glActiveTexture ActiveTexture;
    _glTexImage3D TexImage3D;

    _glGenBuffers GenBuffers;
    _glBindBuffer BindBuffer;
    _glBufferData BufferData;
    _glDeleteBuffers DeleteBuffers;
    _glMapBuffer MapBuffer;
    _glUnmapBuffer UnmapBuffer;
};

inline GLExtensionFunctions &getGLExtensionFunctions()
{
    static GLExtensionFunctions funcs;
    return funcs;
}

#define glGenFramebuffersEXT getGLExtensionFunctions().GenFramebuffersEXT
#define glGenRenderbuffersEXT getGLExtensionFunctions().GenRenderbuffersEXT
#define glBindRenderbufferEXT getGLExtensionFunctions().BindRenderbufferEXT
#define glRenderbufferStorageEXT getGLExtensionFunctions().RenderbufferStorageEXT
#define glDeleteFramebuffersEXT getGLExtensionFunctions().DeleteFramebuffersEXT
#define glDeleteRenderbuffersEXT getGLExtensionFunctions().DeleteRenderbuffersEXT
#define glBindFramebufferEXT getGLExtensionFunctions().BindFramebufferEXT
#define glFramebufferTexture2DEXT getGLExtensionFunctions().FramebufferTexture2DEXT
#define glFramebufferRenderbufferEXT getGLExtensionFunctions().FramebufferRenderbufferEXT
#define glCheckFramebufferStatusEXT getGLExtensionFunctions().CheckFramebufferStatusEXT

#define glActiveTexture getGLExtensionFunctions().ActiveTexture
#define glTexImage3D getGLExtensionFunctions().TexImage3D

#define glGenBuffers getGLExtensionFunctions().GenBuffers
#define glBindBuffer getGLExtensionFunctions().BindBuffer
#define glBufferData getGLExtensionFunctions().BufferData
#define glDeleteBuffers getGLExtensionFunctions().DeleteBuffers
#define glMapBuffer getGLExtensionFunctions().MapBuffer
#define glUnmapBuffer getGLExtensionFunctions().UnmapBuffer

#endif
