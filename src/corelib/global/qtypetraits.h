// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTYPETRAITS_H
#define QTYPETRAITS_H

#include <QtCore/qtconfigmacros.h>

#include <type_traits>

#if 0
#pragma qt_class(QTypeTraits)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

// like std::to_underlying
template <typename Enum>
constexpr std::underlying_type_t<Enum> qToUnderlying(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

QT_END_NAMESPACE

#endif // QTYPETRAITS_H
