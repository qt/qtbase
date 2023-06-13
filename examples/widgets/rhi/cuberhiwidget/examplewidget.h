// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef EXAMPLEWIDGET_H
#define EXAMPLEWIDGET_H

#include <QRhiWidget>
#include <rhi/qrhi.h>

class ExampleRhiWidget : public QRhiWidget
{
    Q_OBJECT

public:
    ExampleRhiWidget(QWidget *parent = nullptr);

    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
    void releaseResources() override;
//![data-setters]
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
//![data-setters]

signals:
    void resized();
    void rhiChanged(const QString &apiName);

private:
    QRhi *m_rhi = nullptr;
    int m_sampleCount = 1;
    QSize m_pixelSize;

    struct {
        QRhiResourceUpdateBatch *resourceUpdates = nullptr;
        std::unique_ptr<QRhiBuffer> vbuf;
        std::unique_ptr<QRhiBuffer> ubuf;
        std::unique_ptr<QRhiShaderResourceBindings> srb;
        std::unique_ptr<QRhiGraphicsPipeline> ps;
        std::unique_ptr<QRhiSampler> sampler;
        std::unique_ptr<QRhiTexture> cubeTex;
        QMatrix4x4 mvp;
    } scene;

    struct {
        QString cubeText;
        bool cubeTextDirty = false;
        float cubeRotation = 0.0f;
        bool cubeRotationDirty = false;
    } itemData;

    void initScene();
    void updateMvp();
    void updateCubeTexture();
};

#endif
