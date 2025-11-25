// Copyright (C) 2025 Qt Group
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qwasmlocalfileengine_p.h"
#include <QtCore/QDebug>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Custom URL scheme for files handled by this file engine. A complete file URL can
// look like
//
//   "weblocalfile:/n/file.ext"
//
// where n is a counter to ensure uniqueness, needed since web platform gives us the file name only
// and does not provide the file path.
//
// The scheme may be visible to end users if the application displays it, so
// we avoid using "wasm" and instead say "web", which should be more recognizable.
static constexpr QLatin1StringView wasmlocalfileScheme = "weblocalfile"_L1;

// Instantiate the engine, the system will pick it up automatically
// Never destroy it to avoid problems with static destruction.
// The OS will reclaim the memory anyway,
// TODO: cleanup on QApplication destruction and re-init on QApplication re-creation.
static  QWasmFileEngineHandler *singleton = new QWasmFileEngineHandler();

QWasmFileEngineHandler::QWasmFileEngineHandler()
{
}

QWasmFileEngineHandler::~QWasmFileEngineHandler()
{
}

std::unique_ptr<QAbstractFileEngine> QWasmFileEngineHandler::create(const QString &fileName) const
{
    if (!QWasmFileEngineHandler::isWasmFileName(fileName))
        return {};

    // Check if it's a File or FileSystemFileHandle
    if (singleton->m_files.contains(fileName)) {
        qstdweb::File file = singleton->m_files.value(fileName);
        return std::make_unique<QWasmFileEngine>(fileName, file);
    } else if (singleton->m_fileSystemFiles.contains(fileName)) {
        qstdweb::FileSystemFileHandle file = singleton->m_fileSystemFiles.value(fileName);
        return std::make_unique<QWasmFileEngine>(fileName, file);
    }

    // Not an error, this function will be called with partial paths like "weblocalfile:/1/"
    return {};
}

// Check if this is a wasm filename by checking the URL scheme.
bool QWasmFileEngineHandler::isWasmFileName(const QString& fileName)
{
    return QUrl(fileName).scheme() == wasmlocalfileScheme;
}

// Creates a wasm filename using the custom URL scheme and a counter.
QString QWasmFileEngineHandler::makeWasmFileName(const QString &nativeFileName)
{
    static std::atomic<uint64_t> sid = 0;
    const uint64_t id = ++sid;
    return wasmlocalfileScheme + QStringLiteral(":/%1/%2").arg(id).arg(nativeFileName);
}

// Extracts the native filename from the custom URL (removes scheme and counter).
QString QWasmFileEngineHandler::nativeFileName(const QString &wasmFileName)
{
    QUrl url(wasmFileName);
    if (url.scheme() == wasmlocalfileScheme) {
        QString path = url.path();
        // Path is "/n/filename", find the second '/' and extract the filename
        const qsizetype idx = path.indexOf(u'/', 1);
        if (idx != -1)
            return path.mid(idx + 1);
    }
    return wasmFileName;
}

// Adds a File to the set of open files. Returns a prefixed wasm file name.
QString QWasmFileEngineHandler::addFile(qstdweb::File file)
{
    QString nativeFileName = QString::fromStdString(file.name());
    QString wasmFileName = makeWasmFileName(nativeFileName);
    singleton->m_files.insert(wasmFileName, file);
    return wasmFileName;
}

// Adds a FileSystemFileHandle to the set of open files. Returns a prefixed wasm file name.
QString QWasmFileEngineHandler::addFile(qstdweb::FileSystemFileHandle file)
{
    QString nativeFileName = QString::fromStdString(file.name());
    QString wasmFileName = makeWasmFileName(nativeFileName);
    singleton->m_fileSystemFiles.insert(wasmFileName, file);
    return wasmFileName;
}

// Removes a File or FileSystemFileHandle from the set of open file handlers
void QWasmFileEngineHandler::removeFile(const QString fileName)
{
    singleton->m_files.remove(fileName);
    singleton->m_fileSystemFiles.remove(fileName);
}

qstdweb::File QWasmFileEngineHandler::getFile(const QString fileName)
{
    return singleton->m_files.value(fileName);
}

qstdweb::FileSystemFileHandle QWasmFileEngineHandler::getFileSystemFile(const QString fileName)
{
    return singleton->m_fileSystemFiles.value(fileName);
}

/*!
    \class QWasmFileEngine
    \brief The QWasmFileEngine class provides a QAbstractFileEngine
    for files that has the prefix ':weblocalfile/'.
*/

// Constructs a QWasmFileEngine with a File for read-only access
QWasmFileEngine::QWasmFileEngine(const QString &fileName, qstdweb::File file)
    : m_fileName(fileName),
      m_blobDevice(std::make_unique<qstdweb::BlobIODevice>(file.slice(0, file.size())))
{
    // TODO use m_blobDevice in unbuffered mode? if there is already a buffer higher up.
}

// Constructs a QWasmFileEngine with a FileSystemFileHandle for read-write access
QWasmFileEngine::QWasmFileEngine(const QString &fileName, qstdweb::FileSystemFileHandle file)
    : m_fileName(fileName),
      m_fileDevice(std::make_unique<qstdweb::FileSystemFileIODevice>(file))
{

}

QWasmFileEngine::~QWasmFileEngine()
{
    close();
}

bool QWasmFileEngine::open(QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions)
{
    Q_UNUSED(permissions);
    m_openMode = openMode;

    if (m_fileDevice)
        return m_fileDevice->open(openMode);
    else if (m_blobDevice)
        return m_blobDevice->open(openMode);

    return false;
}

bool QWasmFileEngine::close()
{
    if (m_openMode == QIODevice::NotOpen)
        return false;

    bool success = true;
    if (m_fileDevice) {
        m_fileDevice->close();
    }
    if (m_blobDevice) {
        m_blobDevice->close();
    }

    m_openMode = QIODevice::NotOpen;
    return success;
}

bool QWasmFileEngine::flush()
{
    return true;
}

bool QWasmFileEngine::syncToDisk()
{
    return true;
}

qint64 QWasmFileEngine::size() const
{
    if (m_fileDevice)
        return m_fileDevice->size();
    if (m_blobDevice)
        return m_blobDevice->size();
    return 0;
}

qint64 QWasmFileEngine::pos() const
{
    if (m_fileDevice)
        return m_fileDevice->pos();
    if (m_blobDevice)
        return m_blobDevice->pos();
    return 0;
}

bool QWasmFileEngine::seek(qint64 pos)
{
    if (m_fileDevice)
        return m_fileDevice->seek(pos);
    if (m_blobDevice)
        return m_blobDevice->seek(pos);
    return false;
}

bool QWasmFileEngine::isSequential() const
{
    return false;
}

bool QWasmFileEngine::remove()
{
    return false;
}

bool QWasmFileEngine::copy(const QString &newName)
{
    Q_UNUSED(newName);
    return false;
}

bool QWasmFileEngine::rename(const QString &newName)
{
    Q_UNUSED(newName);
    return false;
}

bool QWasmFileEngine::renameOverwrite(const QString &newName)
{
    Q_UNUSED(newName);
    return false;
}

bool QWasmFileEngine::link(const QString &newName)
{
    Q_UNUSED(newName);
    return false;
}

bool QWasmFileEngine::mkdir(const QString &dirName, bool createParentDirectories,
                           std::optional<QFile::Permissions> permissions) const
{
    Q_UNUSED(dirName);
    Q_UNUSED(createParentDirectories);
    Q_UNUSED(permissions);
    return false;
}

bool QWasmFileEngine::rmdir(const QString &dirName, bool recurseParentDirectories) const
{
    Q_UNUSED(dirName);
    Q_UNUSED(recurseParentDirectories);
    return false;
}

bool QWasmFileEngine::setSize(qint64 size)
{
    Q_UNUSED(size);
    return false;
}

bool QWasmFileEngine::caseSensitive() const
{
    return true;
}

bool QWasmFileEngine::isRelativePath() const
{
    return false;
}


QAbstractFileEngine::FileFlags QWasmFileEngine::fileFlags(FileFlags type) const
{
    return type & (QAbstractFileEngine::FileFlag::ExistsFlag |
                    QAbstractFileEngine::FileFlag::FileType |
                    QAbstractFileEngine::FileFlag::ReadOwnerPerm |
                    QAbstractFileEngine::FileFlag::WriteOwnerPerm);
}

bool QWasmFileEngine::setPermissions(uint perms)
{
    Q_UNUSED(perms);
    return false;
}

QByteArray QWasmFileEngine::id() const
{
    return {};
}

QString QWasmFileEngine::fileName(FileName file) const
{
    switch (file) {
    case DefaultName:
    case AbsoluteName:
    case CanonicalName:
        return m_fileName;
    case BaseName: {
        QString native = QWasmFileEngineHandler::nativeFileName(m_fileName);
        QFileInfo info(native);
        return info.fileName();
    }
    case PathName:
    case AbsolutePathName:
    case CanonicalPathName: {
        QString native = QWasmFileEngineHandler::nativeFileName(m_fileName);
        QFileInfo info(native);
        QString path = info.path();
        return path.isEmpty() ? "."_L1 : path;
    }
    default:
        return QString();
    }
}

uint QWasmFileEngine::ownerId(FileOwner) const
{
    return 0;
}

QString QWasmFileEngine::owner(FileOwner) const
{
    return {};
}

bool QWasmFileEngine::setFileTime(const QDateTime &newDate, QFile::FileTime time)
{
    Q_UNUSED(newDate);
    Q_UNUSED(time);
    return false;
}

QDateTime QWasmFileEngine::fileTime(QFile::FileTime time) const
{
    Q_UNUSED(time);
    return {};
}

void QWasmFileEngine::setFileName(const QString &file)
{
    if (m_fileName == file)
        return;
    close();
    m_fileName = file;
}

int QWasmFileEngine::handle() const
{
    return -1;
}

QAbstractFileEngine::TriStateResult QWasmFileEngine::cloneTo(QAbstractFileEngine *target)
{
    Q_UNUSED(target);
    return QAbstractFileEngine::TriStateResult::NotSupported;
}

QAbstractFileEngine::IteratorUniquePtr QWasmFileEngine::beginEntryList(const QString &path, QDirListing::IteratorFlags filters,
                                                                        const QStringList &filterNames)
{
    Q_UNUSED(path);
    Q_UNUSED(filters);
    Q_UNUSED(filterNames);
    return nullptr;
}

qint64 QWasmFileEngine::read(char *data, qint64 maxlen)
{
    if (!(m_openMode & QIODevice::ReadOnly))
        return -1;

    if (m_fileDevice)
        return m_fileDevice->read(data, maxlen);
    if (m_blobDevice)
        return m_blobDevice->read(data, maxlen);
    return -1;
}

qint64 QWasmFileEngine::readLine(char *data, qint64 maxlen)
{
    if (!(m_openMode & QIODevice::ReadOnly))
        return -1;

    if (m_fileDevice)
        return m_fileDevice->readLine(data, maxlen);
    if (m_blobDevice)
        return m_blobDevice->readLine(data, maxlen);
    return -1;
}

qint64 QWasmFileEngine::write(const char *data, qint64 len)
{
    if (!(m_openMode & QIODevice::WriteOnly))
        return -1;

    if (m_fileDevice)
        return m_fileDevice->write(data, len);
    return -1;
}

bool QWasmFileEngine::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
    Q_UNUSED(extension);
    Q_UNUSED(option);
    Q_UNUSED(output);
    return false;
}

bool QWasmFileEngine::supportsExtension(Extension extension) const
{
    Q_UNUSED(extension);
    return false;
}

QT_END_NAMESPACE
