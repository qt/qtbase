// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qohoswindowproxy.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSet>
#include <QtCore/private/qnapi_p.h>
#include <qohosjsenv_p.h>
#include <QtCore/qscopeguard.h>
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
#include <qohosimageformat.h>
#include <qohosjsutils.h>
#include <qohospixelmapconversions.h>
#include <qohosplugincore.h>
#include <qohospointerstyle.h>
#include <qohossettings.h>
#include <qohosutils.h>
#include <render/qohosbatchingrequestshandler.h>
#include <render/qohosjswindowregistry.h>
#include <render/qohoswindowproxydatafactory.h>
#include <render/qxcomponent.h>
#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE

namespace
{

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

QOhosOptional<double> getOptionalNumberPropAsOptionalDouble(const QNapi::Object &object, const std::string &propertyName)
{
    auto propOrEmpty = QNapi::getOptionalPropOrEmpty<QNapi::Number>(object, propertyName);
    return !propOrEmpty.IsEmpty()
        ? QOhosOptional<double>(propOrEmpty)
        : makeEmptyQOhosOptional();
}

QArkUi::WindowProperties getWindowPropertiesFromJsWindow(QNapi::Object jsWindow)
{
    auto windowPropsObj = jsWindow.call<QNapi::Object>("getWindowProperties", {});
    auto displayIdOrEmpty = QNapi::getOptionalPropOrEmpty<QNapi::Number>(windowPropsObj, "displayId");
    return QArkUi::WindowProperties {
        .windowRect = ohosWindowRectToQRect(windowPropsObj.get<QNapi::Object>("windowRect")),
        .drawableRect = ohosWindowRectToQRect(windowPropsObj.get<QNapi::Object>("drawableRect")),
        .id = QArkUi::JsWindowId(windowPropsObj.get<QNapi::Number>("id")),
        .displayId = !displayIdOrEmpty.IsEmpty()
            ? makeQOhosOptional(QOhosDisplayInfo::JsDisplayId{displayIdOrEmpty.DoubleValue()})
            : makeEmptyQOhosOptional(),
    };
}

QNapi::Object toNapiObject(napi_env env, const QOhosWindowProxy::MoveConfiguration &moveConfiguration)
{
    auto moveConfigurationObject = QNapi::makeObject(env);
    if (moveConfiguration.displayId.hasValue()) {
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

QOhosOptional<QEvent::Type> tryMapMouseEventActionToNonClientAreaEventType(::Input_MouseEventAction action)
{
    QOhosOptional<QEvent::Type> eventType;
    switch (action) {
    case ::MOUSE_ACTION_MOVE:
        return makeQOhosOptional(QEvent::NonClientAreaMouseMove);
    case ::MOUSE_ACTION_BUTTON_DOWN:
        return makeQOhosOptional(QEvent::NonClientAreaMouseButtonPress);
    case ::MOUSE_ACTION_BUTTON_UP:
        return makeQOhosOptional(QEvent::NonClientAreaMouseButtonRelease);
    case ::MOUSE_ACTION_CANCEL:
    case ::MOUSE_ACTION_AXIS_BEGIN:
    case ::MOUSE_ACTION_AXIS_UPDATE:
    case ::MOUSE_ACTION_AXIS_END:
        break;
    }
    return makeEmptyQOhosOptional();
}

QOhosOptional<Qt::MouseButton> tryMapMouseEventButtonToQt(::Input_MouseEventButton button)
{
    switch (button) {
    case ::MOUSE_BUTTON_LEFT:
        return makeQOhosOptional(Qt::LeftButton);
    case ::MOUSE_BUTTON_MIDDLE:
        return makeQOhosOptional(Qt::MiddleButton);
    case ::MOUSE_BUTTON_RIGHT:
        return makeQOhosOptional(Qt::RightButton);
    case ::MOUSE_BUTTON_FORWARD:
        return makeQOhosOptional(Qt::ForwardButton);
    case ::MOUSE_BUTTON_BACK:
        return makeQOhosOptional(Qt::BackButton);
    case ::MOUSE_BUTTON_NONE:
        break;
    }
    return makeEmptyQOhosOptional();
}

QOhosOptional<QEventPoint::State> tryMapTouchEventActionToNonClientAreaEventState(::Input_TouchEventAction action)
{
    switch (action) {
    case ::TOUCH_ACTION_MOVE:
        return makeQOhosOptional(QEventPoint::State::Updated);
    case ::TOUCH_ACTION_DOWN:
        return makeQOhosOptional(QEventPoint::State::Pressed);
    case ::TOUCH_ACTION_UP:
        return makeQOhosOptional(QEventPoint::State::Released);
    case ::TOUCH_ACTION_CANCEL:
        break;
    }
    return makeEmptyQOhosOptional();
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
                std::move(windowProxyData.qAbilityPeer))))
    , m_windowProxyType(windowProxyData.windowProxyType)
    , m_nodeXComponent(windowProxyData.nodeXComponent)
    , m_qAbilityInstanceId(m_jsScopeData->qAbilityPeer->instanceId())
{
    for (const auto &eventHandlerDescriptor : eventHandlerDescriptors) {
        m_jsScopeData->registerCallbackListener(
            eventHandlerDescriptor.eventName, eventHandlerDescriptor.eventHandler,
            eventHandlerDescriptor.eventHandlerFlags);
    }
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
        [&](QtOhos::JsState &, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }
            auto optQUiAbilityPeer
                = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_jsScopeData->qAbilityPeer);
            if (!optQUiAbilityPeer) {
                continueFunc();
                return;
            }
            auto promise = optQUiAbilityPeer->windowStage().call<QNapi::Promise>(
                "removeStartingWindow");
            promise.onFinally(std::move(continueFunc));
        });
}

void QOhosWindowProxy::setSize(const QSize &size)
{
    qOhosPrintfDebug("%s: %d,%d", Q_FUNC_INFO, size.width(), size.height());

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }
            auto promise = m_jsScopeData->jsWindowRef->call<QNapi::Promise>(
                "resizeAsync", {size.width(), size.height()});
            promise.onFinally(std::move(continueFunc));
        });
}

void QOhosWindowProxy::setWindowBackgroundColor(const QColor &color)
{
    qCDebug(QtForOhos) << Q_FUNC_INFO << color;

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;
        m_jsScopeData->jsWindowRef->call("setWindowBackgroundColor", {color.name(QColor::HexArgb).toStdString()});
    });
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
    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &jsState, std::function<void()> continueFunc) {
        if (m_jsScopeData->isWindowClosing()) {
            continueFunc();
            return;
        }
        auto windowId = m_jsScopeData->jsWindowRef->jsObject().get<QNapi::Number>("getWindowProperties().id");

        auto jsCursor = QNapi::makeObject(
            jsState.env(),
            {
                {"pixelMap", createNapiPixelMapFromQImage(jsState, convertedImage)},
                {"focusX", hotSpot.x()},
                {"focusY", hotSpot.y()},
            });

        auto jsCursorConfig = QNapi::makeObject(
            jsState.env(),
            {
                {"followSystem", false},
            });

        jsState.eval<QNapi::Promise>(
            "@ohos.multimodalInput.pointer.setCustomCursor(*)", {windowId, jsCursor, jsCursorConfig})
        .onCatch(QtOhos::makeErrorLoggingJsCallback("setCustomCursor()"))
        .onFinally(std::move(continueFunc));
    });
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
    });
}

QArkUi::WindowProperties QOhosWindowProxy::getWindowProperties() const
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            if (m_jsScopeData->isWindowClosing())
                return QArkUi::WindowProperties {};
            return getWindowPropertiesFromJsWindow(m_jsScopeData->jsWindowRef->jsObject());
        });
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
            .onWindowDisplayIdChange = makeQtThreadWindowCallbackDelegate(&WindowCallbacks::onWindowDisplayIdChange, sharedWindowCallbackReceiver),
        });
    });

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
    });

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
    });

    m_qtNonClientAreaTouchWindowCallbackReceiverHandle = qtConsumer;
}

bool QOhosWindowProxy::qtIsMainWindow() const
{
    return m_windowProxyType == WindowProxyType::MainWindow;
}

WindowProxyType QOhosWindowProxy::windowProxyType() const
{
    return m_windowProxyType;
}

void QOhosWindowProxy::raiseToAppTop()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &, std::function<void()> continueFunc) {
        if (m_jsScopeData->isWindowClosing()) {
            continueFunc();
            return;
        }
        auto promise = m_jsScopeData->jsWindowRef->call<QNapi::Promise>("raiseToAppTop");
        promise.onFinally(std::move(continueFunc));
    });
}

void QOhosWindowProxy::showWindow(const ShowWindowOptions &options)
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &jsState, std::function<void()> continueFunc) {
        if (m_jsScopeData->isWindowClosing()) {
            continueFunc();
            return;
        }

        std::vector<std::pair<std::string, QNapi::ValueWrapper>> jsOptionsProps;

        if (options.focusOnShow.hasValue())
            jsOptionsProps.emplace_back("focusOnShow", options.focusOnShow.value());

        std::vector<QNapi::ValueWrapper> showWindowArgs;
        constexpr auto minSupportedSdkVersionForOptions = 20;
        if (QOhosDeviceInfo::sdkApiVersion() >= minSupportedSdkVersionForOptions) {
            constexpr bool brokenHandlingOfEmptyOptionsParamInOhos = true;
            if (!(jsOptionsProps.empty() && brokenHandlingOfEmptyOptionsParamInOhos)) {
                showWindowArgs.push_back(QNapi::makeObject(jsState.env(), jsOptionsProps));
            }
        } else if (!jsOptionsProps.empty()) {
            qOhosPrintfWarning(
                "%s: showWindow() doesn't accept options in SDK version < %d, ignored %zu options",
                Q_FUNC_INFO, minSupportedSdkVersionForOptions, jsOptionsProps.size());
        }

        auto promise = m_jsScopeData->jsWindowRef->call<QNapi::Promise>("showWindow", showWindowArgs);
        promise.onFinally(std::move(continueFunc));
    });
}

void QOhosWindowProxy::recover()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;
        m_jsScopeData->jsWindowRef->call("recover");
    });
}

void QOhosWindowProxy::restore()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &, std::function<void()> continueFunc) {
        if (m_jsScopeData->isWindowClosing()) {
            continueFunc();
            return;
        }
        m_jsScopeData->jsWindowRef->call<QNapi::Promise>("restore").onFinally(std::move(continueFunc));
    });
}

void QOhosWindowProxy::minimize()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &, std::function<void()> continueFunc) {
        if (m_jsScopeData->isWindowClosing()) {
            continueFunc();
            return;
        }
        auto promise = m_jsScopeData->jsWindowRef->call<QNapi::Promise>("minimize");
        promise.onFinally(std::move(continueFunc));
    });
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
        m_jsScopeData->jsWindowRef->call("maximize", {jsState.mapOhosEnumToJs(maximizePresentation)});
    });
}

void QOhosWindowProxy::showAbility()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);

    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &, std::function<void()> continueFunc) {
        if (m_jsScopeData->isWindowClosing()) {
            continueFunc();
            return;
        }

        QNapi::Promise showAbilityPromise;
        try {
            showAbilityPromise = m_jsScopeData->qAbilityPeer->qAbility().call<QNapi::Promise>("context.showAbility");
        } catch (const Napi::Error &error) {
            qOhosPrintfError("showAbility failed with error: %s", error.what());
            continueFunc();
            return;
        }

        showAbilityPromise
        .onCatch(QtOhos::makeErrorLoggingJsCallback("showAbility()"))
        .onFinally(std::move(continueFunc));
    });
}

bool QOhosWindowProxy::tryHideAbility()
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);

    return QtOhos::evalInJsThreadWithConsumer<bool>([&](QtOhos::JsState &, QOhosConsumer<bool> resultConsumer) {
        if (m_jsScopeData->isWindowClosing()) {
            resultConsumer(false);
            return;
        }

        QNapi::Promise hidePromise;
        try {
            hidePromise = m_jsScopeData->qAbilityPeer->qAbility().call<QNapi::Promise>("context.hideAbility");
        } catch (const Napi::Error &error) {
            qOhosPrintfError("hideAbility failed with error: %s", error.what());
            resultConsumer(false);
            return;
        }

        hidePromise
            .withContext(std::move(resultConsumer))
            .onThenWithContext(
                [](const QtOhos::CallbackInfo &, auto &resultConsumer) {
                    resultConsumer(true);
                })
            .onCatchWithContext(
                [](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
                    QtOhos::logJsCallbackError(cbInfo, "got error from hideAbility()");
                    resultConsumer(false);
                });
        });
}

bool QOhosWindowProxy::getImmersiveModeEnabledState()
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            if (m_jsScopeData->isWindowClosing())
                return false;
            return m_jsScopeData->jsWindowRef->call<QNapi::Boolean>("getImmersiveModeEnabledState").Value();
        });
}

void QOhosWindowProxy::setWindowPrivacyMode(bool privacyMode)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, privacyMode ? "true" : "false");

    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &, std::function<void()> continueFunc) {
        if (m_jsScopeData->isWindowClosing()) {
            continueFunc();
            return;
        }

        QNapi::Promise setPrivacyModePromise;
        try {
            setPrivacyModePromise = m_jsScopeData->jsWindowRef->call<QNapi::Promise>(
                "setWindowPrivacyMode", {privacyMode});
        } catch (const Napi::Error &error) {
            qOhosPrintfError("setPrivacyMode failed with error: %s", error.what());
            continueFunc();
            return;
        }

        setPrivacyModePromise
        .onCatch(QtOhos::makeErrorLoggingJsCallback("setPrivacyMode()"))
        .onFinally(std::move(continueFunc));
    });
}

void QOhosWindowProxy::setWindowFocusable(bool focusable)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, focusable ? "true" : "false");
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;
        m_jsScopeData->jsWindowRef->call("setWindowFocusable", {focusable});
    });
}

void QOhosWindowProxy::setWindowTouchable(bool touchable)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, touchable ? "true" : "false");
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;
        m_jsScopeData->jsWindowRef->call("setWindowTouchable", {touchable});
    });
}

void QOhosWindowProxy::setWindowLimits(const QSize &minSize, const QSize &maxSize)
{
    qCDebug(
        QtForOhos, "%s: (%d x %d)-(%d x %d)", Q_FUNC_INFO, minSize.width(), minSize.height(),
        maxSize.width(), maxSize.height());
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
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
            auto setWindowLimitsPromiseOrValue = m_jsScopeData->jsWindowRef->call(
                "setWindowLimits", setWindowLimitsArgs);

            if (setWindowLimitsPromiseOrValue.IsPromise()) {
                QNapi::checkedCast<QNapi::Promise>(setWindowLimitsPromiseOrValue).onFinally(std::move(continueFunc));
            } else {
                qOhosPrintfWarning("setWindowLimits() didn't return a Promise, ignoring result");
                continueFunc();
            }
        });
}

QOhosWindowProxy::WindowLimits QOhosWindowProxy::getWindowLimits() const
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            if (m_jsScopeData->isWindowClosing())
                return WindowLimits {};
            auto windowLimitsObject = m_jsScopeData->jsWindowRef->call<QNapi::Object>("getWindowLimits");
            return WindowLimits {
                .minWidth = getOptionalNumberPropAsOptionalDouble(windowLimitsObject, "minWidth"),
                .minHeight = getOptionalNumberPropAsOptionalDouble(windowLimitsObject, "minHeight"),
                .maxWidth = getOptionalNumberPropAsOptionalDouble(windowLimitsObject, "maxWidth"),
                .maxHeight = getOptionalNumberPropAsOptionalDouble(windowLimitsObject, "maxHeight"),
            };
        });
}

QOhosWindowProxy::AvoidArea QOhosWindowProxy::getWindowAvoidArea(AvoidAreaType avoidAreaType) const
{
    qCDebug(QtForOhos, "%s: %d", Q_FUNC_INFO, avoidAreaType);
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &jsState) {
            if (m_jsScopeData->isWindowClosing())
                return AvoidArea {};
            auto avoidAreaObject = m_jsScopeData->jsWindowRef->call<QNapi::Object>(
                "getWindowAvoidArea", {jsState.mapOhosEnumToJs(avoidAreaType)});
            return mapAvoidAreaFromJs(avoidAreaObject);
        });
}

void QOhosWindowProxy::setWindowMask(
    const WindowMask &windowMask, const QOhosOptional<QSize> &ohosMaskSizeOverride)
{
    if (qtIsMainWindow())
        return;

    auto ohosMaskSize = ohosMaskSizeOverride.hasValue()
        ? ohosMaskSizeOverride.value()
        : getWindowProperties().windowRect.size();

    if (ohosMaskSize.isEmpty()) {
        const auto *maskSrcSizeMsg = ohosMaskSizeOverride.hasValue()
            ? "overridden mask source"
            : "window";
        if (ohosMaskSizeOverride.hasValue()) {
            qOhosPrintfError(
                "%s failed - %s size is 0x0", maskSrcSizeMsg,
                "QOhosWindowProxy::setWindowMask");
        }
        return;
    }

    QtOhos::invokeInJsThreadAndWaitForContinue([&](QtOhos::JsState &jsState, std::function<void()> continueFunc) {
        if (m_jsScopeData->isWindowClosing()) {
            continueFunc();
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

        // HACK - The setWindowMask has inconsistent definition and the function implementation itself may throw
        // without returning any promise.
        QNapi::Promise setWindowMaskPromise;
        try {
            setWindowMaskPromise = m_jsScopeData->jsWindowRef->call<QNapi::Promise>(
                "setWindowMask", {maskRowsArray});
        } catch (const Napi::Error &error) {
            qOhosPrintfError("setWindowMask failed with error: %s", error.what());
            continueFunc();
            return;
        }

        setWindowMaskPromise
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setWindowMask()"))
            .onFinally(std::move(continueFunc));
    });
}

void QOhosWindowProxy::setSubWindowModalDisabled()
{
    if (qtIsMainWindow())
        return;

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }
            m_jsScopeData->jsWindowRef->call<QNapi::Promise>("setSubWindowModal", {false})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setSubWindowModal()"))
            .onFinally(std::move(continueFunc));
        });
}

void QOhosWindowProxy::setSubWindowModalEnabled(ModalityType modalityType)
{
    if (qtIsMainWindow())
        return;

    if (modalityType == ModalityType::APPLICATION_MODALITY && !QOhosSettings::isWindowPcModeEnabled()) {
        qOhosPrintfWarning(
            "%s: APPLICATION_MODALITY option can be used only on devices in the freeform window state - skipping",
            Q_FUNC_INFO);
        return;
    }

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }
            m_jsScopeData->jsWindowRef->call<QNapi::Promise>(
                "setSubWindowModal", {true, jsState.mapOhosEnumToJs(modalityType)})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setSubWindowModal()"))
            .onFinally(std::move(continueFunc));
        });
}

void QOhosWindowProxy::setTitle(const QString &title)
{
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }

            m_jsScopeData->jsWindowRef->call<QNapi::Promise>("setWindowTitle", {title.toStdString()})
            .onFinally(std::move(continueFunc));
        });
}

void QOhosWindowProxy::setWindowTitleButtonVisible(bool maximizeVisible, bool minimizeVisible, bool closeVisible)
{
    qOhosPrintfDebug(
        "%s: isMaximizeVisible:%s, isMinimizeVisible:%s, isCloseVisible:%s",
        Q_FUNC_INFO, maximizeVisible ? "true" : "false", minimizeVisible ? "true": "false",
        closeVisible ? "true" : "false");

    if (!qtIsMainWindow())
        return;

    if (!QOhosSettings::isWindowPcModeEnabled()) {
        qOhosPrintfWarning("%s: can be used only on 2-in-1 devices or tablets in PC mode - skipping", Q_FUNC_INFO);
        return;
    }

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            if (m_jsScopeData->isWindowClosing())
                return;

            m_jsScopeData->jsWindowRef->call(
                "setWindowTitleButtonVisible", {maximizeVisible, minimizeVisible, closeVisible});
        });
}

void QOhosWindowProxy::setWindowTopmost(bool topmost)
{
    if (!qtIsMainWindow())
        return;

    if (!QOhosSettings::isWindowPcModeEnabled()) {
        qOhosPrintfWarning("%s: can be used only on 2-in-1 devices or tablets in PC mode - skipping", Q_FUNC_INFO);
        return;
    }

    qOhosPrintfDebug("%s: topMost: %s", Q_FUNC_INFO, topmost ? "true" : "false");

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }

            m_jsScopeData->jsWindowRef->call<QNapi::Promise>("setWindowTopmost", {topmost})
                .onFinally(std::move(continueFunc));
        });
}

void QOhosWindowProxy::setWindowDecorVisible(bool visible)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;

        m_jsScopeData->jsWindowRef->call("setWindowDecorVisible", {visible});
    });
}

void QOhosWindowProxy::setWindowTitleMoveEnabled(bool enabled)
{
    if (!QOhosSettings::isWindowPcModeEnabled()) {
        qOhosPrintfWarning("%s: can be used only on 2-in-1 devices or tablets in PC mode - skipping", Q_FUNC_INFO);
        return;
    }

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        if (m_jsScopeData->isWindowClosing())
            return;

        m_jsScopeData->jsWindowRef->call("setWindowTitleMoveEnabled", {enabled});
    });
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
        m_jsScopeData->jsWindowRef->call("setWindowShadowRadius", {radius});
    });
}

void QOhosWindowProxy::setWindowCornerRadius(double radius)
{
    if (qtIsMainWindow())
        return;

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, std::function<void()> continueFunc) {
            m_jsScopeData->jsWindowRef->call<QNapi::Promise>("setWindowCornerRadius", {radius})
            .onFinally(std::move(continueFunc));
        });
}

bool QOhosWindowProxy::isWindowRectAutoSave() const
{
    if (!QOhosDeviceInfo::is2in1()) {
        qCWarning(
            QtForOhos,
            "%s: geometry persistence feature is only supported on 2-in-1 devices. Returning default",
            Q_FUNC_INFO);
        return false;
    }

    return QtOhos::evalInJsThreadWithConsumer<bool>(
        [&](QtOhos::JsState &, QOhosConsumer<bool> windowRectAutoSaveEnabledConsumer) {
            if (m_jsScopeData->isWindowClosing()) {
                windowRectAutoSaveEnabledConsumer(false);
                return;
            }
            auto optQUiAbilityPeer = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_jsScopeData->qAbilityPeer);
            if (!optQUiAbilityPeer) {
                windowRectAutoSaveEnabledConsumer(false);
                return;
            }

            optQUiAbilityPeer->windowStage()
            .call<QNapi::Promise>("isWindowRectAutoSave")
            .withContext(std::move(windowRectAutoSaveEnabledConsumer))
            .onThenWithContext(
                [](const QtOhos::CallbackInfo &cbInfo, auto &windowRectAutoSaveEnabledConsumer) {
                    bool windowRectAutoSaveEnabled = cbInfo.getFirstArg<QNapi::Boolean>(Q_FUNC_INFO);
                    windowRectAutoSaveEnabledConsumer(windowRectAutoSaveEnabled);
                })
            .onCatchWithContext(
                [](const QtOhos::CallbackInfo &cbInfo, auto &windowRectAutoSaveEnabledConsumer) {
                    QtOhos::logJsCallbackError(cbInfo, "isWindowRectAutoSave()");
                    windowRectAutoSaveEnabledConsumer(false);
                });
        });
}

void QOhosWindowProxy::setFollowParentMultiScreenPolicy(bool enabled)
{
    if (qtIsMainWindow())
        return;

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }
            m_jsScopeData->jsWindowRef->call<QNapi::Promise>("setFollowParentMultiScreenPolicy", {enabled})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setFollowParentMultiScreenPolicy()"))
            .onFinally(std::move(continueFunc));
        });
}

void QOhosWindowProxy::setWindowKeepScreenOn(bool keepScreenOn)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, QtOhos::mapBoolToTrueFalseStr(keepScreenOn));

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }

            m_jsScopeData->jsWindowRef->call<QNapi::Promise>("setWindowKeepScreenOn", {keepScreenOn})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setWindowKeepScreenOn()"))
            .onFinally(std::move(continueFunc));
        });
}

void QOhosWindowProxy::setSupportedWindowModes(const std::set<SupportWindowMode> &supportedWindowModes)
{
    qCDebug(QtForOhos, "%s: %s", Q_FUNC_INFO, mapEnumsToLogString(supportedWindowModes).c_str());

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }
            auto qUiAbilityPeer = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_jsScopeData->qAbilityPeer);
            if (!qUiAbilityPeer) {
                continueFunc();
                return;
            }

            std::vector<QNapi::ValueWrapper> jsSupportedWindowModes;
            for (const auto mode : supportedWindowModes)
                jsSupportedWindowModes.push_back(jsState.mapOhosEnumToJs(mode));

            qUiAbilityPeer->windowStage().call<QNapi::Promise>("setSupportedWindowModes", {QNapi::makeArray(jsState.env(), jsSupportedWindowModes)})
                .onCatch(QtOhos::makeErrorLoggingJsCallback("setSupportedWindowModes()"))
                .onFinally(std::move(continueFunc));
        });
}

void QOhosWindowProxy::setWindowRectAutoSave(bool enabled)
{
    constexpr bool isSaveBySpecifiedFlag = true;

    if (!QOhosDeviceInfo::is2in1()) {
        qCWarning(
            QtForOhos,
            "%s: geometry persistence feature is only supported on 2-in-1 devices. Ignoring",
            Q_FUNC_INFO);
        return;
    }

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }
            auto optQUiAbilityPeer = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_jsScopeData->qAbilityPeer);
            if (!optQUiAbilityPeer) {
                continueFunc();
                return;
            }

            optQUiAbilityPeer->windowStage()
            .call<QNapi::Promise>("setWindowRectAutoSave", {enabled, isSaveBySpecifiedFlag})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setWindowRectAutoSave()"))
            .onFinally(std::move(continueFunc));
        });
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
        });

    return QtOhos::moveToSharedPtr(
        std::make_tuple(sharedHandler, jsWindowRegistrationHandle));
}

std::shared_ptr<QOhosWindowProxy>
QOhosWindowProxy::createForExistingMainWindow(const ExistingMainWindowCreateInfo &createInfo)
{
    return QtOhos::evalInJsThreadWithConsumer<std::shared_ptr<QOhosWindowProxy>>(
        [&](QtOhos::JsState &jsState, QOhosConsumer<std::shared_ptr<QOhosWindowProxy>> resultConsumer) {
            makeWindowProxyDataForExistingMainWindowInJsThread(
                jsState,
                createInfo,
                [resultConsumer = std::move(resultConsumer)](QtOhos::JsState &jsState, QOhosWindowProxyData windowProxyData) {
                    resultConsumer(QOhosWindowProxy::create(jsState, std::move(windowProxyData)));
                });
        });
}

std::shared_ptr<QOhosWindowProxy>
QOhosWindowProxy::createFloatWindow(const FloatWindowCreateInfo &createInfo)
{
    return QtOhos::evalInJsThreadWithConsumer<std::shared_ptr<QOhosWindowProxy>>(
        [&](QtOhos::JsState &jsState, QOhosConsumer<std::shared_ptr<QOhosWindowProxy>> resultConsumer) {
            makeWindowProxyDataForFloatWindowInJsThread(
                jsState, createInfo,
                [resultConsumer = std::move(resultConsumer)](QtOhos::JsState &jsState, QOhosWindowProxyData windowProxyData) {
                    resultConsumer(QOhosWindowProxy::create(jsState, std::move(windowProxyData)));
                });
        });
}

std::shared_ptr<QOhosWindowProxy>
QOhosWindowProxy::createMainWindow(const MainWindowCreateInfo &createInfo)
{
    return QtOhos::evalInJsThreadWithConsumer<std::shared_ptr<QOhosWindowProxy>>(
        [&](QtOhos::JsState &jsState, QOhosConsumer<std::shared_ptr<QOhosWindowProxy>> resultConsumer) {
            makeWindowProxyDataForMainWindowInJsThread(
                jsState,
                createInfo,
                [resultConsumer = std::move(resultConsumer)](QtOhos::JsState &jsState, QOhosWindowProxyData windowProxyData) {
                    resultConsumer(QOhosWindowProxy::create(jsState, std::move(windowProxyData)));
                });
        });
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
    return QtOhos::evalInJsThreadWithConsumer<std::shared_ptr<QOhosWindowProxy>>(
        [&](QtOhos::JsState &jsState, QOhosConsumer<std::shared_ptr<QOhosWindowProxy>> resultConsumer) {
            auto proxyDataConsumer =
                [resultConsumer = std::move(resultConsumer)](QtOhos::JsState &jsState, QOhosWindowProxyData windowProxyData) {
                    resultConsumer(QOhosWindowProxy::create(jsState, std::move(windowProxyData)));
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
        });
}

QOhosWindowProxy::JsScopeData::JsScopeData(
    WindowProxyType windowProxyType, QNapi::Reference<QNapi::Object> jsWindow,
    std::shared_ptr<void> optKeepAliveData,
    std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer)
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
            getWindowPropertiesFromJsWindow(jsWindow.Value()).id,
            jsWindow.Value()))
{
}

QOhosWindowProxy::JsScopeData::~JsScopeData()
{
    if (isWindowClosingFromSystem(jsWindowRef->jsObject(), windowProxyType, qAbilityPeer)) {
        windowDestroyedFromSystem = true;
        return;
    }

    if (!windowDestroyedFromSystem) {
        for (const auto &eventHandlerDescriptor : eventHandlerDescriptors)
            jsWindowRef->call("off", {eventHandlerDescriptor.eventName});
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
        qAbilityPeer->qAbility().call("context.terminateSelf");
    } else if (!windowDestroyedFromSystem) {
        // FIXME - destroyWindow usually does and returns nothing
        // once the actual implementation is provided wait for the proomise that this function should return
        jsWindowRef->call("destroyWindow");
    }
}

void QOhosWindowProxy::JsScopeData::registerCallbackListener(
    const std::string &eventName,
    void (QOhosWindowProxy::JsScopeData::*handleFunction)(const QtOhos::CallbackInfo &),
    QFlags<EventHandlerFlagBits> eventHandlerFlags)
{

    std::weak_ptr<JsScopeData> weakSelf = shared_from_this();
    bool ignoreWhenAbilityIsTerminating = !eventHandlerFlags.testFlag(EventHandlerFlagBits::allowCallWhenAbilityIsTerminating);
    try {
        jsWindowRef->call(
            "on",
            {
                eventName,
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
                }
            });
    } catch (const Napi::Error &error) {
        constexpr std::uint32_t capabilityNotSupportedErrorCode = 801;
        constexpr std::uint32_t windowStateIsAbnormalErrorCode = 1300002;

        const QSet<std::uint32_t> ignorableErrorCodes = {
            capabilityNotSupportedErrorCode,
            windowStateIsAbnormalErrorCode,
        };

        auto errorCode = QtOhos::tryGetCodeFromJsBusinessError(error);

        auto ignorableError =
            eventHandlerFlags.testFlag(EventHandlerFlagBits::allowEventHandlerRegistrationFailure)
            && errorCode.hasValue()
            && ignorableErrorCodes.contains(errorCode.value());

        if (!ignorableError)
            throw;

        qOhosPrintfWarning(
            "%s: Ignored error %u while registering for window event '%s'",
            Q_FUNC_INFO, errorCode.value(), eventName.c_str());
    }
}

std::shared_ptr<void> QOhosWindowProxy::JsScopeData::registerSubWindowCloseHandler(
    QtOhos::JsState &, std::function<bool()> handler)
{
    auto weakSelf = QtOhos::makeWeakPtr(shared_from_this());
    return QtOhos::registerOnOffMethodsBasedEventHandler(
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
    if (!optAction.hasValue())
        return;

    auto optWindowProperties = QArkUi::tryGetWindowProperties(event.jsWindowId);
    if (!optWindowProperties.hasValue()) {
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
        .button = tryMapMouseEventButtonToQt(event.button).valueOr(Qt::NoButton),
        .displayPosition = event.displayPosition,
        .localPosition = event.displayPosition - windowOrigin,
    };

    nonClientAreaMouseEventConsumer(nonClientAreaMouseEvent);
}

void QOhosWindowProxy::JsScopeData::onTouchEventFromArkUi(const QArkUi::TouchEvent &event)
{
    if (nonClientAreaTouchEventConsumer == nullptr)
        return;

    auto optState = tryMapTouchEventActionToNonClientAreaEventState(event.action);
    if (!optState.hasValue())
        return;

    auto optWindowProperties = QArkUi::tryGetWindowProperties(event.jsWindowId);
    if (!optWindowProperties.hasValue()) {
        qOhosPrintfError(
            "%s: Failed to retrieve window properties for js window: %f. Ignoring event.",
            Q_FUNC_INFO, event.jsWindowId.value());
        return;
    }

    if (!isPointInNonClientArea(event.displayPosition, optWindowProperties.value()))
        return;

    NonClientAreaTouchEvent nonClientAreaTouchEvent = {
        .id = event.fingerId,
        .timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(event.actionTime),
        .state = optState.value(),
        .displayPosition = event.displayPosition,
    };

    nonClientAreaTouchEventConsumer(nonClientAreaTouchEvent);
}

QPixmap QOhosWindowProxy::snapshot() const
{
    return QtOhos::evalInJsThreadWithConsumer<QPixmap>(
        [&](QtOhos::JsState &, QOhosConsumer<QPixmap> resultConsumer) {
            m_jsScopeData->jsWindowRef->call<QNapi::Promise>("snapshot")
            .withContext(std::move(resultConsumer))
            .onThenWithContext(
                [](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
                    auto napiPixmap = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

                    ::OH_PixelmapNative *pixelMapNativePtr;
                    QArkUi::callArkUiOrFailOnErrorResult(
                        Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_ConvertPixelmapNativeFromNapi),
                        cbInfo.Env(), napiPixmap, &pixelMapNativePtr);
                    auto pixelMap = wrapNativePixelMapPtr(pixelMapNativePtr);

                    resultConsumer(
                        QPixmap::fromImage(createQImageFromNativePixelMap(pixelMap.get())));
                })
            .onCatchWithContext(
                [](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
                    QtOhos::logJsCallbackError(cbInfo, "Got error from snapshot()");
                    resultConsumer(QPixmap());
                });
        });
}

bool QOhosWindowProxy::startMoving()
{
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }

            m_jsScopeData->jsWindowRef->call<QNapi::Promise>("startMoving")
            .onCatch(QtOhos::makeErrorLoggingJsCallback("startMoving()"))
            .onFinally(std::move(continueFunc));
        });

    return true;
}

QOhosOptional<bool> QOhosWindowProxy::isFocused() const
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            if (m_jsScopeData->isWindowClosing())
                return QOhosOptional<bool>();

            bool focused = m_jsScopeData->jsWindowRef->call<QNapi::Boolean>("isFocused");
            return makeQOhosOptional(focused);
        });
}

std::vector<QArkUi::JsWindowId> QOhosWindowProxy::queryWindowIdsByCoordinate(
    QOhosDisplayInfo::JsDisplayId displayId, const QPoint &queryLocation, std::uint32_t queryLimit)
{
    return QtOhos::evalInJsThreadWithConsumer<std::vector<QArkUi::JsWindowId>>(
        [&](QtOhos::JsState &jsState, auto resultConsumer) {
            jsState.eval<QNapi::Promise>("@ohos.window.getWindowsByCoordinate(*)", {
                displayId.value(),
                queryLimit,
                queryLocation.x(),
                queryLocation.y()
            })
            .withContext(std::move(resultConsumer))
            .onThenWithContext([](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
                auto windowsArray = cbInfo.getFirstArg<QNapi::Array>(Q_FUNC_INFO);
                resultConsumer(
                    QNapi::getArrayElements<std::vector<QArkUi::JsWindowId>, QNapi::Object>(
                        windowsArray,
                        [&](QNapi::Object jsWindow) {
                            return getWindowPropertiesFromJsWindow(jsWindow).id;
                        }));
            })
            .onCatchWithContext([](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
                QtOhos::logJsCallbackError(
                    cbInfo, "got error from @ohos.window.getWindowsByCoordinate()");
                resultConsumer({});
            });
        });
}

std::vector<QArkUi::JsWindowId> QOhosWindowProxy::queryQtManagedWindowIdsByPredicate(
    const std::function<bool(QtOhos::JsState &, const QArkUi::JsWindowRef &)> &predicate)
{
    return QtOhos::evalInJsThread([&](QtOhos::JsState &jsState) {
        auto &jsWindowRegistry = jsState.getAttachedObjectWithLazyCreate<QOhosJsWindowRegistry>();
        return jsWindowRegistry.queryByPredicate(jsState, predicate);
    });
}

void QOhosWindowProxy::moveWindowToGlobal(
    const QPoint &position, const MoveConfiguration &moveConfiguration)
{
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }

            auto moveConfigurationObject = toNapiObject(jsState.env(), moveConfiguration);
            auto moveConfigurationObjectStr = QNapi::toJsonString(moveConfigurationObject);
            qOhosPrintfDebug(
                "%s: %d,%d,%s",
                Q_FUNC_INFO, position.x(), position.y(),
                moveConfigurationObjectStr.c_str());

            m_jsScopeData->jsWindowRef->call<QNapi::Promise>(
                "moveWindowToGlobal", {position.x(), position.y(), moveConfigurationObject})
                .onFinally(std::move(continueFunc));
        });
}

QOhosOptional<QOhosDisplayInfo::JsDisplayId> QOhosWindowProxy::tryGetMainWindowJsDisplayId() const
{
    return qtIsMainWindow()
        ? getWindowProperties().displayId
        : QtOhos::evalInJsThread(
            [&](QtOhos::JsState &) {
                auto qUiAbilityPeer
                    = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_jsScopeData->qAbilityPeer);
                return qUiAbilityPeer
                    ? getWindowPropertiesFromJsWindow(qUiAbilityPeer->window()).displayId
                    : makeEmptyQOhosOptional();
            });
}

void QOhosWindowProxy::shiftAppWindowFocus(QOhosWindowProxy &targetProxy)
{
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, std::function<void()> continueFunc) {
            if (m_jsScopeData->isWindowClosing() || targetProxy.m_jsScopeData->isWindowClosing()) {
                continueFunc();
                return;
            }

            auto srcWindowId = getWindowPropertiesFromJsWindow(m_jsScopeData->jsWindowRef->jsObject()).id;
            auto targetWindowId = getWindowPropertiesFromJsWindow(targetProxy.m_jsScopeData->jsWindowRef->jsObject()).id;
            jsState.eval<QNapi::Promise> (
                "@ohos.window.shiftAppWindowFocus(*)", {srcWindowId.value(), targetWindowId.value()})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("@ohos.window.shiftAppWindowFocus()"))
            .onFinally(std::move(continueFunc));
        });
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
