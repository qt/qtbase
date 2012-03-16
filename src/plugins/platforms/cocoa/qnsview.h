/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNSVIEW_H
#define QNSVIEW_H

#include <Cocoa/Cocoa.h>

#include <QtCore/QPointer>
#include <QtGui/QImage>
#include <QtGui/QAccessible>

QT_BEGIN_NAMESPACE
class QCocoaWindow;
QT_END_NAMESPACE

@interface QNSView : NSView <NSTextInput> {
    CGImageRef m_cgImage;
    QWindow *m_window;
    QCocoaWindow *m_platformWindow;
    Qt::MouseButtons m_buttons;
    QAccessibleInterface *m_accessibleRoot;
    QString m_composingText;
    bool m_keyEventsAccepted;
    QStringList *currentCustomDragTypes;
    Qt::KeyboardModifiers currentWheelModifiers;
}

- (id)init;
- (id)initWithQWindow:(QWindow *)window platformWindow:(QCocoaWindow *) platformWindow;

- (void)setImage:(QImage *)image;
- (void)drawRect:(NSRect)dirtyRect;
- (void)updateGeometry;
- (void)windowDidBecomeKey;
- (void)windowDidResignKey;

- (BOOL)isFlipped;
- (BOOL)acceptsFirstResponder;

- (void)handleMouseEvent:(NSEvent *)theEvent;
- (void)mouseDown:(NSEvent *)theEvent;
- (void)mouseDragged:(NSEvent *)theEvent;
- (void)mouseUp:(NSEvent *)theEvent;
- (void)mouseMoved:(NSEvent *)theEvent;
- (void)mouseEntered:(NSEvent *)theEvent;
- (void)mouseExited:(NSEvent *)theEvent;
- (void)rightMouseDown:(NSEvent *)theEvent;
- (void)rightMouseDragged:(NSEvent *)theEvent;
- (void)rightMouseUp:(NSEvent *)theEvent;
- (void)otherMouseDown:(NSEvent *)theEvent;
- (void)otherMouseDragged:(NSEvent *)theEvent;
- (void)otherMouseUp:(NSEvent *)theEvent;

- (int) convertKeyCode : (QChar)keyCode;
- (Qt::KeyboardModifiers) convertKeyModifiers : (ulong)modifierFlags;
- (void)handleKeyEvent:(NSEvent *)theEvent eventType:(int)eventType;
- (void)keyDown:(NSEvent *)theEvent;
- (void)keyUp:(NSEvent *)theEvent;

- (void)registerDragTypes;
- (NSDragOperation)handleDrag:(id <NSDraggingInfo>)sender;

@end

#endif //QNSVIEW_H
