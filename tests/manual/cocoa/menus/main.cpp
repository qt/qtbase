/****************************************************************************
**
** Copyright (C) 2012 KDAB
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

#include <QtGui>
#include <QtWidgets>


class Responder : public QObject
{
    Q_OBJECT

public:
    Responder(QObject *pr) :
        QObject(pr)
    {
    }

public slots:


    void toggleChecked(bool b)
    {
        QAction *a = qobject_cast<QAction *>(sender());
    }

    void showModalDialog()
    {
        QMessageBox::information(NULL, "Something", "Something happened. Modally.");
    }

    void doPreferences()
    {
        qDebug() << "show preferences";
    }

    void aboutToShowSubmenu()
    {
        QMenu* m = (QMenu*) sender();
        qDebug() << "will show" << m;

        m->clear();

        for (int i=0; i<10; ++i) {
            m->addAction(QString("Recent File %1").arg(i + 1));
        }
    }
};

void createWindow1()
{

    QMainWindow *window = new QMainWindow;
    QMenu *menu = new QMenu("TestMenu", window);

    window->menuBar()->addMenu(menu);

    Responder *r = new Responder(window);

    QAction *a = menu->addAction("TestMenuItem1");
    a->setShortcut( Qt::Key_A | Qt::SHIFT | Qt::CTRL );
    QObject::connect(a, SIGNAL(triggered()),
        r, SLOT(showModalDialog()));


    menu->addAction("T&estMenuItem2");
    a = menu->addAction("Preferences");
    a->setMenuRole(QAction::PreferencesRole);
    QObject::connect(a, SIGNAL(triggered()),
        r, SLOT(doPreferences()));

    a = menu->addAction("TestMenuItem4");
    a->setShortcut( Qt::Key_W | Qt::CTRL);

    QMenu *menu2 = new QMenu("SecondMenu", window);
    window->menuBar()->addMenu(menu2);

    menu2->addAction("Yellow");
    a = menu2->addAction("Mau&ve");

    QFont f;
    f.setPointSize(9);
    a->setFont(f);

    menu2->addAction("Taupe");

    QMenu *submenu1 = new QMenu("Submenu", window);
    submenu1->addAction("Sub Item 1");
    submenu1->addAction("Sub Item 2");
    submenu1->addAction("Sub Item 3");
    menu2->addMenu(submenu1);

    QMenu *submenu2 = new QMenu("Deeper", window);
    submenu2->addAction("Sub Sub Item 1");
    submenu2->addAction("Sub Sub Item 2");
    submenu2->addAction("Sub Sub Item 3");
    submenu1->addMenu(submenu2);

    QMenu *menu3 = new QMenu("A Third Menu", window);

    menu3->addAction("Eins");

    QMenu *submenu3 = new QMenu("Dynamic", window);
    QObject::connect(submenu3, SIGNAL(aboutToShow()), r, SLOT(aboutToShowSubmenu()));
    menu3->addMenu(submenu3);

    a = menu3->addAction("Zwei");
    a->setShortcut( Qt::Key_3 | Qt::ALT);
    a = menu3->addAction("About Drei...");
    a->setMenuRole(QAction::AboutRole);

    window->menuBar()->addMenu(menu3);

    QAction *checkableAction = new QAction("Thing Enabled", window);
    checkableAction->setCheckable(true);
    checkableAction->setChecked(true);
    QObject::connect(checkableAction, SIGNAL(triggered(bool)),
        r, SLOT(toggleChecked(bool)));

    menu2->addAction(checkableAction);

    window->show();

}

void createWindow2()
{
    QMainWindow *window = new QMainWindow;
    QMenu *menu = new QMenu("Nuts", window);

    window->menuBar()->addMenu(menu);

    menu->addAction("Peanuts");
    menu->addAction("Walnuts");

    QMenu *menu2 = new QMenu("Colours", window);
    window->menuBar()->addMenu(menu2);

    menu2->addAction("Pink");
    menu2->addAction("Yellow");
    menu2->addAction("Grape");

    QMenu *menu3 = new QMenu("Edit", window);
    menu3->addAction("Cut");
    menu3->addAction("Copy boring way");
    menu3->addAction("Copy awesomely");
    menu3->addAction("Paste");

    window->menuBar()->addMenu(menu3);
    window->show();
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    app.setApplicationName("Banana");

    createWindow1();
    createWindow2();

    return app.exec();
}

#include "main.moc"
