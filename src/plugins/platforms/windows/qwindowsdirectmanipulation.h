// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWINDOWSDIRECTMANIPULATION_H
#define QWINDOWSDIRECTMANIPULATION_H

#include <QtCore/qt_windows.h>
#include <QtCore/private/qcomptr_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qsharedpointer.h>
#include <QtGui/qpointingdevice.h>

#include <directmanipulation.h>

QT_BEGIN_NAMESPACE

class QWindow;
class QWindowsDirectManipulationViewport;

/*!
    \internal

    Translates Windows Precision Touchpad gestures into Qt events, using the
    Direct Manipulation API.

    Windows reports touchpad gestures to legacy applications by synthesizing
    mouse wheel messages: two-finger scrolling arrives as WM_MOUSEWHEEL and
    WM_MOUSEHWHEEL with fine-grained deltas, and pinching arrives as
    Ctrl+WM_MOUSEWHEEL quantized to whole WHEEL_DELTA steps. Neither carries
    pixel deltas, gesture phases, or a real scale factor.

    Direct Manipulation provides all of those: it reports a continuous content
    transform (translation and scale) together with a state machine that
    distinguishes an ongoing manipulation from the inertia which follows it. We
    turn the translation into QWheelEvent with pixel deltas and scroll phases
    (matching what the Cocoa plugin delivers for a trackpad), and the scale
    into QNativeGestureEvent with Qt::ZoomNativeGesture, wrapped in
    Qt::BeginNativeGesture and Qt::EndNativeGesture (matching the XCB plugin).

    Handing a contact over to Direct Manipulation stops Windows from
    synthesizing the legacy wheel messages for it, so gestures are not
    delivered twice. A mouse wheel is unaffected and keeps arriving as
    WM_MOUSEWHEEL, and touch screens and pens are left to
    QWindowsPointerHandler.

    Rotation is not available: DIRECTMANIPULATION_CONFIGURATION has no rotation
    flag, and the content transform Direct Manipulation reports is always a
    uniform scale plus a translation. Multi-finger swipes are consumed by the
    shell (task view, virtual desktop switching) and never reach the
    application, so Qt::SwipeNativeGesture cannot be supported either.
*/
class QWindowsDirectManipulation
{
    Q_DISABLE_COPY_MOVE(QWindowsDirectManipulation)
public:
    using QPointingDevicePtr = QSharedPointer<QPointingDevice>;

    ~QWindowsDirectManipulation();

    // Returns nullptr if Direct Manipulation is unavailable.
    static QWindowsDirectManipulation *create();

    // Direct Manipulation viewports are per-window: the plugin creates one for
    // each window it creates, and destroys it along with the window.
    void registerWindow(QWindow *window, HWND hwnd);
    void unregisterWindow(HWND hwnd);

    // Handles DM_POINTERHITTEST, which Windows sends at the start of an
    // interaction over a window that has been activated for Direct
    // Manipulation. Returns true if the contact was handed over to Direct
    // Manipulation.
    bool handlePointerHitTest(HWND hwnd, WPARAM wParam);

    const QPointingDevicePtr &device();

private:
    friend class QWindowsDirectManipulationViewport;

    QWindowsDirectManipulation() = default;
    bool init();

    // In DIRECTMANIPULATION_VIEWPORT_OPTIONS_MANUALUPDATE mode Direct
    // Manipulation only produces content transforms when asked to, so an
    // active interaction has to be pumped. A viewport calls this when it is
    // given a contact; the pump stops itself once nothing is interacting.
    void startUpdates();
    void update();
    bool isInteracting() const;

    class UpdatePump;

    ComPtr<IDirectManipulationManager> m_manager;
    ComPtr<IDirectManipulationUpdateManager> m_updateManager;
    QHash<HWND, ComPtr<QWindowsDirectManipulationViewport>> m_viewports;
    QScopedPointer<UpdatePump> m_pump;
    QPointingDevicePtr m_device;
    int m_idleUpdates = 0;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECTMANIPULATION_H
