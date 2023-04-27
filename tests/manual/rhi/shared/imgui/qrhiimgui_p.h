// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QRHIIMGUI_P_H
#define QRHIIMGUI_P_H

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

class QEvent;

class QRhiImguiRenderer
{
public:
    ~QRhiImguiRenderer();

    struct CmdListBuffer {
        quint32 offset;
        QByteArray data;
    };

    struct DrawCmd {
        int cmdListBufferIdx;
        int textureIndex;
        quint32 indexOffset;
        quint32 elemCount;
        QPointF itemPixelOffset;
        QVector4D clipRect;
    };

    struct StaticRenderData {
        QImage fontTextureData;
    };

    struct FrameRenderData {
        quint32 totalVbufSize = 0;
        quint32 totalIbufSize = 0;
        QVarLengthArray<CmdListBuffer, 4> vbuf;
        QVarLengthArray<CmdListBuffer, 4> ibuf;
        QVarLengthArray<DrawCmd, 4> draw;
        QSize outputPixelSize;
    };

    StaticRenderData sf;
    FrameRenderData f;

    void prepare(QRhi *rhi, QRhiRenderTarget *rt, QRhiCommandBuffer *cb, const QMatrix4x4 &mvp, float opacity);
    void render();
    void releaseResources();

private:
    QRhi *m_rhi = nullptr;
    QRhiRenderTarget *m_rt = nullptr;
    QRhiCommandBuffer *m_cb = nullptr;

    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ibuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiGraphicsPipeline> m_ps;
    QVector<quint32> m_renderPassFormat;
    std::unique_ptr<QRhiSampler> m_sampler;

    struct Texture {
        QImage image;
        QRhiTexture *tex = nullptr;
        QRhiShaderResourceBindings *srb = nullptr;
    };
    QVector<Texture> m_textures;
};

class QRhiImgui
{
public:
    QRhiImgui();
    ~QRhiImgui();

    using FrameFunc = std::function<void()>;
    void nextFrame(const QSizeF &logicalOutputSize, float dpr, const QPointF &logicalOffset, FrameFunc frameFunc);
    void syncRenderer(QRhiImguiRenderer *renderer);
    bool processEvent(QEvent *e);

    void rebuildFontAtlas();

private:
    QRhiImguiRenderer::StaticRenderData sf;
    QRhiImguiRenderer::FrameRenderData f;
    Qt::MouseButtons pressedMouseButtons;
};

QT_END_NAMESPACE

#endif
