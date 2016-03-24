/****************************************************************************
**
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

#include "qeglfskmsegldevicescreen.h"
#include "qeglfskmsegldevice.h"

QEglFSKmsEglDeviceScreen::QEglFSKmsEglDeviceScreen(QEglFSKmsIntegration *integration, QEglFSKmsDevice *device, QEglFSKmsOutput output, QPoint position)
    : QEglFSKmsScreen(integration, device, output, position)
{
}

void QEglFSKmsEglDeviceScreen::waitForFlip()
{
    if (!output().mode_set) {
        output().mode_set = true;

        drmModeCrtcPtr currentMode = drmModeGetCrtc(device()->fd(), output().crtc_id);
        const bool alreadySet = currentMode
            && currentMode->width == output().modes[output().mode].hdisplay
            && currentMode->height == output().modes[output().mode].vdisplay;
        if (currentMode)
            drmModeFreeCrtc(currentMode);
        if (alreadySet) {
            // Maybe detecting the DPMS mode could help here, but there are no properties
            // exposed on the connector apparently. So rely on an env var for now.
            static bool alwaysDoSet = qEnvironmentVariableIntValue("QT_QPA_EGLFS_ALWAYS_SET_MODE");
            if (!alwaysDoSet) {
                qCDebug(qLcEglfsKmsDebug, "Mode already set");
                return;
            }
        }

        qCDebug(qLcEglfsKmsDebug, "Setting mode");
        int ret = drmModeSetCrtc(device()->fd(), output().crtc_id,
                                 -1, 0, 0,
                                 &output().connector_id, 1,
                                 &output().modes[output().mode]);
        if (ret)
            qFatal("drmModeSetCrtc failed");
    }

}
