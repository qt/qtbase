/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWindow>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QVector3D>
#include <QKeyEvent>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QRectF>

QT_BEGIN_NAMESPACE

class QOpenGLTexture;
class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLVertexArrayObject;

QT_END_NAMESPACE

class GLWindow : public QOpenGLWindow
{
    Q_OBJECT
    Q_PROPERTY(float blurRadius READ blurRadius WRITE setBlurRadius)

public:
    GLWindow();
    ~GLWindow();

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    float blurRadius() const { return m_blurRadius; }
    void setBlurRadius(float blurRadius);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void setAnimating(bool animate);

private:
    QPropertyAnimation *m_animationForward = nullptr;
    QPropertyAnimation *m_animationBackward = nullptr;
    QSequentialAnimationGroup *m_animationGroup;
    QOpenGLTexture *m_texImageInput = nullptr;
    QOpenGLTexture *m_texImageTmp = nullptr;
    QOpenGLTexture *m_texImageProcessed = nullptr;
    QOpenGLShaderProgram *m_shaderDisplay = nullptr;
    QOpenGLShaderProgram *m_shaderComputeV = nullptr;
    QOpenGLShaderProgram *m_shaderComputeH = nullptr;
    QMatrix4x4 m_proj;
    QSizeF     m_quadSize;

    int m_blurRadius = 0;
    bool m_animate = true;
    QOpenGLVertexArrayObject *m_vao = nullptr;
};

#endif
