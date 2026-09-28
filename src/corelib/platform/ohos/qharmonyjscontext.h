// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHARMONYJSCONTEXT_H
#define QHARMONYJSCONTEXT_H

#include <QtCore/qglobal.h>

#if defined(Q_QDOC) || defined(Q_OS_HARMONY)

#include <napi/native_api.h>
#include <string>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QHarmonyJsContext
{
public:
    static QHarmonyJsContext &fromEnv(napi_env env);

    napi_env env();
    napi_value tryImportModule(const std::string &moduleName);

protected:
    QHarmonyJsContext();
    ~QHarmonyJsContext();

private:
    Q_DISABLE_COPY_MOVE(QHarmonyJsContext)
};

QT_END_NAMESPACE

#endif // Q_QDOC || Q_OS_HARMONY

#endif // QHARMONYJSCONTEXT_H
