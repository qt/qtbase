// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWindow>
#include <QMatrix4x4>
#include <QVector3D>
#include "../hellogl2/logo.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
QT_FORWARD_DECLARE_CLASS(QOpenGLBuffer)
QT_FORWARD_DECLARE_CLASS(QOpenGLVertexArrayObject)

class GLWindow : public QOpenGLWindow
{
    Q_OBJECT
    Q_PROPERTY(float z READ z WRITE setZ)
    Q_PROPERTY(float r READ r WRITE setR)
    Q_PROPERTY(float r2 READ r2 WRITE setR2)

public:
    GLWindow();
    ~GLWindow();

    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    float z() const { return m_eye.z(); }
    void setZ(float v);

    float r() const { return m_r; }
    void setR(float v);
    float r2() const { return m_r2; }
    void setR2(float v);
private slots:
    void startSecondStage();
private:
    QOpenGLTexture *m_texture = nullptr;
    QOpenGLShaderProgram *m_program = nullptr;
    QOpenGLBuffer *m_vbo = nullptr;
    QOpenGLVertexArrayObject *m_vao = nullptr;
    Logo m_logo;
    int m_projMatrixLoc = 0;
    int m_camMatrixLoc = 0;
    int m_worldMatrixLoc = 0;
    int m_myMatrixLoc = 0;
    int m_lightPosLoc = 0;
    QMatrix4x4 m_proj;
    QMatrix4x4 m_world;
    QVector3D m_eye;
    QVector3D m_target = {0, 0, -1};
    bool m_uniformsDirty = true;
    float m_r = 0;
    float m_r2 = 0;
};

#endif
