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

#ifndef QEGLFSKMSSCREEN_H
#define QEGLFSKMSSCREEN_H

#include "qeglfskmsintegration.h"
#include "qeglfsscreen.h"
#include <QtCore/QList>
#include <QtCore/QMutex>

#include <xf86drm.h>
#include <xf86drmMode.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;
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

class Q_EGLFS_EXPORT QEglFSKmsScreen : public QEglFSScreen
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

    qreal refreshRate() const Q_DECL_OVERRIDE;

    QList<QPlatformScreen *> virtualSiblings() const Q_DECL_OVERRIDE { return m_siblings; }
    void setVirtualSiblings(QList<QPlatformScreen *> sl) { m_siblings = sl; }

    QEglFSKmsIntegration *integration() const { return m_integration; }
    QEglFSKmsDevice *device() const { return m_device; }

    void destroySurface();

    virtual void waitForFlip();
    virtual void flip();
    virtual void flipFinished();

    QEglFSKmsOutput &output() { return m_output; }
    void restoreMode();

    SubpixelAntialiasingType subpixelAntialiasingTypeHint() const Q_DECL_OVERRIDE;

    QPlatformScreen::PowerState powerState() const Q_DECL_OVERRIDE;
    void setPowerState(QPlatformScreen::PowerState state) Q_DECL_OVERRIDE;

protected:
    QEglFSKmsIntegration *m_integration;
    QEglFSKmsDevice *m_device;

    QEglFSKmsOutput m_output;
    QPoint m_pos;

    QList<QPlatformScreen *> m_siblings;

    PowerState m_powerState;

    QEglFSKmsInterruptHandler *m_interruptHandler;
};

QT_END_NAMESPACE

#endif
