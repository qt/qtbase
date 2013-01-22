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

#include "qeglfsbackingstore.h"
#include "qeglfscursor.h"
#include "qeglfswindow.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QOpenGLShaderProgram>

#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

QEglFSBackingStore::QEglFSBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_context(new QOpenGLContext)
    , m_texture(0)
    , m_program(0)
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
    return &m_image;
}

void QEglFSBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    makeCurrent();

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglBackingStore::flush %p", window);
#endif

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

    if (!m_dirty.isNull()) {
        QRect imageRect = m_image.rect();

        QRegion fixed;
        foreach (const QRect &rect, m_dirty.rects()) {
            // intersect with image rect to be sure
            QRect r = imageRect & rect;

            // if the rect is wide enough it's cheaper to just
            // extend it instead of doing an image copy
            if (r.width() >= imageRect.width() / 2) {
                r.setX(0);
                r.setWidth(imageRect.width());
            }

            fixed |= r;
        }

        foreach (const QRect &rect, fixed.rects()) {
            // if the sub-rect is full-width we can pass the image data directly to
            // OpenGL instead of copying, since there's no gap between scanlines
            if (rect.width() == imageRect.width()) {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE, m_image.constScanLine(rect.y()));
            } else {
                glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                    m_image.copy(rect).constBits());
            }
        }

        m_dirty = QRegion();
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    m_program->release();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(m_vertexCoordEntry);
    glDisableVertexAttribArray(m_textureCoordEntry);

    // draw the cursor
    if (QEglFSCursor *cursor = static_cast<QEglFSCursor *>(window->screen()->handle()->cursor()))
        cursor->paintOnScreen();

    m_context->swapBuffers(window);

    m_context->doneCurrent();
}

void QEglFSBackingStore::makeCurrent()
{
    // needed to prevent QOpenGLContext::makeCurrent() from failing
    window()->setSurfaceType(QSurface::OpenGLSurface);
    (static_cast<QEglFSWindow *>(window()->handle()))->create();
    m_context->makeCurrent(window());
}

void QEglFSBackingStore::beginPaint(const QRegion &rgn)
{
    m_dirty = m_dirty | rgn;
}

void QEglFSBackingStore::endPaint()
{
}

void QEglFSBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

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
}

QT_END_NAMESPACE
