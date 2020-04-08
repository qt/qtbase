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
#include <qtemporaryfile.h>
#include <qtemporarydir.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qstorageinfo.h>
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
#if !defined(Q_OS_WINRT)
#include <private/qwinregistry_p.h>
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

#if defined(Q_OS_WIN)
QT_BEGIN_NAMESPACE
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
QT_END_NAMESPACE
#  ifndef Q_OS_WINRT
bool IsUserAdmin();
#  endif
#endif

inline bool qIsLikelyToBeFat(const QString &path)
{
    QByteArray name = QStorageInfo(path).fileSystemType().toLower();
    return name.contains("fat") || name.contains("msdos");
}

inline bool qIsLikelyToBeNfs(const QString &path)
{
#ifdef Q_OS_WIN
    Q_UNUSED(path);
    return false;
#else
    QByteArray type = QStorageInfo(path).fileSystemType();
    const char *name = type.constData();

    return (qstrncmp(name, "nfs", 3) == 0
            || qstrncmp(name, "autofs", 6) == 0
            || qstrncmp(name, "autofsng", 8) == 0
            || qstrncmp(name, "cachefs", 7) == 0);
#endif
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
#  ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE // MinGW
#    define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE (0x2)
#  endif

static DWORD createSymbolicLink(const QString &symLinkName, const QString &target,
                                QString *errorMessage)
{
    DWORD result = ERROR_SUCCESS;
    const QString nativeSymLinkName = QDir::toNativeSeparators(symLinkName);
    const QString nativeTarget = QDir::toNativeSeparators(target);
    DWORD flags = 0;
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 14972))
        flags |= SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
    if (QFileInfo(target).isDir())
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    if (CreateSymbolicLink(reinterpret_cast<const wchar_t*>(nativeSymLinkName.utf16()),
                           reinterpret_cast<const wchar_t*>(nativeTarget.utf16()), flags) == FALSE) {
        result = GetLastError();
        QTextStream(errorMessage) << "CreateSymbolicLink(" <<  nativeSymLinkName << ", "
            << nativeTarget << ", 0x" << hex << flags << dec << ") failed with error " << result
            << ": " << qt_error_string(int(result));
    }
    return result;
}

static QByteArray msgInsufficientPrivileges(const QString &errorMessage)
{
    return "Insufficient privileges (" + errorMessage.toLocal8Bit() + ')';
}
#endif  // Q_OS_WIN && !Q_OS_WINRT

static QString seedAndTemplate()
{
    QString base;
#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID)
    // use XDG_RUNTIME_DIR as it's a fully-capable FS
    base = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
#endif
    if (base.isEmpty())
        base = QDir::tempPath();
    return base + "/tst_qfileinfo-XXXXXX";
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
    void fakeFileTimes_data();
    void fakeFileTimes();

    void isSymLink_data();
    void isSymLink();

    void isSymbolicLink_data();
    void isSymbolicLink();

    void isShortcut_data();
    void isShortcut();

    void link_data();
    void link();

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

    void invalidState_data();
    void invalidState();
    void nonExistingFile();

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
    fi = QFileInfo(tempFile.fileName() + QString::fromLatin1("/"));
    QCOMPARE(fi.canonicalFilePath(), QString::fromLatin1(""));
    tempFile.remove();

    // This used to crash on Mac, verify that it doesn't anymore.
    QFileInfo info("/tmp/../../../../../../../../../../../../../../../../../");
    info.canonicalFilePath();

#if defined(Q_OS_UNIX)
    // If this file exists, you can't log in to run this test ...
    const QString notExtantPath(QStringLiteral("/etc/nologin"));
    QFileInfo notExtant(notExtantPath);
    QCOMPARE(notExtant.canonicalFilePath(), QString());

    // A path with a non-directory as a directory component also doesn't exist:
    const QString badDirPath(QStringLiteral("/dev/null/sub/dir/n'existe.pas"));
    QFileInfo badDir(badDirPath);
    QCOMPARE(badDir.canonicalFilePath(), QString());

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
    {
        QString errorMessage;
        const QString linkTarget = QStringLiteral("res");
        const DWORD dwErr = createSymbolicLink(linkTarget, m_resourcesDir, &errorMessage);
        if (dwErr == ERROR_PRIVILEGE_NOT_HELD)
            QSKIP(msgInsufficientPrivileges(errorMessage));
        QVERIFY2(dwErr == ERROR_SUCCESS, qPrintable(errorMessage));
        QString currentPath = QDir::currentPath();
        QVERIFY(QDir::setCurrent(linkTarget));
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
    QVERIFY(fi.metadataChangeTime().isValid());
    QCOMPARE(fi.metadataChangeTime(), fi.lastModified());   // On Windows, they're the same
    QVERIFY(fi.birthTime().isValid());
    QVERIFY(fi.birthTime() <= fi.lastModified());
#if QT_DEPRECATED_SINCE(5, 10)
    QCOMPARE(fi.created(), fi.birthTime());                 // On Windows, they're the same
#endif
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
    auto datePairString = [](const QDateTime &actual, const QDateTime &before) {
        return (actual.toString(Qt::ISODateWithMs) + " (should be >) " + before.toString(Qt::ISODateWithMs))
                .toLatin1();
    };

    QFETCH(QString, fileName);
    int sleepTime = 100;

    // on Linux and Windows, the filesystem timestamps may be slightly out of
    // sync with the system clock (maybe they're using CLOCK_REALTIME_COARSE),
    // so add a margin of error to our comparisons
    int fsClockSkew = 10;
#ifdef Q_OS_WIN
    fsClockSkew = 500;
#endif

    // NFS clocks may be WAY out of sync
    if (qIsLikelyToBeNfs(fileName))
        QSKIP("This test doesn't work on NFS");

    bool noAccessTime = false;
    {
        // try to guess if file times on this filesystem round to the second
        QFileInfo cwd(".");
        if (cwd.lastModified().toMSecsSinceEpoch() % 1000 == 0
                && cwd.lastRead().toMSecsSinceEpoch() % 1000 == 0) {
            fsClockSkew = sleepTime = 1000;

            noAccessTime = qIsLikelyToBeFat(fileName);
            if (noAccessTime) {
                // FAT filesystems (but maybe not exFAT) store timestamps with 2-second
                // granularity and access time with 1-day granularity
                fsClockSkew = sleepTime = 2000;
            }
        }
    }

    if (QFile::exists(fileName)) {
        QVERIFY(QFile::remove(fileName));
    }

    QDateTime beforeBirth, beforeWrite, beforeMetadataChange, beforeRead;
    QDateTime birthTime, writeTime, metadataChangeTime, readTime;

    // --- Create file and write to it
    beforeBirth = QDateTime::currentDateTime().addMSecs(-fsClockSkew);
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::WriteOnly | QFile::Text));
        QFileInfo fileInfo(fileName);
        birthTime = fileInfo.birthTime();
        QVERIFY2(!birthTime.isValid() || birthTime > beforeBirth,
                 datePairString(birthTime, beforeBirth));

        QTest::qSleep(sleepTime);
        beforeWrite = QDateTime::currentDateTime().addMSecs(-fsClockSkew);
        QTextStream ts(&file);
        ts << fileName << Qt::endl;
    }
    {
        QFileInfo fileInfo(fileName);
        writeTime = fileInfo.lastModified();
        QVERIFY2(writeTime > beforeWrite, datePairString(writeTime, beforeWrite));
        QCOMPARE(fileInfo.birthTime(), birthTime); // mustn't have changed
    }

    // --- Change the file's metadata
    QTest::qSleep(sleepTime);
    beforeMetadataChange = QDateTime::currentDateTime().addMSecs(-fsClockSkew);
    {
        QFile file(fileName);
        file.setPermissions(file.permissions());
    }
    {
        QFileInfo fileInfo(fileName);
        metadataChangeTime = fileInfo.metadataChangeTime();
        QVERIFY2(metadataChangeTime > beforeMetadataChange,
                 datePairString(metadataChangeTime, beforeMetadataChange));
        QVERIFY(metadataChangeTime >= writeTime); // not all filesystems can store both times
        QCOMPARE(fileInfo.birthTime(), birthTime); // mustn't have changed
    }

    // --- Read the file
    QTest::qSleep(sleepTime);
    beforeRead = QDateTime::currentDateTime().addMSecs(-fsClockSkew);
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        QTextStream ts(&file);
        QString line = ts.readLine();
        QCOMPARE(line, fileName);
    }

    QFileInfo fileInfo(fileName);
    readTime = fileInfo.lastRead();
    QCOMPARE(fileInfo.lastModified(), writeTime); // mustn't have changed
    QCOMPARE(fileInfo.birthTime(), birthTime); // mustn't have changed
    QVERIFY(readTime.isValid());

#if defined(Q_OS_WINRT) || defined(Q_OS_QNX) || (defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED))
    noAccessTime = true;
#elif defined(Q_OS_WIN)
    //In Vista the last-access timestamp is not updated when the file is accessed/touched (by default).
    //To enable this the HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\NtfsDisableLastAccessUpdate
    //is set to 0, in the test machine.
    const auto disabledAccessTimes =
        QWinRegistryKey(HKEY_LOCAL_MACHINE,
                        LR"(SYSTEM\CurrentControlSet\Control\FileSystem)")
        .dwordValue(L"NtfsDisableLastAccessUpdate");
    if (disabledAccessTimes.second && disabledAccessTimes.first != 0)
        noAccessTime = true;
#endif

    if (noAccessTime)
        return;

    QVERIFY2(readTime > beforeRead, datePairString(readTime, beforeRead));
    QVERIFY(writeTime < beforeRead);
}

void tst_QFileInfo::fakeFileTimes_data()
{
    QTest::addColumn<QDateTime>("when");

    // This is 2^{31} seconds before 1970-01-01 15:14:8,
    // i.e. shortly after the start of time_t, in any time-zone:
    QTest::newRow("early") << QDateTime(QDate(1901, 12, 14), QTime(12, 0));

    // QTBUG-12006 claims XP handled this (2010-Mar-26 8:46:10) wrong due to an MS API bug:
    QTest::newRow("XP-bug") << QDateTime::fromSecsSinceEpoch(1269593170);
}

void tst_QFileInfo::fakeFileTimes()
{
    QFETCH(QDateTime, when);

    QFile file("faketimefile.txt");
    file.open(QIODevice::WriteOnly);
    file.write("\n", 1);
    file.close();

    /*
      QFile's setFileTime calls QFSFileEngine::setFileTime() which fails unless
      the file is open at the time.  Of course, when writing, close() changes
      modification time, so need to re-open for read in order to setFileTime().
     */
    file.open(QIODevice::ReadOnly);
    bool ok = file.setFileTime(when, QFileDevice::FileModificationTime);
    file.close();

    if (ok)
        QCOMPARE(QFileInfo(file.fileName()).lastModified(), when);
    else
        QSKIP("Unable to set file metadata to contrived values");
}

void tst_QFileInfo::isSymLink_data()
{
#ifndef Q_NO_SYMLINKS
    QFile::remove("link.lnk");
    QFile::remove("brokenlink.lnk");
    QFile::remove("dummyfile");
    QFile::remove("relative/link.lnk");

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

#ifndef Q_OS_WIN
    QDir::current().mkdir("relative");
    QFile::link("../dummyfile", "relative/link.lnk");
    QTest::newRow("relative link") << "relative/link.lnk" << true << QFileInfo("dummyfile").absoluteFilePath();
#endif
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

void tst_QFileInfo::isShortcut_data()
{
    QFile::remove("link.lnk");
    QFile::remove("symlink.lnk");
    QFile::remove("link");
    QFile::remove("symlink");
    QFile::remove("directory.lnk");
    QFile::remove("directory");

    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isShortcut");

    QFile regularFile(m_sourceFile);
    QTest::newRow("regular")
        << regularFile.fileName() << false;
    QTest::newRow("directory")
        << QDir::currentPath() << false;
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    // windows shortcuts
    QVERIFY(regularFile.link("link.lnk"));
    QTest::newRow("shortcut")
        << "link.lnk" << true;
    QVERIFY(regularFile.link("link"));
    QTest::newRow("invalid-shortcut")
        << "link" << false;
    QVERIFY(QFile::link(QDir::currentPath(), "directory.lnk"));
    QTest::newRow("directory-shortcut")
        << "directory.lnk" << true;
#endif
}

void tst_QFileInfo::isShortcut()
{
    QFETCH(QString, path);
    QFETCH(bool, isShortcut);

    QFileInfo fi(path);
    QCOMPARE(fi.isShortcut(), isShortcut);
}

void tst_QFileInfo::isSymbolicLink_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isSymbolicLink");

    QFile regularFile(m_sourceFile);
    QTest::newRow("regular")
        << regularFile.fileName() << false;
    QTest::newRow("directory")
        << QDir::currentPath() << false;

#ifndef Q_NO_SYMLINKS
#if defined(Q_OS_WIN)
#if !defined(Q_OS_WINRT)
    QString errorMessage;
    const DWORD creationResult = createSymbolicLink("symlink", m_sourceFile, &errorMessage);
    if (creationResult == ERROR_PRIVILEGE_NOT_HELD) {
        QWARN(msgInsufficientPrivileges(errorMessage));
    } else {
        QVERIFY2(creationResult == ERROR_SUCCESS, qPrintable(errorMessage));
        QTest::newRow("NTFS-symlink")
            << "symlink" << true;
    }
#endif // !Q_OS_WINRT
#else // Unix:
    QVERIFY(regularFile.link("symlink.lnk"));
    QTest::newRow("symlink.lnk")
        << "symlink.lnk" << true;
    QVERIFY(regularFile.link("symlink"));
    QTest::newRow("symlink")
        << "symlink" << true;
    QVERIFY(QFile::link(QDir::currentPath(), "directory"));
    QTest::newRow("directory-symlink")
        << "directory" << true;
#endif
#endif // !Q_NO_SYMLINKS
}

void tst_QFileInfo::isSymbolicLink()
{
    QFETCH(QString, path);
    QFETCH(bool, isSymbolicLink);

    QFileInfo fi(path);
    QCOMPARE(fi.isSymbolicLink(), isSymbolicLink);
}

void tst_QFileInfo::link_data()
{
    QFile::remove("link");
    QFile::remove("link.lnk");
    QFile::remove("brokenlink");
    QFile::remove("brokenlink.lnk");
    QFile::remove("dummyfile");
    QFile::remove("relative/link");

    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isShortcut");
    QTest::addColumn<bool>("isSymbolicLink");
    QTest::addColumn<QString>("linkTarget");

    QFile file1(m_sourceFile);
    QFile file2("dummyfile");
    file2.open(QIODevice::WriteOnly);

    QTest::newRow("existent file") << m_sourceFile << false << false << "";
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    // windows shortcuts
    QVERIFY(file1.link("link.lnk"));
    QTest::newRow("link.lnk")
        << "link.lnk" << true << false << QFileInfo(m_sourceFile).absoluteFilePath();

    QVERIFY(file2.link("brokenlink.lnk"));
    QTest::newRow("broken link.lnk")
        << "brokenlink.lnk" << true << false << QFileInfo("dummyfile").absoluteFilePath();
#endif

#ifndef Q_NO_SYMLINKS
#if defined(Q_OS_WIN)
#if !defined(Q_OS_WINRT)
    QString errorMessage;
    DWORD creationResult = createSymbolicLink("link", m_sourceFile, &errorMessage);
    if (creationResult == ERROR_PRIVILEGE_NOT_HELD) {
        QWARN(msgInsufficientPrivileges(errorMessage));
    } else {
        QVERIFY2(creationResult == ERROR_SUCCESS, qPrintable(errorMessage));
        QTest::newRow("link")
            << "link" << false << true << QFileInfo(m_sourceFile).absoluteFilePath();
    }

    creationResult = createSymbolicLink("brokenlink", "dummyfile", &errorMessage);
    if (creationResult == ERROR_PRIVILEGE_NOT_HELD) {
        QWARN(msgInsufficientPrivileges(errorMessage));
    } else {
        QVERIFY2(creationResult == ERROR_SUCCESS, qPrintable(errorMessage));
        QTest::newRow("broken link")
            << "brokenlink" << false << true  << QFileInfo("dummyfile").absoluteFilePath();
    }
#endif // !Q_OS_WINRT
#else // Unix:
    QVERIFY(file1.link("link"));
    QTest::newRow("link")
        << "link" << false << true << QFileInfo(m_sourceFile).absoluteFilePath();

    QVERIFY(file2.link("brokenlink"));
    QTest::newRow("broken link")
        << "brokenlink" << false << true << QFileInfo("dummyfile").absoluteFilePath();

    QDir::current().mkdir("relative");
    QFile::link("../dummyfile", "relative/link");
    QTest::newRow("relative link")
        << "relative/link" << false << true << QFileInfo("dummyfile").absoluteFilePath();
#endif
#endif // !Q_NO_SYMLINKS
    file2.remove();
}

void tst_QFileInfo::link()
{
    QFETCH(QString, path);
    QFETCH(bool, isShortcut);
    QFETCH(bool, isSymbolicLink);
    QFETCH(QString, linkTarget);

    QFileInfo fi(path);
    QCOMPARE(fi.isShortcut(), isShortcut);
    QCOMPARE(fi.isSymbolicLink(), isSymbolicLink);
    QCOMPARE(fi.symLinkTarget(), linkTarget);
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
    QTest::newRow("empty") << QString("") << false;

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

struct NtfsTestResource {

    enum Type { None, SymLink, Junction };

    explicit NtfsTestResource(Type tp = None, const QString &s = QString(), const QString &t = QString())
        : source(s), target(t), type(tp) {}

    QString source;
    QString target;
    Type type;
};

Q_DECLARE_METATYPE(NtfsTestResource)

void tst_QFileInfo::ntfsJunctionPointsAndSymlinks_data()
{
    QTest::addColumn<NtfsTestResource>("resource");
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isSymLink");
    QTest::addColumn<QString>("linkTarget");
    QTest::addColumn<QString>("canonicalFilePath");

    QDir pwd;
    pwd.mkdir("target");

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
        QVERIFY2(file.open(QIODevice::ReadWrite), qPrintable(file.errorString()));
        file.close();

        QVERIFY2(file.exists(), msgDoesNotExist(file.fileName()).constData());

        QTest::newRow("absolute dir symlink")
            << NtfsTestResource(NtfsTestResource::SymLink, absSymlink, absTarget)
            << absSymlink << true << QDir::fromNativeSeparators(absTarget) << target.canonicalPath();
        QTest::newRow("relative dir symlink")
            << NtfsTestResource(NtfsTestResource::SymLink, relSymlink, relTarget)
            << relSymlink << true << QDir::fromNativeSeparators(absTarget) << target.canonicalPath();
        QTest::newRow("file in symlink dir")
            << NtfsTestResource()
            << fileInSymlink << false << "" << target.canonicalPath().append("/file");
    }
    {
        //File symlinks
        pwd.mkdir("relative");
        QDir relativeDir("relative");
        QFileInfo target(m_sourceFile);
        QString absTarget = QDir::toNativeSeparators(target.absoluteFilePath());
        QString absSymlink = QDir::toNativeSeparators(pwd.absolutePath()).append("\\abs_symlink.cpp");
        QString relTarget = QDir::toNativeSeparators(pwd.relativeFilePath(target.absoluteFilePath()));
        QString relSymlink = "rel_symlink.cpp";
        QString relToRelTarget = QDir::toNativeSeparators(relativeDir.relativeFilePath(target.absoluteFilePath()));
        QString relToRelSymlink = "relative/rel_symlink";

        QTest::newRow("absolute file symlink")
            << NtfsTestResource(NtfsTestResource::SymLink, absSymlink, absTarget)
            << absSymlink << true << QDir::fromNativeSeparators(absTarget) << target.canonicalFilePath();
        QTest::newRow("relative file symlink")
            << NtfsTestResource(NtfsTestResource::SymLink, relSymlink, relTarget)
            << relSymlink << true << QDir::fromNativeSeparators(absTarget) << target.canonicalFilePath();
        QTest::newRow("relative to relative file symlink")
            << NtfsTestResource(NtfsTestResource::SymLink, relToRelSymlink, relToRelTarget)
            << relToRelSymlink << true << QDir::fromNativeSeparators(absTarget) << target.canonicalFilePath();
    }
    {
        // Symlink to UNC share
        pwd.mkdir("unc");
        QString errorMessage;
        QString uncTarget = QStringLiteral("//") + QtNetworkSettings::winServerName() + "/testshare";
        QString uncSymlink = QDir::toNativeSeparators(pwd.absolutePath().append("\\unc\\link_to_unc"));
        QTest::newRow("UNC symlink")
            << NtfsTestResource(NtfsTestResource::SymLink, uncSymlink, uncTarget)
            << QDir::fromNativeSeparators(uncSymlink) << true << QDir::fromNativeSeparators(uncTarget) << uncTarget;
    }

    //Junctions
    QString target = "target";
    QString junction = "junction_pwd";
    QFileInfo targetInfo(target);
    QTest::newRow("junction_pwd")
        << NtfsTestResource(NtfsTestResource::Junction, junction, target)
        << junction << false << QString() << QString();

    QFileInfo fileInJunction(targetInfo.absoluteFilePath().append("/file"));
    QFile file(fileInJunction.absoluteFilePath());
    QVERIFY2(file.open(QIODevice::ReadWrite), qPrintable(file.errorString()));
    file.close();
    QVERIFY2(file.exists(), msgDoesNotExist(file.fileName()).constData());
    QTest::newRow("file in junction")
        << NtfsTestResource()
        << fileInJunction.absoluteFilePath() << false << QString() << fileInJunction.canonicalFilePath();

    target = QDir::rootPath();
    junction = "junction_root";
    targetInfo.setFile(target);
    QTest::newRow("junction_root")
        << NtfsTestResource(NtfsTestResource::Junction, junction, target)
        << junction << false << QString() << QString();

    //Mountpoint
    wchar_t buffer[MAX_PATH];
    QString rootPath = QDir::toNativeSeparators(QDir::rootPath());
    QVERIFY(GetVolumeNameForVolumeMountPoint((wchar_t*)rootPath.utf16(), buffer, MAX_PATH));
    QString rootVolume = QString::fromWCharArray(buffer);
    junction = "mountpoint";
    rootVolume.replace("\\\\?\\","\\??\\");
    QTest::newRow("mountpoint")
        << NtfsTestResource(NtfsTestResource::Junction, junction, rootVolume)
        << junction << false << QString() << QString();
}

void tst_QFileInfo::ntfsJunctionPointsAndSymlinks()
{
    QFETCH(NtfsTestResource, resource);
    QFETCH(QString, path);
    QFETCH(bool, isSymLink);
    QFETCH(QString, linkTarget);
    QFETCH(QString, canonicalFilePath);

    QString errorMessage;
    DWORD creationResult = ERROR_SUCCESS;
    switch (resource.type) {
    case NtfsTestResource::None:
        break;
    case NtfsTestResource::SymLink:
        creationResult = createSymbolicLink(resource.source, resource.target, &errorMessage);
        break;
    case NtfsTestResource::Junction:
        creationResult = FileSystem::createNtfsJunction(resource.target, resource.source, &errorMessage);
        if (creationResult == ERROR_NOT_SUPPORTED) // Special value indicating non-NTFS drive
            QSKIP(qPrintable(errorMessage));
        break;
    }

    if (creationResult == ERROR_PRIVILEGE_NOT_HELD)
        QSKIP(msgInsufficientPrivileges(errorMessage));
    QVERIFY2(creationResult == ERROR_SUCCESS, qPrintable(errorMessage));

    QFileInfo fi(path);
    auto guard = qScopeGuard([&fi, this]() {
        // Ensure that junctions, mountpoints are removed. If this fails, do not remove
        // temporary directory to prevent it from trashing the system.
        if (fi.isDir()) {
            if (!QDir().rmdir(fi.filePath())) {
                qWarning("Unable to remove NTFS junction '%ls', keeping '%ls'.",
                         qUtf16Printable(fi.fileName()),
                         qUtf16Printable(QDir::toNativeSeparators(m_dir.path())));
                m_dir.setAutoRemove(false);
            }
        }
    });
    const QString actualSymLinkTarget = isSymLink ? fi.symLinkTarget() : QString();
    const QString actualCanonicalFilePath = isSymLink ? fi.canonicalFilePath() : QString();
    QCOMPARE(fi.isJunction(), resource.type == NtfsTestResource::Junction);
    QCOMPARE(fi.isSymbolicLink(), isSymLink);
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
    QVERIFY(!info.isSymbolicLink());
    QVERIFY(info.isShortcut());
    QVERIFY(!info.exists());
    QFile::remove(linkName);

    QDir current; // QTBUG-21863
    QVERIFY(current.mkdir(linkName));
    QFileInfo dirInfo(linkName);
    QVERIFY(!dirInfo.isSymbolicLink());
    QVERIFY(!dirInfo.isShortcut());
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

#if defined (Q_OS_WIN) && !defined(Q_OS_WINRT)
    QScopedValueRollback<int> ntfsMode(qt_ntfs_permission_lookup);
    qt_ntfs_permission_lookup = 1;
    QFileInfo fi2(QFile::decodeName(qgetenv("SystemRoot") + "/system.ini"));
    QVERIFY(fi2.exists());
    QCOMPARE(fi2.isWritable(), IsUserAdmin());
#endif

#if defined (Q_OS_QNX) // On QNX /etc is usually on a read-only filesystem
    QVERIFY(!QFileInfo("/etc/passwd").isWritable());
#elif defined (Q_OS_UNIX) && !defined(Q_OS_VXWORKS) // VxWorks does not have users/groups
    for (const char *attempt : { "/etc/passwd", "/etc/machine-id", "/proc/version" }) {
        if (access(attempt, F_OK) == -1)
            continue;
        QCOMPARE(QFileInfo(attempt).isWritable(), ::access(attempt, W_OK) == 0);
    }
#endif
}

void tst_QFileInfo::isExecutable()
{
    QString appPath = QCoreApplication::applicationDirPath();
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
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
bool IsUserAdmin()
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

    return b != FALSE;
}

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
        if (IsUserAdmin()) {
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

static void stateCheck(const QFileInfo &info, const QString &dirname, const QString &filename)
{
    QCOMPARE(info.size(), qint64(0));
    QVERIFY(!info.exists());

    QString path;
    QString abspath;
    if (!dirname.isEmpty()) {
        path = ".";
        abspath = dirname + '/' + filename;
    }

    QCOMPARE(info.filePath(), filename);
    QCOMPARE(info.absoluteFilePath(), abspath);
    QCOMPARE(info.canonicalFilePath(), QString());
    QCOMPARE(info.fileName(), filename);
    QCOMPARE(info.baseName(), filename);
    QCOMPARE(info.completeBaseName(), filename);
    QCOMPARE(info.suffix(), QString());
    QCOMPARE(info.bundleName(), QString());
    QCOMPARE(info.completeSuffix(), QString());

    QVERIFY(info.isRelative());
    QCOMPARE(info.path(), path);
    QCOMPARE(info.absolutePath(), dirname);
    QCOMPARE(info.dir().path(), ".");

    // these don't look right
    QCOMPARE(info.canonicalPath(), path);
    QCOMPARE(info.absoluteDir().path(), dirname.isEmpty() ? "." : dirname);

    QVERIFY(!info.isReadable());
    QVERIFY(!info.isWritable());
    QVERIFY(!info.isExecutable());
    QVERIFY(!info.isHidden());
    QVERIFY(!info.isFile());
    QVERIFY(!info.isDir());
    QVERIFY(!info.isSymbolicLink());
    QVERIFY(!info.isShortcut());
    QVERIFY(!info.isBundle());
    QVERIFY(!info.isRoot());
    QCOMPARE(info.isNativePath(), !filename.isEmpty());

    QCOMPARE(info.symLinkTarget(), QString());
    QCOMPARE(info.ownerId(), uint(-2));
    QCOMPARE(info.groupId(), uint(-2));
    QCOMPARE(info.owner(), QString());
    QCOMPARE(info.group(), QString());

    QCOMPARE(info.permissions(), QFile::Permissions());

#if QT_DEPRECATED_SINCE(5, 10)
    QVERIFY(!info.created().isValid());
#endif
    QVERIFY(!info.birthTime().isValid());
    QVERIFY(!info.metadataChangeTime().isValid());
    QVERIFY(!info.lastRead().isValid());
    QVERIFY(!info.lastModified().isValid());
};

void tst_QFileInfo::invalidState_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("default") << 0;
    QTest::newRow("empty") << 1;
    QTest::newRow("copy-of-default") << 2;
    QTest::newRow("copy-of-empty") << 3;
}

void tst_QFileInfo::invalidState()
{
    // Shouldn't crash or produce warnings
    QFETCH(int, mode);
    const QFileInfo &info = (mode & 1 ? QFileInfo("") : QFileInfo());

    if (mode & 2) {
        QFileInfo copy(info);
        stateCheck(copy, QString(), QString());
    } else {
        stateCheck(info, QString(), QString());
    }
}

void tst_QFileInfo::nonExistingFile()
{
    QString dirname = QDir::currentPath();
    QString cdirname = QFileInfo(dirname).canonicalFilePath();
    if (dirname != cdirname)
        QDir::setCurrent(cdirname); // chdir() to our canonical path

    QString filename = "non-existing-file-foobar";
    QFileInfo info(filename);
    stateCheck(info, dirname, filename);
}


QTEST_MAIN(tst_QFileInfo)
#include "tst_qfileinfo.moc"
