// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef FORWARDDECLARED_H
#define FORWARDDECLARED_H

#include <qglobal.h>

struct MyForwardDeclaredType;      // and ONLY forward-declared

const MyForwardDeclaredType &getForwardDeclaredType() noexcept;
MyForwardDeclaredType *getForwardDeclaredPointer() noexcept;

QT_BEGIN_NAMESPACE
class QEasingCurve;

const QEasingCurve &getEasingCurve() noexcept;
QEasingCurve *getEasingCurvePointer() noexcept;
QT_END_NAMESPACE

#endif // FORWARDDECLARED_H
