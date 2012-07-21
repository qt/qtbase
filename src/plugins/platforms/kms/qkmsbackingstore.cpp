/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qkmsbackingstore.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

QKmsBackingStore::QKmsBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_context(new QOpenGLContext)
    , m_texture(0)
    , m_program(0)
{
    m_context->setFormat(window->requestedFormat());
    m_context->setScreen(window->screen());
    m_context->create();
    window->setSurfaceType(QSurface::OpenGLSurface);
}

QKmsBackingStore::~QKmsBackingStore()
{
    delete m_program;
    if (m_texture)
        glDeleteTextures(1, &m_texture);

    delete m_context;
}

QPaintDevice *QKmsBackingStore::paintDevice()
{
    return &m_image;
}

void QKmsBackingStore::beginPaint(const QRegion &)
{
}

void QKmsBackingStore::endPaint()
{
}

void QKmsBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region)
    Q_UNUSED(offset)

    m_context->makeCurrent(window);

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
            "void main() {\n"
            "   gl_FragColor = texture2D(texture, textureCoord).bgra;\n"
            "}\n";

        m_program = new QOpenGLShaderProgram;

        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, textureVertexProgram);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, textureFragmentProgram);
        m_program->bindAttributeLocation("vertexCoordEntry", 0);
        m_program->bindAttributeLocation("textureCoordEntry", 1);
        m_program->link();
    }

    m_program->bind();

    QRectF r = window->geometry();
    QRectF sr = window->screen()->geometry();

    GLfloat x1 = (r.left() / sr.width()) * 2 - 1;
    GLfloat x2 = (r.right() / sr.width()) * 2 - 1;
    GLfloat y1 = -1 * ((r.top() / sr.height()) * 2 - 1);
    GLfloat y2 = -1 * ((r.bottom() / sr.height()) * 2 - 1);

    const GLfloat vertexCoordinates[] = {
        x1, y1,
        x2, y1,
        x2, y2,
        x1, y2
    };

    const GLfloat textureCoordinates[] = {
        0, 0,
        1, 0,
        1, 1,
        0, 1
    };

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertexCoordinates);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, textureCoordinates);

    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image.width(), m_image.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, m_image.constScanLine(0));

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    m_program->release();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    m_context->swapBuffers(window);

    m_context->doneCurrent();
}

void QKmsBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents)

    m_image = QImage(size, QImage::Format_RGB32);

    m_context->makeCurrent(window());

    if (m_texture)
        glDeleteTextures(1, &m_texture);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

}

QT_END_NAMESPACE
