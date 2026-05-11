// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSGESTURERECOGNIZER_P_H
#define QOHOSGESTURERECOGNIZER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qgesturerecognizer.h"
#include <memory>

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

std::unique_ptr<QGestureRecognizer> makeQOhosPinchGestureRecognizer();

QT_END_NAMESPACE

#endif // QT_NO_GESTURES

#endif // QOHOSGESTURERECOGNIZER_P_H
