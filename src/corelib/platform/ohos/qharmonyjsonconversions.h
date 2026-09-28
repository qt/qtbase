// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHARMONYJSONCONVERSIONS_H
#define QHARMONYJSONCONVERSIONS_H

#include <QtCore/qglobal.h>

#if defined(Q_QDOC) || defined(Q_OS_HARMONY)

#include <QtCore/qjsonobject.h>
#include <napi/native_api.h>

QT_BEGIN_NAMESPACE

namespace QtHarmony {

Q_CORE_EXPORT napi_value mapQJsonObjectToJsObject(napi_env env, const QJsonObject &jsonObject);
Q_CORE_EXPORT QJsonObject mapJsObjectToQJsonObject(napi_env env, napi_value jsObject);

}

QT_END_NAMESPACE

#endif // Q_QDOC || Q_OS_HARMONY

#endif // QHARMONYJSONCONVERSIONS_H
