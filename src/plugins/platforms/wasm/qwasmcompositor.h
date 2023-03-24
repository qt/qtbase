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

class QWasmCompositor final : public QObject
{
    Q_OBJECT
public:
    QWasmCompositor(QWasmScreen *screen);
    ~QWasmCompositor() final;

    void addWindow(QWasmWindow *window);
    void removeWindow(QWasmWindow *window);

    void setVisible(QWasmWindow *window, bool visible);
    void setActive(QWasmWindow *window);
    void raise(QWasmWindow *window);
    void lower(QWasmWindow *window);
    void windowPositionPreferenceChanged(QWasmWindow *window, Qt::WindowFlags flags);

    QWindow *windowAt(QPoint globalPoint, int padding = 0) const;
    QWindow *keyWindow() const;

    QWasmScreen *screen();

    enum UpdateRequestDeliveryType { ExposeEventDelivery, UpdateRequestDelivery };

    void requestUpdateWindow(QWasmWindow *window, UpdateRequestDeliveryType updateType = ExposeEventDelivery);

    void handleBackingStoreFlush(QWindow *window);

private:
    void frame(const QList<QWasmWindow *> &windows);

    void onTopWindowChanged();

    void deregisterEventHandlers();

    void requestUpdate();
    void deliverUpdateRequests();
    void deliverUpdateRequest(QWasmWindow *window, UpdateRequestDeliveryType updateType);

    void updateEnabledState();

    QWasmWindowStack m_windowStack;
    QWasmWindow *m_activeWindow = nullptr;

    bool m_isEnabled = true;
    QMap<QWasmWindow *, UpdateRequestDeliveryType> m_requestUpdateWindows;
    int m_requestAnimationFrameId = -1;
    bool m_inDeliverUpdateRequest = false;
};

QT_END_NAMESPACE

#endif
