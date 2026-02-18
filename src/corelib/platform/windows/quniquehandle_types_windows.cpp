// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "quniquehandle_types_windows_p.h"

QT_BEGIN_NAMESPACE

namespace QtUniqueHandleTraits {

bool HDCTraits::close(Type handle, HWND hwnd) noexcept
{
    return ::ReleaseDC(hwnd, handle);
}

} // namespace QtUniqueHandleTraits

QT_END_NAMESPACE
