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

#include "qiostextresponder.h"

#include "qiosglobal.h"
#include "qiosinputcontext.h"
#include "quiview.h"

#include <QtCore/qscopedvaluerollback.h>

#include <QtGui/qevent.h>
#include <QtGui/qtextformat.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformwindow.h>

// -------------------------------------------------------------------------

@interface QUITextPosition : UITextPosition

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

- (NSRange)range
{
    return _range;
}

- (BOOL)isEmpty
{
    return (self.range.length == 0);
}

@end

// -------------------------------------------------------------------------

@interface WrapperView : UIView
@end

@implementation WrapperView

- (id)initWithView:(UIView *)view
{
    if (self = [self init]) {
        [self addSubview:view];

        self.autoresizingMask = view.autoresizingMask;

        [self sizeToFit];
    }

    return self;
}

- (void)layoutSubviews
{
    UIView* view = [self.subviews firstObject];
    view.frame = self.bounds;

    // FIXME: During orientation changes the size and position
    // of the view is not respected by the host view, even if
    // we call sizeToFit or setNeedsLayout on the superview.
}

- (CGSize)sizeThatFits:(CGSize)size
{
    return [[self.subviews firstObject] sizeThatFits:size];
}

// By keeping the responder (QIOSTextInputResponder in this case)
// retained, we ensure that all messages sent to the view during
// its lifetime in a window hierarcy will be able to traverse the
// responder chain.
- (void)willMoveToWindow:(UIWindow *)window
{
    if (window)
        [[self nextResponder] retain];
    else
        [[self nextResponder] autorelease];
}

@end

// -------------------------------------------------------------------------

@implementation QIOSTextInputResponder

- (id)initWithInputContext:(QT_PREPEND_NAMESPACE(QIOSInputContext) *)inputContext
{
    if (!(self = [self init]))
        return self;

    m_inSendEventToFocusObject = NO;
    m_inSelectionChange = NO;
    m_inputContext = inputContext;

    m_configuredImeState = new QInputMethodQueryEvent(m_inputContext->imeState().currentState);
    QVariantMap platformData = m_configuredImeState->value(Qt::ImPlatformData).toMap();
    Qt::InputMethodHints hints = Qt::InputMethodHints(m_configuredImeState->value(Qt::ImHints).toUInt());

    Qt::EnterKeyType enterKeyType = Qt::EnterKeyType(m_configuredImeState->value(Qt::ImEnterKeyType).toUInt());

    switch (enterKeyType) {
    case Qt::EnterKeyReturn:
        self.returnKeyType = UIReturnKeyDefault;
        break;
    case Qt::EnterKeyDone:
        self.returnKeyType = UIReturnKeyDone;
        break;
    case Qt::EnterKeyGo:
        self.returnKeyType = UIReturnKeyGo;
        break;
    case Qt::EnterKeySend:
        self.returnKeyType = UIReturnKeySend;
        break;
    case Qt::EnterKeySearch:
        self.returnKeyType = UIReturnKeySearch;
        break;
    case Qt::EnterKeyNext:
        self.returnKeyType = UIReturnKeyNext;
        break;
    default:
        self.returnKeyType = (hints & Qt::ImhMultiLine) ? UIReturnKeyDefault : UIReturnKeyDone;
        break;
    }

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
        self.keyboardType = UIKeyboardTypePhonePad;
    else if (hints & Qt::ImhLatinOnly)
        self.keyboardType = UIKeyboardTypeASCIICapable;
    else if (hints & Qt::ImhPreferNumbers)
        self.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
    else
        self.keyboardType = UIKeyboardTypeDefault;

    if (UIView *inputView = static_cast<UIView *>(platformData.value(kImePlatformDataInputView).value<void *>()))
        self.inputView = [[[WrapperView alloc] initWithView:inputView] autorelease];
    if (UIView *accessoryView = static_cast<UIView *>(platformData.value(kImePlatformDataInputAccessoryView).value<void *>()))
        self.inputAccessoryView = [[[WrapperView alloc] initWithView:accessoryView] autorelease];

#ifndef Q_OS_TVOS
    if (__builtin_available(iOS 9, *)) {
        if (platformData.value(kImePlatformDataHideShortcutsBar).toBool()) {
            // According to the docs, leadingBarButtonGroups/trailingBarButtonGroups should be set to nil to hide the shortcuts bar.
            // However, starting with iOS 10, the API has been surrounded with NS_ASSUME_NONNULL, which contradicts this and causes
            // compiler warnings. Still it is the way to go to really hide the space reserved for that.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
            self.inputAssistantItem.leadingBarButtonGroups = nil;
            self.inputAssistantItem.trailingBarButtonGroups = nil;
#pragma clang diagnostic pop
        }
    }
#endif

    self.undoManager.groupsByEvent = NO;
    [self rebuildUndoStack];

    return self;
}

- (void)dealloc
{
    self.inputView = 0;
    self.inputAccessoryView = 0;
    delete m_configuredImeState;

    [super dealloc];
}

- (BOOL)needsKeyboardReconfigure:(Qt::InputMethodQueries)updatedProperties
{
    if ((updatedProperties & Qt::ImEnabled)) {
        Q_ASSERT([self currentImeState:Qt::ImEnabled].toBool());

        // When switching on input-methods we need to consider hints and platform data
        // as well, as the IM state that we were based on may have been invalidated when
        // IM was switched off.

        qImDebug("IM was turned on, we need to check hints and platform data as well");
        updatedProperties |= (Qt::ImHints | Qt::ImPlatformData);
    }

    // Based on what we set up in initWithInputContext above
    updatedProperties &= (Qt::ImHints | Qt::ImEnterKeyType | Qt::ImPlatformData);

    if (!updatedProperties)
        return NO;

    for (uint i = 0; i < (sizeof(Qt::ImQueryAll) * CHAR_BIT); ++i) {
        if (Qt::InputMethodQuery property = Qt::InputMethodQuery(int(updatedProperties & (1 << i)))) {
            if ([self currentImeState:property] != m_configuredImeState->value(property)) {
                qImDebug() << property << "has changed since text responder was configured, need reconfigure";
                return YES;
            }
        }
    }

    return NO;
}

- (BOOL)canBecomeFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    FirstResponderCandidate firstResponderCandidate(self);

    qImDebug() << "self:" << self << "first:" << [UIResponder currentFirstResponder];

    if (![super becomeFirstResponder]) {
        qImDebug() << self << "was not allowed to become first responder";
        return NO;
    }

    qImDebug() << self << "became first responder";

    return YES;
}

- (BOOL)resignFirstResponder
{
    qImDebug() << "self:" << self << "first:" << [UIResponder currentFirstResponder];

    // Don't allow activation events of the window that we're doing text on behalf on
    // to steal responder.
    if (FirstResponderCandidate::currentCandidate() == [self nextResponder]) {
        qImDebug("not allowing parent window to steal responder");
        return NO;
    }

    if (![super resignFirstResponder])
        return NO;

    qImDebug() << self << "resigned first responder";

    // Dismissing the keyboard will trigger resignFirstResponder, but so will
    // a regular responder transfer to another window. In the former case, iOS
    // will set the new first-responder to our next-responder, and in the latter
    // case we'll have an active responder candidate.
    if (![UIResponder currentFirstResponder] && !FirstResponderCandidate::currentCandidate()) {
        // No first responder set anymore, sync this with Qt by clearing the
        // focus object.
        m_inputContext->clearCurrentFocusObject();
    } else if ([UIResponder currentFirstResponder] == [self nextResponder]) {
        // We have resigned the keyboard, and transferred first responder back to the parent view
        Q_ASSERT(!FirstResponderCandidate::currentCandidate());
        if ([self currentImeState:Qt::ImEnabled].toBool()) {
            // The current focus object expects text input, but there
            // is no keyboard to get input from. So we clear focus.
            qImDebug("no keyboard available, clearing focus object");
            m_inputContext->clearCurrentFocusObject();
        }
    } else {
        // We've lost responder status because another Qt window was made active,
        // another QIOSTextResponder was made first-responder, another UIView was
        // made first-responder, or the first-responder was cleared globally. In
        // either of these cases we don't have to do anything.
        qImDebug("lost first responder, but not clearing focus object");
    }

    return YES;
}


- (UIResponder*)nextResponder
{
    return qApp->focusWindow() ?
        reinterpret_cast<QUIView *>(qApp->focusWindow()->handle()->winId()) : 0;
}

// -------------------------------------------------------------------------

- (void)sendKeyPressRelease:(Qt::Key)key modifiers:(Qt::KeyboardModifiers)modifiers
{
    QScopedValueRollback<BOOL> rollback(m_inSendEventToFocusObject, true);
    QWindowSystemInterface::handleKeyEvent(qApp->focusWindow(), QEvent::KeyPress, key, modifiers);
    QWindowSystemInterface::handleKeyEvent(qApp->focusWindow(), QEvent::KeyRelease, key, modifiers);
}

#ifndef QT_NO_SHORTCUT

- (void)sendShortcut:(QKeySequence::StandardKey)standardKey
{
    const int keys = QKeySequence(standardKey)[0];
    Qt::Key key = Qt::Key(keys & 0x0000FFFF);
    Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers(keys & 0xFFFF0000);
    [self sendKeyPressRelease:key modifiers:modifiers];
}

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender
{
    bool isEditAction = (action == @selector(cut:)
        || action == @selector(copy:)
        || action == @selector(paste:)
        || action == @selector(delete:)
        || action == @selector(toggleBoldface:)
        || action == @selector(toggleItalics:)
        || action == @selector(toggleUnderline:)
        || action == @selector(undo)
        || action == @selector(redo));

    bool isSelectAction = (action == @selector(select:)
        || action == @selector(selectAll:)
        || action == @selector(paste:)
        || action == @selector(undo)
        || action == @selector(redo));

    const bool unknownAction = !isEditAction && !isSelectAction;
    const bool hasSelection = ![self selectedTextRange].empty;

    if (unknownAction)
        return [super canPerformAction:action withSender:sender];
    return (hasSelection && isEditAction) || (!hasSelection && isSelectAction);
}

- (void)cut:(id)sender
{
    Q_UNUSED(sender);
    [self sendShortcut:QKeySequence::Cut];
}

- (void)copy:(id)sender
{
    Q_UNUSED(sender);
    [self sendShortcut:QKeySequence::Copy];
}

- (void)paste:(id)sender
{
    Q_UNUSED(sender);
    [self sendShortcut:QKeySequence::Paste];
}

- (void)select:(id)sender
{
    Q_UNUSED(sender);
    [self sendShortcut:QKeySequence::MoveToPreviousWord];
    [self sendShortcut:QKeySequence::SelectNextWord];
}

- (void)selectAll:(id)sender
{
    Q_UNUSED(sender);
    [self sendShortcut:QKeySequence::SelectAll];
}

- (void)delete:(id)sender
{
    Q_UNUSED(sender);
    [self sendShortcut:QKeySequence::Delete];
}

- (void)toggleBoldface:(id)sender
{
    Q_UNUSED(sender);
    [self sendShortcut:QKeySequence::Bold];
}

- (void)toggleItalics:(id)sender
{
    Q_UNUSED(sender);
    [self sendShortcut:QKeySequence::Italic];
}

- (void)toggleUnderline:(id)sender
{
    Q_UNUSED(sender);
    [self sendShortcut:QKeySequence::Underline];
}

// -------------------------------------------------------------------------

- (void)undo
{
    [self sendShortcut:QKeySequence::Undo];
    [self rebuildUndoStack];
}

- (void)redo
{
    [self sendShortcut:QKeySequence::Redo];
    [self rebuildUndoStack];
}

- (void)registerRedo
{
    NSUndoManager *undoMgr = self.undoManager;
    [undoMgr beginUndoGrouping];
    [undoMgr registerUndoWithTarget:self selector:@selector(redo) object:nil];
    [undoMgr endUndoGrouping];
}

- (void)rebuildUndoStack
{
    dispatch_async(dispatch_get_main_queue (), ^{
        // Register dummy undo/redo operations to enable Cmd-Z and Cmd-Shift-Z
        // Ensure we do this outside any undo/redo callback since NSUndoManager
        // will treat registerUndoWithTarget as registering a redo when called
        // from within a undo callback.
        NSUndoManager *undoMgr = self.undoManager;
        [undoMgr removeAllActions];
        [undoMgr beginUndoGrouping];
        [undoMgr registerUndoWithTarget:self selector:@selector(undo) object:nil];
        [undoMgr endUndoGrouping];

        // Schedule an operation that we immediately pop off to be able to schedule a redo
        [undoMgr beginUndoGrouping];
        [undoMgr registerUndoWithTarget:self selector:@selector(registerRedo) object:nil];
        [undoMgr endUndoGrouping];
        [undoMgr undo];

        // Note that, perhaps because of a bug in UIKit, the buttons on the shortcuts bar ends up
        // disabled if a undo/redo callback doesn't lead to a [UITextInputDelegate textDidChange].
        // And we only call that method if Qt made changes to the text. The effect is that the buttons
        // become disabled when there is nothing more to undo (Qt didn't change anything upon receiving
        // an undo request). This seems to be OK behavior, so we let it stay like that unless it shows
        // to cause problems.
    });
}

// -------------------------------------------------------------------------

- (void)keyCommandTriggered:(UIKeyCommand *)keyCommand
{
    Qt::Key key = Qt::Key_unknown;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    if (keyCommand.input == UIKeyInputLeftArrow)
        key = Qt::Key_Left;
    else if (keyCommand.input == UIKeyInputRightArrow)
        key = Qt::Key_Right;
    else if (keyCommand.input == UIKeyInputUpArrow)
        key = Qt::Key_Up;
    else if (keyCommand.input == UIKeyInputDownArrow)
        key = Qt::Key_Down;
    else
        Q_UNREACHABLE();

    if (keyCommand.modifierFlags & UIKeyModifierAlternate)
        modifiers |= Qt::AltModifier;
    if (keyCommand.modifierFlags & UIKeyModifierShift)
        modifiers |= Qt::ShiftModifier;
    if (keyCommand.modifierFlags & UIKeyModifierCommand)
        modifiers |= Qt::ControlModifier;

    [self sendKeyPressRelease:key modifiers:modifiers];
}

- (void)addKeyCommandsToArray:(NSMutableArray *)array key:(NSString *)key
{
    SEL s = @selector(keyCommandTriggered:);
    [array addObject:[UIKeyCommand keyCommandWithInput:key modifierFlags:0 action:s]];
    [array addObject:[UIKeyCommand keyCommandWithInput:key modifierFlags:UIKeyModifierShift action:s]];
    [array addObject:[UIKeyCommand keyCommandWithInput:key modifierFlags:UIKeyModifierAlternate action:s]];
    [array addObject:[UIKeyCommand keyCommandWithInput:key modifierFlags:UIKeyModifierAlternate|UIKeyModifierShift action:s]];
    [array addObject:[UIKeyCommand keyCommandWithInput:key modifierFlags:UIKeyModifierCommand action:s]];
    [array addObject:[UIKeyCommand keyCommandWithInput:key modifierFlags:UIKeyModifierCommand|UIKeyModifierShift action:s]];
}

- (NSArray *)keyCommands
{
    // Since keyCommands is called for every key
    // press/release, we cache the result
    static dispatch_once_t once;
    static NSMutableArray *array;

    dispatch_once(&once, ^{
        // We let Qt move the cursor around when the arrow keys are being used. This
        // is normally implemented through UITextInput, but since IM in Qt have poor
        // support for moving the cursor vertically, and even less support for selecting
        // text across multiple paragraphs, we do this through key events.
        array = [NSMutableArray new];
        [self addKeyCommandsToArray:array key:UIKeyInputUpArrow];
        [self addKeyCommandsToArray:array key:UIKeyInputDownArrow];
        [self addKeyCommandsToArray:array key:UIKeyInputLeftArrow];
        [self addKeyCommandsToArray:array key:UIKeyInputRightArrow];
    });

    return array;
}

#endif // QT_NO_SHORTCUT

// -------------------------------------------------------------------------

- (void)notifyInputDelegate:(Qt::InputMethodQueries)updatedProperties
{
    // As documented, we should not report textWillChange/textDidChange unless the text
    // was changed externally. That will cause spell checking etc to fail. But we don't
    // really know if the text/selection was changed by UITextInput or Qt/app when getting
    // update calls from Qt. We therefore use a less ideal approach where we always assume
    // that UITextView caused the change if we're currently processing an event sendt from it.
    if (m_inSendEventToFocusObject)
        return;

    if (updatedProperties & (Qt::ImCursorPosition | Qt::ImAnchorPosition)) {
        QScopedValueRollback<BOOL> rollback(m_inSelectionChange, true);
        [self.inputDelegate selectionWillChange:self];
        [self.inputDelegate selectionDidChange:self];
    }

    if (updatedProperties & Qt::ImSurroundingText) {
        [self.inputDelegate textWillChange:self];
        [self.inputDelegate textDidChange:self];
    }
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

- (QVariant)currentImeState:(Qt::InputMethodQuery)query
{
    return m_inputContext->imeState().currentState.value(query);
}

- (id<UITextInputTokenizer>)tokenizer
{
    return [[[UITextInputStringTokenizer alloc] initWithTextInput:self] autorelease];
}

- (UITextPosition *)beginningOfDocument
{
    return [QUITextPosition positionWithIndex:0];
}

- (UITextPosition *)endOfDocument
{
    QString surroundingText = [self currentImeState:Qt::ImSurroundingText].toString();
    int endPosition = surroundingText.length() + m_markedText.length();
    return [QUITextPosition positionWithIndex:endPosition];
}

- (void)setSelectedTextRange:(UITextRange *)range
{
    if (m_inSelectionChange) {
        // After [UITextInputDelegate selectionWillChange], UIKit will cancel
        // any ongoing auto correction (if enabled) and ask us to set an empty selection.
        // This is contradictory to our current attempt to set a selection, so we ignore
        // the callback. UIKit will be re-notified of the new selection after
        // [UITextInputDelegate selectionDidChange].
        return;
    }

    QUITextRange *r = static_cast<QUITextRange *>(range);
    QList<QInputMethodEvent::Attribute> attrs;
    attrs << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, r.range.location, r.range.length, 0);
    QInputMethodEvent e(m_markedText, attrs);
    [self sendEventToFocusObject:e];
}

- (UITextRange *)selectedTextRange
{
    int cursorPos = [self currentImeState:Qt::ImCursorPosition].toInt();
    int anchorPos = [self currentImeState:Qt::ImAnchorPosition].toInt();
    return [QUITextRange rangeWithNSRange:NSMakeRange(qMin(cursorPos, anchorPos), qAbs(anchorPos - cursorPos))];
}

- (NSString *)textInRange:(UITextRange *)range
{
    QString text = [self currentImeState:Qt::ImSurroundingText].toString();
    if (!m_markedText.isEmpty()) {
        // [UITextInput textInRange] is sparsely documented, but it turns out that unconfirmed
        // marked text should be seen as a part of the text document. This is different from
        // ImSurroundingText, which excludes it.
        int cursorPos = [self currentImeState:Qt::ImCursorPosition].toInt();
        text = text.left(cursorPos) + m_markedText + text.mid(cursorPos);
    }

    int s = static_cast<QUITextPosition *>([range start]).index;
    int e = static_cast<QUITextPosition *>([range end]).index;
    return text.mid(s, e - s).toNSString();
}

- (void)setMarkedText:(NSString *)markedText selectedRange:(NSRange)selectedRange
{
    Q_UNUSED(selectedRange);

    m_markedText = markedText ? QString::fromNSString(markedText) : QString();

    static QTextCharFormat markedTextFormat;
    if (markedTextFormat.isEmpty()) {
        // There seems to be no way to query how the preedit text
        // should be drawn. So we need to hard-code the color.
        markedTextFormat.setBackground(QColor(206, 221, 238));
    }

    QList<QInputMethodEvent::Attribute> attrs;
    attrs << QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 0, markedText.length, markedTextFormat);
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

- (UITextRange *)markedTextRange
{
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

    switch (direction) {
    case UITextLayoutDirectionLeft:
        return [QUITextPosition positionWithIndex:p - offset];
    case UITextLayoutDirectionRight:
        return [QUITextPosition positionWithIndex:p + offset];
    default:
        // Qt doesn't support getting the position above or below the current position, so
        // for those cases we just return the current position, making it a no-op.
        return position;
    }
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
    Q_ASSERT(qApp->focusWindow()->handle());
    QPlatformWindow *topLevel = qApp->focusWindow()->handle();
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

    int cursorPos = [self currentImeState:Qt::ImCursorPosition].toInt();
    int anchorPos = [self currentImeState:Qt::ImAnchorPosition].toInt();

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
        attrs << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, qMin(cursorPos, anchorPos), qAbs(cursorPos - anchorPos), 0);
        e = QInputMethodEvent(m_markedText, attrs);
        [self sendEventToFocusObject:e];
    }

    return startRect.united(endRect).toCGRect();
}

- (NSArray *)selectionRectsForRange:(UITextRange *)range
{
    Q_UNUSED(range);
    // This method is supposed to return a rectangle for each line with selection. Since we don't
    // expose an API in Qt/IM for getting this information, and since we never seems to be getting
    // a call from UIKit for this, we return an empty array until a need arise.
    return [[NSArray new] autorelease];
}

- (CGRect)caretRectForPosition:(UITextPosition *)position
{
    Q_UNUSED(position);
    // Assume for now that position is always the same as
    // cursor index until a better API is in place:
    QRectF cursorRect = qApp->inputMethod()->cursorRectangle();
    return cursorRect.toCGRect();
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
    int l = [self currentImeState:Qt::ImSurroundingText].toString().length();
    return [QUITextRange rangeWithNSRange:NSMakeRange(p, l - p)];
}

- (UITextPosition *)closestPositionToPoint:(CGPoint)point
{
    QPointF p = QPointF::fromCGPoint(point);
    const QTransform mapToLocal = QGuiApplication::inputMethod()->inputItemTransform().inverted();
    int textPos = QInputMethod::queryFocusObject(Qt::ImCursorPosition, p * mapToLocal).toInt();
    return [QUITextPosition positionWithIndex:textPos];
}

- (UITextPosition *)closestPositionToPoint:(CGPoint)point withinRange:(UITextRange *)range
{
    // No API in Qt for determining this. Use sensible default instead:
    Q_UNUSED(point);
    Q_UNUSED(range);
    return [QUITextPosition positionWithIndex:[self currentImeState:Qt::ImCursorPosition].toInt()];
}

- (UITextRange *)characterRangeAtPoint:(CGPoint)point
{
    // No API in Qt for determining this. Use sensible default instead:
    Q_UNUSED(point);
    return [QUITextRange rangeWithNSRange:NSMakeRange([self currentImeState:Qt::ImCursorPosition].toInt(), 0)];
}

- (void)setMarkedTextStyle:(NSDictionary *)style
{
    Q_UNUSED(style);
    // No-one is going to change our style. If UIKit itself did that
    // it would be very welcome, since then we knew how to style marked
    // text instead of just guessing...
}

#ifndef Q_OS_TVOS
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
    return [NSDictionary dictionaryWithObject:uifont forKey:NSFontAttributeName];
}
#endif

- (NSDictionary *)markedTextStyle
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
        [self sendKeyPressRelease:Qt::Key_Return modifiers:Qt::NoModifier];

        // An onEnter handler of a TextInput might move to the next input by calling
        // nextInput.forceActiveFocus() which changes the focusObject.
        // In that case we don't want to hide the VKB.
        if (focusObject != QGuiApplication::focusObject()) {
            qImDebug() << "focusObject already changed, not resigning first responder.";
            return;
        }

        if (self.returnKeyType == UIReturnKeyDone || self.returnKeyType == UIReturnKeyGo
            || self.returnKeyType == UIReturnKeySend || self.returnKeyType == UIReturnKeySearch)
            [self resignFirstResponder];

        return;
    }

    QInputMethodEvent e;
    e.setCommitString(QString::fromNSString(text));
    [self sendEventToFocusObject:e];
}

- (void)deleteBackward
{
    // UITextInput selects the text to be deleted before calling this method. To avoid
    // drawing the selection, we flush after posting the key press/release.
    [self sendKeyPressRelease:Qt::Key_Backspace modifiers:Qt::NoModifier];
}

@end
