// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsql_sqlite_vfs_p.h"

#include <QFile>

#include <limits.h> // defines PATH_MAX on unix
#include <sqlite3.h>
#include <stdio.h> // defines FILENAME_MAX everywhere

#ifndef PATH_MAX
#  define PATH_MAX FILENAME_MAX
#endif
#if SQLITE_VERSION_NUMBER < 3040000
typedef const char *sqlite3_filename;
#endif

QT_BEGIN_NAMESPACE

namespace {
struct Vfs : sqlite3_vfs {
    sqlite3_vfs *pVfs;
    sqlite3_io_methods ioMethods;
};

struct File : sqlite3_file {
    class QtFile : public QFile {
    public:
        QtFile(const QString &name, bool removeOnClose)
            : QFile(name)
            , removeOnClose(removeOnClose)
        {}

        ~QtFile() override
        {
            if (removeOnClose)
                remove();
        }
    private:
        bool removeOnClose;
    };
    QtFile *pFile;
};


int xClose(sqlite3_file *sfile)
{
    auto file = static_cast<File *>(sfile);
    delete file->pFile;
    file->pFile = nullptr;
    return SQLITE_OK;
}

int xRead(sqlite3_file *sfile, void *ptr, int iAmt, sqlite3_int64 iOfst)
{
    auto file = static_cast<File *>(sfile);
    if (!file->pFile->seek(iOfst))
        return SQLITE_IOERR_READ;

    auto sz = file->pFile->read(static_cast<char *>(ptr), iAmt);
    if (sz < iAmt) {
        memset(static_cast<char *>(ptr) + sz, 0, size_t(iAmt - sz));
        return SQLITE_IOERR_SHORT_READ;
    }
    return SQLITE_OK;
}

int xWrite(sqlite3_file *sfile, const void *data, int iAmt, sqlite3_int64 iOfst)
{
    auto file = static_cast<File *>(sfile);
    if (!file->pFile->seek(iOfst))
        return SQLITE_IOERR_SEEK;
    return file->pFile->write(reinterpret_cast<const char*>(data), iAmt) == iAmt ? SQLITE_OK : SQLITE_IOERR_WRITE;
}

int xTruncate(sqlite3_file *sfile, sqlite3_int64 size)
{
    auto file = static_cast<File *>(sfile);
    return file->pFile->resize(size) ? SQLITE_OK : SQLITE_IOERR_TRUNCATE;
}

int xSync(sqlite3_file *sfile, int /*flags*/)
{
    static_cast<File *>(sfile)->pFile->flush();
    return SQLITE_OK;
}

int xFileSize(sqlite3_file *sfile, sqlite3_int64 *pSize)
{
    auto file = static_cast<File *>(sfile);
    *pSize = file->pFile->size();
    return SQLITE_OK;
}

// No lock/unlock for QFile, QLockFile doesn't work for me

int xLock(sqlite3_file *, int) { return SQLITE_OK; }

int xUnlock(sqlite3_file *, int) { return SQLITE_OK; }

int xCheckReservedLock(sqlite3_file *, int *pResOut)
{
    *pResOut = 0;
    return SQLITE_OK;
}

int xFileControl(sqlite3_file *, int, void *) { return SQLITE_NOTFOUND; }

int xSectorSize(sqlite3_file *)
{
    return 4096;
}

int xDeviceCharacteristics(sqlite3_file *)
{
    return 0; // no SQLITE_IOCAP_XXX
}

int xOpen(sqlite3_vfs *svfs, sqlite3_filename zName, sqlite3_file *sfile,
          int flags, int *pOutFlags)
{
    auto vfs = static_cast<Vfs *>(svfs);
    auto file = static_cast<File *>(sfile);
    memset(file, 0, sizeof(File));
    QIODeviceBase::OpenMode mode = QIODeviceBase::NotOpen;
    if (!zName || (flags & SQLITE_OPEN_MEMORY))
        return SQLITE_PERM;
    if ((flags & SQLITE_OPEN_READONLY) &&
        !(flags & SQLITE_OPEN_READWRITE) &&
        !(flags & SQLITE_OPEN_CREATE) &&
        !(flags & SQLITE_OPEN_DELETEONCLOSE)) {
        mode |= QIODeviceBase::OpenModeFlag::ReadOnly;
    } else {
        /*
            ** ^The [SQLITE_OPEN_EXCLUSIVE] flag is always used in conjunction
            ** with the [SQLITE_OPEN_CREATE] flag, which are both directly
            ** analogous to the O_EXCL and O_CREAT flags of the POSIX open()
            ** API.  The SQLITE_OPEN_EXCLUSIVE flag, when paired with the
            ** SQLITE_OPEN_CREATE, is used to indicate that file should always
            ** be created, and that it is an error if it already exists.
            ** It is <i>not</i> used to indicate the file should be opened
            ** for exclusive access.
         */
        if ((flags & SQLITE_OPEN_CREATE) && (flags & SQLITE_OPEN_EXCLUSIVE))
            mode |= QIODeviceBase::OpenModeFlag::NewOnly;

        if (flags & SQLITE_OPEN_READWRITE)
            mode |= QIODeviceBase::OpenModeFlag::ReadWrite;
    }

    file->pMethods = &vfs->ioMethods;
    file->pFile = new File::QtFile(QString::fromUtf8(zName), bool(flags & SQLITE_OPEN_DELETEONCLOSE));
    if (!file->pFile->open(mode))
        return SQLITE_CANTOPEN;
    if (pOutFlags)
        *pOutFlags = flags;

    return SQLITE_OK;
}

int xDelete(sqlite3_vfs *, const char *zName, int)
{
    return QFile::remove(QString::fromUtf8(zName)) ? SQLITE_OK : SQLITE_ERROR;
}

int xAccess(sqlite3_vfs */*svfs*/, const char *zName, int flags, int *pResOut)
{
    *pResOut = 0;
    switch (flags) {
    case SQLITE_ACCESS_EXISTS:
    case SQLITE_ACCESS_READ:
        *pResOut = QFile::exists(QString::fromUtf8(zName));
        break;
    default:
        break;
    }
    return SQLITE_OK;
}

int xFullPathname(sqlite3_vfs *, const char *zName, int nOut, char *zOut)
{
    if (!zName)
        return SQLITE_ERROR;

    int i = 0;
    for (;zName[i] && i < nOut; ++i)
        zOut[i] = zName[i];

    if (i >= nOut)
        return SQLITE_ERROR;

    zOut[i] = '\0';
    return SQLITE_OK;
}

int xRandomness(sqlite3_vfs *svfs, int nByte, char *zOut)
{
    auto vfs = static_cast<Vfs *>(svfs)->pVfs;
    return vfs->xRandomness(vfs, nByte, zOut);
}

int xSleep(sqlite3_vfs *svfs, int microseconds)
{
    auto vfs = static_cast<Vfs *>(svfs)->pVfs;
    return vfs->xSleep(vfs, microseconds);
}

int xCurrentTime(sqlite3_vfs *svfs, double *zOut)
{
    auto vfs = static_cast<Vfs *>(svfs)->pVfs;
    return vfs->xCurrentTime(vfs, zOut);
}

int xGetLastError(sqlite3_vfs *, int, char *)
{
   return 0;
}

int xCurrentTimeInt64(sqlite3_vfs *svfs, sqlite3_int64 *zOut)
{
   auto vfs = static_cast<Vfs *>(svfs)->pVfs;
   return vfs->xCurrentTimeInt64(vfs, zOut);
}
} // namespace {

void register_qt_vfs()
{
    static Vfs vfs;
    memset(&vfs, 0, sizeof(Vfs));
    vfs.iVersion = 1;
    vfs.szOsFile = sizeof(File);
    vfs.mxPathname = PATH_MAX;
    vfs.zName = "QtVFS";
    vfs.xOpen = &xOpen;
    vfs.xDelete = &xDelete;
    vfs.xAccess = &xAccess;
    vfs.xFullPathname = &xFullPathname;
    vfs.xRandomness = &xRandomness;
    vfs.xSleep = &xSleep;
    vfs.xCurrentTime = &xCurrentTime;
    vfs.xGetLastError = &xGetLastError;
    vfs.xCurrentTimeInt64 = &xCurrentTimeInt64;
    vfs.pVfs = sqlite3_vfs_find(nullptr);
    vfs.ioMethods.iVersion = 1;
    vfs.ioMethods.xClose = &xClose;
    vfs.ioMethods.xRead = &xRead;
    vfs.ioMethods.xWrite = &xWrite;
    vfs.ioMethods.xTruncate = &xTruncate;
    vfs.ioMethods.xSync = &xSync;
    vfs.ioMethods.xFileSize = &xFileSize;
    vfs.ioMethods.xLock = &xLock;
    vfs.ioMethods.xUnlock = &xUnlock;
    vfs.ioMethods.xCheckReservedLock = &xCheckReservedLock;
    vfs.ioMethods.xFileControl = &xFileControl;
    vfs.ioMethods.xSectorSize = &xSectorSize;
    vfs.ioMethods.xDeviceCharacteristics = &xDeviceCharacteristics;

    sqlite3_vfs_register(&vfs, 0);
}

QT_END_NAMESPACE
