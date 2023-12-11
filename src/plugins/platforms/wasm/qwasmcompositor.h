// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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

enum class QWasmWindowTreeNodeChangeType;

class QWasmCompositor final : public QObject
{
    Q_OBJECT
public:
    QWasmCompositor(QWasmScreen *screen);
    ~QWasmCompositor() final;

    void setVisible(QWasmWindow *window, bool visible);
    void onScreenDeleting();

    QWasmScreen *screen();
    void setEnabled(bool enabled);

    enum UpdateRequestDeliveryType { ExposeEventDelivery, UpdateRequestDelivery };
    void requestUpdateAllWindows();
    void requestUpdateWindow(QWasmWindow *window, UpdateRequestDeliveryType updateType = ExposeEventDelivery);

    void handleBackingStoreFlush(QWindow *window);
    void onWindowTreeChanged(QWasmWindowTreeNodeChangeType changeType, QWasmWindow *window);

private:
    void frame(const QList<QWasmWindow *> &windows);

    void deregisterEventHandlers();
    void destroy();

    void requestUpdate();
    void deliverUpdateRequests();
    void deliverUpdateRequest(QWasmWindow *window, UpdateRequestDeliveryType updateType);

    static int touchCallback(int eventType, const EmscriptenTouchEvent *ev, void *userData);

    bool processTouch(int eventType, const EmscriptenTouchEvent *touchEvent);

    bool m_isEnabled = true;
    QMap<QWasmWindow *, UpdateRequestDeliveryType> m_requestUpdateWindows;
    bool m_requestUpdateAllWindows = false;
    int m_requestAnimationFrameId = -1;
    bool m_inDeliverUpdateRequest = false;
};

QT_END_NAMESPACE

#endif
