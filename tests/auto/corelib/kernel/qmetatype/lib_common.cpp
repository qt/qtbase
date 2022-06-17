// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qcollator.h>
#include "tst_qmetatype_libs.h"

#define DECLARE_FUNCTION(TYPE, ID)                  \
    Q_DECL_EXPORT QMetaType metatype_ ## TYPE()     \
    { return QMetaType::fromType<TYPE>(); }

namespace LIB_NAMESPACE {
FOR_EACH_METATYPE_LIBS(DECLARE_FUNCTION)
}
