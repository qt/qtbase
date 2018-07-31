/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#include "qeglfshooks_p.h"
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglDevDebug)

#ifdef EGLFS_PLATFORM_HOOKS

QEglFSDeviceIntegration *qt_egl_device_integration()
{
    extern QEglFSHooks *platformHooks;
    return platformHooks;
}

#else

namespace {
class DeviceIntegration
{
public:
    DeviceIntegration();
    ~DeviceIntegration() { delete m_integration; }
    QEglFSDeviceIntegration *integration() { return m_integration; }
private:
    QEglFSDeviceIntegration *m_integration;
};
}

Q_GLOBAL_STATIC(DeviceIntegration, deviceIntegration)

DeviceIntegration::DeviceIntegration()
    : m_integration(nullptr)
{
    QStringList pluginKeys = QEglFSDeviceIntegrationFactory::keys();
    if (!pluginKeys.isEmpty()) {
        // Some built-in logic: Prioritize either X11 or KMS/DRM.
        if (qEnvironmentVariableIsSet("DISPLAY")) {
            const QString x11key = QStringLiteral("eglfs_x11");
            if (pluginKeys.contains(x11key)) {
                pluginKeys.removeOne(x11key);
                pluginKeys.prepend(x11key);
            }
        } else {
            const QString kmskey = QStringLiteral("eglfs_kms");
            if (pluginKeys.contains(kmskey)) {
                pluginKeys.removeOne(kmskey);
                pluginKeys.prepend(kmskey);
            }
        }

        QByteArray requested;

        // The environment variable can override everything.
        if (qEnvironmentVariableIsSet("QT_QPA_EGLFS_INTEGRATION")) {
            requested = qgetenv("QT_QPA_EGLFS_INTEGRATION");
        } else {
            // Device-specific makespecs may define a preferred plugin.
#ifdef EGLFS_PREFERRED_PLUGIN
#define DEFAULT_PLUGIN EGLFS_PREFERRED_PLUGIN
#define STR(s) #s
#define STRQ(s) STR(s)
            requested = STRQ(DEFAULT_PLUGIN);
#endif
        }

        // Treat "none" as special. There has to be a way to indicate
        // that plugins must be ignored when the device is known to be
        // functional with the default, non-specialized integration.
        if (requested != QByteArrayLiteral("none")) {
            if (!requested.isEmpty()) {
                QString reqStr = QString::fromLocal8Bit(requested);
                pluginKeys.removeOne(reqStr);
                pluginKeys.prepend(reqStr);
            }
            qCDebug(qLcEglDevDebug) << "EGL device integration plugin keys (sorted):" << pluginKeys;
            while (!m_integration && !pluginKeys.isEmpty()) {
                QString key = pluginKeys.takeFirst();
                qCDebug(qLcEglDevDebug) << "Trying to load device EGL integration" << key;
                m_integration = QEglFSDeviceIntegrationFactory::create(key);
            }
        }
    }

    if (!m_integration) {
        // Use a default, non-specialized device integration when no plugin is available.
        // For some systems this is sufficient.
        qCDebug(qLcEglDevDebug) << "Using base device integration";
        m_integration = new QEglFSDeviceIntegration;
    }
}

QEglFSDeviceIntegration *qt_egl_device_integration()
{
    return deviceIntegration()->integration();
}

#endif // EGLFS_PLATFORM_HOOKS

QT_END_NAMESPACE
