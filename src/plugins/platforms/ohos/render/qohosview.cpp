// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qohosview.h>

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qscopeguard.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/qbitmap.h>
#include <QtGui/qpalette.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <arkui/native_type.h>
#include <functional>
#include <memory>
#include <qarkui/window.h>
#include <qohosdeviceinfo_p.h>
#include <qohosinputmethodeventhandler.h>
#include <qohosjsmain.h>
#include <qohosplatformbackingstore.h>
#include <qohosplatformintegration.h>
#include <qohosplatformscreen.h>
#include <qohosplatformwindow.h>
#include <qohosutils.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qwindowsysteminterface.h>
#include <render/qnativenode.h>
#include <render/qohoswindowproxy.h>
#include <render/qwindowproxyregistry.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "qohoscloseeventcontext_p.h"

QT_BEGIN_NAMESPACE

namespace
{

struct ViewTypeInfo
{
    QOhosView::ViewType viewType;
    QWindow *optLogicalParent;
};

enum class WindowGeometryPersistenceState
{
    Disabled,
    Enabled,
};

std::optional<QOhosWindowProxy::ModalityType> mapQtWindowModalityToOhosOrDefault(
    Qt::WindowModality windowModality)
{
    using ModalityType = QOhosWindowProxy::ModalityType;

    switch (windowModality) {
    case Qt::WindowModality::NonModal:
        return {};
    case Qt::WindowModality::WindowModal:
        return ModalityType::WINDOW_MODALITY;
    case Qt::WindowModality::ApplicationModal:
        return ModalityType::APPLICATION_MODALITY;
    }

    qOhosPrintfWarning(
        "%s: got illegal Qt::WindowModality value (%d), using the default instead",
        Q_FUNC_INFO, static_cast<int>(windowModality));

    return {};
}

template<typename ...SignalParams>
std::function<void(SignalParams...)> makeViewConditionalSignalEmitter(
    QPointer<QOhosView> viewPtr, std::function<bool(QOhosView &)> predicate, void (QOhosView::*signalFuncPtr)(SignalParams ...))
{
    return [signalFuncPtr, viewPtr, predicate = std::move(predicate)](SignalParams ...args) {
        if (viewPtr && predicate(*viewPtr))
            Q_EMIT (*viewPtr.*signalFuncPtr)(args...);
    };
}

class AvoidAreaCache
{
public:
    using AvoidArea = QOhosWindowProxy::AvoidArea;
    using AvoidAreaType = QOhosWindowProxy::AvoidAreaType;

    explicit AvoidAreaCache(std::function<AvoidArea(AvoidAreaType)> avoidAreasProvider);

    void put(
        AvoidAreaType avoidAreaType,
        const AvoidArea &avoidArea);

    AvoidArea getStoredOrRetrieveFromWindowProxy(AvoidAreaType avoidAreaType);

private:
    std::function<AvoidArea(AvoidAreaType)> m_avoidAreasProvider;
    QMap<AvoidAreaType, AvoidArea> m_store;
};

AvoidAreaCache::AvoidAreaCache(std::function<AvoidArea(AvoidAreaType)> avoidAreasProvider)
    : m_avoidAreasProvider(std::move(avoidAreasProvider))
    , m_store()
{
}

void AvoidAreaCache::put(
    QOhosWindowProxy::AvoidAreaType avoidAreaType, const QOhosWindowProxy::AvoidArea &avoidArea)
{
    m_store[avoidAreaType] = avoidArea;
}

QOhosWindowProxy::AvoidArea AvoidAreaCache::getStoredOrRetrieveFromWindowProxy(
    QOhosWindowProxy::AvoidAreaType avoidAreaType)
{
    if (m_store.contains(avoidAreaType))
        return m_store.value(avoidAreaType);

    auto result = m_avoidAreasProvider(avoidAreaType);
    m_store[avoidAreaType] = result;
    return result;
}

void tryUpdateMaximumMarginsFromAvoidArea(QMargins &marginsToUpdate, const QOhosWindowProxy::AvoidArea &avoidArea)
{
    if (!avoidArea.visible)
        return;

    marginsToUpdate.setTop(qMax(marginsToUpdate.top(), avoidArea.topRect.height()));
    marginsToUpdate.setLeft(qMax(marginsToUpdate.left(), avoidArea.leftRect.width()));
    marginsToUpdate.setRight(qMax(marginsToUpdate.right(), avoidArea.rightRect.width()));
    marginsToUpdate.setBottom(qMax(marginsToUpdate.bottom(), avoidArea.bottomRect.height()));
}

QSize evaluateGeometrySizeBasedOnFrameGeometry(
    const QPoint &geometryOrigin, const QRect &frameGeometry)
{
    int topFrameMargin = qAbs(frameGeometry.top() - geometryOrigin.y());
    int sideAndBottomFrameMargin = qAbs(frameGeometry.left() - geometryOrigin.x());

    return {
        frameGeometry.width() - 2 * sideAndBottomFrameMargin,
        frameGeometry.height() - topFrameMargin - sideAndBottomFrameMargin
    };
}

QSize getQSizeGrownBy(const QSize &size, const QMargins &margins)
{
    // HACK
    // when the minimum window size is set to 0, the system seems to treat it as “not set” and
    // applies its own default values instead (width: 320vp, height: 72vp).
    // Therefore, when the intention is to set the minimum size to 0,
    // we set it to 1 instead to avoid the system default being applied.
    return {
        qMax(1, size.width() + margins.left() + margins.right()),
        qMax(1, size.height() + margins.top() + margins.bottom())};
}

QWindow *getFirstTopLevelWindowOrNull()
{
    auto visibleWindows = QWindowProxyRegistry::instance().queryWindowsWithVisibleSystemWindow();
    return !visibleWindows.empty()
        ? visibleWindows.front()
        : nullptr;
}

QWindow *getFirstTopLevelWindowWithSystemFocusOrNull()
{
    auto focusedWindows = QWindowProxyRegistry::instance().queryWindowsWithSystemWindowAndFocus();
    return !focusedWindows.empty()
        ? focusedWindows.front()
        : nullptr;
}

QWindow *syntheticParentForQWindowOrNull(QWindow *qWindow)
{
    auto *focusWindow = getFirstTopLevelWindowWithSystemFocusOrNull();
    auto *firstTopLevelWindow = getFirstTopLevelWindowOrNull();

    auto isValidSyntheticParent = [&](QWindow *w) {
        return w != nullptr && w != qWindow && w->isVisible();
    };

    return isValidSyntheticParent(focusWindow)
        ? focusWindow
        : isValidSyntheticParent(firstTopLevelWindow)
            ? firstTopLevelWindow
            : nullptr;
}

ViewTypeInfo determineViewTypeAndLogicalParent(const QOhosPlatformWindow *platformWindow)
{
    using ViewType = QOhosView::ViewType;

    auto *qWindow = platformWindow->window();
    auto *parent = qWindow->parent();
    auto *transientParent = qWindow->transientParent();

    auto windowType = qWindow->type();

    QWindow *subWindowTagValue = platformWindow->validSubWindowOfTagValueOrNull();

    static QSet<Qt::WindowType> fallbackToSubWindowWindowTypes{
        Qt::Popup,
        Qt::ToolTip,
        Qt::Dialog,
        Qt::Tool,
    };

    bool taggedAsMainWindow = platformWindow->mainWindowTagValueOrFalse();
    if (taggedAsMainWindow) {
        return ViewTypeInfo {
            .viewType = ViewType::MainWindow,
            .optLogicalParent = nullptr,
        };
    }

    bool taggedAsFloatWindow = platformWindow->floatWindowTagValueOrFalse();
    if (taggedAsFloatWindow) {
        return ViewTypeInfo {
            .viewType = ViewType::FloatWindow,
            .optLogicalParent = nullptr,
        };
    }

    bool overrideAsSubWindow = subWindowTagValue != nullptr;
    if (overrideAsSubWindow) {
        return ViewTypeInfo {
            .viewType = ViewType::SubWindow,
            .optLogicalParent = subWindowTagValue,
        };
    }

    if (parent != nullptr) {
        return ViewTypeInfo {
            .viewType = ViewType::EmbeddedWindow,
            .optLogicalParent = parent,
        };
    }

    if (transientParent != nullptr) {
        return ViewTypeInfo {
            .viewType = ViewType::SubWindow,
            .optLogicalParent = transientParent,
        };
    }

    auto *syntheticParent = syntheticParentForQWindowOrNull(platformWindow->window());

    bool automaticOverrideActive =
        fallbackToSubWindowWindowTypes.contains(windowType)
        && syntheticParent != nullptr;
    if (automaticOverrideActive) {
        return ViewTypeInfo {
            .viewType = ViewType::SubWindow,
            .optLogicalParent = syntheticParent,
        };
    }

    return ViewTypeInfo {
        .viewType = ViewType::MainWindow,
        .optLogicalParent = nullptr,
    };
}

QBitmap getCursorBitmap(const QCursor &cursor)
{
    return cursor.bitmap();
}

QBitmap getCursorMask(const QCursor &cursor)
{
    return cursor.mask();
}

QImage createImageFromBitmapAndMask(const QBitmap &bitmap, const QBitmap &mask)
{
    const auto maskTransparentColor = QColor(Qt::color0).rgba();
    const auto transparentColor = QColor(Qt::transparent).rgba();

    QImage image = bitmap.toImage().convertToFormat(QImage::Format_RGBA8888);
    QImage maskImage = mask.toImage().convertToFormat(QImage::Format_RGB32);

    for (int row = 0; row < image.height(); ++row) {
        auto *imageData = reinterpret_cast<QRgb *>(image.scanLine(row));
        auto *maskData = reinterpret_cast<QRgb *>(maskImage.scanLine(row));
        for (int col = 0; col < image.width(); ++col) {
            if (maskData[col] == maskTransparentColor)
                imageData[col] = transparentColor;
        }
    }

    return image;
}

QCursor makeTransparentBitmapCursor(QSize cursorSize)
{
    QBitmap cursorPixels(cursorSize);
    cursorPixels.fill(Qt::transparent);
    QBitmap cursorMask(cursorSize);
    cursorMask.fill(Qt::color0);
    return QCursor(cursorPixels, cursorMask);
}

ViewGeometryPersistencePolicy determineViewGeometryPersistencePolicy()
{
    using WindowGeometryPersistencePolicy = QOhosPlatformIntegration::WindowGeometryPersistencePolicy;

    auto viewGeometryPersistencePolicy = ViewGeometryPersistencePolicy::Ignore;

    auto policy = QOhosPlatformIntegration::getMainWindowGeometryPersistencePolicy();
    switch (policy) {
    case WindowGeometryPersistencePolicy::Disabled:
        viewGeometryPersistencePolicy = ViewGeometryPersistencePolicy::Disabled;
        break;
    case WindowGeometryPersistencePolicy::Enabled:
        viewGeometryPersistencePolicy = ViewGeometryPersistencePolicy::Enabled;
        break;
    case WindowGeometryPersistencePolicy::FollowSystemSetting:
        viewGeometryPersistencePolicy = ViewGeometryPersistencePolicy::FollowSystemSetting;
        break;
    }

    return viewGeometryPersistencePolicy;
}

WindowGeometryPersistenceState syncWindowGeometryPersistenceState(QOhosWindowProxy *windowProxy)
{
    using WindowGeometryPersistencePolicy = QOhosPlatformIntegration::WindowGeometryPersistencePolicy;

    bool geometryPersistenceEnabled = false;
    auto policy = QOhosPlatformIntegration::getMainWindowGeometryPersistencePolicy();
    switch (policy) {
    case WindowGeometryPersistencePolicy::Disabled:
        windowProxy->setWindowRectAutoSave(false);
        break;
    case WindowGeometryPersistencePolicy::Enabled:
        windowProxy->setWindowRectAutoSave(true);
        geometryPersistenceEnabled = windowProxy->isWindowRectAutoSave();
        break;
    case WindowGeometryPersistencePolicy::FollowSystemSetting:
        geometryPersistenceEnabled = windowProxy->isWindowRectAutoSave();
        break;
    }

    return geometryPersistenceEnabled
        ? WindowGeometryPersistenceState::Enabled
        : WindowGeometryPersistenceState::Disabled;
}

std::optional<QOhosDisplayInfo::JsDisplayId> tryGetSubWindowJsDisplayId(
    QWindow *logicalParent, QOhosWindowProxy &windowProxy)
{
    auto *screen = logicalParent->screen();
    auto *ohosPlatformScreen = screen != nullptr
        ? static_cast<QOhosPlatformScreen *>(screen->handle())
        : nullptr;

    return ohosPlatformScreen != nullptr
        ? std::optional(ohosPlatformScreen->displayInfo().id)
        : windowProxy.tryGetMainWindowJsDisplayId();
}

std::optional<QColor> tryGetBackgroundColorFromWindow(QWindow *window)
{
    if (window != nullptr) {
        QWindowPrivate *windowPrivate = qt_window_private(window);
        QPalette palette = windowPrivate->windowPalette();
        QColor backgroundColor = palette.color(QPalette::Window);
        return backgroundColor;
    }
    return {};
}

}

void QOhosView::handleSurfaceContentsUpdated()
{
    auto optPostSurfaceDrawTask = std::exchange(m_optPostSurfaceDrawTask, {});
    if (optPostSurfaceDrawTask)
        optPostSurfaceDrawTask();
}

std::shared_ptr<QOhosWindowProxy> QOhosView::tryCreateWindowProxyIfNeeded(ViewType viewType, QWindow *optLogicalParent)
{
    QWindow *qWindow = m_ownerWindow;
    QOhosPlatformWindow *window = QOhosPlatformWindow::fromQWindow(qWindow);

    std::shared_ptr<QOhosWindowProxy> result;

    switch (viewType) {
    case ViewType::MainWindow:
    {
        // NOTE: treat windows automatically-created by the platform in a special way
        bool useAutoStartedMainWindow = QtOhos::acquireAndCleanPendingAutoStartedInstanceWindowFlag();
        if (useAutoStartedMainWindow) {
            QOhosWindowProxy::ExistingMainWindowCreateInfo createInfo;
            createInfo.qWindowRef = QtOhos::QObjectThreadSafeRef(qWindow);
            createInfo.qAbilityInstanceId =  QtOhos::evalInJsThread([](QtOhos::JsState &jsState) {
                return jsState.defaultQAbilityPeer()->instanceId();
            },
            Q_FUNC_INFO);
            result = QOhosWindowProxy::createForExistingMainWindow(createInfo);
            m_geometryPersistencePolicy = determineViewGeometryPersistencePolicy();
        } else {
            QOhosWindowProxy::MainWindowCreateInfo createInfo;
            createInfo.qWindowRef = QtOhos::QObjectThreadSafeRef(qWindow);
            createInfo.windowId = window->internalWindowId();
            createInfo.windowTitle = qWindow->title().toStdString();
            createInfo.frameGeometry = window->lastRequestedWindowFrameGeometry();
            createInfo.fullscreen = qWindow->windowState() == Qt::WindowFullScreen;
            result = QOhosWindowProxy::createMainWindow(createInfo);
        }

        break;
    }
    case ViewType::SubWindow:
    {
        auto *targetWindowToBeSubWindowOf = optLogicalParent;

        if (Q_UNLIKELY(targetWindowToBeSubWindowOf == nullptr))
            qOhosReportFatalErrorAndAbort("Failed to determine valid parent of a SubWindow");

        if (!targetWindowToBeSubWindowOf->isVisible()) {
            auto *targetParentPlatformWindow = QOhosPlatformWindow::fromQWindowOrNull(targetWindowToBeSubWindowOf);
            if (targetParentPlatformWindow != nullptr)
                targetParentPlatformWindow->setVisible(true);
            else
                targetWindowToBeSubWindowOf->show();
        }

        auto *targetWindowToBeSubWindowOfView =
            QOhosPlatformWindow::fromQWindow(targetWindowToBeSubWindowOf)->ownedViewOrNull();

        const QOhosView *firstViewWithWindow = nullptr;
        switch (targetWindowToBeSubWindowOfView->viewType()) {
        case ViewType::MainWindow:
        case ViewType::SubWindow:
        case ViewType::FloatWindow:
            firstViewWithWindow = targetWindowToBeSubWindowOfView;
            break;
        case ViewType::EmbeddedWindow:
            firstViewWithWindow = ancestorViewWithWindowOrNull();
            break;
        }

        if (firstViewWithWindow == nullptr)
            qOhosReportFatalErrorAndAbort("Failed to determine valid parent for this window.");

        auto parentWindowProxy = firstViewWithWindow->m_ohosWindowProxy;
        if (Q_UNLIKELY(!parentWindowProxy)) {
            qOhosReportFatalErrorAndAbort(
                "parentWindowProxy is null but should not be. This is most likely a programming error");
        }

        QOhosWindowProxy::SubWindowCreateInfo createInfo;
        createInfo.window = QtOhos::QObjectThreadSafeRef(qWindow);
        createInfo.windowTitle = qWindow->title().toStdString();
        createInfo.windowId = window->internalWindowId();
        createInfo.qAbilityInstanceId = parentWindowProxy->qAbilityInstanceId();

        createInfo.decorEnabled = window->decorationPreset() != QOhosPlatformWindow::DecorationPreset::Frameless;
        createInfo.disableWindowFocusableBeforeLoadContentHack = window->windowFlags().testFlag(Qt::WindowDoesNotAcceptFocus);
        createInfo.modal = qWindow->modality() != Qt::NonModal;
        createInfo.windowRect = window->lastRequestedWindowFrameGeometry();
        result = parentWindowProxy->createSubWindow(createInfo);

        break;
    }
    case ViewType::EmbeddedWindow:
    {
        // NOTE - NativeNode is now always created during view instantiation
        break;
    }
    case ViewType::FloatWindow:
    {
        QOhosWindowProxy::FloatWindowCreateInfo createInfo = {
            .qWindowRef = QtOhos::QObjectThreadSafeRef(qWindow),
            .internalWindowId = window->internalWindowId(),
        };
        result = QOhosWindowProxy::createFloatWindow(createInfo);
        break;
    }
    default:
        qOhosReportFatalErrorAndAbort("Unsupported view type: %d", viewType);

    }

    if (result != nullptr) {
        auto viewPtr = QPointer<QOhosView>(this);
        std::weak_ptr<QOhosWindowProxy> weakWindowProxy = result;
        auto shouldEmitSignalPredicate = [weakWindowProxy](QOhosView &view) {
            auto windowProxy = weakWindowProxy.lock();
            return windowProxy != nullptr && windowProxy.get() == view.m_ohosWindowProxy.get();
        };

        result->setWindowCallbackReceiver(
            std::make_unique<QOhosWindowProxy::WindowCallbacks>(
                QOhosWindowProxy::WindowCallbacks {
                    .onWindowEvent = makeViewConditionalSignalEmitter(viewPtr, shouldEmitSignalPredicate, &QOhosView::windowEvent),
                    .onWindowStatusChange = makeViewConditionalSignalEmitter(viewPtr, shouldEmitSignalPredicate, &QOhosView::windowStatusChange),
                    .onWindowVisibilityChange = makeViewConditionalSignalEmitter(viewPtr, shouldEmitSignalPredicate, &QOhosView::windowVisibilityChange),
                    .onTouchOutside = makeViewConditionalSignalEmitter(viewPtr, shouldEmitSignalPredicate, &QOhosView::windowTouchOutside),
                    .onAvoidAreaChange = makeViewConditionalSignalEmitter(viewPtr, shouldEmitSignalPredicate, &QOhosView::avoidAreaChanged),
                    .onWindowRectChange = makeViewConditionalSignalEmitter(viewPtr, shouldEmitSignalPredicate, &QOhosView::windowRectChanged),
                    .onWindowRectChangeInGlobalDisplay = makeViewConditionalSignalEmitter(
                        viewPtr, shouldEmitSignalPredicate, &QOhosView::windowRectChangedInGlobalDisplay),
                    .onWindowDisplayIdChange = makeViewConditionalSignalEmitter(viewPtr, shouldEmitSignalPredicate, &QOhosView::windowDisplayIdChanged),
                }));

        result->setNonClientAreaMouseWindowCallbackReceiver(
            this,
            [viewPtr, shouldEmitSignalPredicate](std::vector<QOhosWindowProxy::NonClientAreaMouseEvent> &&batch) {
                if (!viewPtr.isNull() && shouldEmitSignalPredicate(*viewPtr)) {
                    auto *inputEventHandler = QOhosPlatformIntegration::instance()->inputMethodEventHandler();
                    inputEventHandler->onNonClientAreaMouseEvents(viewPtr->m_ownerWindow, std::move(batch));
                }
            });

        result->setNonClientAreaTouchWindowCallbackReceiver(
            this,
            [viewPtr, shouldEmitSignalPredicate](std::vector<QOhosWindowProxy::NonClientAreaTouchEvent> &&batch) {
                if (!viewPtr.isNull() && shouldEmitSignalPredicate(*viewPtr)) {
                    auto *inputEventHandler = QOhosPlatformIntegration::instance()->inputMethodEventHandler();
                    inputEventHandler->onNonClientAreaTouchEvents(viewPtr->m_ownerWindow, std::move(batch));
                }
            });

        if (result->qtIsMainWindow() && result->isFocused().value_or(false)) {
            // HACK
            // WINDOW_ACTIVE event should be handled by QOhosFloatingWindow after OHOS system event received.
            // Until we cannot register this event properly, keep sending it manually here.
            sendAsyncSyntheticWindowActiveEvent();
        }
    }

    return result;
}

bool QOhosView::isWindowTransparencyRequested() const
{
    return m_ownerWindow->requestedFormat().hasAlpha();
}

QOhosView::QOhosView(QWindow *ownerWindow, QSharedPointer<QNativeNode> nativeNode, QOhosPropertiesProvider windowPropertyProvider)
    : m_windowPropertiesProvider(windowPropertyProvider)
    , m_ownerWindow(ownerWindow)
    , m_ohosWindowProxy()
    , m_nativeNode(std::move(nativeNode))
    , m_updateData()
    , m_updatePending(false)
    , m_avoidAreasProvider()
    , m_requireHandheldDeviceSupport(isHandheldDeviceType())
    , m_ownerWindowId(QOhosPlatformWindow::fromQWindow(ownerWindow)->internalWindowId())
    , m_lastMainWindowHideMethod()
{
    auto avoidAreasProvider = std::make_shared<AvoidAreaCache>([this](AvoidAreaCache::AvoidAreaType avoidAreaType) {
        return m_ohosWindowProxy != nullptr
            ? m_ohosWindowProxy->getWindowAvoidArea(avoidAreaType)
            : AvoidAreaCache::AvoidArea{};
    });

    connect(
        this, &QOhosView::avoidAreaChanged,
        this,
        [avoidAreasProvider](QOhosWindowProxy::AvoidAreaType avoidAreaType, const QOhosWindowProxy::AvoidArea &avoidArea) {
            avoidAreasProvider->put(avoidAreaType, avoidArea);
        });

    m_avoidAreasProvider = [avoidAreasProvider](QOhosWindowProxy::AvoidAreaType type) {
        return avoidAreasProvider->getStoredOrRetrieveFromWindowProxy(type);
    };

    connect(
        m_nativeNode.get(), &QNativeNode::surfaceStatusChanged,
        this, [this](const std::optional<QSize> &optSurfaceSize) {
            Q_EMIT surfaceStatusChanged(optSurfaceSize);
        });

    connect(
        m_nativeNode.get(), &QNativeNode::externalContentClickDetected,
        this, &QOhosView::externalContentInteractionDetected);

    m_nativeNode->setNodeAreaChangeHandler(
        [this](auto nodeAreaChangeEvent) {
            Q_EMIT nodeAreaChanged(nodeAreaChangeEvent);
        });

    m_nativeNode->setNodeVisibilityChangeHandler(
        [this](bool visible) {
            if (viewType() == QOhosView::ViewType::EmbeddedWindow)
                Q_EMIT windowVisibilityChange(visible);
        });

    std::vector<std::shared_ptr<void>> writeCallbacks = {
        m_windowPropertiesProvider.addPropertyWriteCallback<double, &QOhosPlatformWindow::windowCornerRadiusProperty>(
            [this](double windowCornerRadius) {
                setWindowCornerRadius(windowCornerRadius);
            }),
        m_windowPropertiesProvider.addPropertyWriteCallback<bool, &QOhosPlatformWindow::windowPrivacyModeSettingProperty>(
            [this](bool windowPrivacyModeSetting) {
                setPrivacyMode(windowPrivacyModeSetting);
            }),
        m_windowPropertiesProvider.addPropertyWriteCallback<QColor, &QOhosPlatformWindow::surfaceBackgroundColorProperty>(
            [this](QColor surfaceBackgroundColor) {
                setBackgroundColor(surfaceBackgroundColor);
            }),
        m_windowPropertiesProvider.addPropertyWriteCallback<bool, &QOhosPlatformWindow::windowKeepScreenOnProperty>(
            [this](bool keepScreenOn) {
                setWindowKeepScreenOn(keepScreenOn);
            }),
        m_windowPropertiesProvider.addPropertyWriteCallback<bool, &QOhosPlatformWindow::windowFixedSizeStateProperty>(
            [this](bool fixedSizeStateEnabled) {
                setFixedSizeStateEnabled(fixedSizeStateEnabled);
            }),
        m_windowPropertiesProvider.addPropertyWriteCallback<int, &QOhosPlatformWindow::windowBrightnessProperty>(
            [this](int brightness) {
                setBrightness(brightness);
            }),
        m_windowPropertiesProvider.addPropertyWriteCallback<int, &QOhosPlatformWindow::windowContrastProperty>(
            [this](int contrast) {
                setContrast(contrast);
            }),
        m_windowPropertiesProvider.addPropertyWriteCallback<int, &QOhosPlatformWindow::windowSaturationProperty>(
            [this](int saturation) {
                setSaturation(saturation);
            }),
        m_windowPropertiesProvider.addPropertyWriteCallback<bool, &QOhosPlatformWindow::windowDragResizableProperty>(
            [this](bool dragResizable) {
                setWindowDragResizable(dragResizable);
            }),
    };

    m_windowPropertiesProviderCallbacksHandle = QtOhos::moveToSharedPtr(std::move(writeCallbacks));
}

QOhosView::~QOhosView()
{
    // This call is necessary in order to avoid receiving callbacks
    // from the underlying XComponent after we terminate the window.
    // Not having this resulted in exceptions thrown from the system
    // because QOhosNativeXComponent reffered to the window object that is null.
    m_nativeNode.reset();
}

void QOhosView::setPosition(const QPoint &position)
{
    setSystemUpdateProperty(&SystemUpdateData::position, {position, {}});
}

void QOhosView::setPositionOnScreenImmediate(
    const QPoint &position, QOhosDisplayInfo::JsDisplayId jsDisplayId)
{
    setSystemUpdateProperty(&SystemUpdateData::position, {position, jsDisplayId});
    flushSystemPropertyUpdatesImmediate();
}

void QOhosView::setSize(const QSize &size)
{
    setSystemUpdateProperty(&SystemUpdateData::size, size);
}

void QOhosView::setSizeLimits(const QSize &minSize, const QSize &maxSize)
{
    setSystemUpdateProperty(&SystemUpdateData::sizeLimits, std::make_pair(minSize, maxSize));
}

void QOhosView::setTransparentBackground(bool transparent)
{
    setSystemUpdateProperty(&SystemUpdateData::backgroundTransparent, transparent);
}

void QOhosView::setFocusable(bool focusable)
{
    setSystemUpdateProperty(&SystemUpdateData::focusable, focusable);
}

void QOhosView::setBackgroundColor(const QColor &color)
{
    setSystemUpdateProperty(&SystemUpdateData::backgroundColor, color);
}

void QOhosView::setBrightness(int brightness)
{
    setSystemUpdateProperty(&SystemUpdateData::brightness, brightness);
}

void QOhosView::setContrast(int contrast)
{
    setSystemUpdateProperty(&SystemUpdateData::contrast, contrast);
}

void QOhosView::setSaturation(int saturation)
{
    setSystemUpdateProperty(&SystemUpdateData::saturation, saturation);
}

void QOhosView::setWindowKeepScreenOn(bool keepScreenOn)
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->setWindowKeepScreenOn(keepScreenOn);
}

void QOhosView::setFixedSizeStateEnabled(bool enabled)
{
    if (viewType() != ViewType::MainWindow)
        return;

    if (queryQOhosRuntimeDeviceAndMode() == QOhosRuntimeDeviceTypeAndMode::HandheldDeviceFullScreen) {
        qOhosPrintfWarning(
            "%s: fixed size state is not supported while in HandheldDeviceFullScreen mode",
            Q_FUNC_INFO);
        return;
    }

    using SupportWindowMode = QOhosWindowProxy::SupportWindowMode;
    const auto supportedWindowModes = enabled
        ? std::set<SupportWindowMode>({SupportWindowMode::FLOATING})
        : std::set<SupportWindowMode>({SupportWindowMode::FULL_SCREEN, SupportWindowMode::FLOATING, SupportWindowMode::SPLIT});

    m_ohosWindowProxy->setSupportedWindowModes(supportedWindowModes);
}

void QOhosView::showWindow()
{
    auto *ohosPlatformWindow = static_cast<QOhosPlatformWindow *>(m_ownerWindow->handle());
    m_ohosWindowProxy->showWindow(
        QOhosWindowProxy::ShowWindowOptions{
            .focusOnShow = ohosPlatformWindow->shouldShowWindowWithoutActivating()
                ? std::optional(false)
                : std::nullopt,
        });
}

void QOhosView::sendAsyncSyntheticWindowActiveEvent()
{
     QMetaObject::invokeMethod(
        this,
        [this]() {
            QOhosWindowProxy::WindowEvent syntheticWindowActiveEvent = {
                .type = QOhosWindowProxy::WindowEventType::WINDOW_ACTIVE,
            };
            Q_EMIT windowEvent(syntheticWindowActiveEvent);
        },
        Qt::QueuedConnection);
}

void QOhosView::setCursor(const QCursor &cursor)
{
    setSystemUpdateProperty(&SystemUpdateData::cursor, cursor);
}

void QOhosView::setModality(Qt::WindowModality modality)
{
    setSystemUpdateProperty(&SystemUpdateData::modality, modality);
}

void QOhosView::setTitle(const QString &title)
{
    setSystemUpdateProperty(&SystemUpdateData::title, title);
}

void QOhosView::raise()
{
    switch (viewType()) {
    case ViewType::MainWindow:
        showWindow();
        break;
    case ViewType::SubWindow:
        m_ohosWindowProxy->raiseToAppTop();
        break;
    case ViewType::FloatWindow:
        break;
    case ViewType::EmbeddedWindow:
        m_nativeNode->raise();
        break;
    }
}

void QOhosView::lower()
{
    if (m_ohosWindowProxy != nullptr && m_ownerWindow->isVisible())
        showWindow();
    else
        m_nativeNode->lower();
}

void QOhosView::updateWindowSize(const QSize &size)
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->setSize(size);
    else
        m_nativeNode->setSize(size);
}

void QOhosView::updateWindowPosition(const std::pair<QPoint, std::optional<QOhosDisplayInfo::JsDisplayId>> &positionProp)
{
    QPoint position;
    std::optional<QOhosDisplayInfo::JsDisplayId> displayId;
    std::tie(position, displayId) = positionProp;

    if (m_ohosWindowProxy != nullptr) {
        m_ohosWindowProxy->moveWindowToGlobalOrGlobalDisplay(position, displayId);
    } else {
        m_nativeNode->setPosition(position);
    }
}

void QOhosView::updateWindowBackgroundTransparency(bool transparent)
{
    if (m_ohosWindowProxy != nullptr) {
        auto opaqueSystemBackgroundRgb = QGuiApplicationPrivate::platformTheme()
            ->palette(QPlatformTheme::SystemPalette)
            ->window()
            .color()
            .rgb();
        m_ohosWindowProxy->setWindowBackgroundColor(
            transparent
                ? QColor(Qt::transparent)
                : tryGetBackgroundColorFromWindow(m_ownerWindow).value_or(
                    QColor(opaqueSystemBackgroundRgb)));
    }
}

void QOhosView::updateWindowCursor(const QCursor &cursor)
{
    if (m_ohosWindowProxy == nullptr)
        return;

    if (cursor.shape() == Qt::BitmapCursor || cursor.shape() == Qt::BlankCursor) {
        auto bitmapCursor =
            cursor.shape() == Qt::BlankCursor
                ? makeTransparentBitmapCursor({1, 1})
                : cursor;

        QImage bitmapCursorImage = bitmapCursor.pixmap().isNull()
            ? createImageFromBitmapAndMask(getCursorBitmap(bitmapCursor), getCursorMask(bitmapCursor))
            : bitmapCursor.pixmap().toImage();

        m_ohosWindowProxy->setCustomCursor(bitmapCursorImage, bitmapCursor.hotSpot());
    } else {
        m_ohosWindowProxy->setPointerStyleSync(cursor);
    }
}

void QOhosView::updateWindowFocusable(bool focusable)
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->setWindowFocusable(focusable);
    m_nativeNode->setFocusable(focusable);
}

void QOhosView::updateWindowBackgroundColor(const QColor &color)
{
    m_nativeNode->setBackgroundColor(color);
}

void QOhosView::updateWindowBrightness(int brightness)
{
    m_nativeNode->setBrightness(brightness);
}

void QOhosView::updateWindowContrast(int contrast)
{
    m_nativeNode->setContrast(contrast);
}

void QOhosView::updateWindowSaturation(int saturation)
{
    m_nativeNode->setSaturation(saturation);
}

void QOhosView::updateWindowTransparentForInput(bool transparentForInput)
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->setWindowTouchable(!transparentForInput);
    m_nativeNode->setTransparentForInput(transparentForInput);
}

void QOhosView::updateWindowSizeLimits(const std::pair<QSize, QSize> &sizeLimits)
{
    if (m_ohosWindowProxy != nullptr) {
        auto windowMargins = QOhosPlatformWindow::fromQWindow(m_ownerWindow)->frameMargins();
        m_ohosWindowProxy->setWindowLimits(
            getQSizeGrownBy(sizeLimits.first, windowMargins),
            getQSizeGrownBy(sizeLimits.second, windowMargins));
    }
}

void QOhosView::updateWindowModality(Qt::WindowModality modality)
{
    if (m_ohosWindowProxy != nullptr) {
        if (viewType() != ViewType::SubWindow) {
            qCWarning(QtForOhos, "%s: Modality is supported only for sub-windows.", Q_FUNC_INFO);
            return;
        }
        if (modality == Qt::WindowModality::ApplicationModal) {
            qCWarning(
                QtForOhos,
                "%s: Qt::ApplicationModal policy is unsupported by the platform. The window will behave like Qt::WindowModal.",
                Q_FUNC_INFO);
        }

        auto ohosModalityType = mapQtWindowModalityToOhosOrDefault(modality);
        if (ohosModalityType.has_value())
            m_ohosWindowProxy->setSubWindowModalEnabled(ohosModalityType.value());
        else
            m_ohosWindowProxy->setSubWindowModalDisabled();
    }
}

void QOhosView::updateWindowTitle(const QString &title)
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->setTitle(title);
}

void QOhosView::scheduleSystemUpdateIfNeeded()
{
    if (m_updatePending)
        return;
    m_updatePending = QMetaObject::invokeMethod(
        this,
        [this]() {
            flushSystemPropertyUpdatesImmediate();
        },
        Qt::QueuedConnection);
}

QWindow *QOhosView::ownerWindow() const
{
    return m_ownerWindow;
}

void QOhosView::showImmediate()
{
    auto *platformWindow = QOhosPlatformWindow::fromQWindow(m_ownerWindow);

    auto currentViewTypeInfo = determineViewTypeAndLogicalParent(platformWindow);
    bool viewTypeChanged = viewType() != currentViewTypeInfo.viewType;

    if (viewTypeChanged) {
        auto qOhosWindowProxy = tryCreateWindowProxyIfNeeded(currentViewTypeInfo.viewType, currentViewTypeInfo.optLogicalParent);

        if (qOhosWindowProxy != nullptr
            && currentViewTypeInfo.viewType == ViewType::SubWindow) {
            constexpr bool preventSubWindowClose = true;
            qOhosWindowProxy->setSubWindowCloseHandler(
                [this]() {
                    if (!m_ownerWindow.isNull()) {
                        qOhosPrintfDebug("onSubWindowCloseHandler: calling close()");
                        QOhosCloseEventContext::runWithCloseRootCauseSet(
                            QOhosCloseEventContext::CloseRootCause::SubWindowClose,
                            [&]() {
                                m_ownerWindow->close();
                            });
                    }
                },
                preventSubWindowClose);
        }

        if (qOhosWindowProxy != nullptr) {
            m_nativeNode->setParent(qOhosWindowProxy->nodeXComponent());
            m_nativeNode->fillToParent();
        }

        setOrResetWindowProxy(qOhosWindowProxy, currentViewTypeInfo.optLogicalParent);
    } else {
        syncWindowStateImmediate();
    }

    if (viewType() == ViewType::MainWindow) {
        const auto windowStates = m_ownerWindow->windowStates();
        if (windowStates.testFlag(Qt::WindowState::WindowFullScreen)) {
            // A phone window created fullscreen still shows the system bars, so
            // hide them explicitly. On 2-in-1/tablet the window is already
            // immersive at creation; re-applying would only make it twitch.
            if (!viewTypeChanged || QOhosDeviceInfo::isPhone())
                setFullScreen();
        } else if (windowStates.testFlag(Qt::WindowState::WindowMaximized)) {
            maximize();
        } else if (QOhosDeviceInfo::isPhone()
                   && m_ownerWindow->flags().testFlag(Qt::ExpandedClientAreaHint)) {
            applyPhoneWindowChrome();
        }
    } else if (viewType() == ViewType::SubWindow && m_ohosWindowProxy != nullptr) {
        m_ohosWindowProxy->setFollowParentMultiScreenPolicy(true);
    }

    if (m_ohosWindowProxy != nullptr) {
        restoreMainWindow();
        showWindow();
    }

    auto *surface = surfaceOrNull();
    if (surface != nullptr)
        surface->clearNativeWindowSurface();

    m_nativeNode->setVisibility(true);
}

void QOhosView::applyPhoneWindowChrome()
{
    if (m_ohosWindowProxy == nullptr)
        return;

    // Lay the window out under the system bars when fullscreen or when the
    // client area is expanded; only fullscreen also hides the bars.
    const bool fullScreen = m_ownerWindow->windowStates().testFlag(Qt::WindowFullScreen);
    const bool expanded = m_ownerWindow->flags().testFlag(Qt::ExpandedClientAreaHint);
    m_ohosWindowProxy->setWindowLayoutFullScreen(fullScreen || expanded);
    m_ohosWindowProxy->setWindowSystemBarEnable(
        fullScreen ? QStringList{}
                   : QStringList{ QStringLiteral("status"), QStringLiteral("navigation") });
}

void QOhosView::setFullScreen()
{
    if (m_ohosWindowProxy == nullptr)
        return;

    if (!QOhosDeviceInfo::isPhone()) {
        flushSystemPropertyUpdatesImmediate();
        m_ohosWindowProxy->maximize(QOhosWindowProxy::MaximizePresentation::ENTER_IMMERSIVE);
    } else {
        applyPhoneWindowChrome();
    }
}

void QOhosView::recover()
{
    if (m_ohosWindowProxy == nullptr)
        return;

    if (!QOhosDeviceInfo::isPhone())
        m_ohosWindowProxy->recover();
    else
        applyPhoneWindowChrome();
}

void QOhosView::minimize()
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->minimize();
}

void QOhosView::requestActivate()
{
    if (m_ohosWindowProxy != nullptr) {
        if (QGuiApplicationPrivate::focus_window) {
            const auto *focusPlatformWindow = QOhosPlatformWindow::fromQWindow(QGuiApplicationPrivate::focus_window);
            const auto *focusView = focusPlatformWindow->ownedViewOrNull();
            if (focusView && focusView != this) {
                if (focusView->m_ohosWindowProxy) {
                    focusView->m_ohosWindowProxy->shiftAppWindowFocus(*m_ohosWindowProxy);
                } else {
                    // If the previously focused window is an EmbeddedWindow.
                    sendAsyncSyntheticWindowActiveEvent();
                }
            }
        }
    } else {
        // for EmbeddedWindow
        sendAsyncSyntheticWindowActiveEvent();
    }
}

void QOhosView::maximize()
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->maximize(QOhosWindowProxy::MaximizePresentation::EXIT_IMMERSIVE);
}

std::unique_ptr<QOhosView> QOhosView::createForWindow(QOhosPlatformWindow *window, QOhosPropertiesProvider windowPropertiesProvider)
{
    auto *qWindow = window->window();
    QNativeNode::CreateInfo createInfo;
    createInfo.geometry = window->geometry();
    createInfo.window = qWindow;
    createInfo.backgroundColor = windowPropertiesProvider.tryGetProperty<QColor, &QOhosPlatformWindow::surfaceBackgroundColorProperty>();
    createInfo.renderFitPolicyHint = qTransform(
        windowPropertiesProvider.tryGetProperty<int, &QOhosPlatformWindow::nativeNodeRenderFitPolicyHintProperty>(),
        [](int renderFit) {
            return static_cast<::ArkUI_RenderFit>(renderFit);
        });

    QPlatformWindow *parentPlatformWindow = window->parent();

    if (parentPlatformWindow != nullptr) {
        auto *parentPlatformWindow = static_cast<QOhosPlatformWindow *>(window->parent());
        auto *parentView = parentPlatformWindow->ownedViewOrNull();
        createInfo.optParent = parentView != nullptr
            ? std::optional(parentView->m_nativeNode.get())
            : std::nullopt;
    }

    auto nativeNode = QSharedPointer<QNativeNode>::create(createInfo);
    return std::make_unique<QOhosView>(qWindow, nativeNode, windowPropertiesProvider);
}

QOhosView::ViewGeometry QOhosView::viewGeometry() const
{
    QOhosView::ViewGeometry result;
    if (m_ohosWindowProxy != nullptr) {
        auto windowProperties = m_ohosWindowProxy->getWindowProperties();
        result.frameGeometry = windowProperties.windowRect;
        // Translate relative coordinates of the window to absolute screen values
        result.geometry = windowProperties.drawableRect.translated(windowProperties.windowRect.topLeft());
        // HACK:
        // "result.geometry" might contain invalid size but (1) it will likely contain proper
        // top-left corner coords and (2) "result.frameGeometry" is also valid, so adjust the
        // invalidated size based on these two.
        result.geometry.setSize(
            evaluateGeometrySizeBasedOnFrameGeometry(result.geometry.topLeft(), result.frameGeometry));
        result.displayId = windowProperties.displayId;
    } else {
        result.frameGeometry = m_nativeNode->geometry().toRect();
        result.geometry = result.frameGeometry;

        const auto *ancestorViewWithWindow = ancestorViewWithWindowOrNull();
        result.displayId = ancestorViewWithWindow != nullptr
            ? ancestorViewWithWindow->m_ohosWindowProxy->getWindowProperties().displayId
            : std::nullopt;
    }

    return result;
}

QMargins QOhosView::avoidAreaMargins(QOhosWindowProxy::AvoidAreaType type) const
{
    auto avoidArea = m_avoidAreasProvider(type);
    QMargins margins;
    tryUpdateMaximumMarginsFromAvoidArea(margins, avoidArea);
    return margins;
}

QOhosView::ViewType QOhosView::viewType() const
{
    if (!m_ohosWindowProxy)
        return ViewType::EmbeddedWindow;

    switch (m_ohosWindowProxy->windowProxyType()) {
    case WindowProxyType::FloatWindow:
        return ViewType::FloatWindow;
    case WindowProxyType::MainWindow:
        return ViewType::MainWindow;
    case WindowProxyType::SubWindow:
        return ViewType::SubWindow;
    }

    qOhosReportFatalErrorAndAbort("Unrecognized WindowProxyType: %d", static_cast<int>(m_ohosWindowProxy->windowProxyType()));
}

void QOhosView::forceGeometryUpdate()
{
    auto *platformWindow = QOhosPlatformWindow::fromQWindow(m_ownerWindow);
    auto targetWindowGeometry = platformWindow->geometry().marginsAdded(platformWindow->frameMargins());

    m_updateData.position.optPendingUpdateRequest = {targetWindowGeometry.topLeft(), {}};
    m_updateData.size.optPendingUpdateRequest = targetWindowGeometry.size();
    scheduleSystemUpdateIfNeeded();
}

void QOhosView::hide()
{
    switch (viewType()) {
    case ViewType::MainWindow:
        hideMainWindow();
        break;
    case ViewType::FloatWindow:
    case ViewType::SubWindow:
    {
        setOrResetWindowProxy(nullptr, nullptr);
        auto syntheticWindowHiddenEvent = QOhosWindowProxy::WindowEvent {
            .type = QOhosWindowProxy::WindowEventType::WINDOW_HIDDEN,
        };
        Q_EMIT windowEvent(syntheticWindowHiddenEvent);
        break;
    }
    case ViewType::EmbeddedWindow:
        Q_EMIT windowVisibilityChange(false);
        break;
    }

    m_nativeNode->setVisibility(false);
}

void QOhosView::handlePaletteChange()
{
    setTransparentBackground(isWindowTransparencyRequested());
}

void QOhosView::setWindowMask(const QOhosWindowProxy::WindowMask &mask)
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->setWindowMask(mask);
}

QOhosSurface *QOhosView::surfaceOrNull() const
{
    return m_nativeNode->surfaceOrNull();
}

WId QOhosView::viewWindowId() const
{
    return m_nativeNode->windowId();
}

void QOhosView::setParentOrReparent(QOhosView &parentView)
{
    qCDebug(QtForOhos) << "view:" << this << "setParentOrReparent parentView:" << &parentView;
    m_nativeNode->setParent(*parentView.m_nativeNode);
    setOrResetWindowProxy(nullptr, parentView.ownerWindow());
}

void QOhosView::tryDetachFromEmbeddedParent()
{
    if (viewType() != ViewType::EmbeddedWindow) {
        qCDebug(QtForOhos) << Q_FUNC_INFO << m_ownerWindow;
        return;
    }

    m_nativeNode->detachFromParentIfPresent();
    m_optLogicalParent = nullptr;

    if (m_ownerWindow->isVisible() && !m_ohosWindowProxy)
        showImmediate();
}

void QOhosView::setNativeNodeVisibility(bool visible)
{
    m_nativeNode->setVisibility(visible);
    m_lastMainWindowHideMethod = WindowHideMethod::NativeNodeVisibility;
}

void QOhosView::hideMainWindow()
{
    if (m_ohosWindowProxy->tryHideAbility()) {
        m_lastMainWindowHideMethod = WindowHideMethod::HideAbility;
        return;
    }

    if (QOhosPlatformWindow::isWindowBeingClosedOrDestroyed(m_ownerWindow)) {
        setNativeNodeVisibility(false);
        return;
    }

    m_ohosWindowProxy->minimize();
    m_lastMainWindowHideMethod = WindowHideMethod::Minimize;
}

void QOhosView::syncWindowStateImmediate(WindowStateSyncReason reason)
{
    if (viewType() == ViewType::EmbeddedWindow && m_optLogicalParent == nullptr)
        return;

    std::vector<std::function<void()>> postSurfaceDrawTasks;
    auto updatePostSurfaceDrawTask = qScopeGuard([&]() {
        if (!postSurfaceDrawTasks.empty()) {
            m_optPostSurfaceDrawTask = [postSurfaceDrawTasks = std::move(postSurfaceDrawTasks)]() {
                for (const auto &task: postSurfaceDrawTasks)
                    task();
            };
        }
    });

    if (reason == WindowStateSyncReason::ViewTypeChanged && m_ohosWindowProxy
            && viewType() == ViewType::MainWindow) {
        postSurfaceDrawTasks.emplace_back(
            [weakWindowProxy = QtOhos::makeWeakPtr(m_ohosWindowProxy)]() {
                auto windowProxy = weakWindowProxy.lock();
                if (windowProxy)
                    windowProxy->removeStartingWindow();
            });
    }

    auto *platformWindow = QOhosPlatformWindow::fromQWindow(m_ownerWindow);

    auto targetWindowLimitsToSet =
        viewType() == ViewType::EmbeddedWindow
            ? std::make_pair(m_ownerWindow->minimumSize(), m_ownerWindow->maximumSize())
            : std::make_pair(platformWindow->windowMinimumSize(), platformWindow->windowMaximumSize());

    auto windowFlags = QOhosPlatformWindow::platformWindowFlagsForQWindow(m_ownerWindow);
    bool focusable = !windowFlags.testFlag(Qt::WindowDoesNotAcceptFocus);

    auto submitSystemPropertyUpdate = [](auto &targetProperty, const auto &targetValue) {
        targetProperty.optPendingUpdateRequest = targetValue;
    };

    auto submitOrResetSystemPropertyUpdate = [&submitSystemPropertyUpdate](
        auto &targetProperty,
        const auto &targetValue,
        WindowGeometryPersistenceState windowGeometryPersistenceState) {
        if (windowGeometryPersistenceState == WindowGeometryPersistenceState::Enabled)
            targetProperty.optPendingUpdateRequest.reset();
        else
            submitSystemPropertyUpdate(targetProperty, targetValue);
    };

    submitSystemPropertyUpdate(m_updateData.title, m_ownerWindow->title());

    auto windowGeometryPersistenceState =
        viewType() == ViewType::MainWindow
            && m_geometryPersistencePolicy != ViewGeometryPersistencePolicy::Ignore
        ? syncWindowGeometryPersistenceState(m_ohosWindowProxy.get())
        : WindowGeometryPersistenceState::Disabled;

    auto currentRuntimeDeviceTypeAndMode = queryQOhosRuntimeDeviceAndMode();
    bool windowGeometrySyncEnabled = false;
    switch (currentRuntimeDeviceTypeAndMode) {
    case QOhosRuntimeDeviceTypeAndMode::HandheldDeviceWindowPcMode:
    case QOhosRuntimeDeviceTypeAndMode::_2in1:
        windowGeometrySyncEnabled = true;
        break;
    case QOhosRuntimeDeviceTypeAndMode::HandheldDeviceFullScreen:
        windowGeometrySyncEnabled = viewType() != ViewType::MainWindow;
        break;
    }

    std::optional<QOhosDisplayInfo::JsDisplayId> targetDisplayId;
    switch (viewType()) {
    case ViewType::SubWindow:
        if (m_optLogicalParent == nullptr)
            qOhosReportFatalErrorAndAbort("%s: logical parent for a subwindow is null", Q_FUNC_INFO);
        targetDisplayId = tryGetSubWindowJsDisplayId(m_optLogicalParent, *m_ohosWindowProxy);
        break;
    case ViewType::MainWindow:
    {
        targetDisplayId = m_ohosWindowProxy->tryGetMainWindowJsDisplayId();
        break;
    }
    case ViewType::FloatWindow:
    case ViewType::EmbeddedWindow:
        break;
    }

    auto targetGeometryToSet = platformWindow->lastRequestedWindowFrameGeometry();

    if (windowGeometrySyncEnabled) {
        submitSystemPropertyUpdate(m_updateData.sizeLimits, targetWindowLimitsToSet);
        submitOrResetSystemPropertyUpdate(
            m_updateData.size, targetGeometryToSet.size(), windowGeometryPersistenceState);

        if (!qt_window_private(m_ownerWindow)->positionAutomatic) {
            submitOrResetSystemPropertyUpdate(
                m_updateData.position,
                std::make_pair(targetGeometryToSet.topLeft(), targetDisplayId),
                windowGeometryPersistenceState);
        }
    }

    submitSystemPropertyUpdate(m_updateData.backgroundTransparent, isWindowTransparencyRequested());

    if (currentRuntimeDeviceTypeAndMode == QOhosRuntimeDeviceTypeAndMode::_2in1)
        submitSystemPropertyUpdate(m_updateData.focusable, focusable);

    if (viewType() == ViewType::SubWindow)
        submitSystemPropertyUpdate(m_updateData.modality, m_ownerWindow->modality());

    submitSystemPropertyUpdate(
        m_updateData.windowMinMaxCloseButtonsState,
        WindowMinMaxCloseButtonsState{
            .maxButtonShown = windowFlags.testFlag(Qt::WindowMaximizeButtonHint),
            .minButtonShown = windowFlags.testFlag(Qt::WindowMinimizeButtonHint),
            .closeButtonShown = windowFlags.testFlag(Qt::WindowCloseButtonHint),
        });

    submitSystemPropertyUpdate(
        m_updateData.windowStaysOnTop, windowFlags.testFlag(Qt::WindowStaysOnTopHint));

    submitSystemPropertyUpdate(m_updateData.frameless, windowFlags.testFlag(Qt::FramelessWindowHint));

    submitSystemPropertyUpdate(
        m_updateData.windowTransparentForInput, windowFlags.testFlag(Qt::WindowTransparentForInput));

    flushSystemPropertyUpdatesImmediate();

    bool disableWindowShadow = windowFlags.testFlag(Qt::WindowType::NoDropShadowWindowHint);
    if (disableWindowShadow)
        setWindowShadowDisabled();

    if (!m_ownerWindow->mask().isNull() && m_ohosWindowProxy != nullptr) {
        m_ohosWindowProxy->setWindowMask(
            QOhosWindowProxy::WindowMask {
                .windowMaskRegion = m_ownerWindow->mask(),
            },
            targetGeometryToSet.size());
    }
    const auto privacyModeSetting = m_windowPropertiesProvider
        .tryGetProperty<bool, &QOhosPlatformWindow::windowPrivacyModeSettingProperty>();
    if (privacyModeSetting.has_value())
        setPrivacyMode(privacyModeSetting.value());

    const auto windowCornerRadius = m_windowPropertiesProvider
        .tryGetProperty<double, &QOhosPlatformWindow::windowCornerRadiusProperty>();
    if (windowCornerRadius.has_value())
        setWindowCornerRadius(windowCornerRadius.value());

    const auto keepScreenOn = m_windowPropertiesProvider
        .tryGetProperty<bool, &QOhosPlatformWindow::windowKeepScreenOnProperty>();
    if (keepScreenOn.has_value())
        setWindowKeepScreenOn(keepScreenOn.value());

    auto fixedSizeStateEnabled = m_windowPropertiesProvider
        .tryGetProperty<bool, &QOhosPlatformWindow::windowFixedSizeStateProperty>();
    if (fixedSizeStateEnabled.has_value())
        setFixedSizeStateEnabled(fixedSizeStateEnabled.value());

    auto windowDragResizable = m_windowPropertiesProvider
        .tryGetProperty<bool, &QOhosPlatformWindow::windowDragResizableProperty>();
    if (windowDragResizable.has_value())
        setWindowDragResizable(windowDragResizable.value());

    // NOTE - when position automatic is set we do not control window position
    // therefore no window callback about window position change will be sent
    // and the screen that given window belongs to needs to be synchronized
    bool shouldSynchronizeTargetDisplayIdWithQpa =
        qt_window_private(m_ownerWindow)->positionAutomatic
        && reason == WindowStateSyncReason::ViewTypeChanged
        && m_ohosWindowProxy != nullptr;

    if (shouldSynchronizeTargetDisplayIdWithQpa) {
        auto optTargetDisplayId = m_ohosWindowProxy->getWindowProperties().displayId;
        if (optTargetDisplayId.has_value()) {
            QOhosDisplayInfo::JsDisplayId syntheticDisplayIdChangeEvent = optTargetDisplayId.value();
            Q_EMIT windowDisplayIdChanged(syntheticDisplayIdChangeEvent);
        }
    }
}

void QOhosView::flushSystemPropertyUpdatesImmediate()
{
    static const auto systemDataPropertyUpdateFuncPairsTuple = makeSystemUpdateDataPropertyUpdateFuncPairsTuple();

    QtOhos::tupleForEach(
        systemDataPropertyUpdateFuncPairsTuple,
        [&](const auto &updateDataMemberPtrUpdateFuncPair) {
            auto memberPtr = updateDataMemberPtrUpdateFuncPair.first;
            auto updateFuncPtr = updateDataMemberPtrUpdateFuncPair.second;
            auto &property = m_updateData.*memberPtr;
            auto optUpdateRequest = std::exchange(property.optPendingUpdateRequest, {});
            if (optUpdateRequest.has_value())
                (this->*updateFuncPtr)(optUpdateRequest.value());
    });
    m_updatePending = false;
}

void QOhosView::addForeignWindowChild(QOhosForeignWindow *foreignWindow)
{
    m_nativeNode->addForeignWindowChild(foreignWindow);
}

std::optional<QSize> QOhosView::surfaceResolution() const
{
    auto *surface = m_nativeNode->surfaceOrNull();
    return surface != nullptr
        ? surface->surfaceResolution()
        : std::nullopt;
}

bool QOhosView::isFullscreenImmersiveModeEnabled()
{
    return m_ohosWindowProxy != nullptr
        ? m_ohosWindowProxy->getImmersiveModeEnabledState()
        : false;
}

bool QOhosView::isSubWindowCoveringFullScreen() const
{
    if (viewType() == ViewType::SubWindow) {
        auto *screen = m_ownerWindow->screen();
        return screen != nullptr && m_ownerWindow->geometry().size() == screen->geometry().size();
    }

    return false;
}

void QOhosView::updateWindowMinMaxCloseButtonsState(const WindowMinMaxCloseButtonsState &state)
{
    if (m_ohosWindowProxy != nullptr) {
        m_ohosWindowProxy->setWindowTitleButtonVisible(
            state.maxButtonShown, state.minButtonShown, state.closeButtonShown);
    }
}

void QOhosView::updateWindowStaysOnTop(bool staysOnTop)
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->setWindowTopmost(staysOnTop);
}

void QOhosView::updateWindowFrameless(bool frameless)
{
    bool supportsFramelessWindow = viewType() == ViewType::MainWindow || viewType() == ViewType::SubWindow;
    if (m_ohosWindowProxy != nullptr && supportsFramelessWindow) {
        m_ohosWindowProxy->setWindowDecorVisible(!frameless);
        m_ohosWindowProxy->setWindowTitleMoveEnabled(!frameless);
    }
}

void QOhosView::setWindowDragResizable(bool dragResizable)
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->enableDrag(dragResizable);
}

void QOhosView::setWindowMinMaxCloseButtonState(const WindowMinMaxCloseButtonsState &state)
{
    setSystemUpdateProperty(&SystemUpdateData::windowMinMaxCloseButtonsState, state);
}

void QOhosView::setWindowStaysOnTop(bool staysOnTop)
{
    setSystemUpdateProperty(&SystemUpdateData::windowStaysOnTop, staysOnTop);
}

void QOhosView::setFramelessWindow(bool frameless)
{
    setSystemUpdateProperty(&SystemUpdateData::frameless, frameless);
}

void QOhosView::setWindowShadowDisabled()
{
    if (m_ohosWindowProxy != nullptr) {
        constexpr double windowShadowDisabledRadius = 0.0;
        m_ohosWindowProxy->setWindowShadowRadius(windowShadowDisabledRadius);
    }
}

void QOhosView::setWindowCornerRadius(double radius)
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->setWindowCornerRadius(radius);
}

void QOhosView::setWindowTransparentForInput(bool transparentForInput)
{
    setSystemUpdateProperty(&SystemUpdateData::windowTransparentForInput, transparentForInput);
}

bool QOhosView::startMoving()
{
    if (m_ohosWindowProxy != nullptr)
        return m_ohosWindowProxy->startMoving();
    return false;
}

void QOhosView::setPrivacyMode(bool privacyModeEnabled)
{
    if (m_ohosWindowProxy != nullptr)
        m_ohosWindowProxy->setWindowPrivacyMode(privacyModeEnabled);
}

void QOhosView::startDrag(
    const std::vector<QImage> &images, const QPointF &hotspot,
    const QMimeData &mimeData, QOhosConsumer<Qt::DropAction> dropActionConsumer)
{
    m_nativeNode->startDrag(images, hotspot, mimeData, std::move(dropActionConsumer));
}

void QOhosView::restoreMainWindow()
{
    const auto lastMainWindowHideMethod = std::exchange(m_lastMainWindowHideMethod, {});
    if (m_ohosWindowProxy == nullptr || !m_ohosWindowProxy->qtIsMainWindow())
        return;

    if (!lastMainWindowHideMethod.has_value()) {
        m_ohosWindowProxy->restore();
        return;
    }

    switch (lastMainWindowHideMethod.value()) {
    case WindowHideMethod::NativeNodeVisibility:
    case WindowHideMethod::Minimize:
        m_ohosWindowProxy->restore();
        break;
    case WindowHideMethod::HideAbility:
        m_ohosWindowProxy->showAbility();
        break;
    }
}

void QOhosView::handleWindowStateChange(
    Qt::WindowStates previousWindowState, Qt::WindowStates currentWindowState)
{
    auto stateChange = previousWindowState ^ currentWindowState;
    if (stateChange.testFlag(Qt::WindowState::WindowMinimized) && !currentWindowState.testFlag(Qt::WindowState::WindowMinimized))
        restoreMainWindow();

    if (stateChange.testFlag(Qt::WindowState::WindowMaximized) && currentWindowState.testFlag(Qt::WindowState::WindowMaximized)) {
        maximize();
        return;
    }

    if (stateChange.testFlag(Qt::WindowState::WindowFullScreen) && currentWindowState.testFlag(Qt::WindowState::WindowFullScreen)) {
        setFullScreen();
        return;
    }

    if (stateChange.testFlag(Qt::WindowState::WindowMinimized) && currentWindowState.testFlag(Qt::WindowState::WindowMinimized)) {
        minimize();
        return;
    }

    if (!currentWindowState) {
        recover();
        return;
    }
}

void QOhosView::handleWindowFlagsChange(
    Qt::WindowFlags previousWindowFlags, Qt::WindowFlags currentWindowFlags)
{
    const auto flagsChange = previousWindowFlags ^ currentWindowFlags;

    bool minMaxCloseButtonStateChanged = (flagsChange & (Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)) != 0;
    bool framelessChanged = (flagsChange & Qt::FramelessWindowHint) != 0;

    if (minMaxCloseButtonStateChanged || framelessChanged) {
        setWindowMinMaxCloseButtonState(QOhosView::WindowMinMaxCloseButtonsState{
            .maxButtonShown = currentWindowFlags.testFlag(Qt::WindowMaximizeButtonHint),
            .minButtonShown = currentWindowFlags.testFlag(Qt::WindowMinimizeButtonHint),
            .closeButtonShown = currentWindowFlags.testFlag(Qt::WindowCloseButtonHint),
        });
    }

    bool windowStaysOnTopChanged = (flagsChange & Qt::WindowStaysOnTopHint) != 0;
    if (windowStaysOnTopChanged)
        setWindowStaysOnTop(currentWindowFlags.testFlag(Qt::WindowStaysOnTopHint));

    bool transparentForInputChanged = (flagsChange & Qt::WindowTransparentForInput) != 0;
    if (transparentForInputChanged)
        setWindowTransparentForInput(currentWindowFlags.testFlag(Qt::WindowTransparentForInput));

    if (framelessChanged)
        setFramelessWindow(currentWindowFlags.testFlag(Qt::FramelessWindowHint));

    bool focusableChanged = (flagsChange & Qt::WindowDoesNotAcceptFocus) != 0;
    if (focusableChanged) {
        bool windowAcceptsFocus = !currentWindowFlags.testFlag(Qt::WindowDoesNotAcceptFocus);
        setFocusable(windowAcceptsFocus);
    }

    bool disableWindowShadowChanged = (flagsChange & Qt::NoDropShadowWindowHint) != 0;
    if (disableWindowShadowChanged && currentWindowFlags.testFlag(Qt::NoDropShadowWindowHint))
        setWindowShadowDisabled();

    bool expandedClientAreaChanged = (flagsChange & Qt::ExpandedClientAreaHint) != 0;
    if (expandedClientAreaChanged && QOhosDeviceInfo::isPhone())
        applyPhoneWindowChrome();
}

QArkUi::QQtEmbeddedWindowNode::NodeAreaInfo QOhosView::nodeAreaInfo() const
{
    return m_nativeNode->nodeAreaInfo();
}

bool QOhosView::WindowMinMaxCloseButtonsState::operator==(const WindowMinMaxCloseButtonsState &other) const
{
    return maxButtonShown == other.maxButtonShown
        && minButtonShown == other.minButtonShown
        && closeButtonShown == other.closeButtonShown;
}

bool QOhosView::WindowMinMaxCloseButtonsState::operator!=(const WindowMinMaxCloseButtonsState &other) const
{
    return !(*this == other);
}

void QOhosView::setOrResetWindowProxy(std::shared_ptr<QOhosWindowProxy> windowProxy, QWindow *optLogicalParent)
{
    m_ohosWindowProxy.reset();
    m_ohosWindowProxy = windowProxy
        ? QtOhos::makeSharedPtrWithAttachedExtraData<QOhosWindowProxy>(
            windowProxy,
            QWindowProxyRegistry::instance().registerQWindowWithWindowProxy(ownerWindow(), *windowProxy))
        : nullptr;
    m_optLogicalParent = optLogicalParent;
    syncWindowStateImmediate(WindowStateSyncReason::ViewTypeChanged);
}

const QOhosView *QOhosView::viewParentOrNull() const
{
    auto *platformWindow = m_optLogicalParent != nullptr
        ? QOhosPlatformWindow::fromQWindowOrNull(m_optLogicalParent)
        : nullptr;

    return platformWindow != nullptr
        ? platformWindow->ownedViewOrNull()
        : nullptr;
}

const QOhosView *QOhosView::ancestorViewWithWindowOrNull() const
{
    const QOhosView *parent = viewParentOrNull();
    while (parent != nullptr && !parent->m_ohosWindowProxy)
        parent = parent->viewParentOrNull();

    return parent != nullptr && parent->m_ohosWindowProxy
        ? parent
        : nullptr;
}

QPixmap QOhosView::makeSnapshot() const
{
    return m_ohosWindowProxy ? m_ohosWindowProxy->snapshot() : QPixmap();
}

QRect QOhosView::nodeScreenGeometryPixels() const
{
    return m_nativeNode->nodeScreenGeometryPixels();
}

QRect QOhosView::nodeParentRelativeGeometryPixels() const
{
    return m_nativeNode->nodeParentRelativeGeometryPixels();
}

QT_END_NAMESPACE
