// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMWINDOW_H
#define QWASMWINDOW_H

#include "qwasmintegration.h"
#include "qwasmbackingstore.h"
#include "qwasmscreen.h"
#include "qwasmcompositor.h"
#include "qwasmwindownonclientarea.h"
#include "qwasmwindowstack.h"
#include "qwasmwindowtreenode.h"
#include "qwasmevent.h"

#include <QtCore/private/qstdweb_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>

#include <emscripten/val.h>
#include <emscripten/html5.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace qstdweb {
class EventCallback;
}

struct KeyEvent;
struct PointerEvent;
class QWasmDeadKeySupport;
struct WheelEvent;

Q_DECLARE_LOGGING_CATEGORY(qLcQpaWasmInputContext)

class QWasmWindow final : public QPlatformWindow,
                          public QWasmWindowTreeNode<>,
                          public QNativeInterface::Private::QWasmWindow
{
public:
    QWasmWindow(QWindow *w, QWasmDeadKeySupport *deadKeySupport, QWasmCompositor *compositor,
                QWasmBackingStore *backingStore, WId nativeHandle);
    ~QWasmWindow() final;

    static QWasmWindow *fromWindow(const QWindow *window);
    QWasmWindow *transientParent() const;
    Qt::WindowFlags windowFlags() const;
    bool isModal() const;
    QSurfaceFormat format() const override;

    void registerEventHandlers();

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

    std::string canvasSelector() const;

    emscripten::val context2d() const { return m_context2d; }
    emscripten::val a11yContainer() const { return m_a11yContainer; }
    emscripten::val inputHandlerElement() const { return m_window; }
    emscripten::val inputElement() const { return m_inputElement; }

    // QNativeInterface::Private::QWasmWindow
    emscripten::val document() const override { return m_document; }
    emscripten::val clientArea() const override { return m_decoratedWindow; }

    // QWasmWindowTreeNode:
    emscripten::val containerElement() final;
    QWasmWindowTreeNode *parentNode() final;

public slots:
    void onTransientParentChanged(QWindow *newTransientParent);
    void onModalityChanged();

private:
    friend class QWasmScreen;
    static constexpr auto defaultWindowSize = 160;

    QMetaObject::Connection m_transientWindowChangedConnection;
    QMetaObject::Connection m_modalityChangedConnection;

    // QWasmWindowTreeNode:
    QWasmWindow *asWasmWindow() final;
    void onParentChanged(QWasmWindowTreeNode *previous, QWasmWindowTreeNode *current,
                         QWasmWindowStack<>::PositionPreference positionPreference) final;

    void shutdown();
    void invalidate();
    bool hasFrame() const;
    bool hasTitleBar() const;
    bool hasBorder() const;
    bool hasShadow() const;
    bool hasMaximizeButton() const;
    void applyWindowState();
    void commitParent(QWasmWindowTreeNode *parent);

    void handleKeyEvent(const KeyEvent &event);
    bool processKey(const KeyEvent &event);
    void handleKeyForInputContextEvent(const KeyEvent &event);
    bool processKeyForInputContext(const KeyEvent &event);
    void handleInputEvent(emscripten::val event);
    void handleCompositionStartEvent(emscripten::val event);
    void handleCompositionUpdateEvent(emscripten::val event);
    void handleCompositionEndEvent(emscripten::val event);

    void handlePointerEnterLeaveEvent(const PointerEvent &event);
    bool processPointerEnterLeave(const PointerEvent &event);
    void processPointer(const PointerEvent &event);
    bool deliverPointerEvent(const PointerEvent &event);
    void handleWheelEvent(const emscripten::val &event);
    bool processWheel(const WheelEvent &event);
    Qt::WindowFlags fixTopLevelWindowFlags(Qt::WindowFlags) const;
    bool shouldBeAboveTransientParentFlags(Qt::WindowFlags flags) const;
    QWasmWindowStack<>::PositionPreference positionPreferenceFromWindowFlags(Qt::WindowFlags) const;

    QWasmCompositor *m_compositor = nullptr;
    QWasmBackingStore *m_backingStore = nullptr;
    QWasmDeadKeySupport *m_deadKeySupport;
    QRect m_normalGeometry {0, 0, 0 ,0};

    emscripten::val m_document;
    emscripten::val m_decoratedWindow;
    emscripten::val m_window;
    emscripten::val m_a11yContainer;
    emscripten::val m_canvas;
    emscripten::val m_focusHelper;
    emscripten::val m_inputElement;

    emscripten::val m_context2d = emscripten::val::undefined();

    std::unique_ptr<NonClientArea> m_nonClientArea;

    QWasmWindowTreeNode *m_commitedParent = nullptr;

    QWasmEventHandler m_keyDownCallback;
    QWasmEventHandler m_keyUpCallback;
    QWasmEventHandler m_keyDownCallbackForInputContext;
    QWasmEventHandler m_keyUpCallbackForInputContext;
    QWasmEventHandler m_inputCallback;
    QWasmEventHandler m_compositionStartCallback;
    QWasmEventHandler m_compositionUpdateCallback;
    QWasmEventHandler m_compositionEndCallback;

    QWasmEventHandler m_pointerDownCallback;
    QWasmEventHandler m_pointerMoveCallback;
    QWasmEventHandler m_pointerUpCallback;
    QWasmEventHandler m_pointerCancelCallback;
    QWasmEventHandler m_pointerLeaveCallback;
    QWasmEventHandler m_pointerEnterCallback;

    QWasmEventHandler m_dragOverCallback;
    QWasmEventHandler m_dragStartCallback;
    QWasmEventHandler m_dragEndCallback;
    QWasmEventHandler m_dropCallback;
    QWasmEventHandler m_dragLeaveCallback;

    QWasmEventHandler m_wheelEventCallback;

    QMap<int, QWindowSystemInterface::TouchPoint> m_pointerIdToTouchPoints;

    QWasmEventHandler m_cutCallback;
    QWasmEventHandler m_copyCallback;
    QWasmEventHandler m_pasteCallback;

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
