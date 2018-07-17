/****************************************************************************
 **
 ** Copyright (C) 2017 The Qt Company Ltd.
 ** Contact: https://www.qt.io/licensing/
 **
 ** This file is part of the test suite of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:GPL-EXCEPT$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and The Qt Company. For licensing terms
 ** and conditions see https://www.qt.io/terms-conditions. For further
 ** information use the contact form at https://www.qt.io/contact-us.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU
 ** General Public License version 3 as published by the Free Software
 ** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
 ** included in the packaging of this file. Please review the following
 ** information to ensure the GNU General Public License requirements will
 ** be met: https://www.gnu.org/licenses/gpl-3.0.html.
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

// This tests that when in a multiple screen setup, that screens that have a top-left of 0x0 or
// a top left of being above/below the other screen then showing the toolbutton menu will be
// placed correctly.

#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QScreen>

class MyMainWindow : public QMainWindow
{
public:
    MyMainWindow(QWidget *parent = 0) : QMainWindow(parent)
    {
        auto *toolBar = new QToolBar;
        QPixmap pix(16, 16);
        pix.fill(Qt::red);
        auto *button = new QToolButton;
        button->setIcon(pix);
        toolBar->addWidget(button);
        auto *menu = new QMenu(button);
        for (int i = 0; i < 10; ++i)
            menu->addAction(QString("Test Action %1").arg(i));
        button->setMenu(menu);
        addToolBar(toolBar);
    }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QList<MyMainWindow *> windows;
    for (QScreen *s : a.screens()) {
        MyMainWindow *w = new MyMainWindow;
        w->setGeometry(s->availableGeometry());
        w->show();
        windows << w;
    }
    int ret = a.exec();
    qDeleteAll(windows);
    return ret;
}

