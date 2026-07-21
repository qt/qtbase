// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSFLOATINGWINDOW_H
#define QOHOSFLOATINGWINDOW_H

#include <QtCore/qglobal.h>
#include <QtCore/qmap.h>
#include <napi.h>
#include <optional>
#include <qohosplatformscreen.h>
#include <qohosplatformwindow.h>
#include <render/qohossurface.h>
#include <render/qohosview.h>

QT_BEGIN_NAMESPACE

class QOhosFloatingWindow : public QOhosPlatformWindow
{
public:
    explicit QOhosFloatingWindow(QWindow *window);
    ~QOhosFloatingWindow() override;

    void initialize() override;
    void setGeometry(const QRect &rect) override;

    void setVisible(bool visible) override;

    WId winId() const override;

    void raise() override;
    void lower() override;

    QOhosSurface *ownedSurfaceOrNull() const override;
    QOhosView *ownedViewOrNull() const override;

    void setMask(const QRegion &region) override;
    bool startSystemMove() override;

    void requestActivateWindow() override;

protected:
    bool windowEvent(QEvent *event) override;

private:
    void tryAcquireNativeSurfaceIfNeeded();
    void restoreWindowCurrentCursorIfNeeded();
    void onWindowFlagsChanged(
        Qt::WindowFlags previousWindowFlags, Qt::WindowFlags currentWindowFlags) override;
    void onWindowStateChanged(
        Qt::WindowStates oldWindowState, Qt::WindowStates currentWindowState) override;

    void internalHijackSystemFocusAsPopup();
    void focusHijackingPopupHidden();
    void startAsyncWaitForNodeResizeIfNeeded();
    void handleNodeResizeEvent(const QArkUi::QQtEmbeddedWindowNode::NodeAreaInfo &areaChangeEvent);

    void handleWindowEvent(QOhosWindowProxy::WindowEvent evt);
    void handleWindowStatusChange(QOhosWindowProxy::WindowStatus evt);
    void handleWindowVisibilityChange(bool visible);
    void handleAvoidAreaChanged(QOhosWindowProxy::AvoidAreaType avoidAreaType,
                                const QOhosWindowProxy::AvoidArea &systemAvoidArea);
    void handleWindowRectChanged(const QOhosWindowProxy::RectChangeOptions &rectChangeOptions);
    void handleSurfaceStatusChanged(const std::optional<QSize> &optSurfaceSize);
    void handleWindowDisplayIdChanged(QOhosDisplayInfo::JsDisplayId displayId);

    std::unique_ptr<QOhosView> m_view;
    std::optional<QOhosWindowProxy::WindowEventType> m_lastWindowEventType;
    std::optional<QOhosWindowProxy::WindowStatusType> m_lastWindowStatusType;
    std::optional<QRegion> m_windowMask;
    std::optional<QSize> m_optLastSurfaceSize;
    QMap<QOhosWindowProxy::AvoidAreaType, QOhosWindowProxy::AvoidArea> m_avoidAreaCache;
    QBasicTimer m_geometryChangeTimer;
};

QT_END_NAMESPACE

#endif // QOHOSFLOATINGWINDOW_H
