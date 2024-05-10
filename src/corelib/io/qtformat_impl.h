// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTFORMAT_IMPL_H
#define QTFORMAT_IMPL_H

#if 0
#pragma qt_no_master_include
#pragma qt_sync_skip_header_check
#endif

#include <QtCore/qsystemdetection.h>
#include <QtCore/qtconfigmacros.h>

#if __has_include(<format>)
#  include <format>
#endif

#if (defined(__cpp_lib_format) && (__cpp_lib_format >= 202106L))

#define QT_SUPPORTS_STD_FORMAT  1

#endif // __cpp_lib_format

#endif // QTFORMAT_IMPL_H
