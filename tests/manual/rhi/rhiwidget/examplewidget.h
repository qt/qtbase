/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef EXAMPLEWIDGET_H
#define EXAMPLEWIDGET_H

#include "rhiwidget.h"
#include <QtGui/private/qrhi_p.h>

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
