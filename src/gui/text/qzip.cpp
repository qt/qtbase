// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qzipwriter_p.h"

#include <qdatetime.h>
#include <qendian.h>
#include <qdebug.h>
#include <qdir.h>

#include <memory>

#include <zlib.h>

// Zip standard version for archives handled by this API
// (actually, the only basic support of this version is implemented but it is enough for now)
#define ZIP_VERSION 20

#if 0
#define ZDEBUG qDebug
#else
#define ZDEBUG if (0) qDebug
#endif

QT_BEGIN_NAMESPACE

static inline void writeUInt(uchar *data, uint i)
{
    data[0] = i & 0xff;
    data[1] = (i>>8) & 0xff;
    data[2] = (i>>16) & 0xff;
    data[3] = (i>>24) & 0xff;
}

static inline void writeUShort(uchar *data, ushort i)
{
    data[0] = i & 0xff;
    data[1] = (i>>8) & 0xff;
}

static inline void copyUInt(uchar *dest, const uchar *src)
{
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
}

static inline void copyUShort(uchar *dest, const uchar *src)
{
    dest[0] = src[0];
    dest[1] = src[1];
}

static void writeMSDosDate(uchar *dest, const QDateTime& dt)
{
    if (dt.isValid()) {
        quint16 time =
            (dt.time().hour() << 11)    // 5 bit hour
            | (dt.time().minute() << 5)   // 6 bit minute
            | (dt.time().second() >> 1);  // 5 bit double seconds

        dest[0] = time & 0xff;
        dest[1] = time >> 8;

        quint16 date =
            ((dt.date().year() - 1980) << 9) // 7 bit year 1980-based
            | (dt.date().month() << 5)           // 4 bit month
            | (dt.date().day());                 // 5 bit day

        dest[2] = char(date);
        dest[3] = char(date >> 8);
    } else {
        dest[0] = 0;
        dest[1] = 0;
        dest[2] = 0;
        dest[3] = 0;
    }
}

static int deflate (Bytef *dest, ulong *destLen, const Bytef *source, ulong sourceLen)
{
    z_stream stream;
    int err;

    stream.next_in = const_cast<Bytef*>(source);
    stream.avail_in = (uInt)sourceLen;
    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

    stream.zalloc = (alloc_func)nullptr;
    stream.zfree = (free_func)nullptr;
    stream.opaque = (voidpf)nullptr;

    err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
    if (err != Z_OK) return err;

    err = deflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        deflateEnd(&stream);
        return err == Z_OK ? Z_BUF_ERROR : err;
    }
    *destLen = stream.total_out;

    err = deflateEnd(&stream);
    return err;
}


namespace WindowsFileAttributes {
enum {
    Dir        = 0x10, // FILE_ATTRIBUTE_DIRECTORY
    File       = 0x80, // FILE_ATTRIBUTE_NORMAL
    TypeMask   = 0x90,

    ReadOnly   = 0x01, // FILE_ATTRIBUTE_READONLY
    PermMask   = 0x01
};
}

namespace UnixFileAttributes {
enum {
    Dir        = 0040000, // __S_IFDIR
    File       = 0100000, // __S_IFREG
    SymLink    = 0120000, // __S_IFLNK
    TypeMask   = 0170000, // __S_IFMT

    ReadUser   = 0400, // __S_IRUSR
    WriteUser  = 0200, // __S_IWUSR
    ExeUser    = 0100, // __S_IXUSR
    ReadGroup  = 0040, // __S_IRGRP
    WriteGroup = 0020, // __S_IWGRP
    ExeGroup   = 0010, // __S_IXGRP
    ReadOther  = 0004, // __S_IROTH
    WriteOther = 0002, // __S_IWOTH
    ExeOther   = 0001, // __S_IXOTH
    PermMask   = 0777
};
}

static quint32 permissionsToMode(QFile::Permissions perms)
{
    quint32 mode = 0;
    if (perms & (QFile::ReadOwner | QFile::ReadUser))
        mode |= UnixFileAttributes::ReadUser;
    if (perms & (QFile::WriteOwner | QFile::WriteUser))
        mode |= UnixFileAttributes::WriteUser;
    if (perms & (QFile::ExeOwner | QFile::ExeUser))
        mode |= UnixFileAttributes::WriteUser;
    if (perms & QFile::ReadGroup)
        mode |= UnixFileAttributes::ReadGroup;
    if (perms & QFile::WriteGroup)
        mode |= UnixFileAttributes::WriteGroup;
    if (perms & QFile::ExeGroup)
        mode |= UnixFileAttributes::ExeGroup;
    if (perms & QFile::ReadOther)
        mode |= UnixFileAttributes::ReadOther;
    if (perms & QFile::WriteOther)
        mode |= UnixFileAttributes::WriteOther;
    if (perms & QFile::ExeOther)
        mode |= UnixFileAttributes::ExeOther;
    return mode;
}

// for details, see http://www.pkware.com/documents/casestudies/APPNOTE.TXT

enum HostOS {
    HostFAT      = 0,
    HostAMIGA    = 1,
    HostVMS      = 2,  // VAX/VMS
    HostUnix     = 3,
    HostVM_CMS   = 4,
    HostAtari    = 5,  // what if it's a minix filesystem? [cjh]
    HostHPFS     = 6,  // filesystem used by OS/2 (and NT 3.x)
    HostMac      = 7,
    HostZ_System = 8,
    HostCPM      = 9,
    HostTOPS20   = 10, // pkzip 2.50 NTFS
    HostNTFS     = 11, // filesystem used by Windows NT
    HostQDOS     = 12, // SMS/QDOS
    HostAcorn    = 13, // Archimedes Acorn RISC OS
    HostVFAT     = 14, // filesystem used by Windows 95, NT
    HostMVS      = 15,
    HostBeOS     = 16, // hybrid POSIX/database filesystem
    HostTandem   = 17,
    HostOS400    = 18,
    HostOSX      = 19
};
Q_DECLARE_TYPEINFO(HostOS, Q_PRIMITIVE_TYPE);

enum GeneralPurposeFlag {
    Encrypted = 0x01,
    AlgTune1 = 0x02,
    AlgTune2 = 0x04,
    HasDataDescriptor = 0x08,
    PatchedData = 0x20,
    StrongEncrypted = 0x40,
    Utf8Names = 0x0800,
    CentralDirectoryEncrypted = 0x2000
};
Q_DECLARE_TYPEINFO(GeneralPurposeFlag, Q_PRIMITIVE_TYPE);

enum CompressionMethod {
    CompressionMethodStored = 0,
    CompressionMethodShrunk = 1,
    CompressionMethodReduced1 = 2,
    CompressionMethodReduced2 = 3,
    CompressionMethodReduced3 = 4,
    CompressionMethodReduced4 = 5,
    CompressionMethodImploded = 6,
    CompressionMethodReservedTokenizing = 7, // reserved for tokenizing
    CompressionMethodDeflated = 8,
    CompressionMethodDeflated64 = 9,
    CompressionMethodPKImploding = 10,

    CompressionMethodBZip2 = 12,

    CompressionMethodLZMA = 14,

    CompressionMethodTerse = 18,
    CompressionMethodLz77 = 19,

    CompressionMethodJpeg = 96,
    CompressionMethodWavPack = 97,
    CompressionMethodPPMd = 98,
    CompressionMethodWzAES = 99
};
Q_DECLARE_TYPEINFO(CompressionMethod, Q_PRIMITIVE_TYPE);

struct LocalFileHeader
{
    uchar signature[4]; //  0x04034b50
    uchar version_needed[2];
    uchar general_purpose_bits[2];
    uchar compression_method[2];
    uchar last_mod_file[4];
    uchar crc_32[4];
    uchar compressed_size[4];
    uchar uncompressed_size[4];
    uchar file_name_length[2];
    uchar extra_field_length[2];
};
Q_DECLARE_TYPEINFO(LocalFileHeader, Q_PRIMITIVE_TYPE);

struct DataDescriptor
{
    uchar crc_32[4];
    uchar compressed_size[4];
    uchar uncompressed_size[4];
};
Q_DECLARE_TYPEINFO(DataDescriptor, Q_PRIMITIVE_TYPE);

struct CentralFileHeader
{
    uchar signature[4]; // 0x02014b50
    uchar version_made[2];
    uchar version_needed[2];
    uchar general_purpose_bits[2];
    uchar compression_method[2];
    uchar last_mod_file[4];
    uchar crc_32[4];
    uchar compressed_size[4];
    uchar uncompressed_size[4];
    uchar file_name_length[2];
    uchar extra_field_length[2];
    uchar file_comment_length[2];
    uchar disk_start[2];
    uchar internal_file_attributes[2];
    uchar external_file_attributes[4];
    uchar offset_local_header[4];
};
Q_DECLARE_TYPEINFO(CentralFileHeader, Q_PRIMITIVE_TYPE);

struct EndOfDirectory
{
    uchar signature[4]; // 0x06054b50
    uchar this_disk[2];
    uchar start_of_directory_disk[2];
    uchar num_dir_entries_this_disk[2];
    uchar num_dir_entries[2];
    uchar directory_size[4];
    uchar dir_start_offset[4];
    uchar comment_length[2];
};
Q_DECLARE_TYPEINFO(EndOfDirectory, Q_PRIMITIVE_TYPE);

struct FileHeader
{
    CentralFileHeader h;
    QByteArray file_name;
    QByteArray extra_field;
    QByteArray file_comment;
};
Q_DECLARE_TYPEINFO(FileHeader, Q_RELOCATABLE_TYPE);

class QZipPrivate
{
public:
    QZipPrivate(QIODevice *device, bool ownDev)
        : device(device), ownDevice(ownDev), dirtyFileTree(true), start_of_directory(0)
    {
    }

    ~QZipPrivate()
    {
        if (ownDevice)
            delete device;
    }

    QIODevice *device;
    bool ownDevice;
    bool dirtyFileTree;
    QList<FileHeader> fileHeaders;
    QByteArray comment;
    uint start_of_directory;
};

class QZipWriterPrivate : public QZipPrivate
{
public:
    QZipWriterPrivate(QIODevice *device, bool ownDev)
        : QZipPrivate(device, ownDev),
        status(QZipWriter::NoError),
        permissions(QFile::ReadOwner | QFile::WriteOwner),
        compressionPolicy(QZipWriter::AlwaysCompress)
    {
    }

    QZipWriter::Status status;
    QFile::Permissions permissions;
    QZipWriter::CompressionPolicy compressionPolicy;

    enum EntryType { Directory, File, Symlink };

    void addEntry(EntryType type, const QString &fileName, const QByteArray &contents);
};

static LocalFileHeader toLocalHeader(const CentralFileHeader &ch)
{
    LocalFileHeader h;
    writeUInt(h.signature, 0x04034b50);
    copyUShort(h.version_needed, ch.version_needed);
    copyUShort(h.general_purpose_bits, ch.general_purpose_bits);
    copyUShort(h.compression_method, ch.compression_method);
    copyUInt(h.last_mod_file, ch.last_mod_file);
    copyUInt(h.crc_32, ch.crc_32);
    copyUInt(h.compressed_size, ch.compressed_size);
    copyUInt(h.uncompressed_size, ch.uncompressed_size);
    copyUShort(h.file_name_length, ch.file_name_length);
    copyUShort(h.extra_field_length, ch.extra_field_length);
    return h;
}

void QZipWriterPrivate::addEntry(EntryType type, const QString &fileName, const QByteArray &contents/*, QFile::Permissions permissions, QZip::Method m*/)
{
#ifndef NDEBUG
    static const char *const entryTypes[] = {
        "directory",
        "file     ",
        "symlink  " };
    ZDEBUG() << "adding" << entryTypes[type] <<":" << fileName.toUtf8().data() << (type == 2 ? QByteArray(" -> " + contents).constData() : "");
#endif

    if (! (device->isOpen() || device->open(QIODevice::WriteOnly))) {
        status = QZipWriter::FileOpenError;
        return;
    }
    device->seek(start_of_directory);

    // don't compress small files
    QZipWriter::CompressionPolicy compression = compressionPolicy;
    if (compressionPolicy == QZipWriter::AutoCompress) {
        if (contents.size() < 64)
            compression = QZipWriter::NeverCompress;
        else
            compression = QZipWriter::AlwaysCompress;
    }

    FileHeader header;
    memset(&header.h, 0, sizeof(CentralFileHeader));
    writeUInt(header.h.signature, 0x02014b50);

    writeUShort(header.h.version_needed, ZIP_VERSION);
    writeUInt(header.h.uncompressed_size, contents.size());
    writeMSDosDate(header.h.last_mod_file, QDateTime::currentDateTime());
    QByteArray data = contents;
    if (compression == QZipWriter::AlwaysCompress) {
        writeUShort(header.h.compression_method, CompressionMethodDeflated);

       ulong len = contents.size();
        // shamelessly copied form zlib
        len += (len >> 12) + (len >> 14) + 11;
        int res;
        do {
            data.resize(len);
            res = deflate((uchar*)data.data(), &len, (const uchar*)contents.constData(), contents.size());

            switch (res) {
            case Z_OK:
                data.resize(len);
                break;
            case Z_MEM_ERROR:
                qWarning("QZip: Z_MEM_ERROR: Not enough memory to compress file, skipping");
                data.resize(0);
                break;
            case Z_BUF_ERROR:
                len *= 2;
                break;
            }
        } while (res == Z_BUF_ERROR);
    }
// TODO add a check if data.length() > contents.length().  Then try to store the original and revert the compression method to be uncompressed
    writeUInt(header.h.compressed_size, data.size());
    uint crc_32 = ::crc32(0, nullptr, 0);
    crc_32 = ::crc32(crc_32, (const uchar *)contents.constData(), contents.size());
    writeUInt(header.h.crc_32, crc_32);

    // if bit 11 is set, the filename and comment fields must be encoded using UTF-8
    ushort general_purpose_bits = Utf8Names; // always use utf-8
    writeUShort(header.h.general_purpose_bits, general_purpose_bits);

    const bool inUtf8 = (general_purpose_bits & Utf8Names) != 0;
    header.file_name = inUtf8 ? fileName.toUtf8() : fileName.toLocal8Bit();
    if (header.file_name.size() > 0xffff) {
        qWarning("QZip: Filename is too long, chopping it to 65535 bytes");
        header.file_name = header.file_name.left(0xffff); // ### don't break the utf-8 sequence, if any
    }
    if (header.file_comment.size() + header.file_name.size() > 0xffff) {
        qWarning("QZip: File comment is too long, chopping it to 65535 bytes");
        header.file_comment.truncate(0xffff - header.file_name.size()); // ### don't break the utf-8 sequence, if any
    }
    writeUShort(header.h.file_name_length, header.file_name.size());
    //h.extra_field_length[2];

    writeUShort(header.h.version_made, HostUnix << 8);
    //uchar internal_file_attributes[2];
    //uchar external_file_attributes[4];
    quint32 mode = permissionsToMode(permissions);
    switch (type) {
    case Symlink:
        mode |= UnixFileAttributes::SymLink;
        break;
    case Directory:
        mode |= UnixFileAttributes::Dir;
        break;
    case File:
        mode |= UnixFileAttributes::File;
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
    writeUInt(header.h.external_file_attributes, mode << 16);
    writeUInt(header.h.offset_local_header, start_of_directory);


    fileHeaders.append(header);

    LocalFileHeader h = toLocalHeader(header.h);
    device->write((const char *)&h, sizeof(LocalFileHeader));
    device->write(header.file_name);
    device->write(data);
    start_of_directory = device->pos();
    dirtyFileTree = true;
}

////////////////////////////// Writer

/*!
    \class QZipWriter
    \internal
    \since 4.5

    \brief the QZipWriter class provides a way to create a new zip archive.

    QZipWriter can be used to create a zip archive containing any number of files
    and directories. The files in the archive will be compressed in a way that is
    compatible with common zip reader applications.
*/


/*!
    Create a new zip archive that operates on the \a archive filename.  The file will
    be opened with the \a mode.
    \sa isValid()
*/
QZipWriter::QZipWriter(const QString &fileName, QIODevice::OpenMode mode)
{
    auto f = std::make_unique<QFile>(fileName);
    QZipWriter::Status status;
    if (f->open(mode) && f->error() == QFile::NoError)
        status = QZipWriter::NoError;
    else {
        if (f->error() == QFile::WriteError)
            status = QZipWriter::FileWriteError;
        else if (f->error() == QFile::OpenError)
            status = QZipWriter::FileOpenError;
        else if (f->error() == QFile::PermissionsError)
            status = QZipWriter::FilePermissionsError;
        else
            status = QZipWriter::FileError;
    }

    d = new QZipWriterPrivate(f.get(), /*ownDevice=*/true);
    Q_UNUSED(f.release());
    d->status = status;
}

/*!
    Create a new zip archive that operates on the archive found in \a device.
    You have to open the device previous to calling the constructor and
    only a device that is readable will be scanned for zip filecontent.
 */
QZipWriter::QZipWriter(QIODevice *device)
    : d(new QZipWriterPrivate(device, /*ownDevice=*/false))
{
    Q_ASSERT(device);
}

QZipWriter::~QZipWriter()
{
    close();
    delete d;
}

/*!
    Returns device used for writing zip archive.
*/
QIODevice* QZipWriter::device() const
{
    return d->device;
}

/*!
    Returns \c true if the user can write to the archive; otherwise returns \c false.
*/
bool QZipWriter::isWritable() const
{
    return d->device->isWritable();
}

/*!
    Returns \c true if the file exists; otherwise returns \c false.
*/
bool QZipWriter::exists() const
{
    QFile *f = qobject_cast<QFile*> (d->device);
    if (f == nullptr)
        return true;
    return f->exists();
}

/*!
    \enum QZipWriter::Status

    The following status values are possible:

    \value NoError  No error occurred.
    \value FileWriteError    An error occurred when writing to the device.
    \value FileOpenError    The file could not be opened.
    \value FilePermissionsError The file could not be accessed.
    \value FileError        Another file error occurred.
*/

/*!
    Returns a status code indicating the first error that was met by QZipWriter,
    or QZipWriter::NoError if no error occurred.
*/
QZipWriter::Status QZipWriter::status() const
{
    return d->status;
}

/*!
    \enum QZipWriter::CompressionPolicy

    \value AlwaysCompress   A file that is added is compressed.
    \value NeverCompress    A file that is added will be stored without changes.
    \value AutoCompress     A file that is added will be compressed only if that will give a smaller file.
*/

/*!
     Sets the policy for compressing newly added files to the new \a policy.

    \note the default policy is AlwaysCompress

    \sa compressionPolicy()
    \sa addFile()
*/
void QZipWriter::setCompressionPolicy(CompressionPolicy policy)
{
    d->compressionPolicy = policy;
}

/*!
     Returns the currently set compression policy.
    \sa setCompressionPolicy()
    \sa addFile()
*/
QZipWriter::CompressionPolicy QZipWriter::compressionPolicy() const
{
    return d->compressionPolicy;
}

/*!
    Sets the permissions that will be used for newly added files.

    \note the default permissions are QFile::ReadOwner | QFile::WriteOwner.

    \sa creationPermissions()
    \sa addFile()
*/
void QZipWriter::setCreationPermissions(QFile::Permissions permissions)
{
    d->permissions = permissions;
}

/*!
     Returns the currently set creation permissions.

    \sa setCreationPermissions()
    \sa addFile()
*/
QFile::Permissions QZipWriter::creationPermissions() const
{
    return d->permissions;
}

/*!
    Add a file to the archive with \a data as the file contents.
    The file will be stored in the archive using the \a fileName which
    includes the full path in the archive.

    The new file will get the file permissions based on the current
    creationPermissions and it will be compressed using the zip compression
    based on the current compression policy.

    \sa setCreationPermissions()
    \sa setCompressionPolicy()
*/
void QZipWriter::addFile(const QString &fileName, const QByteArray &data)
{
    d->addEntry(QZipWriterPrivate::File, QDir::fromNativeSeparators(fileName), data);
}

/*!
    Add a file to the archive with \a device as the source of the contents.
    The contents returned from QIODevice::readAll() will be used as the
    filedata.
    The file will be stored in the archive using the \a fileName which
    includes the full path in the archive.
*/
void QZipWriter::addFile(const QString &fileName, QIODevice *device)
{
    Q_ASSERT(device);
    QIODevice::OpenMode mode = device->openMode();
    bool opened = false;
    if ((mode & QIODevice::ReadOnly) == 0) {
        opened = true;
        if (! device->open(QIODevice::ReadOnly)) {
            d->status = FileOpenError;
            return;
        }
    }
    d->addEntry(QZipWriterPrivate::File, QDir::fromNativeSeparators(fileName), device->readAll());
    if (opened)
        device->close();
}

/*!
    Create a new directory in the archive with the specified \a dirName and
    the \a permissions;
*/
void QZipWriter::addDirectory(const QString &dirName)
{
    QString name(QDir::fromNativeSeparators(dirName));
    // separator is mandatory
    if (!name.endsWith(u'/'))
        name.append(u'/');
    d->addEntry(QZipWriterPrivate::Directory, name, QByteArray());
}

/*!
    Create a new symbolic link in the archive with the specified \a dirName
    and the \a permissions;
    A symbolic link contains the destination (relative) path and name.
*/
void QZipWriter::addSymLink(const QString &fileName, const QString &destination)
{
    d->addEntry(QZipWriterPrivate::Symlink, QDir::fromNativeSeparators(fileName), QFile::encodeName(destination));
}

/*!
   Closes the zip file.
*/
void QZipWriter::close()
{
    if (!(d->device->openMode() & QIODevice::WriteOnly)) {
        d->device->close();
        return;
    }

    //qDebug("QZip::close writing directory, %d entries", d->fileHeaders.size());
    d->device->seek(d->start_of_directory);
    // write new directory
    for (int i = 0; i < d->fileHeaders.size(); ++i) {
        const FileHeader &header = d->fileHeaders.at(i);
        d->device->write((const char *)&header.h, sizeof(CentralFileHeader));
        d->device->write(header.file_name);
        d->device->write(header.extra_field);
        d->device->write(header.file_comment);
    }
    int dir_size = d->device->pos() - d->start_of_directory;
    // write end of directory
    EndOfDirectory eod;
    memset(&eod, 0, sizeof(EndOfDirectory));
    writeUInt(eod.signature, 0x06054b50);
    //uchar this_disk[2];
    //uchar start_of_directory_disk[2];
    writeUShort(eod.num_dir_entries_this_disk, d->fileHeaders.size());
    writeUShort(eod.num_dir_entries, d->fileHeaders.size());
    writeUInt(eod.directory_size, dir_size);
    writeUInt(eod.dir_start_offset, d->start_of_directory);
    writeUShort(eod.comment_length, d->comment.size());

    d->device->write((const char *)&eod, sizeof(EndOfDirectory));
    d->device->write(d->comment);
    d->device->close();
}

QT_END_NAMESPACE
