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

    void addWindow(QWasmWindow *window);
    void removeWindow(QWasmWindow *window);

    void setVisible(QWasmWindow *window, bool visible);
    void raise(QWasmWindow *window);
    void lower(QWasmWindow *window);

    void onScreenDeleting();

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

    static int touchCallback(int eventType, const EmscriptenTouchEvent *ev, void *userData);

    bool processKeyboard(int eventType, const EmscriptenKeyboardEvent *keyEvent);
    bool processWheel(int eventType, const EmscriptenWheelEvent *wheelEvent);
    bool processTouch(int eventType, const EmscriptenTouchEvent *touchEvent);

    void enterWindow(QWindow *window, const QPoint &localPoint, const QPoint &globalPoint);
    void leaveWindow(QWindow *window);

    void updateEnabledState();

    QWasmWindowStack m_windowStack;

    bool m_isEnabled = true;
    QSize m_targetSize;
    qreal m_targetDevicePixelRatio = 1;
    QMap<QWasmWindow *, UpdateRequestDeliveryType> m_requestUpdateWindows;
    bool m_requestUpdateAllWindows = false;
    int m_requestAnimationFrameId = -1;
    bool m_inDeliverUpdateRequest = false;

    std::unique_ptr<QPointingDevice> m_touchDevice;

    QMap <int, QPointF> m_pressedTouchIds;

    std::unique_ptr<QWasmEventTranslator> m_eventTranslator;
};

QT_END_NAMESPACE

#endif
