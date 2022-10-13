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

#include <QtCore/private/qstdweb_p.h>
#include "QtGui/qopenglcontext.h"
#include <QtOpenGL/qopengltextureblitter.h>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

class QWasmWindow final : public QPlatformWindow
{
public:
    QWasmWindow(QWindow *w, QWasmCompositor *compositor, QWasmBackingStore *backingStore);
    ~QWasmWindow() final;
    void destroy();

    void initialize() override;

    void paint();
    void setZOrder(int order);
    void onActivationChanged(bool active);

    void setGeometry(const QRect &) override;
    void setVisible(bool visible) override;
    bool isVisible() const;
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

    bool startSystemResize(Qt::Edges edges) final;

    bool isPointOnTitle(QPoint point) const;

    void setWindowFlags(Qt::WindowFlags flags) override;
    void setWindowState(Qt::WindowStates state) override;
    void setWindowTitle(const QString &title) override;
    void setWindowIcon(const QIcon &icon) override;
    void applyWindowState();
    bool setKeyboardGrabEnabled(bool) override { return false; }
    bool setMouseGrabEnabled(bool grab) final;
    bool windowEvent(QEvent *event) final;

    std::string canvasSelector() const;
    emscripten::val context2d() { return m_context2d; }

private:
    friend class QWasmScreen;

    class Resizer;
    class WebImageButton;

    QMarginsF borderMargins() const;

    void onRestoreClicked();
    void onMaximizeClicked();
    void onCloseClicked();
    void onInteraction();

    void invalidate();
    bool hasTitleBar() const;

    QWindow *m_window = nullptr;
    QWasmCompositor *m_compositor = nullptr;
    QWasmBackingStore *m_backingStore = nullptr;
    QRect m_normalGeometry {0, 0, 0 ,0};

    emscripten::val m_document;
    emscripten::val m_qtWindow;
    emscripten::val m_windowContents;
    emscripten::val m_titleBar;
    emscripten::val m_label;
    emscripten::val m_canvasContainer;
    emscripten::val m_canvas;
    emscripten::val m_context2d = emscripten::val::undefined();

    std::unique_ptr<Resizer> m_resizer;

    std::unique_ptr<WebImageButton> m_close;
    std::unique_ptr<WebImageButton> m_maximize;
    std::unique_ptr<WebImageButton> m_restore;
    std::unique_ptr<WebImageButton> m_icon;

    Qt::WindowStates m_state = Qt::WindowNoState;
    Qt::WindowStates m_previousWindowState = Qt::WindowNoState;

    Qt::WindowFlags m_flags = Qt::Widget;

    WId m_winId = 0;
    bool m_hasTitle = false;
    bool m_needsCompositor = false;
    long m_requestAnimationFrameId = -1;
    friend class QWasmCompositor;
    friend class QWasmEventTranslator;
    bool windowIsPopupType(Qt::WindowFlags flags) const;
};

QT_END_NAMESPACE
#endif // QWASMWINDOW_H
