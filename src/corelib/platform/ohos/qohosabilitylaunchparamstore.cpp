// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosabilitylaunchparamstore_p.h"

QT_BEGIN_NAMESPACE

QOhosAbilityLaunchParamStore &QOhosAbilityLaunchParamStore::instance(QOhosJsState &jsState)
{
    return jsState.getAttachedObjectWithLazyCreate<QOhosAbilityLaunchParamStore>();
}

QOhosAbilityLaunchParamStore::QOhosAbilityLaunchParamStore()
    : m_keyRef(QNapi::Reference<>::makePersistentFrom(
          QNapi::Symbol::New(QOhosJsThreadOps::instance().jsState().env(), "_io_qt_abilityLaunchParam")))
{
}

void QOhosAbilityLaunchParamStore::set(QNapi::Object ability, QNapi::Object launchParam)
{
    ability.set(m_keyRef.Value(), launchParam);
}

QNapi::Object QOhosAbilityLaunchParamStore::getOrEmpty(QNapi::Object ability)
{
    return QNapi::getOptionalPropOrEmpty<QNapi::Object>(ability, m_keyRef.Value());
}

QT_END_NAMESPACE
