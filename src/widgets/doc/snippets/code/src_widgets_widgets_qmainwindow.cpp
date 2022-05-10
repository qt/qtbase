// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//! [0]
    void MainWindow::createMenus()
    {
        fileMenu = menuBar()->addMenu(tr("&File"));
        fileMenu->addAction(newAct);
        fileMenu->addAction(openAct);
        fileMenu->addAction(saveAct);
//! [0]

//! [1]
    void MainWindow::createToolBars()
    {
        fileToolBar = addToolBar(tr("File"));
        fileToolBar->addAction(newAct);
//! [1]

//! [2]
    resizeDocks({blueWidget, yellowWidget}, {20 , 40}, Qt::Horizontal);
//! [2]
