// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// This test is to check that the menus in a menubar are displayed correctly in both the top and
// bottom cases. Especially when using multiple screens. If possible relayout the screens in order
// to have one that is entirely in negative coordinates (i.e. the primary starts at 0x0 and the
// secondary is above it).

#include <QtWidgets>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformwindow_p.h>

class MainWindow : public QMainWindow
{
public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
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
        mainWindow->winId();
#ifdef Q_OS_WIN
        using namespace QNativeInterface::Private;
        if (auto *windowsWindow = dynamic_cast<QWindowsWindow *>(mainWindow->windowHandle()->handle()))
            windowsWindow->setHasBorderInFullScreen(true);
#endif
        mainWindow->showMaximized();
    }
    int ret = a.exec();
    qDeleteAll(windows);
    return ret;
}
