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

QEglFSCompositor::QEglFSCompositor()
    : m_rootWindow(0)
{
    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(0);
    connect(&m_updateTimer, SIGNAL(timeout()), SLOT(renderAll()));
}

void QEglFSCompositor::schedule(QEglFSWindow *rootWindow)
{
    m_rootWindow = rootWindow;
    if (!m_updateTimer.isActive())
        m_updateTimer.start();
}

void QEglFSCompositor::renderAll()
{
    Q_ASSERT(m_rootWindow);
    QOpenGLContext *context = QEglFSBackingStore::makeRootCurrent(m_rootWindow);

    QEglFSScreen *screen = m_rootWindow->screen();
    QList<QEglFSWindow *> windows = screen->windows();
    for (int i = 0; i < windows.size(); ++i) {
        if (windows.at(i)->backingStore())
            render(windows.at(i), m_rootWindow);
    }

    context->swapBuffers(m_rootWindow->window());
    context->doneCurrent();
}

void QEglFSCompositor::render(QEglFSWindow *window, QEglFSWindow *rootWindow)
{
    QEglFSBackingStore *rootBackingStore = rootWindow->backingStore();
    rootBackingStore->m_program->bind();

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

    GLfloat x1 = (tl.x() / sr.width()) * 2 - 1;
    GLfloat x2 = (br.x() / sr.width()) * 2 - 1;
    GLfloat y1 = ((sr.height() - tl.y()) / sr.height()) * 2 - 1;
    GLfloat y2 = ((sr.height() - br.y()) / sr.height()) * 2 - 1;

    const GLfloat vertexCoordinates[] = {
        x1, y1,
        x2, y1,
        x2, y2,
        x1, y2
    };

    glViewport(0, 0, sr.width(), sr.height());

    glEnableVertexAttribArray(rootBackingStore->m_vertexCoordEntry);
    glEnableVertexAttribArray(rootBackingStore->m_textureCoordEntry);

    glVertexAttribPointer(rootBackingStore->m_vertexCoordEntry, 2, GL_FLOAT, GL_FALSE, 0, vertexCoordinates);
    glVertexAttribPointer(rootBackingStore->m_textureCoordEntry, 2, GL_FLOAT, GL_FALSE, 0, textureCoordinates);

    glBindTexture(GL_TEXTURE_2D, window->backingStore()->m_texture);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    rootBackingStore->m_program->release();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(rootBackingStore->m_vertexCoordEntry);
    glDisableVertexAttribArray(rootBackingStore->m_textureCoordEntry);
}

static QEglFSCompositor *compositor = 0;

QEglFSCompositor *QEglFSCompositor::instance()
{
    if (!compositor)
        compositor = new QEglFSCompositor;
    return compositor;
}

QEglFSBackingStore::QEglFSBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_window(static_cast<QEglFSWindow *>(window->handle()))
    , m_context(0)
    , m_texture(0)
    , m_program(0)
{
    m_window->setBackingStore(this);
}

QEglFSBackingStore::~QEglFSBackingStore()
{
    delete m_program;
    delete m_context;
}

QPaintDevice *QEglFSBackingStore::paintDevice()
{
    return &m_image;
}

void QEglFSBackingStore::updateTexture()
{
    glBindTexture(GL_TEXTURE_2D, m_texture);

    if (!m_dirty.isNull()) {
        QRegion fixed;
        QRect imageRect = m_image.rect();
        m_dirty |= imageRect;

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
}

void QEglFSBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglBackingStore::flush %p", window);
#endif

    m_window->create();
    QEglFSWindow *rootWin = m_window->screen()->rootWindow();
    if (rootWin) {
        makeRootCurrent(rootWin);
        updateTexture();
        QEglFSCompositor::instance()->schedule(rootWin);
    }
}

void QEglFSBackingStore::makeCurrent()
{
    Q_ASSERT(m_window->hasNativeWindow());

    QWindow *wnd = window();
    if (!m_context) {
        m_context = new QOpenGLContext;
        m_context->setFormat(wnd->requestedFormat());
        m_context->setScreen(wnd->screen());
        m_context->create();
    }

    m_context->makeCurrent(wnd);
}

QOpenGLContext *QEglFSBackingStore::makeRootCurrent(QEglFSWindow *rootWin)
{
    Q_ASSERT(rootWin->hasNativeWindow() && rootWin->isRasterRoot());

    QEglFSBackingStore *rootBackingStore = rootWin->backingStore();
    rootBackingStore->makeCurrent();
    if (!rootBackingStore->m_program) {
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

        rootBackingStore->m_program = new QOpenGLShaderProgram;

        rootBackingStore->m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, textureVertexProgram);
        rootBackingStore->m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, textureFragmentProgram);
        rootBackingStore->m_program->link();

        rootBackingStore->m_vertexCoordEntry = rootBackingStore->m_program->attributeLocation("vertexCoordEntry");
        rootBackingStore->m_textureCoordEntry = rootBackingStore->m_program->attributeLocation("textureCoordEntry");
    }
    return rootBackingStore->m_context;
}

void QEglFSBackingStore::beginPaint(const QRegion &rgn)
{
    m_dirty |= rgn;
}

void QEglFSBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    m_image = QImage(size, QImage::Format_RGB32);
    m_window->create();
    makeRootCurrent(m_window->screen()->rootWindow());

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
