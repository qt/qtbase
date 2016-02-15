/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QWindow>
#include <QtGui/QMatrix4x4>
#include <qpa/qplatformbackingstore.h>

#include "qopenglcompositor_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLCompositor
    \brief A generic OpenGL-based compositor
    \since 5.4
    \internal
    \ingroup qpa

    This class provides a lightweight compositor that maintains the
    basic stacking order of windows and composites them by drawing
    textured quads via OpenGL.

    It it meant to be used by platform plugins that run without a
    windowing system.

    It is up to the platform plugin to manage the lifetime of the
    compositor (instance(), destroy()), set the correct destination
    context and window as early as possible (setTarget()),
    register the composited windows as they are shown, activated,
    raised and lowered (addWindow(), moveToTop(), etc.), and to
    schedule repaints (update()).

    \note To get support for QWidget-based windows, just use
    QOpenGLCompositorBackingStore. It will automatically create
    textures from the raster-rendered content and trigger the
    necessary repaints.
 */

static QOpenGLCompositor *compositor = 0;

QOpenGLCompositor::QOpenGLCompositor()
    : m_context(0),
      m_targetWindow(0)
{
    Q_ASSERT(!compositor);
    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(0);
    connect(&m_updateTimer, SIGNAL(timeout()), SLOT(handleRenderAllRequest()));
}

QOpenGLCompositor::~QOpenGLCompositor()
{
    Q_ASSERT(compositor == this);
    m_blitter.destroy();
    compositor = 0;
}

void QOpenGLCompositor::setTarget(QOpenGLContext *context, QWindow *targetWindow)
{
    m_context = context;
    m_targetWindow = targetWindow;
}

void QOpenGLCompositor::update()
{
    if (!m_updateTimer.isActive())
        m_updateTimer.start();
}

QImage QOpenGLCompositor::grab()
{
    Q_ASSERT(m_context && m_targetWindow);
    m_context->makeCurrent(m_targetWindow);
    QScopedPointer<QOpenGLFramebufferObject> fbo(new QOpenGLFramebufferObject(m_targetWindow->geometry().size()));
    renderAll(fbo.data());
    return fbo->toImage();
}

void QOpenGLCompositor::handleRenderAllRequest()
{
    Q_ASSERT(m_context && m_targetWindow);
    m_context->makeCurrent(m_targetWindow);
    renderAll(0);
}

void QOpenGLCompositor::renderAll(QOpenGLFramebufferObject *fbo)
{
    if (fbo)
        fbo->bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    const QRect targetWindowRect(QPoint(0, 0), m_targetWindow->geometry().size());
    glViewport(0, 0, targetWindowRect.width(), targetWindowRect.height());

    if (!m_blitter.isCreated())
        m_blitter.create();

    m_blitter.bind();

    for (int i = 0; i < m_windows.size(); ++i)
        m_windows.at(i)->beginCompositing();

    for (int i = 0; i < m_windows.size(); ++i)
        render(m_windows.at(i));

    m_blitter.release();
    if (!fbo)
        m_context->swapBuffers(m_targetWindow);
    else
        fbo->release();

    for (int i = 0; i < m_windows.size(); ++i)
        m_windows.at(i)->endCompositing();
}

struct BlendStateBinder
{
    BlendStateBinder() : m_blend(false) {
        glDisable(GL_BLEND);
    }
    void set(bool blend) {
        if (blend != m_blend) {
            if (blend) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            } else {
                glDisable(GL_BLEND);
            }
            m_blend = blend;
        }
    }
    ~BlendStateBinder() {
        if (m_blend)
            glDisable(GL_BLEND);
    }
    bool m_blend;
};

static inline QRect toBottomLeftRect(const QRect &topLeftRect, int windowHeight)
{
    return QRect(topLeftRect.x(), windowHeight - topLeftRect.bottomRight().y() - 1,
                 topLeftRect.width(), topLeftRect.height());
}

static void clippedBlit(const QPlatformTextureList *textures, int idx, const QRect &targetWindowRect, QOpenGLTextureBlitter *blitter)
{
    const QRect clipRect = textures->clipRect(idx);
    if (clipRect.isEmpty())
        return;

    const QRect rectInWindow = textures->geometry(idx);
    const QRect clippedRectInWindow = rectInWindow & clipRect.translated(rectInWindow.topLeft());
    const QRect srcRect = toBottomLeftRect(clipRect, rectInWindow.height());

    const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(clippedRectInWindow, targetWindowRect);
    const QMatrix3x3 source = QOpenGLTextureBlitter::sourceTransform(srcRect, rectInWindow.size(),
                                                                     QOpenGLTextureBlitter::OriginBottomLeft);

    blitter->blit(textures->textureId(idx), target, source);
}

void QOpenGLCompositor::render(QOpenGLCompositorWindow *window)
{
    const QPlatformTextureList *textures = window->textures();
    if (!textures)
        return;

    const QRect targetWindowRect(QPoint(0, 0), m_targetWindow->geometry().size());
    float currentOpacity = 1.0f;
    BlendStateBinder blend;

    for (int i = 0; i < textures->count(); ++i) {
        uint textureId = textures->textureId(i);
        const float opacity = window->sourceWindow()->opacity();
        if (opacity != currentOpacity) {
            currentOpacity = opacity;
            m_blitter.setOpacity(currentOpacity);
        }

        if (textures->count() > 1 && i == textures->count() - 1) {
            // Backingstore for a widget with QOpenGLWidget subwidgets
            blend.set(true);
            const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(textures->geometry(i), targetWindowRect);
            m_blitter.blit(textureId, target, QOpenGLTextureBlitter::OriginTopLeft);
        } else if (textures->count() == 1) {
            // A regular QWidget window
            const bool translucent = window->sourceWindow()->requestedFormat().alphaBufferSize() > 0;
            blend.set(translucent);
            const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(textures->geometry(i), targetWindowRect);
            m_blitter.blit(textureId, target, QOpenGLTextureBlitter::OriginTopLeft);
        } else if (!textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop)) {
            // Texture from an FBO belonging to a QOpenGLWidget
            blend.set(false);
            clippedBlit(textures, i, targetWindowRect, &m_blitter);
        }
    }

    for (int i = 0; i < textures->count(); ++i) {
        if (textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop)) {
            blend.set(true);
            clippedBlit(textures, i, targetWindowRect, &m_blitter);
        }
    }

    m_blitter.setOpacity(1.0f);
}

QOpenGLCompositor *QOpenGLCompositor::instance()
{
    if (!compositor)
        compositor = new QOpenGLCompositor;
    return compositor;
}

void QOpenGLCompositor::destroy()
{
    delete compositor;
    compositor = 0;
}

void QOpenGLCompositor::addWindow(QOpenGLCompositorWindow *window)
{
    if (!m_windows.contains(window)) {
        m_windows.append(window);
        emit topWindowChanged(window);
    }
}

void QOpenGLCompositor::removeWindow(QOpenGLCompositorWindow *window)
{
    m_windows.removeOne(window);
    if (!m_windows.isEmpty())
        emit topWindowChanged(m_windows.last());
}

void QOpenGLCompositor::moveToTop(QOpenGLCompositorWindow *window)
{
    m_windows.removeOne(window);
    m_windows.append(window);
    emit topWindowChanged(window);
}

void QOpenGLCompositor::changeWindowIndex(QOpenGLCompositorWindow *window, int newIdx)
{
    int idx = m_windows.indexOf(window);
    if (idx != -1 && idx != newIdx) {
        m_windows.move(idx, newIdx);
        if (newIdx == m_windows.size() - 1)
            emit topWindowChanged(m_windows.last());
    }
}

QT_END_NAMESPACE
