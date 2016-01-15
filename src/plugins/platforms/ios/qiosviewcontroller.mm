/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#import "qiosviewcontroller.h"

#include <QtCore/qscopedvaluerollback.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/QScreen>

#include <QtGui/private/qwindow_p.h>

#include "qiosintegration.h"
#include "qiosscreen.h"
#include "qiosglobal.h"
#include "qioswindow.h"
#include "quiview.h"

// -------------------------------------------------------------------------

@interface QIOSViewController () {
  @public
    QPointer<QIOSScreen> m_screen;
    BOOL m_updatingProperties;
    QMetaObject::Connection m_focusWindowChangeConnection;
}
@property (nonatomic, assign) BOOL changingOrientation;
@end

// -------------------------------------------------------------------------

@interface QIOSDesktopManagerView : UIView
@end

@implementation QIOSDesktopManagerView

- (id)init
{
    if (!(self = [super init]))
        return nil;

    QIOSIntegration *iosIntegration = QIOSIntegration::instance();
    if (iosIntegration && iosIntegration->debugWindowManagement()) {
        static UIImage *gridPattern = nil;
        static dispatch_once_t onceToken;
        dispatch_once(&onceToken, ^{
            CGFloat dimension = 100.f;

            UIGraphicsBeginImageContextWithOptions(CGSizeMake(dimension, dimension), YES, 0.0f);
            CGContextRef context = UIGraphicsGetCurrentContext();

            CGContextTranslateCTM(context, -0.5, -0.5);

            #define gridColorWithBrightness(br) \
                [UIColor colorWithHue:0.6 saturation:0.0 brightness:br alpha:1.0].CGColor

            CGContextSetFillColorWithColor(context, gridColorWithBrightness(0.05));
            CGContextFillRect(context, CGRectMake(0, 0, dimension, dimension));

            CGFloat gridLines[][2] = { { 10, 0.1 }, { 20, 0.2 }, { 100, 0.3 } };
            for (size_t l = 0; l < sizeof(gridLines) / sizeof(gridLines[0]); ++l) {
                CGFloat step = gridLines[l][0];
                for (int c = step; c <= dimension; c += step) {
                    CGContextMoveToPoint(context, c, 0);
                    CGContextAddLineToPoint(context, c, dimension);
                    CGContextMoveToPoint(context, 0, c);
                    CGContextAddLineToPoint(context, dimension, c);
                }

                CGFloat brightness = gridLines[l][1];
                CGContextSetStrokeColorWithColor(context, gridColorWithBrightness(brightness));
                CGContextStrokePath(context);
            }

            gridPattern = UIGraphicsGetImageFromCurrentImageContext();
            UIGraphicsEndImageContext();

            [gridPattern retain];
        });

        self.backgroundColor = [UIColor colorWithPatternImage:gridPattern];
    }

    return self;
}

- (void)didAddSubview:(UIView *)subview
{
    Q_UNUSED(subview);

    QIOSScreen *screen = self.qtViewController->m_screen;

    // The 'window' property of our view is not valid until the window
    // has been shown, so we have to access it through the QIOSScreen.
    UIWindow *uiWindow = screen->uiWindow();

    if (uiWindow.hidden) {
        // Associate UIWindow to screen and show it the first time a QWindow
        // is mapped to the screen. For external screens this means disabling
        // mirroring mode and presenting alternate content on the screen.
        uiWindow.screen = screen->uiScreen();
        uiWindow.hidden = NO;
    }
}

- (void)willRemoveSubview:(UIView *)subview
{
    Q_UNUSED(subview);

    Q_ASSERT(self.window);
    UIWindow *uiWindow = self.window;

    if (uiWindow.screen != [UIScreen mainScreen] && self.subviews.count == 1) {
        // Removing the last view of an external screen, go back to mirror mode
        uiWindow.screen = [UIScreen mainScreen];
        uiWindow.hidden = YES;
    }
}

- (void)layoutSubviews
{
    for (int i = int(self.subviews.count) - 1; i >= 0; --i) {
        UIView *view = static_cast<UIView *>([self.subviews objectAtIndex:i]);
        if (![view isKindOfClass:[QUIView class]])
            continue;

        [self layoutView: static_cast<QUIView *>(view)];
    }
}

- (void)layoutView:(QUIView *)view
{
    QWindow *window = view.qwindow;

    // Return early if the QIOSWindow is still constructing, as we'll
    // take care of setting the correct window state in the constructor.
    if (!window->handle())
        return;

    // Re-apply window states to update geometry
    if (window->windowState() & (Qt::WindowFullScreen | Qt::WindowMaximized))
        window->handle()->setWindowState(window->windowState());
}

// Even if the root view controller has both wantsFullScreenLayout and
// extendedLayoutIncludesOpaqueBars enabled, iOS will still push the root
// view down 20 pixels (and shrink the view accordingly) when the in-call
// statusbar is active (instead of updating the topLayoutGuide). Since
// we treat the root view controller as our screen, we want to reflect
// the in-call statusbar as a change in available geometry, not in screen
// geometry. To simplify the screen geometry mapping code we reset the
// view modifications that iOS does and take the statusbar height
// explicitly into account in QIOSScreen::updateProperties().

- (void)setFrame:(CGRect)newFrame
{
    Q_UNUSED(newFrame);
    Q_ASSERT(!self.window || self.window.rootViewController.view == self);

    // When presenting view controllers our view may be temporarily reparented into a UITransitionView
    // instead of the UIWindow, and the UITransitionView may have a transform set, so we need to do a
    // mapping even if we still expect to always be the root view-controller.
    CGRect transformedWindowBounds = [self.superview convertRect:self.window.bounds fromView:self.window];
    [super setFrame:transformedWindowBounds];
}

- (void)setBounds:(CGRect)newBounds
{
    Q_UNUSED(newBounds);
    CGRect transformedWindowBounds = [self convertRect:self.window.bounds fromView:self.window];
    [super setBounds:CGRectMake(0, 0, CGRectGetWidth(transformedWindowBounds), CGRectGetHeight(transformedWindowBounds))];
}

- (void)setCenter:(CGPoint)newCenter
{
    Q_UNUSED(newCenter);
    [super setCenter:self.window.center];
}

- (void)didMoveToWindow
{
    // The initial frame computed during startup may happen before the view has
    // a window, meaning our calculations above will be wrong. We ensure that the
    // frame is set correctly once we have a window to base our calulations on.
    [self setFrame:self.window.bounds];
}

@end

// -------------------------------------------------------------------------

@implementation QIOSViewController

- (id)initWithQIOSScreen:(QIOSScreen *)screen
{
    if (self = [self init]) {
        m_screen = screen;

#if QT_IOS_DEPLOYMENT_TARGET_BELOW(__IPHONE_7_0)
        QSysInfo::MacVersion iosVersion = QSysInfo::MacintoshVersion;

        // We prefer to keep the root viewcontroller in fullscreen layout, so that
        // we don't have to compensate for the viewcontroller position. This also
        // gives us the same behavior on iOS 5/6 as on iOS 7, where full screen layout
        // is the only way.
        if (iosVersion < QSysInfo::MV_IOS_7_0)
            self.wantsFullScreenLayout = YES;

        // Use translucent statusbar by default on iOS6 iPhones (unless the user changed
        // the default in the Info.plist), so that windows placed under the stausbar are
        // still visible, just like on iOS7.
        if (screen->uiScreen() == [UIScreen mainScreen]
            && iosVersion >= QSysInfo::MV_IOS_6_0 && iosVersion < QSysInfo::MV_IOS_7_0
            && [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone
            && [UIApplication sharedApplication].statusBarStyle == UIStatusBarStyleDefault)
            [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleBlackTranslucent];
#endif

        self.lockedOrientation = UIInterfaceOrientationUnknown;
        self.changingOrientation = NO;

        // Status bar may be initially hidden at startup through Info.plist
        self.prefersStatusBarHidden = infoPlistValue(@"UIStatusBarHidden", false);
        self.preferredStatusBarUpdateAnimation = UIStatusBarAnimationNone;
        self.preferredStatusBarStyle = UIStatusBarStyle(infoPlistValue(@"UIStatusBarStyle", UIStatusBarStyleDefault));

        m_focusWindowChangeConnection = QObject::connect(qApp, &QGuiApplication::focusWindowChanged, [self]() {
            [self updateProperties];
        });
    }

    return self;
}

- (void)dealloc
{
    QObject::disconnect(m_focusWindowChangeConnection);
    [super dealloc];
}

- (void)loadView
{
    self.view = [[[QIOSDesktopManagerView alloc] init] autorelease];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(willChangeStatusBarFrame:)
            name:UIApplicationWillChangeStatusBarFrameNotification
            object:[UIApplication sharedApplication]];

    [center addObserver:self selector:@selector(didChangeStatusBarOrientation:)
            name:UIApplicationDidChangeStatusBarOrientationNotification
            object:[UIApplication sharedApplication]];
}

- (void)viewDidUnload
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:nil object:nil];
    [super viewDidUnload];
}

// -------------------------------------------------------------------------

- (BOOL)shouldAutorotate
{
    return m_screen && m_screen->uiScreen() == [UIScreen mainScreen] && !self.lockedOrientation;
}

#if QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_6_0)
- (NSUInteger)supportedInterfaceOrientations
{
    // As documented by Apple in the iOS 6.0 release notes, setStatusBarOrientation:animated:
    // only works if the supportedInterfaceOrientations of the view controller is 0, making
    // us responsible for ensuring that the status bar orientation is consistent. We enter
    // this mode when auto-rotation is disabled due to an explicit content orientation being
    // set on the focus window. Note that this is counter to what the documentation for
    // supportedInterfaceOrientations says, which states that the method should not return 0.
    return [self shouldAutorotate] ? UIInterfaceOrientationMaskAll : 0;
}
#endif

#if QT_IOS_DEPLOYMENT_TARGET_BELOW(__IPHONE_6_0)
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    Q_UNUSED(interfaceOrientation);
    return [self shouldAutorotate];
}
#endif

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation duration:(NSTimeInterval)duration
{
    Q_UNUSED(orientation);
    Q_UNUSED(duration);

    self.changingOrientation = YES;
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)orientation
{
    Q_UNUSED(orientation);

    self.changingOrientation = NO;
}

- (void)willChangeStatusBarFrame:(NSNotification*)notification
{
    Q_UNUSED(notification);

    if (self.view.window.screen != [UIScreen mainScreen])
        return;

    // Orientation changes will already result in laying out subviews, so we don't
    // need to do anything extra for frame changes during an orientation change.
    // Technically we can receive another actual statusbar frame update during the
    // orientation change that we should react to, but to simplify the logic we
    // use a simple bool variable instead of a ignoreNextFrameChange approach.
    if (self.changingOrientation)
        return;

    // UIKit doesn't have a delegate callback for statusbar changes that's run inside the
    // animation block, like UIViewController's willAnimateRotationToInterfaceOrientation,
    // nor does it expose a constant for the duration and easing of the animation. However,
    // though poking at the various UIStatusBar methods, we can observe that the animation
    // uses the default easing curve, and runs with a duration of 0.35 seconds.
    static qreal kUIStatusBarAnimationDuration = 0.35;

    [UIView animateWithDuration:kUIStatusBarAnimationDuration animations:^{
        [self.view setNeedsLayout];
        [self.view layoutIfNeeded];
    }];
}

- (void)didChangeStatusBarOrientation:(NSNotification *)notification
{
    Q_UNUSED(notification);

    if (self.view.window.screen != [UIScreen mainScreen])
        return;

    // If the statusbar changes orientation due to auto-rotation we don't care,
    // there will be re-layout anyways. Only if the statusbar changes due to
    // reportContentOrientation, we need to update the window layout.
    if (self.changingOrientation)
        return;

    [self.view setNeedsLayout];
}

- (void)viewWillLayoutSubviews
{
    if (!QCoreApplication::instance())
        return;

    if (m_screen)
        m_screen->updateProperties();
}

// -------------------------------------------------------------------------

- (void)updateProperties
{
    if (!isQtApplication())
        return;

    if (!m_screen || !m_screen->screen())
        return;

    // For now we only care about the main screen, as both the statusbar
    // visibility and orientation is only appropriate for the main screen.
    if (m_screen->uiScreen() != [UIScreen mainScreen])
        return;

    // Prevent recursion caused by updating the status bar appearance (position
    // or visibility), which in turn may cause a layout of our subviews, and
    // a reset of window-states, which themselves affect the view controller
    // properties such as the statusbar visibilty.
    if (m_updatingProperties)
        return;

    QScopedValueRollback<BOOL> updateRollback(m_updatingProperties, YES);

    QWindow *focusWindow = QGuiApplication::focusWindow();

    // If we don't have a focus window we leave the statusbar
    // as is, so that the user can activate a new window with
    // the same window state without the status bar jumping
    // back and forth.
    if (!focusWindow)
        return;

    // We only care about changes to focusWindow that involves our screen
    if (!focusWindow->screen() || focusWindow->screen()->handle() != m_screen)
        return;

    // All decisions are based on the the top level window
    focusWindow = qt_window_private(focusWindow)->topLevelWindow();

    UIApplication *uiApplication = [UIApplication sharedApplication];

    // -------------- Status bar style and visbility ---------------

    UIStatusBarStyle oldStatusBarStyle = self.preferredStatusBarStyle;
    if (focusWindow->flags() & Qt::MaximizeUsingFullscreenGeometryHint)
        self.preferredStatusBarStyle = UIStatusBarStyleDefault;
    else
        self.preferredStatusBarStyle = QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_7_0 ?
            UIStatusBarStyleLightContent : UIStatusBarStyleBlackTranslucent;

    if (self.preferredStatusBarStyle != oldStatusBarStyle) {
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_7_0)
            [self setNeedsStatusBarAppearanceUpdate];
        else
            [uiApplication setStatusBarStyle:self.preferredStatusBarStyle];
    }

    bool currentStatusBarVisibility = self.prefersStatusBarHidden;
    self.prefersStatusBarHidden = focusWindow->windowState() == Qt::WindowFullScreen;

    if (self.prefersStatusBarHidden != currentStatusBarVisibility) {
#if QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_7_0)
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_7_0) {
            [self setNeedsStatusBarAppearanceUpdate];
        } else
#endif
        {
            [uiApplication setStatusBarHidden:self.prefersStatusBarHidden
                withAnimation:self.preferredStatusBarUpdateAnimation];
        }

        [self.view setNeedsLayout];
    }


    // -------------- Content orientation ---------------

    static BOOL kAnimateContentOrientationChanges = YES;

    Qt::ScreenOrientation contentOrientation = focusWindow->contentOrientation();
    if (contentOrientation != Qt::PrimaryOrientation) {
        // An explicit content orientation has been reported for the focus window,
        // so we keep the status bar in sync with content orientation. This will ensure
        // that the task bar (and associated gestures) are also rotated accordingly.

        if (!self.lockedOrientation) {
            // We are moving from Qt::PrimaryOrientation to an explicit orientation,
            // so we need to store the current statusbar orientation, as we need it
            // later when mapping screen coordinates for QScreen and for returning
            // to Qt::PrimaryOrientation.
            self.lockedOrientation = uiApplication.statusBarOrientation;
        }

        [uiApplication setStatusBarOrientation:
            UIInterfaceOrientation(fromQtScreenOrientation(contentOrientation))
            animated:kAnimateContentOrientationChanges];

    } else {
        // The content orientation is set to Qt::PrimaryOrientation, meaning
        // that auto-rotation should be enabled. But we may be coming out of
        // a state of locked orientation, which needs some cleanup before we
        // can enable auto-rotation again.
        if (self.lockedOrientation) {
            // First we need to restore the statusbar to what it was at the
            // time of locking the orientation, otherwise iOS will be very
            // confused when it starts doing auto-rotation again.
            [uiApplication setStatusBarOrientation:self.lockedOrientation
                animated:kAnimateContentOrientationChanges];

            // Then we can re-enable auto-rotation
            self.lockedOrientation = UIInterfaceOrientationUnknown;

            // And finally let iOS rotate the root view to match the device orientation
            [UIViewController attemptRotationToDeviceOrientation];
        }
    }
}

@end

