/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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
#include <QtCore/QCoreApplication>

class tst_QResourceEngine: public QObject
{
    Q_OBJECT

public:
    tst_QResourceEngine()
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
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
    void checkStructure_data();
    void checkStructure();
    void searchPath_data();
    void searchPath();
    void doubleSlashInRoot();
    void setLocale();

private:
    const QString m_runtimeResourceRcc;
};


void tst_QResourceEngine::initTestCase()
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
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

void tst_QResourceEngine::checkStructure_data()
{
    QTest::addColumn<QString>("pathName");
    QTest::addColumn<QString>("contents");
    QTest::addColumn<QStringList>("containedFiles");
    QTest::addColumn<QStringList>("containedDirs");
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<qlonglong>("contentsSize");

    QFileInfo info;

    QStringList rootContents;
    rootContents << QLatin1String("aliasdir")
                 << QLatin1String("otherdir")
                 << QLatin1String("qt-project.org")
                 << QLatin1String("runtime_resource")
                 << QLatin1String("searchpath1")
                 << QLatin1String("searchpath2")
                 << QLatin1String("secondary_root")
                 << QLatin1String("test")
                 << QLatin1String("withoutslashes");

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
    rootContents.insert(1, QLatin1String("android_testdata"));
#endif

    QTest::newRow("root dir")          << QString(":/")
                                       << QString()
                                       << (QStringList() << "search_file.txt")
                                       << rootContents
                                       << QLocale::c()
                                       << qlonglong(0);

    QTest::newRow("secondary root")  << QString(":/secondary_root/")
                                     << QString()
                                     << QStringList()
                                     << (QStringList() << QLatin1String("runtime_resource"))
                                     << QLocale::c()
                                     << qlonglong(0);

    QStringList roots;
    roots << QString(":/") << QString(":/runtime_resource/") << QString(":/secondary_root/runtime_resource/");
    for(int i = 0; i < roots.size(); ++i) {
        const QString root = roots.at(i);

        QTest::newRow(QString(root + "prefix dir").toLatin1().constData())  << QString(root + "test/abc/123/+++")
                                            << QString()
                                            << (QStringList() << QLatin1String("currentdir.txt") << QLatin1String("currentdir2.txt") << QLatin1String("parentdir.txt"))
                                            << (QStringList() << QLatin1String("subdir"))
                                            << QLocale::c()
                                            << qlonglong(0);

        QTest::newRow(QString(root + "parent to prefix").toLatin1().constData())  << QString(root + "test/abc/123")
                                                  << QString()
                                                  << QStringList()
                                                  << (QStringList() << QLatin1String("+++"))
                                                  << QLocale::c()
                                                  << qlonglong(0);

        QTest::newRow(QString(root + "two parents prefix").toLatin1().constData()) << QString(root + "test/abc")
                                                   << QString()
                                                   << QStringList()
                                                   << QStringList(QLatin1String("123"))
                                                   << QLocale::c()
                                                   << qlonglong(0);

        QTest::newRow(QString(root + "test dir ").toLatin1().constData())          << QString(root + "test")
                                                   << QString()
                                                   << (QStringList() << QLatin1String("testdir.txt"))
                                                   << (QStringList() << QLatin1String("abc") << QLatin1String("test"))
                                                   << QLocale::c()
                                                   << qlonglong(0);

        QTest::newRow(QString(root + "prefix no slashes").toLatin1().constData()) << QString(root + "withoutslashes")
                                                  << QString()
                                                  << QStringList("blahblah.txt")
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(0);

        QTest::newRow(QString(root + "other dir").toLatin1().constData())         << QString(root + "otherdir")
                                                  << QString()
                                                  << QStringList(QLatin1String("otherdir.txt"))
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(0);

        QTest::newRow(QString(root + "alias dir").toLatin1().constData())         << QString(root + "aliasdir")
                                                  << QString()
                                                  << QStringList(QLatin1String("aliasdir.txt"))
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(0);

        QTest::newRow(QString(root + "second test dir").toLatin1().constData())   << QString(root + "test/test")
                                                  << QString()
                                                  << (QStringList() << QLatin1String("test1.txt") << QLatin1String("test2.txt"))
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(0);

        info = QFileInfo(QFINDTESTDATA("testqrc/test/test/test1.txt"));
        QTest::newRow(QString(root + "test1 text").toLatin1().constData())        << QString(root + "test/test/test1.txt")
                                                  << QString("abc")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/blahblah.txt"));
        QTest::newRow(QString(root + "text no slashes").toLatin1().constData())   << QString(root + "withoutslashes/blahblah.txt")
                                                  << QString("qwerty")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());


        info = QFileInfo(QFINDTESTDATA("testqrc/test/test/test2.txt"));
        QTest::newRow(QString(root + "test1 text").toLatin1().constData())        << QString(root + "test/test/test2.txt")
                                                  << QString("def")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/currentdir.txt"));
        QTest::newRow(QString(root + "currentdir text").toLatin1().constData())   << QString(root + "test/abc/123/+++/currentdir.txt")
                                                  << QString("\"This is the current dir\" ")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/currentdir2.txt"));
        QTest::newRow(QString(root + "currentdir text2").toLatin1().constData())  << QString(root + "test/abc/123/+++/currentdir2.txt")
                                                  << QString("\"This is also the current dir\" ")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("parentdir.txt"));
        QTest::newRow(QString(root + "parentdir text").toLatin1().constData())    << QString(root + "test/abc/123/+++/parentdir.txt")
                                                  << QString("abcdefgihklmnopqrstuvwxyz ")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/subdir/subdir.txt"));
        QTest::newRow(QString(root + "subdir text").toLatin1().constData())       << QString(root + "test/abc/123/+++/subdir/subdir.txt")
                                                  << QString("\"This is in the sub directory\" ")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/test/testdir.txt"));
        QTest::newRow(QString(root + "testdir text").toLatin1().constData())      << QString(root + "test/testdir.txt")
                                                  << QString("\"This is in the test directory\" ")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/otherdir/otherdir.txt"));
        QTest::newRow(QString(root + "otherdir text").toLatin1().constData())     << QString(root + "otherdir/otherdir.txt")
                                                  << QString("\"This is the other dir\" ")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/test/testdir2.txt"));
        QTest::newRow(QString(root + "alias text").toLatin1().constData())        << QString(root + "aliasdir/aliasdir.txt")
                                                  << QString("\"This is another file in this directory\" ")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale::c()
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/aliasdir/aliasdir.txt"));
        QTest::newRow(QString(root + "korean text").toLatin1().constData())       << QString(root + "aliasdir/aliasdir.txt")
                                                  << QString("\"This is a korean text file\" ")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale("ko")
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/aliasdir/aliasdir.txt"));
        QTest::newRow(QString(root + "korean text 2").toLatin1().constData())     << QString(root + "aliasdir/aliasdir.txt")
                                                  << QString("\"This is a korean text file\" ")
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale("ko_KR")
                                                  << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/test/german.txt"));
        QTest::newRow(QString(root + "german text").toLatin1().constData())   << QString(root + "aliasdir/aliasdir.txt")
                                              << QString("Deutsch")
                                              << QStringList()
                                              << QStringList()
                                              << QLocale("de")
                                              << qlonglong(info.size());

        info = QFileInfo(QFINDTESTDATA("testqrc/test/german.txt"));
        QTest::newRow(QString(root + "german text 2").toLatin1().constData())   << QString(root + "aliasdir/aliasdir.txt")
                                                << QString("Deutsch")
                                                << QStringList()
                                                << QStringList()
                                                << QLocale("de_DE")
                                                << qlonglong(info.size());

        QFile file(QFINDTESTDATA("testqrc/aliasdir/compressme.txt"));
        file.open(QFile::ReadOnly);
        info = QFileInfo(QFINDTESTDATA("testqrc/aliasdir/compressme.txt"));
        QTest::newRow(QString(root + "compressed text").toLatin1().constData())   << QString(root + "aliasdir/aliasdir.txt")
                                                  << QString(file.readAll())
                                                  << QStringList()
                                                  << QStringList()
                                                  << QLocale("de_CH")
                                                  << qlonglong(info.size());
    }
}

void tst_QResourceEngine::checkStructure()
{
    QFETCH(QString, pathName);
    QFETCH(QString, contents);
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
        QCOMPARE(containedFiles.size(), list.size());

        for (i=0; i<list.size(); ++i) {
            QVERIFY(!list.at(i).isDir());
            QCOMPARE(list.at(i).fileName(), containedFiles.at(i));
        }

        list = dir.entryInfoList(QDir::NoFilter, QDir::SortFlags(QDir::Name | QDir::DirsFirst));
        QCOMPARE(containedFiles.size() + containedDirs.size(), list.size());

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

        QByteArray ba = file.readAll();
        QVERIFY(QString(ba).startsWith(contents));
    }
    QLocale::setDefault(QLocale::system());
}

void tst_QResourceEngine::searchPath_data()
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

void tst_QResourceEngine::searchPath()
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
    QVERIFY(!resource.isCompressed());

    // change the default locale and make sure it doesn't affect the resource
    QLocale::setDefault(QLocale("de_CH"));
    QVERIFY(!resource.isCompressed());

    // then explicitly set the locale on qresource
    resource.setLocale(QLocale("de_CH"));
    QVERIFY(resource.isCompressed());

    // the reset the default locale back
    QLocale::setDefault(QLocale::system());
}

QTEST_MAIN(tst_QResourceEngine)

#include "tst_qresourceengine.moc"

