// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef OHOSPLATFORMWINDOW_H
#define OHOSPLATFORMWINDOW_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qrect.h>
#include <functional>
#include <memory>
#include <qohosdisplayinfo.h>
#include <qohosinternalwindowid_p.h>
#include <qohosplugincore.h>
#include <qohoswindowproperty.h>
#include <qpa/qplatformwindow.h>
#include <render/qohossurface.h>

QT_BEGIN_NAMESPACE

class QOhosPlatformScreen;
class QOhosPlatformBackingStore;
class QOhosView;

class QOhosPlatformWindow: public QPlatformWindow
{
public:
    enum class DecorationPreset {
        Standard,
        Frameless,
    };

    enum class NativeNodeRenderFitPolicy
    {
        TopLeft,
        Fill,
    };

    static const QOhosPropertyDescriptor<QWindow *> subWindowOfTagProperty;
    static const QOhosPropertyDescriptor<bool> mainWindowTagProperty;
    static const QOhosPropertyDescriptor<bool> floatWindowTagProperty;
    static const QOhosPropertyDescriptor<double> windowCornerRadiusProperty;
    static const QOhosPropertyDescriptor<bool> windowPrivacyModeSettingProperty;
    static const QOhosPropertyDescriptor<QColor> surfaceBackgroundColorProperty;
    static const QOhosPropertyDescriptor<NativeNodeRenderFitPolicy> nativeNodeRenderFitPolicyHintProperty;
    static const QOhosPropertyDescriptor<bool> windowKeepScreenOnProperty;
    static const QOhosPropertyDescriptor<bool> windowDragResizableProperty;
    static const QOhosPropertyDescriptor<bool> windowFixedSizeStateProperty;
    static const QOhosPropertyDescriptor<int> windowBrightnessProperty;
    static const QOhosPropertyDescriptor<int> windowContrastProperty;
    static const QOhosPropertyDescriptor<int> windowSaturationProperty;

    static QOhosPlatformWindow* fromQWindow(QWindow *window);
    static QOhosPlatformWindow *fromQWindowOrNull(QWindow *window);
    static void tagWindowOrWidgetAsSubWindowOf(QObject *windowOrWidgetToTag, QWindow *targetMainWindow);
    static void tagWindowOrWidgetAsMainWindow(QObject *windowOrWidgetToTag, bool forceMainWindow);
    static void tagWindowOrWidgetAsFloatWindow(QObject *windowOrWidgetToTag, bool showAsFloatWindow);
    static void setWindowPrivacyMode(QObject *window, bool privacyModeEnabled);
    static void setWindowCornerRadius(QObject *windowOrWidget, double radius);
    static QWindow *getWindowOrWidgetAsSubWindowOfTagValue(QObject *windowOrWidget);
    static void setWindowOrWidgetNativeNodeRenderFitPolicyHint(QObject *windowOrWidget, NativeNodeRenderFitPolicy renderFitPolicy);
    static Qt::WindowFlags platformWindowFlagsForQWindow(QWindow *window);
    static void setSurfaceBackgroundColor(QObject *windowOrWidget, const QColor &color);
    static void setWindowKeepScreenOn(QObject *windowOrWidget, bool keepScreenOn);
    static void setWindowDragResizable(QObject *windowOrWidget, bool dragResizable);
    static void setBrightness(QObject *windowOrWidget, int brightness);
    static void setContrast(QObject *windowOrWidget, int contrast);
    static void setSaturation(QObject *windowOrWidget, int saturation);
    static void closeAllActivePopups();

    template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
    static void setWindowOrWidgetProperty(QObject *windowOrWidget, T propertyValue);

    template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
    static QOhosOptional<T> tryGetWindowOrWidgetProperty(QObject *windowOrWidget);

    static std::shared_ptr<void> setSurfaceConsumer(
        QWindow *targetWindow, QObject *surfaceConsumerContext,
        std::function<void(QOhosOptional<void *>)> surfaceConsumer);

    explicit QOhosPlatformWindow(QWindow *window);

    bool isWindowBeingClosedOrDestroyed(QWindow *window) const;
    void setVisible(bool visible) override;

    void setCursor(const QCursor &cursor);

    void setWindowTitle(const QString &title) override;

    void setParent(const QPlatformWindow *newParent) override;
    QPlatformScreen *screen() const override;

    void setWindowState(Qt::WindowStates state) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    Qt::WindowFlags windowFlags() const;

    void propagateSizeHints() override;
    inline bool isRaster() const {
        if (isForeignWindow())
            return false;

        return window()->surfaceType() == QSurface::RasterSurface;
    }
    bool isExposed() const final;

    void setGeometry(const QRect &rect) override;
    bool shouldDisplayAsOhosWindow() const;
    QtOhos::InternalWindowId internalWindowId() const;

    virtual QOhosSurface *ownedSurfaceOrNull() const;
    virtual QOhosView *ownedViewOrNull() const;

    DecorationPreset decorationPreset() const;
    QMargins frameMargins() const override;

    QWindow *validSubWindowOfTagValueOrNull() const;
    bool mainWindowTagValueOrFalse() const;
    bool floatWindowTagValueOrFalse() const;
    void initialize() override;
    Qt::WindowStates windowStates() const;
    void requestActivateWindow() override;
    void handleDpiChange();
    bool shouldShowWindowWithoutActivating() const;

    QPixmap makeSnapshot() const;

    bool setMouseGrabEnabled(bool grab) override;
    bool setKeyboardGrabEnabled(bool grab) override;

    QOhosOptional<QOhosDisplayInfo::JsDisplayId> tryTakeLastRequestedDisplayId();

protected:
    QOhosPlatformScreen *platformScreen() const;
    bool canBeShownOnScreen() const;

    void setWindowStateFromOhos(Qt::WindowStates state);
    void setWindowMarginsFromOhos(const QMargins &margins);
    void clearExposed();
    void setExposedRegionFromGeometry();
    void setDisplayIdFromOhos(QOhosOptional<QOhosDisplayInfo::JsDisplayId> displayId);
    void setWindowGeometryFromOhos(const QRect &nativeWindowDrawGeometry);
    void notifyWindowDestroyedFromOhos();
    bool checkWindowAcceptsFocus() const;
    bool checkWindowAcceptsInput() const;

    void notifyInputSystemsWindowActiveStatusChanged(bool active);
    QOhosPropertiesProvider propertiesProvider();

    virtual void onWindowFlagsChanged(
        Qt::WindowFlags previousWindowFlags,
        Qt::WindowFlags currentWindowFlags);
    virtual void onWindowStateChanged(
        Qt::WindowStates oldWindowState, Qt::WindowStates currentWindowState);

    bool windowEvent(QEvent *event) override;

    Qt::WindowFlags m_windowFlags;
    Qt::WindowStates m_windowState;
    QRegion m_lastExposedRegion;

    QtOhos::InternalWindowId m_windowId = QtOhos::InternalWindowId::invalidWindowId();
    QRect m_oldGeometry;
    std::unique_ptr<QMargins> m_optFrameMargins;
    QOhosOptional<QCursor> m_cursor;

private:
    void sendExposeUpdate();

    QPlatformWindow *m_parent {nullptr};
    QOhosOptional<QOhosDisplayInfo::JsDisplayId> m_displayId;
    QOhosOptional<QOhosDisplayInfo::JsDisplayId> m_lastRequestedDisplayId;
    QOhosPropertiesStore m_propertiesStore;
};

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
void QOhosPlatformWindow::setWindowOrWidgetProperty(QObject *windowOrWidget, T propertyValue)
{
    if (windowOrWidget == nullptr)
        qOhosReportFatalErrorAndAbort("%s: windowOrWidget is null", Q_FUNC_INFO);

    if (!windowOrWidget->isWindowType() && !windowOrWidget->isWidgetType()) {
        qOhosReportFatalErrorAndAbort(
            "%s: Invalid object - expected either window or widget, got %s",
            Q_FUNC_INFO,
            windowOrWidget->metaObject()->className());
    }

    setQOhosPropertyOnQObject<T, propertyPtr>(windowOrWidget, propertyValue);
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
QOhosOptional<T> QOhosPlatformWindow::tryGetWindowOrWidgetProperty(QObject *windowOrWidget)
{
    return tryGetQOhosPropertyFromQObject<T, propertyPtr>(windowOrWidget);
}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QOhosPlatformWindow::NativeNodeRenderFitPolicy));

#endif // OHOSPLATFORMWINDOW_H
