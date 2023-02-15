// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOANATIVEINTERFACE_H
#define QCOCOANATIVEINTERFACE_H

#include <ApplicationServices/ApplicationServices.h>

#include <qpa/qplatformnativeinterface.h>
#include <QtGui/qpixmap.h>
Q_MOC_INCLUDE(<QWindow>)

QT_BEGIN_NAMESPACE

class QWidget;
class QPrintEngine;
class QPlatformMenu;
class QPlatformMenuBar;

class QCocoaNativeInterface : public QPlatformNativeInterface
{
    Q_OBJECT
public:
    QCocoaNativeInterface();

    void *nativeResourceForWindow(const QByteArray &resourceString, QWindow *window) override;

    NativeResourceForIntegrationFunction nativeResourceFunctionForIntegration(const QByteArray &resource) override;

public Q_SLOTS:
    void onAppFocusWindowChanged(QWindow *window);

private:
    Q_INVOKABLE void clearCurrentThreadCocoaEventDispatcherInterruptFlag();

    static void registerDraggedTypes(const QStringList &types);

    // Set a QWindow as a "guest" (subwindow) of a non-QWindow
    static void setEmbeddedInForeignView(QPlatformWindow *window, bool embedded);

    // Register if a window should deliver touch events. Enabling
    // touch events has implications for delivery of other events,
    // for example by causing scrolling event lag.
    //
    // The registration is ref-counted: multiple widgets can enable
    // touch events, which then will be delivered until the widget
    // deregisters.
    static void registerTouchWindow(QWindow *window,  bool enable);

    // Set the size for a unified toolbar content border area.
    // Multiple callers can register areas and the platform plugin
    // will extend the "unified" area to cover them.
    static void registerContentBorderArea(QWindow *window, quintptr identifer, int upper, int lower);

    // Enables or disiables a content border area.
    static void setContentBorderAreaEnabled(QWindow *window, quintptr identifier, bool enable);

    // Returns true if the given coordinate is inside the current
    // content border.
    static bool testContentBorderPosition(QWindow *window, int position);
};

QT_END_NAMESPACE

#endif // QCOCOANATIVEINTERFACE_H

