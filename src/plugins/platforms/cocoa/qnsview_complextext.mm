/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

@implementation QNSView (ComplexTextAPI)

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

@end

@implementation QNSView (ComplexText)

- (void)insertNewline:(id)sender
{
    Q_UNUSED(sender);
    qCDebug(lcQpaKeys) << "Inserting newline";
    m_resendKeyEvent = true;
}

- (void)doCommandBySelector:(SEL)aSelector
{
    qCDebug(lcQpaKeys) << "Trying to perform command" << aSelector;
    [self tryToPerform:aSelector with:self];
}

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange
{
    qCDebug(lcQpaKeys).nospace() << "Inserting \"" << aString << "\""
        << ", replacing range " << replacementRange;

    if (m_sendKeyEvent && m_composingText.isEmpty() && [aString isEqualToString:m_inputSource]) {
        // don't send input method events for simple text input (let handleKeyEvent send key events instead)
        return;
    }

    QString commitString;
    if ([aString length]) {
        if ([aString isKindOfClass:[NSAttributedString class]]) {
            commitString = QString::fromCFString(reinterpret_cast<CFStringRef>([aString string]));
        } else {
            commitString = QString::fromCFString(reinterpret_cast<CFStringRef>(aString));
        };
    }

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

- (void)setMarkedText:(id)aString selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
    qCDebug(lcQpaKeys).nospace() << "Marking \"" << aString << "\""
        << " with selected range " << selectedRange
        << ", replacing range " << replacementRange;

    QString preeditString;

    QList<QInputMethodEvent::Attribute> attrs;
    attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, selectedRange.location + selectedRange.length, 1, QVariant());

    if ([aString isKindOfClass:[NSAttributedString class]]) {
        // Preedit string has attribution
        preeditString = QString::fromCFString(reinterpret_cast<CFStringRef>([aString string]));
        int composingLength = preeditString.length();
        int index = 0;
        // Create attributes for individual sections of preedit text
        while (index < composingLength) {
            NSRange effectiveRange;
            NSRange range = NSMakeRange(index, composingLength-index);
            NSDictionary *attributes = [aString attributesAtIndex:index
                                            longestEffectiveRange:&effectiveRange
                                                          inRange:range];
            NSNumber *underlineStyle = [attributes objectForKey:NSUnderlineStyleAttributeName];
            if (underlineStyle) {
                QColor clr (Qt::black);
                NSColor *color = [attributes objectForKey:NSUnderlineColorAttributeName];
                if (color) {
                    clr = qt_mac_toQColor(color);
                }
                QTextCharFormat format;
                format.setFontUnderline(true);
                format.setUnderlineColor(clr);
                attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                                    effectiveRange.location,
                                                    effectiveRange.length,
                                                    format);
            }
            index = effectiveRange.location + effectiveRange.length;
        }
    } else {
        // No attributes specified, take only the preedit text.
        preeditString = QString::fromCFString(reinterpret_cast<CFStringRef>(aString));
    }

    if (attrs.isEmpty()) {
        QTextCharFormat format;
        format.setFontUnderline(true);
        attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                            0, preeditString.length(), format);
    }

    m_composingText = preeditString;

    if (QObject *focusObject = m_platformWindow->window()->focusObject()) {
        m_composingFocusObject = focusObject;
        if (queryInputMethod(focusObject)) {
            QInputMethodEvent e(preeditString, attrs);
            QCoreApplication::sendEvent(focusObject, &e);
            // prevent handleKeyEvent from sending a key event
            m_sendKeyEvent = false;
        }
    }
}

- (BOOL)hasMarkedText
{
    return (m_composingText.isEmpty() ? NO: YES);
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
    Q_UNUSED(actualRange);

    QObject *focusObject = m_platformWindow->window()->focusObject();
    if (auto queryResult = queryInputMethod(focusObject, Qt::ImCurrentSelection)) {
        QString selectedText = queryResult.value(Qt::ImCurrentSelection).toString();
        if (selectedText.isEmpty())
            return nil;

        QCFString string(selectedText.mid(aRange.location, aRange.length));
        const NSString *tmpString = reinterpret_cast<const NSString *>((CFStringRef)string);
        return [[[NSAttributedString alloc] initWithString:const_cast<NSString *>(tmpString)] autorelease];
    } else {
        return nil;
    }
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

- (NSRect)firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
    Q_UNUSED(aRange);
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

- (NSUInteger)characterIndexForPoint:(NSPoint)aPoint
{
    // We don't support cursor movements using mouse while composing.
    Q_UNUSED(aPoint);
    return NSNotFound;
}

- (NSArray<NSString *> *)validAttributesForMarkedText
{
    if (!m_platformWindow)
        return nil;

    if (m_platformWindow->window() != QGuiApplication::focusWindow())
        return nil;

    if (queryInputMethod(m_platformWindow->window()->focusObject()))
        return @[NSUnderlineColorAttributeName, NSUnderlineStyleAttributeName];
    else
        return nil;
}

- (void)textInputContextKeyboardSelectionDidChangeNotification:(NSNotification *)textInputContextKeyboardSelectionDidChangeNotification
{
    Q_UNUSED(textInputContextKeyboardSelectionDidChangeNotification);
    if (([NSApp keyWindow] == self.window) && self.window.firstResponder == self) {
        if (QCocoaInputContext *ic = qobject_cast<QCocoaInputContext *>(QCocoaIntegration::instance()->inputContext()))
            ic->updateLocale();
    }
}

@end
