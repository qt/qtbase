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
#include <qstandardpaths.h>
#include <qdebug.h>
#include <qstandardpaths.h>
#include <qfileinfo.h>
#include <qsysinfo.h>
#include <qregexp.h>
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT) && !defined(Q_OS_WINCE)
#  include <qt_windows.h>
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_ANDROID)
#define Q_XDG_PLATFORM
#endif

#include "emulationdetector.h"

// Update this when adding new enum values; update enumNames too
static const int MaxStandardLocation = QStandardPaths::AppConfigLocation;

class tst_qstandardpaths : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void dump();
    void testDefaultLocations();
    void testCustomLocations();
    void enableTestMode();
    void testLocateAll();
    void testDataLocation();
    void testAppConfigLocation();
    void testFindExecutable_data();
    void testFindExecutable();
    void testFindExecutableLinkToDirectory();
    void testRuntimeDirectory();
    void testCustomRuntimeDirectory();
    void testAllWritableLocations_data();
    void testAllWritableLocations();
    void testCleanPath();
    void testXdgPathCleanup();

private:
#ifdef Q_XDG_PLATFORM
    void setCustomLocations() {
        m_localConfigDir = m_localConfigTempDir.path();
        m_globalConfigDir = m_globalConfigTempDir.path();
        qputenv("XDG_CONFIG_HOME", QFile::encodeName(m_localConfigDir));
        qputenv("XDG_CONFIG_DIRS", QFile::encodeName(m_globalConfigDir));
        m_localAppDir = m_localAppTempDir.path();
        m_globalAppDir = m_globalAppTempDir.path();
        qputenv("XDG_DATA_HOME", QFile::encodeName(m_localAppDir));
        qputenv("XDG_DATA_DIRS", QFile::encodeName(m_globalAppDir));
    }
    void setDefaultLocations() {
        qputenv("XDG_CONFIG_HOME", QByteArray());
        qputenv("XDG_CONFIG_DIRS", QByteArray());
        qputenv("XDG_DATA_HOME", QByteArray());
        qputenv("XDG_DATA_DIRS", QByteArray());
    }
#endif

    // Config dirs
    QString m_localConfigDir;
    QTemporaryDir m_localConfigTempDir;
    QString m_globalConfigDir;
    QTemporaryDir m_globalConfigTempDir;

    // App dirs
    QString m_localAppDir;
    QTemporaryDir m_localAppTempDir;
    QString m_globalAppDir;
    QTemporaryDir m_globalAppTempDir;
};

static const char * const enumNames[MaxStandardLocation + 1 - int(QStandardPaths::DesktopLocation)] = {
    "DesktopLocation",
    "DocumentsLocation",
    "FontsLocation",
    "ApplicationsLocation",
    "MusicLocation",
    "MoviesLocation",
    "PicturesLocation",
    "TempLocation",
    "HomeLocation",
    "DataLocation",
    "CacheLocation",
    "GenericDataLocation",
    "RuntimeLocation",
    "ConfigLocation",
    "DownloadLocation",
    "GenericCacheLocation",
    "GenericConfigLocation",
    "AppDataLocation",
    "AppConfigLocation"
};

void tst_qstandardpaths::initTestCase()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT) && !defined(Q_OS_WINCE)
    // Disable WOW64 redirection, see testFindExecutable()
    if (QSysInfo::buildCpuArchitecture() != QSysInfo::currentCpuArchitecture()) {
        void *oldMode;
        const bool disabledDisableWow64FsRedirection = Wow64DisableWow64FsRedirection(&oldMode) == TRUE;
        if (!disabledDisableWow64FsRedirection)
            qErrnoWarning("Wow64DisableWow64FsRedirection() failed");
        QVERIFY(disabledDisableWow64FsRedirection);
    }
#endif // Q_OS_WIN && !Q_OS_WINRT && !Q_OS_WINCE
    QVERIFY2(m_localConfigTempDir.isValid(), qPrintable(m_localConfigTempDir.errorString()));
    QVERIFY2(m_globalConfigTempDir.isValid(), qPrintable(m_globalConfigTempDir.errorString()));
    QVERIFY2(m_localAppTempDir.isValid(), qPrintable(m_localAppTempDir.errorString()));
    QVERIFY2(m_globalAppTempDir.isValid(), qPrintable(m_globalAppTempDir.errorString()));
}

void tst_qstandardpaths::dump()
{
#ifdef Q_XDG_PLATFORM
    setDefaultLocations();
#endif
    // This is not a test. It merely dumps the output.
    for (int i = QStandardPaths::DesktopLocation; i <= MaxStandardLocation; ++i) {
        QStandardPaths::StandardLocation s = QStandardPaths::StandardLocation(i);
        qDebug() << enumNames[i]
                 << QStandardPaths::writableLocation(s)
                 << QStandardPaths::standardLocations(s);
    }
}

void tst_qstandardpaths::testDefaultLocations()
{
#ifdef Q_XDG_PLATFORM
    setDefaultLocations();

    const QString expectedConfHome = QDir::homePath() + QString::fromLatin1("/.config");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation), expectedConfHome);
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation), expectedConfHome);
    const QStringList confDirs = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    QCOMPARE(confDirs.count(), 2);
    QVERIFY(confDirs.contains(expectedConfHome));
    QCOMPARE(QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation), confDirs);

    const QStringList genericDataDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QCOMPARE(genericDataDirs.count(), 3);
    const QString expectedDataHome = QDir::homePath() + QString::fromLatin1("/.local/share");
    QCOMPARE(genericDataDirs.at(0), expectedDataHome);
    QCOMPARE(genericDataDirs.at(1), QString::fromLatin1("/usr/local/share"));
    QCOMPARE(genericDataDirs.at(2), QString::fromLatin1("/usr/share"));
#endif
}

static void createTestFile(const QString &fileName)
{
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("Hello"));
}

void tst_qstandardpaths::testCustomLocations()
{
#ifdef Q_XDG_PLATFORM
    setCustomLocations();

    // test writableLocation()
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation), m_localConfigDir);
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation), m_localConfigDir);

    // test locate()
    const QString thisFileName = QString::fromLatin1("aFile");
    createTestFile(m_localConfigDir + QLatin1Char('/') + thisFileName);
    const QString thisFile = QStandardPaths::locate(QStandardPaths::ConfigLocation, thisFileName);
    QVERIFY(!thisFile.isEmpty());
    QVERIFY(thisFile.endsWith(thisFileName));

    const QString subdir = QString::fromLatin1("subdir");
    const QString subdirPath = m_localConfigDir + QLatin1Char('/') + subdir;
    QVERIFY(QDir().mkdir(subdirPath));
    const QString dir = QStandardPaths::locate(QStandardPaths::ConfigLocation, subdir, QStandardPaths::LocateDirectory);
    QCOMPARE(dir, subdirPath);
    const QString thisDirAsFile = QStandardPaths::locate(QStandardPaths::ConfigLocation, subdir);
    QVERIFY(thisDirAsFile.isEmpty()); // not a file

    const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    QCOMPARE(dirs, QStringList() << m_localConfigDir << m_globalConfigDir);
#endif
}

void tst_qstandardpaths::enableTestMode()
{
    QVERIFY(!QStandardPaths::isTestModeEnabled());
    QStandardPaths::setTestModeEnabled(true);
    QVERIFY(QStandardPaths::isTestModeEnabled());

#ifdef Q_XDG_PLATFORM
    setCustomLocations(); // for the global config dir
    const QString qttestDir = QDir::homePath() + QLatin1String("/.qttest");

    // ConfigLocation
    const QString configDir = qttestDir + QLatin1String("/config");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation), configDir);
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation), configDir);
    const QStringList confDirs = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    QCOMPARE(confDirs, QStringList() << configDir << m_globalConfigDir);

    // GenericDataLocation
    const QString dataDir = qttestDir + QLatin1String("/share");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation), dataDir);
    const QStringList gdDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QCOMPARE(gdDirs, QStringList() << dataDir << m_globalAppDir);

    // GenericCacheLocation
    const QString cacheDir = qttestDir + QLatin1String("/cache");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation), cacheDir);
    const QStringList cacheDirs = QStandardPaths::standardLocations(QStandardPaths::GenericCacheLocation);
    QCOMPARE(cacheDirs, QStringList() << cacheDir);
#endif

    // On all platforms, we want to ensure that the writableLocation is different in test mode and real mode.
    // Check this for locations where test programs typically write. Not desktop, download, music etc...
    typedef QHash<QStandardPaths::StandardLocation, QString> LocationHash;
    LocationHash testLocations;
    testLocations.insert(QStandardPaths::AppDataLocation, QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    testLocations.insert(QStandardPaths::AppLocalDataLocation, QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    testLocations.insert(QStandardPaths::GenericDataLocation, QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    testLocations.insert(QStandardPaths::ConfigLocation, QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    testLocations.insert(QStandardPaths::GenericConfigLocation, QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
    testLocations.insert(QStandardPaths::CacheLocation, QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    testLocations.insert(QStandardPaths::GenericCacheLocation, QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation));
    // On Windows, what should "Program Files" become, in test mode?
    //testLocations.insert(QStandardPaths::ApplicationsLocation, QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation));

    QStandardPaths::setTestModeEnabled(false);

    for (LocationHash::const_iterator it = testLocations.constBegin(); it != testLocations.constEnd(); ++it)
        QVERIFY2(QStandardPaths::writableLocation(it.key()) != it.value(), qPrintable(it.value()));

    // Check that this is also true with no env vars set
#ifdef Q_XDG_PLATFORM
    setDefaultLocations();
    for (LocationHash::const_iterator it = testLocations.constBegin(); it != testLocations.constEnd(); ++it)
        QVERIFY2(QStandardPaths::writableLocation(it.key()) != it.value(), qPrintable(it.value()));
#endif
}

void tst_qstandardpaths::testLocateAll()
{
#ifdef Q_XDG_PLATFORM
    setCustomLocations();
    const QStringList appsDirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "applications", QStandardPaths::LocateDirectory);
    QCOMPARE(appsDirs.count(), 0); // they don't exist yet
    const QStringList expectedAppsDirs = QStringList() << m_localAppDir + QLatin1String("/applications")
                                                       << m_globalAppDir + QLatin1String("/applications");
    QDir().mkdir(expectedAppsDirs.at(0));
    QDir().mkdir(expectedAppsDirs.at(1));
    const QStringList appsDirs2 = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "applications", QStandardPaths::LocateDirectory);
    QCOMPARE(appsDirs2, expectedAppsDirs);

    const QStringList appsDirs3 = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    QCOMPARE(appsDirs3, expectedAppsDirs);

    const QString thisFileName = QString::fromLatin1("aFile");
    const QStringList expectedFiles = QStringList() << m_localConfigDir + QLatin1Char('/') + thisFileName
                                                    << m_globalConfigDir + QLatin1Char('/') + thisFileName;
    createTestFile(expectedFiles.at(0));
    createTestFile(expectedFiles.at(1));
    const QStringList allFiles = QStandardPaths::locateAll(QStandardPaths::ConfigLocation, thisFileName);
    QCOMPARE(allFiles, expectedFiles);
#endif
}

void tst_qstandardpaths::testDataLocation()
{
    // On all platforms, DataLocation should be GenericDataLocation / organization name / app name
    // This allows one app to access the data of another app.
    // Android and WinRT are an exception to this case, owing to the fact that
    // applications are sandboxed.
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WINRT)
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation), base + "/tst_qstandardpaths");
    QCoreApplication::instance()->setOrganizationName("Qt");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation), base + "/Qt/tst_qstandardpaths");
    QCoreApplication::instance()->setApplicationName("QtTest");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation), base + "/Qt/QtTest");
#endif

#ifdef Q_XDG_PLATFORM
    setDefaultLocations();
    const QString expectedAppDataDir = QDir::homePath() + QString::fromLatin1("/.local/share/Qt/QtTest");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation), expectedAppDataDir);
    const QStringList appDataDirs = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
    QCOMPARE(appDataDirs.count(), 3);
    QCOMPARE(appDataDirs.at(0), expectedAppDataDir);
    QCOMPARE(appDataDirs.at(1), QString::fromLatin1("/usr/local/share/Qt/QtTest"));
    QCOMPARE(appDataDirs.at(2), QString::fromLatin1("/usr/share/Qt/QtTest"));
#endif

    // reset for other tests
    QCoreApplication::setOrganizationName(QString());
    QCoreApplication::setApplicationName(QString());
}

void tst_qstandardpaths::testAppConfigLocation()
{
    // On all platforms where applications are not sandboxed,
    // AppConfigLocation should be GenericConfigLocation / organization name / app name
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WINRT)
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), base + "/tst_qstandardpaths");
    QCoreApplication::setOrganizationName("Qt");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), base + "/Qt/tst_qstandardpaths");
    QCoreApplication::setApplicationName("QtTest");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), base + "/Qt/QtTest");
    // reset for other tests
    QCoreApplication::setOrganizationName(QString());
    QCoreApplication::setApplicationName(QString());
#endif
}

#ifndef Q_OS_WIN
// Find "sh" on Unix.
// It may exist twice, in /bin/sh and /usr/bin/sh, in that case use the PATH order.
static inline QFileInfo findSh()
{
    QLatin1String sh("/sh");
    QByteArray pEnv = qgetenv("PATH");
    const QLatin1Char pathSep(':');
    const QStringList rawPaths = QString::fromLocal8Bit(pEnv.constData()).split(pathSep, QString::SkipEmptyParts);
    foreach (const QString &path, rawPaths) {
        if (QFile::exists(path + sh))
            return path + sh;
    }
    return QFileInfo();
}
#endif

void tst_qstandardpaths::testFindExecutable_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("needle");
    QTest::addColumn<QString>("expected");
#ifdef Q_OS_WIN
# ifndef Q_OS_WINRT
    const QFileInfo cmdFi = QFileInfo(QDir::cleanPath(QString::fromLocal8Bit(qgetenv("COMSPEC"))));
    const QString cmdPath = cmdFi.absoluteFilePath();

    Q_ASSERT(cmdFi.exists());
    QTest::newRow("win-cmd")
        << QString() << QString::fromLatin1("cmd.eXe") << cmdPath;
    QTest::newRow("win-full-path")
        << QString() << cmdPath << cmdPath;
    QTest::newRow("win-relative-path")
        << cmdFi.absolutePath() << QString::fromLatin1("./cmd.exe") << cmdPath;
    QTest::newRow("win-cmd-nosuffix")
        << QString() << QString::fromLatin1("cmd") << cmdPath;

    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8) {
        // The logo executable on Windows 8 is perfectly suited for testing that the
        // suffix mechanism is not thrown off by dots in the name.
        // Note: Requires disabling WOW64 redirection, see initTestCase()
        const QString logo = QLatin1String("microsoft.windows.softwarelogo.showdesktop");
        const QString logoPath = cmdFi.absolutePath() + QLatin1Char('/') + logo + QLatin1String(".exe");
        QTest::newRow("win8-logo")
            << QString() << (logo + QLatin1String(".exe")) << logoPath;
        QTest::newRow("win8-logo-nosuffix")
            << QString() << logo << logoPath;
    }
# endif // Q_OS_WINRT
#else
    const QFileInfo shFi = findSh();
    Q_ASSERT(shFi.exists());
    const QString shPath = shFi.absoluteFilePath();
    QTest::newRow("unix-sh")
        << QString() << QString::fromLatin1("sh") << shPath;
    QTest::newRow("unix-sh-fullpath")
        << QString() << shPath << shPath;
    QTest::newRow("unix-sh-relativepath")
        << QString(shFi.absolutePath()) << QString::fromLatin1("./sh") << shPath;
#endif
    QTest::newRow("idontexist")
        << QString() << QString::fromLatin1("idontexist") << QString();
    QTest::newRow("empty")
        << QString() << QString() << QString();
}

void tst_qstandardpaths::testFindExecutable()
{
    QFETCH(QString, directory);
    QFETCH(QString, needle);
    QFETCH(QString, expected);
    const bool changeDirectory = !directory.isEmpty();
    const QString currentDirectory = QDir::currentPath();
    if (changeDirectory)
        QVERIFY(QDir::setCurrent(directory));
    const QString result = QStandardPaths::findExecutable(needle);
    if (changeDirectory)
        QVERIFY(QDir::setCurrent(currentDirectory));

#ifdef Q_OS_WIN
    const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif
    QVERIFY2(!result.compare(expected, sensitivity),
             qPrintable(QString::fromLatin1("Actual: '%1', Expected: '%2'").arg(result, expected)));
}

void tst_qstandardpaths::testFindExecutableLinkToDirectory()
{
    // WinRT has no link support
#ifndef Q_OS_WINRT
    // link to directory
    const QString target = QDir::tempPath() + QDir::separator() + QLatin1String("link.lnk");
    QFile::remove(target);
    QFile appFile(QCoreApplication::applicationDirPath());
    QVERIFY(appFile.link(target));
    QVERIFY(QStandardPaths::findExecutable(target).isEmpty());
    QFile::remove(target);
#endif
}

void tst_qstandardpaths::testRuntimeDirectory()
{
#ifdef Q_XDG_PLATFORM
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QVERIFY(!runtimeDir.isEmpty());

    // Check that it can automatically fix permissions
    QFile file(runtimeDir);
    const QFile::Permissions wantedPerms = QFile::ReadUser | QFile::WriteUser | QFile::ExeUser;
    const QFile::Permissions additionalPerms = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;
    QCOMPARE(file.permissions(), wantedPerms | additionalPerms);
    QVERIFY(file.setPermissions(wantedPerms | QFile::ExeGroup));
    const QString runtimeDirAgain = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QCOMPARE(runtimeDirAgain, runtimeDir);
    QCOMPARE(QFile(runtimeDirAgain).permissions(), wantedPerms | additionalPerms);
#endif
}

void tst_qstandardpaths::testCustomRuntimeDirectory()
{
#if defined(Q_OS_UNIX)
    if (::getuid() == 0)
        QSKIP("Running this test as root doesn't make sense");
#endif

#ifdef Q_XDG_PLATFORM
    struct EnvVarRestorer
    {
        EnvVarRestorer() : origRuntimeDir(qgetenv("XDG_RUNTIME_DIR")) {}
        ~EnvVarRestorer() { qputenv("XDG_RUNTIME_DIR", origRuntimeDir.constData()); }
        const QByteArray origRuntimeDir;
    };
    EnvVarRestorer restorer;

    // When $XDG_RUNTIME_DIR points to a directory with wrong ownership, QStandardPaths should warn
    QByteArray rootOwnedFileName = "/tmp";
    if (EmulationDetector::isRunningArmOnX86()) {
        // Directory "tmp" under toolchain sysroot is detected by qemu and has same uid as current user.
        // Try /opt instead, it might not be located in the sysroot.
        QFileInfo rootOwnedFile = QFileInfo(QString::fromLatin1(rootOwnedFileName));
        if (rootOwnedFile.ownerId() == ::geteuid()) {
            rootOwnedFileName = "/opt";
        }
    }
    qputenv("XDG_RUNTIME_DIR", QFile::encodeName(rootOwnedFileName));

    // It's very unlikely that /tmp is 0600 or that we can chmod it
    // The call below outputs
    //   "QStandardPaths: wrong ownership on runtime directory /tmp, 0 instead of $UID"
    // but we can't reliably expect that it's owned by uid 0, I think.
    const uid_t uid = geteuid();
    QTest::ignoreMessage(QtWarningMsg,
            qPrintable(QString::fromLatin1("QStandardPaths: wrong ownership on runtime directory " + rootOwnedFileName + ", 0 instead of %1").arg(uid)));
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QVERIFY2(runtimeDir.isEmpty(), qPrintable(runtimeDir));

    // When $XDG_RUNTIME_DIR points to a non-existing directory, QStandardPaths should warn (QTBUG-48771)
    qputenv("XDG_RUNTIME_DIR", "does_not_exist");
    QTest::ignoreMessage(QtWarningMsg, "QStandardPaths: XDG_RUNTIME_DIR points to non-existing path 'does_not_exist', please create it with 0700 permissions.");
    const QString nonExistingRuntimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QVERIFY2(nonExistingRuntimeDir.isEmpty(), qPrintable(nonExistingRuntimeDir));

    // When $XDG_RUNTIME_DIR points to a file, QStandardPaths should warn
    const QString file = QFINDTESTDATA("tst_qstandardpaths.cpp");
    QVERIFY(!file.isEmpty());
    qputenv("XDG_RUNTIME_DIR", QFile::encodeName(file));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(QString::fromLatin1("QStandardPaths: XDG_RUNTIME_DIR points to '%1' which is not a directory").arg(file)));
    const QString noRuntimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QVERIFY2(noRuntimeDir.isEmpty(), qPrintable(file));
#endif
}

Q_DECLARE_METATYPE(QStandardPaths::StandardLocation)
void tst_qstandardpaths::testAllWritableLocations_data()
{
    QTest::addColumn<QStandardPaths::StandardLocation>("location");
    QTest::newRow("DesktopLocation") << QStandardPaths::DesktopLocation;
    QTest::newRow("DocumentsLocation") << QStandardPaths::DocumentsLocation;
    QTest::newRow("FontsLocation") << QStandardPaths::FontsLocation;
    QTest::newRow("ApplicationsLocation") << QStandardPaths::ApplicationsLocation;
    QTest::newRow("MusicLocation") << QStandardPaths::MusicLocation;
    QTest::newRow("MoviesLocation") << QStandardPaths::MoviesLocation;
    QTest::newRow("PicturesLocation") << QStandardPaths::PicturesLocation;
    QTest::newRow("TempLocation") << QStandardPaths::TempLocation;
    QTest::newRow("HomeLocation") << QStandardPaths::HomeLocation;
    QTest::newRow("AppLocalDataLocation") << QStandardPaths::AppLocalDataLocation;
    QTest::newRow("DownloadLocation") << QStandardPaths::DownloadLocation;
}

void tst_qstandardpaths::testAllWritableLocations()
{
    QFETCH(QStandardPaths::StandardLocation, location);
    QStandardPaths::writableLocation(location);
    QStandardPaths::displayName(location);

    // Currently all desktop locations return their writable location
    // with "Unix-style" paths (i.e. they use a slash, not backslash).
    QString loc = QStandardPaths::writableLocation(location);
    if (loc.size() > 1)  // workaround for unlikely case of locations that return '/'
        QCOMPARE(loc.endsWith(QLatin1Char('/')), false);
    QVERIFY(loc.isEmpty() || loc.contains(QLatin1Char('/')));
    QVERIFY(!loc.contains(QLatin1Char('\\')));
}

void tst_qstandardpaths::testCleanPath()
{
    const QRegExp filter(QStringLiteral("\\\\"));
    QVERIFY(filter.isValid());
    for (int i = 0; i <= QStandardPaths::GenericCacheLocation; ++i) {
        const QStringList paths = QStandardPaths::standardLocations(QStandardPaths::StandardLocation(i));
        QVERIFY2(paths.filter(filter).isEmpty(),
                 qPrintable(QString::fromLatin1("Backslash found in %1 %2")
                            .arg(i).arg(paths.join(QLatin1Char(',')))));
    }
}

void tst_qstandardpaths::testXdgPathCleanup()
{
#ifdef Q_XDG_PLATFORM
    setCustomLocations();
    const QString uncleanGlobalAppDir = "/./" + QFile::encodeName(m_globalAppDir);
    qputenv("XDG_DATA_DIRS", QFile::encodeName(uncleanGlobalAppDir) + "::relative/path");
    const QStringList appsDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    QVERIFY(!appsDirs.contains("/applications"));
    QVERIFY(!appsDirs.contains(uncleanGlobalAppDir + "/applications"));
    QVERIFY(!appsDirs.contains("relative/path/applications"));
#endif
}

QTEST_MAIN(tst_qstandardpaths)

#include "tst_qstandardpaths.moc"
