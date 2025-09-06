// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define WHICH_TYPE_IS_RELOCATABLE RelocatableInPluginType
#include "relocatable_change.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_TYPEINFO(RelocatableInPluginType, Q_RELOCATABLE_TYPE);

extern "C" QVariant Q_DECL_EXPORT pluginCreateVariant(bool relocatable)
{
    return relocatable ? relocatabilityChange_create<RelocatableInPluginType>()
                       : relocatabilityChange_create<RelocatableInAppType>();
}

QT_END_NAMESPACE
