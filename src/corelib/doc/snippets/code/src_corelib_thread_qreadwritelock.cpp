// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QReadWriteLock lock;

void ReaderThread::run()
{
    ...
    lock.lockForRead();
    read_file();
    lock.unlock();
    ...
}

void WriterThread::run()
{
    ...
    lock.lockForWrite();
    write_file();
    lock.unlock();
    ...
}
//! [0]


//! [1]
QReadWriteLock lock;

QByteArray readData()
{
    QReadLocker locker(&lock);
    ...
    return data;
}
//! [1]


//! [2]
QReadWriteLock lock;

QByteArray readData()
{
    lock.lockForRead();
    ...
    lock.unlock();
    return data;
}
//! [2]


//! [3]
QReadWriteLock lock;

void writeData(const QByteArray &data)
{
    QWriteLocker locker(&lock);
    ...
}
//! [3]


//! [4]
QReadWriteLock lock;

void writeData(const QByteArray &data)
{
    lock.lockForWrite();
    ...
    lock.unlock();
}
//! [4]
