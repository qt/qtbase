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
