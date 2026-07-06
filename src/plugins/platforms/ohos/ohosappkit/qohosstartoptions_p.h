// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSTARTOPTIONS_P_H
#define QOHOSSTARTOPTIONS_P_H

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

#include "qohosqpafunctionspart1_p.h"
#include <QtOhosAppKit/qohosstartoptions.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

std::optional<QtOhos::QOhosQpaFunctionsPart1::StartOptions> tryConvertStartOptionsToQpaFunctionsStruct(
    const QOhosStartOptions &options);

}

QT_END_NAMESPACE

#endif
