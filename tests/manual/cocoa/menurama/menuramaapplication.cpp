// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

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
