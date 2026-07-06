// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWANTUTILS_H
#define QOHOSWANTUTILS_H

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

#include "qohosqpafunctionspart1_p.h"
#include <QtCore/qjsonobject.h>
#include <QtCore/qsharedpointer.h>
#include <QtOhosAppKit/qohoswant.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

QJsonObject convertWantToJsonObject(const QOhosWant &want);

QOhosWant convertWantFromJsonObject(const QJsonObject &jsonWant);

QSharedPointer<QOhosWantInfo> convertToOhosAppKitWantInfo(
    QSharedPointer<QtOhos::QOhosQpaFunctionsPart1::WantInfo> wantInfo);

QSharedPointer<QtOhos::QOhosQpaFunctionsPart1::WantInfo> convertToQpaWantInfo(
    QSharedPointer<QOhosWantInfo> wantInfo);

}

QT_END_NAMESPACE

#endif // QOHOSWANTUTILS_H
