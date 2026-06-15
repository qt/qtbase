// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/private/qlibraryinfo_p.h>
#include <QStandardPaths>

#if defined(Q_OS_MACOS)
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <filesystem>
#endif

class tst_QLibraryInfo : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void path_data();
    void path();
    void paths();
    void merge();
    void mergeRelativePrefix();
#if defined(Q_OS_MACOS)
    void bundle_data();
    void bundle();
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

    QList<QString> values = QLibraryInfo::paths(QLibraryInfo::DocumentationPath);
    QCOMPARE(values.length(), 3);
    QCOMPARE(values[0], "/path/to/mydoc");
    QCOMPARE(values[1], "/path/to/anotherdoc");
    QString baseDir = QCoreApplication::applicationDirPath();
    QCOMPARE(values[2], baseDir + "/relativePath");

    const QStringList qmlImportPaths = QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);
    const QStringList expected = {
        ":/a/resource/path", ":a/broken/path", baseDir + "/a/relative/path"
    };
    QCOMPARE(qmlImportPaths, expected);
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

void tst_QLibraryInfo::bundle()
{
    QFETCH(QString, bundleName);
    QFETCH(QString, packageType);
    QFETCH(QString, subBundlePath);

    const QString helper = QCoreApplication::applicationDirPath() + "/bundlehelper";

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
    const QString executableDir = (subBundlePath.isEmpty()
            ? bundlePath : bundlePath + '/' + subBundlePath) + "/Contents/MacOS";
    QVERIFY(QDir().mkpath(executableDir));

    // Write the PkgInfo file describing the bundle's package type

    QFile pkgInfo(bundlePath + "/Contents/PkgInfo");
    QVERIFY2(pkgInfo.open(QIODevice::WriteOnly), qPrintable(pkgInfo.errorString()));
    pkgInfo.write(packageType.toUtf8() + "TQTC");
    pkgInfo.close();

    // Copy helper to bundle

    const QString bundledHelper = executableDir + "/bundlehelper";
    QVERIFY2(QFile::copy(helper, bundledHelper),
             qPrintable("Failed to copy " + helper + " to " + bundledHelper));

#if !QT_CONFIG(static)
    // Relocate Qt into the bundle

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

    // Launch helper

    QProcess process;
    process.start(bundledHelper, QStringList());
    QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
    QVERIFY2(process.waitForFinished(), qPrintable(process.errorString()));
    QVERIFY2(process.exitStatus() == QProcess::NormalExit, qPrintable(process.readAllStandardError()));
    QCOMPARE(process.exitCode(), 0);

    // Check output

    QByteArray output = process.readAllStandardOutput();

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(output, &error);
    QVERIFY2(error.error == QJsonParseError::NoError, qPrintable(error.errorString()));
    QVERIFY(document.isObject());

    const QJsonObject paths = document.object();
    QVERIFY(!paths.isEmpty());

    // Verify the prefix paths resolve inside the bundle

    const QString canonicalBundlePath = QFileInfo(bundlePath).canonicalFilePath();
    QVERIFY(!canonicalBundlePath.isEmpty());

    const QJsonArray prefixPaths = paths.value("PrefixPath").toArray();
    QVERIFY(!prefixPaths.isEmpty());
    for (const QJsonValue &value : prefixPaths) {
        const QString prefixPath = QFileInfo(value.toString()).canonicalFilePath();
        QVERIFY2(prefixPath.startsWith(canonicalBundlePath),
                 qPrintable("Prefix path '" + prefixPath + "' is not inside bundle '"
                            + canonicalBundlePath + "'"));
#if QT_CONFIG(static)
        // Static Qt builds with non-sandboxed apps will still report the Qt install prefix
        // as the Qt prefix, for compatibility reasons, so only check the app prefix here.
        break;
#endif
    }

#if !QT_CONFIG(static)
    if (!subBundlePath.isEmpty()) {
        // Verify that both the main bundle and sub bundle paths are present in the prefix paths
        QStringList canonicalPrefixPaths;
        for (const QJsonValue &value : prefixPaths)
            canonicalPrefixPaths << QFileInfo(value.toString()).canonicalFilePath();

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

    // The modern bundle suffixes are rooted at the application prefix, which
    // is the first of the prefix paths.
    const QString appPrefix = prefixPaths.first().toString();
    const auto pathsFor = [&](const QString &key) {
        QStringList list;
        for (const QJsonValue &value : paths.value(key).toArray())
            list << value.toString();
        return list;
    };

    // Verify that the libraries and plugins follow modern bundle conventions

    const QStringList libraryPaths = pathsFor("LibrariesPath");
    QVERIFY2(libraryPaths.contains(QDir::cleanPath(appPrefix + "/Frameworks")),
             qPrintable("Frameworks not in LibrariesPath: " + libraryPaths.join(", ")));

    const QStringList pluginPaths = pathsFor("PluginsPath");
    QVERIFY2(pluginPaths.contains(QDir::cleanPath(appPrefix + "/PlugIns")),
             qPrintable("PlugIns not in PluginsPath: " + pluginPaths.join(", ")));

    // Verify that QML import paths include the QML resources

    const QStringList qmlImportPaths = pathsFor("QmlImportsPath");
    const QString expectedQmlImport = QDir::cleanPath(appPrefix + "/Resources/qml");
    QVERIFY2(qmlImportPaths.contains(expectedQmlImport),
             qPrintable(expectedQmlImport + " not in QmlImportsPath: " + qmlImportPaths.join(", ")));
}
#endif

QTEST_GUILESS_MAIN(tst_QLibraryInfo)

#include "tst_qlibraryinfo.moc"
