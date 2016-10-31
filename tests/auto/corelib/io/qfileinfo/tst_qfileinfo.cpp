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

#include <qfile.h>
#include <qdir.h>
#include <qcoreapplication.h>
#include <qlibrary.h>
#include <qtemporaryfile.h>
#include <qtemporarydir.h>
#include <qdir.h>
#include <qfileinfo.h>
#ifdef Q_OS_UNIX
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef Q_OS_VXWORKS
#include <pwd.h>
#endif
#endif
#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <qlibrary.h>
#if !defined(Q_OS_WINRT)
#include <lm.h>
#endif
#endif
#include <qplatformdefs.h>
#include <qdebug.h>
#if defined(Q_OS_WIN)
#include "../../../network-settings.h"
#endif
#include <private/qfileinfo_p.h>
#include "../../../../shared/filesystem.h"

#if defined(Q_OS_VXWORKS) || defined(Q_OS_WINRT)
#define Q_NO_SYMLINKS
#endif


#if defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
inline bool qt_isEvilFsTypeName(const char *name)
{
    return (qstrncmp(name, "nfs", 3) == 0
            || qstrncmp(name, "autofs", 6) == 0
            || qstrncmp(name, "cachefs", 7) == 0);
}

#if defined(Q_OS_BSD4) && !defined(Q_OS_NETBSD)
# include <sys/param.h>
# include <sys/mount.h>

bool qIsLikelyToBeNfs(int handle)
{
    struct statfs buf;
    if (fstatfs(handle, &buf) != 0)
        return false;
    return qt_isEvilFsTypeName(buf.f_fstypename);
}

#elif defined(Q_OS_LINUX) || defined(Q_OS_HURD)

# include <sys/vfs.h>
# ifdef QT_LINUXBASE
   // LSB 3.2 has fstatfs in sys/statfs.h, sys/vfs.h is just an empty dummy header
#  include <sys/statfs.h>
# endif

# ifndef NFS_SUPER_MAGIC
#  define NFS_SUPER_MAGIC       0x00006969
# endif
# ifndef AUTOFS_SUPER_MAGIC
#  define AUTOFS_SUPER_MAGIC    0x00000187
# endif
# ifndef AUTOFSNG_SUPER_MAGIC
#  define AUTOFSNG_SUPER_MAGIC  0x7d92b1a0
# endif

bool qIsLikelyToBeNfs(int handle)
{
    struct statfs buf;
    if (fstatfs(handle, &buf) != 0)
        return false;
    return buf.f_type == NFS_SUPER_MAGIC
           || buf.f_type == AUTOFS_SUPER_MAGIC
           || buf.f_type == AUTOFSNG_SUPER_MAGIC;
}

#elif defined(Q_OS_SOLARIS) || defined(Q_OS_IRIX) || defined(Q_OS_AIX) || defined(Q_OS_HPUX) \
      || defined(Q_OS_OSF) || defined(Q_OS_QNX) || defined(Q_OS_SCO) \
      || defined(Q_OS_UNIXWARE) || defined(Q_OS_RELIANT) || defined(Q_OS_NETBSD)

# include <sys/statvfs.h>

bool qIsLikelyToBeNfs(int handle)
{
    struct statvfs buf;
    if (fstatvfs(handle, &buf) != 0)
        return false;
#if defined(Q_OS_NETBSD)
    return qt_isEvilFsTypeName(buf.f_fstypename);
#else
    return qt_isEvilFsTypeName(buf.f_basetype);
#endif
}
#else
inline bool qIsLikelyToBeNfs(int /* handle */)
{
    return false;
}
#endif
#endif

static QString seedAndTemplate()
{
    qsrand(QDateTime::currentSecsSinceEpoch());
    return QDir::tempPath() + "/tst_qfileinfo-XXXXXX";
}

static QByteArray msgDoesNotExist(const QString &name)
{
    return (QLatin1Char('"') + QDir::toNativeSeparators(name)
        + QLatin1String("\" does not exist.")).toLocal8Bit();
}

static QByteArray msgIsNoDirectory(const QString &name)
{
    return (QLatin1Char('"') + QDir::toNativeSeparators(name)
        + QLatin1String("\" is not a directory.")).toLocal8Bit();
}

static QByteArray msgIsNotRoot(const QString &name)
{
    return (QLatin1Char('"') + QDir::toNativeSeparators(name)
        + QLatin1String("\" is no root directory.")).toLocal8Bit();
}

class tst_QFileInfo : public QObject
{
Q_OBJECT

public:
    tst_QFileInfo() : m_currentDir(QDir::currentPath()), m_dir(seedAndTemplate())
 {}

private slots:
    void initTestCase();
    void cleanupTestCase();

    void getSetCheck();

    void copy();

    void isFile_data();
    void isFile();

    void isDir_data();
    void isDir();

    void isRoot_data();
    void isRoot();

    void exists_data();
    void exists();

    void absolutePath_data();
    void absolutePath();

    void absFilePath_data();
    void absFilePath();

    void canonicalPath();
    void canonicalFilePath();

    void fileName_data();
    void fileName();

    void bundleName_data();
    void bundleName();

    void dir_data();
    void dir();

    void suffix_data();
    void suffix();

    void completeSuffix_data();
    void completeSuffix();

    void baseName_data();
    void baseName();

    void completeBaseName_data();
    void completeBaseName();

    void permission_data();
    void permission();

    void size_data();
    void size();

    void systemFiles();

    void compare_data();
    void compare();

    void consistent_data();
    void consistent();

    void fileTimes_data();
    void fileTimes();
    void fileTimes_oldFile();

    void isSymLink_data();
    void isSymLink();

    void isHidden_data();
    void isHidden();
#if defined(Q_OS_MAC)
    void isHiddenFromFinder();
#endif

    void isBundle_data();
    void isBundle();

    void isNativePath_data();
    void isNativePath();

    void refresh();

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    void ntfsJunctionPointsAndSymlinks_data();
    void ntfsJunctionPointsAndSymlinks();
    void brokenShortcut();
#endif

    void isWritable();
    void isExecutable();
    void testDecomposedUnicodeNames_data();
    void testDecomposedUnicodeNames();

    void equalOperator() const;
    void equalOperatorWithDifferentSlashes() const;
    void notEqualOperator() const;

    void detachingOperations();

#if !defined(Q_OS_WINRT)
    void owner();
#endif
    void group();

    void invalidState();
    void nonExistingFileDates();

private:
    const QString m_currentDir;
    QString m_sourceFile;
    QString m_proFile;
    QString m_resourcesDir;
    QTemporaryDir m_dir;
    QSharedPointer<QTemporaryDir> m_dataDir;
};

void tst_QFileInfo::initTestCase()
{
    m_dataDir = QEXTRACTTESTDATA("/testdata");
    QVERIFY(m_dataDir);
    const QString dataPath = m_dataDir->path();
    QVERIFY(!dataPath.isEmpty());

    m_sourceFile = dataPath + QLatin1String("/tst_qfileinfo.cpp");
    m_resourcesDir = dataPath + QLatin1String("/resources");
    m_proFile = dataPath + QLatin1String("/tst_qfileinfo.pro");

    QVERIFY2(m_dir.isValid(),
             ("Failed to create temporary dir: " + m_dir.errorString()).toUtf8());
    QVERIFY(QDir::setCurrent(m_dir.path()));
}

void tst_QFileInfo::cleanupTestCase()
{
    QDir::setCurrent(m_currentDir); // Release temporary directory so that it can be deleted on Windows
}

// Testing get/set functions
void tst_QFileInfo::getSetCheck()
{
    QFileInfo obj1;
    // bool QFileInfo::caching()
    // void QFileInfo::setCaching(bool)
    obj1.setCaching(false);
    QCOMPARE(false, obj1.caching());
    obj1.setCaching(true);
    QCOMPARE(true, obj1.caching());
}

static QFileInfoPrivate* getPrivate(QFileInfo &info)
{
    return (*reinterpret_cast<QFileInfoPrivate**>(&info));
}

void tst_QFileInfo::copy()
{
    QTemporaryFile t;
    QVERIFY2(t.open(), qPrintable(t.errorString()));
    QFileInfo info(t.fileName());
    QVERIFY(info.exists());

    //copy constructor
    QFileInfo info2(info);
    QFileInfoPrivate *privateInfo = getPrivate(info);
    QFileInfoPrivate *privateInfo2 = getPrivate(info2);
    QCOMPARE(privateInfo, privateInfo2);

    //operator =
    QFileInfo info3 = info;
    QFileInfoPrivate *privateInfo3 = getPrivate(info3);
    QCOMPARE(privateInfo, privateInfo3);
    QCOMPARE(privateInfo2, privateInfo3);

    //refreshing info3 will detach it
    QFile file(info.absoluteFilePath());
    QVERIFY(file.open(QFile::WriteOnly));
    QCOMPARE(file.write("JAJAJAA"), qint64(7));
    file.flush();

    QTest::qWait(250);
#if defined(Q_OS_WIN)
    file.close();
#endif
    info3.refresh();
    privateInfo3 = getPrivate(info3);
    QVERIFY(privateInfo != privateInfo3);
    QVERIFY(privateInfo2 != privateInfo3);
    QCOMPARE(privateInfo, privateInfo2);
}

void tst_QFileInfo::isFile_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("expected");

    QTest::newRow("data0") << QDir::currentPath() << false;
    QTest::newRow("data1") << m_sourceFile << true;
    QTest::newRow("data2") << ":/tst_qfileinfo/resources/" << false;
    QTest::newRow("data3") << ":/tst_qfileinfo/resources/file1" << true;
    QTest::newRow("data4") << ":/tst_qfileinfo/resources/afilethatshouldnotexist" << false;
}

void tst_QFileInfo::isFile()
{
    QFETCH(QString, path);
    QFETCH(bool, expected);

    QFileInfo fi(path);
    QCOMPARE(fi.isFile(), expected);
}


void tst_QFileInfo::isDir_data()
{
    // create a broken symlink
    QFile::remove("brokenlink.lnk");
    QFile::remove("dummyfile");
    QFile file3("dummyfile");
    file3.open(QIODevice::WriteOnly);
    if (file3.link("brokenlink.lnk")) {
        file3.remove();
        QFileInfo info3("brokenlink.lnk");
        QVERIFY( info3.isSymLink() );
    }

    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("expected");

    QTest::newRow("data0") << QDir::currentPath() << true;
    QTest::newRow("data1") << m_sourceFile << false;
    QTest::newRow("data2") << ":/tst_qfileinfo/resources/" << true;
    QTest::newRow("data3") << ":/tst_qfileinfo/resources/file1" << false;
    QTest::newRow("data4") << ":/tst_qfileinfo/resources/afilethatshouldnotexist" << false;

    QTest::newRow("simple dir") << m_resourcesDir << true;
    QTest::newRow("simple dir with slash") << (m_resourcesDir + QLatin1Char('/')) << true;

    QTest::newRow("broken link") << "brokenlink.lnk" << false;

#if (defined(Q_OS_WIN) && !defined(Q_OS_WINRT))
    QTest::newRow("drive 1") << "c:" << true;
    QTest::newRow("drive 2") << "c:/" << true;
    //QTest::newRow("drive 2") << "t:s" << false;
#endif
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    const QString uncRoot = QStringLiteral("//") + QtNetworkSettings::winServerName();
    QTest::newRow("unc 1") << uncRoot << true;
    QTest::newRow("unc 2") << uncRoot + QLatin1Char('/') << true;
    QTest::newRow("unc 3") << uncRoot + "/testshare" << true;
    QTest::newRow("unc 4") << uncRoot + "/testshare/" << true;
    QTest::newRow("unc 5") << uncRoot + "/testshare/tmp" << true;
    QTest::newRow("unc 6") << uncRoot + "/testshare/tmp/" << true;
    QTest::newRow("unc 7") << uncRoot + "/testshare/adirthatshouldnotexist" << false;
#endif
}

void tst_QFileInfo::isDir()
{
    QFETCH(QString, path);
    QFETCH(bool, expected);

    const bool isDir = QFileInfo(path).isDir();
    if (expected)
        QVERIFY2(isDir, msgIsNoDirectory(path).constData());
    else
        QVERIFY(!isDir);
}

void tst_QFileInfo::isRoot_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("expected");
    QTest::newRow("data0") << QDir::currentPath() << false;
    QTest::newRow("data1") << "/" << true;
    QTest::newRow("data2") << "*" << false;
    QTest::newRow("data3") << "/*" << false;
    QTest::newRow("data4") << ":/tst_qfileinfo/resources/" << false;
    QTest::newRow("data5") << ":/" << true;

    QTest::newRow("simple dir") << m_resourcesDir << false;
    QTest::newRow("simple dir with slash") << (m_resourcesDir + QLatin1Char('/')) << false;
#if (defined(Q_OS_WIN) && !defined(Q_OS_WINRT))
    QTest::newRow("drive 1") << "c:" << false;
    QTest::newRow("drive 2") << "c:/" << true;
    QTest::newRow("drive 3") << "p:/" << false;
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    const QString uncRoot = QStringLiteral("//") + QtNetworkSettings::winServerName();
    QTest::newRow("unc 1") << uncRoot << true;
    QTest::newRow("unc 2") << uncRoot + QLatin1Char('/') << true;
    QTest::newRow("unc 3") << uncRoot + "/testshare" << false;
    QTest::newRow("unc 4") << uncRoot + "/testshare/" << false;
    QTest::newRow("unc 7") << "//ahostthatshouldnotexist" << false;
#endif
}

void tst_QFileInfo::isRoot()
{
    QFETCH(QString, path);
    QFETCH(bool, expected);

    const bool isRoot = QFileInfo(path).isRoot();
    if (expected)
        QVERIFY2(isRoot, msgIsNotRoot(path).constData());
    else
        QVERIFY(!isRoot);
}

void tst_QFileInfo::exists_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("expected");

    QTest::newRow("data0") << QDir::currentPath() << true;
    QTest::newRow("data1") << m_sourceFile << true;
    QTest::newRow("data2") << "/I/do_not_expect_this_path_to_exist/" << false;
    QTest::newRow("data3") << ":/tst_qfileinfo/resources/" << true;
    QTest::newRow("data4") << ":/tst_qfileinfo/resources/file1" << true;
    QTest::newRow("data5") << ":/I/do_not_expect_this_path_to_exist/" << false;
    QTest::newRow("data6") << (m_resourcesDir + "/*") << false;
    QTest::newRow("data7") << (m_resourcesDir + "/*.foo") << false;
    QTest::newRow("data8") << (m_resourcesDir + "/*.ext1") << false;
    QTest::newRow("data9") << (m_resourcesDir + "/file?.ext1") << false;
    QTest::newRow("data10") << "." << true;

    // Skip for the WinRT case, as GetFileAttributesEx removes _any_
    // trailing whitespace and "." is a valid entry as seen in data10
#ifndef Q_OS_WINRT
    QTest::newRow("data11") << ". " << false;
#endif
    QTest::newRow("empty") << "" << false;

    QTest::newRow("simple dir") << m_resourcesDir << true;
    QTest::newRow("simple dir with slash") << (m_resourcesDir + QLatin1Char('/')) << true;

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    const QString uncRoot = QStringLiteral("//") + QtNetworkSettings::winServerName();
    QTest::newRow("unc 1") << uncRoot << true;
    QTest::newRow("unc 2") << uncRoot + QLatin1Char('/') << true;
    QTest::newRow("unc 3") << uncRoot + "/testshare" << true;
    QTest::newRow("unc 4") << uncRoot + "/testshare/" << true;
    QTest::newRow("unc 5") << uncRoot + "/testshare/tmp" << true;
    QTest::newRow("unc 6") << uncRoot + "/testshare/tmp/" << true;
    QTest::newRow("unc 7") << uncRoot + "/testshare/adirthatshouldnotexist" << false;
    QTest::newRow("unc 8") << uncRoot + "/asharethatshouldnotexist" << false;
    QTest::newRow("unc 9") << "//ahostthatshouldnotexist" << false;
#endif
}

void tst_QFileInfo::exists()
{
    QFETCH(QString, path);
    QFETCH(bool, expected);

    QFileInfo fi(path);
    const bool exists = fi.exists();
    QCOMPARE(exists, QFileInfo::exists(path));
    if (expected)
        QVERIFY2(exists, msgDoesNotExist(path).constData());
    else
        QVERIFY(!exists);
}

void tst_QFileInfo::absolutePath_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("filename");

    QString drivePrefix;
#if (defined(Q_OS_WIN) && !defined(Q_OS_WINRT))
    drivePrefix = QDir::currentPath().left(2);
    QString nonCurrentDrivePrefix =
        drivePrefix.left(1).compare("X", Qt::CaseInsensitive) == 0 ? QString("Y:") : QString("X:");

    // Make sure drive-relative paths return correct absolute paths.
    QTest::newRow("<current drive>:my.dll") << drivePrefix + "my.dll" << QDir::currentPath() << "my.dll";
    QTest::newRow("<not current drive>:my.dll") << nonCurrentDrivePrefix + "my.dll"
                                                << nonCurrentDrivePrefix + "/"
                                                << "my.dll";
#elif defined(Q_OS_WINRT)
    drivePrefix = QDir::currentPath().left(2);
#endif
    QTest::newRow("0") << "/machine/share/dir1/" << drivePrefix + "/machine/share/dir1" << "";
    QTest::newRow("1") << "/machine/share/dir1" << drivePrefix + "/machine/share" << "dir1";
    QTest::newRow("2") << "/usr/local/bin" << drivePrefix + "/usr/local" << "bin";
    QTest::newRow("3") << "/usr/local/bin/" << drivePrefix + "/usr/local/bin" << "";
    QTest::newRow("/test") << "/test" << drivePrefix + "/" << "test";

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    QTest::newRow("c:\\autoexec.bat") << "c:\\autoexec.bat" << "C:/"
                                      << "autoexec.bat";
    QTest::newRow("c:autoexec.bat") << QDir::currentPath().left(2) + "autoexec.bat" << QDir::currentPath()
                                    << "autoexec.bat";
#endif
    QTest::newRow("QTBUG-19995.1") << drivePrefix + "/System/Library/StartupItems/../Frameworks"
                                   << drivePrefix + "/System/Library"
                                   << "Frameworks";
    QTest::newRow("QTBUG-19995.2") << drivePrefix + "/System/Library/StartupItems/../Frameworks/"
                                   << drivePrefix + "/System/Library/Frameworks" << "";
}

void tst_QFileInfo::absolutePath()
{
    QFETCH(QString, file);
    QFETCH(QString, path);
    QFETCH(QString, filename);

    QFileInfo fi(file);

    QCOMPARE(fi.absolutePath(), path);
    QCOMPARE(fi.fileName(), filename);
}

void tst_QFileInfo::absFilePath_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("relativeFile") << "tmp.txt" << QDir::currentPath() + "/tmp.txt";
    QTest::newRow("relativeFileInSubDir") << "temp/tmp.txt" << QDir::currentPath() + "/" + "temp/tmp.txt";
    QString drivePrefix;
#if defined(Q_OS_WIN)
    QString curr = QDir::currentPath();

    curr.remove(0, 2);   // Make it a absolute path with no drive specifier: \depot\qt-4.2\tests\auto\qfileinfo
    QTest::newRow(".")            << curr << QDir::currentPath();
    QTest::newRow("absFilePath") << "c:\\home\\andy\\tmp.txt" << "C:/home/andy/tmp.txt";

    // Make sure drive-relative paths return correct absolute paths.
    drivePrefix = QDir::currentPath().left(2);
    QString nonCurrentDrivePrefix =
        drivePrefix.left(1).compare("X", Qt::CaseInsensitive) == 0 ? QString("Y:") : QString("X:");

    QTest::newRow("absFilePathWithoutSlash") << drivePrefix + "tmp.txt" << QDir::currentPath() + "/tmp.txt";
    QTest::newRow("<current drive>:my.dll") << drivePrefix + "temp/my.dll" << QDir::currentPath() + "/temp/my.dll";
    QTest::newRow("<not current drive>:my.dll") << nonCurrentDrivePrefix + "temp/my.dll"
                                                << nonCurrentDrivePrefix + "/temp/my.dll";
#else
    QTest::newRow("absFilePath") << "/home/andy/tmp.txt" << "/home/andy/tmp.txt";
#endif
    QTest::newRow("QTBUG-19995") << drivePrefix + "/System/Library/StartupItems/../Frameworks"
                                 << drivePrefix + "/System/Library/Frameworks";
}

void tst_QFileInfo::absFilePath()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileInfo fi(file);
#if defined(Q_OS_WIN)
    QVERIFY(QString::compare(fi.absoluteFilePath(), expected, Qt::CaseInsensitive) == 0);
#else
    QCOMPARE(fi.absoluteFilePath(), expected);
#endif
}

void tst_QFileInfo::canonicalPath()
{
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    QVERIFY2(tempFile.open(), qPrintable(tempFile.errorString()));
    QFileInfo fi(tempFile.fileName());
    QCOMPARE(fi.canonicalPath(), QFileInfo(QDir::tempPath()).canonicalFilePath());
}

class FileDeleter {
    Q_DISABLE_COPY(FileDeleter)
public:
    explicit FileDeleter(const QString fileName) : m_fileName(fileName) {}
    ~FileDeleter() { QFile::remove(m_fileName); }

private:
    const QString m_fileName;
};

void tst_QFileInfo::canonicalFilePath()
{
    const QString fileName("tmp.canon");
    QFile tempFile(fileName);
    QVERIFY(tempFile.open(QFile::WriteOnly));
    QFileInfo fi(tempFile.fileName());
    QCOMPARE(fi.canonicalFilePath(), QDir::currentPath() + "/" + fileName);
    tempFile.remove();

    // This used to crash on Mac, verify that it doesn't anymore.
    QFileInfo info("/tmp/../../../../../../../../../../../../../../../../../");
    info.canonicalFilePath();

#if defined(Q_OS_UNIX)
    // This used to crash on Mac
    QFileInfo dontCrash(QLatin1String("/"));
    QCOMPARE(dontCrash.canonicalFilePath(), QLatin1String("/"));
#endif

#ifndef Q_OS_WIN
    // test symlinks
    QFile::remove("link.lnk");
    {
        QFile file(m_sourceFile);
        if (file.link("link.lnk")) {
            QFileInfo info1(file);
            QFileInfo info2("link.lnk");
            QCOMPARE(info1.canonicalFilePath(), info2.canonicalFilePath());
        }
    }

    const QString dirSymLinkName = QLatin1String("tst_qfileinfo")
        + QDateTime::currentDateTime().toString(QLatin1String("yyMMddhhmmss"));
    const QString link(QDir::tempPath() + QLatin1Char('/') + dirSymLinkName);
    FileDeleter dirSymLinkDeleter(link);

    {
        QFile file(QDir::currentPath());
        if (file.link(link)) {
            QFile tempfile("tempfile.txt");
            tempfile.open(QIODevice::ReadWrite);
            tempfile.write("This file is generated by the QFileInfo autotest.");
            QVERIFY(tempfile.flush());
            tempfile.close();

            QFileInfo info1("tempfile.txt");
            QFileInfo info2(link + QDir::separator() + "tempfile.txt");

            QVERIFY(info1.exists());
            QVERIFY(info2.exists());
            QCOMPARE(info1.canonicalFilePath(), info2.canonicalFilePath());

            QFileInfo info3(link + QDir::separator() + "link.lnk");
            QFileInfo info4(m_sourceFile);
            QVERIFY(!info3.canonicalFilePath().isEmpty());
            QCOMPARE(info4.canonicalFilePath(), info3.canonicalFilePath());

            tempfile.remove();
        }
    }
    {
        QString link(QDir::tempPath() + QLatin1Char('/') + dirSymLinkName
                     + "/link_to_tst_qfileinfo");
        QFile::remove(link);

        QFile file(QDir::tempPath() + QLatin1Char('/') +  dirSymLinkName
                   + "tst_qfileinfo.cpp");
        if (file.link(link))
        {
            QFileInfo info1("tst_qfileinfo.cpp");
            QFileInfo info2(link);
            QCOMPARE(info1.canonicalFilePath(), info2.canonicalFilePath());
        }
    }
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    typedef BOOL (WINAPI *PtrCreateSymbolicLink)(LPTSTR, LPTSTR, DWORD);
    PtrCreateSymbolicLink ptrCreateSymbolicLink =
            (PtrCreateSymbolicLink)QLibrary::resolve(QLatin1String("kernel32"), "CreateSymbolicLinkW");

    if (!ptrCreateSymbolicLink) {
        QSKIP("Symbolic links aren't supported by FS");
    } else {
        // CreateSymbolicLink can return TRUE & still fail to create the link,
        // the error code in that case is ERROR_PRIVILEGE_NOT_HELD (1314)
        SetLastError(0);
        const QString linkTarget = QStringLiteral("res");
        BOOL ret = ptrCreateSymbolicLink((wchar_t*)linkTarget.utf16(), (wchar_t*)m_resourcesDir.utf16(), 1);
        DWORD dwErr = GetLastError();
        if (!ret)
            QSKIP("Symbolic links aren't supported by FS");
        QString currentPath = QDir::currentPath();
        bool is_res_Current = QDir::setCurrent(linkTarget);
        if (!is_res_Current && dwErr == 1314)
            QSKIP("Not enough privilages to create Symbolic links");
        QCOMPARE(is_res_Current, true);
        const QString actualCanonicalPath = QFileInfo("file1").canonicalFilePath();
        QVERIFY(QDir::setCurrent(currentPath));
        QCOMPARE(actualCanonicalPath, m_resourcesDir + QStringLiteral("/file1"));

        QDir::current().rmdir(linkTarget);
    }
#endif

#ifdef Q_OS_DARWIN
    {
        // Check if canonicalFilePath's result is in Composed normalization form.
        QString path = QString::fromLatin1("caf\xe9");
        QDir dir(QDir::tempPath());
        dir.mkdir(path);
        QString canonical = QFileInfo(dir.filePath(path)).canonicalFilePath();
        QString roundtrip = QFile::decodeName(QFile::encodeName(canonical));
        QCOMPARE(canonical, roundtrip);
        dir.rmdir(path);
    }
#endif
}

void tst_QFileInfo::fileName_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("relativeFile") << "tmp.txt" << "tmp.txt";
    QTest::newRow("relativeFileInSubDir") << "temp/tmp.txt" << "tmp.txt";
#if defined(Q_OS_WIN)
    QTest::newRow("absFilePath") << "c:\\home\\andy\\tmp.txt" << "tmp.txt";
    QTest::newRow("driveWithNoSlash") << "c:tmp.txt" << "tmp.txt";
#else
    QTest::newRow("absFilePath") << "/home/andy/tmp.txt" << "tmp.txt";
#endif
    QTest::newRow("resource1") << ":/tst_qfileinfo/resources/file1.ext1" << "file1.ext1";
    QTest::newRow("resource2") << ":/tst_qfileinfo/resources/file1.ext1.ext2" << "file1.ext1.ext2";

    QTest::newRow("ending slash [small]") << QString::fromLatin1("/a/") << QString::fromLatin1("");
    QTest::newRow("no ending slash [small]") << QString::fromLatin1("/a") << QString::fromLatin1("a");

    QTest::newRow("ending slash") << QString::fromLatin1("/somedir/") << QString::fromLatin1("");
    QTest::newRow("no ending slash") << QString::fromLatin1("/somedir") << QString::fromLatin1("somedir");
}

void tst_QFileInfo::fileName()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileInfo fi(file);
    QCOMPARE(fi.fileName(), expected);
}

void tst_QFileInfo::bundleName_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("root") << "/" << "";
    QTest::newRow("etc") << "/etc" << "";
#ifdef Q_OS_MAC
    QTest::newRow("safari") << "/Applications/Safari.app" << "Safari";
#endif
}

void tst_QFileInfo::bundleName()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileInfo fi(file);
    QCOMPARE(fi.bundleName(), expected);
}

void tst_QFileInfo::dir_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>("absPath");
    QTest::addColumn<QString>("expected");

    QTest::newRow("relativeFile") << "tmp.txt" << false << ".";
    QTest::newRow("relativeFileAbsPath") << "tmp.txt" << true << QDir::currentPath();
    QTest::newRow("relativeFileInSubDir") << "temp/tmp.txt" << false << "temp";
    QTest::newRow("relativeFileInSubDirAbsPath") << "temp/tmp.txt" << true << QDir::currentPath() + "/temp";
    QTest::newRow("absFilePath") << QDir::currentPath() + "/tmp.txt" << false << QDir::currentPath();
    QTest::newRow("absFilePathAbsPath") << QDir::currentPath() + "/tmp.txt" << true << QDir::currentPath();
    QTest::newRow("resource1") << ":/tst_qfileinfo/resources/file1.ext1" << true << ":/tst_qfileinfo/resources";
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    QTest::newRow("driveWithSlash") << "C:/file1.ext1.ext2" << true << "C:/";
    QTest::newRow("driveWithoutSlash") << QDir::currentPath().left(2) + "file1.ext1.ext2" << false << QDir::currentPath().left(2);
#endif
}

void tst_QFileInfo::dir()
{
    QFETCH(QString, file);
    QFETCH(bool, absPath);
    QFETCH(QString, expected);

    QFileInfo fi(file);
    if (absPath) {
        QCOMPARE(fi.absolutePath(), expected);
        QCOMPARE(fi.absoluteDir().path(), expected);
    } else {
        QCOMPARE(fi.path(), expected);
        QCOMPARE(fi.dir().path(), expected);
    }
}


void tst_QFileInfo::suffix_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("noextension0") << "file" << "";
    QTest::newRow("noextension1") << "/path/to/file" << "";
    QTest::newRow("data0") << "file.tar" << "tar";
    QTest::newRow("data1") << "file.tar.gz" << "gz";
    QTest::newRow("data2") << "/path/file/file.tar.gz" << "gz";
    QTest::newRow("data3") << "/path/file.tar" << "tar";
    QTest::newRow("resource1") << ":/tst_qfileinfo/resources/file1.ext1" << "ext1";
    QTest::newRow("resource2") << ":/tst_qfileinfo/resources/file1.ext1.ext2" << "ext2";
    QTest::newRow("hidden1") << ".ext1" << "ext1";
    QTest::newRow("hidden1") << ".ext" << "ext";
    QTest::newRow("hidden1") << ".ex" << "ex";
    QTest::newRow("hidden1") << ".e" << "e";
    QTest::newRow("hidden2") << ".ext1.ext2" << "ext2";
    QTest::newRow("hidden2") << ".ext.ext2" << "ext2";
    QTest::newRow("hidden2") << ".ex.ext2" << "ext2";
    QTest::newRow("hidden2") << ".e.ext2" << "ext2";
    QTest::newRow("hidden2") << "..ext2" << "ext2";
#ifdef Q_OS_WIN
    QTest::newRow("driveWithSlash") << "c:/file1.ext1.ext2" << "ext2";
    QTest::newRow("driveWithoutSlash") << "c:file1.ext1.ext2" << "ext2";
#endif
}

void tst_QFileInfo::suffix()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileInfo fi(file);
    QCOMPARE(fi.suffix(), expected);
}


void tst_QFileInfo::completeSuffix_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("noextension0") << "file" << "";
    QTest::newRow("noextension1") << "/path/to/file" << "";
    QTest::newRow("data0") << "file.tar" << "tar";
    QTest::newRow("data1") << "file.tar.gz" << "tar.gz";
    QTest::newRow("data2") << "/path/file/file.tar.gz" << "tar.gz";
    QTest::newRow("data3") << "/path/file.tar" << "tar";
    QTest::newRow("resource1") << ":/tst_qfileinfo/resources/file1.ext1" << "ext1";
    QTest::newRow("resource2") << ":/tst_qfileinfo/resources/file1.ext1.ext2" << "ext1.ext2";
#ifdef Q_OS_WIN
    QTest::newRow("driveWithSlash") << "c:/file1.ext1.ext2" << "ext1.ext2";
    QTest::newRow("driveWithoutSlash") << "c:file1.ext1.ext2" << "ext1.ext2";
#endif
}

void tst_QFileInfo::completeSuffix()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileInfo fi(file);
    QCOMPARE(fi.completeSuffix(), expected);
}

void tst_QFileInfo::baseName_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("data0") << "file.tar" << "file";
    QTest::newRow("data1") << "file.tar.gz" << "file";
    QTest::newRow("data2") << "/path/file/file.tar.gz" << "file";
    QTest::newRow("data3") << "/path/file.tar" << "file";
    QTest::newRow("data4") << "/path/file" << "file";
    QTest::newRow("resource1") << ":/tst_qfileinfo/resources/file1.ext1" << "file1";
    QTest::newRow("resource2") << ":/tst_qfileinfo/resources/file1.ext1.ext2" << "file1";
#ifdef Q_OS_WIN
    QTest::newRow("driveWithSlash") << "c:/file1.ext1.ext2" << "file1";
    QTest::newRow("driveWithoutSlash") << "c:file1.ext1.ext2" << "file1";
#endif
}

void tst_QFileInfo::baseName()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileInfo fi(file);
    QCOMPARE(fi.baseName(), expected);
}

void tst_QFileInfo::completeBaseName_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("data0") << "file.tar" << "file";
    QTest::newRow("data1") << "file.tar.gz" << "file.tar";
    QTest::newRow("data2") << "/path/file/file.tar.gz" << "file.tar";
    QTest::newRow("data3") << "/path/file.tar" << "file";
    QTest::newRow("data4") << "/path/file" << "file";
    QTest::newRow("resource1") << ":/tst_qfileinfo/resources/file1.ext1" << "file1";
    QTest::newRow("resource2") << ":/tst_qfileinfo/resources/file1.ext1.ext2" << "file1.ext1";
#ifdef Q_OS_WIN
    QTest::newRow("driveWithSlash") << "c:/file1.ext1.ext2" << "file1.ext1";
    QTest::newRow("driveWithoutSlash") << "c:file1.ext1.ext2" << "file1.ext1";
#endif
}

void tst_QFileInfo::completeBaseName()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileInfo fi(file);
    QCOMPARE(fi.completeBaseName(), expected);
}

void tst_QFileInfo::permission_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("perms");
    QTest::addColumn<bool>("expected");

    QTest::newRow("data0") << QCoreApplication::instance()->applicationFilePath() << int(QFile::ExeUser) << true;
    QTest::newRow("data1") << m_sourceFile << int(QFile::ReadUser) << true;
    QTest::newRow("resource1") << ":/tst_qfileinfo/resources/file1.ext1" << int(QFile::ReadUser) << true;
    QTest::newRow("resource2") << ":/tst_qfileinfo/resources/file1.ext1" << int(QFile::WriteUser) << false;
    QTest::newRow("resource3") << ":/tst_qfileinfo/resources/file1.ext1" << int(QFile::ExeUser) << false;
}

void tst_QFileInfo::permission()
{
    QFETCH(QString, file);
    QFETCH(int, perms);
    QFETCH(bool, expected);
    QFileInfo fi(file);
    QCOMPARE(fi.permission(QFile::Permissions(perms)), expected);
}

void tst_QFileInfo::size_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("size");

    QTest::newRow("resource1") << ":/tst_qfileinfo/resources/file1.ext1" << 0;
    QFile::remove("file1");
    QFile file("file1");
    QVERIFY(file.open(QFile::WriteOnly));
    QCOMPARE(file.write("JAJAJAA"), qint64(7));
    QTest::newRow("created-file") << "file1" << 7;

    QTest::newRow("resource2") << ":/tst_qfileinfo/resources/file1.ext1.ext2" << 0;
}

void tst_QFileInfo::size()
{
    QFETCH(QString, file);

    QFileInfo fi(file);
    (void)fi.permissions();
    QTEST(int(fi.size()), "size");
}

void tst_QFileInfo::systemFiles()
{
#if !defined(Q_OS_WIN) || defined(Q_OS_WINRT)
    QSKIP("This is a Windows only test");
#endif
    QFileInfo fi("c:\\pagefile.sys");
    QVERIFY2(fi.exists(), msgDoesNotExist(fi.absoluteFilePath()).constData());
    QVERIFY(fi.size() > 0);
    QVERIFY(fi.lastModified().isValid());
}

void tst_QFileInfo::compare_data()
{
    QTest::addColumn<QString>("file1");
    QTest::addColumn<QString>("file2");
    QTest::addColumn<bool>("same");

    QString caseChangedSource = m_sourceFile;
    caseChangedSource.replace("info", "Info");

    QTest::newRow("data0")
        << m_sourceFile
        << m_sourceFile
        << true;
    QTest::newRow("data1")
        << m_sourceFile
        << QString::fromLatin1("/tst_qfileinfo.cpp")
        << false;
    QTest::newRow("data2")
        << QString::fromLatin1("tst_qfileinfo.cpp")
        << QDir::currentPath() + QString::fromLatin1("/tst_qfileinfo.cpp")
        << true;
    QTest::newRow("casesense1")
        << caseChangedSource
        << m_sourceFile
#if defined(Q_OS_WIN)
        << true;
#elif defined(Q_OS_MAC)
        << !pathconf(QDir::currentPath().toLatin1().constData(), _PC_CASE_SENSITIVE);
#else
        << false;
#endif
}

void tst_QFileInfo::compare()
{
#if defined(Q_OS_MAC)
    if (qstrcmp(QTest::currentDataTag(), "casesense1") == 0)
        QSKIP("Qt thinks all UNIX filesystems are case sensitive, see QTBUG-28246");
#endif

    QFETCH(QString, file1);
    QFETCH(QString, file2);
    QFETCH(bool, same);
    QFileInfo fi1(file1), fi2(file2);
    QCOMPARE(fi1 == fi2, same);
}

void tst_QFileInfo::consistent_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

#if defined(Q_OS_WIN)
    QTest::newRow("slashes") << QString::fromLatin1("\\a\\a\\a\\a") << QString::fromLatin1("/a/a/a/a");
#endif
    QTest::newRow("ending slash") << QString::fromLatin1("/a/somedir/") << QString::fromLatin1("/a/somedir/");
    QTest::newRow("no ending slash") << QString::fromLatin1("/a/somedir") << QString::fromLatin1("/a/somedir");
}

void tst_QFileInfo::consistent()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    QFileInfo fi(file);
    QCOMPARE(fi.filePath(), expected);
    QCOMPARE(fi.dir().path() + QLatin1Char('/') + fi.fileName(), expected);
}


void tst_QFileInfo::fileTimes_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::newRow("simple") << QString::fromLatin1("simplefile.txt");
    QTest::newRow( "longfile" ) << QString::fromLatin1("longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName.txt");
    QTest::newRow( "longfile absolutepath" ) << QFileInfo(QString::fromLatin1("longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName.txt")).absoluteFilePath();
}

void tst_QFileInfo::fileTimes()
{
    int sleepTime = 2000;
    QFETCH(QString, fileName);
    if (QFile::exists(fileName)) {
        QVERIFY(QFile::remove(fileName));
    }
    QTest::qSleep(sleepTime);
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::WriteOnly | QFile::Text));
#if defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
        if (qIsLikelyToBeNfs(file.handle()))
            QSKIP("This Test doesn't work on NFS");
#endif
        QTextStream ts(&file);
        ts << fileName << endl;
    }
    QTest::qSleep(sleepTime);
    QDateTime beforeWrite = QDateTime::currentDateTime();
    QTest::qSleep(sleepTime);
    {
        QFileInfo fileInfo(fileName);
        QVERIFY(fileInfo.created() < beforeWrite);
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadWrite | QFile::Text));
        QTextStream ts(&file);
        ts << fileName << endl;
    }
    QTest::qSleep(sleepTime);
    QDateTime beforeRead = QDateTime::currentDateTime();
    QTest::qSleep(sleepTime);
    {
        QFileInfo fileInfo(fileName);
// On unix created() returns the same as lastModified().
#if !defined(Q_OS_UNIX)
        QVERIFY(fileInfo.created() < beforeWrite);
#endif
        QVERIFY(fileInfo.lastModified() > beforeWrite);
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        QTextStream ts(&file);
        QString line = ts.readLine();
        QCOMPARE(line, fileName);
    }

    QFileInfo fileInfo(fileName);
#if !defined(Q_OS_UNIX)
    QVERIFY(fileInfo.created() < beforeWrite);
#endif
    //In Vista the last-access timestamp is not updated when the file is accessed/touched (by default).
    //To enable this the HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\NtfsDisableLastAccessUpdate
    //is set to 0, in the test machine.
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    HKEY key;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\FileSystem",
        0, KEY_READ, &key)) {
            DWORD disabledAccessTimes = 0;
            DWORD size = sizeof(DWORD);
            LONG error = RegQueryValueEx(key, L"NtfsDisableLastAccessUpdate"
                , NULL, NULL, (LPBYTE)&disabledAccessTimes, &size);
            if (ERROR_SUCCESS == error && disabledAccessTimes)
                QEXPECT_FAIL("", "File access times are disabled in windows registry (this is the default setting)", Continue);
            RegCloseKey(key);
    }
#endif
#if defined(Q_OS_WINRT)
    QEXPECT_FAIL("", "WinRT does not allow timestamp handling change in the filesystem due to sandboxing", Continue);
#elif defined(Q_OS_QNX)
    QEXPECT_FAIL("", "QNX uses the noatime filesystem option", Continue);
#elif defined(Q_OS_ANDROID)
    if (fileInfo.lastRead() <= beforeRead)
        QEXPECT_FAIL("", "Android may use relatime or noatime on mounts", Continue);
#endif

    QVERIFY(fileInfo.lastRead() > beforeRead);
    QVERIFY(fileInfo.lastModified() > beforeWrite);
    QVERIFY(fileInfo.lastModified() < beforeRead);
}

void tst_QFileInfo::fileTimes_oldFile()
{
    // This is not supported on WinRT
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    // All files are opened in share mode (both read and write).
    DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    // All files on Windows can be read; there's no such thing as an
    // unreadable file. Add GENERIC_WRITE if WriteOnly is passed.
    int accessRights = GENERIC_READ | GENERIC_WRITE;

    SECURITY_ATTRIBUTES securityAtts = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    // Regular file mode. In Unbuffered mode, pass the no-buffering flag.
    DWORD flagsAndAtts = FILE_ATTRIBUTE_NORMAL;

    // WriteOnly can create files, ReadOnly cannot.
    DWORD creationDisp = OPEN_ALWAYS;

    // Create the file handle.
    HANDLE fileHandle = CreateFile(L"oldfile.txt",
        accessRights,
        shareMode,
        &securityAtts,
        creationDisp,
        flagsAndAtts,
        NULL);

    // Set file times back to 1601.
    SYSTEMTIME stime;
    stime.wYear = 1601;
    stime.wMonth = 1;
    stime.wDayOfWeek = 1;
    stime.wDay = 1;
    stime.wHour = 1;
    stime.wMinute = 0;
    stime.wSecond = 0;
    stime.wMilliseconds = 0;

    FILETIME ctime;
    QVERIFY(SystemTimeToFileTime(&stime, &ctime));
    FILETIME atime = ctime;
    FILETIME mtime = atime;
    QVERIFY(fileHandle);
    QVERIFY(SetFileTime(fileHandle, &ctime, &atime, &mtime) != 0);

    CloseHandle(fileHandle);

    QFileInfo info("oldfile.txt");
    QCOMPARE(info.lastModified(), QDateTime(QDate(1601, 1, 1), QTime(1, 0), Qt::UTC).toLocalTime());
#endif
}

void tst_QFileInfo::isSymLink_data()
{
#ifndef Q_NO_SYMLINKS
    QFile::remove("link.lnk");
    QFile::remove("brokenlink.lnk");
    QFile::remove("dummyfile");

    QFile file1(m_sourceFile);
    QVERIFY(file1.link("link.lnk"));

    QFile file2("dummyfile");
    file2.open(QIODevice::WriteOnly);
    QVERIFY(file2.link("brokenlink.lnk"));
    file2.remove();

    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isSymLink");
    QTest::addColumn<QString>("linkTarget");

    QTest::newRow("existent file") << m_sourceFile << false << "";
    QTest::newRow("link") << "link.lnk" << true << QFileInfo(m_sourceFile).absoluteFilePath();
    QTest::newRow("broken link") << "brokenlink.lnk" << true << QFileInfo("dummyfile").absoluteFilePath();
#endif
}

void tst_QFileInfo::isSymLink()
{
#ifdef Q_NO_SYMLINKS
    QSKIP("No symlink support", SkipAll);
#else
    QFETCH(QString, path);
    QFETCH(bool, isSymLink);
    QFETCH(QString, linkTarget);

    QFileInfo fi(path);
    QCOMPARE(fi.isSymLink(), isSymLink);
    QCOMPARE(fi.symLinkTarget(), linkTarget);
#endif
}

void tst_QFileInfo::isHidden_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isHidden");
    foreach (const QFileInfo& info, QDir::drives()) {
        QTest::newRow(qPrintable("drive." + info.path())) << info.path() << false;
    }

#if defined(Q_OS_WIN)
    QVERIFY(QDir("./hidden-directory").exists() || QDir().mkdir("./hidden-directory"));
    QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(QString("./hidden-directory").utf16()),FILE_ATTRIBUTE_HIDDEN));
    QTest::newRow("C:/path/to/hidden-directory") << QDir::currentPath() + QString::fromLatin1("/hidden-directory") << true;
    QTest::newRow("C:/path/to/hidden-directory/.") << QDir::currentPath() + QString::fromLatin1("/hidden-directory/.") << true;
#endif
#if defined(Q_OS_UNIX)
    QVERIFY(QDir("./.hidden-directory").exists() || QDir().mkdir("./.hidden-directory"));
    QTest::newRow("/path/to/.hidden-directory") << QDir::currentPath() + QString("/.hidden-directory") << true;
    QTest::newRow("/path/to/.hidden-directory/.") << QDir::currentPath() + QString("/.hidden-directory/.") << true;
    QTest::newRow("/path/to/.hidden-directory/..") << QDir::currentPath() + QString("/.hidden-directory/..") << true;
#endif

#if defined(Q_OS_MAC)
    // /bin has the hidden attribute on OS X
    QTest::newRow("/bin/") << QString::fromLatin1("/bin/") << true;
#elif !defined(Q_OS_WIN)
    QTest::newRow("/bin/") << QString::fromLatin1("/bin/") << false;
#endif

#ifdef Q_OS_MAC
    QTest::newRow("mac_etc") << QString::fromLatin1("/etc") << true;
    QTest::newRow("mac_private_etc") << QString::fromLatin1("/private/etc") << false;
    QTest::newRow("mac_Applications") << QString::fromLatin1("/Applications") << false;
#endif
}

void tst_QFileInfo::isHidden()
{
    QFETCH(QString, path);
    QFETCH(bool, isHidden);
    QFileInfo fi(path);

    QCOMPARE(fi.isHidden(), isHidden);
}

#if defined(Q_OS_MAC)
void tst_QFileInfo::isHiddenFromFinder()
{
    const char *filename = "test_foobar.txt";

    QFile testFile(filename);
    testFile.open(QIODevice::WriteOnly | QIODevice::Append);
    testFile.write(QByteArray("world"));
    testFile.close();

    struct stat buf;
    stat(filename, &buf);
    chflags(filename, buf.st_flags | UF_HIDDEN);

    QFileInfo fi(filename);
    QCOMPARE(fi.isHidden(), true);

    testFile.remove();
}
#endif

void tst_QFileInfo::isBundle_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isBundle");
    QTest::newRow("root") << QString::fromLatin1("/") << false;
#ifdef Q_OS_MAC
    QTest::newRow("mac_Applications") << QString::fromLatin1("/Applications") << false;
    QTest::newRow("mac_Applications") << QString::fromLatin1("/Applications/Safari.app") << true;
#endif
}

void tst_QFileInfo::isBundle()
{
    QFETCH(QString, path);
    QFETCH(bool, isBundle);
    QFileInfo fi(path);
    QCOMPARE(fi.isBundle(), isBundle);
}

void tst_QFileInfo::isNativePath_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isNativePath");

    QTest::newRow("default-constructed") << QString() << false;
    QTest::newRow("empty") << QString("") << true;

    QTest::newRow("local root") << QString::fromLatin1("/") << true;
    QTest::newRow("local non-existent file") << QString::fromLatin1("/abrakadabra.boo") << true;

    QTest::newRow("qresource root") << QString::fromLatin1(":/") << false;
}

void tst_QFileInfo::isNativePath()
{
    QFETCH(QString, path);
    QFETCH(bool, isNativePath);

    QFileInfo info(path);
    if (path.isNull())
        info = QFileInfo();
    QCOMPARE(info.isNativePath(), isNativePath);
}

void tst_QFileInfo::refresh()
{
#if defined(Q_OS_WIN)
    int sleepTime = 3000;
#else
    int sleepTime = 2000;
#endif

    QFile::remove("file1");
    QFile file("file1");
    QVERIFY(file.open(QFile::WriteOnly));
    QCOMPARE(file.write("JAJAJAA"), qint64(7));
    file.flush();

    QFileInfo info(file);
    QDateTime lastModified = info.lastModified();
    QCOMPARE(info.size(), qint64(7));

    QTest::qSleep(sleepTime);

    QCOMPARE(file.write("JOJOJO"), qint64(6));
    file.flush();
    QCOMPARE(info.lastModified(), lastModified);

    QCOMPARE(info.size(), qint64(7));
#if defined(Q_OS_WIN)
    file.close();
#endif
    info.refresh();
    QCOMPARE(info.size(), qint64(13));
    QVERIFY(info.lastModified() > lastModified);

    QFileInfo info2 = info;
    QCOMPARE(info2.size(), info.size());

    info2.refresh();
    QCOMPARE(info2.size(), info.size());
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
void tst_QFileInfo::ntfsJunctionPointsAndSymlinks_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isSymLink");
    QTest::addColumn<QString>("linkTarget");
    QTest::addColumn<QString>("canonicalFilePath");

    QDir pwd;
    pwd.mkdir("target");

    QLibrary kernel32("kernel32");
    typedef BOOLEAN (WINAPI *PtrCreateSymbolicLink)(LPCWSTR, LPCWSTR, DWORD);
    PtrCreateSymbolicLink createSymbolicLinkW = 0;
    createSymbolicLinkW = (PtrCreateSymbolicLink) kernel32.resolve("CreateSymbolicLinkW");
    if (!createSymbolicLinkW) {
        //we need at least one data set for the test not to fail when skipping _data function
        QDir target("target");
        QTest::newRow("dummy") << target.path() << false << "" << target.canonicalPath();
        QSKIP("symbolic links not supported by operating system");
    }
    {
        //Directory symlinks
        QDir target("target");
        QVERIFY(target.exists());

        QString absTarget = QDir::toNativeSeparators(target.absolutePath());
        QString absSymlink = QDir::toNativeSeparators(pwd.absolutePath()).append("\\abs_symlink");
        QString relTarget = "target";
        QString relSymlink = "rel_symlink";
        QString fileInTarget(absTarget);
        fileInTarget.append("\\file");
        QString fileInSymlink(absSymlink);
        fileInSymlink.append("\\file");
        QFile file(fileInTarget);
        file.open(QIODevice::ReadWrite);
        file.close();

        DWORD err = ERROR_SUCCESS ;
        if (!pwd.exists("abs_symlink"))
            if (!createSymbolicLinkW((wchar_t*)absSymlink.utf16(),(wchar_t*)absTarget.utf16(),0x1))
                err = GetLastError();
        if (err == ERROR_SUCCESS && !pwd.exists(relSymlink))
            if (!createSymbolicLinkW((wchar_t*)relSymlink.utf16(),(wchar_t*)relTarget.utf16(),0x1))
                err = GetLastError();
        if (err != ERROR_SUCCESS) {
            wchar_t errstr[0x100];
            DWORD count = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                0, err, 0, errstr, 0x100, 0);
            QString error(QString::fromWCharArray(errstr, count));
            qWarning() << error;
            //we need at least one data set for the test not to assert fail when skipping _data function
            QDir target("target");
            QTest::newRow("dummy") << target.path() << false << "" << target.canonicalPath();
            QSKIP("link not supported by FS or insufficient privilege");
        }
        QVERIFY2(file.exists(), msgDoesNotExist(file.fileName()).constData());

        QTest::newRow("absolute dir symlink") << absSymlink << true << QDir::fromNativeSeparators(absTarget) << target.canonicalPath();
        QTest::newRow("relative dir symlink") << relSymlink << true << QDir::fromNativeSeparators(relTarget) << target.canonicalPath();
        QTest::newRow("file in symlink dir") << fileInSymlink << false << "" << target.canonicalPath().append("/file");
    }
    {
        //File symlinks
        QFileInfo target(m_sourceFile);
        QString absTarget = QDir::toNativeSeparators(target.absoluteFilePath());
        QString absSymlink = QDir::toNativeSeparators(pwd.absolutePath()).append("\\abs_symlink.cpp");
        QString relTarget = QDir::toNativeSeparators(pwd.relativeFilePath(target.absoluteFilePath()));
        QString relSymlink = "rel_symlink.cpp";
        QVERIFY(pwd.exists("abs_symlink.cpp") || createSymbolicLinkW((wchar_t*)absSymlink.utf16(),(wchar_t*)absTarget.utf16(),0x0));
        QVERIFY(pwd.exists(relSymlink) || createSymbolicLinkW((wchar_t*)relSymlink.utf16(),(wchar_t*)relTarget.utf16(),0x0));

        QTest::newRow("absolute file symlink") << absSymlink << true << QDir::fromNativeSeparators(absTarget) << target.canonicalFilePath();
        QTest::newRow("relative file symlink") << relSymlink << true << QDir::fromNativeSeparators(relTarget) << target.canonicalFilePath();
    }

    //Junctions
    QString target = "target";
    QString junction = "junction_pwd";
    FileSystem::createNtfsJunction(target, junction);
    QFileInfo targetInfo(target);
    QTest::newRow("junction_pwd") << junction << false << QString() << QString();

    QFileInfo fileInJunction(targetInfo.absoluteFilePath().append("/file"));
    QFile file(fileInJunction.absoluteFilePath());
    file.open(QIODevice::ReadWrite);
    file.close();
    QVERIFY2(file.exists(), msgDoesNotExist(file.fileName()).constData());
    QTest::newRow("file in junction") << fileInJunction.absoluteFilePath() << false << "" << fileInJunction.canonicalFilePath();

    target = QDir::rootPath();
    junction = "junction_root";
    FileSystem::createNtfsJunction(target, junction);
    targetInfo.setFile(target);
    QTest::newRow("junction_root") << junction << false << QString() << QString();

    //Mountpoint
    typedef BOOLEAN (WINAPI *PtrGetVolumeNameForVolumeMountPointW)(LPCWSTR, LPWSTR, DWORD);
    PtrGetVolumeNameForVolumeMountPointW getVolumeNameForVolumeMountPointW = 0;
    getVolumeNameForVolumeMountPointW = (PtrGetVolumeNameForVolumeMountPointW) kernel32.resolve("GetVolumeNameForVolumeMountPointW");
    if(getVolumeNameForVolumeMountPointW)
    {
        wchar_t buffer[MAX_PATH];
        QString rootPath = QDir::toNativeSeparators(QDir::rootPath());
        QVERIFY(getVolumeNameForVolumeMountPointW((wchar_t*)rootPath.utf16(), buffer, MAX_PATH));
        QString rootVolume = QString::fromWCharArray(buffer);
        junction = "mountpoint";
        rootVolume.replace("\\\\?\\","\\??\\");
        FileSystem::createNtfsJunction(rootVolume, junction);
        QTest::newRow("mountpoint") << junction << false << QString() << QString();
    }
}

void tst_QFileInfo::ntfsJunctionPointsAndSymlinks()
{
    QFETCH(QString, path);
    QFETCH(bool, isSymLink);
    QFETCH(QString, linkTarget);
    QFETCH(QString, canonicalFilePath);

    QFileInfo fi(path);
    const bool actualIsSymLink = fi.isSymLink();
    const QString actualSymLinkTarget = isSymLink ? fi.symLinkTarget() : QString();
    const QString actualCanonicalFilePath = isSymLink ? fi.canonicalFilePath() : QString();
    // Ensure that junctions, mountpoints are removed. If this fails, do not remove
    // temporary directory to prevent it from trashing the system.
    if (fi.isDir()) {
        if (!QDir().rmdir(fi.fileName())) {
            qWarning("Unable to remove NTFS junction '%s'', keeping '%s'.",
                     qPrintable(fi.fileName()), qPrintable(QDir::toNativeSeparators(m_dir.path())));
            m_dir.setAutoRemove(false);
        }
    }
    QCOMPARE(actualIsSymLink, isSymLink);
    if (isSymLink) {
        QCOMPARE(actualSymLinkTarget, linkTarget);
        QCOMPARE(actualCanonicalFilePath, canonicalFilePath);
    }
}

void tst_QFileInfo::brokenShortcut()
{
    QString linkName("borkenlink.lnk");
    QFile::remove(linkName);
    QFile file(linkName);
    file.open(QFile::WriteOnly);
    file.write("b0rk");
    file.close();

    QFileInfo info(linkName);
    QVERIFY(info.isSymLink());
    QVERIFY(!info.exists());
    QFile::remove(linkName);

    QDir current; // QTBUG-21863
    QVERIFY(current.mkdir(linkName));
    QFileInfo dirInfo(linkName);
    QVERIFY(!dirInfo.isSymLink());
    QVERIFY(dirInfo.isDir());
    current.rmdir(linkName);
}
#endif

void tst_QFileInfo::isWritable()
{
    QFile tempfile("tempfile.txt");
    tempfile.open(QIODevice::WriteOnly);
    tempfile.write("This file is generated by the QFileInfo autotest.");
    tempfile.close();

    QVERIFY(QFileInfo("tempfile.txt").isWritable());
    tempfile.remove();

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    QFileInfo fi("c:\\pagefile.sys");
    QVERIFY2(fi.exists(), msgDoesNotExist(fi.absoluteFilePath()).constData());
    QVERIFY(!fi.isWritable());
#endif
#if defined (Q_OS_QNX) // On QNX /etc is usually on a read-only filesystem
    QVERIFY(!QFileInfo("/etc/passwd").isWritable());
#elif defined (Q_OS_UNIX) && !defined(Q_OS_VXWORKS) // VxWorks does not have users/groups
    if (::getuid() == 0)
        QVERIFY(QFileInfo("/etc/passwd").isWritable());
    else
        QVERIFY(!QFileInfo("/etc/passwd").isWritable());
#endif
}

void tst_QFileInfo::isExecutable()
{
    QString appPath = QCoreApplication::applicationDirPath();
#if defined(Q_OS_ANDROID)
    appPath += "/libtst_qfileinfo.so";
#else
    appPath += "/tst_qfileinfo";
# if defined(Q_OS_WIN)
    appPath += ".exe";
# endif
#endif
    QFileInfo fi(appPath);
    QCOMPARE(fi.isExecutable(), true);

    QCOMPARE(QFileInfo(m_proFile).isExecutable(), false);

#ifdef Q_OS_UNIX
    QFile::remove("link.lnk");

    // Symlink to executable
    QFile appFile(appPath);
    QVERIFY(appFile.link("link.lnk"));
    QCOMPARE(QFileInfo("link.lnk").isExecutable(), true);
    QFile::remove("link.lnk");

    // Symlink to .pro file
    QFile proFile(m_proFile);
    QVERIFY(proFile.link("link.lnk"));
    QCOMPARE(QFileInfo("link.lnk").isExecutable(), false);
    QFile::remove("link.lnk");
#endif

}


void tst_QFileInfo::testDecomposedUnicodeNames_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<bool>("exists");
    QString currPath = QDir::currentPath();
    QTest::newRow("latin-only") << currPath + "/4.pdf" << "4.pdf" << true;
    QTest::newRow("one-decomposed uni") << currPath + QString::fromUtf8("/4 ä.pdf") << QString::fromUtf8("4 ä.pdf") << true;
    QTest::newRow("many-decomposed uni") << currPath + QString::fromUtf8("/4 äääcopy.pdf") << QString::fromUtf8("4 äääcopy.pdf") << true;
    QTest::newRow("no decomposed") << currPath + QString::fromUtf8("/4 øøøcopy.pdf") << QString::fromUtf8("4 øøøcopy.pdf") << true;
}

// This is a helper class that ensures that files created during the test
// will be removed afterwards, even if the test fails or throws an exception.
class NativeFileCreator
{
public:
    NativeFileCreator(const QString &filePath)
        : m_filePath(filePath), m_error(0)
    {
#ifdef Q_OS_UNIX
        int fd = open(m_filePath.normalized(QString::NormalizationForm_D).toUtf8().constData(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
        if (fd >= 0)
            close(fd);
        else
            m_error = errno;
#endif
    }
    ~NativeFileCreator()
    {
#ifdef Q_OS_UNIX
        if (m_error == 0)
            unlink(m_filePath.normalized(QString::NormalizationForm_D).toUtf8().constData());
#endif
    }
    int error() const
    {
        return m_error;
    }

private:
    QString m_filePath;
    int m_error;
};

void tst_QFileInfo::testDecomposedUnicodeNames()
{
#ifndef Q_OS_MAC
    QSKIP("This is a OS X only test (unless you know more about filesystems, then maybe you should try it ;)");
#else
    QFETCH(QString, filePath);
    NativeFileCreator nativeFileCreator(filePath);
    int error = nativeFileCreator.error();
    QVERIFY2(error == 0, qPrintable(QString("Couldn't create native file %1: %2").arg(filePath).arg(strerror(error))));

    QFileInfo file(filePath);
    QTEST(file.fileName(), "fileName");
    QTEST(file.exists(), "exists");
#endif
}

void tst_QFileInfo::equalOperator() const
{
    /* Compare two default constructed values. Yes, to me it seems it should be the opposite too, but
     * this is how the code was written. */
    QVERIFY(!(QFileInfo() == QFileInfo()));
}


void tst_QFileInfo::equalOperatorWithDifferentSlashes() const
{
    const QFileInfo fi1("/usr");
    const QFileInfo fi2("/usr/");

    QCOMPARE(fi1, fi2);
}

void tst_QFileInfo::notEqualOperator() const
{
    /* Compare two default constructed values. Yes, to me it seems it should be the opposite too, but
     * this is how the code was written. */
    QVERIFY(QFileInfo() != QFileInfo());
}

void tst_QFileInfo::detachingOperations()
{
    QFileInfo info1;
    QVERIFY(info1.caching());
    info1.setCaching(false);

    {
        QFileInfo info2 = info1;

        QVERIFY(!info1.caching());
        QVERIFY(!info2.caching());

        info2.setCaching(true);
        QVERIFY(info2.caching());

        info1.setFile("foo");
        QVERIFY(!info1.caching());
    }

    {
        QFile file("foo");
        info1.setFile(file);
        QVERIFY(!info1.caching());
    }

    info1.setFile(QDir(), "foo");
    QVERIFY(!info1.caching());

    {
        QFileInfo info3;
        QVERIFY(info3.caching());

        info3 = info1;
        QVERIFY(!info3.caching());
    }

    info1.refresh();
    QVERIFY(!info1.caching());

    QVERIFY(info1.makeAbsolute());
    QVERIFY(!info1.caching());
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
BOOL IsUserAdmin()
{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    b = AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup);
    if (b) {
        if (!CheckTokenMembership( NULL, AdministratorsGroup, &b))
            b = false;
        FreeSid(AdministratorsGroup);
    }

    return(b);
}

QT_BEGIN_NAMESPACE
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
QT_END_NAMESPACE

#endif // Q_OS_WIN && !Q_OS_WINRT

#ifndef Q_OS_WINRT
void tst_QFileInfo::owner()
{
    QString userName;
#if defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
    {
        passwd *user = getpwuid(geteuid());
        QVERIFY(user);
        char *usernameBuf = user->pw_name;
        userName = QString::fromLocal8Bit(usernameBuf);
    }
#endif
#if defined(Q_OS_WIN)
    wchar_t  usernameBuf[1024];
    DWORD  bufSize = 1024;
    if (GetUserNameW(usernameBuf, &bufSize)) {
        userName = QString::fromWCharArray(usernameBuf);
        if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA && IsUserAdmin()) {
            // Special case : If the user is a member of Administrators group, all files
            // created by the current user are owned by the Administrators group.
            LPLOCALGROUP_USERS_INFO_0 pBuf = NULL;
            DWORD dwLevel = 0;
            DWORD dwFlags = LG_INCLUDE_INDIRECT ;
            DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
            DWORD dwEntriesRead = 0;
            DWORD dwTotalEntries = 0;
            NET_API_STATUS nStatus;
            nStatus = NetUserGetLocalGroups(0, usernameBuf, dwLevel, dwFlags, (LPBYTE *) &pBuf,
                                            dwPrefMaxLen, &dwEntriesRead, &dwTotalEntries);
            // Check if the current user is a member of Administrators group
            if (nStatus == NERR_Success && pBuf){
                for (int i = 0; i < (int)dwEntriesRead; i++) {
                    QString groupName = QString::fromWCharArray(pBuf[i].lgrui0_name);
                    if (!groupName.compare(QLatin1String("Administrators")))
                        userName = groupName;
                }
            }
            if (pBuf != NULL)
                NetApiBufferFree(pBuf);
        }
    }
    qt_ntfs_permission_lookup = 1;
#endif
    if (userName.isEmpty())
        QSKIP("Can't retrieve the user name");
    QString fileName("ownertest.txt");
    QVERIFY(!QFile::exists(fileName) || QFile::remove(fileName));
    {
        QFile testFile(fileName);
        QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Text));
        QByteArray testData("testfile");
        QVERIFY(testFile.write(testData) != -1);
    }
    QFileInfo fi(fileName);
    QVERIFY2(fi.exists(), msgDoesNotExist(fi.absoluteFilePath()).constData());
    QCOMPARE(fi.owner(), userName);

    QFile::remove(fileName);
#if defined(Q_OS_WIN)
    qt_ntfs_permission_lookup = 0;
#endif
}
#endif // !Q_OS_WINRT

void tst_QFileInfo::group()
{
    QString expected;
#if defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
    struct group *gr;
    gid_t gid = getegid();

    errno = 0;
    gr = getgrgid(gid);

    QVERIFY2(gr, qPrintable(
        QString("getgrgid returned 0: %1, cannot determine my own group")
        .arg(QString::fromLocal8Bit(strerror(errno)))));
    expected = QString::fromLocal8Bit(gr->gr_name);
#endif

    QString fileName("ownertest.txt");
    if (QFile::exists(fileName))
        QFile::remove(fileName);
    QFile testFile(fileName);
    QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QByteArray testData("testfile");
    QVERIFY(testFile.write(testData) != -1);
    testFile.close();
    QFileInfo fi(fileName);
    QVERIFY2(fi.exists(), msgDoesNotExist(fi.absoluteFilePath()).constData());

    QCOMPARE(fi.group(), expected);
}

void tst_QFileInfo::invalidState()
{
    // Shouldn't crash;

    {
        QFileInfo info;
        QCOMPARE(info.size(), qint64(0));
        QVERIFY(!info.exists());

        info.setCaching(false);

        info.created();
        info.lastRead();
        info.lastModified();
    }

    {
        QFileInfo info("");
        QCOMPARE(info.size(), qint64(0));
        QVERIFY(!info.exists());

        info.setCaching(false);

        info.created();
        info.lastRead();
        info.lastModified();
    }

    {
        QFileInfo info("file-doesn't-really-exist.txt");
        QCOMPARE(info.size(), qint64(0));
        QVERIFY(!info.exists());

        info.setCaching(false);

        info.created();
        info.lastRead();
        info.lastModified();
    }

    QVERIFY(true);
}

void tst_QFileInfo::nonExistingFileDates()
{
    QFileInfo info("non-existing-file.foobar");
    QVERIFY(!info.exists());
    QVERIFY(!info.created().isValid());
    QVERIFY(!info.lastRead().isValid());
    QVERIFY(!info.lastModified().isValid());
}

QTEST_MAIN(tst_QFileInfo)
#include "tst_qfileinfo.moc"
