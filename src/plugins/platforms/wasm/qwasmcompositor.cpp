// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmcompositor.h"
#include "qwasmwindow.h"
#include "qwasmeventdispatcher.h"
#include "qwasmclipboard.h"
#include "qwasmevent.h"

#include <QtGui/private/qwindow_p.h>

#include <private/qguiapplication_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qguiapplication.h>

#include <emscripten/bind.h>

using namespace emscripten;

Q_GUI_EXPORT int qt_defaultDpiX();

QWasmCompositor::QWasmCompositor(QWasmScreen *screen)
    : QObject(screen), m_windowStack(std::bind(&QWasmCompositor::onTopWindowChanged, this))
{
    QWindowSystemInterface::setSynchronousWindowSystemEvents(true);
}

QWasmCompositor::~QWasmCompositor()
{
    if (m_requestAnimationFrameId != -1)
        emscripten_cancel_animation_frame(m_requestAnimationFrameId);

    destroy();
}

void QWasmCompositor::onScreenDeleting()
{
    deregisterEventHandlers();
}

void QWasmCompositor::deregisterEventHandlers()
{
    QByteArray screenElementSelector = screen()->eventTargetId().toUtf8();

    emscripten_set_touchstart_callback(screenElementSelector.constData(), 0, 0, NULL);
    emscripten_set_touchend_callback(screenElementSelector.constData(), 0, 0, NULL);
    emscripten_set_touchmove_callback(screenElementSelector.constData(), 0, 0, NULL);

    emscripten_set_touchcancel_callback(screenElementSelector.constData(), 0, 0, NULL);
}

void QWasmCompositor::destroy()
{
    // TODO(mikolaj.boc): Investigate if m_isEnabled is needed at all. It seems like a frame should
    // not be generated after this instead.
    m_isEnabled = false; // prevent frame() from creating a new m_context
}

void QWasmCompositor::addWindow(QWasmWindow *window)
{
    m_windowStack.pushWindow(window);
    m_windowStack.topWindow()->requestActivateWindow();

    updateEnabledState();
}

void QWasmCompositor::removeWindow(QWasmWindow *window)
{
    m_requestUpdateWindows.remove(window);
    m_windowStack.removeWindow(window);
    if (m_windowStack.topWindow())
        m_windowStack.topWindow()->requestActivateWindow();

    updateEnabledState();
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
    return m_windowStack.topWindow() ? m_windowStack.topWindow()->window() : nullptr;
}

void QWasmCompositor::requestUpdateAllWindows()
{
    m_requestUpdateAllWindows = true;
    requestUpdate();
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
    bool requestUpdateAllWindows = m_requestUpdateAllWindows;
    m_requestUpdateAllWindows = false;

    // Update window content, either all windows or a spesific set of windows. Use the correct
    // update type: QWindow subclasses expect that requested and delivered updateRequests matches
    // exactly.
    m_inDeliverUpdateRequest = true;
    if (requestUpdateAllWindows) {
        for (QWasmWindow *window : m_windowStack) {
            auto it = requestUpdateWindows.find(window);
            UpdateRequestDeliveryType updateType =
                (it == m_requestUpdateWindows.end() ? ExposeEventDelivery : it.value());
            deliverUpdateRequest(window, updateType);
        }
    } else {
        for (auto it = requestUpdateWindows.constBegin(); it != requestUpdateWindows.constEnd(); ++it) {
            auto *window = it.key();
            UpdateRequestDeliveryType updateType = it.value();
            deliverUpdateRequest(window, updateType);
        }
    }
    m_inDeliverUpdateRequest = false;
    frame(requestUpdateAllWindows, requestUpdateWindows.keys());
}

void QWasmCompositor::deliverUpdateRequest(QWasmWindow *window, UpdateRequestDeliveryType updateType)
{
    // update by deliverUpdateRequest and expose event accordingly.
    if (updateType == UpdateRequestDelivery) {
        window->QPlatformWindow::deliverUpdateRequest();
    } else {
        QWindow *qwindow = window->window();
        QWindowSystemInterface::handleExposeEvent(
            qwindow, QRect(QPoint(0, 0), qwindow->geometry().size()));
    }
}

void QWasmCompositor::handleBackingStoreFlush(QWindow *window)
{
    // Request update to flush the updated backing store content, unless we are currently
    // processing an update, in which case the new content will flushed as a part of that update.
    if (!m_inDeliverUpdateRequest)
        requestUpdateWindow(static_cast<QWasmWindow *>(window->handle()));
}

int dpiScaled(qreal value)
{
    return value * (qreal(qt_defaultDpiX()) / 96.0);
}

void QWasmCompositor::frame(bool all, const QList<QWasmWindow *> &windows)
{
    if (!m_isEnabled || m_windowStack.empty() || !screen())
        return;

    if (all) {
        std::for_each(m_windowStack.rbegin(), m_windowStack.rend(),
                      [](QWasmWindow *window) { window->paint(); });
    } else {
        std::for_each(windows.begin(), windows.end(), [](QWasmWindow *window) { window->paint(); });
    }
}

void QWasmCompositor::onTopWindowChanged()
{
    constexpr int zOrderForElementInFrontOfScreen = 3;
    int z = zOrderForElementInFrontOfScreen;
    std::for_each(m_windowStack.rbegin(), m_windowStack.rend(),
                  [&z](QWasmWindow *window) { window->setZOrder(z++); });

    auto it = m_windowStack.begin();
    if (it == m_windowStack.end()) {
        return;
    }
    (*it)->onActivationChanged(true);
    ++it;
    for (; it != m_windowStack.end(); ++it) {
        (*it)->onActivationChanged(false);
    }
}

QWasmScreen *QWasmCompositor::screen()
{
    return static_cast<QWasmScreen *>(parent());
}
