// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmetatype.h"

#include <QtCore/private/qmetaobjectbuilder_p.h>


void tst_QMetaType::automaticTemplateRegistration_2()
{
    FOR_EACH_STATIC_PRIMITIVE_TYPE(
      PRINT_2ARG_TEMPLATE
    )
}
