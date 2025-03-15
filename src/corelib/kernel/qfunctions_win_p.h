// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFUNCTIONS_WIN_P_H
#define QFUNCTIONS_WIN_P_H

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

#if !defined(QT_BOOTSTRAPPED)
#include <QtCore/private/qfunctions_winrt_p.h>
#endif

#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QComHelper
{
    Q_DISABLE_COPY_MOVE(QComHelper)
public:
    QComHelper(COINIT concurrencyModel = COINIT_APARTMENTTHREADED);
    ~QComHelper();

    bool isValid() const { return SUCCEEDED(m_initResult); }
    explicit operator bool() const { return isValid(); }

private:
    HRESULT m_initResult = E_FAIL;
    DWORD m_threadId{ GetCurrentThreadId() };
};

Q_CORE_EXPORT void qt_win_ensureComInitializedOnThisThread();

Q_CORE_EXPORT bool qt_win_hasPackageIdentity();

QT_END_NAMESPACE

#endif // Q_OS_WIN

#endif // QFUNCTIONS_WIN_P_H
