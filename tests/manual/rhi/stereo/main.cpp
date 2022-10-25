// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// This is a demo showing stereoscopic rendering.
// For now, the backend is hardcoded to be OpenGL, because that's the only
// supported backend.

#include <QGuiApplication>
#include <QCommandLineParser>
#include "window.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);

    // Request stereoscopic rendering support with left/right buffers
    fmt.setStereo(true);

    QSurfaceFormat::setDefaultFormat(fmt);

    Window w;
    w.resize(1280, 720);
    w.setTitle(QCoreApplication::applicationName());
    w.show();

    int ret = app.exec();

    if (w.handle())
        w.releaseSwapChain();

    return ret;
}
