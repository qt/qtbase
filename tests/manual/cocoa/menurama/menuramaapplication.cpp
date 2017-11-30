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

#include "menuramaapplication.h"

MenuramaApplication::MenuramaApplication(int &argc, char **argv)
    : QApplication (argc, argv)
{
#if 0
    QMenuBar *mb = new QMenuBar();
    QMenu *menu = mb->addMenu("App Dynamic");
    QMenu *dynMenu = menu->addMenu("After aboutToShow()");
    connect(dynMenu, &QMenu::aboutToShow, [=] {
        qDebug() << "aboutToShow(), populating" << dynMenu;
        menuApp->populateMenu(dynMenu, true /*clear*/);
    });
#endif
}

void MenuramaApplication::populateMenu(QMenu *menu, bool clear)
{
    if (clear)
        menu->clear();

    static const char *sym[] = { "Foo", "Bar", "Baz", "Huux" };
    static int id = 0;
    for (unsigned i = 0; i < sizeof(sym) / sizeof(sym[0]); i++)
        menu->addAction(QStringLiteral("%1 — %2 %3 ")
                        .arg(menu->title()).arg(sym[i]).arg(id));
    ++id;
}

void MenuramaApplication::addDynMenu(QLatin1String title, QMenu *parentMenu)
{
    if (QAction *a = findAction(title, parentMenu))
        parentMenu->removeAction(a);

    QMenu *subMenu = new QMenu(title, parentMenu);
    populateMenu(subMenu, false /*clear*/);
    parentMenu->addMenu(subMenu);
}

QAction *MenuramaApplication::findAction(QLatin1String title, QMenu *parentMenu)
{
    foreach (QAction *a, parentMenu->actions())
        if (a->text() == title)
            return a;

    return nullptr;
}
