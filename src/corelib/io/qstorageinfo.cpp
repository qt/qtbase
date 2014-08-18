/****************************************************************************
**
** Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qstorageinfo.h"
#include "qstorageinfo_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QStorageInfo
    \inmodule QtCore
    \since 5.4
    \brief Provides information about currently mounted storages and drives.

    \ingroup io
    \ingroup shared

    Allows retrieving information about the volume's space, its mount point,
    label, filesystem name.

    You can create an instance of QStorageInfo by passing the path to the
    volume's mount point as the constructor parameter, or you can set it using
    setPath() method. The static mountedVolumes() method can be used to get the
    list of all mounted filesystems.

    QStorageInfo always caches the retrieved information but you can call
    refresh() to invalidate the cache.

    The following example retrieves the most common information about the root
    volume of the system and prints information about it.

    \snippet code/src_corelib_io_qstorageinfo.cpp 2
*/

/*!
    Constructs an empty QStorageInfo object.

    This object is not ready for use, invalid and all its parameters are empty.

    \sa setPath(), isReady(), isValid()
*/
QStorageInfo::QStorageInfo()
    : d(new QStorageInfoPrivate)
{
}

/*!
    Constructs a new QStorageInfo that gives information about the volume
    mounted at \a path.

    If you pass a directory or file, the QStorageInfo object will refer to the
    volume where this directory or file is located.
    You can check if the created object is correct using the isValid() method.

    The following example shows how to get volume on which application is
    located. It is recommended to always check that volume is ready and valid.

    \snippet code/src_corelib_io_qstorageinfo.cpp 0

    \sa setPath()
*/
QStorageInfo::QStorageInfo(const QString &path)
    : d(new QStorageInfoPrivate)
{
    setPath(path);
}

/*!
    Constructs a new QStorageInfo that gives information about the volume
    that contains the \a dir folder.
*/
QStorageInfo::QStorageInfo(const QDir &dir)
    : d(new QStorageInfoPrivate)
{
    setPath(dir.absolutePath());
}

/*!
    Constructs a new QStorageInfo that is a copy of the \a other QStorageInfo.
*/
QStorageInfo::QStorageInfo(const QStorageInfo &other)
    : d(other.d)
{
}

/*!
    Destroys the QStorageInfo and frees its resources.
*/
QStorageInfo::~QStorageInfo()
{
}

/*!
    Makes a copy of \a other QStorageInfo and assigns it to this QStorageInfo.
*/
QStorageInfo &QStorageInfo::operator=(const QStorageInfo &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn QStorageInfo &QStorageInfo::operator=(QStorageInfo &&other)

    Move-assigns \a other to this QStorageInfo instance.
*/

/*!
    \fn void QStorageInfo::swap(QStorageInfo &other)

    Swaps this volume info with the \a other. This function is very fast and
    never fails.
*/

/*!
    Sets QStorageInfo to the filesystem mounted where \a path is located.

    Path can either be a root path of the filesystem, or a directory or a file
    within that filesystem.

    \sa rootPath()
*/
void QStorageInfo::setPath(const QString &path)
{
    if (d->rootPath == path)
        return;
    d.detach();
    d->rootPath = path;
    d->doStat();
}

/*!
    Returns the mount point of the filesystem this QStorageInfo object
    represents.

    On Windows, returns the volume letter in case the volume is not mounted to
    a directory.

    Note that the value returned by rootPath() is the real mount point of a
    volume and may not be equal to the value passed to constructor or setPath()
    method. For example, if you have only the root volume in the system and
    pass '/directory' to setPath(), then this method will return '/'.

    \sa setPath(), device()
*/
QString QStorageInfo::rootPath() const
{
    return d->rootPath;
}

/*!
    Returns the size (in bytes) available for the current user. If the user is
    the root user or a system administrator returns all available size.

    This size can be less than or equal to the free size, returned by
    bytesFree() function.

    \sa bytesTotal(), bytesFree()
*/
qint64 QStorageInfo::bytesAvailable() const
{
    return d->bytesAvailable;
}

/*!
    Returns the number of free bytes on a volume. Note, that if there are some
    kind of quotas on the filesystem, this value can be bigger than
    bytesAvailable().

    \sa bytesTotal(), bytesAvailable()
*/
qint64 QStorageInfo::bytesFree() const
{
    return d->bytesFree;
}

/*!
    Returns total volume size in bytes.

    \sa bytesFree(), bytesAvailable()
*/
qint64 QStorageInfo::bytesTotal() const
{
    return d->bytesTotal;
}

/*!
    Returns the type name of the filesystem.

    This is a platform-dependent function, and filesystem names can vary
    between different operating systems. For example, on Windows filesystems
    can be named as 'NTFS' and on Linux as 'ntfs-3g' or 'fuseblk'.

    \sa name()
*/
QByteArray QStorageInfo::fileSystemType() const
{
    return d->fileSystemType;
}

/*!
    Returns the device for this volume.

    For example, on Unix filesystems (including OS X), this returns the
    devpath like '/dev/sda0' for local storages. On Windows, returns the UNC
    path starting with \\\\?\\ for local storages (i.e. volume GUID).

    \sa rootPath()
*/
QByteArray QStorageInfo::device() const
{
    return d->device;
}

/*!
    Returns the human-readable name of a filesystem, usually called 'label'.

    Not all filesystems support this feature, in this case value returned by
    this method could be empty. An empty string is returned if the file system
    does not support labels or no label is set.

    On Linux, retrieving the volume's label requires udev to be present in the
    system.

    \sa fileSystemType()
*/
QString QStorageInfo::name() const
{
    return d->name;
}

/*!
    Returns the volume's name, if available, or the root path if not.
*/
QString QStorageInfo::displayName() const
{
    if (!d->name.isEmpty())
        return d->name;
    return d->rootPath;
}

/*!
    \fn bool QStorageInfo::isRoot() const

    Returns true if this QStorageInfo represents the system root volume; false
    otherwise.

    On Unix filesystems, the root volume is a volume mounted at "/", on Windows
    the root volume is the volume where OS is installed.

    \sa root()
*/

/*!
    Returns true if the current filesystem is protected from writing; false
    otherwise.
*/
bool QStorageInfo::isReadOnly() const
{
    return d->readOnly;
}

/*!
    Returns true if current filesystem is ready to work; false otherwise. For
    example, false is returned if CD volume is not inserted.

    Note that fileSystemType(), name(), bytesTotal(), bytesFree(), and
    bytesAvailable() will return invalid data until the volume is ready.

    \sa isValid()
*/
bool QStorageInfo::isReady() const
{
    return d->ready;
}

/*!
    Returns true if the QStorageInfo specified by rootPath exists and is mounted
    correctly.

    \sa isReady()
*/
bool QStorageInfo::isValid() const
{
    return d->valid;
}

/*!
    Resets QStorageInfo's internal cache.

    QStorageInfo caches information about storages to speed up performance -
    QStorageInfo retrieves information during object construction and/or call
    to setPath() method. You have to manually reset the cache by calling this
    function to update storage information.
*/
void QStorageInfo::refresh()
{
    d.detach();
    d->doStat();
}

/*!
    Returns list of QStorageInfos that corresponds to the list of currently
    mounted filesystems.

    On Windows, this returns drives presented in 'My Computer' folder. On Unix
    operating systems, returns list of all mounted filesystems (except for
    pseudo filesystems).

    By default, returns all currently mounted filesystems.

    The example shows how to retrieve all storages present in the system and
    skip read-only storages.

    \snippet code/src_corelib_io_qstorageinfo.cpp 1

    \sa root()
*/
QList<QStorageInfo> QStorageInfo::mountedVolumes()
{
    return QStorageInfoPrivate::mountedVolumes();
}

Q_GLOBAL_STATIC_WITH_ARGS(QStorageInfo, getRoot, (QStorageInfoPrivate::root()))

/*!
    Returns a QStorageInfo object that represents the system root volume.

    On Unix systems this call returns '/' volume, on Windows the volume where
    operating system is installed is returned.

    \sa isRoot()
*/
QStorageInfo QStorageInfo::root()
{
    return *getRoot();
}

/*!
    \fn inline bool operator==(const QStorageInfo &first, const QStorageInfo &second)

    \relates QStorageInfo

    Returns true if \a first QStorageInfo object refers to the same drive or volume
    as the \a second; otherwise returns false.

    Note that the result of comparing two invalid QStorageInfo objects is always
    positive.
*/

/*!
    \fn inline bool operator!=(const QStorageInfo &first, const QStorageInfo &second)

    \relates QStorageInfo

    Returns true if \a first QStorageInfo object refers to a different drive or
    volume than the one specified by \a second; otherwise returns false.
*/

QT_END_NAMESPACE
