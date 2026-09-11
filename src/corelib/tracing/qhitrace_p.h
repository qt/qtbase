// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHITRACE_P_H
#define QHITRACE_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>

QT_REQUIRE_CONFIG(hitrace);

QT_BEGIN_NAMESPACE

constexpr inline int qtHiTraceRecordBudget = 420;
constexpr inline int qtHiTraceSmallestValueBudget = 16;

constexpr int qtHiTraceValueBudget(int sliceNameSize, int fixedSize, int valueCount)
{
    return qMax(qtHiTraceSmallestValueBudget,
                (qtHiTraceRecordBudget - sliceNameSize - fixedSize) / valueCount);
}

Q_CORE_EXPORT const char *qtHiTraceFormatArguments(int sliceNameSize, const char *format, ...)
        Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);

Q_CORE_EXPORT QByteArray qtHiTraceArgument(const char *value, qsizetype maximumSize);
Q_CORE_EXPORT QByteArray qtHiTraceArgument(const QString &value, qsizetype maximumSize);

QT_END_NAMESPACE

#endif // QHITRACE_P_H
