// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWINDOWPROXY_H
#define QOHOSWINDOWPROXY_H

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <QtCore/qharmonyjsobjectref.h>
#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qpointer.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtGui/qcolor.h>
#include <QtGui/qeventpoint.h>
#include <QtGui/qwindow.h>
#include <functional>
#include <cstdint>
#include <memory>
#include <optional>
#include <qarkui/input.h>
#include <qarkui/window.h>
#include <qohosdisplayinfo.h>
#include <qohosjsscopedataowner.h>
#include <qohosplugincore.h>
#include <qohosqpaenums.h>
#include <qohosutils.h>
#include <render/qohoswindowproxydatafactory.h>
#include <render/qxcomponent.h>
#include <set>

QT_BEGIN_NAMESPACE

class QOhosWindowProxy final
{
public:
    using WindowEventType = QtOhosQpa::enums::ohos::window::WindowEventType;

    using WindowStatusType = QtOhosQpa::enums::ohos::window::WindowStatusType;

    using AvoidAreaType = QtOhosQpa::enums::ohos::window::AvoidAreaType;

    using RectChangeReason = QtOhosQpa::enums::ohos::window::RectChangeReason;

    using MaximizePresentation = QtOhosQpa::enums::ohos::window::MaximizePresentation;

    using ModalityType = QtOhosQpa::enums::ohos::window::ModalityType;

    using JsCallBatch = QtOhos::JsCallBatch;

    struct WindowEvent
    {
        WindowEventType type;
    };

    struct WindowStatus
    {
        WindowStatusType type;
    };

    struct WindowLimits
    {
        std::optional<double> minWidth;
        std::optional<double> minHeight;
        std::optional<double> maxWidth;
        std::optional<double> maxHeight;
    };

    struct AvoidArea
    {
        bool visible;
        QRect leftRect;
        QRect topRect;
        QRect rightRect;
        QRect bottomRect;
    };

    struct RectChangeOptions
    {
        QRect rect;
        RectChangeReason reason;
    };

    struct WindowMask
    {
        QRegion windowMaskRegion;
    };

    struct MoveConfiguration
    {
        std::optional<QOhosDisplayInfo::JsDisplayId> displayId;
    };

    struct NonClientAreaMouseEvent
    {
        std::chrono::milliseconds timestamp;
        QEvent::Type action;
        Qt::MouseButton button;
        QPointF displayPosition;
        QPointF localPosition;
        QPointF globalPosition;
    };

    struct NonClientAreaTouchEvent
    {
        std::int32_t id;
        std::chrono::milliseconds timestamp;
        QEvent::Type action;
        QPointF displayPosition;
        QPointF localPosition;
        QPointF globalPosition;
    };

    struct ShowWindowOptions
    {
        std::optional<bool> focusOnShow;
    };

    using SupportWindowMode = QtOhosQpa::enums::ohos::bundle::bundleManager::SupportWindowMode;

    struct WindowCallbacks
    {
        QOhosConsumer<QOhosWindowProxy::WindowEvent> onWindowEvent;
        QOhosConsumer<QOhosWindowProxy::WindowStatus> onWindowStatusChange;
        QOhosConsumer<bool> onWindowVisibilityChange;
        std::function<void()> onTouchOutside;
        QOhosConsumer<QOhosWindowProxy::AvoidAreaType, QOhosWindowProxy::AvoidArea> onAvoidAreaChange;
        QOhosConsumer<QOhosWindowProxy::RectChangeOptions> onWindowRectChange;
        QOhosConsumer<QOhosWindowProxy::RectChangeOptions> onWindowRectChangeInGlobalDisplay;
        QOhosConsumer<QOhosDisplayInfo::JsDisplayId> onWindowDisplayIdChange;
    };

    using MainWindowCreateInfo = QOhosWindowProxyMainWindowCreateInfo;
    using ExistingMainWindowCreateInfo = QOhosWindowProxyExistingMainWindowCreateInfo;
    using SubWindowCreateInfo = QOhosWindowProxySubWindowCreateInfo;
    using FloatWindowCreateInfo = QOhosWindowProxyFloatWindowCreateInfo;

    static std::shared_ptr<QOhosWindowProxy> createMainWindow(const MainWindowCreateInfo &createInfo);
    static std::shared_ptr<QOhosWindowProxy>
        createForExistingMainWindow(const ExistingMainWindowCreateInfo &createInfo);
    static std::shared_ptr<QOhosWindowProxy> createFloatWindow(const FloatWindowCreateInfo &createInfo);

    QOhosWindowProxy(const QOhosWindowProxy &) = delete;
    QOhosWindowProxy(QOhosWindowProxy &&) = delete;
    QOhosWindowProxy &operator=(const QOhosWindowProxy &) = delete;
    QOhosWindowProxy &operator=(QOhosWindowProxy &&) = delete;

    ~QOhosWindowProxy();

    void moveWindowToGlobalOrGlobalDisplay(
        const QPoint &position, std::optional<QOhosDisplayInfo::JsDisplayId> optDisplayId,
        JsCallBatch &jsCallBatch = JsCallBatch::immediate());

    void setSize(const QSize &size, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void moveWindowToGlobal(
        const QPoint &position, const MoveConfiguration &moveConfiguration,
        JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowBackgroundColor(const QColor &color, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setCustomCursor(
        const QImage &customCursorImage, const QPoint &hotSpot, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setPointerStyleSync(const QCursor &cursor, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowPrivacyMode(bool privacyMode, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowFocusable(bool focusable, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowTouchable(bool touchable, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowLimits(
        const QSize &minSize, const QSize &maxSize, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowMask(
        const WindowMask &windowMask, const std::optional<QSize> &ohosMaskSizeOverride = {},
        JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setSubWindowModalDisabled(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setSubWindowModalEnabled(ModalityType ModalityType, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setTitle(const QString &title, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowTitleButtonVisible(
        bool maximizeVisible, bool minimizeVisible, bool closeVisible,
        JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowTopmost(bool topmost, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowDecorVisible(bool visible, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowTitleMoveEnabled(bool enabled, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowShadowRadius(double radius, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowCornerRadius(double radius, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowRectAutoSave(bool enabed, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    bool isWindowRectAutoSave() const;
    void setFollowParentMultiScreenPolicy(bool enabled, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowKeepScreenOn(bool keepScreenOn, JsCallBatch &jsCallBatch = JsCallBatch::immediate());

    void setSupportedWindowModes(
        const std::set<SupportWindowMode> &supportedWindowModes, JsCallBatch &jsCallBatch = JsCallBatch::immediate());

    void setSubWindowCloseHandler(std::function<void()> handler, bool handlerReturnValue);
    void resetSubWindowCloseHandler();

    void raiseToAppTop(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void showWindow(
        const ShowWindowOptions &options = ShowWindowOptions(), JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void recover(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void restore(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    bool tryMaximize(MaximizePresentation maximizePresentation);
    void minimize(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowLayoutFullScreen(bool isLayoutFullScreen, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowSystemBarEnable(const QStringList &names, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    QList<QRect> getDisplayCutoutRects() const;
    void showAbility(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    bool tryHideAbility();
    void removeStartingWindow(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    bool getImmersiveModeEnabledState();
    QPixmap snapshot() const;
    bool startMoving();
    void enableDrag(bool enable, JsCallBatch &jsCallBatch = JsCallBatch::immediate());

    std::optional<bool> isFocused() const;

    QArkUi::WindowProperties getWindowProperties() const;
    void setWindowCallbackReceiver(std::unique_ptr<WindowCallbacks> receiver);
    void setNonClientAreaMouseWindowCallbackReceiver(
        QObject *contextObject, QOhosConsumer<std::vector<NonClientAreaMouseEvent>> mouseEventBatchConsumer);
    void setNonClientAreaTouchWindowCallbackReceiver(
        QObject *contextObject, QOhosConsumer<std::vector<NonClientAreaTouchEvent>> touchEventBatchConsumer);

    std::shared_ptr<QOhosWindowProxy> createSubWindow(const SubWindowCreateInfo &createInfo);

    bool qtIsMainWindow() const;
    bool isSubWindowMaximizeSupported() const;
    WindowLimits getWindowLimits() const;
    AvoidArea getWindowAvoidArea(AvoidAreaType type) const;
    std::shared_ptr<QXComponentNode> nodeXComponent() const;
    std::string qAbilityInstanceId() const;
    QHarmonyJsObjectRef abilityObjectRef() const;
    QHarmonyJsObjectRef jsWindowObjectRef() const;

    static std::vector<QArkUi::JsWindowId> queryWindowIdsByCoordinate(
        QOhosDisplayInfo::JsDisplayId displayId, const QPoint &queryLocation,
        std::uint32_t queryLimit = 0);
    static std::vector<QArkUi::JsWindowId> queryQtManagedWindowIdsByPredicate(
        const std::function<bool(QOhosJsState &jsState, const QArkUi::JsWindowRef &)> &predicate);

    std::optional<QOhosDisplayInfo::JsDisplayId> tryGetMainWindowJsDisplayId() const;
    void shiftAppWindowFocus(QOhosWindowProxy &targetProxy);

private:
    enum class EventHandlerFlagBits
    {
        allowCallWhenAbilityIsTerminating = 1 << 0,
        allowEventHandlerRegistrationFailure = 1 << 1,
    };

    struct JsScopeData : public std::enable_shared_from_this<JsScopeData>
    {
        struct NonClientAreaEventsDispatcher
        {
            void onMouseEventFromArkUi(const QArkUi::MouseEvent &event) const;
            void onTouchEventFromArkUi(const QArkUi::TouchEvent &event) const;

            QOhosConsumer<NonClientAreaMouseEvent> mouseEventsConsumer;
            QOhosConsumer<NonClientAreaTouchEvent> touchEventsConsumer;
        };

        JsScopeData();
        JsScopeData(
            WindowProxyType windowProxyType, QNapi::Reference<QNapi::Object> jsWindow,
            std::shared_ptr<void> optKeepAliveData,
            std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer,
            QtOhos::QObjectThreadSafeRef owningQWindowRef);
        JsScopeData(JsScopeData &&) = default;
        JsScopeData &operator=(JsScopeData &&) = default;
        ~JsScopeData();
        JsScopeData(const JsScopeData &) = delete;
        JsScopeData &operator=(const JsScopeData &) = delete;

        std::shared_ptr<void> registerEventListener(
            const std::string &eventName,
            void (JsScopeData::*handleFunctions)(const QtOhos::CallbackInfo &),
            QFlags<EventHandlerFlagBits> eventHandlerFlags);
        std::shared_ptr<void> registerSubWindowCloseHandler(
            QOhosJsState &jsState,
            std::function<bool()> handler);
        void handleWindowEventCallback(const QtOhos::CallbackInfo &cbInfo);
        void handleWindowStatusCallback(const QtOhos::CallbackInfo &cbInfo);
        void handleWindowVisibilityCallback(const QtOhos::CallbackInfo &cbInfo);
        void handleWindowSizeChangeCallback(const QtOhos::CallbackInfo &cbInfo);
        void handleWindowTouchOutsideCallback(const QtOhos::CallbackInfo &cbInfo);
        void handleAvoidAreaChangeCallback(const QtOhos::CallbackInfo &cbInfo);
        void handleWindowRectChangeCallback(const QtOhos::CallbackInfo &cbInfo);
        void handleWindowRectChangeInGlobalDisplayCallback(const QtOhos::CallbackInfo &cbInfo);
        void handleWindowDisplayIdChangeCallback(const QtOhos::CallbackInfo &cbInfo);
        void onWindowEvent(QtOhos::JsState &jsState, const WindowEvent &windowEvent);
        bool isWindowClosing() const;

        WindowProxyType windowProxyType = {};
        std::shared_ptr<WindowCallbacks> windowCallbackReceiver;
        bool windowDestroyedFromSystem = false;
        std::shared_ptr<void> optKeepAliveData;
        std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer;
        std::shared_ptr<NonClientAreaEventsDispatcher> nonClientAreaEventsDispatcher;
        std::shared_ptr<void> m_windowFrameMouseFilterHandle;
        std::shared_ptr<void> m_windowFrameTouchFilterHandle;
        std::shared_ptr<void> m_eventListenersHandle;
        std::shared_ptr<QArkUi::JsWindowRef> jsWindowRef;
    };

    struct EventHandlerDescriptor
    {
        const char *eventName;
        void (JsScopeData::*eventHandler)(const QtOhos::CallbackInfo &);
        QFlags<EventHandlerFlagBits> eventHandlerFlags;
    };

    struct JsThreadInitResult
    {
        WindowProxyType windowProxyType;
        bool subWindowMaximizeSupported;
        std::shared_ptr<QXComponentNode> nodeXComponent;
        std::string qAbilityInstanceId;
        QHarmonyJsObjectRef abilityObjectRef;
        QHarmonyJsObjectRef jsWindowObjectRef;
        std::shared_ptr<void> jsWindowRegistrationHandle;
    };

    QOhosWindowProxy(
        const std::function<void(QtOhos::JsState &, QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData>)> &makeWindowProxyDataInJsThread);
    JsThreadInitResult initInJsThread(QtOhos::JsState &jsState, QOhosWindowProxyData windowProxyData);

    std::shared_ptr<void> registerSubWindowCloseHandler(
        std::function<void()> handler,
        bool handlerReturnValue);

    static const EventHandlerDescriptor eventHandlerDescriptors[];

    QtOhos::JsScopeDataOwner<JsScopeData> m_jsScopeData;
    WindowProxyType m_windowProxyType;
    bool m_subWindowMaximizeSupported;
    std::shared_ptr<QXComponentNode> m_nodeXComponent;
    std::string m_qAbilityInstanceId;
    QHarmonyJsObjectRef m_abilityObjectRef;
    QHarmonyJsObjectRef m_jsWindowObjectRef;
    std::shared_ptr<void> m_jsWindowRegistrationHandle;
    std::shared_ptr<void> m_subWindowCloseRegistrationHandle;
    std::shared_ptr<void> m_qtWindowCallbacksReceiverHandle;
    std::shared_ptr<void> m_qtNonClientAreaMouseWindowCallbackReceiverHandle;
    std::shared_ptr<void> m_qtNonClientAreaTouchWindowCallbackReceiverHandle;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QOhosWindowProxy::WindowEvent);
Q_DECLARE_METATYPE(QOhosWindowProxy::WindowStatus);
Q_DECLARE_METATYPE(QOhosWindowProxy::AvoidArea);
Q_DECLARE_METATYPE(QOhosWindowProxy::RectChangeOptions);

#endif
