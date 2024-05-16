// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/private/qlibraryinfo_p.h>


class tst_QLibraryInfo : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void path_data();
    void path();
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

}

QTEST_GUILESS_MAIN(tst_QLibraryInfo)

#include "tst_qlibraryinfo.moc"
