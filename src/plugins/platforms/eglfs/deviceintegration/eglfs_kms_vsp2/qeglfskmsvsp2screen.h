/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QEGLFSKMSVSP2SCREEN_H
#define QEGLFSKMSVSP2SCREEN_H

#include "qeglfskmsscreen.h"
#include "qvsp2blendingdevice.h"
#include <QtCore/QMutex>

#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsVsp2Screen : public QEglFSKmsScreen
{
public:
    QEglFSKmsVsp2Screen(QEglFSKmsDevice *device, const QKmsOutput &output);

    gbm_surface *createSurface();
    void resetSurface();

    void initDumbFrameBuffers();
    void initVsp2();
    void initQtLayer();

    //TODO: use a fixed index API instead of auto increment?
    int addLayer(int dmabufFd, const QSize &size, const QPoint &position, uint drmPixelFormat, uint bytesPerLine);
    void setLayerBuffer(int id, int dmabufFd);
    void setLayerPosition(int id, const QPoint &position);
    void setLayerAlpha(int id, qreal alpha);
    bool removeLayer(int id);
    void addBlendListener(void (*callback)());

    void flip();
    void blendAndFlipDrm();

private:
    class Blender : public QObject // This is a workaround we really would want the screen to be a QObject
    {
    public:
        Blender(QEglFSKmsVsp2Screen *screen) : m_screen(screen) {}
        ~Blender() override {}
        bool event(QEvent *event) override;
        QEglFSKmsVsp2Screen *m_screen = nullptr;
    };
    QScopedArrayPointer<Blender> m_blender;

    gbm_surface *m_gbmSurface = nullptr;
    gbm_bo *m_currentGbmBo = nullptr;
    gbm_bo *m_nextGbmBo = nullptr;

    QScopedPointer<QVsp2BlendingDevice> m_blendDevice;

    struct FrameBuffer { //these are for buffers that have been blended by the bru
        uint32_t drmBufferId = 0;
        int dmabufFd = -1;
    };
    std::array<FrameBuffer, 2> m_frameBuffers;
    uint m_backFb = 0;
    void initDumbFrameBuffer(FrameBuffer &fb);
    QVector<void (*)()> m_blendFinishedCallbacks;

    struct DmaBuffer { //these are for qt buffers before blending with additional layers (gbm buffer data)
        int dmabufFd = -1;
    };
    static void dmaBufferDestroyedHandler(gbm_bo *gbmBo, void *data);
    DmaBuffer *dmaBufferForGbmBuffer(gbm_bo *gbmBo);

    void ensureModeSet();
    void doDrmFlip();

    bool m_blendScheduled = false;
    int m_qtLayer = 0; //TODO: add API for changing this
};

QT_END_NAMESPACE

#endif // QEGLFSKMSVSP2SCREEN_H
