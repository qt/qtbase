// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QSurfaceFormat>
#include "mainwindow.h"

int main( int argc, char ** argv )
{
    QApplication a( argc, argv );

    QCoreApplication::setApplicationName("Qt QOpenGLWidget Stereoscopic Rendering Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    //! [1]
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);

    // Enable stereoscopic rendering support
    format.setStereo(true);

    QSurfaceFormat::setDefaultFormat(format);
    //! [1]

    MainWindow mw;
    mw.resize(1280, 720);
    mw.show();
    return a.exec();
}
