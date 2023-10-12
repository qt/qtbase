// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTemporaryFile>

#include <stdarg.h>

#include "../../../../manual/qstorageinfo/printvolumes.cpp"

class tst_QStorageInfo : public QObject
{
    Q_OBJECT
private slots:
    void defaultValues();
    void dump();
    void operatorEqual();
    void operatorNotEqual();
    void root();
    void currentStorage();
    void storageList();
    void tempFile();
    void caching();
};

void tst_QStorageInfo::defaultValues()
{
    QStorageInfo storage;

    QVERIFY(!storage.isValid());
    QVERIFY(!storage.isReady());
    QVERIFY(storage.rootPath().isEmpty());
    QVERIFY(!storage.isRoot());
    QVERIFY(storage.device().isEmpty());
    QVERIFY(storage.fileSystemType().isEmpty());
    QCOMPARE(storage.bytesTotal(), -1);
    QCOMPARE(storage.bytesFree(), -1);
    QCOMPARE(storage.bytesAvailable(), -1);
}

static int qInfoPrinter(const char *format, ...)
{
    static char buf[1024];
    static size_t bufuse = 0;

    va_list ap;
    va_start(ap, format); // use variable arg list
    int n = qvsnprintf(buf + bufuse, sizeof(buf) - bufuse, format, ap);
    va_end(ap);

    bufuse += n;
    if (bufuse >= sizeof(buf) - 1 || format[strlen(format) - 1] == '\n') {
        // flush
        QtMessageHandler qt_message_print = qInstallMessageHandler(0);
        qInstallMessageHandler(qt_message_print);   // restore the handler
        qt_message_print(QtInfoMsg, QMessageLogContext(), QString::fromLocal8Bit(buf).trimmed());
        bufuse = 0;
    }

    return 1;
}

void tst_QStorageInfo::dump()
{
    printVolumes(QStorageInfo::mountedVolumes(), qInfoPrinter);
}

void tst_QStorageInfo::operatorEqual()
{
    {
        QStorageInfo storage1 = QStorageInfo::root();
        QStorageInfo storage2(QDir::rootPath());
        QCOMPARE(storage1, storage2);
    }

    {
        QStorageInfo storage1(QCoreApplication::applicationDirPath());
        QStorageInfo storage2(QCoreApplication::applicationFilePath());
        QCOMPARE(storage1, storage2);
    }

    {
        QStorageInfo storage1;
        QStorageInfo storage2;
        QCOMPARE(storage1, storage2);
    }

    // Test copy ctor
    {
        QStorageInfo storage1 = QStorageInfo::root();
        QStorageInfo storage2(storage1);
        QCOMPARE(storage1, storage2);
    }
}

void tst_QStorageInfo::operatorNotEqual()
{
    QStorageInfo storage1 = QStorageInfo::root();
    QStorageInfo storage2;
    QCOMPARE_NE(storage1, storage2);
}

void tst_QStorageInfo::root()
{
    QStorageInfo storage = QStorageInfo::root();

    QVERIFY(storage.isValid());
    QVERIFY(storage.isReady());
    QCOMPARE(storage.rootPath(), QDir::rootPath());
    QVERIFY(storage.isRoot());
    QVERIFY(!storage.device().isEmpty());
    QVERIFY(!storage.fileSystemType().isEmpty());
#ifndef Q_OS_HAIKU
    QCOMPARE_GE(storage.bytesTotal(), 0);
    QCOMPARE_GE(storage.bytesFree(), 0);
    QCOMPARE_GE(storage.bytesAvailable(), 0);
#endif
}

void tst_QStorageInfo::currentStorage()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QStorageInfo storage(appPath);
    QVERIFY(storage.isValid());
    QVERIFY(storage.isReady());
    QVERIFY(appPath.startsWith(storage.rootPath(), Qt::CaseInsensitive));
    QVERIFY(!storage.device().isEmpty());
    QVERIFY(!storage.fileSystemType().isEmpty());
    QCOMPARE_GE(storage.bytesTotal(), 0);
    QCOMPARE_GE(storage.bytesFree(), 0);
    QCOMPARE_GE(storage.bytesAvailable(), 0);
}

void tst_QStorageInfo::storageList()
{
    QStorageInfo root = QStorageInfo::root();

    QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();

    // at least, root storage should be present
    QVERIFY(volumes.contains(root));
    volumes.removeOne(root);
    QVERIFY(!volumes.contains(root));

    foreach (const QStorageInfo &storage, volumes) {
        if (!storage.isReady())
            continue;

        QVERIFY(storage.isValid());
        QVERIFY(!storage.isRoot());
#ifndef Q_OS_WIN
        QVERIFY(!storage.device().isEmpty());
        QVERIFY(!storage.fileSystemType().isEmpty());
#endif
    }
}

static bool checkFilesystemGoodForWriting(QTemporaryFile &file, QStorageInfo &storage)
{
#ifdef Q_OS_LINUX
    auto reconstructAt = [](auto *where, auto &&... how) {
        // it's very difficult to convince QTemporaryFile to change the path...
        std::destroy_at(where);
        q20::construct_at(where, std::forward<decltype(how)>(how)...);
    };
    if (storage.fileSystemType() == "btrfs") {
        // let's see if we can find another, writable FS
        QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if (!runtimeDir.isEmpty()) {
            reconstructAt(&file, runtimeDir + "/XXXXXX");
            if (file.open()) {
                storage.setPath(file.fileName());
                if (storage.fileSystemType() != "btrfs")
                    return true;
            }
        }
        QTest::qSkip("btrfs does not synchronously update free space; this test would fail",
                     __FILE__, __LINE__);
        return false;
    }
#else
    Q_UNUSED(file);
    Q_UNUSED(storage);
#endif
    return true;
}

void tst_QStorageInfo::tempFile()
{
    QTemporaryFile file;
    QVERIFY2(file.open(), qPrintable(file.errorString()));

    QStorageInfo storage1(file.fileName());
    if (!checkFilesystemGoodForWriting(file, storage1))
        return;

    qint64 free = storage1.bytesFree();
    QCOMPARE_NE(free, -1);

    file.write(QByteArray(1024*1024, '1'));
    file.flush();
    file.close();

    QStorageInfo storage2(file.fileName());
    if (free == storage2.bytesFree() && storage2.fileSystemType() == "apfs") {
        QEXPECT_FAIL("", "This test is likely to fail on APFS", Continue);
    }

    QCOMPARE_NE(free, storage2.bytesFree());
}

void tst_QStorageInfo::caching()
{
    QTemporaryFile file;
    QVERIFY2(file.open(), qPrintable(file.errorString()));

    QStorageInfo storage1(file.fileName());
    if (!checkFilesystemGoodForWriting(file, storage1))
        return;

    qint64 free = storage1.bytesFree();
    QStorageInfo storage2(storage1);
    QCOMPARE(free, storage2.bytesFree());
    QCOMPARE_NE(free, -1);

    file.write(QByteArray(1024*1024, '\0'));
    file.flush();

    QCOMPARE(free, storage1.bytesFree());
    QCOMPARE(free, storage2.bytesFree());
    storage2.refresh();
    QCOMPARE(storage1, storage2);
    if (free == storage2.bytesFree() && storage2.fileSystemType() == "apfs") {
        QEXPECT_FAIL("", "This test is likely to fail on APFS", Continue);
    }
    QCOMPARE_NE(free, storage2.bytesFree());
}

QTEST_MAIN(tst_QStorageInfo)

#include "tst_qstorageinfo.moc"
