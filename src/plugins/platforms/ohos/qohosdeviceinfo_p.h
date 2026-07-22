// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSDEVICEINFO_H
#define QOHOSDEVICEINFO_H

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
#include <QtCore/qmap.h>
#include <QtCore/qvariant.h>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QOhosDeviceInfo {
enum class Type {
    deviceType,
    manufacture,
    brand,
    marketName,
    productSeries,
    productModel,
    softwareModel,
    hardwareModel,
    hardwareProfile,
    serial,
    bootloaderVersion,
    abiList,
    securityPatchTag,
    displayVersion,
    incrementalVersion,
    osReleaseType,
    osFullName,
    majorVersion,
    seniorVersion,
    featureVersion,
    buildVersion,
    sdkApiVersion,
    firstApiVersion,
    versionId,
    buildType,
    buildUser,
    buildHost,
    buildTime,
    buildRootHash,
    udid,
    distributionOSName,
    distributionOSVersion,
    distributionOSApiVersion,
    distributionOSReleaseType,
};

enum class RecognizedDeviceType {
    _2in1,
    tablet,
    phone,
};

void init(QMap<Type, QVariant> devinfo);
QVariant getProperty(Type prop);

std::optional<RecognizedDeviceType> tryGetRecognizedDeviceType();
bool isTablet();
bool is2in1();
bool isPhone();
int sdkApiVersion();
bool isCurrentDeviceSupported();

}

QT_END_NAMESPACE

#endif // QOHOSDEVICEINFO_H
