// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDARWINPERMISSIONPLUGIN_P_P_H
#define QDARWINPERMISSIONPLUGIN_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. This header file may change
// from version to version without notice, or even be removed.
//
// We mean it.
//

#if !defined(QT_DARWIN_PERMISSION_PLUGIN)
#error "This header should only be included from permission plugins"
#endif

#include <QtCore/qnamespace.h>
#include <QtCore/private/qpermissions_p.h>
#include <QtCore/private/qcore_mac_p.h>

#include "qdarwinpermissionplugin_p.h"

using namespace QPermissions::Private;

#ifndef QT_JOIN
#define QT_JOIN_IMPL(A, B) A ## B
#define QT_JOIN(A, B) QT_JOIN_IMPL(A, B)
#endif

#define PERMISSION_PLUGIN_NAME(SUFFIX) \
    QT_JOIN(QT_JOIN(QT_JOIN( \
        QDarwin, QT_DARWIN_PERMISSION_PLUGIN), Permission), SUFFIX)

#define PERMISSION_PLUGIN_CLASSNAME PERMISSION_PLUGIN_NAME(Plugin)
#define PERMISSION_PLUGIN_HANDLER PERMISSION_PLUGIN_NAME(Handler)

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(
    PERMISSION_PLUGIN_HANDLER,
    QDarwinPermissionHandler
)

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT PERMISSION_PLUGIN_CLASSNAME : public QDarwinPermissionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(
        IID QPermissionPluginInterface_iid
        FILE "QDarwin" QT_STRINGIFY(QT_DARWIN_PERMISSION_PLUGIN) "PermissionPlugin.json")
public:
    PERMISSION_PLUGIN_CLASSNAME()
        : QDarwinPermissionPlugin([[PERMISSION_PLUGIN_HANDLER alloc] init])
    {}
};

QT_END_NAMESPACE

// Request
#if defined(BUILDING_PERMISSION_REQUEST)
extern "C" void PERMISSION_PLUGIN_NAME(Request)() {}
#endif

// -------------------------------------------------------

namespace {
template <typename NativeStatus>
struct NativeStatusHelper;

template <typename NativeStatus>
Qt::PermissionStatus nativeStatusToQtStatus(NativeStatus status)
{
    using Converter = NativeStatusHelper<NativeStatus>;
    switch (status) {
    case Converter::Authorized:
        return Qt::PermissionStatus::Granted;
    case Converter::Denied:
    case Converter::Restricted:
        return Qt::PermissionStatus::Denied;
    case Converter::Undetermined:
        return Qt::PermissionStatus::Undetermined;
    }
    qCWarning(lcPermissions) << "Unknown permission status" << status << "detected in"
        << QT_STRINGIFY(QT_DARWIN_PERMISSION_PLUGIN);
    return Qt::PermissionStatus::Denied;
}
} // namespace

#define QT_DEFINE_PERMISSION_STATUS_CONVERTER(NativeStatus) \
namespace { template<> \
struct NativeStatusHelper<NativeStatus> \
{\
    enum { \
        Authorized = NativeStatus##Authorized, \
        Denied = NativeStatus##Denied, \
        Restricted = NativeStatus##Restricted, \
        Undetermined = NativeStatus##NotDetermined \
    }; \
}; }

#endif // QDARWINPERMISSIONPLUGIN_P_P_H
