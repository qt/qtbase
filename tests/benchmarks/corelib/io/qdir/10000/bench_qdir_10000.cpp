/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#ifdef Q_OS_WIN
#   include <windows.h>
#else
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <dirent.h>
#   include <unistd.h>
#endif

class bench_QDir_10000 : public QObject{
  Q_OBJECT
public slots:
    void initTestCase() {
        QDir testdir = QDir::tempPath();

        const QString subfolder_name = QLatin1String("test_speed");
        QVERIFY(testdir.mkdir(subfolder_name));
        QVERIFY(testdir.cd(subfolder_name));

        for (uint i=0; i<10000; ++i) {
            QFile file(testdir.absolutePath() + "/testfile_" + QString::number(i));
            file.open(QIODevice::WriteOnly);
        }
    }
    void cleanupTestCase() {
        {
            QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
            testdir.setSorting(QDir::Unsorted);
            testdir.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);
            foreach (const QString &filename, testdir.entryList()) {
                testdir.remove(filename);
            }
        }
        const QDir temp = QDir(QDir::tempPath());
        temp.rmdir(QLatin1String("test_speed"));
    }
private slots:
    void baseline() {}

    void sizeSpeed() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        QBENCHMARK {
            QFileInfoList fileInfoList = testdir.entryInfoList(QDir::Files, QDir::Unsorted);
            foreach (const QFileInfo &fileInfo, fileInfoList) {
                fileInfo.isDir();
                fileInfo.size();
            }
        }
    }
    void sizeSpeedIterator() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        QBENCHMARK {
            QDirIterator dit(testdir.path(), QDir::Files);
            while (dit.hasNext()) {
                dit.next();
                dit.fileInfo().isDir();
                dit.fileInfo().size();
            }
        }
    }

    void sizeSpeedWithoutFilter() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        QBENCHMARK {
            QFileInfoList fileInfoList = testdir.entryInfoList(QDir::NoFilter, QDir::Unsorted);
            foreach (const QFileInfo &fileInfo, fileInfoList) {
                fileInfo.size();
            }
        }
    }
    void sizeSpeedWithoutFilterIterator() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        QBENCHMARK {
            QDirIterator dit(testdir.path());
            while (dit.hasNext()) {
                dit.next();
                dit.fileInfo().isDir();
                dit.fileInfo().size();
            }
        }
    }

    void sizeSpeedWithoutFileInfoList() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        testdir.setSorting(QDir::Unsorted);
        QBENCHMARK {
            QStringList fileList = testdir.entryList(QDir::NoFilter, QDir::Unsorted);
            foreach (const QString &filename, fileList) {
                QFileInfo fileInfo(filename);
                fileInfo.size();
            }
        }
    }

    void iDontWantAnyStat() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        testdir.setSorting(QDir::Unsorted);
        testdir.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);
        QBENCHMARK {
            QStringList fileList = testdir.entryList(QDir::NoFilter, QDir::Unsorted);
            foreach (const QString &filename, fileList) {

            }
        }
    }
    void iDontWantAnyStatIterator() {
        QBENCHMARK {
            QDirIterator dit(QDir::tempPath() + QLatin1String("/test_speed"));
            while (dit.hasNext()) {
                dit.next();
            }
        }
    }

    void sizeSpeedWithoutFilterLowLevel() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
#ifdef Q_OS_WIN
        const wchar_t *dirpath = (wchar_t*)testdir.absolutePath().utf16();
        wchar_t appendedPath[MAX_PATH];
        wcscpy(appendedPath, dirpath);
        wcscat(appendedPath, L"\\*");

        WIN32_FIND_DATA fd;
        HANDLE hSearch = FindFirstFileW(appendedPath, &fd);
        QVERIFY(hSearch != INVALID_HANDLE_VALUE);

        QBENCHMARK {
            do {

            } while (FindNextFile(hSearch, &fd));
        }
        FindClose(hSearch);
#else
        DIR *dir = opendir(qPrintable(testdir.absolutePath()));
        QVERIFY(dir);

        QVERIFY(!chdir(qPrintable(testdir.absolutePath())));
        QBENCHMARK {
            struct dirent *item = readdir(dir);
            while (item) {
                char *fileName = item->d_name;

                struct stat fileStat;
                QVERIFY(!stat(fileName, &fileStat));

                item = readdir(dir);
            }
        }
        closedir(dir);
#endif
    }
};

QTEST_MAIN(bench_QDir_10000)
#include "bench_qdir_10000.moc"
