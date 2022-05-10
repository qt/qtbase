// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QPixmap pixmap(":/splash.png");
QSplashScreen *splash = new QSplashScreen(pixmap);
splash->show();

... // Loading some items
splash->showMessage("Loaded modules");

QCoreApplication::processEvents();

... // Establishing connections
splash->showMessage("Established connections");

QCoreApplication::processEvents();
//! [0]
