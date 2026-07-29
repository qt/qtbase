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

#include <QtCore/qjsonobject.h>
#include <QtCore/qsharedpointer.h>
#include <QtHarmonyExtras/private/qohoswantinfo_p.h>
#include <QtHarmonyExtras/private/qohoswant_p.h>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QtHarmonyExtras::Private {

QJsonObject convertWantToJsonObject(const Want &want);

Want convertWantFromJsonObject(const QJsonObject &jsonWant);

std::shared_ptr<WantInfo> convertToOhosExtrasWantInfo(
    QSharedPointer<detail::WantInfoPriv> wantInfo);

QSharedPointer<detail::WantInfoPriv> convertToQpaWantInfo(
    std::shared_ptr<WantInfo> wantInfo);

}

QT_END_NAMESPACE

#endif // QOHOSWANTUTILS_H
