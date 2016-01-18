/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#import <AppKit/AppKit.h>

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
