// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandhardwareintegration_p.h"

#include "qwaylanddisplay_p.h"
QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandHardwareIntegration::QWaylandHardwareIntegration(struct ::wl_registry *registry, int id)
    : qt_hardware_integration(registry, id, 1)
{
}

QString QWaylandHardwareIntegration::clientBufferIntegration()
{
    return m_client_buffer;
}

QString QWaylandHardwareIntegration::serverBufferIntegration()
{
    return m_server_buffer;
}

void QWaylandHardwareIntegration::hardware_integration_client_backend(const QString &name)
{
    m_client_buffer = name;
}

void QWaylandHardwareIntegration::hardware_integration_server_backend(const QString &name)
{
    m_server_buffer = name;
}

}

QT_END_NAMESPACE
