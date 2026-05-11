// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosapplicationstatetracker.h>

#include <QtCore/private/qohoslogger_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qwindow.h>
#include <memory>
#include <qohosplatformwindow.h>
#include <qohosutils.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <render/qohosview.h>
#include <set>
#include <tuple>

QT_BEGIN_NAMESPACE

namespace
{

using WindowSystemEventType = QWindowSystemInterfacePrivate::EventType;
using WindowSystemEvent = QWindowSystemInterfacePrivate::WindowSystemEvent;
using WindowStateChangedEvent = QWindowSystemInterfacePrivate::WindowStateChangedEvent;
using FocusWindowEvent = QWindowSystemInterfacePrivate::FocusWindowEvent;
using ExposeEvent = QWindowSystemInterfacePrivate::ExposeEvent;
using ApplicationStateChangedEvent = QWindowSystemInterfacePrivate::ApplicationStateChangedEvent;

enum class ProcessEventResult
{
    ForwardToGuiApplication,
    Ignore,
};

class ApplicationStateTracker final : public QWindowSystemEventHandler
{
public:
    bool sendEvent(WindowSystemEvent *event) override;

private:
    ProcessEventResult processWindowStateChangedEvent(WindowStateChangedEvent *event);
    ProcessEventResult processExposeEvent(ExposeEvent *event);
    ProcessEventResult processApplicationStateChangedEvent(ApplicationStateChangedEvent *event);

    void startTrackingViewIfNeeded(QOhosView *view);
    void tryUpdateApplicationState();

    std::set<QOhosView *> m_trackedViews;
    std::set<QOhosView *> m_visibleViews;
    std::set<QOhosView *> m_activeViews;

    QOhosOptional<Qt::ApplicationState> m_lastReceivedApplicationState;
    QOhosOptional<Qt::ApplicationState> m_lastSentApplicationState;
    QObject m_signalReceiver;
};

QOhosView *mapQWindowToViewOrNull(QWindow *window)
{
    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(window);
    return platformWindow != nullptr
        ? platformWindow->ownedViewOrNull()
        : nullptr;
}

ProcessEventResult ApplicationStateTracker::processWindowStateChangedEvent(WindowStateChangedEvent *evt)
{
    auto *view = evt->window != nullptr
        ? mapQWindowToViewOrNull(evt->window)
        : nullptr;

    if (view == nullptr)
        return ProcessEventResult::ForwardToGuiApplication;

    startTrackingViewIfNeeded(view);

    if (evt->newState.testFlag(Qt::WindowState::WindowActive))
        std::ignore = m_activeViews.insert(view);
    else
        std::ignore = m_activeViews.erase(view);

    return ProcessEventResult::ForwardToGuiApplication;
}

ProcessEventResult ApplicationStateTracker::processExposeEvent(ExposeEvent *evt)
{
    auto *view = evt->window != nullptr
        ? mapQWindowToViewOrNull(evt->window)
        : nullptr;

    if (view == nullptr)
        return ProcessEventResult::ForwardToGuiApplication;

    startTrackingViewIfNeeded(view);

    if (evt->isExposed)
        std::ignore = m_visibleViews.insert(view);
    else
        std::ignore = m_visibleViews.erase(view);

    return ProcessEventResult::ForwardToGuiApplication;
}

ProcessEventResult ApplicationStateTracker::processApplicationStateChangedEvent(ApplicationStateChangedEvent *evt)
{
    auto state = evt->newState;
    m_lastReceivedApplicationState = state;
    return ProcessEventResult::Ignore;
}

bool ApplicationStateTracker::sendEvent(WindowSystemEvent *event)
{
    QOhosOptional<ProcessEventResult> processEventResult;
    switch (event->type) {
    case WindowSystemEventType::WindowStateChanged:
        processEventResult = processWindowStateChangedEvent(static_cast<WindowStateChangedEvent *>(event));
        break;
    case WindowSystemEventType::Expose:
        processEventResult = processExposeEvent(static_cast<ExposeEvent *>(event));
        break;
    case WindowSystemEventType::ApplicationStateChanged:
        processEventResult = processApplicationStateChangedEvent(static_cast<ApplicationStateChangedEvent *>(event));
        break;
    default:
        break;
    }

    bool processEvent = processEventResult != ProcessEventResult::Ignore;
    if (processEvent)
        QGuiApplicationPrivate::processWindowSystemEvent(event);

    bool hasPendingEvents = QWindowSystemInterfacePrivate::windowSystemEventsQueued();
    if (!hasPendingEvents)
        tryUpdateApplicationState();

    return processEvent;
}

void ApplicationStateTracker::tryUpdateApplicationState()
{
    bool allWindowsInactive = m_activeViews.empty();
    bool allWindowsHidden = m_visibleViews.empty();

    auto updatedApplicationState =
        m_lastReceivedApplicationState == Qt::ApplicationSuspended
            ? Qt::ApplicationSuspended
            : allWindowsHidden
                ? Qt::ApplicationHidden
                :  allWindowsInactive
                    ? Qt::ApplicationInactive
                    : Qt::ApplicationActive;

    if (m_lastSentApplicationState == updatedApplicationState)
        return;

    m_lastSentApplicationState = updatedApplicationState;
    ApplicationStateChangedEvent eventToSend{updatedApplicationState};
    QGuiApplicationPrivate::processWindowSystemEvent(&eventToSend);
}

void ApplicationStateTracker::startTrackingViewIfNeeded(QOhosView *viewToTrack)
{
    bool added;
    std::tie(std::ignore, added) = m_trackedViews.insert(viewToTrack);
    if (added) {
        QObject::connect(
            viewToTrack, &QOhosView::destroyed,
            &m_signalReceiver, [this, viewToTrack](QObject *) {
                std::ignore = m_trackedViews.erase(viewToTrack);
                std::ignore = m_visibleViews.erase(viewToTrack);
                std::ignore = m_activeViews.erase(viewToTrack);
            });
    }
}

}

std::shared_ptr<QWindowSystemEventHandler> makeApplicationStateTracker()
{
    return std::make_shared<ApplicationStateTracker>();
}

QT_END_NAMESPACE

