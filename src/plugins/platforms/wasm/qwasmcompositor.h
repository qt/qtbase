// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMCOMPOSITOR_H
#define QWASMCOMPOSITOR_H

#include "qwasmwindowstack.h"

#include <qpa/qplatformwindow.h>
#include <QMap>

#include <QtGui/qinputdevice.h>
#include <QtCore/private/qstdweb_p.h>

#include <emscripten/html5.h>
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

QT_BEGIN_NAMESPACE

class QWasmWindow;
class QWasmScreen;
class QOpenGLContext;
class QOpenGLTexture;

class QWasmCompositor final : public QObject
{
    Q_OBJECT
public:
    QWasmCompositor(QWasmScreen *screen);
    ~QWasmCompositor() final;

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

    void handleBackingStoreFlush(QWindow *window);

private:
    void frame(bool all, const QList<QWasmWindow *> &windows);

    void onTopWindowChanged();

    void deregisterEventHandlers();
    void destroy();

    void requestUpdate();
    void deliverUpdateRequests();
    void deliverUpdateRequest(QWasmWindow *window, UpdateRequestDeliveryType updateType);

    static int touchCallback(int eventType, const EmscriptenTouchEvent *ev, void *userData);

    bool processTouch(int eventType, const EmscriptenTouchEvent *touchEvent);

    void updateEnabledState();

    QWasmWindowStack m_windowStack;

    bool m_isEnabled = true;
    QMap<QWasmWindow *, UpdateRequestDeliveryType> m_requestUpdateWindows;
    bool m_requestUpdateAllWindows = false;
    int m_requestAnimationFrameId = -1;
    bool m_inDeliverUpdateRequest = false;
};

QT_END_NAMESPACE

#endif
