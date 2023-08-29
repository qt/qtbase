// Copyright (C) 2019 Volker Krause <vkrause@kde.org>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidcontentfileengine.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmimedatabase.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface;
using namespace Qt::StringLiterals;

Q_DECLARE_JNI_TYPE(ContentResolverType, "Landroid/content/ContentResolver;");
Q_DECLARE_JNI_TYPE(UriType, "Landroid/net/Uri;");
Q_DECLARE_JNI_CLASS(Uri, "android/net/Uri");
Q_DECLARE_JNI_TYPE(ParcelFileDescriptorType, "Landroid/os/ParcelFileDescriptor;");
Q_DECLARE_JNI_TYPE(CursorType, "Landroid/database/Cursor;");
Q_DECLARE_JNI_TYPE(StringArray, "[Ljava/lang/String;");

static QJniObject &contentResolverInstance()
{
    static QJniObject contentResolver;
    if (!contentResolver.isValid()) {
        contentResolver = QJniObject(QNativeInterface::QAndroidApplication::context())
                .callMethod<QtJniTypes::ContentResolverType>("getContentResolver");
    }

    return contentResolver;
}

AndroidContentFileEngine::AndroidContentFileEngine(const QString &filename)
    : m_initialFile(filename),
      m_documentFile(DocumentFile::parseFromAnyUri(filename))
{
    setFileName(filename);
}

bool AndroidContentFileEngine::open(QIODevice::OpenMode openMode,
                                    std::optional<QFile::Permissions> permissions)
{
    Q_UNUSED(permissions);
    QString openModeStr;
    if (openMode & QFileDevice::ReadOnly) {
        openModeStr += u'r';
    }
    if (openMode & QFileDevice::WriteOnly) {
        openModeStr += u'w';
        if (!m_documentFile->exists()) {
            if (QUrl(m_initialFile).path().startsWith("/tree/"_L1)) {
                const int lastSeparatorIndex = m_initialFile.lastIndexOf('/');
                const QString fileName = m_initialFile.mid(lastSeparatorIndex + 1);

                QString mimeType;
                const auto mimeTypes = QMimeDatabase().mimeTypesForFileName(fileName);
                if (!mimeTypes.empty())
                    mimeType = mimeTypes.first().name();
                else
                    mimeType = "application/octet-stream";

                if (m_documentFile->parent()) {
                    auto createdFile = m_documentFile->parent()->createFile(mimeType, fileName);
                    if (createdFile)
                        m_documentFile = createdFile;
                }
            } else {
                qWarning() << "open(): non-existent content URI with a document type provided";
            }
        }
    }
    if (openMode & QFileDevice::Truncate) {
        openModeStr += u't';
    } else if (openMode & QFileDevice::Append) {
        openModeStr += u'a';
    }

    m_pfd = contentResolverInstance().callMethod<
            QtJniTypes::ParcelFileDescriptorType, QtJniTypes::UriType, jstring>(
                "openFileDescriptor",
                m_documentFile->uri().object(),
                QJniObject::fromString(openModeStr).object<jstring>());

    if (!m_pfd.isValid())
        return false;

    const auto fd = m_pfd.callMethod<jint>("getFd");

    if (fd < 0) {
        closeNativeFileDescriptor();
        return false;
    }

    return QFSFileEngine::open(openMode, fd, QFile::DontCloseHandle);
}

bool AndroidContentFileEngine::close()
{
    closeNativeFileDescriptor();
    return QFSFileEngine::close();
}

void AndroidContentFileEngine::closeNativeFileDescriptor()
{
    if (m_pfd.isValid()) {
        m_pfd.callMethod<void>("close");
        m_pfd = QJniObject();
    }
}

qint64 AndroidContentFileEngine::size() const
{
    return m_documentFile->length();
}

bool AndroidContentFileEngine::remove()
{
    return m_documentFile->remove();
}

bool AndroidContentFileEngine::rename(const QString &newName)
{
    if (m_documentFile->rename(newName)) {
        m_initialFile = m_documentFile->uri().toString();
        return true;
    }
    return false;
}

bool AndroidContentFileEngine::mkdir(const QString &dirName, bool createParentDirectories,
                                     std::optional<QFileDevice::Permissions> permissions) const
{
    Q_UNUSED(permissions)

    QString tmp = dirName;
    tmp.remove(m_initialFile);

    QStringList dirParts = tmp.split(u'/');
    dirParts.removeAll("");

    if (dirParts.isEmpty())
        return false;

    auto createdDir = m_documentFile;
    bool allDirsCreated = true;
    for (const auto &dir : dirParts) {
        // Find if the sub-dir already exists and then don't re-create it
        bool subDirExists = false;
        for (const DocumentFilePtr &subDir : m_documentFile->listFiles()) {
            if (dir == subDir->name() && subDir->isDirectory()) {
                createdDir = subDir;
                subDirExists = true;
            }
        }

        if (!subDirExists) {
            createdDir = createdDir->createDirectory(dir);
            if (!createdDir) {
                allDirsCreated = false;
                break;
            }
        }

        if (!createParentDirectories)
            break;
    }

    return allDirsCreated;
}

bool AndroidContentFileEngine::rmdir(const QString &dirName, bool recurseParentDirectories) const
{
    if (recurseParentDirectories)
        qWarning() << "rmpath(): Unsupported for Content URIs";

    const QString dirFileName = QUrl(dirName).fileName();
    bool deleted = false;
    for (const DocumentFilePtr &dir : m_documentFile->listFiles()) {
        if (dirFileName == dir->name() && dir->isDirectory()) {
            deleted = dir->remove();
            break;
        }
    }

    return deleted;
}

QByteArray AndroidContentFileEngine::id() const
{
    return m_documentFile->id().toUtf8();
}

QDateTime AndroidContentFileEngine::fileTime(FileTime time) const
{
    switch (time) {
    case FileTime::ModificationTime:
        return m_documentFile->lastModified();
        break;
    default:
        break;
    }

    return QDateTime();
}

AndroidContentFileEngine::FileFlags AndroidContentFileEngine::fileFlags(FileFlags type) const
{
    FileFlags flags;
    if (!m_documentFile->exists())
        return flags;

    flags = ExistsFlag;
    if (!m_documentFile->canRead())
        return flags;

    flags |= ReadOwnerPerm|ReadUserPerm|ReadGroupPerm|ReadOtherPerm;

    if (m_documentFile->isDirectory()) {
        flags |= DirectoryType;
    } else {
        flags |= FileType;
        if (m_documentFile->canWrite())
            flags |= WriteOwnerPerm|WriteUserPerm|WriteGroupPerm|WriteOtherPerm;
    }
    return type & flags;
}

QString AndroidContentFileEngine::fileName(FileName f) const
{
    switch (f) {
        case PathName:
        case AbsolutePathName:
        case CanonicalPathName:
        case DefaultName:
        case AbsoluteName:
        case CanonicalName:
            return m_documentFile->uri().toString();
        case BaseName:
            return m_documentFile->name();
        default:
            break;
    }

    return QString();
}

QAbstractFileEngine::Iterator *AndroidContentFileEngine::beginEntryList(QDir::Filters filters,
                                                                    const QStringList &filterNames)
{
    return new AndroidContentFileEngineIterator(filters, filterNames);
}

QAbstractFileEngine::Iterator *AndroidContentFileEngine::endEntryList()
{
    return nullptr;
}

AndroidContentFileEngineHandler::AndroidContentFileEngineHandler() = default;
AndroidContentFileEngineHandler::~AndroidContentFileEngineHandler() = default;

QAbstractFileEngine* AndroidContentFileEngineHandler::create(const QString &fileName) const
{
    if (!fileName.startsWith("content"_L1))
        return nullptr;

    return new AndroidContentFileEngine(fileName);
}

AndroidContentFileEngineIterator::AndroidContentFileEngineIterator(QDir::Filters filters,
                                                                   const QStringList &filterNames)
    : QAbstractFileEngineIterator(filters, filterNames)
{
}

AndroidContentFileEngineIterator::~AndroidContentFileEngineIterator()
{
}

QString AndroidContentFileEngineIterator::next()
{
    if (!hasNext())
        return QString();
    ++m_index;
    return currentFilePath();
}

bool AndroidContentFileEngineIterator::hasNext() const
{
    if (m_index == -1 && m_files.isEmpty()) {
        const auto currentPath = path();
        if (currentPath.isEmpty())
            return false;

        const auto iterDoc = DocumentFile::parseFromAnyUri(currentPath);
        if (iterDoc->isDirectory())
            for (const auto &doc : iterDoc->listFiles())
                m_files.append(doc);
    }

    return m_index < (m_files.size() - 1);
}

QString AndroidContentFileEngineIterator::currentFileName() const
{
    if (m_index < 0 || m_index > m_files.size())
        return QString();
    return m_files.at(m_index)->name();
}

QString AndroidContentFileEngineIterator::currentFilePath() const
{
    if (m_index < 0 || m_index > m_files.size())
        return QString();
    return m_files.at(m_index)->uri().toString();
}

// Start of Cursor

class Cursor
{
public:
    explicit Cursor(const QJniObject &object)
        : m_object{object} { }

    ~Cursor()
    {
        if (m_object.isValid())
            m_object.callMethod<void>("close");
    }

    enum Type {
        FIELD_TYPE_NULL       = 0x00000000,
        FIELD_TYPE_INTEGER    = 0x00000001,
        FIELD_TYPE_FLOAT      = 0x00000002,
        FIELD_TYPE_STRING     = 0x00000003,
        FIELD_TYPE_BLOB       = 0x00000004
    };

    QVariant data(int columnIndex) const
    {
        int type = m_object.callMethod<jint>("getType", columnIndex);
        switch (type) {
        case FIELD_TYPE_NULL:
            return {};
        case FIELD_TYPE_INTEGER:
            return QVariant::fromValue(m_object.callMethod<jlong>("getLong", columnIndex));
        case FIELD_TYPE_FLOAT:
            return QVariant::fromValue(m_object.callMethod<jdouble>("getDouble", columnIndex));
        case FIELD_TYPE_STRING:
            return QVariant::fromValue(m_object.callMethod<jstring>("getString",
                                                                    columnIndex).toString());
        case FIELD_TYPE_BLOB: {
            auto blob = m_object.callMethod<jbyteArray>("getBlob", columnIndex);
            QJniEnvironment env;
            const auto blobArray = blob.object<jbyteArray>();
            const int size = env->GetArrayLength(blobArray);
            const auto byteArray = env->GetByteArrayElements(blobArray, nullptr);
            QByteArray data{reinterpret_cast<const char *>(byteArray), size};
            env->ReleaseByteArrayElements(blobArray, byteArray, 0);
            return QVariant::fromValue(data);
        }
        }
        return {};
    }

    static std::unique_ptr<Cursor> queryUri(const QJniObject &uri,
                                            const QStringList &projection = {},
                                            const QString &selection = {},
                                            const QStringList &selectionArgs = {},
                                            const QString &sortOrder = {})
    {
        auto cursor = contentResolverInstance().callMethod<QtJniTypes::CursorType>(
            "query",
            uri.object<QtJniTypes::UriType>(),
            projection.isEmpty() ?
                        nullptr : fromStringList(projection).object<QtJniTypes::StringArray>(),
            selection.isEmpty() ? nullptr : QJniObject::fromString(selection).object<jstring>(),
            selectionArgs.isEmpty() ?
                        nullptr : fromStringList(selectionArgs).object<QtJniTypes::StringArray>(),
            sortOrder.isEmpty() ? nullptr : QJniObject::fromString(sortOrder).object<jstring>());
        if (!cursor.isValid())
            return {};
        return std::make_unique<Cursor>(cursor);
    }

    static QVariant queryColumn(const QJniObject &uri, const QString &column)
    {
        const auto query = queryUri(uri, {column});
        if (!query)
            return {};

        if (query->rowCount() != 1 || query->columnCount() != 1)
            return {};
        query->moveToFirst();
        return query->data(0);
    }

    bool isNull(int columnIndex) const
    {
        return m_object.callMethod<jboolean>("isNull", columnIndex);
    }

    int columnCount() const { return m_object.callMethod<jint>("getColumnCount"); }
    int rowCount() const { return m_object.callMethod<jint>("getCount"); }
    int row() const { return m_object.callMethod<jint>("getPosition"); }
    bool isFirst() const { return m_object.callMethod<jboolean>("isFirst"); }
    bool isLast() const { return m_object.callMethod<jboolean>("isLast"); }
    bool moveToFirst() { return m_object.callMethod<jboolean>("moveToFirst"); }
    bool moveToLast() { return m_object.callMethod<jboolean>("moveToLast"); }
    bool moveToNext() { return m_object.callMethod<jboolean>("moveToNext"); }

private:
    static QJniObject fromStringList(const QStringList &list)
    {
        QJniEnvironment env;
        auto array = env->NewObjectArray(list.size(), env.findClass("java/lang/String"), nullptr);
        for (int i = 0; i < list.size(); ++i)
            env->SetObjectArrayElement(array, i, QJniObject::fromString(list[i]).object());
        return QJniObject::fromLocalRef(array);
    }

    QJniObject m_object;
};

// End of Cursor

// Start of DocumentsContract

Q_DECLARE_JNI_CLASS(DocumentsContract, "android/provider/DocumentsContract");

/*!
 *
 * DocumentsContract Api.
 * Check https://developer.android.com/reference/android/provider/DocumentsContract
 * for more information.
 *
 * \note This does not implement all facilities of the native API.
 *
 */
namespace DocumentsContract
{

namespace Document {
const QLatin1String COLUMN_DISPLAY_NAME("_display_name");
const QLatin1String COLUMN_DOCUMENT_ID("document_id");
const QLatin1String COLUMN_FLAGS("flags");
const QLatin1String COLUMN_LAST_MODIFIED("last_modified");
const QLatin1String COLUMN_MIME_TYPE("mime_type");
const QLatin1String COLUMN_SIZE("_size");

constexpr int FLAG_DIR_SUPPORTS_CREATE = 0x00000008;
constexpr int FLAG_SUPPORTS_DELETE = 0x00000004;
constexpr int FLAG_SUPPORTS_MOVE = 0x00000100;
constexpr int FLAG_SUPPORTS_RENAME = 0x00000040;
constexpr int FLAG_SUPPORTS_WRITE = 0x00000002;
constexpr int FLAG_VIRTUAL_DOCUMENT = 0x00000200;

const QLatin1String MIME_TYPE_DIR("vnd.android.document/directory");
} // namespace Document

QString documentId(const QJniObject &uri)
{
    return QJniObject::callStaticMethod<jstring, QtJniTypes::UriType>(
                QtJniTypes::className<QtJniTypes::DocumentsContract>(),
                "getDocumentId",
                uri.object()).toString();
}

QString treeDocumentId(const QJniObject &uri)
{
    return QJniObject::callStaticMethod<jstring, QtJniTypes::UriType>(
                QtJniTypes::className<QtJniTypes::DocumentsContract>(),
                "getTreeDocumentId",
                uri.object()).toString();
}

QJniObject buildChildDocumentsUriUsingTree(const QJniObject &uri, const QString &parentDocumentId)
{
    return QJniObject::callStaticMethod<QtJniTypes::UriType>(
                QtJniTypes::className<QtJniTypes::DocumentsContract>(),
                "buildChildDocumentsUriUsingTree",
                uri.object<QtJniTypes::UriType>(),
                QJniObject::fromString(parentDocumentId).object<jstring>());

}

QJniObject buildDocumentUriUsingTree(const QJniObject &treeUri, const QString &documentId)
{
    return QJniObject::callStaticMethod<QtJniTypes::UriType>(
                QtJniTypes::className<QtJniTypes::DocumentsContract>(),
                "buildDocumentUriUsingTree",
                treeUri.object<QtJniTypes::UriType>(),
                QJniObject::fromString(documentId).object<jstring>());
}

bool isDocumentUri(const QJniObject &uri)
{
    return QJniObject::callStaticMethod<jboolean>(
                QtJniTypes::className<QtJniTypes::DocumentsContract>(),
                "isDocumentUri",
                QNativeInterface::QAndroidApplication::context(),
                uri.object<QtJniTypes::UriType>());
}

bool isTreeUri(const QJniObject &uri)
{
    return QJniObject::callStaticMethod<jboolean>(
                QtJniTypes::className<QtJniTypes::DocumentsContract>(),
                "isTreeUri",
                uri.object<QtJniTypes::UriType>());
}

QJniObject createDocument(const QJniObject &parentDocumentUri, const QString &mimeType,
                          const QString &displayName)
{
    return QJniObject::callStaticMethod<QtJniTypes::UriType>(
                QtJniTypes::className<QtJniTypes::DocumentsContract>(),
                "createDocument",
                contentResolverInstance().object<QtJniTypes::ContentResolverType>(),
                parentDocumentUri.object<QtJniTypes::UriType>(),
                QJniObject::fromString(mimeType).object<jstring>(),
                QJniObject::fromString(displayName).object<jstring>());
}

bool deleteDocument(const QJniObject &documentUri)
{
    const int flags = Cursor::queryColumn(documentUri, Document::COLUMN_FLAGS).toInt();
    if (!(flags & Document::FLAG_SUPPORTS_DELETE))
        return {};

    return QJniObject::callStaticMethod<jboolean>(
                QtJniTypes::className<QtJniTypes::DocumentsContract>(),
                "deleteDocument",
                contentResolverInstance().object<QtJniTypes::ContentResolverType>(),
                documentUri.object<QtJniTypes::UriType>());
}

QJniObject moveDocument(const QJniObject &sourceDocumentUri,
                      const QJniObject &sourceParentDocumentUri,
                      const QJniObject &targetParentDocumentUri)
{
    const int flags = Cursor::queryColumn(sourceDocumentUri, Document::COLUMN_FLAGS).toInt();
    if (!(flags & Document::FLAG_SUPPORTS_MOVE))
        return {};

    return QJniObject::callStaticMethod<QtJniTypes::UriType>(
                QtJniTypes::className<QtJniTypes::DocumentsContract>(),
                "moveDocument",
                contentResolverInstance().object<QtJniTypes::ContentResolverType>(),
                sourceDocumentUri.object<QtJniTypes::UriType>(),
                sourceParentDocumentUri.object<QtJniTypes::UriType>(),
                targetParentDocumentUri.object<QtJniTypes::UriType>());
}

QJniObject renameDocument(const QJniObject &documentUri, const QString &displayName)
{
    const int flags = Cursor::queryColumn(documentUri, Document::COLUMN_FLAGS).toInt();
    if (!(flags & Document::FLAG_SUPPORTS_RENAME))
        return {};

    return QJniObject::callStaticMethod<QtJniTypes::UriType>(
                QtJniTypes::className<QtJniTypes::DocumentsContract>(),
                "renameDocument",
                contentResolverInstance().object<QtJniTypes::ContentResolverType>(),
                documentUri.object<QtJniTypes::UriType>(),
                QJniObject::fromString(displayName).object<jstring>());
}
} // End DocumentsContract namespace

// Start of DocumentFile

using namespace DocumentsContract;

namespace {
class MakeableDocumentFile : public DocumentFile
{
public:
    MakeableDocumentFile(const QJniObject &uri, const DocumentFilePtr &parent = {})
        : DocumentFile(uri, parent)
    {}
};
}

DocumentFile::DocumentFile(const QJniObject &uri,
                           const DocumentFilePtr &parent)
    : m_uri{uri}
    , m_parent{parent}
{}

QJniObject parseUri(const QString &uri)
{
    QString uriToParse = uri;
    if (uriToParse.contains(' '))
        uriToParse.replace(' ', QUrl::toPercentEncoding(" "));

    return QJniObject::callStaticMethod<QtJniTypes::UriType>(
                QtJniTypes::className<QtJniTypes::Uri>(),
                "parse",
                QJniObject::fromString(uriToParse).object<jstring>());
}

DocumentFilePtr DocumentFile::parseFromAnyUri(const QString &fileName)
{
    const QString encodedUri = QUrl(fileName).toEncoded();
    const QJniObject uri = parseUri(encodedUri);

    if (DocumentsContract::isDocumentUri(uri))
        return fromSingleUri(uri);

    const QString documentType = "/document/"_L1;
    const QString treeType = "/tree/"_L1;

    const int treeIndex = encodedUri.indexOf(treeType);
    const int documentIndex = encodedUri.indexOf(documentType);
    const int index = fileName.lastIndexOf("/");

    if (index <= treeIndex + treeType.size() || index <= documentIndex + documentType.size())
        return fromTreeUri(uri);

    const QString parentUrl = encodedUri.left(index);
    DocumentFilePtr parentDocFile = fromTreeUri(parseUri(parentUrl));

    const QString baseName = encodedUri.mid(index);
    const QString fileUrl = parentUrl + QUrl::toPercentEncoding(baseName);

    DocumentFilePtr docFile = std::make_shared<MakeableDocumentFile>(parseUri(fileUrl));
    if (parentDocFile && parentDocFile->isDirectory())
        docFile->m_parent = parentDocFile;

    return docFile;
}

DocumentFilePtr DocumentFile::fromSingleUri(const QJniObject &uri)
{
    return std::make_shared<MakeableDocumentFile>(uri);
}

DocumentFilePtr DocumentFile::fromTreeUri(const QJniObject &treeUri)
{
    QString docId;
    if (isDocumentUri(treeUri))
        docId = documentId(treeUri);
    else
        docId = treeDocumentId(treeUri);

    return std::make_shared<MakeableDocumentFile>(buildDocumentUriUsingTree(treeUri, docId));
}

DocumentFilePtr DocumentFile::createFile(const QString &mimeType, const QString &displayName)
{
    if (isDirectory()) {
        return std::make_shared<MakeableDocumentFile>(
                    createDocument(m_uri, mimeType, displayName),
                    shared_from_this());
    }
    return {};
}

DocumentFilePtr DocumentFile::createDirectory(const QString &displayName)
{
    if (isDirectory()) {
        return std::make_shared<MakeableDocumentFile>(
                    createDocument(m_uri, Document::MIME_TYPE_DIR, displayName),
                    shared_from_this());
    }
    return {};
}

const QJniObject &DocumentFile::uri() const
{
    return m_uri;
}

const DocumentFilePtr &DocumentFile::parent() const
{
    return m_parent;
}

QString DocumentFile::name() const
{
    return Cursor::queryColumn(m_uri, Document::COLUMN_DISPLAY_NAME).toString();
}

QString DocumentFile::id() const
{
    return DocumentsContract::documentId(uri());
}

QString DocumentFile::mimeType() const
{
    return Cursor::queryColumn(m_uri, Document::COLUMN_MIME_TYPE).toString();
}

bool DocumentFile::isDirectory() const
{
    return mimeType() == Document::MIME_TYPE_DIR;
}

bool DocumentFile::isFile() const
{
    const QString type = mimeType();
    return  type != Document::MIME_TYPE_DIR && !type.isEmpty();
}

bool DocumentFile::isVirtual() const
{
    return isDocumentUri(m_uri) && (Cursor::queryColumn(m_uri,
                                Document::COLUMN_FLAGS).toInt() & Document::FLAG_VIRTUAL_DOCUMENT);
}

QDateTime DocumentFile::lastModified() const
{
    const auto timeVariant = Cursor::queryColumn(m_uri, Document::COLUMN_LAST_MODIFIED);
    if (timeVariant.isValid())
        return QDateTime::fromMSecsSinceEpoch(timeVariant.toLongLong());
    return {};
}

int64_t DocumentFile::length() const
{
    return Cursor::queryColumn(m_uri, Document::COLUMN_SIZE).toLongLong();
}

namespace {
constexpr int FLAG_GRANT_READ_URI_PERMISSION = 0x00000001;
constexpr int FLAG_GRANT_WRITE_URI_PERMISSION = 0x00000002;
}

bool DocumentFile::canRead() const
{
    const auto context = QJniObject(QNativeInterface::QAndroidApplication::context());
    const bool selfUriPermission = context.callMethod<jint>("checkCallingOrSelfUriPermission",
                                                            m_uri.object<QtJniTypes::UriType>(),
                                                            FLAG_GRANT_READ_URI_PERMISSION);
    if (selfUriPermission != 0)
        return false;

    return !mimeType().isEmpty();
}

bool DocumentFile::canWrite() const
{
    const auto context = QJniObject(QNativeInterface::QAndroidApplication::context());
    const bool selfUriPermission = context.callMethod<jint>("checkCallingOrSelfUriPermission",
                                                            m_uri.object<QtJniTypes::UriType>(),
                                                            FLAG_GRANT_WRITE_URI_PERMISSION);
    if (selfUriPermission != 0)
        return false;

    const QString type = mimeType();
    if (type.isEmpty())
        return false;

    const int flags = Cursor::queryColumn(m_uri, Document::COLUMN_FLAGS).toInt();
    if (flags & Document::FLAG_SUPPORTS_DELETE)
        return true;

    const bool supportsWrite = (flags & Document::FLAG_SUPPORTS_WRITE);
    const bool isDir = (type == Document::MIME_TYPE_DIR);
    const bool dirSupportsCreate = (isDir && (flags & Document::FLAG_DIR_SUPPORTS_CREATE));

    return  dirSupportsCreate || supportsWrite;
}

bool DocumentFile::remove()
{
    return deleteDocument(m_uri);
}

bool DocumentFile::exists() const
{
    return !name().isEmpty();
}

std::vector<DocumentFilePtr> DocumentFile::listFiles()
{
    std::vector<DocumentFilePtr> res;
    const auto childrenUri = buildChildDocumentsUriUsingTree(m_uri, documentId(m_uri));
    const auto query = Cursor::queryUri(childrenUri, {Document::COLUMN_DOCUMENT_ID});
    if (!query)
        return res;

    while (query->moveToNext()) {
        const auto uri = buildDocumentUriUsingTree(m_uri, query->data(0).toString());
        res.push_back(std::make_shared<MakeableDocumentFile>(uri, shared_from_this()));
    }
    return res;
}

bool DocumentFile::rename(const QString &newName)
{
    QJniObject uri;
    if (newName.startsWith("content://"_L1)) {
        auto lastSeparatorIndex = [](const QString &file) {
            int posDecoded = file.lastIndexOf("/");
            int posEncoded = file.lastIndexOf(QUrl::toPercentEncoding("/"));
            return posEncoded > posDecoded ? posEncoded : posDecoded;
        };

        // first try to see if the new file is under the same tree and thus used rename only
        const QString parent = m_uri.toString().left(lastSeparatorIndex(m_uri.toString()));
        if (newName.contains(parent)) {
            QString displayName = newName.mid(lastSeparatorIndex(newName));
            if (displayName.startsWith('/'))
                displayName.remove(0, 1);
            else if (displayName.startsWith(QUrl::toPercentEncoding("/")))
                displayName.remove(0, 3);

            uri = renameDocument(m_uri, displayName);
        } else {
            // Move
            QJniObject srcParentUri = fromTreeUri(parseUri(parent))->uri();
            const QString destParent = newName.left(lastSeparatorIndex(newName));
            QJniObject targetParentUri = fromTreeUri(parseUri(destParent))->uri();
            uri = moveDocument(m_uri, srcParentUri, targetParentUri);
        }
    } else {
        uri = renameDocument(m_uri, newName);
    }

    if (uri.isValid()) {
        m_uri = uri;
        return true;
    }

    return false;
}

QT_END_NAMESPACE

// End of DocumentFile
