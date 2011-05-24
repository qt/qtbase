/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
// Helper functions for creating file-system hierarchies and cleaning up.

#ifndef QT_TESTS_SHARED_FILESYSTEM_H_INCLUDED
#define QT_TESTS_SHARED_FILESYSTEM_H_INCLUDED

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
#include <windows.h>
#include <winioctl.h>
#ifndef IO_REPARSE_TAG_MOUNT_POINT
#define IO_REPARSE_TAG_MOUNT_POINT       (0xA0000003L)
#endif
#define REPARSE_MOUNTPOINT_HEADER_SIZE   8
#ifndef FSCTL_SET_REPARSE_POINT
#define FSCTL_SET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#endif
#endif

struct FileSystem
{
    ~FileSystem()
    {
        Q_FOREACH(QString fileName, createdFiles)
            QFile::remove(fileName);

        Q_FOREACH(QString dirName, createdDirectories)
            currentDir.rmdir(dirName);
    }

    bool createDirectory(const QString &dirName)
    {
        if (currentDir.mkdir(dirName)) {
            createdDirectories.prepend(dirName);
            return true;
        }
        return false;
    }

    bool createFile(const QString &fileName)
    {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            createdFiles << fileName;
            return true;
        }
        return false;
    }

    qint64 createFileWithContent(const QString &fileName)
    {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            createdFiles << fileName;
            return file.write(fileName.toUtf8());
        }
        return -1;
    }

    bool createLink(const QString &destination, const QString &linkName)
    {
        if (QFile::link(destination, linkName)) {
            createdFiles << linkName;
            return true;
        }
        return false;
    }
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    static void createNtfsJunction(QString target, QString linkName)
    {
        typedef struct {
            DWORD   ReparseTag;
            DWORD   ReparseDataLength;
            WORD    Reserved;
            WORD    ReparseTargetLength;
            WORD    ReparseTargetMaximumLength;
            WORD    Reserved1;
            WCHAR   ReparseTarget[1];
        } REPARSE_MOUNTPOINT_DATA_BUFFER, *PREPARSE_MOUNTPOINT_DATA_BUFFER;

        char    reparseBuffer[MAX_PATH*3];
        HANDLE  hFile;
        DWORD   returnedLength;
        wchar_t fileSystem[MAX_PATH] = L"";
        PREPARSE_MOUNTPOINT_DATA_BUFFER reparseInfo = (PREPARSE_MOUNTPOINT_DATA_BUFFER) reparseBuffer;

        QFileInfo junctionInfo(linkName);
        linkName = QDir::toNativeSeparators(junctionInfo.absoluteFilePath());

        GetVolumeInformationW( (wchar_t*)linkName.left(3).utf16(), NULL, 0, NULL, NULL, NULL,
                               fileSystem, sizeof(fileSystem)/sizeof(WCHAR));
        if(QString().fromWCharArray(fileSystem) != "NTFS")
            QSKIP("This seems not to be an NTFS volume. Junctions are not allowed.",SkipSingle);

        if (!target.startsWith("\\??\\") && !target.startsWith("\\\\?\\")) {
            QFileInfo targetInfo(target);
            target = QDir::toNativeSeparators(targetInfo.absoluteFilePath());
            target.prepend("\\??\\");
            if(target.endsWith('\\') && target.at(target.length()-2) != ':')
                target.chop(1);
        }
        QDir().mkdir(linkName);
        hFile = CreateFileW( (wchar_t*)linkName.utf16(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                             FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS, NULL );
        QVERIFY(hFile != INVALID_HANDLE_VALUE );

        memset( reparseInfo, 0, sizeof( *reparseInfo ));
        reparseInfo->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
        reparseInfo->ReparseTargetLength = target.size() * sizeof(wchar_t);
        reparseInfo->ReparseTargetMaximumLength = reparseInfo->ReparseTargetLength + sizeof(wchar_t);
        target.toWCharArray(reparseInfo->ReparseTarget);
        reparseInfo->ReparseDataLength = reparseInfo->ReparseTargetLength + 12;

        bool ioc = DeviceIoControl(hFile, FSCTL_SET_REPARSE_POINT, reparseInfo,
                                 reparseInfo->ReparseDataLength + REPARSE_MOUNTPOINT_HEADER_SIZE,
                                 NULL, 0, &returnedLength, NULL);
        CloseHandle( hFile );
        QVERIFY(ioc);
    }
#endif

private:
    QDir currentDir;

    QStringList createdDirectories;
    QStringList createdFiles;
};

#endif // include guard
