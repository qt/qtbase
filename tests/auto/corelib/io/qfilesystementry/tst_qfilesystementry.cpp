/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>

#include <QtCore/private/qfilesystementry_p.h>

#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
#   define WIN_STUFF
#endif

//TESTED_CLASS=
//TESTED_FILES=

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
#if defined(WIN_STUFF)
    void absoluteOrRelative_data();
    void absoluteOrRelative();
#endif
    void isClean_data();
    void isClean();
};

#if defined(WIN_STUFF)
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

    QTest::newRow("noextension0") << "file" << "";
    QTest::newRow("noextension1") << "/path/to/file" << "";
    QTest::newRow("data0") << "file.tar" << "tar";
    QTest::newRow("data1") << "file.tar.gz" << "gz";
    QTest::newRow("data2") << "/path/file/file.tar.gz" << "gz";
    QTest::newRow("data3") << "/path/file.tar" << "tar";
    QTest::newRow("hidden1") << ".ext1" << "ext1";
    QTest::newRow("hidden1") << ".ext" << "ext";
    QTest::newRow("hidden1") << ".ex" << "ex";
    QTest::newRow("hidden1") << ".e" << "e";
    QTest::newRow("hidden2") << ".ext1.ext2" << "ext2";
    QTest::newRow("hidden2") << ".ext.ext2" << "ext2";
    QTest::newRow("hidden2") << ".ex.ext2" << "ext2";
    QTest::newRow("hidden2") << ".e.ext2" << "ext2";
    QTest::newRow("hidden2") << "..ext2" << "ext2";
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

#if defined(WIN_STUFF)
void tst_QFileSystemEntry::absoluteOrRelative_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isAbsolute");
    QTest::addColumn<bool>("isRelative");

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

QTEST_MAIN(tst_QFileSystemEntry)
#include <tst_qfilesystementry.moc>
