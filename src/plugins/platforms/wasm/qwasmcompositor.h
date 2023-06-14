// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMCOMPOSITOR_H
#define QWASMCOMPOSITOR_H

#include "qwasmwindowstack.h"

#include <qpa/qplatformwindow.h>

#include <QMap>

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

    enum UpdateRequestDeliveryType { ExposeEventDelivery, UpdateRequestDelivery };

    void requestUpdateWindow(QWasmWindow *window, UpdateRequestDeliveryType updateType = ExposeEventDelivery);

    void handleBackingStoreFlush(QWindow *window);
    void onWindowTreeChanged(QWasmWindowTreeNodeChangeType changeType, QWasmWindow *window);

private:
    void frame(const QList<QWasmWindow *> &windows);

    void deregisterEventHandlers();

    void requestUpdate();
    void deliverUpdateRequests();
    void deliverUpdateRequest(QWasmWindow *window, UpdateRequestDeliveryType updateType);

    bool m_isEnabled = true;
    QMap<QWasmWindow *, UpdateRequestDeliveryType> m_requestUpdateWindows;
    int m_requestAnimationFrameId = -1;
    bool m_inDeliverUpdateRequest = false;
};

QT_END_NAMESPACE

#endif
