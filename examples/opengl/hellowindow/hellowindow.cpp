#include "hellowindow.h"

#include <QWindowContext>

#include <QTimer>

#include <qmath.h>

HelloWindow::HelloWindow()
    : colorIndex(0)
{
    setSurfaceType(OpenGLSurface);
    setWindowTitle(QLatin1String("Hello Window"));

    QWindowFormat format;
    format.setDepthBufferSize(16);
    format.setSamples(4);

    setWindowFormat(format);

    setGeometry(QRect(10, 10, 640, 480));

    create();

    initialize();

    QTimer *timer = new QTimer(this);
    timer->start(10);

    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
}

void HelloWindow::mousePressEvent(QMouseEvent *)
{
    updateColor();
}

void HelloWindow::resizeEvent(QResizeEvent *)
{
    glContext()->makeCurrent();

    glViewport(0, 0, geometry().width(), geometry().height());
}

void HelloWindow::updateColor()
{
    float colors[][4] =
    {
        { 0.4, 1.0, 0.0, 0.0 },
        { 0.0, 0.4, 1.0, 0.0 }
    };

    glContext()->makeCurrent();

    program.bind();
    program.setUniformValue(colorUniform, colors[colorIndex][0], colors[colorIndex][1], colors[colorIndex][2], colors[colorIndex][3]);
    program.release();

    colorIndex++;
    if (colorIndex >= sizeof(colors) / sizeof(colors[0]))
        colorIndex = 0;
}

void HelloWindow::render()
{
    if (!glContext())
        return;

    glContext()->makeCurrent();

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
    paintQtLogo();
    program.release();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glContext()->swapBuffers();

    m_fAngle += 1.0f;
}

void HelloWindow::paintQtLogo()
{
    program.enableAttributeArray(normalAttr);
    program.enableAttributeArray(vertexAttr);
    program.setAttributeArray(vertexAttr, vertices.constData());
    program.setAttributeArray(normalAttr, normals.constData());
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    program.disableAttributeArray(normalAttr);
    program.disableAttributeArray(vertexAttr);
}

void HelloWindow::initialize()
{
    glContext()->makeCurrent();

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
    updateColor();
}

void HelloWindow::createGeometry()
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

void HelloWindow::quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4)
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

void HelloWindow::extrude(qreal x1, qreal y1, qreal x2, qreal y2)
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
