// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QTIMER_P_H
#define QTIMER_P_H
//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt translation tools.  This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//
#include "qobject_p.h"
#include "qproperty_p.h"
#include "qtimer.h"

QT_BEGIN_NAMESPACE

class QTimerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QTimer)
public:
    static constexpr int INV_TIMER = -1; // invalid timer id

    void setInterval(int msec) { q_func()->setInterval(msec); }
    bool isActiveActualCalculation() const { return id >= 0; }

    int id = INV_TIMER;
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QTimerPrivate, int, inter, &QTimerPrivate::setInterval, 0)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QTimerPrivate, bool, single, false)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QTimerPrivate, Qt::TimerType, type, Qt::CoarseTimer)
    Q_OBJECT_COMPUTED_PROPERTY(QTimerPrivate, bool, isActiveData,
                               &QTimerPrivate::isActiveActualCalculation)
};

QT_END_NAMESPACE
#endif // QTIMER_P_H
