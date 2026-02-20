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
#include <array>
#include <functional>
#include <cstdint>
#include <memory>
#include <qarkui/input.h>
#include <qarkui/window.h>
#include <qohosdisplayinfo.h>
#include <qohosenums.h>
#include <qohosplugincore.h>
#include <qohospointerstyle.h>
#include <qohosutils.h>
#include <render/qohoswindowproxydatafactory.h>
#include <render/qxcomponent.h>
#include <set>

QT_BEGIN_NAMESPACE

class QOhosWindowProxy final
{
public:
    enum class WindowEventType
    {
        WINDOW_SHOWN,
        WINDOW_ACTIVE,
        WINDOW_INACTIVE,
        WINDOW_HIDDEN,
        WINDOW_DESTROYED,
    };

    enum class WindowStatusType
    {
        UNDEFINED,
        FULL_SCREEN,
        MAXIMIZE,
        MINIMIZE,
        FLOATING,
        SPLIT_SCREEN,
    };

    enum class AvoidAreaType
    {
        TYPE_SYSTEM,
        TYPE_CUTOUT,
        TYPE_SYSTEM_GESTURE,
        TYPE_KEYBOARD,
        TYPE_NAVIGATION_INDICATOR,
    };

    enum class RectChangeReason
    {
        UNDEFINED,
        MAXIMIZE,
        RECOVER,
        MOVE,
        DRAG,
        DRAG_START,
        DRAG_END,
    };

    enum class MaximizePresentation
    {
        FOLLOW_APP_IMMERSIVE_SETTING,
        EXIT_IMMERSIVE,
        ENTER_IMMERSIVE,
    };

    enum class ModalityType
    {
        WINDOW_MODALITY,
        APPLICATION_MODALITY,
    };

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
        QOhosOptional<double> minWidth;
        QOhosOptional<double> minHeight;
        QOhosOptional<double> maxWidth;
        QOhosOptional<double> maxHeight;
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
        QOhosOptional<QOhosDisplayInfo::JsDisplayId> displayId;
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
        QEventPoint::State state;
        QPointF displayPosition;
        QPointF globalPosition;
    };

    struct ShowWindowOptions
    {
        QOhosOptional<bool> focusOnShow;
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
        const WindowMask &windowMask, const QOhosOptional<QSize> &ohosMaskSizeOverride = {});
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
    void showAbility();
    bool tryHideAbility();
    void removeStartingWindow();
    bool getImmersiveModeEnabledState();
    QPixmap snapshot() const;
    bool startMoving();
    void enableDrag(bool enable);

    QOhosOptional<bool> isFocused() const;

    QArkUi::WindowProperties getWindowProperties() const;
    void setWindowCallbackReceiver(std::unique_ptr<WindowCallbacks> receiver);
    void setNonClientAreaMouseWindowCallbackReceiver(
        QObject *contextObject, QOhosConsumer<std::vector<NonClientAreaMouseEvent>> mouseEventBatchConsumer);
    void setNonClientAreaTouchWindowCallbackReceiver(
        QObject *contextObject, QOhosConsumer<std::vector<NonClientAreaTouchEvent>> touchEventBatchConsumer);

    std::shared_ptr<QOhosWindowProxy> createSubWindow(const SubWindowCreateInfo &createInfo);

    bool qtIsMainWindow() const;
    WindowProxyType windowProxyType() const;
    WindowLimits getWindowLimits() const;
    AvoidArea getWindowAvoidArea(AvoidAreaType type) const;
    std::shared_ptr<QXComponentNode> nodeXComponent() const;
    std::string qAbilityInstanceId() const;

    static std::vector<QArkUi::JsWindowId> queryWindowIdsByCoordinate(
        QOhosDisplayInfo::JsDisplayId displayId, const QPoint &queryLocation,
        std::uint32_t queryLimit = 0);
    static std::vector<QArkUi::JsWindowId> queryQtManagedWindowIdsByPredicate(
        const std::function<bool(QtOhos::JsState &jsState, const QArkUi::JsWindowRef &)> &predicate);

    QOhosOptional<QOhosDisplayInfo::JsDisplayId> tryGetMainWindowJsDisplayId() const;
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
            std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer);
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

namespace QtOhos
{

template<>
struct OhosEnumMeta<QOhosWindowProxy::WindowEventType>
{
    static constexpr const char *fullTypeName = "@ohos.window.WindowEventType";
    static constexpr std::array<std::pair<QOhosWindowProxy::WindowEventType, const char *>, 5> enumeratorsNames = {{
        {QOhosWindowProxy::WindowEventType::WINDOW_SHOWN, "WINDOW_SHOWN"},
        {QOhosWindowProxy::WindowEventType::WINDOW_ACTIVE, "WINDOW_ACTIVE"},
        {QOhosWindowProxy::WindowEventType::WINDOW_INACTIVE, "WINDOW_INACTIVE"},
        {QOhosWindowProxy::WindowEventType::WINDOW_HIDDEN, "WINDOW_HIDDEN"},
        {QOhosWindowProxy::WindowEventType::WINDOW_DESTROYED, "WINDOW_DESTROYED"},
    }};
};

template<>
struct OhosEnumMeta<QOhosWindowProxy::AvoidAreaType>
{
    static constexpr const char *fullTypeName = "@ohos.window.AvoidAreaType";
    static constexpr std::array<std::pair<QOhosWindowProxy::AvoidAreaType, const char *>, 5> enumeratorsNames = {{
        {QOhosWindowProxy::AvoidAreaType::TYPE_SYSTEM, "TYPE_SYSTEM"},
        {QOhosWindowProxy::AvoidAreaType::TYPE_CUTOUT, "TYPE_CUTOUT"},
        {QOhosWindowProxy::AvoidAreaType::TYPE_SYSTEM_GESTURE, "TYPE_SYSTEM_GESTURE"},
        {QOhosWindowProxy::AvoidAreaType::TYPE_KEYBOARD, "TYPE_KEYBOARD"},
        {QOhosWindowProxy::AvoidAreaType::TYPE_NAVIGATION_INDICATOR, "TYPE_NAVIGATION_INDICATOR"},
    }};
};

template <>
struct OhosEnumMeta<QOhosWindowProxy::RectChangeReason>
{
    static constexpr const char *fullTypeName = "@ohos.window.RectChangeReason";
    static constexpr std::array<std::pair<QOhosWindowProxy::RectChangeReason, const char*>, 7> enumeratorsNames = {{
        {QOhosWindowProxy::RectChangeReason::UNDEFINED, "UNDEFINED"},
        {QOhosWindowProxy::RectChangeReason::MAXIMIZE, "MAXIMIZE"},
        {QOhosWindowProxy::RectChangeReason::RECOVER, "RECOVER"},
        {QOhosWindowProxy::RectChangeReason::MOVE, "MOVE"},
        {QOhosWindowProxy::RectChangeReason::DRAG, "DRAG"},
        {QOhosWindowProxy::RectChangeReason::DRAG_START, "DRAG_START"},
        {QOhosWindowProxy::RectChangeReason::DRAG_END, "DRAG_END"},
    }};
};

template<>
struct OhosEnumMeta<QOhosWindowProxy::WindowStatusType>
{
    static constexpr const char *fullTypeName = "@ohos.window.WindowStatusType";
    static constexpr std::array<std::pair<QOhosWindowProxy::WindowStatusType, const char*>, 6> enumeratorsNames = {{
        {QOhosWindowProxy::WindowStatusType::UNDEFINED, "UNDEFINED"},
        {QOhosWindowProxy::WindowStatusType::FULL_SCREEN, "FULL_SCREEN"},
        {QOhosWindowProxy::WindowStatusType::MAXIMIZE, "MAXIMIZE"},
        {QOhosWindowProxy::WindowStatusType::MINIMIZE, "MINIMIZE"},
        {QOhosWindowProxy::WindowStatusType::FLOATING, "FLOATING"},
        {QOhosWindowProxy::WindowStatusType::SPLIT_SCREEN, "SPLIT_SCREEN"},
    }};
};

template<>
struct OhosEnumMeta<QOhosWindowProxy::MaximizePresentation>
{
    static constexpr const char *fullTypeName = "@ohos.window.MaximizePresentation";
    static constexpr std::array<std::pair<QOhosWindowProxy::MaximizePresentation, const char *>, 3> enumeratorsNames = {{
        {QOhosWindowProxy::MaximizePresentation::FOLLOW_APP_IMMERSIVE_SETTING, "FOLLOW_APP_IMMERSIVE_SETTING"},
        {QOhosWindowProxy::MaximizePresentation::EXIT_IMMERSIVE, "EXIT_IMMERSIVE"},
        {QOhosWindowProxy::MaximizePresentation::ENTER_IMMERSIVE, "ENTER_IMMERSIVE"},
    }};
};

template<>
struct OhosEnumMeta<QOhosWindowProxy::ModalityType>
{
    static constexpr const char *fullTypeName = "@ohos.window.ModalityType";
    static constexpr std::array<std::pair<QOhosWindowProxy::ModalityType, const char *>, 2> enumeratorsNames = {{
        {QOhosWindowProxy::ModalityType::WINDOW_MODALITY, "WINDOW_MODALITY"},
        {QOhosWindowProxy::ModalityType::APPLICATION_MODALITY, "APPLICATION_MODALITY"},
    }};
};

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QOhosWindowProxy::WindowEvent);
Q_DECLARE_METATYPE(QOhosWindowProxy::WindowStatus);
Q_DECLARE_METATYPE(QOhosWindowProxy::AvoidArea);
Q_DECLARE_METATYPE(QOhosWindowProxy::AvoidAreaType);
Q_DECLARE_METATYPE(QOhosWindowProxy::RectChangeOptions);

#endif
