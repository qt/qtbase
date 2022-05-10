// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QGuiApplication>
#include <QRasterWindow>
#include <QScreen>
#include <QTimer>

// Simple test application just to verify that it comes up properly

int main(int argc, char ** argv)
{
   QGuiApplication app(argc, argv);
   QRasterWindow w;
   w.setTitle("windeployqt test application");
   const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
   w.resize(availableGeometry.size() / 4);
   w.show();
   QTimer::singleShot(200, &w, &QCoreApplication::quit);
   return app.exec();
}
