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
#include "qwasmwindownonclientarea.h"

#include <QtCore/private/qstdweb_p.h>
#include "QtGui/qopenglcontext.h"
#include <QtOpenGL/qopengltextureblitter.h>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

namespace qstdweb {
class EventCallback;
}

class ClientArea;

class QWasmWindow final : public QPlatformWindow
{
public:
    QWasmWindow(QWindow *w, QWasmCompositor *compositor, QWasmBackingStore *backingStore);
    ~QWasmWindow() final;

    void destroy();
    void paint();
    void setZOrder(int order);
    void onActivationChanged(bool active);
    bool isVisible() const;

    void onNonClientAreaInteraction();
    void onRestoreClicked();
    void onMaximizeClicked();
    void onToggleMaximized();
    void onCloseClicked();
    bool onNonClientEvent(const PointerEvent &event);

    // QPlatformWindow:
    void initialize() override;
    void setGeometry(const QRect &) override;
    void setVisible(bool visible) override;
    QMargins frameMargins() const override;
    WId winId() const override;
    void propagateSizeHints() override;
    void raise() override;
    void lower() override;
    QRect normalGeometry() const override;
    qreal devicePixelRatio() const override;
    void requestUpdate() override;
    void requestActivateWindow() override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    void setWindowState(Qt::WindowStates state) override;
    void setWindowTitle(const QString &title) override;
    void setWindowIcon(const QIcon &icon) override;
    bool setKeyboardGrabEnabled(bool) override { return false; }
    bool setMouseGrabEnabled(bool grab) final;
    bool windowEvent(QEvent *event) final;

    QWasmScreen *platformScreen() const;
    void setBackingStore(QWasmBackingStore *store) { m_backingStore = store; }
    QWasmBackingStore *backingStore() const { return m_backingStore; }
    QWindow *window() const { return m_window; }

    std::string canvasSelector() const;
    emscripten::val context2d() { return m_context2d; }
    emscripten::val a11yContainer() { return m_a11yContainer; }


private:
    friend class QWasmScreen;

    void invalidate();
    bool hasTitleBar() const;
    void applyWindowState();

    bool processPointer(const PointerEvent &event);

    QWindow *m_window = nullptr;
    QWasmCompositor *m_compositor = nullptr;
    QWasmBackingStore *m_backingStore = nullptr;
    QRect m_normalGeometry {0, 0, 0 ,0};

    emscripten::val m_document;
    emscripten::val m_qtWindow;
    emscripten::val m_windowContents;
    emscripten::val m_canvasContainer;
    emscripten::val m_a11yContainer;
    emscripten::val m_canvas;
    emscripten::val m_context2d = emscripten::val::undefined();

    std::unique_ptr<NonClientArea> m_nonClientArea;
    std::unique_ptr<ClientArea> m_clientArea;

    std::unique_ptr<qstdweb::EventCallback> m_pointerLeaveCallback;
    std::unique_ptr<qstdweb::EventCallback> m_pointerEnterCallback;
    std::unique_ptr<qstdweb::EventCallback> m_pointerMoveCallback;

    Qt::WindowStates m_state = Qt::WindowNoState;
    Qt::WindowStates m_previousWindowState = Qt::WindowNoState;

    Qt::WindowFlags m_flags = Qt::Widget;

    QPoint m_lastPointerMovePoint;

    WId m_winId = 0;
    bool m_wantCapture = false;
    bool m_hasTitle = false;
    bool m_needsCompositor = false;
    long m_requestAnimationFrameId = -1;
    friend class QWasmCompositor;
    friend class QWasmEventTranslator;
    bool windowIsPopupType(Qt::WindowFlags flags) const;
};

QT_END_NAMESPACE
#endif // QWASMWINDOW_H
