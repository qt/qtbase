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

#include "glbuffers.h"

void qgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    const GLdouble ymax = zNear * tan(qDegreesToRadians(fovy) / 2.0);
    const GLdouble ymin = -ymax;
    const GLdouble xmin = ymin * aspect;
    const GLdouble xmax = ymax * aspect;
    glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

//============================================================================//
//                                  GLTexture                                 //
//============================================================================//

GLTexture::GLTexture()
{
    glGenTextures(1, &m_texture);
}

GLTexture::~GLTexture()
{
    glDeleteTextures(1, &m_texture);
}

//============================================================================//
//                                 GLTexture2D                                //
//============================================================================//

GLTexture2D::GLTexture2D(int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
        GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, 0);
}


GLTexture2D::GLTexture2D(const QString &fileName, int width, int height)
{
    // TODO: Add error handling.
    QImage image(fileName);

    if (image.isNull()) {
        m_failed = true;
        return;
    }

    image = image.convertToFormat(QImage::Format_ARGB32);

    //qDebug() << "Image size:" << image.width() << "x" << image.height();
    if (width <= 0)
        width = image.width();
    if (height <= 0)
        height = image.height();
    if (width != image.width() || height != image.height())
        image = image.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Works on x86, so probably works on all little-endian systems.
    // Does it work on big-endian systems?
    glTexImage2D(GL_TEXTURE_2D, 0, 4, image.width(), image.height(), 0,
        GL_BGRA, GL_UNSIGNED_BYTE, image.bits());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture2D::load(int width, int height, QRgb *data)
{
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
        GL_BGRA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture2D::bind()
{
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glEnable(GL_TEXTURE_2D);
}

void GLTexture2D::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}


//============================================================================//
//                                 GLTexture3D                                //
//============================================================================//

GLTexture3D::GLTexture3D(int width, int height, int depth)
{
    GLBUFFERS_ASSERT_OPENGL("GLTexture3D::GLTexture3D", glTexImage3D, return)

    glBindTexture(GL_TEXTURE_3D, m_texture);
    glTexImage3D(GL_TEXTURE_3D, 0, 4, width, height, depth, 0,
        GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_3D, GL_GENERATE_MIPMAP, GL_TRUE);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void GLTexture3D::load(int width, int height, int depth, QRgb *data)
{
    GLBUFFERS_ASSERT_OPENGL("GLTexture3D::load", glTexImage3D, return)

    glBindTexture(GL_TEXTURE_3D, m_texture);
    glTexImage3D(GL_TEXTURE_3D, 0, 4, width, height, depth, 0,
        GL_BGRA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void GLTexture3D::bind()
{
    glBindTexture(GL_TEXTURE_3D, m_texture);
    glEnable(GL_TEXTURE_3D);
}

void GLTexture3D::unbind()
{
    glBindTexture(GL_TEXTURE_3D, 0);
    glDisable(GL_TEXTURE_3D);
}

//============================================================================//
//                                GLTextureCube                               //
//============================================================================//

GLTextureCube::GLTextureCube(int size)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);

    for (int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 4, size, size, 0,
            GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

GLTextureCube::GLTextureCube(const QStringList &fileNames, int size)
{
    // TODO: Add error handling.

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);

    int index = 0;
    for (const QString &file : fileNames) {
        QImage image(file);
        if (image.isNull()) {
            m_failed = true;
            break;
        }

        image = image.convertToFormat(QImage::Format_ARGB32);

        //qDebug() << "Image size:" << image.width() << "x" << image.height();
        if (size <= 0)
            size = image.width();
        if (size != image.width() || size != image.height())
            image = image.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        // Works on x86, so probably works on all little-endian systems.
        // Does it work on big-endian systems?
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, 4, image.width(), image.height(), 0,
            GL_BGRA, GL_UNSIGNED_BYTE, image.bits());

        if (++index == 6)
            break;
    }

    // Clear remaining faces.
    while (index < 6) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, 4, size, size, 0,
            GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
        ++index;
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void GLTextureCube::load(int size, int face, QRgb *data)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, 4, size, size, 0,
            GL_BGRA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void GLTextureCube::bind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
    glEnable(GL_TEXTURE_CUBE_MAP);
}

void GLTextureCube::unbind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDisable(GL_TEXTURE_CUBE_MAP);
}

//============================================================================//
//                            GLFrameBufferObject                             //
//============================================================================//

GLFrameBufferObject::GLFrameBufferObject(int width, int height)
    : m_width(width)
    , m_height(height)
{
    GLBUFFERS_ASSERT_OPENGL("GLFrameBufferObject::GLFrameBufferObject",
        glGenFramebuffersEXT && glGenRenderbuffersEXT && glBindRenderbufferEXT && glRenderbufferStorageEXT, return)

    // TODO: share depth buffers of same size
    glGenFramebuffersEXT(1, &m_fbo);
    //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
    glGenRenderbuffersEXT(1, &m_depthBuffer);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_depthBuffer);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, m_width, m_height);
    //glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depthBuffer);
    //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

GLFrameBufferObject::~GLFrameBufferObject()
{
    GLBUFFERS_ASSERT_OPENGL("GLFrameBufferObject::~GLFrameBufferObject",
        glDeleteFramebuffersEXT && glDeleteRenderbuffersEXT, return)

    glDeleteFramebuffersEXT(1, &m_fbo);
    glDeleteRenderbuffersEXT(1, &m_depthBuffer);
}

void GLFrameBufferObject::setAsRenderTarget(bool state)
{
    GLBUFFERS_ASSERT_OPENGL("GLFrameBufferObject::setAsRenderTarget", glBindFramebufferEXT, return)

    if (state) {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(0, 0, m_width, m_height);
    } else {
        glPopAttrib();
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }
}

bool GLFrameBufferObject::isComplete()
{
    GLBUFFERS_ASSERT_OPENGL("GLFrameBufferObject::isComplete", glCheckFramebufferStatusEXT, return false)

    return GL_FRAMEBUFFER_COMPLETE_EXT == glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
}

//============================================================================//
//                             GLRenderTargetCube                             //
//============================================================================//

GLRenderTargetCube::GLRenderTargetCube(int size)
    : GLTextureCube(size)
    , m_fbo(size, size)
{
}

void GLRenderTargetCube::begin(int face)
{
    GLBUFFERS_ASSERT_OPENGL("GLRenderTargetCube::begin",
        glFramebufferTexture2DEXT && glFramebufferRenderbufferEXT, return)

    m_fbo.setAsRenderTarget(true);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_texture, 0);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_fbo.m_depthBuffer);
}

void GLRenderTargetCube::end()
{
    m_fbo.setAsRenderTarget(false);
}

void GLRenderTargetCube::getViewMatrix(QMatrix4x4& mat, int face)
{
    if (face < 0 || face >= 6) {
        qWarning("GLRenderTargetCube::getViewMatrix: 'face' must be in the range [0, 6). (face == %d)", face);
        return;
    }

    static constexpr int perm[6][3] = {
        {2, 1, 0},
        {2, 1, 0},
        {0, 2, 1},
        {0, 2, 1},
        {0, 1, 2},
        {0, 1, 2},
    };

    static constexpr float signs[6][3] = {
        {-1.0f, -1.0f, -1.0f},
        {+1.0f, -1.0f, +1.0f},
        {+1.0f, +1.0f, -1.0f},
        {+1.0f, -1.0f, +1.0f},
        {+1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f, +1.0f},
    };

    mat.fill(0.0f);
    for (int i = 0; i < 3; ++i)
        mat(i, perm[face][i]) = signs[face][i];
    mat(3, 3) = 1.0f;
}

void GLRenderTargetCube::getProjectionMatrix(QMatrix4x4& mat, float nearZ, float farZ)
{
    static const QMatrix4x4 reference(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, -1.0f, 0.0f);

    mat = reference;
    mat(2, 2) = (nearZ+farZ)/(nearZ-farZ);
    mat(2, 3) = 2.0f*nearZ*farZ/(nearZ-farZ);
}
