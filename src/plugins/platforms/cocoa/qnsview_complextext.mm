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

    if (m_composingFocusObject) {
        QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
        if (QCoreApplication::sendEvent(m_composingFocusObject, &queryEvent)) {
            if (queryEvent.value(Qt::ImEnabled).toBool()) {
                QInputMethodEvent e;
                QCoreApplication::sendEvent(m_composingFocusObject, &e);
            }
        }
    }

    m_composingText.clear();
    m_composingFocusObject = nullptr;
}

- (void)unmarkText
{
    if (!m_composingText.isEmpty()) {
        if (QObject *fo = m_platformWindow->window()->focusObject()) {
            QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
            if (QCoreApplication::sendEvent(fo, &queryEvent)) {
                if (queryEvent.value(Qt::ImEnabled).toBool()) {
                    QInputMethodEvent e;
                    e.setCommitString(m_composingText);
                    QCoreApplication::sendEvent(fo, &e);
                }
            }
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
    m_resendKeyEvent = true;
}

- (void)doCommandBySelector:(SEL)aSelector
{
    [self tryToPerform:aSelector with:self];
}

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange
{
    Q_UNUSED(replacementRange)

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
    if (QObject *fo = m_platformWindow->window()->focusObject()) {
        QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
        if (QCoreApplication::sendEvent(fo, &queryEvent)) {
            if (queryEvent.value(Qt::ImEnabled).toBool()) {
                QInputMethodEvent e;
                e.setCommitString(commitString);
                QCoreApplication::sendEvent(fo, &e);
                // prevent handleKeyEvent from sending a key event
                m_sendKeyEvent = false;
            }
        }
    }

    m_composingText.clear();
    m_composingFocusObject = nullptr;
}

- (void)setMarkedText:(id)aString selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
    Q_UNUSED(replacementRange)
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

    if (QObject *fo = m_platformWindow->window()->focusObject()) {
        m_composingFocusObject = fo;
        QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
        if (QCoreApplication::sendEvent(fo, &queryEvent)) {
            if (queryEvent.value(Qt::ImEnabled).toBool()) {
                QInputMethodEvent e(preeditString, attrs);
                QCoreApplication::sendEvent(fo, &e);
                // prevent handleKeyEvent from sending a key event
                m_sendKeyEvent = false;
            }
        }
    }
}

- (BOOL)hasMarkedText
{
    return (m_composingText.isEmpty() ? NO: YES);
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
    Q_UNUSED(actualRange)
    QObject *fo = m_platformWindow->window()->focusObject();
    if (!fo)
        return nil;
    QInputMethodQueryEvent queryEvent(Qt::ImEnabled | Qt::ImCurrentSelection);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return nil;
    if (!queryEvent.value(Qt::ImEnabled).toBool())
        return nil;

    QString selectedText = queryEvent.value(Qt::ImCurrentSelection).toString();
    if (selectedText.isEmpty())
        return nil;

    QCFString string(selectedText.mid(aRange.location, aRange.length));
    const NSString *tmpString = reinterpret_cast<const NSString *>((CFStringRef)string);
    return [[[NSAttributedString alloc]  initWithString:const_cast<NSString *>(tmpString)] autorelease];
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
    NSRange selectedRange = {0, 0};

    QObject *fo = m_platformWindow->window()->focusObject();
    if (!fo)
        return selectedRange;
    QInputMethodQueryEvent queryEvent(Qt::ImEnabled | Qt::ImCurrentSelection);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return selectedRange;
    if (!queryEvent.value(Qt::ImEnabled).toBool())
        return selectedRange;

    QString selectedText = queryEvent.value(Qt::ImCurrentSelection).toString();

    if (!selectedText.isEmpty()) {
        selectedRange.location = 0;
        selectedRange.length = selectedText.length();
    }
    return selectedRange;
}

- (NSRect)firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
    Q_UNUSED(aRange)
    Q_UNUSED(actualRange)

    QObject *fo = m_platformWindow->window()->focusObject();
    if (!fo)
        return NSZeroRect;

    QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return NSZeroRect;
    if (!queryEvent.value(Qt::ImEnabled).toBool())
        return NSZeroRect;

    // The returned rect is always based on the internal cursor.
    QRect mr = qApp->inputMethod()->cursorRectangle().toRect();
    mr.moveBottomLeft(m_platformWindow->window()->mapToGlobal(mr.bottomLeft()));
    return QCocoaScreen::mapToNative(mr);
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

    QObject *fo = m_platformWindow->window()->focusObject();
    if (!fo)
        return nil;

    QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
    if (!QCoreApplication::sendEvent(fo, &queryEvent))
        return nil;
    if (!queryEvent.value(Qt::ImEnabled).toBool())
        return nil;

    // Support only underline color/style.
    return @[NSUnderlineColorAttributeName, NSUnderlineStyleAttributeName];
}

- (void)textInputContextKeyboardSelectionDidChangeNotification:(NSNotification *)textInputContextKeyboardSelectionDidChangeNotification
{
    Q_UNUSED(textInputContextKeyboardSelectionDidChangeNotification)
    if (([NSApp keyWindow] == self.window) && self.window.firstResponder == self) {
        if (QCocoaInputContext *ic = qobject_cast<QCocoaInputContext *>(QCocoaIntegration::instance()->inputContext()))
            ic->updateLocale();
    }
}

@end
