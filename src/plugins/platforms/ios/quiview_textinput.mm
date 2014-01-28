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

- (BOOL)hasText
{
    return YES;
}

- (void)insertText:(NSString *)text
{
    QString string = QString::fromUtf8([text UTF8String]);
    int key = 0;
    if ([text isEqualToString:@"\n"]) {
        key = (int)Qt::Key_Return;
        if (self.returnKeyType == UIReturnKeyDone)
            [self resignFirstResponder];
    }

    // Send key event to window system interface
    QWindowSystemInterface::handleKeyEvent(
        0, QEvent::KeyPress, key, Qt::NoModifier, string, false, int(string.length()));
    QWindowSystemInterface::handleKeyEvent(
        0, QEvent::KeyRelease, key, Qt::NoModifier, string, false, int(string.length()));
}

- (void)deleteBackward
{
    // Send key event to window system interface
    QWindowSystemInterface::handleKeyEvent(
        0, QEvent::KeyPress, (int)Qt::Key_Backspace, Qt::NoModifier);
    QWindowSystemInterface::handleKeyEvent(
        0, QEvent::KeyRelease, (int)Qt::Key_Backspace, Qt::NoModifier);
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
