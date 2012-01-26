/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qstandardpaths.h>
#include <qdebug.h>
#include <qstandardpaths.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#define Q_XDG_PLATFORM
#endif

class tst_qstandardpaths : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultLocations();
    void testCustomLocations();
    void testLocateAll();
    void testDataLocation();
    void testFindExecutable();
    void testRuntimeDirectory();
    void testCustomRuntimeDirectory();
    void testAllWritableLocations_data();
    void testAllWritableLocations();

private:
    void setCustomLocations() {
        m_thisDir = QFileInfo(QFINDTESTDATA("tst_qstandardpaths.cpp")).absolutePath();

        qputenv("XDG_CONFIG_HOME", QFile::encodeName(m_thisDir));
        QDir parent(m_thisDir);
        parent.cdUp();
        m_globalDir = parent.path(); // gives us a trailing slash
        qputenv("XDG_CONFIG_DIRS", QFile::encodeName(m_globalDir));
    }
    QString m_thisDir;
    QString m_globalDir;
};

void tst_qstandardpaths::testDefaultLocations()
{
#ifdef Q_XDG_PLATFORM
    qputenv("XDG_CONFIG_HOME", QByteArray());
    qputenv("XDG_CONFIG_DIRS", QByteArray());
    const QString expectedConfHome = QDir::homePath() + QString::fromLatin1("/.config");
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation), expectedConfHome);
    const QStringList confDirs = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    QCOMPARE(confDirs.count(), 2);
    QVERIFY(confDirs.contains(expectedConfHome));

    qputenv("XDG_DATA_HOME", QByteArray());
    qputenv("XDG_DATA_DIRS", QByteArray());
    const QStringList genericDataDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QCOMPARE(genericDataDirs.count(), 3);
    const QString expectedDataHome = QDir::homePath() + QString::fromLatin1("/.local/share");
    QCOMPARE(genericDataDirs.at(0), expectedDataHome);
    QCOMPARE(genericDataDirs.at(1), QString::fromLatin1("/usr/local/share"));
    QCOMPARE(genericDataDirs.at(2), QString::fromLatin1("/usr/share"));
#endif
}

void tst_qstandardpaths::testCustomLocations()
{
#ifdef Q_XDG_PLATFORM
    setCustomLocations();

    // test writableLocation()
    QCOMPARE(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation), m_thisDir);

    QString thisDir_base = QFileInfo(m_thisDir).fileName();

    // test locate()
    const QString thisFileName = QString::fromLatin1("tst_qstandardpaths.cpp");
    QVERIFY(QFile::exists(m_thisDir + '/' + thisFileName));
    const QString thisFile = QStandardPaths::locate(QStandardPaths::ConfigLocation, thisFileName);
    QVERIFY(!thisFile.isEmpty());
    QVERIFY(thisFile.endsWith(thisFileName));

    const QString dir = QStandardPaths::locate(QStandardPaths::ConfigLocation, QString::fromLatin1("../") + thisDir_base, QStandardPaths::LocateDirectory);
    QVERIFY(!dir.isEmpty());
    const QString thisDirAsFile = QStandardPaths::locate(QStandardPaths::ConfigLocation, QString::fromLatin1("../") + thisDir_base);
    QVERIFY(thisDirAsFile.isEmpty()); // not a file

    const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    QCOMPARE(dirs, QStringList() << m_thisDir << m_globalDir);
#endif
}

// We really need QTemporaryDir for this test...

void tst_qstandardpaths::testLocateAll()
{
#ifdef Q_XDG_PLATFORM
    const QStringList appsDirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "applications", QStandardPaths::LocateDirectory);
    foreach (const QString &dir, appsDirs)
        QVERIFY2(dir.endsWith(QLatin1String("/share/applications")), qPrintable(dir));

    setCustomLocations();
    const QStringList allFiles = QStandardPaths::locateAll(QStandardPaths::ConfigLocation, "qstandardpaths.pro");
    QVERIFY2(allFiles.count() > 0, "qstandardpaths.pro not found in config locations!");
    QCOMPARE(allFiles.first(), QString(m_thisDir + QString::fromLatin1("/qstandardpaths.pro")));
#endif
}

void tst_qstandardpaths::testDataLocation()
{
    // On all platforms, DataLocation should be GenericDataLocation / organization name / app name
    // This allows one app to access the data of another app.
    {
        const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        const QString app = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        QCOMPARE(base, app);
    }
    QCoreApplication::instance()->setOrganizationName("Qt");
    QCoreApplication::instance()->setApplicationName("QtTest");
    {
        const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        const QString app = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        QCOMPARE(app, base + "/Qt/QtTest");
    }
}

void tst_qstandardpaths::testFindExecutable()
{
    // Search for 'sh' on unix and 'cmd.exe' on Windows
#ifdef Q_OS_WIN
    const QString exeName = "cmd.exe";
#else
    const QString exeName = "sh";
#endif

    const QString result = QStandardPaths::findExecutable(exeName);
    QVERIFY(!result.isEmpty());
#ifdef Q_OS_WIN
    QVERIFY(result.endsWith("/cmd.exe"));
#else
    QVERIFY(result.endsWith("/bin/sh"));
#endif

    // full path as argument
    QCOMPARE(QStandardPaths::findExecutable(result), result);

    // exe no found
    QVERIFY(QStandardPaths::findExecutable("idontexist").isEmpty());
    QVERIFY(QStandardPaths::findExecutable("").isEmpty());

    // link to directory
    const QString target = QDir::tempPath() + QDir::separator() + QLatin1String("link.lnk");
    QFile::remove(target);
    QFile appFile(QCoreApplication::applicationDirPath());
    QVERIFY(appFile.link(target));
    QVERIFY(QStandardPaths::findExecutable(target).isEmpty());
    QFile::remove(target);

    // findExecutable with a relative path
#ifdef Q_OS_UNIX
    const QString pwd = QDir::currentPath();
    QDir::setCurrent("/bin");
    QStringList possibleResults;
    possibleResults << QString::fromLatin1("/bin/sh") << QString::fromLatin1("/usr/bin/sh");
    const QString sh = QStandardPaths::findExecutable("./sh");
    QVERIFY2(possibleResults.contains(sh), qPrintable(sh));
    QDir::setCurrent(pwd);
#endif
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

QTEST_MAIN(tst_qstandardpaths)

#include "tst_qstandardpaths.moc"
