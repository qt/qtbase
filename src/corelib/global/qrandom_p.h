/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
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
****************************************************************************/

#ifndef QRANDOM_P_H
#define QRANDOM_P_H

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

#include "qglobal_p.h"
#include <private/qsimd_p.h>

QT_BEGIN_NAMESPACE

enum QRandomGeneratorControl {
    UseSystemRNG = 1,
    SkipSystemRNG = 2,
    SkipHWRNG = 4,
    SetRandomData = 8,

    // 28 bits
    RandomDataMask = 0xfffffff0
};

enum RNGType {
    SystemRNG = 0,
    MersenneTwister = 1
};

#if defined(QT_BUILD_INTERNAL) && defined(QT_BUILD_CORE_LIB)
Q_CORE_EXPORT QBasicAtomicInteger<uint> qt_randomdevice_control = Q_BASIC_ATOMIC_INITIALIZER(0U);
#elif defined(QT_BUILD_INTERNAL)
extern Q_CORE_EXPORT QBasicAtomicInteger<uint> qt_randomdevice_control;
#else
static const struct {
    uint loadAcquire() const { return 0; }
} qt_randomdevice_control;
#endif



QT_END_NAMESPACE

#endif // QRANDOM_P_H
