// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
