// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qohoswindowproxy.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qset.h>
#include <QtCore/private/qnapi_p.h>
#include <qohosjsenv_p.h>
#include <QtCore/qscopeguard.h>
#include <QtGui/private/qohosimageconversions_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qimage.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <qarkui/input.h>
#include <qarkui/qarkuiutils.h>
#include <qarkui/qxcomponentregistry.h>
#include <qarkui/window.h>
#include <qarkui/window_manager.h>
#include <qohosdeviceinfo_p.h>
#include <qohosdisplayinfo.h>
#include <qohosenums.h>
#include <qohosjsutils.h>
#include <qohospixelmapconversions.h>
#include <qohosplugincore.h>
#include <qohossettings.h>
#include <qohosutils.h>
#include <render/qohosbatchingrequestshandler.h>
#include <render/qohosjswindowregistry.h>
#include <render/qohoswindowproxydatafactory.h>
#include <render/qxcomponent.h>
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace
{

using QOhosPointerStyle = QtOhos::enums::ohos::multimodalInput::pointer::PointerStyle;

QRect ohosWindowRectToQRect(const QNapi::Object &ohosWindowRect)
{
    return QRect {
        ohosWindowRect.get<QNapi::Number>("left"),
        ohosWindowRect.get<QNapi::Number>("top"),
        ohosWindowRect.get<QNapi::Number>("width"),
        ohosWindowRect.get<QNapi::Number>("height"),
    };
}

QOhosWindowProxy::AvoidArea mapAvoidAreaFromJs(const QNapi::Object &avoidAreaObject)
{
    return {
        .visible = avoidAreaObject.get<QNapi::Boolean>("visible"),
        .leftRect = ohosWindowRectToQRect(avoidAreaObject.get<QNapi::Object>("leftRect")),
        .topRect = ohosWindowRectToQRect(avoidAreaObject.get<QNapi::Object>("topRect")),
        .rightRect = ohosWindowRectToQRect(avoidAreaObject.get<QNapi::Object>("rightRect")),
        .bottomRect = ohosWindowRectToQRect(avoidAreaObject.get<QNapi::Object>("bottomRect")),
    };
}

QOhosPointerStyle convertToOhosCursor(Qt::CursorShape shape)
{
    switch (shape) {
    case Qt::ArrowCursor:
        return QOhosPointerStyle::DEFAULT;
    case Qt::UpArrowCursor:
        return QOhosPointerStyle::NORTH;
    case Qt::CrossCursor:
        return QOhosPointerStyle::CROSS;
    case Qt::WaitCursor:
        return QOhosPointerStyle::LOADING;
    case Qt::IBeamCursor:
        return QOhosPointerStyle::TEXT_CURSOR;
    case Qt::SizeVerCursor:
        return QOhosPointerStyle::NORTH_SOUTH;
    case Qt::SizeHorCursor:
        return QOhosPointerStyle::WEST_EAST;
    case Qt::SizeBDiagCursor:
        return QOhosPointerStyle::NORTH_EAST_SOUTH_WEST;
    case Qt::SizeFDiagCursor:
        return QOhosPointerStyle::NORTH_WEST_SOUTH_EAST;
    case Qt::SizeAllCursor:
        return QOhosPointerStyle::MOVE;
    case Qt::BlankCursor:
        // TODO: there is no dedicated Ohos 'Qt::BlankCursor'. Return default.
        return QOhosPointerStyle::DEFAULT;
    case Qt::SplitVCursor:
        return QOhosPointerStyle::RESIZE_UP_DOWN;
    case Qt::SplitHCursor:
        return QOhosPointerStyle::RESIZE_LEFT_RIGHT;
    case Qt::PointingHandCursor:
        return QOhosPointerStyle::HAND_POINTING;
    case Qt::ForbiddenCursor:
        return QOhosPointerStyle::CURSOR_FORBID;
    case Qt::WhatsThisCursor:
        return QOhosPointerStyle::HELP;
    case Qt::BusyCursor:
        return QOhosPointerStyle::RUNNING;
    case Qt::OpenHandCursor:
        return QOhosPointerStyle::HAND_OPEN;
    case Qt::ClosedHandCursor:
        return QOhosPointerStyle::HAND_GRABBING;
    case Qt::DragCopyCursor:
        return QOhosPointerStyle::CURSOR_COPY;
    case Qt::DragMoveCursor:
        return QOhosPointerStyle::CROSS;
    case Qt::DragLinkCursor:
        // TODO: there is no dedicated Ohos 'Qt::DragLinkCursor'. Return default.
        return QOhosPointerStyle::DEFAULT;
    case Qt::BitmapCursor:
        // TODO: there is no dedicated Ohos 'Qt::BitmapCursor'. Return default.
        return QOhosPointerStyle::DEFAULT;
    case Qt::CustomCursor:
        // TODO: there is no dedicated Ohos 'Qt::CustomCursor'. Return default.
        return QOhosPointerStyle::DEFAULT;
    }

    return QOhosPointerStyle::DEFAULT;
}

std::optional<double> getOptionalNumberPropAsOptionalDouble(const QNapi::Object &object, const std::string &propertyName)
{
    auto propOrEmpty = QNapi::getOptionalPropOrEmpty<QNapi::Number>(object, propertyName);
    return !propOrEmpty.IsEmpty()
        ? std::optional<double>(propOrEmpty)
        : std::nullopt;
}

QArkUi::WindowProperties getWindowPropertiesFromJsWindow(QNapi::Object jsWindow)
{
    auto windowPropsObj = jsWindow.eval<QNapi::Object>("getWindowProperties()");
    auto displayIdOrEmpty = QNapi::getOptionalPropOrEmpty<QNapi::Number>(windowPropsObj, "displayId");
    return QArkUi::WindowProperties {
        .windowRect = ohosWindowRectToQRect(windowPropsObj.get<QNapi::Object>("windowRect")),
        .drawableRect = ohosWindowRectToQRect(windowPropsObj.get<QNapi::Object>("drawableRect")),
        .id = QArkUi::JsWindowId(windowPropsObj.get<QNapi::Number>("id")),
        .displayId = !displayIdOrEmpty.IsEmpty()
            ? std::optional(QOhosDisplayInfo::JsDisplayId{displayIdOrEmpty.DoubleValue()})
            : std::nullopt,
    };
}

QNapi::Object toNapiObject(napi_env env, const QOhosWindowProxy::MoveConfiguration &moveConfiguration)
{
    auto moveConfigurationObject = QNapi::makeObject(env);
    if (moveConfiguration.displayId.has_value()) {
        moveConfigurationObject.set(
            "displayId", moveConfiguration.displayId.value().value());
    }
    return moveConfigurationObject;
}

template<typename ...Args>
std::function<void(Args...)> makeQtThreadWindowCallbackDelegate(
    std::function<void(Args...)> QOhosWindowProxy::WindowCallbacks::*memberPtr,
    std::shared_ptr<QOhosWindowProxy::WindowCallbacks> qtWindowCallbacks)
{
    auto weakQtWindowCallbacks = QtOhos::makeWeakPtr(qtWindowCallbacks);

    return [memberPtr, weakQtWindowCallbacks](Args ...args) {
        QtOhos::invokeInQtThread([memberPtr, weakQtWindowCallbacks, args...]() {
            auto qtWindowCallbacks = weakQtWindowCallbacks.lock();
            if (qtWindowCallbacks)
                (*qtWindowCallbacks.*memberPtr)(args...);
        });
    };
}

template<typename T>
std::function<void(T)> makeCompressingQtThreadWindowCallbackDelegate(
    std::function<void(T)> QOhosWindowProxy::WindowCallbacks::*memberPtr,
    std::shared_ptr<QOhosWindowProxy::WindowCallbacks> qtWindowCallbacks)
{
    auto weakQtWindowCallbacks = QtOhos::makeWeakPtr(qtWindowCallbacks);

    return QtOhos::makeCompressingAsyncConsumer<T>(
        [memberPtr, weakQtWindowCallbacks](T value) {
            auto qtWindowCallbacks = weakQtWindowCallbacks.lock();
            if (qtWindowCallbacks)
                (*qtWindowCallbacks.*memberPtr)(value);
        },
        QtOhos::invokeInQtThread);
}

bool isPointInNonClientArea(const QPoint &point, const QArkUi::WindowProperties &windowProperties)
{
    constexpr bool containsPolicyExcludeEdgeValue = true;

    auto drawableRectInScreenSpace =
        windowProperties.drawableRect.translated(windowProperties.windowRect.topLeft());
    return !drawableRectInScreenSpace.contains(point, containsPolicyExcludeEdgeValue);
}

std::optional<QEvent::Type> tryMapMouseEventActionToNonClientAreaEventType(::Input_MouseEventAction action)
{
    switch (action) {
    case ::MOUSE_ACTION_MOVE:
        return QEvent::NonClientAreaMouseMove;
    case ::MOUSE_ACTION_BUTTON_DOWN:
        return QEvent::NonClientAreaMouseButtonPress;
    case ::MOUSE_ACTION_BUTTON_UP:
        return QEvent::NonClientAreaMouseButtonRelease;
    case ::MOUSE_ACTION_CANCEL:
    case ::MOUSE_ACTION_AXIS_BEGIN:
    case ::MOUSE_ACTION_AXIS_UPDATE:
    case ::MOUSE_ACTION_AXIS_END:
        break;
    }
    return {};
}

std::optional<Qt::MouseButton> tryMapMouseEventButtonToQt(::Input_MouseEventButton button)
{
    switch (button) {
    case ::MOUSE_BUTTON_LEFT:
        return Qt::LeftButton;
    case ::MOUSE_BUTTON_MIDDLE:
        return Qt::MiddleButton;
    case ::MOUSE_BUTTON_RIGHT:
        return Qt::RightButton;
    case ::MOUSE_BUTTON_FORWARD:
        return Qt::ForwardButton;
    case ::MOUSE_BUTTON_BACK:
        return Qt::BackButton;
    case ::MOUSE_BUTTON_NONE:
        break;
    }
    return {};
}

std::optional<QEvent::Type> tryMapTouchEventActionToNonClientAreaEventType(::Input_TouchEventAction action)
{
    switch (action) {
    case ::TOUCH_ACTION_MOVE:
        return QEvent::NonClientAreaMouseMove;
    case ::TOUCH_ACTION_DOWN:
        return QEvent::NonClientAreaMouseButtonPress;
    case ::TOUCH_ACTION_UP:
        return QEvent::NonClientAreaMouseButtonRelease;
    case ::TOUCH_ACTION_CANCEL:
        break;
    }
    return {};
}

template<typename EnumsContainer>
std::string mapEnumsToLogString(const EnumsContainer &enums)
{
    std::string output;
    for (auto enumValue : enums) {
        if (!output.empty())
            output += ",";
        output += std::to_string(static_cast<std::underlying_type_t<decltype(enumValue)>>(enumValue));
    }
    return output;
}

bool isWindowClosingFromSystem(
    QNapi::Object jsWindow, WindowProxyType windowType, std::shared_ptr<QtOhos::QAbilityPeer> abilityPeer)
{
    const bool boundToAbility = windowType != WindowProxyType::FloatWindow;
    return QtOhos::JsWindowsTracker::isWindowClosing(jsWindow) || (boundToAbility && abilityPeer->isTerminating());
}

}

const QOhosWindowProxy::EventHandlerDescriptor QOhosWindowProxy::eventHandlerDescriptors[] = {
    {
        .eventName = "avoidAreaChange",
        .eventHandler = &QOhosWindowProxy::JsScopeData::handleAvoidAreaChangeCallback,
        .eventHandlerFlags = {},
    },
    {
        .eventName = "touchOutside",
        .eventHandler = &QOhosWindowProxy::JsScopeData::handleWindowTouchOutsideCallback,
        .eventHandlerFlags = {},
    },
    {
        .eventName = "windowEvent",
        .eventHandler = &QOhosWindowProxy::JsScopeData::handleWindowEventCallback,
        .eventHandlerFlags = EventHandlerFlagBits::allowCallWhenAbilityIsTerminating,
    },
    {
        .eventName = "windowRectChange",
        .eventHandler = &QOhosWindowProxy::JsScopeData::handleWindowRectChangeCallback,
        .eventHandlerFlags = {},
    },
    {
        .eventName = "rectChangeInGlobalDisplay",
        .eventHandler = &QOhosWindowProxy::JsScopeData::handleWindowRectChangeInGlobalDisplayCallback,
        .eventHandlerFlags = {},
    },
    {
        .eventName = "windowStatusChange",
        .eventHandler = &QOhosWindowProxy::JsScopeData::handleWindowStatusCallback,
        .eventHandlerFlags = {},
    },
    {
        .eventName = "windowVisibilityChange",
        .eventHandler = &QOhosWindowProxy::JsScopeData::handleWindowVisibilityCallback,
        .eventHandlerFlags = {},
    },
    {
        .eventName = "displayIdChange",
        .eventHandler = &QOhosWindowProxy::JsScopeData::handleWindowDisplayIdChangeCallback,
        .eventHandlerFlags = EventHandlerFlagBits::allowEventHandlerRegistrationFailure,
    },
};

QOhosWindowProxy::QOhosWindowProxy(
    QOhosWindowProxyData windowProxyData)
    : m_jsScopeData(
        QtOhos::makeProxyWithJsThreadDeleter(
            std::make_shared<JsScopeData>(
                windowProxyData.windowProxyType,
                std::move(windowProxyData.jsWindow),
                std::move(windowProxyData.jsKeepAliveData),
                std::move(windowProxyData.qAbilityPeer),
                windowProxyData.owningQWindowRef)))
    , m_windowProxyType(windowProxyData.windowProxyType)
    , m_nodeXComponent(windowProxyData.nodeXComponent)
    , m_qAbilityInstanceId(m_jsScopeData->qAbilityPeer->instanceId())
{
    std::vector<std::shared_ptr<void>> eventListenersHandles;
    for (const auto &eventHandlerDescriptor : eventHandlerDescriptors) {
        eventListenersHandles.push_back(
            m_jsScopeData->registerEventListener(
                eventHandlerDescriptor.eventName, eventHandlerDescriptor.eventHandler,
                eventHandlerDescriptor.eventHandlerFlags));
    }
    m_jsScopeData->m_eventListenersHandle = QtOhos::moveToSharedPtr(std::move(eventListenersHandles));
}

QOhosWindowProxy::~QOhosWindowProxy()
{
    // NOTE
    // we need to unregister from JS Window events now, while the Window is
    // still in consistent state. While destroying the m_jsScopeData we
    // call Window::destroyWindow(), which makes the JS object unusable
    // (trying to unregister something on it will throw).
    m_subWindowCloseRegistrationHandle.reset();
    m_jsScopeData.reset();
}

void QOhosWindowProxy::removeStartingWindow()
{
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }
            auto optQUiAbilityPeer
                = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_jsScopeData->qAbilityPeer);
            if (!optQUiAbilityPeer) {
                taskPromise();
                return;
            }
            auto promise = optQUiAbilityPeer->windowStage().evalToPromiseOrRejectOnThrow(
                "removeStartingWindow()");
            promise.onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::moveWindowToGlobalOrGlobalDisplay(
    const QPoint &point, std::optional<QOhosDisplayInfo::JsDisplayId> optMoveToTargetDisplay)
{
    auto displayIdValue = optMoveToTargetDisplay.value_or(QOhosDisplayInfo::JsDisplayId(-1)).value();
    qOhosPrintfDebug("%s: %d,%d,%f", Q_FUNC_INFO, point.x(), point.y(), displayIdValue);

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }

            const auto primaryJsDisplayId = QOhosDisplayInfo::JsDisplayId(0);

            auto targetDisplayId = optMoveToTargetDisplay.has_value()
                ? optMoveToTargetDisplay.value()
                : getWindowProperties().displayId.value_or(primaryJsDisplayId);

            auto optDisplayInfo = qTransform(
                QOhosDisplayInfo::tryGetDisplayById(jsState, targetDisplayId),
                [&](auto displayObject) {
                    return QOhosDisplayInfo::makeFromOhosDisplayObject(jsState, displayObject);
                });

            auto optIsDisplayMainOrExtended = qTransform(
                optDisplayInfo,
                [](const QOhosDisplayInfo &displayInfo) {
                    return displayInfo.isDisplayMainOrExtended();
                });

            bool isDisplayMainOrExtended;
            if (optIsDisplayMainOrExtended.has_value()) {
                isDisplayMainOrExtended = optIsDisplayMainOrExtended.value();
            } else {
                qOhosPrintfWarning(
                    "%s: no display source mode detected. Assumming main/extended screen", Q_FUNC_INFO);
                isDisplayMainOrExtended = true;
            }

            QNapi::Promise promise;
            if (!isDisplayMainOrExtended) {
                auto optDisplayOffset = qAndThen(
                    optDisplayInfo,
                    [](const QOhosDisplayInfo &displayInfo) {
                        return displayInfo.topLeftOffsetPixels;
                    });

                const QPoint defaultDisplayOffset(0, 0);
                auto targetCoordinates = point - optDisplayOffset.value_or(defaultDisplayOffset);

                if (!optMoveToTargetDisplay.has_value())
                    qOhosPrintfWarning("%s: trying to move window to not valid target display", Q_FUNC_INFO);

                auto moveConfigurationObject = toNapiObject(
                    jsState.env(),
                    MoveConfiguration {
                        .displayId = optMoveToTargetDisplay,
                    });

                promise = m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow(
                    "moveWindowToGlobal(*)", {targetCoordinates.x(), targetCoordinates.y(), moveConfigurationObject});
            } else {
                promise = m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow(
                    "moveWindowToGlobalDisplay(*)", {point.x(), point.y()});
            }

            promise.onFinally(std::move(taskPromise));
        });
}

void QOhosWindowProxy::setSize(const QSize &size)
{
    qOhosPrintfDebug("%s: %d,%d", Q_FUNC_INFO, size.width(), size.height());

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }

            const auto currentSize = getWindowPropertiesFromJsWindow(
                m_jsScopeData->jsWindowRef->jsObject()).windowRect.size();
            if (currentSize == size) {
                taskPromise();
                return;
            }

            auto promise = m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow(
                "resizeAsync(*)", {size.width(), size.height()});
            promise.onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowBackgroundColor(const QColor &color)
{
    qCDebug(QtForOhos) << Q_FUNC_INFO << color;

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;
        m_jsScopeData->jsWindowRef->eval("setWindowBackgroundColor(*)", {color.name(QColor::HexArgb).toStdString()});
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::setCustomCursor(const QImage &customCursorImage, const QPoint &hotSpot)
{
    const auto maxOhosCustomCursorSize = QSize(256, 256);
    const auto customCursorSize = customCursorImage.size();
    if (customCursorSize.width() > maxOhosCustomCursorSize.width()
        || customCursorSize.height() > maxOhosCustomCursorSize.height()) {
        qOhosPrintfError(
            "%s: can't set %dx%d custom cursor, OHOS max custom cursor size is %dx%d",
            Q_FUNC_INFO, customCursorSize.width(), customCursorSize.height(),
            maxOhosCustomCursorSize.width(), maxOhosCustomCursorSize.height());
        return;
    }

    auto convertedImage = customCursorImage.convertToFormat(QImage::Format_RGBA8888);
    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
        if (m_jsScopeData->isWindowClosing()) {
            taskPromise();
            return;
        }
        auto windowId = m_jsScopeData->jsWindowRef->jsObject().get<QNapi::Number>("getWindowProperties().id");

        auto jsCursor = QNapi::makeObject(
            jsState.env(),
            {
                {"pixelMap", makeOhosNapiPixelMapFromQImage(jsState, convertedImage)},
                {"focusX", hotSpot.x()},
                {"focusY", hotSpot.y()},
            });

        auto jsCursorConfig = QNapi::makeObject(
            jsState.env(),
            {
                {"followSystem", false},
            });

        jsState.evalToPromiseOrRejectOnThrow(
            "@ohos.multimodalInput.pointer.setCustomCursor(*)", {windowId, jsCursor, jsCursorConfig})
        .onCatch(QtOhos::makeErrorLoggingJsCallback("setCustomCursor()"))
        .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::setPointerStyleSync(const QCursor &cursor)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &jsState) {
        if (m_jsScopeData->isWindowClosing())
            return;
        auto windowId = m_jsScopeData->jsWindowRef->jsObject().get<QNapi::Number>("getWindowProperties().id");

        jsState.eval(
            "@ohos.multimodalInput.pointer.setPointerStyleSync(*)",
            {windowId, jsState.mapOhosEnumToJs(convertToOhosCursor(cursor.shape()))});
    },
    Q_FUNC_INFO);
}

QArkUi::WindowProperties QOhosWindowProxy::getWindowProperties() const
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            if (m_jsScopeData->isWindowClosing())
                return QArkUi::WindowProperties {};
            return getWindowPropertiesFromJsWindow(m_jsScopeData->jsWindowRef->jsObject());
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowCallbackReceiver(std::unique_ptr<WindowCallbacks> callbackReceiver)
{
    auto sharedWindowCallbackReceiver = std::shared_ptr<WindowCallbacks>(std::move(callbackReceiver));

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsScopeData->windowCallbackReceiver = QtOhos::moveToSharedPtr(WindowCallbacks {
            .onWindowEvent = makeQtThreadWindowCallbackDelegate(&WindowCallbacks::onWindowEvent, sharedWindowCallbackReceiver),
            .onWindowStatusChange = makeQtThreadWindowCallbackDelegate(&WindowCallbacks::onWindowStatusChange, sharedWindowCallbackReceiver),
            .onWindowVisibilityChange = makeQtThreadWindowCallbackDelegate(&WindowCallbacks::onWindowVisibilityChange, sharedWindowCallbackReceiver),
            .onTouchOutside = makeQtThreadWindowCallbackDelegate(&WindowCallbacks::onTouchOutside, sharedWindowCallbackReceiver),
            .onAvoidAreaChange = makeQtThreadWindowCallbackDelegate(&WindowCallbacks::onAvoidAreaChange, sharedWindowCallbackReceiver),
            .onWindowRectChange = makeCompressingQtThreadWindowCallbackDelegate(&WindowCallbacks::onWindowRectChange, sharedWindowCallbackReceiver),
            .onWindowRectChangeInGlobalDisplay = makeCompressingQtThreadWindowCallbackDelegate(&WindowCallbacks::onWindowRectChangeInGlobalDisplay, sharedWindowCallbackReceiver),
            .onWindowDisplayIdChange = makeQtThreadWindowCallbackDelegate(&WindowCallbacks::onWindowDisplayIdChange, sharedWindowCallbackReceiver),
        });
    },
    Q_FUNC_INFO);

    m_qtWindowCallbacksReceiverHandle = sharedWindowCallbackReceiver;
}

void QOhosWindowProxy::setNonClientAreaMouseWindowCallbackReceiver(
    QObject *contextObject, QOhosConsumer<std::vector<NonClientAreaMouseEvent>> mouseEventBatchConsumer)
{
    auto qtConsumer = QtOhos::moveToSharedPtr(std::move(mouseEventBatchConsumer));
    auto weakQtConsumer = QtOhos::makeWeakPtr(qtConsumer);

    auto jsConsumer = makeQtOhosSimpleBatchingQtRequestsHandler<NonClientAreaMouseEvent>(
        QtOhos::QObjectThreadSafeRef(contextObject),
        [weakQtConsumer](std::vector<NonClientAreaMouseEvent> &&batch) {
            auto qtConsumer = weakQtConsumer.lock();
            if (qtConsumer)
                (*qtConsumer)(std::move(batch));
        });

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsScopeData->nonClientAreaMouseEventConsumer = std::move(jsConsumer);
    },
    Q_FUNC_INFO);

    m_qtNonClientAreaMouseWindowCallbackReceiverHandle = qtConsumer;
}

void QOhosWindowProxy::setNonClientAreaTouchWindowCallbackReceiver(
    QObject *contextObject, QOhosConsumer<std::vector<NonClientAreaTouchEvent>> touchEventBatchConsumer)
{
    auto qtConsumer = QtOhos::moveToSharedPtr(std::move(touchEventBatchConsumer));
    auto weakQtConsumer = QtOhos::makeWeakPtr(qtConsumer);

    auto jsConsumer = makeQtOhosSimpleBatchingQtRequestsHandler<NonClientAreaTouchEvent>(
        QtOhos::QObjectThreadSafeRef(contextObject),
        [weakQtConsumer](std::vector<NonClientAreaTouchEvent> &&batch) {
            auto qtConsumer = weakQtConsumer.lock();
            if (qtConsumer)
                (*qtConsumer)(std::move(batch));
        });

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsScopeData->nonClientAreaTouchEventConsumer = std::move(jsConsumer);
    },
    Q_FUNC_INFO);

    m_qtNonClientAreaTouchWindowCallbackReceiverHandle = qtConsumer;
}

bool QOhosWindowProxy::qtIsMainWindow() const
{
    return m_windowProxyType == WindowProxyType::MainWindow;
}

void QOhosWindowProxy::raiseToAppTop()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
        if (m_jsScopeData->isWindowClosing()) {
            taskPromise();
            return;
        }
        auto promise = m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("raiseToAppTop()");
        promise.onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::showWindow(const ShowWindowOptions &options)
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
        if (m_jsScopeData->isWindowClosing()) {
            taskPromise();
            return;
        }

        std::vector<std::pair<std::string, QNapi::ValueWrapper>> jsOptionsProps;

        if (options.focusOnShow.has_value())
            jsOptionsProps.emplace_back("focusOnShow", options.focusOnShow.value());

        std::vector<QNapi::ValueWrapper> showWindowArgs;
        constexpr bool brokenHandlingOfEmptyOptionsParamInOhos = true;
        if (!(jsOptionsProps.empty() && brokenHandlingOfEmptyOptionsParamInOhos))
            showWindowArgs.push_back(QNapi::makeObject(jsState.env(), jsOptionsProps));

        auto promise = m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("showWindow(*)", showWindowArgs);
        promise.onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::recover()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;
        m_jsScopeData->jsWindowRef->eval("recover()");
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::restore()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
        if (m_jsScopeData->isWindowClosing()) {
            taskPromise();
            return;
        }
        m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("restore()").onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::minimize()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
        if (m_jsScopeData->isWindowClosing()) {
            taskPromise();
            return;
        }
        m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("minimize()").onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::maximize(MaximizePresentation maximizePresentation)
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);

    if (!qtIsMainWindow()) {
        qCWarning(QtForOhos(), "%s: Maximize is currently supported only on main windows", Q_FUNC_INFO);
        return;
    }

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &jsState) {
        if (m_jsScopeData->isWindowClosing())
            return;
        m_jsScopeData->jsWindowRef->eval("maximize(*)", {jsState.mapOhosEnumToJs(maximizePresentation)});
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowLayoutFullScreen(bool isLayoutFullScreen)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, isLayoutFullScreen ? "true" : "false");
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }
            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow(
                "setWindowLayoutFullScreen(*)", {isLayoutFullScreen})
                .onCatch(QtOhos::makeErrorLoggingJsCallback("setWindowLayoutFullScreen()"))
                .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowSystemBarEnable(const QStringList &names)
{
    qCDebug(QtForOhos) << Q_FUNC_INFO << names;
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }
            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow(
                "setWindowSystemBarEnable(*)",
                {QNapi::makeArray(jsState.env(), names, std::mem_fn(&QString::toStdString))})
                .onCatch(QtOhos::makeErrorLoggingJsCallback("setWindowSystemBarEnable()"))
                .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::showAbility()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);

    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
        if (m_jsScopeData->isWindowClosing()) {
            taskPromise();
            return;
        }

        m_jsScopeData->qAbilityPeer->qAbility().evalToPromiseOrRejectOnThrow("context.showAbility()")
        .onCatch(QtOhos::makeErrorLoggingJsCallback("showAbility()"))
        .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
    },
    Q_FUNC_INFO);
}

bool QOhosWindowProxy::tryHideAbility()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);

    return QtOhos::evalInJsThreadWithPromise<bool>([&](QtOhos::JsState &, QOhosTaskPromise<bool> evalPromise) {
        if (m_jsScopeData->isWindowClosing()) {
            evalPromise(false);
            return;
        }

        auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
        m_jsScopeData->qAbilityPeer->qAbility().evalToPromiseOrRejectOnThrow("context.hideAbility()")
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QtOhos::CallbackInfo &) {
                    thenPromise(true);
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QtOhos::CallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "got error from hideAbility()");
                    catchPromise(false);
                });
        },
        Q_FUNC_INFO);
}

bool QOhosWindowProxy::getImmersiveModeEnabledState()
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            if (m_jsScopeData->isWindowClosing())
                return false;
            return m_jsScopeData->jsWindowRef->eval<QNapi::Boolean>("getImmersiveModeEnabledState()").Value();
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowPrivacyMode(bool privacyMode)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, privacyMode ? "true" : "false");

    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
        if (m_jsScopeData->isWindowClosing()) {
            taskPromise();
            return;
        }

        m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("setWindowPrivacyMode(*)", {privacyMode})
        .onCatch(QtOhos::makeErrorLoggingJsCallback("setPrivacyMode()"))
        .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowFocusable(bool focusable)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, focusable ? "true" : "false");
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;
        m_jsScopeData->jsWindowRef->eval("setWindowFocusable(*)", {focusable});
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowTouchable(bool touchable)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, touchable ? "true" : "false");
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;
        m_jsScopeData->jsWindowRef->eval("setWindowTouchable(*)", {touchable});
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowLimits(const QSize &minSize, const QSize &maxSize)
{
    qCDebug(
        QtForOhos, "%s: (%d x %d)-(%d x %d)", Q_FUNC_INFO, minSize.width(), minSize.height(),
        maxSize.width(), maxSize.height());
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }
            auto windowLimits = QNapi::makeObject(
                jsState.env(),
                {
                    {"minWidth", minSize.width()},
                    {"minHeight", minSize.height()},
                    {"maxWidth", maxSize.width()},
                    {"maxHeight", maxSize.height()},
                });

            std::vector<QNapi::ValueWrapper> setWindowLimitsArgs = {windowLimits};
            if (QOhosDeviceInfo::is2in1()) {
                constexpr bool isForcible = true;
                setWindowLimitsArgs.push_back(isForcible);
            }
            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow(
                "setWindowLimits(*)", setWindowLimitsArgs)
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setWindowLimits()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

QOhosWindowProxy::WindowLimits QOhosWindowProxy::getWindowLimits() const
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            if (m_jsScopeData->isWindowClosing())
                return WindowLimits {};
            auto windowLimitsObject = m_jsScopeData->jsWindowRef->eval<QNapi::Object>("getWindowLimits()");
            return WindowLimits {
                .minWidth = getOptionalNumberPropAsOptionalDouble(windowLimitsObject, "minWidth"),
                .minHeight = getOptionalNumberPropAsOptionalDouble(windowLimitsObject, "minHeight"),
                .maxWidth = getOptionalNumberPropAsOptionalDouble(windowLimitsObject, "maxWidth"),
                .maxHeight = getOptionalNumberPropAsOptionalDouble(windowLimitsObject, "maxHeight"),
            };
        },
        Q_FUNC_INFO);
}

QOhosWindowProxy::AvoidArea QOhosWindowProxy::getWindowAvoidArea(AvoidAreaType avoidAreaType) const
{
    qCDebug(QtForOhos, "%s: %d", Q_FUNC_INFO, avoidAreaType);
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &jsState) {
            if (m_jsScopeData->isWindowClosing())
                return AvoidArea {};
            auto avoidAreaObject = m_jsScopeData->jsWindowRef->eval<QNapi::Object>(
                "getWindowAvoidArea(*)", {jsState.mapOhosEnumToJs(avoidAreaType)});
            return mapAvoidAreaFromJs(avoidAreaObject);
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowMask(
    const WindowMask &windowMask, const std::optional<QSize> &ohosMaskSizeOverride)
{
    if (qtIsMainWindow())
        return;

    auto ohosMaskSize = ohosMaskSizeOverride.has_value()
        ? ohosMaskSizeOverride.value()
        : getWindowProperties().windowRect.size();

    if (ohosMaskSize.isEmpty()) {
        const auto *maskSrcSizeMsg = ohosMaskSizeOverride.has_value()
            ? "overridden mask source"
            : "window";
        if (ohosMaskSizeOverride.has_value()) {
            qOhosPrintfError(
                "%s failed - %s size is 0x0", maskSrcSizeMsg,
                "QOhosWindowProxy::setWindowMask");
        }
        return;
    }

    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
        if (m_jsScopeData->isWindowClosing()) {
            taskPromise();
            return;
        }
        auto *env = jsState.env();

        QNapi::Array maskRowsArray = QNapi::Array::New(env, ohosMaskSize.height());
        const int defaultValue = windowMask.windowMaskRegion.isEmpty() ? 1 : 0;

        for (int rowIndex = 0; rowIndex < ohosMaskSize.height(); ++rowIndex) {
            auto arr = QNapi::Array::New(env, ohosMaskSize.width());
            arr.fill(defaultValue);
            maskRowsArray[rowIndex] = arr;
        }

        auto valueToSet = QNapi::Number::New(env, 1);
        for (const auto &rect: windowMask.windowMaskRegion) {
            auto top = qBound(0, rect.top(), ohosMaskSize.height() - 1);
            auto bottom = qBound(0, rect.bottom(), ohosMaskSize.height() - 1);
            auto left = qBound(0, rect.left(), ohosMaskSize.width() - 1);
            auto right = qBound(0, rect.right(), ohosMaskSize.width() - 1);

            for (auto rowIndex = top; rowIndex <= bottom; ++rowIndex) {
                auto row = maskRowsArray.Get(rowIndex).As<QNapi::Array>();
                for (auto columnIndex = left; columnIndex <= right; ++columnIndex)
                    row[columnIndex] = valueToSet;
            }
        }

        m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("setWindowMask(*)", {maskRowsArray})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setWindowMask()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::setSubWindowModalDisabled()
{
    if (qtIsMainWindow())
        return;

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }
            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("setSubWindowModal(*)", {false})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setSubWindowModal()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setSubWindowModalEnabled(ModalityType modalityType)
{
    if (qtIsMainWindow())
        return;

    if (modalityType == ModalityType::APPLICATION_MODALITY && !QOhosSettings::instance().isWindowPcModeEnabled()) {
        qOhosPrintfWarning(
            "%s: APPLICATION_MODALITY option can be used only on devices in the freeform window state - skipping",
            Q_FUNC_INFO);
        return;
    }

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }
            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow(
                "setSubWindowModal(*)", {true, jsState.mapOhosEnumToJs(modalityType)})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setSubWindowModal()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setTitle(const QString &title)
{
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }

            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("setWindowTitle(*)", {title.toStdString()})
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowTitleButtonVisible(bool maximizeVisible, bool minimizeVisible, bool closeVisible)
{
    qOhosPrintfDebug(
        "%s: isMaximizeVisible:%s, isMinimizeVisible:%s, isCloseVisible:%s",
        Q_FUNC_INFO, maximizeVisible ? "true" : "false", minimizeVisible ? "true": "false",
        closeVisible ? "true" : "false");

    if (!qtIsMainWindow())
        return;

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &jsState) {
            if (m_jsScopeData->isWindowClosing())
                return;

            constexpr auto capabilityNotSupportedErrorCode = 801;
            QtOhos::runIgnoringJsBusinessError(
                jsState, capabilityNotSupportedErrorCode, "setWindowTitleButtonVisible()",
                [&]() {
                    m_jsScopeData->jsWindowRef->eval(
                        "setWindowTitleButtonVisible(*)",
                        {maximizeVisible, minimizeVisible, closeVisible});
                });
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowTopmost(bool topmost)
{
    if (!qtIsMainWindow())
        return;

    if (!QOhosSettings::instance().isWindowPcModeEnabled()) {
        qOhosPrintfWarning("%s: can be used only on 2-in-1 devices or tablets in PC mode - skipping", Q_FUNC_INFO);
        return;
    }

    qOhosPrintfDebug("%s: topMost: %s", Q_FUNC_INFO, topmost ? "true" : "false");

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }

            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("setWindowTopmost(*)", {topmost})
                .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowDecorVisible(bool visible)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;

        m_jsScopeData->jsWindowRef->eval("setWindowDecorVisible(*)", {visible});
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowTitleMoveEnabled(bool enabled)
{
    if (!QOhosSettings::instance().isWindowPcModeEnabled()) {
        qOhosPrintfWarning("%s: can be used only on 2-in-1 devices or tablets in PC mode - skipping", Q_FUNC_INFO);
        return;
    }

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;

        m_jsScopeData->jsWindowRef->eval("setWindowTitleMoveEnabled(*)", {enabled});
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowShadowRadius(double radius)
{
    if (!(QOhosDeviceInfo::is2in1() || QOhosDeviceInfo::isTablet())) {
        qOhosPrintfWarning("%s: can be used only on 2-in-1 devices or tablets - skipping", Q_FUNC_INFO);
        return;
    }

    if (qtIsMainWindow())
        return;

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsScopeData->jsWindowRef->eval("setWindowShadowRadius(*)", {radius});
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowCornerRadius(double radius)
{
    if (qtIsMainWindow())
        return;

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("setWindowCornerRadius(*)", {radius})
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

bool QOhosWindowProxy::isWindowRectAutoSave() const
{
    return QtOhos::evalInJsThreadWithPromise<bool>(
        [&](QtOhos::JsState &, QOhosTaskPromise<bool> evalPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                evalPromise(false);
                return;
            }
            auto optQUiAbilityPeer = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_jsScopeData->qAbilityPeer);
            if (!optQUiAbilityPeer) {
                evalPromise(false);
                return;
            }

            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            optQUiAbilityPeer->windowStage()
            .evalToPromiseOrRejectOnThrow("isWindowRectAutoSave()")
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QtOhos::CallbackInfo &cbInfo) {
                    bool windowRectAutoSaveEnabled = cbInfo.getFirstArg<QNapi::Boolean>(Q_FUNC_INFO);
                    thenPromise(windowRectAutoSaveEnabled);
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QtOhos::CallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "isWindowRectAutoSave()");
                    catchPromise(false);
                });
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setFollowParentMultiScreenPolicy(bool enabled)
{
    if (qtIsMainWindow())
        return;

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }
            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("setFollowParentMultiScreenPolicy(*)", {enabled})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setFollowParentMultiScreenPolicy()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowKeepScreenOn(bool keepScreenOn)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, QtOhos::mapBoolToTrueFalseStr(keepScreenOn));

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }

            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("setWindowKeepScreenOn(*)", {keepScreenOn})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setWindowKeepScreenOn()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setSupportedWindowModes(const std::set<SupportWindowMode> &supportedWindowModes)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, mapEnumsToLogString(supportedWindowModes).c_str());

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }
            auto qUiAbilityPeer = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_jsScopeData->qAbilityPeer);
            if (!qUiAbilityPeer) {
                taskPromise();
                return;
            }

            auto jsSupportedWindowModes = QNapi::makeArray(
                jsState.env(), supportedWindowModes,
                [&](auto mode) {
                    return jsState.mapOhosEnumToJs(mode);
                });

            qUiAbilityPeer->windowStage().evalToPromiseOrRejectOnThrow("setSupportedWindowModes(*)", {jsSupportedWindowModes})
                .onCatch(QtOhos::makeErrorLoggingJsCallback("setSupportedWindowModes()"))
                .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setWindowRectAutoSave(bool enabled)
{
    constexpr bool isSaveBySpecifiedFlag = true;

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }
            auto optQUiAbilityPeer = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_jsScopeData->qAbilityPeer);
            if (!optQUiAbilityPeer) {
                taskPromise();
                return;
            }

            optQUiAbilityPeer->windowStage()
            .evalToPromiseOrRejectOnThrow("setWindowRectAutoSave(*)", {enabled, isSaveBySpecifiedFlag})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setWindowRectAutoSave()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

void QOhosWindowProxy::setSubWindowCloseHandler(
    std::function<void()> handler, bool handlerReturnValue)
{
    m_subWindowCloseRegistrationHandle =
        registerSubWindowCloseHandler(std::move(handler), handlerReturnValue);
}

void QOhosWindowProxy::resetSubWindowCloseHandler()
{
    m_subWindowCloseRegistrationHandle.reset();
}

std::shared_ptr<void> QOhosWindowProxy::registerSubWindowCloseHandler(
    std::function<void()> handler, bool handlerReturnValue)
{
    auto sharedHandler = QtOhos::moveToSharedPtr(std::move(handler));
    auto jsWindowRegistrationHandle = QtOhos::evalInJsThread(
        [&](QtOhos::JsState &jsState) {
            return QtOhos::makeProxyWithJsThreadDeleter(
                m_jsScopeData->registerSubWindowCloseHandler(
                    jsState,
                    [weakHandler = QtOhos::makeWeakPtr(sharedHandler), handlerReturnValue]() {
                        QtOhos::invokeInQtThread(
                            [weakHandler]() {
                                auto sharedHandler = weakHandler.lock();
                                if (sharedHandler)
                                    (*sharedHandler)();
                            });
                        return handlerReturnValue;
                    }));
        },
        Q_FUNC_INFO);

    return QtOhos::moveToSharedPtr(
        std::make_tuple(sharedHandler, jsWindowRegistrationHandle));
}

std::shared_ptr<QOhosWindowProxy>
QOhosWindowProxy::createForExistingMainWindow(const ExistingMainWindowCreateInfo &createInfo)
{
    return QtOhos::evalInJsThreadWithPromise<std::shared_ptr<QOhosWindowProxy>>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<std::shared_ptr<QOhosWindowProxy>> evalPromise) {
            auto sharedEvalPromise = QtOhos::moveToSharedPtr(std::move(evalPromise).makeChained(Q_FUNC_INFO));
            makeWindowProxyDataForExistingMainWindowInJsThread(
                jsState,
                createInfo,
                [sharedEvalPromise](QtOhos::JsState &jsState, QOhosWindowProxyData windowProxyData) {
                    (*sharedEvalPromise)(QOhosWindowProxy::create(jsState, std::move(windowProxyData)));
                });
        },
        Q_FUNC_INFO);
}

std::shared_ptr<QOhosWindowProxy>
QOhosWindowProxy::createFloatWindow(const FloatWindowCreateInfo &createInfo)
{
    return QtOhos::evalInJsThreadWithPromise<std::shared_ptr<QOhosWindowProxy>>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<std::shared_ptr<QOhosWindowProxy>> evalPromise) {
            auto sharedEvalPromise = QtOhos::moveToSharedPtr(std::move(evalPromise).makeChained(Q_FUNC_INFO));
            makeWindowProxyDataForFloatWindowInJsThread(
                jsState, createInfo,
                [sharedEvalPromise](QtOhos::JsState &jsState, QOhosWindowProxyData windowProxyData) {
                    (*sharedEvalPromise)(QOhosWindowProxy::create(jsState, std::move(windowProxyData)));
                });
        },
        Q_FUNC_INFO);
}

std::shared_ptr<QOhosWindowProxy>
QOhosWindowProxy::createMainWindow(const MainWindowCreateInfo &createInfo)
{
    return QtOhos::evalInJsThreadWithPromise<std::shared_ptr<QOhosWindowProxy>>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<std::shared_ptr<QOhosWindowProxy>> evalPromise) {
            auto sharedEvalPromise = QtOhos::moveToSharedPtr(std::move(evalPromise).makeChained(Q_FUNC_INFO));
            makeWindowProxyDataForMainWindowInJsThread(
                jsState,
                createInfo,
                [sharedEvalPromise](QtOhos::JsState &jsState, QOhosWindowProxyData windowProxyData) {
                    (*sharedEvalPromise)(QOhosWindowProxy::create(jsState, std::move(windowProxyData)));
                });
        },
        Q_FUNC_INFO);
}

std::shared_ptr<QXComponentNode> QOhosWindowProxy::nodeXComponent() const
{
    return m_nodeXComponent;
}

std::string QOhosWindowProxy::qAbilityInstanceId() const
{
    return m_qAbilityInstanceId;
}

std::shared_ptr<QOhosWindowProxy>
QOhosWindowProxy::createSubWindow(const SubWindowCreateInfo &createInfo)
{
    return QtOhos::evalInJsThreadWithPromise<std::shared_ptr<QOhosWindowProxy>>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<std::shared_ptr<QOhosWindowProxy>> evalPromise) {
            auto sharedEvalPromise = QtOhos::moveToSharedPtr(std::move(evalPromise).makeChained(Q_FUNC_INFO));
            auto proxyDataConsumer =
                [sharedEvalPromise](QtOhos::JsState &jsState, QOhosWindowProxyData windowProxyData) {
                    (*sharedEvalPromise)(QOhosWindowProxy::create(jsState, std::move(windowProxyData)));
                };
            // HACK - calling createSubWindow from window may throw while the context is termination
            // This is only a problem because we currenlty do not properly handle main window closing
            // Remove this hacky branch after QTFOROH-1080 issues are resolved.
            if (m_jsScopeData->isWindowClosing()) {
                makeWindowProxyDataForSubWindowInJsThread(
                    jsState, createInfo, std::move(proxyDataConsumer));
            } else {
                makeWindowProxyDataForSubWindowInJsThread(
                    jsState, m_jsScopeData->jsWindowRef->jsObject(), createInfo,
                    std::move(proxyDataConsumer));
            }
        },
        Q_FUNC_INFO);
}

QOhosWindowProxy::JsScopeData::JsScopeData(
    WindowProxyType windowProxyType, QNapi::Reference<QNapi::Object> jsWindow,
    std::shared_ptr<void> optKeepAliveData,
    std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer,
    QtOhos::QObjectThreadSafeRef owningQWindowRef)
    : windowProxyType(windowProxyType)
    , windowCallbackReceiver(nullptr)
    , windowDestroyedFromSystem(false)
    , optKeepAliveData(optKeepAliveData)
    , qAbilityPeer(qAbilityPeer)
    , m_windowFrameMouseFilterHandle(
        QArkUi::registerMouseEventsConsumer(
            getWindowPropertiesFromJsWindow(jsWindow.Value()).id,
            [this](const QArkUi::MouseEvent &event) {
                onMouseEventFromArkUi(event);
            }))
    , m_windowFrameTouchFilterHandle(
        QArkUi::registerTouchEventsConsumer(
            getWindowPropertiesFromJsWindow(jsWindow.Value()).id,
            [this](const QArkUi::TouchEvent &event) {
                onTouchEventFromArkUi(event);
            }))
    , jsWindowRef(
        std::make_shared<QArkUi::JsWindowRef>(
            qAbilityPeer->instanceId(),
            getWindowPropertiesFromJsWindow(jsWindow.Value()).id,
            jsWindow.Value(),
            owningQWindowRef))
{
}

QOhosWindowProxy::JsScopeData::~JsScopeData()
{
    if (isWindowClosingFromSystem(jsWindowRef->jsObject(), windowProxyType, qAbilityPeer)) {
        windowDestroyedFromSystem = true;
        return;
    }

    QtOhos::JsWindowsTracker::tagWindowAsClosing(jsWindowRef->jsObject(), "QOhosWindowProxy::JsScopeData destructor");

    if (windowProxyType == WindowProxyType::MainWindow) {
        // NOTE - Set the windowDestroyedFromSystem flag here early
        // to avoid callbacks being invoked directly as a result of
        // calling terminate
        windowDestroyedFromSystem = true;
        qOhosPrintfWarning(
            "Attempting to terminate qAbility with instance id: %s",
            qAbilityPeer->instanceId().c_str());
        qAbilityPeer->qAbility().eval("context.terminateSelf()");
    } else if (!windowDestroyedFromSystem) {
        // FIXME - destroyWindow usually does and returns nothing
        // once the actual implementation is provided wait for the proomise that this function should return
        jsWindowRef->eval("destroyWindow()");
    }
}

std::shared_ptr<void> QOhosWindowProxy::JsScopeData::registerEventListener(
    const std::string &eventName,
    void (QOhosWindowProxy::JsScopeData::*handleFunction)(const QtOhos::CallbackInfo &),
    QFlags<EventHandlerFlagBits> eventHandlerFlags)
{

    std::weak_ptr<JsScopeData> weakSelf = shared_from_this();
    bool ignoreWhenAbilityIsTerminating = !eventHandlerFlags.testFlag(EventHandlerFlagBits::allowCallWhenAbilityIsTerminating);

    return registerQOhosOnOffMethodsBasedEventHandler(
        jsWindowRef->jsObject(), eventName,
        [weakSelf, handleFunction, eventName, ignoreWhenAbilityIsTerminating](const QtOhos::CallbackInfo &cbInfo) {
            auto self = weakSelf.lock();
            if (Q_UNLIKELY(!self)) {
                qOhosPrintfWarning(
                    "callback '%s' called for destroyed QOhosWindowProxy::JsScopeData, ignoring",
                    eventName.c_str());
                return;
            }

            if (self->isWindowClosing() && ignoreWhenAbilityIsTerminating) {
                qOhosPrintfError(
                    "QOhosWindowProxy: Received callback for event '%s' during termination of the related QAbility.",
                    eventName.c_str());
                return;
            }

            if (Q_UNLIKELY(self->windowDestroyedFromSystem)) {
                qOhosPrintfError(
                    "QOhosWindowProxy: Received callback for event '%s' after WINDOW_DESTROYED",
                    eventName.c_str());
                return;
            }

            ((*self).*handleFunction)(cbInfo);
        },
        {
            .optEventSourceAliveCheckFunc = {},
            .extraOnArg = {},
            .extraOffArg = {},
            .optOnCallExceptionHandler = [&](const Napi::Error &error) {
                constexpr std::uint32_t capabilityNotSupportedErrorCode = 801;
                constexpr std::uint32_t windowStateIsAbnormalErrorCode = 1300002;

                const QSet<std::uint32_t> ignorableErrorCodes = {
                    capabilityNotSupportedErrorCode,
                    windowStateIsAbnormalErrorCode,
                };

                auto errorCode = QtOhos::tryGetCodeFromJsBusinessError(error);

                auto ignorableError =
                    eventHandlerFlags.testFlag(EventHandlerFlagBits::allowEventHandlerRegistrationFailure)
                    && errorCode.has_value()
                    && ignorableErrorCodes.contains(errorCode.value());

                if (!ignorableError)
                    throw;

                qOhosPrintfWarning(
                    "%s: Ignored error %u while registering for window event '%s'",
                    Q_FUNC_INFO, errorCode.value(), eventName.c_str());
            },
        });
}

std::shared_ptr<void> QOhosWindowProxy::JsScopeData::registerSubWindowCloseHandler(
    QtOhos::JsState &, std::function<bool()> handler)
{
    auto weakSelf = QtOhos::makeWeakPtr(shared_from_this());
    return registerQOhosOnOffMethodsBasedEventHandler(
        jsWindowRef->jsObject(), "subWindowClose",
        [weakSelf, handler = std::move(handler)](const QtOhos::CallbackInfo &cbInfo) {
            bool deferClose = handler();
            auto self = weakSelf.lock();
            if (self && !deferClose)
                QtOhos::JsWindowsTracker::tagWindowAsClosing(self->jsWindowRef->jsObject(), "subWindowClose => false");
            return QNapi::Boolean::New(cbInfo.Env(), deferClose);
        });
}

void QOhosWindowProxy::JsScopeData::handleWindowEventCallback(const QtOhos::CallbackInfo &cbInfo)
{
    auto eventType = cbInfo.getFirstArg<QNapi::Number>(Q_FUNC_INFO);
    // NOTE - All windowEvents should be handled by qt but currently
    // some are not exposed as a part of public api.
    WindowEvent event;
    try {
        event.type = cbInfo.jsState().mapOhosEnumFromJs<WindowEventType>(eventType);
    } catch (const Napi::Error &err) {
        qOhosPrintfError(
            "Error converting WindowEventType to known value: %s. Event will be ignored.", err.what());
        return;
    }

    if (isWindowClosing() && event.type != WindowEventType::WINDOW_DESTROYED) {
        qOhosPrintfError(
            "Received WindowEvent for window when it's closing. WindowEventType: %d",
            event.type);
        return;
    }

    onWindowEvent(cbInfo.jsState(), event);
}

void QOhosWindowProxy::JsScopeData::handleWindowStatusCallback(const QtOhos::CallbackInfo &cbInfo)
{
    auto windowStatusType = cbInfo.getFirstArg<QNapi::Number>(Q_FUNC_INFO);
    if (windowCallbackReceiver != nullptr) {
        windowCallbackReceiver->onWindowStatusChange(
            WindowStatus {
                .type = cbInfo.jsState().mapOhosEnumFromJs<WindowStatusType>(windowStatusType),
            });
    }
}

void QOhosWindowProxy::JsScopeData::handleWindowVisibilityCallback(const QtOhos::CallbackInfo &cbInfo)
{
    auto windowVisibility = cbInfo.getFirstArg<QNapi::Boolean>(Q_FUNC_INFO);
    if (windowCallbackReceiver != nullptr)
        windowCallbackReceiver->onWindowVisibilityChange(windowVisibility);
}

void QOhosWindowProxy::JsScopeData::handleWindowTouchOutsideCallback(const QtOhos::CallbackInfo &)
{
    if (windowCallbackReceiver != nullptr)
        windowCallbackReceiver->onTouchOutside();
}

void QOhosWindowProxy::JsScopeData::handleAvoidAreaChangeCallback(const QtOhos::CallbackInfo &cbInfo)
{
    if (windowCallbackReceiver != nullptr) {
        auto callbackArg = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
        windowCallbackReceiver->onAvoidAreaChange(
            cbInfo.jsState().mapOhosEnumFromJs<AvoidAreaType>(callbackArg.get<QNapi::Number>("type")),
            mapAvoidAreaFromJs(callbackArg.get<QNapi::Object>("area")));
    }
}

void QOhosWindowProxy::JsScopeData::handleWindowRectChangeCallback(const QtOhos::CallbackInfo &cbInfo)
{
    auto rectChangeOptionsObjectArg = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
    auto rectChangeOptions = RectChangeOptions {
        .rect = ohosWindowRectToQRect(rectChangeOptionsObjectArg.get<QNapi::Object>("rect")),
        .reason = cbInfo.jsState().mapOhosEnumFromJs<RectChangeReason>(rectChangeOptionsObjectArg.get<QNapi::Number>("reason")),
    };

    if (windowCallbackReceiver != nullptr)
        windowCallbackReceiver->onWindowRectChange(rectChangeOptions);
}

void QOhosWindowProxy::JsScopeData::handleWindowRectChangeInGlobalDisplayCallback(const QtOhos::CallbackInfo &cbInfo)
{
    auto rectChangeOptionsObjectArg = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
    auto rectChangeOptions = RectChangeOptions {
        .rect = ohosWindowRectToQRect(rectChangeOptionsObjectArg.get<QNapi::Object>("rect")),
        .reason = cbInfo.jsState().mapOhosEnumFromJs<RectChangeReason>(rectChangeOptionsObjectArg.get<QNapi::Number>("reason")),
    };

    if (windowCallbackReceiver != nullptr)
        windowCallbackReceiver->onWindowRectChangeInGlobalDisplay(rectChangeOptions);
}

void QOhosWindowProxy::JsScopeData::handleWindowDisplayIdChangeCallback(const QtOhos::CallbackInfo &cbInfo)
{
    auto displayIdNumber = cbInfo.getFirstArg<QNapi::Number>(Q_FUNC_INFO);
    auto displayId = QOhosDisplayInfo::JsDisplayId(displayIdNumber);

    if (windowCallbackReceiver != nullptr)
        windowCallbackReceiver->onWindowDisplayIdChange(displayId);
}

void QOhosWindowProxy::JsScopeData::onWindowEvent(QtOhos::JsState &, const WindowEvent &windowEvent)
{
    if (windowEvent.type == WindowEventType::WINDOW_DESTROYED) {
        QtOhos::JsWindowsTracker::tagWindowAsClosing(jsWindowRef->jsObject(), "WINDOW_DESTROYED");
        windowDestroyedFromSystem = true;
    }

    if (windowCallbackReceiver != nullptr)
        windowCallbackReceiver->onWindowEvent(windowEvent);
}

bool QOhosWindowProxy::JsScopeData::isWindowClosing() const
{
    return isWindowClosingFromSystem(jsWindowRef->jsObject(), windowProxyType, qAbilityPeer);
}

void QOhosWindowProxy::JsScopeData::onMouseEventFromArkUi(const QArkUi::MouseEvent &event)
{
    if (nonClientAreaMouseEventConsumer == nullptr)
        return;

    auto optAction = tryMapMouseEventActionToNonClientAreaEventType(event.action);
    if (!optAction.has_value())
        return;

    auto optWindowProperties = QArkUi::tryGetWindowProperties(event.jsWindowId);
    if (!optWindowProperties.has_value()) {
        qOhosPrintfError(
            "%s: Failed to retrieve window properties for js window: %f. Ignoring event.",
            Q_FUNC_INFO, event.jsWindowId.value());
        return;
    }

    const auto &windowProperties = optWindowProperties.value();
    if (!isPointInNonClientArea(event.displayPosition, windowProperties))
        return;

    auto windowOrigin = windowProperties.windowRect.topLeft() + windowProperties.drawableRect.topLeft();
    NonClientAreaMouseEvent nonClientAreaMouseEvent = {
        .timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(event.actionTime),
        .action = optAction.value(),
        .button = tryMapMouseEventButtonToQt(event.button).value_or(Qt::NoButton),
        .displayPosition = event.displayPosition,
        .localPosition = event.displayPosition - windowOrigin,
        .globalPosition = event.globalPosition,
    };

    nonClientAreaMouseEventConsumer(nonClientAreaMouseEvent);
}

void QOhosWindowProxy::JsScopeData::onTouchEventFromArkUi(const QArkUi::TouchEvent &event)
{
    if (nonClientAreaTouchEventConsumer == nullptr)
        return;

    auto optAction = tryMapTouchEventActionToNonClientAreaEventType(event.action);
    if (!optAction.has_value())
        return;

    auto optWindowProperties = QArkUi::tryGetWindowProperties(event.jsWindowId);
    if (!optWindowProperties.has_value()) {
        qOhosPrintfError(
            "%s: Failed to retrieve window properties for js window: %f. Ignoring event.",
            Q_FUNC_INFO, event.jsWindowId.value());
        return;
    }

    const auto &windowProperties = optWindowProperties.value();
    if (!isPointInNonClientArea(event.displayPosition, windowProperties))
        return;

    auto windowOrigin = windowProperties.windowRect.topLeft() + windowProperties.drawableRect.topLeft();
    NonClientAreaTouchEvent nonClientAreaTouchEvent = {
        .id = event.fingerId,
        .timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(event.actionTime),
        .action = optAction.value(),
        .displayPosition = event.displayPosition,
        .localPosition = event.displayPosition - windowOrigin,
        .globalPosition = event.globalPosition,
    };

    nonClientAreaTouchEventConsumer(nonClientAreaTouchEvent);
}

QPixmap QOhosWindowProxy::snapshot() const
{
    return QtOhos::evalInJsThreadWithPromise<QPixmap>(
        [&](QtOhos::JsState &, QOhosTaskPromise<QPixmap> evalPromise) {
            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("snapshot()")
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QtOhos::CallbackInfo &cbInfo) {
                    auto napiPixmap = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

                    ::OH_PixelmapNative *pixelMapNativePtr;
                    QArkUi::callArkUiOrFailOnErrorResult(
                        Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_ConvertPixelmapNativeFromNapi),
                        cbInfo.Env(), napiPixmap, &pixelMapNativePtr);
                    auto pixelMap = wrapOhosNativePixelMapPtr(pixelMapNativePtr);

                    thenPromise(
                        QPixmap::fromImage(createQImageFromNativePixelMap(pixelMap.get())));
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QtOhos::CallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "Got error from snapshot()");
                    catchPromise(QPixmap());
                });
        },
        Q_FUNC_INFO);
}

QList<QRect> QOhosWindowProxy::getDisplayCutoutRects() const
{
    // The display cutout (e.g. camera hole) is not in getWindowAvoidArea(); read
    // it from the display. Returns the bounding rects in display coordinates.
    return QtOhos::evalInJsThreadWithPromise<QList<QRect>>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<QList<QRect>> evalPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                evalPromise(QList<QRect>());
                return;
            }

            auto optWindowDisplay = qAndThen(
                getWindowPropertiesFromJsWindow(
                    m_jsScopeData->jsWindowRef->jsObject()).displayId,
                [&](QOhosDisplayInfo::JsDisplayId id) {
                    return QOhosDisplayInfo::tryGetDisplayById(jsState, id);
                });
            auto display = optWindowDisplay.has_value()
                ? optWindowDisplay.value()
                : jsState.eval<QNapi::Object>("@ohos.display.getDefaultDisplaySync()");

            auto branches = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            display
                .evalToPromiseOrRejectOnThrow("getCutoutInfo()")
                .onThen(
                    [thenPromise = std::move(branches.first)](const QtOhos::CallbackInfo &cbInfo) {
                        auto cutoutInfo = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                        auto cutoutRects = QNapi::getArrayElements<QList<QRect>, QNapi::Object>(
                            cutoutInfo.get<QNapi::Array>("boundingRects"), [](QNapi::Object r) {
                                return QRect(r.get<QNapi::Number>("left"),
                                             r.get<QNapi::Number>("top"),
                                             r.get<QNapi::Number>("width"),
                                             r.get<QNapi::Number>("height"));
                            });
                        thenPromise(cutoutRects);
                    })
                .onCatch(
                    [catchPromise = std::move(branches.second)](const QtOhos::CallbackInfo &cbInfo) {
                        QtOhos::logJsCallbackError(cbInfo, "getCutoutInfo() failed");
                        catchPromise(QList<QRect>());
                    });
        },
        Q_FUNC_INFO);
}

bool QOhosWindowProxy::startMoving()
{
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }

            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("startMoving()")
            .onCatch(QtOhos::makeErrorLoggingJsCallback("startMoving()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);

    return true;
}

void QOhosWindowProxy::enableDrag(bool enable)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, QtOhos::mapBoolToTrueFalseStr(enable));

    if (qtIsMainWindow()) {
        qCWarning(QtForOhos(), "%s: enableDrag is not supported on main windows", Q_FUNC_INFO);
        return;
    }

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }
            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow("enableDrag(*)", {enable})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("enableDrag()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

std::optional<bool> QOhosWindowProxy::isFocused() const
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) -> std::optional<bool> {
            if (m_jsScopeData->isWindowClosing())
                return {};

            return m_jsScopeData->jsWindowRef->eval<QNapi::Boolean>("isFocused()");
        },
        Q_FUNC_INFO);
}

std::vector<QArkUi::JsWindowId> QOhosWindowProxy::queryWindowIdsByCoordinate(
    QOhosDisplayInfo::JsDisplayId displayId, const QPoint &queryLocation, std::uint32_t queryLimit)
{
    return QtOhos::evalInJsThreadWithPromise<std::vector<QArkUi::JsWindowId>>(
        [&](QtOhos::JsState &jsState, auto evalPromise) {
            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            jsState.evalToPromiseOrRejectOnThrow("@ohos.window.getWindowsByCoordinate(*)", {
                displayId.value(),
                queryLimit,
                queryLocation.x(),
                queryLocation.y()
            })
            .onThen([thenPromise = std::move(thenCatchPromises.first)](const QtOhos::CallbackInfo &cbInfo) {
                auto windowsArray = cbInfo.getFirstArg<QNapi::Array>(Q_FUNC_INFO);
                thenPromise(
                    QNapi::getArrayElements<std::vector<QArkUi::JsWindowId>, QNapi::Object>(
                        windowsArray,
                        [&](QNapi::Object jsWindow) {
                            return getWindowPropertiesFromJsWindow(jsWindow).id;
                        }));
            })
            .onCatch([catchPromise = std::move(thenCatchPromises.second)](const QtOhos::CallbackInfo &cbInfo) {
                QtOhos::logJsCallbackError(
                    cbInfo, "got error from @ohos.window.getWindowsByCoordinate()");
                catchPromise({});
            });
        },
        Q_FUNC_INFO);
}

std::vector<QArkUi::JsWindowId> QOhosWindowProxy::queryQtManagedWindowIdsByPredicate(
    const std::function<bool(QtOhos::JsState &, const QArkUi::JsWindowRef &)> &predicate)
{
    return QtOhos::evalInJsThread([&](QtOhos::JsState &jsState) {
        auto &jsWindowRegistry = jsState.getAttachedObjectWithLazyCreate<QOhosJsWindowRegistry>();
        return jsWindowRegistry.queryByPredicate(jsState, predicate);
    },
    Q_FUNC_INFO);
}

void QOhosWindowProxy::moveWindowToGlobal(
    const QPoint &position, const MoveConfiguration &moveConfiguration)
{
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }

            auto moveConfigurationObject = toNapiObject(jsState.env(), moveConfiguration);
            auto moveConfigurationObjectStr = QNapi::toJsonString(moveConfigurationObject);
            qOhosPrintfDebug(
                "%s: %d,%d,%s",
                Q_FUNC_INFO, position.x(), position.y(),
                moveConfigurationObjectStr.c_str());

            m_jsScopeData->jsWindowRef->evalToPromiseOrRejectOnThrow(
                "moveWindowToGlobal(*)", {position.x(), position.y(), moveConfigurationObject})
                .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

std::optional<QOhosDisplayInfo::JsDisplayId> QOhosWindowProxy::tryGetMainWindowJsDisplayId() const
{
    return qtIsMainWindow()
        ? getWindowProperties().displayId
        : QtOhos::evalInJsThread(
            [&](QtOhos::JsState &) {
                auto qUiAbilityPeer
                    = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_jsScopeData->qAbilityPeer);
                return qUiAbilityPeer
                    ? getWindowPropertiesFromJsWindow(qUiAbilityPeer->window()).displayId
                    : std::nullopt;
            },
            Q_FUNC_INFO);
}

void QOhosWindowProxy::shiftAppWindowFocus(QOhosWindowProxy &targetProxy)
{
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isWindowClosing() || targetProxy.m_jsScopeData->isWindowClosing()) {
                taskPromise();
                return;
            }

            auto srcWindowId = getWindowPropertiesFromJsWindow(m_jsScopeData->jsWindowRef->jsObject()).id;
            auto targetWindowId = getWindowPropertiesFromJsWindow(targetProxy.m_jsScopeData->jsWindowRef->jsObject()).id;
            jsState.evalToPromiseOrRejectOnThrow(
                "@ohos.window.shiftAppWindowFocus(*)", {srcWindowId.value(), targetWindowId.value()})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("@ohos.window.shiftAppWindowFocus()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

std::shared_ptr<QOhosWindowProxy> QOhosWindowProxy::create(QtOhos::JsState &jsState, QOhosWindowProxyData data)
{
    auto window = std::shared_ptr<QOhosWindowProxy>(new QOhosWindowProxy(std::move(data)));
    auto &jsWindowRegistry = jsState.getAttachedObjectWithLazyCreate<QOhosJsWindowRegistry>();
    return QtOhos::makeSharedPtrWithAttachedExtraData(
        window,
        jsWindowRegistry.registerJsWindow(window->m_jsScopeData->jsWindowRef));
}

QT_END_NAMESPACE
