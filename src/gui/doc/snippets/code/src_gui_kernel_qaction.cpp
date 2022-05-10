// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QApplication app(argc, argv);
app.setAttribute(Qt::AA_DontShowIconsInMenus);  // Icons are *no longer shown* in menus
// ...
QAction *myAction = new QAction();
// ...
myAction->setIcon(SomeIcon);
myAction->setIconVisibleInMenu(true);   // Icon *will* be shown in menus for *this* action.
//! [0]
