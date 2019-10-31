/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>

#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemengine_p.h>

class tst_QFileSystemEngine : public QObject
{
    Q_OBJECT

private slots:
    void cleanupTestCase();
    void moveToTrash_data();
    void moveToTrash();

private:
    QStringList createdEntries;
};

void tst_QFileSystemEngine::cleanupTestCase()
{
    for (QString entry : createdEntries) {
        QFileInfo entryInfo(entry);
        if (!entryInfo.exists())
            continue;
        QDir entryDir(entry);
        if (entryInfo.isDir()) {
            if (!entryDir.removeRecursively())
                qWarning("Failed to remove trashed dir '%s'", entry.toLocal8Bit().constData());
        } else if (!QFile::remove(entry)) {
            qWarning("Failed to remove trashed file '%s'", entry.toLocal8Bit().constData());
        }
    }
}

void tst_QFileSystemEngine::moveToTrash_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<bool>("create");
    QTest::addColumn<bool>("success");

    {
        QTemporaryFile tempFile;
        tempFile.open();
        QTest::newRow("temporary file")
            << tempFile.fileName()
            << true << true;
    }
    {
        QTemporaryDir tempDir;
        tempDir.setAutoRemove(false);
        QTest::newRow("temporary dir")
            << tempDir.path() + QLatin1Char('/')
            << true << true;
    }
    {
        QTemporaryDir homeDir(QFileSystemEngine::homePath() + QLatin1String("/XXXXXX"));
        homeDir.setAutoRemove(false);
        QTemporaryFile homeFile(homeDir.path()
                              + QLatin1String("/tst_qfilesystemengine-XXXXXX"));
        homeFile.open();
        QTest::newRow("home file")
            << homeFile.fileName()
            << true << true;

        QTest::newRow("home dir")
            << homeDir.path() + QLatin1Char('/')
            << true << true;
    }

    QTest::newRow("unmovable")
        << QFileSystemEngine::rootPath()
        << false << false;
    QTest::newRow("no such file")
        << QString::fromLatin1("no/such/file")
        << false << false;
}

void tst_QFileSystemEngine::moveToTrash()
{
    QFETCH(QString, filePath);
    QFETCH(bool, create);
    QFETCH(bool, success);

#if defined(Q_OS_WINRT)
    QSKIP("WinRT does not have a trash", SkipAll);
#endif

    if (create && !QFileInfo::exists(filePath)) {
        createdEntries << filePath;
        if (filePath.endsWith(QLatin1Char('/'))) {
            QDir temp(QFileSystemEngine::rootPath());
            temp.mkdir(filePath);
            QFile file(filePath + QLatin1String("test"));
            if (!file.open(QIODevice::WriteOnly))
                QSKIP("Couldn't create directory with file");
        } else {
            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly))
                QSKIP("Couldn't open file for writing");
        }
        QVERIFY(QFileInfo::exists(filePath));
    }

    QFileSystemEntry entry(filePath);
    QFileSystemEntry newLocation;
    QSystemError error;

    bool existed = QFileInfo::exists(filePath);
    bool result = QFileSystemEngine::moveFileToTrash(entry, newLocation, error);
    QCOMPARE(result, success);
    if (result) {
        QCOMPARE(error.error(), 0);
        QVERIFY(existed != QFileInfo::exists(filePath));
        const QString newPath = newLocation.filePath();
#if defined(Q_OS_WIN)
        // one of the Windows code paths doesn't provide the location of the object in the trash
        if (newPath.isEmpty())
            QEXPECT_FAIL("", "Qt built without IFileOperations support on Windows!", Continue);
#endif
        QVERIFY(!newPath.isEmpty());
        if (!newPath.isEmpty()) {
            createdEntries << newPath;
            QFileInfo trashInfo(newPath);
            QVERIFY(trashInfo.exists());
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
            QString infoFile = trashInfo.absolutePath() + QLatin1String("/../info/")
                             + trashInfo.fileName() + QLatin1String(".trashinfo");
            createdEntries << infoFile;
#endif
        }
    } else {
        QVERIFY(error.error() != 0);
        QCOMPARE(existed, QFileInfo::exists(filePath));
    }
}

QTEST_MAIN(tst_QFileSystemEngine)
#include <tst_qfilesystemengine.moc>
