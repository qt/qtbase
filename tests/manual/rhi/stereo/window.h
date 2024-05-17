// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WINDOW_H
#define WINDOW_H

#include <QWindow>
#include <QOffscreenSurface>
#include <rhi/qrhi.h>

class Window : public QWindow
{
public:
    Window(QRhi::Implementation graphicsApi);

    void releaseSwapChain();

protected:
    QVulkanInstance instance;
    std::unique_ptr<QOffscreenSurface> m_fallbackSurface;
    std::unique_ptr<QRhi> m_rhi;
    std::unique_ptr<QRhiSwapChain> m_sc;
    std::unique_ptr<QRhiRenderBuffer> m_ds;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rp;

    QRhi::Implementation m_graphicsApi;
    bool m_hasSwapChain = false;
    QMatrix4x4 m_proj;

private:
    void init();
    void resizeSwapChain();
    void render();
    void recordFrame();

    void exposeEvent(QExposeEvent *) override;
    bool event(QEvent *) override;

    bool m_running = false;
    bool m_notExposed = false;
    bool m_newlyExposed = false;

    QShader getShader(const QString &name);

    std::unique_ptr<QRhiBuffer> m_vbuf;
    bool m_vbufReady = false;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiBuffer> m_ubuf2;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb2;
    std::unique_ptr<QRhiGraphicsPipeline> m_ps;

    float m_rotation = 0;
    float m_translation = -5;
    int m_translationDir = -1;
};

#endif
