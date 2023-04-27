// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>
#include <rhi/qrhi.h>
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
      m_targetWindow(0),
      m_rotation(0)
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

void QOpenGLCompositor::setTargetWindow(QWindow *targetWindow, const QRect &nativeTargetGeometry)
{
    m_targetWindow = targetWindow;
    m_nativeTargetGeometry = nativeTargetGeometry;
}

void QOpenGLCompositor::setTargetContext(QOpenGLContext *context)
{
    m_context = context;
}

void QOpenGLCompositor::setRotation(int degrees)
{
    m_rotation = degrees;
    m_rotationMatrix.setToIdentity();
    m_rotationMatrix.rotate(degrees, 0, 0, 1);
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
    QScopedPointer<QOpenGLFramebufferObject> fbo(new QOpenGLFramebufferObject(m_nativeTargetGeometry.size()));
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
    glViewport(0, 0, m_nativeTargetGeometry.width(), m_nativeTargetGeometry.height());

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

static void clippedBlit(const QPlatformTextureList *textures, int idx, const QRect &sourceWindowRect,
                        const QRect &targetWindowRect,
                        QOpenGLTextureBlitter *blitter, QMatrix4x4 *rotationMatrix)
{
    const QRect clipRect = textures->clipRect(idx);
    if (clipRect.isEmpty())
        return;

    const QRect rectInWindow = textures->geometry(idx).translated(sourceWindowRect.topLeft());
    const QRect clippedRectInWindow = rectInWindow & clipRect.translated(rectInWindow.topLeft());
    const QRect srcRect = toBottomLeftRect(clipRect, rectInWindow.height());

    QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(clippedRectInWindow, targetWindowRect);
    if (rotationMatrix)
        target = *rotationMatrix * target;

    const QMatrix3x3 source = QOpenGLTextureBlitter::sourceTransform(srcRect, rectInWindow.size(),
                                                                     QOpenGLTextureBlitter::OriginBottomLeft);

    const uint textureId = textures->texture(idx)->nativeTexture().object;
    blitter->blit(textureId, target, source);
}

void QOpenGLCompositor::render(QOpenGLCompositorWindow *window)
{
    const QPlatformTextureList *textures = window->textures();
    if (!textures)
        return;

    const QRect targetWindowRect(QPoint(0, 0), m_targetWindow->geometry().size());
    float currentOpacity = 1.0f;
    BlendStateBinder blend;
    const QRect sourceWindowRect = window->sourceWindow()->geometry();
    for (int i = 0; i < textures->count(); ++i) {
        const uint textureId = textures->texture(i)->nativeTexture().object;
        const float opacity = window->sourceWindow()->opacity();
        if (opacity != currentOpacity) {
            currentOpacity = opacity;
            m_blitter.setOpacity(currentOpacity);
        }

        if (textures->count() > 1 && i == textures->count() - 1) {
            // Backingstore for a widget with QOpenGLWidget subwidgets
            blend.set(true);
            QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(textures->geometry(i), targetWindowRect);
            if (m_rotation)
                target = m_rotationMatrix * target;
            m_blitter.blit(textureId, target, QOpenGLTextureBlitter::OriginTopLeft);
        } else if (textures->count() == 1) {
            // A regular QWidget window
            const bool translucent = window->sourceWindow()->requestedFormat().alphaBufferSize() > 0;
            blend.set(translucent);
            QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(textures->geometry(i), targetWindowRect);
            if (m_rotation)
                target = m_rotationMatrix * target;
            m_blitter.blit(textureId, target, QOpenGLTextureBlitter::OriginTopLeft);
        } else if (!textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop)) {
            // Texture from an FBO belonging to a QOpenGLWidget or QQuickWidget
            blend.set(false);
            clippedBlit(textures, i, sourceWindowRect, targetWindowRect, &m_blitter, m_rotation ? &m_rotationMatrix : nullptr);
        }
    }

    for (int i = 0; i < textures->count(); ++i) {
        if (textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop)) {
            blend.set(true);
            clippedBlit(textures, i, sourceWindowRect, targetWindowRect, &m_blitter, m_rotation ? &m_rotationMatrix : nullptr);
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
        ensureCorrectZOrder();
        if (window == m_windows.constLast())
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
    if (!m_windows.isEmpty() && window == m_windows.constLast()) {
        // Already on top
        return;
    }

    m_windows.removeOne(window);
    m_windows.append(window);
    ensureCorrectZOrder();

    if (window == m_windows.constLast())
        emit topWindowChanged(window);
}

void QOpenGLCompositor::changeWindowIndex(QOpenGLCompositorWindow *window, int newIdx)
{
    int idx = m_windows.indexOf(window);
    if (idx != -1 && idx != newIdx) {
        m_windows.move(idx, newIdx);
        ensureCorrectZOrder();
        if (window == m_windows.constLast())
            emit topWindowChanged(m_windows.last());
    }
}

void QOpenGLCompositor::ensureCorrectZOrder()
{
    const auto originalOrder = m_windows;

    std::sort(m_windows.begin(), m_windows.end(),
              [this, &originalOrder](QOpenGLCompositorWindow *cw1, QOpenGLCompositorWindow *cw2) {
                  QWindow *w1 = cw1->sourceWindow();
                  QWindow *w2 = cw2->sourceWindow();

                  // Case #1: The main window needs to have less z-order. It can never be in
                  // front of our tool windows, popups etc, because it's fullscreen!
                  if (w1 == m_targetWindow || w2 == m_targetWindow)
                      return w1 == m_targetWindow;

                  // Case #2:
                  if (w2->isAncestorOf(w1)) {
                      // w1 is transient child of w2. W1 goes in front then.
                      return false;
                  }

                  if (w1->isAncestorOf(w2)) {
                      // Or the other way around
                      return true;
                  }

                  // Case #3: Modality gets higher Z
                  if (w1->modality() != Qt::NonModal && w2->modality() == Qt::NonModal)
                      return false;

                  if (w2->modality() != Qt::NonModal && w1->modality() == Qt::NonModal)
                      return true;

                  const bool isTool1 = (w1->flags() & Qt::Tool) == Qt::Tool;
                  const bool isTool2 = (w2->flags() & Qt::Tool) == Qt::Tool;
                  const bool isPurePopup1 = !isTool1 && (w1->flags() & Qt::Popup) == Qt::Popup;
                  const bool isPurePopup2 = !isTool2 && (w2->flags() & Qt::Popup) == Qt::Popup;

                  // Case #4: By pure-popup we mean menus and tooltips. Qt::Tool implies Qt::Popup
                  // and we don't want to catch QDockWidget and other tool windows just yet
                  if (isPurePopup1 != isPurePopup2)
                      return !isPurePopup1;

                  // Case #5: One of the window is a Tool, that goes to front, as done in other QPAs
                  if (isTool1 != isTool2)
                      return !isTool1;

                  // Case #6: Just preserve original sorting:
                  return originalOrder.indexOf(cw1) < originalOrder.indexOf(cw2);
              });
}

QT_END_NAMESPACE

#include "moc_qopenglcompositor_p.cpp"
