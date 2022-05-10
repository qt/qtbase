// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qeglfsdeviceintegration_p.h"
#include "qeglfskmsvsp2integration.h"

QT_BEGIN_NAMESPACE

class QEglFSKmsVsp2IntegrationPlugin : public QEglFSDeviceIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QEglFSDeviceIntegrationFactoryInterface_iid FILE "eglfs_kms_vsp2.json")

public:
    QEglFSDeviceIntegration *create() override { return new QEglFSKmsVsp2Integration; }
};

QT_END_NAMESPACE

#include "qeglfskmsvsp2main.moc"
