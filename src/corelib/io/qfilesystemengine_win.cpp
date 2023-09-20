// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfilesystemengine_p.h"
#include "qoperatingsystemversion.h"
#include "qplatformdefs.h"
#include "qsysinfo.h"
#include "qscopeguard.h"
#include "private/qabstractfileengine_p.h"
#include "private/qfiledevice_p.h"
#include "private/qfsfileengine_p.h"
#include <private/qsystemlibrary_p.h>
#include <qdebug.h>

#include "qdir.h"
#include "qdatetime.h"
#include "qfile.h"
#include "qvarlengtharray.h"
#include "qt_windows.h"
#if QT_CONFIG(regularexpression)
#include "qregularexpression.h"
#endif
#include "qstring.h"

#include <sys/types.h>
#include <direct.h>
#include <winioctl.h>
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <lm.h>
#include <accctrl.h>
#include <initguid.h>
#include <ctype.h>
#include <limits.h>
#define SECURITY_WIN32
#include <security.h>

#include <QtCore/private/qfunctions_win_p.h>

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
#    define FSCTL_GET_REPARSE_POINT                                                                \
        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#if QT_CONFIG(fslibs)
#include <aclapi.h>
#include <authz.h>
#include <userenv.h>
static PSID currentUserSID = nullptr;
static PSID currentGroupSID = nullptr;
static PSID worldSID = nullptr;
static HANDLE currentUserImpersonatedToken = nullptr;
#endif // fslibs

QT_BEGIN_NAMESPACE
using namespace Qt::StringLiterals;

#if QT_CONFIG(fslibs)
namespace {
struct GlobalSid
{
    GlobalSid();
    ~GlobalSid();
};

GlobalSid::~GlobalSid()
{
    free(currentUserSID);
    currentUserSID = nullptr;

    free(currentGroupSID);
    currentGroupSID = nullptr;

    // worldSID was allocated with AllocateAndInitializeSid so it needs to be freed with FreeSid
    if (worldSID) {
        ::FreeSid(worldSID);
        worldSID = nullptr;
    }

    if (currentUserImpersonatedToken) {
        ::CloseHandle(currentUserImpersonatedToken);
        currentUserImpersonatedToken = nullptr;
    }
}

/*
    Helper for GetTokenInformation that allocates chunk of memory to hold the requested information.

    The memory size is determined by doing a dummy call first. The returned memory should be
    freed by calling free().
*/
template<typename T>
static T *getTokenInfo(HANDLE token, TOKEN_INFORMATION_CLASS infoClass)
{
    DWORD retsize = 0;
    GetTokenInformation(token, infoClass, nullptr, 0, &retsize);
    if (retsize) {
        void *tokenBuffer = malloc(retsize);
        if (::GetTokenInformation(token, infoClass, tokenBuffer, retsize, &retsize))
            return reinterpret_cast<T *>(tokenBuffer);
        else
            free(tokenBuffer);
    }
    return nullptr;
}

/*
    Takes a copy of the original SID and stores it into dstSid.
    The copy can be destroyed using free().
*/
static void copySID(PSID &dstSid, PSID srcSid)
{
    DWORD sidLen = GetLengthSid(srcSid);
    dstSid = reinterpret_cast<PSID>(malloc(sidLen));
    Q_CHECK_PTR(dstSid);
    CopySid(sidLen, dstSid, srcSid);
}

GlobalSid::GlobalSid()
{
    HANDLE hnd = ::GetCurrentProcess();
    HANDLE token = nullptr;
    if (::OpenProcessToken(hnd, TOKEN_QUERY, &token)) {
        // Create SID for current user
        if (auto info = getTokenInfo<TOKEN_USER>(token, TokenUser)) {
            copySID(currentUserSID, info->User.Sid);
            free(info);
        }

        // Create SID for the current user's primary group.
        if (auto info = getTokenInfo<TOKEN_GROUPS>(token, TokenGroups)) {
            copySID(currentGroupSID, info->Groups[0].Sid);
            free(info);
        }
        ::CloseHandle(token);
    }

    token = nullptr;
    if (::OpenProcessToken(hnd,
                           TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ,
                           &token)) {
        ::DuplicateToken(token, SecurityImpersonation, &currentUserImpersonatedToken);
        ::CloseHandle(token);
    }

    // Create SID for Everyone (World)
    SID_IDENTIFIER_AUTHORITY worldAuth = { SECURITY_WORLD_SID_AUTHORITY };
    AllocateAndInitializeSid(&worldAuth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &worldSID);
}

Q_GLOBAL_STATIC(GlobalSid, initGlobalSid)

/*!
    \class QAuthzResourceManager
    \internal

    RAII wrapper around Windows Authz resource manager.
*/
class QAuthzResourceManager
{
public:
    QAuthzResourceManager();
    ~QAuthzResourceManager();

    bool isValid() const { return resourceManager != nullptr; }

private:
    friend class QAuthzClientContext;
    Q_DISABLE_COPY_MOVE(QAuthzResourceManager)

    AUTHZ_RESOURCE_MANAGER_HANDLE resourceManager;
};

/*!
    \class QAuthzClientContext
    \internal

    RAII wrapper around Windows Authz client context.
*/
class QAuthzClientContext
{
public:
    // Tag to differentiate SID and TOKEN constructors. Those two types are pointers to void.
    struct TokenTag
    {
    };

    QAuthzClientContext(const QAuthzResourceManager &rm, PSID pSID);
    QAuthzClientContext(const QAuthzResourceManager &rm, HANDLE tokenHandle, TokenTag);

    ~QAuthzClientContext();

    bool isValid() const { return context != nullptr; }

    static constexpr ACCESS_MASK InvalidAccess = ~ACCESS_MASK(0);

    ACCESS_MASK accessMask(PSECURITY_DESCRIPTOR pSD) const;

private:
    Q_DISABLE_COPY_MOVE(QAuthzClientContext)
    AUTHZ_CLIENT_CONTEXT_HANDLE context = nullptr;
};

QAuthzResourceManager::QAuthzResourceManager()
{
    if (!AuthzInitializeResourceManager(AUTHZ_RM_FLAG_NO_AUDIT, nullptr, nullptr, nullptr, nullptr,
                                        &resourceManager)) {
        resourceManager = nullptr;
    }
}

QAuthzResourceManager::~QAuthzResourceManager()
{
    if (resourceManager)
        AuthzFreeResourceManager(resourceManager);
}

/*!
    \internal

    Create an Authz client context from a security identifier.

    The created context will not include any group information associated with \a pSID.
*/
QAuthzClientContext::QAuthzClientContext(const QAuthzResourceManager &rm, PSID pSID)
{
    if (!rm.isValid())
        return;

    LUID unusedId = {};

    if (!AuthzInitializeContextFromSid(AUTHZ_SKIP_TOKEN_GROUPS, pSID, rm.resourceManager, nullptr,
                                       unusedId, nullptr, &context)) {
        context = nullptr;
    }
}

/*!
    \internal

    Create an Authz client context from a token handle.
*/
QAuthzClientContext::QAuthzClientContext(const QAuthzResourceManager &rm, HANDLE tokenHandle,
                                         TokenTag)
{
    if (!rm.isValid())
        return;

    LUID unusedId = {};

    if (!AuthzInitializeContextFromToken(0, tokenHandle, rm.resourceManager, nullptr, unusedId,
                                         nullptr, &context)) {
        context = nullptr;
    }
}

QAuthzClientContext::~QAuthzClientContext()
{
    if (context)
        AuthzFreeContext(context);
}

/*!
    \internal

    Returns permissions that are granted to this client by \a pSD.

    Returns \c InvalidAccess in case of an error.
*/
ACCESS_MASK QAuthzClientContext::accessMask(PSECURITY_DESCRIPTOR pSD) const
{
    if (!isValid())
        return InvalidAccess;

    AUTHZ_ACCESS_REQUEST accessRequest = {};
    AUTHZ_ACCESS_REPLY accessReply = {};
    ACCESS_MASK accessMask = 0;
    DWORD error = 0;

    accessRequest.DesiredAccess = MAXIMUM_ALLOWED;

    accessReply.ResultListLength = 1;
    accessReply.GrantedAccessMask = &accessMask;
    accessReply.Error = &error;

    if (!AuthzAccessCheck(0, context, &accessRequest, nullptr, pSD, nullptr, 0, &accessReply,
                          nullptr)
        || error != 0) {
        return InvalidAccess;
    }

    return accessMask;
}

enum NonSpecificPermission {
    ReadPermission = 0x4,
    WritePermission = 0x2,
    ExePermission = 0x1,
    AllPermissions = ReadPermission | WritePermission | ExePermission
};
Q_DECLARE_FLAGS(NonSpecificPermissions, NonSpecificPermission)
Q_DECLARE_OPERATORS_FOR_FLAGS(NonSpecificPermissions)

enum PermissionTag { OtherTag = 0, GroupTag = 4, UserTag = 8, OwnerTag = 12 };

constexpr NonSpecificPermissions toNonSpecificPermissions(PermissionTag tag,
                                                          QFileDevice::Permissions permissions)
{
    return NonSpecificPermissions::fromInt((permissions.toInt() >> int(tag)) & 0x7);
}

[[maybe_unused]] // Not currently used; included to show how to do it (without bit-rotting).
constexpr QFileDevice::Permissions toSpecificPermissions(PermissionTag tag,
                                                         NonSpecificPermissions permissions)
{
    return QFileDevice::Permissions::fromInt(permissions.toInt() << int(tag));
}

} // anonymous namespace
#endif // QT_CONFIG(fslibs)

#if QT_DEPRECATED_SINCE(6,6)
int qt_ntfs_permission_lookup = 0;
#endif

static QBasicAtomicInt qt_ntfs_permission_lookup_v2 = Q_BASIC_ATOMIC_INITIALIZER(0);

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

/*!
    \internal

    Returns true if the check was previously enabled.
*/

bool qEnableNtfsPermissionChecks() noexcept
{
    return qt_ntfs_permission_lookup_v2.fetchAndAddRelaxed(1)
QT_IF_DEPRECATED_SINCE(6, 6, /*nothing*/, + qt_ntfs_permission_lookup)
        != 0;
}

/*!
    \internal

    Returns true if the check is disabled, i.e. there are no more users.
*/

bool qDisableNtfsPermissionChecks() noexcept
{
    return qt_ntfs_permission_lookup_v2.fetchAndSubRelaxed(1)
QT_IF_DEPRECATED_SINCE(6, 6, /*nothing*/, + qt_ntfs_permission_lookup)
        == 1;
}

/*!
    \internal

    Returns true if the check is enabled.
*/

bool qAreNtfsPermissionChecksEnabled() noexcept
{
    return qt_ntfs_permission_lookup_v2.loadRelaxed()
QT_IF_DEPRECATED_SINCE(6, 6, /*nothing*/, + qt_ntfs_permission_lookup)
        ;
}
QT_WARNING_POP

/*!
    \class QNativeFilePermissions
    \internal

    This class can be used to produce a security descriptor that contains ACL that produces
    result similar to what is expected for POSIX permission corresponding to the supplied
    \c QFileDevice::Permissions value. When supplied optional value is empty, a null
    security descriptor is produced. Files or directories with such null security descriptor
    will inherit ACLs from parent directories. Otherwise an ACL is generated and applied to
    the security descriptor. The created ACL has permission bits set similar to what Cygwin
    does. Unlike Cygwin, this code tries to reorder the access control entries (ACE) inside
    the ACL to match the canonical ordering (deny ACEs followed by allow ACEs) if possible.

    The default ordering of ACEs is as follows:

       * User deny ACE, only lists permission that may be granted by the subsequent Group and
         Other allow ACEs.
       * User allow ACE.
       * Group deny ACE, only lists permissions that may be granted by the subsequent Other
         allow ACE.
       * Group allow ACE.
       * Other allow ACE.

    Any ACEs that would have zero mask are skipped. Group deny ACE may be moved to before
    User allow ACE if these 2 ACEs don't have any common mask bits set. This allows use of
    canonical ordering in more cases. ACLs for permissions with group having less permissions
    than both user and others (ex.: 0757) are still in noncanonical order. Files with
    noncanonical ACLs generate warnings when one tries to edit permissions with Windows GUI,
    and don't work correctly with API like GetEffectiveRightsFromAcl(), but otherwise access
    checks work fine and such ACLs can still be edited with the "Advanced" GUI.
*/
QNativeFilePermissions::QNativeFilePermissions(std::optional<QFileDevice::Permissions> perms,
                                               bool isDir)
{
#if QT_CONFIG(fslibs)
    if (!perms) {
        ok = true;
        return;
    }

    initGlobalSid();

    const auto permissions = *perms;

    PACL acl = reinterpret_cast<PACL>(aclStorage);

    if (!InitializeAcl(acl, sizeof(aclStorage), ACL_REVISION))
        return;

    struct Masks
    {
        ACCESS_MASK denyMask, allowMask;
    };

    auto makeMasks = [isDir](NonSpecificPermissions allowPermissions,
                             NonSpecificPermissions denyPermissions, bool owner) {
        constexpr ACCESS_MASK AllowRead = FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA;
        constexpr ACCESS_MASK DenyRead = FILE_READ_DATA | FILE_READ_EA;

        constexpr ACCESS_MASK AllowWrite =
                FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA;
        constexpr ACCESS_MASK DenyWrite = AllowWrite | FILE_DELETE_CHILD;
        constexpr ACCESS_MASK DenyWriteOwner =
                FILE_WRITE_DATA | FILE_WRITE_EA | FILE_APPEND_DATA | FILE_DELETE_CHILD;

        constexpr ACCESS_MASK AllowExe = FILE_EXECUTE;
        constexpr ACCESS_MASK DenyExe = AllowExe;

        constexpr ACCESS_MASK StdRightsOther =
                STANDARD_RIGHTS_READ | FILE_READ_ATTRIBUTES | SYNCHRONIZE;
        constexpr ACCESS_MASK StdRightsOwner =
                STANDARD_RIGHTS_ALL | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE;

        ACCESS_MASK allow = owner ? StdRightsOwner : StdRightsOther;
        ACCESS_MASK deny = 0;

        if (denyPermissions & ReadPermission)
            deny |= DenyRead;

        if (denyPermissions & WritePermission)
            deny |= owner ? DenyWriteOwner : DenyWrite;

        if (denyPermissions & ExePermission)
            deny |= DenyExe;

        if (allowPermissions & ReadPermission)
            allow |= AllowRead;

        if (allowPermissions & WritePermission)
            allow |= AllowWrite;

        if (allowPermissions & ExePermission)
            allow |= AllowExe;

        // Give the owner "full access" if all the permissions are allowed
        if (owner && allowPermissions == AllPermissions)
            allow |= FILE_DELETE_CHILD;

        if (isDir
            && (allowPermissions & (WritePermission | ExePermission))
                    == (WritePermission | ExePermission)) {
            allow |= FILE_DELETE_CHILD;
        }

        return Masks { deny, allow };
    };

    auto userPermissions = toNonSpecificPermissions(OwnerTag, permissions)
            | toNonSpecificPermissions(UserTag, permissions);
    auto groupPermissions = toNonSpecificPermissions(GroupTag, permissions);
    auto otherPermissions = toNonSpecificPermissions(OtherTag, permissions);

    auto userMasks = makeMasks(userPermissions,
                               ~userPermissions & (groupPermissions | otherPermissions), true);
    auto groupMasks = makeMasks(groupPermissions, ~groupPermissions & otherPermissions, false);
    auto otherMasks = makeMasks(otherPermissions, {}, false);

    const DWORD aceFlags = isDir ? OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE : 0;
    const bool reorderGroupDeny = (groupMasks.denyMask & userMasks.allowMask) == 0;

    const auto addDenyAce = [acl, aceFlags](const Masks &masks, PSID pSID) {
        if (masks.denyMask)
            return AddAccessDeniedAceEx(acl, ACL_REVISION, aceFlags, masks.denyMask, pSID);
        return TRUE;
    };

    const auto addAllowAce = [acl, aceFlags](const Masks &masks, PSID pSID) {
        if (masks.allowMask)
            return AddAccessAllowedAceEx(acl, ACL_REVISION, aceFlags, masks.allowMask, pSID);
        return TRUE;
    };

    if (!addDenyAce(userMasks, currentUserSID))
        return;

    if (reorderGroupDeny) {
        if (!addDenyAce(groupMasks, currentGroupSID))
            return;
    }

    if (!addAllowAce(userMasks, currentUserSID))
        return;

    if (!reorderGroupDeny) {
        if (!addDenyAce(groupMasks, currentGroupSID))
            return;
    }

    if (!addAllowAce(groupMasks, currentGroupSID))
        return;

    if (!addAllowAce(otherMasks, worldSID))
        return;

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        return;

    if (!SetSecurityDescriptorOwner(&sd, currentUserSID, FALSE))
        return;

    if (!SetSecurityDescriptorGroup(&sd, currentGroupSID, FALSE))
        return;

    if (!SetSecurityDescriptorDacl(&sd, TRUE, acl, FALSE))
        return;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    isNull = false;
#else
    Q_UNUSED(perms);
    Q_UNUSED(isDir);
#endif // QT_CONFIG(fslibs)
    ok = true;
}

/*!
    \internal
    Return pointer to a \c SECURITY_ATTRIBUTES object describing the permissions.

    The returned pointer many be null if default permissions were requested or
    during bootstrap. The callers must call \c isOk() to check if the object
    was successfully constructed before using this method.
*/
SECURITY_ATTRIBUTES *QNativeFilePermissions::securityAttributes()
{
    Q_ASSERT(ok);
    return isNull ? nullptr : &sa;
}

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

        if (!::TzSpecificLocalTimeToSystemTime(nullptr, &lTime, &sTime))
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
    HANDLE handle = CreateFile((wchar_t *)link.nativeFilePath().utf16(), FILE_READ_EA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle != INVALID_HANDLE_VALUE) {
        DWORD bufsize = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
        REPARSE_DATA_BUFFER *rdb = (REPARSE_DATA_BUFFER*)malloc(bufsize);
        Q_CHECK_PTR(rdb);
        DWORD retsize = 0;
        if (::DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, nullptr, 0, rdb, bufsize, &retsize,
                              nullptr)) {
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
            // remove "\\?\", "\??\" or "\\?\UNC\"
            result = QFileSystemEntry::removeUncOrLongPathPrefix(result);
        }
        free(rdb);
        CloseHandle(handle);

#if QT_CONFIG(fslibs) && QT_CONFIG(regularexpression)
        initGlobalSid();
        QRegularExpression matchVolumeRe("^Volume\\{([a-z]|[0-9]|-)+\\}\\\\"_L1,
                                         QRegularExpression::CaseInsensitiveOption);
        auto matchVolume = matchVolumeRe.match(result);
        if (matchVolume.hasMatch()) {
            Q_ASSERT(matchVolume.capturedStart() == 0);
            DWORD len;
            wchar_t buffer[MAX_PATH];
            const QString volumeName = "\\\\?\\"_L1 + matchVolume.captured();
            if (GetVolumePathNamesForVolumeName(reinterpret_cast<LPCWSTR>(volumeName.utf16()),
                                                buffer, MAX_PATH, &len)
                != 0) {
                result.replace(0, matchVolume.capturedLength(), QString::fromWCharArray(buffer));
            }
        }
#endif // QT_CONFIG(fslibs)
    }
    return result;
}

static QString readLink(const QFileSystemEntry &link)
{
#if QT_CONFIG(fslibs)
    QString ret;

    IShellLink *psl;                            // pointer to IShellLink i/f
    WIN32_FIND_DATA wfd;
    wchar_t szGotPath[MAX_PATH];

    QComHelper comHelper;

    // Get pointer to the IShellLink interface.
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink,
                                    (LPVOID *)&psl);

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

    return ret;
#else
    Q_UNUSED(link);
    return QString();
#endif // QT_CONFIG(fslibs)
}

static bool uncShareExists(const QString &server)
{
    // This code assumes the UNC path is always like \\?\UNC\server...
    const auto parts = QStringView{server}.split(u'\\', Qt::SkipEmptyParts);
    if (parts.count() >= 3) {
        QStringList shares;
        if (QFileSystemEngine::uncListSharesOnServer("\\\\"_L1 + parts.at(2), &shares))
            return parts.count() < 4
                    || shares.contains(parts.at(3).toString(), Qt::CaseInsensitive);
    }
    return false;
}

static inline bool getFindData(QString path, WIN32_FIND_DATA &findData)
{
    // path should not end with a trailing slash
    while (path.endsWith(u'\\'))
        path.chop(1);

    // can't handle drives
    if (!path.endsWith(u':')) {
        HANDLE hFind = ::FindFirstFile((wchar_t*)path.utf16(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            ::FindClose(hFind);
            return true;
        }
    }

    return false;
}

class FileOperationProgressSink : public IFileOperationProgressSink
{
public:
    FileOperationProgressSink()
    : ref(1)
    {}
    virtual ~FileOperationProgressSink() {}

    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--ref == 0) {
            delete this;
            return 0;
        }
        return ref;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;

        *ppvObject = nullptr;

        if (iid == __uuidof(IUnknown)) {
            *ppvObject = static_cast<IUnknown*>(this);
        } else if (iid == __uuidof(IFileOperationProgressSink)) {
            *ppvObject = static_cast<IFileOperationProgressSink*>(this);
        }

        if (*ppvObject) {
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE StartOperations() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE FinishOperations(HRESULT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreRenameItem(DWORD, IShellItem *, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostRenameItem(DWORD, IShellItem *, LPCWSTR, HRESULT,
                                             IShellItem *) override
    { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreMoveItem(DWORD, IShellItem *, IShellItem *, LPCWSTR) override
    { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostMoveItem(DWORD, IShellItem *, IShellItem *, LPCWSTR, HRESULT,
                                           IShellItem *) override
    { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreCopyItem(DWORD, IShellItem *, IShellItem *, LPCWSTR) override
    { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostCopyItem(DWORD, IShellItem *, IShellItem *, LPCWSTR, HRESULT,
                                           IShellItem *) override
    { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD dwFlags, IShellItem *) override
    {
        // stop the operation if the file will be deleted rather than trashed
        return (dwFlags & TSF_DELETE_RECYCLE_IF_POSSIBLE) ? S_OK : E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE PostDeleteItem(DWORD /* dwFlags */, IShellItem * /* psiItem */,
                                             HRESULT hrDelete,
                                             IShellItem *psiNewlyCreated) override
    {
        deleteResult = hrDelete;
        if (psiNewlyCreated) {
            wchar_t *pszName = nullptr;
            psiNewlyCreated->GetDisplayName(SIGDN_FILESYSPATH, &pszName);
            if (pszName) {
                targetPath = QString::fromWCharArray(pszName);
                CoTaskMemFree(pszName);
            }
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE PreNewItem(DWORD, IShellItem *, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostNewItem(DWORD, IShellItem *, LPCWSTR, LPCWSTR, DWORD, HRESULT,
                                          IShellItem *) override
    { return S_OK; }
    HRESULT STDMETHODCALLTYPE UpdateProgress(UINT, UINT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE ResetTimer() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PauseTimer() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE ResumeTimer() override { return S_OK; }

    QString targetPath;
    HRESULT deleteResult = S_OK;
private:
    ULONG ref;
};

bool QFileSystemEngine::uncListSharesOnServer(const QString &server, QStringList *list)
{
    DWORD res = ERROR_NOT_SUPPORTED;
    SHARE_INFO_1 *BufPtr, *p;
    DWORD er = 0, tr = 0, resume = 0, i;
    do {
        res = NetShareEnum((wchar_t *)server.utf16(), 1, (LPBYTE *)&BufPtr, DWORD(-1), &er, &tr,
                           &resume);
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
    QFileSystemEntry ret = getRawLinkPath(link, data);
    if (!ret.isEmpty() && ret.isRelative()) {
        QString target = absoluteName(link).path() + u'/' + ret.filePath();
        ret = QFileSystemEntry(QDir::cleanPath(target));
    }
    return ret;
}

//static
QFileSystemEntry QFileSystemEngine::getRawLinkPath(const QFileSystemEntry &link,
                                                   QFileSystemMetaData &data)
{
    Q_CHECK_FILE_NAME(link, link);

    if (data.missingFlags(QFileSystemMetaData::LinkType))
       QFileSystemEngine::fillMetaData(link, data, QFileSystemMetaData::LinkType);

    QString target;
    if (data.isLnkFile())
        target = readLink(link);
    else if (data.isLink())
        target = readSymLink(link);
    return QFileSystemEntry(target);
}

//static
QFileSystemEntry QFileSystemEngine::junctionTarget(const QFileSystemEntry &link,
                                                   QFileSystemMetaData &data)
{
    Q_CHECK_FILE_NAME(link, link);

    if (data.missingFlags(QFileSystemMetaData::JunctionType))
       QFileSystemEngine::fillMetaData(link, data, QFileSystemMetaData::LinkType);

    QString target;
    if (data.isJunction())
        target = readSymLink(link);
    QFileSystemEntry ret(target);
    if (!target.isEmpty() && ret.isRelative()) {
        target.prepend(absoluteName(link).path() + u'/');
        ret = QFileSystemEntry(QDir::cleanPath(target));
    }
    return ret;
}

//static
QFileSystemEntry QFileSystemEngine::canonicalName(const QFileSystemEntry &entry,
                                                  QFileSystemMetaData &data)
{
    Q_CHECK_FILE_NAME(entry, entry);

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
    Q_CHECK_FILE_NAME(path, QString());

    // can be //server or //server/share
    QString absPath;
    QVarLengthArray<wchar_t, MAX_PATH> buf(qMax(MAX_PATH, path.size() + 1));
    wchar_t *fileName = nullptr;
    DWORD retLen = GetFullPathName((wchar_t*)path.utf16(), buf.size(), buf.data(), &fileName);
    if (retLen > (DWORD)buf.size()) {
        buf.resize(retLen);
        retLen = GetFullPathName((wchar_t*)path.utf16(), buf.size(), buf.data(), &fileName);
    }
    if (retLen != 0)
        absPath = QString::fromWCharArray(buf.data(), retLen);

    // This is really ugly, but GetFullPathName strips off whitespace at the end.
    // If you for instance write ". " in the lineedit of QFileDialog,
    // (which is an invalid filename) this function will strip the space off and viola,
    // the file is later reported as existing. Therefore, we re-add the whitespace that
    // was at the end of path in order to keep the filename invalid.
    if (!path.isEmpty() && path.at(path.size() - 1) == u' ')
        absPath.append(u' ');
    return absPath;
}

//static
QFileSystemEntry QFileSystemEngine::absoluteName(const QFileSystemEntry &entry)
{
    Q_CHECK_FILE_NAME(entry, entry);

    QString ret;

    if (!entry.isRelative()) {
        if (entry.isAbsolute() && entry.isClean())
            ret = entry.filePath();
        else
            ret = QDir::fromNativeSeparators(nativeAbsoluteFilePath(entry.filePath()));
    } else {
        ret = QDir::cleanPath(QDir::currentPath() + u'/' + entry.filePath());
    }

    // The path should be absolute at this point.
    // From the docs :
    // Absolute paths begin with the directory separator "/"
    // (optionally preceded by a drive specification under Windows).
    if (ret.at(0) != u'/') {
        Q_ASSERT(ret.length() >= 2);
        Q_ASSERT(ret.at(0).isLetter());
        Q_ASSERT(ret.at(1) == u':');

        // Force uppercase drive letters.
        ret[0] = ret.at(0).toUpper();
    }
    return QFileSystemEntry(ret, QFileSystemEntry::FromInternalPath());
}

// File ID for Windows up to version 7 and FAT32 drives
static inline QByteArray fileId(HANDLE handle)
{
    BY_HANDLE_FILE_INFORMATION info;
    if (GetFileInformationByHandle(handle, &info)) {
        char buffer[sizeof "01234567:0123456701234567"];
        qsnprintf(buffer, sizeof(buffer), "%lx:%08lx%08lx",
                  info.dwVolumeSerialNumber,
                  info.nFileIndexHigh,
                  info.nFileIndexLow);
        return buffer;
    }
    return QByteArray();
}

// File ID for Windows starting from version 8.
QByteArray fileIdWin8(HANDLE handle)
{
#if !defined(QT_BOOTSTRAPPED)
    QByteArray result;
    FILE_ID_INFO infoEx;
    if (GetFileInformationByHandleEx(
                handle,
                static_cast<FILE_INFO_BY_HANDLE_CLASS>(18), // FileIdInfo in Windows 8
                &infoEx, sizeof(FILE_ID_INFO))) {
        result = QByteArray::number(infoEx.VolumeSerialNumber, 16);
        result += ':';
        // Note: MinGW-64's definition of FILE_ID_128 differs from the MSVC one.
        result += QByteArray(reinterpret_cast<const char *>(&infoEx.FileId),
                             int(sizeof(infoEx.FileId)))
                          .toHex();
    } else {
        // GetFileInformationByHandleEx() is observed to fail for FAT32, QTBUG-74759
        result = fileId(handle);
    }
    return result;
#else // !QT_BOOTSTRAPPED
    return fileId(handle);
#endif
}

//static
QByteArray QFileSystemEngine::id(const QFileSystemEntry &entry)
{
    Q_CHECK_FILE_NAME(entry, QByteArray());

    QByteArray result;

    const HANDLE handle = CreateFile((wchar_t *)entry.nativeFilePath().utf16(), 0, FILE_SHARE_READ,
                                     nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle != INVALID_HANDLE_VALUE) {
        result = id(handle);
        CloseHandle(handle);
    }
    return result;
}

//static
QByteArray QFileSystemEngine::id(HANDLE fHandle)
{
    return fileIdWin8(HANDLE(fHandle));
}

//static
bool QFileSystemEngine::setFileTime(HANDLE fHandle, const QDateTime &newDate,
                                    QAbstractFileEngine::FileTime time, QSystemError &error)
{
    FILETIME fTime;
    FILETIME *pLastWrite = nullptr;
    FILETIME *pLastAccess = nullptr;
    FILETIME *pCreationTime = nullptr;

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
    if (qAreNtfsPermissionChecksEnabled()) {
        initGlobalSid();
        {
            PSID pOwner = 0;
            PSECURITY_DESCRIPTOR pSD;
            if (GetNamedSecurityInfo(
                        reinterpret_cast<const wchar_t *>(entry.nativeFilePath().utf16()),
                        SE_FILE_OBJECT,
                        own == QAbstractFileEngine::OwnerGroup ? GROUP_SECURITY_INFORMATION
                                                               : OWNER_SECURITY_INFORMATION,
                        own == QAbstractFileEngine::OwnerUser ? &pOwner : nullptr,
                        own == QAbstractFileEngine::OwnerGroup ? &pOwner : nullptr, nullptr,
                        nullptr, &pSD)
                == ERROR_SUCCESS) {
                DWORD lowner = 64;
                DWORD ldomain = 64;
                QVarLengthArray<wchar_t, 64> owner(lowner);
                QVarLengthArray<wchar_t, 64> domain(ldomain);
                SID_NAME_USE use = SidTypeUnknown;
                // First call, to determine size of the strings (with '\0').
                if (!LookupAccountSid(nullptr, pOwner, (LPWSTR)owner.data(), &lowner, domain.data(),
                                      &ldomain, &use)) {
                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                        if (lowner > (DWORD)owner.size())
                            owner.resize(lowner);
                        if (ldomain > (DWORD)domain.size())
                            domain.resize(ldomain);
                        // Second call, try on resized buf-s
                        if (!LookupAccountSid(nullptr, pOwner, owner.data(), &lowner, domain.data(),
                                              &ldomain, &use)) {
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
    if (qAreNtfsPermissionChecksEnabled()) {
        initGlobalSid();

        QString fname = entry.nativeFilePath();
        PSID pOwner;
        PSID pGroup;
        PACL pDacl;
        PSECURITY_DESCRIPTOR pSD;

        // pDacl is unused directly by the code below, but it is still needed here because
        // access checks below return incorrect results unless DACL_SECURITY_INFORMATION is
        // passed to this call.
        DWORD res = GetNamedSecurityInfo(
                reinterpret_cast<const wchar_t *>(fname.utf16()), SE_FILE_OBJECT,
                OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                &pOwner, &pGroup, &pDacl, nullptr, &pSD);
        if (res == ERROR_SUCCESS) {
            QAuthzResourceManager rm;

            auto addPermissions = [&data](ACCESS_MASK accessMask,
                                          QFileSystemMetaData::MetaDataFlag readFlags,
                                          QFileSystemMetaData::MetaDataFlag writeFlags,
                                          QFileSystemMetaData::MetaDataFlag executeFlags) {
                // Check for generic permissions and file-specific bits that most closely
                // represent POSIX permissions.

                // Constants like FILE_GENERIC_{READ,WRITE,EXECUTE} cannot be used
                // here because they contain permission bits shared between all of them.
                if (accessMask & (GENERIC_READ | FILE_READ_DATA))
                    data.entryFlags |= readFlags;
                if (accessMask & (GENERIC_WRITE | FILE_WRITE_DATA))
                    data.entryFlags |= writeFlags;
                if (accessMask & (GENERIC_EXECUTE | FILE_EXECUTE))
                    data.entryFlags |= executeFlags;
            };

            if (what & QFileSystemMetaData::UserPermissions && currentUserImpersonatedToken) {
                data.knownFlagsMask |= QFileSystemMetaData::UserPermissions;
                QAuthzClientContext context(rm, currentUserImpersonatedToken,
                                            QAuthzClientContext::TokenTag {});
                addPermissions(context.accessMask(pSD),
                               QFileSystemMetaData::UserReadPermission,
                               QFileSystemMetaData::UserWritePermission,
                               QFileSystemMetaData::UserExecutePermission);
            }

            if (what & QFileSystemMetaData::OwnerPermissions) {
                data.knownFlagsMask |= QFileSystemMetaData::OwnerPermissions;
                QAuthzClientContext context(rm, pOwner);
                addPermissions(context.accessMask(pSD),
                               QFileSystemMetaData::OwnerReadPermission,
                               QFileSystemMetaData::OwnerWritePermission,
                               QFileSystemMetaData::OwnerExecutePermission);
            }

            if (what & QFileSystemMetaData::GroupPermissions) {
                data.knownFlagsMask |= QFileSystemMetaData::GroupPermissions;
                QAuthzClientContext context(rm, pGroup);
                addPermissions(context.accessMask(pSD),
                               QFileSystemMetaData::GroupReadPermission,
                               QFileSystemMetaData::GroupWritePermission,
                               QFileSystemMetaData::GroupExecutePermission);
            }

            if (what & QFileSystemMetaData::OtherPermissions) {
                data.knownFlagsMask |= QFileSystemMetaData::OtherPermissions;
                QAuthzClientContext context(rm, worldSID);
                addPermissions(context.accessMask(pSD),
                               QFileSystemMetaData::OtherReadPermission,
                               QFileSystemMetaData::OtherWritePermission,
                               QFileSystemMetaData::OtherExecutePermission);
            }

            LocalFree(pSD);
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
        if (data.isDirectory() || ext == ".exe"_L1 || ext == ".com"_L1
            || ext == ".bat"_L1 || ext == ".pif"_L1 || ext == ".cmd"_L1) {
            data.entryFlags |= QFileSystemMetaData::OwnerExecutePermission
                    | QFileSystemMetaData::GroupExecutePermission
                    | QFileSystemMetaData::OtherExecutePermission
                    | QFileSystemMetaData::UserExecutePermission;
        }
        data.knownFlagsMask |= QFileSystemMetaData::OwnerPermissions
                | QFileSystemMetaData::GroupPermissions | QFileSystemMetaData::OtherPermissions
                | QFileSystemMetaData::UserExecutePermission;
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
    if (fname.isDriveRoot()) {
        // a valid drive ??
        const UINT oldErrorMode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
        DWORD drivesBitmask = ::GetLogicalDrives();
        ::SetErrorMode(oldErrorMode);
        int drivebit =
                1 << (fname.filePath().at(0).toUpper().unicode() - u'A');
        if (drivesBitmask & drivebit) {
            fileAttrib = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM;
            entryExists = true;
        }
    } else {
        const QString &path = fname.nativeFilePath();
        bool is_dir = false;
        if (path.startsWith("\\\\?\\UNC"_L1)) {
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
    }
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
    auto fHandle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
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
    BY_HANDLE_FILE_INFORMATION fileInfo;
    UINT oldmode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    if (GetFileInformationByHandle(fHandle , &fileInfo)) {
        data.fillFromFindInfo(fileInfo);
    }
    SetErrorMode(oldmode);
    return data.hasFlags(what);
}

//static
bool QFileSystemEngine::fillMetaData(const QFileSystemEntry &entry, QFileSystemMetaData &data,
                                     QFileSystemMetaData::MetaDataFlags what)
{
    Q_CHECK_FILE_NAME(entry, false);
    what |= QFileSystemMetaData::WinLnkType | QFileSystemMetaData::WinStatFlags;
    data.entryFlags &= ~what;

    QFileSystemEntry fname;
    data.knownFlagsMask |= QFileSystemMetaData::WinLnkType;
    // Check for ".lnk": Directories named ".lnk" should be skipped, corrupted
    // link files should still be detected as links.
    const QString origFilePath = entry.filePath();
    if (origFilePath.endsWith(".lnk"_L1) && !isDirPath(origFilePath, nullptr)) {
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
        UINT oldmode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
        clearWinStatData(data);
        WIN32_FIND_DATA findData;
        // The memory structure for WIN32_FIND_DATA is same as WIN32_FILE_ATTRIBUTE_DATA
        // for all members used by fillFindData().
        bool ok = ::GetFileAttributesEx(
                reinterpret_cast<const wchar_t *>(fname.nativeFilePath().utf16()),
                GetFileExInfoStandard, reinterpret_cast<WIN32_FILE_ATTRIBUTE_DATA *>(&findData));
        if (ok) {
            data.fillFromFindData(findData, false, fname.isDriveRoot());
        } else {
            const DWORD lastError = GetLastError();
            // disconnected drive
            if (lastError == ERROR_LOGON_FAILURE || lastError == ERROR_BAD_NETPATH
                || (!tryFindFallback(fname, data) && !tryDriveUNCFallback(fname, data))) {
                data.clearFlags();
                SetErrorMode(oldmode);
                return false;
            }
        }
        SetErrorMode(oldmode);
    }

    if (what & QFileSystemMetaData::Permissions)
        fillPermissions(fname, data, what);
    if (what & QFileSystemMetaData::LinkType) {
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

static inline bool mkDir(const QString &path, SECURITY_ATTRIBUTES *securityAttributes,
                         DWORD *lastError = nullptr)
{
    if (lastError)
        *lastError = 0;
    const QString longPath = QFSFileEnginePrivate::longFileName(path);
    const bool result = ::CreateDirectory((wchar_t *)longPath.utf16(), securityAttributes);
    // Capture lastError before any QString is freed since custom allocators might change it.
    if (lastError)
        *lastError = GetLastError();
    return result;
}

static inline bool rmDir(const QString &path)
{
    return ::RemoveDirectory((wchar_t*)QFSFileEnginePrivate::longFileName(path).utf16());
}

//static
bool QFileSystemEngine::isDirPath(const QString &dirPath, bool *existed)
{
    QString path = dirPath;
    if (path.length() == 2 && path.at(1) == u':')
        path += u'\\';

    const QString longPath = QFSFileEnginePrivate::longFileName(path);
    DWORD fileAttrib = ::GetFileAttributes(reinterpret_cast<const wchar_t*>(longPath.utf16()));
    if (fileAttrib == INVALID_FILE_ATTRIBUTES) {
        int errorCode = GetLastError();
        if (errorCode == ERROR_ACCESS_DENIED || errorCode == ERROR_SHARING_VIOLATION) {
            WIN32_FIND_DATA findData;
            if (getFindData(longPath, findData))
                fileAttrib = findData.dwFileAttributes;
        }
    }

    if (existed)
        *existed = fileAttrib != INVALID_FILE_ATTRIBUTES;

    if (fileAttrib == INVALID_FILE_ATTRIBUTES)
        return false;

    return fileAttrib & FILE_ATTRIBUTE_DIRECTORY;
}

// NOTE: if \a shouldMkdirFirst is false, we assume the caller did try to mkdir
// before calling this function.
static bool createDirectoryWithParents(const QString &nativeName,
                                       SECURITY_ATTRIBUTES *securityAttributes,
                                       bool shouldMkdirFirst = true)
{
    const auto isUNCRoot = [](const QString &nativeName) {
        return nativeName.startsWith("\\\\"_L1)
                && nativeName.count(QDir::separator()) <= 3;
    };
    const auto isDriveName = [](const QString &nativeName) {
        return nativeName.size() == 2 && nativeName.at(1) == u':';
    };
    const auto isDir = [](const QString &nativeName) {
        bool exists = false;
        return QFileSystemEngine::isDirPath(nativeName, &exists) && exists;
    };
    // Do not try to mkdir a UNC root path or a drive letter.
    if (isUNCRoot(nativeName) || isDriveName(nativeName))
        return false;

    if (shouldMkdirFirst) {
        if (mkDir(nativeName, securityAttributes))
            return true;
    }

    const int backSlash = nativeName.lastIndexOf(QDir::separator());
    if (backSlash < 1)
        return false;

    const QString parentNativeName = nativeName.left(backSlash);
    if (!createDirectoryWithParents(parentNativeName, securityAttributes))
        return false;

    // try again
    if (mkDir(nativeName, securityAttributes))
        return true;
    return isDir(nativeName);
}

//static
bool QFileSystemEngine::createDirectory(const QFileSystemEntry &entry, bool createParents,
                                        std::optional<QFile::Permissions> permissions)
{
    QString dirName = entry.filePath();
    Q_CHECK_FILE_NAME(dirName, false);

    dirName = QDir::toNativeSeparators(QDir::cleanPath(dirName));

    QNativeFilePermissions nativePermissions(permissions, true);
    if (!nativePermissions.isOk())
        return false;

    auto securityAttributes = nativePermissions.securityAttributes();

    // try to mkdir this directory
    DWORD lastError;
    if (mkDir(dirName, securityAttributes, &lastError))
        return true;
    // mkpath should return true, if the directory already exists, mkdir false.
    if (!createParents)
        return false;
    if (lastError == ERROR_ALREADY_EXISTS || lastError == ERROR_ACCESS_DENIED)
        return isDirPath(dirName, nullptr);

    return createDirectoryWithParents(dirName, securityAttributes, false);
}

//static
bool QFileSystemEngine::removeDirectory(const QFileSystemEntry &entry, bool removeEmptyParents)
{
    QString dirName = entry.filePath();
    Q_CHECK_FILE_NAME(dirName, false);

    if (removeEmptyParents) {
        dirName = QDir::toNativeSeparators(QDir::cleanPath(dirName));
        for (int oldslash = 0, slash=dirName.length(); slash > 0; oldslash = slash) {
            const auto chunkRef = QStringView{dirName}.left(slash);
            if (chunkRef.length() == 2 && chunkRef.at(0).isLetter()
                && chunkRef.at(1) == u':') {
                break;
            }
            const QString chunk = chunkRef.toString();
            if (!isDirPath(chunk, nullptr))
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
    QString ret = QString::fromLatin1(qgetenv("SystemDrive"));
    if (ret.isEmpty())
        ret = "c:"_L1;
    ret.append(u'/');
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
        HANDLE token = nullptr;
        BOOL ok = ::OpenProcessToken(hnd, TOKEN_QUERY, &token);
        if (ok) {
            DWORD dwBufferSize = 0;
            // First call, to determine size of the strings (with '\0').
            ok = GetUserProfileDirectory(token, nullptr, &dwBufferSize);
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
        while (ret.endsWith(u'\\'))
            ret.chop(1);
        ret = QDir::fromNativeSeparators(ret);
    }
    if (ret.isEmpty()) {
        ret = "C:/tmp"_L1;
    } else if (ret.length() >= 2 && ret[1] == u':')
        ret[0] = ret.at(0).toUpper(); // Force uppercase drive letters.
    return ret;
}

bool QFileSystemEngine::setCurrentPath(const QFileSystemEntry &entry)
{
    QFileSystemMetaData meta;
    fillMetaData(entry, meta,
                 QFileSystemMetaData::ExistsAttribute | QFileSystemMetaData::DirectoryType);
    if (!(meta.exists() && meta.isDirectory()))
        return false;

    // TODO: this should really be using nativeFilePath(), but that returns a path
    // in long format \\?\c:\foo which causes many problems later on when it's
    // returned through currentPath()
    return ::SetCurrentDirectory(reinterpret_cast<const wchar_t *>(
                   QDir::toNativeSeparators(entry.filePath()).utf16()))
            != 0;
}

QFileSystemEntry QFileSystemEngine::currentPath()
{
    QString ret(PATH_MAX, Qt::Uninitialized);
    DWORD size = GetCurrentDirectoryW(PATH_MAX, reinterpret_cast<wchar_t *>(ret.data()));
    if (size > PATH_MAX) {
        // try again after enlarging the buffer
        ret.resize(size);
        size = GetCurrentDirectoryW(size, reinterpret_cast<wchar_t *>(ret.data()));

        // note: the current directory may have changed underneath us; if the
        // new one is even bigger, we may return a truncated string!
    }
    if (size >= 2 && ret.at(1) == u':')
        ret[0] = ret.at(0).toUpper(); // Force uppercase drive letters.
    ret.resize(size);
    return QFileSystemEntry(std::move(ret), QFileSystemEntry::FromNativePath());
}

//static
bool QFileSystemEngine::createLink(const QFileSystemEntry &source, const QFileSystemEntry &target,
                                   QSystemError &error)
{
    bool ret = false;
    QComHelper comHelper;
    IShellLink *psl = nullptr;
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink,
                                    reinterpret_cast<void **>(&psl));

    if (SUCCEEDED(hres)) {
        const auto name = QDir::toNativeSeparators(source.filePath());
        const auto pathName = QDir::toNativeSeparators(source.path());
        if (SUCCEEDED(psl->SetPath(reinterpret_cast<const wchar_t *>(name.utf16())))
            && SUCCEEDED(psl->SetWorkingDirectory(
                    reinterpret_cast<const wchar_t *>(pathName.utf16())))) {
            IPersistFile *ppf = nullptr;
            if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void **>(&ppf)))) {
                ret = SUCCEEDED(ppf->Save(
                        reinterpret_cast<const wchar_t *>(target.filePath().utf16()), TRUE));
                ppf->Release();
            }
        }
        psl->Release();
    }

    if (!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);

    return ret;
}

//static
bool QFileSystemEngine::copyFile(const QFileSystemEntry &source, const QFileSystemEntry &target,
                                 QSystemError &error)
{
    bool ret = ::CopyFile((wchar_t*)source.nativeFilePath().utf16(),
                          (wchar_t*)target.nativeFilePath().utf16(), true) != 0;
    if (!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
    return ret;
}

//static
bool QFileSystemEngine::renameFile(const QFileSystemEntry &source, const QFileSystemEntry &target,
                                   QSystemError &error)
{
    Q_CHECK_FILE_NAME(source, false);
    Q_CHECK_FILE_NAME(target, false);

    bool ret = ::MoveFile((wchar_t*)source.nativeFilePath().utf16(),
                          (wchar_t*)target.nativeFilePath().utf16()) != 0;
    if (!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
    return ret;
}

//static
bool QFileSystemEngine::renameOverwriteFile(const QFileSystemEntry &source,
                                            const QFileSystemEntry &target, QSystemError &error)
{
    Q_CHECK_FILE_NAME(source, false);
    Q_CHECK_FILE_NAME(target, false);

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
    Q_CHECK_FILE_NAME(entry, false);

    bool ret = ::DeleteFile((wchar_t*)entry.nativeFilePath().utf16()) != 0;
    if (!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
    return ret;
}

/*
    If possible, we use the IFileOperation implementation, which allows us to determine
    the location of the object in the trash.
    If not (likely on mingw), we fall back to the old API, which won't allow us to know
    that.
*/
//static
bool QFileSystemEngine::moveFileToTrash(const QFileSystemEntry &source,
                                        QFileSystemEntry &newLocation, QSystemError &error)
{
    // we need the "display name" of the file, so can't use nativeAbsoluteFilePath
    const QString sourcePath = QDir::toNativeSeparators(absoluteName(source).filePath());

    QComHelper comHelper;

    IFileOperation *pfo = nullptr;
    IShellItem *deleteItem = nullptr;
    FileOperationProgressSink *sink = nullptr;
    HRESULT hres = E_FAIL;

    auto coUninitialize = qScopeGuard([&](){
        if (sink)
            sink->Release();
        if (deleteItem)
            deleteItem->Release();
        if (pfo)
            pfo->Release();
        if (!SUCCEEDED(hres))
            error = QSystemError(hres, QSystemError::NativeError);
    });

    hres = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pfo));
    if (!pfo)
        return false;
    pfo->SetOperationFlags(FOF_ALLOWUNDO | FOFX_RECYCLEONDELETE | FOF_NOCONFIRMATION
                        | FOF_SILENT | FOF_NOERRORUI);
    hres = SHCreateItemFromParsingName(reinterpret_cast<const wchar_t*>(sourcePath.utf16()),
                                    nullptr, IID_PPV_ARGS(&deleteItem));
    if (!deleteItem)
        return false;
    sink = new FileOperationProgressSink;
    hres = pfo->DeleteItem(deleteItem, static_cast<IFileOperationProgressSink*>(sink));
    if (!SUCCEEDED(hres))
        return false;
    hres = pfo->PerformOperations();
    if (!SUCCEEDED(hres))
        return false;

    if (!SUCCEEDED(sink->deleteResult)) {
        error = QSystemError(sink->deleteResult, QSystemError::NativeError);
        return false;
    }
    newLocation = QFileSystemEntry(sink->targetPath);
    return true;
}

//static
bool QFileSystemEngine::setPermissions(const QFileSystemEntry &entry,
                                       QFile::Permissions permissions, QSystemError &error,
                                       QFileSystemMetaData *data)
{
    Q_CHECK_FILE_NAME(entry, false);

    Q_UNUSED(data);
    int mode = 0;

    if (permissions & (QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther))
        mode |= _S_IREAD;
    if (permissions
        & (QFile::WriteOwner | QFile::WriteUser | QFile::WriteGroup | QFile::WriteOther)) {
        mode |= _S_IWRITE;
    }

    if (mode == 0) // not supported
        return false;

    bool ret =
            ::_wchmod(reinterpret_cast<const wchar_t *>(entry.nativeFilePath().utf16()), mode) == 0;
    if (!ret)
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
                     QTimeZone::UTC);
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
