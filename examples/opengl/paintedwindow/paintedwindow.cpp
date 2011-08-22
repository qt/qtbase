#include "paintedwindow.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QTimer>

#include <qmath.h>

PaintedWindow::PaintedWindow()
    : m_fbo(0)
{
    QSurfaceFormat format;
    format.setStencilBufferSize(8);

    setSurfaceType(QWindow::OpenGLSurface);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setFormat(format);

    create();

    m_context = new QOpenGLContext(this);
    m_context->setFormat(format);
    m_context->create();

    QTimer *timer = new QTimer(this);
    timer->setInterval(16);
    connect(timer, SIGNAL(timeout()), this, SLOT(paint()));
    timer->start();

    m_context->makeCurrent(this);

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    const char *vsrc =
        "attribute highp vec2 vertexCoordsInput;\n"
        "attribute mediump vec2 texCoordsInput;\n"
        "varying mediump vec2 texCoords;\n"
        "void main(void)\n"
        "{\n"
        "    texCoords = texCoordsInput;\n"
        "    gl_Position = vec4(vertexCoordsInput, 0, 1);\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char *fsrc =
        "uniform sampler2D tex;\n"
        "varying mediump vec2 texCoords;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = texture2D(tex, texCoords);\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    m_program = new QOpenGLShaderProgram;
    m_program->addShader(vshader);
    m_program->addShader(fshader);
    m_program->link();

    m_vertexAttribute = m_program->attributeLocation("vertexCoordsInput");
    m_texCoordsAttribute = m_program->attributeLocation("texCoordsInput");
}

void PaintedWindow::resizeEvent(QResizeEvent *)
{
    m_context->makeCurrent(this);

    delete m_fbo;
    m_fbo = new QOpenGLFramebufferObject(size());
}

void PaintedWindow::paint()
{
    if (!m_fbo)
        return;

    m_context->makeCurrent(this);

    QPainterPath path;
    path.addEllipse(0, 0, m_fbo->width(), m_fbo->height());

    QPainter painter;
    painter.begin(m_fbo);
    painter.fillRect(0, 0, m_fbo->width(), m_fbo->height(), Qt::white);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillPath(path, Qt::blue);
    painter.end();

    glViewport(0, 0, width(), height());

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLfloat texCoords[] = { 0, 0, 1, 0, 0, 1,
                           1, 0, 1, 1, 0, 1 };

    GLfloat vertexCoords[] = { -1, -1, 1, -1, -1, 1,
                              1, -1, 1, 1, -1, 1 };

    m_program->bind();

    m_context->functions()->glEnableVertexAttribArray(m_vertexAttribute);
    m_context->functions()->glEnableVertexAttribArray(m_texCoordsAttribute);

    m_context->functions()->glVertexAttribPointer(m_vertexAttribute, 2, GL_FLOAT, GL_FALSE, 0, vertexCoords);
    m_context->functions()->glVertexAttribPointer(m_texCoordsAttribute, 2, GL_FLOAT, GL_FALSE, 0, texCoords);

    glBindTexture(GL_TEXTURE_2D, m_fbo->texture());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);

    m_context->functions()->glDisableVertexAttribArray(m_vertexAttribute);
    m_context->functions()->glDisableVertexAttribArray(m_texCoordsAttribute);

    m_program->release();

    m_context->swapBuffers(this);
}
