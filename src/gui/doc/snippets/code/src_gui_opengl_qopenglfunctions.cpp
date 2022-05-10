// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QOpenGLFunctions>
#include <QtOpenGL/QOpenGLWindow>

#include <QSurface>
#include <QWidget>
#include <QWindow>

namespace src_gui_opengl_qopenglfunctions {

//! [0]
class MyGLWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit MyGLWindow(QScreen *screen = nullptr);

protected:
    void initializeGL();
    void paintGL();

    QOpenGLContext *m_context;
};

MyGLWindow::MyGLWindow(QScreen *screen)
  : QWindow(screen)
{
    setSurfaceType(OpenGLSurface);
    create();

    // Create an OpenGL context
    m_context = new QOpenGLContext;
    m_context->create();

    // Setup scene and render it
    initializeGL();
    paintGL();
};

void MyGLWindow::initializeGL()
{
    m_context->makeCurrent(this);
    initializeOpenGLFunctions();
}
//! [0]


int textureId = 0;

//! [1]
void MyGLWindow::paintGL()
{
    m_context->makeCurrent(this);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureId);
    // ...
    m_context->swapBuffers(this);
    m_context->doneCurrent();
}
//! [1]


void wrapper0() {
//! [2]
QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
glFuncs.glActiveTexture(GL_TEXTURE1);
//! [2]
} // wrapper0


void wrapper1() {
//! [3]
QOpenGLFunctions *glFuncs = QOpenGLContext::currentContext()->functions();
glFuncs->glActiveTexture(GL_TEXTURE1);
//! [3]


//! [4]
QOpenGLFunctions funcs(QOpenGLContext::currentContext());
bool npot = funcs.hasOpenGLFeature(QOpenGLFunctions::NPOTTextures);
//! [4]

Q_UNUSED(npot);
} // wrapper1
} // src_gui_opengl_qopenglfunctions
