// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:file-handling

#include "androidjnimain.h"
#include "qandroidassetsfileenginehandler.h"

#include <optional>

#include <QCoreApplication>
#include <QList>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static const auto assetsPrefix = "assets:"_L1;
const static int prefixSize = 7;

static inline QString cleanedAssetPath(QString file)
{
    if (file.startsWith(assetsPrefix))
        file.remove(0, prefixSize);
    file.replace("//"_L1, "/"_L1);
    if (file.startsWith(u'/'))
        file.remove(0, 1);
    if (file.endsWith(u'/'))
        file.chop(1);
    return file;
}

static inline QString prefixedPath(QString path)
{
    path = assetsPrefix + u'/' + path;
    path.replace("//"_L1, "/"_L1);
    return path;
}

struct AssetItem {
    enum class Type {
        File,
        Folder,
        Invalid
    };
    AssetItem() = default;
    AssetItem (const QString &rawName)
        : name(rawName)
    {
        if (name.endsWith(u'/')) {
            type = Type::Folder;
            name.chop(1);
        }
    }
    Type type = Type::File;
    QString name;
    qint64 size = -1;
};

using AssetItemList = QList<AssetItem>;

class FolderIterator : public AssetItemList
{
public:
    static QSharedPointer<FolderIterator> fromCache(const QString &path, bool clone)
    {
        QMutexLocker lock(&m_assetsCacheMutex);
        QSharedPointer<FolderIterator> *folder = m_assetsCache.object(path);
        if (!folder) {
            folder = new QSharedPointer<FolderIterator>{new FolderIterator{path}};
            if ((*folder)->empty() || !m_assetsCache.insert(path, folder)) {
                QSharedPointer<FolderIterator> res = *folder;
                delete folder;
                return res;
            }
        }
        return clone ? QSharedPointer<FolderIterator>{new FolderIterator{*(*folder)}} : *folder;
    }

    static AssetItem::Type fileType(const QString &filePath)
    {
        if (filePath.isEmpty())
            return AssetItem::Type::Folder;
        const QStringList paths = filePath.split(u'/');
        QString fullPath;
        AssetItem::Type res = AssetItem::Type::Invalid;
        for (const auto &path: paths) {
            auto folder = fromCache(fullPath, false);
            auto it = std::lower_bound(folder->begin(), folder->end(), AssetItem{path}, [](const AssetItem &val, const AssetItem &assetItem) {
                return val.name < assetItem.name;
            });
            if (it == folder->end() || it->name != path)
                return AssetItem::Type::Invalid;
            if (!fullPath.isEmpty())
                fullPath.append(u'/');
            fullPath += path;
            res = it->type;
        }
        return res;
    }

    FolderIterator(const FolderIterator &other)
        : AssetItemList(other)
        , m_index(-1)
        , m_path(other.m_path)
    {}

    FolderIterator(const QString &path)
        : m_path(path)
    {
        // Note that empty dirs in the assets dir before the build are not going to be
        // included in the final apk, so no empty folders should expected to be listed.
        QJniObject files = QJniObject::callStaticObjectMethod(QtAndroid::applicationClass(),
                                                                            "listAssetContent",
                                                                            "(Landroid/content/res/AssetManager;Ljava/lang/String;)[Ljava/lang/String;",
                                                                            QtAndroid::assets(), QJniObject::fromString(path).object());
        if (files.isValid()) {
            QJniEnvironment env;
            jobjectArray jFiles = files.object<jobjectArray>();
            const jint nFiles = env->GetArrayLength(jFiles);
            for (int i = 0; i < nFiles; ++i) {
                AssetItem item{QJniObject::fromLocalRef(env->GetObjectArrayElement(jFiles, i)).toString()};
                insert(std::upper_bound(begin(), end(), item, [](const auto &a, const auto &b){
                    return a.name < b.name;
                }), item);
            }
        }
        m_path = assetsPrefix + u'/' + m_path + u'/';
        m_path.replace("//"_L1, "/"_L1);
    }

    QString currentFileName() const
    {
        if (m_index < 0 || m_index >= size())
            return {};
        return at(m_index).name;
    }
    QString currentFilePath() const
    {
        if (m_index < 0 || m_index >= size())
            return {};
        return m_path + at(m_index).name;
    }

    bool advance()
    {
        if (!empty() && m_index + 1 < size()) {
            ++m_index;
            return true;
        }
        return false;
    }

private:
    int m_index = -1;
    QString m_path;
    static QCache<QString, QSharedPointer<FolderIterator>> m_assetsCache;
    static QMutex m_assetsCacheMutex;
};

QCache<QString, QSharedPointer<FolderIterator>> FolderIterator::m_assetsCache(std::max(50, qEnvironmentVariableIntValue("QT_ANDROID_MAX_ASSETS_CACHE_SIZE")));
Q_CONSTINIT QMutex FolderIterator::m_assetsCacheMutex;

class AndroidAbstractFileEngineIterator: public QAbstractFileEngineIterator
{
public:
    AndroidAbstractFileEngineIterator(QDirListing::IteratorFlags filters,
                                      const QStringList &nameFilters,
                                      const QString &path)
        : QAbstractFileEngineIterator(path, filters, nameFilters)
    {
        m_currentIterator = FolderIterator::fromCache(cleanedAssetPath(path), true);
    }

    QFileInfo currentFileInfo() const override
    {
        return QFileInfo(currentFilePath());
    }

    QString currentFileName() const override
    {
        if (!m_currentIterator)
            return {};
        return m_currentIterator->currentFileName();
    }

    QString currentFilePath() const override
    {
        if (!m_currentIterator)
            return {};
        return m_currentIterator->currentFilePath();
    }

    bool advance() override
    {
        return m_currentIterator ? m_currentIterator->advance() : false;
    }

private:
    QSharedPointer<FolderIterator> m_currentIterator;
};

class AndroidAbstractFileEngine: public QAbstractFileEngine
{
public:
    explicit AndroidAbstractFileEngine(AAssetManager *assetManager, const QString &fileName)
        : m_assetManager(assetManager)
    {
        setFileName(fileName);
    }

    ~AndroidAbstractFileEngine()
    {
        close();
    }

    bool open(QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions) override
    {
        Q_UNUSED(permissions);

        if (!m_assetInfo || m_assetInfo->type != AssetItem::Type::File || (openMode & QIODevice::WriteOnly))
            return false;
        close();
        m_assetFile = AAssetManager_open(m_assetManager, m_fileName.toUtf8(), AASSET_MODE_BUFFER);
        return m_assetFile;
    }

    bool close() override
    {
        if (m_assetFile) {
            AAsset_close(m_assetFile);
            m_assetFile = 0;
            return true;
        }
        return false;
    }

    qint64 size() const override
    {
        if (m_assetInfo)
            return m_assetInfo->size;
        return -1;
    }

    qint64 pos() const override
    {
        if (m_assetFile)
            return AAsset_seek(m_assetFile, 0, SEEK_CUR);
        return -1;
    }

    bool seek(qint64 pos) override
    {
        if (m_assetFile)
            return pos == AAsset_seek(m_assetFile, pos, SEEK_SET);
        return false;
    }

    qint64 read(char *data, qint64 maxlen) override
    {
        if (m_assetFile)
            return AAsset_read(m_assetFile, data, maxlen);
        return -1;
    }

    bool caseSensitive() const override
    {
        return true;
    }

    FileFlags fileFlags(FileFlags type = FileInfoAll) const override
    {
        FileFlags commonFlags(ReadOwnerPerm|ReadUserPerm|ReadGroupPerm|ReadOtherPerm|ExistsFlag);
        FileFlags flags;
        if (m_assetInfo) {
            if (m_assetInfo->type == AssetItem::Type::File)
                flags = FileType | commonFlags;
            else if (m_assetInfo->type == AssetItem::Type::Folder)
                flags = DirectoryType | commonFlags;
        }
        return type & flags;
    }

    QString fileName(FileName file = DefaultName) const override
    {
        qsizetype pos;
        switch (file) {
        case DefaultName:
        case AbsoluteName:
        case CanonicalName:
                return prefixedPath(m_fileName);
        case BaseName:
            if ((pos = m_fileName.lastIndexOf(u'/')) != -1)
                return m_fileName.mid(pos + 1);
            else
                return m_fileName;
        case PathName:
        case AbsolutePathName:
        case CanonicalPathName:
            if ((pos = m_fileName.lastIndexOf(u'/')) != -1)
                return prefixedPath(m_fileName.left(pos));
            else
                return prefixedPath(m_fileName);
        default:
            return QString();
        }
    }

    void setFileName(const QString &file) override
    {
        if (m_fileName == cleanedAssetPath(file))
            return;
        close();
        m_fileName = cleanedAssetPath(file);

        {
            QMutexLocker lock(&m_assetsInfoCacheMutex);
            QSharedPointer<AssetItem> *assetInfoPtr = m_assetsInfoCache.object(m_fileName);
            if (assetInfoPtr) {
                m_assetInfo = *assetInfoPtr;
                return;
            }
        }

        QSharedPointer<AssetItem> *newAssetInfoPtr = new QSharedPointer<AssetItem>(new AssetItem);

        m_assetInfo = *newAssetInfoPtr;
        m_assetInfo->name = m_fileName;
        m_assetInfo->type = AssetItem::Type::Invalid;

        m_assetFile = AAssetManager_open(m_assetManager, m_fileName.toUtf8(), AASSET_MODE_BUFFER);

        if (m_assetFile) {
            m_assetInfo->type = AssetItem::Type::File;
            m_assetInfo->size = AAsset_getLength(m_assetFile);
        } else {
            auto *assetDir = AAssetManager_openDir(m_assetManager, m_fileName.toUtf8());
            if (assetDir) {
                if (AAssetDir_getNextFileName(assetDir)
                        || (!FolderIterator::fromCache(m_fileName, false)->empty())) {
                    // If AAssetDir_getNextFileName is not valid, it still can be a directory that
                    // contains only other directories (no files). FolderIterator will not be called
                    // on the directory containing files so it should not be too time consuming now.
                    m_assetInfo->type = AssetItem::Type::Folder;
                }
                AAssetDir_close(assetDir);
            }
        }

        QMutexLocker lock(&m_assetsInfoCacheMutex);
        m_assetsInfoCache.insert(m_fileName, newAssetInfoPtr);
    }

    IteratorUniquePtr beginEntryList(const QString &, QDirListing::IteratorFlags filters,
                                     const QStringList &filterNames) override
    {
        // AndroidAbstractFileEngineIterator use `m_fileName` as the path
        if (m_assetInfo && m_assetInfo->type == AssetItem::Type::Folder)
            return std::make_unique<AndroidAbstractFileEngineIterator>(filters, filterNames, m_fileName);
        return nullptr;
    }

private:
    AAsset *m_assetFile = nullptr;
    AAssetManager *m_assetManager = nullptr;
    // initialize with a name that can't be used as a file name
    QString m_fileName = "."_L1;
    QSharedPointer<AssetItem> m_assetInfo;

    static QCache<QString, QSharedPointer<AssetItem>> m_assetsInfoCache;
    static QMutex m_assetsInfoCacheMutex;
};

QCache<QString, QSharedPointer<AssetItem>> AndroidAbstractFileEngine::m_assetsInfoCache(std::max(200, qEnvironmentVariableIntValue("QT_ANDROID_MAX_FILEINFO_ASSETS_CACHE_SIZE")));
Q_CONSTINIT QMutex AndroidAbstractFileEngine::m_assetsInfoCacheMutex;

AndroidAssetsFileEngineHandler::AndroidAssetsFileEngineHandler()
{
    m_assetManager = QtAndroid::assetManager();
}

std::unique_ptr<QAbstractFileEngine>
AndroidAssetsFileEngineHandler::create(const QString &fileName) const
{
    if (fileName.isEmpty())
        return {};

    if (!fileName.startsWith(assetsPrefix))
        return {};

    QString path = fileName.mid(prefixSize);
    path.replace("//"_L1, "/"_L1);
    if (path.startsWith(u'/'))
        path.remove(0, 1);
    if (path.endsWith(u'/'))
        path.chop(1);
    return std::make_unique<AndroidAbstractFileEngine>(m_assetManager, path);
}

QT_END_NAMESPACE
