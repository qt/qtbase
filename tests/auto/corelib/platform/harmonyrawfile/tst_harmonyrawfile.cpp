// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qdir.h>
#include <QtCore/qdirlisting.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtTest/qtest.h>

#include <future>
#include <vector>

using namespace Qt::StringLiterals;

namespace {

constexpr char testText[] = "hello rawfile\n";
constexpr char dataJson[] = "{\"rawfile\": true}\n";
constexpr auto testTextPath = "rawfile:/harmonyrawfile/test.txt"_L1;
constexpr auto dataJsonPath = "rawfile:/harmonyrawfile/sub/data.json"_L1;

}

class tst_HarmonyRawFile : public QObject
{
    Q_OBJECT

private slots:
    void read_data();
    void read();
    void seek();
    void seekPastEnd();
    void seekNegative_fails();
    void readFromWorkerThreads();
    void openForWriting_fails_data();
    void openForWriting_fails();
    void openNonFile_fails_data();
    void openNonFile_fails();
    void fileInfo_data();
    void fileInfo();
    void fileInfo_names();
    void listDirectory_data();
    void listDirectory();
    void listFile_isEmpty();
    void listRoot();
    void listRecursively();
};

void tst_HarmonyRawFile::read_data()
{
    QTest::addColumn<QString>("fileName");

    QTest::newRow("canonical") << QString(testTextPath);
    QTest::newRow("no-leading-slash") << u"rawfile:harmonyrawfile/test.txt"_s;
    QTest::newRow("url-like") << u"rawfile:///harmonyrawfile/test.txt"_s;
    QTest::newRow("double-slash") << u"rawfile:/harmonyrawfile//test.txt"_s;
    QTest::newRow("dot-segments") << u"rawfile:/harmonyrawfile/./sub/../test.txt"_s;
}

void tst_HarmonyRawFile::read()
{
    QFETCH(QString, fileName);

    QFile file(fileName);
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    QCOMPARE(file.size(), qint64(qstrlen(testText)));
    QCOMPARE(file.readAll(), QByteArray(testText));
    QVERIFY(file.atEnd());
    file.close();
    QCOMPARE(file.error(), QFile::NoError);
}

void tst_HarmonyRawFile::seek()
{
    QFile file(testTextPath);
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));

    QVERIFY(file.seek(6));
    QCOMPARE(file.pos(), qint64(6));
    QCOMPARE(file.read(7), "rawfile");
    QVERIFY(file.seek(0));
    QCOMPARE(file.read(5), "hello");
}

void tst_HarmonyRawFile::seekPastEnd()
{
    QFile file(testTextPath);
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));

    QVERIFY(file.seek(file.size()));
    QVERIFY(file.atEnd());
    QVERIFY(file.seek(file.size() + 10));
    QCOMPARE(file.read(1), QByteArray());
    QVERIFY(file.seek(0));
    QCOMPARE(file.read(5), "hello");
}

void tst_HarmonyRawFile::seekNegative_fails()
{
    QFile file(testTextPath);
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));

    QVERIFY(file.seek(6));
    QVERIFY(!file.seek(-1));
    QCOMPARE(file.read(7), "rawfile");
}

void tst_HarmonyRawFile::readFromWorkerThreads()
{
    constexpr int threadCount = 4;
    constexpr int readsPerThread = 50;
    std::vector<std::future<int>> failedReadCounts;
    for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        failedReadCounts.push_back(std::async(
            std::launch::async,
            []() {
                int failedReads = 0;
                for (int readIndex = 0; readIndex < readsPerThread; ++readIndex) {
                    QFile file(testTextPath);
                    if (!file.open(QIODevice::ReadOnly) || file.readAll() != testText)
                        ++failedReads;
                }
                return failedReads;
            }));
    }
    int failedReads = 0;
    for (std::future<int> &failedReadCount : failedReadCounts)
        failedReads += failedReadCount.get();

    QCOMPARE(failedReads, 0);
}

void tst_HarmonyRawFile::openForWriting_fails_data()
{
    QTest::addColumn<QIODevice::OpenMode>("openMode");

    QTest::newRow("WriteOnly") << QIODevice::OpenMode(QIODevice::WriteOnly);
    QTest::newRow("ReadWrite") << QIODevice::OpenMode(QIODevice::ReadWrite);
    QTest::newRow("Append") << QIODevice::OpenMode(QIODevice::Append);
}

void tst_HarmonyRawFile::openForWriting_fails()
{
    QFETCH(QIODevice::OpenMode, openMode);

    QFile file(testTextPath);
    QVERIFY(!file.open(openMode));
    QCOMPARE(file.error(), QFile::OpenError);
    QCOMPARE(file.size(), qint64(qstrlen(testText)));
}

void tst_HarmonyRawFile::openNonFile_fails_data()
{
    QTest::addColumn<QString>("fileName");

    QTest::newRow("directory") << u"rawfile:/harmonyrawfile/sub"_s;
    QTest::newRow("root") << u"rawfile:/"_s;
}

void tst_HarmonyRawFile::openNonFile_fails()
{
    QFETCH(QString, fileName);

    QFile file(fileName);
    QVERIFY(!file.open(QIODevice::ReadOnly));
    QCOMPARE(file.error(), QFile::OpenError);
}

void tst_HarmonyRawFile::fileInfo_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<bool>("exists");
    QTest::addColumn<bool>("isFile");
    QTest::addColumn<bool>("isDir");
    QTest::addColumn<bool>("isRoot");
    QTest::addColumn<qint64>("size");

    QTest::newRow("file")
        << QString(testTextPath) << true << true << false << false << qint64(qstrlen(testText));
    QTest::newRow("nested-file")
        << QString(dataJsonPath) << true << true << false << false << qint64(qstrlen(dataJson));
    QTest::newRow("directory")
        << u"rawfile:/harmonyrawfile/sub"_s << true << false << true << false << qint64(0);
    QTest::newRow("directory-trailing-slash")
        << u"rawfile:/harmonyrawfile/sub/"_s << true << false << true << false << qint64(0);
    QTest::newRow("root") << u"rawfile:/"_s << true << false << true << true << qint64(0);
    QTest::newRow("root-without-slash")
        << u"rawfile:"_s << true << false << true << true << qint64(0);
    QTest::newRow("missing")
        << u"rawfile:/harmonyrawfile/missing.txt"_s << false << false << false << false
        << qint64(0);
    QTest::newRow("above-root")
        << u"rawfile:/../harmonyrawfile/test.txt"_s << false << false << false << false
        << qint64(0);
    QTest::newRow("above-root-after-segment")
        << u"rawfile:/harmonyrawfile/../../harmonyrawfile/test.txt"_s << false << false << false
        << false << qint64(0);
    QTest::newRow("embedded-nul")
        << QString(testTextPath + QChar(u'\0') + u".bak"_s) << false << false << false << false
        << qint64(0);
}

void tst_HarmonyRawFile::fileInfo()
{
    QFETCH(QString, fileName);
    QFETCH(bool, exists);
    QFETCH(bool, isFile);
    QFETCH(bool, isDir);
    QFETCH(bool, isRoot);
    QFETCH(qint64, size);

    const QFileInfo info(fileName);
    QCOMPARE(info.exists(), exists);
    QCOMPARE(info.isFile(), isFile);
    QCOMPARE(info.isDir(), isDir);
    QCOMPARE(info.isRoot(), isRoot);
    QCOMPARE(info.size(), size);
    QCOMPARE(info.isReadable(), exists);
    QCOMPARE(info.canonicalFilePath().isEmpty(), !exists);
    QCOMPARE(info.canonicalPath().isEmpty(), !exists);
    QVERIFY(!info.isWritable());
    QVERIFY(!info.isSymLink());
}

void tst_HarmonyRawFile::fileInfo_names()
{
    const QString fileName = u"rawfile:harmonyrawfile//sub/data.json"_s;
    const QFileInfo info(fileName);
    QCOMPARE(info.filePath(), fileName);
    QCOMPARE(info.fileName(), u"data.json"_s);
    QCOMPARE(info.absoluteFilePath(), dataJsonPath);
    QCOMPARE(info.absolutePath(), u"rawfile:/harmonyrawfile/sub"_s);
    QCOMPARE(info.canonicalFilePath(), dataJsonPath);
    QVERIFY(info.isAbsolute());
    QCOMPARE(QFile(fileName).fileName(), fileName);
}

void tst_HarmonyRawFile::listDirectory_data()
{
    QTest::addColumn<QString>("path");

    QTest::newRow("canonical") << u"rawfile:/harmonyrawfile"_s;
    QTest::newRow("no-leading-slash") << u"rawfile:harmonyrawfile"_s;
    QTest::newRow("url-like") << u"rawfile:///harmonyrawfile"_s;
}

void tst_HarmonyRawFile::listDirectory()
{
    QFETCH(QString, path);

    const QDir dir(path);
    QVERIFY(dir.exists());
    QCOMPARE(
        dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name),
        QStringList({ u"sub"_s, u"test.txt"_s }));
    QCOMPARE(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot), QStringList({ u"sub"_s }));
    QCOMPARE(dir.entryList(QDir::Files, QDir::Name), QStringList({ u"test.txt"_s }));
}

void tst_HarmonyRawFile::listFile_isEmpty()
{
    QStringList entryPaths;
    for (const auto &entry : QDirListing(testTextPath))
        entryPaths.append(entry.filePath());
    QCOMPARE(entryPaths, QStringList());
}

void tst_HarmonyRawFile::listRoot()
{
    const QDir root(u"rawfile:/"_s);
    QVERIFY(root.exists());
    QVERIFY(root.entryList(QDir::Dirs | QDir::NoDotAndDotDot).contains(u"harmonyrawfile"_s));
}

void tst_HarmonyRawFile::listRecursively()
{
    QStringList filePaths;
    const QDirListing listing(
        u"rawfile:/harmonyrawfile"_s,
        QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::Recursive);
    for (const auto &entry : listing)
        filePaths.append(entry.filePath());
    filePaths.sort();

    const QStringList expectedFilePaths = {
        dataJsonPath,
        testTextPath,
    };
    QCOMPARE(filePaths, expectedFilePaths);
}

QTEST_GUILESS_MAIN(tst_HarmonyRawFile)
#include "tst_harmonyrawfile.moc"
