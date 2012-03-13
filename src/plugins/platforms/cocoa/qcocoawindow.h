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

#ifndef QCOCOAWINDOW_H
#define QCOCOAWINDOW_H

#include <Cocoa/Cocoa.h>

#include <QPlatformWindow>
#include <QRect>

#include "qcocoaglcontext.h"
#include "qnsview.h"
class QCocoaWindow;

@interface QNSWindow : NSWindow {
    @public QCocoaWindow *m_cocoaPlatformWindow;
}

- (BOOL)canBecomeKeyWindow;
@end

@interface QNSPanel : NSPanel {
    @public QCocoaWindow *m_cocoaPlatformWindow;
}
- (BOOL)canBecomeKeyWindow;
@end

QT_BEGIN_NAMESPACE
// QCocoaWindow
//
// QCocoaWindow is an NSView (not an NSWindow!) in the sense
// that it relies on a NSView for all event handling and
// graphics output and does not require a NSWindow, except for
// for the window-related functions like setWindowTitle.
//
// As a consequence of this it is possible to embed the QCocoaWindow
// in an NSView hierarchy by getting a pointer to the "backing"
// NSView and not calling QCocoaWindow::show():
//
// QWindow *qtWindow = new MyWindow();
// qtWindow->create();
// QPlatformNativeInterface *platformNativeInterface = QGuiApplication::platformNativeInterface();
// NSView *qtView = (NSView *)platformNativeInterface->nativeResourceForWindow("nsview", qtWindow);
// [parentView addSubview:qtView];
//
// See the qt_on_cocoa manual tests for a working example, located
// in tests/manual/cocoa at the time of writing.

class QCocoaWindow : public QPlatformWindow
{
public:
    QCocoaWindow(QWindow *tlw);
    ~QCocoaWindow();

    void setGeometry(const QRect &rect);
    void setCocoaGeometry(const QRect &rect);
    void setVisible(bool visible);
    Qt::WindowFlags setWindowFlags(Qt::WindowFlags flags);
    void setWindowTitle(const QString &title);
    void raise();
    void lower();
    void propagateSizeHints();
    bool setKeyboardGrabEnabled(bool grab);
    bool setMouseGrabEnabled(bool grab);

    WId winId() const;
    void setParent(const QPlatformWindow *window);

    NSView *contentView() const;

    void windowWillMove();
    void windowDidMove();
    void windowDidResize();
    void windowWillClose();

    void setCurrentContext(QCocoaGLContext *context);
    QCocoaGLContext *currentContext() const;

protected:
    // NSWindow handling. The QCocoaWindow/QNSView can either be displayed
    // in an existing NSWindow or in one created by Qt.
    void recreateWindow(const QPlatformWindow *parentWindow);
    NSWindow *createNSWindow();
    void setNSWindow(NSWindow *window);
    void clearNSWindow(NSWindow *window);

    QRect windowGeometry() const;
    QCocoaWindow *parentCocoaWindow() const;

// private:
public: // for QNSView
    friend class QCocoaBackingStore;
    friend class QCocoaNativeInterface;

    QNSView *m_contentView;
    QNSWindow *m_nsWindow;
    Qt::WindowFlags m_windowFlags;
    QPointer<QWindow> m_activePopupWindow;

    bool m_inConstructor;
    QCocoaGLContext *m_glContext;
};

QT_END_NAMESPACE

#endif // QCOCOAWINDOW_H

