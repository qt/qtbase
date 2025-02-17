// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMWINDOW_H
#define QWASMWINDOW_H

#include "qwasmintegration.h"
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>
#include <emscripten/html5.h>
#include "qwasmbackingstore.h"
#include "qwasmscreen.h"
#include "qwasmcompositor.h"
#include "qwasmwindownonclientarea.h"
#include "qwasmwindowstack.h"
#include "qwasmwindowtreenode.h"

#include <QtCore/private/qstdweb_p.h>
#include "QtGui/qopenglcontext.h"
#include <QtOpenGL/qopengltextureblitter.h>

#include <emscripten/val.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace qstdweb {
class EventCallback;
}

class ClientArea;
struct KeyEvent;
struct PointerEvent;
class QWasmDeadKeySupport;
struct WheelEvent;

Q_DECLARE_LOGGING_CATEGORY(qLcQpaWasmInputContext)

class QWasmWindow final : public QPlatformWindow,
                          public QWasmWindowTreeNode,
                          public QNativeInterface::Private::QWasmWindow
{
public:
    QWasmWindow(QWindow *w, QWasmDeadKeySupport *deadKeySupport, QWasmCompositor *compositor,
                QWasmBackingStore *backingStore);
    ~QWasmWindow() final;

    static QWasmWindow *fromWindow(QWindow *window);
    QSurfaceFormat format() const override;

    void paint();
    void setZOrder(int order);
    void setWindowCursor(QByteArray cssCursorName);
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
    void setOpacity(qreal level) override;
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
    void setMask(const QRegion &region) final;
    void setParent(const QPlatformWindow *window) final;
    void focus();

    QWasmScreen *platformScreen() const;
    void setBackingStore(QWasmBackingStore *store) { m_backingStore = store; }
    QWasmBackingStore *backingStore() const { return m_backingStore; }
    QWindow *window() const { return m_window; }

    std::string canvasSelector() const;

    emscripten::val context2d() const { return m_context2d; }
    emscripten::val a11yContainer() const { return m_a11yContainer; }
    emscripten::val inputHandlerElement() const { return m_windowContents; }

    // QNativeInterface::Private::QWasmWindow
    emscripten::val document() const override { return m_document; }
    emscripten::val clientArea() const override { return m_qtWindow; }

    // QWasmWindowTreeNode:
    emscripten::val containerElement() final;
    QWasmWindowTreeNode *parentNode() final;

private:
    friend class QWasmScreen;
    static constexpr auto defaultWindowSize = 160;

    // QWasmWindowTreeNode:
    QWasmWindow *asWasmWindow() final;
    void onParentChanged(QWasmWindowTreeNode *previous, QWasmWindowTreeNode *current,
                         QWasmWindowStack::PositionPreference positionPreference) final;

    void invalidate();
    bool hasFrame() const;
    bool hasTitleBar() const;
    bool hasBorder() const;
    bool hasShadow() const;
    bool hasMaximizeButton() const;
    void applyWindowState();
    void commitParent(QWasmWindowTreeNode *parent);

    void handleKeyEvent(const emscripten::val &event);
    bool processKey(const KeyEvent &event);
    void handleKeyForInputContextEvent(const emscripten::val &event);
    bool processKeyForInputContext(const KeyEvent &event);
    void handlePointerEvent(const emscripten::val &event);
    bool processPointer(const PointerEvent &event);
    void handleWheelEvent(const emscripten::val &event);
    bool processWheel(const WheelEvent &event);

    QWindow *m_window = nullptr;
    QWasmCompositor *m_compositor = nullptr;
    QWasmBackingStore *m_backingStore = nullptr;
    QWasmDeadKeySupport *m_deadKeySupport;
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

    QWasmWindowTreeNode *m_commitedParent = nullptr;

    std::unique_ptr<qstdweb::EventCallback> m_keyDownCallback;
    std::unique_ptr<qstdweb::EventCallback> m_keyUpCallback;
    std::unique_ptr<qstdweb::EventCallback> m_keyDownCallbackForInputContext;
    std::unique_ptr<qstdweb::EventCallback> m_keyUpCallbackForInputContext;

    std::unique_ptr<qstdweb::EventCallback> m_pointerLeaveCallback;
    std::unique_ptr<qstdweb::EventCallback> m_pointerEnterCallback;

    std::unique_ptr<qstdweb::EventCallback> m_dropCallback;

    std::unique_ptr<qstdweb::EventCallback> m_wheelEventCallback;

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
