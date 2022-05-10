// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "hellowindow.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QRandomGenerator>
#include <qmath.h>
#include <QElapsedTimer>

Renderer::Renderer(const QSurfaceFormat &format, Renderer *share, QScreen *screen)
    : m_initialized(false)
    , m_format(format)
{
    m_context = new QOpenGLContext(this);
    if (screen)
        m_context->setScreen(screen);
    m_context->setFormat(format);
    if (share)
        m_context->setShareContext(share->m_context);
    m_context->create();

    m_backgroundColor = QColor::fromRgbF(0.1f, 0.1f, 0.2f, 1.0f);
    m_backgroundColor.setRed(QRandomGenerator::global()->bounded(64));
    m_backgroundColor.setGreen(QRandomGenerator::global()->bounded(128));
    m_backgroundColor.setBlue(QRandomGenerator::global()->bounded(256));
}

HelloWindow::HelloWindow(const QSharedPointer<Renderer> &renderer, QScreen *screen)
    : m_colorIndex(0), m_renderer(renderer)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    setGeometry(QRect(10, 10, 640, 480));

    setFormat(renderer->format());
    if (screen)
        setScreen(screen);

    create();

    updateColor();

    connect(renderer.data(), &Renderer::requestUpdate, this, &QWindow::requestUpdate);
}

void HelloWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed())
        render();
}

bool HelloWindow::event(QEvent *ev)
{
    if (ev->type() == QEvent::UpdateRequest && isExposed())
        render();
    return QWindow::event(ev);
}

void HelloWindow::render()
{
    static QElapsedTimer timer;
    if (!timer.isValid())
        timer.start();
    qreal a = (qreal)(((timer.elapsed() * 3) % 36000) / 100.0);
    auto call = [this, r = m_renderer.data(), a, c = color()]() { r->render(this, a, c); };
    QMetaObject::invokeMethod(m_renderer.data(), call);
}

void HelloWindow::mousePressEvent(QMouseEvent *)
{
    updateColor();
}

QColor HelloWindow::color() const
{
    return m_color;
}

void HelloWindow::updateColor()
{
    QColor colors[] =
    {
        QColor(100, 255, 0),
        QColor(0, 100, 255)
    };

    m_color = colors[m_colorIndex];
    m_colorIndex = 1 - m_colorIndex;
}

void Renderer::render(HelloWindow *surface, qreal angle, const QColor &color)
{
    if (!m_context->makeCurrent(surface))
        return;

    QSize viewSize = surface->size();

    if (!m_initialized) {
        initialize();
        m_initialized = true;
    }

    QOpenGLFunctions *f = m_context->functions();
    f->glViewport(0, 0, viewSize.width() * surface->devicePixelRatio(), viewSize.height() * surface->devicePixelRatio());
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    f->glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), m_backgroundColor.alphaF());
    f->glFrontFace(GL_CW);
    f->glCullFace(GL_FRONT);
    f->glEnable(GL_CULL_FACE);
    f->glEnable(GL_DEPTH_TEST);

    m_program->bind();
    m_vbo.bind();

    m_program->enableAttributeArray(vertexAttr);
    m_program->enableAttributeArray(normalAttr);
    m_program->setAttributeBuffer(vertexAttr, GL_FLOAT, 0, 3);
    const int verticesSize = vertices.count() * 3 * sizeof(GLfloat);
    m_program->setAttributeBuffer(normalAttr, GL_FLOAT, verticesSize, 3);

    QMatrix4x4 modelview;
    modelview.rotate(angle, 0.0f, 1.0f, 0.0f);
    modelview.rotate(angle, 1.0f, 0.0f, 0.0f);
    modelview.rotate(angle, 0.0f, 0.0f, 1.0f);
    modelview.translate(0.0f, -0.2f, 0.0f);

    m_program->setUniformValue(matrixUniform, modelview);
    m_program->setUniformValue(colorUniform, color);

    m_context->functions()->glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    m_context->swapBuffers(surface);

    emit requestUpdate();
}

Q_GLOBAL_STATIC(QMutex, initMutex)

void Renderer::initialize()
{
    // Threaded shader compilation can confuse some drivers. Avoid it.
    QMutexLocker lock(initMutex());

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceCode(
        "attribute highp vec4 vertex;"
        "attribute mediump vec3 normal;"
        "uniform mediump mat4 matrix;"
        "uniform lowp vec4 sourceColor;"
        "varying mediump vec4 color;"
        "void main(void)"
        "{"
        "    vec3 toLight = normalize(vec3(0.0, 0.3, 1.0));"
        "    float angle = max(dot(normal, toLight), 0.0);"
        "    vec3 col = sourceColor.rgb;"
        "    color = vec4(col * 0.2 + col * 0.8 * angle, 1.0);"
        "    color = clamp(color, 0.0, 1.0);"
        "    gl_Position = matrix * vertex;"
        "}");

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceCode(
        "varying mediump vec4 color;"
        "void main(void)"
        "{"
        "    gl_FragColor = color;"
        "}");

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShader(vshader);
    m_program->addShader(fshader);
    m_program->link();
    m_program->bind();

    vertexAttr = m_program->attributeLocation("vertex");
    normalAttr = m_program->attributeLocation("normal");
    matrixUniform = m_program->uniformLocation("matrix");
    colorUniform = m_program->uniformLocation("sourceColor");

    createGeometry();

    m_vbo.create();
    m_vbo.bind();
    const int verticesSize = vertices.count() * 3 * sizeof(GLfloat);
    m_vbo.allocate(verticesSize * 2);
    m_vbo.write(0, vertices.constData(), verticesSize);
    m_vbo.write(verticesSize, normals.constData(), verticesSize);
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
        qreal x5 = 0.30 * qSin(angle);
        qreal y5 = 0.30 * qCos(angle);
        qreal x6 = 0.20 * qSin(angle);
        qreal y6 = 0.20 * qCos(angle);

        angle += sectorAngle;
        qreal x7 = 0.20 * qSin(angle);
        qreal y7 = 0.20 * qCos(angle);
        qreal x8 = 0.30 * qSin(angle);
        qreal y8 = 0.30 * qCos(angle);

        quad(x5, y5, x6, y6, x7, y7, x8, y8);

        extrude(x6, y6, x7, y7);
        extrude(x8, y8, x5, y5);
    }

    for (int i = 0;i < vertices.size();i++)
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
