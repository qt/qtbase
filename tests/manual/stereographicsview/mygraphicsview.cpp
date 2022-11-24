// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mygraphicsview.h"
#include <QResizeEvent>
#include <QFileDialog>

Q_OPENGL_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

MyGraphicsView::MyGraphicsView(QWidget *parent) :
    QGraphicsView(parent),
    m_indexBuf(QOpenGLBuffer::IndexBuffer)
{

}

MyGraphicsView::MyGraphicsView(QGraphicsScene *scene, QWidget *parent) :
    QGraphicsView(scene, parent)
{

}

void MyGraphicsView::saveImage(QOpenGLWidget::TargetBuffer targetBuffer)
{
    QOpenGLWidget *w = static_cast<QOpenGLWidget*>(viewport());
    Q_ASSERT(w);

    w->makeCurrent(targetBuffer);
    draw(targetBuffer);

    QImage img = qt_gl_read_framebuffer(w->size() * w->devicePixelRatio(), true, true);

    if (img.isNull()) {
        qFatal("Failed to grab framebuffer");
    }

    const char *fn =
            targetBuffer == QOpenGLWidget::LeftBuffer
            ? "leftBuffer.png" : "rightBuffer.png";

    QFileDialog fd(this);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setDefaultSuffix("png");
    fd.selectFile(fn);
    if (fd.exec() == QDialog::Accepted)
        img.save(fd.selectedFiles().first());
}

void MyGraphicsView::drawBackground(QPainter *painter, const QRectF &rect)
{
    if (!m_initialized)
        init();

    QOpenGLWidget *w = static_cast<QOpenGLWidget*>(viewport());

    painter->beginNativePainting();

    draw(w->currentTargetBuffer());

    painter->endNativePainting();


    m_yaw += 0.5;
    if (m_yaw > 360)
        m_yaw = 0;
}

void MyGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    resize(event->size().width(), event->size().height());
}

void MyGraphicsView::init()
{
    if (m_initialized)
        return;

    initializeOpenGLFunctions();

    initShaders();
    initCube();

    resize(viewport()->width(), viewport()->height());

    m_initialized = true;
}

void MyGraphicsView::initShaders()
{
    static const char *vertexShaderSource =
            "#ifdef GL_ES\n"
            "// Set default precision to medium\n"
            "precision mediump int;\n"
            "precision mediump float;\n"
            "#endif\n"
            "attribute vec4 a_position;\n"
            "uniform mat4 u_mvp;\n"
            "void main() {\n"
               "gl_Position = u_mvp * a_position;\n"
            "}\n";

    static const char *fragmentShaderSource =
            "#ifdef GL_ES\n"
            "// Set default precision to medium\n"
            "precision mediump int;\n"
            "precision mediump float;\n"
            "#endif\n"
            "void main() {\n"
               "gl_FragColor = vec4(0.7, 0.1, 0.0, 1.0);\n"
            "}\n";

    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource))
        close();

    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource))
        close();

    if (!m_program.link())
        close();

    if (!m_program.bind())
        close();
}

void MyGraphicsView::initCube()
{
    m_arrayBuf.create();
    m_indexBuf.create();

    QVector3D vertices[] = {
        QVector3D(-1.0f, -1.0f,  1.0f),
        QVector3D( 1.0f, -1.0f,  1.0f),
        QVector3D(-1.0f,  1.0f,  1.0f),
        QVector3D( 1.0f,  1.0f,  1.0f),
        QVector3D( 1.0f, -1.0f,  1.0f),
        QVector3D( 1.0f, -1.0f, -1.0f),
        QVector3D( 1.0f,  1.0f,  1.0f),
        QVector3D( 1.0f,  1.0f, -1.0f),
        QVector3D( 1.0f, -1.0f, -1.0f),
        QVector3D(-1.0f, -1.0f, -1.0f),
        QVector3D( 1.0f,  1.0f, -1.0f),
        QVector3D(-1.0f,  1.0f, -1.0f),
        QVector3D(-1.0f, -1.0f, -1.0f),
        QVector3D(-1.0f, -1.0f,  1.0f),
        QVector3D(-1.0f,  1.0f, -1.0f),
        QVector3D(-1.0f,  1.0f,  1.0f),
        QVector3D(-1.0f, -1.0f, -1.0f),
        QVector3D( 1.0f, -1.0f, -1.0f),
        QVector3D(-1.0f, -1.0f,  1.0f),
        QVector3D( 1.0f, -1.0f,  1.0f),
        QVector3D(-1.0f,  1.0f,  1.0f),
        QVector3D( 1.0f,  1.0f,  1.0f),
        QVector3D(-1.0f,  1.0f, -1.0f),
        QVector3D( 1.0f,  1.0f, -1.0f)
    };

    GLushort indices[] = {
        0,  1,  2,  3,  3,
        4,  4,  5,  6,  7,  7,
        8,  8,  9, 10, 11, 11,
        12, 12, 13, 14, 15, 15,
        16, 16, 17, 18, 19, 19,
        20, 20, 21, 22, 23
    };


    m_arrayBuf.bind();
    m_arrayBuf.allocate(vertices, 24 * sizeof(QVector3D));

    m_indexBuf.bind();
    m_indexBuf.allocate(indices, 34 * sizeof(GLushort));
}

void MyGraphicsView::draw(QOpenGLWidget::TargetBuffer targetBuffer)
{
    glClearColor(0.0f, 0.7f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_program.bind();
    m_arrayBuf.bind();
    m_indexBuf.bind();

    QMatrix4x4 matrix;
    matrix.translate(targetBuffer == QOpenGLWidget::LeftBuffer ? -2.0 : 2.0,
                     0.0,
                     -10.0);
    matrix.scale(2.0);
    matrix.rotate(QQuaternion::fromEulerAngles(10, m_yaw, 0));
    m_program.setUniformValue("u_mvp", m_projection * matrix);

    int vertexLocation = m_program.attributeLocation("a_position");
    m_program.enableAttributeArray(vertexLocation);
    m_program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(QVector3D));

    glDrawElements(GL_TRIANGLE_STRIP, 34, GL_UNSIGNED_SHORT, nullptr);
}

void MyGraphicsView::resize(int w, int h)
{
    qreal aspect = qreal(w) / qreal(h ? h : 1);
    const qreal zNear = 0.0, zFar = 100.0, fov = 45.0;
    m_projection.setToIdentity();
    m_projection.perspective(fov, aspect, zNear, zFar);

    if (m_initialized)
        glViewport(0, 0, w, h);
}
