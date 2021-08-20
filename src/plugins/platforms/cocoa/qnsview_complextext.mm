/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

// This file is included from qnsview.mm, and only used to organize the code

@implementation QNSView (ComplexText)

// ------------- Text insertion -------------

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

    if (m_sendKeyEvent && m_composingText.isEmpty() && [text isEqualToString:m_inputSource]) {
        // We do not send input method events for simple text input,
        // and instead let handleKeyEvent send the key event.
        qCDebug(lcQpaKeys) << "Not sending simple text as input method event";
        return;
    }

    const bool isAttributedString = [text isKindOfClass:NSAttributedString.class];
    QString commitString = QString::fromNSString(isAttributedString ? [text string] : text);

    QObject *focusObject = m_platformWindow->window()->focusObject();
    if (queryInputMethod(focusObject)) {
        QInputMethodEvent e;
        e.setCommitString(commitString);
        QCoreApplication::sendEvent(focusObject, &e);
        // prevent handleKeyEvent from sending a key event
        m_sendKeyEvent = false;
    }

    m_composingText.clear();
    m_composingFocusObject = nullptr;
}

- (void)insertNewline:(id)sender
{
    Q_UNUSED(sender);
    qCDebug(lcQpaKeys) << "Inserting newline";
    m_resendKeyEvent = true;
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
    QString preeditString = QString::fromNSString(isAttributedString ? [text string] : text);

    QList<QInputMethodEvent::Attribute> preeditAttributes;
    preeditAttributes << QInputMethodEvent::Attribute(
        QInputMethodEvent::Cursor, selectedRange.location + selectedRange.length, true);

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
            // input methods to highlight the selected clause segments, so we fake
            // it using QTextCharFormat::WaveUnderline.
            if ((style & NSUnderlineStyleThick) == NSUnderlineStyleThick
                || (style & NSUnderlineStyleDouble) == NSUnderlineStyleDouble)
                format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
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

    m_composingText = preeditString;

    if (QObject *focusObject = m_platformWindow->window()->focusObject()) {
        m_composingFocusObject = focusObject;
        if (queryInputMethod(focusObject)) {
            QInputMethodEvent event(preeditString, preeditAttributes);
            QCoreApplication::sendEvent(focusObject, &event);
            // prevent handleKeyEvent from sending a key event
            m_sendKeyEvent = false;
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

- (NSRange)markedRange
{
    NSRange range;
    if (!m_composingText.isEmpty()) {
        range.location = 0;
        range.length = m_composingText.length();
    } else {
        range.location = NSNotFound;
        range.length = 0;
    }
    return range;
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
        QObject *focusObject = m_platformWindow->window()->focusObject();
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
    qCDebug(lcQpaKeys) << "Trying to perform command" << selector;
    [self tryToPerform:selector with:self];
}

// ------------- Various text properties -------------

- (NSRange)selectedRange
{
    QObject *focusObject = m_platformWindow->window()->focusObject();
    if (auto queryResult = queryInputMethod(focusObject, Qt::ImCurrentSelection)) {
        QString selectedText = queryResult.value(Qt::ImCurrentSelection).toString();
        return selectedText.isEmpty() ? NSMakeRange(0, 0) : NSMakeRange(0, selectedText.length());
    } else {
        return NSMakeRange(0, 0);
    }
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
    Q_UNUSED(actualRange);

    QObject *focusObject = m_platformWindow->window()->focusObject();
    if (auto queryResult = queryInputMethod(focusObject, Qt::ImCurrentSelection)) {
        QString selectedText = queryResult.value(Qt::ImCurrentSelection).toString();
        if (selectedText.isEmpty())
            return nil;

        NSString *substring = QStringView(selectedText).mid(range.location, range.length).toNSString();
        return [[[NSAttributedString alloc] initWithString:substring] autorelease];

    } else {
        return nil;
    }
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
    Q_UNUSED(range);
    Q_UNUSED(actualRange);

    QWindow *window = m_platformWindow->window();
    if (queryInputMethod(window->focusObject())) {
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

@end
