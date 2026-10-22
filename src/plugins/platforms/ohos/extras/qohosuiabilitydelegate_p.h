// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSUIABILITYDELEGATE_P_H
#define QOHOSUIABILITYDELEGATE_P_H

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

#include <QtCore/qharmonyappjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>

#include <functional>
#include <memory>

#include <napi/native_api.h>

QT_BEGIN_NAMESPACE

namespace QtHarmonyExtras::Private {

// One handler per ability, abilityRef must not be null. The handler's result is the ability's
// onContinue() result. Call it and release the handle on the JS thread.
[[nodiscard]] std::shared_ptr<void> setUiAbilityOnContinueHandler(
    QHarmonyAppJsContext &appJsContext, QHarmonyJsObjectRef abilityRef,
    std::function<napi_value(QHarmonyAppJsContext &, napi_value wantParamValue)> handler);

}

QT_END_NAMESPACE

#endif // QOHOSUIABILITYDELEGATE_P_H
