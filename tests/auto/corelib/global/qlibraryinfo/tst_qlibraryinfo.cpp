// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/private/qlibraryinfo_p.h>
#include <QStandardPaths>

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

QTEST_GUILESS_MAIN(tst_QLibraryInfo)

#include "tst_qlibraryinfo.moc"
