// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSAPPPERMISSIONS_P_H
#define QOHOSAPPPERMISSIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohospermissionshelper_p.h>
#include <QtCore/qglobal.h>
#include <qohosplugincore.h>
#include <string>

QT_BEGIN_NAMESPACE

namespace QOhosAppPermissions {

using AppPermissionResult = QOhosPermissionsHelper::PermissionRequestResult;

Q_CORE_EXPORT void checkAppPermissionGrantedWithConsumer(
    QtOhos::JsState &jsState, const std::string &permissionName,
    QOhosConsumer<QtOhos::JsState &, bool> resultConsumer);

Q_CORE_EXPORT void requestAppPermissionFromUser(
    QtOhos::JsState &jsState, const std::string &permissionName,
    QOhosConsumer<QtOhos::JsState &, bool> resultConsumer);

Q_CORE_EXPORT void requestAppPermissionFromUser(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> abilityPeer,
    const std::string &permissionName,
    QOhosConsumer<QtOhos::JsState &, bool> resultConsumer);

Q_CORE_EXPORT void requestAppPermissionsFromUserWithResult(
    QtOhos::JsState &jsState, const std::vector<std::string> &permissionNames,
    QOhosConsumer<QtOhos::JsState &, std::vector<AppPermissionResult>> resultConsumer);

Q_CORE_EXPORT void requestAppPermissionsFromUserWithResult(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> abilityPeer,
    const std::vector<std::string> &permissionNames,
    QOhosConsumer<QtOhos::JsState &, std::vector<AppPermissionResult>> resultConsumer);

Q_CORE_EXPORT void requestAppPermissionsOnSetting(
    QtOhos::JsState &jsState, const std::vector<std::string> &permissionNames,
    QOhosConsumer<QtOhos::JsState &, std::vector<bool>> resultConsumer);

Q_CORE_EXPORT void requestAppPermissionsOnSetting(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> abilityPeer,
    const std::vector<std::string> &permissionNames,
    QOhosConsumer<QtOhos::JsState &, std::vector<bool>> resultConsumer);

}

QT_END_NAMESPACE

#endif
