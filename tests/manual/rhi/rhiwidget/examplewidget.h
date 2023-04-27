// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef EXAMPLEWIDGET_H
#define EXAMPLEWIDGET_H

#include "rhiwidget.h"
#include <rhi/qrhi.h>

class ExampleRhiWidget : public QRhiWidget
{
public:
    ExampleRhiWidget(QWidget *parent = nullptr, Qt::WindowFlags f = {});

    void initialize(QRhi *rhi, QRhiTexture *outputTexture) override;
    void render(QRhiCommandBuffer *cb) override;

    void setCubeTextureText(const QString &s)
    {
        if (itemData.cubeText == s)
            return;
        itemData.cubeText = s;
        itemData.cubeTextDirty = true;
        update();
    }

    void setCubeRotation(float r)
    {
        if (itemData.cubeRotation == r)
            return;
        itemData.cubeRotation = r;
        itemData.cubeRotationDirty = true;
        update();
    }

private:
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_output = nullptr;
    QScopedPointer<QRhiRenderBuffer> m_ds;
    QScopedPointer<QRhiTextureRenderTarget> m_rt;
    QScopedPointer<QRhiRenderPassDescriptor> m_rp;

    struct {
        QRhiResourceUpdateBatch *resourceUpdates = nullptr;
        QScopedPointer<QRhiBuffer> vbuf;
        QScopedPointer<QRhiBuffer> ubuf;
        QScopedPointer<QRhiShaderResourceBindings> srb;
        QScopedPointer<QRhiGraphicsPipeline> ps;
        QScopedPointer<QRhiSampler> sampler;
        QScopedPointer<QRhiTexture> cubeTex;
        QMatrix4x4 mvp;
    } scene;

    void initScene();
    void updateMvp();
    void updateCubeTexture();

    struct {
        QString cubeText;
        bool cubeTextDirty = false;
        float cubeRotation = 0.0f;
        bool cubeRotationDirty = false;
    } itemData;
};

#endif
