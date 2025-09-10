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
            if (!plugin->parent()) {
                // We want to re-parent the plugin to the factory loader, so that it's
                // cleaned up properly. To do so we first need to move the plugin to the
                // same thread as the factory loader, as the plugin might be instantiated
                // on a secondary thread if triggered from a checkPermission call (which
                // is allowed on any thread).
                plugin->moveToThread(pluginLoader->thread());

                // Also, as setParent will involve sending a ChildAdded event to the parent,
                // we need to make the call on the same thread as the parent lives, as events
                // are not allowed to be sent to an object owned by another thread.
                QMetaObject::invokeMethod(plugin, [=] {
                    plugin->setParent(pluginLoader);
                });
            }
            return plugin;
        }
    }

    qCWarning(lcPermissions).nospace() << "Could not find permission plugin for "
        << permission.type().name() << ". Please make sure you have included the "
        << "required usage description in your Info.plist";

    return nullptr;
}

} // Unnamed namespace

namespace QPermissions::Private
{
    Qt::PermissionStatus checkPermission(const QPermission &permission)
    {
        if (auto *plugin = permissionPlugin(permission))
            return plugin->checkPermission(permission);
        else
            return Qt::PermissionStatus::Denied;
    }

    void requestPermission(const QPermission &permission, const QPermissions::Private::PermissionCallback &callback)
    {
        if (auto *plugin = permissionPlugin(permission))
            plugin->requestPermission(permission, callback);
        else
            callback(Qt::PermissionStatus::Denied);
    }
}

QT_END_NAMESPACE
