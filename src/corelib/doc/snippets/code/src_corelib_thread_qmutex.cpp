// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
int number = 6;

void method1()
{
    number *= 5;
    number /= 4;
}

void method2()
{
    number *= 3;
    number /= 2;
}
//! [0]


//! [1]
// method1()
number *= 5;        // number is now 30
number /= 4;        // number is now 7

// method2()
number *= 3;        // number is now 21
number /= 2;        // number is now 10
//! [1]


//! [2]
// Thread 1 calls method1()
number *= 5;        // number is now 30

// Thread 2 calls method2().
//
// Most likely Thread 1 has been put to sleep by the operating
// system to allow Thread 2 to run.
number *= 3;        // number is now 90
number /= 2;        // number is now 45

// Thread 1 finishes executing.
number /= 4;        // number is now 11, instead of 10
//! [2]


//! [3]
QMutex mutex;
int number = 6;

void method1()
{
    mutex.lock();
    number *= 5;
    number /= 4;
    mutex.unlock();
}

void method2()
{
    mutex.lock();
    number *= 3;
    number /= 2;
    mutex.unlock();
}
//! [3]


//! [4]
int complexFunction(int flag)
{
    mutex.lock();

    int retVal = 0;

    switch (flag) {
    case 0:
    case 1:
        retVal = moreComplexFunction(flag);
        break;
    case 2:
        {
            int status = anotherFunction();
            if (status < 0) {
                mutex.unlock();
                return -2;
            }
            retVal = status + flag;
        }
        break;
    default:
        if (flag > 10) {
            mutex.unlock();
            return -1;
        }
        break;
    }

    mutex.unlock();
    return retVal;
}
//! [4]


//! [5]
int complexFunction(int flag)
{
    QMutexLocker locker(&mutex);

    int retVal = 0;

    switch (flag) {
    case 0:
    case 1:
        return moreComplexFunction(flag);
    case 2:
        {
            int status = anotherFunction();
            if (status < 0)
                return -2;
            retVal = status + flag;
        }
        break;
    default:
        if (flag > 10)
            return -1;
        break;
    }

    return retVal;
}
//! [5]


//! [6]
class SignalWaiter
{
private:
    QMutexLocker<QMutex> locker;

public:
    SignalWaiter(QMutex *mutex)
        : locker(mutex)
    {
    }

    void waitForSignal()
    {
        ...
        while (!signalled)
            waitCondition.wait(locker.mutex());
        ...
    }
};
//! [6]
