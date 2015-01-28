/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#include "qeglfshooks.h"
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglDevDebug)

#ifdef EGLFS_PLATFORM_HOOKS

QEGLDeviceIntegration *qt_egl_device_integration()
{
    extern QEglFSHooks *platformHooks;
    return platformHooks;
}

#else

class DeviceIntegration
{
public:
    DeviceIntegration();
    ~DeviceIntegration() { delete m_integration; }
    QEGLDeviceIntegration *integration() { return m_integration; }
private:
    QEGLDeviceIntegration *m_integration;
};

Q_GLOBAL_STATIC(DeviceIntegration, deviceIntegration)

DeviceIntegration::DeviceIntegration()
{
    QStringList pluginKeys = QEGLDeviceIntegrationFactory::keys();
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
                m_integration = QEGLDeviceIntegrationFactory::create(key);
            }
        }
    }

    if (!m_integration) {
        // Use a default, non-specialized device integration when no plugin is available.
        // For some systems this is sufficient.
        qCDebug(qLcEglDevDebug) << "Using base device integration";
        m_integration = new QEGLDeviceIntegration;
    }
}

QEGLDeviceIntegration *qt_egl_device_integration()
{
    return deviceIntegration()->integration();
}

#endif // EGLFS_PLATFORM_HOOKS

QT_END_NAMESPACE
