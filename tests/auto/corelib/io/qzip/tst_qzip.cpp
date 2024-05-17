// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QDebug>
#include <QBuffer>

#include <private/qzipwriter_p.h>
#include <private/qzipreader_p.h>

class tst_QZip : public QObject
{
    Q_OBJECT

private slots:
    void basicUnpack();
    void symlinks();
    void readTest();
    void createArchive();
};

void tst_QZip::basicUnpack()
{
    QZipReader zip(QFINDTESTDATA("/testdata/test.zip"), QIODevice::ReadOnly);
    QList<QZipReader::FileInfo> files = zip.fileInfoList();
    QCOMPARE(files.size(), 2);

    QZipReader::FileInfo fi = files.at(0);
    QVERIFY(fi.isValid());
    QCOMPARE(fi.filePath, QString("test"));
    QCOMPARE(uint(fi.isDir), (uint) 1);
    QCOMPARE(uint(fi.isFile), (uint) 0);
    QCOMPARE(uint(fi.isSymLink), (uint) 0);

    QCOMPARE(fi.permissions,QFile::Permissions(  QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                                                 | QFile::ReadUser  | QFile::WriteUser | QFile::ExeUser   ));

    QCOMPARE(fi.lastModified, QDateTime::fromString("2005.11.11 13:08:02", "yyyy.MM.dd HH:mm:ss"));

    fi = files.at(1);
    QVERIFY(fi.isValid());
    QCOMPARE(fi.filePath, QString("test/test.txt"));
    QCOMPARE(uint(fi.isDir), (uint) 0);
    QCOMPARE(uint(fi.isFile), (uint) 1);
    QCOMPARE(uint(fi.isSymLink), (uint) 0);

    QVERIFY(fi.permissions == QFile::Permissions(  QFile::ReadOwner | QFile::WriteOwner
                                                 | QFile::ReadUser  | QFile::WriteUser ));

    QCOMPARE(fi.lastModified, QDateTime::fromString("2005.11.11 13:08:02", "yyyy.MM.dd HH:mm:ss"));

    QCOMPARE(zip.fileData("test/test.txt"), QByteArray("content\n"));

    fi = zip.entryInfoAt(-1);
    QVERIFY(!fi.isValid());
}

void tst_QZip::symlinks()
{
    QZipReader zip(QFINDTESTDATA("/testdata/symlink.zip"), QIODevice::ReadOnly);
    QList<QZipReader::FileInfo> files = zip.fileInfoList();
    QCOMPARE(files.size(), 2);

    QZipReader::FileInfo fi = files.at(0);
    QVERIFY(fi.isValid());
    QCOMPARE(fi.filePath, QString("symlink"));
    QVERIFY(!fi.isDir);
    QVERIFY(!fi.isFile);
    QVERIFY(fi.isSymLink);

    QCOMPARE(zip.fileData("symlink"), QByteArray("destination"));

    fi = files.at(1);
    QVERIFY(fi.isValid());
    QCOMPARE(fi.filePath, QString("destination"));
    QVERIFY(!fi.isDir);
    QVERIFY(fi.isFile);
    QVERIFY(!fi.isSymLink);
}

void tst_QZip::readTest()
{
    QZipReader zip("foobar.zip", QIODevice::ReadOnly); // non existing file.
    QList<QZipReader::FileInfo> files = zip.fileInfoList();
    QCOMPARE(files.size(), 0);
    QByteArray b = zip.fileData("foobar");
    QCOMPARE(b.size(), 0);
}

void tst_QZip::createArchive()
{
    QBuffer buffer;
    QZipWriter zip(&buffer);
    QByteArray fileContents("simple file contents\nline2\n");
    zip.addFile("My Filename", fileContents);
    zip.close();
    QByteArray zipFile = buffer.buffer();

    // QFile f("createArchiveTest.zip"); f.open(QIODevice::WriteOnly); f.write(zipFile); f.close();

    QBuffer buffer2(&zipFile);
    QZipReader zip2(&buffer2);
    QList<QZipReader::FileInfo> files = zip2.fileInfoList();
    QCOMPARE(files.size(), 1);
    QZipReader::FileInfo file = files.at(0);
    QCOMPARE(file.filePath, QString("My Filename"));
    QCOMPARE(uint(file.isDir), (uint) 0);
    QCOMPARE(uint(file.isFile), (uint) 1);
    QCOMPARE(uint(file.isSymLink), (uint) 0);
    QCOMPARE(file.permissions, QFile::Permissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser) );
    QCOMPARE(file.size, (long long) 27);
    QCOMPARE(zip2.fileData("My Filename"), fileContents);
}

QTEST_MAIN(tst_QZip)
#include "tst_qzip.moc"
