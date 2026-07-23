// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSOPERATIONSTATUS_P_H
#define QOHOSOPERATIONSTATUS_P_H

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

#include <QtCore/qsharedpointer.h>
#include <QtOhosAppKit/qohosoperationstatus.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

QSharedPointer<OperationStatus> createOperationStatus(bool status);

}

QT_END_NAMESPACE

#endif // QOHOSOPERATIONSTATUS_P_H
