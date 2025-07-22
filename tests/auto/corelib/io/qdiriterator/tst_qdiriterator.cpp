// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdiriterator.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <QSet>
#include <QString>

#include <QtCore/private/qfsfileengine_p.h>

#if defined(Q_OS_VXWORKS)
#define Q_NO_SYMLINKS
#endif

#include "../../../../shared/filesystem.h"

#ifdef Q_OS_ANDROID
#include <QStandardPaths>
#endif

using namespace Qt::StringLiterals;

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

    bool createFile(const QString &fileName)
    {
        QFile file(fileName);
        return file.open(QIODevice::WriteOnly);
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
    void iterateRelativeDirectory_data();
    void iterateRelativeDirectory();
    void iterateResource_data();
    void iterateResource();
    void stopLinkLoop();
#ifdef QT_BUILD_INTERNAL
    void engineWithNoIterator();
    void testQFsFileEngineIterator_data() { iterateRelativeDirectory_data(); }
    void testQFsFileEngineIterator();
#endif
    void absoluteFilePathsFromRelativeIteratorPath();
    void recurseWithFilters() const;
    void longPath();
    void dirorder();
    void relativePaths();
    void dotNameFilters_data();
    void dotNameFilters();
#if defined(Q_OS_WIN)
    void uncPaths_data();
    void uncPaths();
#endif
#ifndef Q_OS_WIN
    void hiddenDirs_hiddenFiles();
#endif

    void hasNextFalseNoCrash();

private:
    QSharedPointer<QTemporaryDir> m_dataDir;
};

void tst_QDirIterator::initTestCase()
{
    QString testdata_dir;
#ifdef Q_OS_ANDROID
    testdata_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString resourceSourcePath = QStringLiteral(":/testdata");
    QDirIterator it(resourceSourcePath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo fileInfo = it.nextFileInfo();

        if (!fileInfo.isDir()) {
            QString destination = testdata_dir + QLatin1Char('/')
                                  + fileInfo.filePath().mid(resourceSourcePath.length());
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
    testdata_dir = m_dataDir->path();
#else
    m_dataDir.reset(new QTemporaryDir);
    testdata_dir = m_dataDir->path();
#endif

    QVERIFY(!testdata_dir.isEmpty());
    // Must call QDir::setCurrent() here because all the tests that use relative
    // paths depend on that.
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));

    createDirectory("entrylist");
    createDirectory("entrylist/directory");
    createFile("entrylist/file");
    createFile("entrylist/writable");
    createFile("entrylist/directory/dummy");

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
    createLink("entrylist/directory", "entrylist/linktodirectory.lnk");
    createLink("entrylist/nothing", "entrylist/brokenlink.lnk");
#  else
    createLink("file", "entrylist/linktofile.lnk");
    createLink("directory", "entrylist/linktodirectory.lnk");
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

void tst_QDirIterator::iterateRelativeDirectory_data()
{
    QTest::addColumn<QString>("dirName"); // relative from current path or abs
    QTest::addColumn<QDirIterator::IteratorFlags>("flags");
    QTest::addColumn<QDir::Filters>("filters");
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<QStringList>("entries");

    QTest::newRow("no flags")
        << QString("entrylist") << QDirIterator::IteratorFlags{}
        << QDir::Filters(QDir::NoFilter) << QStringList("*")
        << QString(
                  "entrylist/.,"
                   "entrylist/..,"
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktodirectory.lnk,"
#endif
                   "entrylist/writable").split(',');

    QTest::newRow("NoDot")
        << QString("entrylist") << QDirIterator::IteratorFlags{}
        << QDir::Filters(QDir::AllEntries | QDir::NoDot) << QStringList("*")
        << QString(
                   "entrylist/..,"
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktodirectory.lnk,"
#endif
                   "entrylist/writable").split(',');

    QTest::newRow("NoDotDot")
        << QString("entrylist") << QDirIterator::IteratorFlags{}
        << QDir::Filters(QDir::AllEntries | QDir::NoDotDot) << QStringList("*")
        << QString(
                  "entrylist/.,"
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktodirectory.lnk,"
#endif
                   "entrylist/writable").split(',');

    QTest::newRow("NoDotAndDotDot")
        << QString("entrylist") << QDirIterator::IteratorFlags{}
        << QDir::Filters(QDir::AllEntries | QDir::NoDotAndDotDot) << QStringList("*")
        << QString(
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory,"
#ifndef Q_NO_SYMLINKS
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
#ifndef Q_NO_SYMLINKS
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
        << QString("empty") << QDirIterator::IteratorFlags{}
        << QDir::Filters(QDir::NoFilter) << QStringList("*")
        << QString("empty/.,empty/..").split(',');

        QTest::newRow("empty, QDir::NoDotAndDotDot")
            << QString("empty") << QDirIterator::IteratorFlags{}
            << QDir::Filters(QDir::NoDotAndDotDot) << QStringList("*")
            << QStringList();
}

void tst_QDirIterator::iterateRelativeDirectory()
{
    QFETCH(QString, dirName);
    QFETCH(QDirIterator::IteratorFlags, flags);
    QFETCH(QDir::Filters, filters);
    QFETCH(QStringList, nameFilters);
    QFETCH(const QStringList, entries);

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
    for (const QString &item : entries)
        sortedEntries.append(QFileInfo(item).canonicalFilePath());
    sortedEntries.sort();

    if (sortedEntries != list) {
        qDebug() << "ACTUAL:  " << list;
        qDebug() << "EXPECTED:" << sortedEntries;
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

    QTest::newRow("invalid") << QString::fromLatin1(":/testdata/burpaburpa") << QDirIterator::IteratorFlags{}
                             << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                             << QStringList();
    QTest::newRow("qrc:/testdata") << u":/testdata/"_s << QDirIterator::IteratorFlags{}
                               << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                               << QString::fromLatin1(":/testdata/entrylist").split(QLatin1String(","));
    QTest::newRow("qrc:/testdata/entrylist") << u":/testdata/entrylist"_s << QDirIterator::IteratorFlags{}
                               << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                               << QString::fromLatin1(":/testdata/entrylist/directory,:/testdata/entrylist/file").split(QLatin1String(","));
    QTest::newRow("qrc:/testdata recursive") << u":/testdata"_s
                                         << QDirIterator::IteratorFlags(QDirIterator::Subdirectories)
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
        qDebug() << "ACTUAL:" << list;
        qDebug() << "EXPECTED:" << sortedEntries;
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
        it.nextFileInfo();
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

    IteratorUniquePtr
    beginEntryList(const QString &, QDirListing::IteratorFlags, const QStringList &) override
    { return nullptr; }
};

class EngineWithNoIteratorHandler : public QAbstractFileEngineHandler
{
    Q_DISABLE_COPY_MOVE(EngineWithNoIteratorHandler)
public:
    EngineWithNoIteratorHandler() = default;

    std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const override
    {
        return std::make_unique<EngineWithNoIterator>(fileName);
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

class CustomEngineHandler : public QAbstractFileEngineHandler
{
    Q_DISABLE_COPY_MOVE(CustomEngineHandler)
public:
    CustomEngineHandler() = default;

    std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const override
    {
        // We want to test QFSFileEngine specifically, so force QDirIterator to use it
        // over the default QFileSystemEngine
        return std::make_unique<QFSFileEngine>(fileName);
    }
};

void tst_QDirIterator::testQFsFileEngineIterator()
{
    QFETCH(QString, dirName);
    QFETCH(QStringList, nameFilters);
    QFETCH(QDir::Filters, filters);
    QFETCH(QDirIterator::IteratorFlags, flags);

    if (dirName == u"empty")
        return; // This row isn't useful in this test

    CustomEngineHandler handler;
    bool isEmpty = true;
    QDirIterator iter(dirName, nameFilters, filters, flags);
    while (iter.hasNext()) {
        const QFileInfo &fi = iter.nextFileInfo();
        if (fi.filePath().contains(u"entrylist"))
            isEmpty = false;  // At least one entry in `entrylist` dir
    }
    QVERIFY(!isEmpty);
}
#endif

void tst_QDirIterator::absoluteFilePathsFromRelativeIteratorPath()
{
    QDirIterator it("entrylist/", QDir::NoDotAndDotDot);
    while (it.hasNext())
        QVERIFY(it.nextFileInfo().absoluteFilePath().contains("entrylist"));
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
    actualEntries.insert(it.next());
    QVERIFY(it.hasNext());
    actualEntries.insert(it.next());
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
        if (n >= 20480)
        {
            break;
        }
    }
    if (n >= 20480)
    {
        qWarning("No maximum length on directory names");
    }
    QDirIterator it(dir.absolutePath(), QDir::NoDotAndDotDot|QDir::Dirs, QDirIterator::Subdirectories);
    int m = 0;
    while (it.hasNext()) {
        ++m;
        it.nextFileInfo();
    }

    QCOMPARE(n, m);
    dirName.chop(1);
    while (dirName.size() > 0 && dir.exists(dirName) && dir.rmdir(dirName)) {
        --n;
        dirName.chop(1);
    }
    QCOMPARE(n, 0);
    QVERIFY(dir.cdUp());
    QVERIFY(dir.rmdir("longpaths"));
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

void tst_QDirIterator::dotNameFilters_data()
{
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<QDir::Filters>("dirFilters");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("NoDotAndDotDot")
        << QStringList{u"."_s}
        << QDir::Filters(QDir::AllEntries | QDir::NoDotAndDotDot)
        << QStringList{};

    QTest::newRow("default-dirfilters")
        << QStringList{u"."_s}
        << QDir::Filters(QDir::AllEntries)
        << QStringList{u"."_s};

    QTest::newRow("glob-everything")
        << QStringList{u".*"_s}
        << QDir::Filters(QDir::AllEntries)
        << QStringList{u"."_s, u".."_s};
}

void tst_QDirIterator::dotNameFilters()
{
    QFETCH(QStringList, nameFilters);
    QFETCH(QDir::Filters, dirFilters);
    QFETCH(QStringList, expected);

    const auto dirPath = u"empty"_s;
    QVERIFY(QFileInfo(dirPath).isDir());

    QDirIterator dit(dirPath, nameFilters, dirFilters);
    QStringList entries;
    while (dit.hasNext())
        entries.append(dit.nextFileInfo().fileName());
    entries.sort();
    QCOMPARE_EQ(entries, expected);
}

#if defined(Q_OS_WIN)
void tst_QDirIterator::uncPaths_data()
{
    QTest::addColumn<QString>("dirName");
    QTest::newRow("uncserver")
            <<QString("//" + QTest::uncServerName());
    QTest::newRow("uncserver/testshare")
            <<QString("//" + QTest::uncServerName() + "/testshare");
    QTest::newRow("uncserver/testshare/tmp")
            <<QString("//" + QTest::uncServerName() + "/testshare/tmp");
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
            if (di.nextFileInfo().isDir())
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
            if (!di.nextFileInfo().isDir())
                ++failures;    // search was only supposed to find files
        }
        QCOMPARE(matches, 6);
        QCOMPARE(failures, 0);
    }
}
#endif // Q_OS_WIN

void tst_QDirIterator::hasNextFalseNoCrash()
{
    QVERIFY(QFileInfo(u"empty"_s).exists());
    QDirIterator iter(u"empty"_s);
    int count = 0;
    while (iter.hasNext()) {
        iter.next();
        ++count;
    }
    QVERIFY(count > 0);
    QVERIFY(!iter.hasNext());
    // QTBUG-130142
    // When the iteration reaches the end, calling next() returns an empty string
    // and no crash happens
    QVERIFY(iter.next().isEmpty());
}

QTEST_MAIN(tst_QDirIterator)

#include "tst_qdiriterator.moc"

