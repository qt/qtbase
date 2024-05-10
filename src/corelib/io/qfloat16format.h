// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFLOAT16FORMAT_H
#define QFLOAT16FORMAT_H

#if 0
#pragma qt_class(QFloat16Format)
#pragma qt_sync_skip_header_check
#endif

#include <QtCore/qglobal.h>
#include <QtCore/qtformat_impl.h>

#ifdef QT_SUPPORTS_STD_FORMAT

#include <QtCore/qfloat16.h>

template <typename CharT>
struct std::formatter<QT_PREPEND_NAMESPACE(qfloat16), CharT> : std::formatter<float, CharT>
{
    template <typename FormatContext>
    auto format(QT_PREPEND_NAMESPACE(qfloat16) val, FormatContext &ctx) const
    {
        return std::formatter<float, CharT>::format(float(val), ctx);
    }
};

#endif // QT_SUPPORTS_STD_FORMAT

#endif // QFLOAT16FORMAT_H
