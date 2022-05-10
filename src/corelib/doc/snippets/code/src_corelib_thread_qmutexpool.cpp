// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
class Number {
public:
    Number(double n) : num (n) { }

    void setNumber(double n) { num = n; }
    double number() const { return num; }

private:
    double num;
};
//! [0]


//! [1]
void calcSquare(Number *num)
{
    QMutexLocker locker(mutexpool.get(num));
    num.setNumber(num.number() * num.number());
}
//! [1]
