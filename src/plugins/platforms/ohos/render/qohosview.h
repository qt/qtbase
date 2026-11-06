// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSVIEW_H
#define QOHOSVIEW_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <QtCore/qharmonyjsobjectref.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qpointer.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qsize.h>
#include <QtCore/qstring.h>
#include <QtGui/qimage.h>
#include <QtGui/qwindow.h>
#include <memory>
#include <optional>
#include <qohosdisplayinfo.h>
#include <qohosforeignwindow.h>
#include <qohosplatformwindow.h>
#include <qohosplugincore.h>
#include <qohosruntimedevicetypeandmode.h>
#include <qohossettings.h>
#include <qohoswindowproperty.h>
#include <render/qnativenode.h>
#include <render/qohossurface.h>
#include <render/qohoswindowproxy.h>
#include <functional>
#include <vector>

QT_BEGIN_NAMESPACE

enum class ViewGeometryPersistencePolicy
{
    Disabled,
    Enabled,
    FollowSystemSetting,
    Ignore,
};

class QOhosView : public QObject
{
    Q_OBJECT

public:
    enum class ViewType
    {
        MainWindow,
        SubWindow,
        EmbeddedWindow,
        FloatWindow,
    };

    enum class WindowHideMethod {
        Minimize,
        HideAbility,
        NativeNodeVisibility,
    };

    struct SubWindowHacks
    {
        bool disableWindowFocusableBeforeLoadContent = false;
    };

    struct SubWindowOptions
    {
        bool decorEnabled;
        SubWindowHacks hacks;
    };

    struct ViewGeometry
    {
        std::optional<QOhosDisplayInfo::JsDisplayId> displayId;
        QRect frameGeometry;
        QRect geometry;
    };

    struct WindowMinMaxCloseButtonsState
    {
        bool maxButtonShown = true;
        bool minButtonShown = true;
        bool closeButtonShown = true;
        bool operator==(const WindowMinMaxCloseButtonsState &other) const;
        bool operator!=(const WindowMinMaxCloseButtonsState &other) const;
    };

    enum class WindowStateSyncReason
    {
        Normal,
        ViewTypeChanged,
    };

    static std::unique_ptr<QOhosView> createForWindow(
        QOhosPlatformWindow *window, QOhosPropertiesProvider windowPropertiesProvider);

    explicit QOhosView(
        QWindow *ownerWindow, QSharedPointer<QNativeNode> nativeNode,
        QOhosPropertiesProvider propertiesProvider);

    void setSizeLimits(const QSize &minSize, const QSize &maxSize);
    void setPosition(const QPoint &position);
    void setPositionOnScreenImmediate(const QPoint &position, QOhosDisplayInfo::JsDisplayId jsDisplayId);
    void setSize(const QSize &size);
    void setOpacity(qreal opacity);
    void setCursor(const QCursor &cursor);
    void setTransparentBackground(bool transparent);
    void setFocusable(bool focusable);
    void setWindowTransparentForInput(bool transparentForInput);
    void setModality(Qt::WindowModality modality);
    void setTitle(const QString &title);
    void raise();
    void lower();
    void setFullScreen();
    void recover();
    void minimize();
    void maximize();
    void setWindowMask(const QOhosWindowProxy::WindowMask &windowMask);
    void handleSurfaceContentsUpdated();
    void setParentOrReparent(QOhosView &parentView);
    void tryDetachFromEmbeddedParent();
    WId viewWindowId() const;
    void addForeignWindowChild(QOhosForeignWindow *foreignWindow);
    bool isFullscreenImmersiveModeEnabled();
    bool isSubWindowCoveringFullScreen() const;
    void setWindowMinMaxCloseButtonState(const WindowMinMaxCloseButtonsState &state);
    void setWindowStaysOnTop(bool staysOnTop);
    void setFramelessWindow(bool frameless);
    void setNativeNodeVisibility(bool visible);
    QPixmap makeSnapshot() const;
    bool startMoving();
    void startDrag(
        const std::vector<QImage> &images, const QPointF &hotspot,
        const QMimeData &mimeData, QOhosConsumer<Qt::DropAction> dropActionConsumer);
    void restoreMainWindow();
    void handleWindowStateChange(
        Qt::WindowStates previousWindowState, Qt::WindowStates currentWindowState);
    void handleWindowFlagsChange(
        Qt::WindowFlags previousWindowFlags, Qt::WindowFlags currentWindowFlags);
    void handlePaletteChange();
    QArkUi::QQtEmbeddedWindowNode::NodeAreaInfo nodeAreaInfo() const;

    void showImmediate();
    void hide();
    void requestActivate();
    void forceGeometryUpdate();

    QWindow *ownerWindow() const;
    QOhosSurface *surfaceOrNull() const;
    QHarmonyJsObjectRef abilityObjectRef() const;
    QHarmonyJsObjectRef jsWindowObjectRef() const;

    ViewGeometry viewGeometry() const;
    QMargins avoidAreaMargins(QOhosWindowProxy::AvoidAreaType type) const;
    QList<QRect> displayCutoutRects() const;
    ViewType viewType() const;
    std::optional<QSize> surfaceResolution() const;
    const QOhosView *viewParentOrNull() const;
    QRect nodeScreenGeometryPixels() const;
    QRect nodeParentRelativeGeometryPixels() const;
    bool consumePendingSelfMinimizeEcho();

    ~QOhosView();

Q_SIGNALS:
    void windowEvent(QOhosWindowProxy::WindowEvent event);
    void windowStatusChange(QOhosWindowProxy::WindowStatus windowStatus);
    void windowVisibilityChange(bool visibility);
    void windowTouchOutside();
    void avoidAreaChanged(QOhosWindowProxy::AvoidAreaType avoidAreaType, const QOhosWindowProxy::AvoidArea &avoidArea);
    void windowRectChanged(QOhosWindowProxy::RectChangeOptions rectChangeOptions);
    void windowRectChangedInGlobalDisplay(QOhosWindowProxy::RectChangeOptions rectChangeOptions);
    void surfaceStatusChanged(const std::optional<QSize> &optSurfaceSize);
    void windowDisplayIdChanged(QOhosDisplayInfo::JsDisplayId);
    void externalContentInteractionDetected();
    void nodeAreaChanged(QArkUi::QQtEmbeddedWindowNode::NodeAreaInfo areaChangeEvt);

private:
    using JsCallBatch = QtOhos::JsCallBatch;

    void applyPhoneWindowChrome(JsCallBatch &jsCallBatch);

    static constexpr QOhosRuntimeDeviceTypeAndMode allModes =
        QOhosRuntimeDeviceTypeAndMode::_2in1
        | QOhosRuntimeDeviceTypeAndMode::HandheldDeviceFullScreen
        | QOhosRuntimeDeviceTypeAndMode::HandheldDeviceWindowPcMode;

    template <typename T, QOhosRuntimeDeviceTypeAndMode supportedModes = allModes>
    struct SystemUpdateDataProperty
    {
        std::optional<T> optPendingUpdateRequest;
    };

    template<typename T>
    using NoTabletSystemUpdateDataProperty = SystemUpdateDataProperty<T, QOhosRuntimeDeviceTypeAndMode::_2in1>;

    template<typename T>
    using WindowModeOnlySystemUpdateDataProperty = SystemUpdateDataProperty<
        T, QOhosRuntimeDeviceTypeAndMode::_2in1 | QOhosRuntimeDeviceTypeAndMode::HandheldDeviceWindowPcMode>;

    struct SystemUpdateData
    {
        WindowModeOnlySystemUpdateDataProperty<QSize> size;
        WindowModeOnlySystemUpdateDataProperty<std::pair<QSize, QSize>> sizeLimits;
        WindowModeOnlySystemUpdateDataProperty<std::pair<QPoint, std::optional<QOhosDisplayInfo::JsDisplayId>>> position;
        SystemUpdateDataProperty<bool> visibility;
        NoTabletSystemUpdateDataProperty<bool> backgroundTransparent;
        SystemUpdateDataProperty<QCursor> cursor;
        NoTabletSystemUpdateDataProperty<bool> focusable;
        SystemUpdateDataProperty<QColor> backgroundColor;
        SystemUpdateDataProperty<int> brightness;
        SystemUpdateDataProperty<int> contrast;
        SystemUpdateDataProperty<int> saturation;
        SystemUpdateDataProperty<bool> windowTransparentForInput;
        WindowModeOnlySystemUpdateDataProperty<Qt::WindowModality> modality;
        WindowModeOnlySystemUpdateDataProperty<QString> title;
        WindowModeOnlySystemUpdateDataProperty<WindowMinMaxCloseButtonsState> windowMinMaxCloseButtonsState;
        WindowModeOnlySystemUpdateDataProperty<bool> windowStaysOnTop;
        WindowModeOnlySystemUpdateDataProperty<bool> frameless;
    };

    static constexpr auto makeSystemUpdateDataPropertyUpdateFuncPairsTuple()
    {
        return std::make_tuple(
            std::make_pair(&SystemUpdateData::sizeLimits, &QOhosView::updateWindowSizeLimits),
            std::make_pair(&SystemUpdateData::size, &QOhosView::updateWindowSize),
            std::make_pair(&SystemUpdateData::position, &QOhosView::updateWindowPosition),
            std::make_pair(&SystemUpdateData::backgroundTransparent, &QOhosView::updateWindowBackgroundTransparency),
            std::make_pair(&SystemUpdateData::cursor, &QOhosView::updateWindowCursor),
            std::make_pair(&SystemUpdateData::focusable, &QOhosView::updateWindowFocusable),
            std::make_pair(&SystemUpdateData::backgroundColor, &QOhosView::updateWindowBackgroundColor),
            std::make_pair(&SystemUpdateData::brightness, &QOhosView::updateWindowBrightness),
            std::make_pair(&SystemUpdateData::contrast, &QOhosView::updateWindowContrast),
            std::make_pair(&SystemUpdateData::saturation, &QOhosView::updateWindowSaturation),
            std::make_pair(&SystemUpdateData::windowTransparentForInput, &QOhosView::updateWindowTransparentForInput),
            std::make_pair(&SystemUpdateData::modality, &QOhosView::updateWindowModality),
            std::make_pair(&SystemUpdateData::title, &QOhosView::updateWindowTitle),
            std::make_pair(&SystemUpdateData::windowMinMaxCloseButtonsState, &QOhosView::updateWindowMinMaxCloseButtonsState),
            std::make_pair(&SystemUpdateData::windowStaysOnTop, &QOhosView::updateWindowStaysOnTop),
            std::make_pair(&SystemUpdateData::frameless, &QOhosView::updateWindowFrameless));
    };

    template <typename T, QOhosRuntimeDeviceTypeAndMode SupportedModes>
    void setSystemUpdateProperty(SystemUpdateDataProperty<T, SupportedModes> SystemUpdateData::*property, const T &value);

    void scheduleSystemUpdateIfNeeded();
    void updateWindowSize(const QSize &size, JsCallBatch &jsCallBatch);
    void updateWindowSizeLimits(const std::pair<QSize, QSize> &sizeLimits, JsCallBatch &jsCallBatch);
    void updateWindowPosition(
        const std::pair<QPoint, std::optional<QOhosDisplayInfo::JsDisplayId>> &position, JsCallBatch &jsCallBatch);
    void updateWindowBackgroundTransparency(bool transparent, JsCallBatch &jsCallBatch);
    void updateWindowCursor(const QCursor &cursor, JsCallBatch &jsCallBatch);
    void updateWindowFocusable(bool focusable, JsCallBatch &jsCallBatch);
    void updateWindowBackgroundColor(const QColor &color, JsCallBatch &jsCallBatch);
    void updateWindowBrightness(int brightness, JsCallBatch &jsCallBatch);
    void updateWindowContrast(int contast, JsCallBatch &jsCallBatch);
    void updateWindowSaturation(int saturation, JsCallBatch &jsCallBatch);
    void updateWindowTransparentForInput(bool transparentForInput, JsCallBatch &jsCallBatch);
    void updateWindowModality(Qt::WindowModality modality, JsCallBatch &jsCallBatch);
    void updateWindowTitle(const QString &title, JsCallBatch &jsCallBatch);
    void updateWindowMinMaxCloseButtonsState(const WindowMinMaxCloseButtonsState &state, JsCallBatch &jsCallBatch);
    void updateWindowStaysOnTop(bool staysOnTop, JsCallBatch &jsCallBatch);
    void updateWindowFrameless(bool frameless, JsCallBatch &jsCallBatch);
    void updateWindowShadowDisabled(JsCallBatch &jsCallBatch);
    void updateMainWindowRestored(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void syncWindowStateImmediate(WindowStateSyncReason reason = WindowStateSyncReason::Normal);
    void flushSystemPropertyUpdatesImmediate(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    QOhosWindowProxy *windowProxyAfterPropertyUpdatesOrNull(JsCallBatch &jsCallBatch);
    void updateWindowFullScreen(JsCallBatch &jsCallBatch);
    void updateWindowMaximized(JsCallBatch &jsCallBatch);
    void setOrResetWindowProxy(std::shared_ptr<QOhosWindowProxy> windowProxy, QWindow *optLogicalParent);
    const QOhosView *ancestorViewWithWindowOrNull() const;
    void hideMainWindow();
    void setWindowCornerRadius(double radius, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setPrivacyMode(bool privacyModeEnabled, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setBackgroundColor(const QColor &color);
    void setBrightness(int brightness);
    void setContrast(int contrast);
    void setSaturation(int saturation);
    void setWindowKeepScreenOn(bool keepScreenOn, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setFixedSizeStateEnabled(bool fixedSizeStateEnabled, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setWindowDragResizable(bool dragResizable, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void showWindow(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void sendAsyncSyntheticWindowActiveEvent();

    std::shared_ptr<QOhosWindowProxy> tryCreateWindowProxyIfNeeded(ViewType viewType, QWindow *optLogicalParent);

    bool isWindowTransparencyRequested() const;

    bool isFixedSizeStateEnabled() const;
    bool isMaximizeRequestSupportedOrLogWarning() const;

    QOhosPropertiesProvider m_windowPropertiesProvider;
    QPointer<QWindow> m_ownerWindow;
    std::shared_ptr<QOhosWindowProxy> m_ohosWindowProxy;
    QSharedPointer<QNativeNode> m_nativeNode;
    SystemUpdateData m_updateData;
    bool m_updatePending;
    std::function<QOhosWindowProxy::AvoidArea(QOhosWindowProxy::AvoidAreaType)> m_avoidAreasProvider;
    bool m_requireHandheldDeviceSupport;
    QtOhos::InternalWindowId m_ownerWindowId;
    bool m_windowDestroyed = false;
    std::optional<ViewType> m_viewType;
    QPointer<QWindow> m_optLogicalParent;
    std::optional<WindowHideMethod> m_lastMainWindowHideMethod;
    ViewGeometryPersistencePolicy m_geometryPersistencePolicy = ViewGeometryPersistencePolicy::Ignore;
    std::shared_ptr<void> m_windowPropertiesProviderCallbacksHandle;
    std::function<void()> m_optPostSurfaceDrawTask;
    int m_pendingSelfMinimizeEchoes = 0;
};

template <typename T, QOhosRuntimeDeviceTypeAndMode SupportedModes>
void QOhosView::setSystemUpdateProperty(SystemUpdateDataProperty<T, SupportedModes> SystemUpdateData::*propertyMemberPtr, const T &value)
{
    QFlags<QOhosRuntimeDeviceTypeAndMode> supportedModes(SupportedModes);

    if (m_requireHandheldDeviceSupport
        && !supportedModes.testFlag(QOhosRuntimeDeviceTypeAndMode::HandheldDeviceWindowPcMode)
        && !supportedModes.testFlag(QOhosRuntimeDeviceTypeAndMode::HandheldDeviceFullScreen)) {
        return;
    }

    bool windowModeOnly =
        !supportedModes.testFlag(QOhosRuntimeDeviceTypeAndMode::HandheldDeviceFullScreen)
        && (supportedModes.testFlag(QOhosRuntimeDeviceTypeAndMode::_2in1)
            || supportedModes.testFlag(QOhosRuntimeDeviceTypeAndMode::HandheldDeviceWindowPcMode));

    if (windowModeOnly && !QOhosSettings::instance().isWindowPcModeEnabled()
        && viewType() == ViewType::MainWindow) {
        return;
    }

    auto &property = m_updateData.*propertyMemberPtr;
    if (property.optPendingUpdateRequest != value) {
        property.optPendingUpdateRequest = value;
        scheduleSystemUpdateIfNeeded();
    }
}

QT_END_NAMESPACE

#endif
