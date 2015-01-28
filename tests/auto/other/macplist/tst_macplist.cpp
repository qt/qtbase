/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
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
"	<key>CFBundleGetInfoString</key>\n"
"	<string>Created by Qt/QMake</string>\n"
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
"	<key>CFBundleGetInfoString</key>\n"
"	<string>Created by Qt/QMake</string>\n"
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
"	<key>CFBundleGetInfoString</key>\n"
"	<string>Created by Qt/QMake</string>\n"
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
"	<key>CFBundleGetInfoString</key>\n"
"	<string>Created by Qt/QMake</string>\n"
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
"	<key>CFBundleGetInfoString</key>\n"
"	<string>Created by Qt/QMake</string>\n"
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
    QVERIFY(dir.cdUp());
    QVERIFY(dir.cdUp());
    QVERIFY(dir.cdUp());
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
