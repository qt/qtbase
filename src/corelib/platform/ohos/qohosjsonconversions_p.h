// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSJSONCONVERSIONS_P_H
#define QOHOSJSONCONVERSIONS_P_H

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

#include <QtCore/private/qnapi_p.h>
#include <QtCore/qjsonobject.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

// follows JSON.stringify(): property values without a JSON counterpart are skipped in objects and become null in arrays
Q_CORE_EXPORT QJsonObject mapNapiObjectToJsonObject(const QNapi::Object &napiObject);

// null and undefined QJsonValue both become JS null
Q_CORE_EXPORT QNapi::Object mapJsonObjectToNapiObject(napi_env env, const QJsonObject &jsonObject);

}

QT_END_NAMESPACE

#endif // QOHOSJSONCONVERSIONS_P_H
