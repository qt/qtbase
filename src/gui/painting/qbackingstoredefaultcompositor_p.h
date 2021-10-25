/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QBACKINGSTOREDEFAULTCOMPOSITOR_P_H
#define QBACKINGSTOREDEFAULTCOMPOSITOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformbackingstore.h>
#include <QtGui/private/qrhi_p.h>

QT_BEGIN_NAMESPACE

class QBackingStoreDefaultCompositor
{
public:
    ~QBackingStoreDefaultCompositor();

    void reset();

    QRhiTexture *toTexture(const QPlatformBackingStore *backingStore,
                           QRhi *rhi,
                           QRhiResourceUpdateBatch *resourceUpdates,
                           const QRegion &dirtyRegion,
                           QPlatformBackingStore::TextureFlags *flags) const;

    QPlatformBackingStore::FlushResult flush(QPlatformBackingStore *backingStore,
                                             QRhi *rhi,
                                             QRhiSwapChain *swapchain,
                                             QWindow *window,
                                             const QRegion &region,
                                             const QPoint &offset,
                                             QPlatformTextureList *textures,
                                             bool translucentBackground);

private:
    void ensureResources(QRhiSwapChain *swapchain, QRhiResourceUpdateBatch *resourceUpdates);
    QRhiTexture *toTexture(const QImage &image,
                           QRhi *rhi,
                           QRhiResourceUpdateBatch *resourceUpdates,
                           const QRegion &dirtyRegion,
                           QPlatformBackingStore::TextureFlags *flags) const;

    mutable QRhi *m_rhi = nullptr;
    mutable QRhiTexture *m_texture = nullptr;

    QRhiBuffer *m_vbuf = nullptr;
    QRhiSampler *m_sampler = nullptr;
    QRhiGraphicsPipeline *m_psNoBlend = nullptr;
    QRhiGraphicsPipeline *m_psBlend = nullptr;
    QRhiGraphicsPipeline *m_psPremulBlend = nullptr;

    struct PerQuadData {
        QRhiBuffer *ubuf = nullptr;
        // All srbs are layout-compatible.
        QRhiShaderResourceBindings *srb = nullptr;
        QRhiTexture *lastUsedTexture = nullptr;
        bool isValid() const { return ubuf && srb; }
        void reset() {
            delete ubuf;
            ubuf = nullptr;
            delete srb;
            srb = nullptr;
            lastUsedTexture = nullptr;
        }
    };
    PerQuadData m_widgetQuadData;
    QVarLengthArray<PerQuadData, 8> m_textureQuadData;

    PerQuadData createPerQuadData(QRhiTexture *texture);
    void updatePerQuadData(PerQuadData *d, QRhiTexture *texture);
    void updateUniforms(PerQuadData *d, QRhiResourceUpdateBatch *resourceUpdates,
                        const QMatrix4x4 &target, const QMatrix3x3 &source, bool needsRedBlueSwap);
};

QT_END_NAMESPACE

#endif // QBACKINGSTOREDEFAULTCOMPOSITOR_P_H
