/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QEGLFSKMSGBMSCREEN_H
#define QEGLFSKMSGBMSCREEN_H

#include "qeglfskmsscreen.h"
#include <QMutex>
#include <QWaitCondition>

#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsGbmCursor;

class QEglFSKmsGbmScreen : public QEglFSKmsScreen
{
public:
    QEglFSKmsGbmScreen(QEglFSKmsDevice *device, const QKmsOutput &output, bool headless);
    ~QEglFSKmsGbmScreen();

    QPlatformCursor *cursor() const override;

    gbm_surface *createSurface(EGLConfig eglConfig);
    void resetSurface();

    void initCloning(QPlatformScreen *screenThisScreenClones,
                     const QVector<QPlatformScreen *> &screensCloningThisScreen);

    void waitForFlip() override;

    void flip();

private:
    void flipFinished();
    void ensureModeSet(uint32_t fb);
    void cloneDestFlipFinished(QEglFSKmsGbmScreen *cloneDestScreen);
    void updateFlipStatus();

    gbm_surface *m_gbm_surface;

    gbm_bo *m_gbm_bo_current;
    gbm_bo *m_gbm_bo_next;
    bool m_flipPending;

    QMutex m_flipMutex;
    QWaitCondition m_flipCond;

    QScopedPointer<QEglFSKmsGbmCursor> m_cursor;

    struct FrameBuffer {
        uint32_t fb = 0;
    };
    static void bufferDestroyedHandler(gbm_bo *bo, void *data);
    FrameBuffer *framebufferForBufferObject(gbm_bo *bo);

    QEglFSKmsGbmScreen *m_cloneSource;
    struct CloneDestination {
        QEglFSKmsGbmScreen *screen = nullptr;
        bool cloneFlipPending = false;
    };
    QVector<CloneDestination> m_cloneDests;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMSCREEN_H
