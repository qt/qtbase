// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwidget.h"
#include "../joint.h"
#include <QMouseEvent>


#include <cmath>

MainWidget::MainWidget(std::string filename) {
    std::pair<std::pair<Joint*, std::vector<Joint*>>, std::pair<int, double>> data = Joint::createFromFile(filename);
    root = data.first.first;
    jntVec = data.first.second;
    nFrames = data.second.first;
    interval = data.second.second;
    currFrame = 0;
    motionTimer = new QTimer(this);
    //std::cout<<"INTERVALLE : "<<interval<<std::endl;
    motionTimer->setInterval(1);
    connect(motionTimer, &QTimer::timeout, this, [&](){MainWidget::motionEvent(nullptr);});
    motionTimer->start();
    startTime = std::chrono::high_resolution_clock::now();
}

MainWidget::~MainWidget()
{
    // Make sure the context is current when deleting the texture
    // and the buffers.
    makeCurrent();
    delete texture;
    delete geometries;
    doneCurrent();
}

//! [0]
void MainWidget::mousePressEvent(QMouseEvent *e)
{
    // Save mouse press position
    mousePressPosition = QVector2D(e->pos());
}

void MainWidget::mouseReleaseEvent(QMouseEvent *e)
{
    // Mouse release position - mouse press position
    QVector2D diff = QVector2D(e->pos()) - mousePressPosition;

    // Rotation axis is perpendicular to the mouse position difference
    // vector
    QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();

    // Accelerate angular speed relative to the length of the mouse sweep
    qreal acc = diff.length() / 100.0;

    // Calculate new rotation axis as weighted sum
    rotationAxis = (rotationAxis * angularSpeed + n * acc).normalized();

    // Increase angular speed
    angularSpeed += acc;
}
//! [0]

//! [1]
void MainWidget::timerEvent(QTimerEvent *)
{
    // Decrease angular speed (friction)
    angularSpeed *= 0.95;

    // Stop rotation when speed goes below threshold
    if (angularSpeed < 0.01) {
        angularSpeed = 0.0;
    } else {
        // Update rotation
        rotation = QQuaternion::fromAxisAndAngle(rotationAxis, angularSpeed) * rotation;

        // Request an update
        update();
    }
}
//! [1]

void MainWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0, 0, 0, 1);

    initShaders();
    initTextures();

//! [2]
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);

    // Enable back face culling
    glEnable(GL_CULL_FACE);
//! [2]

    geometries = new GeometryEngine(root, jntVec);

    // Use QBasicTimer because its faster than QTimer
    timer.start(0, this);
}

//! [3]
void MainWidget::initShaders()
{
    // Compile vertex shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl"))
        close();

    // Compile fragment shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl"))
        close();

    // Link shader pipeline
    if (!program.link())
        close();

    // Bind shader pipeline for use
    if (!program.bind())
        close();
}
//! [3]

//! [4]
void MainWidget::initTextures()
{
    // Load cube.png image
    texture = new QOpenGLTexture(QImage(":/cube.png").mirrored());

    // Set nearest filtering mode for texture minification
    texture->setMinificationFilter(QOpenGLTexture::Nearest);

    // Set bilinear filtering mode for texture magnification
    texture->setMagnificationFilter(QOpenGLTexture::Linear);

    // Wrap texture coordinates by repeating
    // f.ex. texture coordinate (1.1, 1.2) is same as (0.1, 0.2)
    texture->setWrapMode(QOpenGLTexture::Repeat);
}
//! [4]

//! [5]
void MainWidget::resizeGL(int w, int h)
{
    // Calculate aspect ratio
    qreal aspect = qreal(w) / qreal(h ? h : 1);

    // Set near plane to 3.0, far plane to 7.0, field of view 45 degrees
    const qreal zNear = 3.0, zFar = 1500.0, fov = 45.0;

    // Reset projection
    projection.setToIdentity();

    // Set perspective projection
    projection.perspective(fov, aspect, zNear, zFar);
}
//! [5]

void MainWidget::paintGL()
{
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    texture->bind();

//! [6]
    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(-80, -200, -1000.0);
    matrix.rotate(rotation);

    // Set modelview-projection matrix
    program.setUniformValue("mvp_matrix", projection * matrix);
//! [6]

    // Use texture unit 0 which contains cube.png
    program.setUniformValue("texture", 0);

    // Draw cube geometry
    //geometries->drawCubeGeometry(&program);
    // geometries->drawLineGeometry(&program);
    geometries->drawSkinGeometry(&program);
}

void MainWidget::motionEvent(QTimerEvent* e) {
    auto time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(time - startTime);
    if (elapsed.count()/interval >= currFrame + 1){
        std::cout << currFrame << std::endl;
        root->animate(currFrame);
        // geometries->updatePos(root);
        geometries->updateSkinPos(root);
        currFrame++;
        update();
        if (currFrame>=nFrames) {
            currFrame = 0;
            startTime = std::chrono::high_resolution_clock::now();
        }
    }
}