// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMWINDOW_H
#define QWASMWINDOW_H

#include "qwasmintegration.h"
#include <qpa/qplatformwindow.h>
#include <emscripten/html5.h>
#include "qwasmbackingstore.h"
#include "qwasmscreen.h"
#include "qwasmcompositor.h"

QT_BEGIN_NAMESPACE

class QWasmWindow : public QPlatformWindow
{
public:
    QWasmWindow(QWindow *w, QWasmCompositor *compositor, QWasmBackingStore *backingStore);
    ~QWasmWindow();
    void destroy();

    void initialize() override;

    void setGeometry(const QRect &) override;
    void setVisible(bool visible) override;
    bool isVisible();
    QMargins frameMargins() const override;

    WId winId() const override;

    void propagateSizeHints() override;
    void raise() override;
    void lower() override;
    QRect normalGeometry() const override;
    qreal devicePixelRatio() const override;
    void requestUpdate() override;
    void requestActivateWindow() override;

    QWasmScreen *platformScreen() const;
    void setBackingStore(QWasmBackingStore *store) { m_backingStore = store; }
    QWasmBackingStore *backingStore() const { return m_backingStore; }
    QWindow *window() const { return m_window; }

    void injectMousePressed(const QPoint &local, const QPoint &global,
                            Qt::MouseButton button, Qt::KeyboardModifiers mods);
    void injectMouseReleased(const QPoint &local, const QPoint &global,
                            Qt::MouseButton button, Qt::KeyboardModifiers mods);

    int titleHeight() const;
    int borderWidth() const;
    QRegion titleGeometry() const;
    QRegion resizeRegion() const;
    bool isPointOnTitle(QPoint point) const;
    bool isPointOnResizeRegion(QPoint point) const;
    QWasmCompositor::ResizeMode resizeModeAtPoint(QPoint point) const;
    QRect maxButtonRect() const;
    QRect minButtonRect() const;
    QRect closeButtonRect() const;
    QRect sysMenuRect() const;
    QRect normButtonRect() const;
    QRegion titleControlRegion() const;
    QWasmCompositor::SubControls activeSubControl() const;

    void setWindowState(Qt::WindowStates state) override;
    void applyWindowState();
    bool setKeyboardGrabEnabled(bool) override { return false; }
    bool setMouseGrabEnabled(bool grab) final;

protected:
    void invalidate();
    bool hasTitleBar() const;

protected:
    friend class QWasmScreen;

    QWindow* m_window = nullptr;
    QWasmCompositor *m_compositor = nullptr;
    QWasmBackingStore *m_backingStore = nullptr;
    QRect m_normalGeometry {0, 0, 0 ,0};

    Qt::WindowStates m_windowState = Qt::WindowNoState;
    Qt::WindowStates m_previousWindowState = Qt::WindowNoState;
    QWasmCompositor::SubControls m_activeControl = QWasmCompositor::SC_None;
    WId m_winid = 0;
    bool m_hasTitle = false;
    bool m_needsCompositor = false;
    long m_requestAnimationFrameId = -1;
    friend class QWasmCompositor;
    friend class QWasmEventTranslator;
    bool windowIsPopupType(Qt::WindowFlags flags) const;
};
QT_END_NAMESPACE
#endif // QWASMWINDOW_H
