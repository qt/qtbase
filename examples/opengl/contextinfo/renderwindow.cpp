// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "renderwindow.h"
#include <QTimer>
#include <QMatrix4x4>
#include <QOpenGLContext>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QOpenGLFunctions>

RenderWindow::RenderWindow(const QSurfaceFormat &format)
    : m_context(nullptr),
      m_initialized(false),
      m_forceGLSL110(false),
      m_angle(0.0f)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setFormat(format);
    m_context = new QOpenGLContext(this);
    m_context->setFormat(requestedFormat());
    if (!m_context->create()) {
        delete m_context;
        m_context = nullptr;
    }
}

void RenderWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed())
        render();
}

// ES needs the precision qualifiers.
// On desktop GL QOpenGLShaderProgram inserts dummy defines for highp/mediump/lowp.
static const char *vertexShaderSource110 =
    "attribute highp vec4 posAttr;\n"
    "attribute lowp vec4 colAttr;\n"
    "varying lowp vec4 col;\n"
    "uniform highp mat4 matrix;\n"
    "void main() {\n"
    "   col = colAttr;\n"
    "   gl_Position = matrix * posAttr;\n"
    "}\n";

static const char *fragmentShaderSource110 =
    "varying lowp vec4 col;\n"
    "void main() {\n"
    "   gl_FragColor = col;\n"
    "}\n";

static const char *vertexShaderSource =
    "#version 150\n"
    "in vec4 posAttr;\n"
    "in vec4 colAttr;\n"
    "out vec4 col;\n"
    "uniform mat4 matrix;\n"
    "void main() {\n"
    "   col = colAttr;\n"
    "   gl_Position = matrix * posAttr;\n"
    "}\n";

static const char *fragmentShaderSource =
    "#version 150\n"
    "in vec4 col;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "   fragColor = col;\n"
    "}\n";

static GLfloat vertices[] = {
    0.0f, 0.707f,
    -0.5f, -0.5f,
    0.5f, -0.5f
};

static GLfloat colors[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};

void RenderWindow::init()
{
    m_program = new QOpenGLShaderProgram(this);

    QSurfaceFormat format = m_context->format();
    bool useNewStyleShader = format.profile() == QSurfaceFormat::CoreProfile;
    // Try to handle 3.0 & 3.1 that do not have the core/compatibility profile concept 3.2+ has.
    // This may still fail since version 150 (3.2) is specified in the sources but it's worth a try.
    if (format.renderableType() == QSurfaceFormat::OpenGL && format.majorVersion() == 3 && format.minorVersion() <= 1)
        useNewStyleShader = !format.testOption(QSurfaceFormat::DeprecatedFunctions);
    if (m_forceGLSL110)
        useNewStyleShader = false;

    const char *vsrc = useNewStyleShader ? vertexShaderSource : vertexShaderSource110;
    const char *fsrc = useNewStyleShader ? fragmentShaderSource : fragmentShaderSource110;
    qDebug("Using version %s shader", useNewStyleShader ? "150" : "110");

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc)) {
        emit error(m_program->log());
        return;
    }
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc)) {
        emit error(m_program->log());
        return;
    }
    if (!m_program->link()) {
        emit error(m_program->log());
        return;
    }

    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");
    m_matrixUniform = m_program->uniformLocation("matrix");

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(vertices, sizeof(vertices) + sizeof(colors));
    m_vbo.write(sizeof(vertices), colors, sizeof(colors));
    m_vbo.release();

    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
    if (m_vao.isCreated()) // have VAO support, use it
        setupVertexAttribs();
}

void RenderWindow::setupVertexAttribs()
{
    m_vbo.bind();
    m_program->setAttributeBuffer(m_posAttr, GL_FLOAT, 0, 2);
    m_program->setAttributeBuffer(m_colAttr, GL_FLOAT, sizeof(vertices), 3);
    m_program->enableAttributeArray(m_posAttr);
    m_program->enableAttributeArray(m_colAttr);
    m_vbo.release();
}

bool RenderWindow::event(QEvent *ev)
{
    if (ev->type() == QEvent::UpdateRequest)
        render();
    return QWindow::event(ev);
}

void RenderWindow::render()
{
    if (!m_context->makeCurrent(this)) {
        emit error(tr("makeCurrent() failed"));
        return;
    }

    QOpenGLFunctions *f = m_context->functions();
    if (!m_initialized) {
        m_initialized = true;
        f->glEnable(GL_DEPTH_TEST);
        f->glClearColor(0, 0, 0, 1);
        init();
        emit ready();
    }

    if (!m_vbo.isCreated()) // init() failed, don't bother with trying to render
        return;

    const qreal retinaScale = devicePixelRatio();
    f->glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_program->bind();
    QMatrix4x4 matrix;
    matrix.perspective(60.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    matrix.translate(0.0f, 0.0f, -2.0f);
    matrix.rotate(m_angle, 0.0f, 1.0f, 0.0f);
    m_program->setUniformValue(m_matrixUniform, matrix);

    if (m_vao.isCreated())
        m_vao.bind();
    else // no VAO support, set the vertex attribute arrays now
        setupVertexAttribs();

    f->glDrawArrays(GL_TRIANGLES, 0, 3);

    m_vao.release();
    m_program->release();

    // swapInterval is 1 by default which means that swapBuffers() will (hopefully) block
    // and wait for vsync.
    m_context->swapBuffers(this);

    m_angle += 1.0f;

    requestUpdate();
}
