// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QChronoTimer>
#include <QObject>
#include <QTimer>

using namespace std::chrono;

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

// QTimer
class MyWidget : QObject
{
    MyWidget()
    {
//! [qtimer-singleshot]
        MyWidget widget;
        QTimer::singleShot(200ms, &widget, &MyWidget::updateCaption);
//! [qtimer-singleshot]

//! [zero-timer]
        // The default interval is 0ns
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MyWidget::processOneThing);
        timer->start();
//! [zero-timer]

        {
//! [timer-interval-in-ctor]
        QTimer *timer = new QTimer(1s, this);
        connect(timer, &QTimer::timeout, this, &MyWidget::processOneThing);
        timer->start();
//! [timer-interval-in-ctor]
        }

        {
//! [timer-setinterval]
        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MyWidget::processOneThing);
        timer->setInterval(1s);
        timer->start();
//! [timer-setinterval]
        }

        {
//! [qchronotimer-interval-in-ctor]
        auto *timer = new QChronoTimer(1s, this);
        connect(timer, &QChronoTimer::timeout, this, &MyWidget::processOneThing);
        timer->start();
//! [qchronotimer-interval-in-ctor]
        }

        {
//! [qchronotimer-setinterval]
        QChronoTimer *timer = new QChronoTimer(this);
        connect(timer, &QChronoTimer::timeout, this, &MyWidget::processOneThing);
        timer->setInterval(1s);
        timer->start();
//! [qchronotimer-setinterval]
        }
    }

public Q_SLOTS:
    void processOneThing();
};

int main()
{
}
