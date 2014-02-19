/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT_TESTS_SHARED_FILESYSTEM_H_INCLUDED
#define QT_TESTS_SHARED_FILESYSTEM_H_INCLUDED

#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QScopedPointer>
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

// QTemporaryDir-based helper class for creating file-system hierarchies and cleaning up.
class FileSystem
{
    Q_DISABLE_COPY(FileSystem)
public:
    FileSystem() : m_temporaryDir(FileSystem::tempFilePattern()) {}

    QString path() const { return m_temporaryDir.path(); }
    QString absoluteFilePath(const QString &fileName) const { return path() + QLatin1Char('/') + fileName; }

    bool createDirectory(const QString &relativeDirName)
    {
        if (m_temporaryDir.isValid()) {
            QDir dir(m_temporaryDir.path());
            return dir.mkpath(relativeDirName);
        }
        return false;
    }

    bool createFile(const QString &relativeFileName)
    {
        QScopedPointer<QFile> file(openFileForWrite(relativeFileName));
        return !file.isNull();
    }

    qint64 createFileWithContent(const QString &relativeFileName)
    {
        QScopedPointer<QFile> file(openFileForWrite(relativeFileName));
        return file.isNull() ? qint64(-1) : file->write(relativeFileName.toUtf8());
    }

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
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
            QSKIP("This seems not to be an NTFS volume. Junctions are not allowed.");

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
        reparseInfo->ReparseTargetLength = WORD(target.size()) * WORD(sizeof(wchar_t));
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
    static QString tempFilePattern()
    {
        QString result = QDir::tempPath();
        if (!result.endsWith(QLatin1Char('/')))
            result.append(QLatin1Char('/'));
        result += QStringLiteral("qt-test-filesystem-");
        result += QCoreApplication::applicationName();
        result += QStringLiteral("-XXXXXX");
        return result;
    }

    QFile *openFileForWrite(const QString &fileName) const
    {
        if (m_temporaryDir.isValid()) {
            const QString absName = absoluteFilePath(fileName);
            QScopedPointer<QFile> file(new QFile(absName));
            if (file->open(QIODevice::WriteOnly))
                return file.take();
            qWarning("Cannot open '%s' for writing: %s", qPrintable(absName), qPrintable(file->errorString()));
        }
        return 0;
    }

    QTemporaryDir m_temporaryDir;
};

#endif // include guard
