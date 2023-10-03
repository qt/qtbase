/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QEGLFSKMSGBMSCREEN_H
#define QEGLFSKMSGBMSCREEN_H

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

#include <private/qeglfskmsscreen_p.h>
#include <QMutex>
#include <QWaitCondition>

#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsGbmCursor;

class Q_EGLFS_EXPORT QEglFSKmsGbmScreen : public QEglFSKmsScreen
{
public:
    QEglFSKmsGbmScreen(QEglFSKmsDevice *device, const QKmsOutput &output, bool headless);
    ~QEglFSKmsGbmScreen();

    QPlatformCursor *cursor() const override;

    gbm_surface *createSurface(EGLConfig eglConfig);
    void resetSurface();

    void initCloning(QPlatformScreen *screenThisScreenClones,
                     const QList<QPlatformScreen *> &screensCloningThisScreen);

    void waitForFlip() override;

    virtual void flip();
    virtual void updateFlipStatus();

    virtual uint32_t gbmFlags() { return GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING; }

protected:
    void flipFinished();
    void ensureModeSet(uint32_t fb);
    void cloneDestFlipFinished(QEglFSKmsGbmScreen *cloneDestScreen);
    void waitForFlipWithEventReader(QEglFSKmsGbmScreen *screen);
    static void nonThreadedPageFlipHandler(int fd,
                                           unsigned int sequence,
                                           unsigned int tv_sec,
                                           unsigned int tv_usec,
                                           void *user_data);

    gbm_surface *m_gbm_surface;

    gbm_bo *m_gbm_bo_current;
    gbm_bo *m_gbm_bo_next;
    bool m_flipPending;

    QMutex m_flipMutex;
    QWaitCondition m_flipCond;
    static QMutex m_nonThreadedFlipMutex;

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
    QList<CloneDestination> m_cloneDests;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMSCREEN_H
