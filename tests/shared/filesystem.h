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

#ifndef QT_TESTS_SHARED_FILESYSTEM_H_INCLUDED
#define QT_TESTS_SHARED_FILESYSTEM_H_INCLUDED

#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QScopedPointer>
#include <QDir>
#include <QFile>

#if defined(Q_OS_WIN)
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

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    static DWORD createNtfsJunction(QString target, QString linkName, QString *errorMessage)
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
        DWORD result = ERROR_SUCCESS;

        QFileInfo junctionInfo(linkName);
        linkName = QDir::toNativeSeparators(junctionInfo.absoluteFilePath());
        const QString drive = linkName.left(3);
        if (GetVolumeInformationW(reinterpret_cast<const wchar_t *>(drive.utf16()),
                                  NULL, 0, NULL, NULL, NULL,
                                  fileSystem, sizeof(fileSystem)/sizeof(WCHAR)) == FALSE) {
            result = GetLastError();
            *errorMessage = "GetVolumeInformationW() failed: " + qt_error_string(int(result));
            return result;
        }
        if (QString::fromWCharArray(fileSystem) != "NTFS") {
            *errorMessage = "This seems not to be an NTFS volume. Junctions are not allowed.";
            return ERROR_NOT_SUPPORTED;
        }

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
        if (hFile == INVALID_HANDLE_VALUE) {
            result = GetLastError();
            *errorMessage = "CreateFileW(" + linkName + ") failed: " + qt_error_string(int(result));
            return result;
        }

        memset( reparseInfo, 0, sizeof( *reparseInfo ));
        reparseInfo->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
        reparseInfo->ReparseTargetLength = WORD(target.size()) * WORD(sizeof(wchar_t));
        reparseInfo->ReparseTargetMaximumLength = reparseInfo->ReparseTargetLength + sizeof(wchar_t);
        target.toWCharArray(reparseInfo->ReparseTarget);
        reparseInfo->ReparseDataLength = reparseInfo->ReparseTargetLength + 12;

        bool ioc = DeviceIoControl(hFile, FSCTL_SET_REPARSE_POINT, reparseInfo,
                                 reparseInfo->ReparseDataLength + REPARSE_MOUNTPOINT_HEADER_SIZE,
                                 NULL, 0, &returnedLength, NULL);
        if (!ioc) {
            result = GetLastError();
            *errorMessage = "DeviceIoControl() failed: " + qt_error_string(int(result));
        }
        CloseHandle( hFile );
        return result;
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
