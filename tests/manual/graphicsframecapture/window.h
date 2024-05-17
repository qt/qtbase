// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WINDOW_H
#define WINDOW_H

#include <QWindow>
#include <QOffscreenSurface>
#include <rhi/qrhi.h>
#include <QtGui/private/qgraphicsframecapture_p.h>

class Window : public QWindow
{
public:
    Window(QRhi::Implementation graphicsApi);

    void releaseSwapChain();

protected:
    virtual void customInit();
    virtual void customRender();

    // destruction order matters to a certain degree: the fallbackSurface must
    // outlive the rhi, the rhi must outlive all other resources.  The resources
    // need no special order when destroying.
#if QT_CONFIG(opengl)
    std::unique_ptr<QOffscreenSurface> m_fallbackSurface;
#endif
    std::unique_ptr<QRhi> m_rhi;
    std::unique_ptr<QRhiSwapChain> m_sc;
    std::unique_ptr<QRhiRenderBuffer> m_ds;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rp;

    bool m_hasSwapChain = false;
    QMatrix4x4 m_proj;

private:
    void init();
    void resizeSwapChain();
    void render();

    void exposeEvent(QExposeEvent *) override;
    bool event(QEvent *) override;

    QRhi::Implementation m_graphicsApi;

    bool m_running = false;
    bool m_notExposed = false;
    bool m_newlyExposed = false;

    QScopedPointer<QGraphicsFrameCapture> m_capturer;
    bool m_shouldCapture = false;
};

#endif
