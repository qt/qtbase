/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <QtCore/private/qfilesystementry_p.h>

class tst_QFileSystemEntry : public QObject
{
    Q_OBJECT

private slots:
    void getSetCheck_data();
    void getSetCheck();
    void suffix_data();
    void suffix();
    void completeSuffix_data();
    void completeSuffix();
    void baseName_data();
    void baseName();
    void completeBaseName_data();
    void completeBaseName();
#if defined(Q_OS_WIN)
    void absoluteOrRelative_data();
    void absoluteOrRelative();
#endif
    void isClean_data();
    void isClean();
    void defaultCtor();
};

#if defined(Q_OS_WIN)
void tst_QFileSystemEntry::getSetCheck_data()
{
    QTest::addColumn<QString>("nativeFilePath");
    QTest::addColumn<QString>("internalnativeFilePath");
    QTest::addColumn<QString>("filepath");
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("baseName");
    QTest::addColumn<QString>("completeBasename");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<QString>("completeSuffix");
    QTest::addColumn<bool>("absolute");
    QTest::addColumn<bool>("relative");

    QString absPrefix = QLatin1String("\\\\?\\");
    QString relPrefix = absPrefix
                + QDir::toNativeSeparators(QDir::currentPath())
                + QLatin1String("\\");

    QTest::newRow("simple")
            << QString("A:\\home\\qt\\in\\a\\dir.tar.gz")
            << absPrefix +  QString("A:\\home\\qt\\in\\a\\dir.tar.gz")
            << "A:/home/qt/in/a/dir.tar.gz"
            << "dir.tar.gz" << "dir" << "dir.tar" << "gz" << "tar.gz" << true << false;

    QTest::newRow("relative")
            << QString("in\\a\\dir.tar.gz")
            << relPrefix +  QString("in\\a\\dir.tar.gz")
            << "in/a/dir.tar.gz"
            << "dir.tar.gz" << "dir" << "dir.tar" << "gz" << "tar.gz" << false <<true;

    QTest::newRow("noSuffix")
            << QString("myDir\\myfile")
            << relPrefix + QString("myDir\\myfile")
            << "myDir/myfile" << "myfile" << "myfile" << "myfile" << "" << "" << false <<true;

    QTest::newRow("noLongSuffix")
            << QString("myDir\\myfile.txt")
            << relPrefix + QString("myDir\\myfile.txt")
            << "myDir/myfile.txt" << "myfile.txt" << "myfile" << "myfile" << "txt" << "txt" << false << true;

    QTest::newRow("endingSlash")
            << QString("myDir\\myfile.bla\\")
            << relPrefix + QString("myDir\\myfile.bla\\")
            << "myDir/myfile.bla/" << "" << "" << "" << "" << "" << false << true;

    QTest::newRow("absolutePath")
            << QString("A:dir\\without\\leading\\backslash.bat")
            << absPrefix + QString("A:\\dir\\without\\leading\\backslash.bat")
            << "A:dir/without/leading/backslash.bat" << "backslash.bat" << "backslash" << "backslash" << "bat" << "bat" << false << false;
}

void tst_QFileSystemEntry::getSetCheck()
{
    QFETCH(QString, nativeFilePath);
    QFETCH(QString, internalnativeFilePath);
    QFETCH(QString, filepath);
    QFETCH(QString, filename);
    QFETCH(QString, baseName);
    QFETCH(QString, completeBasename);
    QFETCH(QString, suffix);
    QFETCH(QString, completeSuffix);
    QFETCH(bool, absolute);
    QFETCH(bool, relative);

    QFileSystemEntry entry1(filepath);
    QCOMPARE(entry1.filePath(), filepath);
    QCOMPARE(entry1.nativeFilePath().toLower(), internalnativeFilePath.toLower());
    QCOMPARE(entry1.fileName(), filename);
    QCOMPARE(entry1.suffix(), suffix);
    QCOMPARE(entry1.completeSuffix(), completeSuffix);
    QCOMPARE(entry1.isAbsolute(), absolute);
    QCOMPARE(entry1.isRelative(), relative);
    QCOMPARE(entry1.baseName(), baseName);
    QCOMPARE(entry1.completeBaseName(), completeBasename);

    QFileSystemEntry entry2(nativeFilePath, QFileSystemEntry::FromNativePath());
    QCOMPARE(entry2.suffix(), suffix);
    QCOMPARE(entry2.completeSuffix(), completeSuffix);
    QCOMPARE(entry2.isAbsolute(), absolute);
    QCOMPARE(entry2.isRelative(), relative);
    QCOMPARE(entry2.filePath(), filepath);
    // Since this entry was created using the native path,
    // the object shouldnot change nativeFilePath.
    QCOMPARE(entry2.nativeFilePath(), nativeFilePath);
    QCOMPARE(entry2.fileName(), filename);
    QCOMPARE(entry2.baseName(), baseName);
    QCOMPARE(entry2.completeBaseName(), completeBasename);
}

#else

void tst_QFileSystemEntry::getSetCheck_data()
{
    QTest::addColumn<QByteArray>("nativeFilePath");
    QTest::addColumn<QString>("filepath");
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("basename");
    QTest::addColumn<QString>("completeBasename");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<QString>("completeSuffix");
    QTest::addColumn<bool>("absolute");

    QTest::newRow("empty")
        << QByteArray()
        << QString()
        << QString() << QString() << QString() << QString() << QString() << false;

    QTest::newRow("simple")
        << QByteArray("/home/qt/in/a/dir.tar.gz")
        << "/home/qt/in/a/dir.tar.gz"
        << "dir.tar.gz" << "dir" << "dir.tar" << "gz" << "tar.gz" << true;

    QTest::newRow("relative")
        << QByteArray("in/a/dir.tar.gz")
        << "in/a/dir.tar.gz"
        << "dir.tar.gz" << "dir" << "dir.tar" << "gz" << "tar.gz" << false;

    QTest::newRow("noSuffix")
        << QByteArray("myDir/myfile")
        << "myDir/myfile" << "myfile" << "myfile" << "myfile" << "" << "" << false;

    QTest::newRow("noLongSuffix")
        << QByteArray("myDir/myfile.txt")
        << "myDir/myfile.txt" << "myfile.txt" << "myfile" << "myfile" << "txt" << "txt" << false;

    QTest::newRow("endingSlash")
        << QByteArray("myDir/myfile.bla/")
        << "myDir/myfile.bla/" << "" << "" << "" << "" << "" << false;

    QTest::newRow("relativePath")
        << QByteArray("A:dir/without/leading/backslash.bat")
        << "A:dir/without/leading/backslash.bat" << "backslash.bat" << "backslash" << "backslash" << "bat" << "bat" << false;
}

void tst_QFileSystemEntry::getSetCheck()
{
    QFETCH(QByteArray, nativeFilePath);
    QFETCH(QString, filepath);
    QFETCH(QString, filename);
    QFETCH(QString, basename);
    QFETCH(QString, completeBasename);
    QFETCH(QString, suffix);
    QFETCH(QString, completeSuffix);
    QFETCH(bool, absolute);

    QFileSystemEntry entry1(filepath);
    QCOMPARE(entry1.filePath(), filepath);
    QCOMPARE(entry1.nativeFilePath(), nativeFilePath);
    QCOMPARE(entry1.fileName(), filename);
    QCOMPARE(entry1.suffix(), suffix);
    QCOMPARE(entry1.completeSuffix(), completeSuffix);
    QCOMPARE(entry1.isAbsolute(), absolute);
    QCOMPARE(entry1.isRelative(), !absolute);
    QCOMPARE(entry1.baseName(), basename);
    QCOMPARE(entry1.completeBaseName(), completeBasename);

    QFileSystemEntry entry2(nativeFilePath, QFileSystemEntry::FromNativePath());
    QCOMPARE(entry2.suffix(), suffix);
    QCOMPARE(entry2.completeSuffix(), completeSuffix);
    QCOMPARE(entry2.isAbsolute(), absolute);
    QCOMPARE(entry2.isRelative(), !absolute);
    QCOMPARE(entry2.filePath(), filepath);
    QCOMPARE(entry2.nativeFilePath(), nativeFilePath);
    QCOMPARE(entry2.fileName(), filename);
    QCOMPARE(entry2.baseName(), basename);
    QCOMPARE(entry2.completeBaseName(), completeBasename);
}
#endif

void tst_QFileSystemEntry::suffix_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("noextension0") << "file" << "";
    QTest::newRow("noextension1") << "/path/to/file" << "";
    QTest::newRow("data0") << "file.tar" << "tar";
    QTest::newRow("data1") << "file.tar.gz" << "gz";
    QTest::newRow("data2") << "/path/file/file.tar.gz" << "gz";
    QTest::newRow("data3") << "/path/file.tar" << "tar";
    QTest::newRow("hidden1-1") << ".ext1" << "ext1";
    QTest::newRow("hidden1-2") << ".ext" << "ext";
    QTest::newRow("hidden1-3") << ".ex" << "ex";
    QTest::newRow("hidden1-4") << ".e" << "e";
    QTest::newRow("hidden2-1") << ".ext1.ext2" << "ext2";
    QTest::newRow("hidden2-2") << ".ext.ext2" << "ext2";
    QTest::newRow("hidden2-3") << ".ex.ext2" << "ext2";
    QTest::newRow("hidden2-4") << ".e.ext2" << "ext2";
    QTest::newRow("hidden2-5") << "..ext2" << "ext2";
    QTest::newRow("dots") << "/path/file.with.dots/file..ext2" << "ext2";
    QTest::newRow("dots2") << "/path/file.with.dots/.file..ext2" << "ext2";
}

void tst_QFileSystemEntry::suffix()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileSystemEntry fe(file);
    QCOMPARE(fe.suffix(), expected);

    QFileSystemEntry fi2(file);
    // first resolve the last slash
    (void) fi2.path();
    QCOMPARE(fi2.suffix(), expected);
}

void tst_QFileSystemEntry::completeSuffix_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("noextension0") << "file" << "";
    QTest::newRow("noextension1") << "/path/to/file" << "";
    QTest::newRow("data0") << "file.tar" << "tar";
    QTest::newRow("data1") << "file.tar.gz" << "tar.gz";
    QTest::newRow("data2") << "/path/file/file.tar.gz" << "tar.gz";
    QTest::newRow("data3") << "/path/file.tar" << "tar";
    QTest::newRow("dots") << "/path/file.with.dots/file..ext2" << ".ext2";
    QTest::newRow("dots2") << "/path/file.with.dots/.file..ext2" << "file..ext2";
}

void tst_QFileSystemEntry::completeSuffix()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileSystemEntry fi(file);
    QCOMPARE(fi.completeSuffix(), expected);

    QFileSystemEntry fi2(file);
    // first resolve the last slash
    (void) fi2.path();
    QCOMPARE(fi2.completeSuffix(), expected);
}

void tst_QFileSystemEntry::baseName_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("data0") << "file.tar" << "file";
    QTest::newRow("data1") << "file.tar.gz" << "file";
    QTest::newRow("data2") << "/path/file/file.tar.gz" << "file";
    QTest::newRow("data3") << "/path/file.tar" << "file";
    QTest::newRow("data4") << "/path/file" << "file";
    QTest::newRow("dots") << "/path/file.with.dots/file..ext2" << "file";
    QTest::newRow("dots2") << "/path/file.with.dots/.file..ext2" << "";
}

void tst_QFileSystemEntry::baseName()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileSystemEntry fi(file);
    QCOMPARE(fi.baseName(), expected);

    QFileSystemEntry fi2(file);
    // first resolve the last slash
    (void) fi2.path();
    QCOMPARE(fi2.baseName(), expected);
}

void tst_QFileSystemEntry::completeBaseName_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("data0") << "file.tar" << "file";
    QTest::newRow("data1") << "file.tar.gz" << "file.tar";
    QTest::newRow("data2") << "/path/file/file.tar.gz" << "file.tar";
    QTest::newRow("data3") << "/path/file.tar" << "file";
    QTest::newRow("data4") << "/path/file" << "file";
    QTest::newRow("dots") << "/path/file.with.dots/file..ext2" << "file.";
    QTest::newRow("dots2") << "/path/file.with.dots/.file..ext2" << ".file.";
}

void tst_QFileSystemEntry::completeBaseName()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileSystemEntry fi(file);
    QCOMPARE(fi.completeBaseName(), expected);

    QFileSystemEntry fi2(file);
    // first resolve the last slash
    (void) fi2.path();
    QCOMPARE(fi2.completeBaseName(), expected);
}

#if defined(Q_OS_WIN)
void tst_QFileSystemEntry::absoluteOrRelative_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isAbsolute");
    QTest::addColumn<bool>("isRelative");

    QTest::newRow("empty") << QString() << false << true;
    QTest::newRow("data0") << "file.tar" << false << true;
    QTest::newRow("data1") << "/path/file/file.tar.gz" << false << false;
    QTest::newRow("data1") << "C:path/file/file.tar.gz" << false << false;
    QTest::newRow("data3") << "C:/path/file" << true << false;
    QTest::newRow("data3") << "//machine/share" << true << false;
}

void tst_QFileSystemEntry::absoluteOrRelative()
{
    QFETCH(QString, path);
    QFETCH(bool, isAbsolute);
    QFETCH(bool, isRelative);

    QFileSystemEntry fi(path);
    QCOMPARE(fi.isAbsolute(), isAbsolute);
    QCOMPARE(fi.isRelative(), isRelative);
}
#endif

void tst_QFileSystemEntry::isClean_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isClean");

    QTest::newRow("empty") << QString() << true;
    QTest::newRow("simple") << "foo" << true;
    QTest::newRow("complex") << "/foo/bar/bz" << true;
    QTest::newRow(".file") << "/foo/.file" << true;
    QTest::newRow("..file") << "/foo/..file" << true;
    QTest::newRow("...") << "/foo/.../bar" << true;
    QTest::newRow("./") << "./" << false;
    QTest::newRow("../") << "../" << false;
    QTest::newRow(".") << "." << false;
    QTest::newRow("..") << ".." << false;
    QTest::newRow("/.") << "/." << false;
    QTest::newRow("/..") << "/.." << false;
    QTest::newRow("/../") << "foo/../bar" << false;
    QTest::newRow("/./") << "foo/./bar" << false;
    QTest::newRow("//") << "foo//bar" << false;
}

void tst_QFileSystemEntry::isClean()
{
    QFETCH(QString, path);
    QFETCH(bool, isClean);

    QFileSystemEntry fi(path);
    QCOMPARE(fi.isClean(), isClean);
}

void tst_QFileSystemEntry::defaultCtor()
{
    QFileSystemEntry entry;

    QVERIFY(entry.filePath().isNull());
    QVERIFY(entry.nativeFilePath().isNull());

    QVERIFY(entry.fileName().isNull());
    QCOMPARE(entry.path(), QString("."));

    QVERIFY(entry.baseName().isNull());
    QVERIFY(entry.completeBaseName().isNull());
    QVERIFY(entry.suffix().isNull());
    QVERIFY(entry.completeSuffix().isNull());

    QVERIFY(!entry.isAbsolute());
    QVERIFY(entry.isRelative());

    QVERIFY(entry.isClean());

#if defined(Q_OS_WIN)
    QVERIFY(!entry.isDriveRoot());
#endif
    QVERIFY(!entry.isRoot());

    QVERIFY(entry.isEmpty());
}

QTEST_MAIN(tst_QFileSystemEntry)
#include <tst_qfilesystementry.moc>
