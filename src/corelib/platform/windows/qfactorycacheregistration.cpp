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
