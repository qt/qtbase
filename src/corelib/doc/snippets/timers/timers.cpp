// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QTimer>

class Foo : public QObject
{
public:
    Foo();
};

Foo::Foo()
{
//! [0]
    QTimer *timer = new QTimer(this);
//! [0] //! [1]
    connect(timer, &QTimer::timeout, this, &Foo::updateCaption);
//! [1] //! [2]
    timer->start(1000);
//! [2]

//! [3]
    QTimer::singleShot(200, this, &Foo::updateCaption);
//! [3]

    {
    // ZERO-CASE
//! [4]
    QTimer *timer = new QTimer(this);
//! [4] //! [5]
    connect(timer, &QTimer::timeout, this, &Foo::processOneThing);
//! [5] //! [6]
    timer->start();
//! [6]
    }
}

int main()
{

}
