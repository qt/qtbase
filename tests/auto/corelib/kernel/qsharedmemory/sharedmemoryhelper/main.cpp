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

#include <QSharedMemory>
#include <QStringList>
#include <QDebug>
#include <QTest>
#include <stdio.h>

void set(QSharedMemory &sm, int pos, QChar value)
{
    ((char*)sm.data())[pos] = value.toLatin1();
}

QChar get(QSharedMemory &sm, int i)
{
    return QChar::fromLatin1(((char*)sm.data())[i]);
}

int readonly_segfault()
{
    QSharedMemory sharedMemory;
    sharedMemory.setKey("readonly_segfault");
    sharedMemory.create(1024, QSharedMemory::ReadOnly);
    sharedMemory.lock();
    set(sharedMemory, 0, 'a');
    sharedMemory.unlock();
    return EXIT_SUCCESS;
}

int producer()
{
    QSharedMemory producer;
    producer.setKey("market");

    int size = 1024;
    if (!producer.create(size)) {
        if (producer.error() == QSharedMemory::AlreadyExists) {
            if (!producer.attach()) {
                qWarning() << "Could not attach to" << producer.key();
                return EXIT_FAILURE;
            }
        } else {
            qWarning() << "Could not create" << producer.key();
            return EXIT_FAILURE;
        }
    }
    // tell parent we're ready
    //qDebug("producer created and attached");
    puts("");
    fflush(stdout);

    if (!producer.lock()) {
        qWarning() << "Could not lock" << producer.key();
        return EXIT_FAILURE;
    }
    set(producer, 0, 'Q');
    if (!producer.unlock()) {
        qWarning() << "Could not lock" << producer.key();
        return EXIT_FAILURE;
    }

    int i = 0;
    while (i < 5) {
        if (!producer.lock()) {
            qWarning() << "Could not lock" << producer.key();
            return EXIT_FAILURE;
        }
        if (get(producer, 0) == 'Q') {
            if (!producer.unlock()) {
                qWarning() << "Could not unlock" << producer.key();
                return EXIT_FAILURE;
            }
            QTest::qSleep(1);
            continue;
        }
        //qDebug() << "producer:" << i);
        ++i;
        set(producer, 0, 'Q');
        if (!producer.unlock()) {
            qWarning() << "Could not unlock" << producer.key();
            return EXIT_FAILURE;
        }
        QTest::qSleep(1);
    }
    if (!producer.lock()) {
        qWarning() << "Could not lock" << producer.key();
        return EXIT_FAILURE;
    }
    set(producer, 0, 'E');
    if (!producer.unlock()) {
        qWarning() << "Could not unlock" << producer.key();
        return EXIT_FAILURE;
    }

    //qDebug("producer done");

    // Sleep for a bit to let all consumers exit
    getchar();
    return EXIT_SUCCESS;
}

int consumer()
{
    QSharedMemory consumer;
    consumer.setKey("market");

    //qDebug("consumer starting");
    int tries = 0;
    while (!consumer.attach()) {
        if (tries == 5000) {
            qWarning() << "consumer exiting, waiting too long";
            return EXIT_FAILURE;
        }
        ++tries;
        QTest::qSleep(1);
    }
    //qDebug("consumer attached");


    int i = 0;
    while (true) {
        if (!consumer.lock()) {
            qWarning() << "Could not lock" << consumer.key();
            return EXIT_FAILURE;
        }
        if (get(consumer, 0) == 'Q') {
            set(consumer, 0, ++i);
            //qDebug() << "consumer sets" << i;
        }
        if (get(consumer, 0) == 'E') {
            if (!consumer.unlock()) {
                qWarning() << "Could not unlock" << consumer.key();
                return EXIT_FAILURE;
            }
            break;
        }
        if (!consumer.unlock()) {
            qWarning() << "Could not unlock" << consumer.key();
            return EXIT_FAILURE;
        }
        QTest::qSleep(10);
    }

    //qDebug("consumer detaching");
    if (!consumer.detach()) {
        qWarning() << "Could not detach" << consumer.key();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList arguments = app.arguments();
    if (app.arguments().count() != 2) {
        qWarning("Please call the helper with the function to call as argument");
        return EXIT_FAILURE;
    }
    QString function = arguments.at(1);
    if (function == QLatin1String("readonly_segfault"))
        return readonly_segfault();
    else if (function == QLatin1String("producer"))
        return producer();
    else if (function == QLatin1String("consumer"))
        return consumer();
    else
        qWarning() << "Unknown function" << arguments.at(1);

    return EXIT_SUCCESS;
}
