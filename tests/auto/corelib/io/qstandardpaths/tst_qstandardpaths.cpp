/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_BLACKBERRY)
#define Q_XDG_PLATFORM
#endif

class tst_qstandardpaths : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultLocations();
    void testCustomLocations();
    void enableTestMode();
    void testLocateAll();
    void testDataLocation();
    void testFindExecutable_data();
    void testFindExecutable();
    void testFindExecutableLinkToDirectory();
    void testRuntimeDirectory();
    void testCustomRuntimeDirectory();
    void testAllWritableLocations_data();
    void testAllWritableLocations();
    void testCleanPath();

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

void tst_qstandardpaths::testDefaultLocations()
{
#ifdef Q_XDG_PLATFORM
    setDefaultLocations();

    const QString expectedConfHome = QDir::homePath() + QString::fromLatin1("/.config");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation), expectedConfHome);
    const QStringList confDirs = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    QCOMPARE(confDirs.count(), 2);
    QVERIFY(confDirs.contains(expectedConfHome));

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
    QStandardPaths::enableTestMode(true);
    QVERIFY(QStandardPaths::isTestModeEnabled());

#ifdef Q_XDG_PLATFORM
    setCustomLocations(); // for the global config dir
    const QString qttestDir = QDir::homePath() + QLatin1String("/.qttest");

    // ConfigLocation
    const QString configDir = qttestDir + QLatin1String("/config");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation), configDir);
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
    testLocations.insert(QStandardPaths::DataLocation, QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    testLocations.insert(QStandardPaths::GenericDataLocation, QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    testLocations.insert(QStandardPaths::ConfigLocation, QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    testLocations.insert(QStandardPaths::CacheLocation, QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    testLocations.insert(QStandardPaths::GenericCacheLocation, QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation));
    // On Windows, what should "Program Files" become, in test mode?
    //testLocations.insert(QStandardPaths::ApplicationsLocation, QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation));

    QStandardPaths::enableTestMode(false);

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
    // Blackberry OS is an exception to this case, owing to the fact that
    // applications are sandboxed.
#ifndef Q_OS_BLACKBERRY
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::DataLocation), base + "/tst_qstandardpaths");
    QCoreApplication::instance()->setOrganizationName("Qt");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::DataLocation), base + "/Qt/tst_qstandardpaths");
    QCoreApplication::instance()->setApplicationName("QtTest");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::DataLocation), base + "/Qt/QtTest");
#endif

#ifdef Q_XDG_PLATFORM
    setDefaultLocations();
    const QString expectedAppDataDir = QDir::homePath() + QString::fromLatin1("/.local/share/Qt/QtTest");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::DataLocation), expectedAppDataDir);
    const QStringList appDataDirs = QStandardPaths::standardLocations(QStandardPaths::DataLocation);
    QCOMPARE(appDataDirs.count(), 3);
    QCOMPARE(appDataDirs.at(0), expectedAppDataDir);
    QCOMPARE(appDataDirs.at(1), QString::fromLatin1("/usr/local/share/Qt/QtTest"));
    QCOMPARE(appDataDirs.at(2), QString::fromLatin1("/usr/share/Qt/QtTest"));
#endif
}

#ifndef Q_OS_WIN
// Find "sh" on Unix.
static inline QFileInfo findSh()
{
    const char *shPaths[] = {"/bin/sh", "/usr/bin/sh", 0};
    for (const char **shPath = shPaths; *shPath; ++shPath) {
        const QFileInfo fi = QFileInfo(QLatin1String(*shPath));
        if (fi.exists())
            return fi;
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

    if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS8) {
        // The logo executable on Windows 8 is perfectly suited for testing that the
        // suffix mechanism is not thrown off by dots in the name.
        const QString logo = QLatin1String("microsoft.windows.softwarelogo.showdesktop");
        const QString logoPath = cmdFi.absolutePath() + QLatin1Char('/') + logo + QLatin1String(".exe");
        QTest::newRow("win8-logo")
            << QString() << (logo + QLatin1String(".exe")) << logoPath;
        QTest::newRow("win8-logo-nosuffix")
            << QString() << logo << logoPath;
    }
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
    // link to directory
    const QString target = QDir::tempPath() + QDir::separator() + QLatin1String("link.lnk");
    QFile::remove(target);
    QFile appFile(QCoreApplication::applicationDirPath());
    QVERIFY(appFile.link(target));
    QVERIFY(QStandardPaths::findExecutable(target).isEmpty());
    QFile::remove(target);
}

void tst_qstandardpaths::testRuntimeDirectory()
{
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QVERIFY(!runtimeDir.isEmpty());

    // Check that it can automatically fix permissions
#ifdef Q_XDG_PLATFORM
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
    qputenv("XDG_RUNTIME_DIR", QFile::encodeName("/tmp"));
    // It's very unlikely that /tmp is 0600 or that we can chmod it
    // The call below outputs
    //   "QStandardPaths: wrong ownership on runtime directory /tmp, 0 instead of $UID"
    // but we can't reliably expect that it's owned by uid 0, I think.
    const uid_t uid = geteuid();
    QTest::ignoreMessage(QtWarningMsg,
            qPrintable(QString::fromLatin1("QStandardPaths: wrong ownership on runtime directory /tmp, 0 instead of %1").arg(uid)));
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QVERIFY2(runtimeDir.isEmpty(), qPrintable(runtimeDir));
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
    QTest::newRow("DataLocation") << QStandardPaths::DataLocation;
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

QTEST_MAIN(tst_qstandardpaths)

#include "tst_qstandardpaths.moc"
