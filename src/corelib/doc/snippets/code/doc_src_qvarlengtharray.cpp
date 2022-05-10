// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
int myfunc(int n)
{
    int table[n + 1];  // WRONG
    ...
    return table[n];
}
//! [0]


//! [1]
int myfunc(int n)
{
    int *table = new int[n + 1];
    ...
    int ret = table[n];
    delete[] table;
    return ret;
}
//! [1]


//! [2]
int myfunc(int n)
{
    QVarLengthArray<int, 1024> array(n + 1);
    ...
    return array[n];
}
//! [2]


//! [3]
QVarLengthArray<int> array(10);
int *data = array.data();
for (int i = 0; i < 10; ++i)
    data[i] = 2 * i;
//! [3]
