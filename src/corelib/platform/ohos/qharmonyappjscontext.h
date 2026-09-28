// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHARMONYAPPJSCONTEXT_H
#define QHARMONYAPPJSCONTEXT_H

#include <QtCore/qharmonyjscontext.h>

#if defined(Q_QDOC) || defined(Q_OS_HARMONY)

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QHarmonyAppJsContext : public QHarmonyJsContext
{
public:
    static QHarmonyAppJsContext &fromEnv(napi_env env);
    static QHarmonyAppJsContext &fromJsContext(QHarmonyJsContext &jsContext);

    napi_value defaultAbility();

private:
    QHarmonyAppJsContext();
    ~QHarmonyAppJsContext();
};

QT_END_NAMESPACE

#endif // Q_QDOC || Q_OS_HARMONY

#endif // QHARMONYAPPJSCONTEXT_H
