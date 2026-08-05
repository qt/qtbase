// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/private/qlibraryinfo_p.h>
#include <QStandardPaths>
#include <QDir>

#if defined(Q_OS_MACOS)
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <filesystem>
#endif

using namespace Qt::StringLiterals;

class tst_QLibraryInfo : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void path_data();
    void path();
    void paths();
    void relativePrefix();
    void merge();
    void mergeRelativePrefix();
    void mergeUnlistedPaths();
#if defined(Q_OS_MACOS)
    void bundle_data();
    void bundle();
    void bundleWithExternalQt();
    void bundleWithExternalQtAndQtConf();
#endif

#if defined(Q_OS_MACOS)
private:
    void createBundle(const QString &bundlePath, const QString &packageType,
                      const QString &subBundlePath, const QString &qtConf,
                      QString *executableDir);
    void runBundledHelper(const QString &executableDir, QJsonObject *paths);
#endif
};

void tst_QLibraryInfo::initTestCase()
{
#if !QT_CONFIG(settings)
    QSKIP("QSettings support is required for the test to run.");
#endif
}

void tst_QLibraryInfo::cleanup()
{
    QLibraryInfoPrivate::setQtconfManualPath(nullptr);
    QLibraryInfoPrivate::reload();
}

void tst_QLibraryInfo::path_data()
{
    QTest::addColumn<QString>("qtConfPath");
    QTest::addColumn<QLibraryInfo::LibraryPath>("path");
    QTest::addColumn<QString>("expected");

    // TODO: deal with bundle on macOs?
    QString baseDir = QCoreApplication::applicationDirPath();

    // empty means we fall-back to default entries
    QTest::addRow("empty_qmlimports") << ":/empty.qt.conf" << QLibraryInfo::QmlImportsPath << (baseDir + "/qml");
    QTest::addRow("empty_Data") << ":/empty.qt.conf" << QLibraryInfo::DataPath << baseDir;

    // partial override; use given entry if provided, otherwise default
    QTest::addRow("partial_qmlimports") << ":/partial.qt.conf" << QLibraryInfo::QmlImportsPath << "/path/to/myqml";
    QTest::addRow("partial_Data") << ":/partial.qt.conf" << QLibraryInfo::DataPath << baseDir;
}

void tst_QLibraryInfo::path()
{
    QFETCH(QString, qtConfPath);
    QFETCH(QLibraryInfo::LibraryPath, path);
    QFETCH(QString, expected);

    QLibraryInfoPrivate::setQtconfManualPath(&qtConfPath);
    QLibraryInfoPrivate::reload();
    QString value = QLibraryInfo::path(path);
    QCOMPARE(value, expected);

    // check consistency with paths
    auto values = QLibraryInfo::paths(path);
    QVERIFY(!values.isEmpty());
    QCOMPARE(values.first(), expected);
}

void tst_QLibraryInfo::paths()
{
    QString qtConfPath(u":/list.qt.conf");
    QLibraryInfoPrivate::setQtconfManualPath(&qtConfPath);
    QLibraryInfoPrivate::reload();

    // The qt.conf is user-written, in that it doesn't merge with the Qt build
    // defaults, so it declares the layout the application follows, and replaces
    // the paths of the Qt build we run against: the paths it lists come first,
    // followed by the generic layout at the application prefix for the locations
    // it leaves out, and nothing else.
    //
    // The qt.conf gives an absolute Prefix, which is used verbatim as the
    // application prefix, so we know exactly what an app-prefixed path looks like
    // on every platform. It also keeps the application prefix from being detected
    // as a bundle, which would add the modern Apple suffixes to the app paths and
    // make the expectations below platform dependent, as every application is a
    // bundle on iOS.
    const QString appPrefix = u"/nonexistent/list/prefix"_s;
    QCOMPARE(QLibraryInfo::paths(QLibraryInfo::PrefixPath), QStringList { appPrefix });

    const auto expectedPaths = [&](const QStringList &listed, const QString &suffix) {
        return listed + QStringList { appPrefix + '/' + suffix };
    };

    const QStringList docPaths = QLibraryInfo::paths(QLibraryInfo::DocumentationPath);
    QCOMPARE(docPaths, expectedPaths({
        "/path/to/mydoc", "/path/to/anotherdoc", appPrefix + "/relativePath"
    }, "doc"));

    const QStringList qmlImportPaths = QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);
    QCOMPARE(qmlImportPaths, expectedPaths({
        ":/a/resource/path", ":a/broken/path", appPrefix + "/a/relative/path"
    }, "qml"));
}

void tst_QLibraryInfo::relativePrefix()
{
    QString qtConfPath(u":/relative-prefix.qt.conf");
    QLibraryInfoPrivate::setQtconfManualPath(&qtConfPath);
    QLibraryInfoPrivate::reload();

    // A static and relocatable Qt embeds a qt.conf like this one into its own
    // tools, with a Prefix relative to the bin directory the tool lives in.
    // The Prefix is resolved against the application directory, once, and must
    // not be applied a second time to the application prefix it produced.
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString appPrefix = QDir::cleanPath(appDir + "/.."_L1);
    QCOMPARE(QLibraryInfo::paths(QLibraryInfo::PrefixPath), QStringList { appPrefix });

    // The locations the qt.conf leaves out are rooted at that prefix, as a
    // user-written qt.conf declares the layout the application follows.
    QCOMPARE(QLibraryInfo::path(QLibraryInfo::QmlImportsPath), appPrefix + "/qml"_L1);
}

void tst_QLibraryInfo::merge()
{
    QString qtConfPath(u":/merge.qt.conf");
    QLibraryInfoPrivate::setQtconfManualPath(&qtConfPath);
    QLibraryInfoPrivate::reload();

    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString docPath = QLibraryInfo::path(QLibraryInfo::DocumentationPath);
    // we can't know where exactly the doc path points, but it should not point to ${baseDir}/doc,
    // which would be the  behavior without merge_qt_conf
    QCOMPARE_NE(docPath, baseDir + "/doc");

    QList<QString> values = QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);
    QCOMPARE(values.size(), 2); // custom entry + Qt default entry
    QCOMPARE(values[0], "/path/to/myqml");
}

void tst_QLibraryInfo::mergeRelativePrefix()
{
    QString qtConfPath(u":/merge-relative-prefix.qt.conf");
    QLibraryInfoPrivate::setQtconfManualPath(&qtConfPath);
    QLibraryInfoPrivate::reload();

    // A relative Prefix in qt.conf is resolved against the application prefix,
    // so the first PrefixPath ends up pointing at the application directory.
    const QString prefixPath = QLibraryInfo::paths(QLibraryInfo::PrefixPath).first();

    // The Qt configure fallback path suffixes (e.g. "qml") must still be
    // rooted at the Qt prefix, not at the application prefix.
    const QString qmlImportsPath = QLibraryInfo::paths(QLibraryInfo::QmlImportsPath).last();
    QCOMPARE_NE(qmlImportsPath, prefixPath + "/qml");
}

void tst_QLibraryInfo::mergeUnlistedPaths()
{
    QString qtConfPath(u":/merge-absolute-prefix.qt.conf");
    QLibraryInfoPrivate::setQtconfManualPath(&qtConfPath);
    QLibraryInfoPrivate::reload();

    // An absolute Prefix becomes the application prefix verbatim, independently
    // of where the application lives, so we know exactly what an app-prefixed
    // path looks like. It also keeps the application prefix from being detected
    // as a bundle, which would report app paths on its own.
    const QString appPrefix = QStringLiteral("/nonexistent/app/prefix");
    QCOMPARE(QLibraryInfo::path(QLibraryInfo::PrefixPath), appPrefix);

    // The one location the qt.conf does list is still honored.
    QCOMPARE(QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
             QStringLiteral("/path/to/myqml"));

    // A merging qt.conf is an overlay: it contributes only the locations it
    // lists, and we report nothing of our own for the rest, so they resolve
    // against the Qt prefix. Rooting the generic default at the app prefix
    // instead would shadow the Qt path, and path() reports the first one only.
    const QLibraryInfo::LibraryPath unlisted[] = {
        QLibraryInfo::BinariesPath,
        QLibraryInfo::LibraryExecutablesPath,
        QLibraryInfo::DocumentationPath,
    };
    for (const QLibraryInfo::LibraryPath location : unlisted) {
        const QStringList paths = QLibraryInfo::paths(location);
        QVERIFY(!paths.isEmpty());
        for (const QString &path : paths) {
            QVERIFY2(!path.startsWith(appPrefix),
                     qPrintable("Unlisted location was rooted at the app prefix: " + path));
        }
    }
}

#if defined(Q_OS_MACOS)
void tst_QLibraryInfo::bundle_data()
{
    QTest::addColumn<QString>("bundleName");
    QTest::addColumn<QString>("packageType");
    QTest::addColumn<QString>("subBundlePath");

    QTest::newRow("regular-app") << "regular-app.app" << "APPL" << QString();
    QTest::newRow("app-with-helper") << "app-with-helper.app" << "APPL" << QString();
    QTest::newRow("generic-bundle") << "generic.bundle" << "BNDL" << QString();
    QTest::newRow("app-extension") << "app-extension.appex" << "XPC!" << QString();
    QTest::newRow("audio-unit-v2") << "audio-unit-v2.component" << "BNDL" << QString();
    QTest::newRow("audio-unit-v3") << "audio-unit-v3.app" << "APPL"
                                   << "Contents/PlugIns/audio-unit-v3.appex";
}

/*
    The paths the bundlehelper reported for the \a location key of \a paths.
*/
static QStringList pathsFor(const QJsonObject &paths, const QString &location)
{
    QStringList list;
    for (const QJsonValue &value : paths.value(location).toArray())
        list << value.toString();
    return list;
}

/*
    Creates an app bundle at \a bundlePath, of the given \a packageType,
    containing the bundlehelper executable, optionally in a nested bundle at
    \a subBundlePath, and optionally with the given \a qtConf written to the
    bundle's Resources directory. Reports the directory the bundled executable
    ended up in via \a executableDir.
*/
void tst_QLibraryInfo::createBundle(const QString &bundlePath, const QString &packageType,
                                    const QString &subBundlePath, const QString &qtConf,
                                    QString *executableDir)
{
    *executableDir = (subBundlePath.isEmpty()
            ? bundlePath : bundlePath + '/' + subBundlePath) + "/Contents/MacOS";
    QVERIFY(QDir().mkpath(*executableDir));

    // Write the PkgInfo file describing the bundle's package type

    QFile pkgInfo(bundlePath + "/Contents/PkgInfo");
    QVERIFY2(pkgInfo.open(QIODevice::WriteOnly), qPrintable(pkgInfo.errorString()));
    pkgInfo.write(packageType.toUtf8() + "TQTC");
    pkgInfo.close();

    // Write the qt.conf, if any, where CFBundleCopyResourceURL will find it

    if (!qtConf.isEmpty()) {
        const QString resourcesDir = bundlePath + "/Contents/Resources";
        QVERIFY(QDir().mkpath(resourcesDir));
        QFile qtConfFile(resourcesDir + "/qt.conf");
        QVERIFY2(qtConfFile.open(QIODevice::WriteOnly), qPrintable(qtConfFile.errorString()));
        qtConfFile.write(qtConf.toUtf8());
        qtConfFile.close();
    }

    // Copy helper to bundle

    const QString helper = QCoreApplication::applicationDirPath() + "/bundlehelper";
    const QString bundledHelper = *executableDir + "/bundlehelper";
    QVERIFY2(QFile::copy(helper, bundledHelper),
             qPrintable("Failed to copy " + helper + " to " + bundledHelper));
}

/*
    Runs the bundlehelper in \a executableDir and reports the library paths it
    sees via \a paths, keyed by QLibraryInfo::LibraryPath enum name.
*/
void tst_QLibraryInfo::runBundledHelper(const QString &executableDir, QJsonObject *paths)
{
    QProcess process;
    process.start(executableDir + "/bundlehelper", QStringList());
    QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
    QVERIFY2(process.waitForFinished(), qPrintable(process.errorString()));
    QVERIFY2(process.exitStatus() == QProcess::NormalExit,
             qPrintable(process.readAllStandardError()));
    QCOMPARE(process.exitCode(), 0);

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(
        process.readAllStandardOutput(), &error);
    QVERIFY2(error.error == QJsonParseError::NoError, qPrintable(error.errorString()));
    QVERIFY(document.isObject());

    *paths = document.object();
    QVERIFY(!paths->isEmpty());
}

void tst_QLibraryInfo::bundle()
{
    QFETCH(QString, bundleName);
    QFETCH(QString, packageType);
    QFETCH(QString, subBundlePath);

    // Create bundle

    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), qPrintable(tempDir.errorString()));
    auto cleanup = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            tempDir.setAutoRemove(false);
            qDebug() << tempDir.path();
        }
    });

    const QString bundlePath = tempDir.filePath(bundleName);
    QString executableDir;
    createBundle(bundlePath, packageType, subBundlePath, QString(), &executableDir);
    if (QTest::currentTestFailed())
        return;

#if !QT_CONFIG(static)
    // Relocate Qt into the bundle

    const QString bundledHelper = executableDir + "/bundlehelper";

    namespace fs = std::filesystem;
    const QString librariesPath = QLibraryInfo::paths(QLibraryInfo::LibrariesPath).last();
    const QString libraryName = QString::fromUtf8(QTCORE_LIBRARY_NAME);
    const fs::path librarySource = fs::path(librariesPath.toStdString()) / libraryName.toStdString();

    const QString frameworksDir = bundlePath + "/Contents/Frameworks";
    QVERIFY(QDir().mkpath(frameworksDir));

    const fs::path destination = fs::path(frameworksDir.toStdString()) / libraryName.toStdString();
    std::error_code ec;
    if (fs::is_directory(librarySource)) {
        fs::copy(librarySource, destination,
                 fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
    } else {
        fs::copy_file(librarySource, destination, ec);
    }
    QVERIFY2(!ec, ec.message().c_str());

    // Update rpath

    const QString buildRPath = librariesPath;
    const QString bundleRPath = "@executable_path/" + QDir(executableDir).relativeFilePath(frameworksDir);

    QProcess installNameTool;
    installNameTool.start("install_name_tool", { "-rpath", buildRPath, bundleRPath, bundledHelper });
    QVERIFY2(installNameTool.waitForStarted(), qPrintable(installNameTool.errorString()));
    QVERIFY2(installNameTool.waitForFinished(), qPrintable(installNameTool.errorString()));
    QVERIFY2(installNameTool.exitCode() == 0, qPrintable(installNameTool.readAllStandardError()));
#endif // !QT_CONFIG(static)

    // Launch helper and check output

    QJsonObject paths;
    runBundledHelper(executableDir, &paths);
    if (QTest::currentTestFailed())
        return;

    // Verify the app prefix is reported, and resolves inside the bundle

    const QString canonicalBundlePath = QFileInfo(bundlePath).canonicalFilePath();
    QVERIFY(!canonicalBundlePath.isEmpty());

    const QJsonArray prefixPaths = paths.value("PrefixPath").toArray();
    QVERIFY(!prefixPaths.isEmpty());

    // The app prefix is the Contents directory of the bundle the executable
    // lives in. We look it up among the reported prefix paths rather than
    // assuming a position, as the Qt prefix may take priority over it.
    const QString expectedAppPrefix = QFileInfo(executableDir + "/..").canonicalFilePath();
    QVERIFY2(expectedAppPrefix.startsWith(canonicalBundlePath),
             qPrintable("App prefix '" + expectedAppPrefix + "' is not inside bundle '"
                        + canonicalBundlePath + "'"));

    QStringList canonicalPrefixPaths;
    for (const QJsonValue &value : prefixPaths)
        canonicalPrefixPaths << QFileInfo(value.toString()).canonicalFilePath();

    QString appPrefix;
    for (const QJsonValue &value : prefixPaths) {
        if (QFileInfo(value.toString()).canonicalFilePath() == expectedAppPrefix) {
            appPrefix = value.toString();
            break;
        }
    }
    QVERIFY2(!appPrefix.isEmpty(),
             qPrintable("App prefix '" + expectedAppPrefix + "' not in prefix paths: "
                        + canonicalPrefixPaths.join(", ")));

#if !QT_CONFIG(static)
    // Qt was relocated into the bundle, so all the prefix paths are in there

    for (const QString &prefixPath : canonicalPrefixPaths) {
        QVERIFY2(prefixPath.startsWith(canonicalBundlePath),
                 qPrintable("Prefix path '" + prefixPath + "' is not inside bundle '"
                            + canonicalBundlePath + "'"));
    }

    if (!subBundlePath.isEmpty()) {
        // Verify that both the main bundle and sub bundle paths are present in the prefix paths
        const QString mainBundlePrefix = QFileInfo(bundlePath + "/Contents").canonicalFilePath();
        QVERIFY2(canonicalPrefixPaths.contains(mainBundlePrefix),
                 qPrintable("Main bundle prefix '" + mainBundlePrefix
                            + "' not in prefix paths: " + canonicalPrefixPaths.join(", ")));

        const QString subBundlePrefix = QFileInfo(bundlePath + '/'
            + subBundlePath + "/Contents").canonicalFilePath();
        QVERIFY2(canonicalPrefixPaths.contains(subBundlePrefix),
                 qPrintable("Sub bundle prefix '" + subBundlePrefix
                            + "' not in prefix paths: " + canonicalPrefixPaths.join(", ")));
    }
#endif

    // Verify that the libraries and plugins follow modern bundle conventions,
    // rooted at the application prefix

    const QStringList libraryPaths = pathsFor(paths, "LibrariesPath");
    QVERIFY2(libraryPaths.contains(QDir::cleanPath(appPrefix + "/Frameworks")),
             qPrintable("Frameworks not in LibrariesPath: " + libraryPaths.join(", ")));

    const QStringList pluginPaths = pathsFor(paths, "PluginsPath");
    QVERIFY2(pluginPaths.contains(QDir::cleanPath(appPrefix + "/PlugIns")),
             qPrintable("PlugIns not in PluginsPath: " + pluginPaths.join(", ")));

    // Verify that QML import paths include the QML resources

    const QStringList qmlImportPaths = pathsFor(paths, "QmlImportsPath");
    const QString expectedQmlImport = QDir::cleanPath(appPrefix + "/Resources/qml");
    QVERIFY2(qmlImportPaths.contains(expectedQmlImport),
             qPrintable(expectedQmlImport + " not in QmlImportsPath: " + qmlImportPaths.join(", ")));
}

/*
    An app bundle that links an external Qt, an app under development for
    example, must report the paths of the Qt it runs against first. This is what
    the Qt tools asking QLibraryInfo where the Qt they run against lives depend
    on, as QLibraryInfo::path() reports the first path only. The paths implied by
    the app's own bundle layout are still reported, as further candidates.
*/
void tst_QLibraryInfo::bundleWithExternalQt()
{
#if QT_CONFIG(static)
    QSKIP("A static build of Qt is never external to the app");
#else
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), qPrintable(tempDir.errorString()));
    auto cleanup = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            tempDir.setAutoRemove(false);
            qDebug() << tempDir.path();
        }
    });

    // Canonical, so that the paths the helper reports match ours verbatim
    const QString bundlePath = QFileInfo(tempDir.path()).canonicalFilePath() + "/external-qt.app";

    QString executableDir;
    createBundle(bundlePath, u"APPL"_s, QString(), QString(), &executableDir);
    if (QTest::currentTestFailed())
        return;

    QJsonObject paths;
    runBundledHelper(executableDir, &paths);
    if (QTest::currentTestFailed())
        return;

    // The test executable is a plain executable with no qt.conf of its own, so
    // the paths it reports are those of the Qt we, and the helper, run against.
    const QString appPrefix = bundlePath + "/Contents";

    const QStringList prefixPaths = pathsFor(paths, "PrefixPath");
    QCOMPARE(prefixPaths.first(), QLibraryInfo::path(QLibraryInfo::PrefixPath));
    QVERIFY2(prefixPaths.contains(appPrefix),
             qPrintable(appPrefix + " not in PrefixPath: " + prefixPaths.join(", ")));

    const QStringList binaryPaths = pathsFor(paths, "BinariesPath");
    QCOMPARE(binaryPaths.first(), QLibraryInfo::path(QLibraryInfo::BinariesPath));
    QVERIFY2(binaryPaths.contains(appPrefix + "/MacOS"),
             qPrintable(appPrefix + "/MacOS not in BinariesPath: " + binaryPaths.join(", ")));

    const QStringList qmlImportPaths = pathsFor(paths, "QmlImportsPath");
    QCOMPARE(qmlImportPaths.first(), QLibraryInfo::path(QLibraryInfo::QmlImportsPath));
    QVERIFY2(qmlImportPaths.contains(appPrefix + "/Resources/qml"),
             qPrintable(appPrefix + "/Resources/qml not in QmlImportsPath: "
                        + qmlImportPaths.join(", ")));

    // A bundle on its own only implies the modern Apple layout, not the Unixy
    // one, which nothing but a qt.conf roots at the application prefix.
    QVERIFY2(!binaryPaths.contains(appPrefix + "/bin"),
             qPrintable("Unexpected " + appPrefix + "/bin in BinariesPath: "
                        + binaryPaths.join(", ")));

    // The settings path has no bundle equivalent, and nothing implies the Unixy
    // layout here, so the app contributes nothing at all: system settings are
    // looked up where the Qt we run against says, not inside the app bundle.
    QCOMPARE(pathsFor(paths, "SettingsPath"),
             QStringList { QLibraryInfo::path(QLibraryInfo::SettingsPath) });
#endif
}

/*
    A user-written qt.conf, with explicit paths and no merging with the Qt build
    defaults, declares the layout the app follows, and replaces the paths of the
    Qt we run against, including for the locations the qt.conf leaves out. The
    modern Apple layout takes priority over the Unixy one there as well.
*/
void tst_QLibraryInfo::bundleWithExternalQtAndQtConf()
{
#if QT_CONFIG(static)
    QSKIP("A static build of Qt is never external to the app");
#else
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), qPrintable(tempDir.errorString()));
    auto cleanup = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            tempDir.setAutoRemove(false);
            qDebug() << tempDir.path();
        }
    });

    const QString bundlePath = QFileInfo(tempDir.path()).canonicalFilePath() + "/qt-conf.app";

    // The qt.conf lists the prefix only, so every other location is left out
    QString executableDir;
    createBundle(bundlePath, u"APPL"_s, QString(), u"[Paths]\nPrefix=.\n"_s, &executableDir);
    if (QTest::currentTestFailed())
        return;

    QJsonObject paths;
    runBundledHelper(executableDir, &paths);
    if (QTest::currentTestFailed())
        return;

    const QString appPrefix = bundlePath + "/Contents";

    // The lists are exhaustive: the Qt we run against is external to the bundle,
    // and the qt.conf pointed away from it, so none of its paths show up.

    QCOMPARE(pathsFor(paths, "PrefixPath"), QStringList({ appPrefix }));

    QCOMPARE(pathsFor(paths, "BinariesPath"), QStringList({
        appPrefix + "/MacOS", appPrefix + "/bin"
    }));

    QCOMPARE(pathsFor(paths, "QmlImportsPath"), QStringList({
        appPrefix + "/Resources/qml", appPrefix + "/qml"
    }));
#endif
}
#endif

QTEST_GUILESS_MAIN(tst_QLibraryInfo)

#include "tst_qlibraryinfo.moc"
