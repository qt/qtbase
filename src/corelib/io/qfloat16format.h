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

#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

// [format.formatter.spec] / 5
template <typename T, typename CharT>
constexpr bool FormatterDoesNotExist =
        std::negation_v<
            std::disjunction<
                std::is_default_constructible<std::formatter<T, CharT>>,
                std::is_copy_constructible<std::formatter<T, CharT>>,
                std::is_move_constructible<std::formatter<T, CharT>>,
                std::is_copy_assignable<std::formatter<T, CharT>>,
                std::is_move_assignable<std::formatter<T, CharT>>
            >
        >;

template <typename CharT>
using QFloat16FormatterBaseType =
        std::conditional_t<FormatterDoesNotExist<qfloat16::NearestFloat, CharT>,
                           float,
                           qfloat16::NearestFloat>;

} // namespace QtPrivate

QT_END_NAMESPACE

namespace std {
template <typename CharT>
struct formatter<QT_PREPEND_NAMESPACE(qfloat16), CharT>
    : std::formatter<QT_PREPEND_NAMESPACE(QtPrivate::QFloat16FormatterBaseType<CharT>), CharT>
{
    template <typename FormatContext>
    auto format(QT_PREPEND_NAMESPACE(qfloat16) val, FormatContext &ctx) const
    {
        using FloatType = QT_PREPEND_NAMESPACE(QtPrivate::QFloat16FormatterBaseType<CharT>);
        return std::formatter<FloatType, CharT>::format(FloatType(val), ctx);
    }
};
} // namespace std

#endif // QT_SUPPORTS_STD_FORMAT

#endif // QFLOAT16FORMAT_H
