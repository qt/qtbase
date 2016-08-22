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

#include "qeglfskmsscreen.h"
#include <QtCore/QMutex>

#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsGbmCursor;

class QEglFSKmsGbmScreen : public QEglFSKmsScreen
{
public:
    QEglFSKmsGbmScreen(QEglFSKmsIntegration *integration,
                       QEglFSKmsDevice *device,
                       QEglFSKmsOutput output);
    ~QEglFSKmsGbmScreen();

    QPlatformCursor *cursor() const Q_DECL_OVERRIDE;

    gbm_surface *surface() const { return m_gbm_surface; }
    gbm_surface *createSurface();
    void destroySurface();

    void waitForFlip() Q_DECL_OVERRIDE;
    void flip() Q_DECL_OVERRIDE;
    void flipFinished() Q_DECL_OVERRIDE;

private:
    gbm_surface *m_gbm_surface;

    gbm_bo *m_gbm_bo_current;
    gbm_bo *m_gbm_bo_next;

    QScopedPointer<QEglFSKmsGbmCursor> m_cursor;

    struct FrameBuffer {
        FrameBuffer() : fb(0) {}
        uint32_t fb;
    };
    static void bufferDestroyedHandler(gbm_bo *bo, void *data);
    FrameBuffer *framebufferForBufferObject(gbm_bo *bo);

    static QMutex m_waitForFlipMutex;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMSCREEN_H
