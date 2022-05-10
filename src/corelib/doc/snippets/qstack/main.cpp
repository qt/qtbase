// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
//! [0]
    QStack<int> stack;
    stack.push(1);
    stack.push(2);
    stack.push(3);
    while (!stack.isEmpty())
        cout << stack.pop() << Qt::endl;
//! [0]
}
