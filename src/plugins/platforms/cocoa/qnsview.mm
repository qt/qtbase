/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnsview.h"

#include <QtGui/QWindowSystemInterface>

#include <QtCore/QDebug>

@implementation QNSView

- (id) init
{
    self = [super init];
    if (self) {
        m_cgImage = 0;
        m_window = 0;
        m_buttons = Qt::NoButton;
    }
    return self;
}

- (id)initWithQWindow:(QWindow *)widget {
    self = [self init];
    if (self) {
        m_window = widget;
    }
    return self;
}

- (void) setImage:(QImage *)image
{
    CGImageRelease(m_cgImage);

    const uchar *imageData = image->bits();
    int bitDepth = image->depth();
    int colorBufferSize = 8;
    int bytesPrLine = image->bytesPerLine();
    int width = image->width();
    int height = image->height();

    CGColorSpaceRef cgColourSpaceRef = CGColorSpaceCreateDeviceRGB();

    CGDataProviderRef cgDataProviderRef = CGDataProviderCreateWithData(
                NULL,
                imageData,
                image->byteCount(),
                NULL);

    m_cgImage = CGImageCreate(width,
                              height,
                              colorBufferSize,
                              bitDepth,
                              bytesPrLine,
                              cgColourSpaceRef,
                              kCGImageAlphaNone,
                              cgDataProviderRef,
                              NULL,
                              false,
                              kCGRenderingIntentDefault);

    CGColorSpaceRelease(cgColourSpaceRef);

}

- (void) drawRect:(NSRect)dirtyRect
{
    if (!m_cgImage)
        return;

    CGRect dirtyCGRect = NSRectToCGRect(dirtyRect);

    NSGraphicsContext *nsGraphicsContext = [NSGraphicsContext currentContext];
    CGContextRef cgContext = (CGContextRef) [nsGraphicsContext graphicsPort];

    CGContextSaveGState( cgContext );
    int dy = dirtyCGRect.origin.y + CGRectGetMaxY(dirtyCGRect);
    CGContextTranslateCTM(cgContext, 0, dy);
    CGContextScaleCTM(cgContext, 1, -1);

    CGImageRef subImage = CGImageCreateWithImageInRect(m_cgImage, dirtyCGRect);
    CGContextDrawImage(cgContext,dirtyCGRect,subImage);

    CGContextRestoreGState(cgContext);

    CGImageRelease(subImage);

}

- (BOOL) isFlipped
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)handleMouseEvent:(NSEvent *)theEvent;
{
    NSPoint windowPoint = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    QPoint qt_windowPoint(windowPoint.x, windowPoint.y);

    NSTimeInterval timestamp = [theEvent timestamp];
    ulong qt_timestamp = timestamp * 1000;

    // ### Should the points be windowPoint and screenPoint?
    QWindowSystemInterface::handleMouseEvent(m_window, qt_timestamp, qt_windowPoint, qt_windowPoint, m_buttons);
}

- (void)mouseDown:(NSEvent *)theEvent
{
    m_buttons |= Qt::LeftButton;
    [self handleMouseEvent:theEvent];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    if (!(m_buttons & Qt::LeftButton))
        qWarning("Internal Mousebutton tracking invalid(missing Qt::LeftButton");
    [self handleMouseEvent:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent
{
    m_buttons &= QFlag(~int(Qt::LeftButton));
    [self handleMouseEvent:theEvent];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    [self handleMouseEvent:theEvent];
}
- (void)mouseEntered:(NSEvent *)theEvent
{
        Q_UNUSED(theEvent);
        QWindowSystemInterface::handleEnterEvent(m_window);
}
- (void)mouseExited:(NSEvent *)theEvent
{
        Q_UNUSED(theEvent);
        QWindowSystemInterface::handleLeaveEvent(m_window);
}
- (void)rightMouseDown:(NSEvent *)theEvent
{
        m_buttons |= Qt::RightButton;
    [self handleMouseEvent:theEvent];
}
- (void)rightMouseDragged:(NSEvent *)theEvent
{
        if (!(m_buttons & Qt::LeftButton))
            qWarning("Internal Mousebutton tracking invalid(missing Qt::LeftButton");
        [self handleMouseEvent:theEvent];
}
- (void)rightMouseUp:(NSEvent *)theEvent
{
        m_buttons &= QFlag(~int(Qt::RightButton));
        [self handleMouseEvent:theEvent];
}
- (void)otherMouseDown:(NSEvent *)theEvent
{
        m_buttons |= Qt::RightButton;
    [self handleMouseEvent:theEvent];
}
- (void)otherMouseDragged:(NSEvent *)theEvent
{
        if (!(m_buttons & Qt::LeftButton))
            qWarning("Internal Mousebutton tracking invalid(missing Qt::LeftButton");
        [self handleMouseEvent:theEvent];
}
- (void)otherMouseUp:(NSEvent *)theEvent
{
        m_buttons &= QFlag(~int(Qt::MiddleButton));
        [self handleMouseEvent:theEvent];
}

- (int) convertKeyCode : (QChar)keyChar
{
    if (keyChar.isLower())
        keyChar = keyChar.toUpper();
    int keyCode = keyChar.unicode();

    int qtKeyCode = Qt::Key(keyCode); // default case, overrides below
    switch (keyCode) {
        case NSEnterCharacter: qtKeyCode = Qt::Key_Enter; break;
        case NSBackspaceCharacter: qtKeyCode = Qt::Key_Backspace; break;
        case NSTabCharacter: qtKeyCode = Qt::Key_Tab; break;
        case NSNewlineCharacter:  qtKeyCode = Qt::Key_Return; break;
        case NSCarriageReturnCharacter: qtKeyCode = Qt::Key_Return; break;
        case NSBackTabCharacter: qtKeyCode = Qt::Key_Backtab; break;
        case 27 : qtKeyCode = Qt::Key_Escape; break;
        case NSDeleteCharacter : qtKeyCode = Qt::Key_Backspace; break; // Cocoa sends us delete when pressing backspace.
        case NSUpArrowFunctionKey: qtKeyCode = Qt::Key_Up; break;
        case NSDownArrowFunctionKey: qtKeyCode = Qt::Key_Down; break;
        case NSLeftArrowFunctionKey: qtKeyCode = Qt::Key_Left; break;
        case NSRightArrowFunctionKey: qtKeyCode = Qt::Key_Right; break;
        case NSInsertFunctionKey: qtKeyCode = Qt::Key_Insert; break;
        case NSDeleteFunctionKey: qtKeyCode = Qt::Key_Delete; break;
        case NSHomeFunctionKey: qtKeyCode = Qt::Key_Home; break;
        case NSEndFunctionKey: qtKeyCode = Qt::Key_End; break;
        case NSPageUpFunctionKey: qtKeyCode = Qt::Key_PageUp; break;
        case NSPageDownFunctionKey: qtKeyCode = Qt::Key_PageDown; break;
        case NSPrintScreenFunctionKey: qtKeyCode = Qt::Key_Print; break;
        case NSScrollLockFunctionKey: qtKeyCode = Qt::Key_ScrollLock; break;
        case NSPauseFunctionKey: qtKeyCode = Qt::Key_Pause; break;
        case NSSysReqFunctionKey: qtKeyCode = Qt::Key_SysReq; break;
        case NSMenuFunctionKey: qtKeyCode = Qt::Key_Menu; break;
        case NSHelpFunctionKey: qtKeyCode = Qt::Key_Help; break;
        default : break;
    }

    // handle all function keys (F1-F35)
    if (keyCode >= NSF1FunctionKey && keyCode <= NSF35FunctionKey)
        qtKeyCode = Qt::Key_F1 + (keyCode - NSF1FunctionKey);

    return qtKeyCode;
}

- (Qt::KeyboardModifiers) convertKeyModifiers : (ulong)modifierFlags
{
    Qt::KeyboardModifiers qtMods =Qt::NoModifier;
    if (modifierFlags &  NSShiftKeyMask)
        qtMods |= Qt::ShiftModifier;
    if (modifierFlags & NSControlKeyMask)
        qtMods |= Qt::MetaModifier;
    if (modifierFlags & NSAlternateKeyMask)
        qtMods |= Qt::AltModifier;
    if (modifierFlags & NSCommandKeyMask)
        qtMods |= Qt::ControlModifier;
    if (modifierFlags & NSNumericPadKeyMask)
        qtMods |= Qt::KeypadModifier;
    return qtMods;
}

- (void)handleKeyEvent:(NSEvent *)theEvent eventType:(int)eventType
{
    NSTimeInterval timestamp = [theEvent timestamp];
    ulong qt_timestamp = timestamp * 1000;
    QString characters = QString::fromUtf8([[theEvent characters] UTF8String]);
    Qt::KeyboardModifiers modifiers = [self convertKeyModifiers : [theEvent modifierFlags]];
    QChar ch([[theEvent charactersIgnoringModifiers] characterAtIndex:0]);
    int keyCode = [self convertKeyCode : ch];

    QWindowSystemInterface::handleKeyEvent(m_window, qt_timestamp, QEvent::Type(eventType), keyCode, modifiers, characters);
}

- (void)keyDown:(NSEvent *)theEvent
{
    [self handleKeyEvent : theEvent eventType :int(QEvent::KeyPress)];
}

- (void)keyUp:(NSEvent *)theEvent
{
    [self handleKeyEvent : theEvent eventType :int(QEvent::KeyRelease)];
}

@end
