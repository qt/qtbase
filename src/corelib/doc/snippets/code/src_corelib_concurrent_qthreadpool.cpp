// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
class HelloWorldTask : public QRunnable
{
    void run() override
    {
        qDebug() << "Hello world from thread" << QThread::currentThread();
    }
};

HelloWorldTask *hello = new HelloWorldTask();
// QThreadPool takes ownership and deletes 'hello' automatically
QThreadPool::globalInstance()->start(hello);
//! [0]
