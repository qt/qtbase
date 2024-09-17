// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>
#include <QSet>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTemporaryFile>
#include "private/qemulationdetector_p.h"

#include <cstdio>

#include <stdarg.h>

#ifdef Q_OS_WIN
#  include <io.h>       // _get_osfhandle
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "../../../../manual/qstorageinfo/printvolumes.cpp"

#if defined(Q_OS_LINUX) && defined(QT_BUILD_INTERNAL)
#  include "../../../../../src/corelib/io/qstorageinfo_linux_p.h"
#endif

using namespace Qt::StringLiterals;

class tst_QStorageInfo : public QObject
{
    Q_OBJECT
private slots:
    void defaultValues();
    void dump();
    void compareCompiles();
    void operatorEqual();
    void operatorNotEqual();
    void root();
    void currentStorage();
    void storageList_data();
    void storageList();
    void freeSpaceUpdate();

#if defined(Q_OS_LINUX) && defined(QT_BUILD_INTERNAL)
    void testParseMountInfo_data();
    void testParseMountInfo();
    void testParseMountInfo_filtered_data();
    void testParseMountInfo_filtered();
#endif
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
    const int n = std::vsnprintf(buf + bufuse, sizeof(buf) - bufuse, format, ap);
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

void tst_QStorageInfo::compareCompiles()
{
    QTestPrivate::testEqualityOperatorsCompile<QStorageInfo>();
}

void tst_QStorageInfo::operatorEqual()
{
    {
        QStorageInfo storage1 = QStorageInfo::root();
        QStorageInfo storage2(QDir::rootPath());
        QT_TEST_EQUALITY_OPS(storage1, storage2, true);
    }

    {
        QStorageInfo storage1(QCoreApplication::applicationDirPath());
        QStorageInfo storage2(QCoreApplication::applicationFilePath());
        QT_TEST_EQUALITY_OPS(storage1, storage2, true);
    }

    {
        QStorageInfo storage1;
        QStorageInfo storage2;
        QT_TEST_EQUALITY_OPS(storage1, storage2, true);
    }

    // Test copy ctor
    {
        QStorageInfo storage1 = QStorageInfo::root();
        QStorageInfo storage2(storage1);
        QT_TEST_EQUALITY_OPS(storage1, storage2, true);
    }
}

void tst_QStorageInfo::operatorNotEqual()
{
    QStorageInfo storage1 = QStorageInfo::root();
    QStorageInfo storage2;
    QT_TEST_EQUALITY_OPS(storage1, storage2, false);
}

void tst_QStorageInfo::root()
{
    QStorageInfo storage = QStorageInfo::root();

    QVERIFY(storage.isValid());
    QVERIFY(storage.isReady());
    QCOMPARE(storage.rootPath(), QDir::rootPath());
    QVERIFY(storage.isRoot());
#ifndef Q_OS_WASM
    QVERIFY(!storage.device().isEmpty());
    QVERIFY(!storage.fileSystemType().isEmpty());
#endif
#ifndef Q_OS_HAIKU
    QCOMPARE_GE(storage.bytesTotal(), 0);
    QCOMPARE_GE(storage.bytesFree(), 0);
    QCOMPARE_GE(storage.bytesAvailable(), 0);
#endif
}

void tst_QStorageInfo::currentStorage()
{
    QString appPath = QCoreApplication::applicationFilePath();
    if (appPath.isEmpty())
        QSKIP("No applicationFilePath(), cannot test");

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

void tst_QStorageInfo::storageList_data()
{
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("QEMU appears not to emulate the system calls correctly.");

    QStorageInfo root = QStorageInfo::root();

    QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();

    // at least, root storage should be present
    QVERIFY(volumes.contains(root));
    volumes.removeOne(root);
    QVERIFY(!volumes.contains(root));

    if (volumes.isEmpty())
        QSKIP("Only the root volume was mounted on this system");

    QTest::addColumn<QStorageInfo>("storage");
    QSet<QString> seenRoots;
    for (const QStorageInfo &storage : std::as_const(volumes)) {
        if (!storage.isReady())
            continue;
        QString rootPath = storage.rootPath();
        if (seenRoots.contains(rootPath))
            qInfo() << rootPath << "is mounted over; QStorageInfo may be unpredictable";
        else
            QTest::newRow(qUtf8Printable(rootPath)) << storage;
        seenRoots.insert(rootPath);
    }
}

void tst_QStorageInfo::storageList()
{
        QFETCH(QStorageInfo, storage);

        QVERIFY(storage.isValid());
        QVERIFY(!storage.isRoot());
#ifndef Q_OS_WIN
        QVERIFY(!storage.device().isEmpty());
        QVERIFY(!storage.fileSystemType().isEmpty());
#endif

        QStorageInfo other(storage.rootPath());
        QVERIFY(other.isValid());
        QCOMPARE(other.rootPath(), storage.rootPath());
        QCOMPARE(other.device(), storage.device());
        QCOMPARE(other.subvolume(), storage.subvolume());
        QCOMPARE(other.fileSystemType(), storage.fileSystemType());
        QCOMPARE(other.name(), storage.name());
        QCOMPARE(other.displayName(), storage.displayName());

        QCOMPARE(other.bytesTotal(), storage.bytesTotal());
        QCOMPARE(other.blockSize(), storage.blockSize());
        // not comparing free space because it may have changed

        QCOMPARE(other.isRoot(), storage.isRoot());
        QCOMPARE(other.isReadOnly(), storage.isReadOnly());
        QCOMPARE(other.isReady(), storage.isReady());
}

static QString suitableDirectoryForWriting()
{
    std::initializer_list<const char *> inadvisableFs = {
#ifdef Q_OS_LINUX
        // See comment below. If we can get a tmpfs, let's try it.
        "btrfs",
        "xfs",
#endif
    };

    QString tempDir = QDir::tempPath();
    QString fsType = QStorageInfo(tempDir).fileSystemType();
    if (std::find(std::begin(inadvisableFs), std::end(inadvisableFs), fsType)
            != std::end(inadvisableFs)) {
        // the RuntimeLocation on Linux is almost always a tmpfs
        QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if (!runtimeDir.isEmpty())
            return runtimeDir;
    }

    return tempDir;
}

void tst_QStorageInfo::freeSpaceUpdate()
{
    // Some filesystems don't update the free space unless we ask that the OS
    // flush its buffers to disk and even then the update may not be entirely
    // synchronous. So we always ask the OS to flush them and we may keep
    // trying to write until the free space changes (with a maximum so we don't
    // exhaust and cause other problems).
    //
    // In the past, we had this issue with APFS (Apple systems), BTRFS and XFS
    // (Linux). Current testing is that APFS and XFS always succeed after the
    // first block is written and BTRFS almost always by the second block.

    auto flushAndSync = [](QFile &file) {
        file.flush();

#ifdef Q_OS_WIN
        FlushFileBuffers(HANDLE(_get_osfhandle(file.handle())));
#elif _POSIX_VERSION >= 200112L
        fsync(file.handle());
# ifndef Q_OS_VXWORKS
        sync();
# endif // Q_OS_VXWORKS
#endif
    };

    QTemporaryFile file(suitableDirectoryForWriting() + "/tst_qstorageinfo.XXXXXX");
    QVERIFY2(file.open(), qPrintable(file.errorString()));

    QStorageInfo storage1(file.fileName());
    qInfo() << "Testing on" << storage1;

    qint64 free = storage1.bytesFree();
    QStorageInfo storage2(storage1);
    QCOMPARE(free, storage2.bytesFree());
    QCOMPARE_NE(free, -1);

    // let's see if we can make it change
    QByteArray block(1024 * 1024 / 2, '\0');

    // let's try and keep to less than ~10% of the free disk space
    int maxIterations = 25;
    if (free < 256 * block.size())
        maxIterations = free / 10 / block.size();
    if (maxIterations == 0)
        QSKIP("Not enough free disk space to continue");

    file.write(block);
    flushAndSync(file);
    for (int i = 0; i < maxIterations; ++i) {
        QStorageInfo storage3(file.fileName());
        qint64 nowFree = storage3.bytesFree();
        if (nowFree != free)
            break;

        // grow some more
        file.write(block);
        flushAndSync(file);
    }
    // qDebug() << "Needed to grow" << file.fileName() << "to" << file.size();

    QCOMPARE(storage1, storage2);
    QCOMPARE(free, storage1.bytesFree());
    QCOMPARE(free, storage2.bytesFree());
    storage2.refresh();
    QCOMPARE(storage1, storage2);

#ifndef Q_OS_WASM
    QCOMPARE_NE(free, storage2.bytesFree());
#endif
}

#if defined(Q_OS_LINUX) && defined(QT_BUILD_INTERNAL)
void tst_QStorageInfo::testParseMountInfo_data()
{
    QTest::addColumn<QByteArray>("line");
    QTest::addColumn<MountInfo>("expected");

    QTest::newRow("tmpfs")
        << "17 25 0:18 / /dev rw,nosuid,relatime shared:2 - tmpfs tmpfs rw,seclabel,mode=755\n"_ba
        << MountInfo{"/dev", "tmpfs", "tmpfs", "", makedev(0, 18), 17};
    QTest::newRow("proc")
        << "23 66 0:21 / /proc rw,nosuid,nodev,noexec,relatime shared:12 - proc proc rw\n"_ba
        << MountInfo{"/proc", "proc", "proc", "", makedev(0, 21), 23};

    // E.g. on Android
    QTest::newRow("rootfs")
        << "618 618 0:1 / / ro,relatime master:1 - rootfs rootfs ro,seclabel\n"_ba
        << MountInfo{"/", "rootfs", "rootfs", "", makedev(0, 1), 618};

    QTest::newRow("ext4")
        << "47 66 8:3 / /home rw,relatime shared:50 - ext4 /dev/sda3 rw,stripe=32736\n"_ba
        << MountInfo{"/home", "ext4", "/dev/sda3", "", makedev(8, 3), 47};

    QTest::newRow("empty-optional-field")
        << "23 25 0:22 / /apex rw,nosuid,nodev,noexec,relatime - tmpfs tmpfs rw,seclabel,mode=755\n"_ba
        << MountInfo{"/apex", "tmpfs", "tmpfs", "", makedev(0, 22), 23};

    QTest::newRow("one-optional-field")
        << "47 66 8:3 / /home rw,relatime shared:50 - ext4 /dev/sda3 rw,stripe=32736\n"_ba
        << MountInfo{"/home", "ext4", "/dev/sda3", "", makedev(8, 3), 47};

    QTest::newRow("multiple-optional-fields")
        << "47 66 8:3 / /home rw,relatime shared:142 master:111 - ext4 /dev/sda3 rw,stripe=32736\n"_ba
        << MountInfo{"/home", "ext4", "/dev/sda3", "", makedev(8, 3), 47};

    QTest::newRow("mountdir-with-utf8")
        << "129 66 8:51 / /mnt/lab\xC3\xA9l rw,relatime shared:234 - ext4 /dev/sdd3 rw\n"_ba
        << MountInfo{"/mnt/labél", "ext4", "/dev/sdd3", "", makedev(8, 51), 129};

    QTest::newRow("mountdir-with-space")
        << "129 66 8:51 / /mnt/labe\\040l rw,relatime shared:234 - ext4 /dev/sdd3 rw\n"_ba
        << MountInfo{"/mnt/labe l", "ext4", "/dev/sdd3", "", makedev(8, 51), 129};

    QTest::newRow("mountdir-with-tab")
        << "129 66 8:51 / /mnt/labe\\011l rw,relatime shared:234 - ext4 /dev/sdd3 rw\n"_ba
        << MountInfo{"/mnt/labe\tl", "ext4", "/dev/sdd3", "", makedev(8, 51), 129};

    QTest::newRow("mountdir-with-backslash")
        << "129 66 8:51 / /mnt/labe\\134l rw,relatime shared:234 - ext4 /dev/sdd3 rw\n"_ba
        << MountInfo{"/mnt/labe\\l", "ext4", "/dev/sdd3", "", makedev(8, 51), 129};

    QTest::newRow("mountdir-with-newline")
        << "129 66 8:51 / /mnt/labe\\012l rw,relatime shared:234 - ext4 /dev/sdd3 rw\n"_ba
        << MountInfo{"/mnt/labe\nl", "ext4", "/dev/sdd3", "", makedev(8, 51), 129};

    QTest::newRow("btrfs-subvol")
        << "775 503 0:49 /foo/bar / rw,relatime shared:142 master:111 - btrfs "
           "/dev/mapper/vg0-stuff rw,ssd,discard,space_cache,subvolid=272,subvol=/foo/bar\n"_ba
        << MountInfo{"/", "btrfs", "/dev/mapper/vg0-stuff", "/foo/bar", makedev(0, 49), 775};

    QTest::newRow("bind-mount")
        << "59 47 8:17 /rpmbuild /home/user/rpmbuild rw,relatime shared:48 - ext4 /dev/sdb1 rw\n"_ba
        << MountInfo{"/home/user/rpmbuild", "ext4", "/dev/sdb1", "/rpmbuild", makedev(8, 17), 59};

    QTest::newRow("space-dash-space")
        << "47 66 8:3 / /home\\040-\\040dir rw,relatime shared:50 - ext4 /dev/sda3 rw,stripe=32736\n"_ba
        << MountInfo{"/home - dir", "ext4", "/dev/sda3", "", makedev(8, 3), 47};

    QTest::newRow("btrfs-mount-bind-file")
        << "1799 1778 0:49 "
            "/var_lib_docker/containers/81fde0fec3dd3d99765c3f7fd9cf1ab121b6ffcfd05d5d7ff434db933fe9d795/resolv.conf "
            "/etc/resolv.conf rw,relatime - btrfs /dev/mapper/vg0-stuff "
            "rw,ssd,discard,space_cache,subvolid=1773,subvol=/var_lib_docker\n"_ba
        << MountInfo{"/etc/resolv.conf", "btrfs", "/dev/mapper/vg0-stuff",
                     "/var_lib_docker/containers/81fde0fec3dd3d99765c3f7fd9cf1ab121b6ffcfd05d5d7ff434db933fe9d795/resolv.conf",
                     makedev(0, 49), 1799};

    QTest::newRow("very-long-line-QTBUG-77059")
        << "727 26 0:52 / "
           "/var/lib/docker/overlay2/f3fbad5eedef71145f00729f0826ea8c44defcfec8c92c58aee0aa2c5ea3fa3a/merged "
           "rw,relatime shared:399 - overlay overlay "
           "rw,lowerdir=/var/lib/docker/overlay2/l/PUP2PIY4EQLAOEDQOZ56BHVE53:"
           "/var/lib/docker/overlay2/l/6IIID3C6J3SUXZEA3GJXKQSTLD:"
           "/var/lib/docker/overlay2/l/PA6N6URNR7XDBBGGOSFWSFQ2CG:"
           "/var/lib/docker/overlay2/l/5EOMBTZNCPOCE4LM3I4JCTNSTT:"
           "/var/lib/docker/overlay2/l/DAMINQ46P3LKX2GDDDIWQKDIWC:"
           "/var/lib/docker/overlay2/l/DHR3N57AEH4OG5QER5XJW2LXIN:"
           "/var/lib/docker/overlay2/l/NW26KA7QPRS2KSVQI77QJWLMHW,"
           "upperdir=/var/lib/docker/overlay2/f3fbad5eedef71145f00729f0826ea8c44defcfec8c92c58aee0aa2c5ea3fa3a/diff,"
           "workdir=/var/lib/docker/overlay2/f3fbad5eedef71145f00729f0826ea8c44defcfec8c92c58aee0aa2c5ea3fa3a/work,"
           "index=off,xino=off\n"_ba
        << MountInfo{"/var/lib/docker/overlay2/f3fbad5eedef71145f00729f0826ea8c44defcfec8c92c58aee0aa2c5ea3fa3a/merged",
                     "overlay", "overlay", "", makedev(0, 52), 727};

    QTest::newRow("sshfs-src-device-not-start-with-slash")
        << "128 92 0:64 / /mnt-point rw,nosuid,nodev,relatime shared:234 - "
           "fuse.sshfs admin@192.168.1.2:/storage/emulated/0 rw,user_id=1000,group_id=1000\n"_ba
        << MountInfo{"/mnt-point", "fuse.sshfs",
                     "admin@192.168.1.2:/storage/emulated/0", "", makedev(0, 64), 128};
}

void tst_QStorageInfo::testParseMountInfo()
{
    QFETCH(QByteArray, line);
    QFETCH(MountInfo, expected);

    const std::vector<MountInfo> result = doParseMountInfo(line);
    QVERIFY(!result.empty());
    const MountInfo &a = result.front();
    QCOMPARE(a.mountPoint, expected.mountPoint);
    QCOMPARE(a.fsType, expected.fsType);
    QCOMPARE(a.device, expected.device);
    QCOMPARE(a.fsRoot, expected.fsRoot);
    QCOMPARE(a.stDev, expected.stDev);
    QCOMPARE(a.mntid, expected.mntid);
}

void tst_QStorageInfo::testParseMountInfo_filtered_data()
{
    QTest::addColumn<QByteArray>("line");

    QTest::newRow("proc")
        << "23 66 0:21 / /proc rw,nosuid,nodev,noexec,relatime shared:12 - proc proc rw\n"_ba;

    QTest::newRow("sys")
        << "24 66 0:22 / /sys rw,nosuid,nodev,noexec,relatime shared:2 - sysfs sysfs rw\n"_ba;
    QTest::newRow("sys-kernel")
        << "26 24 0:6 / /sys/kernel/security rw,nosuid,nodev,noexec,relatime "
           "shared:3 - securityfs securityfs rw\n"_ba;

    QTest::newRow("dev")
        << "25 66 0:5 / /dev rw,nosuid shared:8 - devtmpfs devtmpfs "
           "rw,size=4096k,nr_inodes=8213017,mode=755,inode64\n"_ba;
    QTest::newRow("dev-shm")
            << "27 25 0:23 / /dev/shm rw,nosuid,nodev shared:9 - tmpfs tmpfs rw,inode64\n"_ba;

    QTest::newRow("var-run")
        << "46 28 0:25 / /var/run rw,nosuid,nodev,noexec,relatime shared:1 - "
           "tmpfs tmpfs rw,size=32768k,mode=755,inode64\n"_ba;
    QTest::newRow("var-lock")
        << "46 28 0:25 / /var/lock rw,nosuid,nodev,noexec,relatime shared:1 - "
           "tmpfs tmpfs rw,size=32768k,mode=755,inode64\n"_ba;
}
void tst_QStorageInfo::testParseMountInfo_filtered()
{
    QFETCH(QByteArray, line);
    QVERIFY(doParseMountInfo(line, FilterMountInfo::Filtered).empty());
}

#endif // Q_OS_LINUX

QTEST_MAIN(tst_QStorageInfo)

#include "tst_qstorageinfo.moc"
