// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmcompositor.h"
#include "qwasmwindow.h"

#include <qpa/qwindowsysteminterface.h>

#include <emscripten/html5.h>

namespace {

QWasmWindowStack::PositionPreference positionPreferenceFromWindowFlags(Qt::WindowFlags flags)
{
    if (flags.testFlag(Qt::WindowStaysOnTopHint))
        return QWasmWindowStack::PositionPreference::StayOnTop;
    if (flags.testFlag(Qt::WindowStaysOnBottomHint))
        return QWasmWindowStack::PositionPreference::StayOnBottom;
    return QWasmWindowStack::PositionPreference::Regular;
}

} // namespace

QWasmCompositor::QWasmCompositor(QWasmScreen *screen)
    : QObject(screen), m_windowStack(std::bind(&QWasmCompositor::onTopWindowChanged, this))
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

void QWasmCompositor::addWindow(QWasmWindow *window)
{
    if (m_windowStack.empty())
        window->window()->setFlag(Qt::WindowStaysOnBottomHint);
    m_windowStack.pushWindow(window, positionPreferenceFromWindowFlags(window->window()->flags()));
    window->requestActivateWindow();
    setActive(window);

    updateEnabledState();
}

void QWasmCompositor::removeWindow(QWasmWindow *window)
{
    m_requestUpdateWindows.remove(window);
    m_windowStack.removeWindow(window);
    if (m_windowStack.topWindow()) {
        m_windowStack.topWindow()->requestActivateWindow();
        setActive(m_windowStack.topWindow());
    }

    updateEnabledState();
}

void QWasmCompositor::setActive(QWasmWindow *window)
{
    m_activeWindow = window;

    auto it = m_windowStack.begin();
    if (it == m_windowStack.end()) {
        return;
    }
    for (; it != m_windowStack.end(); ++it) {
        (*it)->onActivationChanged(*it == m_activeWindow);
    }
}

void QWasmCompositor::updateEnabledState()
{
    m_isEnabled = std::any_of(m_windowStack.begin(), m_windowStack.end(), [](QWasmWindow *window) {
        return !window->context2d().isUndefined();
    });
}

void QWasmCompositor::raise(QWasmWindow *window)
{
    m_windowStack.raise(window);
}

void QWasmCompositor::lower(QWasmWindow *window)
{
    m_windowStack.lower(window);
}

void QWasmCompositor::windowPositionPreferenceChanged(QWasmWindow *window, Qt::WindowFlags flags)
{
    m_windowStack.windowPositionPreferenceChanged(window, positionPreferenceFromWindowFlags(flags));
}

QWindow *QWasmCompositor::windowAt(QPoint targetPointInScreenCoords, int padding) const
{
    const auto found = std::find_if(
            m_windowStack.begin(), m_windowStack.end(),
            [padding, &targetPointInScreenCoords](const QWasmWindow *window) {
                const QRect geometry = window->windowFrameGeometry().adjusted(-padding, -padding,
                                                                              padding, padding);

                return window->isVisible() && geometry.contains(targetPointInScreenCoords);
            });
    return found != m_windowStack.end() ? (*found)->window() : nullptr;
}

QWindow *QWasmCompositor::keyWindow() const
{
    return m_activeWindow ? m_activeWindow->window() : nullptr;
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

    static auto frame = [](double frameTime, void *context) -> int {
        Q_UNUSED(frameTime);

        QWasmCompositor *compositor = reinterpret_cast<QWasmCompositor *>(context);

        compositor->m_requestAnimationFrameId = -1;
        compositor->deliverUpdateRequests();

        return 0;
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
    // Update by deliverUpdateRequest and expose event according to requested update
    // type. If the window has not yet been exposed then we must expose it first regardless
    // of update type. The deliverUpdateRequest must still be sent in this case in order
    // to maintain correct window update state.
    QWindow *qwindow = window->window();
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

    std::for_each(windows.begin(), windows.end(), [](QWasmWindow *window) { window->paint(); });
}

void QWasmCompositor::onTopWindowChanged()
{
    constexpr int zOrderForElementInFrontOfScreen = 3;
    int z = zOrderForElementInFrontOfScreen;
    std::for_each(m_windowStack.rbegin(), m_windowStack.rend(),
                  [&z](QWasmWindow *window) { window->setZOrder(z++); });
}

QWasmScreen *QWasmCompositor::screen()
{
    return static_cast<QWasmScreen *>(parent());
}
