// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "glwidget.h"
#include <QPainter>
#include <QPaintEngine>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QFileDialog>
#include <qmath.h>

GLWidget::GLWidget(const QColor &background)
    : m_background(background)
{
    setMinimumSize(300, 250);
}

GLWidget::~GLWidget()
{
    reset();
}

void GLWidget::saveImage(TargetBuffer targetBuffer)
{
    QImage img = grabFramebuffer(targetBuffer);
    if (img.isNull()) {
        qFatal("Failed to grab framebuffer");
    }

    const char *fn =
            targetBuffer == TargetBuffer::LeftBuffer
            ? "leftBuffer.png" : "rightBuffer.png";

    QFileDialog fd(this);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setDefaultSuffix("png");
    fd.selectFile(fn);
    if (fd.exec() == QDialog::Accepted)
        img.save(fd.selectedFiles().first());
}

void GLWidget::reset()
{
    // And now release all OpenGL resources.
    makeCurrent();
    delete m_program;
    m_program = nullptr;
    delete m_vshader;
    m_vshader = nullptr;
    delete m_fshader;
    m_fshader = nullptr;
    m_vbo.destroy();
    doneCurrent();

    // We are done with the current QOpenGLContext, forget it. If there is a
    // subsequent initialize(), that will then connect to the new context.
    QObject::disconnect(m_contextWatchConnection);
}
void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    m_vshader = new QOpenGLShader(QOpenGLShader::Vertex);
    const char *vsrc1 =
        "attribute highp vec4 vertex;\n"
        "attribute mediump vec3 normal;\n"
        "uniform mediump mat4 matrix;\n"
        "varying mediump vec4 color;\n"
        "void main(void)\n"
        "{\n"
        "    vec3 toLight = normalize(vec3(0.0, 0.3, 1.0));\n"
        "    float angle = max(dot(normal, toLight), 0.0);\n"
        "    vec3 col = vec3(0.40, 1.0, 0.0);\n"
        "    color = vec4(col * 0.2 + col * 0.8 * angle, 1.0);\n"
        "    color = clamp(color, 0.0, 1.0);\n"
        "    gl_Position = matrix * vertex;\n"
        "}\n";
    m_vshader->compileSourceCode(vsrc1);

    m_fshader = new QOpenGLShader(QOpenGLShader::Fragment);
    const char *fsrc1 =
        "varying mediump vec4 color;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = color;\n"
        "}\n";
    m_fshader->compileSourceCode(fsrc1);

    m_program = new QOpenGLShaderProgram;
    m_program->addShader(m_vshader);
    m_program->addShader(m_fshader);
    m_program->link();


    m_vertexAttr = m_program->attributeLocation("vertex");
    m_normalAttr = m_program->attributeLocation("normal");
    m_matrixUniform = m_program->uniformLocation("matrix");

    createGeometry();

    m_vbo.create();
    m_vbo.bind();
    const int vertexCount = m_vertices.count();
    QList<GLfloat> buf;
    buf.resize(vertexCount * 3 * 2);
    GLfloat *p = buf.data();
    for (int i = 0; i < vertexCount; ++i) {
        *p++ = m_vertices[i].x();
        *p++ = m_vertices[i].y();
        *p++ = m_vertices[i].z();
        *p++ = m_normals[i].x();
        *p++ = m_normals[i].y();
        *p++ = m_normals[i].z();
    }
    m_vbo.allocate(buf.constData(), (int)buf.count() * sizeof(GLfloat));
    m_vbo.release();

    m_contextWatchConnection = QObject::connect(context(), &QOpenGLContext::aboutToBeDestroyed, context(), [this] { reset(); });

    glFrontFace(GL_CW);
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void GLWidget::paintGL()
{
    // When QSurfaceFormat::StereoBuffers is enabled, this function is called twice.
    // Once where currentTargetBuffer() == QOpenGLWidget::LeftBuffer,
    // and once where currentTargetBuffer() == QOpenGLWidget::RightBuffer.

    glClearColor(m_background.redF(), m_background.greenF(), m_background.blueF(), 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //! [1]
    // Slightly translate the model, so that there's a visible difference in each buffer.
    QMatrix4x4 modelview;
    if (currentTargetBuffer() == QOpenGLWidget::LeftBuffer)
        modelview.translate(-0.4f, 0.0f, 0.0f);
    else if (currentTargetBuffer() == QOpenGLWidget::RightBuffer)
        modelview.translate(0.4f, 0.0f, 0.0f);
     //! [1]

    m_program->bind();
    m_program->setUniformValue(m_matrixUniform, modelview);
    m_program->enableAttributeArray(m_vertexAttr);
    m_program->enableAttributeArray(m_normalAttr);

    m_vbo.bind();
    m_program->setAttributeBuffer(m_vertexAttr, GL_FLOAT, 0, 3, 6 * sizeof(GLfloat));
    m_program->setAttributeBuffer(m_normalAttr, GL_FLOAT, 3 * sizeof(GLfloat), 3, 6 * sizeof(GLfloat));
    m_vbo.release();

    glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());

    m_program->disableAttributeArray(m_normalAttr);
    m_program->disableAttributeArray(m_vertexAttr);
    m_program->release();
    update();
}

void GLWidget::createGeometry()
{
    m_vertices.clear();
    m_normals.clear();

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
        qreal x5 = 0.30 * sin(angle);
        qreal y5 = 0.30 * cos(angle);
        qreal x6 = 0.20 * sin(angle);
        qreal y6 = 0.20 * cos(angle);

        angle += sectorAngle;
        qreal x7 = 0.20 * sin(angle);
        qreal y7 = 0.20 * cos(angle);
        qreal x8 = 0.30 * sin(angle);
        qreal y8 = 0.30 * cos(angle);

        quad(x5, y5, x6, y6, x7, y7, x8, y8);

        extrude(x6, y6, x7, y7);
        extrude(x8, y8, x5, y5);
    }

    for (int i = 0;i < m_vertices.size();i++)
        m_vertices[i] *= 2.0f;
}

void GLWidget::quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4)
{
    m_vertices << QVector3D(x1, y1, -0.05f);
    m_vertices << QVector3D(x2, y2, -0.05f);
    m_vertices << QVector3D(x4, y4, -0.05f);

    m_vertices << QVector3D(x3, y3, -0.05f);
    m_vertices << QVector3D(x4, y4, -0.05f);
    m_vertices << QVector3D(x2, y2, -0.05f);

    QVector3D n = QVector3D::normal
        (QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(x4 - x1, y4 - y1, 0.0f));

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_vertices << QVector3D(x4, y4, 0.05f);
    m_vertices << QVector3D(x2, y2, 0.05f);
    m_vertices << QVector3D(x1, y1, 0.05f);

    m_vertices << QVector3D(x2, y2, 0.05f);
    m_vertices << QVector3D(x4, y4, 0.05f);
    m_vertices << QVector3D(x3, y3, 0.05f);

    n = QVector3D::normal
        (QVector3D(x2 - x4, y2 - y4, 0.0f), QVector3D(x1 - x4, y1 - y4, 0.0f));

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_normals << n;
    m_normals << n;
    m_normals << n;
}

void GLWidget::extrude(qreal x1, qreal y1, qreal x2, qreal y2)
{
    m_vertices << QVector3D(x1, y1, +0.05f);
    m_vertices << QVector3D(x2, y2, +0.05f);
    m_vertices << QVector3D(x1, y1, -0.05f);

    m_vertices << QVector3D(x2, y2, -0.05f);
    m_vertices << QVector3D(x1, y1, -0.05f);
    m_vertices << QVector3D(x2, y2, +0.05f);

    QVector3D n = QVector3D::normal
        (QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(0.0f, 0.0f, -0.1f));

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_normals << n;
    m_normals << n;
    m_normals << n;
}
