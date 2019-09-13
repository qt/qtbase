/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QVector3D>
#include <QMatrix4x4>
#include <QElapsedTimer>
#include <QVector>
#include <QPushButton>

class Bubble;
class MainWindow;

QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLShader)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    GLWidget(MainWindow *mw, bool button, const QColor &background);
    ~GLWidget();

public slots:
    void setScaling(int scale);
    void setLogo();
    void setTexture();
    void setShowBubbles(bool);
    void setTransparent(bool transparent);

private slots:
    void handleButtonPress();

protected:
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void initializeGL() override;

private:
    void paintTexturedCube();
    void paintQtLogo();
    void createGeometry();
    void createBubbles(int number);
    void quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4);
    void extrude(qreal x1, qreal y1, qreal x2, qreal y2);

    MainWindow *m_mainWindow;
    qreal m_fAngle = 0;
    qreal m_fScale = 1;
    bool m_showBubbles = true;
    QVector<QVector3D> m_vertices;
    QVector<QVector3D> m_normals;
    bool m_qtLogo = true;
    QVector<Bubble *> m_bubbles;
    int m_frames = 0;
    QElapsedTimer m_time;
    QOpenGLShader *m_vshader1 = nullptr;
    QOpenGLShader *m_fshader1 = nullptr;
    QOpenGLShader *m_vshader2 = nullptr;
    QOpenGLShader *m_fshader2 = nullptr;
    QOpenGLShaderProgram *m_program1 = nullptr;
    QOpenGLShaderProgram *m_program2 = nullptr;
    QOpenGLTexture *m_texture = nullptr;
    QOpenGLBuffer m_vbo1;
    QOpenGLBuffer m_vbo2;
    int m_vertexAttr1 = 0;
    int m_normalAttr1 = 0;
    int m_matrixUniform1 = 0;
    int m_vertexAttr2 = 0;
    int m_normalAttr2 = 0;
    int m_texCoordAttr2 = 0;
    int m_matrixUniform2 = 0;
    int m_textureUniform2 = 0;
    bool m_transparent = false;
    QPushButton *m_btn = nullptr;
    bool m_hasButton;
    QColor m_background;
};

#endif
