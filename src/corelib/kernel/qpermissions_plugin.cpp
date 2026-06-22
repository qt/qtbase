// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpermissions.h"
#include "qpermissions_p.h"

#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/qcborarray.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, pluginLoader,
    (QPermissionPluginInterface_iid, QLatin1String("/permissions"), Qt::CaseInsensitive))

QPermissionPlugin *permissionPlugin(const QPermission &permission)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    const char *permissionType = permission.type().name();
    qCDebug(lcPermissions, "Looking for permission plugin for %s", permissionType);

    if (Q_UNLIKELY(!pluginLoader)) {
        qCWarning(lcPermissions, "Cannot check or request permissions during application shutdown");
        return nullptr;
    }

    auto metaDataList = pluginLoader()->metaData();
    for (int i = 0; i < metaDataList.size(); ++i) {
        auto metaData = metaDataList.at(i).value(QtPluginMetaDataKeys::MetaData).toMap();
        auto permissions = metaData.value("Permissions"_L1).toArray();
        if (permissions.contains(QString::fromUtf8(permissionType))) {
            auto className = metaDataList.at(i).value(QtPluginMetaDataKeys::ClassName).toString();
            qCDebug(lcPermissions) << "Found matching plugin" << qUtf8Printable(className);
            auto *plugin = static_cast<QPermissionPlugin*>(pluginLoader()->instance(i));
            return plugin;
        }
    }

    qCWarning(lcPermissions).nospace() << "Could not find permission plugin for "
        << permission.type().name() <<
#ifdef Q_OS_DARWIN
            ". Please make sure you have included the required usage description in your Info.plist"
#endif
            ".";

    return nullptr;
}

} // Unnamed namespace

namespace QPermissions::Private
{
    static constexpr auto FallbackPermission =
#ifdef Q_OS_DARWIN
        // On Apple systems, we always have a plugin to get permissions
        // with. If it is missing, we fail-closed because something is
        // wrong and likely to fail anyway.
        Qt::PermissionStatus::Denied;
#else
        // On other systems, the plugin may be missing. In that case,
        // we optimistically grant access.
        Qt::PermissionStatus::Granted;
#endif

    Qt::PermissionStatus checkPermission(const QPermission &permission)
    {
        if (auto *plugin = permissionPlugin(permission))
            return plugin->checkPermission(permission);
        else
            return FallbackPermission;
    }

    void requestPermission(const QPermission &permission, const QPermissions::Private::PermissionCallback &callback)
    {
        if (auto *plugin = permissionPlugin(permission))
            plugin->requestPermission(permission, callback);
        else
            callback(FallbackPermission);
    }
}

QT_END_NAMESPACE
