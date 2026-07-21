// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosfloatingwindow.h>

#include <QtCore/private/qohoslogger_p.h>
#include "QtGui/private/qguiapplication_p.h"
#include <QtGui/private/qwindow_p.h>
#include <qohosdeviceinfo_p.h>
#include <qohosinputcontext.h>
#include <qohosinputmethodeventhandler.h>
#include <qohosjsmain.h>
#include <qohosplatformbackingstore.h>
#include <qohosplatformwindow.h>
#include <qohosruntimedevicetypeandmode.h>
#include <qohossettings.h>
#include <render/qohossurface.h>
#include <render/qohoswindowproxy.h>
#include <render/qwindowproxyregistry.h>
#include <utility>

QT_BEGIN_NAMESPACE

namespace {

bool isWindowRotatedByTabletScreenRotation(
    QWindow *window, QOhosWindowProxy::RectChangeOptions rectChangeOptions)
{
    return !QOhosSettings::instance().isWindowPcModeEnabled()
        && rectChangeOptions.rect.isValid()
        && window->geometry() != rectChangeOptions.rect
        && rectChangeOptions.reason == QOhosWindowProxy::RectChangeReason::UNDEFINED;
}

}

QOhosFloatingWindow::QOhosFloatingWindow(QWindow *window)
: QOhosPlatformWindow(window)
{
}

QOhosFloatingWindow::~QOhosFloatingWindow() = default;

void QOhosFloatingWindow::setGeometry(const QRect &rect)
{
    auto *view = ownedViewOrNull();
    bool geometryControlledBySystem =
        !QOhosSettings::instance().isWindowPcModeEnabled()
        && view != nullptr
        && view->viewType() == QOhosView::ViewType::MainWindow;

    if (geometryControlledBySystem) {
        setWindowGeometryFromOhos(windowGeometry());
        return;
    }

    auto *currentScreen = QOhosPlatformWindow::screen();
    QOhosPlatformWindow::setGeometry(rect);

    auto *targetScreen = QOhosPlatformWindow::screenForGeometry(rect.marginsAdded(frameMargins()));
    if (targetScreen != nullptr && targetScreen != currentScreen)
        QWindowSystemInterface::handleWindowScreenChanged(window(), targetScreen->screen());

    if (view != nullptr) {
        auto frameGeometry = rect.marginsAdded(frameMargins());
        if (!qt_window_private(window())->positionAutomatic)
            view->setPosition(frameGeometry.topLeft());
        view->setSize(frameGeometry.size());
    }
}

void QOhosFloatingWindow::setVisible(bool visible)
{
    if (!visible)
        m_view->hide();
    else
        m_view->showImmediate();
    startAsyncWaitForNodeResizeIfNeeded();
}

WId QOhosFloatingWindow::winId() const
{
    auto windowObjectName = window()->objectName();

    qOhosPrintfDebug(
        "%s: winId called for window named: \"%s\"",
        Q_FUNC_INFO,
        windowObjectName.toStdString().c_str());

    return reinterpret_cast<WId>(m_view->viewWindowId());
}

void QOhosFloatingWindow::raise()
{
    auto *view = ownedViewOrNull();
    if (view != nullptr) {
        view->raise();
        window()->requestUpdate();
    }
}

void QOhosFloatingWindow::lower()
{
    auto *view = ownedViewOrNull();
    if (view != nullptr) {
        view->lower();
        window()->requestUpdate();
    }
}

QOhosSurface *QOhosFloatingWindow::ownedSurfaceOrNull() const
{
    auto *view = ownedViewOrNull();
    return view != nullptr ? view->surfaceOrNull() : nullptr;
}

QOhosView *QOhosFloatingWindow::ownedViewOrNull() const
{
    return m_view.get();
}

void QOhosFloatingWindow::initialize()
{
    QOhosPlatformWindow::initialize();
    m_view = QOhosView::createForWindow(this, propertiesProvider());

    auto *qWindow = window();

    // HACK
    // There is no functionality to fetch the initial window status from ohos
    // We can only listen for the changes - we need to assume sane default for tablets
    // so assign the default most-likely status of the tablet here.
    if (m_view->viewType() == QOhosView::ViewType::MainWindow
        && !QOhosSettings::instance().isWindowPcModeEnabled()
        && !m_lastWindowStatusType.has_value()) {
        m_lastWindowStatusType = QOhosWindowProxy::WindowStatusType::FULL_SCREEN;
    }

    QObject::connect(
        m_view.get(), &QOhosView::externalContentInteractionDetected,
        qWindow, &QOhosPlatformWindow::closeAllActivePopups);

    QObject::connect(
        m_view.get(), &QOhosView::nodeAreaChanged, m_view.get(),
        [this](QArkUi::QQtEmbeddedWindowNode::NodeAreaInfo event) {
            handleNodeResizeEvent(event);
        });

    QObject::connect(
        m_view.get(), &QOhosView::windowEvent,
        qWindow,
        [this](QOhosWindowProxy::WindowEvent evt) { handleWindowEvent(evt); });

    QObject::connect(
        m_view.get(), &QOhosView::windowStatusChange,
        qWindow,
        [this](QOhosWindowProxy::WindowStatus evt) { handleWindowStatusChange(evt); });

    QObject::connect(
        m_view.get(), &QOhosView::windowVisibilityChange,
        qWindow,
        [this](bool visible) { handleWindowVisibilityChange(visible); });

    bool monitorAvoidAreaChange =
        !(isHandheldDeviceType() && m_view->viewType() == QOhosView::ViewType::SubWindow);

    if (monitorAvoidAreaChange) {
        QObject::connect(
            m_view.get(), &QOhosView::avoidAreaChanged,
            qWindow,
            [this](QOhosWindowProxy::AvoidAreaType avoidAreaType,
                   const QOhosWindowProxy::AvoidArea &systemAvoidArea) {
                handleAvoidAreaChanged(avoidAreaType, systemAvoidArea);
            });
    }

    QObject::connect(
        m_view.get(), &QOhosView::windowRectChangedInGlobalDisplay,
        qWindow,
        [this](const QOhosWindowProxy::RectChangeOptions &rectChangeOptions) {
            handleWindowRectChanged(rectChangeOptions);
        });

    QObject::connect(qWindow, &QWindow::modalityChanged, m_view.get(), &QOhosView::setModality);

    QObject::connect(
        m_view.get(), &QOhosView::surfaceStatusChanged,
        qWindow,
        [this](const std::optional<QSize> &optSurfaceSize) { handleSurfaceStatusChanged(optSurfaceSize); });

    QObject::connect(qWindow, &QWindow::windowTitleChanged, m_view.get(), &QOhosView::setTitle);

    QObject::connect(
        m_view.get(), &QOhosView::windowDisplayIdChanged,
        qWindow,
        [this](QOhosDisplayInfo::JsDisplayId displayId) { handleWindowDisplayIdChanged(displayId); });
}

void QOhosFloatingWindow::restoreWindowCurrentCursorIfNeeded()
{
    auto *view = ownedViewOrNull();
    if (view != nullptr && m_cursor.has_value())
        view->setCursor(m_cursor.value());
}

void QOhosFloatingWindow::onWindowFlagsChanged(
    Qt::WindowFlags previousWindowFlags, Qt::WindowFlags currentWindowFlags)
{
    auto *view = ownedViewOrNull();
    if (view == nullptr)
        return;

    view->handleWindowFlagsChange(previousWindowFlags, currentWindowFlags);
}

void QOhosFloatingWindow::onWindowStateChanged(
    Qt::WindowStates oldWindowState, Qt::WindowStates currentWindowState)
{
    auto *view = ownedViewOrNull();
    if (view == nullptr)
        return;

    view->handleWindowStateChange(oldWindowState, currentWindowState);
}

void QOhosFloatingWindow::internalHijackSystemFocusAsPopup()
{
    QWindowSystemInterface::handleFocusWindowChanged(window(), Qt::PopupFocusReason);
}

void QOhosFloatingWindow::focusHijackingPopupHidden()
{
    auto systemFocusedWindows = QWindowProxyRegistry::instance().queryWindowsWithSystemWindowAndFocus();
    QWindowSystemInterface::handleFocusWindowChanged(
        !systemFocusedWindows.empty() ? systemFocusedWindows.front() : nullptr,
        Qt::ActiveWindowFocusReason);
}

void QOhosFloatingWindow::setMask(const QRegion &region)
{
    m_windowMask = region;

    auto *view = ownedViewOrNull();
    if (view == nullptr)
        return;

    view->setWindowMask(QOhosWindowProxy::WindowMask{region});
}

bool QOhosFloatingWindow::startSystemMove()
{
    auto *view = ownedViewOrNull();
    if (view != nullptr)
        return view->startMoving();
    return false;
}

void QOhosFloatingWindow::requestActivateWindow()
{
    if (windowFlags().testFlag(Qt::WindowDoesNotAcceptFocus) && window()->type() == Qt::Popup) {
        internalHijackSystemFocusAsPopup();
        return;
    }

    QOhosPlatformWindow::requestActivateWindow();
}

void QOhosFloatingWindow::handleWindowEvent(QOhosWindowProxy::WindowEvent evt)
{
    auto *qWindow = window();
    bool windowAcceptsFocusAndInput = checkWindowAcceptsFocus() && checkWindowAcceptsInput();
    Qt::WindowStates windowStatesToSet = windowStates();
    bool windowActive = true;
    auto previousWindowEventType = std::exchange(m_lastWindowEventType, evt.type);

    switch (evt.type) {
    case QOhosWindowProxy::WindowEventType::WINDOW_ACTIVE:
    {
        QWindow *modalWindow = nullptr;
        if (QGuiApplicationPrivate::instance()->isWindowBlocked(qWindow, &modalWindow)
            && qWindow != modalWindow) {
            modalWindow->requestActivate();
            return;
        }

        // FIXME - restore cursor set by the User every time window is activated.
        // Perform it because Ohos overrides it with default "standard arrow" cursor (when
        // window is deactivated) and does not bring selected cursor back (when window is
        // activated again). This bahaviour is Ohos platform issue.
        restoreWindowCurrentCursorIfNeeded();
        if (windowAcceptsFocusAndInput) {
            QWindowSystemInterface::handleFocusWindowChanged(qWindow, Qt::ActiveWindowFocusReason);
            notifyInputSystemsWindowActiveStatusChanged(true);
        }
        break;
    }
    case QOhosWindowProxy::WindowEventType::WINDOW_INACTIVE:
        windowActive = false;
        if (!windowAcceptsFocusAndInput)
            break;
        if (QGuiApplicationPrivate::focus_window == qWindow
            || (QGuiApplicationPrivate::focus_window != nullptr
                && QOhosPlatformWindow::platformWindowFlagsForQWindow(
                    QGuiApplicationPrivate::focus_window).testFlag(Qt::WindowDoesNotAcceptFocus))) {
            QWindowSystemInterface::handleFocusWindowChanged(nullptr, Qt::ActiveWindowFocusReason);
        }
        notifyInputSystemsWindowActiveStatusChanged(false);
        break;
    case QOhosWindowProxy::WindowEventType::WINDOW_HIDDEN:
        if (!checkWindowAcceptsFocus() && QGuiApplicationPrivate::focus_window == qWindow)
            focusHijackingPopupHidden();
        setExposedFromOhos(false);
        windowActive = false;
        break;
    case QOhosWindowProxy::WindowEventType::WINDOW_SHOWN:
        // HACK
        // This is a fix for the windows hidden using `QWidget::hide()`.
        // When a window is hidden and then bring back from the system Dock
        // Qt side keeps wrong (hidden) state. This causes different issues due to
        // improper state. To fix it - change visibility state based on the system
        // window state.
        if (previousWindowEventType == QOhosWindowProxy::WindowEventType::WINDOW_HIDDEN && !qWindow->isVisible())
            qWindow->setVisible(true);
        setExposedFromOhos(true);
        startAsyncWaitForNodeResizeIfNeeded();
        break;
    case QOhosWindowProxy::WindowEventType::WINDOW_DESTROYED:
        windowActive = false;
        notifyWindowDestroyedFromOhos();
        return;
    }

    windowStatesToSet.setFlag(Qt::WindowState::WindowActive, windowActive && windowAcceptsFocusAndInput);
    setWindowStateFromOhos(windowStatesToSet);
}

void QOhosFloatingWindow::handleWindowStatusChange(QOhosWindowProxy::WindowStatus evt)
{
    auto *qWindow = window();
    qCDebug(
        QtForOhos,
        "Window status changed, window: %p(%s) status: %d",
        qWindow,
        qPrintable(qWindow->objectName()),
        evt.type);

    // Drop duplicate FULL_SCREEN notifications; MainWindow views in non-PC mode
    // are exempt because initialize() pre-seeds their status to FULL_SCREEN.
    if (m_lastWindowStatusType == evt.type
        && m_lastWindowStatusType == QOhosWindowProxy::WindowStatusType::FULL_SCREEN
        && !(m_view->viewType() == QOhosView::ViewType::MainWindow
             && !QOhosSettings::instance().isWindowPcModeEnabled())) {
        qCDebug(QtForOhos, "Window status is the same as the previous one");
        return;
    }

    Qt::WindowStates windowStatesToSet = windowStates();
    Qt::WindowState flagToSet;
    m_lastWindowStatusType = evt.type;

    switch (evt.type) {
    case QOhosWindowProxy::WindowStatusType::FULL_SCREEN:
        flagToSet = Qt::WindowState::WindowFullScreen;
        break;
    case QOhosWindowProxy::WindowStatusType::MAXIMIZE:
        flagToSet = Qt::WindowState::WindowMaximized;
        break;
    case QOhosWindowProxy::WindowStatusType::MINIMIZE:
        flagToSet = Qt::WindowState::WindowMinimized;
        break;
    case QOhosWindowProxy::WindowStatusType::UNDEFINED:
    case QOhosWindowProxy::WindowStatusType::FLOATING:
        Q_FALLTHROUGH();
    case QOhosWindowProxy::WindowStatusType::SPLIT_SCREEN:
        flagToSet = Qt::WindowState::WindowNoState;
        break;
    }

    // NOTE - Qt::WindowFullScreen and Qt::WindowMaximized represent the exact same
    // OHOS Window state.
    if (flagToSet == Qt::WindowState::WindowFullScreen
        || flagToSet == Qt::WindowState::WindowMaximized) {
        windowStatesToSet = m_view->isFullscreenImmersiveModeEnabled()
            ? Qt::WindowState::WindowFullScreen
            : Qt::WindowState::WindowMaximized;
    } else {
        static constexpr Qt::WindowState mutuallyExclusiveFlags[] = {
            Qt::WindowState::WindowFullScreen,
            Qt::WindowState::WindowMaximized,
            Qt::WindowState::WindowMinimized,
            Qt::WindowState::WindowNoState,
        };

        for (const auto exclusiveState : mutuallyExclusiveFlags)
            windowStatesToSet.setFlag(exclusiveState, flagToSet == exclusiveState);
    }

    setWindowStateFromOhos(windowStatesToSet);

    // HACK
    // There is no functionality in OHOS to fetch the window status that would allow checking
    // the value while processing the WINDOW_SHOWN window event type. Therefore, the window
    // geometry view needs to be recalculated with any change.
    startAsyncWaitForNodeResizeIfNeeded();
}

void QOhosFloatingWindow::handleWindowVisibilityChange(bool visible)
{
    auto *qWindow = window();
    qCDebug(
        QtForOhos,
        "Window %p(%s) visibility changed: %s",
        qWindow,
        qPrintable(qWindow->objectName()),
        visible ? "true" : "false");
    if (visible && m_windowMask.has_value())
        m_view->setWindowMask(QOhosWindowProxy::WindowMask{m_windowMask.value()});

    setExposedFromOhos(visible);
}

void QOhosFloatingWindow::handleAvoidAreaChanged(
    QOhosWindowProxy::AvoidAreaType avoidAreaType,
    const QOhosWindowProxy::AvoidArea &systemAvoidArea)
{
    const auto &cached = m_avoidAreaCache[avoidAreaType];

    bool actuallyChanged =
        cached.visible != systemAvoidArea.visible
        || cached.leftRect != systemAvoidArea.leftRect
        || cached.rightRect != systemAvoidArea.rightRect
        || cached.bottomRect != systemAvoidArea.bottomRect
        || cached.topRect != systemAvoidArea.topRect;

    if (actuallyChanged) {
        m_avoidAreaCache[avoidAreaType] = systemAvoidArea;
        qCDebug(QtForOhos) << "Avoid area changed:"
            << static_cast<int>(avoidAreaType)
            << "visible:" << systemAvoidArea.visible
            << "top:" <<  systemAvoidArea.topRect
            << "left:" <<  systemAvoidArea.leftRect
            << "right:" <<  systemAvoidArea.rightRect
            << "bottom:" <<  systemAvoidArea.bottomRect;
        startAsyncWaitForNodeResizeIfNeeded();
    }
}

void QOhosFloatingWindow::handleSurfaceStatusChanged(const std::optional<QSize> &optSurfaceSize)
{
    m_optLastSurfaceSize = optSurfaceSize;
    bool hasSurface = m_view->surfaceOrNull() != nullptr;
    if (m_view->viewType() == QOhosView::ViewType::EmbeddedWindow) {
        setExposedFromOhos(hasSurface);
    }

    if (hasSurface)
        startAsyncWaitForNodeResizeIfNeeded();
}

void QOhosFloatingWindow::handleWindowDisplayIdChanged(QOhosDisplayInfo::JsDisplayId displayId)
{
    setDisplayIdFromOhos(displayId);
    startAsyncWaitForNodeResizeIfNeeded();
}

void QOhosFloatingWindow::handleWindowRectChanged(
    const QOhosWindowProxy::RectChangeOptions &rectChangeOptions)
{
    bool needsCloseAllActivePopups = rectChangeOptions.reason == QOhosWindowProxy::RectChangeReason::DRAG_START;

    qCDebug(QtForOhos)
        << "windowRectChanged window:" << window()
        << "rect:" << rectChangeOptions.rect
        << "reason:" << static_cast<int>(rectChangeOptions.reason);

    startAsyncWaitForNodeResizeIfNeeded();

    if (isWindowRotatedByTabletScreenRotation(window(), rectChangeOptions))
        needsCloseAllActivePopups = true;

    if (needsCloseAllActivePopups)
        QOhosPlatformWindow::closeAllActivePopups();
}

bool QOhosFloatingWindow::windowEvent(QEvent *event)
{
    if (event->type() == QEvent::Timer) {
        auto *timerEvent = static_cast<QTimerEvent *>(event);
        if (m_view && timerEvent->timerId() == m_geometryChangeTimer.timerId()) {
            auto syntheticEvent = m_view->nodeAreaInfo();
            handleNodeResizeEvent(syntheticEvent);
        }
    }

    return QOhosPlatformWindow::windowEvent(event);
}

void QOhosFloatingWindow::startAsyncWaitForNodeResizeIfNeeded()
{
    constexpr auto geometryChangeEventTimeoutMs = std::chrono::milliseconds(16);

    if (!m_geometryChangeTimer.isActive()) {
        m_geometryChangeTimer.start(
            geometryChangeEventTimeoutMs.count(),
            Qt::PreciseTimer,
            window());
    }
}

void QOhosFloatingWindow::handleNodeResizeEvent(const QArkUi::QQtEmbeddedWindowNode::NodeAreaInfo &areaChangeEvent)
{
    m_geometryChangeTimer.stop();

    if (Q_UNLIKELY(!m_view))
        return;

    setWindowGeometryFromOhos(
        m_view->viewType() != QOhosView::ViewType::EmbeddedWindow
            ? QRect(areaChangeEvent.globalRelativeOffsetPixels, areaChangeEvent.screenGeometryPixels.size())
            : QRect(areaChangeEvent.parentRelativeOffsetPixels, areaChangeEvent.screenGeometryPixels.size()));
}

QT_END_NAMESPACE
