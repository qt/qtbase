// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXPTYPE_TRAITS_H
#define QXPTYPE_TRAITS_H

#include <QtCore/qtconfigmacros.h>

#include <type_traits>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. Types and functions defined in this
// file can reliably be replaced by their std counterparts, once available.
// You may use these definitions in your own code, but be aware that we
// will remove them once Qt depends on the C++ version that supports
// them in namespace std. There will be NO deprecation warning, the
// definitions will JUST go away.
//
// If you can't agree to these terms, don't use these definitions!
//
// We mean it.
//

QT_BEGIN_NAMESPACE

// like std::experimental::{nonesuch,is_detected/_v}(LFTSv2)
namespace qxp {

struct nonesuch {
    ~nonesuch() = delete;
    nonesuch(const nonesuch&) = delete;
    void operator=(const nonesuch&) = delete;
};

namespace _detail {
    template <typename T, typename Void, template <typename...> class Op, typename...Args>
    struct detector {
        using value_t = std::false_type;
        using type = T;
    };
    template <typename T, template <typename...> class Op, typename...Args>
    struct detector<T, std::void_t<Op<Args...>>, Op, Args...> {
        using value_t = std::true_type;
        using type = Op<Args...>;
    };
} // namespace _detail

template <template <typename...> class Op, typename...Args>
using is_detected = typename _detail::detector<qxp::nonesuch, void, Op, Args...>::value_t;

template <template <typename...> class Op, typename...Args>
constexpr inline bool is_detected_v = is_detected<Op, Args...>::value;

} // namespace qxp

QT_END_NAMESPACE

#endif // QXPTYPE_TRAITS_H

