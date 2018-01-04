/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfilesystemengine_p.h"
#include "qoperatingsystemversion.h"
#include "qplatformdefs.h"
#include "qsysinfo.h"
#include "private/qabstractfileengine_p.h"
#include "private/qfsfileengine_p.h"
#include <private/qsystemlibrary_p.h>
#include <qdebug.h>

#include "qfile.h"
#include "qdir.h"
#include "qvarlengtharray.h"
#include "qdatetime.h"
#include "qt_windows.h"
#include "qvector.h"

#include <sys/types.h>
#include <direct.h>
#include <winioctl.h>
#include <objbase.h>
#ifndef Q_OS_WINRT
#  include <shlobj.h>
#  include <lm.h>
#  include <accctrl.h>
#endif
#include <initguid.h>
#include <ctype.h>
#include <limits.h>
#ifndef Q_OS_WINRT
#  define SECURITY_WIN32
#  include <security.h>
#else // !Q_OS_WINRT
#  include "qstandardpaths.h"
#  include "qthreadstorage.h"
#  include <wrl.h>
#  include <windows.foundation.h>
#  include <windows.storage.h>
#  include <Windows.ApplicationModel.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::ApplicationModel;
#endif // Q_OS_WINRT

#ifndef SPI_GETPLATFORMTYPE
#define SPI_GETPLATFORMTYPE 257
#endif

#ifndef PATH_MAX
#define PATH_MAX FILENAME_MAX
#endif

#ifndef _INTPTR_T_DEFINED
#ifdef  _WIN64
typedef __int64             intptr_t;
#else
#ifdef _W64
typedef _W64 int            intptr_t;
#else
typedef INT_PTR intptr_t;
#endif
#endif
#define _INTPTR_T_DEFINED
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#  define INVALID_FILE_ATTRIBUTES (DWORD (-1))
#endif

#if !defined(REPARSE_DATA_BUFFER_HEADER_SIZE)
typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#  define REPARSE_DATA_BUFFER_HEADER_SIZE  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#endif // !defined(REPARSE_DATA_BUFFER_HEADER_SIZE)

#ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
#  define MAXIMUM_REPARSE_DATA_BUFFER_SIZE 16384
#endif
#ifndef IO_REPARSE_TAG_SYMLINK
#  define IO_REPARSE_TAG_SYMLINK (0xA000000CL)
#endif
#ifndef FSCTL_GET_REPARSE_POINT
#  define FSCTL_GET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#if defined(Q_OS_WINRT) || defined(QT_BOOTSTRAPPED)
#  define QT_FEATURE_fslibs -1
#else
#  define QT_FEATURE_fslibs 1
#endif // Q_OS_WINRT

#if QT_CONFIG(fslibs)
#include <aclapi.h>
#include <userenv.h>
static TRUSTEE_W currentUserTrusteeW;
static TRUSTEE_W worldTrusteeW;
static PSID currentUserSID = 0;
static PSID worldSID = 0;
static HANDLE currentUserImpersonatedToken = nullptr;

QT_BEGIN_NAMESPACE

namespace {
struct GlobalSid
{
    GlobalSid();
    ~GlobalSid();
};

GlobalSid::~GlobalSid()
{
    free(currentUserSID);
    currentUserSID = 0;

    // worldSID was allocated with AllocateAndInitializeSid so it needs to be freed with FreeSid
    if (worldSID) {
        ::FreeSid(worldSID);
        worldSID = 0;
    }

    if (currentUserImpersonatedToken) {
        ::CloseHandle(currentUserImpersonatedToken);
        currentUserImpersonatedToken = nullptr;
    }
}

GlobalSid::GlobalSid()
{
    {
        {
            // Create TRUSTEE for current user
            HANDLE hnd = ::GetCurrentProcess();
            HANDLE token = 0;
            if (::OpenProcessToken(hnd, TOKEN_QUERY, &token)) {
                DWORD retsize = 0;
                // GetTokenInformation requires a buffer big enough for the TOKEN_USER struct and
                // the SID struct. Since the SID struct can have variable number of subauthorities
                // tacked at the end, its size is variable. Obtain the required size by first
                // doing a dummy GetTokenInformation call.
                ::GetTokenInformation(token, TokenUser, 0, 0, &retsize);
                if (retsize) {
                    void *tokenBuffer = malloc(retsize);
                    if (::GetTokenInformation(token, TokenUser, tokenBuffer, retsize, &retsize)) {
                        PSID tokenSid = reinterpret_cast<PTOKEN_USER>(tokenBuffer)->User.Sid;
                        DWORD sidLen = ::GetLengthSid(tokenSid);
                        currentUserSID = reinterpret_cast<PSID>(malloc(sidLen));
                        if (::CopySid(sidLen, currentUserSID, tokenSid))
                            BuildTrusteeWithSid(&currentUserTrusteeW, currentUserSID);
                    }
                    free(tokenBuffer);
                }
                ::CloseHandle(token);
            }

            token = nullptr;
            if (::OpenProcessToken(hnd, TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ, &token)) {
                ::DuplicateToken(token, SecurityImpersonation, &currentUserImpersonatedToken);
                ::CloseHandle(token);
            }

            {
                // Create TRUSTEE for Everyone (World)
                SID_IDENTIFIER_AUTHORITY worldAuth = { SECURITY_WORLD_SID_AUTHORITY };
                if (AllocateAndInitializeSid(&worldAuth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &worldSID))
                    BuildTrusteeWithSid(&worldTrusteeW, worldSID);
            }
        }
    }
}

Q_GLOBAL_STATIC(GlobalSid, initGlobalSid)

QT_END_NAMESPACE

} // anonymous namespace
#endif // QT_CONFIG(fslibs)

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT int qt_ntfs_permission_lookup = 0;

static inline bool toFileTime(const QDateTime &date, FILETIME *fileTime)
{
    SYSTEMTIME sTime;
    if (date.timeSpec() == Qt::LocalTime) {
        SYSTEMTIME lTime;
        const QDate d = date.date();
        const QTime t = date.time();

        lTime.wYear = d.year();
        lTime.wMonth = d.month();
        lTime.wDay = d.day();
        lTime.wHour = t.hour();
        lTime.wMinute = t.minute();
        lTime.wSecond = t.second();
        lTime.wMilliseconds = t.msec();
        lTime.wDayOfWeek = d.dayOfWeek() % 7;

        if (!::TzSpecificLocalTimeToSystemTime(0, &lTime, &sTime))
            return false;
    } else {
        QDateTime utcDate = date.toUTC();
        const QDate d = utcDate.date();
        const QTime t = utcDate.time();

        sTime.wYear = d.year();
        sTime.wMonth = d.month();
        sTime.wDay = d.day();
        sTime.wHour = t.hour();
        sTime.wMinute = t.minute();
        sTime.wSecond = t.second();
        sTime.wMilliseconds = t.msec();
        sTime.wDayOfWeek = d.dayOfWeek() % 7;
    }

    return ::SystemTimeToFileTime(&sTime, fileTime);
}

static QString readSymLink(const QFileSystemEntry &link)
{
    QString result;
#if !defined(Q_OS_WINRT)
    HANDLE handle = CreateFile((wchar_t*)link.nativeFilePath().utf16(),
                               FILE_READ_EA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               0,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                               0);
    if (handle != INVALID_HANDLE_VALUE) {
        DWORD bufsize = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
        REPARSE_DATA_BUFFER *rdb = (REPARSE_DATA_BUFFER*)malloc(bufsize);
        DWORD retsize = 0;
        if (::DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, 0, 0, rdb, bufsize, &retsize, 0)) {
            if (rdb->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
                int length = rdb->MountPointReparseBuffer.SubstituteNameLength / sizeof(wchar_t);
                int offset = rdb->MountPointReparseBuffer.SubstituteNameOffset / sizeof(wchar_t);
                const wchar_t* PathBuffer = &rdb->MountPointReparseBuffer.PathBuffer[offset];
                result = QString::fromWCharArray(PathBuffer, length);
            } else if (rdb->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
                int length = rdb->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(wchar_t);
                int offset = rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(wchar_t);
                const wchar_t* PathBuffer = &rdb->SymbolicLinkReparseBuffer.PathBuffer[offset];
                result = QString::fromWCharArray(PathBuffer, length);
            }
            // cut-off "//?/" and "/??/"
            if (result.size() > 4 && result.at(0) == QLatin1Char('\\') && result.at(2) == QLatin1Char('?') && result.at(3) == QLatin1Char('\\'))
                result = result.mid(4);
        }
        free(rdb);
        CloseHandle(handle);

#if QT_CONFIG(fslibs)
        initGlobalSid();
        QRegExp matchVolName(QLatin1String("^Volume\\{([a-z]|[0-9]|-)+\\}\\\\"), Qt::CaseInsensitive);
        if (matchVolName.indexIn(result) == 0) {
            DWORD len;
            wchar_t buffer[MAX_PATH];
            const QString volumeName = QLatin1String("\\\\?\\") + result.leftRef(matchVolName.matchedLength());
            if (GetVolumePathNamesForVolumeName(reinterpret_cast<LPCWSTR>(volumeName.utf16()), buffer, MAX_PATH, &len) != 0)
                result.replace(0,matchVolName.matchedLength(), QString::fromWCharArray(buffer));
        }
#endif // QT_CONFIG(fslibs)
    }
#else
    Q_UNUSED(link);
#endif // Q_OS_WINRT
    return result;
}

static QString readLink(const QFileSystemEntry &link)
{
#if QT_CONFIG(fslibs)
    QString ret;

    bool neededCoInit = false;
    IShellLink *psl;                            // pointer to IShellLink i/f
    WIN32_FIND_DATA wfd;
    wchar_t szGotPath[MAX_PATH];

    // Get pointer to the IShellLink interface.
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);

    if (hres == CO_E_NOTINITIALIZED) { // COM was not initialized
        neededCoInit = true;
        CoInitialize(NULL);
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IShellLink, (LPVOID *)&psl);
    }
    if (SUCCEEDED(hres)) {    // Get pointer to the IPersistFile interface.
        IPersistFile *ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
        if (SUCCEEDED(hres))  {
            hres = ppf->Load((LPOLESTR)link.nativeFilePath().utf16(), STGM_READ);
            //The original path of the link is retrieved. If the file/folder
            //was moved, the return value still have the old path.
            if (SUCCEEDED(hres)) {
                if (psl->GetPath(szGotPath, MAX_PATH, &wfd, SLGP_UNCPRIORITY) == NOERROR)
                    ret = QString::fromWCharArray(szGotPath);
            }
            ppf->Release();
        }
        psl->Release();
    }
    if (neededCoInit)
        CoUninitialize();

    return ret;
#else
    Q_UNUSED(link);
    return QString();
#endif // QT_CONFIG(fslibs)
}

static bool uncShareExists(const QString &server)
{
    // This code assumes the UNC path is always like \\?\UNC\server...
    const QVector<QStringRef> parts = server.splitRef(QLatin1Char('\\'), QString::SkipEmptyParts);
    if (parts.count() >= 3) {
        QStringList shares;
        if (QFileSystemEngine::uncListSharesOnServer(QLatin1String("\\\\") + parts.at(2), &shares))
            return parts.count() < 4 || shares.contains(parts.at(3).toString(), Qt::CaseInsensitive);
    }
    return false;
}

static inline bool getFindData(QString path, WIN32_FIND_DATA &findData)
{
    // path should not end with a trailing slash
    while (path.endsWith(QLatin1Char('\\')))
        path.chop(1);

    // can't handle drives
    if (!path.endsWith(QLatin1Char(':'))) {
#ifndef Q_OS_WINRT
        HANDLE hFind = ::FindFirstFile((wchar_t*)path.utf16(), &findData);
#else
        HANDLE hFind = ::FindFirstFileEx((const wchar_t*)path.utf16(), FindExInfoStandard, &findData, FindExSearchNameMatch, NULL, 0);
#endif
        if (hFind != INVALID_HANDLE_VALUE) {
            ::FindClose(hFind);
            return true;
        }
    }

    return false;
}

bool QFileSystemEngine::uncListSharesOnServer(const QString &server, QStringList *list)
{
    DWORD res = ERROR_NOT_SUPPORTED;
#ifndef Q_OS_WINRT
    SHARE_INFO_1 *BufPtr, *p;
    DWORD er = 0, tr = 0, resume = 0, i;
    do {
        res = NetShareEnum((wchar_t*)server.utf16(), 1, (LPBYTE *)&BufPtr, DWORD(-1), &er, &tr, &resume);
        if (res == ERROR_SUCCESS || res == ERROR_MORE_DATA) {
            p = BufPtr;
            for (i = 1; i <= er; ++i) {
                if (list && p->shi1_type == 0)
                    list->append(QString::fromWCharArray(p->shi1_netname));
                p++;
            }
        }
        NetApiBufferFree(BufPtr);
    } while (res == ERROR_MORE_DATA);
#else
    Q_UNUSED(server);
    Q_UNUSED(list);
#endif
    return res == ERROR_SUCCESS;
}

void QFileSystemEngine::clearWinStatData(QFileSystemMetaData &data)
{
    data.size_ = 0;
    data.fileAttribute_ =  0;
    data.birthTime_ = FILETIME();
    data.changeTime_ = FILETIME();
    data.lastAccessTime_ = FILETIME();
    data.lastWriteTime_ = FILETIME();
}

//static
QFileSystemEntry QFileSystemEngine::getLinkTarget(const QFileSystemEntry &link,
                                                  QFileSystemMetaData &data)
{
   if (data.missingFlags(QFileSystemMetaData::LinkType))
       QFileSystemEngine::fillMetaData(link, data, QFileSystemMetaData::LinkType);

    QString target;
    if (data.isLnkFile())
        target = readLink(link);
    else if (data.isLink())
        target = readSymLink(link);
    QFileSystemEntry ret(target);
    if (!target.isEmpty() && ret.isRelative()) {
        target.prepend(absoluteName(link).path() + QLatin1Char('/'));
        ret = QFileSystemEntry(QDir::cleanPath(target));
    }
    return ret;
}

//static
QFileSystemEntry QFileSystemEngine::canonicalName(const QFileSystemEntry &entry, QFileSystemMetaData &data)
{
    if (data.missingFlags(QFileSystemMetaData::ExistsAttribute))
       QFileSystemEngine::fillMetaData(entry, data, QFileSystemMetaData::ExistsAttribute);

    if (data.exists())
        return QFileSystemEntry(slowCanonicalized(absoluteName(entry).filePath()));
    else
        return QFileSystemEntry();
}

//static
QString QFileSystemEngine::nativeAbsoluteFilePath(const QString &path)
{
    // can be //server or //server/share
    QString absPath;
    QVarLengthArray<wchar_t, MAX_PATH> buf(qMax(MAX_PATH, path.size() + 1));
    wchar_t *fileName = 0;
    DWORD retLen = GetFullPathName((wchar_t*)path.utf16(), buf.size(), buf.data(), &fileName);
    if (retLen > (DWORD)buf.size()) {
        buf.resize(retLen);
        retLen = GetFullPathName((wchar_t*)path.utf16(), buf.size(), buf.data(), &fileName);
    }
    if (retLen != 0)
        absPath = QString::fromWCharArray(buf.data(), retLen);
#  if defined(Q_OS_WINRT)
    // Win32 returns eg C:/ as root directory with a trailing /.
    // WinRT returns the sandbox root without /.
    // Also C:/../.. returns C:/ on Win32, while for WinRT it steps outside the package
    // and goes beyond package root. Hence force the engine to stay inside
    // the package.
    const QString rootPath = QDir::toNativeSeparators(QDir::rootPath());
    if (absPath.size() < rootPath.size() && rootPath.startsWith(absPath))
        absPath = rootPath;
#  endif // Q_OS_WINRT

    // This is really ugly, but GetFullPathName strips off whitespace at the end.
    // If you for instance write ". " in the lineedit of QFileDialog,
    // (which is an invalid filename) this function will strip the space off and viola,
    // the file is later reported as existing. Therefore, we re-add the whitespace that
    // was at the end of path in order to keep the filename invalid.
    if (!path.isEmpty() && path.at(path.size() - 1) == QLatin1Char(' '))
        absPath.append(QLatin1Char(' '));
    return absPath;
}

//static
QFileSystemEntry QFileSystemEngine::absoluteName(const QFileSystemEntry &entry)
{
    QString ret;

    if (!entry.isRelative()) {
        if (entry.isAbsolute() && entry.isClean())
            ret = entry.filePath();
        else
            ret = QDir::fromNativeSeparators(nativeAbsoluteFilePath(entry.filePath()));
    } else {
        ret = QDir::cleanPath(QDir::currentPath() + QLatin1Char('/') + entry.filePath());
    }

#ifndef Q_OS_WINRT
    // The path should be absolute at this point.
    // From the docs :
    // Absolute paths begin with the directory separator "/"
    // (optionally preceded by a drive specification under Windows).
    if (ret.at(0) != QLatin1Char('/')) {
        Q_ASSERT(ret.length() >= 2);
        Q_ASSERT(ret.at(0).isLetter());
        Q_ASSERT(ret.at(1) == QLatin1Char(':'));

        // Force uppercase drive letters.
        ret[0] = ret.at(0).toUpper();
    }
#endif // !Q_OS_WINRT
    return QFileSystemEntry(ret, QFileSystemEntry::FromInternalPath());
}

#if defined(Q_CC_MINGW) && WINVER < 0x0602 //  Windows 8 onwards

typedef struct _FILE_ID_INFO {
    ULONGLONG VolumeSerialNumber;
    FILE_ID_128 FileId;
} FILE_ID_INFO, *PFILE_ID_INFO;

#endif // if defined (Q_CC_MINGW) && WINVER < 0x0602

// File ID for Windows up to version 7.
static inline QByteArray fileId(HANDLE handle)
{
#ifndef Q_OS_WINRT
    BY_HANDLE_FILE_INFORMATION info;
    if (GetFileInformationByHandle(handle, &info)) {
        char buffer[sizeof "01234567:0123456701234567"];
        qsnprintf(buffer, sizeof(buffer), "%lx:%08lx%08lx",
                  info.dwVolumeSerialNumber,
                  info.nFileIndexHigh,
                  info.nFileIndexLow);
        return buffer;
    }
#else // !Q_OS_WINRT
    Q_UNUSED(handle);
    Q_UNIMPLEMENTED();
#endif // Q_OS_WINRT
    return QByteArray();
}

// File ID for Windows starting from version 8.
QByteArray fileIdWin8(HANDLE handle)
{
#if !defined(QT_BOOTSTRAPPED) && !defined(QT_BUILD_QMAKE)
    QByteArray result;
    FILE_ID_INFO infoEx;
    if (GetFileInformationByHandleEx(handle,
                                     static_cast<FILE_INFO_BY_HANDLE_CLASS>(18), // FileIdInfo in Windows 8
                                     &infoEx, sizeof(FILE_ID_INFO))) {
        result = QByteArray::number(infoEx.VolumeSerialNumber, 16);
        result += ':';
        // Note: MinGW-64's definition of FILE_ID_128 differs from the MSVC one.
        result += QByteArray(reinterpret_cast<const char *>(&infoEx.FileId), int(sizeof(infoEx.FileId))).toHex();
    }
    return result;
#else // !QT_BOOTSTRAPPED && !QT_BUILD_QMAKE
    return fileId(handle);
#endif
}

//static
QByteArray QFileSystemEngine::id(const QFileSystemEntry &entry)
{
    QByteArray result;

#ifndef Q_OS_WINRT
    const HANDLE handle =
        CreateFile((wchar_t*)entry.nativeFilePath().utf16(), 0,
                   FILE_SHARE_READ, NULL, OPEN_EXISTING,
                   FILE_FLAG_BACKUP_SEMANTICS, NULL);
#else // !Q_OS_WINRT
    CREATEFILE2_EXTENDED_PARAMETERS params;
    params.dwSize = sizeof(params);
    params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    params.dwFileFlags = FILE_FLAG_BACKUP_SEMANTICS;
    params.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    params.lpSecurityAttributes = NULL;
    params.hTemplateFile = NULL;
    const HANDLE handle =
        CreateFile2((const wchar_t*)entry.nativeFilePath().utf16(), 0,
                    FILE_SHARE_READ, OPEN_EXISTING, &params);
#endif // Q_OS_WINRT
    if (handle != INVALID_HANDLE_VALUE) {
        result = id(handle);
        CloseHandle(handle);
    }
    return result;
}

//static
QByteArray QFileSystemEngine::id(HANDLE fHandle)
{
    return QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8 ?
                fileIdWin8(HANDLE(fHandle)) : fileId(HANDLE(fHandle));
}

//static
bool QFileSystemEngine::setFileTime(HANDLE fHandle, const QDateTime &newDate,
                                    QAbstractFileEngine::FileTime time, QSystemError &error)
{
    FILETIME fTime;
    FILETIME *pLastWrite = NULL;
    FILETIME *pLastAccess = NULL;
    FILETIME *pCreationTime = NULL;

    switch (time) {
    case QAbstractFileEngine::ModificationTime:
        pLastWrite = &fTime;
        break;

    case QAbstractFileEngine::AccessTime:
        pLastAccess = &fTime;
        break;

    case QAbstractFileEngine::BirthTime:
        pCreationTime = &fTime;
        break;

    default:
        error = QSystemError(ERROR_INVALID_PARAMETER, QSystemError::NativeError);
        return false;
    }

    if (!toFileTime(newDate, &fTime))
        return false;

    if (!::SetFileTime(fHandle, pCreationTime, pLastAccess, pLastWrite)) {
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
        return false;
    }
    return true;
}

QString QFileSystemEngine::owner(const QFileSystemEntry &entry, QAbstractFileEngine::FileOwner own)
{
    QString name;
#if QT_CONFIG(fslibs)
    extern int qt_ntfs_permission_lookup;
    if (qt_ntfs_permission_lookup > 0) {
        initGlobalSid();
        {
            PSID pOwner = 0;
            PSECURITY_DESCRIPTOR pSD;
            if (GetNamedSecurityInfo(reinterpret_cast<const wchar_t*>(entry.nativeFilePath().utf16()), SE_FILE_OBJECT,
                                     own == QAbstractFileEngine::OwnerGroup ? GROUP_SECURITY_INFORMATION : OWNER_SECURITY_INFORMATION,
                                     own == QAbstractFileEngine::OwnerUser ? &pOwner : 0, own == QAbstractFileEngine::OwnerGroup ? &pOwner : 0,
                                     0, 0, &pSD) == ERROR_SUCCESS) {
                DWORD lowner = 64;
                DWORD ldomain = 64;
                QVarLengthArray<wchar_t, 64> owner(lowner);
                QVarLengthArray<wchar_t, 64> domain(ldomain);
                SID_NAME_USE use = SidTypeUnknown;
                // First call, to determine size of the strings (with '\0').
                if (!LookupAccountSid(NULL, pOwner, (LPWSTR)owner.data(), &lowner,
                                      domain.data(), &ldomain, &use)) {
                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                        if (lowner > (DWORD)owner.size())
                            owner.resize(lowner);
                        if (ldomain > (DWORD)domain.size())
                            domain.resize(ldomain);
                        // Second call, try on resized buf-s
                        if (!LookupAccountSid(NULL, pOwner, owner.data(), &lowner,
                                              domain.data(), &ldomain, &use)) {
                            lowner = 0;
                        }
                    } else {
                        lowner = 0;
                    }
                }
                if (lowner != 0)
                    name = QString::fromWCharArray(owner.data());
                LocalFree(pSD);
            }
        }
    }
#else
    Q_UNUSED(entry);
    Q_UNUSED(own);
#endif
    return name;
}

//static
bool QFileSystemEngine::fillPermissions(const QFileSystemEntry &entry, QFileSystemMetaData &data,
                                        QFileSystemMetaData::MetaDataFlags what)
{
#if QT_CONFIG(fslibs)
    if (qt_ntfs_permission_lookup > 0) {
        initGlobalSid();
        {
            enum { ReadMask = 0x00000001, WriteMask = 0x00000002, ExecMask = 0x00000020 };

            QString fname = entry.nativeFilePath();
            PSID pOwner = 0;
            PSID pGroup = 0;
            PACL pDacl;
            PSECURITY_DESCRIPTOR pSD;
            DWORD res = GetNamedSecurityInfo(reinterpret_cast<const wchar_t*>(fname.utf16()), SE_FILE_OBJECT,
                                             OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                             &pOwner, &pGroup, &pDacl, 0, &pSD);
            if(res == ERROR_SUCCESS) {
                ACCESS_MASK access_mask;
                TRUSTEE_W trustee;
                if (what & QFileSystemMetaData::UserPermissions) { // user
                    // Using AccessCheck because GetEffectiveRightsFromAcl doesn't account for elevation
                    if (currentUserImpersonatedToken) {
                        GENERIC_MAPPING mapping = {FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS};
                        PRIVILEGE_SET privileges;
                        DWORD grantedAccess;
                        BOOL result;

                        data.knownFlagsMask |= QFileSystemMetaData::UserPermissions;
                        DWORD genericAccessRights = GENERIC_READ;
                        ::MapGenericMask(&genericAccessRights, &mapping);

                        DWORD privilegesLength = sizeof(privileges);
                        if (::AccessCheck(pSD, currentUserImpersonatedToken, genericAccessRights,
                                          &mapping, &privileges, &privilegesLength, &grantedAccess, &result) && result) {
                            data.entryFlags |= QFileSystemMetaData::UserReadPermission;
                        }

                        privilegesLength = sizeof(privileges);
                        genericAccessRights = GENERIC_WRITE;
                        ::MapGenericMask(&genericAccessRights, &mapping);
                        if (::AccessCheck(pSD, currentUserImpersonatedToken, genericAccessRights,
                                          &mapping, &privileges, &privilegesLength, &grantedAccess, &result) && result) {
                            data.entryFlags |= QFileSystemMetaData::UserWritePermission;
                        }

                        privilegesLength = sizeof(privileges);
                        genericAccessRights = GENERIC_EXECUTE;
                        ::MapGenericMask(&genericAccessRights, &mapping);
                        if (::AccessCheck(pSD, currentUserImpersonatedToken, genericAccessRights,
                                          &mapping, &privileges, &privilegesLength, &grantedAccess, &result) && result) {
                            data.entryFlags |= QFileSystemMetaData::UserExecutePermission;
                        }
                    } else { // fallback to GetEffectiveRightsFromAcl
                        data.knownFlagsMask |= QFileSystemMetaData::UserPermissions;
                        if (GetEffectiveRightsFromAclW(pDacl, &currentUserTrusteeW, &access_mask) != ERROR_SUCCESS)
                            access_mask = ACCESS_MASK(-1);
                        if (access_mask & ReadMask)
                            data.entryFlags |= QFileSystemMetaData::UserReadPermission;
                        if (access_mask & WriteMask)
                            data.entryFlags|= QFileSystemMetaData::UserWritePermission;
                        if (access_mask & ExecMask)
                            data.entryFlags|= QFileSystemMetaData::UserExecutePermission;
                    }
                }
                if (what & QFileSystemMetaData::OwnerPermissions) { // owner
                    data.knownFlagsMask |= QFileSystemMetaData::OwnerPermissions;
                    BuildTrusteeWithSid(&trustee, pOwner);
                    if (GetEffectiveRightsFromAcl(pDacl, &trustee, &access_mask) != ERROR_SUCCESS)
                        access_mask = (ACCESS_MASK)-1;
                    if(access_mask & ReadMask)
                        data.entryFlags |= QFileSystemMetaData::OwnerReadPermission;
                    if(access_mask & WriteMask)
                        data.entryFlags |= QFileSystemMetaData::OwnerWritePermission;
                    if(access_mask & ExecMask)
                        data.entryFlags |= QFileSystemMetaData::OwnerExecutePermission;
                }
                if (what & QFileSystemMetaData::GroupPermissions) { // group
                    data.knownFlagsMask |= QFileSystemMetaData::GroupPermissions;
                    BuildTrusteeWithSid(&trustee, pGroup);
                    if (GetEffectiveRightsFromAcl(pDacl, &trustee, &access_mask) != ERROR_SUCCESS)
                        access_mask = (ACCESS_MASK)-1;
                    if(access_mask & ReadMask)
                        data.entryFlags |= QFileSystemMetaData::GroupReadPermission;
                    if(access_mask & WriteMask)
                        data.entryFlags |= QFileSystemMetaData::GroupWritePermission;
                    if(access_mask & ExecMask)
                        data.entryFlags |= QFileSystemMetaData::GroupExecutePermission;
                }
                if (what & QFileSystemMetaData::OtherPermissions) { // other (world)
                    data.knownFlagsMask |= QFileSystemMetaData::OtherPermissions;
                    if (GetEffectiveRightsFromAcl(pDacl, &worldTrusteeW, &access_mask) != ERROR_SUCCESS)
                        access_mask = (ACCESS_MASK)-1; // ###
                    if(access_mask & ReadMask)
                        data.entryFlags |= QFileSystemMetaData::OtherReadPermission;
                    if(access_mask & WriteMask)
                        data.entryFlags |= QFileSystemMetaData::OtherWritePermission;
                    if(access_mask & ExecMask)
                        data.entryFlags |= QFileSystemMetaData::OwnerExecutePermission;
                }
                LocalFree(pSD);
            }
        }
    } else
#endif
    {
        //### what to do with permissions if we don't use NTFS
        // for now just add all permissions and what about exe missions ??
        // also qt_ntfs_permission_lookup is now not set by default ... should it ?
        data.entryFlags |= QFileSystemMetaData::OwnerReadPermission
                           | QFileSystemMetaData::GroupReadPermission
                           | QFileSystemMetaData::OtherReadPermission;

        if (!(data.fileAttribute_ & FILE_ATTRIBUTE_READONLY)) {
            data.entryFlags |= QFileSystemMetaData::OwnerWritePermission
                   | QFileSystemMetaData::GroupWritePermission
                   | QFileSystemMetaData::OtherWritePermission;
        }

        QString fname = entry.filePath();
        QString ext = fname.right(4).toLower();
        if (data.isDirectory() ||
            ext == QLatin1String(".exe") || ext == QLatin1String(".com") || ext == QLatin1String(".bat") ||
            ext == QLatin1String(".pif") || ext == QLatin1String(".cmd")) {
            data.entryFlags |= QFileSystemMetaData::OwnerExecutePermission | QFileSystemMetaData::GroupExecutePermission
                               | QFileSystemMetaData::OtherExecutePermission | QFileSystemMetaData::UserExecutePermission;
        }
        data.knownFlagsMask |= QFileSystemMetaData::OwnerPermissions | QFileSystemMetaData::GroupPermissions
                                | QFileSystemMetaData::OtherPermissions | QFileSystemMetaData::UserExecutePermission;
        // calculate user permissions
        if (what & QFileSystemMetaData::UserReadPermission) {
            if (::_waccess((wchar_t*)entry.nativeFilePath().utf16(), R_OK) == 0)
                data.entryFlags |= QFileSystemMetaData::UserReadPermission;
            data.knownFlagsMask |= QFileSystemMetaData::UserReadPermission;
        }
        if (what & QFileSystemMetaData::UserWritePermission) {
            if (::_waccess((wchar_t*)entry.nativeFilePath().utf16(), W_OK) == 0)
                data.entryFlags |= QFileSystemMetaData::UserWritePermission;
            data.knownFlagsMask |= QFileSystemMetaData::UserWritePermission;
        }
    }

    return data.hasFlags(what);
}

static bool tryDriveUNCFallback(const QFileSystemEntry &fname, QFileSystemMetaData &data)
{
    bool entryExists = false;
    DWORD fileAttrib = 0;
#if !defined(Q_OS_WINRT)
    if (fname.isDriveRoot()) {
        // a valid drive ??
        const UINT oldErrorMode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
        DWORD drivesBitmask = ::GetLogicalDrives();
        ::SetErrorMode(oldErrorMode);
        int drivebit = 1 << (fname.filePath().at(0).toUpper().unicode() - QLatin1Char('A').unicode());
        if (drivesBitmask & drivebit) {
            fileAttrib = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM;
            entryExists = true;
        }
    } else {
#endif
        const QString &path = fname.nativeFilePath();
        bool is_dir = false;
        if (path.startsWith(QLatin1String("\\\\?\\UNC"))) {
            // UNC - stat doesn't work for all cases (Windows bug)
            int s = path.indexOf(path.at(0),7);
            if (s > 0) {
                // "\\?\UNC\server\..."
                s = path.indexOf(path.at(0),s+1);
                if (s > 0) {
                    // "\\?\UNC\server\share\..."
                    if (s == path.size() - 1) {
                        // "\\?\UNC\server\share\"
                        is_dir = true;
                    } else {
                        // "\\?\UNC\server\share\notfound"
                    }
                } else {
                    // "\\?\UNC\server\share"
                    is_dir = true;
                }
            } else {
                // "\\?\UNC\server"
                is_dir = true;
            }
        }
        if (is_dir && uncShareExists(path)) {
            // looks like a UNC dir, is a dir.
            fileAttrib = FILE_ATTRIBUTE_DIRECTORY;
            entryExists = true;
        }
#if !defined(Q_OS_WINRT)
    }
#endif
    if (entryExists)
        data.fillFromFileAttribute(fileAttrib);
    return entryExists;
}

static bool tryFindFallback(const QFileSystemEntry &fname, QFileSystemMetaData &data)
{
    bool filledData = false;
    // This assumes the last call to a Windows API failed.
    int errorCode = GetLastError();
    if (errorCode == ERROR_ACCESS_DENIED || errorCode == ERROR_SHARING_VIOLATION) {
        WIN32_FIND_DATA findData;
        if (getFindData(fname.nativeFilePath(), findData)
            && findData.dwFileAttributes != INVALID_FILE_ATTRIBUTES) {
            data.fillFromFindData(findData, true, fname.isDriveRoot());
            filledData = true;
        }
    }
    return filledData;
}

//static
bool QFileSystemEngine::fillMetaData(int fd, QFileSystemMetaData &data,
                                     QFileSystemMetaData::MetaDataFlags what)
{
    HANDLE fHandle = (HANDLE)_get_osfhandle(fd);
    if (fHandle  != INVALID_HANDLE_VALUE) {
        return fillMetaData(fHandle, data, what);
    }
    return false;
}

//static
bool QFileSystemEngine::fillMetaData(HANDLE fHandle, QFileSystemMetaData &data,
                                     QFileSystemMetaData::MetaDataFlags what)
{
    data.entryFlags &= ~what;
    clearWinStatData(data);
#ifndef Q_OS_WINRT
    BY_HANDLE_FILE_INFORMATION fileInfo;
    UINT oldmode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    if (GetFileInformationByHandle(fHandle , &fileInfo)) {
        data.fillFromFindInfo(fileInfo);
    }
    SetErrorMode(oldmode);
#else // !Q_OS_WINRT
    FILE_BASIC_INFO fileBasicInfo;
    if (GetFileInformationByHandleEx(fHandle, FileBasicInfo, &fileBasicInfo, sizeof(fileBasicInfo))) {
        data.fillFromFileAttribute(fileBasicInfo.FileAttributes);
        data.birthTime_.dwHighDateTime = fileBasicInfo.CreationTime.HighPart;
        data.birthTime_.dwLowDateTime = fileBasicInfo.CreationTime.LowPart;
        data.changeTime_.dwHighDateTime = fileBasicInfo.ChangeTime.HighPart;
        data.changeTime_.dwLowDateTime = fileBasicInfo.ChangeTime.LowPart;
        data.lastAccessTime_.dwHighDateTime = fileBasicInfo.LastAccessTime.HighPart;
        data.lastAccessTime_.dwLowDateTime = fileBasicInfo.LastAccessTime.LowPart;
        data.lastWriteTime_.dwHighDateTime = fileBasicInfo.LastWriteTime.HighPart;
        data.lastWriteTime_.dwLowDateTime = fileBasicInfo.LastWriteTime.LowPart;
        if (!(data.fileAttribute_ & FILE_ATTRIBUTE_DIRECTORY)) {
            FILE_STANDARD_INFO fileStandardInfo;
            if (GetFileInformationByHandleEx(fHandle, FileStandardInfo, &fileStandardInfo, sizeof(fileStandardInfo)))
                data.size_ = fileStandardInfo.EndOfFile.QuadPart;
        } else
            data.size_ = 0;
        data.knownFlagsMask |=  QFileSystemMetaData::Times | QFileSystemMetaData::SizeAttribute;
    }
#endif // Q_OS_WINRT
    return data.hasFlags(what);
}

static bool isDirPath(const QString &dirPath, bool *existed);

//static
bool QFileSystemEngine::fillMetaData(const QFileSystemEntry &entry, QFileSystemMetaData &data,
                                     QFileSystemMetaData::MetaDataFlags what)
{
    what |= QFileSystemMetaData::WinLnkType | QFileSystemMetaData::WinStatFlags;
    data.entryFlags &= ~what;

    QFileSystemEntry fname;
    data.knownFlagsMask |= QFileSystemMetaData::WinLnkType;
    // Check for ".lnk": Directories named ".lnk" should be skipped, corrupted
    // link files should still be detected as links.
    const QString origFilePath = entry.filePath();
    if (origFilePath.endsWith(QLatin1String(".lnk")) && !isDirPath(origFilePath, 0)) {
        data.entryFlags |= QFileSystemMetaData::WinLnkType;
        fname = QFileSystemEntry(readLink(entry));
    } else {
        fname = entry;
    }

    if (fname.isEmpty()) {
        data.knownFlagsMask |= what;
        clearWinStatData(data);
        return false;
    }

    if (what & QFileSystemMetaData::WinStatFlags) {
#ifndef Q_OS_WINRT
        UINT oldmode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
#endif
        clearWinStatData(data);
        WIN32_FIND_DATA findData;
        // The memory structure for WIN32_FIND_DATA is same as WIN32_FILE_ATTRIBUTE_DATA
        // for all members used by fillFindData().
        bool ok = ::GetFileAttributesEx((wchar_t*)fname.nativeFilePath().utf16(), GetFileExInfoStandard,
                                        reinterpret_cast<WIN32_FILE_ATTRIBUTE_DATA *>(&findData));
        if (ok) {
            data.fillFromFindData(findData, false, fname.isDriveRoot());
        } else {
            if (!tryFindFallback(fname, data))
                if (!tryDriveUNCFallback(fname, data)) {
#ifndef Q_OS_WINRT
                    SetErrorMode(oldmode);
#endif
                    return false;
                }
        }
#ifndef Q_OS_WINRT
        SetErrorMode(oldmode);
#endif
    }

    if (what & QFileSystemMetaData::Permissions)
        fillPermissions(fname, data, what);
    if ((what & QFileSystemMetaData::LinkType)
        && data.missingFlags(QFileSystemMetaData::LinkType)) {
        data.knownFlagsMask |= QFileSystemMetaData::LinkType;
        if (data.fileAttribute_ & FILE_ATTRIBUTE_REPARSE_POINT) {
            WIN32_FIND_DATA findData;
            if (getFindData(fname.nativeFilePath(), findData))
                data.fillFromFindData(findData, true);
        }
    }
    data.knownFlagsMask |= what;
    return data.hasFlags(what);
}

static inline bool mkDir(const QString &path, DWORD *lastError = 0)
{
    if (lastError)
        *lastError = 0;
    const QString longPath = QFSFileEnginePrivate::longFileName(path);
    const bool result = ::CreateDirectory((wchar_t*)longPath.utf16(), 0);
    if (lastError) // Capture lastError before any QString is freed since custom allocators might change it.
        *lastError = GetLastError();
    return result;
}

static inline bool rmDir(const QString &path)
{
    return ::RemoveDirectory((wchar_t*)QFSFileEnginePrivate::longFileName(path).utf16());
}

static bool isDirPath(const QString &dirPath, bool *existed)
{
    QString path = dirPath;
    if (path.length() == 2 && path.at(1) == QLatin1Char(':'))
        path += QLatin1Char('\\');

#ifndef Q_OS_WINRT
    DWORD fileAttrib = ::GetFileAttributes((wchar_t*)QFSFileEnginePrivate::longFileName(path).utf16());
#else // Q_OS_WINRT
    DWORD fileAttrib = INVALID_FILE_ATTRIBUTES;
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (::GetFileAttributesEx((const wchar_t*)QFSFileEnginePrivate::longFileName(path).utf16(), GetFileExInfoStandard, &data))
        fileAttrib = data.dwFileAttributes;
#endif // Q_OS_WINRT
    if (fileAttrib == INVALID_FILE_ATTRIBUTES) {
        int errorCode = GetLastError();
        if (errorCode == ERROR_ACCESS_DENIED || errorCode == ERROR_SHARING_VIOLATION) {
            WIN32_FIND_DATA findData;
            if (getFindData(QFSFileEnginePrivate::longFileName(path), findData))
                fileAttrib = findData.dwFileAttributes;
        }
    }

    if (existed)
        *existed = fileAttrib != INVALID_FILE_ATTRIBUTES;

    if (fileAttrib == INVALID_FILE_ATTRIBUTES)
        return false;

    return fileAttrib & FILE_ATTRIBUTE_DIRECTORY;
}

//static
bool QFileSystemEngine::createDirectory(const QFileSystemEntry &entry, bool createParents)
{
    QString dirName = entry.filePath();
    if (createParents) {
        dirName = QDir::toNativeSeparators(QDir::cleanPath(dirName));
        // We spefically search for / so \ would break it..
        int oldslash = -1;
        if (dirName.startsWith(QLatin1String("\\\\"))) {
            // Don't try to create the root path of a UNC path;
            // CreateDirectory() will just return ERROR_INVALID_NAME.
            for (int i = 0; i < dirName.size(); ++i) {
                if (dirName.at(i) != QDir::separator()) {
                    oldslash = i;
                    break;
                }
            }
            if (oldslash != -1)
                oldslash = dirName.indexOf(QDir::separator(), oldslash);
        } else if (dirName.size() > 2
                && dirName.at(1) == QLatin1Char(':')) {
            // Don't try to call mkdir with just a drive letter
            oldslash = 2;
        }
        for (int slash=0; slash != -1; oldslash = slash) {
            slash = dirName.indexOf(QDir::separator(), oldslash+1);
            if (slash == -1) {
                if (oldslash == dirName.length())
                    break;
                slash = dirName.length();
            }
            if (slash) {
                DWORD lastError;
                QString chunk = dirName.left(slash);
                if (!mkDir(chunk, &lastError)) {
                    if (lastError == ERROR_ALREADY_EXISTS || lastError == ERROR_ACCESS_DENIED) {
                        bool existed = false;
                        if (isDirPath(chunk, &existed) && existed)
                            continue;
#ifdef Q_OS_WINRT
                        static QThreadStorage<QString> dataLocation;
                        if (!dataLocation.hasLocalData())
                            dataLocation.setLocalData(QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DataLocation)));
                        static QThreadStorage<QString> tempLocation;
                        if (!tempLocation.hasLocalData())
                            tempLocation.setLocalData(QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
                        // We try to create something outside the sandbox, which is forbidden
                        // However we could still try to pass into the sandbox
                        if (dataLocation.localData().startsWith(chunk) || tempLocation.localData().startsWith(chunk))
                            continue;
#endif
                    }
                    return false;
                }
            }
        }
        return true;
    }
    return mkDir(entry.filePath());
}

//static
bool QFileSystemEngine::removeDirectory(const QFileSystemEntry &entry, bool removeEmptyParents)
{
    QString dirName = entry.filePath();
    if (removeEmptyParents) {
        dirName = QDir::toNativeSeparators(QDir::cleanPath(dirName));
        for (int oldslash = 0, slash=dirName.length(); slash > 0; oldslash = slash) {
            const QStringRef chunkRef = dirName.leftRef(slash);
            if (chunkRef.length() == 2 && chunkRef.at(0).isLetter() && chunkRef.at(1) == QLatin1Char(':'))
                break;
            const QString chunk = chunkRef.toString();
            if (!isDirPath(chunk, 0))
                return false;
            if (!rmDir(chunk))
                return oldslash != 0;
            slash = dirName.lastIndexOf(QDir::separator(), oldslash-1);
        }
        return true;
    }
    return rmDir(entry.filePath());
}

//static
QString QFileSystemEngine::rootPath()
{
#if defined(Q_OS_WINRT)
    // We specify the package root as root directory
    QString ret = QLatin1String("/");
    // Get package location
    ComPtr<IPackageStatics> statics;
    if (FAILED(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Package).Get(), &statics)))
        return ret;
    ComPtr<IPackage> package;
    if (FAILED(statics->get_Current(&package)))
        return ret;
    ComPtr<IStorageFolder> installedLocation;
    if (FAILED(package->get_InstalledLocation(&installedLocation)))
        return ret;

    ComPtr<IStorageItem> item;
    if (FAILED(installedLocation.As(&item)))
        return ret;

    HString finalWinPath;
    if (FAILED(item->get_Path(finalWinPath.GetAddressOf())))
        return ret;

    const QString qtWinPath = QDir::fromNativeSeparators(QString::fromWCharArray(finalWinPath.GetRawBuffer(nullptr)));
    ret = qtWinPath.endsWith(QLatin1Char('/')) ? qtWinPath : qtWinPath + QLatin1Char('/');
#else
    QString ret = QString::fromLatin1(qgetenv("SystemDrive"));
    if (ret.isEmpty())
        ret = QLatin1String("c:");
    ret.append(QLatin1Char('/'));
#endif
    return ret;
}

//static
QString QFileSystemEngine::homePath()
{
    QString ret;
#if QT_CONFIG(fslibs)
    initGlobalSid();
    {
        HANDLE hnd = ::GetCurrentProcess();
        HANDLE token = 0;
        BOOL ok = ::OpenProcessToken(hnd, TOKEN_QUERY, &token);
        if (ok) {
            DWORD dwBufferSize = 0;
            // First call, to determine size of the strings (with '\0').
            ok = GetUserProfileDirectory(token, NULL, &dwBufferSize);
            if (!ok && dwBufferSize != 0) {        // We got the required buffer size
                wchar_t *userDirectory = new wchar_t[dwBufferSize];
                // Second call, now we can fill the allocated buffer.
                ok = GetUserProfileDirectory(token, userDirectory, &dwBufferSize);
                if (ok)
                    ret = QString::fromWCharArray(userDirectory);
                delete [] userDirectory;
            }
            ::CloseHandle(token);
        }
    }
#endif
    if (ret.isEmpty() || !QFile::exists(ret)) {
        ret = QString::fromLocal8Bit(qgetenv("USERPROFILE"));
        if (ret.isEmpty() || !QFile::exists(ret)) {
            ret = QString::fromLocal8Bit(qgetenv("HOMEDRIVE"))
                  + QString::fromLocal8Bit(qgetenv("HOMEPATH"));
            if (ret.isEmpty() || !QFile::exists(ret)) {
                ret = QString::fromLocal8Bit(qgetenv("HOME"));
                if (ret.isEmpty() || !QFile::exists(ret))
                    ret = rootPath();
            }
        }
    }
    return QDir::fromNativeSeparators(ret);
}

QString QFileSystemEngine::tempPath()
{
    QString ret;
#ifndef Q_OS_WINRT
    wchar_t tempPath[MAX_PATH];
    const DWORD len = GetTempPath(MAX_PATH, tempPath);
    if (len) { // GetTempPath() can return short names, expand.
        wchar_t longTempPath[MAX_PATH];
        const DWORD longLen = GetLongPathName(tempPath, longTempPath, MAX_PATH);
        ret = longLen && longLen < MAX_PATH ?
              QString::fromWCharArray(longTempPath, longLen) :
              QString::fromWCharArray(tempPath, len);
    }
    if (!ret.isEmpty()) {
        while (ret.endsWith(QLatin1Char('\\')))
            ret.chop(1);
        ret = QDir::fromNativeSeparators(ret);
    }
#else // !Q_OS_WINRT
    ComPtr<IApplicationDataStatics> applicationDataStatics;
    if (FAILED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), &applicationDataStatics)))
        return ret;
    ComPtr<IApplicationData> applicationData;
    if (FAILED(applicationDataStatics->get_Current(&applicationData)))
        return ret;
    ComPtr<IStorageFolder> tempFolder;
    if (FAILED(applicationData->get_TemporaryFolder(&tempFolder)))
        return ret;
    ComPtr<IStorageItem> tempFolderItem;
    if (FAILED(tempFolder.As(&tempFolderItem)))
        return ret;
    HString path;
    if (FAILED(tempFolderItem->get_Path(path.GetAddressOf())))
        return ret;
    ret = QDir::fromNativeSeparators(QString::fromWCharArray(path.GetRawBuffer(nullptr)));
#endif // Q_OS_WINRT
    if (ret.isEmpty()) {
        ret = QLatin1String("C:/tmp");
    } else if (ret.length() >= 2 && ret[1] == QLatin1Char(':'))
        ret[0] = ret.at(0).toUpper(); // Force uppercase drive letters.
    return ret;
}

bool QFileSystemEngine::setCurrentPath(const QFileSystemEntry &entry)
{
    QFileSystemMetaData meta;
    fillMetaData(entry, meta, QFileSystemMetaData::ExistsAttribute | QFileSystemMetaData::DirectoryType);
    if(!(meta.exists() && meta.isDirectory()))
        return false;

    //TODO: this should really be using nativeFilePath(), but that returns a path in long format \\?\c:\foo
    //which causes many problems later on when it's returned through currentPath()
    return ::SetCurrentDirectory(reinterpret_cast<const wchar_t*>(QDir::toNativeSeparators(entry.filePath()).utf16())) != 0;
}

QFileSystemEntry QFileSystemEngine::currentPath()
{
    QString ret;
    DWORD size = 0;
    wchar_t currentName[PATH_MAX];
    size = ::GetCurrentDirectory(PATH_MAX, currentName);
    if (size != 0) {
        if (size > PATH_MAX) {
            wchar_t *newCurrentName = new wchar_t[size];
            if (::GetCurrentDirectory(PATH_MAX, newCurrentName) != 0)
                ret = QString::fromWCharArray(newCurrentName, size);
            delete [] newCurrentName;
        } else {
            ret = QString::fromWCharArray(currentName, size);
        }
    }
    if (ret.length() >= 2 && ret[1] == QLatin1Char(':'))
        ret[0] = ret.at(0).toUpper(); // Force uppercase drive letters.
    return QFileSystemEntry(ret, QFileSystemEntry::FromNativePath());
}

//static
bool QFileSystemEngine::createLink(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    Q_ASSERT(false);
    Q_UNUSED(source)
    Q_UNUSED(target)
    Q_UNUSED(error)

    return false; // TODO implement; - code needs to be moved from qfsfileengine_win.cpp
}

//static
bool QFileSystemEngine::copyFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
#ifndef Q_OS_WINRT
    bool ret = ::CopyFile((wchar_t*)source.nativeFilePath().utf16(),
                          (wchar_t*)target.nativeFilePath().utf16(), true) != 0;
#else // !Q_OS_WINRT
    COPYFILE2_EXTENDED_PARAMETERS copyParams = {
        sizeof(copyParams), COPY_FILE_FAIL_IF_EXISTS, NULL, NULL, NULL
    };
    HRESULT hres = ::CopyFile2((const wchar_t*)source.nativeFilePath().utf16(),
                           (const wchar_t*)target.nativeFilePath().utf16(), &copyParams);
    bool ret = SUCCEEDED(hres);
#endif // Q_OS_WINRT
    if(!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
    return ret;
}

//static
bool QFileSystemEngine::renameFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
#ifndef Q_OS_WINRT
    bool ret = ::MoveFile((wchar_t*)source.nativeFilePath().utf16(),
                          (wchar_t*)target.nativeFilePath().utf16()) != 0;
#else // !Q_OS_WINRT
    bool ret = ::MoveFileEx((const wchar_t*)source.nativeFilePath().utf16(),
                            (const wchar_t*)target.nativeFilePath().utf16(), 0) != 0;
#endif // Q_OS_WINRT
    if(!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
    return ret;
}

//static
bool QFileSystemEngine::renameOverwriteFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    bool ret = ::MoveFileEx(reinterpret_cast<const wchar_t *>(source.nativeFilePath().utf16()),
                            reinterpret_cast<const wchar_t *>(target.nativeFilePath().utf16()),
                            MOVEFILE_REPLACE_EXISTING) != 0;
    if (!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
    return ret;
}

//static
bool QFileSystemEngine::removeFile(const QFileSystemEntry &entry, QSystemError &error)
{
    bool ret = ::DeleteFile((wchar_t*)entry.nativeFilePath().utf16()) != 0;
    if(!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
    return ret;
}

//static
bool QFileSystemEngine::setPermissions(const QFileSystemEntry &entry, QFile::Permissions permissions, QSystemError &error,
                                       QFileSystemMetaData *data)
{
    Q_UNUSED(data);
    int mode = 0;

    if (permissions & (QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther))
        mode |= _S_IREAD;
    if (permissions & (QFile::WriteOwner | QFile::WriteUser | QFile::WriteGroup | QFile::WriteOther))
        mode |= _S_IWRITE;

    if (mode == 0) // not supported
        return false;

    bool ret = (::_wchmod((wchar_t*)entry.nativeFilePath().utf16(), mode) == 0);
    if(!ret)
        error = QSystemError(errno, QSystemError::StandardLibraryError);
    return ret;
}

static inline QDateTime fileTimeToQDateTime(const FILETIME *time)
{
    if (time->dwHighDateTime == 0 && time->dwLowDateTime == 0)
        return QDateTime();

    SYSTEMTIME sTime;
    FileTimeToSystemTime(time, &sTime);
    return QDateTime(QDate(sTime.wYear, sTime.wMonth, sTime.wDay),
                     QTime(sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMilliseconds),
                     Qt::UTC);
}

QDateTime QFileSystemMetaData::birthTime() const
{
    return fileTimeToQDateTime(&birthTime_);
}
QDateTime QFileSystemMetaData::metadataChangeTime() const
{
    return fileTimeToQDateTime(&changeTime_);
}
QDateTime QFileSystemMetaData::modificationTime() const
{
    return fileTimeToQDateTime(&lastWriteTime_);
}
QDateTime QFileSystemMetaData::accessTime() const
{
    return fileTimeToQDateTime(&lastAccessTime_);
}

QT_END_NAMESPACE
