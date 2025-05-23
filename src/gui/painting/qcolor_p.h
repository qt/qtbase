// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QCOLOR_P_H
#define QCOLOR_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "QtGui/qrgb.h"

#include <optional>

QT_BEGIN_NAMESPACE

std::optional<QRgb> qt_get_hex_rgb(const char *) Q_DECL_PURE_FUNCTION;

QT_END_NAMESPACE

#endif // QCOLOR_P_H
