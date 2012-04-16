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

#include "qeglfsbackingstore.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QOpenGLShaderProgram>

#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

QEglFSBackingStore::QEglFSBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_context(new QOpenGLContext)
#ifdef EGLFS_BACKINGSTORE_USE_IMAGE
    , m_texture(0)
    , m_program(0)
#endif
{
    m_context->setFormat(window->requestedFormat());
    m_context->setScreen(window->screen());
    m_context->create();
}

QEglFSBackingStore::~QEglFSBackingStore()
{
    delete m_context;
}

QPaintDevice *QEglFSBackingStore::paintDevice()
{
#ifdef EGLFS_BACKINGSTORE_USE_IMAGE
    return &m_image;
#else
    return m_device;
#endif
}

void QEglFSBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglBackingStore::flush %p", window);
#endif

#ifdef EGLFS_BACKINGSTORE_USE_IMAGE
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
            "   gl_FragColor = texture2D(texture, textureCoord);\n"
            "}\n";

        m_program = new QOpenGLShaderProgram;

        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, textureVertexProgram);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, textureFragmentProgram);
        m_program->link();

        m_vertexCoordEntry = m_program->attributeLocation("vertexCoordEntry");
        m_textureCoordEntry = m_program->attributeLocation("textureCoordEntry");
    }

    m_program->bind();

    const GLfloat textureCoordinates[] = {
        0, 1,
        1, 1,
        1, 0,
        0, 0
    };

    QRectF r = window->geometry();
    QRectF sr = window->screen()->geometry();

    GLfloat x1 = (r.left() / sr.width()) * 2 - 1;
    GLfloat x2 = (r.right() / sr.width()) * 2 - 1;
    GLfloat y1 = (r.top() / sr.height()) * 2 - 1;
    GLfloat y2 = (r.bottom() / sr.height()) * 2 - 1;

    const GLfloat vertexCoordinates[] = {
        x1, y1,
        x2, y1,
        x2, y2,
        x1, y2
    };

    glEnableVertexAttribArray(m_vertexCoordEntry);
    glEnableVertexAttribArray(m_textureCoordEntry);

    glVertexAttribPointer(m_vertexCoordEntry, 2, GL_FLOAT, GL_FALSE, 0, vertexCoordinates);
    glVertexAttribPointer(m_textureCoordEntry, 2, GL_FLOAT, GL_FALSE, 0, textureCoordinates);

    glBindTexture(GL_TEXTURE_2D, m_texture);

    foreach (const QRect &rect, m_dirty.rects()) {
        if (rect == m_image.rect()) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE, m_image.constBits());
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                m_image.copy(rect).constBits());
        }
    }

    m_dirty = QRegion();

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);

    glDisableVertexAttribArray(m_vertexCoordEntry);
    glDisableVertexAttribArray(m_textureCoordEntry);
#endif

    m_context->swapBuffers(window);
}

void QEglFSBackingStore::makeCurrent()
{
    // needed to prevent QOpenGLContext::makeCurrent() from failing
    window()->setSurfaceType(QSurface::OpenGLSurface);

    m_context->makeCurrent(window());
}

void QEglFSBackingStore::beginPaint(const QRegion &rgn)
{
    makeCurrent();

#ifdef EGLFS_BACKINGSTORE_USE_IMAGE
    m_dirty = m_dirty | rgn;
#else
    Q_UNUSED(rgn);
    m_device = new QOpenGLPaintDevice(window()->size());
#endif
}

void QEglFSBackingStore::endPaint()
{
#ifndef EGLFS_BACKINGSTORE_USE_IMAGE
    delete m_device;
#endif
}

void QEglFSBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

#ifdef EGLFS_BACKINGSTORE_USE_IMAGE
    m_image = QImage(size, QImage::Format_RGB32);
    makeCurrent();
    if (m_texture)
        glDeleteTextures(1, &m_texture);
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
#else
    Q_UNUSED(size);
#endif
}

QT_END_NAMESPACE
