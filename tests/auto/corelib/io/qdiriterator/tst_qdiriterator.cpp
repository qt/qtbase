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

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdiriterator.h>
#include <qfileinfo.h>
#include <qstringlist.h>

#include <QtCore/private/qfsfileengine_p.h>

#if defined(Q_OS_VXWORKS) || defined(Q_OS_WINRT)
#define Q_NO_SYMLINKS
#endif

#if defined(Q_OS_WIN)
#  include "../../../network-settings.h"
#endif

Q_DECLARE_METATYPE(QDirIterator::IteratorFlags)
Q_DECLARE_METATYPE(QDir::Filters)

class tst_QDirIterator : public QObject
{
    Q_OBJECT

private: // convenience functions
    QStringList createdDirectories;
    QStringList createdFiles;

    QDir currentDir;
    bool createDirectory(const QString &dirName)
    {
        if (currentDir.mkdir(dirName)) {
            createdDirectories.prepend(dirName);
            return true;
        }
        return false;
    }

    enum Cleanup { DoDelete, DontDelete };
    bool createFile(const QString &fileName, Cleanup cleanup = DoDelete)
    {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            if (cleanup == DoDelete)
                createdFiles << fileName;
            return true;
        }
        return false;
    }

    bool createLink(const QString &destination, const QString &linkName)
    {
        if (QFile::link(destination, linkName)) {
            createdFiles << linkName;
            return true;
        }
        return false;
    }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void iterateRelativeDirectory_data();
    void iterateRelativeDirectory();
    void iterateResource_data();
    void iterateResource();
    void stopLinkLoop();
#ifdef QT_BUILD_INTERNAL
    void engineWithNoIterator();
#endif
    void absoluteFilePathsFromRelativeIteratorPath();
    void recurseWithFilters() const;
    void longPath();
    void dirorder();
    void relativePaths();
#if defined(Q_OS_WIN)
    void uncPaths_data();
    void uncPaths();
#endif
#ifndef Q_OS_WIN
    void hiddenDirs_hiddenFiles();
#endif
#ifdef BUILTIN_TESTDATA
private:
    QSharedPointer<QTemporaryDir> m_dataDir;
#endif
};

void tst_QDirIterator::initTestCase()
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
    QString testdata_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString resourceSourcePath = QStringLiteral(":/testdata");
    QDirIterator it(resourceSourcePath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();

        QFileInfo fileInfo = it.fileInfo();

        if (!fileInfo.isDir()) {
            QString destination = testdata_dir + QLatin1Char('/') + fileInfo.filePath().mid(resourceSourcePath.length());
            QFileInfo destinationFileInfo(destination);
            if (!destinationFileInfo.exists()) {
                QDir().mkpath(destinationFileInfo.path());
                if (!QFile::copy(fileInfo.filePath(), destination))
                    qWarning("Failed to copy %s", qPrintable(fileInfo.filePath()));
            }
        }

    }

    testdata_dir += QStringLiteral("/entrylist");
#elif defined(BUILTIN_TESTDATA)
    m_dataDir = QEXTRACTTESTDATA("/testdata");
    QVERIFY2(!m_dataDir.isNull(), qPrintable("Could not extract test data"));
    QString testdata_dir = m_dataDir->path();
#else

    // chdir into testdata directory, then find testdata by relative paths.
    QString testdata_dir = QFileInfo(QFINDTESTDATA("entrylist")).absolutePath();
#endif

    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));

    QFile::remove("entrylist/entrylist1.lnk");
    QFile::remove("entrylist/entrylist2.lnk");
    QFile::remove("entrylist/entrylist3.lnk");
    QFile::remove("entrylist/entrylist4.lnk");
    QFile::remove("entrylist/directory/entrylist1.lnk");
    QFile::remove("entrylist/directory/entrylist2.lnk");
    QFile::remove("entrylist/directory/entrylist3.lnk");
    QFile::remove("entrylist/directory/entrylist4.lnk");

    createDirectory("entrylist");
    createDirectory("entrylist/directory");
    createFile("entrylist/file", DontDelete);
    createFile("entrylist/writable");
    createFile("entrylist/directory/dummy", DontDelete);

    createDirectory("recursiveDirs");
    createDirectory("recursiveDirs/dir1");
    createFile("recursiveDirs/textFileA.txt");
    createFile("recursiveDirs/dir1/aPage.html");
    createFile("recursiveDirs/dir1/textFileB.txt");

    createDirectory("foo");
    createDirectory("foo/bar");
    createFile("foo/bar/readme.txt");

    createDirectory("empty");

#ifndef Q_NO_SYMLINKS
#  if defined(Q_OS_WIN)
    // ### Sadly, this is a platform difference right now.
    createLink("entrylist/file", "entrylist/linktofile.lnk");
#    ifndef Q_NO_SYMLINKS_TO_DIRS
    createLink("entrylist/directory", "entrylist/linktodirectory.lnk");
#    endif
    createLink("entrylist/nothing", "entrylist/brokenlink.lnk");
#  else
    createLink("file", "entrylist/linktofile.lnk");
#    ifndef Q_NO_SYMLINKS_TO_DIRS
    createLink("directory", "entrylist/linktodirectory.lnk");
#    endif
    createLink("nothing", "entrylist/brokenlink.lnk");
#  endif
#endif

#if !defined(Q_OS_WIN)
    createDirectory("hiddenDirs_hiddenFiles");
    createFile("hiddenDirs_hiddenFiles/normalFile");
    createFile("hiddenDirs_hiddenFiles/.hiddenFile");
    createDirectory("hiddenDirs_hiddenFiles/normalDirectory");
    createDirectory("hiddenDirs_hiddenFiles/.hiddenDirectory");
    createFile("hiddenDirs_hiddenFiles/normalDirectory/normalFile");
    createFile("hiddenDirs_hiddenFiles/normalDirectory/.hiddenFile");
    createFile("hiddenDirs_hiddenFiles/.hiddenDirectory/normalFile");
    createFile("hiddenDirs_hiddenFiles/.hiddenDirectory/.hiddenFile");
    createDirectory("hiddenDirs_hiddenFiles/normalDirectory/normalDirectory");
    createDirectory("hiddenDirs_hiddenFiles/normalDirectory/.hiddenDirectory");
    createDirectory("hiddenDirs_hiddenFiles/.hiddenDirectory/normalDirectory");
    createDirectory("hiddenDirs_hiddenFiles/.hiddenDirectory/.hiddenDirectory");
#endif
}

void tst_QDirIterator::cleanupTestCase()
{
    Q_FOREACH(QString fileName, createdFiles)
        QFile::remove(fileName);

    Q_FOREACH(QString dirName, createdDirectories)
        currentDir.rmdir(dirName);

#ifdef Q_OS_WINRT
    QDir::setCurrent(QCoreApplication::applicationDirPath());
#endif // Q_OS_WINRT

}

void tst_QDirIterator::iterateRelativeDirectory_data()
{
    QTest::addColumn<QString>("dirName"); // relative from current path or abs
    QTest::addColumn<QDirIterator::IteratorFlags>("flags");
    QTest::addColumn<QDir::Filters>("filters");
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<QStringList>("entries");

    QTest::newRow("no flags")
        << QString("entrylist") << QDirIterator::IteratorFlags(0)
        << QDir::Filters(QDir::NoFilter) << QStringList("*")
        << QString(
                  "entrylist/.,"
                   "entrylist/..,"
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory,"
#if !defined(Q_NO_SYMLINKS) && !defined(Q_NO_SYMLINKS_TO_DIRS)
                   "entrylist/linktodirectory.lnk,"
#endif
                   "entrylist/writable").split(',');

    QTest::newRow("NoDot")
        << QString("entrylist") << QDirIterator::IteratorFlags(0)
        << QDir::Filters(QDir::AllEntries | QDir::NoDot) << QStringList("*")
        << QString(
                   "entrylist/..,"
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory,"
#if !defined(Q_NO_SYMLINKS) && !defined(Q_NO_SYMLINKS_TO_DIRS)
                   "entrylist/linktodirectory.lnk,"
#endif
                   "entrylist/writable").split(',');

    QTest::newRow("NoDotDot")
        << QString("entrylist") << QDirIterator::IteratorFlags(0)
        << QDir::Filters(QDir::AllEntries | QDir::NoDotDot) << QStringList("*")
        << QString(
                  "entrylist/.,"
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory,"
#if !defined(Q_NO_SYMLINKS) && !defined(Q_NO_SYMLINKS_TO_DIRS)
                   "entrylist/linktodirectory.lnk,"
#endif
                   "entrylist/writable").split(',');

    QTest::newRow("NoDotAndDotDot")
        << QString("entrylist") << QDirIterator::IteratorFlags(0)
        << QDir::Filters(QDir::AllEntries | QDir::NoDotAndDotDot) << QStringList("*")
        << QString(
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory,"
#if !defined(Q_NO_SYMLINKS) && !defined(Q_NO_SYMLINKS_TO_DIRS)
                   "entrylist/linktodirectory.lnk,"
#endif
                   "entrylist/writable").split(',');

    QTest::newRow("QDir::Subdirectories | QDir::FollowSymlinks")
        << QString("entrylist") << QDirIterator::IteratorFlags(QDirIterator::Subdirectories | QDirIterator::FollowSymlinks)
        << QDir::Filters(QDir::NoFilter) << QStringList("*")
        << QString(
                   "entrylist/.,"
                   "entrylist/..,"
                   "entrylist/directory/.,"
                   "entrylist/directory/..,"
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory,"
                   "entrylist/directory/dummy,"
#if !defined(Q_NO_SYMLINKS) && !defined(Q_NO_SYMLINKS_TO_DIRS)
                   "entrylist/linktodirectory.lnk,"
#endif
                   "entrylist/writable").split(',');

    QTest::newRow("QDir::Subdirectories / QDir::Files")
        << QString("entrylist") << QDirIterator::IteratorFlags(QDirIterator::Subdirectories)
        << QDir::Filters(QDir::Files) << QStringList("*")
        << QString("entrylist/directory/dummy,"
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/writable").split(',');

    QTest::newRow("QDir::Subdirectories | QDir::FollowSymlinks / QDir::Files")
        << QString("entrylist") << QDirIterator::IteratorFlags(QDirIterator::Subdirectories | QDirIterator::FollowSymlinks)
        << QDir::Filters(QDir::Files) << QStringList("*")
        << QString("entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory/dummy,"
                   "entrylist/writable").split(',');

    QTest::newRow("empty, default")
        << QString("empty") << QDirIterator::IteratorFlags(0)
        << QDir::Filters(QDir::NoFilter) << QStringList("*")
        << QString("empty/.,empty/..").split(',');

        QTest::newRow("empty, QDir::NoDotAndDotDot")
            << QString("empty") << QDirIterator::IteratorFlags(0)
            << QDir::Filters(QDir::NoDotAndDotDot) << QStringList("*")
            << QStringList();
}

void tst_QDirIterator::iterateRelativeDirectory()
{
    QFETCH(QString, dirName);
    QFETCH(QDirIterator::IteratorFlags, flags);
    QFETCH(QDir::Filters, filters);
    QFETCH(QStringList, nameFilters);
    QFETCH(QStringList, entries);

    QDirIterator it(dirName, nameFilters, filters, flags);
    QStringList list;
    while (it.hasNext()) {
        QString next = it.next();

        QString fileName = it.fileName();
        QString filePath = it.filePath();
        QString path = it.path();

        QFileInfo info = it.fileInfo();

        QCOMPARE(path, dirName);
        QCOMPARE(next, filePath);

        QCOMPARE(info, QFileInfo(next));
        QCOMPARE(fileName, info.fileName());
        QCOMPARE(filePath, info.filePath());

        // Using canonical file paths for final comparison
        list << info.canonicalFilePath();
    }

    // The order of items returned by QDirIterator is not guaranteed.
    list.sort();

    QStringList sortedEntries;
    foreach(QString item, entries)
        sortedEntries.append(QFileInfo(item).canonicalFilePath());
    sortedEntries.sort();

    if (sortedEntries != list) {
        qDebug() << "EXPECTED:" << sortedEntries;
        qDebug() << "ACTUAL:  " << list;
    }

    QCOMPARE(list, sortedEntries);
}

void tst_QDirIterator::iterateResource_data()
{
    QTest::addColumn<QString>("dirName"); // relative from current path or abs
    QTest::addColumn<QDirIterator::IteratorFlags>("flags");
    QTest::addColumn<QDir::Filters>("filters");
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<QStringList>("entries");

    QTest::newRow("invalid") << QString::fromLatin1(":/testdata/burpaburpa") << QDirIterator::IteratorFlags(0)
                             << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                             << QStringList();
    QTest::newRow(":/testdata") << QString::fromLatin1(":/testdata/") << QDirIterator::IteratorFlags(0)
                               << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                               << QString::fromLatin1(":/testdata/entrylist").split(QLatin1String(","));
    QTest::newRow(":/testdata/entrylist") << QString::fromLatin1(":/testdata/entrylist") << QDirIterator::IteratorFlags(0)
                               << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                               << QString::fromLatin1(":/testdata/entrylist/directory,:/testdata/entrylist/file").split(QLatin1String(","));
    QTest::newRow(":/testdata recursive") << QString::fromLatin1(":/testdata") << QDirIterator::IteratorFlags(QDirIterator::Subdirectories)
                                         << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                                         << QString::fromLatin1(":/testdata/entrylist,:/testdata/entrylist/directory,:/testdata/entrylist/directory/dummy,:/testdata/entrylist/file").split(QLatin1String(","));
}

void tst_QDirIterator::iterateResource()
{
    QFETCH(QString, dirName);
    QFETCH(QDirIterator::IteratorFlags, flags);
    QFETCH(QDir::Filters, filters);
    QFETCH(QStringList, nameFilters);
    QFETCH(QStringList, entries);

    QDirIterator it(dirName, nameFilters, filters, flags);
    QStringList list;
    while (it.hasNext()) {
        const QString dir = it.next();
        if (!dir.startsWith(":/qt-project.org"))
            list << dir;
    }

    list.sort();
    QStringList sortedEntries = entries;
    sortedEntries.sort();

    if (sortedEntries != list) {
        qDebug() << "EXPECTED:" << sortedEntries;
        qDebug() << "ACTUAL:" << list;
    }

    QCOMPARE(list, sortedEntries);
}

void tst_QDirIterator::stopLinkLoop()
{
#ifdef Q_OS_WIN
    // ### Sadly, this is a platform difference right now.
    createLink(QDir::currentPath() + QLatin1String("/entrylist"), "entrylist/entrylist1.lnk");
    createLink("entrylist/.", "entrylist/entrylist2.lnk");
    createLink("entrylist/../entrylist/.", "entrylist/entrylist3.lnk");
    createLink("entrylist/..", "entrylist/entrylist4.lnk");
    createLink(QDir::currentPath() + QLatin1String("/entrylist"), "entrylist/directory/entrylist1.lnk");
    createLink("entrylist/.", "entrylist/directory/entrylist2.lnk");
    createLink("entrylist/../directory/.", "entrylist/directory/entrylist3.lnk");
    createLink("entrylist/..", "entrylist/directory/entrylist4.lnk");
#else
    createLink(QDir::currentPath() + QLatin1String("/entrylist"), "entrylist/entrylist1.lnk");
    createLink(".", "entrylist/entrylist2.lnk");
    createLink("../entrylist/.", "entrylist/entrylist3.lnk");
    createLink("..", "entrylist/entrylist4.lnk");
    createLink(QDir::currentPath() + QLatin1String("/entrylist"), "entrylist/directory/entrylist1.lnk");
    createLink(".", "entrylist/directory/entrylist2.lnk");
    createLink("../directory/.", "entrylist/directory/entrylist3.lnk");
    createLink("..", "entrylist/directory/entrylist4.lnk");
#endif

    QDirIterator it(QLatin1String("entrylist"), QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    QStringList list;
    int max = 200;
    while (--max && it.hasNext())
        it.next();
    QVERIFY(max);

    // The goal of this test is only to ensure that the test above don't malfunction
}

#ifdef QT_BUILD_INTERNAL
class EngineWithNoIterator : public QFSFileEngine
{
public:
    EngineWithNoIterator(const QString &fileName)
        : QFSFileEngine(fileName)
    { }

    QAbstractFileEngineIterator *beginEntryList(QDir::Filters, const QStringList &)
    { return 0; }
};

class EngineWithNoIteratorHandler : public QAbstractFileEngineHandler
{
public:
    QAbstractFileEngine *create(const QString &fileName) const
    {
        return new EngineWithNoIterator(fileName);
    }
};
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QDirIterator::engineWithNoIterator()
{
    EngineWithNoIteratorHandler handler;

    QDir("entrylist").entryList();
    QVERIFY(true); // test that the above line doesn't crash
}
#endif

void tst_QDirIterator::absoluteFilePathsFromRelativeIteratorPath()
{
    QDirIterator it("entrylist/", QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        it.next();
        QVERIFY(QFileInfo(it.filePath()).absoluteFilePath().contains("entrylist"));
    }
}

void tst_QDirIterator::recurseWithFilters() const
{
    QStringList nameFilters;
    nameFilters.append("*.txt");

    QDirIterator it("recursiveDirs/", nameFilters, QDir::Files,
                    QDirIterator::Subdirectories);

    QSet<QString> actualEntries;
    QSet<QString> expectedEntries;
    expectedEntries.insert(QString::fromLatin1("recursiveDirs/dir1/textFileB.txt"));
    expectedEntries.insert(QString::fromLatin1("recursiveDirs/textFileA.txt"));

    QVERIFY(it.hasNext());
    it.next();
    actualEntries.insert(it.fileInfo().filePath());
    QVERIFY(it.hasNext());
    it.next();
    actualEntries.insert(it.fileInfo().filePath());
    QCOMPARE(actualEntries, expectedEntries);

    QVERIFY(!it.hasNext());
}

void tst_QDirIterator::longPath()
{
    QDir dir;
    dir.mkdir("longpaths");
    dir.cd("longpaths");

    QString dirName = "x";
    int n = 0;
    while (dir.exists(dirName) || dir.mkdir(dirName)) {
        ++n;
        dirName.append('x');
    }

    QDirIterator it(dir.absolutePath(), QDir::NoDotAndDotDot|QDir::Dirs, QDirIterator::Subdirectories);
    int m = 0;
    while (it.hasNext()) {
        ++m;
        it.next();
    }

    QCOMPARE(n, m);

    dirName.chop(1);
    while (dirName.length() > 0 && dir.exists(dirName) && dir.rmdir(dirName)) {
        dirName.chop(1);
    }
    dir.cdUp();
    dir.rmdir("longpaths");
}

void tst_QDirIterator::dirorder()
{
    QDirIterator iterator("foo", QDirIterator::Subdirectories);
    while (iterator.hasNext() && iterator.next() != "foo/bar")
    { }

    QCOMPARE(iterator.filePath(), QString("foo/bar"));
    QCOMPARE(iterator.fileInfo().filePath(), QString("foo/bar"));
}

void tst_QDirIterator::relativePaths()
{
    QDirIterator iterator("*", QDirIterator::Subdirectories);
    while(iterator.hasNext()) {
        QCOMPARE(iterator.filePath(), QDir::cleanPath(iterator.filePath()));
    }
}

#if defined(Q_OS_WIN)
void tst_QDirIterator::uncPaths_data()
{
    QTest::addColumn<QString>("dirName");
    QTest::newRow("uncserver")
            <<QString("//" + QtNetworkSettings::winServerName());
    QTest::newRow("uncserver/testshare")
            <<QString("//" + QtNetworkSettings::winServerName() + "/testshare");
    QTest::newRow("uncserver/testshare/tmp")
            <<QString("//" + QtNetworkSettings::winServerName() + "/testshare/tmp");
}
void tst_QDirIterator::uncPaths()
{
    QFETCH(QString, dirName);
    QDirIterator iterator(dirName, QDir::AllEntries|QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while(iterator.hasNext()) {
        iterator.next();
        QCOMPARE(iterator.filePath(), QDir::cleanPath(iterator.filePath()));
    }
}
#endif

#ifndef Q_OS_WIN
// In Unix it is easy to create hidden files, but in Windows it requires
// a special call since hidden files need to be "marked" while in Unix
// anything starting by a '.' is a hidden file.
// For that reason this test is not run in Windows.
void tst_QDirIterator::hiddenDirs_hiddenFiles()
{
    // Only files
    {
        int matches = 0;
        int failures = 0;
        QDirIterator di("hiddenDirs_hiddenFiles", QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (di.hasNext()) {
            ++matches;
            QString filename = di.next();
            if (QFileInfo(filename).isDir())
                ++failures;    // search was only supposed to find files
        }
        QCOMPARE(matches, 6);
        QCOMPARE(failures, 0);
    }
    // Only directories
    {
        int matches = 0;
        int failures = 0;
        QDirIterator di("hiddenDirs_hiddenFiles", QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (di.hasNext()) {
            ++matches;
            QString filename = di.next();
            if (!QFileInfo(filename).isDir())
                ++failures;    // search was only supposed to find files
        }
        QCOMPARE(matches, 6);
        QCOMPARE(failures, 0);
    }
}
#endif // Q_OS_WIN

QTEST_MAIN(tst_QDirIterator)

#include "tst_qdiriterator.moc"

