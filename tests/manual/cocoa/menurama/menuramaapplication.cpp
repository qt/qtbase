/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the qtbase module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

#include "menuramaapplication.h"

MenuramaApplication::MenuramaApplication(int argc, char **argv)
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

    return Q_NULLPTR;
}
