// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>
#include <QuartzCore/QuartzCore.h>

#include "qcocoawindow.h"
#include "qcocoaintegration.h"
#include "qcocoascreen.h"
#include "qnswindowdelegate.h"
#include "qcocoaeventdispatcher.h"
#ifndef QT_NO_OPENGL
#include "qcocoaglcontext.h"
#endif
#include "qcocoahelpers.h"
#include "qcocoanativeinterface.h"
#include "qnsview.h"
#include "qnswindow.h"
#include <QtCore/qfileinfo.h>
#include <QtCore/private/qcore_mac_p.h>
#include <qwindow.h>
#include <private/qwindow_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformscreen.h>
#include <QtGui/private/qcoregraphics_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/private/qmetallayer_p.h>

#include <QDebug>

#include <vector>

QT_BEGIN_NAMESPACE

enum {
    defaultWindowWidth = 160,
    defaultWindowHeight = 160
};

Q_LOGGING_CATEGORY(lcCocoaNotifications, "qt.qpa.cocoa.notifications");

static void qRegisterNotificationCallbacks()
{
    static const QLatin1StringView notificationHandlerPrefix(Q_NOTIFICATION_PREFIX);

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];

    const QMetaObject *metaObject = QMetaType(qRegisterMetaType<QCocoaWindow*>()).metaObject();
    Q_ASSERT(metaObject);

    for (int i = 0; i < metaObject->methodCount(); ++i) {
        QMetaMethod method = metaObject->method(i);
        const QString methodTag = QString::fromLatin1(method.tag());
        if (!methodTag.startsWith(notificationHandlerPrefix))
            continue;

        const QString notificationName = methodTag.mid(notificationHandlerPrefix.size());
        [center addObserverForName:notificationName.toNSString() object:nil queue:nil
            usingBlock:^(NSNotification *notification) {

            QVarLengthArray<QCocoaWindow *, 32> cocoaWindows;
            if ([notification.object isKindOfClass:[NSWindow class]]) {
                NSWindow *nsWindow = notification.object;
                for (const QWindow *window : QGuiApplication::allWindows()) {
                    if (QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle()))
                        if (cocoaWindow->nativeWindow() == nsWindow)
                            cocoaWindows += cocoaWindow;
                }
            } else if ([notification.object isKindOfClass:[NSView class]]) {
                if (QNSView *qnsView = qnsview_cast(notification.object))
                    cocoaWindows += qnsView.platformWindow;
            } else {
                qCWarning(lcCocoaNotifications) << "Unhandled notification"
                    << notification.name << "for" << notification.object;
                return;
            }

            if (lcCocoaNotifications().isDebugEnabled() && !cocoaWindows.isEmpty()) {
                QVector<QCocoaWindow *> debugWindows;
                for (QCocoaWindow *cocoaWindow : cocoaWindows)
                    debugWindows += cocoaWindow;
                qCDebug(lcCocoaNotifications) << "Forwarding" << qPrintable(notificationName) <<
                    "to" << debugWindows;
            }

            // FIXME: Could be a foreign window, look up by iterating top level QWindows

            for (QCocoaWindow *cocoaWindow : cocoaWindows) {
                if (!method.invoke(cocoaWindow, Qt::DirectConnection)) {
                    qCWarning(lcQpaWindow) << "Failed to invoke NSNotification callback for"
                        << notification.name << "on" << cocoaWindow;
                }
            }
        }];
    }
}
Q_CONSTRUCTOR_FUNCTION(qRegisterNotificationCallbacks)

const int QCocoaWindow::NoAlertRequest = -1;
QPointer<QCocoaWindow> QCocoaWindow::s_windowUnderMouse;

QCocoaWindow::QCocoaWindow(QWindow *win, WId nativeHandle) : QPlatformWindow(win)
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::QCocoaWindow" << window();

    if (nativeHandle) {
        m_view = reinterpret_cast<NSView *>(nativeHandle);
        [m_view retain];
    }
}

void QCocoaWindow::initialize()
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::initialize" << window();

    QMacAutoReleasePool pool;

    if (!m_view)
        m_view = [[QNSView alloc] initWithCocoaWindow:this];

    if (!isForeignWindow()) {
        // Compute the initial geometry based on the geometry set on the
        // QWindow. This geometry has already been reflected to the
        // QPlatformWindow in the constructor, so to ensure that the
        // resulting setGeometry call does not think the geometry has
        // already been applied, we reset the QPlatformWindow's view
        // of the geometry first.
        auto initialGeometry = QPlatformWindow::initialGeometry(window(),
            windowGeometry(), defaultWindowWidth, defaultWindowHeight);
        QPlatformWindow::d_ptr->rect = QRect();
        setGeometry(initialGeometry);

        setMask(QHighDpi::toNativeLocalRegion(window()->mask(), window()));

        m_safeAreaInsetsObserver = QMacKeyValueObserver(
            m_view, @"safeAreaInsets", [this] {
                // Defer to next runloop pass, so that any changes to the
                // margins during resizing have settled down.
                QMetaObject::invokeMethod(this, [this]{
                    updateSafeAreaMarginsIfNeeded();
                }, Qt::QueuedConnection);
            }, NSKeyValueObservingOptionNew);

    } else {
        // Pick up essential foreign window state
        QPlatformWindow::setGeometry(QRectF::fromCGRect(m_view.frame).toRect());
    }

    recreateWindowIfNeeded();

    m_initialized = true;
}

const NSNotificationName QCocoaWindowWillReleaseQNSViewNotification = @"QCocoaWindowWillReleaseQNSViewNotification";

QCocoaWindow::~QCocoaWindow()
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::~QCocoaWindow" << window();

    QMacAutoReleasePool pool;
    [m_nsWindow makeFirstResponder:nil];
    [m_nsWindow setContentView:nil];

    m_safeAreaInsetsObserver = {};

    // Remove from superview only if we have a Qt window parent,
    // as we don't want to affect window container foreign windows.
    if (QPlatformWindow::parent())
        [m_view removeFromSuperview];

    // Make sure to disconnect observer in all case if view is valid
    // to avoid notifications received when deleting when using Qt::AA_NativeWindows attribute
    if (!isForeignWindow())
        [[NSNotificationCenter defaultCenter] removeObserver:m_view];

#if QT_CONFIG(vulkan)
    if (QCocoaIntegration *cocoaIntegration = QCocoaIntegration::instance()) {
        auto vulcanInstance = cocoaIntegration->getCocoaVulkanInstance();
        if (vulcanInstance)
            vulcanInstance->destroySurface(m_vulkanSurface);
    }
#endif

    // Must send notification before calling release, as doing it from
    // [QNSView dealloc] would mean that any weak references to the view
    // would already return nil.
    [NSNotificationCenter.defaultCenter
        postNotificationName:QCocoaWindowWillReleaseQNSViewNotification
        object:m_view];

    [m_view release];
    [m_nsWindow closeAndRelease];

    // Disposing of the view and window should have resulted in an
    // expose event with isExposed=false, but just in case we try
    // to stop the display link here as well.
    static_cast<QCocoaScreen *>(screen())->maybeStopDisplayLink();
}

QSurfaceFormat QCocoaWindow::format() const
{
    auto format = window()->requestedFormat();
    if (auto *view = qnsview_cast(m_view); view.colorSpace) {
        auto colorSpace = QColorSpace::fromIccProfile(QByteArray::fromNSData(view.colorSpace.ICCProfileData));
        if (!colorSpace.isValid()) {
            qCWarning(lcQpaWindow) << "Failed to parse ICC profile for" << view.colorSpace
                                   << "with ICC data" << view.colorSpace.ICCProfileData;
        }
        format.setColorSpace(colorSpace);
    }
    return format;
}

void QCocoaWindow::setGeometry(const QRect &rectIn)
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::setGeometry" << window() << rectIn;

    QScopedValueRollback inSetGeometry(m_inSetGeometry, true);

    QRect rect = rectIn;
    // This means it is a call from QWindow::setFramePosition() and
    // the coordinates include the frame (size is still the contents rectangle).
    if (qt_window_private(const_cast<QWindow *>(window()))->positionPolicy
            == QWindowPrivate::WindowFrameInclusive) {
        const QMargins margins = frameMargins();
        rect.moveTopLeft(rect.topLeft() + QPoint(margins.left(), margins.top()));
    }

    setCocoaGeometry(rect);
}

bool QCocoaWindow::isForeignWindow() const
{
    return ![m_view isKindOfClass:[QNSView class]];
}

QRect QCocoaWindow::geometry() const
{
    // QWindows that are embedded in a NSView hierarchy may be considered
    // top-level from Qt's point of view but are not from Cocoa's point
    // of view. Embedded QWindows get global (screen) geometry.
    if (isEmbedded()) {
        NSPoint windowPoint = [m_view convertPoint:NSMakePoint(0, 0) toView:nil];
        NSRect screenRect = [[m_view window] convertRectToScreen:NSMakeRect(windowPoint.x, windowPoint.y, 1, 1)];
        NSPoint screenPoint = screenRect.origin;
        QPoint position = QCocoaScreen::mapFromNative(screenPoint).toPoint();
        QSize size = QRectF::fromCGRect(NSRectToCGRect([m_view bounds])).toRect().size();
        return QRect(position, size);
    }

    return QPlatformWindow::geometry();
}

/*!
    \brief the geometry of the window as it will appear when shown as
    a normal (not maximized or full screen) top-level window.

    For child windows this property always holds an empty rectangle.

    \sa QWidget::normalGeometry()
*/
QRect QCocoaWindow::normalGeometry() const
{
    if (!isContentView())
        return QRect();

    // We only persist the normal the geometry when going into
    // fullscreen and maximized states. For all other cases we
    // can just report the geometry as is.

    if (!(windowState() & (Qt::WindowFullScreen | Qt::WindowMaximized)))
        return geometry();

    return m_normalGeometry;
}

void QCocoaWindow::updateNormalGeometry()
{
    if (!isContentView())
        return;

    if (windowState() != Qt::WindowNoState)
        return;

    m_normalGeometry = geometry();
}

void QCocoaWindow::setCocoaGeometry(const QRect &rect)
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::setCocoaGeometry" << window() << rect;
    QMacAutoReleasePool pool;

    QPlatformWindow::setGeometry(rect);

    if (isEmbedded()) {
        if (!isForeignWindow()) {
            [m_view setFrame:NSMakeRect(0, 0, rect.width(), rect.height())];
        }
        return;
    }

    if (isContentView()) {
        NSRect bounds = QCocoaScreen::mapToNative(rect);
        [m_view.window setFrame:[m_view.window frameRectForContentRect:bounds] display:YES animate:NO];
    } else {
        [m_view setFrame:NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height())];
    }

    // will call QPlatformWindow::setGeometry(rect) during resize confirmation (see qnsview.mm)
}

QMargins QCocoaWindow::safeAreaMargins() const
{
    // The safe area of the view reflects the area not covered by navigation
    // bars, tab bars, toolbars, and other ancestor views that might obscure
    // the current view (by setting additionalSafeAreaInsets). If the window
    // uses NSWindowStyleMaskFullSizeContentView this also includes the area
    // of the view covered by the title bar.
    QMarginsF viewSafeAreaMargins = {
        m_view.safeAreaInsets.left,
        m_view.safeAreaInsets.top,
        m_view.safeAreaInsets.right,
        m_view.safeAreaInsets.bottom
    };

    // The screen's safe area insets represent the distances from the screen's
    // edges at which content isn't obscured. The view's safe area margins do
    // not include the screen's insets automatically, so we need to manually
    // merge them.
    auto screenRect = m_view.window.screen.frame;
    auto screenInsets = m_view.window.screen.safeAreaInsets;
    auto screenRelativeViewBounds = QCocoaScreen::mapFromNative(
        [m_view.window convertRectToScreen:
            [m_view convertRect:m_view.bounds toView:nil]]
    );

    // The margins are relative to the screen the window is on.
    // Note that we do not want represent the area outside of the
    // screen as being outside of the safe area.
    QMarginsF screenSafeAreaMargins = {
        screenInsets.left ?
            qMax(0.0f, screenInsets.left - screenRelativeViewBounds.left())
            : 0.0f,
        screenInsets.top ?
            qMax(0.0f, screenInsets.top - screenRelativeViewBounds.top())
            : 0.0f,
        screenInsets.right ?
            qMax(0.0f, screenInsets.right
                - (screenRect.size.width - screenRelativeViewBounds.right()))
            : 0.0f,
        screenInsets.bottom ?
            qMax(0.0f, screenInsets.bottom
                - (screenRect.size.height - screenRelativeViewBounds.bottom()))
            : 0.0f
    };

    return (screenSafeAreaMargins | viewSafeAreaMargins).toMargins();
}

void QCocoaWindow::updateSafeAreaMarginsIfNeeded()
{
    if (safeAreaMargins() != m_lastReportedSafeAreaMargins) {
        m_lastReportedSafeAreaMargins = safeAreaMargins();
        QWindowSystemInterface::handleSafeAreaMarginsChanged(window());
    }
}

bool QCocoaWindow::startSystemMove()
{
    switch (NSApp.currentEvent.type) {
    case NSEventTypeLeftMouseDown:
    case NSEventTypeRightMouseDown:
    case NSEventTypeOtherMouseDown:
    case NSEventTypeMouseMoved:
    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDragged:
        // The documentation only describes starting a system move
        // based on mouse down events, but move events also work.
        [m_view.window performWindowDragWithEvent:NSApp.currentEvent];
        return true;
    default:
        return false;
    }
}

void QCocoaWindow::setVisible(bool visible)
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::setVisible" << window() << visible;

    // Our implementation of setVisible below is not idempotent, as for
    // modal windows it calls beginSheet/endSheet or starts/ends modal
    // sessions. However we can't simply guard for m_view.hidden already
    // having the right state, as the behavior of this function differs
    // based on whether the window has been initialized or not, as
    // handleGeometryChange will bail out if the window is still
    // initializing. Since we know we'll get a second setVisible
    // call after creation, we can check for that case specifically,
    // which means we can then safely guard on m_view.hidden changing.

    if (!m_initialized) {
        qCDebug(lcQpaWindow) << "Window still initializing, skipping setting visibility";
        return; // We'll get another setVisible call after create is done
    }

    if (visible == !m_view.hidden && (!isContentView() || visible == m_view.window.visible)) {
        qCDebug(lcQpaWindow) << "No change in visible status. Ignoring.";
        return;
    }

    if (m_inSetVisible) {
        qCWarning(lcQpaWindow) << "Already setting window visible!";
        return;
    }

    QScopedValueRollback<bool> rollback(m_inSetVisible, true);

    QMacAutoReleasePool pool;
    QCocoaWindow *parentCocoaWindow = nullptr;
    if (window()->transientParent())
        parentCocoaWindow = static_cast<QCocoaWindow *>(window()->transientParent()->handle());

    auto eventDispatcher = [] {
        return static_cast<QCocoaEventDispatcherPrivate *>(QObjectPrivate::get(qApp->eventDispatcher()));
    };

    if (visible) {
        // We need to recreate if the modality has changed as the style mask will need updating
        recreateWindowIfNeeded();

        // We didn't send geometry changes during creation, as that would have confused
        // Qt, which expects a show-event to be sent before any resize events. But now
        // that the window is made visible, we know that the show-event has been sent
        // so we can send the geometry change. FIXME: Get rid of this workaround.
        handleGeometryChange();

        if (parentCocoaWindow) {
            // The parent window might have moved while this window was hidden,
            // update the window geometry if there is a parent.
            setGeometry(windowGeometry());

            if (window()->type() == Qt::Popup) {
                // QTBUG-30266: a window should not be resizable while a transient popup is open
                // Since this isn't a native popup, the window manager doesn't close the popup when you click outside
                NSWindow *nativeParentWindow = parentCocoaWindow->nativeWindow();
                NSUInteger parentStyleMask = nativeParentWindow.styleMask;
                if ((m_resizableTransientParent = (parentStyleMask & NSWindowStyleMaskResizable))
                    && !(nativeParentWindow.styleMask & NSWindowStyleMaskFullScreen))
                    nativeParentWindow.styleMask &= ~NSWindowStyleMaskResizable;
            }

        }

        // Make the NSView visible first, before showing the NSWindow (in case of top level windows)
        m_view.hidden = NO;

        if (isContentView()) {
            QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);

            // setWindowState might have been called while the window was hidden and
            // will not change the NSWindow state in that case. Sync up here:
            applyWindowState(window()->windowStates());

            if (window()->windowState() != Qt::WindowMinimized) {
                if (parentCocoaWindow && (window()->modality() == Qt::WindowModal || window()->type() == Qt::Sheet)) {
                    // Show the window as a sheet
                    NSWindow *nativeParentWindow = parentCocoaWindow->nativeWindow();
                    if (!nativeParentWindow.attachedSheet)
                        [nativeParentWindow beginSheet:m_view.window completionHandler:nil];
                    else
                        [nativeParentWindow beginCriticalSheet:m_view.window completionHandler:nil];
                } else if (window()->modality() == Qt::ApplicationModal) {
                    // Show the window as application modal
                    eventDispatcher()->beginModalSession(window());
                } else if (m_view.window.canBecomeKeyWindow) {
                    bool shouldBecomeKeyNow = !NSApp.modalWindow
                                              || m_view.window.worksWhenModal
                                              || !NSApp.modalWindow.visible;

                    // Panels with becomesKeyOnlyIfNeeded set should not activate until a view
                    // with needsPanelToBecomeKey, for example a line edit, is clicked.
                    if ([m_view.window isKindOfClass:[NSPanel class]])
                        shouldBecomeKeyNow &= !(static_cast<NSPanel*>(m_view.window).becomesKeyOnlyIfNeeded);

                    if (shouldBecomeKeyNow)
                        [m_view.window makeKeyAndOrderFront:nil];
                    else
                        [m_view.window orderFront:nil];
                } else {
                    [m_view.window orderFront:nil];
                }
            }
        }
    } else {
        // Window not visible, hide it
        if (isContentView()) {
            if (eventDispatcher()->hasModalSession())
                eventDispatcher()->endModalSession(window());
            else if ([m_view.window isSheet])
                [m_view.window.sheetParent endSheet:m_view.window];

            // Note: We do not guard the order out by checking NSWindow.visible, as AppKit will
            // in some cases, such as when hiding the application, order out and make a window
            // invisible, but keep it in a list of "hidden windows", that it then restores again
            // when the application is unhidden. We need to call orderOut explicitly, to bring
            // the window out of this "hidden list".
            [m_view.window orderOut:nil];

            if (m_view.window == [NSApp keyWindow] && !eventDispatcher()->hasModalSession()) {
                // Probably because we call runModalSession: outside [NSApp run] in QCocoaEventDispatcher
                // (e.g., when show()-ing a modal QDialog instead of exec()-ing it), it can happen that
                // the current NSWindow is still key after being ordered out. Then, after checking we
                // don't have any other modal session left, it's safe to make the main window key again.
                NSWindow *mainWindow = [NSApp mainWindow];
                if (mainWindow && [mainWindow canBecomeKeyWindow])
                    [mainWindow makeKeyWindow];
            }
        }

        // AppKit will in some cases set up the key view loop for child views, even if we
        // don't set autorecalculatesKeyViewLoop, nor call recalculateKeyViewLoop ourselves.
        // When a child window is promoted to a top level, AppKit will maintain the key view
        // loop between the views, even if these views now cross NSWindows, even after we
        // explicitly call recalculateKeyViewLoop. When the top level is then hidden, AppKit
        // will complain when -[NSView _setHidden:setNeedsDisplay:] tries to transfer first
        // responder by reading the nextValidKeyView, and it turns out to live in a different
        // window. We mitigate this by a last second reset of the first responder, which is
        // what AppKit also falls back to. It's unclear if the original situation of views
        // having their nextKeyView pointing to views in other windows is kosher or not.
        if (m_view.window.firstResponder == m_view && m_view.nextValidKeyView
            && m_view.nextValidKeyView.window != m_view.window) {
            qCDebug(lcQpaWindow) << "Detected nextValidKeyView" << m_view.nextValidKeyView
                << "in different window" << m_view.nextValidKeyView.window
                << "Resetting" << m_view.window << "first responder to nil.";
            [m_view.window makeFirstResponder:nil];
        }

        m_view.hidden = YES;

        if (parentCocoaWindow && window()->type() == Qt::Popup) {
            NSWindow *nativeParentWindow = parentCocoaWindow->nativeWindow();
            if (m_resizableTransientParent
                && !(nativeParentWindow.styleMask & NSWindowStyleMaskFullScreen))
                // A window should not be resizable while a transient popup is open
                nativeParentWindow.styleMask |= NSWindowStyleMaskResizable;
        }
    }
}

NSInteger QCocoaWindow::windowLevel(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    NSInteger windowLevel = NSNormalWindowLevel;

    if (type == Qt::Tool)
        windowLevel = NSFloatingWindowLevel;
    else if ((type & Qt::Popup) == Qt::Popup)
        windowLevel = NSPopUpMenuWindowLevel;

    // StayOnTop window should appear above Tool windows.
    if (flags & Qt::WindowStaysOnTopHint)
        windowLevel = NSModalPanelWindowLevel;
    // Tooltips should appear above StayOnTop windows.
    if (type == Qt::ToolTip)
        windowLevel = NSScreenSaverWindowLevel;

    auto *transientParent = window()->transientParent();
    if (transientParent && transientParent->handle()) {
        // We try to keep windows in at least the same window level as
        // their transient parent. Unfortunately this only works when the
        // window is created. If the window level changes after that, as
        // a result of a call to setWindowFlags, or by changing the level
        // of the native window, we will not pick this up, and the window
        // will be left behind (or in a different window level than) its
        // parent. We could KVO-observe the window level of our transient
        // parent, but that requires us to know when the parent goes away
        // so that we can unregister the observation before the parent is
        // dealloced, something we can't do for generic NSWindows. Another
        // way would be to override [NSWindow setLevel:] and notify child
        // windows about the change, but that doesn't work for foreign
        // windows, which can still be transient parents via fromWinId().
        // One area where this problem is apparent is when AppKit tweaks
        // the window level of modal windows during application activation
        // and deactivation. Since we don't pick up on these window level
        // changes in a generic way, we need to add logic explicitly to
        // re-evaluate the window level after AppKit has done its tweaks.

        auto *transientCocoaWindow = static_cast<QCocoaWindow *>(transientParent->handle());
        auto *nsWindow = transientCocoaWindow->nativeWindow();

        // We only upgrade the window level for "special" windows, to work
        // around Qt Widgets Designer parenting the designer windows to the widget
        // palette window (QTBUG-31779). This should be fixed in designer.
        if (type != Qt::Window)
            windowLevel = qMax(windowLevel, nsWindow.level);
    }

    return windowLevel;
}

NSUInteger QCocoaWindow::windowStyleMask(Qt::WindowFlags flags)
{
    const Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    // Determine initial style mask based on whether the window should
    // have a frame and title or not. The NSWindowStyleMaskBorderless
    // and NSWindowStyleMaskTitled styles are mutually exclusive, with
    // values of 0 and 1 correspondingly.
    NSUInteger styleMask = [&]{
        // Honor explicit requests for borderless windows
        if (flags & Qt::FramelessWindowHint)
            return NSWindowStyleMaskBorderless;

        // Popup windows should always be borderless
        if (windowIsPopupType(type))
            return NSWindowStyleMaskBorderless;

        if (flags & Qt::CustomizeWindowHint) {
            // CustomizeWindowHint turns off the default window title hints,
            // so the choice is then up to the user via Qt::WindowTitleHint.
            return flags & Qt::WindowTitleHint
                ? NSWindowStyleMaskTitled
                : NSWindowStyleMaskBorderless;
        } else {
            // Otherwise, default to using titled windows
            return NSWindowStyleMaskTitled;
        }
    }();

    // We determine which buttons to show in updateTitleBarButtons,
    // so we can enable all the relevant style masks here to ensure
    // that behaviors that don't involve the title bar buttons are
    // working (for example minimizing frameless windows, or resizing
    // windows that don't have zoom or fullscreen titlebar buttons).
    styleMask |= NSWindowStyleMaskClosable
               | NSWindowStyleMaskMiniaturizable;

    if (type != Qt::Popup) // We only care about popups exactly.
        styleMask |= NSWindowStyleMaskResizable;

    if (type == Qt::Tool)
        styleMask |= NSWindowStyleMaskUtilityWindow;

    // FIXME (QTBUG-138829)
    if (m_drawContentBorderGradient)
        styleMask |= QT_IGNORE_DEPRECATIONS(NSWindowStyleMaskTexturedBackground);

    if (flags & Qt::ExpandedClientAreaHint)
        styleMask |= NSWindowStyleMaskFullSizeContentView;

    // Don't wipe existing states for style flags we don't control here
    styleMask |= (m_view.window.styleMask & (
          NSWindowStyleMaskFullScreen
        | NSWindowStyleMaskUnifiedTitleAndToolbar
        | NSWindowStyleMaskDocModalWindow
        | NSWindowStyleMaskNonactivatingPanel
        | NSWindowStyleMaskHUDWindow));

    return styleMask;
}

bool QCocoaWindow::isFixedSize() const
{
    return windowMinimumSize().isValid() && windowMaximumSize().isValid()
        && windowMinimumSize() == windowMaximumSize();
}

void QCocoaWindow::updateTitleBarButtons(Qt::WindowFlags windowFlags)
{
    if (!isContentView())
        return;

    static constexpr std::pair<NSWindowButton, Qt::WindowFlags> buttons[] = {
        { NSWindowCloseButton, Qt::WindowCloseButtonHint },
        { NSWindowMiniaturizeButton, Qt::WindowMinimizeButtonHint},
        { NSWindowZoomButton, Qt::WindowMaximizeButtonHint | Qt::WindowFullscreenButtonHint }
    };

    bool hideButtons = true;
    for (const auto &[button, buttonHint] : buttons) {
        // Set up Qt defaults based on window type
        bool enabled = true;
        if (button == NSWindowMiniaturizeButton)
            enabled = window()->type() != Qt::Dialog;

        // Let users override via CustomizeWindowHint
        if (windowFlags & Qt::CustomizeWindowHint)
            enabled = windowFlags & buttonHint;

        // Then do some final sanitizations

        if (button == NSWindowZoomButton && isFixedSize())
            enabled = false;

        // Mimic what macOS natively does for parent windows of modal
        // sheets, which is to disable the close button, but leave the
        // other buttons as they were.
        if (button == NSWindowCloseButton && enabled
            && QWindowPrivate::get(window())->blockedByModalWindow) {
            enabled = false;
            // If we end up having no enabled buttons, our workaround
            // should not be a reason for hiding all of them.
            hideButtons = false;
        }

        [m_view.window standardWindowButton:button].enabled = enabled;
        hideButtons &= !enabled;
    }

    // Hide buttons in case we disabled all of them
    for (const auto &[button, buttonHint] : buttons)
        [m_view.window standardWindowButton:button].hidden = hideButtons;
}

void QCocoaWindow::setWindowFlags(Qt::WindowFlags flags)
{
    // Updating the window flags may affect the window's theme frame, which
    // in the process retains and then autoreleases the NSWindow. To make
    // sure this doesn't leave lingering releases when there is no pool in
    // place (e.g. during main(), before exec), we add one locally here.
    QMacAutoReleasePool pool;

    if (!isContentView())
        return;

    // While setting style mask we can have handleGeometryChange calls on a content
    // view with null geometry, reporting an invalid coordinates as a result.
    m_inSetStyleMask = true;
    m_view.window.styleMask = windowStyleMask(flags);
    m_inSetStyleMask = false;

    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));
    if ((type & Qt::Popup) != Qt::Popup && (type & Qt::Dialog) != Qt::Dialog) {
        NSWindowCollectionBehavior behavior = m_view.window.collectionBehavior;
        const bool enableFullScreen = m_view.window.qt_fullScreen
                                    || !(flags & Qt::CustomizeWindowHint)
                                    || (flags & Qt::WindowFullscreenButtonHint);
        if (enableFullScreen) {
            behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
            behavior &= ~NSWindowCollectionBehaviorFullScreenAuxiliary;
        } else {
            behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
            behavior &= ~NSWindowCollectionBehaviorFullScreenPrimary;
        }
        m_view.window.collectionBehavior = behavior;
    }

    // Set styleMask and collectionBehavior before applying window level, as
    // the window level change will trigger verification of the two properties.
    m_view.window.level = this->windowLevel(flags);

    m_view.window.hasShadow = !(flags & Qt::NoDropShadowWindowHint);

    if (!(flags & Qt::FramelessWindowHint))
        setWindowTitle(window()->title());

    updateTitleBarButtons(flags);

    // Make window ignore mouse events if WindowTransparentForInput is set.
    // Note that ignoresMouseEvents has a special initial state where events
    // are ignored (passed through) based on window transparency, and that
    // setting the property to false does not return us to that state. Instead,
    // this makes the window capture all mouse events. Take care to only
    // set the property if needed. FIXME: recreate window if needed or find
    // some other way to implement WindowTransparentForInput.
    bool ignoreMouse = flags & Qt::WindowTransparentForInput;
    if (m_view.window.ignoresMouseEvents != ignoreMouse)
        m_view.window.ignoresMouseEvents = ignoreMouse;

    // FIXME (QTBUG-138829)
    m_view.window.titlebarAppearsTransparent = (flags & Qt::NoTitleBarBackgroundHint)
        || (m_view.window.styleMask & QT_IGNORE_DEPRECATIONS(NSWindowStyleMaskTexturedBackground));
}

// ----------------------- Window state -----------------------

/*!
    Changes the state of the NSWindow, going in/out of minimize/zoomed/fullscreen

    When this is called from QWindow::setWindowState(), the QWindow state has not been
    updated yet, so window()->windowState() will reflect the previous state that was
    reported to QtGui.
*/
void QCocoaWindow::setWindowState(Qt::WindowStates state)
{
    if (window()->isVisible())
        applyWindowState(state); // Window state set for hidden windows take effect when show() is called
}

void QCocoaWindow::applyWindowState(Qt::WindowStates requestedState)
{
    if (!isContentView())
        return;

    const Qt::WindowState currentState = QWindowPrivate::effectiveState(windowState());
    const Qt::WindowState newState = QWindowPrivate::effectiveState(requestedState);

    if (newState == currentState)
        return;

    qCDebug(lcQpaWindow) << "Applying" << newState << "to" << window() << "in" << currentState;

    const NSSize contentSize = m_view.frame.size;
    if (contentSize.width <= 0 || contentSize.height <= 0) {
        // If content view width or height is 0 then the window animations will crash so
        // do nothing. We report the current state back to reflect the failed operation.
        qWarning("invalid window content view size, check your window geometry");
        handleWindowStateChanged(HandleUnconditionally);
        return;
    }

    const NSWindow *nsWindow = m_view.window;

    if (nsWindow.styleMask & NSWindowStyleMaskUtilityWindow
        && newState & (Qt::WindowMinimized | Qt::WindowFullScreen)) {
        qWarning() << window()->type() << "windows cannot be made" << newState;
        handleWindowStateChanged(HandleUnconditionally);
        return;
    }

    const id sender = nsWindow;

    // First we need to exit states that can't transition directly to other states
    switch (currentState) {
    case Qt::WindowMinimized:
        [nsWindow deminiaturize:sender];
        // Deminiaturizing is not synchronous, so we need to wait for the
        // NSWindowDidMiniaturizeNotification before continuing to apply
        // the new state.
        return;
    case Qt::WindowFullScreen: {
        toggleFullScreen();
        // Exiting fullscreen is not synchronous, so we need to wait for the
        // NSWindowDidExitFullScreenNotification before continuing to apply
        // the new state.
        return;
    }
    default:;
    }

    // Then we apply the new state if needed
    if (newState == windowState())
        return;

    switch (newState) {
    case Qt::WindowFullScreen:
        toggleFullScreen();
        break;
    case Qt::WindowMaximized:
        toggleMaximized();
        break;
    case Qt::WindowMinimized:
        [nsWindow miniaturize:sender];
        break;
    case Qt::WindowNoState:
        if (windowState() == Qt::WindowMaximized)
            toggleMaximized();
        break;
    default:
        Q_UNREACHABLE();
    }
}

Qt::WindowStates QCocoaWindow::windowState() const
{
    Qt::WindowStates states = Qt::WindowNoState;
    NSWindow *window = m_view.window;

    if (window.miniaturized)
        states |= Qt::WindowMinimized;

    // Full screen and maximized are mutually exclusive, as macOS
    // will report a full screen window as zoomed.
    if (window.qt_fullScreen) {
        states |= Qt::WindowFullScreen;
    } else if ((window.zoomed && !isTransitioningToFullScreen())
        || (m_lastReportedWindowState == Qt::WindowMaximized && isTransitioningToFullScreen())) {
        states |= Qt::WindowMaximized;
    }

    // Note: We do not report Qt::WindowActive, even if isActive()
    // is true, as QtGui does not expect this window state to be set.

    return states;
}

void QCocoaWindow::toggleMaximized()
{
    const NSWindow *window = m_view.window;

    // The NSWindow needs to be resizable, otherwise the window will
    // not be possible to zoom back to non-zoomed state.
    const bool wasResizable = window.styleMask & NSWindowStyleMaskResizable;
    window.styleMask |= NSWindowStyleMaskResizable;

    const id sender = window;
    [window zoom:sender];

    if (!wasResizable)
        window.styleMask &= ~NSWindowStyleMaskResizable;
}

void QCocoaWindow::windowWillZoom()
{
    updateNormalGeometry();
}

void QCocoaWindow::toggleFullScreen()
{
    const NSWindow *window = m_view.window;

    // The window needs to have the correct collection behavior for the
    // toggleFullScreen call to have an effect. The collection behavior
    // will be reset in windowDidEnterFullScreen/windowDidLeaveFullScreen.
    window.collectionBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;

    const id sender = window;
    [window toggleFullScreen:sender];
}

void QCocoaWindow::windowWillEnterFullScreen()
{
    if (!isContentView())
        return;

    updateNormalGeometry();

    // The NSWindow needs to be resizable, otherwise we'll end up with
    // the normal window geometry, centered in the middle of the screen
    // on a black background. The styleMask will be reset below.
    m_view.window.styleMask |= NSWindowStyleMaskResizable;
}

bool QCocoaWindow::isTransitioningToFullScreen() const
{
    NSWindow *window = m_view.window;
    return window.styleMask & NSWindowStyleMaskFullScreen && !window.qt_fullScreen;
}

void QCocoaWindow::windowDidEnterFullScreen()
{
    if (!isContentView())
        return;

    Q_ASSERT_X(m_view.window.qt_fullScreen, "QCocoaWindow",
        "FullScreen category processes window notifications first");

    // Reset to original styleMask
    setWindowFlags(window()->flags());

    handleWindowStateChanged();
}

void QCocoaWindow::windowWillExitFullScreen()
{
    if (!isContentView())
        return;

    // The NSWindow needs to be resizable, otherwise we'll end up with
    // a weird zoom animation. The styleMask will be reset below.
    m_view.window.styleMask |= NSWindowStyleMaskResizable;
}

void QCocoaWindow::windowDidExitFullScreen()
{
    if (!isContentView())
        return;

    Q_ASSERT_X(!m_view.window.qt_fullScreen, "QCocoaWindow",
        "FullScreen category processes window notifications first");

    // Reset to original styleMask
    setWindowFlags(window()->flags());

    Qt::WindowState requestedState = window()->windowState();

    // Deliver update of QWindow state
    handleWindowStateChanged();

    if (requestedState != windowState() && requestedState != Qt::WindowFullScreen) {
        // We were only going out of full screen as an intermediate step before
        // progressing into the final step, so re-sync the desired state.
       applyWindowState(requestedState);
    }
}

void QCocoaWindow::windowDidMiniaturize()
{
    if (!isContentView())
        return;

    handleWindowStateChanged();
}

void QCocoaWindow::windowDidDeminiaturize()
{
    if (!isContentView())
        return;

    Qt::WindowState requestedState = window()->windowState();

    handleWindowStateChanged();

    if (requestedState != windowState() && requestedState != Qt::WindowMinimized) {
        // We were only going out of minimized as an intermediate step before
        // progressing into the final step, so re-sync the desired state.
       applyWindowState(requestedState);
    }
}

void QCocoaWindow::handleWindowStateChanged(HandleFlags flags)
{
    Qt::WindowStates currentState = windowState();
    if (!(flags & HandleUnconditionally) && currentState == m_lastReportedWindowState)
        return;

    qCDebug(lcQpaWindow) << "QCocoaWindow::handleWindowStateChanged" <<
        m_lastReportedWindowState << "-->" << currentState;

    QWindowSystemInterface::handleWindowStateChanged<QWindowSystemInterface::SynchronousDelivery>(
        window(), currentState, m_lastReportedWindowState);
    m_lastReportedWindowState = currentState;
}

// ------------------------------------------------------------

void QCocoaWindow::setWindowTitle(const QString &title)
{
    if (!isContentView())
        return;

    QMacAutoReleasePool pool;
    m_view.window.title = title.toNSString();

    if (title.isEmpty() && !window()->filePath().isEmpty()) {
        // Clearing the title should restore the default filename
        setWindowFilePath(window()->filePath());
    }
}

void QCocoaWindow::setWindowFilePath(const QString &filePath)
{
    if (!isContentView())
        return;

    QMacAutoReleasePool pool;

    if (window()->title().isNull())
        [m_view.window setTitleWithRepresentedFilename:filePath.toNSString()];
    else
        m_view.window.representedFilename = filePath.toNSString();

    // Changing the file path may affect icon visibility
    setWindowIcon(window()->icon());
}

void QCocoaWindow::setWindowIcon(const QIcon &icon)
{
    if (!isContentView())
        return;

    NSButton *iconButton = [m_view.window standardWindowButton:NSWindowDocumentIconButton];
    if (!iconButton) {
        // Window icons are only supported on macOS in combination with a document filePath
        return;
    }

    QMacAutoReleasePool pool;

    if (icon.isNull()) {
        iconButton.image = [NSWorkspace.sharedWorkspace iconForFile:m_view.window.representedFilename];
    } else {
        // Fall back to a size that looks good on the highest resolution screen available
        auto fallbackSize = iconButton.frame.size.height * qGuiApp->devicePixelRatio();
        iconButton.image = [NSImage imageFromQIcon:icon withSize:fallbackSize];
    }
}

void QCocoaWindow::setAlertState(bool enabled)
{
    if (m_alertRequest == NoAlertRequest && enabled) {
        m_alertRequest = [NSApp requestUserAttention:NSCriticalRequest];
    } else if (m_alertRequest != NoAlertRequest && !enabled) {
        [NSApp cancelUserAttentionRequest:m_alertRequest];
        m_alertRequest = NoAlertRequest;
    }
}

bool QCocoaWindow::isAlertState() const
{
    return m_alertRequest != NoAlertRequest;
}

void QCocoaWindow::raise()
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::raise" << window();

    // ### handle spaces (see Qt 4 raise_sys in qwidget_mac.mm)
    if (isContentView()) {
        if (m_view.window.visible) {
            {
                // Clean up auto-released temp objects from orderFront immediately.
                // Failure to do so has been observed to cause leaks also beyond any outer
                // autorelease pool (for example around a complete QWindow
                // construct-show-raise-hide-delete cycle), counter to expected autoreleasepool
                // behavior.
                QMacAutoReleasePool pool;
                [m_view.window orderFront:m_view.window];
            }
            static bool raiseProcess = qt_mac_resolveOption(true, "QT_MAC_SET_RAISE_PROCESS");
            if (raiseProcess)
                [NSApp activateIgnoringOtherApps:YES];
        }
    } else {
        [m_view.superview addSubview:m_view positioned:NSWindowAbove relativeTo:nil];
    }
}

void QCocoaWindow::lower()
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::lower" << window();

    if (isContentView()) {
        if (m_view.window.visible)
            [m_view.window orderBack:m_view.window];
    } else {
        [m_view.superview addSubview:m_view positioned:NSWindowBelow relativeTo:nil];
    }
}

bool QCocoaWindow::isExposed() const
{
    return !m_exposedRect.isEmpty();
}

bool QCocoaWindow::isEmbedded() const
{
    // Child QWindows are not embedded
    if (window()->parent())
        return false;

    // Top-level QWindows with non-Qt NSWindows are embedded
    if (m_view.window)
        return !([m_view.window isKindOfClass:[QNSWindow class]] ||
                 [m_view.window isKindOfClass:[QNSPanel class]]);

    // The window has no QWindow parent but also no NSWindow,
    // conservatively reuturn false.
    return false;
}

bool QCocoaWindow::isOpaque() const
{
    // OpenGL surfaces can be ordered either above(default) or below the NSWindow.
    // When ordering below the window must be tranclucent.
    static GLint openglSourfaceOrder = qt_mac_resolveOption(1, "QT_MAC_OPENGL_SURFACE_ORDER");

    bool translucent = window()->format().alphaBufferSize() > 0
                        || window()->opacity() < 1
                        || !window()->mask().isEmpty()
                        || (surface()->supportsOpenGL() && openglSourfaceOrder == -1);
    return !translucent;
}

void QCocoaWindow::propagateSizeHints()
{
    QMacAutoReleasePool pool;
    if (!isContentView())
        return;

    qCDebug(lcQpaWindow) << "QCocoaWindow::propagateSizeHints" << window()
                              << "min:" << windowMinimumSize() << "max:" << windowMaximumSize()
                              << "increment:" << windowSizeIncrement()
                              << "base:" << windowBaseSize();

    const NSWindow *window = m_view.window;

    // Set the minimum content size.
    QSize minimumSize = windowMinimumSize();
    if (!minimumSize.isValid()) // minimumSize is (-1, -1) when not set. Make that (0, 0) for Cocoa.
        minimumSize = QSize(0, 0);
    window.contentMinSize = NSSizeFromCGSize(minimumSize.toCGSize());

    // Set the maximum content size.
    window.contentMaxSize = NSSizeFromCGSize(windowMaximumSize().toCGSize());

    // The window may end up with a fixed size; in this case the zoom button should be disabled.
    updateTitleBarButtons(this->window()->flags());

    // sizeIncrement is observed to take values of (-1, -1) and (0, 0) for windows that should be
    // resizable and that have no specific size increment set. Cocoa expects (1.0, 1.0) in this case.
    QSize sizeIncrement = windowSizeIncrement();
    if (sizeIncrement.isEmpty())
        sizeIncrement = QSize(1, 1);
    window.resizeIncrements = NSSizeFromCGSize(sizeIncrement.toCGSize());

    QRect rect = geometry();
    QSize baseSize = windowBaseSize();
    if (!baseSize.isNull() && baseSize.isValid())
        [window setFrame:NSMakeRect(rect.x(), rect.y(), baseSize.width(), baseSize.height()) display:YES];
}

void QCocoaWindow::setOpacity(qreal level)
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::setOpacity" << level;
    if (!isContentView())
        return;

    m_view.window.alphaValue = level;
}

void QCocoaWindow::setMask(const QRegion &region)
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::setMask" << window() << region;

    if (!region.isEmpty()) {
        QCFType<CGMutablePathRef> maskPath = CGPathCreateMutable();
        for (const QRect &r : region)
            CGPathAddRect(maskPath, nullptr, r.toCGRect());
        CAShapeLayer *maskLayer = [CAShapeLayer layer];
        maskLayer.path = maskPath;
        m_view.layer.mask = maskLayer;
    } else {
        m_view.layer.mask = nil;
    }
}

bool QCocoaWindow::setKeyboardGrabEnabled(bool)
{
    return false; // FIXME (QTBUG-106597)
}

bool QCocoaWindow::setMouseGrabEnabled(bool)
{
    return false; // FIXME (QTBUG-106597)
}

WId QCocoaWindow::winId() const
{
    return WId(m_view);
}

void QCocoaWindow::setParent(const QPlatformWindow *parentWindow)
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::setParent" << window() << (parentWindow ? parentWindow->window() : 0);

    // Recreate in case we need to get rid of a NSWindow, or create one
    recreateWindowIfNeeded();

    setCocoaGeometry(geometry());
}

NSView *QCocoaWindow::view() const
{
    return m_view;
}

NSWindow *QCocoaWindow::nativeWindow() const
{
    return m_view.window;
}

void QCocoaWindow::setEmbeddedInForeignView()
{
    // Release any previously created NSWindow.
    [m_nsWindow closeAndRelease];
    m_nsWindow = 0;
}

// ----------------------- NSView notifications -----------------------

void QCocoaWindow::viewDidChangeFrame()
{
    // Note: When the view is the content view, it would seem redundant
    // to deliver geometry changes both from windowDidResize and this
    // callback, but in some cases such as when macOS native tabbed
    // windows are enabled we may end up with the wrong geometry in
    // the initial windowDidResize callback when a new tab is created.
    handleGeometryChange();
}

/*!
    Callback for NSViewGlobalFrameDidChangeNotification.

    Posted whenever an NSView object that has attached surfaces (that is,
    NSOpenGLContext objects) moves to a different screen, or other cases
    where the NSOpenGLContext object needs to be updated.
*/
void QCocoaWindow::viewDidChangeGlobalFrame()
{
    [m_view setNeedsDisplay:YES];
}

// ----------------------- NSWindow notifications -----------------------

// Note: The following notifications are delivered to every QCocoaWindow
// that is a child of the NSWindow that triggered the notification. Each
// callback should make sure to filter out notifications if they do not
// apply to that QCocoaWindow, e.g. if the window is not a content view.

void QCocoaWindow::windowDidMove()
{
    if (!isContentView())
        return;

    handleGeometryChange();

    // Moving a window might bring it out of maximized state
    handleWindowStateChanged();
}

void QCocoaWindow::windowDidResize()
{
    if (!isContentView())
        return;

    handleGeometryChange();

    if (!m_view.inLiveResize)
        handleWindowStateChanged();
}

void QCocoaWindow::windowWillStartLiveResize()
{
    // Track live resizing for all windows, including
    // child windows, so we know if it's safe to update
    // the window unthrottled outside of the main thread.
    m_inLiveResize = true;
}

bool QCocoaWindow::allowsIndependentThreadedRendering() const
{
    // Use member variable to track this instead of reflecting
    // NSView.inLiveResize directly, so it can be called from
    // non-main threads.
    return !m_inLiveResize;
}

void QCocoaWindow::windowDidEndLiveResize()
{
    m_inLiveResize = false;

    if (!isContentView())
        return;

    handleWindowStateChanged();
}

void QCocoaWindow::windowDidBecomeKey()
{
    // The NSWindow we're part of become key. Check if we're the first
    // responder, and if so, deliver focus window change to our window.
    if (m_view.window.firstResponder != m_view)
        return;

    qCDebug(lcQpaWindow) << m_view.window << "became key window."
        << "Updating focus window to" << this << "with view" << m_view;

    if (windowIsPopupType()) {
        qCDebug(lcQpaWindow) << "Window is popup. Skipping focus window change.";
        return;
    }

    // See also [QNSView becomeFirstResponder]
    QWindowSystemInterface::handleFocusWindowChanged<QWindowSystemInterface::SynchronousDelivery>(
                window(), Qt::ActiveWindowFocusReason);
}

void QCocoaWindow::windowDidResignKey()
{
    // The NSWindow we're part of lost key. Check if we're the first
    // responder, and if so, deliver window deactivation to our window.
    if (m_view.window.firstResponder != m_view)
        return;

    qCDebug(lcQpaWindow) << m_view.window << "resigned key window."
        << "Clearing focus window" << this << "with view" << m_view;

    // Make sure popups are closed before we deliver activation changes, which are
    // otherwise ignored by QApplication.
    closeAllPopups();

    // The current key window will be non-nil if another window became key. If that
    // window is a Qt window, we delay the window activation event until the didBecomeKey
    // notification is delivered to the active window, to ensure an atomic update.
    NSWindow *newKeyWindow = [NSApp keyWindow];
    if (newKeyWindow && newKeyWindow != m_view.window
        && [newKeyWindow conformsToProtocol:@protocol(QNSWindowProtocol)]) {
        qCDebug(lcQpaWindow) << "New key window" << newKeyWindow
            << "is Qt window. Deferring focus window change.";
        return;
    }

    // Lost key window, go ahead and set the active window to zero
    if (!windowIsPopupType()) {
        QWindowSystemInterface::handleFocusWindowChanged<QWindowSystemInterface::SynchronousDelivery>(
            nullptr, Qt::ActiveWindowFocusReason);
    }
}

void QCocoaWindow::windowDidOrderOnScreen()
{
    // The current mouse window needs to get a leave event when a popup window opens.
    // For modal dialogs, QGuiApplicationPrivate::showModalWindow takes care of this.
    if (QWindowPrivate::get(window())->isPopup()) {
        QWindowSystemInterface::handleLeaveEvent<QWindowSystemInterface::SynchronousDelivery>
            (QGuiApplicationPrivate::currentMouseWindow);
    }

    [m_view setNeedsDisplay:YES];
}

void QCocoaWindow::windowDidOrderOffScreen()
{
    handleExposeEvent(QRegion());
    // We are closing a window, so the window that is now under the mouse
    // might need to get an Enter event if it isn't already the mouse window.
    if (window()->type() & Qt::Window) {
        const QPointF screenPoint = QCocoaScreen::mapFromNative([NSEvent mouseLocation]);
        if (QWindow *windowUnderMouse = QGuiApplication::topLevelAt(screenPoint.toPoint())) {
            if (windowUnderMouse != QGuiApplicationPrivate::instance()->currentMouseWindow) {
                const auto windowPoint = windowUnderMouse->mapFromGlobal(screenPoint);
                // asynchronous delivery on purpose
                QWindowSystemInterface::handleEnterEvent<QWindowSystemInterface::AsynchronousDelivery>
                    (windowUnderMouse, windowPoint, screenPoint);
            }
        }
    }
}

void QCocoaWindow::windowDidChangeOcclusionState()
{
    // Note, we don't take the view's hiddenOrHasHiddenAncestor state into
    // account here, but instead leave that up to handleExposeEvent, just
    // like all the other signals that could potentially change the exposed
    // state of the window.
    bool visible = m_view.window.occlusionState & NSWindowOcclusionStateVisible;
    qCDebug(lcQpaWindow) << "Occlusion state of" << m_view.window << "for"
        << window() << "changed to" << (visible ? "visible" : "occluded");

    if (visible)
        [m_view setNeedsDisplay:YES];
    else
        handleExposeEvent(QRegion());
}

void QCocoaWindow::windowDidChangeScreen()
{
    if (!window())
        return;

    // Note: When a window is resized to 0x0 Cocoa will report the window's screen as nil
    NSScreen *nsScreen = m_view.window.screen;

    qCDebug(lcQpaWindow) << window() << "did change" << nsScreen;
    QCocoaScreen::updateScreens();

    auto *previousScreen = static_cast<QCocoaScreen*>(screen());
    auto *currentScreen = QCocoaScreen::get(nsScreen);

    qCDebug(lcQpaWindow) << "Screen changed for" << window() << "from" << previousScreen << "to" << currentScreen;

    // Note: The previous screen may be the same as the current screen, either because
    // a) the screen was just reconfigured, which still results in AppKit sending an
    // NSWindowDidChangeScreenNotification, b) because the previous screen was removed,
    // and we ended up calling QWindow::setScreen to move the window, which doesn't
    // actually move the window to the new screen, or c) because we've delivered the
    // screen change to the top level window, which will make all the child windows
    // of that window report the new screen when requested via QWindow::screen().
    // We still need to deliver the screen change in all these cases, as the
    // device-pixel ratio may have changed, and needs to be delivered to all
    // windows, both top level and child windows.

    QWindowSystemInterface::handleWindowScreenChanged<QWindowSystemInterface::SynchronousDelivery>(
        window(), currentScreen ? currentScreen->screen() : nullptr);

    if (currentScreen && hasPendingUpdateRequest()) {
        // Restart display-link on new screen. We need to do this unconditionally,
        // since we can't rely on the previousScreen reflecting whether or not the
        // window actually moved from one screen to another, or just stayed on the
        // same screen.
        currentScreen->requestUpdate();
    }
}

// ----------------------- NSWindowDelegate callbacks -----------------------

bool QCocoaWindow::windowShouldClose()
{
    qCDebug(lcQpaWindow) << "QCocoaWindow::windowShouldClose" << window();

    // This callback should technically only determine if the window
    // should (be allowed to) close, but since our QPA API to determine
    // that also involves actually closing the window we do both at the
    // same time, instead of doing the latter in windowWillClose.

    // If the window is closed, we will release and deallocate the NSWindow.
    // But frames higher up in the stack might still expect the window to
    // be alive, since the windowShouldClose: callback is technically only
    // supposed to answer YES or NO. To ensure the window is still alive
    // we put an autorelease in the closest pool (typically the runloop).
    [[m_view.window retain] autorelease];

    return QWindowSystemInterface::handleCloseEvent<QWindowSystemInterface::SynchronousDelivery>(window());
}

// ----------------------------- QPA forwarding -----------------------------

void QCocoaWindow::handleGeometryChange()
{
    QRect newGeometry;
    if (isContentView() && !isEmbedded()) {
        // Content views are positioned at (0, 0) in the window, so we resolve via the window
        CGRect contentRect = [m_view.window contentRectForFrameRect:m_view.window.frame];

        // The result above is in native screen coordinates, so remap to the Qt coordinate system
        newGeometry = QCocoaScreen::mapFromNative(contentRect).toRect();
    } else {
        // QNSView has isFlipped set, so no need to remap the geometry
        newGeometry = QRectF::fromCGRect(m_view.frame).toRect();
    }

    qCDebug(lcQpaWindow) << "QCocoaWindow::handleGeometryChange" << window()
                               << "current" << geometry() << "new" << newGeometry;

    // It can happen that the current NSWindow is nil (if we are changing styleMask
    // from/to borderless, and the content view is being re-parented), which results
    // in invalid coordinates.
    if (m_inSetStyleMask && !m_view.window) {
        qCDebug(lcQpaWindow) << "Lacking window during style mask update, ignoring geometry change";
        return;
    }

    // Prevent geometry change during initialization, as that will result
    // in a resize event, and Qt expects those to come after the show event.
    // FIXME: Remove once we've clarified the Qt behavior for this.
    if (!m_initialized) {
        // But update the QPlatformWindow reality
        QPlatformWindow::setGeometry(newGeometry);
        qCDebug(lcQpaWindow) << "Window still initializing, skipping event";
        return;
    }

    QWindowSystemInterface::handleGeometryChange(window(), newGeometry);

    // Changing the window geometry may affect the safe area margins
    updateSafeAreaMarginsIfNeeded();

    // Guard against processing window system events during QWindow::setGeometry
    // calls, which Qt and Qt applications do not expect.
    if (!m_inSetGeometry)
        QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
}

void QCocoaWindow::handleExposeEvent(const QRegion &region)
{
    // Ideally we'd implement isExposed() in terms of these properties,
    // plus the occlusionState of the NSWindow, and let the expose event
    // pull the exposed state out when needed. However, when the window
    // is first shown we receive a drawRect call where the occlusionState
    // of the window is still hidden, but we still want to prepare the
    // window for display by issuing an expose event to Qt. To work around
    // this we don't use the occlusionState directly, but instead base
    // the exposed state on the region we get in, which in the case of
    // a window being obscured is an empty region, and in the case of
    // a drawRect call is a non-null region, even if occlusionState
    // is still hidden. This ensures the window is prepared for display.
    if (m_view.window.visible && m_view.window.screen
            && !geometry().size().isEmpty() && !region.isEmpty()
            && !m_view.hiddenOrHasHiddenAncestor) {
        m_exposedRect = region.boundingRect();
    } else {
        m_exposedRect = QRect();
    }

    qCDebug(lcQpaDrawing) << "QCocoaWindow::handleExposeEvent" << window() << region << "isExposed" << isExposed();
    QWindowSystemInterface::handleExposeEvent<QWindowSystemInterface::SynchronousDelivery>(window(), region);

    if (!isExposed())
        static_cast<QCocoaScreen *>(screen())->maybeStopDisplayLink();
}

// --------------------------------------------------------------------------

bool QCocoaWindow::windowIsPopupType(Qt::WindowType type) const
{
    if (type == Qt::Widget)
        type = window()->type();
    if (type == Qt::Tool)
        return false; // Qt::Tool has the Popup bit set but isn't, at least on Mac.

    return ((type & Qt::Popup) == Qt::Popup);
}

/*!
    Checks if the window is the content view of its immediate NSWindow.

    Being the content view of a NSWindow means the QWindow is
    the highest accessible NSView object in the window's view
    hierarchy.

    This is the case if the QWindow is a top level window.
*/
bool QCocoaWindow::isContentView() const
{
    return m_view.window.contentView == m_view;
}

/*!
    Recreates (or removes) the NSWindow for this QWindow, if needed.

    A QWindow may need a corresponding NSWindow/NSPanel, depending on
    whether or not it's a top level or not, window flags, etc.
*/
void QCocoaWindow::recreateWindowIfNeeded()
{
    QMacAutoReleasePool pool;

    QPlatformWindow *parentWindow = QPlatformWindow::parent();
    auto *parentCocoaWindow = static_cast<QCocoaWindow *>(parentWindow);

    QCocoaWindow *oldParentCocoaWindow = nullptr;
    if (QNSView *qnsView = qnsview_cast(m_view.superview))
        oldParentCocoaWindow = qnsView.platformWindow;

    if (isForeignWindow()) {
        // A foreign window is created as such, and can never move between being
        // foreign and not, so we don't need to get rid of any existing NSWindows,
        // nor create new ones, as a foreign window is a single simple NSView.
        qCDebug(lcQpaWindow) << "Skipping NSWindow management for foreign window" << this;

        // We do however need to manage the parent relationship
        if (parentCocoaWindow)
            [parentCocoaWindow->m_view addSubview:m_view];
        else if (oldParentCocoaWindow)
            [m_view removeFromSuperview];

        return;
    }

    const bool isEmbeddedView = isEmbedded();
    RecreationReasons recreateReason = RecreationNotNeeded;

    if (parentWindow != oldParentCocoaWindow)
         recreateReason |= ParentChanged;

    if (!m_view.window)
        recreateReason |= MissingWindow;

    // If the modality has changed the style mask will need updating
    if (m_windowModality != window()->modality())
        recreateReason |= WindowModalityChanged;

    Qt::WindowType type = window()->type();

    const bool shouldBeContentView = !parentWindow
        && !((type & Qt::SubWindow) == Qt::SubWindow)
        && !isEmbeddedView;
    if (isContentView() != shouldBeContentView)
        recreateReason |= ContentViewChanged;

    const bool isPanel = isContentView() && [m_view.window isKindOfClass:[QNSPanel class]];
    const bool shouldBePanel = shouldBeContentView &&
        ((type & Qt::Popup) == Qt::Popup || (type & Qt::Dialog) == Qt::Dialog);

    if (isPanel != shouldBePanel)
         recreateReason |= PanelChanged;

    qCDebug(lcQpaWindow) << "QCocoaWindow::recreateWindowIfNeeded" << window() << recreateReason;

    if (recreateReason == RecreationNotNeeded)
        return;

    // Remove current window (if any)
    if ((isContentView() && !shouldBeContentView) || (recreateReason & PanelChanged)) {
        if (m_nsWindow) {
            qCDebug(lcQpaWindow) << "Getting rid of existing window" << m_nsWindow;
            [m_nsWindow closeAndRelease];
            if (isContentView() && !isEmbeddedView) {
                // We explicitly disassociate m_view from the window's contentView,
                // as AppKit does not automatically do this in response to removing
                // the view from the NSThemeFrame subview list, so we might end up
                // with a NSWindow contentView pointing to a deallocated NSView.
                m_view.window.contentView = nil;
            }
            m_nsWindow = nil;
        }
    }

    if (shouldBeContentView && !m_nsWindow) {
        // Move view to new NSWindow if needed
        if (auto *newWindow = createNSWindow(shouldBePanel)) {
            qCDebug(lcQpaWindow) << "Ensuring that" << m_view << "is content view for" << newWindow;
            [m_view setPostsFrameChangedNotifications:NO];
            [newWindow setContentView:m_view];
            [m_view setPostsFrameChangedNotifications:YES];

            m_nsWindow = newWindow;
            Q_ASSERT(m_view.window == m_nsWindow);
        }
    }

    if (parentCocoaWindow) {
        // Child windows have no NSWindow, re-parent to superview instead
        [parentCocoaWindow->m_view addSubview:m_view];
    }
}

void QCocoaWindow::requestUpdate()
{
    qCDebug(lcQpaDrawing) << "QCocoaWindow::requestUpdate" << window()
        << "using" << (updatesWithDisplayLink() ? "display-link" : "timer");

    if (updatesWithDisplayLink()) {
        if (!static_cast<QCocoaScreen *>(screen())->requestUpdate()) {
            qCDebug(lcQpaDrawing) << "Falling back to timer-based update request";
            QPlatformWindow::requestUpdate();
        }
    } else {
        // Fall back to the un-throttled timer-based callback
        QPlatformWindow::requestUpdate();
    }
}

bool QCocoaWindow::updatesWithDisplayLink() const
{
    // Update via CVDisplayLink if Vsync is enabled
    return format().swapInterval() != 0;
}

void QCocoaWindow::deliverUpdateRequest()
{
    qCDebug(lcQpaDrawing) << "Delivering update request to" << window();

    if (auto *qtMetalLayer = qt_objc_cast<QMetalLayer*>(contentLayer())) {
        // We attempt a read lock here, so that the animation/render thread is
        // prioritized lower than the main thread's displayLayer processing.
        // Without this the two threads might fight over the next drawable,
        // starving the main thread's presentation of the resized layer.
        if (!qtMetalLayer.displayLock.tryLockForRead()) {
            qCDebug(lcQpaDrawing) << "Deferring update request"
                << "due to" << qtMetalLayer << "needing display";
            return;
        }

        // But we don't hold the lock, as the update request can recurse
        // back into setNeedsDisplay, which would deadlock.
        qtMetalLayer.displayLock.unlock();
    }

    QPlatformWindow::deliverUpdateRequest();
}

void QCocoaWindow::requestActivateWindow()
{
    QMacAutoReleasePool pool;
    [m_view.window makeFirstResponder:m_view];
    [m_view.window makeKeyWindow];
}

/*
    Closes all popups, and removes observers and monitors.
*/
void QCocoaWindow::closeAllPopups()
{
    QGuiApplicationPrivate::instance()->closeAllPopups();

    removePopupMonitor();
}

void QCocoaWindow::removePopupMonitor()
{
    if (s_globalMouseMonitor) {
        [NSEvent removeMonitor:s_globalMouseMonitor];
        s_globalMouseMonitor = nil;
    }
    if (s_applicationActivationObserver) {
        [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:s_applicationActivationObserver];
        s_applicationActivationObserver = nil;
    }
}

void QCocoaWindow::setupPopupMonitor()
{
    // we open a popup window while we are not active. None of our existing event
    // handlers will get called if the user now clicks anywhere outside the application
    // or activates another window. Use a global event monitor to watch for mouse
    // presses, and close popups. We also want mouse tracking in the popup to work, so
    // also watch for MouseMoved.
    if (!s_globalMouseMonitor) {
        // we only get LeftMouseDown events when we also set LeftMouseUp.
        constexpr NSEventMask mouseButtonMask = NSEventTypeLeftMouseDown | NSEventTypeLeftMouseUp
                                                | NSEventMaskRightMouseDown | NSEventMaskOtherMouseDown
                                                | NSEventMaskMouseMoved;
        s_globalMouseMonitor = [NSEvent addGlobalMonitorForEventsMatchingMask:mouseButtonMask
                                        handler:^(NSEvent *e){
            if (!QGuiApplicationPrivate::instance()->activePopupWindow()) {
                removePopupMonitor();
                return;
            }
            const auto eventType = cocoaEvent2QtMouseEvent(e);
            if (eventType == QEvent::MouseMove) {
                if (s_windowUnderMouse) {
                    QWindow *window = s_windowUnderMouse->window();
                    const auto button = cocoaButton2QtButton(e);
                    const auto buttons = currentlyPressedMouseButtons();
                    const auto globalPoint = QCocoaScreen::mapFromNative(NSEvent.mouseLocation);
                    const auto localPoint = window->mapFromGlobal(globalPoint.toPoint());
                    QWindowSystemInterface::handleMouseEvent(window, localPoint, globalPoint,
                                                             buttons, button, eventType);
                }
            } else {
                closeAllPopups();
            }
        }];
    }
    // The activation observer also gets called when we become active because the user clicks
    // into the popup. This should not close the popup, so QCocoaApplicationDelegate's
    // applicationDidBecomeActive implementation removes this observer.
    if (!s_applicationActivationObserver) {
        s_applicationActivationObserver = [[[NSWorkspace sharedWorkspace] notificationCenter]
                                            addObserverForName:NSWorkspaceDidActivateApplicationNotification
                                                        object:nil queue:nil
                                                    usingBlock:^(NSNotification *){
            closeAllPopups();
        }];
    }
}

QCocoaNSWindow *QCocoaWindow::createNSWindow(bool shouldBePanel)
{
    QMacAutoReleasePool pool;

    Qt::WindowType type = window()->type();
    Qt::WindowFlags flags = window()->flags();

    QRect rect = geometry();

    QScreen *targetScreen = nullptr;
    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen->geometry().contains(rect.topLeft())) {
            targetScreen = screen;
            break;
        }
    }

    NSWindowStyleMask styleMask = windowStyleMask(flags);

    if (!targetScreen) {
        qCWarning(lcQpaWindow) << "Window position" << rect << "outside any known screen, using primary screen";
        targetScreen = QGuiApplication::primaryScreen();
        // Unless the window is created as borderless AppKit won't find a position and
        // screen that's close to the requested invalid position, and will always place
        // the window on the primary screen.
        styleMask = NSWindowStyleMaskBorderless;
    }

    rect.translate(-targetScreen->geometry().topLeft());
    auto *targetCocoaScreen = static_cast<QCocoaScreen *>(targetScreen->handle());
    NSRect contentRect = QCocoaScreen::mapToNative(rect, targetCocoaScreen);

    if (targetScreen->primaryOrientation() == Qt::PortraitOrientation) {
        // The macOS window manager has a bug, where if a screen is rotated, it will not allow
        // a window to be created within the area of the screen that has a Y coordinate (I quadrant)
        // higher than the height of the screen in its non-rotated state (including a magic padding
        // of 24 points), unless the window is created with the NSWindowStyleMaskBorderless style mask.
        if (styleMask && (contentRect.origin.y + 24 > targetScreen->geometry().width())) {
            qCDebug(lcQpaWindow) << "Window positioned on portrait screen."
                << "Adjusting style mask during creation";
            styleMask = NSWindowStyleMaskBorderless;
        }
    }

    // Create NSWindow
    Class windowClass = shouldBePanel ? [QNSPanel class] : [QNSWindow class];
    QCocoaNSWindow *nsWindow = [[windowClass alloc] initWithContentRect:contentRect
        // Mask will be updated in setWindowFlags if not the final mask
        styleMask:styleMask
        // Deferring window creation breaks OpenGL (the GL context is
        // set up before the window is shown and needs a proper window)
        backing:NSBackingStoreBuffered defer:NO
        screen:targetCocoaScreen->nativeScreen()
        platformWindow:this];

    // The resulting screen can be different from the screen requested if
    // for example the application has been assigned to a specific display.
    auto resultingScreen = QCocoaScreen::get(nsWindow.screen);

    // But may not always be resolved at this point, in which case we fall back
    // to the target screen. The real screen will be delivered as a screen change
    // when resolved as part of ordering the window on screen.
    if (!resultingScreen)
        resultingScreen = targetCocoaScreen;

    if (resultingScreen->screen() != window()->screen()) {
        QWindowSystemInterface::handleWindowScreenChanged<
            QWindowSystemInterface::SynchronousDelivery>(window(), resultingScreen->screen());
    }

    static QSharedPointer<QNSWindowDelegate> sharedDelegate([[QNSWindowDelegate alloc] init],
        [](QNSWindowDelegate *delegate) { [delegate release]; });
    nsWindow.delegate = sharedDelegate.get();

    // Prevent Cocoa from releasing the window on close. Qt
    // handles the close event asynchronously and we want to
    // make sure that NSWindow stays valid until the
    // QCocoaWindow is deleted by Qt.
    [nsWindow setReleasedWhenClosed:NO];

    if (alwaysShowToolWindow()) {
        static dispatch_once_t onceToken;
        dispatch_once(&onceToken, ^{
            NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
            [center addObserver:[QNSWindow class] selector:@selector(applicationActivationChanged:)
                name:NSApplicationWillResignActiveNotification object:nil];
            [center addObserver:[QNSWindow class] selector:@selector(applicationActivationChanged:)
                name:NSApplicationWillBecomeActiveNotification object:nil];
        });
    }

    nsWindow.restorable = NO;
    nsWindow.level = windowLevel(flags);
    nsWindow.tabbingMode = NSWindowTabbingModeDisallowed;

    if (shouldBePanel) {
        // Qt::Tool windows hide on app deactivation, unless Qt::WA_MacAlwaysShowToolWindow is set
        nsWindow.hidesOnDeactivate = ((type & Qt::Tool) == Qt::Tool) && !alwaysShowToolWindow();

        // Make popup windows show on the same desktop as the parent window
        nsWindow.collectionBehavior = NSWindowCollectionBehaviorFullScreenAuxiliary
                | NSWindowCollectionBehaviorMoveToActiveSpace;

        if ((type & Qt::Popup) == Qt::Popup) {
            nsWindow.hasShadow = YES;
            nsWindow.animationBehavior = NSWindowAnimationBehaviorUtilityWindow;
            if (QGuiApplication::applicationState() != Qt::ApplicationActive)
                setupPopupMonitor();
        }
    }

    // Persist modality so we can detect changes later on
    m_windowModality = QPlatformWindow::window()->modality();

    applyContentBorderThickness(nsWindow);

    // We propagate the view's color space granulary to both the IOSurfaces
    // used for QSurface::RasterSurface, as well as the CAMetalLayer used for
    // QSurface::MetalSurface, but for QSurface::OpenGLSurface we don't have
    // that option as we use NSOpenGLContext instead of CAOpenGLLayer. As a
    // workaround we set the NSWindow's color space, which affects GL drawing
    // with NSOpenGLContext as well. This does not conflict with the granular
    // modifications we do to each surface for raster or Metal.
    if (auto *qtView = qnsview_cast(m_view))
        nsWindow.colorSpace = qtView.colorSpace;

    return nsWindow;
}

bool QCocoaWindow::alwaysShowToolWindow() const
{
    return qt_mac_resolveOption(false, window(), "_q_macAlwaysShowToolWindow", "");
}

bool QCocoaWindow::setWindowModified(bool modified)
{
    if (!isContentView())
        return false;

    m_view.window.documentEdited = modified;
    return true;
}

void QCocoaWindow::setMenubar(QCocoaMenuBar *mb)
{
    m_menubar = mb;
}

QCocoaMenuBar *QCocoaWindow::menubar() const
{
    return m_menubar;
}

void QCocoaWindow::setWindowCursor(NSCursor *cursor)
{
    // Setting a cursor in a foreign view is not supported
    if (isForeignWindow())
        return;

    qCInfo(lcQpaMouse) << "Setting" << this << "cursor to" << cursor;

    QNSView *view = qnsview_cast(m_view);
    if (cursor == view.cursor)
        return;

    view.cursor = cursor;

    // We're not using the the legacy cursor rects API to manage our
    // cursor, but calling this function also invalidates AppKit's
    // view of whether or not we need a cursorUpdate callback for
    // our tracking area.
    [m_view.window invalidateCursorRectsForView:m_view];

    // We've informed AppKit that we need a cursorUpdate, but cursor
    // updates for tracking areas are deferred in some cases, such as
    // when the mouse is down, whereas we want a synchronous update.
    // To ensure an updated cursor we synthesize a cursor update event
    // now if the window is otherwise allowed to change the cursor.
    auto locationInWindow = m_view.window.mouseLocationOutsideOfEventStream;
    auto locationInSuperview = [m_view.superview convertPoint:locationInWindow fromView:nil];
    bool mouseIsOverView = [m_view hitTest:locationInSuperview] == m_view;
    auto utilityMask = NSWindowStyleMaskUtilityWindow | NSWindowStyleMaskTitled;
    bool isUtilityWindow = (m_view.window.styleMask & utilityMask) == utilityMask;
    if (mouseIsOverView && (m_view.window.keyWindow || isUtilityWindow)) {
        qCDebug(lcQpaMouse) << "Synthesizing cursor update";
        [m_view cursorUpdate:[NSEvent enterExitEventWithType:NSEventTypeCursorUpdate
            location:locationInWindow modifierFlags:0 timestamp:0
            windowNumber:m_view.window.windowNumber context:nil
            eventNumber:0 trackingNumber:0 userData:0]];
    }
}

void QCocoaWindow::registerTouch(bool enable)
{
    m_registerTouchCount += enable ? 1 : -1;
    if (enable && m_registerTouchCount == 1)
        m_view.allowedTouchTypes |= NSTouchTypeMaskIndirect;
    else if (m_registerTouchCount == 0)
        m_view.allowedTouchTypes &= ~NSTouchTypeMaskIndirect;
}

void QCocoaWindow::registerContentBorderArea(quintptr identifier, int upper, int lower)
{
    m_contentBorderAreas.insert(identifier, BorderRange(identifier, upper, lower));
    applyContentBorderThickness();
}

void QCocoaWindow::setContentBorderAreaEnabled(quintptr identifier, bool enable)
{
    m_enabledContentBorderAreas.insert(identifier, enable);
    applyContentBorderThickness();
}

void QCocoaWindow::setContentBorderEnabled(bool enable)
{
    m_drawContentBorderGradient = enable;
    applyContentBorderThickness();
}

void QCocoaWindow::applyContentBorderThickness(NSWindow *window)
{
    if (!window && isContentView())
        window = m_view.window;

    if (!window)
        return;

    if (!m_drawContentBorderGradient) {
        // FIXME (QTBUG-138829)
        window.styleMask = window.styleMask & QT_IGNORE_DEPRECATIONS(~NSWindowStyleMaskTexturedBackground);
        setWindowFlags(QPlatformWindow::window()->flags());
        [window.contentView.superview setNeedsDisplay:YES];
        return;
    }

    // Find consecutive registered border areas, starting from the top.
    std::vector<BorderRange> ranges(m_contentBorderAreas.cbegin(), m_contentBorderAreas.cend());
    std::sort(ranges.begin(), ranges.end());
    int effectiveTopContentBorderThickness = 0;
    for (BorderRange range : ranges) {
        // Skip disiabled ranges (typically hidden tool bars)
        if (!m_enabledContentBorderAreas.value(range.identifier, false))
            continue;

        // Is this sub-range adjacent to or overlapping the
        // existing total border area range? If so merge
        // it into the total range,
        if (range.upper <= (effectiveTopContentBorderThickness + 1))
            effectiveTopContentBorderThickness = qMax(effectiveTopContentBorderThickness, range.lower);
        else
            break;
    }

    int effectiveBottomContentBorderThickness = 0;

    // FIXME (QTBUG-138829)
    [window setStyleMask:[window styleMask] | QT_IGNORE_DEPRECATIONS(NSWindowStyleMaskTexturedBackground)];
    setWindowFlags(QPlatformWindow::window()->flags());

    // Setting titlebarAppearsTransparent to YES means that the border thickness has to account
    // for the title bar height as well, otherwise sheets will not be presented at the correct
    // position, which should be (title bar height + top content border size).
    const NSRect frameRect = window.frame;
    const NSRect contentRect = [window contentRectForFrameRect:frameRect];
    const CGFloat titlebarHeight = frameRect.size.height - contentRect.size.height;
    effectiveTopContentBorderThickness += titlebarHeight;

    [window setContentBorderThickness:effectiveTopContentBorderThickness forEdge:NSMaxYEdge];
    [window setAutorecalculatesContentBorderThickness:NO forEdge:NSMaxYEdge];

    [window setContentBorderThickness:effectiveBottomContentBorderThickness forEdge:NSMinYEdge];
    [window setAutorecalculatesContentBorderThickness:NO forEdge:NSMinYEdge];

    [[[window contentView] superview] setNeedsDisplay:YES];
}

bool QCocoaWindow::testContentBorderAreaPosition(int position) const
{
    if (!m_drawContentBorderGradient || !isContentView())
        return false;

    // Determine if the given y position (relative to the content area) is inside the
    // unified toolbar area. Note that the value returned by contentBorderThicknessForEdge
    // includes the title bar height; subtract it.
    const int contentBorderThickness = [m_view.window contentBorderThicknessForEdge:NSMaxYEdge];
    const NSRect frameRect = m_view.window.frame;
    const NSRect contentRect = [m_view.window contentRectForFrameRect:frameRect];
    const CGFloat titlebarHeight = frameRect.size.height - contentRect.size.height;
    return 0 <= position && position < (contentBorderThickness - titlebarHeight);
}

qreal QCocoaWindow::devicePixelRatio() const
{
    // The documented way to observe the relationship between device-independent
    // and device pixels is to use one for the convertToBacking functions. Other
    // methods such as [NSWindow backingScaleFactor] might not give the correct
    // result, for example if setWantsBestResolutionOpenGLSurface is not set or
    // or ignored by the OpenGL driver.
    NSSize backingSize = [m_view convertSizeToBacking:NSMakeSize(1.0, 1.0)];
    return backingSize.height;
}

QWindow *QCocoaWindow::childWindowAt(QPoint windowPoint)
{
    QWindow *targetWindow = window();
    for (QObject *child : targetWindow->children())
        if (QWindow *childWindow = qobject_cast<QWindow *>(child))
            if (QPlatformWindow *handle = childWindow->handle())
                if (handle->isExposed() && childWindow->geometry().contains(windowPoint))
                    targetWindow = static_cast<QCocoaWindow*>(handle)->childWindowAt(windowPoint - childWindow->position());

    return targetWindow;
}

bool QCocoaWindow::shouldRefuseKeyWindowAndFirstResponder()
{
    // This function speaks up if there's any reason
    // to refuse key window or first responder state.

    if (window()->flags() & (Qt::WindowDoesNotAcceptFocus | Qt::WindowTransparentForInput))
        return true;

    // For application modal windows, as well as direct parent windows
    // of window modal windows, AppKit takes care of blocking interaction.
    // The Qt expectation however, is that all transient parents of a
    // window modal window is blocked, as reflected by QGuiApplication.
    // We reflect this by returning false from this function for transient
    // parents blocked by a modal window, but limit it to the cases not
    // covered by AppKit to avoid potential unwanted side effects.
    QWindow *modalWindow = nullptr;
    if (QGuiApplicationPrivate::instance()->isWindowBlocked(window(), &modalWindow)) {
        if (modalWindow->modality() == Qt::WindowModal && modalWindow->transientParent() != window()) {
            qCDebug(lcQpaWindow) << "Refusing key window for" << this << "due to being"
                << "blocked by" << modalWindow;
            return true;
        }
    }

    if (m_inSetVisible) {
        QVariant showWithoutActivating = window()->property("_q_showWithoutActivating");
        if (showWithoutActivating.isValid() && showWithoutActivating.toBool())
            return true;
    }

    return false;
}

bool QCocoaWindow::windowEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowBlocked:
    case QEvent::WindowUnblocked:
        updateTitleBarButtons(window()->flags());
        break;
    default:
        break;
    }

    return QPlatformWindow::windowEvent(event);
}

QPoint QCocoaWindow::bottomLeftClippedByNSWindowOffset() const
{
    if (!m_view)
        return QPoint();
    const NSPoint origin = [m_view isFlipped] ? NSMakePoint(0, [m_view frame].size.height)
                                                     : NSMakePoint(0,                                 0);
    const NSRect visibleRect = [m_view visibleRect];

    return QPoint(visibleRect.origin.x, -visibleRect.origin.y + (origin.y - visibleRect.size.height));
}

QMargins QCocoaWindow::frameMargins() const
{
    if (!isContentView())
        return QMargins();

    NSRect frameW = m_view.window.frame;
    NSRect frameC = [m_view.window contentRectForFrameRect:frameW];

    return QMargins(frameW.origin.x - frameC.origin.x,
        (frameW.origin.y + frameW.size.height) - (frameC.origin.y + frameC.size.height),
        (frameW.origin.x + frameW.size.width) - (frameC.origin.x + frameC.size.width),
        frameC.origin.y - frameW.origin.y);
}

void QCocoaWindow::setFrameStrutEventsEnabled(bool enabled)
{
    m_frameStrutEventsEnabled = enabled;
}

CALayer *QCocoaWindow::contentLayer() const
{
    auto *layer = m_view.layer;
    if (auto *containerLayer = qt_objc_cast<QContainerLayer*>(layer))
        layer = containerLayer.contentLayer;
    return layer;
}

void QCocoaWindow::manageVisualEffectArea(quintptr identifier, const QRect &rect,
    NSVisualEffectMaterial material, NSVisualEffectBlendingMode blendMode,
    NSVisualEffectState activationState)
{
    if (!qt_objc_cast<QContainerLayer*>(m_view.layer)) {
        qCWarning(lcQpaWindow) << "Can not manage visual effect areas"
            << "in views without a container layer";
        return;
    }

    qCDebug(lcQpaWindow) << "Updating visual effect area" << identifier
        << "to" << rect << "with material" << material << "blend mode"
        << blendMode << "and activation state" << activationState;

    NSVisualEffectView *effectView = nullptr;
    if (m_effectViews.contains(identifier)) {
        effectView = m_effectViews.value(identifier);
        if (rect.isEmpty()) {
            [effectView removeFromSuperview];
            m_effectViews.remove(identifier);
            return;
        }
    } else if (!rect.isEmpty()) {
        effectView = [NSVisualEffectView new];
        // Ensure that the visual effect layer is stacked well
        // below our content layer (which defaults to a z of 0).
        effectView.wantsLayer = YES;
        effectView.layer.zPosition = -FLT_MAX;
        [m_view addSubview:effectView];
        m_effectViews.insert(identifier, effectView);
    }

    if (!effectView)
        return;

    effectView.frame = rect.toCGRect();
    effectView.material = material;
    effectView.blendingMode = blendMode;
    effectView.state = activationState;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QCocoaWindow *window)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QCocoaWindow(" << (const void *)window;
    if (window)
        debug << ", window=" << window->window();
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#include "moc_qcocoawindow.cpp"
