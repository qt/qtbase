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

#include <qpa/qplatformtheme.h>

#include "qcocoahelpers.h"
#include "qnsview.h"

#include <QtCore>
#include <QtGui>
#include <qpa/qplatformscreen.h>
#include <private/qguiapplication_p.h>
#include <private/qwindow_p.h>
#include <QtGui/private/qcoregraphics_p.h>

#ifndef QT_NO_WIDGETS
#include <QtWidgets/QWidget>
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaWindow, "qt.qpa.window");
Q_LOGGING_CATEGORY(lcQpaDrawing, "qt.qpa.drawing");
Q_LOGGING_CATEGORY(lcQpaMouse, "qt.qpa.input.mouse", QtCriticalMsg);
Q_LOGGING_CATEGORY(lcQpaScreen, "qt.qpa.screen", QtCriticalMsg);

//
// Conversion Functions
//

QStringList qt_mac_NSArrayToQStringList(NSArray<NSString *> *array)
{
    QStringList result;
    for (NSString *string in array)
        result << QString::fromNSString(string);
    return result;
}

NSMutableArray<NSString *> *qt_mac_QStringListToNSMutableArray(const QStringList &list)
{
    NSMutableArray<NSString *> *result = [NSMutableArray<NSString *> arrayWithCapacity:list.size()];
    for (const QString &string : list)
        [result addObject:string.toNSString()];
    return result;
}

struct dndenum_mapper
{
    NSDragOperation mac_code;
    Qt::DropAction qt_code;
    bool Qt2Mac;
};

static dndenum_mapper dnd_enums[] = {
    { NSDragOperationLink,  Qt::LinkAction, true },
    { NSDragOperationMove,  Qt::MoveAction, true },
    { NSDragOperationDelete,  Qt::MoveAction, true },
    { NSDragOperationCopy,  Qt::CopyAction, true },
    { NSDragOperationGeneric,  Qt::CopyAction, false },
    { NSDragOperationEvery, Qt::ActionMask, false },
    { NSDragOperationNone, Qt::IgnoreAction, false }
};

NSDragOperation qt_mac_mapDropAction(Qt::DropAction action)
{
    for (int i=0; dnd_enums[i].qt_code; i++) {
        if (dnd_enums[i].Qt2Mac && (action & dnd_enums[i].qt_code)) {
            return dnd_enums[i].mac_code;
        }
    }
    return NSDragOperationNone;
}

NSDragOperation qt_mac_mapDropActions(Qt::DropActions actions)
{
    NSDragOperation nsActions = NSDragOperationNone;
    for (int i=0; dnd_enums[i].qt_code; i++) {
        if (dnd_enums[i].Qt2Mac && (actions & dnd_enums[i].qt_code))
            nsActions |= dnd_enums[i].mac_code;
    }
    return nsActions;
}

Qt::DropAction qt_mac_mapNSDragOperation(NSDragOperation nsActions)
{
    Qt::DropAction action = Qt::IgnoreAction;
    for (int i=0; dnd_enums[i].mac_code; i++) {
        if (nsActions & dnd_enums[i].mac_code)
            return dnd_enums[i].qt_code;
    }
    return action;
}

Qt::DropActions qt_mac_mapNSDragOperations(NSDragOperation nsActions)
{
    Qt::DropActions actions = Qt::IgnoreAction;

    for (int i=0; dnd_enums[i].mac_code; i++) {
        if (dnd_enums[i].mac_code == NSDragOperationEvery)
            continue;

        if (nsActions & dnd_enums[i].mac_code)
            actions |= dnd_enums[i].qt_code;
    }
    return actions;
}

/*!
    Returns the view cast to a QNSview if possible.

    If the view is not a QNSView, nil is returned, which is safe to
    send messages to, effectivly making [qnsview_cast(view) message]
    a no-op.

    For extra verbosity and clearer code, please consider checking
    that the platform window is not a foreign window before using
    this cast, via QPlatformWindow::isForeignWindow().

    Do not use this method soley to check for foreign windows, as
    that will make the code harder to read for people not working
    primarily on macOS, who do not know the difference between the
    NSView and QNSView cases.
*/
QNSView *qnsview_cast(NSView *view)
{
    return qt_objc_cast<QNSView *>(view);
}

//
// Misc
//

// Sets the activation policy for this process to NSApplicationActivationPolicyRegular,
// unless either LSUIElement or LSBackgroundOnly is set in the Info.plist.
void qt_mac_transformProccessToForegroundApplication()
{
    bool forceTransform = true;
    CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
                                                           CFSTR("LSUIElement"));
    if (value) {
        CFTypeID valueType = CFGetTypeID(value);
        // Officially it's supposed to be a string, a boolean makes sense, so we'll check.
        // A number less so, but OK.
        if (valueType == CFStringGetTypeID())
            forceTransform = !(QString::fromCFString(static_cast<CFStringRef>(value)).toInt());
        else if (valueType == CFBooleanGetTypeID())
            forceTransform = !CFBooleanGetValue(static_cast<CFBooleanRef>(value));
        else if (valueType == CFNumberGetTypeID()) {
            int valueAsInt;
            CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &valueAsInt);
            forceTransform = !valueAsInt;
        }
    }

    if (forceTransform) {
        value = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
                                                     CFSTR("LSBackgroundOnly"));
        if (value) {
            CFTypeID valueType = CFGetTypeID(value);
            if (valueType == CFBooleanGetTypeID())
                forceTransform = !CFBooleanGetValue(static_cast<CFBooleanRef>(value));
            else if (valueType == CFStringGetTypeID())
                forceTransform = !(QString::fromCFString(static_cast<CFStringRef>(value)).toInt());
            else if (valueType == CFNumberGetTypeID()) {
                int valueAsInt;
                CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &valueAsInt);
                forceTransform = !valueAsInt;
            }
        }
    }

    if (forceTransform) {
        [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
    }
}

QString qt_mac_applicationName()
{
    QString appName;
    CFTypeRef string = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleName"));
    if (string)
        appName = QString::fromCFString(static_cast<CFStringRef>(string));

    if (appName.isEmpty()) {
        QString arg0 = QGuiApplicationPrivate::instance()->appName();
        if (arg0.contains("/")) {
            QStringList parts = arg0.split(QLatin1Char('/'));
            appName = parts.at(parts.count() - 1);
        } else {
            appName = arg0;
        }
    }
    return appName;
}

// -------------------------------------------------------------------------

/*!
    \fn QPointF qt_mac_flip(const QPointF &pos, const QRectF &reference)
    \fn QRectF qt_mac_flip(const QRectF &rect, const QRectF &reference)

    Flips the Y coordinate of the point/rect between quadrant I and IV.

    The native coordinate system on macOS uses quadrant I, with origin
    in bottom left, and Qt uses quadrant IV, with origin in top left.

    By flipping the Y coordinate, we can map the point/rect between
    the two coordinate systems.

    The flip is always in relation to a reference rectangle, e.g.
    the frame of the parent view, or the screen geometry. In the
    latter case the specialized QCocoaScreen::mapFrom/To functions
    should be used instead.
*/
QPointF qt_mac_flip(const QPointF &pos, const QRectF &reference)
{
    return QPointF(pos.x(), reference.height() - pos.y());
}

QRectF qt_mac_flip(const QRectF &rect, const QRectF &reference)
{
    return QRectF(qt_mac_flip(rect.bottomLeft(), reference), rect.size());
}

// -------------------------------------------------------------------------

/*!
  \fn Qt::MouseButton cocoaButton2QtButton(NSInteger buttonNum)

  Returns the Qt::Button that corresponds to an NSEvent.buttonNumber.

  \note AppKit will use buttonNumber 0 to indicate both "left button"
  and "no button". Only NSEvents that describes mouse press/release
  events (e.g NSEventTypeOtherMouseDown) will contain a valid
  button number.
*/
Qt::MouseButton cocoaButton2QtButton(NSInteger buttonNum)
{
    if (buttonNum >= 0 && buttonNum <= 31)
        return Qt::MouseButton(1 << buttonNum);
    return Qt::NoButton;
}

/*!
  \fn Qt::MouseButton cocoaButton2QtButton(NSEvent *event)

  Returns the Qt::Button that corresponds to an NSEvent.buttonNumber.

  \note AppKit will use buttonNumber 0 to indicate both "left button"
  and "no button". Only NSEvents that describes mouse press/release/dragging
  events (e.g NSEventTypeOtherMouseDown) will contain a valid
  button number.

  \note Wacom tablet might not return the correct button number for NSEvent buttonNumber
  on right clicks. Decide here that the button is the "right" button.
*/
Qt::MouseButton cocoaButton2QtButton(NSEvent *event)
{
    if (cocoaEvent2QtMouseEvent(event) == QEvent::MouseMove)
        return Qt::NoButton;

    switch (event.type) {
    case NSEventTypeRightMouseUp:
    case NSEventTypeRightMouseDown:
        return Qt::RightButton;

    default:
        break;
    }

    return cocoaButton2QtButton(event.buttonNumber);
}

/*!
  \fn QEvent::Type cocoaEvent2QtMouseEvent(NSEvent *event)

  Returns the QEvent::Type that corresponds to an NSEvent.type.
*/
QEvent::Type cocoaEvent2QtMouseEvent(NSEvent *event)
{
    switch (event.type) {
    case NSEventTypeLeftMouseDown:
    case NSEventTypeRightMouseDown:
    case NSEventTypeOtherMouseDown:
        return QEvent::MouseButtonPress;

    case NSEventTypeLeftMouseUp:
    case NSEventTypeRightMouseUp:
    case NSEventTypeOtherMouseUp:
        return QEvent::MouseButtonRelease;

    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDragged:
        return QEvent::MouseMove;

    case NSEventTypeMouseMoved:
        return QEvent::MouseMove;

    default:
        break;
    }

    return QEvent::None;
}

/*!
  \fn Qt::MouseButtons cocoaMouseButtons2QtMouseButtons(NSInteger pressedMouseButtons)

  Returns the Qt::MouseButtons that corresponds to an NSEvent.pressedMouseButtons.
*/
Qt::MouseButtons cocoaMouseButtons2QtMouseButtons(NSInteger pressedMouseButtons)
{
  return static_cast<Qt::MouseButton>(pressedMouseButtons & Qt::MouseButtonMask);
}

/*!
  \fn Qt::MouseButtons currentlyPressedMouseButtons()

  Returns the Qt::MouseButtons that corresponds to an NSEvent.pressedMouseButtons.
*/
Qt::MouseButtons currentlyPressedMouseButtons()
{
  return cocoaMouseButtons2QtMouseButtons(NSEvent.pressedMouseButtons);
}

QString qt_mac_removeAmpersandEscapes(QString s)
{
    return QPlatformTheme::removeMnemonics(s).trimmed();
}

QT_END_NAMESPACE

/*! \internal

    This NSView derived class is used to add OK/Cancel
    buttons to NSColorPanel and NSFontPanel. It replaces
    the panel's content view, while reparenting the former
    content view into itself. It also takes care of setting
    the target-action for the OK/Cancel buttons and making
    sure the layout is consistent.
 */
@implementation QNSPanelContentsWrapper {
    NSButton *_okButton;
    NSButton *_cancelButton;
    NSView *_panelContents;
    NSEdgeInsets _panelContentsMargins;
}

@synthesize okButton = _okButton;
@synthesize cancelButton = _cancelButton;
@synthesize panelContents = _panelContents;
@synthesize panelContentsMargins = _panelContentsMargins;

- (instancetype)initWithPanelDelegate:(id<QNSPanelDelegate>)panelDelegate
{
    if ((self = [super initWithFrame:NSZeroRect])) {
        // create OK and Cancel buttons and add these as subviews
        _okButton = [self createButtonWithTitle:QPlatformDialogHelper::Ok];
        _okButton.action = @selector(onOkClicked);
        _okButton.target = panelDelegate;
        _cancelButton = [self createButtonWithTitle:QPlatformDialogHelper::Cancel];
        _cancelButton.action = @selector(onCancelClicked);
        _cancelButton.target = panelDelegate;

        _panelContents = nil;

        _panelContentsMargins = NSEdgeInsetsMake(0, 0, 0, 0);
    }

    return self;
}

- (void)dealloc
{
    [_okButton release];
    _okButton = nil;
    [_cancelButton release];
    _cancelButton = nil;

    _panelContents = nil;

    [super dealloc];
}

- (NSButton *)createButtonWithTitle:(QPlatformDialogHelper::StandardButton)type
{
    NSButton *button = [[NSButton alloc] initWithFrame:NSZeroRect];
    button.buttonType = NSMomentaryLightButton;
    button.bezelStyle = NSRoundedBezelStyle;
    const QString &cleanTitle =
         QPlatformTheme::removeMnemonics(QGuiApplicationPrivate::platformTheme()->standardButtonText(type));
    // FIXME: Not obvious, from Cocoa's documentation, that QString::toNSString() makes a deep copy
    button.title = (NSString *)cleanTitle.toCFString();
    ((NSButtonCell *)button.cell).font =
            [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSControlSizeRegular]];
    [self addSubview:button];
    return button;
}

- (void)layout
{
    static const CGFloat ButtonMinWidth = 78.0; // 84.0 for Carbon
    static const CGFloat ButtonMinHeight = 32.0;
    static const CGFloat ButtonSpacing = 0.0;
    static const CGFloat ButtonTopMargin = 0.0;
    static const CGFloat ButtonBottomMargin = 7.0;
    static const CGFloat ButtonSideMargin = 9.0;

    NSSize frameSize = self.frame.size;

    [self.okButton sizeToFit];
    NSSize okSizeHint = self.okButton.frame.size;

    [self.cancelButton sizeToFit];
    NSSize cancelSizeHint = self.cancelButton.frame.size;

    const CGFloat buttonWidth = qMin(qMax(ButtonMinWidth,
                                          qMax(okSizeHint.width, cancelSizeHint.width)),
                                     CGFloat((frameSize.width - 2.0 * ButtonSideMargin - ButtonSpacing) * 0.5));
    const CGFloat buttonHeight = qMax(ButtonMinHeight,
                                     qMax(okSizeHint.height, cancelSizeHint.height));

    NSRect okRect = { { frameSize.width - ButtonSideMargin - buttonWidth,
                        ButtonBottomMargin },
                      { buttonWidth, buttonHeight } };
    self.okButton.frame = okRect;
    self.okButton.needsDisplay = YES;

    NSRect cancelRect = { { okRect.origin.x - ButtonSpacing - buttonWidth,
                            ButtonBottomMargin },
                            { buttonWidth, buttonHeight } };
    self.cancelButton.frame = cancelRect;
    self.cancelButton.needsDisplay = YES;

    // The third view should be the original panel contents. Cache it.
    if (!self.panelContents)
        for (NSView *view in self.subviews)
            if (view != self.okButton && view != self.cancelButton) {
                _panelContents = view;
                break;
            }

    const CGFloat buttonBoxHeight = ButtonBottomMargin + buttonHeight + ButtonTopMargin;
    const NSRect panelContentsFrame = NSMakeRect(
                self.panelContentsMargins.left,
                buttonBoxHeight + self.panelContentsMargins.bottom,
                frameSize.width - (self.panelContentsMargins.left + self.panelContentsMargins.right),
                frameSize.height - buttonBoxHeight - (self.panelContentsMargins.top + self.panelContentsMargins.bottom));
    self.panelContents.frame = panelContentsFrame;
    self.panelContents.needsDisplay = YES;

    self.needsDisplay = YES;
    [super layout];
}

// -------------------------------------------------------------------------

io_object_t q_IOObjectRetain(io_object_t obj)
{
    kern_return_t ret = IOObjectRetain(obj);
    Q_ASSERT(!ret);
    return obj;
}

void q_IOObjectRelease(io_object_t obj)
{
    kern_return_t ret = IOObjectRelease(obj);
    Q_ASSERT(!ret);
}

@end
