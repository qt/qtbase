// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QSharedMemory>
#include <QStringList>
#include <QDebug>
#include <QTest>
#include <stdio.h>

void set(QSharedMemory &sm, int pos, char value)
{
    ((char*)sm.data())[pos] = value;
}

QChar get(QSharedMemory &sm, int i)
{
    return QChar::fromLatin1(((char*)sm.data())[i]);
}

int readonly_segfault(const QNativeIpcKey &key)
{
    QSharedMemory sharedMemory(key);
    sharedMemory.create(1024, QSharedMemory::ReadOnly);
    sharedMemory.lock();
    set(sharedMemory, 0, 'a');
    sharedMemory.unlock();
    return EXIT_SUCCESS;
}

int producer(const QNativeIpcKey &key)
{
    QSharedMemory producer(key);

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

int consumer(const QNativeIpcKey &key)
{
    QSharedMemory consumer(key);

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
    if (app.arguments().size() != 3) {
        fprintf(stderr, "Usage: %s <mode> <key>\n"
                "<mode> is one of: readonly_segfault, producer, consumer\n",
                argv[0]);
        return EXIT_FAILURE;
    }
    QString function = arguments.at(1);
    QNativeIpcKey key = QNativeIpcKey::fromString(arguments.at(2));

    if (function == QLatin1String("readonly_segfault"))
        return readonly_segfault(key);
    else if (function == QLatin1String("producer"))
        return producer(key);
    else if (function == QLatin1String("consumer"))
        return consumer(key);
    else
        qWarning() << "Unknown function" << arguments.at(1);

    return EXIT_SUCCESS;
}
