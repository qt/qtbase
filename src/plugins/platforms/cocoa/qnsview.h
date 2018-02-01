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

#ifndef QNSVIEW_H
#define QNSVIEW_H

#include <AppKit/AppKit.h>

#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtGui/QImage>
#include <QtGui/QAccessible>

#include "private/qcore_mac_p.h"

QT_BEGIN_NAMESPACE
class QCocoaWindow;
class QCocoaGLContext;
QT_END_NAMESPACE

Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(QNSViewMouseMoveHelper));

@interface QT_MANGLE_NAMESPACE(QNSView) : NSView <NSTextInputClient> {
    QPointer<QCocoaWindow> m_platformWindow;
    NSTrackingArea *m_trackingArea;
    Qt::MouseButtons m_buttons;
    Qt::MouseButtons m_acceptedMouseDowns;
    Qt::MouseButtons m_frameStrutButtons;
    QString m_composingText;
    QPointer<QObject> m_composingFocusObject;
    bool m_sendKeyEvent;
    QStringList *currentCustomDragTypes;
    bool m_dontOverrideCtrlLMB;
    bool m_sendUpAsRightButton;
    Qt::KeyboardModifiers currentWheelModifiers;
#ifndef QT_NO_OPENGL
    QCocoaGLContext *m_glContext;
    bool m_shouldSetGLContextinDrawRect;
#endif
    NSString *m_inputSource;
    QT_MANGLE_NAMESPACE(QNSViewMouseMoveHelper) *m_mouseMoveHelper;
    bool m_resendKeyEvent;
    bool m_scrolling;
    bool m_updatingDrag;
    NSEvent *m_currentlyInterpretedKeyEvent;
    bool m_isMenuView;
    QSet<quint32> m_acceptedKeyDowns;
    bool m_updateRequested;
}

@property (nonatomic, retain) NSCursor *cursor;

- (id)init;
- (id)initWithCocoaWindow:(QCocoaWindow *)platformWindow;
#ifndef QT_NO_OPENGL
- (void)setQCocoaGLContext:(QCocoaGLContext *)context;
#endif
- (void)drawRect:(NSRect)dirtyRect;
- (void)textInputContextKeyboardSelectionDidChangeNotification : (NSNotification *) textInputContextKeyboardSelectionDidChangeNotification;
- (void)viewDidHide;
- (void)removeFromSuperview;
- (void)cancelComposingText;

- (BOOL)isFlipped;
- (BOOL)acceptsFirstResponder;
- (BOOL)becomeFirstResponder;
- (BOOL)isOpaque;

- (void)convertFromScreen:(NSPoint)mouseLocation toWindowPoint:(QPointF *)qtWindowPoint andScreenPoint:(QPointF *)qtScreenPoint;

- (void)resetMouseButtons;

- (void)requestUpdate;

- (void)handleMouseEvent:(NSEvent *)theEvent;
- (bool)handleMouseDownEvent:(NSEvent *)theEvent withButton:(int)buttonNumber;
- (bool)handleMouseDraggedEvent:(NSEvent *)theEvent withButton:(int)buttonNumber;
- (bool)handleMouseUpEvent:(NSEvent *)theEvent withButton:(int)buttonNumber;
- (void)mouseDown:(NSEvent *)theEvent;
- (void)mouseDragged:(NSEvent *)theEvent;
- (void)mouseUp:(NSEvent *)theEvent;
- (void)mouseMovedImpl:(NSEvent *)theEvent;
- (void)mouseEnteredImpl:(NSEvent *)theEvent;
- (void)mouseExitedImpl:(NSEvent *)theEvent;
- (void)rightMouseDown:(NSEvent *)theEvent;
- (void)rightMouseDragged:(NSEvent *)theEvent;
- (void)rightMouseUp:(NSEvent *)theEvent;
- (void)otherMouseDown:(NSEvent *)theEvent;
- (void)otherMouseDragged:(NSEvent *)theEvent;
- (void)otherMouseUp:(NSEvent *)theEvent;
- (void)handleFrameStrutMouseEvent:(NSEvent *)theEvent;

#ifndef QT_NO_TABLETEVENT
- (bool)handleTabletEvent: (NSEvent *)theEvent;
- (void)tabletPoint: (NSEvent *)theEvent;
- (void)tabletProximity: (NSEvent *)theEvent;
#endif

- (int) convertKeyCode : (QChar)keyCode;
+ (Qt::KeyboardModifiers) convertKeyModifiers : (ulong)modifierFlags;
- (bool)handleKeyEvent:(NSEvent *)theEvent eventType:(int)eventType;
- (void)keyDown:(NSEvent *)theEvent;
- (void)keyUp:(NSEvent *)theEvent;

- (void)registerDragTypes;
- (NSDragOperation)handleDrag:(id <NSDraggingInfo>)sender;

@end

@interface QT_MANGLE_NAMESPACE(QNSView) (QtExtras)
@property (nonatomic, readonly) QCocoaWindow *platformWindow;
@property (nonatomic, readonly) BOOL isMenuView;
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSView);

#endif //QNSVIEW_H
