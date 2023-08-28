// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (C) 2018 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// Copyright (C) 2019 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmimeprovider_p.h"

#include "qmimetypeparser_p.h"
#include <qstandardpaths.h>
#include "qmimemagicrulematcher_p.h"

#include <QMap>
#include <QXmlStreamReader>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QByteArrayMatcher>
#include <QDebug>
#include <QDateTime>
#include <QtEndian>

#if QT_CONFIG(mimetype_database)
#  if defined(Q_CC_MSVC_ONLY)
#    pragma section(".qtmimedatabase", read, shared)
__declspec(allocate(".qtmimedatabase")) __declspec(align(4096))
#  elif defined(Q_OS_DARWIN)
__attribute__((section("__TEXT,.qtmimedatabase"), aligned(4096)))
#  elif (defined(Q_OF_ELF) || defined(Q_OS_WIN)) && defined(Q_CC_GNU)
__attribute__((section(".qtmimedatabase"), aligned(4096)))
#  endif

#  include "qmimeprovider_database.cpp"

#  ifdef MIME_DATABASE_IS_ZSTD
#    if !QT_CONFIG(zstd)
#      error "MIME database is zstd but no support compiled in!"
#    endif
#    include <zstd.h>
#  endif
#  ifdef MIME_DATABASE_IS_GZIP
#    ifdef QT_NO_COMPRESS
#      error "MIME database is zlib but no support compiled in!"
#    endif
#    define ZLIB_CONST
#    include <zconf.h>
#    include <zlib.h>
#  endif
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static inline void appendIfNew(QStringList &list, const QString &str)
{
    if (!list.contains(str))
        list.push_back(str);
}

QMimeProviderBase::QMimeProviderBase(QMimeDatabasePrivate *db, const QString &directory)
    : m_db(db), m_directory(directory)
{
}


QMimeBinaryProvider::QMimeBinaryProvider(QMimeDatabasePrivate *db, const QString &directory)
    : QMimeProviderBase(db, directory), m_mimetypeListLoaded(false)
{
    ensureLoaded();
}

struct QMimeBinaryProvider::CacheFile
{
    CacheFile(const QString &fileName);
    ~CacheFile();

    bool isValid() const { return m_valid; }
    inline quint16 getUint16(int offset) const
    {
        return qFromBigEndian(*reinterpret_cast<quint16 *>(data + offset));
    }
    inline quint32 getUint32(int offset) const
    {
        return qFromBigEndian(*reinterpret_cast<quint32 *>(data + offset));
    }
    inline const char *getCharStar(int offset) const
    {
        return reinterpret_cast<const char *>(data + offset);
    }
    bool load();
    bool reload();

    QFile file;
    uchar *data;
    QDateTime m_mtime;
    bool m_valid;
};

QMimeBinaryProvider::CacheFile::CacheFile(const QString &fileName)
    : file(fileName), m_valid(false)
{
    load();
}

QMimeBinaryProvider::CacheFile::~CacheFile()
{
}

bool QMimeBinaryProvider::CacheFile::load()
{
    if (!file.open(QIODevice::ReadOnly))
        return false;
    data = file.map(0, file.size());
    if (data) {
        const int major = getUint16(0);
        const int minor = getUint16(2);
        m_valid = (major == 1 && minor >= 1 && minor <= 2);
    }
    m_mtime = QFileInfo(file).lastModified(QTimeZone::UTC);
    return m_valid;
}

bool QMimeBinaryProvider::CacheFile::reload()
{
    m_valid = false;
    if (file.isOpen()) {
        file.close();
    }
    data = nullptr;
    return load();
}

QMimeBinaryProvider::~QMimeBinaryProvider() = default;

bool QMimeBinaryProvider::isValid()
{
    return m_cacheFile != nullptr;
}

bool QMimeBinaryProvider::isInternalDatabase() const
{
    return false;
}

// Position of the "list offsets" values, at the beginning of the mime.cache file
enum {
    PosAliasListOffset = 4,
    PosParentListOffset = 8,
    PosLiteralListOffset = 12,
    PosReverseSuffixTreeOffset = 16,
    PosGlobListOffset = 20,
    PosMagicListOffset = 24,
    // PosNamespaceListOffset = 28,
    PosIconsListOffset = 32,
    PosGenericIconsListOffset = 36
};

bool QMimeBinaryProvider::checkCacheChanged()
{
    QFileInfo fileInfo(m_cacheFile->file);
    if (fileInfo.lastModified(QTimeZone::UTC) > m_cacheFile->m_mtime) {
        // Deletion can't happen by just running update-mime-database.
        // But the user could use rm -rf :-)
        m_cacheFile->reload(); // will mark itself as invalid on failure
        return true;
    }
    return false;
}

void QMimeBinaryProvider::ensureLoaded()
{
    if (!m_cacheFile) {
        const QString cacheFileName = m_directory + "/mime.cache"_L1;
        m_cacheFile = std::make_unique<CacheFile>(cacheFileName);
        m_mimetypeListLoaded = false;
        m_mimetypeExtra.clear();
    } else {
        if (checkCacheChanged()) {
            m_mimetypeListLoaded = false;
            m_mimetypeExtra.clear();
        } else {
            return; // nothing to do
        }
    }
    if (!m_cacheFile->isValid()) // verify existence and version
        m_cacheFile.reset();
}

static QMimeType mimeTypeForNameUnchecked(const QString &name)
{
    QMimeTypePrivate data;
    data.name = name;
    data.fromCache = true;
    // The rest is retrieved on demand.
    // comment and globPatterns: in loadMimeTypePrivate
    // iconName: in loadIcon
    // genericIconName: in loadGenericIcon
    return QMimeType(data);
}

QMimeType QMimeBinaryProvider::mimeTypeForName(const QString &name)
{
    if (!m_mimetypeListLoaded)
        loadMimeTypeList();
    if (!m_mimetypeNames.contains(name))
        return QMimeType(); // unknown mimetype
    return mimeTypeForNameUnchecked(name);
}

void QMimeBinaryProvider::addFileNameMatches(const QString &fileName, QMimeGlobMatchResult &result)
{
    if (fileName.isEmpty())
        return;
    Q_ASSERT(m_cacheFile);
    const QString lowerFileName = fileName.toLower();
    int numMatches = 0;
    // Check literals (e.g. "Makefile")
    numMatches = matchGlobList(result, m_cacheFile.get(),
                               m_cacheFile->getUint32(PosLiteralListOffset), fileName);
    // Check the very common *.txt cases with the suffix tree
    if (numMatches == 0) {
        const int reverseSuffixTreeOffset = m_cacheFile->getUint32(PosReverseSuffixTreeOffset);
        const int numRoots = m_cacheFile->getUint32(reverseSuffixTreeOffset);
        const int firstRootOffset = m_cacheFile->getUint32(reverseSuffixTreeOffset + 4);
        if (matchSuffixTree(result, m_cacheFile.get(), numRoots, firstRootOffset, lowerFileName,
                            lowerFileName.size() - 1, false)) {
            ++numMatches;
        } else if (matchSuffixTree(result, m_cacheFile.get(), numRoots, firstRootOffset, fileName,
                                   fileName.size() - 1, true)) {
            ++numMatches;
        }
    }
    // Check complex globs (e.g. "callgrind.out[0-9]*" or "README*")
    if (numMatches == 0)
        matchGlobList(result, m_cacheFile.get(), m_cacheFile->getUint32(PosGlobListOffset),
                      fileName);
}

bool QMimeBinaryProvider::isMimeTypeGlobsExcluded(const char *mimeTypeName)
{
    return m_mimeTypesWithExcludedGlobs.contains(QLatin1StringView(mimeTypeName));
}

void QMimeBinaryProvider::excludeMimeTypeGlobs(const QStringList &toExclude)
{
    for (const auto &mt : toExclude)
        appendIfNew(m_mimeTypesWithExcludedGlobs, mt);
}

int QMimeBinaryProvider::matchGlobList(QMimeGlobMatchResult &result, CacheFile *cacheFile, int off,
                                       const QString &fileName)
{
    int numMatches = 0;
    const int numGlobs = cacheFile->getUint32(off);
    //qDebug() << "Loading" << numGlobs << "globs from" << cacheFile->file.fileName() << "at offset" << cacheFile->globListOffset;
    for (int i = 0; i < numGlobs; ++i) {
        const int globOffset = cacheFile->getUint32(off + 4 + 12 * i);
        const int mimeTypeOffset = cacheFile->getUint32(off + 4 + 12 * i + 4);
        const int flagsAndWeight = cacheFile->getUint32(off + 4 + 12 * i + 8);
        const int weight = flagsAndWeight & 0xff;
        const bool caseSensitive = flagsAndWeight & 0x100;
        const Qt::CaseSensitivity qtCaseSensitive = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        const QString pattern = QLatin1StringView(cacheFile->getCharStar(globOffset));

        const char *mimeType = cacheFile->getCharStar(mimeTypeOffset);
        //qDebug() << pattern << mimeType << weight << caseSensitive;
        if (isMimeTypeGlobsExcluded(mimeType))
            continue;

        QMimeGlobPattern glob(pattern, QString() /*unused*/, weight, qtCaseSensitive);
        if (glob.matchFileName(fileName)) {
            result.addMatch(QLatin1StringView(mimeType), weight, pattern);
            ++numMatches;
        }
    }
    return numMatches;
}

bool QMimeBinaryProvider::matchSuffixTree(QMimeGlobMatchResult &result,
                                          QMimeBinaryProvider::CacheFile *cacheFile, int numEntries,
                                          int firstOffset, const QString &fileName,
                                          qsizetype charPos, bool caseSensitiveCheck)
{
    QChar fileChar = fileName[charPos];
    int min = 0;
    int max = numEntries - 1;
    while (min <= max) {
        const int mid = (min + max) / 2;
        const int off = firstOffset + 12 * mid;
        const QChar ch = char16_t(cacheFile->getUint32(off));
        if (ch < fileChar)
            min = mid + 1;
        else if (ch > fileChar)
            max = mid - 1;
        else {
            --charPos;
            int numChildren = cacheFile->getUint32(off + 4);
            int childrenOffset = cacheFile->getUint32(off + 8);
            bool success = false;
            if (charPos > 0)
                success = matchSuffixTree(result, cacheFile, numChildren, childrenOffset, fileName, charPos, caseSensitiveCheck);
            if (!success) {
                for (int i = 0; i < numChildren; ++i) {
                    const int childOff = childrenOffset + 12 * i;
                    const int mch = cacheFile->getUint32(childOff);
                    if (mch != 0)
                        break;
                    const int mimeTypeOffset = cacheFile->getUint32(childOff + 4);
                    const char *mimeType = cacheFile->getCharStar(mimeTypeOffset);
                    if (isMimeTypeGlobsExcluded(mimeType))
                        continue;
                    const int flagsAndWeight = cacheFile->getUint32(childOff + 8);
                    const int weight = flagsAndWeight & 0xff;
                    const bool caseSensitive = flagsAndWeight & 0x100;
                    if (caseSensitiveCheck || !caseSensitive) {
                        result.addMatch(QLatin1StringView(mimeType), weight,
                                        u'*' + QStringView{fileName}.mid(charPos + 1),
                                        fileName.size() - charPos - 2);
                        success = true;
                    }
                }
            }
            return success;
        }
    }
    return false;
}

bool QMimeBinaryProvider::matchMagicRule(QMimeBinaryProvider::CacheFile *cacheFile, int numMatchlets, int firstOffset, const QByteArray &data)
{
    const char *dataPtr = data.constData();
    const qsizetype dataSize = data.size();
    for (int matchlet = 0; matchlet < numMatchlets; ++matchlet) {
        const int off = firstOffset + matchlet * 32;
        const int rangeStart = cacheFile->getUint32(off);
        const int rangeLength = cacheFile->getUint32(off + 4);
        //const int wordSize = cacheFile->getUint32(off + 8);
        const int valueLength = cacheFile->getUint32(off + 12);
        const int valueOffset = cacheFile->getUint32(off + 16);
        const int maskOffset = cacheFile->getUint32(off + 20);
        const char *mask = maskOffset ? cacheFile->getCharStar(maskOffset) : nullptr;

        if (!QMimeMagicRule::matchSubstring(dataPtr, dataSize, rangeStart, rangeLength, valueLength, cacheFile->getCharStar(valueOffset), mask))
            continue;

        const int numChildren = cacheFile->getUint32(off + 24);
        const int firstChildOffset = cacheFile->getUint32(off + 28);
        if (numChildren == 0) // No submatch? Then we are done.
            return true;
        // Check that one of the submatches matches too
        if (matchMagicRule(cacheFile, numChildren, firstChildOffset, data))
            return true;
    }
    return false;
}

void QMimeBinaryProvider::findByMagic(const QByteArray &data, int *accuracyPtr, QMimeType &candidate)
{
    const int magicListOffset = m_cacheFile->getUint32(PosMagicListOffset);
    const int numMatches = m_cacheFile->getUint32(magicListOffset);
    //const int maxExtent = cacheFile->getUint32(magicListOffset + 4);
    const int firstMatchOffset = m_cacheFile->getUint32(magicListOffset + 8);

    for (int i = 0; i < numMatches; ++i) {
        const int off = firstMatchOffset + i * 16;
        const int numMatchlets = m_cacheFile->getUint32(off + 8);
        const int firstMatchletOffset = m_cacheFile->getUint32(off + 12);
        if (matchMagicRule(m_cacheFile.get(), numMatchlets, firstMatchletOffset, data)) {
            const int mimeTypeOffset = m_cacheFile->getUint32(off + 4);
            const char *mimeType = m_cacheFile->getCharStar(mimeTypeOffset);
            *accuracyPtr = m_cacheFile->getUint32(off);
            // Return the first match. We have no rules for conflicting magic data...
            // (mime.cache itself is sorted, but what about local overrides with a lower prio?)
            candidate = mimeTypeForNameUnchecked(QLatin1StringView(mimeType));
            return;
        }
    }
}

void QMimeBinaryProvider::addParents(const QString &mime, QStringList &result)
{
    const QByteArray mimeStr = mime.toLatin1();
    const int parentListOffset = m_cacheFile->getUint32(PosParentListOffset);
    const int numEntries = m_cacheFile->getUint32(parentListOffset);

    int begin = 0;
    int end = numEntries - 1;
    while (begin <= end) {
        const int medium = (begin + end) / 2;
        const int off = parentListOffset + 4 + 8 * medium;
        const int mimeOffset = m_cacheFile->getUint32(off);
        const char *aMime = m_cacheFile->getCharStar(mimeOffset);
        const int cmp = qstrcmp(aMime, mimeStr);
        if (cmp < 0) {
            begin = medium + 1;
        } else if (cmp > 0) {
            end = medium - 1;
        } else {
            const int parentsOffset = m_cacheFile->getUint32(off + 4);
            const int numParents = m_cacheFile->getUint32(parentsOffset);
            for (int i = 0; i < numParents; ++i) {
                const int parentOffset = m_cacheFile->getUint32(parentsOffset + 4 + 4 * i);
                const char *aParent = m_cacheFile->getCharStar(parentOffset);
                const QString strParent = QString::fromLatin1(aParent);
                appendIfNew(result, strParent);
            }
            break;
        }
    }
}

QString QMimeBinaryProvider::resolveAlias(const QString &name)
{
    const QByteArray input = name.toLatin1();
    const int aliasListOffset = m_cacheFile->getUint32(PosAliasListOffset);
    const int numEntries = m_cacheFile->getUint32(aliasListOffset);
    int begin = 0;
    int end = numEntries - 1;
    while (begin <= end) {
        const int medium = (begin + end) / 2;
        const int off = aliasListOffset + 4 + 8 * medium;
        const int aliasOffset = m_cacheFile->getUint32(off);
        const char *alias = m_cacheFile->getCharStar(aliasOffset);
        const int cmp = qstrcmp(alias, input);
        if (cmp < 0) {
            begin = medium + 1;
        } else if (cmp > 0) {
            end = medium - 1;
        } else {
            const int mimeOffset = m_cacheFile->getUint32(off + 4);
            const char *mimeType = m_cacheFile->getCharStar(mimeOffset);
            return QLatin1StringView(mimeType);
        }
    }
    return QString();
}

void QMimeBinaryProvider::addAliases(const QString &name, QStringList &result)
{
    const QByteArray input = name.toLatin1();
    const int aliasListOffset = m_cacheFile->getUint32(PosAliasListOffset);
    const int numEntries = m_cacheFile->getUint32(aliasListOffset);
    for (int pos = 0; pos < numEntries; ++pos) {
        const int off = aliasListOffset + 4 + 8 * pos;
        const int mimeOffset = m_cacheFile->getUint32(off + 4);
        const char *mimeType = m_cacheFile->getCharStar(mimeOffset);

        if (input == mimeType) {
            const int aliasOffset = m_cacheFile->getUint32(off);
            const char *alias = m_cacheFile->getCharStar(aliasOffset);
            const QString strAlias = QString::fromLatin1(alias);
            appendIfNew(result, strAlias);
        }
    }
}

void QMimeBinaryProvider::loadMimeTypeList()
{
    if (!m_mimetypeListLoaded) {
        m_mimetypeListLoaded = true;
        m_mimetypeNames.clear();
        // Unfortunately mime.cache doesn't have a full list of all mimetypes.
        // So we have to parse the plain-text files called "types".
        QFile file(m_directory + QStringLiteral("/types"));
        if (file.open(QIODevice::ReadOnly)) {
            while (!file.atEnd()) {
                QByteArray line = file.readLine();
                if (line.endsWith('\n'))
                    line.chop(1);
                m_mimetypeNames.insert(QString::fromLatin1(line));
            }
        }
    }
}

void QMimeBinaryProvider::addAllMimeTypes(QList<QMimeType> &result)
{
    loadMimeTypeList();
    if (result.isEmpty()) {
        result.reserve(m_mimetypeNames.size());
        for (const QString &name : std::as_const(m_mimetypeNames))
            result.append(mimeTypeForNameUnchecked(name));
    } else {
        for (const QString &name : std::as_const(m_mimetypeNames))
            if (std::find_if(result.constBegin(), result.constEnd(), [name](const QMimeType &mime) -> bool { return mime.name() == name; })
                    == result.constEnd())
                result.append(mimeTypeForNameUnchecked(name));
    }
}

bool QMimeBinaryProvider::loadMimeTypePrivate(QMimeTypePrivate &data)
{
#if QT_CONFIG(xmlstreamreader)
    if (data.loaded)
        return true;

    auto it = m_mimetypeExtra.constFind(data.name);
    if (it == m_mimetypeExtra.constEnd()) {
        // load comment and globPatterns

        // shared-mime-info since 1.3 lowercases the xml files
        QString mimeFile = m_directory + u'/' + data.name.toLower() + ".xml"_L1;
        if (!QFile::exists(mimeFile))
            mimeFile = m_directory + u'/' + data.name + ".xml"_L1; // pre-1.3

        QFile qfile(mimeFile);
        if (!qfile.open(QFile::ReadOnly))
            return false;

        auto insertIt = m_mimetypeExtra.insert(data.name, MimeTypeExtra{});
        it = insertIt;
        MimeTypeExtra &extra = insertIt.value();
        QString mainPattern;

        QXmlStreamReader xml(&qfile);
        if (xml.readNextStartElement()) {
            if (xml.name() != "mime-type"_L1) {
                return false;
            }
            const auto name = xml.attributes().value("type"_L1);
            if (name.isEmpty())
                return false;
            if (name.compare(data.name, Qt::CaseInsensitive))
                qWarning() << "Got name" << name << "in file" << mimeFile << "expected" << data.name;

            while (xml.readNextStartElement()) {
                const auto tag = xml.name();
                if (tag == "comment"_L1) {
                    QString lang = xml.attributes().value("xml:lang"_L1).toString();
                    const QString text = xml.readElementText();
                    if (lang.isEmpty()) {
                        lang = "default"_L1; // no locale attribute provided, treat it as default.
                    }
                    extra.localeComments.insert(lang, text);
                    continue; // we called readElementText, so we're at the EndElement already.
                } else if (tag == "glob-deleteall"_L1) { // as written out by shared-mime-info >= 0.70
                    extra.globPatterns.clear();
                    mainPattern.clear();
                } else if (tag == "glob"_L1) { // as written out by shared-mime-info >= 0.70
                    const QString pattern = xml.attributes().value("pattern"_L1).toString();
                    if (mainPattern.isEmpty() && pattern.startsWith(u'*')) {
                        mainPattern = pattern;
                    }
                    appendIfNew(extra.globPatterns, pattern);
                }
                xml.skipCurrentElement();
            }
            Q_ASSERT(xml.name() == "mime-type"_L1);
        }

        // Let's assume that shared-mime-info is at least version 0.70
        // Otherwise we would need 1) a version check, and 2) code for parsing patterns from the globs file.
        if (!mainPattern.isEmpty() &&
                (extra.globPatterns.isEmpty() || extra.globPatterns.constFirst() != mainPattern)) {
            // ensure it's first in the list of patterns
            extra.globPatterns.removeAll(mainPattern);
            extra.globPatterns.prepend(mainPattern);
        }
    }
    const MimeTypeExtra &e = it.value();
    data.localeComments = e.localeComments;
    data.globPatterns = e.globPatterns;
    return true;
#else
    Q_UNUSED(data);
    qWarning("Cannot load mime type since QXmlStreamReader is not available.");
    return false;
#endif // feature xmlstreamreader
}

// Binary search in the icons or generic-icons list
QLatin1StringView QMimeBinaryProvider::iconForMime(CacheFile *cacheFile, int posListOffset,
                                                   const QByteArray &inputMime)
{
    const int iconsListOffset = cacheFile->getUint32(posListOffset);
    const int numIcons = cacheFile->getUint32(iconsListOffset);
    int begin = 0;
    int end = numIcons - 1;
    while (begin <= end) {
        const int medium = (begin + end) / 2;
        const int off = iconsListOffset + 4 + 8 * medium;
        const int mimeOffset = cacheFile->getUint32(off);
        const char *mime = cacheFile->getCharStar(mimeOffset);
        const int cmp = qstrcmp(mime, inputMime);
        if (cmp < 0)
            begin = medium + 1;
        else if (cmp > 0)
            end = medium - 1;
        else {
            const int iconOffset = cacheFile->getUint32(off + 4);
            return QLatin1StringView(cacheFile->getCharStar(iconOffset));
        }
    }
    return QLatin1StringView();
}

void QMimeBinaryProvider::loadIcon(QMimeTypePrivate &data)
{
    const QByteArray inputMime = data.name.toLatin1();
    const QLatin1StringView icon = iconForMime(m_cacheFile.get(), PosIconsListOffset, inputMime);
    if (!icon.isEmpty()) {
        data.iconName = icon;
    }
}

void QMimeBinaryProvider::loadGenericIcon(QMimeTypePrivate &data)
{
    const QByteArray inputMime = data.name.toLatin1();
    const QLatin1StringView icon = iconForMime(m_cacheFile.get(), PosGenericIconsListOffset, inputMime);
    if (!icon.isEmpty()) {
        data.genericIconName = icon;
    }
}

////

#if QT_CONFIG(mimetype_database)
static QString internalMimeFileName()
{
    return QStringLiteral("<internal MIME data>");
}

QMimeXMLProvider::QMimeXMLProvider(QMimeDatabasePrivate *db, InternalDatabaseEnum)
    : QMimeProviderBase(db, internalMimeFileName())
{
    static_assert(sizeof(mimetype_database), "Bundled MIME database is empty");
    static_assert(sizeof(mimetype_database) <= MimeTypeDatabaseOriginalSize,
                      "Compressed MIME database is larger than the original size");
    static_assert(MimeTypeDatabaseOriginalSize <= 16*1024*1024,
                      "Bundled MIME database is too big");
    const char *data = reinterpret_cast<const char *>(mimetype_database);
    qsizetype size = MimeTypeDatabaseOriginalSize;

#ifdef MIME_DATABASE_IS_ZSTD
    // uncompress with libzstd
    std::unique_ptr<char []> uncompressed(new char[size]);
    size = ZSTD_decompress(uncompressed.get(), size, mimetype_database, sizeof(mimetype_database));
    Q_ASSERT(!ZSTD_isError(size));
    data = uncompressed.get();
#elif defined(MIME_DATABASE_IS_GZIP)
    std::unique_ptr<char []> uncompressed(new char[size]);
    z_stream zs = {};
    zs.next_in = const_cast<Bytef *>(mimetype_database);
    zs.avail_in = sizeof(mimetype_database);
    zs.next_out = reinterpret_cast<Bytef *>(uncompressed.get());
    zs.avail_out = size;

    int res = inflateInit2(&zs, MAX_WBITS | 32);
    Q_ASSERT(res == Z_OK);
    res = inflate(&zs, Z_FINISH);
    Q_ASSERT(res == Z_STREAM_END);
    res = inflateEnd(&zs);
    Q_ASSERT(res == Z_OK);

    data = uncompressed.get();
    size = zs.total_out;
#endif

    load(data, size);
}
#else // !QT_CONFIG(mimetype_database)
// never called in release mode, but some debug builds may need
// this to be defined.
QMimeXMLProvider::QMimeXMLProvider(QMimeDatabasePrivate *db, InternalDatabaseEnum)
    : QMimeProviderBase(db, QString())
{
    Q_UNREACHABLE();
}
#endif // QT_CONFIG(mimetype_database)

QMimeXMLProvider::QMimeXMLProvider(QMimeDatabasePrivate *db, const QString &directory)
    : QMimeProviderBase(db, directory)
{
    ensureLoaded();
}

QMimeXMLProvider::~QMimeXMLProvider()
{
}

bool QMimeXMLProvider::isValid()
{
    // If you change this method, adjust the logic in QMimeDatabasePrivate::loadProviders,
    // which assumes isValid==false is only possible in QMimeBinaryProvider.
    return true;
}

bool QMimeXMLProvider::isInternalDatabase() const
{
#if QT_CONFIG(mimetype_database)
    return m_directory == internalMimeFileName();
#else
    return false;
#endif
}

QMimeType QMimeXMLProvider::mimeTypeForName(const QString &name)
{
    return m_nameMimeTypeMap.value(name);
}

void QMimeXMLProvider::addFileNameMatches(const QString &fileName, QMimeGlobMatchResult &result)
{
    m_mimeTypeGlobs.matchingGlobs(fileName, result);
}

void QMimeXMLProvider::findByMagic(const QByteArray &data, int *accuracyPtr, QMimeType &candidate)
{
    QString candidateName;
    bool foundOne = false;
    for (const QMimeMagicRuleMatcher &matcher : std::as_const(m_magicMatchers)) {
        if (matcher.matches(data)) {
            const int priority = matcher.priority();
            if (priority > *accuracyPtr) {
                *accuracyPtr = priority;
                candidateName = matcher.mimetype();
                foundOne = true;
            }
        }
    }
    if (foundOne)
        candidate = mimeTypeForName(candidateName);
}

void QMimeXMLProvider::ensureLoaded()
{
    QStringList allFiles;
    const QString packageDir = m_directory + QStringLiteral("/packages");
    QDir dir(packageDir);
    const QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    allFiles.reserve(files.size());
    for (const QString &xmlFile : files)
        allFiles.append(packageDir + u'/' + xmlFile);

    if (m_allFiles == allFiles)
        return;
    m_allFiles = allFiles;

    m_nameMimeTypeMap.clear();
    m_aliases.clear();
    m_parents.clear();
    m_mimeTypeGlobs.clear();
    m_magicMatchers.clear();
    m_mimeTypesWithDeletedGlobs.clear();

    //qDebug() << "Loading" << m_allFiles;

    for (const QString &file : std::as_const(allFiles))
        load(file);
}

void QMimeXMLProvider::load(const QString &fileName)
{
    QString errorMessage;
    if (!load(fileName, &errorMessage))
        qWarning("QMimeDatabase: Error loading %ls\n%ls", qUtf16Printable(fileName), qUtf16Printable(errorMessage));
}

bool QMimeXMLProvider::load(const QString &fileName, QString *errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = "Cannot open "_L1 + fileName + ": "_L1 + file.errorString();
        return false;
    }

    if (errorMessage)
        errorMessage->clear();

    QMimeTypeParser parser(*this);
    return parser.parse(&file, fileName, errorMessage);
}

#if QT_CONFIG(mimetype_database)
void QMimeXMLProvider::load(const char *data, qsizetype len)
{
    QBuffer buffer;
    buffer.setData(QByteArray::fromRawData(data, len));
    buffer.open(QIODevice::ReadOnly);
    QString errorMessage;
    QMimeTypeParser parser(*this);
    if (!parser.parse(&buffer, internalMimeFileName(), &errorMessage))
        qWarning("QMimeDatabase: Error loading internal MIME data\n%s", qPrintable(errorMessage));
}
#endif

void QMimeXMLProvider::addGlobPattern(const QMimeGlobPattern &glob)
{
    m_mimeTypeGlobs.addGlob(glob);
}

void QMimeXMLProvider::addMimeType(const QMimeType &mt)
{
    Q_ASSERT(!mt.d.data()->fromCache);

    QString name = mt.name();
    if (mt.d->hasGlobDeleteAll)
        appendIfNew(m_mimeTypesWithDeletedGlobs, name);
    m_nameMimeTypeMap.insert(mt.name(), mt);
}

/*
    \a toExclude is a list of mime type names that should have the the glob patterns
    associated with them cleared (because there are mime types with the same names
    in a higher precedence Provider that have glob-deleteall tags).

    This method is called from QMimeDatabasePrivate::loadProviders() to exclude mime
    type glob patterns in lower precedence Providers.
*/
void QMimeXMLProvider::excludeMimeTypeGlobs(const QStringList &toExclude)
{
    for (const auto &mt : toExclude) {
        auto it = m_nameMimeTypeMap.find(mt);
        if (it != m_nameMimeTypeMap.end())
            it->d->globPatterns.clear();
        m_mimeTypeGlobs.removeMimeType(mt);
    }
}

void QMimeXMLProvider::addParents(const QString &mime, QStringList &result)
{
    for (const QString &parent : m_parents.value(mime)) {
        if (!result.contains(parent))
            result.append(parent);
    }
}

void QMimeXMLProvider::addParent(const QString &child, const QString &parent)
{
    m_parents[child].append(parent);
}

void QMimeXMLProvider::addAliases(const QString &name, QStringList &result)
{
    // Iterate through the whole hash. This method is rarely used.
    for (const auto &[alias, mimeName] : std::as_const(m_aliases).asKeyValueRange()) {
        if (mimeName == name)
            appendIfNew(result, alias);
    }
}

QString QMimeXMLProvider::resolveAlias(const QString &name)
{
    return m_aliases.value(name);
}

void QMimeXMLProvider::addAlias(const QString &alias, const QString &name)
{
    m_aliases.insert(alias, name);
}

void QMimeXMLProvider::addAllMimeTypes(QList<QMimeType> &result)
{
    if (result.isEmpty()) { // fast path
        result = m_nameMimeTypeMap.values();
    } else {
        for (auto it = m_nameMimeTypeMap.constBegin(), end = m_nameMimeTypeMap.constEnd() ; it != end ; ++it) {
            const QString newMime = it.key();
            if (std::find_if(result.constBegin(), result.constEnd(), [newMime](const QMimeType &mime) -> bool { return mime.name() == newMime; })
                    == result.constEnd())
                result.append(it.value());
        }
    }
}

void QMimeXMLProvider::addMagicMatcher(const QMimeMagicRuleMatcher &matcher)
{
    m_magicMatchers.append(matcher);
}

QT_END_NAMESPACE
