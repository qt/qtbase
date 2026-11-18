// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSABILITYLAUNCHPARAMSTORE_P_H
#define QOHOSABILITYLAUNCHPARAMSTORE_P_H

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

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>

QT_BEGIN_NAMESPACE

// keeps the launchParam of each UIAbility on the ability object
class Q_CORE_EXPORT QOhosAbilityLaunchParamStore
{
public:
    static QOhosAbilityLaunchParamStore &instance(QOhosJsState &jsState);

    QOhosAbilityLaunchParamStore();

    void set(QNapi::Object ability, QNapi::Object launchParam);
    QNapi::Object getOrEmpty(QNapi::Object ability);

private:
    Q_DISABLE_COPY_MOVE(QOhosAbilityLaunchParamStore)

    QNapi::Reference<QNapi::Symbol> m_keyRef;
};

QT_END_NAMESPACE

#endif // QOHOSABILITYLAUNCHPARAMSTORE_P_H
