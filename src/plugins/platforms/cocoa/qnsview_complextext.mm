// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// This file is included from qnsview.mm, and only used to organize the code

@implementation QNSView (ComplexText)

// ------------- Text insertion -------------

- (QObject*)focusObject
{
    // The text input system may still hold a reference to our QNSView,
    // even after QCocoaWindow has been destructed, delivering text input
    // events to us, so we need to guard for this situation explicitly.
    if (!m_platformWindow)
        return nullptr;

    return m_platformWindow->window()->focusObject();
}

/*
    Inserts the given text, potentially replacing existing text.

    The text input management system calls this as a result of:

     - A normal key press, via [NSView interpretKeyEvents:] or
       [NSInputContext handleEvent:]

     - An input method finishing (confirming) composition

     - Pressing a key in the Keyboard Viewer panel

     - Confirming an inline input area (accent popup e.g.)

    \a replacementRange refers to the existing text to replace.
    Under normal circumstances this is {NSNotFound, 0}, and the
    implementation should replace either the existing marked text,
    the current selection, or just insert the text at the current
    cursor location.
*/
- (void)insertText:(id)text replacementRange:(NSRange)replacementRange
{
    qCDebug(lcQpaKeys).nospace() << "Inserting \"" << text << "\""
        << ", replacing range " << replacementRange;

    NSString *string = [self stringForText:text];

    if (m_composingText.isEmpty()) {
        // The input method may have transformed the incoming key event
        // to text that doesn't match what the original key event would
        // have produced, for example when 'Pinyin - Simplified' does smart
        // replacement of quotes. If that's the case we can't rely on
        // handleKeyEvent for sending the text.
        auto *currentEvent = NSApp.currentEvent;
        NSString *eventText = currentEvent.type == NSEventTypeKeyDown
                           || currentEvent.type == NSEventTypeKeyUp
                                ? currentEvent.characters : nil;

        if ([string isEqualToString:eventText]) {
            // We do not send input method events for simple text input,
            // and instead let handleKeyEvent send the key event.
            qCDebug(lcQpaKeys) << "Ignoring text insertion for simple text";
            m_sendKeyEvent = true;
            return;
        }
    }

    if (queryInputMethod(self.focusObject)) {
        QInputMethodEvent inputMethodEvent;

        QString commitString = QString::fromNSString(string);

        // Ensure we have a valid replacement range
        replacementRange = [self sanitizeReplacementRange:replacementRange];

        // Qt's QInputMethodEvent has different semantics for the replacement
        // range than AppKit does, so we need to sanitize the range first.
        auto [replaceFrom, replaceLength] = [self inputMethodRangeForRange:replacementRange];

        if (replaceFrom == NSNotFound) {
            qCWarning(lcQpaKeys) << "Failed to compute valid replacement range for text insertion";
            inputMethodEvent.setCommitString(commitString);
        } else {
            qCDebug(lcQpaKeys) << "Replacing from" << replaceFrom << "with length" << replaceLength
                << "based on replacement range" << replacementRange;
            inputMethodEvent.setCommitString(commitString, replaceFrom, replaceLength);
        }

        QCoreApplication::sendEvent(self.focusObject, &inputMethodEvent);
    }

    m_composingText.clear();
    m_composingFocusObject = nullptr;
}

- (void)insertNewline:(id)sender
{
    Q_UNUSED(sender);

    if (!m_platformWindow)
        return;

    // Depending on the input method, pressing enter may
    // result in simply dismissing the input method editor,
    // without confirming the composition. In other cases
    // it may confirm the composition as well. And in some
    // cases the IME will produce an explicit new line, which
    // brings us here.

    // Semantically, the input method has asked us to insert
    // a newline, and we should do so via an QInputMethodEvent,
    // either directly or via [self insertText:@"\r"]. This is
    // also how NSTextView handles the command. But, if we did,
    // we would bypass all the code in Qt (and clients) that
    // assume that pressing the return key results in a key
    // event, for example the QLineEdit::returnPressed logic.
    // To ensure that clients will still see the Qt::Key_Return
    // key event, we send it as a normal key event.

    // But, we can not fall back to handleKeyEvent for this,
    // as the original key event may have text that reflects
    // the combination of the inserted text and the newline,
    // e.g. "~\r". We have already inserted the composition,
    // so we need to follow up with a single newline event.

    KeyEvent newlineEvent(m_currentlyInterpretedKeyEvent ?
        m_currentlyInterpretedKeyEvent : NSApp.currentEvent);
    newlineEvent.type = QEvent::KeyPress;

    const bool isEnter = newlineEvent.modifiers & Qt::KeypadModifier;
    newlineEvent.key = isEnter ? Qt::Key_Enter : Qt::Key_Return;
    newlineEvent.text = isEnter ? QLatin1Char(kEnterCharCode)
                                : QLatin1Char(kReturnCharCode);
    newlineEvent.nativeVirtualKey = isEnter ? quint32(kVK_ANSI_KeypadEnter)
                                            : quint32(kVK_Return);

    qCDebug(lcQpaKeys) << "Inserting newline via" << newlineEvent;
    newlineEvent.sendWindowSystemEvent(m_platformWindow->window());
}

// ------------- Text composition -------------

/*
    Updates the composed text, potentially replacing existing text.

    The NSTextInputClient protocol refers to composed text as "marked",
    since it is "marked differently from the selection, using temporary
    attributes that affect only display, not layout or storage.""

    The concept maps to the preeditString of our QInputMethodEvent.

    \a selectedRange refers to the part of the marked text that
    is considered selected, for example when composing text with
    multiple clause segments (Hiragana - Kana e.g.).

    \a replacementRange refers to the existing text to replace.
    Under normal circumstances this is {NSNotFound, 0}, and the
    implementation should replace either the existing marked text,
    the current selection, or just insert the text at the current
    cursor location. But when initiating composition of existing
    committed text (Hiragana - Kana e.g.), the range will be valid.
*/
- (void)setMarkedText:(id)text selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
    qCDebug(lcQpaKeys).nospace() << "Marking \"" << text << "\""
        << " with selected range " << selectedRange
        << ", replacing range " << replacementRange;

    const bool isAttributedString = [text isKindOfClass:NSAttributedString.class];
    QString preeditString = QString::fromNSString([self stringForText:text]);

    QList<QInputMethodEvent::Attribute> preeditAttributes;

    // The QInputMethodEvent::Cursor specifies that the length
    // determines whether the cursor is visible or not, but uses
    // logic opposite of that of native AppKit application, where
    // the cursor is visible if there's no selection, and hidden
    // if there's a selection. Instead of passing on the length
    // directly we need to inverse the logic.
    const bool showCursor = !selectedRange.length;
    preeditAttributes << QInputMethodEvent::Attribute(
        QInputMethodEvent::Cursor, selectedRange.location, showCursor);

    // QInputMethodEvent::Selection unfortunately doesn't apply to the
    // preedit text, and QInputMethodEvent::Cursor which does, doesn't
    // support setting a selection. Until we've introduced attributes
    // that allow us to propagate the preedit selection semantically
    // we resort to styling the selection via the TextFormat attribute,
    // so that the preedit selection is visible to the user.
    QTextCharFormat selectionFormat;
    auto *platformTheme = QGuiApplicationPrivate::platformTheme();
    auto *systemPalette = platformTheme->palette();
    selectionFormat.setBackground(systemPalette->color(QPalette::Highlight));
    preeditAttributes << QInputMethodEvent::Attribute(
        QInputMethodEvent::TextFormat,
        selectedRange.location, selectedRange.length,
        selectionFormat);

    int index = 0;
    int composingLength = preeditString.length();
    while (index < composingLength) {
        NSRange range = NSMakeRange(index, composingLength - index);

        static NSDictionary *defaultMarkedTextAttributes = []{
            NSTextView *textView = [[NSTextView new] autorelease];
            return [textView.markedTextAttributes retain];
        }();

        NSDictionary *attributes = isAttributedString
            ? [text attributesAtIndex:index longestEffectiveRange:&range inRange:range]
            : defaultMarkedTextAttributes;

        qCDebug(lcQpaKeys) << "Decorating range" << range << "based on" << attributes;
        QTextCharFormat format;

        if (NSNumber *underlineStyle = attributes[NSUnderlineStyleAttributeName]) {
            format.setFontUnderline(true);
            NSUnderlineStyle style = underlineStyle.integerValue;
            if (style & NSUnderlineStylePatternDot)
                format.setUnderlineStyle(QTextCharFormat::DotLine);
            else if (style & NSUnderlineStylePatternDash)
                format.setUnderlineStyle(QTextCharFormat::DashUnderline);
            else if (style & NSUnderlineStylePatternDashDot)
                format.setUnderlineStyle(QTextCharFormat::DashDotLine);
            if (style & NSUnderlineStylePatternDashDotDot)
                format.setUnderlineStyle(QTextCharFormat::DashDotDotLine);
            else
                format.setUnderlineStyle(QTextCharFormat::SingleUnderline);

            // Unfortunately QTextCharFormat::UnderlineStyle does not distinguish
            // between NSUnderlineStyle{Single,Thick,Double}, which is used by CJK
            // input methods to highlight the selected clause segments.
        }
        if (NSColor *underlineColor = attributes[NSUnderlineColorAttributeName])
            format.setUnderlineColor(qt_mac_toQColor(underlineColor));
        if (NSColor *foregroundColor = attributes[NSForegroundColorAttributeName])
            format.setForeground(qt_mac_toQColor(foregroundColor));
        if (NSColor *backgroundColor = attributes[NSBackgroundColorAttributeName])
            format.setBackground(qt_mac_toQColor(backgroundColor));

        if (format != QTextCharFormat()) {
            preeditAttributes << QInputMethodEvent::Attribute(
                QInputMethodEvent::TextFormat, range.location, range.length, format);
        }

        index = range.location + range.length;
    }

    // Ensure we have a valid replacement range
    replacementRange = [self sanitizeReplacementRange:replacementRange];

    // Qt's QInputMethodEvent has different semantics for the replacement
    // range than AppKit does, so we need to sanitize the range first.
    auto [replaceFrom, replaceLength] = [self inputMethodRangeForRange:replacementRange];

    // Update the composition, now that we've computed the replacement range
    m_composingText = preeditString;

    if (QObject *focusObject = self.focusObject) {
        m_composingFocusObject = focusObject;
        if (queryInputMethod(focusObject)) {
            QInputMethodEvent event(preeditString, preeditAttributes);
            if (replaceLength > 0) {
                // The input method may extend the preedit into already
                // committed text. If so, we need to replace existing text
                // by committing an empty string.
                qCDebug(lcQpaKeys) << "Replacing from" << replaceFrom << "with length"
                    << replaceLength << "based on replacement range" << replacementRange;
                event.setCommitString(QString(), replaceFrom, replaceLength);
            }
            QCoreApplication::sendEvent(focusObject, &event);
        }
    }
}

- (NSArray<NSString *> *)validAttributesForMarkedText
{
    return @[
        NSUnderlineColorAttributeName,
        NSUnderlineStyleAttributeName,
        NSForegroundColorAttributeName,
        NSBackgroundColorAttributeName
    ];
}

- (BOOL)hasMarkedText
{
    return !m_composingText.isEmpty();
}

/*
    Returns the range of marked text or {cursorPosition, 0} if there's none.

    This maps to the location and length of the current preedit (composited) string.

    The returned range measures from the start of the receiver’s text storage,
    that is, from 0 to the document length.
*/
- (NSRange)markedRange
{
    if (auto queryResult = queryInputMethod(self.focusObject, Qt::ImAbsolutePosition)) {
        int absoluteCursorPosition = queryResult.value(Qt::ImAbsolutePosition).toInt();

        // The cursor position as reflected by Qt::ImAbsolutePosition is not
        // affected by the offset of the cursor in the preedit area. That means
        // that when composing text, the cursor position stays the same, at the
        // preedit insertion point, regardless of where the cursor is positioned within
        // the preedit string by the QInputMethodEvent::Cursor attribute. This means
        // we can use the cursor position to determine the range of the marked text.

        // The NSTextInputClient documentation says {NSNotFound, 0} should be returned if there
        // is no marked text, but in practice NSTextView seems to report {cursorPosition, 0},
        // so we do the same.
        return NSMakeRange(absoluteCursorPosition, m_composingText.length());
    } else {
        return {NSNotFound, 0};
    }
}

/*
    Confirms the marked (composed) text.

    The marked text is accepted as if it had been inserted normally,
    and the preedit string is cleared.

    If there is no marked text this method has no effect.
*/
- (void)unmarkText
{
    // FIXME: Match cancelComposingText in early exit and focus object handling

    qCDebug(lcQpaKeys) << "Unmarking" << m_composingText
        << "for focus object" << m_composingFocusObject;

    if (!m_composingText.isEmpty()) {
        QObject *focusObject = self.focusObject;
        if (queryInputMethod(focusObject)) {
            QInputMethodEvent e;
            e.setCommitString(m_composingText);
            QCoreApplication::sendEvent(focusObject, &e);
        }
    }

    m_composingText.clear();
    m_composingFocusObject = nullptr;
}

/*
    Cancels composition.

    The marked text is discarded, and the preedit string is cleared.

    If there is no marked text this method has no effect.
*/
- (void)cancelComposingText
{
    if (m_composingText.isEmpty())
        return;

    qCDebug(lcQpaKeys) << "Canceling composition" << m_composingText
        << "for focus object" << m_composingFocusObject;

    if (queryInputMethod(m_composingFocusObject)) {
        QInputMethodEvent e;
        QCoreApplication::sendEvent(m_composingFocusObject, &e);
    }

    m_composingText.clear();
    m_composingFocusObject = nullptr;
}

// ------------- Key binding command handling -------------

- (void)doCommandBySelector:(SEL)selector
{
    // Note: if the selector cannot be invoked, then doCommandBySelector:
    // should not pass this message up the responder chain (nor should it
    // call super, as the NSResponder base class would in that case pass
    // the message up the responder chain, which we don't want). We will
    // pass the originating key event up the responder chain if applicable.

    qCDebug(lcQpaKeys) << "Trying to perform command" << selector;
    if (![self tryToPerform:selector with:self]) {
        m_sendKeyEvent = true;

        if (![NSStringFromSelector(selector) hasPrefix:@"insert"]) {
            // The text input system determined that the key event was not
            // meant for text insertion, and instead asked us to treat it
            // as a (possibly noop) command. This typically happens for key
            // events with either ⌘ or ⌃, function keys such as F1-F35,
            // arrow keys, etc. We reflect that when sending the key event
            // later on, by removing the text from the event, so that the
            // event does not result in text insertion on the client side.
            m_sendKeyEventWithoutText = true;
        }
    }
}

// ------------- Various text properties -------------

/*
    Returns the range of selected text, or {cursorPosition, 0} if there's none.

    The returned range measures from the start of the receiver’s text storage,
    that is, from 0 to the document length.
*/
- (NSRange)selectedRange
{
    if (auto queryResult = queryInputMethod(self.focusObject,
            Qt::ImCursorPosition | Qt::ImAbsolutePosition | Qt::ImAnchorPosition)) {

        // Unfortunately the Qt::InputMethodQuery values are all relative
        // to the start of the current editing block (paragraph), but we
        // need them in absolute values relative to the entire text.
        // Luckily we have one property, Qt::ImAbsolutePosition, that
        // we can use to compute the offset.
        int cursorPosition = queryResult.value(Qt::ImCursorPosition).toInt();
        int absoluteCursorPosition = queryResult.value(Qt::ImAbsolutePosition).toInt();
        int absoluteOffset = absoluteCursorPosition - cursorPosition;

        int anchorPosition = absoluteOffset + queryResult.value(Qt::ImAnchorPosition).toInt();
        int selectionStart = anchorPosition >= absoluteCursorPosition ? absoluteCursorPosition : anchorPosition;
        int selectionEnd = selectionStart == anchorPosition ? absoluteCursorPosition : anchorPosition;
        int selectionLength = selectionEnd - selectionStart;

        // Note: The cursor position as reflected by these properties are not
        // affected by the offset of the cursor in the preedit area. That means
        // that when composing text, the cursor position stays the same, at the
        // preedit insertion point, regardless of where the cursor is positioned within
        // the preedit string by the QInputMethodEvent::Cursor attribute.

        // The NSTextInputClient documentation says {NSNotFound, 0} should be returned if there is no
        // selection, but in practice NSTextView seems to report {cursorPosition, 0}, so we do the same.
        return NSMakeRange(selectionStart, selectionLength);
    } else {
        return {NSNotFound, 0};
    }
}

/*
    Returns an attributed string derived from the given range
    in the underlying focus object's text storage.

    Input methods may call this with a proposed range that is
    out of bounds. For example, the InkWell text input service
    may ask for the contents of the text input client that extends
    beyond the document's range. To remedy this we always compute
    the intersection between the proposed range and the available
    text.

    If the intersection is completely outside of the available text
    this method returns nil.
*/
- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
    if (auto queryResult = queryInputMethod(self.focusObject,
            Qt::ImAbsolutePosition | Qt::ImTextBeforeCursor | Qt::ImTextAfterCursor)) {
        const int absoluteCursorPosition = queryResult.value(Qt::ImAbsolutePosition).toInt();
        const QString textBeforeCursor = queryResult.value(Qt::ImTextBeforeCursor).toString();
        const QString textAfterCursor = queryResult.value(Qt::ImTextAfterCursor).toString();

        // The documentation doesn't say whether the marked text should be included
        // in the available text, but observing NSTextView shows that this is the
        // case, so we follow suit.
        const QString availableText = textBeforeCursor + m_composingText + textAfterCursor;
        const NSRange availableRange = NSMakeRange(absoluteCursorPosition - textBeforeCursor.length(),
                                  availableText.length());

        const NSRange intersectedRange = NSIntersectionRange(range, availableRange);
        if (actualRange)
            *actualRange = intersectedRange;

        if (!intersectedRange.length)
            return nil;

        NSString *substring = QStringView(availableText).mid(
            intersectedRange.location - availableRange.location,
            intersectedRange.length).toNSString();

        return [[[NSAttributedString alloc] initWithString:substring] autorelease];

    } else {
        return nil;
    }
}

/*
    Returns the first logical boundary rectangle for characters in the given range,
    in screen coordinates.

    The "first" in the name refers to the rectangle enclosing the first line when
    the range encompasses multiple lines of text. In that case, actualRange should
    be set to the range covered by the first rect, so all line fragments can
    be queried by invoking this method repeatedly.

    If the length of range is 0 (as it would be if there is nothing selected at
    the insertion point), then the rectangle coincides with the insertion point.
*/
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
    Q_UNUSED(range);
    Q_UNUSED(actualRange);

    QWindow *window = m_platformWindow ? m_platformWindow->window() : nullptr;
    if (window && queryInputMethod(window->focusObject())) {
        if (range.length) // FIXME: Handle the case when range is non-zero
            qCWarning(lcQpaKeys) << "Can't satisfy firstRectForCharacterRange for" << range;
        QRect cursorRect = qApp->inputMethod()->cursorRectangle().toRect();
        cursorRect.moveBottomLeft(window->mapToGlobal(cursorRect.bottomLeft()));
        return QCocoaScreen::mapToNative(cursorRect);
    } else {
        return NSZeroRect;
    }
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
    // We don't support cursor movements using mouse while composing.
    Q_UNUSED(point);
    return NSNotFound;
}

/*
    Returns the window level of the text input.

    This allows the input method to place its input panel
    above the text input.
*/
- (NSInteger)windowLevel
{
    // The default level assumed by input methods is NSFloatingWindowLevel,
    // but our NSWindow level could be higher than that for many reasons,
    // including being set via QWindow::setFlags() or directly on the
    // NSWindow, or because we're embedded into a native view hierarchy.
    // Return the actual window level to account for this.
    auto level = m_platformWindow ? m_platformWindow->nativeWindow().level
                                  : NSNormalWindowLevel;

    // The logic above only covers our own window though. In some cases,
    // such as when a completer is active, the text input has a lower
    // window level than another window that's also visible, and we don't
    // want the input panel to be sandwiched between these two windows.
    // Account for this by explicitly using NSPopUpMenuWindowLevel as
    // the minimum window level, which corresponds to the highest level
    // one can get via QWindow::setFlags(), except for Qt::ToolTip.
    return qMax(level, NSPopUpMenuWindowLevel);
}

// ------------- Helper functions -------------

/*
    Sanitizes the replacement range, ensuring it's valid.

    If \a range is not valid the range of the current
    marked text will be used.

    If there's no marked text the range of the current
    selection will be used.

    If there's no selection the range will be {cursorPosition, 0}.
*/
- (NSRange)sanitizeReplacementRange:(NSRange)range
{
    if (range.location != NSNotFound)
        return range; // Use as is

    // If the replacement range is not specified we are expected to compute
    // the range ourselves, based on the current state of the input context.

    const auto markedRange = [self markedRange];
    const auto selectedRange = [self selectedRange];

    if (markedRange.length)
        return markedRange;
    else if (selectedRange.length)
        return selectedRange;
    else
        return markedRange; // Represents cursor position when length is 0

}

/*
    Computes the QInputMethodEvent commit string range,
    based on the NSTextInputClient replacement range.

    The two APIs have different semantics.
*/
- (std::pair<long long, long long>)inputMethodRangeForRange:(NSRange)replacementRange
{
    long long replaceFrom = replacementRange.location;
    long long replaceLength = replacementRange.length;

    const auto markedRange = [self markedRange];
    const auto selectedRange = [self selectedRange];

    if (markedRange.length && selectedRange.length) {
        // We assume below that we have either marked text or selected text
        qCWarning(lcQpaKeys) << "Got both markedRange" << markedRange
                             << "and selectedRange" << selectedRange;
    }

    if (markedRange.length) {
        // The replacement length of QInputMethodEvent already includes
        // the preedit string, as the documentation says that "When doing
        // replacement, the area of the preedit string is ignored".
        replaceLength -= markedRange.length;

        // The QInputMethodEvent replacement start is relative to the start
        // of the marked text (the location of the preedit string).
        replaceFrom -= markedRange.location;
    } else if (selectedRange.length) {
        if (!NSEqualRanges(NSIntersectionRange(replacementRange, selectedRange), selectedRange)) {
            qCWarning(lcQpaKeys) << "Replacement range" << replacementRange
                                 << "is a subset of selection" << selectedRange;
            // FIXME: To support this case we would need to extract parts of the
            // selection into the committed text. But for now we ignore it, as we
            // don't know if it happens in practice.
        }

        // Our input method protocol specifies that the entire selection
        // should be removed as the first step, and the replacement length
        // of the QInputMethodEvent refers to any additional text that should
        // be removed/replaced.
        replaceLength -= selectedRange.length;

        // Once the selection has been removed the cursor position will be
        // at the leftmost point of the selection, regardless of whether the
        // cursor was at the start or end of the selection. The replacement
        // start of QInputMethodEvent should be relative to this position.
        replaceFrom -= selectedRange.location;
    } else if (markedRange.location != NSNotFound) {
        // The QInputMethodEvent replacement start is relative to the cursor
        // position.
        replaceFrom -= markedRange.location;
    } else{
        replaceFrom = 0;
    }

    // What we're left with is any _additional_ replacement.
    // Make sure it's valid before passing it on.
    replaceLength = qMax(0ll, replaceLength);

    return {replaceFrom, replaceLength};
}

- (NSString*)stringForText:(id)text
{
    return [text isKindOfClass:NSAttributedString.class] ? [text string] : text;
}

@end

@implementation QNSView (ServicesMenu)

// Support for reading and writing from service menu pasteboards, which is also
// how the writing tools interact with custom NSView. Note that we only support
// plain text, which means that a rich text selection will lose all its styling
// when fed through a service that changes the text. To support rich text we
// need IM plumbing that operates on QMimeData.

- (id)validRequestorForSendType:(NSPasteboardType)sendType returnType:(NSPasteboardType)returnType
{
    bool canWriteToPasteboard = [&]{
        if (![sendType isEqualToString:NSPasteboardTypeString])
            return false;
        if (auto queryResult = queryInputMethod(self.focusObject, Qt::ImCurrentSelection)) {
            auto selectedText = queryResult.value(Qt::ImCurrentSelection).toString();
            if (!selectedText.isEmpty())
                return true;
        }
        return false;
    }();

    bool canReadFromPastboard = [returnType isEqualToString:NSPasteboardTypeString];

    if ((sendType && !canWriteToPasteboard) || (returnType && !canReadFromPastboard)) {
        return [super validRequestorForSendType:sendType returnType:returnType];
    } else {
        qCDebug(lcQpaServices) << "Accepting service interaction for send" << sendType << "and receive" << returnType;
        return self;
    }
}

- (BOOL)writeSelectionToPasteboard:(NSPasteboard *)pasteboard types:(NSArray<NSPasteboardType> *)types
{
    if ([types containsObject:NSPasteboardTypeString]
        // Check for the deprecated NSStringPboardType as well, as even if we
        // claim to only support NSPasteboardTypeString, we get callbacks for
        // the deprecated type.
        || QT_IGNORE_DEPRECATIONS([types containsObject:NSStringPboardType])) {
        if (auto queryResult = queryInputMethod(self.focusObject, Qt::ImCurrentSelection)) {
            auto selectedText = queryResult.value(Qt::ImCurrentSelection).toString();
            qCDebug(lcQpaServices) << "Writing" << selectedText << "to service pasteboard" << pasteboard.name;
            return [pasteboard writeObjects:@[ selectedText.toNSString() ]];
        }
    }
    return NO;
}

- (BOOL)readSelectionFromPasteboard:(NSPasteboard *)pasteboard
{
    NSString *insertedString = [pasteboard stringForType:NSPasteboardTypeString];
    if (!insertedString)
        return NO;

    qCDebug(lcQpaServices) << "Reading" << insertedString << "from service pasteboard" << pasteboard.name;
    [self insertText:insertedString replacementRange:{NSNotFound, 0}];
    return YES;
}

@end

#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(150000)
@implementation QNSView (ContentSelectionInfo)

/*
    This method is used by AppKit for positioning of context menus in
    response to the context menu keyboard hotkey, and for placement of
    the Writing Tools popup.
*/
- (NSRect)selectionAnchorRect
{
    if (queryInputMethod(self.focusObject)) {
        // We don't have a way of querying the selection rectangle via
        // the input method protocol (yet), so we use crude heuristics.
        const auto *inputMethod = qApp->inputMethod();
        auto cursorRect = inputMethod->cursorRectangle();
        auto anchorRect = inputMethod->anchorRectangle();
        auto selectionRect = cursorRect.united(anchorRect);
        if (cursorRect.top() != anchorRect.top()) {
            // Multi line selection. Assume the selections extends to
            // the entire width of the input item. This does not account
            // for center-aligned text and a bunch of other cases. FIXME
            auto itemClipRect = inputMethod->inputItemClipRectangle();
            selectionRect.setLeft(itemClipRect.left());
            selectionRect.setRight(itemClipRect.right());
        }
        return selectionRect.toCGRect();
    } else {
        return NSZeroRect;
    }
}
@end
#endif // macOS 15 SDK
