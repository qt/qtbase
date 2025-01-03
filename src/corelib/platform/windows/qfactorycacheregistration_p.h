// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFACTORYCACHEREGISTRATION_P_H
#define QFACTORYCACHEREGISTRATION_P_H

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

#include <QtCore/qglobal.h>

#if !defined(QT_BOOTSTRAPPED) && QT_CONFIG(cpp_winrt)
#    define QT_USE_FACTORY_CACHE_REGISTRATION
#endif

#ifdef QT_USE_FACTORY_CACHE_REGISTRATION

#include "qt_winrtbase_p.h"

QT_BEGIN_NAMESPACE

namespace detail {

class QWinRTFactoryCacheRegistration
{
public:
    Q_CORE_EXPORT explicit QWinRTFactoryCacheRegistration(QFunctionPointer clearFunction);
    Q_CORE_EXPORT ~QWinRTFactoryCacheRegistration();
    Q_CORE_EXPORT static void clearAllCaches();

    Q_DISABLE_COPY_MOVE(QWinRTFactoryCacheRegistration)
private:
    QWinRTFactoryCacheRegistration **m_prevNext = nullptr;
    QWinRTFactoryCacheRegistration *m_next = nullptr;
    QFunctionPointer m_clearFunction;
};

inline QWinRTFactoryCacheRegistration reg([]() noexcept { winrt::clear_factory_cache(); });
}

QT_END_NAMESPACE

#endif
#endif // QFACTORYCACHEREGISTRATION_P_H
