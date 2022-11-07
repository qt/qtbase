// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <QtCore/qpermissions.h>
#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qtimer.h>

#if defined(Q_OS_MACOS) && defined(QT_BUILD_INTERNAL)
#include <private/qcore_mac_p.h>
Q_CONSTRUCTOR_FUNCTION(qt_mac_ensureResponsible);
#endif

class tst_QPermissions : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase_data();

    void checkPermission();
    void checkPermissionInNonMainThread();

    void requestPermission();
    void requestPermissionInNonMainThread();
};

void tst_QPermissions::initTestCase_data()
{
    QTest::addColumn<QPermission>("permission");

    QTest::newRow("Camera") << QPermission(QCameraPermission{});
    QTest::newRow("Microphone") << QPermission(QMicrophonePermission{});
    QTest::newRow("Bluetooth") << QPermission(QBluetoothPermission{});
    QTest::newRow("Contacts") << QPermission(QContactsPermission{});
    QTest::newRow("Calendar") << QPermission(QCalendarPermission{});
    QTest::newRow("Location") << QPermission(QLocationPermission{});
}

void tst_QPermissions::checkPermission()
{
    QFETCH_GLOBAL(QPermission, permission);
    qApp->checkPermission(permission);
}

class Thread : public QThread
{
public:
    QMutex mutex;
    QWaitCondition cond;
    std::function<void()> function;

    void run() override
    {
        QMutexLocker locker(&mutex);
        function();
        cond.wakeOne();
    }
};

void tst_QPermissions::checkPermissionInNonMainThread()
{
    QFETCH_GLOBAL(QPermission, permission);

    Thread thread;
    thread.function = [=]{
        qApp->checkPermission(permission);
    };

    QVERIFY(!thread.isFinished());
    QMutexLocker locker(&thread.mutex);
    thread.start();
    QVERIFY(!thread.isFinished());
    thread.cond.wait(locker.mutex());
    QVERIFY(thread.wait(1000));
    QVERIFY(thread.isFinished());
}

void tst_QPermissions::requestPermission()
{
    QFETCH_GLOBAL(QPermission, permission);
    QTimer::singleShot(0, this, [=] {
        qApp->requestPermission(permission, [=](auto result) {
            qDebug() << result;
            Q_ASSERT(QThread::currentThread() == thread());
            qApp->exit();
        });
    });
    qApp->exec();
}

void tst_QPermissions::requestPermissionInNonMainThread()
{
    QFETCH_GLOBAL(QPermission, permission);

    QTest::ignoreMessage(QtWarningMsg, "Permissions can only be requested from the GUI (main) thread");

    Thread thread;
    thread.function = [&]{
        qApp->requestPermission(permission, [&]() {});
    };

    QVERIFY(!thread.isFinished());
    QMutexLocker locker(&thread.mutex);
    thread.start();
    QVERIFY(!thread.isFinished());
    thread.cond.wait(locker.mutex());
    QVERIFY(thread.wait(1000));
    QVERIFY(thread.isFinished());
}

QTEST_MAIN(tst_QPermissions)
#include "tst_qpermissions.moc"
