// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmcompositor.h"
#include "qwasmwindow.h"

#include <private/qeventdispatcher_wasm_p.h>

#include <qpa/qwindowsysteminterface.h>

#include <emscripten/html5.h>

using namespace emscripten;

bool QWasmCompositor::m_requestUpdateHoldEnabled = false;

QWasmCompositor::QWasmCompositor(QWasmScreen *screen) : QObject(screen)
{
    QWindowSystemInterface::setSynchronousWindowSystemEvents(true);
}

QWasmCompositor::~QWasmCompositor()
{
    if (m_requestAnimationFrameId != -1)
        emscripten_cancel_animation_frame(m_requestAnimationFrameId);

    // TODO(mikolaj.boc): Investigate if m_isEnabled is needed at all. It seems like a frame should
    // not be generated after this instead.
    m_isEnabled = false; // prevent frame() from creating a new m_context
}

void QWasmCompositor::onWindowTreeChanged(QWasmWindowTreeNodeChangeType changeType,
                                          QWasmWindow *window)
{
    auto allWindows = screen()->allWindows();
    setEnabled(std::any_of(allWindows.begin(), allWindows.end(), [](QWasmWindow *element) {
        return !element->context2d().isUndefined();
    }));
    if (changeType == QWasmWindowTreeNodeChangeType::NodeRemoval)
        m_requestUpdateWindows.remove(window);
}

void QWasmCompositor::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
}

// requestUpdate delivery is initially disabled at startup, while Qt completes
// startup tasks such as font loading. This function enables requestUpdate delivery
// again.
bool QWasmCompositor::releaseRequestUpdateHold()
{
    const bool wasEnabled = m_requestUpdateHoldEnabled;
    m_requestUpdateHoldEnabled = false;
    return wasEnabled;
}

void QWasmCompositor::requestUpdateWindow(QWasmWindow *window, UpdateRequestDeliveryType updateType)
{
    auto it = m_requestUpdateWindows.find(window);
    if (it == m_requestUpdateWindows.end()) {
        m_requestUpdateWindows.insert(window, updateType);
    } else {
        // Already registered, but upgrade ExposeEventDeliveryType to UpdateRequestDeliveryType.
        // if needed, to make sure QWindow::updateRequest's are matched.
        if (it.value() == ExposeEventDelivery && updateType == UpdateRequestDelivery)
            it.value() = UpdateRequestDelivery;
    }

    requestUpdate();
}

// Requests an update/new frame using RequestAnimationFrame
void QWasmCompositor::requestUpdate()
{
    if (m_requestAnimationFrameId != -1)
        return;

    if (m_requestUpdateHoldEnabled)
        return;

    static auto frame = [](double frameTime, void *context) -> EM_BOOL {
        Q_UNUSED(frameTime);

        QWasmCompositor *compositor = reinterpret_cast<QWasmCompositor *>(context);

        compositor->m_requestAnimationFrameId = -1;
        compositor->deliverUpdateRequests();

        return EM_FALSE;
    };
    m_requestAnimationFrameId = emscripten_request_animation_frame(frame, this);
}

void QWasmCompositor::deliverUpdateRequests()
{
    // We may get new update requests during the window content update below:
    // prepare for recording the new update set by setting aside the current
    // update set.
    auto requestUpdateWindows = m_requestUpdateWindows;
    m_requestUpdateWindows.clear();

    // Update window content, either all windows or a spesific set of windows. Use the correct
    // update type: QWindow subclasses expect that requested and delivered updateRequests matches
    // exactly.
    m_inDeliverUpdateRequest = true;
    for (auto it = requestUpdateWindows.constBegin(); it != requestUpdateWindows.constEnd(); ++it) {
        auto *window = it.key();
        UpdateRequestDeliveryType updateType = it.value();
        deliverUpdateRequest(window, updateType);
    }

    m_inDeliverUpdateRequest = false;
    frame(requestUpdateWindows.keys());
}

void QWasmCompositor::deliverUpdateRequest(QWasmWindow *window, UpdateRequestDeliveryType updateType)
{
    QWindow *qwindow = window->window();

    // Make sure the DPR value for the window is up to date on expose/repaint.
    // FIXME: listen to native DPR change events instead, if/when available.
    QWindowSystemInterface::handleWindowDevicePixelRatioChanged(qwindow);

    // Update by deliverUpdateRequest and expose event according to requested update
    // type. If the window has not yet been exposed then we must expose it first regardless
    // of update type. The deliverUpdateRequest must still be sent in this case in order
    // to maintain correct window update state.
    QRect updateRect(QPoint(0, 0), qwindow->geometry().size());
    if (updateType == UpdateRequestDelivery) {
        if (qwindow->isExposed() == false)
            QWindowSystemInterface::handleExposeEvent(qwindow, updateRect);
        window->deliverUpdateRequest();
    } else {
        QWindowSystemInterface::handleExposeEvent(qwindow, updateRect);
    }
}

void QWasmCompositor::handleBackingStoreFlush(QWindow *window)
{
    // Request update to flush the updated backing store content, unless we are currently
    // processing an update, in which case the new content will flushed as a part of that update.
    if (!m_inDeliverUpdateRequest)
        requestUpdateWindow(static_cast<QWasmWindow *>(window->handle()));
}

void QWasmCompositor::frame(const QList<QWasmWindow *> &windows)
{
    if (!m_isEnabled || !screen())
        return;

    for (QWasmWindow *window : windows)
        window->paint();
}

QWasmScreen *QWasmCompositor::screen()
{
    return static_cast<QWasmScreen *>(parent());
}
