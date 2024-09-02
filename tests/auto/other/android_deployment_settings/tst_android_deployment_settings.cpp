// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTest>
#include <QLibraryInfo>
#include <QDir>

class tst_android_deployment_settings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase_data();
    void init();

    void DeploymentSettings_data();
    void DeploymentSettings();

    void QtPaths_data();
    void QtPaths();

private:
    static QString makePath(QLibraryInfo::LibraryLocation loc);
    QJsonDocument jsonDoc;
};

QString tst_android_deployment_settings::makePath(QLibraryInfo::LibraryLocation loc)
{
    const auto prefix = QLibraryInfo::path(QLibraryInfo::PrefixPath);
    auto path = QLibraryInfo::path(loc);
    path.remove(0, prefix.size() + 1);
    if (path.isEmpty()) // Assume that if path is empty it's '.'
        path = ".";
    return path;
}

void tst_android_deployment_settings::initTestCase_data()
{
    QTest::addColumn<QString>("file");
    QTest::newRow("old") << ":/old_settings.json";
    QTest::newRow("new") << ":/new_settings.json";
}

void tst_android_deployment_settings::init()
{
    QFETCH_GLOBAL(QString, file);
    QFile settings(file);
    QVERIFY(settings.open(QIODeviceBase::ReadOnly));
    jsonDoc = QJsonDocument::fromJson(settings.readAll());
    QVERIFY(!jsonDoc.isNull());
}

void tst_android_deployment_settings::DeploymentSettings_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("value");

    QTest::newRow("sdkBuildToolsRevision") << "sdkBuildToolsRevision"
                                           << "23.0.2";
    QTest::newRow("deployment-dependencies") << "deployment-dependencies"
                                             << "dep1.so,dep2.so,dep3.so";
    QTest::newRow("android-extra-plugins")
            << "android-extra-plugins"
            << "some/path/to/plugin1.so,some/path/to/plugin2.so,some/path/to/plugin3.so";
    QTest::newRow("android-extra-libs") << "android-extra-libs"
                                        << "some/path/to/lib1.so,some/path/to/lib2.so,some/path/to/"
                                           "lib3.so,some/path/to/lib4.so";
    QTest::newRow("android-system-libs-prefix") << "android-system-libs-prefix"
                                                << "myLibPrefix";
    QTest::newRow("android-package-source-directory") << "android-package-source-directory"
                                                      << "path/to/source/dir";
    QTest::newRow("android-min-sdk-version") << "android-min-sdk-version"
                                             << "1";
    QTest::newRow("android-target-sdk-version") << "android-target-sdk-version"
                                                << "2";
    QTest::newRow("android-compile-sdk-version") << "android-compile-sdk-version"
                                                << "35";
    QTest::newRow("android-package-name") << "android-package-name"
                                          << "org.qtproject.android_deployment_settings_test";
    QTest::newRow("android-app-name") << "android-app-name"
                                          << "Android Deployment Settings Test";
    QTest::newRow("permissions") << "permissions"
                        << "[{\"name\":\"PERMISSION_WITH_ATTRIBUTES\","
                           "\"extras\":\"android:minSdkVersion='32' android:maxSdkVersion='34' \"},"
                           "{\"name\":\"PERMISSION_WITHOUT_ATTRIBUTES\"}]";
}

void tst_android_deployment_settings::DeploymentSettings()
{
    QFETCH(QString, key);
    QFETCH(QString, value);
    QJsonValue keyValue = jsonDoc[key];
    if (keyValue.type() == QJsonValue::Type::String) {
        QCOMPARE(keyValue.toString(), value);
    } else if (keyValue.type() == QJsonValue::Type::Array) {
        QJsonParseError parseError;
        // For robustness (field order, whitespaces etc.) make comparison between QJsonDocuments
        QJsonDocument expectedDoc = QJsonDocument::fromJson(value.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
            qFatal("Failed to parse expected JSON array: %s", qPrintable(parseError.errorString()));
        QCOMPARE(QJsonDocument(keyValue.toArray()), expectedDoc);
    } else {
        qFatal("Unhandled JSON type: %i", keyValue.type());
    }
}

void tst_android_deployment_settings::QtPaths_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("value");

    QTest::newRow("qtDataDirectory") << "qtDataDirectory" << makePath(QLibraryInfo::DataPath);
    QTest::newRow("qtLibExecsDirectory")
            << "qtLibExecsDirectory" << makePath(QLibraryInfo::LibraryExecutablesPath);
    QTest::newRow("qtLibsDirectory") << "qtLibsDirectory" << makePath(QLibraryInfo::LibrariesPath);
    QTest::newRow("qtPluginsDirectory")
            << "qtPluginsDirectory" << makePath(QLibraryInfo::PluginsPath);
    QTest::newRow("qtQmlDirectory") << "qtQmlDirectory" << makePath(QLibraryInfo::QmlImportsPath);
}

void tst_android_deployment_settings::QtPaths()
{
    QFETCH(QString, key);
    QFETCH(QString, value);
    QCOMPARE(QDir::cleanPath(jsonDoc[key].toObject()[DEFAULT_ABI].toString()),
             QDir::cleanPath(value));
}

QTEST_MAIN(tst_android_deployment_settings)

#include "tst_android_deployment_settings.moc"
