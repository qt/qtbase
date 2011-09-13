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

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qstringlist.h>
#include "../../../network-settings.h"

#if defined(Q_OS_WIN)
#define _WIN32_WINNT  0x500
#endif

#include "../../../../shared/filesystem.h"

#if defined(Q_OS_SYMBIAN)
# include <f32file.h>
# define STRINGIFY(x) #x
# define TOSTRING(x) STRINGIFY(x)
# define SRCDIR "C:/Private/" TOSTRING(SYMBIAN_SRCDIR_UID) "/"
#elif defined(Q_OS_UNIX)
# include <unistd.h>
# include <sys/stat.h>
#endif

#if defined(Q_OS_VXWORKS)
#define Q_NO_SYMLINKS
#endif

#if defined(Q_OS_SYMBIAN)
#define Q_NO_SYMLINKS
#define Q_NO_SYMLINKS_TO_DIRS
#endif


//TESTED_CLASS=
//TESTED_FILES=

class tst_QDir : public QObject
{
Q_OBJECT

public:
    tst_QDir();
    virtual ~tst_QDir();

private slots:
    void getSetCheck();
    void construction();

    void setPath_data();
    void setPath();

    void entryList_data();
    void entryList();

    void entryListSimple_data();
    void entryListSimple();

    void entryListWithSymLinks();

    void mkdir_data();
    void mkdir();

    void makedirReturnCode();

    void rmdir_data();
    void rmdir();

    void exists_data();
    void exists();

    void isRelativePath_data();
    void isRelativePath();

    void canonicalPath_data();
    void canonicalPath();

    void current_data();
    void current();

    void cd_data();
    void cd();

    void setNameFilters_data();
    void setNameFilters();

    void cleanPath_data();
    void cleanPath();

    void compare();
    void QDir_default();

    void filePath_data();
    void filePath();

    void absoluteFilePath_data();
    void absoluteFilePath();

    void absolutePath_data();
    void absolutePath();

    void relativeFilePath_data();
    void relativeFilePath();

    void remove();
    void rename();

    void exists2_data();
    void exists2();

    void dirName_data();
    void dirName();

    void operator_eq();

    void dotAndDotDot();
    void homePath();
    void tempPath();
    void rootPath();

    void nativeSeparators();

    void searchPaths();
    void searchPaths_data();

    void entryListWithSearchPaths();

    void longFileName_data();
    void longFileName();

    void updateFileLists();

    void detachingOperations();

    void testCaching();

    void isRoot_data();
    void isRoot();

#ifndef QT_NO_REGEXP
    void match_data();
    void match();
#endif

    void drives();

    void arrayOperator();

    void equalityOperator_data();
    void equalityOperator();

    void isRelative_data();
    void isRelative();

    void isReadable();
};

// Testing get/set functions
void tst_QDir::getSetCheck()
{
    QDir obj1;
    // Filters QDir::filter()
    // void QDir::setFilter(Filters)
    obj1.setFilter(QDir::Filters(QDir::Dirs));
    QCOMPARE(QDir::Filters(QDir::Dirs), obj1.filter());
    obj1.setFilter(QDir::Filters(QDir::Dirs | QDir::Files));
    QCOMPARE(QDir::Filters(QDir::Dirs | QDir::Files), obj1.filter());
    obj1.setFilter(QDir::Filters(QDir::NoFilter));
    QCOMPARE(QDir::Filters(QDir::NoFilter), obj1.filter());

    // SortFlags QDir::sorting()
    // void QDir::setSorting(SortFlags)
    obj1.setSorting(QDir::SortFlags(QDir::Name));
    QCOMPARE(QDir::SortFlags(QDir::Name), obj1.sorting());
    obj1.setSorting(QDir::SortFlags(QDir::Name | QDir::IgnoreCase));
    QCOMPARE(QDir::SortFlags(QDir::Name | QDir::IgnoreCase), obj1.sorting());
    obj1.setSorting(QDir::SortFlags(QDir::NoSort));
    QCOMPARE(QDir::SortFlags(QDir::NoSort), obj1.sorting());
}

tst_QDir::tst_QDir()
{
#ifdef Q_OS_SYMBIAN
    // Can't deploy empty test dir, so create it here
    QDir dir(SRCDIR);
    dir.mkdir("testData");
#endif
}

tst_QDir::~tst_QDir()
{
#ifdef Q_OS_SYMBIAN
    // Remove created test dir
    QDir dir(SRCDIR);
    dir.rmdir("testData");
#endif
}

void tst_QDir::construction()
{
    QFileInfo myFileInfo("/machine/share/dir1/file1");
    QDir myDir(myFileInfo.absoluteDir()); // this asserted
    QCOMPARE(myFileInfo.absoluteDir().absolutePath(), myDir.absolutePath());
}

void tst_QDir::setPath_data()
{
    QTest::addColumn<QString>("dir1");
    QTest::addColumn<QString>("dir2");

    QTest::newRow("data0") << QString(".") << QString("..");
#if (defined(Q_WS_WIN) && !defined(Q_OS_WINCE)) || defined(Q_OS_SYMBIAN)
    QTest::newRow("data1") << QString("c:/") << QDir::currentPath();
#endif
}

void tst_QDir::setPath()
{
    QFETCH(QString, dir1);
    QFETCH(QString, dir2);

    QDir shared;
    QDir qDir1(dir1);
    QStringList entries1 = qDir1.entryList();
    shared.setPath(dir1);
    QCOMPARE(shared.entryList(), entries1);

    QDir qDir2(dir2);
    QStringList entries2 = qDir2.entryList();
    shared.setPath(dir2);
    QCOMPARE(shared.entryList(), entries2);
}

void tst_QDir::mkdir_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("recurse");

    QStringList dirs;
    dirs << QDir::currentPath() + "/testdir/one/two/three"
         << QDir::currentPath() + "/testdir/two"
         << QDir::currentPath() + "/testdir/two/three";
    QTest::newRow("data0") << dirs.at(0) << true;
    QTest::newRow("data1") << dirs.at(1) << false;
    QTest::newRow("data2") << dirs.at(2) << false;

    // Ensure that none of these directories already exist
    QDir dir;
    for (int i = 0; i < dirs.count(); ++i)
        dir.rmpath(dirs.at(i));
}

void tst_QDir::mkdir()
{
    QFETCH(QString, path);
    QFETCH(bool, recurse);

    QDir dir;
    dir.rmdir(path);
    if (recurse)
        QVERIFY(dir.mkpath(path));
    else
        QVERIFY(dir.mkdir(path));

    //make sure it really exists (ie that mkdir returns the right value)
    QFileInfo fi(path);
    QVERIFY(fi.exists() && fi.isDir());
}

void tst_QDir::makedirReturnCode()
{
    QString dirName = QString::fromLatin1("makedirReturnCode");
    QDir::current().rmdir(dirName); // cleanup a previous run.
    QDir dir(dirName);
    QVERIFY(!dir.exists());
    QVERIFY(QDir::current().mkdir(dirName));
    QVERIFY(!QDir::current().mkdir(dirName)); // calling mkdir on an existing dir will fail.
    QVERIFY(QDir::current().mkpath(dirName)); // calling mkpath on an existing dir will pass
}

void tst_QDir::rmdir_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("recurse");

    QTest::newRow("data0") << QDir::currentPath() + "/testdir/one/two/three" << true;
    QTest::newRow("data1") << QDir::currentPath() + "/testdir/two/three" << false;
    QTest::newRow("data2") << QDir::currentPath() + "/testdir/two" << false;
}

void tst_QDir::rmdir()
{
    QFETCH(QString, path);
    QFETCH(bool, recurse);

    QDir dir;
    if (recurse)
        QVERIFY(dir.rmpath(path));
    else
        QVERIFY(dir.rmdir(path));

    //make sure it really doesn't exist (ie that rmdir returns the right value)
    QFileInfo fi(path);
    QVERIFY(!fi.exists());
}

void tst_QDir::exists_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("expected");

    QTest::newRow("data0") << QDir::currentPath() << true;
    QTest::newRow("data0.1") << QDir::currentPath() + "/" << true;
    QTest::newRow("data1") << QString("/I/Do_not_expect_this_path_to_exist/") << false;
    QTest::newRow("resource0") << QString(":/tst_qdir/") << true;
    QTest::newRow("resource1") << QString(":/I/Do_not_expect_this_resource_to_exist/") << false;

    QTest::newRow("simple dir") << SRCDIR "resources" << true;
    QTest::newRow("simple dir with slash") << SRCDIR "resources/" << true;
#if (defined(Q_OS_WIN) && !defined(Q_OS_WINCE))
    QTest::newRow("unc 1") << "//" + QtNetworkSettings::winServerName() << true;
    QTest::newRow("unc 2") << "//"  + QtNetworkSettings::winServerName() + "/" << true;
    QTest::newRow("unc 3") << "//"  + QtNetworkSettings::winServerName() + "/testshare" << true;
    QTest::newRow("unc 4") << "//"  + QtNetworkSettings::winServerName() + "/testshare/" << true;
    QTest::newRow("unc 5") << "//"  + QtNetworkSettings::winServerName() + "/testshare/tmp" << true;
    QTest::newRow("unc 6") << "//"  + QtNetworkSettings::winServerName() + "/testshare/tmp/" << true;
    QTest::newRow("unc 7") << "//"  + QtNetworkSettings::winServerName() + "/testshare/adirthatshouldnotexist" << false;
    QTest::newRow("unc 8") << "//"  + QtNetworkSettings::winServerName() + "/asharethatshouldnotexist" << false;
    QTest::newRow("unc 9") << "//ahostthatshouldnotexist" << false;
#endif
#if (defined(Q_OS_WIN) && !defined(Q_OS_WINCE)) || defined(Q_OS_SYMBIAN)
    QTest::newRow("This drive should exist") <<  "C:/" << true;
    // find a non-existing drive and check if it does not exist
    QFileInfoList drives = QFSFileEngine::drives();
    QStringList driveLetters;
    for (int i = 0; i < drives.count(); ++i) {
        driveLetters+=drives.at(i).absoluteFilePath();
    }
    char drive = 'Z';
    QString driv;
    do {
        driv = QString::fromAscii("%1:/").arg(drive);
        if (!driveLetters.contains(driv)) break;
        --drive;
    } while (drive >= 'A');
    if (drive >= 'A') {
        QTest::newRow("This drive should not exist") <<  driv << false;
    }
#endif
}

void tst_QDir::exists()
{
    QFETCH(QString, path);
    QFETCH(bool, expected);

    QDir dir(path);
    QCOMPARE(dir.exists(), expected);
}

void tst_QDir::isRelativePath_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("relative");

    QTest::newRow("data0") << "../somedir" << true;
#if (defined(Q_WS_WIN) && !defined(Q_OS_WINCE)) || defined(Q_OS_SYMBIAN)
    QTest::newRow("data1") << "C:/sOmedir" << false;
#endif
    QTest::newRow("data2") << "somedir" << true;
    QTest::newRow("data3") << "/somedir" << false;

    QTest::newRow("resource0") << ":/prefix" << false;
    QTest::newRow("resource1") << ":/prefix/foo.bar" << false;
}

void tst_QDir::isRelativePath()
{
    QFETCH(QString, path);
    QFETCH(bool, relative);

    QCOMPARE(QDir::isRelativePath(path),relative);
}


void tst_QDir::QDir_default()
{
    //default constructor QDir();
    QDir dir; // according to documentation should be currentDirPath
    QCOMPARE(dir.absolutePath(), QDir::currentPath());
}

void tst_QDir::compare()
{
    // operator==
    QDir dir;
    dir.makeAbsolute();
    QVERIFY(dir == QDir::currentPath());
}

static QStringList filterLinks(const QStringList &list)
{
#ifndef Q_NO_SYMLINKS
    return list;
#else
    QStringList result;
    foreach (QString str, list) {
        if (!str.endsWith(QLatin1String(".lnk")))
            result.append(str);
    }
    return result;
#endif
}

void tst_QDir::entryList_data()
{
    QTest::addColumn<QString>("dirName"); // relative from current path or abs
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<int>("filterspec");
    QTest::addColumn<int>("sortspec");
    QTest::addColumn<QStringList>("expected");
    QTest::newRow("spaces1") << SRCDIR "testdir/spaces" << QStringList("*. bar")
              << (int)(QDir::NoFilter) << (int)(QDir::NoSort)
              << QStringList("foo. bar"); // notice how spaces5 works
    QTest::newRow("spaces2") << SRCDIR "testdir/spaces" << QStringList("*.bar")
              << (int)(QDir::NoFilter) << (int)(QDir::NoSort)
              << QStringList("foo.bar");
    QTest::newRow("spaces3") << SRCDIR "testdir/spaces" << QStringList("foo.*")
              << (int)(QDir::NoFilter) << (int)(QDir::NoSort)
              << QString("foo. bar,foo.bar").split(',');
    QTest::newRow("files1")  << SRCDIR "testdir/dir" << QString("*r.cpp *.pro").split(" ")
              << (int)(QDir::NoFilter) << (int)(QDir::NoSort)
              << QString("qdir.pro,qrc_qdir.cpp,tst_qdir.cpp").split(',');
    QTest::newRow("testdir1")  << SRCDIR "testdir" << QStringList()
              << (int)(QDir::AllDirs) << (int)(QDir::NoSort)
              << QString(".,..,dir,spaces").split(',');
// #### this test uses filenames that cannot be represented on all filesystems we test, in
// particular HFS+ on the Mac. When checking out the files with perforce it silently ignores the
// error that it cannot represent the file names stored in the repository and the test fails. That
// is why the test is marked as 'skip' for the mac. When checking out the files with git on the mac
// the error of not being able to represent the files stored in the repository is not silently
// ignored but git reports an error. The test only tried to prevent QDir from _hanging_ when listing
// the directory.
//    QTest::newRow("unprintablenames")  << SRCDIR "unprintablenames" << QStringList("*")
//              << (int)(QDir::NoFilter) << (int)(QDir::NoSort)
//              << QString(".,..").split(",");
    QTest::newRow("resources1") << QString(":/tst_qdir/resources/entryList") << QStringList("*.data")
                             << (int)(QDir::NoFilter) << (int)(QDir::NoSort)
                             << QString("file1.data,file2.data,file3.data").split(',');
    QTest::newRow("resources2") << QString(":/tst_qdir/resources/entryList") << QStringList("*.data")
                             << (int)(QDir::Files) << (int)(QDir::NoSort)
                             << QString("file1.data,file2.data,file3.data").split(',');

    QTest::newRow("nofilter") << SRCDIR "entrylist/" << QStringList("*")
                              << int(QDir::NoFilter) << int(QDir::Name)
                              << filterLinks(QString(".,..,directory,file,linktodirectory.lnk,linktofile.lnk,writable").split(','));
    QTest::newRow("QDir::AllEntries") << SRCDIR "entrylist/" << QStringList("*")
                              << int(QDir::AllEntries) << int(QDir::Name)
                              << filterLinks(QString(".,..,directory,file,linktodirectory.lnk,linktofile.lnk,writable").split(','));
    QTest::newRow("QDir::Files") << SRCDIR "entrylist/" << QStringList("*")
                                 << int(QDir::Files) << int(QDir::Name)
                                 << filterLinks(QString("file,linktofile.lnk,writable").split(','));
    QTest::newRow("QDir::Dirs") << SRCDIR "entrylist/" << QStringList("*")
                                << int(QDir::Dirs) << int(QDir::Name)
                                << filterLinks(QString(".,..,directory,linktodirectory.lnk").split(','));
    QTest::newRow("QDir::Dirs | QDir::NoDotAndDotDot") << SRCDIR "entrylist/" << QStringList("*")
                                                       << int(QDir::Dirs | QDir::NoDotAndDotDot) << int(QDir::Name)
                                << filterLinks(QString("directory,linktodirectory.lnk").split(','));
    QTest::newRow("QDir::AllDirs") << SRCDIR "entrylist/" << QStringList("*")
                                   << int(QDir::AllDirs) << int(QDir::Name)
                                   << filterLinks(QString(".,..,directory,linktodirectory.lnk").split(','));
    QTest::newRow("QDir::AllDirs | QDir::Dirs") << SRCDIR "entrylist/" << QStringList("*")
                                                << int(QDir::AllDirs | QDir::Dirs) << int(QDir::Name)
                                                << filterLinks(QString(".,..,directory,linktodirectory.lnk").split(','));
    QTest::newRow("QDir::AllDirs | QDir::Files") << SRCDIR "entrylist/" << QStringList("*")
                                                 << int(QDir::AllDirs | QDir::Files) << int(QDir::Name)
                                                 << filterLinks(QString(".,..,directory,file,linktodirectory.lnk,linktofile.lnk,writable").split(','));
    QTest::newRow("QDir::AllEntries | QDir::NoSymLinks") << SRCDIR "entrylist/" << QStringList("*")
                                      << int(QDir::AllEntries | QDir::NoSymLinks) << int(QDir::Name)
                                      << filterLinks(QString(".,..,directory,file,writable").split(','));
    QTest::newRow("QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot") << SRCDIR "entrylist/" << QStringList("*")
                                      << int(QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot) << int(QDir::Name)
                                      << filterLinks(QString("directory,file,writable").split(','));
    QTest::newRow("QDir::Files | QDir::NoSymLinks") << SRCDIR "entrylist/" << QStringList("*")
                                                    << int(QDir::Files | QDir::NoSymLinks) << int(QDir::Name)
                                                    << filterLinks(QString("file,writable").split(','));
    QTest::newRow("QDir::Dirs | QDir::NoSymLinks") << SRCDIR "entrylist/" << QStringList("*")
                                                   << int(QDir::Dirs | QDir::NoSymLinks) << int(QDir::Name)
                                                   << filterLinks(QString(".,..,directory").split(','));
    QTest::newRow("QDir::Drives | QDir::Files | QDir::NoDotAndDotDot") << SRCDIR "entrylist/" << QStringList("*")
                                                   << int(QDir::Drives | QDir::Files | QDir::NoDotAndDotDot) << int(QDir::Name)
                                                   << filterLinks(QString("file,linktofile.lnk,writable").split(','));
    QTest::newRow("QDir::System") << SRCDIR "entrylist/" << QStringList("*")
                                  << int(QDir::System) << int(QDir::Name)
                                  << filterLinks(QStringList("brokenlink.lnk"));
    QTest::newRow("QDir::Hidden") << SRCDIR "entrylist/" << QStringList("*")
                                  << int(QDir::Hidden) << int(QDir::Name)
                                  << QStringList();
    QTest::newRow("QDir::System | QDir::Hidden") << SRCDIR "entrylist/" << QStringList("*")
                                  << int(QDir::System | QDir::Hidden) << int(QDir::Name)
                                  << filterLinks(QStringList("brokenlink.lnk"));
    QTest::newRow("QDir::AllDirs | QDir::NoSymLinks") << SRCDIR "entrylist/" << QStringList("*")
                                                      << int(QDir::AllDirs | QDir::NoSymLinks) << int(QDir::Name)
                                                      << filterLinks(QString(".,..,directory").split(','));
    QTest::newRow("QDir::AllEntries | QDir::Hidden | QDir::System") << SRCDIR "entrylist/" << QStringList("*")
                                  << int(QDir::AllEntries | QDir::Hidden | QDir::System) << int(QDir::Name)
                                  << filterLinks(QString(".,..,brokenlink.lnk,directory,file,linktodirectory.lnk,linktofile.lnk,writable").split(','));
    QTest::newRow("QDir::AllEntries | QDir::Readable") << SRCDIR "entrylist/" << QStringList("*")
                                  << int(QDir::AllEntries | QDir::Readable) << int(QDir::Name)
                                                << filterLinks(QString(".,..,directory,file,linktodirectory.lnk,linktofile.lnk,writable").split(','));
    QTest::newRow("QDir::AllEntries | QDir::Writable") << SRCDIR "entrylist/" << QStringList("*")
                                  << int(QDir::AllEntries | QDir::Writable) << int(QDir::Name)
                                  << filterLinks(QString(".,..,directory,linktodirectory.lnk,writable").split(','));
    QTest::newRow("QDir::Files | QDir::Readable") << SRCDIR "entrylist/" << QStringList("*")
                                  << int(QDir::Files | QDir::Readable) << int(QDir::Name)
                                  << filterLinks(QString("file,linktofile.lnk,writable").split(','));
    QTest::newRow("QDir::Dirs | QDir::Readable") << SRCDIR "entrylist/" << QStringList("*")
                                  << int(QDir::Dirs | QDir::Readable) << int(QDir::Name)
                                  << filterLinks(QString(".,..,directory,linktodirectory.lnk").split(','));
    QTest::newRow("Namefilters b*") << SRCDIR "entrylist/" << QStringList("d*")
                                  << int(QDir::NoFilter) << int(QDir::Name)
                                  << filterLinks(QString("directory").split(','));
    QTest::newRow("Namefilters f*") << SRCDIR "entrylist/" << QStringList("f*")
                                  << int(QDir::NoFilter) << int(QDir::Name)
                                  << filterLinks(QString("file").split(','));
    QTest::newRow("Namefilters link*") << SRCDIR "entrylist/" << QStringList("link*")
                                  << int(QDir::NoFilter) << int(QDir::Name)
                                  << filterLinks(QString("linktodirectory.lnk,linktofile.lnk").split(','));
    QTest::newRow("Namefilters *to*") << SRCDIR "entrylist/" << QStringList("*to*")
                                  << int(QDir::NoFilter) << int(QDir::Name)
                                  << filterLinks(QString("directory,linktodirectory.lnk,linktofile.lnk").split(','));
    QTest::newRow("Sorting QDir::Name") << SRCDIR "entrylist/" << QStringList("*")
                                  << int(QDir::NoFilter) << int(QDir::Name)
                                  << filterLinks(QString(".,..,directory,file,linktodirectory.lnk,linktofile.lnk,writable").split(','));
    QTest::newRow("Sorting QDir::Name | QDir::Reversed") << SRCDIR "entrylist/" << QStringList("*")
                                  << int(QDir::NoFilter) << int(QDir::Name | QDir::Reversed)
                                  << filterLinks(QString("writable,linktofile.lnk,linktodirectory.lnk,file,directory,..,.").split(','));

    QTest::newRow("Sorting QDir::Type") << SRCDIR "types/" << QStringList("*")
                                  << int(QDir::NoFilter) << int(QDir::Type)
                                  << QString(".,..,a,b,c,d,e,f,a.a,b.a,c.a,d.a,e.a,f.a,a.b,b.b,c.b,d.b,e.b,f.b,a.c,b.c,c.c,d.c,e.c,f.c").split(',');
    QTest::newRow("Sorting QDir::Type | QDir::Reversed") << SRCDIR "types/" << QStringList("*")
                                  << int(QDir::NoFilter) << int(QDir::Type | QDir::Reversed)
                                  << QString("f.c,e.c,d.c,c.c,b.c,a.c,f.b,e.b,d.b,c.b,b.b,a.b,f.a,e.a,d.a,c.a,b.a,a.a,f,e,d,c,b,a,..,.").split(',');
    QTest::newRow("Sorting QDir::Type | QDir::DirsLast") << SRCDIR "types/" << QStringList("*")
                                  << int(QDir::NoFilter) << int(QDir::Type | QDir::DirsLast)
                                  << QString("a,b,c,a.a,b.a,c.a,a.b,b.b,c.b,a.c,b.c,c.c,.,..,d,e,f,d.a,e.a,f.a,d.b,e.b,f.b,d.c,e.c,f.c").split(',');
    QTest::newRow("Sorting QDir::Type | QDir::DirsFirst") << SRCDIR "types/" << QStringList("*")
                                  << int(QDir::NoFilter) << int(QDir::Type | QDir::DirsFirst)
                                  << QString(".,..,d,e,f,d.a,e.a,f.a,d.b,e.b,f.b,d.c,e.c,f.c,a,b,c,a.a,b.a,c.a,a.b,b.b,c.b,a.c,b.c,c.c").split(',');
    QTest::newRow("Sorting QDir::Size") << SRCDIR "types/" << QStringList("*")
                                  << int(QDir::AllEntries|QDir::NoDotAndDotDot) << int(QDir::Size | QDir::DirsFirst)
                                  << QString("d,d.a,d.b,d.c,e,e.a,e.b,e.c,f,f.a,f.b,f.c,c.a,c.b,c.c,b.a,b.c,b.b,a.c,a.b,a.a,a,b,c").split(',');
    QTest::newRow("Sorting QDir::Size | QDir::Reversed") << SRCDIR "types/" << QStringList("*")
                                  << int(QDir::AllEntries|QDir::NoDotAndDotDot) << int(QDir::Size | QDir::Reversed | QDir::DirsLast)
                                  << QString("c,b,a,a.a,a.b,a.c,b.b,b.c,b.a,c.c,c.b,c.a,f.c,f.b,f.a,f,e.c,e.b,e.a,e,d.c,d.b,d.a,d").split(',');
}

void tst_QDir::entryList()
{
    QFETCH(QString, dirName);
    QFETCH(QStringList, nameFilters);
    QFETCH(int, filterspec);
    QFETCH(int, sortspec);
    QFETCH(QStringList, expected);

    QFile(SRCDIR "entrylist/writable").open(QIODevice::ReadWrite);
    QFile(SRCDIR "entrylist/file").setPermissions(QFile::ReadOwner | QFile::ReadUser);
    QFile::remove(SRCDIR "entrylist/linktofile");
    QFile::remove(SRCDIR "entrylist/linktodirectory");
    QFile::remove(SRCDIR "entrylist/linktofile.lnk");
    QFile::remove(SRCDIR "entrylist/linktodirectory.lnk");
    QFile::remove(SRCDIR "entrylist/brokenlink.lnk");
    QFile::remove(SRCDIR "entrylist/brokenlink");

    // WinCE/Symbian does not have . and .. in the directory listing
#if defined(Q_OS_WINCE) || defined(Q_OS_SYMBIAN)
    expected.removeAll(".");
    expected.removeAll("..");
#endif

#ifndef Q_NO_SYMLINKS
#if defined(Q_OS_WIN)
    // ### Sadly, this is a platform difference right now.
    QFile::link(SRCDIR "entryList/file", SRCDIR "entrylist/linktofile.lnk");
    QFile::link(SRCDIR "entryList/directory", SRCDIR "entrylist/linktodirectory.lnk");
    QFile::link(SRCDIR "entryList/nothing", SRCDIR "entrylist/brokenlink.lnk");
#elif defined(Q_OS_SYMBIAN)
    // Symbian doesn't support links to directories
      expected.removeAll("linktodirectory.lnk");

    // Expecting failures from a couple of OpenC bugs. Do checks only once.
    static int xFailChecked = false;
    static int expectedFail1 = false;
    static int expectedFail2 = false;

    if (!expectedFail1) {
        // Can't create link if file doesn't exist in symbian, so create file temporarily,
        // But only if testing for
        QFile tempFile(SRCDIR "entryList/nothing");
        tempFile.open(QIODevice::WriteOnly);
        tempFile.link(SRCDIR "entryList/brokenlink.lnk");
        tempFile.remove();
    }

    if (!expectedFail2) {
        QFile::link(SRCDIR "entryList/file", SRCDIR "entrylist/linktofile.lnk");
    }

    if (!xFailChecked) {
        // ### Until OpenC supports stat correctly for symbolic links, expect them to fail.
        expectedFail1 = QFileInfo(SRCDIR "entryList/brokenlink.lnk").exists();
        expectedFail2 = !(QFileInfo(SRCDIR "entryList/linktofile.lnk").isFile());

        QEXPECT_FAIL("", "OpenC bug, stat for broken links returns normally, when it should return error.", Continue);
        QVERIFY(!expectedFail1);

        QEXPECT_FAIL("", "OpenC bug, stat for file links doesn't indicate them as such.", Continue);
        QVERIFY(!expectedFail2);
        xFailChecked = true;
    }

    if (expectedFail1) {
        expected.removeAll("brokenlink.lnk");
        QFile::remove(SRCDIR "entrylist/brokenlink.lnk");
    }

    if (expectedFail2) {
        expected.removeAll("linktofile.lnk");
        QFile::remove(SRCDIR "entrylist/linktofile.lnk");
    }
#else
    QFile::link("file", SRCDIR "entrylist/linktofile.lnk");
    QFile::link("directory", SRCDIR "entrylist/linktodirectory.lnk");
    QFile::link("nothing", SRCDIR "entrylist/brokenlink.lnk");
#endif
#endif //Q_NO_SYMLINKS

#ifdef Q_WS_MAC
    if (qstrcmp(QTest::currentDataTag(), "unprintablenames") == 0)
        QSKIP("p4 doesn't sync the files with the unprintable names properly on Mac",SkipSingle);
#endif
    QDir dir(dirName);
    QVERIFY(dir.exists());

    QStringList actual = dir.entryList(nameFilters, (QDir::Filters)filterspec,
                                       (QDir::SortFlags)sortspec);

    int max = qMin(actual.count(), expected.count());

    if (qstrcmp(QTest::currentDataTag(), "unprintablenames") == 0) {
        // The purpose of this entry is to check that QDir doesn't
        // lock up. The actual result depends on the file system.
        return;
    }
    bool doContentCheck = true;
#if defined(Q_OS_UNIX) && !defined(Q_OS_SYMBIAN)
    if (qstrcmp(QTest::currentDataTag(), "QDir::AllEntries | QDir::Writable") == 0) {
        // for root, everything is writeable
        if (::getuid() == 0)
            doContentCheck = false;
    }
#endif

    if (doContentCheck) {
        for (int i=0; i<max; ++i)
            QCOMPARE(actual[i], expected[i]);

        QCOMPARE(actual.count(), expected.count());
    }

#if defined(Q_OS_SYMBIAN)
    // Test cleanup on device requires setting the permissions back to normal
    QFile(SRCDIR "entrylist/file").setPermissions(QFile::WriteUser | QFile::ReadUser);
#endif

    QFile::remove(SRCDIR "entrylist/writable");
    QFile::remove(SRCDIR "entrylist/linktofile");
    QFile::remove(SRCDIR "entrylist/linktodirectory");
    QFile::remove(SRCDIR "entrylist/linktofile.lnk");
    QFile::remove(SRCDIR "entrylist/linktodirectory.lnk");
    QFile::remove(SRCDIR "entrylist/brokenlink.lnk");
    QFile::remove(SRCDIR "entrylist/brokenlink");
}

void tst_QDir::entryListSimple_data()
{
    QTest::addColumn<QString>("dirName");
    QTest::addColumn<int>("countMin");

    QTest::newRow("data2") << "do_not_expect_this_path_to_exist/" << 0;
#if defined(Q_OS_WINCE) || defined(Q_OS_SYMBIAN)
    QTest::newRow("simple dir") << SRCDIR "resources" << 0;
    QTest::newRow("simple dir with slash") << SRCDIR "resources/" << 0;
#else
    QTest::newRow("simple dir") << SRCDIR "resources" << 2;
    QTest::newRow("simple dir with slash") << SRCDIR "resources/" << 2;
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    QTest::newRow("unc 1") << "//"  + QtNetworkSettings::winServerName() << 2;
    QTest::newRow("unc 2") << "//"  + QtNetworkSettings::winServerName() + "/" << 2;
    QTest::newRow("unc 3") << "//"  + QtNetworkSettings::winServerName() + "/testshare" << 2;
    QTest::newRow("unc 4") << "//"  + QtNetworkSettings::winServerName() + "/testshare/" << 2;
    QTest::newRow("unc 5") << "//"  + QtNetworkSettings::winServerName() + "/testshare/tmp" << 2;
    QTest::newRow("unc 6") << "//"  + QtNetworkSettings::winServerName() + "/testshare/tmp/" << 2;
    QTest::newRow("unc 7") << "//"  + QtNetworkSettings::winServerName() + "/testshare/adirthatshouldnotexist" << 0;
    QTest::newRow("unc 8") << "//"  + QtNetworkSettings::winServerName() + "/asharethatshouldnotexist" << 0;
    QTest::newRow("unc 9") << "//ahostthatshouldnotexist" << 0;
#endif
}

void tst_QDir::entryListSimple()
{
    QFETCH(QString, dirName);
    QFETCH(int, countMin);

    QDir dir(dirName);
    QStringList actual = dir.entryList();
    QVERIFY(actual.count() >= countMin);
}

void tst_QDir::entryListWithSymLinks()
{
#ifndef Q_NO_SYMLINKS
#  ifndef Q_NO_SYMLINKS_TO_DIRS
    QFile::remove("myLinkToDir.lnk");
#  endif
    QFile::remove("myLinkToFile.lnk");
    QFile::remove("testfile.cpp");
    QDir dir;
    dir.mkdir("myDir");
    QFile("testfile.cpp").open(QIODevice::WriteOnly);
#  ifndef Q_NO_SYMLINKS_TO_DIRS
    QVERIFY(QFile::link("myDir", "myLinkToDir.lnk"));
#  endif
    QVERIFY(QFile::link("testfile.cpp", "myLinkToFile.lnk"));

    {
        QStringList entryList = QDir().entryList();
        QVERIFY(entryList.contains("myDir"));
#  ifndef Q_NO_SYMLINKS_TO_DIRS
        QVERIFY(entryList.contains("myLinkToDir.lnk"));
#endif
        QVERIFY(entryList.contains("myLinkToFile.lnk"));
    }
    {
        QStringList entryList = QDir().entryList(QDir::Dirs);
        QVERIFY(entryList.contains("myDir"));
#  ifndef Q_NO_SYMLINKS_TO_DIRS
        QVERIFY(entryList.contains("myLinkToDir.lnk"));
#endif
#if defined(Q_OS_SYMBIAN)
    QEXPECT_FAIL("", "OpenC stat for symlinks is buggy.", Continue);
#endif
        QVERIFY(!entryList.contains("myLinkToFile.lnk"));
    }
    {
        QStringList entryList = QDir().entryList(QDir::Dirs | QDir::NoSymLinks);
        QVERIFY(entryList.contains("myDir"));
        QVERIFY(!entryList.contains("myLinkToDir.lnk"));
        QVERIFY(!entryList.contains("myLinkToFile.lnk"));
    }

    QFile::remove("myLinkToDir.lnk");
    QFile::remove("myLinkToFile.lnk");
    QFile::remove("testfile.cpp");
    dir.rmdir("myDir");
#endif
}

void tst_QDir::canonicalPath_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("canonicalPath");
    QString appPath = SRCDIR;
    if (appPath.isEmpty())
        appPath = QCoreApplication::instance()->applicationDirPath();
    else
        appPath.chop(1);        // remove the ending slash

#if defined Q_WS_WIN
    if (appPath.endsWith("release", Qt::CaseInsensitive) || appPath.endsWith("debug", Qt::CaseInsensitive)) {
        QDir appDir(appPath);
        QVERIFY(appDir.cdUp());
        appPath = appDir.absolutePath();
    }
#endif

    QTest::newRow("relative") << "." << appPath;
    QTest::newRow("relativeSubDir") << "./testData/../testData" << appPath + "/testData";

#ifndef Q_WS_WIN
    QTest::newRow("absPath") << appPath + "/testData/../testData" << appPath + "/testData";
#else
    QTest::newRow("absPath") << appPath + "\\testData\\..\\testData" << appPath + "/testData";
#endif
    QTest::newRow("nonexistant") << "testd" << QString();

    QTest::newRow("rootPath") << QDir::rootPath() << QDir::rootPath();

#ifdef Q_OS_MAC
    // On Mac OS X 10.5 and earlier, canonicalPath depends on cleanPath which
    // is itself very broken and fundamentally wrong on "/./" which, this would
    // exercise
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_6)
#endif
        QTest::newRow("rootPath + ./") << QDir::rootPath().append("./") << QDir::rootPath();

    QTest::newRow("rootPath + ../.. ") << QDir::rootPath().append("../..") << QDir::rootPath();
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    QTest::newRow("drive:\\") << QDir::toNativeSeparators(QDir::rootPath()) << QDir::rootPath();
    QTest::newRow("drive:\\.\\") << QDir::toNativeSeparators(QDir::rootPath().append("./")) << QDir::rootPath();
    QTest::newRow("drive:\\..\\..") << QDir::toNativeSeparators(QDir::rootPath().append("../..")) << QDir::rootPath();
    QTest::newRow("drive:") << QDir().canonicalPath().left(2) << QDir().canonicalPath();
#endif

    QTest::newRow("resource") << ":/tst_qdir/resources/entryList" << ":/tst_qdir/resources/entryList";
}

void tst_QDir::canonicalPath()
{
    QDir srcPath;
    if (strlen(SRCDIR) > 0)
        srcPath = QDir(SRCDIR);
    else
        srcPath = QDir(".");
    if (srcPath.absolutePath() != srcPath.canonicalPath())
        QSKIP("This test does not work if this directory path consists of symlinks.", SkipAll);

    QString oldpwd = QDir::currentPath();
    QDir::setCurrent(srcPath.absolutePath());

    QFETCH(QString, path);
    QFETCH(QString, canonicalPath);

    QDir dir(path);
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    QCOMPARE(dir.canonicalPath().toLower(), canonicalPath.toLower());
#else
    QCOMPARE(dir.canonicalPath(), canonicalPath);
#endif

    QDir::setCurrent(oldpwd);
}

void tst_QDir::current_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("currentDir");
    QString appPath = SRCDIR;
    if (appPath.isEmpty())
        appPath = QCoreApplication::instance()->applicationDirPath();
    else
        appPath.chop(1);        // remove the ending slash
#if defined Q_WS_WIN
    if (appPath.endsWith("release", Qt::CaseInsensitive))
        appPath = appPath.left(appPath.length()-8);
    else if (appPath.endsWith("debug", Qt::CaseInsensitive))
        appPath = appPath.left(appPath.length()-6);
#endif

    QTest::newRow("startup") << QString() << appPath;
    QTest::newRow("relPath") << "testData" << appPath + "/testData";
#ifndef Q_WS_WIN
    QTest::newRow("absPath") << appPath + "/testData" << appPath + "/testData";
#else
    QTest::newRow("absPath") << appPath + "\\testData" << appPath + "/testData";
#endif
    QTest::newRow("nonexistant") << "testd" << QString();

    QTest::newRow("parent") << ".." << appPath.left(appPath.lastIndexOf('/'));
}

void tst_QDir::current()
{
    QString oldDir = QDir::currentPath();
    QString appPath = SRCDIR;
    if (appPath.isEmpty())
        appPath = QCoreApplication::instance()->applicationDirPath();
    QDir::setCurrent(appPath);
    QFETCH(QString, path);
    QFETCH(QString, currentDir);

    if (!path.isEmpty()) {
        bool b = QDir::setCurrent(path);
        // If path is non existent, then setCurrent should be false (currentDir is empty in testData)
        QVERIFY(b == !currentDir.isEmpty());
    }
    if (!currentDir.isEmpty()) {
        QDir newCurrent = QDir::current();
        QDir::setCurrent(oldDir);
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    QCOMPARE(newCurrent.absolutePath().toLower(), currentDir.toLower());
#else
    QCOMPARE(newCurrent.absolutePath(), currentDir);
#endif
    }

    QDir::setCurrent(oldDir);
}

void tst_QDir::cd_data()
{
    QTest::addColumn<QString>("startDir");
    QTest::addColumn<QString>("cdDir");
    QTest::addColumn<bool>("successExpected");
    QTest::addColumn<QString>("newDir");

    QString appPath = QDir::currentPath();
    int index = appPath.lastIndexOf("/");
    QTest::newRow("cdUp") << QDir::currentPath() << ".." << true << appPath.left(index==0?1:index);
    QTest::newRow("noChange") << QDir::currentPath() << "." << true << appPath;
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN) // on windows QDir::root() is usually c:/ but cd "/" will not force it to be root
    QTest::newRow("absolute") << QDir::currentPath() << "/" << true << "/";
#else
    QTest::newRow("absolute") << QDir::currentPath() << "/" << true << QDir::root().absolutePath();
#endif
    QTest::newRow("non existant") << "." << "../anonexistingdir" << false << QDir::currentPath();
    QTest::newRow("self") << "." << (QString("../") + QFileInfo(QDir::currentPath()).fileName()) << true << QDir::currentPath();
    QTest::newRow("file") << "." << "qdir.pro" << false << "";
}

void tst_QDir::cd()
{
    QFETCH(QString, startDir);
    QFETCH(QString, cdDir);
    QFETCH(bool, successExpected);
    QFETCH(QString, newDir);

    QDir d = startDir;
    bool notUsed = d.exists(); // make sure we cache this before so we can see if 'cd' fails to flush this
    Q_UNUSED(notUsed);
    QCOMPARE(d.cd(cdDir), successExpected);
    if (successExpected)
        QCOMPARE(d.absolutePath(), newDir);
}

void tst_QDir::setNameFilters_data()
{
    // Effectively copied from entryList2() test

    QTest::addColumn<QString>("dirName"); // relative from current path or abs
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<QStringList>("expected");

    QString appPath = SRCDIR;
    if (appPath.isEmpty())
        appPath = QCoreApplication::instance()->applicationDirPath();
    if (!appPath.endsWith("/"))
        appPath.append("/");

    QTest::newRow("spaces1") << appPath + "testdir/spaces" << QStringList("*. bar")
                          << QStringList("foo. bar");
    QTest::newRow("spaces2") << appPath + "testdir/spaces" << QStringList("*.bar")
                          << QStringList("foo.bar");
    QTest::newRow("spaces3") << appPath + "testdir/spaces" << QStringList("foo.*")
                            << QString("foo. bar,foo.bar").split(",");
    QTest::newRow("files1")  << appPath + "testdir/dir" << QString("*r.cpp *.pro").split(" ")
                          << QString("qdir.pro,qrc_qdir.cpp,tst_qdir.cpp").split(",");
    QTest::newRow("resources1") << QString(":/tst_qdir/resources/entryList") << QStringList("*.data")
                             << QString("file1.data,file2.data,file3.data").split(',');
}

void tst_QDir::setNameFilters()
{
    QFETCH(QString, dirName);
    QFETCH(QStringList, nameFilters);
    QFETCH(QStringList, expected);

    QDir dir(dirName);
    QVERIFY(dir.exists());

    dir.setNameFilters(nameFilters);
    QStringList actual = dir.entryList();
    int max = qMin(actual.count(), expected.count());

    for (int i=0; i<max; ++i)
        QCOMPARE(actual[i], expected[i]);
    QCOMPARE(actual.count(), expected.count());
}

void
tst_QDir::cleanPath_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("expected");

    QTest::newRow("data0") << "/Users/sam/troll/qt4.0//.." << "/Users/sam/troll";
    QTest::newRow("data1") << "/Users/sam////troll/qt4.0//.." << "/Users/sam/troll";
    QTest::newRow("data2") << "/" << "/";
    QTest::newRow("data3") << QDir::cleanPath("../.") << "..";
    QTest::newRow("data4") << QDir::cleanPath("../..") << "../..";
#if !defined(Q_OS_WINCE)
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    QTest::newRow("data5") << "d:\\a\\bc\\def\\.." << "d:/a/bc";
    QTest::newRow("data6") << "d:\\a\\bc\\def\\../../.." << "d:/";
#else
    QTest::newRow("data5") << "d:\\a\\bc\\def\\.." << "d:\\a\\bc\\def\\..";
    QTest::newRow("data6") << "d:\\a\\bc\\def\\../../.." << "d:\\a\\bc\\def\\../../..";
#endif
#endif
    QTest::newRow("data7") << ".//file1.txt" << "file1.txt";
    QTest::newRow("data8") << "/foo/bar/..//file1.txt" << "/foo/file1.txt";
    QTest::newRow("data9") << "//" << "/";
#if !defined(Q_OS_WINCE)
#if defined Q_OS_WIN
    QTest::newRow("data10") << "c:\\" << "c:/";
#else
    QTest::newRow("data10") << "/:/" << "/:";
#endif
#endif

    QTest::newRow("resource0") << ":/prefix/foo.bar" << ":/prefix/foo.bar";
    QTest::newRow("resource1") << "://prefix/..//prefix/foo.bar" << ":/prefix/foo.bar";
}


void
tst_QDir::cleanPath()
{
    QFETCH(QString, path);
    QFETCH(QString, expected);
    QString cleaned = QDir::cleanPath(path);
    QCOMPARE(cleaned, expected);
}

void tst_QDir::absoluteFilePath_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("expectedFilePath");

    QTest::newRow("0") << "/etc" << "/passwd" << "/passwd";
    QTest::newRow("1") << "/etc" << "passwd" << "/etc/passwd";
    QTest::newRow("2") << "/" << "passwd" << "/passwd";
    QTest::newRow("3") << "relative" << "path" << QDir::currentPath() + "/relative/path";
    QTest::newRow("4") << "" << "" << QDir::currentPath();
    QTest::newRow("resource") << ":/prefix" << "foo.bar" << ":/prefix/foo.bar";
}

void tst_QDir::absoluteFilePath()
{
    QFETCH(QString, path);
    QFETCH(QString, fileName);
    QFETCH(QString, expectedFilePath);

    QDir dir(path);
    QString absFilePath = dir.absoluteFilePath(fileName);
    QCOMPARE(absFilePath, expectedFilePath);
}

void tst_QDir::absolutePath_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("expectedPath");

    QTest::newRow("0") << "/machine/share/dir1" << "/machine/share/dir1";
#if (defined(Q_OS_WIN) && !defined(Q_OS_WINCE)) || defined(Q_OS_SYMBIAN)
    QTest::newRow("1") << "\\machine\\share\\dir1" << "/machine/share/dir1";
# if !defined(Q_OS_SYMBIAN)
    QTest::newRow("2") << "//machine/share/dir1" << "//machine/share/dir1";
    QTest::newRow("3") << "\\\\machine\\share\\dir1" << "//machine/share/dir1";
# endif
    QTest::newRow("4") << "c:/machine/share/dir1" << "c:/machine/share/dir1";
    QTest::newRow("5") << "c:\\machine\\share\\dir1" << "c:/machine/share/dir1";
#endif
    //test dirty paths are cleaned (QTBUG-19995)
    QTest::newRow("/home/qt/.") << QDir::rootPath() + "home/qt/." << QDir::rootPath() + "home/qt";
    QTest::newRow("/system/data/../config") << QDir::rootPath() + "system/data/../config" << QDir::rootPath() + "system/config";
    QTest::newRow("//home//qt/") << QDir::rootPath() + "/home//qt/" << QDir::rootPath() + "home/qt";
    QTest::newRow("foo/../bar") << "foo/../bar" << QDir::currentPath() + "/bar";
    QTest::newRow("resource") << ":/prefix/foo.bar" << ":/prefix/foo.bar";
}

void tst_QDir::absolutePath()
{
    QFETCH(QString, path);
    QFETCH(QString, expectedPath);

    QDir dir(path);
    QCOMPARE(dir.absolutePath(), expectedPath);
}

void tst_QDir::relativeFilePath_data()
{
    QTest::addColumn<QString>("dir");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("expected");

    QTest::newRow("0") << "/foo/bar" << "ding.txt" << "ding.txt";
    QTest::newRow("1") << "/foo/bar" << "ding/dong.txt"    << "ding/dong.txt";
    QTest::newRow("2") << "/foo/bar" << "../ding/dong.txt" << "../ding/dong.txt";

    QTest::newRow("3") << "/foo/bar" << "/foo/bar/ding.txt" << "ding.txt";
    QTest::newRow("4") << "/foo/bar/" << "/foo/bar/ding/dong.txt" << "ding/dong.txt";
    QTest::newRow("5") << "/foo/bar/" << "/ding/dong.txt" << "../../ding/dong.txt";

    QTest::newRow("6") << "/" << "/ding/dong.txt" << "ding/dong.txt";
    QTest::newRow("7") << "/" << "/ding/" << "ding";
    QTest::newRow("8") << "/" << "/ding//" << "ding";
    QTest::newRow("9") << "/" << "/ding/../dong" << "dong";
    QTest::newRow("10") << "/" << "/ding/../../../../dong" << "../../../dong";

    QTest::newRow("11") << "" << "" << "";

#if (defined(Q_OS_WIN) && !defined(Q_OS_WINCE)) || defined(Q_OS_SYMBIAN)
    QTest::newRow("12") << "C:/foo/bar" << "ding" << "ding";
    QTest::newRow("13") << "C:/foo/bar" << "C:/ding/dong" << "../../ding/dong";
    QTest::newRow("14") << "C:/foo/bar" << "/ding/dong" << "../../ding/dong";
    QTest::newRow("15") << "C:/foo/bar" << "D:/ding/dong" << "D:/ding/dong";
    QTest::newRow("16") << "C:" << "C:/ding/dong" << "ding/dong";
    QTest::newRow("17") << "C:/" << "C:/ding/dong" << "ding/dong";
    QTest::newRow("18") << "C:" << "C:" << "";
    QTest::newRow("19") << "C:/" << "C:" << "";
    QTest::newRow("20") << "C:" << "C:/" << "";
    QTest::newRow("21") << "C:/" << "C:/" << "";
    QTest::newRow("22") << "C:" << "C:file.txt" << "file.txt";
    QTest::newRow("23") << "C:/" << "C:file.txt" << "file.txt";
    QTest::newRow("24") << "C:" << "C:/file.txt" << "file.txt";
    QTest::newRow("25") << "C:/" << "C:/file.txt" << "file.txt";
    QTest::newRow("26") << "C:" << "D:" << "D:";
    QTest::newRow("27") << "C:" << "D:/" << "D:/";
    QTest::newRow("28") << "C:/" << "D:" << "D:";
    QTest::newRow("29") << "C:/" << "D:/" << "D:/";
# if !defined(Q_OS_SYMBIAN)
    QTest::newRow("30") << "C:/foo/bar" << "//anotherHost/foo/bar" << "//anotherHost/foo/bar";
    QTest::newRow("31") << "//anotherHost/foo" << "//anotherHost/foo/bar" << "bar";
    QTest::newRow("32") << "//anotherHost/foo" << "bar" << "bar";
    QTest::newRow("33") << "//anotherHost/foo" << "C:/foo/bar" << "C:/foo/bar";
# endif
#endif

    QTest::newRow("resource0") << ":/prefix" << "foo.bar" << "foo.bar";
    QTest::newRow("resource1") << ":/prefix" << ":/prefix/foo.bar" << "foo.bar";
}

void tst_QDir::relativeFilePath()
{
    QFETCH(QString, dir);
    QFETCH(QString, path);
    QFETCH(QString, expected);

    QCOMPARE(QDir(dir).relativeFilePath(path), expected);
}

void tst_QDir::filePath_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("expectedFilePath");

    QTest::newRow("0") << "/etc" << "/passwd" << "/passwd";
    QTest::newRow("1") << "/etc" << "passwd" << "/etc/passwd";
    QTest::newRow("2") << "/" << "passwd" << "/passwd";
    QTest::newRow("3") << "relative" << "path" << "relative/path";
    QTest::newRow("4") << "" << "" << ".";
    QTest::newRow("resource") << ":/prefix" << "foo.bar" << ":/prefix/foo.bar";
}

void tst_QDir::filePath()
{
    QFETCH(QString, path);
    QFETCH(QString, fileName);
    QFETCH(QString, expectedFilePath);

    QDir dir(path);
    QString absFilePath = dir.filePath(fileName);
    QCOMPARE(absFilePath, expectedFilePath);
}

void tst_QDir::remove()
{
    QFile f("remove-test");
    f.open(QIODevice::WriteOnly);
    f.close();
    QDir dir;
    QVERIFY(dir.remove("remove-test"));
    QVERIFY(!dir.remove("/remove-test"));
    QTest::ignoreMessage(QtWarningMsg, "QDir::remove: Empty or null file name");
    QVERIFY(!dir.remove(""));
}

void tst_QDir::rename()
{
    QFile f("rename-test");
    f.open(QIODevice::WriteOnly);
    f.close();
    QDir dir;
    QVERIFY(dir.rename("rename-test", "rename-test-renamed"));
    QVERIFY(dir.rename("rename-test-renamed", "rename-test"));
#if defined(Q_OS_MAC)
    QVERIFY(!dir.rename("rename-test", "/etc/rename-test-renamed"));
#elif defined(Q_OS_SYMBIAN)
    QVERIFY(!dir.rename("rename-test", "/resource/rename-test-renamed"));
#elif !defined(Q_OS_WIN)
    // on windows this is possible - maybe make the test a bit better
    QVERIFY(!dir.rename("rename-test", "/rename-test-renamed"));
#endif
    QTest::ignoreMessage(QtWarningMsg, "QDir::rename: Empty or null file name(s)");
    QVERIFY(!dir.rename("rename-test", ""));
    QTest::ignoreMessage(QtWarningMsg, "QDir::rename: Empty or null file name(s)");
    QVERIFY(!dir.rename("", "rename-test-renamed"));
    QVERIFY(!dir.rename("some-file-that-does-not-exist", "rename-test-renamed"));

    QVERIFY(dir.remove("rename-test"));
}

void tst_QDir::exists2_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("exists");

    QTest::newRow("0") << "." << true;
    QTest::newRow("1") << "/" << true;
    QTest::newRow("2") << "" << false;
    QTest::newRow("3") << "testData" << true;
    QTest::newRow("4") << "/testData" << false;
    QTest::newRow("5") << "tst_qdir.cpp" << true;
    QTest::newRow("6") << "/resources.cpp" << false;
    QTest::newRow("resource0") << ":/prefix/foo.bar" << false;
    QTest::newRow("resource1") << ":/tst_qdir/resources/entryList/file1.data" << true;
}

void tst_QDir::exists2()
{
    QFETCH(QString, path);
    QFETCH(bool, exists);

    QString oldpwd = QDir::currentPath();
    if (strlen(SRCDIR) > 0)
        QDir::setCurrent(SRCDIR);

    if (path.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, "QDir::exists: Empty or null file name");

    QDir dir;
    if (exists)
        QVERIFY(dir.exists(path));
    else
        QVERIFY(!dir.exists(path));

    QDir::setCurrent(oldpwd);
}

void tst_QDir::dirName_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("dirName");

    QTest::newRow("slash0") << "c:/winnt/system32" << "system32";
    QTest::newRow("slash1") << "/winnt/system32" << "system32";
    QTest::newRow("slash2") << "c:/winnt/system32/kernel32.dll" << "kernel32.dll";
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    QTest::newRow("bslash0") << "c:\\winnt\\system32" << "system32";
    QTest::newRow("bslash1") << "\\winnt\\system32" << "system32";
    QTest::newRow("bslash2") << "c:\\winnt\\system32\\kernel32.dll" << "kernel32.dll";
#endif

    QTest::newRow("resource") << ":/prefix" << "prefix";
}

void tst_QDir::dirName()
{
    QFETCH(QString, path);
    QFETCH(QString, dirName);

    QDir dir(path);
    QCOMPARE(dir.dirName(), dirName);
}

void tst_QDir::operator_eq()
{
    QDir dir1(".");
    dir1 = dir1;
    dir1.setPath("..");
}

void tst_QDir::dotAndDotDot()
{
#if defined(Q_OS_WINCE) || defined(Q_OS_SYMBIAN)
    QSKIP("WinCE and Symbian do not have . nor ..", SkipAll);
#else
    QDir dir(QString(SRCDIR "testdir/"));
    QStringList entryList = dir.entryList(QDir::Dirs);
    QCOMPARE(entryList, QStringList() << QString(".") << QString("..") << QString("dir") << QString("spaces"));
    entryList = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QCOMPARE(entryList, QStringList() << QString("dir") << QString("spaces"));
#endif
}

void tst_QDir::homePath()
{
    QDir homeDir = QDir::home();
    QString strHome = QDir::homePath();

    // docs say that homePath() is an absolute path
    QCOMPARE(strHome, homeDir.absolutePath());
    QVERIFY(QDir::isAbsolutePath(strHome));

#ifdef Q_OS_UNIX
    if (strHome.length() > 1)      // root dir = "/"
        QVERIFY(!strHome.endsWith('/'));
#elif defined(Q_OS_WIN)
    if (strHome.length() > 3)      // root dir = "c:/"; "//" is not really valid...
        QVERIFY(!strHome.endsWith('/'));
#endif

    QStringList entries = homeDir.entryList();
    for (int i = 0; i < entries.count(); ++i) {
        QFileInfo fi(QDir::homePath() + "/" + entries[i]);
        QCOMPARE(fi.exists(), true);
    }
}

void tst_QDir::tempPath()
{
    QDir dir = QDir::temp();
    QString path = QDir::tempPath();

    // docs say that tempPath() is an absolute path
    QCOMPARE(path, dir.absolutePath());
    QVERIFY(QDir::isAbsolutePath(path));

#ifdef Q_OS_UNIX
    if (path.length() > 1)      // root dir = "/"
        QVERIFY(!path.endsWith('/'));
#elif defined(Q_OS_WIN)
    if (path.length() > 3)      // root dir = "c:/"; "//" is not really valid...
        QVERIFY(!path.endsWith('/'));
#endif
}

void tst_QDir::rootPath()
{
    QDir dir = QDir::root();
    QString path = QDir::rootPath();

    // docs say that tempPath() is an absolute path
    QCOMPARE(path, dir.absolutePath());
    QVERIFY(QDir::isAbsolutePath(path));

#if defined(Q_OS_UNIX) && !defined(Q_OS_SYMBIAN)
    QCOMPARE(path, QString("/"));
#endif
}

void tst_QDir::nativeSeparators()
{
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    QCOMPARE(QDir::toNativeSeparators(QLatin1String("/")), QString("\\"));
    QCOMPARE(QDir::toNativeSeparators(QLatin1String("\\")), QString("\\"));
    QCOMPARE(QDir::fromNativeSeparators(QLatin1String("/")), QString("/"));
    QCOMPARE(QDir::fromNativeSeparators(QLatin1String("\\")), QString("/"));
#else
    QCOMPARE(QDir::toNativeSeparators(QLatin1String("/")), QString("/"));
    QCOMPARE(QDir::toNativeSeparators(QLatin1String("\\")), QString("\\"));
    QCOMPARE(QDir::fromNativeSeparators(QLatin1String("/")), QString("/"));
    QCOMPARE(QDir::fromNativeSeparators(QLatin1String("\\")), QString("\\"));
#endif
}

void tst_QDir::searchPaths_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("searchPathPrefixes");
    QTest::addColumn<QString>("searchPaths");
    QTest::addColumn<QString>("expectedAbsolutePath");

    QString srcdir = SRCDIR;
    if (srcdir.isEmpty())
        srcdir = QDir::currentPath();
    else
        srcdir.chop(1);         // remove ending slash
    QString searchDir = srcdir + "/searchdir";

    // sanity
    QTest::newRow("nopath") << "picker.png" << QString() << QString() << QString();
    QTest::newRow("emptysearchpath") << "subdir1/picker.png" << QString() << QString() << QString();
    QTest::newRow("searchpathwithoutprefix") << SRCDIR "searchdir/subdir1/picker.png" << QString("searchpath") << QString("searchdir") << (searchDir+"/subdir1/picker.png");

    // new
    QTest::newRow("novalidsearchpath") << "searchpath:subdir1/picker.png" << QString() << QString() << QString();
    QTest::newRow("invalidsearchpath") << "searchpath:subdir1/picker.png" << QString("invalid") << QString("invalid") << QString();
    QTest::newRow("onlyvalidsearchpath") << "searchpath:subdir1/picker.png" << QString("searchpath") << QString(SRCDIR "searchdir") << (searchDir+"/subdir1/picker.png");
    QTest::newRow("validandinvalidsearchpath") << "searchpath:subdir1/picker.png" << QString("invalid;searchpath") << QString("invalid;" SRCDIR "searchdir") << (searchDir+"/subdir1/picker.png");
    QTest::newRow("precedence1") << "searchpath:picker.png" << QString("invalid;searchpath") << QString("invalid;" SRCDIR "searchdir/subdir1," SRCDIR "searchdir/subdir2") << (searchDir+"/subdir1/picker.png");
    QTest::newRow("precedence2") << "searchpath:picker.png" << QString("invalid;searchpath") << QString("invalid;" SRCDIR "searchdir/subdir2," SRCDIR "searchdir/subdir1") << (searchDir+"/subdir2/picker.png");
    QTest::newRow("precedence3") << "searchpath2:picker.png" << QString("searchpath1;searchpath2") << QString(SRCDIR "searchdir/subdir1;" SRCDIR "searchdir/subdir2") << (searchDir+"/subdir2/picker.png");

    // re
}

void tst_QDir::searchPaths()
{
    QFETCH(QString, filename);
    QFETCH(QString, searchPathPrefixes);
    QStringList searchPathPrefixList = searchPathPrefixes.split(";", QString::SkipEmptyParts);
    QFETCH(QString, searchPaths);
    QStringList searchPathsList = searchPaths.split(";", QString::SkipEmptyParts);
    QFETCH(QString, expectedAbsolutePath);
    bool exists = !expectedAbsolutePath.isEmpty();

    for (int i = 0; i < searchPathPrefixList.count(); ++i) {
        QDir::setSearchPaths(searchPathPrefixList.at(i), searchPathsList.at(i).split(","));
    }
    for (int i = 0; i < searchPathPrefixList.count(); ++i) {
        QVERIFY(QDir::searchPaths(searchPathPrefixList.at(i)) == searchPathsList.at(i).split(","));
    }

    QCOMPARE(QFile(filename).exists(), exists);
    QCOMPARE(QFileInfo(filename).exists(), exists);

    if (exists) {
        QCOMPARE(QFileInfo(filename).absoluteFilePath(), expectedAbsolutePath);
    }

    for (int i = 0; i < searchPathPrefixList.count(); ++i) {
        QDir::setSearchPaths(searchPathPrefixList.at(i), QStringList());
    }
    for (int i = 0; i < searchPathPrefixList.count(); ++i) {
        QVERIFY(QDir::searchPaths(searchPathPrefixList.at(i)).isEmpty());
    }

    for (int i = 0; i < searchPathPrefixList.count(); ++i) {
        foreach (QString path, searchPathsList.at(i).split(",")) {
            QDir::addSearchPath(searchPathPrefixList.at(i), path);
        }
    }
    for (int i = 0; i < searchPathPrefixList.count(); ++i) {
        QVERIFY(QDir::searchPaths(searchPathPrefixList.at(i)) == searchPathsList.at(i).split(","));
    }

    QCOMPARE(QFile(filename).exists(), exists);
    QCOMPARE(QFileInfo(filename).exists(), exists);

    if (exists) {
        QCOMPARE(QFileInfo(filename).absoluteFilePath(), expectedAbsolutePath);
    }

    for (int i = 0; i < searchPathPrefixList.count(); ++i) {
        QDir::setSearchPaths(searchPathPrefixList.at(i), QStringList());
    }
    for (int i = 0; i < searchPathPrefixList.count(); ++i) {
        QVERIFY(QDir::searchPaths(searchPathPrefixList.at(i)).isEmpty());
    }
}

void tst_QDir::entryListWithSearchPaths()
{
    QDir realDir(":/tst_qdir/resources/entryList");
    QVERIFY(realDir.exists());
    QVERIFY(!realDir.entryList().isEmpty());
    QVERIFY(realDir.entryList().contains("file3.data"));

    QDir::setSearchPaths("searchpath", QStringList(":/tst_qdir/resources"));
    QDir dir("searchpath:entryList/");
    QCOMPARE(dir.path(), QString(":/tst_qdir/resources/entryList"));
    QVERIFY(dir.exists());
    QStringList entryList = dir.entryList();
    QVERIFY(entryList.contains("file3.data"));
}

void tst_QDir::longFileName_data()
{
    QTest::addColumn<int>("length");

    QTest::newRow("128") << 128;
    QTest::newRow("256") << 256;
    QTest::newRow("512") << 512;
    QTest::newRow("1024") << 1024;
    QTest::newRow("2048") << 2048;
    QTest::newRow("4096") << 4096;
}

void tst_QDir::longFileName()
{
    QFETCH(int, length);

    QString fileName(length, QLatin1Char('a'));
    fileName += QLatin1String(".txt");

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        QSKIP("Cannot create long file names", SkipAll);

    QFile file2(fileName);
    QVERIFY(file2.open(QFile::ReadOnly));

    QVERIFY(QDir().entryList().contains(fileName));

    file.close();
    file2.close();

    QFile::remove(fileName);
}

void tst_QDir::updateFileLists()
{
    //  Test setup

    FileSystem fs;

    QVERIFY( fs.createDirectory("update-file-lists") );
    QVERIFY( fs.createFile("update-file-lists/file1.txt") );
    QVERIFY( fs.createFile("update-file-lists/file2.doc") );

    QVERIFY( fs.createDirectory("update-file-lists/sub-dir1") );
    QVERIFY( fs.createFile("update-file-lists/sub-dir1/file3.txt") );
    QVERIFY( fs.createFile("update-file-lists/sub-dir1/file4.doc") );
    QVERIFY( fs.createFile("update-file-lists/sub-dir1/file5.txt") );

    QVERIFY( fs.createDirectory("update-file-lists/sub-dir2") );
    QVERIFY( fs.createFile("update-file-lists/sub-dir2/file6.txt") );
    QVERIFY( fs.createFile("update-file-lists/sub-dir2/file7.txt") );
    QVERIFY( fs.createFile("update-file-lists/sub-dir2/file8.doc") );
    QVERIFY( fs.createFile("update-file-lists/sub-dir2/file9.doc") );

    //  Actual test

    QDir dir("update-file-lists");

#if defined(Q_OS_SYMBIAN) || defined(Q_OS_WINCE)
    //no . and .. on these OS.
    QCOMPARE(dir.count(), uint(4));
    QCOMPARE(dir.entryList().size(), 4);
    QCOMPARE(dir.entryInfoList().size(), 4);
#else
    QCOMPARE(dir.count(), uint(6));
    QCOMPARE(dir.entryList().size(), 6);
    QCOMPARE(dir.entryInfoList().size(), 6);
#endif

    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    QCOMPARE(dir.entryList().size(), 4);
    QCOMPARE(dir.count(), uint(4));
    QCOMPARE(dir.entryInfoList().size(), 4);

    dir.setPath("update-file-lists/sub-dir1");

    QCOMPARE(dir.entryInfoList().size(), 3);
    QCOMPARE(dir.count(), uint(3));
    QCOMPARE(dir.entryList().size(), 3);

    dir.setNameFilters(QStringList("*.txt"));

    QCOMPARE(dir.entryInfoList().size(), 2);
    QCOMPARE(dir.entryList().size(), 2);
    QCOMPARE(dir.count(), uint(2));

    dir.setPath("update-file-lists");
    dir = QDir(dir.path(),
            "*.txt",
            QDir::Name | QDir::DirsLast,
            QDir::AllEntries | QDir::AllDirs | QDir::NoDotAndDotDot);

    QCOMPARE(dir.count(), uint(3));
    QCOMPARE(dir.entryList().size(), 3);
    QCOMPARE(dir.entryInfoList().size(), 3);
    QCOMPARE(dir.entryList(), QStringList() << "file1.txt" << "sub-dir1" << "sub-dir2");

    dir.setSorting(QDir::Name | QDir::DirsFirst);

    QCOMPARE(dir.count(), uint(3));
    QCOMPARE(dir.entryList().size(), 3);
    QCOMPARE(dir.entryInfoList().size(), 3);
    QCOMPARE(dir.entryList(), QStringList() << "sub-dir1" << "sub-dir2" << "file1.txt");

    {
        QVERIFY( fs.createFile("update-file-lists/extra-file.txt") );

        QDir dir2(dir);

        QCOMPARE(dir2.count(), uint(3));
        QCOMPARE(dir2.entryList().size(), 3);
        QCOMPARE(dir2.entryInfoList().size(), 3);
        QCOMPARE(dir2.entryList(), QStringList() << "sub-dir1" << "sub-dir2" << "file1.txt");

        dir2.refresh();

        QCOMPARE(dir2.count(), uint(4));
        QCOMPARE(dir2.entryList().size(), 4);
        QCOMPARE(dir2.entryInfoList().size(), 4);
        QCOMPARE(dir2.entryList(), QStringList() << "sub-dir1" << "sub-dir2" << "extra-file.txt" << "file1.txt");
    }

    QCOMPARE(dir.count(), uint(3));
    QCOMPARE(dir.entryList().size(), 3);
    QCOMPARE(dir.entryInfoList().size(), 3);
    QCOMPARE(dir.entryList(), QStringList() << "sub-dir1" << "sub-dir2" << "file1.txt");
}

void tst_QDir::detachingOperations()
{
    QString const defaultPath(".");
    QStringList const defaultNameFilters = QStringList("*");
    QDir::SortFlags const defaultSorting = QDir::Name | QDir::IgnoreCase;
    QDir::Filters const defaultFilter = QDir::AllEntries;

    QString const path1("..");
    QString const path2("./foo");
    QStringList const nameFilters = QStringList(QString("*.txt"));
    QDir::SortFlags const sorting = QDir::Name | QDir::DirsLast | QDir::Reversed;
    QDir::Filters const filter = QDir::Writable;

    QDir dir1;

    QCOMPARE(dir1.path(), defaultPath);
    QCOMPARE(dir1.filter(), defaultFilter);
    QCOMPARE(dir1.nameFilters(), defaultNameFilters);
    QCOMPARE(dir1.sorting(), defaultSorting);

    dir1.setPath(path1);
    QCOMPARE(dir1.path(), path1);
    QCOMPARE(dir1.filter(), defaultFilter);
    QCOMPARE(dir1.nameFilters(), defaultNameFilters);
    QCOMPARE(dir1.sorting(), defaultSorting);

    dir1.setFilter(filter);
    QCOMPARE(dir1.path(), path1);
    QCOMPARE(dir1.filter(), filter);
    QCOMPARE(dir1.nameFilters(), defaultNameFilters);
    QCOMPARE(dir1.sorting(), defaultSorting);

    dir1.setNameFilters(nameFilters);
    QCOMPARE(dir1.path(), path1);
    QCOMPARE(dir1.filter(), filter);
    QCOMPARE(dir1.nameFilters(), nameFilters);
    QCOMPARE(dir1.sorting(), defaultSorting);

    dir1.setSorting(sorting);
    QCOMPARE(dir1.path(), path1);
    QCOMPARE(dir1.filter(), filter);
    QCOMPARE(dir1.nameFilters(), nameFilters);
    QCOMPARE(dir1.sorting(), sorting);

    dir1.setPath(path2);
    QCOMPARE(dir1.path(), path2);
    QCOMPARE(dir1.filter(), filter);
    QCOMPARE(dir1.nameFilters(), nameFilters);
    QCOMPARE(dir1.sorting(), sorting);

    {
        QDir dir2(dir1);
        QCOMPARE(dir2.path(), path2);
        QCOMPARE(dir2.filter(), filter);
        QCOMPARE(dir2.nameFilters(), nameFilters);
        QCOMPARE(dir2.sorting(), sorting);
    }

    {
        QDir dir2;
        QCOMPARE(dir2.path(), defaultPath);
        QCOMPARE(dir2.filter(), defaultFilter);
        QCOMPARE(dir2.nameFilters(), defaultNameFilters);
        QCOMPARE(dir2.sorting(), defaultSorting);

        dir2 = dir1;
        QCOMPARE(dir2.path(), path2);
        QCOMPARE(dir2.filter(), filter);
        QCOMPARE(dir2.nameFilters(), nameFilters);
        QCOMPARE(dir2.sorting(), sorting);

        dir2 = path1;
        QCOMPARE(dir2.path(), path1);
        QCOMPARE(dir2.filter(), filter);
        QCOMPARE(dir2.nameFilters(), nameFilters);
        QCOMPARE(dir2.sorting(), sorting);
    }

    dir1.refresh();
    QCOMPARE(dir1.path(), path2);
    QCOMPARE(dir1.filter(), filter);
    QCOMPARE(dir1.nameFilters(), nameFilters);
    QCOMPARE(dir1.sorting(), sorting);

    QString const currentPath = QDir::currentPath();
    QVERIFY(dir1.cd(currentPath));
    QCOMPARE(dir1.path(), currentPath);
    QCOMPARE(dir1.filter(), filter);
    QCOMPARE(dir1.nameFilters(), nameFilters);
    QCOMPARE(dir1.sorting(), sorting);

    QVERIFY(dir1.cdUp());
    QCOMPARE(dir1.filter(), filter);
    QCOMPARE(dir1.nameFilters(), nameFilters);
    QCOMPARE(dir1.sorting(), sorting);
}

void tst_QDir::testCaching()
{
    QString dirName = QString::fromLatin1("testCaching");
    QDir::current().rmdir(dirName); // cleanup a previous run.
    QDir dir(dirName);
    QVERIFY(!dir.exists());
    QDir::current().mkdir(dirName);
    QVERIFY(QDir(dirName).exists()); // dir exists
    QVERIFY(dir.exists()); // QDir doesn't cache the 'exist' between calls.
}

void tst_QDir::isRoot_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isRoot");

    QString test = QDir::rootPath();
    QTest::newRow(QString("rootPath " + test).toLatin1()) << test << true;
    test = QDir::rootPath().append("./");
    QTest::newRow(QString("./ appended " + test).toLatin1()) << test << false;

    test = QDir(QDir::rootPath().append("./")).canonicalPath();
#ifdef Q_OS_MAC
    // On Mac OS X 10.5 and earlier, canonicalPath depends on cleanPath which
    // is itself very broken and fundamentally wrong on "/./", which this would
    // exercise
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_6)
#endif
        QTest::newRow(QString("canonicalPath " + test).toLatin1()) << test << true;

#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    test = QDir::rootPath().left(2);
    QTest::newRow(QString("drive relative " + test).toLatin1()) << test << false;
#endif

    QTest::newRow("resources root") << ":/" << true;
    QTest::newRow("resources nonroot") << ":/entrylist" << false;
}

void tst_QDir::isRoot()
{
    QFETCH(QString, path);
    QFETCH(bool, isRoot);

    QDir dir(path);
    QCOMPARE(dir.isRoot(),isRoot);
}

#ifndef QT_NO_REGEXP
void tst_QDir::match_data()
{
    QTest::addColumn<QString>("filter");
    QTest::addColumn<QString>("filename");
    QTest::addColumn<bool>("match");

    QTest::newRow("single, matching") << "*.cpp" << "tst_qdir.cpp" << true;
    QTest::newRow("single, not matching") << "*.cpp" << "tst_qdir.h" << false;
    QTest::newRow("multi, matching") << "*.cpp;*.h" << "tst_qdir.cpp" << true;
    QTest::newRow("multi, matching2") << "*.cpp;*.h" << "tst_qdir.h" << true;
    QTest::newRow("multi, not matching") << "*.cpp;*.h" << "readme.txt" << false;
}

void tst_QDir::match()
{
    QFETCH(QString, filter);
    QFETCH(QString, filename);
    QFETCH(bool, match);

    QCOMPARE(QDir::match(filter, filename), match);
    QCOMPARE(QDir::match(filter.split(QLatin1Char(';')), filename), match);
}
#endif

void tst_QDir::drives()
{
    QFileInfoList list(QDir::drives());
#if defined(Q_OS_WIN)
    QVERIFY(list.count() >= 1); //system
    QLatin1Char systemdrive('c');
#elif defined(Q_OS_SYMBIAN)
    QVERIFY(list.count() >= 2); //system, rom
    QLatin1Char romdrive('z');
    QLatin1Char systemdrive('a' + int(RFs::GetSystemDrive()));
#endif
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    QVERIFY(list.count() <= 26);
    bool foundsystem = false;
#ifdef Q_OS_SYMBIAN
    bool foundrom = false;
#endif
    foreach (QFileInfo fi, list) {
        QCOMPARE(fi.absolutePath().size(), 3); //"x:/"
        QCOMPARE(fi.absolutePath().at(1), QChar(QLatin1Char(':')));
        QCOMPARE(fi.absolutePath().at(2), QChar(QLatin1Char('/')));
        if (fi.absolutePath().at(0).toLower() == systemdrive)
            foundsystem = true;
#ifdef Q_OS_SYMBIAN
        if (fi.absolutePath().at(0).toLower() == romdrive)
            foundrom = true;
#endif
    }
    QCOMPARE(foundsystem, true);
#ifdef Q_OS_SYMBIAN
    QCOMPARE(foundrom, true);
#endif
#else
    QCOMPARE(list.count(), 1); //root
    QCOMPARE(list.at(0).absolutePath(), QLatin1String("/"));
#endif
}

void tst_QDir::arrayOperator()
{
    QDir dir1(SRCDIR "entrylist/");
    QDir dir2(SRCDIR "entrylist/");

    QStringList entries(dir1.entryList());
    int i = dir2.count();
    QCOMPARE(i, entries.count());
    --i;
    for (;i>=0;--i) {
        QCOMPARE(dir2[i], entries.at(i));
    }
}

void tst_QDir::equalityOperator_data()
{
    QTest::addColumn<QString>("leftPath");
    QTest::addColumn<QString>("leftNameFilters");
    QTest::addColumn<int>("leftSort");
    QTest::addColumn<int>("leftFilters");
    QTest::addColumn<QString>("rightPath");
    QTest::addColumn<QString>("rightNameFilters");
    QTest::addColumn<int>("rightSort");
    QTest::addColumn<int>("rightFilters");
    QTest::addColumn<bool>("expected");

    QTest::newRow("same") << SRCDIR << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << SRCDIR << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << true;

    QTest::newRow("relativepaths") << "entrylist/" << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << "./entrylist" << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << true;

    QTest::newRow("QTBUG-20495") << QDir::currentPath() + "/entrylist/.." << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << "." << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << true;

    QTest::newRow("QTBUG-20495-root") << QDir::rootPath() + "tmp/.." << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << QDir::rootPath() << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << true;

    QTest::newRow("diff-filters") << SRCDIR << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << SRCDIR << "*.cpp" << int(QDir::Name) << int(QDir::Dirs)
        << false;

    QTest::newRow("diff-sort") << SRCDIR << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << SRCDIR << "*.cpp" << int(QDir::Time) << int(QDir::Files)
        << false;

    QTest::newRow("diff-namefilters") << SRCDIR << "*.cpp" << int(QDir::Name) << int(QDir::Files)
        << SRCDIR << "*.jpg" << int(QDir::Name) << int(QDir::Files)
        << false;
}

void tst_QDir::equalityOperator()
{
    QFETCH(QString, leftPath);
    QFETCH(QString, leftNameFilters);
    QFETCH(int, leftSort);
    QFETCH(int, leftFilters);
    QFETCH(QString, rightPath);
    QFETCH(QString, rightNameFilters);
    QFETCH(int, rightSort);
    QFETCH(int, rightFilters);
    QFETCH(bool, expected);

    QDir dir1(leftPath, leftNameFilters, QDir::SortFlags(leftSort), QDir::Filters(leftFilters));
    QDir dir2(rightPath, rightNameFilters, QDir::SortFlags(rightSort), QDir::Filters(rightFilters));

    QCOMPARE((dir1 == dir2), expected);
    QCOMPARE((dir2 == dir1), expected);
    QCOMPARE((dir1 != dir2), !expected);
    QCOMPARE((dir2 != dir1), !expected);
}

void tst_QDir::isRelative_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("relative");

    QTest::newRow(".") << "./" << true;
    QTest::newRow("..") << "../" << true;
    QTest::newRow("content") << "entrylist/" << true;
    QTest::newRow("current") << QDir::currentPath() << false;
    QTest::newRow("homepath") << QDir::homePath() << false;
    QTest::newRow("temppath") << QDir::tempPath() << false;
    QTest::newRow("rootpath") << QDir::rootPath() << false;
    foreach (QFileInfo root, QDir::drives()) {
        QTest::newRow(root.absolutePath().toLocal8Bit()) << root.absolutePath() << false;
    }

    QTest::newRow("resource") << ":/prefix" << false;
}

void tst_QDir::isRelative()
{
    QFETCH(QString, path);
    QFETCH(bool, relative);

    QCOMPARE(QDir(path).isRelative(), relative);
}

void tst_QDir::isReadable()
{
    QDir dir;

    QVERIFY(dir.isReadable());
#if defined (Q_OS_UNIX) && !defined (Q_OS_SYMBIAN)
    QVERIFY(dir.mkdir("nonreadabledir"));
    QVERIFY(0 == ::chmod("nonreadabledir", 0));
    QVERIFY(!QDir("nonreadabledir").isReadable());
    QVERIFY(0 == ::chmod("nonreadabledir", S_IRUSR | S_IWUSR | S_IXUSR));
    QVERIFY(dir.rmdir("nonreadabledir"));
#endif
}

QTEST_MAIN(tst_QDir)
#include "tst_qdir.moc"

