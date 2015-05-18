/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

#include "glwidget.h"
#include <QImage>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLExtraFunctions>
#include <QPropertyAnimation>
#include <QPauseAnimation>
#include <QSequentialAnimationGroup>

GLWidget::GLWidget()
    : m_texture(0),
      m_program(0),
      m_vbo(0),
      m_vao(0),
      m_target(0, 0, -1),
      m_uniformsDirty(true)
{
    m_world.setToIdentity();
    m_world.translate(0, 0, -1);
    m_world.rotate(180, 1, 0, 0);

    QSequentialAnimationGroup *animGroup = new QSequentialAnimationGroup(this);
    animGroup->setLoopCount(-1);
    QPropertyAnimation *zAnim0 = new QPropertyAnimation(this, QByteArrayLiteral("z"));
    zAnim0->setStartValue(0.0f);
    zAnim0->setEndValue(1.0f);
    zAnim0->setDuration(2000);
    animGroup->addAnimation(zAnim0);
    QPropertyAnimation *zAnim1 = new QPropertyAnimation(this, QByteArrayLiteral("z"));
    zAnim1->setStartValue(0.0f);
    zAnim1->setEndValue(70.0f);
    zAnim1->setDuration(4000);
    zAnim1->setEasingCurve(QEasingCurve::OutElastic);
    animGroup->addAnimation(zAnim1);
    QPropertyAnimation *zAnim2 = new QPropertyAnimation(this, QByteArrayLiteral("z"));
    zAnim2->setStartValue(70.0f);
    zAnim2->setEndValue(0.0f);
    zAnim2->setDuration(2000);
    animGroup->addAnimation(zAnim2);
    animGroup->start();
}

GLWidget::~GLWidget()
{
    makeCurrent();
    delete m_texture;
    delete m_program;
    delete m_vbo;
    delete m_vao;
}

void GLWidget::setZ(float v)
{
    m_eye.setZ(v);
    m_uniformsDirty = true;
    update();
}

static const char *vertexShaderSource =
    "layout(location = 0) in vec4 vertex;\n"
    "layout(location = 1) in vec3 normal;\n"
    "out vec3 vert;\n"
    "out vec3 vertNormal;\n"
    "out vec3 color;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 camMatrix;\n"
    "uniform mat4 worldMatrix;\n"
    "uniform sampler2D sampler;\n"
    "void main() {\n"
    "   ivec2 pos = ivec2(gl_InstanceID % 32, gl_InstanceID / 32);\n"
    "   vec2 t = vec2(-16 + pos.x, -18 + pos.y);\n"
    "   mat4 wm = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, t.x, t.y, 0, 1) * worldMatrix;\n"
    "   color = texelFetch(sampler, pos, 0).rgb * vec3(0.4, 1.0, 0.0);\n"
    "   vert = vec3(wm * vertex);\n"
    "   vertNormal = mat3(transpose(inverse(wm))) * normal;\n"
    "   gl_Position = projMatrix * camMatrix * wm * vertex;\n"
    "}\n";

static const char *fragmentShaderSource =
    "in highp vec3 vert;\n"
    "in highp vec3 vertNormal;\n"
    "in highp vec3 color;\n"
    "out highp vec4 fragColor;\n"
    "uniform highp vec3 lightPos;\n"
    "void main() {\n"
    "   highp vec3 L = normalize(lightPos - vert);\n"
    "   highp float NL = max(dot(normalize(vertNormal), L), 0.0);\n"
    "   highp vec3 col = clamp(color * 0.2 + color * 0.8 * NL, 0.0, 1.0);\n"
    "   fragColor = vec4(col, 1.0);\n"
    "}\n";

QByteArray versionedShaderCode(const char *src)
{
    QByteArray versionedSrc;

    if (QOpenGLContext::currentContext()->isOpenGLES())
        versionedSrc.append(QByteArrayLiteral("#version 300 es\n"));
    else
        versionedSrc.append(QByteArrayLiteral("#version 330\n"));

    versionedSrc.append(src);
    return versionedSrc;
}

void GLWidget::initializeGL()
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    if (m_texture) {
        delete m_texture;
        m_texture = 0;
    }
    QImage img(":/qtlogo.png");
    Q_ASSERT(!img.isNull());
    m_texture = new QOpenGLTexture(img.scaled(32, 36).mirrored());

    if (m_program) {
        delete m_program;
        m_program = 0;
    }
    m_program = new QOpenGLShaderProgram;
    // Prepend the correct version directive to the sources. The rest is the
    // same, thanks to the common GLSL syntax.
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vertexShaderSource));
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fragmentShaderSource));
    m_program->link();

    m_projMatrixLoc = m_program->uniformLocation("projMatrix");
    m_camMatrixLoc = m_program->uniformLocation("camMatrix");
    m_worldMatrixLoc = m_program->uniformLocation("worldMatrix");
    m_lightPosLoc = m_program->uniformLocation("lightPos");

    // Create a VAO. Not strictly required for ES 3, but it is for plain OpenGL.
    if (m_vao) {
        delete m_vao;
        m_vao = 0;
    }
    m_vao = new QOpenGLVertexArrayObject;
    if (m_vao->create())
        m_vao->bind();

    if (m_vbo) {
        delete m_vbo;
        m_vbo = 0;
    }
    m_program->bind();
    m_vbo = new QOpenGLBuffer;
    m_vbo->create();
    m_vbo->bind();
    m_vbo->allocate(m_logo.constData(), m_logo.count() * sizeof(GLfloat));
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
    m_vbo->release();

    f->glEnable(GL_DEPTH_TEST);
    f->glEnable(GL_CULL_FACE);
}

void GLWidget::resizeGL(int w, int h)
{
    m_proj.setToIdentity();
    m_proj.perspective(45.0f, GLfloat(w) / h, 0.01f, 100.0f);
    m_uniformsDirty = true;
}

void GLWidget::paintGL()
{
    // Now use QOpenGLExtraFunctions instead of QOpenGLFunctions as we want to
    // do more than what GL(ES) 2.0 offers.
    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();

    f->glClearColor(0, 0, 0, 1);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_program->bind();
    m_texture->bind();

    if (m_uniformsDirty) {
        m_uniformsDirty = false;
        QMatrix4x4 camera;
        camera.lookAt(m_eye, m_eye + m_target, QVector3D(0, 1, 0));
        m_program->setUniformValue(m_projMatrixLoc, m_proj);
        m_program->setUniformValue(m_camMatrixLoc, camera);
        m_program->setUniformValue(m_worldMatrixLoc, m_world);
        m_program->setUniformValue(m_lightPosLoc, QVector3D(0, 0, 70));
    }

    // Now call a function introduced in OpenGL 3.1 / OpenGL ES 3.0. We
    // requested a 3.3 or ES 3.0 context, so we know this will work.
    f->glDrawArraysInstanced(GL_TRIANGLES, 0, m_logo.vertexCount(), 32 * 36);
}
