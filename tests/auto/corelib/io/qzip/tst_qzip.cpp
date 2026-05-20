// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QDebug>
#include <QProcess>
#include <QTemporaryFile>

#include <private/qzipwriter_p.h>

class tst_QZip : public QObject
{
    Q_OBJECT

private slots:
    void createArchive();
};

void tst_QZip::createArchive()
{
#if !QT_CONFIG(process)
    QSKIP("Need QProcess to run unzip");
#endif
    QTemporaryFile f(QDir::tempPath() + "/createArchiveTest-XXXXXX.zip");
    QVERIFY(f.open());
    QZipWriter zip(&f);
    QByteArray fileContents("simple file contents\nline2\n");
    zip.addFile("My Filename", fileContents);
    zip.close();
    QString zipName = f.fileName();
    QCOMPARE_GT(f.size(), 0);
    f.close();

    QProcess unzip;
    unzip.setProgram("unzip");  // search $PATH
    unzip.setArguments({ "--help" });
    unzip.start();
    if (!unzip.waitForFinished()) {
        qInfo("Incomplete test: verified QZipWriter ran, but can't check what it created");
        return;
    }

    unzip.setArguments({ "-Z", "-l", "--h", "--t", zipName });
    unzip.start();
    QVERIFY(unzip.waitForFinished());
    QCOMPARE(unzip.exitCode(), 0);
    QCOMPARE(unzip.readAllStandardError(), QString());
    QByteArray out = unzip.readAllStandardOutput().trimmed();
    QVERIFY2(out.startsWith("-rw-------"), out);
    QVERIFY2(out.endsWith("My Filename"), out);

    unzip.setArguments({ "-p", zipName, "My Filename" });
    unzip.start();
    QVERIFY(unzip.waitForFinished());
    QCOMPARE(unzip.exitCode(), 0);
    QCOMPARE(unzip.readAllStandardOutput(), fileContents);
    QCOMPARE(unzip.readAllStandardError(), QString());
}

QTEST_MAIN(tst_QZip)
#include "tst_qzip.moc"
