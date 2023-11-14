// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QTNOOP_H
#define QTNOOP_H

#if 0
#pragma qt_sync_stop_processing
#endif

#ifdef __cplusplus
constexpr
#endif
inline void qt_noop(void)
#ifdef __cplusplus
    noexcept
#endif
{}

#endif // QTNOOP_H
