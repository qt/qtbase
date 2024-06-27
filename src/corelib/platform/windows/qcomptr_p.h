// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPTR_P_H
#define QCOMPTR_P_H

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

#if defined(Q_OS_WIN) || defined(Q_QDOC)

#include <QtCore/qt_windows.h>
#include <wrl/client.h>

QT_BEGIN_NAMESPACE

using Microsoft::WRL::ComPtr;

template <typename T, typename... Args>
ComPtr<T> makeComObject(Args &&...args)
{
    ComPtr<T> p;
    // Don't use Attach because of MINGW64 bug
    // #892 Microsoft::WRL::ComPtr::Attach leaks references
    *p.GetAddressOf() = new T(std::forward<Args>(args)...);
    return p;
}

QT_END_NAMESPACE

#endif // Q_OS_WIN

#endif // QCOMPTR_P_H
