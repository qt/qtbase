// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include <QtCore/qdebug.h>
#include <QtCore/qtimer.h>
#include <qpa/qplatformtheme.h>

#include "qcocoacolordialoghelper.h"
#include "qcocoahelpers.h"
#include "qcocoaeventdispatcher.h"
#include "private/qcoregraphics_p.h"

QT_USE_NAMESPACE

@interface QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) : NSObject<NSWindowDelegate, QNSPanelDelegate>
- (void)restoreOriginalContentView;
- (void)updateQtColor;
- (void)finishOffWithCode:(NSInteger)code;
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSColorPanelDelegate);

@implementation QNSColorPanelDelegate {
    @public
    NSColorPanel *mColorPanel;
    QCocoaColorDialogHelper *mHelper;
    NSView *mStolenContentView;
    QNSPanelContentsWrapper *mPanelButtons;
    QColor mQtColor;
    NSInteger mResultCode;
    BOOL mDialogIsExecuting;
    BOOL mResultSet;
    BOOL mClosingDueToKnownButton;
}

- (instancetype)init
{
    self = [super init];
    mColorPanel = [NSColorPanel sharedColorPanel];
    mHelper = nullptr;
    mStolenContentView = nil;
    mPanelButtons = nil;
    mResultCode = NSModalResponseCancel;
    mDialogIsExecuting = false;
    mResultSet = false;
    mClosingDueToKnownButton = false;

    [mColorPanel setRestorable:NO];

    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(colorChanged:)
        name:NSColorPanelColorDidChangeNotification
        object:mColorPanel];

    [[NSNotificationCenter defaultCenter] addObserver:self
      selector:@selector(windowWillClose:)
      name:NSWindowWillCloseNotification
      object:mColorPanel];

    [mColorPanel retain];
    return self;
}

- (void)dealloc
{
    [mStolenContentView release];
    [mColorPanel setDelegate:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [super dealloc];
}

- (void)setDialogHelper:(QCocoaColorDialogHelper *)helper
{
    mHelper = helper;

    if (mHelper->options()->testOption(QColorDialogOptions::NoButtons)) {
        [self restoreOriginalContentView];
    } else if (!mStolenContentView) {
        // steal the color panel's contents view
        mStolenContentView = mColorPanel.contentView;
        [mStolenContentView retain];
        mColorPanel.contentView = nil;

        // create a new content view and add the stolen one as a subview
        mPanelButtons = [[QNSPanelContentsWrapper alloc] initWithPanelDelegate:self];
        [mPanelButtons addSubview:mStolenContentView];
        mColorPanel.contentView = mPanelButtons;
        mColorPanel.defaultButtonCell = mPanelButtons.okButton.cell;
    }
}

- (void)closePanel
{
    [mColorPanel close];
}

- (void)colorChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);
    [self updateQtColor];
}

- (void)windowWillClose:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (mPanelButtons && mHelper && !mClosingDueToKnownButton) {
        mClosingDueToKnownButton = true; // prevent repeating emit
        emit mHelper->reject();
    }
}

- (void)restoreOriginalContentView
{
    if (mStolenContentView) {
        // return stolen stuff to its rightful owner
        [mStolenContentView removeFromSuperview];
        [mColorPanel setContentView:mStolenContentView];
        [mStolenContentView release];
        mStolenContentView = nil;
        [mPanelButtons release];
        mPanelButtons = nil;
    }
}

- (void)onOkClicked
{
    mClosingDueToKnownButton = true;
    [mColorPanel close];
    [self updateQtColor];
    [self finishOffWithCode:NSModalResponseOK];
}

- (void)onCancelClicked
{
    if (mPanelButtons) {
        mClosingDueToKnownButton = true;
        [mColorPanel close];
        mQtColor = QColor();
        [self finishOffWithCode:NSModalResponseCancel];
    }
}

- (void)updateQtColor
{
    // Discard the color space and pass the color components to QColor. This
    // is a good option as long as QColor is color-unmanaged: we preserve the
    // exact RGB value from the color picker, which is predictable. Further,
    // painting with the color will reproduce the same color on-screen, as
    // long as the the same screen is used for selecting the color.
    NSColor *componentColor = [[mColorPanel color] colorUsingType:NSColorTypeComponentBased];
    switch (componentColor.colorSpace.colorSpaceModel)
    {
    case NSColorSpaceModelGray: {
        CGFloat white = 0, alpha = 0;
        [componentColor getWhite:&white alpha:&alpha];
        mQtColor.setRgbF(white, white, white, alpha);
    } break;
    case NSColorSpaceModelRGB: {
        CGFloat red = 0, green = 0, blue = 0, alpha = 0;
        [componentColor getRed:&red green:&green blue:&blue alpha:&alpha];
        mQtColor.setRgbF(red, green, blue, alpha);
    } break;
    case NSColorSpaceModelCMYK: {
        CGFloat cyan = 0, magenta = 0, yellow = 0, black = 0, alpha = 0;
        [componentColor getCyan:&cyan magenta:&magenta yellow:&yellow black:&black alpha:&alpha];
        mQtColor.setCmykF(cyan, magenta, yellow, black, alpha);
    } break;
    default:
        qWarning("QNSColorPanelDelegate: Unsupported color space model");
    break;
    }

    if (mHelper)
        emit mHelper->currentColorChanged(mQtColor);
}

- (void)showModelessPanel
{
    mDialogIsExecuting = false;
    mResultSet = false;
    mClosingDueToKnownButton = false;
    // Make this an asynchronous call, so the panel is made key only
    // in the next event loop run. This is to make sure that by
    // the time the modal loop is run in runModalForWindow below,
    // which internally also sets the panel to key window,
    // the panel is not yet key, and the NSApp still has the right
    // reference to the _previousKeyWindow. Otherwise both NSApp.key
    // and NSApp._previousKeyWindow would wrongly point to the panel,
    // loosing any reference to the window that was key before.
    dispatch_async(dispatch_get_main_queue(), ^{
        [mColorPanel makeKeyAndOrderFront:mColorPanel];
    });
}

- (BOOL)runApplicationModalPanel
{
    mDialogIsExecuting = true;
    [mColorPanel setDelegate:self];
    [mColorPanel setContinuous:YES];
    // Call processEvents in case the event dispatcher has been interrupted, and needs to do
    // cleanup of modal sessions. Do this before showing the native dialog, otherwise it will
    // close down during the cleanup.
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

    // Make sure we don't interrupt the runModalForWindow call.
    QCocoaEventDispatcher::clearCurrentThreadCocoaEventDispatcherInterruptFlag();

    [NSApp runModalForWindow:mColorPanel];
    mDialogIsExecuting = false;

    // Wake up the event dispatcher so it can check whether the
    // current event loop should continue spinning or not.
    QCoreApplication::eventDispatcher()->wakeUp();

    return (mResultCode == NSModalResponseOK);
}

- (QPlatformDialogHelper::DialogCode)dialogResultCode
{
    return (mResultCode == NSModalResponseOK) ? QPlatformDialogHelper::Accepted : QPlatformDialogHelper::Rejected;
}

- (BOOL)windowShouldClose:(id)window
{
    Q_UNUSED(window);
    if (!mPanelButtons)
        [self updateQtColor];
    if (mDialogIsExecuting) {
        [self finishOffWithCode:NSModalResponseCancel];
    } else {
        mResultSet = true;
        if (mHelper)
            emit mHelper->reject();
    }
    return true;
}

- (void)finishOffWithCode:(NSInteger)code
{
    mResultCode = code;
    if (mDialogIsExecuting) {
        // We stop the current modal event loop. The control
        // will then return inside -(void)exec below.
        // It's important that the modal event loop is stopped before
        // we accept/reject QColorDialog, since QColorDialog has its
        // own event loop that needs to be stopped last.
        [NSApp stopModalWithCode:code];
    } else {
        // Since we are not in a modal event loop, we can safely close
        // down QColorDialog
        // Calling accept() or reject() can in turn call closeCocoaColorPanel.
        // This check will prevent any such recursion.
        if (!mResultSet) {
            mResultSet = true;
            if (mResultCode == NSModalResponseCancel) {
                emit mHelper->reject();
            } else {
                emit mHelper->accept();
            }
        }
    }
}

@end

QT_BEGIN_NAMESPACE

class QCocoaColorPanel
{
public:
    QCocoaColorPanel()
    {
        mDelegate = [[QNSColorPanelDelegate alloc] init];
    }

    ~QCocoaColorPanel()
    {
        [mDelegate release];
    }

    void init(QCocoaColorDialogHelper *helper)
    {
        [mDelegate setDialogHelper:helper];
    }

    void cleanup(QCocoaColorDialogHelper *helper)
    {
        if (mDelegate->mHelper == helper)
            mDelegate->mHelper = nullptr;
    }

    bool exec()
    {
        // Note: If NSApp is not running (which is the case if e.g a top-most
        // QEventLoop has been interrupted, and the second-most event loop has not
        // yet been reactivated (regardless if [NSApp run] is still on the stack)),
        // showing a native modal dialog will fail.
        return [mDelegate runApplicationModalPanel];
    }

    bool show(Qt::WindowModality windowModality, QWindow *parent)
    {
        Q_UNUSED(parent);
        if (windowModality != Qt::ApplicationModal)
            [mDelegate showModelessPanel];
        // no need to show a Qt::ApplicationModal dialog here, because it will be shown in runApplicationModalPanel
        return true;
    }

    void hide()
    {
        [mDelegate closePanel];
    }

    QColor currentColor() const
    {
        return mDelegate->mQtColor;
    }

    void setCurrentColor(const QColor &color)
    {
        // make sure that if ShowAlphaChannel option is set then also setShowsAlpha
        // needs to be set, otherwise alpha value is omitted
        if (color.alpha() < 255)
            [mDelegate->mColorPanel setShowsAlpha:YES];

        NSColor *nsColor;
        const QColor::Spec spec = color.spec();
        if (spec == QColor::Cmyk) {
            nsColor = [NSColor colorWithDeviceCyan:color.cyanF()
                                           magenta:color.magentaF()
                                            yellow:color.yellowF()
                                             black:color.blackF()
                                             alpha:color.alphaF()];
        } else {
            nsColor = [NSColor colorWithCalibratedRed:color.redF()
                                                green:color.greenF()
                                                 blue:color.blueF()
                                                alpha:color.alphaF()];
        }
        mDelegate->mQtColor = color;
        [mDelegate->mColorPanel setColor:nsColor];
    }

private:
    QNSColorPanelDelegate *mDelegate;
};

Q_GLOBAL_STATIC(QCocoaColorPanel, sharedColorPanel)

QCocoaColorDialogHelper::QCocoaColorDialogHelper()
{
}

QCocoaColorDialogHelper::~QCocoaColorDialogHelper()
{
    sharedColorPanel()->cleanup(this);
}

void QCocoaColorDialogHelper::exec()
{
    if (sharedColorPanel()->exec())
        emit accept();
    else
        emit reject();
}

bool QCocoaColorDialogHelper::show(Qt::WindowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    if (windowModality == Qt::ApplicationModal)
        windowModality = Qt::WindowModal;
    // Workaround for Apple rdar://25792119: If you invoke
    // -setShowsAlpha: multiple times before showing the color
    // picker, its height grows irrevocably.  Instead, only
    // invoke it once, when we show the dialog.
    [[NSColorPanel sharedColorPanel] setShowsAlpha:
            options()->testOption(QColorDialogOptions::ShowAlphaChannel)];

    sharedColorPanel()->init(this);
    return sharedColorPanel()->show(windowModality, parent);
}

void QCocoaColorDialogHelper::hide()
{
    sharedColorPanel()->hide();
}

void QCocoaColorDialogHelper::setCurrentColor(const QColor &color)
{
    sharedColorPanel()->init(this);
    sharedColorPanel()->setCurrentColor(color);
}

QColor QCocoaColorDialogHelper::currentColor() const
{
    return sharedColorPanel()->currentColor();
}

QT_END_NAMESPACE
