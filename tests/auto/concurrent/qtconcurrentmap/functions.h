/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

bool keepEvenIntegers(const int &x)
{
    return (x & 1) == 0;
}

class KeepEvenIntegers
{
public:
    bool operator()(const int &x)
    {
        return (x & 1) == 0;
    }
};

class Number
{
    int n;

public:
    Number()
        : n(0)
    { }

    Number(int n)
        : n(n)
    { }

    void multiplyBy2()
    {
        n *= 2;
    }

    Number multipliedBy2() const
    {
        return n * 2;
    }

    bool isEven() const
    {
        return (n & 1) == 0;
    }

    int toInt() const
    {
        return n;
    }

    QString toString() const
    {
        return QString::number(n);
    }

    bool operator==(const Number &other) const
    {
        return n == other.n;
    }
};

void intSumReduce(int &sum, int x)
{
    sum += x;
}

class IntSumReduce
{
public:
    void operator()(int &sum, int x)
    {
        sum += x;
    }
};

void numberSumReduce(int &sum, const Number &x)
{
    sum += x.toInt();
}

class NumberSumReduce
{
public:
    void operator()(int &sum, const Number &x)
    {
        sum += x.toInt();
    }
};

#endif
