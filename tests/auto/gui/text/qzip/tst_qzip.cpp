/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QDebug>
#include <private/qzipwriter_p.h>
#include <private/qzipreader_p.h>

class tst_QZip : public QObject
{
    Q_OBJECT
public slots:
    void init();
    void cleanup();

private slots:
    void basicUnpack();
    void symlinks();
    void readTest();
    void createArchive();
};

void tst_QZip::init()
{
}

void tst_QZip::cleanup()
{
}

void tst_QZip::basicUnpack()
{
    QZipReader zip(QString(SRCDIR) + "/testdata/test.zip", QIODevice::ReadOnly);
    QList<QZipReader::FileInfo> files = zip.fileInfoList();
    QCOMPARE(files.count(), 2);

    QZipReader::FileInfo fi = files.at(0);
    QVERIFY(fi.isValid());
    QCOMPARE(fi.filePath, QString("test/"));
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
    QZipReader zip(QString(SRCDIR) + "/testdata/symlink.zip", QIODevice::ReadOnly);
    QList<QZipReader::FileInfo> files = zip.fileInfoList();
    QCOMPARE(files.count(), 2);

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
    QCOMPARE(files.count(), 0);
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
    QCOMPARE(files.count(), 1);
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
