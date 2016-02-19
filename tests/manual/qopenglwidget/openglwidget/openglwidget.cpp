/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define GL_GLEXT_PROTOTYPES

#include "openglwidget.h"
#include <QtWidgets/private/qwidget_p.h>
#include <QOpenGLFramebufferObject>
#include <QWindow>
#include <qpa/qplatformwindow.h>
#include <QDebug>
#include <QTimer>

#include <QtGui/QOpenGLFunctions>
#include <QtGui/QGuiApplication>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QScreen>

#include <QtCore/qmath.h>
#include <qopengl.h>

class OpenGLWidgetPrivate : protected QOpenGLFunctions
{
public:
    OpenGLWidgetPrivate(QWidget *q)
        : m_program(0), m_frame(0), q(q)
    {

    }

    void initialize();
    void render();


    int width() {return w;}
    int height() {return h;}

    GLuint m_posAttr;
    GLuint m_colAttr;
    GLuint m_matrixUniform;

    QOpenGLShaderProgram *m_program;
    int m_frame;

    int w,h;
    QWidget *q;

    int m_interval;
    QVector3D m_rotAxis;

    float clearColor[3];
};


OpenGLWidget::OpenGLWidget(int interval, const QVector3D &rotAxis, QWidget *parent)
    : QOpenGLWidget(parent)
{
    d.reset(new OpenGLWidgetPrivate(this));
    d->clearColor[0] = d->clearColor[1] = d->clearColor[2] = 0.0f;
    d->m_interval = interval;
    d->m_rotAxis = rotAxis;
    if (interval > 0) {
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(update()));
        timer->start(interval);
    }
}

OpenGLWidget::~OpenGLWidget()
{

}

void OpenGLWidget::initializeGL()
{
//    qDebug("*initializeGL*");
    d->initialize();
}

void OpenGLWidget::resizeGL(int w, int h)
{
//    qDebug("*resizeGL*");
    d->w = w;
    d->h = h;
}
void OpenGLWidget::paintGL()
{
//    qDebug("*paintGL* %d", d->m_frame);
    d->render();
}


static const char *vertexShaderSource =
    "attribute highp vec4 posAttr;\n"
    "attribute lowp vec4 colAttr;\n"
    "varying lowp vec4 col;\n"
    "uniform highp mat4 matrix;\n"
    "void main() {\n"
    "   col = colAttr;\n"
    "   gl_Position = matrix * posAttr;\n"
    "}\n";

static const char *fragmentShaderSource =
    "varying lowp vec4 col;\n"
    "void main() {\n"
    "   gl_FragColor = col;\n"
    "}\n";

void OpenGLWidgetPrivate::initialize()
{
    initializeOpenGLFunctions();
    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();
    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");
    m_matrixUniform = m_program->uniformLocation("matrix");
}

void OpenGLWidgetPrivate::render()
{
    const qreal retinaScale = q->devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_program->bind();

    QMatrix4x4 matrix;
    matrix.perspective(60.0f, 4.0f/3.0f, 0.1f, 100.0f);
    matrix.translate(0, 0, -2);
    const qreal angle = 100.0f * m_frame / 30;
    matrix.rotate(angle, m_rotAxis);

    m_program->setUniformValue(m_matrixUniform, matrix);

    GLfloat vertices[] = {
        0.0f, 0.707f,
        -0.5f, -0.5f,
        0.5f, -0.5f
    };

    GLfloat colors[] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    m_program->release();

    ++m_frame;

    if (m_interval <= 0)
        q->update();
}

void OpenGLWidget::setClearColor(const float *c)
{
    d->clearColor[0] = c[0];
    d->clearColor[1] = c[1];
    d->clearColor[2] = c[2];
}
