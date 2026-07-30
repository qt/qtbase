// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWINDOWPROXY_H
#define QOHOSWINDOWPROXY_H

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
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
#include <qohosenums.h>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <render/qohoswindowproxydatafactory.h>
#include <render/qxcomponent.h>
#include <set>

QT_BEGIN_NAMESPACE

class QOhosWindowProxy final
{
public:
    using WindowEventType = QtOhos::enums::ohos::window::WindowEventType;

    using WindowStatusType = QtOhos::enums::ohos::window::WindowStatusType;

    using AvoidAreaType = QtOhos::enums::ohos::window::AvoidAreaType;

    using RectChangeReason = QtOhos::enums::ohos::window::RectChangeReason;

    using MaximizePresentation = QtOhos::enums::ohos::window::MaximizePresentation;

    using ModalityType = QtOhos::enums::ohos::window::ModalityType;

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

    using SupportWindowMode = QtOhos::enums::ohos::bundle::bundleManager::SupportWindowMode;

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
        const QPoint &position, std::optional<QOhosDisplayInfo::JsDisplayId> optDisplayId);

    void setSize(const QSize &size);
    void moveWindowToGlobal(const QPoint &position, const MoveConfiguration &moveConfiguration);
    void setWindowBackgroundColor(const QColor &color);
    void setCustomCursor(const QImage &customCursorImage, const QPoint &hotSpot);
    void setPointerStyleSync(const QCursor &cursor);
    void setWindowPrivacyMode(bool privacyMode);
    void setWindowFocusable(bool focusable);
    void setWindowTouchable(bool touchable);
    void setWindowLimits(const QSize &minSize, const QSize &maxSize);
    void setWindowMask(
        const WindowMask &windowMask, const std::optional<QSize> &ohosMaskSizeOverride = {});
    void setSubWindowModalDisabled();
    void setSubWindowModalEnabled(ModalityType ModalityType);
    void setTitle(const QString &title);
    void setWindowTitleButtonVisible(bool maximizeVisible, bool minimizeVisible, bool closeVisible);
    void setWindowTopmost(bool topmost);
    void setWindowDecorVisible(bool visible);
    void setWindowTitleMoveEnabled(bool enabled);
    void setWindowShadowRadius(double radius);
    void setWindowCornerRadius(double radius);
    void setWindowRectAutoSave(bool enabed);
    bool isWindowRectAutoSave() const;
    void setFollowParentMultiScreenPolicy(bool enabled);
    void setWindowKeepScreenOn(bool keepScreenOn);

    void setSupportedWindowModes(const std::set<SupportWindowMode> &supportedWindowModes);

    void setSubWindowCloseHandler(std::function<void()> handler, bool handlerReturnValue);
    void resetSubWindowCloseHandler();

    void raiseToAppTop();
    void showWindow(const ShowWindowOptions &options = ShowWindowOptions());
    void recover();
    void restore();
    void maximize(MaximizePresentation maximizePresentation);
    void minimize();
    void setWindowLayoutFullScreen(bool isLayoutFullScreen);
    void setWindowSystemBarEnable(const QStringList &names);
    void showAbility();
    bool tryHideAbility();
    void removeStartingWindow();
    bool getImmersiveModeEnabledState();
    QPixmap snapshot() const;
    bool startMoving();
    void enableDrag(bool enable);

    std::optional<bool> isFocused() const;

    QArkUi::WindowProperties getWindowProperties() const;
    void setWindowCallbackReceiver(std::unique_ptr<WindowCallbacks> receiver);
    void setNonClientAreaMouseWindowCallbackReceiver(
        QObject *contextObject, QOhosConsumer<std::vector<NonClientAreaMouseEvent>> mouseEventBatchConsumer);
    void setNonClientAreaTouchWindowCallbackReceiver(
        QObject *contextObject, QOhosConsumer<std::vector<NonClientAreaTouchEvent>> touchEventBatchConsumer);

    std::shared_ptr<QOhosWindowProxy> createSubWindow(const SubWindowCreateInfo &createInfo);

    bool qtIsMainWindow() const;
    WindowLimits getWindowLimits() const;
    AvoidArea getWindowAvoidArea(AvoidAreaType type) const;
    std::shared_ptr<QXComponentNode> nodeXComponent() const;
    std::string qAbilityInstanceId() const;

    static std::vector<QArkUi::JsWindowId> queryWindowIdsByCoordinate(
        QOhosDisplayInfo::JsDisplayId displayId, const QPoint &queryLocation,
        std::uint32_t queryLimit = 0);
    static std::vector<QArkUi::JsWindowId> queryQtManagedWindowIdsByPredicate(
        const std::function<bool(QtOhos::JsState &jsState, const QArkUi::JsWindowRef &)> &predicate);

    std::optional<QOhosDisplayInfo::JsDisplayId> tryGetMainWindowJsDisplayId() const;
    void shiftAppWindowFocus(QOhosWindowProxy &targetProxy);

private:
    static std::shared_ptr<QOhosWindowProxy> create(QtOhos::JsState &jsState, QOhosWindowProxyData data);

    enum class EventHandlerFlagBits
    {
        allowCallWhenAbilityIsTerminating = 1 << 0,
        allowEventHandlerRegistrationFailure = 1 << 1,
    };

    struct JsScopeData : public std::enable_shared_from_this<JsScopeData>
    {
        JsScopeData(
            WindowProxyType windowProxyType, QNapi::Reference<QNapi::Object> jsWindow,
            std::shared_ptr<void> optKeepAliveData,
            std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer,
            QtOhos::QObjectThreadSafeRef owningQWindowRef);
        ~JsScopeData();

        std::shared_ptr<void> registerEventListener(
            const std::string &eventName,
            void (JsScopeData::*handleFunctions)(const QtOhos::CallbackInfo &),
            QFlags<EventHandlerFlagBits> eventHandlerFlags);
        std::shared_ptr<void> registerSubWindowCloseHandler(
            QtOhos::JsState &jsState,
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
        void onMouseEventFromArkUi(const QArkUi::MouseEvent &event);
        void onTouchEventFromArkUi(const QArkUi::TouchEvent &event);

        WindowProxyType windowProxyType;
        std::shared_ptr<WindowCallbacks> windowCallbackReceiver;
        bool windowDestroyedFromSystem;
        std::shared_ptr<void> optKeepAliveData;
        std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer;
        std::shared_ptr<void> m_windowFrameMouseFilterHandle;
        std::shared_ptr<void> m_windowFrameTouchFilterHandle;
        std::shared_ptr<void> m_eventListenersHandle;
        QOhosConsumer<NonClientAreaMouseEvent> nonClientAreaMouseEventConsumer;
        QOhosConsumer<NonClientAreaTouchEvent> nonClientAreaTouchEventConsumer;
        std::shared_ptr<QArkUi::JsWindowRef> jsWindowRef;
    };

    struct EventHandlerDescriptor
    {
        const char *eventName;
        void (JsScopeData::*eventHandler)(const QtOhos::CallbackInfo &);
        QFlags<EventHandlerFlagBits> eventHandlerFlags;
    };

    QOhosWindowProxy(QOhosWindowProxyData data);

    std::shared_ptr<void> registerSubWindowCloseHandler(
        std::function<void()> handler,
        bool handlerReturnValue);

    static const EventHandlerDescriptor eventHandlerDescriptors[];

    std::shared_ptr<JsScopeData> m_jsScopeData;
    WindowProxyType m_windowProxyType;
    std::shared_ptr<QXComponentNode> m_nodeXComponent;
    std::string m_qAbilityInstanceId;
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
