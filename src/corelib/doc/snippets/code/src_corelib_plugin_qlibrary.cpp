// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QLibrary myLib("mylib");
typedef void (*MyPrototype)();
MyPrototype myFunction = (MyPrototype) myLib.resolve("mysymbol");
if (myFunction)
    myFunction();
//! [0]


//! [1]
typedef void (*MyPrototype)();
MyPrototype myFunction =
        (MyPrototype) QLibrary::resolve("mylib", "mysymbol");
if (myFunction)
    myFunction();
//! [1]


//! [2]
typedef int (*AvgFunction)(int, int);

AvgFunction avg = (AvgFunction) library->resolve("avg");
if (avg)
    return avg(5, 8);
else
    return -1;
//! [2]


//! [3]
extern "C" MY_EXPORT int avg(int a, int b)
{
    return (a + b) / 2;
}
//! [3]


//! [4]
#ifdef Q_OS_WIN
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT
#endif
//! [4]
