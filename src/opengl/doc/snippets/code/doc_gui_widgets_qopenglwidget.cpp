// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
class MyGLWidget : public QOpenGLWidget
{
public:
    MyGLWidget(QWidget *parent) : QOpenGLWidget(parent) { }

protected:
    void initializeGL() override
    {
        // Set up the rendering context, load shaders and other resources, etc.:
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        f->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        ...
    }

    void resizeGL(int w, int h) override
    {
        // Update projection matrix and other size related settings:
        m_projection.setToIdentity();
        m_projection.perspective(45.0f, w / float(h), 0.01f, 100.0f);
        ...
    }

    void paintGL() override
    {
        // Draw the scene:
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        f->glClear(GL_COLOR_BUFFER_BIT);
        ...
    }

};
//! [0]

//! [1]
class MyGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    ...
    void initializeGL() override
    {
        initializeOpenGLFunctions();
        glClearColor(...);
        ...
    }
    ...
};
//! [1]

//! [2]
QOpenGLWidget *widget = new QOpenGLWidget(parent);
QSurfaceFormat format;
format.setDepthBufferSize(24);
format.setStencilBufferSize(8);
format.setVersion(3, 2);
format.setProfile(QSurfaceFormat::CoreProfile);
widget->setFormat(format); // must be called before the widget or its parent window gets shown
//! [2]

//! [3]
    ...
    void paintGL() override
    {
        QOpenGLFunctions_3_2_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_2_Core>();
        ...
        f->glDrawArraysInstanced(...);
        ...
    }
    ...
//! [3]

//! [4]
class MyGLWidget : public QOpenGLWidget
{
    ...

private:
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo;
    QOpenGLShaderProgram *m_program;
    QOpenGLShader *m_shader;
    QOpenGLTexture *m_texture;
};

MyGLWidget::MyGLWidget()
    : m_program(0), m_shader(0), m_texture(0)
{
    // No OpenGL resource initialization is done here.
}

MyGLWidget::~MyGLWidget()
{
    // Make sure the context is current and then explicitly
    // destroy all underlying OpenGL resources.
    makeCurrent();

    delete m_texture;
    delete m_shader;
    delete m_program;

    m_vbo.destroy();
    m_vao.destroy();

    doneCurrent();
}

void MyGLWidget::initializeGL()
{
    m_vao.create();
    if (m_vao.isCreated())
        m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(...);

    m_texture = new QOpenGLTexture(QImage(...));

    m_shader = new QOpenGLShader(...);
    m_program = new QOpenGLShaderProgram(...);

    ...
}
//! [4]

//! [5]
MyGLWidget::~MyGLWidget()
{
    cleanup();
}

void MyGLWidget::initializeGL()
{
    ...
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &MyGLWidget::cleanup);
}

void MyGLWidget::cleanup()
{
    makeCurrent();
    delete m_texture;
    m_texture = 0;
    ...
    doneCurrent();
    disconnect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &MyGLWidget::cleanup);
}
//! [5]

//! [6]
int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    MyWidget widget;
    widget.show();

    return app.exec();
}
//! [6]
