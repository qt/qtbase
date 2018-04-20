/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <QtCore/qtimer.h>
#include <QtGui/qfontdatabase.h>
#include <qpa/qplatformtheme.h>

#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <private/qfontengine_coretext_p.h>

#include "qcocoafontdialoghelper.h"
#include "qcocoahelpers.h"
#include "qcocoaeventdispatcher.h"

#import <AppKit/AppKit.h>

#if !CGFLOAT_DEFINED
typedef float CGFloat;  // Should only not be defined on 32-bit platforms
#endif

QT_USE_NAMESPACE

static QFont qfontForCocoaFont(NSFont *cocoaFont, const QFont &resolveFont)
{
    QFont newFont;
    if (cocoaFont) {
        int pSize = qRound([cocoaFont pointSize]);
        QCFType<CTFontDescriptorRef> font(CTFontCopyFontDescriptor((CTFontRef)cocoaFont));
        QString family(QCFString((CFStringRef)CTFontDescriptorCopyAttribute(font, kCTFontFamilyNameAttribute)));
        QString style(QCFString(((CFStringRef)CTFontDescriptorCopyAttribute(font, kCTFontStyleNameAttribute))));

        newFont = QFontDatabase().font(family, style, pSize);
        newFont.setUnderline(resolveFont.underline());
        newFont.setStrikeOut(resolveFont.strikeOut());
    }
    return newFont;
}

@interface QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) : NSObject<NSWindowDelegate, QNSPanelDelegate>
{
    @public
    NSFontPanel *mFontPanel;
    QCocoaFontDialogHelper *mHelper;
    NSView *mStolenContentView;
    QNSPanelContentsWrapper *mPanelButtons;
    QFont mQtFont;
    NSInteger mResultCode;
    BOOL mDialogIsExecuting;
    BOOL mResultSet;
};
- (void)restoreOriginalContentView;
- (void)updateQtFont;
- (void)changeFont:(id)sender;
- (void)finishOffWithCode:(NSInteger)code;
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSFontPanelDelegate);

@implementation QNSFontPanelDelegate

- (id)init
{
    self = [super init];
    mFontPanel = [NSFontPanel sharedFontPanel];
    mHelper = 0;
    mStolenContentView = 0;
    mPanelButtons = 0;
    mResultCode = NSModalResponseCancel;
    mDialogIsExecuting = false;
    mResultSet = false;

    [mFontPanel setRestorable:NO];
    [mFontPanel setDelegate:self];

    [NSFontManager sharedFontManager].target = self; // Action is changeFont:

    [mFontPanel retain];
    return self;
}

- (void)dealloc
{
    [mStolenContentView release];
    [mFontPanel setDelegate:nil];
    [NSFontManager sharedFontManager].target = nil;
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [super dealloc];
}

- (void)setDialogHelper:(QCocoaFontDialogHelper *)helper
{
    mHelper = helper;

    [mFontPanel setTitle:helper->options()->windowTitle().toNSString()];

    if (mHelper->options()->testOption(QFontDialogOptions::NoButtons)) {
        [self restoreOriginalContentView];
    } else if (!mStolenContentView) {
        // steal the font panel's contents view
        mStolenContentView = [mFontPanel contentView];
        [mStolenContentView retain];
        [mFontPanel setContentView:0];

        // create a new content view and add the stolen one as a subview
        mPanelButtons = [[QNSPanelContentsWrapper alloc] initWithPanelDelegate:self];
        [mPanelButtons addSubview:mStolenContentView];
        mPanelButtons.panelContentsMargins = NSEdgeInsetsMake(0, 0, 7, 0);
        mFontPanel.contentView = mPanelButtons;
        mFontPanel.defaultButtonCell = mPanelButtons.okButton.cell;
    }
}

- (void)closePanel
{
    [mFontPanel close];
}

- (void)restoreOriginalContentView
{
    if (mStolenContentView) {
        // return stolen stuff to its rightful owner
        [mStolenContentView removeFromSuperview];
        [mFontPanel setContentView:mStolenContentView];
        mStolenContentView = 0;
        [mPanelButtons release];
        mPanelButtons = nil;
    }
}

- (void)onOkClicked
{
    [mFontPanel close];
    [self finishOffWithCode:NSModalResponseOK];
}

- (void)onCancelClicked
{
    if (mPanelButtons) {
        [mFontPanel close];
        mQtFont = QFont();
        [self finishOffWithCode:NSModalResponseCancel];
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

    if (mHelper)
        emit mHelper->currentFontChanged(mQtFont);
}

- (void)showModelessPanel
{
    mDialogIsExecuting = false;
    mResultSet = false;
    [mFontPanel makeKeyAndOrderFront:mFontPanel];
}

- (BOOL)runApplicationModalPanel
{
    mDialogIsExecuting = true;
    // Call processEvents in case the event dispatcher has been interrupted, and needs to do
    // cleanup of modal sessions. Do this before showing the native dialog, otherwise it will
    // close down during the cleanup.
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

    // Make sure we don't interrupt the runModalForWindow call.
    QCocoaEventDispatcher::clearCurrentThreadCocoaEventDispatcherInterruptFlag();

    [NSApp runModalForWindow:mFontPanel];
    mDialogIsExecuting = false;
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
        [self updateQtFont];
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

class QCocoaFontPanel
{
public:
    QCocoaFontPanel()
    {
        mDelegate = [[QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) alloc] init];
    }

    ~QCocoaFontPanel()
    {
        [mDelegate release];
    }

    void init(QCocoaFontDialogHelper *helper)
    {
        [mDelegate setDialogHelper:helper];
    }

    void cleanup(QCocoaFontDialogHelper *helper)
    {
        if (mDelegate->mHelper == helper)
            mDelegate->mHelper = 0;
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
        if (windowModality != Qt::WindowModal)
            [mDelegate showModelessPanel];
        // no need to show a Qt::WindowModal dialog here, because it's necessary to call exec() in that case
        return true;
    }

    void hide()
    {
        [mDelegate closePanel];
    }

    QFont currentFont() const
    {
        return mDelegate->mQtFont;
    }

    void setCurrentFont(const QFont &font)
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
        nsFont = [mgr fontWithFamily:fontInfo.family().toNSString()
            traits:mask
            weight:weight
            size:fontInfo.pointSize()];

        [mgr setSelectedFont:const_cast<NSFont *>(nsFont) isMultiple:NO];
        mDelegate->mQtFont = font;
    }

private:
    QT_MANGLE_NAMESPACE(QNSFontPanelDelegate) *mDelegate;
};

Q_GLOBAL_STATIC(QCocoaFontPanel, sharedFontPanel)

QCocoaFontDialogHelper::QCocoaFontDialogHelper()
{
}

QCocoaFontDialogHelper::~QCocoaFontDialogHelper()
{
    sharedFontPanel()->cleanup(this);
}

void QCocoaFontDialogHelper::exec()
{
    if (sharedFontPanel()->exec())
        emit accept();
    else
        emit reject();
}

bool QCocoaFontDialogHelper::show(Qt::WindowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    if (windowModality == Qt::WindowModal)
        windowModality = Qt::ApplicationModal;
    sharedFontPanel()->init(this);
    return sharedFontPanel()->show(windowModality, parent);
}

void QCocoaFontDialogHelper::hide()
{
    sharedFontPanel()->hide();
}

void QCocoaFontDialogHelper::setCurrentFont(const QFont &font)
{
    sharedFontPanel()->init(this);
    sharedFontPanel()->setCurrentFont(font);
}

QFont QCocoaFontDialogHelper::currentFont() const
{
    return sharedFontPanel()->currentFont();
}

QT_END_NAMESPACE
