// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
MySharedType &MySharedType::operator=(const MySharedType &other)
{
    (void) other.data->atomicInt.ref();
    if (!data->atomicInt.deref()) {
        // The last reference has been released
        delete d;
    }
    d = other.d;
    return *this;
}
//! [0]


//! [1]
if (currentValue == expectedValue) {
    currentValue = newValue;
    return true;
}
return false;
//! [1]


//! [2]
int originalValue = currentValue;
currentValue = newValue;
return originalValue;
//! [2]


//! [3]
int originalValue = currentValue;
currentValue += valueToAdd;
return originalValue;
//! [3]


//! [4]
if (currentValue == expectedValue) {
    currentValue = newValue;
    return true;
}
return false;
//! [4]


//! [5]
T *originalValue = currentValue;
currentValue = newValue;
return originalValue;
//! [5]


//! [6]
T *originalValue = currentValue;
currentValue += valueToAdd;
return originalValue;
//! [6]
