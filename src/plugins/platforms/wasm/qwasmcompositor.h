// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMCOMPOSITOR_H
#define QWASMCOMPOSITOR_H

#include "qwasmwindowstack.h"

#include <qpa/qplatformwindow.h>
#include <QMap>

#include <QtGui/qinputdevice.h>
#include <QtCore/private/qstdweb_p.h>

#include <QPointer>
#include <QPointingDevice>

#include <emscripten/html5.h>
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

QT_BEGIN_NAMESPACE

struct PointerEvent;
class QWasmWindow;
class QWasmScreen;
class QOpenGLContext;
class QOpenGLTexture;
class QWasmEventTranslator;

class QWasmCompositor final : public QObject
{
    Q_OBJECT
public:
    QWasmCompositor(QWasmScreen *screen);
    ~QWasmCompositor() final;

    void initEventHandlers();

    struct QWasmFrameOptions {
        QRect rect;
        int lineWidth;
        QPalette palette;
    };

    void startResize(Qt::Edges edges);

    void addWindow(QWasmWindow *window);
    void removeWindow(QWasmWindow *window);

    void setVisible(QWasmWindow *window, bool visible);
    void raise(QWasmWindow *window);
    void lower(QWasmWindow *window);

    QWindow *windowAt(QPoint globalPoint, int padding = 0) const;
    QWindow *keyWindow() const;

    QWasmScreen *screen();

    enum UpdateRequestDeliveryType { ExposeEventDelivery, UpdateRequestDelivery };
    void requestUpdateAllWindows();
    void requestUpdateWindow(QWasmWindow *window, UpdateRequestDeliveryType updateType = ExposeEventDelivery);

    void setCapture(QWasmWindow *window);
    void releaseCapture();

    void handleBackingStoreFlush(QWindow *window);

private:
    class WindowManipulation {
    public:
        enum class Operation {
            None,
            Move,
            Resize,
        };

        WindowManipulation(QWasmScreen* screen);

        void onPointerDown(const PointerEvent& event, QWindow* windowAtPoint);
        void onPointerMove(const PointerEvent& event);
        void onPointerUp(const PointerEvent& event);
        void startResize(Qt::Edges edges);

        Operation operation() const;

    private:
        struct ResizeState {
            Qt::Edges m_resizeEdges;
            QPoint m_originInScreenCoords;
            QRect m_initialWindowBounds;
            const QPoint m_minShrink;
            const QPoint m_maxGrow;
        };
        struct MoveState {
            QPoint m_lastPointInScreenCoords;
        };
        struct OperationState
        {
            int pointerId;
            QPointer<QWindow> window;
            std::variant<ResizeState, MoveState> operationSpecific;
        };
        struct SystemDragInitData
        {
            QPoint lastMouseMovePoint;
            int lastMousePointerId = -1;
        };

        void resizeWindow(const QPoint& amount);
        ResizeState makeResizeState(Qt::Edges edges, const QPoint &startPoint, QWindow *window);

        QWasmScreen *m_screen;

        SystemDragInitData m_systemDragInitData;
        std::unique_ptr<OperationState> m_state;
    };

    void frame(bool all, const QList<QWasmWindow *> &windows);

    void onTopWindowChanged();

    void deregisterEventHandlers();
    void destroy();

    void requestUpdate();
    void deliverUpdateRequests();
    void deliverUpdateRequest(QWasmWindow *window, UpdateRequestDeliveryType updateType);

    static int keyboard_cb(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData);
    static int focus_cb(int eventType, const EmscriptenFocusEvent *focusEvent, void *userData);
    static int wheel_cb(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData);

    bool processPointer(const PointerEvent& event);
    bool deliverEventToTarget(const PointerEvent& event, QWindow *eventTarget);

    static int touchCallback(int eventType, const EmscriptenTouchEvent *ev, void *userData);

    bool processKeyboard(int eventType, const EmscriptenKeyboardEvent *keyEvent);
    bool processWheel(int eventType, const EmscriptenWheelEvent *wheelEvent);
    bool processMouseEnter(const EmscriptenMouseEvent *mouseEvent);
    bool processMouseLeave();
    bool processTouch(int eventType, const EmscriptenTouchEvent *touchEvent);

    void enterWindow(QWindow *window, const QPoint &localPoint, const QPoint &globalPoint);
    void leaveWindow(QWindow *window);

    void updateEnabledState();

    WindowManipulation m_windowManipulation;
    QWasmWindowStack m_windowStack;

    bool m_isEnabled = true;
    QSize m_targetSize;
    qreal m_targetDevicePixelRatio = 1;
    QMap<QWasmWindow *, UpdateRequestDeliveryType> m_requestUpdateWindows;
    bool m_requestUpdateAllWindows = false;
    int m_requestAnimationFrameId = -1;
    bool m_inDeliverUpdateRequest = false;

    QPointer<QWindow> m_lastMouseTargetWindow;
    QPointer<QWindow> m_mouseCaptureWindow;

    std::unique_ptr<qstdweb::EventCallback> m_pointerDownCallback;
    std::unique_ptr<qstdweb::EventCallback> m_pointerMoveCallback;
    std::unique_ptr<qstdweb::EventCallback> m_pointerUpCallback;
    std::unique_ptr<qstdweb::EventCallback> m_pointerLeaveCallback;
    std::unique_ptr<qstdweb::EventCallback> m_pointerEnterCallback;

    std::unique_ptr<QPointingDevice> m_touchDevice;

    QMap <int, QPointF> m_pressedTouchIds;

    bool m_isResizeCursorDisplayed = false;

    std::unique_ptr<QWasmEventTranslator> m_eventTranslator;

    bool m_mouseInScreen = false;
    QPointer<QWindow> m_windowUnderMouse;
};

QT_END_NAMESPACE

#endif
