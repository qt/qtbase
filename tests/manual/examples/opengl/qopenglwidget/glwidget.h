// Copyright (C) 2016 The Qt Company Ltd.
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

class Bubble;
class MainWindow;

QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLShader)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    GLWidget(MainWindow *maybeMainWindow, const QColor &background);
    ~GLWidget();

public slots:
    void setScaling(int scale);
    void setLogo();
    void setTexture();
    void setShowBubbles(bool);
    void setTransparent(bool transparent);

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
    void reset();

    MainWindow *m_mainWindow;
    qreal m_fAngle = 0;
    qreal m_fScale = 1;
    bool m_showBubbles = true;
    QList<QVector3D> m_vertices;
    QList<QVector3D> m_normals;
    bool m_qtLogo = true;
    QList<Bubble *> m_bubbles;
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
    QPushButton *m_btn2 = nullptr;
    QColor m_background;
    QMetaObject::Connection m_contextWatchConnection;
};

#endif
