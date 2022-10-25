// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QT_TESTS_SHARED_FILESYSTEM_H_INCLUDED
#define QT_TESTS_SHARED_FILESYSTEM_H_INCLUDED

#include <QDir>
#include <QFile>
#include <QOperatingSystemVersion>
#include <QScopedPointer>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#include <winioctl.h>
#ifndef IO_REPARSE_TAG_MOUNT_POINT
#define IO_REPARSE_TAG_MOUNT_POINT       (0xA0000003L)
#endif
#define REPARSE_MOUNTPOINT_HEADER_SIZE   8
#ifndef FSCTL_SET_REPARSE_POINT
#define FSCTL_SET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#endif
#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE // MinGW
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE (0x2)
#endif
QT_BEGIN_NAMESPACE
namespace QTest {
    static QString uncServerName() { return qgetenv("COMPUTERNAME"); }
}
QT_END_NAMESPACE
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

#if defined(Q_OS_WIN)
    struct Result {
        DWORD dwErr = ERROR_SUCCESS;
        QString link;
        QString target;
        QString errorMessage;
    };

    static Result createSymbolicLink(const QString &symLinkName, const QString &target)
    {
        Result result;
        const QString nativeSymLinkName = QDir::toNativeSeparators(symLinkName);
        const QString nativeTarget = QDir::toNativeSeparators(target);
        DWORD flags = 0;
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 14972))
            flags |= SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
        if (QFileInfo(target).isDir())
            flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
        if (CreateSymbolicLink(reinterpret_cast<const wchar_t*>(nativeSymLinkName.utf16()),
                               reinterpret_cast<const wchar_t*>(nativeTarget.utf16()), flags) == FALSE) {
            result.dwErr = GetLastError();
            QTextStream(&result.errorMessage) << "CreateSymbolicLink(" <<  nativeSymLinkName << ", "
                << nativeTarget << ", 0x" << Qt::hex << flags << Qt::dec << ") failed with error "
                << result.dwErr << ": " << qt_error_string(int(result.dwErr));
        } else {
            result.link = nativeSymLinkName;
            result.target = nativeTarget;
        }
        return result;
    }

    static Result createNtfsJunction(QString target, QString linkName)
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
        Result result;

        QFileInfo junctionInfo(linkName);
        linkName = QDir::toNativeSeparators(junctionInfo.absoluteFilePath());
        const QString drive = linkName.left(3);
        if (GetVolumeInformationW(reinterpret_cast<const wchar_t *>(drive.utf16()),
                                  NULL, 0, NULL, NULL, NULL,
                                  fileSystem, sizeof(fileSystem)/sizeof(WCHAR)) == FALSE) {
            result.dwErr = GetLastError();
            result.errorMessage = "GetVolumeInformationW() failed: " + qt_error_string(int(result.dwErr));
            return result;
        }
        if (QString::fromWCharArray(fileSystem) != "NTFS") {
            result.errorMessage = "This seems not to be an NTFS volume. Junctions are not allowed.";
            result.dwErr = ERROR_NOT_SUPPORTED;
            return result;
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
            result.dwErr = GetLastError();
            result.errorMessage = "CreateFileW(" + linkName + ") failed: " + qt_error_string(int(result.dwErr));
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
            result.dwErr = GetLastError();
            result.errorMessage = "DeviceIoControl() failed: " + qt_error_string(int(result.dwErr));
        } else {
            result.link = linkName;
            result.target = target;
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
            std::unique_ptr<QFile> file(new QFile(absName));
            if (file->open(QIODevice::WriteOnly))
                return file.release();
            qWarning("Cannot open '%s' for writing: %s", qPrintable(absName), qPrintable(file->errorString()));
        }
        return 0;
    }

    QTemporaryDir m_temporaryDir;
};

#endif // include guard
