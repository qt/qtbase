// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QVector3D>
#include <QMatrix4x4>
#include <QElapsedTimer>
#include <QList>
#include <QPushButton>


QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLShader)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    GLWidget(const QColor &background);
    ~GLWidget();

    void saveImage(QOpenGLWidget::TargetBuffer targetBuffer);

protected:
    void paintGL() override;
    void initializeGL() override;

private:
    void createGeometry();
    void quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4);
    void extrude(qreal x1, qreal y1, qreal x2, qreal y2);
    void reset();

    QList<QVector3D> m_vertices;
    QList<QVector3D> m_normals;
    QOpenGLShader *m_vshader = nullptr;
    QOpenGLShader *m_fshader = nullptr;
    QOpenGLShaderProgram *m_program = nullptr;
    QOpenGLBuffer m_vbo;
    int m_vertexAttr;
    int m_normalAttr;
    int m_matrixUniform;
    QColor m_background;
    QMetaObject::Connection m_contextWatchConnection;
};

#endif
