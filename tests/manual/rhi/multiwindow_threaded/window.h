// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WINDOW_H
#define WINDOW_H

#include <QWindow>

enum GraphicsApi
{
    OpenGL,
    Vulkan,
    D3D11,
    D3D12,
    Metal
};

class Window : public QWindow
{
    Q_OBJECT

public:
    Window(const QString &title, GraphicsApi api);
    ~Window();

    void exposeEvent(QExposeEvent *) override;
    bool event(QEvent *) override;

signals:
    void initRequested();
    void renderRequested(bool newlyExposed);
    void surfaceGoingAway();
    void syncSurfaceSizeRequested();

protected:
    bool m_running = false;
    bool m_notExposed = true;
};

#endif
