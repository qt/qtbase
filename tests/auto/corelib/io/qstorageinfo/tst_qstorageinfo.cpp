/****************************************************************************
**
** Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <QStorageInfo>

class tst_QStorageInfo : public QObject
{
    Q_OBJECT
private slots:
    void defaultValues();
    void operatorEqual();
#ifndef Q_OS_WINRT
    void operatorNotEqual();
    void root();
    void currentStorage();
    void storageList();
    void tempFile();
    void caching();
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
    QVERIFY(storage.bytesTotal() == -1);
    QVERIFY(storage.bytesFree() == -1);
    QVERIFY(storage.bytesAvailable() == -1);
}

void tst_QStorageInfo::operatorEqual()
{
    {
        QStorageInfo storage1 = QStorageInfo::root();
        QStorageInfo storage2(QDir::rootPath());
        QVERIFY(storage1 == storage2);
    }

    {
        QStorageInfo storage1(QCoreApplication::applicationDirPath());
        QStorageInfo storage2(QCoreApplication::applicationFilePath());
        QVERIFY(storage1 == storage2);
    }

    {
        QStorageInfo storage1;
        QStorageInfo storage2;
        QVERIFY(storage1 == storage2);
    }
}

#ifndef Q_OS_WINRT
void tst_QStorageInfo::operatorNotEqual()
{
    QStorageInfo storage1 = QStorageInfo::root();
    QStorageInfo storage2;
    QVERIFY(storage1 != storage2);
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
    QVERIFY(storage.bytesTotal() >= 0);
    QVERIFY(storage.bytesFree() >= 0);
    QVERIFY(storage.bytesAvailable() >= 0);
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
    QVERIFY(storage.bytesTotal() >= 0);
    QVERIFY(storage.bytesFree() >= 0);
    QVERIFY(storage.bytesAvailable() >= 0);
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

void tst_QStorageInfo::tempFile()
{
    QTemporaryFile file;
    QVERIFY2(file.open(), qPrintable(file.errorString()));

    QStorageInfo storage1(file.fileName());
#ifdef Q_OS_LINUX
    if (storage1.fileSystemType() == "btrfs")
        QSKIP("This test doesn't work on btrfs, probably due to a btrfs bug");
#endif

    qint64 free = storage1.bytesFree();

    file.write(QByteArray(1024*1024, '1'));
    file.flush();
    file.close();

    QStorageInfo storage2(file.fileName());
    QVERIFY(free != storage2.bytesFree());
}

void tst_QStorageInfo::caching()
{
    QTemporaryFile file;
    QVERIFY2(file.open(), qPrintable(file.errorString()));

    QStorageInfo storage1(file.fileName());
#ifdef Q_OS_LINUX
    if (storage1.fileSystemType() == "btrfs")
        QSKIP("This test doesn't work on btrfs, probably due to a btrfs bug");
#endif

    qint64 free = storage1.bytesFree();
    QStorageInfo storage2(storage1);
    QVERIFY(free == storage2.bytesFree());

    file.write(QByteArray(1024*1024, '\0'));
    file.flush();

    QVERIFY(free == storage1.bytesFree());
    QVERIFY(free == storage2.bytesFree());
    storage2.refresh();
    QVERIFY(storage1 == storage2);
    QVERIFY(free != storage2.bytesFree());
}
#endif

QTEST_MAIN(tst_QStorageInfo)

#include "tst_qstorageinfo.moc"
