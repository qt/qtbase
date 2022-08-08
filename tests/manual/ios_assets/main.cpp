// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtTest/QtTest>

#ifdef DEBUG_APP_DATA_LOCATION
    #include <QtCore/QDebug>
#endif

class AssetsIos : public QObject
{
    Q_OBJECT
private slots:
    void bundledTextFiles();
    void bundledAppIcons();
    void bundledImageInAssetCatalog();
};

void AssetsIos::bundledTextFiles()
{
#ifdef DEBUG_APP_DATA_LOCATION
    auto paths = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    qDebug() << paths;

    for (const QString& path: paths) {
        QDir oneDir = QDir(path);
        qDebug() << "path" << path << "entries" << oneDir.entryList();
    }
#endif

    /*
    AppDataLocation returns the following 3 paths on iOS
     /var/mobile/Containers/Data/Application/uuid/Library/Application Support/tst_manual_ios_assets
     /Library/Application Support/tst_manual_ios_assets
     /private/var/containers/Bundle/Application/<uuid>/tst_manual_ios_assets.app

    The textFiles/foo.txt file only exists in the third location at
     /private/var/containers/Bundle/Application/<uuid>/tst_manual_ios_assets.app/textFiles/foo.txt

    AppDataLocation returns the following 3 paths on macOS
    /Users/<user>/Library/Application Support/tst_manual_ios_assets
    /Library/Application Support/tst_manual_ios_assets
    <build-dir>/tst_manual_ios_assets.app/Contents/Resources

    Once again the file only exists in the third location at
    <build-dir>/tst_manual_ios_assets.app/Contents/Resources/textFiles/foo.txt
    */
    QString textFile = QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                              QStringLiteral("textFiles/foo.txt"));
    QVERIFY(!textFile.isEmpty());
}

void AssetsIos::bundledAppIcons() {
    // Confirm one of the app icons are present.
    QString appIcon = QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                             QStringLiteral("AppIcon40x40@2x.png"));
    QVERIFY(!appIcon.isEmpty());
}

bool imageExistsInAssetCatalog(QString imageName);

void AssetsIos::bundledImageInAssetCatalog() {
    // Asset catalog images can be accessed only via a name, not a path.
    QVERIFY(imageExistsInAssetCatalog(QStringLiteral("Face")));
}

QTEST_MAIN(AssetsIos)
#include "main.moc"
