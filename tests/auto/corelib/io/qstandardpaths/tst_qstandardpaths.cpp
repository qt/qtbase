// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qstandardpaths.h>
#include <QTest>
#include <QOperatingSystemVersion>
#include <qdebug.h>
#include <qfileinfo.h>
#include <qplatformdefs.h>
#include <qregularexpression.h>
#include <qsysinfo.h>
#if defined(Q_OS_WIN)
#  include <qt_windows.h>
#endif
#ifdef Q_OS_ANDROID
#include <private/qjnihelpers_p.h>
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS) && !defined(Q_OS_ANDROID)
#define Q_XDG_PLATFORM
#endif

using namespace Qt::StringLiterals;

// Update this when adding new enum values; update enumNames too
static const int MaxStandardLocation = QStandardPaths::GenericStateLocation;

static QString genericCacheLoc()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
}
static QString cacheLoc()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

static QString genericStateLoc()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation);
}
static QString stateLoc()
{
    return QStandardPaths::writableLocation(QStandardPaths::StateLocation);
}

static QString genericDataLoc()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
}
static QString appDataLoc()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}
static QString appLocalDataLoc()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

static QString genericConfigLoc()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
}
static QString configLoc()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
}
static QString appConfigLoc()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

class tst_qstandardpaths : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void dump();
    void init();
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
    void testCustomRuntimeDirectory_data();
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
        m_stateDir = m_stateTempDir.path();
        qputenv("XDG_DATA_HOME", QFile::encodeName(m_localAppDir));
        qputenv("XDG_DATA_DIRS", QFile::encodeName(m_globalAppDir));
        qputenv("XDG_STATE_HOME", QFile::encodeName(m_stateDir));
    }
    void setDefaultLocations() {
        qputenv("XDG_CONFIG_HOME", nullptr);
        qputenv("XDG_CONFIG_DIRS", nullptr);
        qputenv("XDG_DATA_HOME", nullptr);
        qputenv("XDG_DATA_DIRS", nullptr);
        qputenv("XDG_STATE_HOME", nullptr);
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
    QString m_stateDir;
    QTemporaryDir m_stateTempDir;
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
    "AppLocalDataLocation",
    "CacheLocation",
    "GenericDataLocation",
    "RuntimeLocation",
    "ConfigLocation",
    "DownloadLocation",
    "GenericCacheLocation",
    "GenericConfigLocation",
    "AppDataLocation",
    "AppConfigLocation",
    "PublicShareLocation",
    "TemplatesLocation",
    "StateLocation",
    "GenericStateLocation"
};

void tst_qstandardpaths::initTestCase()
{
#if defined(Q_OS_WIN)
    // Disable WOW64 redirection, see testFindExecutable()
    if (QSysInfo::buildCpuArchitecture() != QSysInfo::currentCpuArchitecture()) {
        void *oldMode;
        const bool disabledDisableWow64FsRedirection = Wow64DisableWow64FsRedirection(&oldMode) == TRUE;
        if (!disabledDisableWow64FsRedirection)
            qErrnoWarning("Wow64DisableWow64FsRedirection() failed");
        QVERIFY(disabledDisableWow64FsRedirection);
    }
#endif // Q_OS_WIN
    QVERIFY2(m_localConfigTempDir.isValid(), qPrintable(m_localConfigTempDir.errorString()));
    QVERIFY2(m_globalConfigTempDir.isValid(), qPrintable(m_globalConfigTempDir.errorString()));
    QVERIFY2(m_localAppTempDir.isValid(), qPrintable(m_localAppTempDir.errorString()));
    QVERIFY2(m_globalAppTempDir.isValid(), qPrintable(m_globalAppTempDir.errorString()));
    QVERIFY2(m_stateTempDir.isValid(), qPrintable(m_stateTempDir.errorString()));
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

void tst_qstandardpaths::init()
{
    // Some unittests set a custom org/app names, restore the original ones
    // before each unittest is run
    static const QString org = QCoreApplication::organizationName();
    static const QString app = QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName(org);
    QCoreApplication::setApplicationName(app);
}

void tst_qstandardpaths::testDefaultLocations()
{
#ifdef Q_XDG_PLATFORM
    setDefaultLocations();

    const QString expectedConfHome = QDir::homePath() + QString::fromLatin1("/.config");
    QCOMPARE(configLoc(), expectedConfHome);
    QCOMPARE(genericConfigLoc(), expectedConfHome);
    const QStringList confDirs = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    QCOMPARE(confDirs.size(), 2);
    QVERIFY(confDirs.contains(expectedConfHome));
    QCOMPARE(QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation), confDirs);

    const QStringList genericDataDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QCOMPARE(genericDataDirs.size(), 3);
    const QString expectedDataHome = QDir::homePath() + QString::fromLatin1("/.local/share");
    QCOMPARE(genericDataDirs.at(0), expectedDataHome);
    QCOMPARE(genericDataDirs.at(1), QString::fromLatin1("/usr/local/share"));
    QCOMPARE(genericDataDirs.at(2), QString::fromLatin1("/usr/share"));
    const QString expectedGenericStateLocation = QDir::homePath() + QString::fromLatin1("/.local/state");
    QCOMPARE(genericStateLoc(), expectedGenericStateLocation);
#endif
}

#ifdef Q_XDG_PLATFORM
static void createTestFile(const QString &fileName)
{
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("Hello"));
}
#endif

void tst_qstandardpaths::testCustomLocations()
{
#ifdef Q_XDG_PLATFORM
    setCustomLocations();

    // test writableLocation()
    QCOMPARE(configLoc(), m_localConfigDir);
    QCOMPARE(genericConfigLoc(), m_localConfigDir);

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

    // *Config*Location
    const QString configDir = qttestDir + QLatin1String("/config");
    QCOMPARE(configLoc(), configDir);
    QCOMPARE(genericConfigLoc(), configDir);
    const QStringList confDirs = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    QCOMPARE(confDirs, QStringList() << configDir << m_globalConfigDir);
    // AppConfigLocation should be "GenericConfigLocation/organization-name/app-name"
    QCOMPARE(appConfigLoc(), configDir + "/tst_qstandardpaths"_L1);

    // *Data*Location
    const QString dataDir = qttestDir + QLatin1String("/share");
    QCOMPARE(genericDataLoc(), dataDir);
    const QStringList gdDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QCOMPARE(gdDirs, QStringList() << dataDir << m_globalAppDir);
    // AppDataLocation/AppLocalDataLocation should be
    // "GenericDataLocation/organization-name/app-name"
    QCOMPARE(appDataLoc(), dataDir + "/tst_qstandardpaths"_L1);
    QCOMPARE(appLocalDataLoc(), dataDir + "/tst_qstandardpaths"_L1);

    // *CacheLocation
    const QString cacheDir = qttestDir + QLatin1String("/cache");
    QCOMPARE(genericCacheLoc(), cacheDir);
    const QStringList cacheDirs = QStandardPaths::standardLocations(QStandardPaths::GenericCacheLocation);
    QCOMPARE(cacheDirs, QStringList() << cacheDir);
    // CacheLocation should be "GenericCacheLocation/organization-name/app-name"
    QCOMPARE(cacheLoc(), cacheDir + "/tst_qstandardpaths"_L1);

    // *StateLocation
    const QString stateDir = qttestDir + QLatin1String("/state");
    QCOMPARE(genericStateLoc(), stateDir);
    const QStringList stateDirs = QStandardPaths::standardLocations(QStandardPaths::GenericStateLocation);
    QCOMPARE(stateDirs, QStringList() << stateDir);
    // StateLocation should be "GenericStateLocation/organization-name/app-name"
    QCOMPARE(stateLoc(), stateDir + "/tst_qstandardpaths"_L1);

    QCoreApplication::setOrganizationName("Qt");
    QCOMPARE(appConfigLoc(), configDir + "/Qt/tst_qstandardpaths"_L1);
    QCOMPARE(appDataLoc(), dataDir + "/Qt/tst_qstandardpaths"_L1);
    QCOMPARE(appLocalDataLoc(), dataDir + "/Qt/tst_qstandardpaths"_L1);
    QCOMPARE(cacheLoc(), cacheDir + "/Qt/tst_qstandardpaths"_L1);
    QCOMPARE(stateLoc(), stateDir + "/Qt/tst_qstandardpaths"_L1);

    QCoreApplication::setApplicationName("QtTest");
    QCOMPARE(appConfigLoc(), configDir + "/Qt/QtTest"_L1);
    QCOMPARE(appDataLoc(), dataDir + "/Qt/QtTest"_L1);
    QCOMPARE(appLocalDataLoc(), dataDir + "/Qt/QtTest"_L1);
    QCOMPARE(cacheLoc(), cacheDir + "/Qt/QtTest"_L1);
    QCOMPARE(stateLoc(), stateDir + "/Qt/QtTest"_L1);

    // Check these are unaffected by org/app names
    QCOMPARE(genericConfigLoc(), configDir);
    QCOMPARE(configLoc(), configDir);
    QCOMPARE(genericDataLoc(), dataDir);
    QCOMPARE(genericCacheLoc(), cacheDir);
    QCOMPARE(genericStateLoc(), stateDir);
#endif

    // On all platforms, we want to ensure that the writableLocation is different in test mode and real mode.
    // Check this for locations where test programs typically write. Not desktop, download, music etc...
    typedef QHash<QStandardPaths::StandardLocation, QString> LocationHash;
    LocationHash testLocations;
    testLocations.insert(QStandardPaths::AppDataLocation, appDataLoc());
    testLocations.insert(QStandardPaths::AppLocalDataLocation, appLocalDataLoc());
    testLocations.insert(QStandardPaths::GenericDataLocation, genericDataLoc());
    testLocations.insert(QStandardPaths::ConfigLocation, configLoc());
    testLocations.insert(QStandardPaths::GenericConfigLocation, genericConfigLoc());
    testLocations.insert(QStandardPaths::CacheLocation, cacheLoc());
    testLocations.insert(QStandardPaths::GenericCacheLocation, genericCacheLoc());
    testLocations.insert(QStandardPaths::StateLocation, stateLoc());
    testLocations.insert(QStandardPaths::GenericStateLocation, genericStateLoc());
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
    QCOMPARE(appsDirs.size(), 0); // they don't exist yet
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
    // On all platforms, AppLocalDataLocation should be GenericDataLocation / organization name / app name
    // This allows one app to access the data of another app.
    // Android is an exception to this case, owing to the fact that
    // applications are sandboxed.
#if !defined(Q_OS_ANDROID)
    const QString base = genericDataLoc();
    QCOMPARE(appLocalDataLoc(), base + "/tst_qstandardpaths");
    QCoreApplication::instance()->setOrganizationName("Qt");
    QCOMPARE(appLocalDataLoc(), base + "/Qt/tst_qstandardpaths");
    QCoreApplication::instance()->setApplicationName("QtTest");
    QCOMPARE(appLocalDataLoc(), base + "/Qt/QtTest");
#endif

#ifdef Q_XDG_PLATFORM
    setDefaultLocations();
    const QString expectedAppDataDir = QDir::homePath() + QString::fromLatin1("/.local/share/Qt/QtTest");
    QCOMPARE(appLocalDataLoc(), expectedAppDataDir);
    const QStringList appDataDirs = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
    QCOMPARE(appDataDirs.size(), 3);
    QCOMPARE(appDataDirs.at(0), expectedAppDataDir);
    QCOMPARE(appDataDirs.at(1), QString::fromLatin1("/usr/local/share/Qt/QtTest"));
    QCOMPARE(appDataDirs.at(2), QString::fromLatin1("/usr/share/Qt/QtTest"));
#endif
}

void tst_qstandardpaths::testAppConfigLocation()
{
    // On all platforms where applications are not sandboxed,
    // AppConfigLocation should be GenericConfigLocation / organization name / app name
#if !defined(Q_OS_ANDROID)
    const QString base = genericConfigLoc();
    QCOMPARE(appConfigLoc(), base + "/tst_qstandardpaths");
    QCoreApplication::setOrganizationName("Qt");
    QCOMPARE(appConfigLoc(), base + "/Qt/tst_qstandardpaths");
    QCoreApplication::setApplicationName("QtTest");
    QCOMPARE(appConfigLoc(), base + "/Qt/QtTest");
#endif
}

#if !defined(Q_OS_WIN) && !defined(Q_OS_WASM)
// Find "sh" on Unix.
// It may exist twice, in /bin/sh and /usr/bin/sh, in that case use the PATH order.
static inline QFileInfo findSh()
{
    QLatin1String sh("/sh");
    QByteArray pEnv = qgetenv("PATH");
    const QLatin1Char pathSep(':');
    const QStringList rawPaths = QString::fromLocal8Bit(pEnv.constData()).split(pathSep, Qt::SkipEmptyParts);
    for (const QString &path : rawPaths) {
        if (QFile::exists(path + sh))
            return QFileInfo(path + sh);
    }
    return QFileInfo();
}
#endif

void tst_qstandardpaths::testFindExecutable_data()
{
#ifdef SKIP_FINDEXECUTABLE
    // Test needs to be skipped or Q_ASSERT below will cancel the test
    // and report FAIL regardless of BLACKLIST contents
    QSKIP("QTBUG-64404");
#endif

    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("needle");
    QTest::addColumn<QString>("expected");
#ifdef Q_OS_WIN
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
#else
# ifndef Q_OS_WASM
    const QFileInfo shFi = findSh();
    Q_ASSERT(shFi.exists());
    const QString shPath = shFi.absoluteFilePath();
    QTest::newRow("unix-sh")
        << QString() << QString::fromLatin1("sh") << shPath;
    QTest::newRow("unix-sh-fullpath")
        << QString() << shPath << shPath;
    QTest::newRow("unix-sh-relativepath")
        << QString(shFi.absolutePath()) << QString::fromLatin1("./sh") << shPath;
#endif /* !WASM */
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
#ifdef Q_OS_WASM
    QSKIP("No applicationdir on wasm");
#else
    const QString appPath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_ANDROID
    if (QtAndroidPrivate::isUncompressedNativeLibs())
        QSKIP("Can't create a link to applicationDir which points inside an APK on Android");
#endif
    // link to directory
    const QString target = QDir::tempPath() + QDir::separator() + QLatin1String("link.lnk");
    QFile::remove(target);
    QFile appFile(appPath);
    QVERIFY(appFile.link(target));
    QVERIFY(QStandardPaths::findExecutable(target).isEmpty());
    QFile::remove(target);
#endif
}

using RuntimeDirSetup = std::optional<QString> (*)(QDir &);
Q_DECLARE_METATYPE(RuntimeDirSetup);

void tst_qstandardpaths::testRuntimeDirectory()
{
#ifdef Q_XDG_PLATFORM
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QVERIFY(!runtimeDir.isEmpty());
#endif
}

// INTEGRITY PJF System doesn't support user ID related APIs. getpwuid is not defined.
// testCustomRuntimeDirectory_data test will always FAIL for INTEGRITY.
#if defined(Q_XDG_PLATFORM) && !defined(Q_OS_INTEGRITY)
static QString fallbackXdgRuntimeDir()
{
    static QString username = [] {
        struct passwd *pw = getpwuid(geteuid());
        return QString::fromLocal8Bit(pw->pw_name);
    }();

    // QDir::temp() might change from call to call
    return QDir::temp().filePath("runtime-" + username);
}
#endif

[[maybe_unused]] static QString updateRuntimeDir(const QString &path)
{
    qputenv("XDG_RUNTIME_DIR", QFile::encodeName(path));
    return path;
}

[[maybe_unused]] static void clearRuntimeDir()
{
    qunsetenv("XDG_RUNTIME_DIR");
#ifdef Q_XDG_PLATFORM
#if !defined(Q_OS_WASM) && !defined(Q_OS_INTEGRITY)
    QTest::ignoreMessage(QtWarningMsg,
                         qPrintable("QStandardPaths: XDG_RUNTIME_DIR not set, defaulting to '"
                                    + fallbackXdgRuntimeDir() + '\''));
#endif
#endif
}

void tst_qstandardpaths::testCustomRuntimeDirectory_data()
{
#ifdef Q_OS_INTEGRITY
    QSKIP("Test requires getgid/getpwuid API that are not available on INTEGRITY");
#elif defined(Q_XDG_PLATFORM)
    QTest::addColumn<RuntimeDirSetup>("setup");
    auto addRow = [](const char *name, RuntimeDirSetup f) {
        QTest::newRow(name) << f;
    };


#  if defined(Q_OS_UNIX)
    if (::getuid() == 0)
        QSKIP("Running this test as root doesn't make sense");
#  endif

    addRow("environment:non-existing", [](QDir &d) -> std::optional<QString> {
        return updateRuntimeDir(d.filePath("runtime"));
    });

    addRow("environment:existing", [](QDir &d) -> std::optional<QString> {
        QString p = d.filePath("runtime");
        d.mkdir("runtime");
        QFile::setPermissions(p, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        return updateRuntimeDir(p);
    });

    addRow("environment-to-existing-wrong-perm", [](QDir &d) -> std::optional<QString> {
        QString p = d.filePath("runtime");
        d.mkdir("runtime");
        QFile::setPermissions(p, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                 QFile::ExeGroup | QFile::ExeOther);
        updateRuntimeDir(p);
        QTest::ignoreMessage(QtWarningMsg,
                             QString("QStandardPaths: wrong permissions on runtime directory %1, "
                                     "0711 instead of 0700")
                             .arg(p).toLatin1());
        return fallbackXdgRuntimeDir();
    });

    addRow("environment:wrong-owner", [](QDir &) -> std::optional<QString> {
        QT_STATBUF st;
        QT_STAT("/", &st);

        updateRuntimeDir("/");
        QTest::ignoreMessage(QtWarningMsg,
                             QString("QStandardPaths: runtime directory '/' is not owned by UID "
                                     "%1, but a directory permissions %2 owned by UID %3 GID %4")
                             .arg(getuid())
                             .arg(st.st_mode & 07777, 4, 8, QChar('0'))
                             .arg(st.st_uid)
                             .arg(st.st_gid).toLatin1());
        return fallbackXdgRuntimeDir();
    });

    // static so that it can be used in RuntimeDirSetup callable without capturing
    static auto failedToOpen = [](const QFile &f) {
        qCritical("QFile::Open: failed to open '%s': %s",
                  qPrintable(f.fileName()), qPrintable(f.errorString()));
        return std::nullopt;
    };

    addRow("environment:file", [](QDir &d) -> std::optional<QString> {
        QString p = d.filePath("file");
        QFile f(p);
        if (!f.open(QIODevice::WriteOnly))
            return failedToOpen(f);
        f.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

        updateRuntimeDir(p);
        QTest::ignoreMessage(QtWarningMsg,
                             QString("QStandardPaths: runtime directory '%1' is not a directory, "
                                     "but a regular file permissions 0600 owned by UID %2 GID %3")
                             .arg(p).arg(getuid()).arg(getgid()).toLatin1());
        return fallbackXdgRuntimeDir();
    });

    addRow("environment:broken-symlink", [](QDir &d) -> std::optional<QString> {
        QString p = d.filePath("link");
        QFile::link(d.filePath("this-goes-nowhere"), p);
        updateRuntimeDir(p);
        QTest::ignoreMessage(QtWarningMsg,
                             QString("QStandardPaths: runtime directory '%1' is not a directory, "
                                     "but a broken symlink")
                             .arg(p).toLatin1());
        return fallbackXdgRuntimeDir();
    });

    addRow("environment:symlink-to-dir", [](QDir &d) -> std::optional<QString> {
        QString p = d.filePath("link");
        d.mkdir("dir");
        QFile::link(d.filePath("dir"), p);
        QFile::setPermissions(p, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        updateRuntimeDir(p);
        QTest::ignoreMessage(QtWarningMsg,
                             QString("QStandardPaths: runtime directory '%1' is not a directory, "
                                     "but a symbolic link to a directory permissions 0700 owned by UID %2 GID %3")
                             .arg(p).arg(getuid()).arg(getgid()).toLatin1());
        return fallbackXdgRuntimeDir();
    });

    addRow("no-environment:non-existing", [](QDir &) -> std::optional<QString> {
        clearRuntimeDir();
        return fallbackXdgRuntimeDir();
    });

    addRow("no-environment:existing", [](QDir &d) -> std::optional<QString> {
        clearRuntimeDir();
        QString p = fallbackXdgRuntimeDir();
        d.mkdir(p);         // probably has wrong permissions
        QFile::setPermissions(p, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        return p;
    });

    addRow("no-environment:fallback-is-file", [](QDir &) -> std::optional<QString> {
        QString p = fallbackXdgRuntimeDir();
        QFile f(p);
        if (!f.open(QIODevice::WriteOnly))
            return failedToOpen(f);
        f.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

        clearRuntimeDir();
        QTest::ignoreMessage(QtWarningMsg,
                             QString("QStandardPaths: runtime directory '%1' is not a directory, "
                                     "but a regular file permissions 0600 owned by UID %2 GID %3")
                             .arg(p).arg(getuid()).arg(getgid()).toLatin1());
        return QString();
    });

    addRow("environment-and-fallback-are-files", [](QDir &d) -> std::optional<QString> {
        QString p = d.filePath("file1");
        QFile f(p);
        if (!f.open(QIODevice::WriteOnly))
            return failedToOpen(f);
        f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup);
        updateRuntimeDir(p);
        QTest::ignoreMessage(QtWarningMsg,
                             QString("QStandardPaths: runtime directory '%1' is not a directory, "
                                     "but a regular file permissions 0640 owned by UID %2 GID %3")
                             .arg(p).arg(getuid()).arg(getgid()).toLatin1());

        f.close();
        f.setFileName(fallbackXdgRuntimeDir());
        if (!f.open(QIODevice::WriteOnly))
            return failedToOpen(f);
        f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup);
        QTest::ignoreMessage(QtWarningMsg,
                             QString("QStandardPaths: runtime directory '%1' is not a directory, "
                                     "but a regular file permissions 0640 owned by UID %2 GID %3")
                             .arg(f.fileName()).arg(getuid()).arg(getgid()).toLatin1());

        return QString();
    });
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
        ~EnvVarRestorer()
        {
            qputenv("XDG_RUNTIME_DIR", origRuntimeDir);
            qputenv("TMPDIR", origTempDir);
        }
        const QByteArray origRuntimeDir = qgetenv("XDG_RUNTIME_DIR");
        const QByteArray origTempDir = qgetenv("TMPDIR");
    };
    EnvVarRestorer restorer;

    // set up the environment to point to a place we control
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), qPrintable(tempDir.errorString()));

    QDir d(tempDir.path());
    qputenv("TMPDIR", QFile::encodeName(tempDir.path()));

    QFETCH(RuntimeDirSetup, setup);
    std::optional<QString> opt = setup(d);
    QVERIFY(opt);
    QString expected = *opt;

    QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QCOMPARE(runtimeDir, expected);

    if (!runtimeDir.isEmpty()) {
        QFileInfo runtimeInfo(runtimeDir);
        QVERIFY(runtimeInfo.isDir());
        QVERIFY(!runtimeInfo.isSymLink());
        auto expectedPerms = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                | QFile::ReadUser | QFile::WriteUser | QFile::ExeUser;
        QCOMPARE(QString::number(runtimeInfo.permissions(), 16),
                 QString::number(expectedPerms, 16));
    }
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
#if QT_CONFIG(regularexpression)
    const QRegularExpression filter(QStringLiteral("\\\\"));
    QVERIFY(filter.isValid());
    for (int i = 0; i <= QStandardPaths::GenericCacheLocation; ++i) {
        const QStringList paths = QStandardPaths::standardLocations(QStandardPaths::StandardLocation(i));
        QVERIFY2(paths.filter(filter).isEmpty(),
                 qPrintable(QString::fromLatin1("Backslash found in %1 %2")
                            .arg(i).arg(paths.join(QLatin1Char(',')))));
    }
#else
    QSKIP("regularexpression feature disabled");
#endif
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

    const QString uncleanGlobalConfigDir = "/./" + QFile::encodeName(m_globalConfigDir);
    qputenv("XDG_CONFIG_DIRS", QFile::encodeName(uncleanGlobalConfigDir) + "::relative/path");
    const QStringList configDirs = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    QVERIFY(!configDirs.contains("relative/path"_L1));
    QVERIFY(!configDirs.contains(""_L1));

    // Relative paths in XDG_* env vars are ignored
    const QString relative("./someRelativeDir");

    qputenv("XDG_CACHE_HOME", relative.toLatin1());
    const QString cacheDir = cacheLoc();
    QCOMPARE_NE(cacheDir, relative);

    qputenv("XDG_STATE_HOME", relative.toLatin1());
    const QString stateDir = stateLoc();
    QCOMPARE_NE(stateDir, relative);

    qputenv("XDG_DATA_HOME", relative.toLatin1());
    const QString localDataDir = genericDataLoc();
    QCOMPARE_NE(localDataDir, relative);

    qputenv("XDG_CONFIG_HOME", relative.toLatin1());
    const QString localConfig = configLoc();
    QCOMPARE_NE(localConfig, relative);

    qputenv("XDG_RUNTIME_DIR", relative.toLatin1());
    const QString runtimeDir = genericDataLoc();
    QCOMPARE_NE(runtimeDir, relative);
#endif
}

QTEST_MAIN(tst_qstandardpaths)

#include "tst_qstandardpaths.moc"
