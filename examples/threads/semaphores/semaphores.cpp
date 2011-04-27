/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include <stdio.h>
#include <stdlib.h>

//! [0]
#ifdef Q_WS_S60
const int DataSize = 300;
#else
const int DataSize = 100000;
#endif

const int BufferSize = 8192;
char buffer[BufferSize];

QSemaphore freeBytes(BufferSize);
QSemaphore usedBytes;
//! [0]

//! [1]
class Producer : public QThread
//! [1] //! [2]
{
public:
    void run()
    {
        qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
        for (int i = 0; i < DataSize; ++i) {
            freeBytes.acquire();
            buffer[i % BufferSize] = "ACGT"[(int)qrand() % 4];
            usedBytes.release();
        }
    }
};
//! [2]

//! [3]
class Consumer : public QThread
//! [3] //! [4]
{
    Q_OBJECT
public:
    void run()
    {
        for (int i = 0; i < DataSize; ++i) {
            usedBytes.acquire();
    #ifdef Q_WS_S60
            QString text(buffer[i % BufferSize]);
            freeBytes.release();
            emit stringConsumed(text);
    #else
            fprintf(stderr, "%c", buffer[i % BufferSize]);
            freeBytes.release();
    #endif
        }
        fprintf(stderr, "\n");
    }

signals:
    void stringConsumed(const QString &text);

protected:
    bool finish;
};
//! [4]

//! [5]
int main(int argc, char *argv[])
//! [5] //! [6]
{
#ifdef Q_WS_S60
    // Self made console for Symbian
    QApplication app(argc, argv);
    QPlainTextEdit console;
    console.setReadOnly(true);
    console.setTextInteractionFlags(Qt::NoTextInteraction);
    console.showMaximized();

    Producer producer;
    Consumer consumer;

    QObject::connect(&consumer, SIGNAL(stringConsumed(const QString&)), &console, SLOT(insertPlainText(QString)), Qt::BlockingQueuedConnection);

    producer.start();
    consumer.start();

    app.exec();
#else
    QCoreApplication app(argc, argv);
    Producer producer;
    Consumer consumer;
    producer.start();
    consumer.start();
    producer.wait();
    consumer.wait();
    return 0;
#endif
}
//! [6]

#include "semaphores.moc"
