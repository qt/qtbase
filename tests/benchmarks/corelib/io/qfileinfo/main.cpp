/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <QDebug>
#include <qtest.h>
#include <QtTest/QtTest>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>

#include "private/qfsfileengine_p.h"
#include "../../../../shared/filesystem.h"

class qfileinfo : public QObject
{
    Q_OBJECT
private slots:
    void existsTemporary();
    void existsStatic();
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    void symLinkTargetPerformanceLNK();
    void symLinkTargetPerformanceMounpoint();
#endif
    void initTestCase();
    void cleanupTestCase();
public:
    qfileinfo() : QObject() {};
};

void qfileinfo::initTestCase()
{
}

void qfileinfo::cleanupTestCase()
{
}

void qfileinfo::existsTemporary()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QBENCHMARK { QFileInfo(appPath).exists(); }
}

void qfileinfo::existsStatic()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QBENCHMARK { QFileInfo::exists(appPath); }
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
void qfileinfo::symLinkTargetPerformanceLNK()
{
    QVERIFY(QFile::link("file","link.lnk"));
    QFileInfo info("link.lnk");
    info.setCaching(false);
    QVERIFY(info.isSymLink());
    QString linkTarget;
    QBENCHMARK {
        for(int i=0; i<100; i++)
            linkTarget = info.readLink();
    }
    QVERIFY(QFile::remove("link.lnk"));
}

void qfileinfo::symLinkTargetPerformanceMounpoint()
{
    wchar_t buffer[MAX_PATH];
    QString rootPath = QDir::toNativeSeparators(QDir::rootPath());
    QVERIFY(GetVolumeNameForVolumeMountPointW((LPCWSTR)rootPath.utf16(), buffer, MAX_PATH));
    QString rootVolume = QString::fromWCharArray(buffer);
    QString mountpoint = "mountpoint";
    rootVolume.replace("\\\\?\\","\\??\\");
    FileSystem::createNtfsJunction(rootVolume, mountpoint);

    QFileInfo info(mountpoint);
    info.setCaching(false);
    QVERIFY(info.isSymLink());
    QString linkTarget;
    QBENCHMARK {
        for(int i=0; i<100; i++)
            linkTarget = info.readLink();
    }
    QVERIFY(QDir().rmdir(mountpoint));
}
#endif

QTEST_MAIN(qfileinfo)

#include "main.moc"
