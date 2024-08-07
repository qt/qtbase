// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QFLAGS64OBJECT_H
#define QFLAGS64OBJECT_H
#include <QtCore/QObject>

class QEnum64Object : public QObject
{
    Q_OBJECT
public:
    enum LargeEnum : qint64 {
        Value0      = 0,
        ValueMixed  = Q_INT64_C(0x1122'3344'5566'7788),
        ValueMinus1 = -1,
    };
    Q_ENUM(LargeEnum)
};

class QFlags64Object : public QObject
{
    Q_OBJECT
public:
    enum LargeFlag : qint64 {
        Value0      = 0,
        ValueMixed  = Q_INT64_C(0x1122'3344'5566'7788),
        ValueMinus1 = -1,
    };
    Q_DECLARE_FLAGS(LargeFlags, LargeFlag)
    Q_FLAG(LargeFlags)

    enum class ScopedLargeFlag : quint64 {
        Value0      = 0,
        ValueMixed  = Q_UINT64_C(0x1122'3344'5566'7788),
        ValueMinus1 = quint64(-1),
    };
    Q_DECLARE_FLAGS(ScopedLargeFlags, ScopedLargeFlag)
    Q_FLAG(ScopedLargeFlags)
};

#endif // QFLAGS64OBJECT_H
