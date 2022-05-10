// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MENURAMAAPPLICATION_H
#define MENURAMAAPPLICATION_H

#include <QtWidgets>

#define menuApp (static_cast<MenuramaApplication *>(QCoreApplication::instance()))

class MenuramaApplication : public QApplication
{
public:
    MenuramaApplication(int &argc, char **argv);
    void addDynMenu(QLatin1String title, QMenu *parentMenu);
    QAction *findAction(QLatin1String title, QMenu *parentMenu);

public slots:
    void populateMenu(QMenu *menu, bool clear);
};

#endif // MENURAMAAPPLICATION_H
