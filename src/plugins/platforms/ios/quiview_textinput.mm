/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtGui/qtextformat.h>

class StaticVariables
{
public:
    QInputMethodQueryEvent inputMethodQueryEvent;
    bool inUpdateKeyboardLayout;
    QTextCharFormat markedTextFormat;

    StaticVariables()
        : inputMethodQueryEvent(Qt::ImQueryInput)
        , inUpdateKeyboardLayout(false)
    {
        // There seems to be no way to query how the preedit text
        // should be drawn. So we need to hard-code the color.
        QSysInfo::MacVersion iosVersion = QSysInfo::MacintoshVersion;
        if (iosVersion < QSysInfo::MV_IOS_7_0)
            markedTextFormat.setBackground(QColor(235, 239, 247));
        else
            markedTextFormat.setBackground(QColor(206, 221, 238));
    }
};

Q_GLOBAL_STATIC(StaticVariables, staticVariables);

// -------------------------------------------------------------------------

@interface QUITextPosition : UITextPosition
{
}

@property (nonatomic) NSUInteger index;
+ (QUITextPosition *)positionWithIndex:(NSUInteger)index;

@end

@implementation QUITextPosition

+ (QUITextPosition *)positionWithIndex:(NSUInteger)index
{
    QUITextPosition *pos = [[QUITextPosition alloc] init];
    pos.index = index;
    return [pos autorelease];
}

@end

// -------------------------------------------------------------------------

@interface QUITextRange : UITextRange
{
}

@property (nonatomic) NSRange range;
+ (QUITextRange *)rangeWithNSRange:(NSRange)range;

@end

@implementation QUITextRange

+ (QUITextRange *)rangeWithNSRange:(NSRange)nsrange
{
    QUITextRange *range = [[QUITextRange alloc] init];
    range.range = nsrange;
    return [range autorelease];
}

- (UITextPosition *)start
{
    return [QUITextPosition positionWithIndex:self.range.location];
}

- (UITextPosition *)end
{
    return [QUITextPosition positionWithIndex:(self.range.location + self.range.length)];
}

- (NSRange) range
{
    return _range;
}

-(BOOL)isEmpty
{
    return (self.range.length == 0);
}

@end

// -------------------------------------------------------------------------

@implementation QUIView (TextInput)

- (BOOL)canBecomeFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    // Note: QIOSInputContext controls our first responder status based on
    // whether or not the keyboard should be open or closed.
    [self updateTextInputTraits];
    return [super becomeFirstResponder];
}

- (BOOL)resignFirstResponder
{
    // Resigning first responed status means that the virtual keyboard was closed, or
    // some other view became first responder. In either case we clear the focus object to
    // avoid blinking cursors in line edits etc:
    if (m_qioswindow)
        static_cast<QWindowPrivate *>(QObjectPrivate::get(m_qioswindow->window()))->clearFocusObject();
    return [super resignFirstResponder];
}

+ (bool)inUpdateKeyboardLayout
{
    return staticVariables()->inUpdateKeyboardLayout;
}

- (void)updateKeyboardLayout
{
    if (![self isFirstResponder])
        return;

    // There seems to be no API to inform that the keyboard layout needs to update.
    // As a work-around, we quickly resign first responder just to reassign it again.
    QScopedValueRollback<bool> rollback(staticVariables()->inUpdateKeyboardLayout);
    staticVariables()->inUpdateKeyboardLayout = true;
    [super resignFirstResponder];
    [self updateTextInputTraits];
    [super becomeFirstResponder];
}

- (void)updateUITextInputDelegate:(NSNumber *)intQuery
{
    // As documented, we should not report textWillChange/textDidChange unless the text
    // was changed externally. That will cause spell checking etc to fail. But we don't
    // really know if the text/selection was changed by UITextInput or Qt/app when getting
    // update calls from Qt. We therefore use a less ideal approach where we always assume
    // that UITextView caused the change if we're currently processing an event sendt from it.
    if (m_inSendEventToFocusObject)
        return;

    Qt::InputMethodQueries query = Qt::InputMethodQueries([intQuery intValue]);
    if (query & (Qt::ImCursorPosition | Qt::ImAnchorPosition)) {
        [self.inputDelegate selectionWillChange:id<UITextInput>(self)];
        [self.inputDelegate selectionDidChange:id<UITextInput>(self)];
    }

    if (query & Qt::ImSurroundingText) {
        [self.inputDelegate textWillChange:id<UITextInput>(self)];
        [self.inputDelegate textDidChange:id<UITextInput>(self)];
    }
}

- (void)updateInputMethodWithQuery:(Qt::InputMethodQueries)query
{
    Q_UNUSED(query);

    QObject *focusObject = QGuiApplication::focusObject();
    if (!focusObject)
        return;

    // Note that we ignore \a query, and instead update using Qt::ImQueryInput. This enables us to just
    // store the event without copying out the result from the event each time. Besides, we seem to be
    // called with Qt::ImQueryInput when only changing selection, and always if typing text. So there would
    // not be any performance gain by only updating \a query.
    staticVariables()->inputMethodQueryEvent = QInputMethodQueryEvent(Qt::ImQueryInput);
    QCoreApplication::sendEvent(focusObject, &staticVariables()->inputMethodQueryEvent);
    [self updateUITextInputDelegate:[NSNumber numberWithInt:int(query)]];
}

- (void)sendEventToFocusObject:(QEvent &)e
{
    QObject *focusObject = QGuiApplication::focusObject();
    if (!focusObject)
        return;

    // While sending the event, we will receive back updateInputMethodWithQuery calls.
    // Note that it would be more correct to post the event instead, but UITextInput expects
    // callbacks to take effect immediately (it will query us for information after a callback).
    QScopedValueRollback<BOOL> rollback(m_inSendEventToFocusObject);
    m_inSendEventToFocusObject = YES;
    QCoreApplication::sendEvent(focusObject, &e);
}

- (void)reset
{
    [self setMarkedText:@"" selectedRange:NSMakeRange(0, 0)];
    [self updateInputMethodWithQuery:Qt::ImQueryInput];
    // Guard agains recursive callbacks by posting calls to UITextInput
    [self performSelectorOnMainThread:@selector(updateKeyboardLayout) withObject:nil waitUntilDone:NO];
    [self performSelectorOnMainThread:@selector(updateUITextInputDelegate:)
      withObject:[NSNumber numberWithInt:int(Qt::ImQueryInput)]
      waitUntilDone:NO];
}

- (void)commit
{
    [self unmarkText];
    // Guard agains recursive callbacks by posting calls to UITextInput
    [self performSelectorOnMainThread:@selector(updateUITextInputDelegate:)
      withObject:[NSNumber numberWithInt:int(Qt::ImSurroundingText)]
      waitUntilDone:NO];
}

- (QVariant)imValue:(Qt::InputMethodQuery)query
{
    return staticVariables()->inputMethodQueryEvent.value(query);
}

-(id<UITextInputTokenizer>)tokenizer
{
    return [[[UITextInputStringTokenizer alloc] initWithTextInput:id<UITextInput>(self)] autorelease];
}

-(UITextPosition *)beginningOfDocument
{
    return [QUITextPosition positionWithIndex:0];
}

-(UITextPosition *)endOfDocument
{
    int endPosition = [self imValue:Qt::ImSurroundingText].toString().length();
    return [QUITextPosition positionWithIndex:endPosition];
}

- (void)setSelectedTextRange:(UITextRange *)range
{
    QUITextRange *r = static_cast<QUITextRange *>(range);
    QList<QInputMethodEvent::Attribute> attrs;
    attrs << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, r.range.location, r.range.length, 0);
    QInputMethodEvent e(m_markedText, attrs);
    [self sendEventToFocusObject:e];
}

- (UITextRange *)selectedTextRange {
    int cursorPos = [self imValue:Qt::ImCursorPosition].toInt();
    int anchorPos = [self imValue:Qt::ImAnchorPosition].toInt();
    return [QUITextRange rangeWithNSRange:NSMakeRange(qMin(cursorPos, anchorPos), qAbs(anchorPos - cursorPos))];
}

- (NSString *)textInRange:(UITextRange *)range
{
    int s = static_cast<QUITextPosition *>([range start]).index;
    int e = static_cast<QUITextPosition *>([range end]).index;
    return [self imValue:Qt::ImSurroundingText].toString().mid(s, e - s).toNSString();
}

- (void)setMarkedText:(NSString *)markedText selectedRange:(NSRange)selectedRange
{
    Q_UNUSED(selectedRange);

    m_markedText = markedText ? QString::fromNSString(markedText) : QString();

    QList<QInputMethodEvent::Attribute> attrs;
    attrs << QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 0, markedText.length, staticVariables()->markedTextFormat);
    QInputMethodEvent e(m_markedText, attrs);
    [self sendEventToFocusObject:e];
}

- (void)unmarkText
{
    if (m_markedText.isEmpty())
        return;

    QInputMethodEvent e;
    e.setCommitString(m_markedText);
    [self sendEventToFocusObject:e];

    m_markedText.clear();
}

- (NSComparisonResult)comparePosition:(UITextPosition *)position toPosition:(UITextPosition *)other
{
    int p = static_cast<QUITextPosition *>(position).index;
    int o = static_cast<QUITextPosition *>(other).index;
    if (p > o)
        return NSOrderedAscending;
    else if (p < o)
        return NSOrderedDescending;
    return NSOrderedSame;
}

- (UITextRange *)markedTextRange {
    return m_markedText.isEmpty() ? nil : [QUITextRange rangeWithNSRange:NSMakeRange(0, m_markedText.length())];
}

- (UITextRange *)textRangeFromPosition:(UITextPosition *)fromPosition toPosition:(UITextPosition *)toPosition
{
    int f = static_cast<QUITextPosition *>(fromPosition).index;
    int t = static_cast<QUITextPosition *>(toPosition).index;
    return [QUITextRange rangeWithNSRange:NSMakeRange(f, t - f)];
}

- (UITextPosition *)positionFromPosition:(UITextPosition *)position offset:(NSInteger)offset
{
    int p = static_cast<QUITextPosition *>(position).index;
    return [QUITextPosition positionWithIndex:p + offset];
}

- (UITextPosition *)positionFromPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction offset:(NSInteger)offset
{
    int p = static_cast<QUITextPosition *>(position).index;
    return [QUITextPosition positionWithIndex:(direction == UITextLayoutDirectionRight ? p + offset : p - offset)];
}

- (UITextPosition *)positionWithinRange:(UITextRange *)range farthestInDirection:(UITextLayoutDirection)direction
{
    NSRange r = static_cast<QUITextRange *>(range).range;
    if (direction == UITextLayoutDirectionRight)
        return [QUITextPosition positionWithIndex:r.location + r.length];
    return [QUITextPosition positionWithIndex:r.location];
}

- (NSInteger)offsetFromPosition:(UITextPosition *)fromPosition toPosition:(UITextPosition *)toPosition
{
    int f = static_cast<QUITextPosition *>(fromPosition).index;
    int t = static_cast<QUITextPosition *>(toPosition).index;
    return t - f;
}

- (UIView *)textInputView
{
    // iOS expects rects we return from other UITextInput methods
    // to be relative to the view this method returns.
    // Since QInputMethod returns rects relative to the top level
    // QWindow, that is also the view we need to return.
    QPlatformWindow *topLevel = m_qioswindow;
    while (QPlatformWindow *p = topLevel->parent())
        topLevel = p;
    return reinterpret_cast<UIView *>(topLevel->winId());
}

- (CGRect)firstRectForRange:(UITextRange *)range
{
    QObject *focusObject = QGuiApplication::focusObject();
    if (!focusObject)
        return CGRectZero;

    // Using a work-around to get the current rect until
    // a better API is in place:
    if (!m_markedText.isEmpty())
        return CGRectZero;

    int cursorPos = [self imValue:Qt::ImCursorPosition].toInt();
    int anchorPos = [self imValue:Qt::ImAnchorPosition].toInt();

    NSRange r = static_cast<QUITextRange*>(range).range;
    QList<QInputMethodEvent::Attribute> attrs;
    attrs << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, r.location, 0, 0);
    QInputMethodEvent e(m_markedText, attrs);
    [self sendEventToFocusObject:e];
    QRectF startRect = qApp->inputMethod()->cursorRectangle();

    attrs = QList<QInputMethodEvent::Attribute>();
    attrs << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, r.location + r.length, 0, 0);
    e = QInputMethodEvent(m_markedText, attrs);
    [self sendEventToFocusObject:e];
    QRectF endRect = qApp->inputMethod()->cursorRectangle();

    if (cursorPos != int(r.location + r.length) || cursorPos != anchorPos) {
        attrs = QList<QInputMethodEvent::Attribute>();
        attrs << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, cursorPos, (cursorPos - anchorPos), 0);
        e = QInputMethodEvent(m_markedText, attrs);
        [self sendEventToFocusObject:e];
    }

    return toCGRect(startRect.united(endRect));
}

- (CGRect)caretRectForPosition:(UITextPosition *)position
{
    Q_UNUSED(position);
    // Assume for now that position is always the same as
    // cursor index until a better API is in place:
    QRectF cursorRect = qApp->inputMethod()->cursorRectangle();
    return toCGRect(cursorRect);
}

- (void)replaceRange:(UITextRange *)range withText:(NSString *)text
{
    [self setSelectedTextRange:range];

    QInputMethodEvent e;
    e.setCommitString(QString::fromNSString(text));
    [self sendEventToFocusObject:e];
}

- (void)setBaseWritingDirection:(UITextWritingDirection)writingDirection forRange:(UITextRange *)range
{
    Q_UNUSED(writingDirection);
    Q_UNUSED(range);
    // Writing direction is handled by QLocale
}

- (UITextWritingDirection)baseWritingDirectionForPosition:(UITextPosition *)position inDirection:(UITextStorageDirection)direction
{
    Q_UNUSED(position);
    Q_UNUSED(direction);
    if (QLocale::system().textDirection() == Qt::RightToLeft)
        return UITextWritingDirectionRightToLeft;
    return UITextWritingDirectionLeftToRight;
}

- (UITextRange *)characterRangeByExtendingPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction
{
    int p = static_cast<QUITextPosition *>(position).index;
    if (direction == UITextLayoutDirectionLeft)
        return [QUITextRange rangeWithNSRange:NSMakeRange(0, p)];
    int l = [self imValue:Qt::ImSurroundingText].toString().length();
    return [QUITextRange rangeWithNSRange:NSMakeRange(p, l - p)];
}

- (UITextPosition *)closestPositionToPoint:(CGPoint)point
{
    // No API in Qt for determining this. Use sensible default instead:
    Q_UNUSED(point);
    return [QUITextPosition positionWithIndex:[self imValue:Qt::ImCursorPosition].toInt()];
}

- (UITextPosition *)closestPositionToPoint:(CGPoint)point withinRange:(UITextRange *)range
{
    // No API in Qt for determining this. Use sensible default instead:
    Q_UNUSED(point);
    Q_UNUSED(range);
    return [QUITextPosition positionWithIndex:[self imValue:Qt::ImCursorPosition].toInt()];
}

- (UITextRange *)characterRangeAtPoint:(CGPoint)point
{
    // No API in Qt for determining this. Use sensible default instead:
    Q_UNUSED(point);
    return [QUITextRange rangeWithNSRange:NSMakeRange([self imValue:Qt::ImCursorPosition].toInt(), 0)];
}

- (void)setMarkedTextStyle:(NSDictionary *)style
{
    Q_UNUSED(style);
    // No-one is going to change our style. If UIKit itself did that
    // it would be very welcome, since then we knew how to style marked
    // text instead of just guessing...
}

- (NSDictionary *)textStylingAtPosition:(UITextPosition *)position inDirection:(UITextStorageDirection)direction
{
    Q_UNUSED(position);
    Q_UNUSED(direction);

    QObject *focusObject = QGuiApplication::focusObject();
    if (!focusObject)
        return [NSDictionary dictionary];

    // Assume position is the same as the cursor for now. QInputMethodQueryEvent with Qt::ImFont
    // needs to be extended to take an extra position argument before this can be fully correct.
    QInputMethodQueryEvent e(Qt::ImFont);
    QCoreApplication::sendEvent(focusObject, &e);
    QFont qfont = qvariant_cast<QFont>(e.value(Qt::ImFont));
    UIFont *uifont = [UIFont fontWithName:qfont.family().toNSString() size:qfont.pointSize()];
    if (!uifont)
        return [NSDictionary dictionary];
    return [NSDictionary dictionaryWithObject:uifont forKey:UITextInputTextFontKey];
}

-(NSDictionary *)markedTextStyle
{
    return [NSDictionary dictionary];
}

- (BOOL)hasText
{
    return YES;
}

- (void)insertText:(NSString *)text
{
    QObject *focusObject = QGuiApplication::focusObject();
    if (!focusObject)
        return;

    if ([text isEqualToString:@"\n"]) {
        if (self.returnKeyType == UIReturnKeyDone)
            qApp->inputMethod()->hide();

        QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
        [self sendEventToFocusObject:press];
        [self sendEventToFocusObject:release];

        return;
    }

    QInputMethodEvent e;
    e.setCommitString(QString::fromNSString(text));
    [self sendEventToFocusObject:e];
}

- (void)deleteBackward
{
    // Since we're posting im events directly to the focus object, we should do the
    // same for key events. Otherwise they might end up in a different place or out
    // of sync with im events.
    QKeyEvent press(QEvent::KeyPress, (int)Qt::Key_Backspace, Qt::NoModifier);
    QKeyEvent release(QEvent::KeyRelease, (int)Qt::Key_Backspace, Qt::NoModifier);
    [self sendEventToFocusObject:press];
    [self sendEventToFocusObject:release];
}

- (void)updateTextInputTraits
{
    // Ask the current focus object what kind of input it
    // expects, and configure the keyboard appropriately:
    QObject *focusObject = QGuiApplication::focusObject();
    if (!focusObject)
        return;
    QInputMethodQueryEvent queryEvent(Qt::ImEnabled | Qt::ImHints);
    if (!QCoreApplication::sendEvent(focusObject, &queryEvent))
        return;
    if (!queryEvent.value(Qt::ImEnabled).toBool())
        return;

    Qt::InputMethodHints hints = static_cast<Qt::InputMethodHints>(queryEvent.value(Qt::ImHints).toUInt());

    self.returnKeyType = (hints & Qt::ImhMultiLine) ? UIReturnKeyDefault : UIReturnKeyDone;
    self.secureTextEntry = BOOL(hints & Qt::ImhHiddenText);
    self.autocorrectionType = (hints & Qt::ImhNoPredictiveText) ?
                UITextAutocorrectionTypeNo : UITextAutocorrectionTypeDefault;
    self.spellCheckingType = (hints & Qt::ImhNoPredictiveText) ?
                UITextSpellCheckingTypeNo : UITextSpellCheckingTypeDefault;

    if (hints & Qt::ImhUppercaseOnly)
        self.autocapitalizationType = UITextAutocapitalizationTypeAllCharacters;
    else if (hints & Qt::ImhNoAutoUppercase)
        self.autocapitalizationType = UITextAutocapitalizationTypeNone;
    else
        self.autocapitalizationType = UITextAutocapitalizationTypeSentences;

    if (hints & Qt::ImhUrlCharactersOnly)
        self.keyboardType = UIKeyboardTypeURL;
    else if (hints & Qt::ImhEmailCharactersOnly)
        self.keyboardType = UIKeyboardTypeEmailAddress;
    else if (hints & Qt::ImhDigitsOnly)
        self.keyboardType = UIKeyboardTypeNumberPad;
    else if (hints & Qt::ImhFormattedNumbersOnly)
        self.keyboardType = UIKeyboardTypeDecimalPad;
    else if (hints & Qt::ImhDialableCharactersOnly)
        self.keyboardType = UIKeyboardTypeNumberPad;
    else
        self.keyboardType = UIKeyboardTypeDefault;
}

@end
