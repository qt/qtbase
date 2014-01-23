/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFramebufferObject>

#include "qeglcompositor_p.h"
#include "qeglplatformwindow_p.h"
#include "qeglplatformscreen_p.h"

QT_BEGIN_NAMESPACE

static QEGLCompositor *compositor = 0;

QEGLCompositor::QEGLCompositor()
    : m_context(0),
      m_window(0),
      m_program(0)
{
    Q_ASSERT(!compositor);
    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(0);
    connect(&m_updateTimer, SIGNAL(timeout()), SLOT(renderAll()));
}

QEGLCompositor::~QEGLCompositor()
{
    Q_ASSERT(compositor == this);
    delete m_program;
    compositor = 0;
}

void QEGLCompositor::schedule(QOpenGLContext *context, QEGLPlatformWindow *window)
{
    m_context = context;
    m_window = window;
    if (!m_updateTimer.isActive())
        m_updateTimer.start();
}

void QEGLCompositor::renderAll()
{
    Q_ASSERT(m_context && m_window);
    m_context->makeCurrent(m_window->window());
    ensureProgram();
    m_program->bind();

    QEGLPlatformScreen *screen = static_cast<QEGLPlatformScreen *>(m_window->screen());
    QList<QEGLPlatformWindow *> windows = screen->windows();
    for (int i = 0; i < windows.size(); ++i) {
        QEGLPlatformWindow *window = windows.at(i);
        uint texture = window->texture();
        if (texture)
            render(window, texture, window->isRaster());
    }

    m_program->release();
    m_context->swapBuffers(m_window->window());
}

void QEGLCompositor::ensureProgram()
{
    if (!m_program) {
        static const char *textureVertexProgram =
            "attribute highp vec2 vertexCoordEntry;\n"
            "attribute highp vec2 textureCoordEntry;\n"
            "varying highp vec2 textureCoord;\n"
            "void main() {\n"
            "   textureCoord = textureCoordEntry;\n"
            "   gl_Position = vec4(vertexCoordEntry, 0.0, 1.0);\n"
            "}\n";

        static const char *textureFragmentProgram =
            "uniform sampler2D texture;\n"
            "varying highp vec2 textureCoord;\n"
            "uniform bool isRaster;\n"
            "void main() {\n"
            "   lowp vec4 c = texture2D(texture, textureCoord);\n"
            "   gl_FragColor = isRaster ? c.bgra : c.rgba;\n"
            "}\n";

        m_program = new QOpenGLShaderProgram;

        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, textureVertexProgram);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, textureFragmentProgram);
        m_program->link();

        m_vertexCoordEntry = m_program->attributeLocation("vertexCoordEntry");
        m_textureCoordEntry = m_program->attributeLocation("textureCoordEntry");
        m_isRasterEntry = m_program->uniformLocation("isRaster");
    }
}

void QEGLCompositor::render(QEGLPlatformWindow *window, uint texture, bool raster)
{
    const GLfloat textureCoordinates[] = {
        0, 0,
        1, 0,
        1, 1,
        0, 1
    };

    QRectF sr = window->screen()->geometry();
    QRect r = window->window()->geometry();
    QPoint tl = r.topLeft();
    QPoint br = r.bottomRight();

    // Map to [-1,1]
    GLfloat x1 = (tl.x() / sr.width()) * 2 - 1;
    GLfloat y1 = ((sr.height() - tl.y()) / sr.height()) * 2 - 1;
    GLfloat x2 = ((br.x() + 1) / sr.width()) * 2 - 1;
    GLfloat y2 = ((sr.height() - (br.y() + 1)) / sr.height()) * 2 - 1;

    if (!raster)
        qSwap(y1, y2);

    const GLfloat vertexCoordinates[] = {
        x1, y1,
        x2, y1,
        x2, y2,
        x1, y2
    };

    glViewport(0, 0, sr.width(), sr.height());

    m_program->enableAttributeArray(m_vertexCoordEntry);
    m_program->enableAttributeArray(m_textureCoordEntry);

    m_program->setAttributeArray(m_vertexCoordEntry, vertexCoordinates, 2);
    m_program->setAttributeArray(m_textureCoordEntry, textureCoordinates, 2);

    glBindTexture(GL_TEXTURE_2D, texture);

    m_program->setUniformValue(m_isRasterEntry, raster);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    m_program->enableAttributeArray(m_textureCoordEntry);
    m_program->enableAttributeArray(m_vertexCoordEntry);
}

QEGLCompositor *QEGLCompositor::instance()
{
    if (!compositor)
        compositor = new QEGLCompositor;
    return compositor;
}

void QEGLCompositor::destroy()
{
    delete compositor;
    compositor = 0;
}

QT_END_NAMESPACE
