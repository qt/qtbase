/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "propertywatcher.h"
#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QDebug>
#include <QFormLayout>
#include <QLineEdit>

int i = 0;

typedef QHash<QScreen*, PropertyWatcher*> ScreensHash;
Q_GLOBAL_STATIC(ScreensHash, props);

void updateSiblings(PropertyWatcher* w)
{
    QLineEdit *siblingsField = w->findChild<QLineEdit *>("siblings");
    QScreen* screen = (QScreen*)w->subject();
    QStringList siblingsList;
    foreach (QScreen *sibling, screen->virtualSiblings())
        siblingsList << sibling->name();
    siblingsField->setText(siblingsList.join(", "));
}

void screenAdded(QScreen* screen)
{
    screen->setOrientationUpdateMask((Qt::ScreenOrientations)0x0F);
    qDebug("\nscreenAdded %s siblings %d first %s", qPrintable(screen->name()), screen->virtualSiblings().count(),
        (screen->virtualSiblings().isEmpty() ? "none" : qPrintable(screen->virtualSiblings().first()->name())));
    PropertyWatcher *w = new PropertyWatcher(screen, QString::number(i++));
    QLineEdit *siblingsField = new QLineEdit();
    siblingsField->setObjectName("siblings");
    siblingsField->setReadOnly(true);
    w->layout()->insertRow(0, "virtualSiblings", siblingsField);
    updateSiblings(w);

    // This doesn't work.  If the multiple screens are part of
    // a virtual desktop (i.e. they are virtual siblings), then
    // setScreen has no effect, and we need the code below to
    // change the window geometry.  If on the other hand the
    // screens are really separate, so that windows are not
    // portable between them, XCreateWindow needs to have not just
    // a different root Window but also a different Display, in order to
    // put the window on the other screen.  That would require a
    // different QXcbConnection.  So this setScreen call doesn't seem useful.
    //w->windowHandle()->setScreen(screen);

    // But this works as long as the screens are all virtual siblings
    w->show();
    QRect geom = w->geometry();
    geom.moveCenter(screen->geometry().center());
    w->move(geom.topLeft());

    props->insert(screen, w);

    // workaround for the fact that virtualSiblings is not a property,
    // thus there is no change notification:
    // allow the user to update the field manually
    QObject::connect(w, &PropertyWatcher::updatedAllFields, &updateSiblings);
}

void screenRemoved(QScreen* screen)
{
    delete props->take(screen);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QList<QScreen *> screens = QGuiApplication::screens();
    foreach (QScreen *screen, screens)
        screenAdded(screen);
    QObject::connect((const QGuiApplication*)QGuiApplication::instance(), &QGuiApplication::screenAdded, &screenAdded);
    QObject::connect((const QGuiApplication*)QGuiApplication::instance(), &QGuiApplication::screenRemoved, &screenRemoved);
    return a.exec();
}
