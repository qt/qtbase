// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdirlisting.h>
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

Q_DECLARE_METATYPE(QDirListing::IteratorFlags)
Q_DECLARE_METATYPE(QDir::Filters)

using ItFlag = QDirListing::IteratorFlag;

class tst_QDirListing : public QObject
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
#if defined(Q_OS_WIN)
    void uncPaths_data();
    void uncPaths();
#endif
#ifndef Q_OS_WIN
    void hiddenFiles();
    void hiddenDirs();
#endif

    void withStdAlgorithms();

private:
    QSharedPointer<QTemporaryDir> m_dataDir;
};

void tst_QDirListing::initTestCase()
{
    QString testdata_dir;
#ifdef Q_OS_ANDROID
    testdata_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString resourceSourcePath = QStringLiteral(":/testdata");
    for (const auto &dirEntry : QDirListing(resourceSourcePath, ItFlag::Recursive)) {
        if (!dirEntry.isDir()) {
            const QString &filePath = dirEntry.filePath();
            QString destination = testdata_dir + QLatin1Char('/')
                                  + filePath.sliced(resourceSourcePath.length());
            QFileInfo destinationFileInfo(destination);
            if (!destinationFileInfo.exists()) {
                QDir().mkpath(destinationFileInfo.path());
                if (!QFile::copy(filePath, destination))
                    qWarning("Failed to copy %s", qPrintable(filePath));
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
    createDirectory("hiddenDirs_hiddenFiles/normalDirectory/subdir");
    createDirectory("hiddenDirs_hiddenFiles/normalDirectory/.hidden-subdir");
    createDirectory("hiddenDirs_hiddenFiles/.hiddenDirectory/subdir");
    createDirectory("hiddenDirs_hiddenFiles/.hiddenDirectory/.hidden-subdir");
#endif
}

void tst_QDirListing::iterateRelativeDirectory_data()
{
    QTest::addColumn<QString>("dirName"); // relative from current path or abs
    QTest::addColumn<QDirListing::IteratorFlags>("flags");
    QTest::addColumn<QDir::Filters>("filters");
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<QStringList>("entries");

    QTest::newRow("no flags")
        << QString("entrylist") << QDirListing::IteratorFlags{}
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
        << QString("entrylist") << QDirListing::IteratorFlags{}
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
        << QString("entrylist") << QDirListing::IteratorFlags{}
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
        << QString("entrylist") << QDirListing::IteratorFlags{}
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
        << QString("entrylist") << QDirListing::IteratorFlags(ItFlag::Recursive | ItFlag::FollowSymlinks)
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
        << QString("entrylist") << QDirListing::IteratorFlags(ItFlag::Recursive)
        << QDir::Filters(QDir::Files) << QStringList("*")
        << QString("entrylist/directory/dummy,"
                   "entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/writable").split(',');

    QTest::newRow("QDir::Subdirectories | QDir::FollowSymlinks / QDir::Files")
        << QString("entrylist") << QDirListing::IteratorFlags(ItFlag::Recursive | QDirListing::IteratorFlag::FollowSymlinks)
        << QDir::Filters(QDir::Files) << QStringList("*")
        << QString("entrylist/file,"
#ifndef Q_NO_SYMLINKS
                   "entrylist/linktofile.lnk,"
#endif
                   "entrylist/directory/dummy,"
                   "entrylist/writable").split(',');

    QTest::newRow("empty, default")
        << QString("empty") << QDirListing::IteratorFlags{}
        << QDir::Filters(QDir::NoFilter) << QStringList("*")
        << QString("empty/.,empty/..").split(',');

        QTest::newRow("empty, QDir::NoDotAndDotDot")
            << QString("empty") << QDirListing::IteratorFlags{}
            << QDir::Filters(QDir::NoDotAndDotDot) << QStringList("*")
            << QStringList();
}

void tst_QDirListing::iterateRelativeDirectory()
{
    QFETCH(QString, dirName);
    QFETCH(QDirListing::IteratorFlags, flags);
    QFETCH(QDir::Filters, filters);
    QFETCH(QStringList, nameFilters);
    QFETCH(const QStringList, entries);

    QStringList list;
    for (const auto &dirEntry : QDirListing(dirName, nameFilters, filters, flags)) {
        // Using canonical file paths for final comparison
        list << dirEntry.fileInfo().canonicalFilePath();
    }

    // The order of items returned by QDirListing is not guaranteed.
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

void tst_QDirListing::iterateResource_data()
{
    QTest::addColumn<QString>("dirName"); // relative from current path or abs
    QTest::addColumn<QDirListing::IteratorFlags>("flags");
    QTest::addColumn<QDir::Filters>("filters");
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<QStringList>("entries");

    QTest::newRow("invalid") << QString::fromLatin1(":/testdata/burpaburpa") << QDirListing::IteratorFlags{}
                             << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                             << QStringList();
    QTest::newRow("qrc:/testdata") << u":/testdata/"_s << QDirListing::IteratorFlags{}
                               << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                               << QString::fromLatin1(":/testdata/entrylist").split(QLatin1String(","));
    QTest::newRow("qrc:/testdata/entrylist") << u":/testdata/entrylist"_s << QDirListing::IteratorFlags{}
                               << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                               << QString::fromLatin1(":/testdata/entrylist/directory,:/testdata/entrylist/file").split(QLatin1String(","));
    QTest::newRow("qrc:/testdata recursive") << u":/testdata"_s
                                         << QDirListing::IteratorFlags(ItFlag::Recursive)
                                         << QDir::Filters(QDir::NoFilter) << QStringList(QLatin1String("*"))
                                         << QString::fromLatin1(":/testdata/entrylist,:/testdata/entrylist/directory,:/testdata/entrylist/directory/dummy,:/testdata/entrylist/file").split(QLatin1String(","));
}

void tst_QDirListing::iterateResource()
{
    QFETCH(QString, dirName);
    QFETCH(QDirListing::IteratorFlags, flags);
    QFETCH(QDir::Filters, filters);
    QFETCH(QStringList, nameFilters);
    QFETCH(QStringList, entries);

    QStringList list;
    for (const auto &dirEntry : QDirListing(dirName, nameFilters, filters, flags)) {
        QString dir = dirEntry.fileInfo().filePath();
        if (!dir.startsWith(":/qt-project.org"))
            list.emplace_back(std::move(dir));
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

void tst_QDirListing::stopLinkLoop()
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

    constexpr auto flags = ItFlag::Recursive | ItFlag::FollowSymlinks;
    QDirListing dirIter(u"entrylist"_s, flags);
    QStringList list;
    int max = 200;
    auto it = dirIter.begin();
    while (--max && it != dirIter.end())
        ++it;
    QCOMPARE_GT(max, 0);

    // The goal of this test is only to ensure that the test above don't malfunction
}

#ifdef QT_BUILD_INTERNAL
class EngineWithNoIterator : public QFSFileEngine
{
public:
    EngineWithNoIterator(const QString &fileName)
        : QFSFileEngine(fileName)
    { }

    IteratorUniquePtr beginEntryList(const QString &, QDir::Filters, const QStringList &) override
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
void tst_QDirListing::engineWithNoIterator()
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
        // We want to test QFSFileEngine specifically, so force QDirListing to use it
        // over the default QFileSystemEngine
        return std::make_unique<QFSFileEngine>(fileName);
    }
};

void tst_QDirListing::testQFsFileEngineIterator()
{
    QFETCH(QString, dirName);
    QFETCH(QStringList, nameFilters);
    QFETCH(QDir::Filters, filters);
    QFETCH(QDirListing::IteratorFlags, flags);

    if (dirName == u"empty")
        return; // This row isn't useful in this test

    CustomEngineHandler handler;
    bool isEmpty = true;
    for (const auto &dirEntry : QDirListing(u"entrylist"_s, nameFilters, filters, flags)) {
        if (dirEntry.filePath().contains(u"entrylist"))
            isEmpty = false;  // At least one entry in `entrylist` dir
    }
    QVERIFY(!isEmpty); // At least one entry
}
#endif

void tst_QDirListing::absoluteFilePathsFromRelativeIteratorPath()
{
    for (const auto &dirEntry : QDirListing(u"entrylist/"_s, QDir::NoDotAndDotDot))
        QVERIFY(dirEntry.absoluteFilePath().contains("entrylist"));
}

void tst_QDirListing::recurseWithFilters() const
{
    QSet<QString> actualEntries;
    QSet<QString> expectedEntries;
    expectedEntries.insert(QString::fromLatin1("recursiveDirs/dir1/textFileB.txt"));
    expectedEntries.insert(QString::fromLatin1("recursiveDirs/textFileA.txt"));

    for (const auto &dirEntry : QDirListing(u"recursiveDirs/"_s, QStringList{u"*.txt"_s},
                                            QDir::Files, ItFlag::Recursive)) {
        actualEntries.insert(dirEntry.filePath());
    }

    QCOMPARE(actualEntries, expectedEntries);
}

void tst_QDirListing::longPath()
{
    QDir dir;
    dir.mkdir("longpaths");
    dir.cd("longpaths");

    QString dirName = "x";
    qsizetype n = 0;
    while (dir.exists(dirName) || dir.mkdir(dirName)) {
        ++n;
        dirName.append('x');
    }

    QDirListing dirList(dir.absolutePath(), QDir::NoDotAndDotDot|QDir::Dirs, ItFlag::Recursive);
    qsizetype m = 0;
    for (auto it = dirList.begin(); it != dirList.end(); ++it)
        ++m;

    QCOMPARE(n, m);

    dirName.chop(1);
    while (dirName.size() > 0 && dir.exists(dirName) && dir.rmdir(dirName))
        dirName.chop(1);

    dir.cdUp();
    dir.rmdir("longpaths");
}

void tst_QDirListing::dirorder()
{
    QStringList entries;
    for (const auto &dirEntry : QDirListing(u"foo"_s, ItFlag::Recursive))
        entries.append(dirEntry.filePath());

    QCOMPARE_GT(entries.indexOf(u"foo/bar"_s), entries.indexOf(u"foo"_s));
}

void tst_QDirListing::relativePaths()
{
    for (const auto &dirEntry : QDirListing(u"*"_s, ItFlag::Recursive))
        QCOMPARE(dirEntry.filePath(), QDir::cleanPath(dirEntry.filePath()));
}

#if defined(Q_OS_WIN)
void tst_QDirListing::uncPaths_data()
{
    QTest::addColumn<QString>("dirName");
    QTest::newRow("uncserver")
            <<QString("//" + QTest::uncServerName());
    QTest::newRow("uncserver/testshare")
            <<QString("//" + QTest::uncServerName() + "/testshare");
    QTest::newRow("uncserver/testshare/tmp")
            <<QString("//" + QTest::uncServerName() + "/testshare/tmp");
}
void tst_QDirListing::uncPaths()
{
    QFETCH(QString, dirName);
    constexpr auto dirFilters = QDir::AllEntries | QDir::NoDotAndDotDot;
    for (const auto &dirEntry : QDirListing(dirName, dirFilters, ItFlag::Recursive)) {
        const QString &filePath = dirEntry.filePath();
        QCOMPARE(filePath, QDir::cleanPath(filePath));
    }
}
#endif

#ifndef Q_OS_WIN
// In Unix it is easy to create hidden files, but in Windows it requires
// a special call since hidden files need to be "marked" while in Unix
// anything starting by a '.' is a hidden file.
// For that reason these two tests aren't run on Windows.

void tst_QDirListing::hiddenFiles()
{
    QStringList expected = {
        "hiddenDirs_hiddenFiles/normalFile"_L1,
        "hiddenDirs_hiddenFiles/.hiddenFile"_L1,
        "hiddenDirs_hiddenFiles/normalDirectory/normalFile"_L1,
        "hiddenDirs_hiddenFiles/normalDirectory/.hiddenFile"_L1,
        "hiddenDirs_hiddenFiles/.hiddenDirectory/normalFile"_L1,
        "hiddenDirs_hiddenFiles/.hiddenDirectory/.hiddenFile"_L1,
    };
    expected.sort();

    constexpr auto filters = QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot;
    QStringList list;
    list.reserve(expected.size());
    for (const auto &dirEntry : QDirListing(u"hiddenDirs_hiddenFiles"_s, filters,
                                            ItFlag::Recursive)) {
        QVERIFY(dirEntry.isFile());
        list.emplace_back(dirEntry.filePath());
    }

    list.sort();

    QCOMPARE_EQ(list, expected);
}

void tst_QDirListing::hiddenDirs()
{
    QStringList expected = {
        "hiddenDirs_hiddenFiles/normalDirectory"_L1,
        "hiddenDirs_hiddenFiles/normalDirectory/subdir"_L1,
        "hiddenDirs_hiddenFiles/normalDirectory/.hidden-subdir"_L1,
        "hiddenDirs_hiddenFiles/.hiddenDirectory"_L1,
        "hiddenDirs_hiddenFiles/.hiddenDirectory/subdir"_L1,
        "hiddenDirs_hiddenFiles/.hiddenDirectory/.hidden-subdir"_L1,
    };
    expected.sort();

    constexpr auto filters = QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot;
    QStringList list;
    list.reserve(expected.size());
    for (const auto &dirEntry : QDirListing(u"hiddenDirs_hiddenFiles"_s, filters,
                                            ItFlag::Recursive)) {
        QVERIFY(dirEntry.isDir());
        list.emplace_back(dirEntry.filePath());
    }
    list.sort();

    QCOMPARE_EQ(list, expected);
}

#endif // Q_OS_WIN

void tst_QDirListing::withStdAlgorithms()
{
    QDirListing dirList(u"entrylist"_s, QDir::AllEntries | QDir::NoDotAndDotDot, ItFlag::Recursive);

    std::for_each(dirList.cbegin(), dirList.cend(), [](const auto &dirEntry) {
        QVERIFY(dirEntry.absoluteFilePath().contains("entrylist"));
    });

    const auto fileName = "dummy"_L1;
    auto it = std::find_if(dirList.cbegin(), dirList.cend(), [fileName](const auto &dirEntry) {
        return dirEntry.fileName() == fileName;
    });
    QVERIFY(it != dirList.cend());
    QCOMPARE(it->fileName(), fileName);
}

QTEST_MAIN(tst_QDirListing)

#include "tst_qdirlisting.moc"

