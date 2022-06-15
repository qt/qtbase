/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

#if !defined(QT_BOOTSTRAPPED) && defined(Q_OS_WIN) && !defined(Q_CC_CLANG) && QT_CONFIG(cpp_winrt)
#    define QT_USE_FACTORY_CACHE_REGISTRATION
#endif

#ifdef QT_USE_FACTORY_CACHE_REGISTRATION

#include <winrt/base.h>

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
