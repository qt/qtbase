// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QDirIterator>

#ifdef Q_OS_WIN
#   include <qt_windows.h>
#else
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <dirent.h>
#   include <unistd.h>
#endif

class tst_QDir_10000 : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase()
    {
        QDir testdir = QDir::tempPath();

        const QString subfolder_name = QLatin1String("test_speed");
        QVERIFY(testdir.mkdir(subfolder_name));
        QVERIFY(testdir.cd(subfolder_name));

        for (uint i=0; i<10000; ++i) {
            QFile file(testdir.absolutePath() + "/testfile_" + QString::number(i));
            file.open(QIODevice::WriteOnly);
        }
    }
    void cleanupTestCase()
    {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        QVERIFY(testdir.removeRecursively());
    }
private slots:
    void sizeSpeed()
    {
        QBENCHMARK {
            QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
            QFileInfoList fileInfoList = testdir.entryInfoList(QDir::Files, QDir::Unsorted);
            foreach (const QFileInfo &fileInfo, fileInfoList) {
                fileInfo.isDir();
                fileInfo.size();
            }
        }
    }
    void sizeSpeedIterator()
    {
        QBENCHMARK {
            QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
            QDirIterator dit(testdir.path(), QDir::Files);
            while (dit.hasNext()) {
                const auto fi = dit.nextFileInfo();
                (void)fi.isDir();
                (void)fi.size();
            }
        }
    }

    void sizeSpeedWithoutFilter()
    {
        QBENCHMARK {
            QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
            QFileInfoList fileInfoList = testdir.entryInfoList(QDir::NoFilter, QDir::Unsorted);
            foreach (const QFileInfo &fileInfo, fileInfoList) {
                fileInfo.size();
            }
        }
    }
    void sizeSpeedWithoutFilterIterator()
    {
        QBENCHMARK {
            QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
            QDirIterator dit(testdir.path());
            while (dit.hasNext()) {
                const auto fi = dit.nextFileInfo();
                (void)fi.isDir();
                (void)fi.size();
            }
        }
    }

    void sizeSpeedWithoutFileInfoList()
    {
        QBENCHMARK {
            QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
            testdir.setSorting(QDir::Unsorted);
            QStringList fileList = testdir.entryList(QDir::NoFilter, QDir::Unsorted);
            foreach (const QString &filename, fileList) {
                QFileInfo fileInfo(filename);
                fileInfo.size();
            }
        }
    }

    void iDontWantAnyStat()
    {
        QBENCHMARK {
            QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
            testdir.setSorting(QDir::Unsorted);
            testdir.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);
            QStringList fileList = testdir.entryList(QDir::NoFilter, QDir::Unsorted);
            foreach (const QString &filename, fileList) {
                Q_UNUSED(filename);
            }
        }
    }
    void iDontWantAnyStatIterator()
    {
        QBENCHMARK {
            QDirIterator dit(QDir::tempPath() + QLatin1String("/test_speed"));
            while (dit.hasNext()) {
                dit.next();
            }
        }
    }

    void sorted_byTime()
    {
        QBENCHMARK {
            QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
            testdir.setSorting(QDir::Time);
            testdir.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);
            QStringList fileList = testdir.entryList(QDir::NoFilter, QDir::Time);
            foreach (const QString &filename, fileList) {
                Q_UNUSED(filename);
            }
        }
    }

    void sorted_byName()
    {
        QBENCHMARK {
            QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
            testdir.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);
            [[maybe_unused]] auto r = testdir.entryInfoList(QDir::NoFilter, QDir::Name);
        }
    }

    void sizeSpeedWithoutFilterLowLevel()
    {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
#ifdef Q_OS_WIN
        const wchar_t *dirpath = (wchar_t*)testdir.absolutePath().utf16();
        wchar_t appendedPath[MAX_PATH];
        wcscpy(appendedPath, dirpath);
        wcscat(appendedPath, L"\\*");

        QBENCHMARK {
            WIN32_FIND_DATA fd;
            HANDLE hSearch = FindFirstFileW(appendedPath, &fd);
            QVERIFY(hSearch != INVALID_HANDLE_VALUE);

            do {

            } while (FindNextFile(hSearch, &fd));
            FindClose(hSearch);
        }
#else
        QVERIFY(!chdir(qPrintable(testdir.absolutePath())));
        QBENCHMARK {
            DIR *dir = opendir(qPrintable(testdir.absolutePath()));
            QVERIFY(dir);

            struct dirent *item = readdir(dir);
            while (item) {
                char *fileName = item->d_name;

                struct stat fileStat;
                QVERIFY(!stat(fileName, &fileStat));

                item = readdir(dir);
            }
            closedir(dir);
        }
#endif
    }
};

QTEST_MAIN(tst_QDir_10000)

#include "tst_bench_qdir_10000.moc"
