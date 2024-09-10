// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FLAGS_PROPERTY_INTEGER_ACCESS
#define FLAGS_PROPERTY_INTEGER_ACCESS
#include <QtCore/qobject.h>

class ClassWithFlagsAccessAsInteger : public QObject
{
    Q_OBJECT
public:
    enum Flag { F1 = 1, F2 = 2 };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)
    Q_PROPERTY(Flags flagsValue READ flagsValue WRITE setFlagsValue)
    uint flagsValue() const { return f; }
    void setFlagsValue(uint v) { f = v; }

private:
    uint f = 0;
};

#endif // FLAGS_PROPERTY_INTEGER_ACCESS
