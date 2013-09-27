/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#import <Cocoa/Cocoa.h>

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QMacNativeWidget>

class RedWidget : public QWidget
{
public:
    RedWidget() {

    }

    void resizeEvent(QResizeEvent *)
    {
        qDebug() << "RedWidget::resize" << size();
    }

    void paintEvent(QPaintEvent *event)
    {
        QPainter p(this);
        Q_UNUSED(event);
        QRect rect(QPoint(0, 0), size());
        qDebug() << "Painting geometry" << rect;
        p.fillRect(rect, QColor(133, 50, 50));
    }
};

namespace {
int qtArgc = 0;
char **qtArgv;
QApplication *qtApp = 0;
}

@interface WindowCreator : NSObject <NSApplicationDelegate>
@end

@implementation WindowCreator

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    Q_UNUSED(notification)
    // Qt widgets rely on a QApplication being alive somewhere
    qtApp = new QApplication(qtArgc, qtArgv);

    // Create the NSWindow
    NSRect frame = NSMakeRect(500, 500, 500, 500);
    NSWindow* window  = [[NSWindow alloc] initWithContentRect:frame
                        styleMask:NSTitledWindowMask |  NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask
                        backing:NSBackingStoreBuffered
                        defer:NO];
    [window setTitle:@"NSWindow"];

    // Create widget hierarchy with QPushButton and QLineEdit
    QMacNativeWidget *nativeWidget = new QMacNativeWidget();
    // Get the NSView for QMacNativeWidget and set it as the content view for the NSWindow
    [window setContentView:nativeWidget->nativeView()];

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addWidget(new QPushButton("Push", nativeWidget));
    hlayout->addWidget(new QLineEdit(nativeWidget));

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->addLayout(hlayout);

    //RedWidget * redWidget = new RedWidget;
    //vlayout->addWidget(redWidget);

    nativeWidget->setLayout(vlayout);


    // show() must be called on nativeWiget to get the widgets int he correct state.
    nativeWidget->show();

    // Show the NSWindow
    [window makeKeyAndOrderFront:NSApp];
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    Q_UNUSED(notification)

    delete qtApp;
}

@end

int main(int argc, char *argv[])
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    Q_UNUSED(pool);

    // Normally, we would use let the main bundle instanciate and set
    // the application delegate, but we set it manually for conciseness.
    WindowCreator *windowCreator= [WindowCreator alloc];
    [[NSApplication sharedApplication] setDelegate:windowCreator];

    // Save these for QApplication
    qtArgc = argc;
    qtArgv = argv;

    // Other than the few lines above, it's business as usual...
    return NSApplicationMain(argc, (const char **)argv);
}
