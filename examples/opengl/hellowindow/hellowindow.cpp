#include "hellowindow.h"

#include <QGuiGLContext>

#include <QTimer>

#include <qmath.h>

Renderer::Renderer()
    : m_initialized(false)
{
    m_format.setDepthBufferSize(16);
    m_format.setSamples(4);

    m_context = new QGuiGLContext(m_format);
}

QGuiGLFormat Renderer::format() const
{
    return m_format;
}

HelloWindow::HelloWindow(Renderer *renderer)
    : m_colorIndex(0)
    , m_renderer(renderer)
{
    setSurfaceType(OpenGLSurface);
    setWindowTitle(QLatin1String("Hello Window"));

    setGLFormat(renderer->format());

    setGeometry(QRect(10, 10, 640, 480));

    create();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
    timer->start(10);

    updateColor();
}

void HelloWindow::mousePressEvent(QMouseEvent *)
{
    updateColor();
}

void HelloWindow::updateColor()
{
    QColor colors[] =
    {
        QColor(100, 255, 0),
        QColor(0, 100, 255)
    };

    m_color = colors[m_colorIndex];

    m_colorIndex++;
    if (m_colorIndex >= int(sizeof(colors) / sizeof(colors[0])))
        m_colorIndex = 0;
}

void HelloWindow::render()
{
    if (glSurface())
        m_renderer->render(glSurface(), m_color, geometry().size());
}

void Renderer::render(QPlatformGLSurface *surface, const QColor &color, const QSize &viewSize)
{
    m_context->makeCurrent(surface);

    if (!m_initialized) {
        initialize();
        m_initialized = true;
    }

    glViewport(0, 0, viewSize.width(), viewSize.height());

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glFrontFace(GL_CW);
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    QMatrix4x4 modelview;
    modelview.rotate(m_fAngle, 0.0f, 1.0f, 0.0f);
    modelview.rotate(m_fAngle, 1.0f, 0.0f, 0.0f);
    modelview.rotate(m_fAngle, 0.0f, 0.0f, 1.0f);
    modelview.translate(0.0f, -0.2f, 0.0f);

    program.bind();
    program.setUniformValue(matrixUniform, modelview);
    program.setUniformValue(colorUniform, color);
    paintQtLogo();
    program.release();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_context->swapBuffers(surface);

    m_fAngle += 1.0f;
}

void Renderer::paintQtLogo()
{
    program.enableAttributeArray(normalAttr);
    program.enableAttributeArray(vertexAttr);
    program.setAttributeArray(vertexAttr, vertices.constData());
    program.setAttributeArray(normalAttr, normals.constData());
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    program.disableAttributeArray(normalAttr);
    program.disableAttributeArray(vertexAttr);
}

void Renderer::initialize()
{
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    QGLShader *vshader = new QGLShader(QGLShader::Vertex, this);
    const char *vsrc =
        "attribute highp vec4 vertex;\n"
        "attribute mediump vec3 normal;\n"
        "uniform mediump mat4 matrix;\n"
        "uniform lowp vec4 sourceColor;\n"
        "varying mediump vec4 color;\n"
        "void main(void)\n"
        "{\n"
        "    vec3 toLight = normalize(vec3(0.0, 0.3, 1.0));\n"
        "    float angle = max(dot(normal, toLight), 0.0);\n"
        "    vec3 col = sourceColor.rgb;\n"
        "    color = vec4(col * 0.2 + col * 0.8 * angle, 1.0);\n"
        "    color = clamp(color, 0.0, 1.0);\n"
        "    gl_Position = matrix * vertex;\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QGLShader *fshader = new QGLShader(QGLShader::Fragment, this);
    const char *fsrc =
        "varying mediump vec4 color;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = color;\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    program.addShader(vshader);
    program.addShader(fshader);
    program.link();

    vertexAttr = program.attributeLocation("vertex");
    normalAttr = program.attributeLocation("normal");
    matrixUniform = program.uniformLocation("matrix");
    colorUniform = program.uniformLocation("sourceColor");

    m_fAngle = 0;
    createGeometry();
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

    const qreal Pi = 3.14159f;
    const int NumSectors = 100;

    for (int i = 0; i < NumSectors; ++i) {
        qreal angle1 = (i * 2 * Pi) / NumSectors;
        qreal x5 = 0.30 * qSin(angle1);
        qreal y5 = 0.30 * qCos(angle1);
        qreal x6 = 0.20 * qSin(angle1);
        qreal y6 = 0.20 * qCos(angle1);

        qreal angle2 = ((i + 1) * 2 * Pi) / NumSectors;
        qreal x7 = 0.20 * qSin(angle2);
        qreal y7 = 0.20 * qCos(angle2);
        qreal x8 = 0.30 * qSin(angle2);
        qreal y8 = 0.30 * qCos(angle2);

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
