// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "signalsandslots.h"

//! [0]
void Counter::setValue(int value)
{
    if (value != m_value) {
        m_value = value;
        emit valueChanged(value);
    }
}
//! [0]

int main()
{
//! [1]
    Counter a, b;
//! [1] //! [2]
    QObject::connect(&a, &Counter::valueChanged,
                     &b, &Counter::setValue);
//! [2]

//! [3]
    a.setValue(12);     // a.value() == 12, b.value() == 12
//! [3] //! [4]
    b.setValue(48);     // a.value() == 12, b.value() == 48
//! [4]
}
