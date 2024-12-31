// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfshooks_p.h"
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

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

        // The environment variable can override everything.
        QString requested = qEnvironmentVariable("QT_QPA_EGLFS_INTEGRATION");
        if (requested.isNull()) {
            // Device-specific makespecs may define a preferred plugin.
#ifdef EGLFS_PREFERRED_PLUGIN
#define DEFAULT_PLUGIN EGLFS_PREFERRED_PLUGIN
#define STR(s) #s
#define STRQ(s) STR(s)
            requested = QStringLiteral(STRQ(DEFAULT_PLUGIN));
#endif
        }

        // Treat "none" as special. There has to be a way to indicate
        // that plugins must be ignored when the device is known to be
        // functional with the default, non-specialized integration.
        if (requested != QStringLiteral("none")) {
            if (!requested.isEmpty()) {
                pluginKeys.removeOne(requested);
                pluginKeys.prepend(requested);
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
