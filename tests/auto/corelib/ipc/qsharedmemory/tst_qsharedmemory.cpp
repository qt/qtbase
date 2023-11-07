// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDebug>
#include <QFile>
#if QT_CONFIG(process)
# include <QProcess>
#endif
#include <QSharedMemory>
#include <QTest>
#include <QThread>
#include <QElapsedTimer>

#include <errno.h>
#include <stdio.h>
#ifdef Q_OS_UNIX
#  include <unistd.h>
#endif

#include "private/qtcore-config_p.h"
#include "../ipctestcommon.h"

#define EXISTING_SIZE 1024

Q_DECLARE_METATYPE(QSharedMemory::SharedMemoryError)
Q_DECLARE_METATYPE(QSharedMemory::AccessMode)

using namespace Qt::StringLiterals;

class tst_QSharedMemory : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

private slots:
    // basics
    void constructor();
    void nativeKey_data();
    void nativeKey();
    void legacyKey_data() { nativeKey_data(); }
    void legacyKey();
    void create_data();
    void create();
    void attach_data();
    void attach();
    void changeKeyType_data() { attach_data(); }
    void changeKeyType();
    void lock();

    // custom edge cases
    void removeWhileAttached();
    void emptyMemory();
    void readOnly();
    void attachBeforeCreate_data();
    void attachBeforeCreate();

    // basics all together
    void simpleProducerConsumer_data();
    void simpleProducerConsumer();
    void simpleDoubleProducerConsumer();

    // with threads
    void simpleThreadedProducerConsumer_data();
    void simpleThreadedProducerConsumer();

    // with processes
    void simpleProcessProducerConsumer_data();
    void simpleProcessProducerConsumer();

    // extreme cases
    void useTooMuchMemory();
    void attachTooMuch();

    // unique keys
    void uniqueKey_data();
    void uniqueKey();

    // legacy
    void createWithSameKey();

protected:
    void remove(const QNativeIpcKey &key);

    QString mangleKey(QStringView key)
    {
        if (key.isEmpty())
            return key.toString();
        return u"tstshm_%1-%2_%3"_s.arg(QCoreApplication::applicationPid())
                .arg(seq).arg(key);
    }

    QNativeIpcKey platformSafeKey(const QString &key)
    {
        QFETCH_GLOBAL(QNativeIpcKey::Type, keyType);
        return QSharedMemory::platformSafeKey(mangleKey(key), keyType);
    }

    QNativeIpcKey rememberKey(const QString &key)
    {
        QNativeIpcKey ipcKey = platformSafeKey(key);
        if (!keys.contains(ipcKey)) {
            keys.append(ipcKey);
            remove(ipcKey);
        }
        return ipcKey;
    }

    QList<QNativeIpcKey> keys;
    QList<QSharedMemory*> jail;
    QSharedMemory *existingSharedMemory = nullptr;
    int seq = 0;

private:
    const QString m_helperBinary = "./producerconsumer_helper";
};

void tst_QSharedMemory::initTestCase()
{
    IpcTestCommon::addGlobalTestRows<QSharedMemory>();
}

void tst_QSharedMemory::init()
{
    QNativeIpcKey key = platformSafeKey("existing");
    existingSharedMemory = new QSharedMemory(key);
    if (!existingSharedMemory->create(EXISTING_SIZE)) {
        QCOMPARE(existingSharedMemory->error(), QSharedMemory::AlreadyExists);
    }
    keys.append(key);
}

void tst_QSharedMemory::cleanup()
{
    delete existingSharedMemory;
    qDeleteAll(jail.begin(), jail.end());
    jail.clear();

    for (int i = 0; i < keys.size(); ++i) {
        QSharedMemory sm(keys.at(i));
        if (!sm.create(1024)) {
            //if (sm.error() != QSharedMemory::KeyError)
            //    qWarning() << "test cleanup: remove failed:" << keys.at(i) << sm.error() << sm.errorString();
            sm.attach();
            sm.detach();
        }
        remove(keys.at(i));
    }
    ++seq;
}

#if QT_CONFIG(posix_shm)
#include <sys/types.h>
#include <sys/mman.h>
#endif
#if QT_CONFIG(sysv_shm)
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

void tst_QSharedMemory::remove(const QNativeIpcKey &key)
{
    // On Unix, the shared memory might exist from a previously failed test
    // or segfault, remove it it does
    if (key.isEmpty())
        return;

    switch (key.type()) {
    case QNativeIpcKey::Type::Windows:
        return;

    case QNativeIpcKey::Type::PosixRealtime:
#if QT_CONFIG(posix_shm)
        if (shm_unlink(QFile::encodeName(key.nativeKey()).constData()) == -1) {
            if (errno != ENOENT) {
                perror("shm_unlink");
                return;
            }
        }
#endif
        return;

    case QNativeIpcKey::Type::SystemV:
        break;
    }

#if QT_CONFIG(sysv_shm)
    // ftok requires that an actual file exists somewhere
    QString fileName = key.nativeKey();
    if (!QFile::exists(fileName)) {
        //qDebug() << "exits failed";
        return;
    }

    int unix_key = ftok(fileName.toLatin1().constData(), int(key.type()));
    if (-1 == unix_key) {
        perror("ftok");
        return;
    }

    int id = shmget(unix_key, 0, 0600);
    if (-1 == id) {
        if (errno != ENOENT)
            perror("shmget");
        return;
    }

    struct shmid_ds shmid_ds;
    if (-1 == shmctl(id, IPC_RMID, &shmid_ds)) {
        perror("shmctl");
        return;
    }

    QFile::remove(fileName);
#endif // Q_OS_WIN
}

/*!
    Tests the default values
 */
void tst_QSharedMemory::constructor()
{
    QSharedMemory sm;
    QVERIFY(!sm.isAttached());
    QVERIFY(!sm.data());
    QCOMPARE(sm.nativeKey(), QString());
    QCOMPARE(sm.nativeIpcKey(), QNativeIpcKey());
    QCOMPARE(sm.size(), 0);
    QCOMPARE(sm.error(), QSharedMemory::NoError);
    QCOMPARE(sm.errorString(), QString());

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(sm.key(), QString());
    QT_WARNING_POP
}

void tst_QSharedMemory::nativeKey_data()
{
    QTest::addColumn<QString>("constructorKey");
    QTest::addColumn<QString>("setKey");
    QTest::addColumn<QString>("setNativeKey");   // only used in the legacyKey test

    QTest::newRow("null, null, null") << QString() << QString() << QString();
    QTest::newRow("one, null, null") << QString("one") << QString() << QString();
    QTest::newRow("null, one, null") << QString() << QString("one") << QString();
    QTest::newRow("null, null, one") << QString() << QString() << QString("one");
    QTest::newRow("one, two, null") << QString("one") << QString("two") << QString();
    QTest::newRow("one, null, two") << QString("one") << QString() << QString("two");
    QTest::newRow("null, one, two") << QString() << QString("one") << QString("two");
    QTest::newRow("one, two, three") << QString("one") << QString("two") << QString("three");
    QTest::newRow("invalid") << QString("o/e") << QString("t/o") << QString("|x");
}

/*!
    Basic key testing
 */
void tst_QSharedMemory::nativeKey()
{
    QFETCH(QString, constructorKey);
    QFETCH(QString, setKey);
    QFETCH(QString, setNativeKey);

    QNativeIpcKey constructorIpcKey = platformSafeKey(constructorKey);
    QNativeIpcKey setIpcKey = platformSafeKey(setKey);

    QSharedMemory sm(constructorIpcKey);
    QCOMPARE(sm.nativeIpcKey(), constructorIpcKey);
    QCOMPARE(sm.nativeKey(), constructorIpcKey.nativeKey());
    sm.setNativeKey(setIpcKey);
    QCOMPARE(sm.nativeIpcKey(), setIpcKey);
    QCOMPARE(sm.nativeKey(), setIpcKey.nativeKey());

    QCOMPARE(sm.isAttached(), false);

    QCOMPARE(sm.error(), QSharedMemory::NoError);
    QCOMPARE(sm.errorString(), QString());
    QVERIFY(!sm.data());
    QCOMPARE(sm.size(), 0);

    QCOMPARE(sm.detach(), false);

    // change the key type
    QNativeIpcKey::Type nextKeyType = IpcTestCommon::nextKeyType(setIpcKey.type());
    if (nextKeyType != setIpcKey.type()) {
        QNativeIpcKey setIpcKey2 = QSharedMemory::platformSafeKey(setKey, nextKeyType);
        sm.setNativeKey(setIpcKey2);

        QCOMPARE(sm.nativeIpcKey(), setIpcKey2);
        QCOMPARE(sm.nativeKey(), setIpcKey2.nativeKey());

        QCOMPARE(sm.isAttached(), false);

        QCOMPARE(sm.error(), QSharedMemory::NoError);
        QCOMPARE(sm.errorString(), QString());
        QVERIFY(!sm.data());
        QCOMPARE(sm.size(), 0);

        QCOMPARE(sm.detach(), false);
    }
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
void tst_QSharedMemory::legacyKey()
{
    QFETCH(QString, constructorKey);
    QFETCH(QString, setKey);
    QFETCH(QString, setNativeKey);

#ifdef Q_OS_QNX
    QSKIP("The legacy native key type is incorrectly set on QNX");
#endif
    QSharedMemory sm(constructorKey);
    QCOMPARE(sm.key(), constructorKey);
    QCOMPARE(sm.nativeKey().isEmpty(), constructorKey.isEmpty());
    sm.setKey(setKey);
    QCOMPARE(sm.key(), setKey);
    QCOMPARE(sm.nativeKey().isEmpty(), setKey.isEmpty());
    sm.setNativeKey(setNativeKey);
    QVERIFY(sm.key().isNull());
    QCOMPARE(sm.nativeKey(), setNativeKey);
    QCOMPARE(sm.isAttached(), false);

    QCOMPARE(sm.error(), QSharedMemory::NoError);
    QCOMPARE(sm.errorString(), QString());
    QVERIFY(!sm.data());
    QCOMPARE(sm.size(), 0);

    QCOMPARE(sm.detach(), false);
}
QT_WARNING_POP

void tst_QSharedMemory::create_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<int>("size");
    QTest::addColumn<bool>("canCreate");
    QTest::addColumn<QSharedMemory::SharedMemoryError>("error");

    QTest::newRow("null key") << QString() << 1024
        << false << QSharedMemory::KeyError;
    QTest::newRow("-1 size") << QString("negsize") << -1
        << false << QSharedMemory::InvalidSize;
    QTest::newRow("nor size") << QString("norsize") << 1024
        << true << QSharedMemory::NoError;
    QTest::newRow("existing") << QString("existing") << EXISTING_SIZE
        << false << QSharedMemory::AlreadyExists;
}

/*!
    Basic create testing
 */
void tst_QSharedMemory::create()
{
    QFETCH(QString, key);
    QFETCH(int, size);
    QFETCH(bool, canCreate);
    QFETCH(QSharedMemory::SharedMemoryError, error);

    QNativeIpcKey nativeKey = rememberKey(key);
    QSharedMemory sm(nativeKey);
    QCOMPARE(sm.create(size), canCreate);
    if (sm.error() != error)
        qDebug() << sm.errorString();
    QCOMPARE(sm.nativeIpcKey(), nativeKey);
    if (canCreate) {
        QCOMPARE(sm.errorString(), QString());
        QVERIFY(sm.data() != 0);
        QVERIFY(sm.size() != 0);
    } else {
        QVERIFY(!sm.data());
        QVERIFY(sm.errorString() != QString());
    }
}

void tst_QSharedMemory::attach_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<bool>("exists");
    QTest::addColumn<QSharedMemory::SharedMemoryError>("error");

    QTest::newRow("null") << QString() << false << QSharedMemory::KeyError;
    QTest::newRow("doesntexists") << QString("doesntexist") << false << QSharedMemory::NotFound;

    QTest::newRow("existing") << QString("existing") << true << QSharedMemory::NoError;
}

/*!
    Basic attach/detach testing
 */
void tst_QSharedMemory::attach()
{
    QFETCH(QString, key);
    QFETCH(bool, exists);
    QFETCH(QSharedMemory::SharedMemoryError, error);

    QNativeIpcKey nativeKey = platformSafeKey(key);
    QSharedMemory sm(nativeKey);
    QCOMPARE(sm.attach(), exists);
    QCOMPARE(sm.isAttached(), exists);
    QCOMPARE(sm.error(), error);
    QCOMPARE(sm.nativeIpcKey(), nativeKey);
    if (exists) {
        QVERIFY(sm.data() != 0);
        QVERIFY(sm.size() != 0);
        QCOMPARE(sm.errorString(), QString());
        QVERIFY(sm.detach());
        // Make sure detach doesn't screw up something and we can't re-attach.
        QVERIFY(sm.attach());
        QVERIFY(sm.data() != 0);
        QVERIFY(sm.size() != 0);
        QVERIFY(sm.detach());
        QCOMPARE(sm.size(), 0);
        QVERIFY(!sm.data());
    } else {
        QVERIFY(!sm.data());
        QCOMPARE(sm.size(), 0);
        QVERIFY(sm.errorString() != QString());
        QVERIFY(!sm.detach());
    }
}

void tst_QSharedMemory::changeKeyType()
{
    QFETCH(QString, key);
    QFETCH(bool, exists);
    QFETCH(QSharedMemory::SharedMemoryError, error);

    QNativeIpcKey nativeKey = platformSafeKey(key);
    QNativeIpcKey::Type nextKeyType = IpcTestCommon::nextKeyType(nativeKey.type());
    if (nextKeyType == nativeKey.type())
        QSKIP("System only supports one key type");
//    qDebug() << "Changing from" << nativeKey.type() << "to" << nextKeyType;

    QSharedMemory sm(nativeKey);
    QCOMPARE(sm.attach(), exists);
    QCOMPARE(sm.error(), error);

    QNativeIpcKey nextKey =
            QSharedMemory::platformSafeKey(mangleKey(key), nextKeyType);
    sm.setNativeKey(nextKey);
    QCOMPARE(sm.isAttached(), false);
    QVERIFY(!sm.attach());

    if (exists)
        QCOMPARE(sm.error(), QSharedMemory::NotFound);
    else
        QCOMPARE(sm.error(), error);
}

void tst_QSharedMemory::lock()
{
    QSharedMemory shm;
    QVERIFY(!shm.lock());
    QCOMPARE(shm.error(), QSharedMemory::LockError);

    shm.setNativeKey(rememberKey(QLatin1String("qsharedmemory")));

    QVERIFY(!shm.lock());
    QCOMPARE(shm.error(), QSharedMemory::LockError);

    QVERIFY(shm.create(100));
    QVERIFY(shm.lock());
    QTest::ignoreMessage(QtWarningMsg, "QSharedMemory::lock: already locked");
    QVERIFY(shm.lock());
    // we didn't unlock(), so ignore the warning from auto-detach in destructor
    QTest::ignoreMessage(QtWarningMsg, "QSharedMemory::lock: already locked");
}

/*!
    Other shared memory are allowed to be attached after we remove,
    but new shared memory are not allowed to attach after a remove.
 */
// HPUX doesn't allow for multiple attaches per process.
void tst_QSharedMemory::removeWhileAttached()
{
    rememberKey("one");

    // attach 1
    QNativeIpcKey keyOne = platformSafeKey("one");
    QSharedMemory *smOne = new QSharedMemory(keyOne);
    QVERIFY(smOne->create(1024));
    QVERIFY(smOne->isAttached());

    // attach 2
    QSharedMemory *smTwo = new QSharedMemory(platformSafeKey("one"));
    QVERIFY(smTwo->attach());
    QVERIFY(smTwo->isAttached());

    // detach 1 and remove, remove one first to catch another error.
    delete smOne;
    delete smTwo;

    if (keyOne.type() == QNativeIpcKey::Type::PosixRealtime) {
        // POSIX IPC doesn't guarantee that the shared memory is removed
        remove(keyOne);
    }

    // three shouldn't be able to attach
    QSharedMemory smThree(keyOne);
    QVERIFY(!smThree.attach());
    QCOMPARE(smThree.error(), QSharedMemory::NotFound);
}

/*!
    The memory should be set to 0 after created.
 */
void tst_QSharedMemory::emptyMemory()
{
    QSharedMemory sm(rememberKey(QLatin1String("voidland")));
    int size = 1024;
    QVERIFY(sm.create(size, QSharedMemory::ReadOnly));
    char *get = (char*)sm.data();
    char null = 0;
    for (int i = 0; i < size; ++i)
        QCOMPARE(get[i], null);
}

/*!
    Verify that attach with ReadOnly is actually read only
    by writing to data and causing a segfault.
*/
void tst_QSharedMemory::readOnly()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#elif defined(Q_OS_MACOS)
    QSKIP("QTBUG-59936: Times out on macOS", SkipAll);
#elif defined(Q_OS_WIN)
    QSKIP("This test opens a crash dialog on Windows.");
#elif defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)
    QSKIP("ASan prevents the crash this test is looking for.", SkipAll);
#else
    QNativeIpcKey key = rememberKey("readonly_segfault");

    // ### on windows disable the popup somehow
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(m_helperBinary, { "readonly_segfault", key.toString() });
    p.waitForFinished();
    QCOMPARE(p.error(), QProcess::Crashed);
#endif
}

void tst_QSharedMemory::attachBeforeCreate_data()
{
    QTest::addColumn<bool>("legacy");

    QTest::addRow("legacy") << true;
    QTest::addRow("non-legacy") << false;
}

void tst_QSharedMemory::attachBeforeCreate()
{
    QFETCH_GLOBAL(const QNativeIpcKey::Type, keyType);
    QFETCH(const bool, legacy);
    const QString keyStr(u"test"_s);
    QNativeIpcKey key;
    if (legacy) {
        key = QSharedMemory::legacyNativeKey(keyStr, keyType);
        // same as rememberKey(), but with legacy
        if (!keys.contains(key)) {
            keys.append(key);
            remove(key);
        }
    } else {
        key = rememberKey(keyStr);
    }
    const qsizetype sz = 100;
    QSharedMemory mem(key);
    QVERIFY(!mem.attach());
    QVERIFY(mem.create(sz));
}

/*!
    Keep making shared memory until the kernel stops us.
 */
void tst_QSharedMemory::useTooMuchMemory()
{
#ifdef Q_OS_LINUX
    bool success = true;
    int count = 0;
    while (success) {
        QString key = QLatin1String("maxmemorytest_") + QString::number(count++);
        QNativeIpcKey nativeKey = rememberKey(key);
        QSharedMemory *sm = new QSharedMemory(nativeKey);
        QVERIFY(sm);
        jail.append(sm);
        int size = 32768 * 1024;
        success = sm->create(size);
        if (!success && sm->error() == QSharedMemory::AlreadyExists) {
            // left over from a crash, clean it up
            sm->attach();
            sm->detach();
            success = sm->create(size);
        }

        if (!success) {
            QVERIFY(!sm->isAttached());
            QCOMPARE(sm->nativeIpcKey(), nativeKey);
            QCOMPARE(sm->size(), 0);
            QVERIFY(!sm->data());
            if (sm->error() != QSharedMemory::OutOfResources)
                qDebug() << sm->error() << sm->errorString();
            // ### Linux won't return OutOfResources if there are not enough semaphores to use.
            QVERIFY(sm->error() == QSharedMemory::OutOfResources
                    || sm->error() == QSharedMemory::LockError);
            QVERIFY(sm->errorString() != QString());
            QVERIFY(!sm->attach());
            QVERIFY(!sm->detach());
        } else {
            QVERIFY(sm->isAttached());
        }
    }
#endif
}

/*!
    Create one shared memory (government) and see how many other shared memories (wars) we can
    attach before the system runs out of resources.
 */
void tst_QSharedMemory::attachTooMuch()
{
    QSKIP("disabled");

    QSharedMemory government(rememberKey("government"));
    QVERIFY(government.create(1024));
    while (true) {
        QSharedMemory *war = new QSharedMemory(government.nativeIpcKey());
        QVERIFY(war);
        jail.append(war);
        if (!war->attach()) {
            QVERIFY(!war->isAttached());
            QCOMPARE(war->nativeIpcKey(), government.nativeIpcKey());
            QCOMPARE(war->size(), 0);
            QVERIFY(!war->data());
            QCOMPARE(war->error(), QSharedMemory::OutOfResources);
            QVERIFY(war->errorString() != QString());
            QVERIFY(!war->detach());
            break;
        } else {
            QVERIFY(war->isAttached());
        }
    }
}

void tst_QSharedMemory::simpleProducerConsumer_data()
{
    QTest::addColumn<QSharedMemory::AccessMode>("mode");

    QTest::newRow("readonly") << QSharedMemory::ReadOnly;
    QTest::newRow("readwrite") << QSharedMemory::ReadWrite;
}

/*!
    The basic consumer producer that rounds out the basic testing.
    If this fails then any muli-threading/process might fail (but be
    harder to debug)

    This doesn't require nor test any locking system.
 */
void tst_QSharedMemory::simpleProducerConsumer()
{
    QFETCH(QSharedMemory::AccessMode, mode);

    rememberKey(QLatin1String("market"));
    QSharedMemory producer(platformSafeKey("market"));
    QSharedMemory consumer(platformSafeKey("market"));
    int size = 512;
    QVERIFY(producer.create(size));
    QVERIFY(consumer.attach(mode));

    char *put = (char*)producer.data();
    char *get = (char*)consumer.data();
    // On Windows CE you always have ReadWrite access. Thus
    // ViewMapOfFile returns the same pointer
    QVERIFY(put != get);
    for (int i = 0; i < size; ++i) {
        put[i] = 'Q';
        QCOMPARE(get[i], 'Q');
    }
    QVERIFY(consumer.detach());
}

void tst_QSharedMemory::simpleDoubleProducerConsumer()
{
    QNativeIpcKey nativeKey = rememberKey(QLatin1String("market"));
    QSharedMemory producer(nativeKey);
    int size = 512;
    QVERIFY(producer.create(size));
    QVERIFY(producer.detach());

    if (nativeKey.type() == QNativeIpcKey::Type::PosixRealtime) {
        // POSIX IPC doesn't guarantee that the shared memory is removed
        remove(nativeKey);
    }

    QVERIFY(producer.create(size));

    {
        QSharedMemory consumer(nativeKey);
        QVERIFY(consumer.attach());
    }
}

class Consumer : public QThread
{
public:
    QNativeIpcKey nativeKey;
    Consumer(const QNativeIpcKey &nativeKey) : nativeKey(nativeKey) {}

    void run() override
    {
        QSharedMemory consumer(nativeKey);
        while (!consumer.attach()) {
            if (consumer.error() != QSharedMemory::NotFound)
                qDebug() << "consumer: failed to connect" << consumer.error() << consumer.errorString();
            QVERIFY(consumer.error() == QSharedMemory::NotFound || consumer.error() == QSharedMemory::KeyError);
            QTest::qWait(1);
        }

        char *memory = (char*)consumer.data();

        int i = 0;
        while (true) {
            if (!consumer.lock())
                break;
            if (memory[0] == 'Q')
                memory[0] = ++i;
            if (memory[0] == 'E') {
                memory[1]++;
                QVERIFY(consumer.unlock());
                break;
            }
            QVERIFY(consumer.unlock());
            QTest::qWait(1);
        }

        QVERIFY(consumer.detach());
    }
};

class Producer : public QThread
{
public:
    Producer(const QNativeIpcKey &nativeKey) : producer(nativeKey)
    {
        int size = 1024;
        if (!producer.create(size)) {
            // left over from a crash...
            if (producer.error() == QSharedMemory::AlreadyExists) {
                producer.attach();
                producer.detach();
                QVERIFY(producer.create(size));
            }
        }
    }

    void run() override
    {

        char *memory = (char*)producer.data();
        memory[1] = '0';
        QElapsedTimer timer;
        timer.start();
        int i = 0;
        while (i < 5 && timer.elapsed() < 5000) {
            QVERIFY(producer.lock());
            if (memory[0] == 'Q') {
                QVERIFY(producer.unlock());
                QTest::qWait(1);
                continue;
            }
            ++i;
            memory[0] = 'Q';
            QVERIFY(producer.unlock());
            QTest::qWait(1);
        }

        // tell everyone to quit
        QVERIFY(producer.lock());
        memory[0] = 'E';
        QVERIFY(producer.unlock());

    }

    QSharedMemory producer;
private:

};

void tst_QSharedMemory::simpleThreadedProducerConsumer_data()
{
    QTest::addColumn<bool>("producerIsThread");
    QTest::addColumn<int>("threads");
    for (int i = 0; i < 5; ++i) {
        QTest::newRow("1 consumer, producer is thread") << true << 1;
        QTest::newRow("1 consumer, producer is this") << false << 1;
        QTest::newRow("5 consumers, producer is thread") << true << 5;
        QTest::newRow("5 consumers, producer is this") << false << 5;
    }
}

/*!
    The basic producer/consumer, but this time using threads.
 */
void tst_QSharedMemory::simpleThreadedProducerConsumer()
{
    QFETCH(bool, producerIsThread);
    QFETCH(int, threads);
    QNativeIpcKey nativeKey = rememberKey(QLatin1String("market"));

    Producer p(nativeKey);
    QVERIFY(p.producer.isAttached());
    if (producerIsThread)
        p.start();

    QList<Consumer*> consumers;
    for (int i = 0; i < threads; ++i) {
        consumers.append(new Consumer(nativeKey));
        consumers.last()->start();
    }

    if (!producerIsThread)
        p.run();

    p.wait(5000);
    while (!consumers.isEmpty()) {
        Consumer *c = consumers.first();
        QVERIFY(c->isFinished() || c->wait(5000));
        delete consumers.takeFirst();
    }
}

void tst_QSharedMemory::simpleProcessProducerConsumer_data()
{
#if QT_CONFIG(process)
    QTest::addColumn<int>("processes");
    int tries = 5;
    for (int i = 0; i < tries; ++i) {
        QTest::newRow("1 process") << 1;
        QTest::newRow("5 processes") << 5;
    }
#endif
}

/*!
    Create external processes that produce and consume.
 */
void tst_QSharedMemory::simpleProcessProducerConsumer()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
    QFETCH(int, processes);

    QSKIP("This test is unstable: QTBUG-25655");

    QNativeIpcKey nativeKey = rememberKey("market");

    QProcess producer;
    producer.start(m_helperBinary, { "producer", nativeKey.toString() });
    QVERIFY2(producer.waitForStarted(), "Could not start helper binary");
    QVERIFY2(producer.waitForReadyRead(), "Helper process failed to create shared memory segment: " +
             producer.readAllStandardError());

    QList<QProcess*> consumers;
    unsigned int failedProcesses = 0;
    QStringList consumerArguments = { "consumer", nativeKey.toString() };
    for (int i = 0; i < processes; ++i) {
        QProcess *p = new QProcess;
        p->setProcessChannelMode(QProcess::ForwardedChannels);
        p->start(m_helperBinary, consumerArguments);
        if (p->waitForStarted(2000))
            consumers.append(p);
        else
            ++failedProcesses;
    }

    bool consumerFailed = false;

    while (!consumers.isEmpty()) {
        QVERIFY(consumers.first()->waitForFinished(3000));
        if (consumers.first()->state() == QProcess::Running ||
            consumers.first()->exitStatus() != QProcess::NormalExit ||
            consumers.first()->exitCode() != 0) {
                consumerFailed = true;
            }
        delete consumers.takeFirst();
    }
    QCOMPARE(consumerFailed, false);
    QCOMPARE(failedProcesses, (unsigned int)(0));

    // tell the producer to exit now
    producer.write("", 1);
    producer.waitForBytesWritten();
    QVERIFY(producer.waitForFinished(5000));
#endif
}

void tst_QSharedMemory::uniqueKey_data()
{
    QTest::addColumn<QString>("key1");
    QTest::addColumn<QString>("key2");

    QTest::newRow("null == null") << QString() << QString();
    QTest::newRow("key == key") << QString("key") << QString("key");
    QTest::newRow("key1 == key1") << QString("key1") << QString("key1");
    QTest::newRow("key != key1") << QString("key") << QString("key1");
    QTest::newRow("ke1y != key1") << QString("ke1y") << QString("key1");
    QTest::newRow("key1 != key2") << QString("key1") << QString("key2");
    QTest::newRow("Noël -> Nol") << QString::fromUtf8("N\xc3\xabl") << QString("Nol");
}

void tst_QSharedMemory::uniqueKey()
{
    QFETCH(QString, key1);
    QFETCH(QString, key2);

    QSharedMemory sm1(platformSafeKey(key1));
    QSharedMemory sm2(platformSafeKey(key2));

    bool setEqual = (key1 == key2);
    bool keyEqual = (sm1.nativeIpcKey() == sm2.nativeIpcKey());
    bool nativeEqual = (sm1.nativeKey() == sm2.nativeKey());

    QCOMPARE(keyEqual, setEqual);
    QCOMPARE(nativeEqual, setEqual);
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
void tst_QSharedMemory::createWithSameKey()
{
    const QString key = u"legacy_key"_s;
    const qsizetype sz = 100;
    QSharedMemory mem1(key);
    QVERIFY(mem1.create(sz));

    {
        QSharedMemory mem2(key);
        QVERIFY(!mem2.create(sz));
        QVERIFY(mem2.attach());
    }
    // and the second create() should fail as well, QTBUG-111855
    {
        QSharedMemory mem2(key);
        QVERIFY(!mem2.create(sz));
        QVERIFY(mem2.attach());
    }
}
QT_WARNING_POP

QTEST_MAIN(tst_QSharedMemory)
#include "tst_qsharedmemory.moc"

