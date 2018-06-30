/****************************************************************************
 **
 ** Copyright (C) 2018 The Qt Company Ltd.
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

// This test is to check that the menus in a menubar are displayed correctly in both the top and
// bottom cases. Especially when using multiple screens. If possible relayout the screens in order
// to have one that is entirely in negative coordinates (i.e. the primary starts at 0x0 and the
// secondary is above it).

#include <QtWidgets>
#include <QtPlatformHeaders/QWindowsWindowFunctions>

class MainWindow : public QMainWindow
{
public:
    MainWindow(QWidget *parent = 0) : QMainWindow(parent)
    {
        auto *menu1Act1 = new QAction("Action 1");
        auto *menu1Act2 = new QAction("Action 2");
        auto *menu1Act3 = new QAction("Action 3");
        auto *menu1Act4 = new QAction("Action 4");
        auto *menu2Act1 = new QAction("2- Action 1");
        auto *menu2Act2 = new QAction("2- Action 2");
        auto *menu2Act3 = new QAction("2- Action 3");
        auto *menu2Act4 = new QAction("2- Action 4");
        auto *menu1 = new QMenu("Menu 1");
        menu1->addAction(menu1Act1);
        menu1->addAction(menu1Act2);
        menu1->addAction(menu1Act3);
        menu1->addAction(menu1Act4);
        auto *menu2 = new QMenu("Menu 2");
        menu2->addAction(menu2Act1);
        menu2->addAction(menu2Act2);
        menu2->addAction(menu2Act3);
        menu2->addAction(menu2Act4);
        menuBar()->addMenu(menu1);
        menuBar()->addMenu(menu2);
        menuBar()->setNativeMenuBar(false);

        auto *menu1Bottom = new QMenu("Menu 1");
        menu1Bottom->addAction(menu1Act1);
        menu1Bottom->addAction(menu1Act2);
        menu1Bottom->addAction(menu1Act3);
        menu1Bottom->addAction(menu1Act4);
        auto *menu2Bottom = new QMenu("Menu 2");
        menu2Bottom->addAction(menu2Act1);
        menu2Bottom->addAction(menu2Act2);
        menu2Bottom->addAction(menu2Act3);
        menu2Bottom->addAction(menu2Act4);

        QWidget *central = new QWidget;
        QVBoxLayout *layout = new QVBoxLayout;
        auto *menuBarBottom = new QMenuBar(this);
        menuBarBottom->addMenu(menu1Bottom);
        menuBarBottom->addMenu(menu2Bottom);
        menuBarBottom->setDefaultUp(true);
        menuBarBottom->setNativeMenuBar(false);
        layout->addWidget(menuBarBottom);
        layout->setAlignment(menuBarBottom, Qt::AlignBottom);
        central->setLayout(layout);
        setCentralWidget(central);
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    }
};

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    QList<MainWindow *> windows;
    for (QScreen *screen : QApplication::screens()) {
        MainWindow *mainWindow = new MainWindow;
        mainWindow->setGeometry(screen->geometry());
        QWindowsWindowFunctions::setHasBorderInFullScreen(mainWindow->windowHandle(), true);
        mainWindow->showMaximized();
    }
    int ret = a.exec();
    qDeleteAll(windows);
    return ret;
}
