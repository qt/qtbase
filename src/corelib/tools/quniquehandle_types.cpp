// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/quniquehandle_types_p.h>

#include "qplatformdefs.h" // For QT_CLOSE

#ifdef Q_OS_WIN
#  include <QtCore/qt_windows.h>
#endif

#ifdef Q_OS_UNIX
#  include <QtCore/private/qcore_unix_p.h> // for qt_safe_close
#endif

QT_BEGIN_NAMESPACE

namespace QtUniqueHandleTraits {

#ifdef Q_OS_WIN

bool InvalidHandleTraits::close(Type handle) noexcept
{
    return ::CloseHandle(handle);
}

bool NullHandleTraits::close(Type handle) noexcept
{
    return ::CloseHandle(handle);
}

#endif

bool FileDescriptorHandleTraits::close(Type handle)
{
    // not noexcept because close() is a POSIX cancellation point
    return QT_CLOSE(handle) == 0;
}

bool FILEHandleTraits::close(Type handle)
{
    // not noexcept because fclose() is a POSIX cancellation point
    return ::fclose(handle);
}

} // namespace QtUniqueHandleTraits

#ifdef Q_OS_UNIX

using QUniqueFileDescriptorHandle = QUniqueHandle<QtUniqueHandleTraits::FileDescriptorHandleTraits>;

#endif

QT_END_NAMESPACE
