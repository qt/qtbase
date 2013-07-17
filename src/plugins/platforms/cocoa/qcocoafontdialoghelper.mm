/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcocoafontdialoghelper.h"

#ifndef QT_NO_FONTDIALOG

#include <QtCore/qtimer.h>
#include <QtGui/qfontdatabase.h>

#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <private/qfontengine_coretext_p.h>

#include "qcocoahelpers.h"

#import <AppKit/AppKit.h>

#if !CGFLOAT_DEFINED
typedef float CGFloat;  // Should only not be defined on 32-bit platforms
#endif

QT_USE_NAMESPACE

// should a priori be kept in sync with qcolordialog_mac.mm
const CGFloat ButtonMinWidth = 78.0;
const CGFloat ButtonMinHeight = 32.0;
const CGFloat ButtonSpacing = 0.0;
const CGFloat ButtonTopMargin = 0.0;
const CGFloat ButtonBottomMargin = 7.0;
const CGFloat ButtonSideMargin = 9.0;

// looks better with some margins
const CGFloat DialogTopMargin = 7.0;
const CGFloat DialogSideMargin = 9.0;

static NSButton *macCreateButton(const char *text, NSView *superview)
{
    static const NSRect buttonFrameRect = { { 0.0, 0.0 }, { 0.0, 0.0 } };

    NSButton *button = [[NSButton alloc] initWithFrame:buttonFrameRect];
    [button setButtonType:NSMomentaryLightButton];
    [button setBezelStyle:NSRoundedBezelStyle];
    [button setTitle:(NSString*)(CFStringRef)QCFString(QCoreApplication::translate("QDialogButtonBox", text)
                                                       .remove(QLatin1Char('&')))];
    [[button cell] setFont:[NSFont systemFontOfSize:
            [NSFont systemFontSizeForControlSize:NSRegularControlSize]]];
    [superview addSubview:button];
    return button;
}

static QFont qfontForCocoaFont(NSFont *cocoaFont, const QFont &resolveFont)
{
    QFont newFont;
    if (cocoaFont) {
        int pSize = qRound([cocoaFont pointSize]);
        CTFontDescriptorRef font = CTFontCopyFontDescriptor((CTFontRef)cocoaFont);
        // QCoreTextFontDatabase::populateFontDatabase() is using localized names
        QString family = QCFString::toQString((CFStringRef) CTFontDescriptorCopyLocalizedAttribute(font, kCTFontFamilyNameAttribute, NULL));
        QString style = QCFString::toQString((CFStringRef) CTFontDescriptorCopyLocalizedAttribute(font, kCTFontStyleNameAttribute, NULL));

        newFont = QFontDatabase().font(family, style, pSize);
        newFont.setUnderline(resolveFont.underline());
        newFont.setStrikeOut(resolveFont.strikeOut());

        CFRelease(font);
    }
    return newFont;
}

@class QT_MANGLE_NAMESPACE(QNSFontPanelDelegate);

@interface QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) : NSObject<NSWindowDelegate>
{
    @public
    NSFontPanel *mFontPanel;
    QCocoaFontDialogHelper *mHelper;
    NSView *mStolenContentView;
    NSButton *mOkButton;
    NSButton *mCancelButton;
    QFont mQtFont;
    NSInteger mResultCode;
    BOOL mDialogIsExecuting;
    BOOL mResultSet;
};
- (void)relayout;
- (void)relayoutToContentSize:(NSSize)frameSize;
- (void)updateQtFont;
- (void)changeFont:(id)sender;
- (void)finishOffWithCode:(NSInteger)code;
@end

@implementation QT_MANGLE_NAMESPACE(QNSFontPanelDelegate)

- (id)initWithDialogHelper:
    (QCocoaFontDialogHelper *)helper
{
    self = [super init];
    mFontPanel = [NSFontPanel sharedFontPanel];
    mHelper = helper;
    mResultCode = NSCancelButton;
    mDialogIsExecuting = false;
    mResultSet = false;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7)
        [mFontPanel setRestorable:NO];
#endif

    [mFontPanel setTitle:QCFString::toNSString(helper->options()->windowTitle())];

    if (mHelper->options()->testOption(QFontDialogOptions::NoButtons)) {
        mStolenContentView = 0;
        mOkButton = 0;
        mCancelButton = 0;
    } else {
        // steal the font panel's contents view
        mStolenContentView = [mFontPanel contentView];
        [mStolenContentView retain];
        [mFontPanel setContentView:0];

        // create a new content view and add the stolen one as a subview
        NSRect frameRect = { { 0.0, 0.0 }, { 0.0, 0.0 } };
        NSView *ourContentView = [[NSView alloc] initWithFrame:frameRect];
        [ourContentView addSubview:mStolenContentView];

        // create OK and Cancel buttons and add these as subviews
        mOkButton = macCreateButton("&OK", ourContentView);
        mCancelButton = macCreateButton("Cancel", ourContentView);

        [mFontPanel setContentView:ourContentView];
        [mFontPanel setDefaultButtonCell:[mOkButton cell]];
        [self relayoutToContentSize:[[mStolenContentView superview] frame].size];

        [mOkButton setAction:@selector(onOkClicked)];
        [mOkButton setTarget:self];

        [mCancelButton setAction:@selector(onCancelClicked)];
        [mCancelButton setTarget:self];
    }

    [mFontPanel retain];
    return self;
}

- (void)dealloc
{
    if (mOkButton) {
        NSView *ourContentView = [mFontPanel contentView];

        // return stolen stuff to its rightful owner
        [mStolenContentView removeFromSuperview];
        [mFontPanel setContentView:mStolenContentView];
        [mOkButton release];
        [mCancelButton release];
        [ourContentView release];
    }

    [mFontPanel setDelegate:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [super dealloc];
}

- (void)closePanel
{
    [mFontPanel close];
}

- (void)windowDidResize:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (mOkButton)
        [self relayout];
}

- (void)relayout
{
    [self relayoutToContentSize:[[mStolenContentView superview] frame].size];
}

- (void)relayoutToContentSize:(NSSize)frameSize
{
    Q_ASSERT(mOkButton);

    [mOkButton sizeToFit];
    NSSize okSizeHint = [mOkButton frame].size;

    [mCancelButton sizeToFit];
    NSSize cancelSizeHint = [mCancelButton frame].size;

    const CGFloat ButtonWidth = qMin(qMax(ButtonMinWidth,
                qMax(okSizeHint.width, cancelSizeHint.width)),
            CGFloat((frameSize.width - 2.0 * ButtonSideMargin - ButtonSpacing) * 0.5));
    const CGFloat ButtonHeight = qMax(ButtonMinHeight,
                                     qMax(okSizeHint.height, cancelSizeHint.height));

    const CGFloat X = DialogSideMargin;
    const CGFloat Y = ButtonBottomMargin + ButtonHeight + ButtonTopMargin;

    NSRect okRect = { { frameSize.width - ButtonSideMargin - ButtonWidth,
                        ButtonBottomMargin },
                      { ButtonWidth, ButtonHeight } };
    [mOkButton setFrame:okRect];
    [mOkButton setNeedsDisplay:YES];

    NSRect cancelRect = { { okRect.origin.x - ButtonSpacing - ButtonWidth,
                            ButtonBottomMargin },
                            { ButtonWidth, ButtonHeight } };
    [mCancelButton setFrame:cancelRect];
    [mCancelButton setNeedsDisplay:YES];

    NSRect stolenCVRect = { { X, Y },
                            { frameSize.width - X - X, frameSize.height - Y - DialogTopMargin } };
    [mStolenContentView setFrame:stolenCVRect];
    [mStolenContentView setNeedsDisplay:YES];

    [[mStolenContentView superview] setNeedsDisplay:YES];
}


- (void)onOkClicked
{
    [mFontPanel close];
    [self finishOffWithCode:NSOKButton];
}

- (void)onCancelClicked
{
    if (mOkButton) {
        [mFontPanel close];
        mQtFont = QFont();
        [self finishOffWithCode:NSCancelButton];
    }
}

- (void)changeFont:(id)sender
{
    Q_UNUSED(sender);
    [self updateQtFont];
}

- (void)updateQtFont
{
    // Get selected font
    NSFontManager *fontManager = [NSFontManager sharedFontManager];
    NSFont *selectedFont = [fontManager selectedFont];
    if (selectedFont == nil) {
        selectedFont = [NSFont systemFontOfSize:[NSFont systemFontSize]];
    }
    NSFont *panelFont = [fontManager convertFont:selectedFont];
    mQtFont = qfontForCocoaFont(panelFont, mQtFont);

    emit mHelper->currentFontChanged(mQtFont);
}

- (void)showModelessPanel
{
    mDialogIsExecuting = false;
    [mFontPanel makeKeyAndOrderFront:mFontPanel];
}

- (BOOL)runApplicationModalPanel
{
    mDialogIsExecuting = true;
    [mFontPanel setDelegate:self];
    // Call processEvents in case the event dispatcher has been interrupted, and needs to do
    // cleanup of modal sessions. Do this before showing the native dialog, otherwise it will
    // close down during the cleanup.
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    [NSApp runModalForWindow:mFontPanel];
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
        [self updateQtFont];
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
        // we accept/reject QFontDialog, since QFontDialog has its
        // own event loop that needs to be stopped last.
        [NSApp stopModalWithCode:code];
    } else {
        // Since we are not in a modal event loop, we can safely close
        // down QFontDialog
        // Calling accept() or reject() can in turn call closeCocoaFontPanel.
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

QCocoaFontDialogHelper::QCocoaFontDialogHelper() :
    mDelegate(0)
{
}

QCocoaFontDialogHelper::~QCocoaFontDialogHelper()
{
    if (!mDelegate)
        return;
    [reinterpret_cast<QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *>(mDelegate) release];
    mDelegate = 0;
}

void QCocoaFontDialogHelper::exec()
{
    // Note: If NSApp is not running (which is the case if e.g a top-most
    // QEventLoop has been interrupted, and the second-most event loop has not
    // yet been reactivated (regardless if [NSApp run] is still on the stack)),
    // showing a native modal dialog will fail.
    QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *>(mDelegate);
    if ([delegate runApplicationModalPanel])
        emit accept();
    else
        emit reject();
}

bool QCocoaFontDialogHelper::show(Qt::WindowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    if (windowModality == Qt::WindowModal) {
        // Cocoa's shared font panel cannot be shown as a sheet
        return false;
    }
    return showCocoaFontPanel(windowModality, parent);
}

void QCocoaFontDialogHelper::hide()
{
    if (!mDelegate)
        return;
    [reinterpret_cast<QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *>(mDelegate)->mFontPanel close];
}

void QCocoaFontDialogHelper::setCurrentFont(const QFont &font)
{
    NSFontManager *mgr = [NSFontManager sharedFontManager];
    const NSFont *nsFont = 0;

    int weight = 5;
    NSFontTraitMask mask = 0;
    if (font.style() == QFont::StyleItalic) {
        mask |= NSItalicFontMask;
    }
    if (font.weight() == QFont::Bold) {
        weight = 9;
        mask |= NSBoldFontMask;
    }

    QFontInfo fontInfo(font);
    nsFont = [mgr fontWithFamily:QCFString::toNSString(fontInfo.family())
        traits:mask
        weight:weight
        size:fontInfo.pointSize()];

    if (!mDelegate)
        createNSFontPanelDelegate();

    [mgr setSelectedFont:const_cast<NSFont *>(nsFont) isMultiple:NO];
    static_cast<QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *>(mDelegate)->mQtFont = font;
}

QFont QCocoaFontDialogHelper::currentFont() const
{
    if (!mDelegate)
        return QFont();
    return reinterpret_cast<QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *>(mDelegate)->mQtFont;
}

void QCocoaFontDialogHelper::createNSFontPanelDelegate()
{
    if (mDelegate)
        return;

    QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *delegate = [[QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) alloc]
          initWithDialogHelper:this];

    mDelegate = delegate;
}

bool QCocoaFontDialogHelper::showCocoaFontPanel(Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(parent);
    createNSFontPanelDelegate();
    QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *>(mDelegate);
    if (windowModality == Qt::NonModal)
        [delegate showModelessPanel];
    // no need to show a Qt::ApplicationModal dialog here, since it will be done in _q_platformRunNativeAppModalPanel()
    return true;
}

bool QCocoaFontDialogHelper::hideCocoaFontPanel()
{
    if (!mDelegate){
        return false;
    } else {
        QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *>(mDelegate);
        [delegate closePanel];
        return true;
    }
}

QT_END_NAMESPACE

#endif // QT_NO_FONTDIALOG
