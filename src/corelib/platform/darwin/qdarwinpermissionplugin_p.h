// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDARWINPERMISSIONPLUGIN_P_H
#define QDARWINPERMISSIONPLUGIN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. This header file may change
// from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qnamespace.h>
#include <QtCore/private/qpermissions_p.h>
#include <QtCore/private/qcore_mac_p.h>

#if defined(__OBJC__)
#include <Foundation/NSObject.h>
#endif

QT_USE_NAMESPACE

using namespace QPermissions::Private;

#if defined(__OBJC__)
Q_CORE_EXPORT
#endif
QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QDarwinPermissionHandler, NSObject
- (Qt::PermissionStatus)checkPermission:(QPermission)permission;
- (void)requestPermission:(QPermission)permission withCallback:(PermissionCallback)callback;
- (QStringList)usageDescriptionsFor:(QPermission)permission;
)

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QDarwinPermissionPlugin : public QPermissionPlugin
{
    Q_OBJECT
public:
    QDarwinPermissionPlugin(QDarwinPermissionHandler *handler);
    ~QDarwinPermissionPlugin();

    Qt::PermissionStatus checkPermission(const QPermission &permission) override;
    void requestPermission(const QPermission &permission, const PermissionCallback &callback) override;

private:
    Q_SLOT void permissionUpdated(Qt::PermissionStatus status, const PermissionCallback &callback);
    bool verifyUsageDescriptions(const QPermission &permission);
    QDarwinPermissionHandler *m_handler = nullptr;
};

QT_END_NAMESPACE

#endif // QDARWINPERMISSIONPLUGIN_P_H
