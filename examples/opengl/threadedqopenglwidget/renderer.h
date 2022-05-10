// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RENDERER_H
#define RENDERER_H

#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QVector3D>
#include <QMatrix4x4>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QElapsedTimer>

QT_FORWARD_DECLARE_CLASS(QOpenGLWidget)

class Renderer : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit Renderer(QOpenGLWidget *w);
    void lockRenderer() { m_renderMutex.lock(); }
    void unlockRenderer() { m_renderMutex.unlock(); }
    QMutex *grabMutex() { return &m_grabMutex; }
    QWaitCondition *grabCond() { return &m_grabCond; }
    void prepareExit() { m_exiting = true; m_grabCond.wakeAll(); }

signals:
    void contextWanted();

public slots:
    void render();

private:
    void paintQtLogo();
    void createGeometry();
    void quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4);
    void extrude(qreal x1, qreal y1, qreal x2, qreal y2);

    bool m_inited = false;
    qreal m_fAngle = 0;
    qreal m_fScale = 1;
    QList<QVector3D> vertices;
    QList<QVector3D> normals;
    QOpenGLShaderProgram program;
    QOpenGLBuffer vbo;
    int vertexAttr = 0;
    int normalAttr = 0;
    int matrixUniform = 0;
    QOpenGLWidget *m_glwidget = nullptr;
    QMutex m_renderMutex;
    QElapsedTimer m_elapsed;
    QMutex m_grabMutex;
    QWaitCondition m_grabCond;
    bool m_exiting = false;
};

#endif // RENDERER_H
