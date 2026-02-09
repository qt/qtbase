// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWASMCOMPOSITOR_H
#define QWASMCOMPOSITOR_H

#include "qwasmwindowstack.h"

#include <qpa/qplatformwindow.h>
#include <private/qwasmsuspendresumecontrol_p.h>

#include <QMap>
#include <tuple>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

class QWasmWindow;
class QWasmScreen;

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

    static bool releaseRequestUpdateHold();

    void requestUpdate();
    enum UpdateRequestDeliveryType { ExposeEventDelivery, UpdateRequestDelivery };
    void requestUpdateWindow(QWasmWindow *window, const QRect &updateRect, UpdateRequestDeliveryType updateType = ExposeEventDelivery);

    void handleBackingStoreFlush(QWindow *window, const QRect &updateRect);
    void onWindowTreeChanged(QWasmWindowTreeNodeChangeType changeType, QWasmWindow *window);

private:
    void frame(const QList<QWasmWindow *> &windows);

    void deregisterEventHandlers();

    void deliverUpdateRequests();
    void deliverUpdateRequest(QWasmWindow *window, const QRect &updateRect, UpdateRequestDeliveryType updateType);

    bool m_isEnabled = true;
    QMap<QWasmWindow *, std::tuple<QRect, UpdateRequestDeliveryType>> m_requestUpdateWindows;
    uint32_t m_drawCallbackHandle = 0;
    bool m_inDeliverUpdateRequest = false;
    static bool m_requestUpdateHoldEnabled;
};

QT_END_NAMESPACE

#endif
