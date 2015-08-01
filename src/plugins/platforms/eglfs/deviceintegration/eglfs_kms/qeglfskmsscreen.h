/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QEGLFSKMSSCREEN_H
#define QEGLFSKMSSCREEN_H

#include "qeglfskmsintegration.h"
#include "qeglfsscreen.h"
#include <QtCore/QList>
#include <QtCore/QMutex>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;
class QEglFSKmsCursor;
class QEglFSKmsInterruptHandler;

struct QEglFSKmsOutput
{
    QString name;
    uint32_t connector_id;
    uint32_t crtc_id;
    QSizeF physical_size;
    int mode; // index of selected mode in list below
    bool mode_set;
    drmModeCrtcPtr saved_crtc;
    QList<drmModeModeInfo> modes;
    int subpixel;
    drmModePropertyPtr dpms_prop;
};

class QEglFSKmsScreen : public QEglFSScreen
{
public:
    QEglFSKmsScreen(QEglFSKmsIntegration *integration,
                    QEglFSKmsDevice *device,
                    QEglFSKmsOutput output,
                    QPoint position);
    ~QEglFSKmsScreen();

    QRect geometry() const Q_DECL_OVERRIDE;
    int depth() const Q_DECL_OVERRIDE;
    QImage::Format format() const Q_DECL_OVERRIDE;

    QSizeF physicalSize() const Q_DECL_OVERRIDE;
    QDpi logicalDpi() const Q_DECL_OVERRIDE;
    Qt::ScreenOrientation nativeOrientation() const Q_DECL_OVERRIDE;
    Qt::ScreenOrientation orientation() const Q_DECL_OVERRIDE;

    QString name() const Q_DECL_OVERRIDE;

    QPlatformCursor *cursor() const Q_DECL_OVERRIDE;

    qreal refreshRate() const Q_DECL_OVERRIDE;

    QList<QPlatformScreen *> virtualSiblings() const Q_DECL_OVERRIDE { return m_siblings; }
    void setVirtualSiblings(QList<QPlatformScreen *> sl) { m_siblings = sl; }

    QEglFSKmsIntegration *integration() const { return m_integration; }
    QEglFSKmsDevice *device() const { return m_device; }

    gbm_surface *surface() const { return m_gbm_surface; }
    gbm_surface *createSurface();
    void destroySurface();

    void waitForFlip();
    void flip();
    void flipFinished();

    QEglFSKmsOutput &output() { return m_output; }
    void restoreMode();

    SubpixelAntialiasingType subpixelAntialiasingTypeHint() const Q_DECL_OVERRIDE;

    QPlatformScreen::PowerState powerState() const Q_DECL_OVERRIDE;
    void setPowerState(QPlatformScreen::PowerState state) Q_DECL_OVERRIDE;

private:
    QEglFSKmsIntegration *m_integration;
    QEglFSKmsDevice *m_device;
    gbm_surface *m_gbm_surface;

    gbm_bo *m_gbm_bo_current;
    gbm_bo *m_gbm_bo_next;

    QEglFSKmsOutput m_output;
    QPoint m_pos;
    QScopedPointer<QEglFSKmsCursor> m_cursor;

    QList<QPlatformScreen *> m_siblings;

    PowerState m_powerState;

    struct FrameBuffer {
        FrameBuffer() : fb(0) {}
        uint32_t fb;
    };
    static void bufferDestroyedHandler(gbm_bo *bo, void *data);
    FrameBuffer *framebufferForBufferObject(gbm_bo *bo);

    static QMutex m_waitForFlipMutex;

    QEglFSKmsInterruptHandler *m_interruptHandler;
};

QT_END_NAMESPACE

#endif
