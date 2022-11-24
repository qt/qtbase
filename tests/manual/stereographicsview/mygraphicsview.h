// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QOpenGLFunctions>
#include <QGraphicsView>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QOpenGLWidget>

class MyGraphicsView : public QGraphicsView, protected QOpenGLFunctions
{
public:
    MyGraphicsView(QWidget *parent = nullptr);
    MyGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);

    void saveImage(QOpenGLWidget::TargetBuffer targetBuffer);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;

    void resizeEvent(QResizeEvent *event) override;

private:
    void init();
    void initShaders();
    void initCube();

    void draw(QOpenGLWidget::TargetBuffer targetBuffer);

    void resize(int w, int h);


    bool m_initialized = false;

    QOpenGLShaderProgram m_program;
    QMatrix4x4 m_projection;
    QOpenGLBuffer m_arrayBuf;
    QOpenGLBuffer m_indexBuf;

    qreal m_yaw = 0;
};

#endif // MYGRAPHICSVIEW_H
