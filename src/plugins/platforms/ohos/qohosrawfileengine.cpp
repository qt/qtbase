// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosrawfileengine_p.h"

#include <QtCore/private/qabstractfileengine_p.h>
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/private/quniquehandle_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>

#include <rawfile/raw_dir.h>
#include <rawfile/raw_file.h>
#include <rawfile/raw_file_manager.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {

constexpr auto rawFileScheme = "rawfile:"_L1;

QString toRawFilePath(const QString &fileName)
{
    QString path = QDir::cleanPath(fileName.sliced(rawFileScheme.size()));
    if (path.startsWith(u'/'))
        path.remove(0, 1);
    if (path == u'.')
        path.clear();
    return path;
}

bool isRefusedRawFilePath(const QString &rawFilePath)
{
    return rawFilePath.contains(u'\0') || rawFilePath == ".."_L1
        || rawFilePath.startsWith("../"_L1);
}

QString toRawFileName(const QString &rawFilePath)
{
    return rawFileScheme + u'/' + rawFilePath;
}

template <typename Handle, void (*release)(Handle *)>
struct NdkHandleTraits
{
    using Type = Handle *;
    static Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        release(handle);
        return true;
    }
};

using UniqueRawFile =
    QUniqueHandle<NdkHandleTraits<::RawFile64, ::OH_ResourceManager_CloseRawFile64>>;

using UniqueNativeResourceManager =
    QUniqueHandle<NdkHandleTraits<
        ::NativeResourceManager, ::OH_ResourceManager_ReleaseNativeResourceManager>>;

using UniqueRawDir = QUniqueHandle<NdkHandleTraits<::RawDir, ::OH_ResourceManager_CloseRawDir>>;

class QOhosRawFileAccess
{
public:
    explicit QOhosRawFileAccess(UniqueNativeResourceManager resourceManager);
    Q_DISABLE_COPY_MOVE(QOhosRawFileAccess)

    bool isRawDir(QOhosJsState &jsState, const QByteArray &rawFilePath) const;
    std::optional<qint64> tryGetRawFileSize(
        QOhosJsState &jsState, const QByteArray &rawFilePath) const;
    std::shared_ptr<const ::RawFileDescriptor64> tryOpenRawFileDescriptor(
        QOhosJsState &jsState, const QByteArray &rawFilePath) const;
    std::vector<std::string> listRawDir(QOhosJsState &jsState, const QByteArray &rawDirPath) const;

private:
    UniqueRawFile tryOpenRawFile(QOhosJsState &jsState, const QByteArray &rawFilePath) const;

    UniqueNativeResourceManager m_resourceManager;
};

class QOhosRawFileEngineIterator final : public QAbstractFileEngineIterator
{
public:
    QOhosRawFileEngineIterator(
        const QString &path, QDirListing::IteratorFlags filters, const QStringList &nameFilters,
        QStringList entryNames);

    bool advance() override;
    QString currentFileName() const override;

private:
    QStringList m_entryNames;
    qsizetype m_currentIndex = -1;
};

class QOhosRawFileEngine final : public QAbstractFileEngine
{
    Q_DECLARE_TR_FUNCTIONS(QOhosRawFileEngine)

public:
    QOhosRawFileEngine(
        const QString &fileName, std::shared_ptr<const QOhosRawFileAccess> rawFileAccess);

    bool open(QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions) override;
    bool close() override;
    bool flush() override;
    qint64 size() const override;
    qint64 pos() const override;
    bool seek(qint64 pos) override;
    qint64 read(char *data, qint64 maxlen) override;
    bool caseSensitive() const override;
    FileFlags fileFlags(FileFlags type) const override;
    QString fileName(FileName file) const override;
    void setFileName(const QString &file) override;
    IteratorUniquePtr beginEntryList(
        const QString &path, QDirListing::IteratorFlags filters,
        const QStringList &nameFilters) override;

private:
    enum class EntryType {
        Missing,
        File,
        Directory,
    };

    struct EntryInfo
    {
        EntryType type = EntryType::Missing;
        qint64 size = 0;
    };

    EntryInfo entryInfo() const;
    EntryInfo queryEntryInfo() const;
    bool mayBeRawFile() const;
    QString parentRawFilePath() const;

    std::shared_ptr<const QOhosRawFileAccess> m_rawFileAccess;
    QString m_fileName;
    QString m_rawFilePath;
    mutable std::optional<EntryInfo> m_entryInfo;
    qint64 m_pos = 0;
    std::shared_ptr<const ::RawFileDescriptor64> m_rawFileDescriptor;
};

class QOhosRawFileEngineHandler final : public QAbstractFileEngineHandler
{
public:
    explicit QOhosRawFileEngineHandler(
        std::shared_ptr<const QOhosRawFileAccess> rawFileAccess);

    std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const override;

private:
    std::shared_ptr<const QOhosRawFileAccess> m_rawFileAccess;
};

QOhosRawFileAccess::QOhosRawFileAccess(UniqueNativeResourceManager resourceManager)
    : m_resourceManager(std::move(resourceManager))
{
}

bool QOhosRawFileAccess::isRawDir(QOhosJsState &, const QByteArray &rawFilePath) const
{
    return ::OH_ResourceManager_IsRawDir(m_resourceManager.get(), rawFilePath.constData());
}

UniqueRawFile QOhosRawFileAccess::tryOpenRawFile(
    QOhosJsState &jsState, const QByteArray &rawFilePath) const
{
    if (isRawDir(jsState, rawFilePath))
        return {};
    return UniqueRawFile(
        ::OH_ResourceManager_OpenRawFile64(m_resourceManager.get(), rawFilePath.constData()));
}

std::optional<qint64> QOhosRawFileAccess::tryGetRawFileSize(
    QOhosJsState &jsState, const QByteArray &rawFilePath) const
{
    const UniqueRawFile rawFile = tryOpenRawFile(jsState, rawFilePath);
    if (!rawFile)
        return std::nullopt;
    return ::OH_ResourceManager_GetRawFileSize64(rawFile.get());
}

std::shared_ptr<const ::RawFileDescriptor64> QOhosRawFileAccess::tryOpenRawFileDescriptor(
    QOhosJsState &jsState, const QByteArray &rawFilePath) const
{
    UniqueRawFile rawFile = tryOpenRawFile(jsState, rawFilePath);
    if (!rawFile)
        return {};
    ::RawFileDescriptor64 rawFileDescriptor = {};
    if (!::OH_ResourceManager_GetRawFileDescriptor64(rawFile.get(), &rawFileDescriptor)) {
        qOhosPrintfWarning(
            "%s: cannot get the descriptor of raw file '%s'", Q_FUNC_INFO, rawFilePath.constData());
        return {};
    }
    return QtOhos::makeProxyWithJsThreadDeleter(std::shared_ptr<const ::RawFileDescriptor64>(
        new ::RawFileDescriptor64(rawFileDescriptor),
        [rawFile = std::move(rawFile)](const ::RawFileDescriptor64 *rawFileDescriptor) mutable {
            if (!::OH_ResourceManager_ReleaseRawFileDescriptor64(rawFileDescriptor)) {
                qOhosPrintfWarning(
                    "%s: cannot release the descriptor of a raw file, fd %d", Q_FUNC_INFO,
                    rawFileDescriptor->fd);
            }
            rawFile.close();
            delete rawFileDescriptor;
        }));
}

std::vector<std::string> QOhosRawFileAccess::listRawDir(
    QOhosJsState &, const QByteArray &rawDirPath) const
{
    const UniqueRawDir rawDir(
        ::OH_ResourceManager_OpenRawDir(m_resourceManager.get(), rawDirPath.constData()));
    if (!rawDir) {
        qOhosPrintfWarning(
            "%s: cannot open raw directory '%s'", Q_FUNC_INFO, rawDirPath.constData());
        return {};
    }
    const int entryCount = ::OH_ResourceManager_GetRawFileCount(rawDir.get());
    std::vector<std::string> entryNames;
    entryNames.reserve(entryCount);
    for (int entryIndex = 0; entryIndex < entryCount; ++entryIndex) {
        const char *entryName = ::OH_ResourceManager_GetRawFileName(rawDir.get(), entryIndex);
        if (!entryName) {
            qOhosReportFatalErrorAndAbort(
                "%s: got inconsistent result from OH_ResourceManager_GetRawFileName() call: "
                "name %d of %d is null",
                Q_FUNC_INFO, entryIndex, entryCount);
        }
        entryNames.emplace_back(entryName);
    }
    return entryNames;
}

QOhosRawFileEngineIterator::QOhosRawFileEngineIterator(
    const QString &path, QDirListing::IteratorFlags filters, const QStringList &nameFilters,
    QStringList entryNames)
    : QAbstractFileEngineIterator(path, filters, nameFilters)
    , m_entryNames(std::move(entryNames))
{
}

bool QOhosRawFileEngineIterator::advance()
{
    if (m_currentIndex + 1 >= m_entryNames.size())
        return false;
    ++m_currentIndex;
    return true;
}

QString QOhosRawFileEngineIterator::currentFileName() const
{
    return m_entryNames.at(m_currentIndex);
}

QOhosRawFileEngine::QOhosRawFileEngine(
    const QString &fileName, std::shared_ptr<const QOhosRawFileAccess> rawFileAccess)
    : m_rawFileAccess(std::move(rawFileAccess))
{
    setFileName(fileName);
}

bool QOhosRawFileEngine::open(
    QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions)
{
    Q_UNUSED(permissions);
    if (openMode & QIODevice::WriteOnly) {
        setError(QFile::OpenError, tr("Raw files are read-only"));
        return false;
    }
    if (!mayBeRawFile()) {
        setError(QFile::OpenError, tr("No such raw file"));
        return false;
    }
    const QByteArray rawFilePath = m_rawFilePath.toUtf8();
    m_rawFileDescriptor = QOhosJsThreadGateway::eval(
        [&](QOhosJsState &jsState) {
            return m_rawFileAccess->tryOpenRawFileDescriptor(jsState, rawFilePath);
        },
        Q_FUNC_INFO);
    if (!m_rawFileDescriptor) {
        setError(QFile::OpenError, tr("Cannot open raw file"));
        return false;
    }
    m_entryInfo = EntryInfo{EntryType::File, m_rawFileDescriptor->length};
    return true;
}

bool QOhosRawFileEngine::close()
{
    m_rawFileDescriptor.reset();
    m_pos = 0;
    return true;
}

bool QOhosRawFileEngine::flush()
{
    return true;
}

qint64 QOhosRawFileEngine::size() const
{
    return entryInfo().size;
}

qint64 QOhosRawFileEngine::pos() const
{
    return m_pos;
}

bool QOhosRawFileEngine::seek(qint64 pos)
{
    if (!m_rawFileDescriptor || pos < 0)
        return false;
    m_pos = qMin(pos, m_rawFileDescriptor->length);
    return true;
}

qint64 QOhosRawFileEngine::read(char *data, qint64 maxlen)
{
    if (!m_rawFileDescriptor)
        return -1;
    const ::RawFileDescriptor64 &descriptor = *m_rawFileDescriptor;
    const qint64 readLength = qMin(maxlen, descriptor.length - m_pos);
    ssize_t bytesRead = 0;
    QT_EINTR_LOOP(bytesRead, ::pread(descriptor.fd, data, readLength, descriptor.start + m_pos));
    if (bytesRead < 0) {
        setError(QFile::ReadError, qt_error_string(errno));
        return -1;
    }
    m_pos += bytesRead;
    return bytesRead;
}

bool QOhosRawFileEngine::caseSensitive() const
{
    return true;
}

QAbstractFileEngine::FileFlags QOhosRawFileEngine::fileFlags(FileFlags type) const
{
    FileFlags flags;
    switch (entryInfo().type) {
    case EntryType::Missing:
        return {};
    case EntryType::File:
        flags = FileType;
        break;
    case EntryType::Directory:
        flags = DirectoryType;
        break;
    }
    flags |= ExistsFlag | ReadOwnerPerm | ReadUserPerm | ReadGroupPerm | ReadOtherPerm;
    if (m_rawFilePath.isEmpty())
        flags |= RootFlag;
    return type & flags;
}

QString QOhosRawFileEngine::fileName(FileName file) const
{
    switch (file) {
    case DefaultName:
        return m_fileName;
    case CanonicalName:
        if (entryInfo().type == EntryType::Missing)
            return {};
        Q_FALLTHROUGH();
    case AbsoluteName:
        return toRawFileName(m_rawFilePath);
    case BaseName:
        return m_rawFilePath.sliced(m_rawFilePath.lastIndexOf(u'/') + 1);
    case CanonicalPathName:
        if (entryInfo().type == EntryType::Missing)
            return {};
        Q_FALLTHROUGH();
    case PathName:
    case AbsolutePathName:
        return toRawFileName(parentRawFilePath());
    case AbsoluteLinkTarget:
    case BundleName:
    case JunctionName:
    case RawLinkPath:
    case NFileNames:
        break;
    }
    return {};
}

void QOhosRawFileEngine::setFileName(const QString &file)
{
    close();
    m_fileName = file;
    m_rawFilePath = toRawFilePath(file);
    m_entryInfo.reset();
}

QOhosRawFileEngine::EntryInfo QOhosRawFileEngine::entryInfo() const
{
    if (!m_entryInfo)
        m_entryInfo = queryEntryInfo();
    return *m_entryInfo;
}

QOhosRawFileEngine::EntryInfo QOhosRawFileEngine::queryEntryInfo() const
{
    if (isRefusedRawFilePath(m_rawFilePath))
        return EntryInfo{EntryType::Missing};
    if (m_rawFilePath.isEmpty())
        return EntryInfo{EntryType::Directory};
    const QByteArray rawFilePath = m_rawFilePath.toUtf8();
    return QOhosJsThreadGateway::eval(
        [&](QOhosJsState &jsState) {
            if (m_rawFileAccess->isRawDir(jsState, rawFilePath))
                return EntryInfo{EntryType::Directory};
            const std::optional<qint64> rawFileSize =
                m_rawFileAccess->tryGetRawFileSize(jsState, rawFilePath);
            if (!rawFileSize)
                return EntryInfo{EntryType::Missing};
            return EntryInfo{EntryType::File, *rawFileSize};
        },
        Q_FUNC_INFO);
}

bool QOhosRawFileEngine::mayBeRawFile() const
{
    if (m_entryInfo)
        return m_entryInfo->type == EntryType::File;
    return !m_rawFilePath.isEmpty() && !isRefusedRawFilePath(m_rawFilePath);
}

QAbstractFileEngine::IteratorUniquePtr QOhosRawFileEngine::beginEntryList(
    const QString &path, QDirListing::IteratorFlags filters, const QStringList &nameFilters)
{
    if (entryInfo().type != EntryType::Directory)
        return {};
    const QByteArray rawDirPath = m_rawFilePath.toUtf8();
    const std::vector<std::string> rawEntryNames = QOhosJsThreadGateway::eval(
        [&](QOhosJsState &jsState) {
            return m_rawFileAccess->listRawDir(jsState, rawDirPath);
        },
        Q_FUNC_INFO);
    QStringList entryNames;
    entryNames.reserve(rawEntryNames.size());
    for (const std::string &rawEntryName : rawEntryNames) {
        const QString entryPath = QString::fromStdString(rawEntryName);
        entryNames.append(entryPath.sliced(entryPath.lastIndexOf(u'/') + 1));
    }
    return std::make_unique<QOhosRawFileEngineIterator>(
        path, filters, nameFilters, std::move(entryNames));
}

QString QOhosRawFileEngine::parentRawFilePath() const
{
    const qsizetype separatorIndex = m_rawFilePath.lastIndexOf(u'/');
    if (separatorIndex < 0)
        return {};
    return m_rawFilePath.first(separatorIndex);
}

QOhosRawFileEngineHandler::QOhosRawFileEngineHandler(
    std::shared_ptr<const QOhosRawFileAccess> rawFileAccess)
    : m_rawFileAccess(std::move(rawFileAccess))
{
}

std::unique_ptr<QAbstractFileEngine> QOhosRawFileEngineHandler::create(
    const QString &fileName) const
{
    if (!fileName.startsWith(rawFileScheme))
        return {};
    return std::make_unique<QOhosRawFileEngine>(fileName, m_rawFileAccess);
}

}

namespace QOhosRawFile {

void installEngineHandler(QtOhos::JsState &jsState, QNapi::Object qAbility)
{
    UniqueNativeResourceManager resourceManager(::OH_ResourceManager_InitNativeResourceManager(
        jsState.env(),
        qAbility.eval<QNapi::Object>("context.getApplicationContext().resourceManager")));
    if (!resourceManager) {
        qOhosPrintfWarning(
            "%s: no native resource manager, rawfile: paths are unavailable", Q_FUNC_INFO);
        return;
    }
    jsState.getAttachedObjectWithLazyCreate<std::optional<QOhosRawFileEngineHandler>>().emplace(
        QtOhos::makeProxyWithJsThreadDeleter(
            std::make_shared<const QOhosRawFileAccess>(std::move(resourceManager))));
}

}

QT_END_NAMESPACE
