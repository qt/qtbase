// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfactorycacheregistration_p.h"

#include <QtCore/QMutex>

QT_BEGIN_NAMESPACE

#ifdef QT_USE_FACTORY_CACHE_REGISTRATION

static QBasicMutex registrationMutex;
static detail::QWinRTFactoryCacheRegistration *firstElement;

detail::QWinRTFactoryCacheRegistration::QWinRTFactoryCacheRegistration(
        QFunctionPointer clearFunction)
    : m_clearFunction(clearFunction)
{
    QMutexLocker lock(&registrationMutex);

    // forward pointers
    m_next = std::exchange(firstElement, this);

    // backward pointers
    m_prevNext = &firstElement;
    if (m_next)
        m_next->m_prevNext = &m_next;
}

detail::QWinRTFactoryCacheRegistration::~QWinRTFactoryCacheRegistration()
{
    QMutexLocker lock(&registrationMutex);

    *m_prevNext = m_next;

    if (m_next)
        m_next->m_prevNext = m_prevNext;
}

void detail::QWinRTFactoryCacheRegistration::clearAllCaches()
{
    QMutexLocker lock(&registrationMutex);

    detail::QWinRTFactoryCacheRegistration *element;

    for (element = firstElement; element != nullptr; element = element->m_next) {
        element->m_clearFunction();
    }
}

#endif

QT_END_NAMESPACE
