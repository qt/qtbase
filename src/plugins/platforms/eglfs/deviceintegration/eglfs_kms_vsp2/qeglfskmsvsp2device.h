// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLFSKMSVSP2DEVICE_H
#define QEGLFSKMSVSP2DEVICE_H

#include <qeglfskmsdevice_p.h>

#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsScreen;

class QEglFSKmsVsp2Device: public QEglFSKmsDevice
{
public:
    QEglFSKmsVsp2Device(QKmsScreenConfig *screenConfig, const QString &path);

    bool open() override;
    void close() override;

    void *nativeDisplay() const override;
    gbm_device *gbmDevice() const;

    QPlatformScreen *createScreen(const QKmsOutput &output) override;
    QPlatformScreen *createHeadlessScreen() override;
    void registerScreenCloning(QPlatformScreen *screen,
                               QPlatformScreen *screenThisScreenClones,
                               const QList<QPlatformScreen *> &screensCloningThisScreen) override;

private:
    Q_DISABLE_COPY(QEglFSKmsVsp2Device)

    gbm_device *m_gbm_device = nullptr;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSVSP2DEVICE_H
