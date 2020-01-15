/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Copyright (C) 2019 Intel Corporation.
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
#include <QtCore/QCoreApplication>
#include <QtCore/QScopeGuard>
#include <QtCore/private/qglobal_p.h>

class tst_QResourceEngine: public QObject
{
    Q_OBJECT

public:
    tst_QResourceEngine()
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
        : m_runtimeResourceRcc(QFileInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/runtime_resource.rcc")).absoluteFilePath())
#else
        : m_runtimeResourceRcc(QFINDTESTDATA("runtime_resource.rcc"))
#endif
    {}

private slots:
    void initTestCase();
    void cleanupTestCase();

    void checkUnregisterResource_data();
    void checkUnregisterResource();
    void compressedResource_data();
    void compressedResource();
    void checkStructure_data();
    void checkStructure();
    void searchPath_data();
    void searchPath();
#if QT_DEPRECATED_SINCE(5, 13)
    void searchPath_deprecated_data();
    void searchPath_deprecated();
#endif
    void doubleSlashInRoot();
    void setLocale();
    void lastModified();
    void resourcesInStaticPlugins();

private:
    const QString m_runtimeResourceRcc;
};


void tst_QResourceEngine::initTestCase()
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
    QString sourcePath(QStringLiteral(":/android_testdata/"));
    QString dataPath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

    QDirIterator it(sourcePath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();

        QFileInfo fileInfo = it.fileInfo();
        if (!fileInfo.isDir()) {
            QString destination(dataPath + QLatin1Char('/') + fileInfo.filePath().mid(sourcePath.length()));
            QFileInfo destinationFileInfo(destination);
            if (!destinationFileInfo.exists()) {
                QVERIFY(QDir().mkpath(destinationFileInfo.path()));
                QVERIFY(QFile::copy(fileInfo.filePath(), destination));
                QVERIFY(QFileInfo(destination).exists());
            }
        }
    }

    QVERIFY(QDir::setCurrent(dataPath));
#endif

    QVERIFY(!m_runtimeResourceRcc.isEmpty());
    QVERIFY(QResource::registerResource(m_runtimeResourceRcc));
    QVERIFY(QResource::registerResource(m_runtimeResourceRcc, "/secondary_root/"));
}

void tst_QResourceEngine::cleanupTestCase()
{
    // make sure we don't leak memory
    QVERIFY(QResource::unregisterResource(m_runtimeResourceRcc));
    QVERIFY(QResource::unregisterResource(m_runtimeResourceRcc, "/secondary_root/"));
}

void tst_QResourceEngine::compressedResource_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("compressionAlgo");
    QTest::addColumn<bool>("supported");

    QTest::newRow("uncompressed")
            << QFINDTESTDATA("uncompressed.rcc") << int(QResource::NoCompression) << true;
    QTest::newRow("zlib")
            << QFINDTESTDATA("zlib.rcc") << int(QResource::ZlibCompression) << true;
    QTest::newRow("zstd")
            << QFINDTESTDATA("zstd.rcc") << int(QResource::ZstdCompression) << QT_CONFIG(zstd);
}

// Note: generateResource.sh parses this line. Make sure it's a simple number.
#define ZERO_FILE_LEN   16384
// End note
void tst_QResourceEngine::compressedResource()
{
    QFETCH(QString, fileName);
    QFETCH(int, compressionAlgo);
    QFETCH(bool, supported);
    const QByteArray expectedData(ZERO_FILE_LEN, '\0');

    QVERIFY(!QResource("zero.txt").isValid());
    QCOMPARE(QResource::registerResource(fileName), supported);
    if (!supported)
        return;

    auto unregister = qScopeGuard([=] { QResource::unregisterResource(fileName); });

    QResource resource("zero.txt");
    QVERIFY(resource.isValid());
    QVERIFY(resource.size() > 0);
    QVERIFY(resource.data());
    QCOMPARE(resource.compressionAlgorithm(), QResource::Compression(compressionAlgo));

    if (compressionAlgo == QResource::NoCompression) {
        QCOMPARE(resource.size(), ZERO_FILE_LEN);
        QCOMPARE(memcmp(resource.data(), expectedData.data(), ZERO_FILE_LEN), 0);

        // API guarantees it will be QByteArray::fromRawData:
        QCOMPARE(static_cast<const void *>(resource.uncompressedData().constData()),
                 static_cast<const void *>(resource.data()));
    } else {
        // reasonable expectation:
        QVERIFY(resource.size() < ZERO_FILE_LEN);
    }

    // using the engine
    QFile f(":/zero.txt");
    QVERIFY(f.exists());
    QVERIFY(f.open(QIODevice::ReadOnly));

    // verify that we can decompress correctly
    QCOMPARE(resource.uncompressedSize(), ZERO_FILE_LEN);
    QCOMPARE(f.size(), ZERO_FILE_LEN);

    QByteArray data = resource.uncompressedData();
    QCOMPARE(data.size(), expectedData.size());
    QCOMPARE(data, expectedData);

    // decompression through the engine
    data = f.readAll();
    QCOMPARE(data.size(), expectedData.size());
    QCOMPARE(data, expectedData);
}


void tst_QResourceEngine::checkStructure_data()
{
    QTest::addColumn<QString>("pathName");
    QTest::addColumn<QByteArray>("contents");
    QTest::addColumn<QStringList>("containedFiles");
    QTest::addColumn<QStringList>("containedDirs");
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<qlonglong>("contentsSize");

    QFileInfo info;

    QStringList rootContents;
    rootContents << QLatin1String("aliasdir")
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
                 << QLatin1String("android_testdata")
#endif
                 << QLatin1String("otherdir")
                 << QLatin1String("runtime_resource")
                 << QLatin1String("searchpath1")
                 << QLatin1String("searchpath2")
                 << QLatin1String("secondary_root")
                 << QLatin1String("staticplugin")
                 << QLatin1String("test")
#if defined(BUILTIN_TESTDATA)
                 << QLatin1String("testqrc")
#endif
                 << QLatin1String("withoutslashes");

    QTest::newRow("root dir")          << QString(":/")
                                       << QByteArray()
                                       << (QStringList()
#if defined(BUILTIN_TESTDATA)
                                           << "parentdir.txt"
                                           << "runtime_resource.rcc"
#endif
                                           << "search_file.txt"
#if defined(BUILTIN_TESTDATA)
                                           << "uncompressed.rcc"
                                           << "zlib.rcc"
                                           << "zstd.rcc"
#endif
                                           )
                                       << rootContents
                                       << QLocale::c()
                                       << qlonglong(0);

    QTest::newRow("secondary root")  << QString(":/secondary_root/")
                                     << QByteArray()
                                     << QStringList()
                                     << (QStringList() << QLatin1String("runtime_resource"))
                                     << QLocale::c()
                                     << qlonglong(0);

    QStringList roots;
    roots << QString(":/") << QString(":/runtime_resource/") << QString(":/secondary_root/runtime_resource/");
    for(int i = 0; i < roots.size(); ++i) {
        const QString root = roots.at(i);

        QTest::addRow("%s prefix dir", qPrintable(root))  << QString(root + "test/abc/123/+++")
                                            << QByteArray()
                                            << (QStringList() << QLatin1String("currentdir.txt") << QLatin1String("currentdir2.txt") << QLatin1String("parentdir.txt"))
                                            << (QStringList() << QLatin1String("subdir"))
                                            << QLocale::c()
                                            << qlonglong(0);

        QTest::addRow("%s parent to prefix", qPrintable(root))  << QString(root + "test/abc/123")
                                                  << QByteArray()
                                                  << QStringList()
                                                  << (QStringList() << QLatin1String("+++"))
                                                  << QLocale::c()
                                                  << qlonglong(0);

        QTest::addRow("%s two parents prefix", qPrintable(root)) << QString(root + "test/abc")
                                                   << QByteArray()
                                                   << QStringList()
                                                   << QStringList(QLatin1String("123"))
                                                   << QLocale::c()
                                                   << qlonglong(0);

        QTest::addRow("%s test dir ", qPrintable(root))          << QString(root + "test")
                                                   << QByteArray()
                                                   << (QStringList() << QLatin1String("testdir.txt"))
                                                   << (QStringList() << QLatin1String("abc") << QLatin1String("test"))
                                                   << QLocale::c()
                                                   << qlonglong(0);

        QTest::addRow("%s prefix no slashes", qPrintable(root)) << QString(root + "withoutslashes")
                                                  << QByteArray()
                                                  << QStringList("blahblah.txt")
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(0);

        QTest::addRow("%s other dir", qPrintable(root))         << QString(root + "otherdir")
                                                  << QByteArray()
                                                  << QStringList(QLatin1String("otherdir.txt"))
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(0);

        QTest::addRow("%s alias dir", qPrintable(root))         << QString(root + "aliasdir")
                                                  << QByteArray()
                                                  << QStringList(QLatin1String("aliasdir.txt"))
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(0);

        QTest::addRow("%s second test dir", qPrintable(root))   << QString(root + "test/test")
                                                  << QByteArray()
                                                  << (QStringList() << QLatin1String("test1.txt") << QLatin1String("test2.txt"))
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(0);

        info = QFileInfo(QFINDTESTDATA("testqrc/test/test/test1.txt"));
        QTest::addRow("%s test1 text", qPrintable(root))        << QString(root + "test/test/test1.txt")
                                                  << QByteArray("abc\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/blahblah.txt"));
        QTest::addRow("%s text no slashes", qPrintable(root))   << QString(root + "withoutslashes/blahblah.txt")
                                                  << QByteArray("qwerty\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());


        info = QFileInfo(QFINDTESTDATA("testqrc/test/test/test2.txt"));
        QTest::addRow("%s test1 text", qPrintable(root))        << QString(root + "test/test/test2.txt")
                                                  << QByteArray("def\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/currentdir.txt"));
        QTest::addRow("%s currentdir text", qPrintable(root))   << QString(root + "test/abc/123/+++/currentdir.txt")
                                                  << QByteArray("\"This is the current dir\"\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/currentdir2.txt"));
        QTest::addRow("%s currentdir text2", qPrintable(root))  << QString(root + "test/abc/123/+++/currentdir2.txt")
                                                  << QByteArray("\"This is also the current dir\"\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("parentdir.txt"));
        QTest::addRow("%s parentdir text", qPrintable(root))    << QString(root + "test/abc/123/+++/parentdir.txt")
                                                  << QByteArray("abcdefgihklmnopqrstuvwxyz \n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/subdir/subdir.txt"));
        QTest::addRow("%s subdir text", qPrintable(root))       << QString(root + "test/abc/123/+++/subdir/subdir.txt")
                                                  << QByteArray("\"This is in the sub directory\"\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/test/testdir.txt"));
        QTest::addRow("%s testdir text", qPrintable(root))      << QString(root + "test/testdir.txt")
                                                  << QByteArray("\"This is in the test directory\"\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/otherdir/otherdir.txt"));
        QTest::addRow("%s otherdir text", qPrintable(root))     << QString(root + "otherdir/otherdir.txt")
                                                  << QByteArray("\"This is the other dir\"\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/test/testdir2.txt"));
        QTest::addRow("%s alias text", qPrintable(root))        << QString(root + "aliasdir/aliasdir.txt")
                                                  << QByteArray("\"This is another file in this directory\"\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/aliasdir/aliasdir.txt"));
        QTest::addRow("%s korean text", qPrintable(root))       << QString(root + "aliasdir/aliasdir.txt")
                                                  << QByteArray("\"This is a korean text file\"\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale("ko")
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/aliasdir/aliasdir.txt"));
        QTest::addRow("%s korean text 2", qPrintable(root))     << QString(root + "aliasdir/aliasdir.txt")
                                                  << QByteArray("\"This is a korean text file\"\n")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale("ko_KR")
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/test/german.txt"));
        QTest::addRow("%s german text", qPrintable(root))   << QString(root + "aliasdir/aliasdir.txt")
                                              << QByteArray("Deutsch\n")
                                              << QStringList()
                                              << QStringList()
                                              << QLocale("de")
                                              << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/test/german.txt"));
        QTest::addRow("%s german text 2", qPrintable(root))   << QString(root + "aliasdir/aliasdir.txt")
                                                << QByteArray("Deutsch\n")
                                                << QStringList()
                                                << QStringList()
                                                << QLocale("de_DE")
                                                << qlonglong(info.size());

        QFile file(QFINDTESTDATA("testqrc/aliasdir/compressme.txt"));
        file.open(QFile::ReadOnly);
        info = QFileInfo(QFINDTESTDATA("testqrc/aliasdir/compressme.txt"));
        QTest::addRow("%s compressed text", qPrintable(root))   << QString(root + "aliasdir/aliasdir.txt")
                                                  << file.readAll()
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale("de_CH")
                                                  << qlonglong(info.size());
    }
}

void tst_QResourceEngine::checkStructure()
{
    QFETCH(QString, pathName);
    QFETCH(QByteArray, contents);
    QFETCH(QStringList, containedFiles);
    QFETCH(QStringList, containedDirs);
    QFETCH(QLocale, locale);
    QFETCH(qlonglong, contentsSize);

    bool directory = (containedDirs.size() + containedFiles.size() > 0);
    QLocale::setDefault(locale);

    QFileInfo fileInfo(pathName);

    QVERIFY(fileInfo.exists());
    QCOMPARE(fileInfo.isDir(), directory);
    QCOMPARE(fileInfo.size(), contentsSize);
    QVERIFY(fileInfo.isReadable());
    QVERIFY(!fileInfo.isWritable());
    QVERIFY(!fileInfo.isExecutable());

    if (directory) {
        QDir dir(pathName);

        // Test the Dir filter
        QFileInfoList list = dir.entryInfoList(QDir::Dirs, QDir::Name);
        QCOMPARE(list.size(), containedDirs.size());

        int i;
        for (i=0; i<list.size(); ++i) {
            QVERIFY(list.at(i).isDir());
            QCOMPARE(list.at(i).fileName(), containedDirs.at(i));
        }

        list = dir.entryInfoList(QDir::Files, QDir::Name);
        QCOMPARE(list.size(), containedFiles.size());

        for (i=0; i<list.size(); ++i) {
            QVERIFY(!list.at(i).isDir());
            QCOMPARE(list.at(i).fileName(), containedFiles.at(i));
        }

        list = dir.entryInfoList(QDir::NoFilter, QDir::SortFlags(QDir::Name | QDir::DirsFirst));
        QCOMPARE(list.size(), containedFiles.size() + containedDirs.size());

        for (i=0; i<list.size(); ++i) {
            QString expectedName;
            if (i < containedDirs.size())
                expectedName = containedDirs.at(i);
            else
                expectedName = containedFiles.at(i - containedDirs.size());

            QCOMPARE(list.at(i).fileName(), expectedName);
        }
    } else {
        QFile file(pathName);
        QVERIFY(file.open(QFile::ReadOnly));

        // check contents
        QCOMPARE(file.readAll(), contents);

        // check memory map too
        uchar *ptr = file.map(0, file.size(), QFile::MapPrivateOption);
        QVERIFY2(ptr, qPrintable(file.errorString()));
        QByteArray ba = QByteArray::fromRawData(reinterpret_cast<const char *>(ptr), file.size());
        QCOMPARE(ba, contents);

        // check that it is still valid after closing the file
        file.close();
        QCOMPARE(ba, contents);
    }
    QLocale::setDefault(QLocale::system());
}

void tst_QResourceEngine::searchPath_data()
{
    auto searchPath = QFileInfo(QFINDTESTDATA("testqrc")).canonicalFilePath();

    QTest::addColumn<QString>("searchPathPrefix");
    QTest::addColumn<QString>("searchPath");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("no_search_path")
            << QString()
            << QString()
            << ":search_file.txt"
            << QByteArray("root\n");
    QTest::newRow("path1")
            << "searchpath1"
            << searchPath
            << "searchpath1:searchpath1/search_file.txt"
            << QByteArray("path1\n");
    QTest::newRow("no_search_path2")
            << QString()
            << QString()
            << ":/search_file.txt"
            << QByteArray("root\n");
    QTest::newRow("path2")
            << "searchpath2"
            << searchPath + "/searchpath2"
            << "searchpath2:search_file.txt"
            << QByteArray("path2\n");
}

void tst_QResourceEngine::searchPath()
{
    QFETCH(QString, searchPathPrefix);
    QFETCH(QString, searchPath);
    QFETCH(QString, file);
    QFETCH(QByteArray, expected);

    if (!searchPath.isEmpty())
        QDir::addSearchPath(searchPathPrefix, searchPath);
    QFile qf(file);
    QVERIFY(qf.open(QFile::ReadOnly));
    QByteArray actual = qf.readAll();

    actual.replace('\r', "");

    QCOMPARE(actual, expected);
    qf.close();
}

#if QT_DEPRECATED_SINCE(5, 13)

void tst_QResourceEngine::searchPath_deprecated_data()
{
    QTest::addColumn<QString>("searchPath");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("no_search_path")  << QString()
                                  << ":search_file.txt"
                                  << QByteArray("root\n");
    QTest::newRow("path1")  << "/searchpath1"
                         << ":search_file.txt"
                         << QByteArray("path1\n");
    QTest::newRow("no_search_path2")  << QString()
                                  << ":/search_file.txt"
                                  << QByteArray("root\n");
    QTest::newRow("path2")  << "/searchpath2"
                         << ":search_file.txt"
                         << QByteArray("path2\n");
}

void tst_QResourceEngine::searchPath_deprecated()
{
    QFETCH(QString, searchPath);
    QFETCH(QString, file);
    QFETCH(QByteArray, expected);

    if(!searchPath.isEmpty())
        QDir::addResourceSearchPath(searchPath);
    QFile qf(file);
    QVERIFY(qf.open(QFile::ReadOnly));
    QByteArray actual = qf.readAll();

    actual.replace('\r', "");

    QCOMPARE(actual, expected);
    qf.close();
}

#endif

void tst_QResourceEngine::checkUnregisterResource_data()
{
    QTest::addColumn<QString>("rcc_file");
    QTest::addColumn<QString>("root");
    QTest::addColumn<QString>("file_check");
    QTest::addColumn<int>("size");

    QTest::newRow("currentdir.txt") << QFINDTESTDATA("runtime_resource.rcc") << QString("/check_unregister/")
                                    << QString(":/check_unregister/runtime_resource/test/abc/123/+++/currentdir.txt")
                                    << (int)QFileInfo(QFINDTESTDATA("testqrc/currentdir.txt")).size();
}

void tst_QResourceEngine::checkUnregisterResource()
{
    QFETCH(QString, rcc_file);
    QFETCH(QString, root);
    QFETCH(QString, file_check);
    QFETCH(int, size);



    QVERIFY(!QFile::exists(file_check));
    QVERIFY(QResource::registerResource(rcc_file, root));
    QVERIFY(QFile::exists(file_check));
    QVERIFY(QResource::unregisterResource(rcc_file, root));
    QVERIFY(!QFile::exists(file_check));
    QVERIFY(QResource::registerResource(rcc_file, root));
    QVERIFY(QFile::exists(file_check));
    QFileInfo fileInfo(file_check);
    fileInfo.setCaching(false);
    QVERIFY(fileInfo.exists());
    QVERIFY(!QResource::unregisterResource(rcc_file, root));
    QVERIFY(!QFile::exists(file_check));
    QCOMPARE((int)fileInfo.size(), size);
}

void tst_QResourceEngine::doubleSlashInRoot()
{
    QVERIFY(QFile::exists(":/secondary_root/runtime_resource/search_file.txt"));
    QVERIFY(QFile::exists("://secondary_root/runtime_resource/search_file.txt"));
}

void tst_QResourceEngine::setLocale()
{
    QLocale::setDefault(QLocale::c());

    // default constructed QResource gets the default locale
    QResource resource;
    resource.setFileName("aliasdir/aliasdir.txt");
    QCOMPARE(resource.compressionAlgorithm(), QResource::NoCompression);

    // change the default locale and make sure it doesn't affect the resource
    QLocale::setDefault(QLocale("de_CH"));
    QCOMPARE(resource.compressionAlgorithm(), QResource::NoCompression);

    // then explicitly set the locale on qresource
    resource.setLocale(QLocale("de_CH"));
    QVERIFY(resource.compressionAlgorithm() != QResource::NoCompression);

    // the reset the default locale back
    QLocale::setDefault(QLocale::system());
}

void tst_QResourceEngine::lastModified()
{
    {
        QFileInfo fi(":/");
        QVERIFY(fi.exists());
        QVERIFY2(!fi.lastModified().isValid(), qPrintable(fi.lastModified().toString()));
    }
    {
        QFileInfo fi(":/search_file.txt");
        QVERIFY(fi.exists());
        QVERIFY(fi.lastModified().isValid());
    }
}

Q_IMPORT_PLUGIN(PluginClass)
void tst_QResourceEngine::resourcesInStaticPlugins()
{
    // We built a separate static plugin and attempted linking against
    // it. That should successfully register the resources linked into
    // the plugin via moc generated Q_INIT_RESOURCE calls in a
    // Q_CONSTRUCTOR_FUNCTION.
    QVERIFY(QFile::exists(":/staticplugin/main.cpp"));
}

QTEST_MAIN(tst_QResourceEngine)

#include "tst_qresourceengine.moc"

