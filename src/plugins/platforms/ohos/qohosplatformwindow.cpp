// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosplatformwindow.h>

#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qpointer.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qwindow_p.h>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <qguiapplication.h>
#include <qohosdeviceinfo_p.h>
#include <qohosinputcontext.h>
#include <qohosinputmethodeventhandler.h>
#include <qohosjsmain.h>
#include <qohosplatformintegration.h>
#include <qohosplatformscreen.h>
#include <qohosqpafunctions_p.h>
#include <qohosruntimedevicetypeandmode.h>
#include <qohossettings.h>
#include <qohosutils.h>
#include <qpa/qwindowsysteminterface.h>
#include <render/qohosview.h>
#include <private/qwindow_p.h>
#include <utility>

QT_BEGIN_NAMESPACE

namespace
{

constexpr int defaultWindowWidth = 160;
constexpr int defaultWindowHeight = 160;

QOhosView *getWindowsViewOrNull(QWindow *targetWindow)
{
    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(targetWindow);
    if (platformWindow == nullptr) {
        qCWarning(QtForOhos, "%s: Target window does not contain PlatformWindow", Q_FUNC_INFO);
        return nullptr;
    }

    auto *view = platformWindow->ownedViewOrNull();
    if (view == nullptr) {
        qCCritical(QtForOhos, "%s: Target window does not contain a view", Q_FUNC_INFO);
        return nullptr;
    }

    return view;
}

}

const QOhosPropertyDescriptor<QWindow *> QOhosPlatformWindow::subWindowOfTagProperty;
const QOhosPropertyDescriptor<bool> QOhosPlatformWindow::mainWindowTagProperty;
const QOhosPropertyDescriptor<bool> QOhosPlatformWindow::floatWindowTagProperty;
const QOhosPropertyDescriptor<double> QOhosPlatformWindow::windowCornerRadiusProperty;
const QOhosPropertyDescriptor<bool> QOhosPlatformWindow::windowPrivacyModeSettingProperty;
const QOhosPropertyDescriptor<QColor> QOhosPlatformWindow::surfaceBackgroundColorProperty;
const QOhosPropertyDescriptor<QOhosPlatformWindow::NativeNodeRenderFitPolicy> QOhosPlatformWindow::nativeNodeRenderFitPolicyHintProperty;
const QOhosPropertyDescriptor<bool> QOhosPlatformWindow::windowKeepScreenOnProperty;
const QOhosPropertyDescriptor<bool> QOhosPlatformWindow::windowDragResizableProperty;
const QOhosPropertyDescriptor<bool> QOhosPlatformWindow::windowFixedSizeStateProperty;
const QOhosPropertyDescriptor<int> QOhosPlatformWindow::windowBrightnessProperty;
const QOhosPropertyDescriptor<int> QOhosPlatformWindow::windowContrastProperty;
const QOhosPropertyDescriptor<int> QOhosPlatformWindow::windowSaturationProperty;

QOhosPlatformWindow::QOhosPlatformWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_propertiesStore(window)
{
    m_windowFlags = Qt::Widget;
    m_windowState = window->windowStates();
    m_windowId = QtOhos::InternalWindowId::generate();
}

void QOhosPlatformWindow::setGeometry(const QRect &rect)
{
    QRect adjustedRect = rect;
    if (qt_window_private(const_cast<QWindow *>(window()))->positionPolicy
        == QWindowPrivate::WindowFrameInclusive) {
        const auto margins = frameMargins();
        adjustedRect.adjust(margins.left(), margins.top(), -margins.right(), -margins.bottom());
    }

    qOhosPrintfDebug(
        "%s: pos: %d,%d size: %d,%d",
        Q_FUNC_INFO,
        adjustedRect.x(), adjustedRect.y(),
        adjustedRect.width(), adjustedRect.height());
    QPlatformWindow::setGeometry(adjustedRect);
}

bool QOhosPlatformWindow::canBeShownOnScreen() const
{
    QRect availableGeometry = screen()->availableGeometry();
    return geometry().width() > 0
        && geometry().height() > 0
        && availableGeometry.width() > 0
        && availableGeometry.height() > 0;
}

void QOhosPlatformWindow::setVisible(bool visible)
{
    if (canBeShownOnScreen())
        QPlatformWindow::setVisible(visible);
}

void QOhosPlatformWindow::setCursor(const QCursor &cursor)
{
    auto *view = ownedViewOrNull();
    if (view != nullptr) {
        m_cursor = cursor;
        view->setCursor(cursor);
    }
}

void QOhosPlatformWindow::setWindowTitle(const QString &title)
{
    auto *view = ownedViewOrNull();
    if (view != nullptr)
        view->setTitle(title);
}

void QOhosPlatformWindow::setParent(const QPlatformWindow *newParent)
{
    if (newParent != nullptr && newParent->isForeignWindow())
        qOhosReportFatalErrorAndAbort("Reparenting to foreign windows is not supported");

    m_parent = parent();

    auto *view = ownedViewOrNull();

    if (newParent == nullptr) {
        view->tryDetachFromEmbeddedParent();
        return;
    }

    auto *parentView = static_cast<const QOhosPlatformWindow *>(newParent)->ownedViewOrNull();
    view->setParentOrReparent(*parentView);
}

QPlatformScreen *QOhosPlatformWindow::screen() const
{
    if (!m_displayId.hasValue())
        return QPlatformWindow::screen();

    const auto &screenManager = *QOhosPlatformIntegration::instance()->screenManager();
    auto *platformScreen = screenManager.platformScreenForDisplayIdOrNull(m_displayId.value());
    if (platformScreen != nullptr)
        return platformScreen;

    qCWarning(QtForOhos)
        << Q_FUNC_INFO << "window:" << window()
        << "display id" << m_displayId.value().value()
        << "has no platform screen. Returning QWindow associated one.";

    return QPlatformWindow::screen();
}

void QOhosPlatformWindow::setWindowState(Qt::WindowStates state)
{
    if (m_windowState == state)
        return;

    auto oldWindowState = std::exchange(m_windowState, state);
    onWindowStateChanged(oldWindowState, m_windowState);
}

void QOhosPlatformWindow::setWindowFlags(Qt::WindowFlags flags)
{
    static QSet<Qt::WindowType> disableFocusableWindowTypes{Qt::ToolTip, Qt::Popup};

    auto *qWindow = window();
    auto correctedFlags = flags;
    bool ohosOverrideDisableFocusableFeatures = disableFocusableWindowTypes.contains(qWindow->type());
    if (ohosOverrideDisableFocusableFeatures) {
        qCDebug(
            QtForOhos,
            "Setting Qt::WindowDoesNotAcceptFocus flag on window %s(%s) because of its type",
            qPrintable(internalWindowId().toString()),
            qPrintable(qWindow->objectName()));
        correctedFlags.setFlag(Qt::WindowDoesNotAcceptFocus, true);
    }

    if (correctedFlags.testFlag(Qt::FramelessWindowHint)) {
        correctedFlags.setFlag(Qt::WindowMaximizeButtonHint, false);
        correctedFlags.setFlag(Qt::WindowMinimizeButtonHint, false);
        correctedFlags.setFlag(Qt::WindowCloseButtonHint, false);
    } else if (!correctedFlags.testFlag(Qt::CustomizeWindowHint)) {
        correctedFlags.setFlag(Qt::WindowMaximizeButtonHint, true);
        correctedFlags.setFlag(Qt::WindowMinimizeButtonHint, true);
        correctedFlags.setFlag(Qt::WindowCloseButtonHint, true);
    }

    auto previousWindowFlags = std::exchange(m_windowFlags, correctedFlags);
    onWindowFlagsChanged(previousWindowFlags, m_windowFlags);
}

Qt::WindowFlags QOhosPlatformWindow::windowFlags() const
{
    return m_windowFlags;
}

QOhosPlatformScreen *QOhosPlatformWindow::platformScreen() const
{
    return static_cast<QOhosPlatformScreen *>(screen());
}

QOhosOptional<QOhosDisplayInfo::JsDisplayId> QOhosPlatformWindow::tryTakeLastRequestedDisplayId()
{
    return std::exchange(m_lastRequestedDisplayId, makeEmptyQOhosOptional());
}

void QOhosPlatformWindow::propagateSizeHints()
{
    auto *view = ownedViewOrNull();
    if (view == nullptr)
        return;

    setWindowOrWidgetProperty<bool, &windowFixedSizeStateProperty>(
        window(), windowMinimumSize() == windowMaximumSize());

    view->setSizeLimits(windowMinimumSize(), windowMaximumSize());
}

bool QOhosPlatformWindow::isExposed() const
{
    return m_exposed;
}

QtOhos::InternalWindowId QOhosPlatformWindow::internalWindowId() const
{
    return m_windowId;
}

bool QOhosPlatformWindow::shouldDisplayAsOhosWindow() const
{
    return window()->isTopLevel();
}

QOhosPlatformWindow::DecorationPreset QOhosPlatformWindow::decorationPreset() const
{
    constexpr Qt::WindowType showWithoutDecorationWindowTypes[] = {
        Qt::WindowType::Popup,
        Qt::WindowType::SplashScreen,
        Qt::WindowType::ToolTip,
    };

    auto *qWindow = window();
    bool showWithoutDecoration =
        qWindow->parent() != nullptr
        || std::find(
                std::begin(showWithoutDecorationWindowTypes),
                std::end(showWithoutDecorationWindowTypes),
                qWindow->type()) != std::end(showWithoutDecorationWindowTypes)
        || windowFlags().testFlag(Qt::WindowType::FramelessWindowHint)
        || floatWindowTagValueOrFalse();

    return showWithoutDecoration
        ? DecorationPreset::Frameless
        : DecorationPreset::Standard;
}

QMargins QOhosPlatformWindow::frameMargins() const
{
    if (m_optFrameMargins)
        return *m_optFrameMargins;

    // TODO: Read this information from the system in the future
    // getWindowDecorHeight: https://developer.huawei.com/consumer/en/doc/harmonyos-references/arkts-apis-window-window#getwindowdecorheight11
    // WindowProperties.drawableRect: https://developer.huawei.com/consumer/en/doc/harmonyos-references/arkts-apis-window-i#windowproperties
    // Check if its viable and adapt the frameGeometry code in window to use getWIndowDecorHeight.

    const auto *screen = platformScreen();
    if (screen == nullptr)
        return QMargins{};

    if (decorationPreset() == DecorationPreset::Standard) {
        // https://gitcode.com/openharmony/window_window_manager/tree/master/utils/include/wm_common_inner.h
        constexpr auto predefinedWindowTitleBarHeight = 37;
        const int titlebarHeightPixels = std::round(predefinedWindowTitleBarHeight * screen->pixelScalingCoefficient());
        return QMargins{0, titlebarHeightPixels, 0, 0};
    }

    return QMargins{};
}

QOhosSurface *QOhosPlatformWindow::ownedSurfaceOrNull() const
{
    return nullptr;
}

QOhosPlatformWindow *QOhosPlatformWindow::fromQWindowOrNull(QWindow *window)
{
    auto *platformWindow = window->handle();
    return platformWindow != nullptr
        ? static_cast<QOhosPlatformWindow *>(platformWindow)
        : nullptr;
}

QOhosPlatformWindow *QOhosPlatformWindow::fromQWindow(QWindow *window)
{
    QOhosPlatformWindow *platformWindow = fromQWindowOrNull(window);
    if (platformWindow == nullptr)
        qOhosReportFatalErrorAndAbort("QWindow %s does not have QPlatformWindow", qPrintable(window->objectName()));
    return platformWindow;
}

void QOhosPlatformWindow::tagWindowOrWidgetAsSubWindowOf(QObject *windowOrWidgetToTag, QWindow *targetMainWindow)
{
    setWindowOrWidgetProperty<QWindow *, &subWindowOfTagProperty>(windowOrWidgetToTag, targetMainWindow);
}

void QOhosPlatformWindow::tagWindowOrWidgetAsMainWindow(QObject *windowOrWidgetToTag, bool forceMainWindow)
{
    setWindowOrWidgetProperty<bool, &mainWindowTagProperty>(windowOrWidgetToTag, forceMainWindow);
}

void QOhosPlatformWindow::tagWindowOrWidgetAsFloatWindow(
    QObject *windowOrWidgetToTag, bool showAsFloatWindow)
{
    setWindowOrWidgetProperty<bool, &floatWindowTagProperty>(windowOrWidgetToTag, showAsFloatWindow);
}

void QOhosPlatformWindow::setWindowPrivacyMode(QObject *window, bool privacyModeEnabled)
{
    setWindowOrWidgetProperty<bool, &windowPrivacyModeSettingProperty>(window, privacyModeEnabled);
}

void QOhosPlatformWindow::setWindowCornerRadius(QObject *windowOrWidget, double radius)
{
    setWindowOrWidgetProperty<double, &windowCornerRadiusProperty>(windowOrWidget, radius);
}

QWindow *QOhosPlatformWindow::validSubWindowOfTagValueOrNull() const
{
    auto *tagValue = QOhosPlatformWindow::getWindowOrWidgetAsSubWindowOfTagValue(window());
    const auto &allWindows = QGuiApplicationPrivate::window_list;
    bool tagValueValid = tagValue != nullptr && allWindows.contains(tagValue);
    qCDebug(
        QtForOhos, "Window %s(%s) - subWindowOf tag %s value: %p",
        qPrintable(internalWindowId().toString()), qPrintable(window()->objectName()),
        tagValueValid ? "valid" : "invalid",
        tagValue);

    return tagValueValid ? tagValue : nullptr;
}

bool QOhosPlatformWindow::mainWindowTagValueOrFalse() const
{
    return m_propertiesStore.tryGetProperty<bool, &mainWindowTagProperty>().valueOr(false);
}

bool QOhosPlatformWindow::shouldShowWindowWithoutActivating() const
{
    const auto showWithoutActivating = window()->property("_q_showWithoutActivating");
    return showWithoutActivating.isValid() && showWithoutActivating.toBool() && window()->modality() == Qt::NonModal;
}

QWindow *QOhosPlatformWindow::getWindowOrWidgetAsSubWindowOfTagValue(QObject *windowOrWidget)
{
    return tryGetWindowOrWidgetProperty<QWindow *, &subWindowOfTagProperty>(windowOrWidget).valueOr(nullptr);
}

void QOhosPlatformWindow::setWindowOrWidgetNativeNodeRenderFitPolicyHint(
    QObject *windowOrWidget, QOhosPlatformWindow::NativeNodeRenderFitPolicy renderFitPolicy)
{
    setWindowOrWidgetProperty<NativeNodeRenderFitPolicy, &nativeNodeRenderFitPolicyHintProperty>(windowOrWidget, renderFitPolicy);
}

Qt::WindowFlags QOhosPlatformWindow::platformWindowFlagsForQWindow(QWindow *window)
{
    auto *platformWindow = QOhosPlatformWindow::fromQWindow(window);
    return platformWindow->windowFlags();
}

void QOhosPlatformWindow::setSurfaceBackgroundColor(QObject *windowOrWidget, const QColor &color)
{
    setWindowOrWidgetProperty<QColor, &surfaceBackgroundColorProperty>(windowOrWidget, color);
}

void QOhosPlatformWindow::setWindowKeepScreenOn(QObject *windowOrWidget, bool keepScreenOn)
{
    setWindowOrWidgetProperty<bool, &windowKeepScreenOnProperty>(windowOrWidget, keepScreenOn);
}

void QOhosPlatformWindow::setWindowDragResizable(QObject *windowOrWidget, bool dragResizable)
{
    setWindowOrWidgetProperty<bool, &windowDragResizableProperty>(windowOrWidget, dragResizable);
}

void QOhosPlatformWindow::setBrightness(QObject *windowOrWidget, int brightness)
{
    setWindowOrWidgetProperty<int, &windowBrightnessProperty>(windowOrWidget, brightness);
}

void QOhosPlatformWindow::setContrast(QObject *windowOrWidget, int contrast)
{
    setWindowOrWidgetProperty<int, &windowContrastProperty>(windowOrWidget, contrast);
}

void QOhosPlatformWindow::setSaturation(QObject *windowOrWidget, int saturation)
{
    setWindowOrWidgetProperty<int, &windowSaturationProperty>(windowOrWidget, saturation);
}

void QOhosPlatformWindow::closeAllActivePopups()
{
    auto windows = qGuiApp->allWindows();
    for (auto *window : windows) {
        if (window->type() == Qt::Popup && window->isVisible())
            QWindowSystemInterface::handleCloseEvent(window);
    }
}

QOhosView *QOhosPlatformWindow::ownedViewOrNull() const
{
    return nullptr;
}

void QOhosPlatformWindow::initialize()
{
    auto *qWindow = window();

    setWindowFlags(qWindow->flags());

    auto initialWindowGeom = windowGeometry();
    auto initialGeom = initialGeometry(window(), initialWindowGeom, defaultWindowWidth, defaultWindowHeight);
    setGeometry(initialGeom);

    m_parent = parent();
}

Qt::WindowStates QOhosPlatformWindow::windowStates() const
{
    return m_windowState;
}

void QOhosPlatformWindow::setWindowStateFromOhos(Qt::WindowStates state)
{
    if (m_windowState != state) {
        m_windowState = state;
        QWindowSystemInterface::handleWindowStateChanged(window(), m_windowState);
    }
}

void QOhosPlatformWindow::requestActivateWindow()
{
    if (windowFlags().testFlag(Qt::WindowDoesNotAcceptFocus))
        return;

    auto *view = ownedViewOrNull();
    if (view != nullptr)
        view->requestActivate();
}

void QOhosPlatformWindow::setWindowMarginsFromOhos(const QMargins &margins)
{
    bool marginsChanged = !m_optFrameMargins || *m_optFrameMargins != margins;
    if (!marginsChanged)
        return;

    if (m_optFrameMargins) {
        *m_optFrameMargins = margins;
    } else {
        m_optFrameMargins = std::make_unique<QMargins>(margins);
    }

    qCDebug(QtForOhos) << "Window margins changed: " << *m_optFrameMargins;
}

void QOhosPlatformWindow::setExposedFromOhos(bool exposed)
{
    m_exposed = exposed;
    sendExposeUpdate();
}

void QOhosPlatformWindow::handleDpiChange()
{
    auto *qWindow = window();
    auto *view = ownedViewOrNull();
    bool needsGeometryUpdate =
        view != nullptr
            && view->viewType() == QOhosView::ViewType::EmbeddedWindow
            && window()->isVisible();
    if (needsGeometryUpdate) {
        auto scaledGeometry = window()->geometry();
        qWindow->setGeometry(scaledGeometry);
        view->forceGeometryUpdate();
    }

    qWindow->requestUpdate();
    setWindowGeometryFromOhos(windowGeometry());
}

std::shared_ptr<void> QOhosPlatformWindow::setSurfaceConsumer(
    QWindow *targetWindow, QObject *surfaceConsumerContext,
    std::function<void(QOhosOptional<void *>)> surfaceConsumer)
{
    qCDebug(
        QtForOhos,
        "%s: %s",
        Q_FUNC_INFO,
        surfaceConsumerContext->metaObject()->className());

    auto *view = getWindowsViewOrNull(targetWindow);
    if (view == nullptr)
        return nullptr;

    if (surfaceConsumerContext->thread() != view->thread() || view->thread() != QThread::currentThread()) {
        qOhosReportFatalErrorAndAbort(
            "%s: inter-thread surface consumer connection is not supported", Q_FUNC_INFO);
    }

    auto sharedSurfaceConsumer = QtOhos::moveToSharedPtr(std::move(surfaceConsumer));

    auto *surface = view->surfaceOrNull();
    if (surface != nullptr) {
        QMetaObject::invokeMethod(
            surfaceConsumerContext,
            [weakSurfaceConsumer = std::weak_ptr<decltype(surfaceConsumer)>(sharedSurfaceConsumer),
            targetWindow = QPointer<QWindow>(targetWindow)]() {
                auto sharedSurfaceConsumer = weakSurfaceConsumer.lock();
                if (!sharedSurfaceConsumer || targetWindow == nullptr)
                    return;

                auto *view = getWindowsViewOrNull(targetWindow);
                if (view == nullptr)
                    return;

                auto *surface = view->surfaceOrNull();
                (*sharedSurfaceConsumer)(
                    surface != nullptr
                        ? QOhosOptional<void *>(surface->nativeWindow())
                        : makeEmptyQOhosOptional());
            },
            Qt::QueuedConnection);
    }

    auto surfaceStatusChangedConnectionHandle = QObject::connect(
        view, &QOhosView::surfaceStatusChanged, surfaceConsumerContext,
        [view = QPointer<QOhosView>(view), sharedSurfaceConsumer](const QOhosOptional<QSize> &) {
            if (view == nullptr)
                return;
            auto *surface = view->surfaceOrNull();
            (*sharedSurfaceConsumer)(
                surface != nullptr
                    ? QOhosOptional<void *>(surface->nativeWindow())
                    : makeEmptyQOhosOptional());
        });

    if (!surfaceStatusChangedConnectionHandle) {
        qCCritical(
            QtForOhos,
            "%s: Connection between ohos view and surface consumer context failed",
            Q_FUNC_INFO);
        return nullptr;
    }

    auto viewDestroyedConnectionHandle = QObject::connect(
        view, &QObject::destroyed, surfaceConsumerContext,
        [sharedSurfaceConsumer]() {
            (*sharedSurfaceConsumer)(makeEmptyQOhosOptional());
        },
        Qt::QueuedConnection);

    if (!viewDestroyedConnectionHandle) {
        qCCritical(
            QtForOhos,
            "%s: Connecting view destroyed signal to surface consumer context failed",
            Q_FUNC_INFO);
        return nullptr;
    }

    return QtOhos::makeDestroyNotifier(
        [surfaceStatusChangedConnectionHandle = std::move(surfaceStatusChangedConnectionHandle),
         viewDestroyedConnectionHandle = std::move(viewDestroyedConnectionHandle)] () mutable {
            QObject::disconnect(surfaceStatusChangedConnectionHandle);
            QObject::disconnect(viewDestroyedConnectionHandle);
        });
}

bool QOhosPlatformWindow::floatWindowTagValueOrFalse() const
{
    return m_propertiesStore.tryGetProperty<bool, &floatWindowTagProperty>().valueOr(false);
}

QPixmap QOhosPlatformWindow::makeSnapshot() const
{
    return ownedViewOrNull() != nullptr ? ownedViewOrNull()->makeSnapshot() : QPixmap();
}

void QOhosPlatformWindow::setDisplayIdFromOhos(QOhosOptional<QOhosDisplayInfo::JsDisplayId> displayId)
{
    if (m_displayId != displayId) {
        m_displayId = displayId;

        qCWarning(QtForOhos)
            << "Screen changed - window:" << window()
            << "displayId:" << (displayId.hasValue()
                ? QString::number(displayId.value().value())
                : QString::fromUtf8("<NO DISPLAY>"));

        QOhosPlatformScreen *screen = m_displayId.hasValue()
            ? QOhosPlatformIntegration::instance()
                ->screenManager()->platformScreenForDisplayIdOrNull(m_displayId.value())
            : nullptr;

        QWindowSystemInterface::handleWindowScreenChanged(
            window(),
            screen != nullptr
                ? screen->screen()
                : nullptr);
    }
}

void QOhosPlatformWindow::setWindowGeometryFromOhos(const QRect &nativeWindowDrawGeometry)
{
    qCDebug(QtForOhos) << "window:" << window() << "geometry change to:" << nativeWindowDrawGeometry;
    QWindowSystemInterface::handleGeometryChange(window(), nativeWindowDrawGeometry);

    if (isExposed())
        sendExposeUpdate();
}

void QOhosPlatformWindow::notifyWindowDestroyedFromOhos()
{
    m_displayId.reset();
    QWindowSystemInterface::handleCloseEvent(window());
}

void QOhosPlatformWindow::notifyInputSystemsWindowActiveStatusChanged(bool active)
{
    if (active) {
        auto *ohosInputContext = qobject_cast<QOhosInputContext *>(QOhosPlatformIntegration::instance()->inputContext());
        if (ohosInputContext != nullptr) {
            ohosInputContext->setLastInputTypeToTriggerSoftKeyboard(QOhosInputContext::RequestKeyboardReason::NONE);
        }
    } else {
        if (QOhosPlatformIntegration::instance()->inputContext()->isInputPanelVisible())
            QOhosPlatformIntegration::instance()->inputContext()->hideInputPanel();
    }
}

void QOhosPlatformWindow::onWindowFlagsChanged(Qt::WindowFlags, Qt::WindowFlags)
{
}

void QOhosPlatformWindow::onWindowStateChanged(Qt::WindowStates, Qt::WindowStates)
{
}

QOhosPropertiesProvider QOhosPlatformWindow::propertiesProvider()
{
    return QOhosPropertiesProvider(m_propertiesStore);
}

bool QOhosPlatformWindow::setMouseGrabEnabled(bool grab)
{
    qCDebug(QtForOhos) << Q_FUNC_INFO << "window:" << window() << "grab:" << grab;

    auto *inputHandler = QOhosPlatformIntegration::instance()->inputMethodEventHandler();
    if (grab)
        inputHandler->grabMouse(window());
    else
        inputHandler->stopAnyMouseGrab();

    return true;
}

bool QOhosPlatformWindow::setKeyboardGrabEnabled(bool grab)
{
    qCDebug(QtForOhos) << Q_FUNC_INFO << "window:" << window() << "grab:" << grab;

    auto *inputHandler = QOhosPlatformIntegration::instance()->inputMethodEventHandler();
    if (grab)
        inputHandler->grabKeyboard(window());
    else
        inputHandler->stopAnyKeyboardGrab();

    return true;
}

bool QOhosPlatformWindow::checkWindowAcceptsFocus() const
{
    auto platformWindowFlags = QOhosPlatformWindow::platformWindowFlagsForQWindow(window());
    return !platformWindowFlags.testFlag(Qt::WindowDoesNotAcceptFocus);
}

bool QOhosPlatformWindow::checkWindowAcceptsInput() const
{
    auto platformWindowFlags = QOhosPlatformWindow::platformWindowFlagsForQWindow(window());
    return !platformWindowFlags.testFlag(Qt::WindowTransparentForInput);
}

bool QOhosPlatformWindow::windowEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ApplicationPaletteChange:
        if (auto *ohosView = ownedViewOrNull())
            ohosView->handlePaletteChange();
        break;
    case QEvent::DynamicPropertyChange: {
        auto propertyName = static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName();
        m_propertiesStore.notifyPropertyWrite(propertyName);
        break;
    }
    case QEvent::Hide:
        if (isWindowBeingClosedOrDestroyed(window())) {
            auto *ohosView = ownedViewOrNull();
            if (ohosView && ohosView->viewType() == QOhosView::ViewType::MainWindow)
                ohosView->setNativeNodeVisibility(false);
        }
        break;
    default: break;
    }

    return QPlatformWindow::windowEvent(event);
}

bool QOhosPlatformWindow::isWindowBeingClosedOrDestroyed(QWindow *window)
{
    QWindowPrivate *windowPriv = qt_window_private(window);
    return windowPriv && (windowPriv->inClose || windowPriv->visibilityOnDestroy);
}

void QOhosPlatformWindow::sendExposeUpdate()
{
    auto exposedSize = m_exposed
        ? geometry().size()
        : QSize();

    QWindowSystemInterface::handleExposeEvent(window(), QRegion(QRect(QPoint(), exposedSize)));
}

QT_END_NAMESPACE
