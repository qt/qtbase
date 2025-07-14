// Copyright (C) 2019 Volker Krause <vkrause@kde.org>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "androidcontentfileengine.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmimedatabase.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface;
using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcAndroidContentFileEngine, "qt.qpa.contentfileengine")

Q_DECLARE_JNI_CLASS(ParcelFileDescriptorType, "android/os/ParcelFileDescriptor");
Q_DECLARE_JNI_CLASS(CursorType, "android/database/Cursor");
Q_DECLARE_JNI_CLASS(QtContentFileEngine, "org/qtproject/qt/android/QtContentFileEngine");
Q_DECLARE_JNI_CLASS(List, "java/util/List");

constexpr char contentScheme[] = "content";
constexpr char contentSchemeFull[] = "content://";
constexpr char treeSegment[] = "tree";
constexpr char documentSegment[] = "document";
constexpr char childrenSegment[] = "%2Fchildren";

static QJniObject &contentResolverInstance()
{
    static QJniObject contentResolver;
    if (!contentResolver.isValid()) {
        contentResolver = QJniObject(QNativeInterface::QAndroidApplication::context())
                .callMethod<QtJniTypes::ContentResolver>("getContentResolver");
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

    if (openMode == QIODeviceBase::NotOpen)
        return true;

    if (!m_documentFile->exists() && (openMode & QIODevice::WriteOnly)) {
        // Create the file if it doesn't exist yet
        DocumentFilePtr parent = m_documentFile->parent();
        if (!parent) {
            qCWarning(lcAndroidContentFileEngine) << "Cannot create file under a null parent.";
            return false;
        }

        if (!parent->exists() || !parent->isDirectory()) {
            qCWarning(lcAndroidContentFileEngine)
                << "Cannot create file, parent doesn't exist or not a directory:"
                << parent->uri().toString();
            return false;
        }

        const QString fileName = m_documentFile->initialName();
        if (fileName.isEmpty()) {
            qCWarning(lcAndroidContentFileEngine) << "Coudln't determine filename from content URI:"
                                                  << m_documentFile->uri().toString();
            return false;
        }

        QMimeDatabase db;
        QString mimeType = db.mimeTypeForFile(fileName, QMimeDatabase::MatchDefault).name();

        m_documentFile = parent->createFile(mimeType, fileName);
        if (!m_documentFile) {
            qCWarning(lcAndroidContentFileEngine) << "Failed to create new document under parent:"
                                                  << parent->uri().toString();
            return false;
        }
    }

    using namespace QtJniTypes;
    const QString openModeStr = (openMode & QIODevice::WriteOnly) ? "w"_L1 : "r"_L1;
    m_pfd = QtContentFileEngine::callStaticMethod<ParcelFileDescriptorType>(
                "openFileDescriptor",
                contentResolverInstance().object<ContentResolver>(),
                m_documentFile->uri().object<Uri>(),
                openModeStr);

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
        m_initialFile = newName;
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
            if (dir == subDir->initialName() && subDir->isDirectory()) {
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
    Q_UNUSED(recurseParentDirectories) // DocumentFile deletes recursively by default

    return DocumentFile::parseFromAnyUri(dirName)->remove();
}

QByteArray AndroidContentFileEngine::id() const
{
    return m_documentFile->id().toUtf8();
}

QDateTime AndroidContentFileEngine::fileTime(QFile::FileTime time) const
{
    switch (time) {
    case QFile::FileModificationTime:
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
        case BaseName: {
            const QString queriedName = m_documentFile->name();
            return queriedName.isEmpty() ? m_documentFile->initialName() : queriedName;
        }
        default:
            break;
    }

    return QString();
}

QAbstractFileEngine::IteratorUniquePtr
AndroidContentFileEngine::beginEntryList(const QString &path, QDirListing::IteratorFlags filters,
                                         const QStringList &filterNames)
{
    return std::make_unique<AndroidContentFileEngineIterator>(path, filters, filterNames);
}

AndroidContentFileEngineHandler::AndroidContentFileEngineHandler() = default;
AndroidContentFileEngineHandler::~AndroidContentFileEngineHandler() = default;

std::unique_ptr<QAbstractFileEngine>
AndroidContentFileEngineHandler::create(const QString &fileName) const
{
    if (fileName.startsWith(contentScheme))
        return std::make_unique<AndroidContentFileEngine>(fileName);

    return {};

}

AndroidContentFileEngineIterator::AndroidContentFileEngineIterator(
    const QString &path, QDirListing::IteratorFlags filters, const QStringList &filterNames)
    : QAbstractFileEngineIterator(path, filters, filterNames)
{
}

AndroidContentFileEngineIterator::~AndroidContentFileEngineIterator()
{
}

bool AndroidContentFileEngineIterator::advance()
{
    if (m_index == -1 && m_files.isEmpty()) {
        const auto currentPath = path();
        if (currentPath.isEmpty())
            return false;

        const auto iterDoc = DocumentFile::parseFromAnyUri(currentPath);
        if (iterDoc->isDirectory())
            for (const auto &doc : iterDoc->listFiles())
                m_files.append(doc);
        if (m_files.isEmpty())
            return false;
        m_index = 0;
        return true;
    }

    if (m_index < m_files.size() - 1) {
        ++m_index;
        return true;
    }

    return false;
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
        using namespace QtJniTypes;
        auto cursor = QtContentFileEngine::callStaticMethod<CursorType>("query",
                contentResolverInstance().object<ContentResolver>(),
                uri.object<Uri>(),
                QJniArray(projection),
                selection.isEmpty() ? nullptr : selection,
                QJniArray(selectionArgs),
                sortOrder.isEmpty() ? nullptr : sortOrder);
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
    return QJniObject::callStaticMethod<jstring, QtJniTypes::Uri>(
                QtJniTypes::Traits<QtJniTypes::DocumentsContract>::className(),
                "getDocumentId",
                uri.object()).toString();
}

QString treeDocumentId(const QJniObject &uri)
{
    return QJniObject::callStaticMethod<jstring, QtJniTypes::Uri>(
                QtJniTypes::Traits<QtJniTypes::DocumentsContract>::className(),
                "getTreeDocumentId",
                uri.object()).toString();
}

QJniObject buildChildDocumentsUriUsingTree(const QJniObject &uri, const QString &parentDocumentId)
{
    return QJniObject::callStaticMethod<QtJniTypes::Uri>(
                QtJniTypes::Traits<QtJniTypes::DocumentsContract>::className(),
                "buildChildDocumentsUriUsingTree",
                uri.object<QtJniTypes::Uri>(),
                QJniObject::fromString(parentDocumentId).object<jstring>());

}

QJniObject buildDocumentUriUsingTree(const QJniObject &treeUri, const QString &documentId)
{
    return QJniObject::callStaticMethod<QtJniTypes::Uri>(
                QtJniTypes::Traits<QtJniTypes::DocumentsContract>::className(),
                "buildDocumentUriUsingTree",
                treeUri.object<QtJniTypes::Uri>(),
                QJniObject::fromString(documentId).object<jstring>());
}

bool isDocumentUri(const QJniObject &uri)
{
    return QJniObject::callStaticMethod<jboolean>(
                QtJniTypes::Traits<QtJniTypes::DocumentsContract>::className(),
                "isDocumentUri",
                QNativeInterface::QAndroidApplication::context(),
                uri.object<QtJniTypes::Uri>());
}

bool isTreeUri(const QJniObject &uri)
{
    return QJniObject::callStaticMethod<jboolean>(
                QtJniTypes::Traits<QtJniTypes::DocumentsContract>::className(),
                "isTreeUri",
                uri.object<QtJniTypes::Uri>());
}

QJniObject createDocument(const QJniObject &parentDocumentUri, const QString &mimeType,
                          const QString &displayName)
{
    return QJniObject::callStaticMethod<QtJniTypes::Uri>(
                QtJniTypes::Traits<QtJniTypes::DocumentsContract>::className(),
                "createDocument",
                contentResolverInstance().object<QtJniTypes::ContentResolver>(),
                parentDocumentUri.object<QtJniTypes::Uri>(),
                QJniObject::fromString(mimeType).object<jstring>(),
                QJniObject::fromString(displayName).object<jstring>());
}

bool deleteDocument(const QJniObject &documentUri)
{
    const int flags = Cursor::queryColumn(documentUri, Document::COLUMN_FLAGS).toInt();
    if (!(flags & Document::FLAG_SUPPORTS_DELETE))
        return {};

    return QJniObject::callStaticMethod<jboolean>(
                QtJniTypes::Traits<QtJniTypes::DocumentsContract>::className(),
                "deleteDocument",
                contentResolverInstance().object<QtJniTypes::ContentResolver>(),
                documentUri.object<QtJniTypes::Uri>());
}

QJniObject moveDocument(const QJniObject &sourceDocumentUri,
                      const QJniObject &sourceParentDocumentUri,
                      const QJniObject &targetParentDocumentUri)
{
    const int flags = Cursor::queryColumn(sourceDocumentUri, Document::COLUMN_FLAGS).toInt();
    if (!(flags & Document::FLAG_SUPPORTS_MOVE))
        return {};

    return QJniObject::callStaticMethod<QtJniTypes::Uri>(
                QtJniTypes::Traits<QtJniTypes::DocumentsContract>::className(),
                "moveDocument",
                contentResolverInstance().object<QtJniTypes::ContentResolver>(),
                sourceDocumentUri.object<QtJniTypes::Uri>(),
                sourceParentDocumentUri.object<QtJniTypes::Uri>(),
                targetParentDocumentUri.object<QtJniTypes::Uri>());
}

QJniObject renameDocument(const QJniObject &documentUri, const QString &displayName)
{
    const int flags = Cursor::queryColumn(documentUri, Document::COLUMN_FLAGS).toInt();
    if (!(flags & Document::FLAG_SUPPORTS_RENAME))
        return {};

    return QJniObject::callStaticMethod<QtJniTypes::Uri>(
                QtJniTypes::Traits<QtJniTypes::DocumentsContract>::className(),
                "renameDocument",
                contentResolverInstance().object<QtJniTypes::ContentResolver>(),
                documentUri.object<QtJniTypes::Uri>(),
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
        : DocumentFile(uri, QString(), parent)
    {
        QString uriString = uri.toString();

        if (uriString.endsWith(childrenSegment)) {
            // A URI ending with /children is a query for accessing documents under
            // the parent tree, so the closest name would be that of the parent.
            uriString.chop(std::size(childrenSegment) - 1);
        }

        const QString path = QUrl(uriString).path();
        if (path.isEmpty() || path == u"/")
            return;

        int displayNameStartIndex = uriString.lastIndexOf(u'/') + 1;

        const int encodedSlashPos = uriString.lastIndexOf(u"%2F");
        if (encodedSlashPos != -1)
            displayNameStartIndex = qMax(displayNameStartIndex, encodedSlashPos + 3);

        const int encodedColonPos = uriString.lastIndexOf(u"%3A");
        if (encodedColonPos != -1)
            displayNameStartIndex = qMax(displayNameStartIndex, encodedColonPos + 3);

        m_displayName = uriString.mid(displayNameStartIndex);
    }

    MakeableDocumentFile(const QJniObject &uri, const QString &displayName, const DocumentFilePtr &parent = {})
        : DocumentFile(uri, displayName, parent)
    {}
};
}

DocumentFile::DocumentFile(const QJniObject &uri,
                           const QString &displayName,
                           const DocumentFilePtr &parent)
    : m_displayName{displayName},
      m_uri{uri},
      m_parent{parent}
{}

QStringList DocumentFile::getPathSegments(const QJniObject &uri)
{
    if (!uri.isValid())
        return {};

    const auto jSegments = uri.callMethod<QtJniTypes::List>("getPathSegments");
    if (!jSegments.isValid())
        return {};

    QStringList segments;
    for (int i = 0; i < jSegments.callMethod<jint>("size"); ++i)
        segments.append(jSegments.callMethod<QJniObject>("get", i).toString());

    return segments;
}

QJniObject parseUri(const QString &uri)
{
    return QJniObject::callStaticMethod<QtJniTypes::Uri>(
                QtJniTypes::Traits<QtJniTypes::Uri>::className(),
                "parse",
                QJniObject::fromString(uri).object<jstring>());
}

DocumentFilePtr DocumentFile::parseFromAnyUri(const QString &fileName)
{
    const QJniObject uri = parseUri(fileName);
    if (!uri.isValid() || uri.toString().isEmpty())
        return {};

    const auto scheme = uri.callMethod<QString>("getScheme");
    if (scheme != QLatin1String(contentScheme))
        return std::make_shared<MakeableDocumentFile>(uri);

    const auto authority = uri.callMethod<QString>("getAuthority");
    const QStringList segments = getPathSegments(uri);

    const int treeIndex = segments.indexOf(treeSegment);
    const int docIndex = segments.indexOf(documentSegment);

    DocumentFilePtr parent;
    QJniObject parsedUri;

    if (treeIndex != -1) {
        // the segment after "tree" is the tree ID, otherwise it's malformed uri
        if (segments.size() <= treeIndex + 1)
            return fromSingleUri(uri);

        const QString treeId = segments.at(treeIndex + 1);
        const QString encodedTreeId = QUrl::toPercentEncoding(treeId);
        const QString treeUriString = "%1://%2/%3/%4"_L1.arg(scheme, authority,
                                                             treeSegment, encodedTreeId);
        const QJniObject treeUri = parseUri(treeUriString);

        const int midIndex = (docIndex > treeIndex) ? docIndex + 1 : treeIndex + 2;
        QString docIdOrPath = segments.mid(midIndex).join('/');

        if (docIdOrPath.isEmpty())
            return fromTreeUri(treeUri);

        QString fullDocId;
        if (docIndex > treeIndex)
            fullDocId = docIdOrPath; // full ID
        else
            fullDocId = treeId + u'/' + docIdOrPath; // relative path

        parsedUri = buildDocumentUriUsingTree(treeUri, fullDocId);

        const int lastSlash = fullDocId.lastIndexOf('/');
        if (lastSlash != -1) {
            const QString parentDocId = fullDocId.left(lastSlash);
            // Parent must be within the same tree, or be the tree root
            if (!parentDocId.isEmpty() && parentDocId.startsWith(treeId)) {
                QJniObject parentUri = buildDocumentUriUsingTree(treeUri, parentDocId);
                parent = std::make_shared<MakeableDocumentFile>(parentUri);
            }
        }

        if (!parent)
            parent = fromTreeUri(treeUri);
    } else if (docIndex != -1) {
        const QString docId = segments.mid(docIndex + 1).join('/');
        const QString encodedDocId = QUrl::toPercentEncoding(docId);
        const QString docUriStr = "%1://%2/%3/%4"_L1.arg(scheme, authority,
                                                         documentSegment, encodedDocId);
        parsedUri = parseUri(docUriStr);

        const int lastSlash = docId.lastIndexOf('/');
        if (lastSlash != -1) {
            const QString parentDocId = docId.left(lastSlash);
            const QString encodedParentDocId = QUrl::toPercentEncoding(parentDocId);
            const QString parentUriStr = "%1://%2/%3/%4"_L1.arg(scheme, authority, documentSegment,
                                                                encodedParentDocId);
            parent = fromSingleUri(parseUri(parentUriStr));
        }
    } else {
        parsedUri = uri;
        if (segments.size() > 1) {
            QStringList parentSegments = segments;
            parentSegments.removeLast();
            const QString parendDocId = parentSegments.join('/');
            const QString parentUriStr = "%1://%2/%3"_L1.arg(scheme, authority, parendDocId);
            parent = fromSingleUri(parseUri(parentUriStr));
        }
    }

    if (!parsedUri.isValid())
        return fromSingleUri(uri);

    auto docFile = std::make_shared<MakeableDocumentFile>(parsedUri);
    if (parent)
        docFile->m_parent = parent;

    return docFile;
}

DocumentFilePtr DocumentFile::fromSingleUri(const QJniObject &uri)
{
    if (!uri.isValid())
        return {};

    return std::make_shared<MakeableDocumentFile>(uri);
}

DocumentFilePtr DocumentFile::fromTreeUri(const QJniObject &treeUri)
{
    if (!treeUri.isValid())
        return {};

    if (isDocumentUri(treeUri))
        return std::make_shared<MakeableDocumentFile>(treeUri);

    if (isTreeUri(treeUri)) {
        const QString docId = treeDocumentId(treeUri);
        if (!docId.isEmpty()) {
            const QJniObject docUri = buildDocumentUriUsingTree(treeUri, docId);
            return std::make_shared<MakeableDocumentFile>(docUri);
        }
    }

    return std::make_shared<MakeableDocumentFile>(treeUri);
}

DocumentFilePtr DocumentFile::createFile(const QString &mimeType, const QString &displayName)
{
    if (isDirectory()) {
        const QString decodedName = QUrl::fromPercentEncoding(displayName.toUtf8());
        return std::make_shared<MakeableDocumentFile>(
                    createDocument(m_uri, mimeType, decodedName),
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

QString DocumentFile::initialName() const
{
    return m_displayName;
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
                                                            m_uri.object<QtJniTypes::Uri>(),
                                                            FLAG_GRANT_READ_URI_PERMISSION);
    if (selfUriPermission != 0)
        return false;

    return !mimeType().isEmpty();
}

bool DocumentFile::canWrite() const
{
    const auto context = QJniObject(QNativeInterface::QAndroidApplication::context());
    const bool selfUriPermission = context.callMethod<jint>("checkCallingOrSelfUriPermission",
                                                            m_uri.object<QtJniTypes::Uri>(),
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
    if (!newName.startsWith(contentSchemeFull)) {
        // Simple rename
        QJniObject renamedUri = renameDocument(m_uri, newName);
        if (!renamedUri.isValid())
            return false;

        m_uri = renamedUri;
        m_displayName = newName;

        return true;
    }

    // Mixed move and rename
    auto destDoc = parseFromAnyUri(newName);
    if (!destDoc)
        return false;

    DocumentFilePtr destParent = destDoc->parent();
    const QString oldName = name();
    const QString destName = QUrl::fromPercentEncoding(QFileInfo(newName).fileName().toUtf8());

    bool isMove = false;
    if (parent() && destParent && parent()->uri().toString() != destParent->uri().toString())
        isMove = true;

    QJniObject currentUri = m_uri;
    if (isMove) {
        if (!parent()) // Cannot move if source parent is unknown
            return false;

        currentUri = moveDocument(m_uri, parent()->uri(), destParent->uri());
        if (!currentUri.isValid())
            return false;
    }

    if (oldName != destName) {
        QJniObject renamedUri = renameDocument(currentUri, destName);
        if (!renamedUri.isValid())
            return false;

        currentUri = renamedUri;
    }

    m_uri = currentUri;
    m_displayName = destName;
    if (isMove)
        m_parent = destParent;

    return true;
}

QT_END_NAMESPACE

// End of DocumentFile
