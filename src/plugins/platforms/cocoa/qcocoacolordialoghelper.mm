/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcocoacolordialoghelper.h"

#ifndef QT_NO_COLORDIALOG

#include <QtCore/qdebug.h>
#include <QtCore/qtimer.h>
#include <QtWidgets/qdialogbuttonbox.h>
#include <QtWidgets/qcolordialog.h>

#include "qcocoahelpers.h"

#import <AppKit/AppKit.h>

QT_USE_NAMESPACE

static NSButton *macCreateButton(const char *text, NSView *superview)
{
    static const NSRect buttonFrameRect = { { 0.0, 0.0 }, { 0.0, 0.0 } };

    NSButton *button = [[NSButton alloc] initWithFrame:buttonFrameRect];
    [button setButtonType:NSMomentaryLightButton];
    [button setBezelStyle:NSRoundedBezelStyle];
    [button setTitle:(NSString*)(CFStringRef)QCFString(QDialogButtonBox::tr(text)
                                                       .remove(QLatin1Char('&')))];
    [[button cell] setFont:[NSFont systemFontOfSize:
            [NSFont systemFontSizeForControlSize:NSRegularControlSize]]];
    [superview addSubview:button];
    return button;
}

@class QT_MANGLE_NAMESPACE(QNSColorPanelDelegate);

@interface QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) : NSObject<NSWindowDelegate>
{
    @public
    NSColorPanel *mColorPanel;
    QCocoaColorDialogHelper *mHelper;
    NSView *mStolenContentView;
    NSButton *mOkButton;
    NSButton *mCancelButton;
    QColor mQtColor;
    NSInteger mResultCode;
    BOOL mDialogIsExecuting;
    BOOL mResultSet;
};
- (void)relayout;
- (void)updateQtColor;
- (void)finishOffWithCode:(NSInteger)code;
@end

@implementation QT_MANGLE_NAMESPACE(QNSColorPanelDelegate)

- (id)initWithDialogHelper:(QCocoaColorDialogHelper *)helper
{
    self = [super init];
    mColorPanel = [NSColorPanel sharedColorPanel];
    mHelper = helper;
    mResultCode = NSCancelButton;
    mDialogIsExecuting = false;
    mResultSet = false;

    if (mHelper->options()->testOption(QColorDialogOptions::NoButtons)) {
        mStolenContentView = 0;
        mOkButton = 0;
        mCancelButton = 0;
    } else {
        // steal the color panel's contents view
        mStolenContentView = [mColorPanel contentView];
        [mStolenContentView retain];
        [mColorPanel setContentView:0];

        // create a new content view and add the stolen one as a subview
        NSRect frameRect = { { 0.0, 0.0 }, { 0.0, 0.0 } };
        NSView *ourContentView = [[NSView alloc] initWithFrame:frameRect];
        [ourContentView addSubview:mStolenContentView];

        // create OK and Cancel buttons and add these as subviews
        mOkButton = macCreateButton("&OK", ourContentView);
        mCancelButton = macCreateButton("Cancel", ourContentView);

        [mColorPanel setContentView:ourContentView];
        [mColorPanel setDefaultButtonCell:[mOkButton cell]];
        [self relayout];

        [mOkButton setAction:@selector(onOkClicked)];
        [mOkButton setTarget:self];

        [mCancelButton setAction:@selector(onCancelClicked)];
        [mCancelButton setTarget:self];
    }

    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(colorChanged:)
        name:NSColorPanelColorDidChangeNotification
        object:mColorPanel];

    [mColorPanel retain];
    return self;
}

- (void)dealloc
{
    if (mOkButton) {
        NSView *ourContentView = [mColorPanel contentView];

        // return stolen stuff to its rightful owner
        [mStolenContentView removeFromSuperview];
        [mColorPanel setContentView:mStolenContentView];
        [mOkButton release];
        [mCancelButton release];
        [ourContentView release];
    }

    [mColorPanel setDelegate:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [super dealloc];
}

- (void)closePanel
{
    [mColorPanel close];
}

- (void)windowDidResize:(NSNotification *)notification
{
    Q_UNUSED(notification);
    [self relayout];
}

- (void)colorChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);
    [self updateQtColor];
    emit mHelper->colorSelected(mQtColor);
}

- (void)relayout
{
    if (!mOkButton)
        return;

    NSRect rect = [[mStolenContentView superview] frame];

    // should a priori be kept in sync with qfontdialog_mac.mm
    const CGFloat ButtonMinWidth = 78.0; // 84.0 for Carbon
    const CGFloat ButtonMinHeight = 32.0;
    const CGFloat ButtonSpacing = 0.0;
    const CGFloat ButtonTopMargin = 0.0;
    const CGFloat ButtonBottomMargin = 7.0;
    const CGFloat ButtonSideMargin = 9.0;

    [mOkButton sizeToFit];
    NSSize okSizeHint = [mOkButton frame].size;

    [mCancelButton sizeToFit];
    NSSize cancelSizeHint = [mCancelButton frame].size;

    const CGFloat ButtonWidth = qMin(qMax(ButtonMinWidth,
                                          qMax(okSizeHint.width, cancelSizeHint.width)),
                                     CGFloat((rect.size.width - 2.0 * ButtonSideMargin - ButtonSpacing) * 0.5));
    const CGFloat ButtonHeight = qMax(ButtonMinHeight,
                                     qMax(okSizeHint.height, cancelSizeHint.height));

    NSRect okRect = { { rect.size.width - ButtonSideMargin - ButtonWidth,
                        ButtonBottomMargin },
                      { ButtonWidth, ButtonHeight } };
    [mOkButton setFrame:okRect];
    [mOkButton setNeedsDisplay:YES];

    NSRect cancelRect = { { okRect.origin.x - ButtonSpacing - ButtonWidth,
                            ButtonBottomMargin },
                            { ButtonWidth, ButtonHeight } };
    [mCancelButton setFrame:cancelRect];
    [mCancelButton setNeedsDisplay:YES];

    const CGFloat Y = ButtonBottomMargin + ButtonHeight + ButtonTopMargin;
    NSRect stolenCVRect = { { 0.0, Y },
                            { rect.size.width, rect.size.height - Y } };
    [mStolenContentView setFrame:stolenCVRect];
    [mStolenContentView setNeedsDisplay:YES];

    [[mStolenContentView superview] setNeedsDisplay:YES];
}

- (void)onOkClicked
{
    Q_ASSERT(mHackedPanel);
    [mColorPanel close];
    [self updateQtColor];
    [self finishOffWithCode:NSOKButton];
}

- (void)onCancelClicked
{
    if (mOkButton) {
        [mColorPanel close];
        mQtColor = QColor();
        [self finishOffWithCode:NSCancelButton];
    }
}

- (void)updateQtColor
{
    NSColor *color = [mColorPanel color];
    NSString *colorSpaceName = [color colorSpaceName];
    if (colorSpaceName == NSDeviceCMYKColorSpace) {
        CGFloat cyan = 0, magenta = 0, yellow = 0, black = 0, alpha = 0;
        [color getCyan:&cyan magenta:&magenta yellow:&yellow black:&black alpha:&alpha];
        mQtColor.setCmykF(cyan, magenta, yellow, black, alpha);
    } else if (colorSpaceName == NSCalibratedRGBColorSpace || colorSpaceName == NSDeviceRGBColorSpace)  {
        CGFloat red = 0, green = 0, blue = 0, alpha = 0;
        [color getRed:&red green:&green blue:&blue alpha:&alpha];
        mQtColor.setRgbF(red, green, blue, alpha);
    } else if (colorSpaceName == NSNamedColorSpace) {
        NSColor *tmpColor = [color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
        CGFloat red = 0, green = 0, blue = 0, alpha = 0;
        [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
        mQtColor.setRgbF(red, green, blue, alpha);
    } else {
        NSColorSpace *colorSpace = [color colorSpace];
        if ([colorSpace colorSpaceModel] == NSCMYKColorSpaceModel && [color numberOfComponents] == 5){
            CGFloat components[5];
            [color getComponents:components];
            mQtColor.setCmykF(components[0], components[1], components[2], components[3], components[4]);
        } else {
            NSColor *tmpColor = [color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
            CGFloat red = 0, green = 0, blue = 0, alpha = 0;
            [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
            mQtColor.setRgbF(red, green, blue, alpha);
        }
    }
    emit mHelper->currentColorChanged(mQtColor);
}

- (void)showModelessPanel
{
    mDialogIsExecuting = false;
    [mColorPanel makeKeyAndOrderFront:mColorPanel];
}

- (BOOL)runApplicationModalPanel
{
    mDialogIsExecuting = true;
    [mColorPanel setDelegate:self];
    [mColorPanel setContinuous:YES];
    [NSApp runModalForWindow:mColorPanel];
    return (mResultCode == NSOKButton);
}

- (QT_PREPEND_NAMESPACE(QPlatformDialogHelper::DialogCode))dialogResultCode
{
    return (mResultCode == NSOKButton) ? QT_PREPEND_NAMESPACE(QPlatformDialogHelper::Accepted) : QT_PREPEND_NAMESPACE(QPlatformDialogHelper::Rejected);
}

- (BOOL)windowShouldClose:(id)window
{
    Q_UNUSED(window);
    if (!mOkButton)
        [self updateQtColor];
    if (mDialogIsExecuting) {
        [self finishOffWithCode:NSCancelButton];
    } else {
        mResultSet = true;
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
            if (mResultCode == NSCancelButton) {
                emit mHelper->reject();
            } else {
                emit mHelper->accept();
            }
        }
    }
}

@end

QT_BEGIN_NAMESPACE

QCocoaColorDialogHelper::QCocoaColorDialogHelper() :
    mDelegate(0)
{
}

QCocoaColorDialogHelper::~QCocoaColorDialogHelper()
{
    deleteNativeDialog_sys();
}

void QCocoaColorDialogHelper::platformNativeDialogModalHelp()
{
    // Do a queued meta-call to open the native modal dialog so it opens after the new
    // event loop has started to execute (in QDialog::exec). Using a timer rather than
    // a queued meta call is intentional to ensure that the call is only delivered when
    // [NSApp run] runs (timers are handeled special in cocoa). If NSApp is not
    // running (which is the case if e.g a top-most QEventLoop has been
    // interrupted, and the second-most event loop has not yet been reactivated (regardless
    // if [NSApp run] is still on the stack)), showing a native modal dialog will fail.
    QTimer::singleShot(1, this, SIGNAL(launchNativeAppModalPanel()));
}

void QCocoaColorDialogHelper::_q_platformRunNativeAppModalPanel()
{
    // TODO:
#if 0
    QBoolBlocker nativeDialogOnTop(QApplicationPrivate::native_modal_dialog_active);
#endif
    QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *>(mDelegate);
    [delegate runApplicationModalPanel];
    if (dialogResultCode_sys() == QPlatformDialogHelper::Accepted)
        emit accept();
    else
        emit reject();
}

void QCocoaColorDialogHelper::deleteNativeDialog_sys()
{
    if (!mDelegate)
        return;
    [reinterpret_cast<QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *>(mDelegate) release];
    mDelegate = 0;
}

bool QCocoaColorDialogHelper::show_sys(QFlags<QPlatformDialogHelper::ShowFlag>, Qt::WindowFlags, QWindow *parent)
{
    return showCocoaColorPanel(parent);
}

void QCocoaColorDialogHelper::hide_sys()
{
    if (!mDelegate)
        return;
    [reinterpret_cast<QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *>(mDelegate)->mColorPanel close];
}

QCocoaColorDialogHelper::DialogCode QCocoaColorDialogHelper::dialogResultCode_sys()
{
    QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *>(mDelegate);
    return [delegate dialogResultCode];
}

void QCocoaColorDialogHelper::setCurrentColor_sys(const QColor &color)
{
    if (!mDelegate)
        createNSColorPanelDelegate();
    QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *>(mDelegate);
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
    delegate->mQtColor = color;
    [delegate->mColorPanel setColor:nsColor];
}

QColor QCocoaColorDialogHelper::currentColor_sys() const
{
    return reinterpret_cast<QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *>(mDelegate)->mQtColor;
}

void QCocoaColorDialogHelper::createNSColorPanelDelegate()
{
    if (mDelegate)
        return;

    QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *delegate = [[QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) alloc]
          initWithDialogHelper:this];

    mDelegate = delegate;
}

bool QCocoaColorDialogHelper::showCocoaColorPanel(QWindow *parent)
{
    Q_UNUSED(parent);
    createNSColorPanelDelegate();
    QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *>(mDelegate);
    [delegate->mColorPanel setShowsAlpha:options()->testOption(QColorDialogOptions::ShowAlphaChannel)];
    [delegate showModelessPanel];
    return true;
}

bool QCocoaColorDialogHelper::hideCocoaColorPanel()
{
    if (!mDelegate){
        return false;
    } else {
        QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *>(mDelegate);
        [delegate closePanel];
        return true;
    }
}

QT_END_NAMESPACE

#endif // QT_NO_COLORDIALOG
