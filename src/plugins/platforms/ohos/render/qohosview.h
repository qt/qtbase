// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSVIEW_H
#define QOHOSVIEW_H

#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qstring.h>
#include <QtGui/qimage.h>
#include <QtGui/qwindow.h>
#include <memory>
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
#include <utility>
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
        QOhosOptional<QOhosDisplayInfo::JsDisplayId> displayId;
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
    void setWindowShadowDisabled();
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

    ViewGeometry viewGeometry() const;
    QMargins avoidAreaMargins(QOhosWindowProxy::AvoidAreaType type) const;
    ViewType viewType() const;
    QOhosOptional<QSize> surfaceResolution() const;
    const QOhosView *viewParentOrNull() const;
    QRect nodeScreenGeometryPixels() const;
    QRect nodeParentRelativeGeometryPixels() const;

    ~QOhosView();

Q_SIGNALS:
    void windowEvent(QOhosWindowProxy::WindowEvent event);
    void windowStatusChange(QOhosWindowProxy::WindowStatus windowStatus);
    void windowVisibilityChange(bool visibility);
    void windowTouchOutside();
    void avoidAreaChanged(QOhosWindowProxy::AvoidAreaType avoidAreaType, const QOhosWindowProxy::AvoidArea &avoidArea);
    void windowRectChanged(QOhosWindowProxy::RectChangeOptions rectChangeOptions);
    void windowRectChangedInGlobalDisplay(QOhosWindowProxy::RectChangeOptions rectChangeOptions);
    void surfaceStatusChanged(const QOhosOptional<QSize> &optSurfaceSize);
    void windowDisplayIdChanged(QOhosDisplayInfo::JsDisplayId);
    void externalContentInteractionDetected();
    void nodeAreaChanged(QArkUi::QQtEmbeddedWindowNode::NodeAreaInfo areaChangeEvt);

private:
    void applyPhoneWindowChrome();

    static constexpr QOhosRuntimeDeviceTypeAndMode allModes =
        QOhosRuntimeDeviceTypeAndMode::_2in1
        | QOhosRuntimeDeviceTypeAndMode::HandheldDeviceFullScreen
        | QOhosRuntimeDeviceTypeAndMode::HandheldDeviceWindowPcMode;

    template <typename T, QOhosRuntimeDeviceTypeAndMode supportedModes = allModes>
    struct SystemUpdateDataProperty
    {
        QOhosOptional<T> optPendingUpdateRequest;
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
        WindowModeOnlySystemUpdateDataProperty<std::pair<QPoint, QOhosOptional<QOhosDisplayInfo::JsDisplayId>>> position;
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
    void updateWindowSize(const QSize &size);
    void updateWindowSizeLimits(const std::pair<QSize, QSize> &sizeLimits);
    void updateWindowPosition(const std::pair<QPoint, QOhosOptional<QOhosDisplayInfo::JsDisplayId>> &position);
    void updateWindowBackgroundTransparency(bool transparent);
    void updateWindowCursor(const QCursor &cursor);
    void updateWindowFocusable(bool focusable);
    void updateWindowBackgroundColor(const QColor &color);
    void updateWindowBrightness(int brightness);
    void updateWindowContrast(int contast);
    void updateWindowSaturation(int saturation);
    void updateWindowTransparentForInput(bool transparentForInput);
    void updateWindowModality(Qt::WindowModality modality);
    void updateWindowTitle(const QString &title);
    void updateWindowMinMaxCloseButtonsState(const WindowMinMaxCloseButtonsState &state);
    void updateWindowStaysOnTop(bool staysOnTop);
    void updateWindowFrameless(bool frameless);
    void syncWindowStateImmediate(WindowStateSyncReason reason = WindowStateSyncReason::Normal);
    void flushSystemPropertyUpdatesImmediate();
    void setOrResetWindowProxy(std::shared_ptr<QOhosWindowProxy> windowProxy, QWindow *optLogicalParent);
    const QOhosView *ancestorViewWithWindowOrNull() const;
    void hideMainWindow();
    void setWindowCornerRadius(double radius);
    void setPrivacyMode(bool privacyModeEnabled);
    void setBackgroundColor(const QColor &color);
    void setBrightness(int brightness);
    void setContrast(int contrast);
    void setSaturation(int saturation);
    void setWindowKeepScreenOn(bool keepScreenOn);
    void setFixedSizeStateEnabled(bool fixedSizeStateEnabled);
    void setWindowDragResizable(bool dragResizable);
    void showWindow();
    void sendAsyncSyntheticWindowActiveEvent();

    std::shared_ptr<QOhosWindowProxy> tryCreateWindowProxyIfNeeded(ViewType viewType, QWindow *optLogicalParent);

    bool isWindowTransparencyRequested() const;

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
    QPointer<QWindow> m_optLogicalParent;
    QOhosOptional<WindowHideMethod> m_lastMainWindowHideMethod;
    ViewGeometryPersistencePolicy m_geometryPersistencePolicy = ViewGeometryPersistencePolicy::Ignore;
    std::shared_ptr<void> m_windowPropertiesProviderCallbacksHandle;
    std::function<void()> m_optPostSurfaceDrawTask;
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
