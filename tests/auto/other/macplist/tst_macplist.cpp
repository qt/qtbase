// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>

class tst_MacPlist : public QObject
{
    Q_OBJECT
public:
    tst_MacPlist() {}

private slots:
#ifdef Q_OS_MAC
    void test_plist_data();
    void test_plist();
#endif
};

#ifdef Q_OS_MAC
void tst_MacPlist::test_plist_data()
{
    QTest::addColumn<QString>("test_plist");

    QTest::newRow("control") << QString::fromLatin1(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"	<key>CFBundleIconFile</key>\n"
"	<string></string>\n"
"	<key>CFBundlePackageType</key>\n"
"	<string>APPL</string>\n"
"	<key>CFBundleExecutable</key>\n"
"	<string>app</string>\n"
"	<key>CFBundleIdentifier</key>\n"
"	<string>com.yourcompany.app</string>\n"
"</dict>\n"
"</plist>\n");

    QTest::newRow("LSUIElement-as-string") << QString::fromLatin1(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"	<key>CFBundleIconFile</key>\n"
"	<string></string>\n"
"	<key>CFBundlePackageType</key>\n"
"	<string>APPL</string>\n"
"	<key>CFBundleExecutable</key>\n"
"	<string>app</string>\n"
"	<key>CFBundleIdentifier</key>\n"
"	<string>com.yourcompany.app</string>\n"
"	<key>LSUIElement</key>\n"
"	<string>false</string>\n"
"</dict>\n"
"</plist>\n");

    QTest::newRow("LSUIElement-as-bool") << QString::fromLatin1(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"	<key>CFBundleIconFile</key>\n"
"	<string></string>\n"
"	<key>CFBundlePackageType</key>\n"
"	<string>APPL</string>\n"
"	<key>CFBundleExecutable</key>\n"
"	<string>app</string>\n"
"	<key>CFBundleIdentifier</key>\n"
"	<string>com.yourcompany.app</string>\n"
"	<key>LSUIElement</key>\n"
"	<false/>\n"
"</dict>\n"
"</plist>\n");

    QTest::newRow("LSUIElement-as-int") << QString::fromLatin1(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"	<key>CFBundleIconFile</key>\n"
"	<string></string>\n"
"	<key>CFBundlePackageType</key>\n"
"	<string>APPL</string>\n"
"	<key>CFBundleExecutable</key>\n"
"	<string>app</string>\n"
"	<key>CFBundleIdentifier</key>\n"
"	<string>com.yourcompany.app</string>\n"
"	<key>LSUIElement</key>\n"
"	<real>0</real>\n"
"</dict>\n"
"</plist>\n");

    QTest::newRow("LSUIElement-as-garbage") << QString::fromLatin1(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"	<key>CFBundleIconFile</key>\n"
"	<string></string>\n"
"	<key>CFBundlePackageType</key>\n"
"	<string>APPL</string>\n"
"	<key>CFBundleExecutable</key>\n"
"	<string>app</string>\n"
"	<key>CFBundleIdentifier</key>\n"
"	<string>com.yourcompany.app</string>\n"
"	<key>LSUIElement</key>\n"
"	<badkey>0</badkey>\n"
"</dict>\n"
"</plist>\n");
}

void tst_MacPlist::test_plist()
{
    QFETCH(QString, test_plist);

    QString infoPlist = QLatin1String("Info.plist");
    QDir dir(QCoreApplication::applicationDirPath());
#ifndef Q_OS_MACOS
    // macOS builds tests as single executables, iOS/tvOS/watchOS does not
    QVERIFY(dir.cdUp());
    QVERIFY(dir.cdUp());
    QVERIFY(dir.cdUp());
#endif
    QVERIFY(dir.cd(QLatin1String("app")));
    QVERIFY(dir.cd(QLatin1String("app.app")));
    QVERIFY(dir.cd(QLatin1String("Contents")));
    QVERIFY(dir.exists(infoPlist));
    {
        QFile file(dir.filePath(infoPlist));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QByteArray ba = test_plist.toUtf8();
        QCOMPARE(file.write(ba), qint64(ba.size()));
    }
    QVERIFY(dir.cd(QLatin1String("MacOS")));
    QVERIFY(dir.exists(QLatin1String("app")));
    QProcess process;
    process.start(dir.filePath("app"));
    QCOMPARE(process.waitForFinished(), true);
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
}
#endif

QTEST_MAIN(tst_MacPlist)
#include "tst_macplist.moc"
