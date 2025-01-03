// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLFSKMSEGLDEVICESCREEN_H
#define QEGLFSKMSEGLDEVICESCREEN_H

#include <private/qeglfskmsscreen_p.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsEglDeviceScreen : public QEglFSKmsScreen
{
public:
    QEglFSKmsEglDeviceScreen(QEglFSKmsDevice *device, const QKmsOutput &output);
    ~QEglFSKmsEglDeviceScreen();

    QPlatformCursor *cursor() const override;

    void waitForFlip() override;
private:
    uint32_t m_default_fb_handle;
    uint32_t m_default_fb_id;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSEGLDEVICESCREEN_H
