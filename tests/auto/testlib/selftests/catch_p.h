// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCATCH_P_H
#define QCATCH_P_H

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

#include <QtCore/qglobal.h>

#include "catch_p_p.h"

QT_BEGIN_NAMESPACE

namespace QTestPrivate {
int catchMain(int argc, char* argv[]);
}

QT_END_NAMESPACE

#endif // QCATCH_P_H
