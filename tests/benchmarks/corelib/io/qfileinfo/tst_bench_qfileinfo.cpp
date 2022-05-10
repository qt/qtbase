// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QDebug>
#include <qtest.h>
#include <QTest>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>

#include "private/qfsfileengine_p.h"
#include "../../../../shared/filesystem.h"

class tst_QFileInfo : public QObject
{
    Q_OBJECT
private slots:
    void existsTemporary();
    void existsStatic();
#if defined(Q_OS_WIN)
    void symLinkTargetPerformanceLNK();
    void junctionTargetPerformanceMountpoint();
#endif
};

void tst_QFileInfo::existsTemporary()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QBENCHMARK { QFileInfo(appPath).exists(); }
}

void tst_QFileInfo::existsStatic()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QBENCHMARK { QFileInfo::exists(appPath); }
}

#if defined(Q_OS_WIN)
void tst_QFileInfo::symLinkTargetPerformanceLNK()
{
    QVERIFY(QFile::link("file","link.lnk"));
    QFileInfo info("link.lnk");
    info.setCaching(false);
    QVERIFY(info.isSymLink());
    QString linkTarget;
    QBENCHMARK {
        for(int i=0; i<100; i++)
            linkTarget = info.symLinkTarget();
    }
    QVERIFY(QFile::remove("link.lnk"));
}

void tst_QFileInfo::junctionTargetPerformanceMountpoint()
{
    wchar_t buffer[MAX_PATH];
    QString rootPath = QDir::toNativeSeparators(QDir::rootPath());
    QVERIFY(GetVolumeNameForVolumeMountPointW((LPCWSTR)rootPath.utf16(), buffer, MAX_PATH));
    QString rootVolume = QString::fromWCharArray(buffer);
    QString mountpoint = "mountpoint";
    rootVolume.replace("\\\\?\\","\\??\\");
    const auto result = FileSystem::createNtfsJunction(rootVolume, mountpoint);
    QVERIFY2(result.dwErr == ERROR_SUCCESS, qPrintable(result.errorMessage));

    QFileInfo info(mountpoint);
    info.setCaching(false);
    QVERIFY(info.isJunction());
    QString junctionTarget;
    QBENCHMARK {
        for(int i=0; i<100; i++)
            junctionTarget = info.junctionTarget();
    }
    QVERIFY(QDir().rmdir(mountpoint));
}
#endif

QTEST_MAIN(tst_QFileInfo)

#include "tst_bench_qfileinfo.moc"
