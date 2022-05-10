// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "renderer.h"
#include <qmath.h>
#include <QOpenGLWidget>
#include <QGuiApplication>

static const char VERTEX_SHADER[] = R"(attribute highp vec4 vertex;
attribute mediump vec3 normal;
uniform mediump mat4 matrix;
varying mediump vec4 color;
void main(void)
{
    vec3 toLight = normalize(vec3(0.0, 0.3, 1.0));
    float angle = max(dot(normal, toLight), 0.0);
    vec3 col = vec3(0.40, 1.0, 0.0);
    color = vec4(col * 0.2 + col * 0.8 * angle, 1.0);
    color = clamp(color, 0.0, 1.0);
    gl_Position = matrix * vertex;
}
)";

static const char FRAGMENT_SHADER[] = R"(varying mediump vec4 color;
void main(void)
{
    gl_FragColor = color;
}
)";

Renderer::Renderer(QOpenGLWidget *w) :
    m_glwidget(w)
{
}

void Renderer::paintQtLogo()
{
    vbo.bind();
    program.setAttributeBuffer(vertexAttr, GL_FLOAT, 0, 3);
    program.setAttributeBuffer(normalAttr, GL_FLOAT, vertices.count() * 3 * sizeof(GLfloat), 3);
    vbo.release();

    program.enableAttributeArray(vertexAttr);
    program.enableAttributeArray(normalAttr);

    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    program.disableAttributeArray(normalAttr);
    program.disableAttributeArray(vertexAttr);
}

// Some OpenGL implementations have serious issues with compiling and linking
// shaders on multiple threads concurrently. Avoid this.
Q_GLOBAL_STATIC(QMutex, initMutex)

void Renderer::render()
{
    if (m_exiting)
        return;

    QOpenGLContext *ctx = m_glwidget->context();
    if (!ctx) // QOpenGLWidget not yet initialized
        return;

    // Grab the context.
    m_grabMutex.lock();
    emit contextWanted();
    m_grabCond.wait(&m_grabMutex);
    QMutexLocker lock(&m_renderMutex);
    m_grabMutex.unlock();

    if (m_exiting)
        return;

    Q_ASSERT(ctx->thread() == QThread::currentThread());

    // Make the context (and an offscreen surface) current for this thread. The
    // QOpenGLWidget's fbo is bound in the context.
    m_glwidget->makeCurrent();

    if (!m_inited) {
        m_inited = true;
        initializeOpenGLFunctions();

        QMutexLocker initLock(initMutex());
        QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
        vshader->compileSourceCode(VERTEX_SHADER);

        QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
        fshader->compileSourceCode(FRAGMENT_SHADER);

        program.addShader(vshader);
        program.addShader(fshader);
        program.link();

        vertexAttr = program.attributeLocation("vertex");
        normalAttr = program.attributeLocation("normal");
        matrixUniform = program.uniformLocation("matrix");

        m_fAngle = 0;
        m_fScale = 1;
        createGeometry();

        vbo.create();
        vbo.bind();
        const int verticesSize = vertices.count() * 3 * sizeof(GLfloat);
        vbo.allocate(verticesSize * 2);
        vbo.write(0, vertices.constData(), verticesSize);
        vbo.write(verticesSize, normals.constData(), verticesSize);

        m_elapsed.start();
    }

    //qDebug("%p elapsed %lld", QThread::currentThread(), m_elapsed.restart());

    glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glFrontFace(GL_CW);
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    QMatrix4x4 modelview;
    modelview.rotate(m_fAngle, 0.0f, 1.0f, 0.0f);
    modelview.rotate(m_fAngle, 1.0f, 0.0f, 0.0f);
    modelview.rotate(m_fAngle, 0.0f, 0.0f, 1.0f);
    modelview.scale(m_fScale);
    modelview.translate(0.0f, -0.2f, 0.0f);

    program.bind();
    program.setUniformValue(matrixUniform, modelview);
    paintQtLogo();
    program.release();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_fAngle += 1.0f;

    // Make no context current on this thread and move the QOpenGLWidget's
    // context back to the gui thread.
    m_glwidget->doneCurrent();
    ctx->moveToThread(qGuiApp->thread());

    // Schedule composition. Note that this will use QueuedConnection, meaning
    // that update() will be invoked on the gui thread.
    QMetaObject::invokeMethod(m_glwidget, "update");
}

void Renderer::createGeometry()
{
    vertices.clear();
    normals.clear();

    qreal x1 = +0.06f;
    qreal y1 = -0.14f;
    qreal x2 = +0.14f;
    qreal y2 = -0.06f;
    qreal x3 = +0.08f;
    qreal y3 = +0.00f;
    qreal x4 = +0.30f;
    qreal y4 = +0.22f;

    quad(x1, y1, x2, y2, y2, x2, y1, x1);
    quad(x3, y3, x4, y4, y4, x4, y3, x3);

    extrude(x1, y1, x2, y2);
    extrude(x2, y2, y2, x2);
    extrude(y2, x2, y1, x1);
    extrude(y1, x1, x1, y1);
    extrude(x3, y3, x4, y4);
    extrude(x4, y4, y4, x4);
    extrude(y4, x4, y3, x3);

    const int NumSectors = 100;
    const qreal sectorAngle = 2 * qreal(M_PI) / NumSectors;

    for (int i = 0; i < NumSectors; ++i) {
        qreal angle = i * sectorAngle;
        qreal sinAngle = sin(angle);
        qreal cosAngle = cos(angle);
        qreal x5 = 0.30 * sinAngle;
        qreal y5 = 0.30 * cosAngle;
        qreal x6 = 0.20 * sinAngle;
        qreal y6 = 0.20 * cosAngle;

        angle += sectorAngle;
        sinAngle = sin(angle);
        cosAngle = cos(angle);
        qreal x7 = 0.20 * sinAngle;
        qreal y7 = 0.20 * cosAngle;
        qreal x8 = 0.30 * sinAngle;
        qreal y8 = 0.30 * cosAngle;

        quad(x5, y5, x6, y6, x7, y7, x8, y8);

        extrude(x6, y6, x7, y7);
        extrude(x8, y8, x5, y5);
    }

    for (qsizetype i = 0; i < vertices.size(); ++i)
        vertices[i] *= 2.0f;
}

void Renderer::quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4)
{
    vertices << QVector3D(x1, y1, -0.05f);
    vertices << QVector3D(x2, y2, -0.05f);
    vertices << QVector3D(x4, y4, -0.05f);

    vertices << QVector3D(x3, y3, -0.05f);
    vertices << QVector3D(x4, y4, -0.05f);
    vertices << QVector3D(x2, y2, -0.05f);

    QVector3D n = QVector3D::normal
        (QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(x4 - x1, y4 - y1, 0.0f));

    normals << n;
    normals << n;
    normals << n;

    normals << n;
    normals << n;
    normals << n;

    vertices << QVector3D(x4, y4, 0.05f);
    vertices << QVector3D(x2, y2, 0.05f);
    vertices << QVector3D(x1, y1, 0.05f);

    vertices << QVector3D(x2, y2, 0.05f);
    vertices << QVector3D(x4, y4, 0.05f);
    vertices << QVector3D(x3, y3, 0.05f);

    n = QVector3D::normal
        (QVector3D(x2 - x4, y2 - y4, 0.0f), QVector3D(x1 - x4, y1 - y4, 0.0f));

    normals << n;
    normals << n;
    normals << n;

    normals << n;
    normals << n;
    normals << n;
}

void Renderer::extrude(qreal x1, qreal y1, qreal x2, qreal y2)
{
    vertices << QVector3D(x1, y1, +0.05f);
    vertices << QVector3D(x2, y2, +0.05f);
    vertices << QVector3D(x1, y1, -0.05f);

    vertices << QVector3D(x2, y2, -0.05f);
    vertices << QVector3D(x1, y1, -0.05f);
    vertices << QVector3D(x2, y2, +0.05f);

    QVector3D n = QVector3D::normal
        (QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(0.0f, 0.0f, -0.1f));

    normals << n;
    normals << n;
    normals << n;

    normals << n;
    normals << n;
    normals << n;
}
