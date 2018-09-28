/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmimeprovider_p.h"

#include "qmimetypeparser_p.h"
#include <qstandardpaths.h>
#include "qmimemagicrulematcher_p.h"

#include <QXmlStreamReader>
#include <QDir>
#include <QFile>
#include <QByteArrayMatcher>
#include <QDebug>
#include <QDateTime>
#include <QtEndian>

static void initResources()
{
    Q_INIT_RESOURCE(mimetypes);
}

QT_BEGIN_NAMESPACE

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
    m_mtime = QFileInfo(file).lastModified();
    return m_valid;
}

bool QMimeBinaryProvider::CacheFile::reload()
{
    m_valid = false;
    if (file.isOpen()) {
        file.close();
    }
    data = 0;
    return load();
}

QMimeBinaryProvider::~QMimeBinaryProvider()
{
    delete m_cacheFile;
}

bool QMimeBinaryProvider::isValid()
{
    return m_cacheFile != nullptr;
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
    if (fileInfo.lastModified() > m_cacheFile->m_mtime) {
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
        const QString cacheFileName = m_directory + QLatin1String("/mime.cache");
        m_cacheFile = new CacheFile(cacheFileName);
        m_mimetypeListLoaded = false;
    } else {
        if (checkCacheChanged())
            m_mimetypeListLoaded = false;
        else
            return; // nothing to do
    }
    if (!m_cacheFile->isValid()) { // verify existence and version
        delete m_cacheFile;
        m_cacheFile = nullptr;
    }
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
    // Check literals (e.g. "Makefile")
    matchGlobList(result, m_cacheFile, m_cacheFile->getUint32(PosLiteralListOffset), fileName);
    // Check complex globs (e.g. "callgrind.out[0-9]*")
    matchGlobList(result, m_cacheFile, m_cacheFile->getUint32(PosGlobListOffset), fileName);
    // Check the very common *.txt cases with the suffix tree
    const int reverseSuffixTreeOffset = m_cacheFile->getUint32(PosReverseSuffixTreeOffset);
    const int numRoots = m_cacheFile->getUint32(reverseSuffixTreeOffset);
    const int firstRootOffset = m_cacheFile->getUint32(reverseSuffixTreeOffset + 4);
    matchSuffixTree(result, m_cacheFile, numRoots, firstRootOffset, lowerFileName, lowerFileName.length() - 1, false);
    if (result.m_matchingMimeTypes.isEmpty())
        matchSuffixTree(result, m_cacheFile, numRoots, firstRootOffset, fileName, fileName.length() - 1, true);
}

void QMimeBinaryProvider::matchGlobList(QMimeGlobMatchResult &result, CacheFile *cacheFile, int off, const QString &fileName)
{
    const int numGlobs = cacheFile->getUint32(off);
    //qDebug() << "Loading" << numGlobs << "globs from" << cacheFile->file.fileName() << "at offset" << cacheFile->globListOffset;
    for (int i = 0; i < numGlobs; ++i) {
        const int globOffset = cacheFile->getUint32(off + 4 + 12 * i);
        const int mimeTypeOffset = cacheFile->getUint32(off + 4 + 12 * i + 4);
        const int flagsAndWeight = cacheFile->getUint32(off + 4 + 12 * i + 8);
        const int weight = flagsAndWeight & 0xff;
        const bool caseSensitive = flagsAndWeight & 0x100;
        const Qt::CaseSensitivity qtCaseSensitive = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        const QString pattern = QLatin1String(cacheFile->getCharStar(globOffset));

        const char *mimeType = cacheFile->getCharStar(mimeTypeOffset);
        //qDebug() << pattern << mimeType << weight << caseSensitive;
        QMimeGlobPattern glob(pattern, QString() /*unused*/, weight, qtCaseSensitive);

        // TODO: this could be done faster for literals where a simple == would do.
        if (glob.matchFileName(fileName))
            result.addMatch(QLatin1String(mimeType), weight, pattern);
    }
}

bool QMimeBinaryProvider::matchSuffixTree(QMimeGlobMatchResult &result, QMimeBinaryProvider::CacheFile *cacheFile, int numEntries, int firstOffset, const QString &fileName, int charPos, bool caseSensitiveCheck)
{
    QChar fileChar = fileName[charPos];
    int min = 0;
    int max = numEntries - 1;
    while (min <= max) {
        const int mid = (min + max) / 2;
        const int off = firstOffset + 12 * mid;
        const QChar ch = cacheFile->getUint32(off);
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
                    const int flagsAndWeight = cacheFile->getUint32(childOff + 8);
                    const int weight = flagsAndWeight & 0xff;
                    const bool caseSensitive = flagsAndWeight & 0x100;
                    if (caseSensitiveCheck || !caseSensitive) {
                        result.addMatch(QLatin1String(mimeType), weight,
                                        QLatin1Char('*') + fileName.midRef(charPos + 1));
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
    const int dataSize = data.size();
    for (int matchlet = 0; matchlet < numMatchlets; ++matchlet) {
        const int off = firstOffset + matchlet * 32;
        const int rangeStart = cacheFile->getUint32(off);
        const int rangeLength = cacheFile->getUint32(off + 4);
        //const int wordSize = cacheFile->getUint32(off + 8);
        const int valueLength = cacheFile->getUint32(off + 12);
        const int valueOffset = cacheFile->getUint32(off + 16);
        const int maskOffset = cacheFile->getUint32(off + 20);
        const char *mask = maskOffset ? cacheFile->getCharStar(maskOffset) : NULL;

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
        if (matchMagicRule(m_cacheFile, numMatchlets, firstMatchletOffset, data)) {
            const int mimeTypeOffset = m_cacheFile->getUint32(off + 4);
            const char *mimeType = m_cacheFile->getCharStar(mimeTypeOffset);
            *accuracyPtr = m_cacheFile->getUint32(off);
            // Return the first match. We have no rules for conflicting magic data...
            // (mime.cache itself is sorted, but what about local overrides with a lower prio?)
            candidate = mimeTypeForNameUnchecked(QLatin1String(mimeType));
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
                if (!result.contains(strParent))
                    result.append(strParent);
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
            return QLatin1String(mimeType);
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
            if (!result.contains(strAlias))
                result.append(strAlias);
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
            QTextStream stream(&file);
            stream.setCodec("ISO 8859-1");
            QString line;
            while (stream.readLineInto(&line))
                m_mimetypeNames.insert(line);
        }
    }
}

void QMimeBinaryProvider::addAllMimeTypes(QList<QMimeType> &result)
{
    loadMimeTypeList();
    if (result.isEmpty()) {
        result.reserve(m_mimetypeNames.count());
        for (const QString &name : m_mimetypeNames)
            result.append(mimeTypeForNameUnchecked(name));
    } else {
        for (const QString &name : m_mimetypeNames)
            if (std::find_if(result.constBegin(), result.constEnd(), [name](const QMimeType &mime) -> bool { return mime.name() == name; })
                    == result.constEnd())
                result.append(mimeTypeForNameUnchecked(name));
    }
}

void QMimeBinaryProvider::loadMimeTypePrivate(QMimeTypePrivate &data)
{
#ifdef QT_NO_XMLSTREAMREADER
    qWarning("Cannot load mime type since QXmlStreamReader is not available.");
    return;
#else
    if (data.loaded)
        return;
    data.loaded = true;
    // load comment and globPatterns

    const QString file = data.name + QLatin1String(".xml");
    // shared-mime-info since 1.3 lowercases the xml files
    QStringList mimeFiles = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("mime/") + file.toLower());
    if (mimeFiles.isEmpty())
        mimeFiles = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("mime/") + file); // pre-1.3
    if (mimeFiles.isEmpty()) {
        qWarning() << "No file found for" << file << ", even though update-mime-info said it would exist.\n"
                      "Either it was just removed, or the directory doesn't have executable permission..."
                   << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("mime"), QStandardPaths::LocateDirectory);
        return;
    }

    QString mainPattern;

    for (QStringList::const_reverse_iterator it = mimeFiles.crbegin(), end = mimeFiles.crend(); it != end; ++it) { // global first, then local.
        QFile qfile(*it);
        if (!qfile.open(QFile::ReadOnly))
            continue;

        QXmlStreamReader xml(&qfile);
        if (xml.readNextStartElement()) {
            if (xml.name() != QLatin1String("mime-type")) {
                continue;
            }
            const QStringRef name = xml.attributes().value(QLatin1String("type"));
            if (name.isEmpty())
                continue;
            if (name.compare(data.name, Qt::CaseInsensitive))
                qWarning() << "Got name" << name << "in file" << file << "expected" << data.name;

            while (xml.readNextStartElement()) {
                const QStringRef tag = xml.name();
                if (tag == QLatin1String("comment")) {
                    QString lang = xml.attributes().value(QLatin1String("xml:lang")).toString();
                    const QString text = xml.readElementText();
                    if (lang.isEmpty()) {
                        lang = QLatin1String("en_US");
                    }
                    data.localeComments.insert(lang, text);
                    continue; // we called readElementText, so we're at the EndElement already.
                } else if (tag == QLatin1String("icon")) { // as written out by shared-mime-info >= 0.40
                    data.iconName = xml.attributes().value(QLatin1String("name")).toString();
                } else if (tag == QLatin1String("glob-deleteall")) { // as written out by shared-mime-info >= 0.70
                    data.globPatterns.clear();
                } else if (tag == QLatin1String("glob")) { // as written out by shared-mime-info >= 0.70
                    const QString pattern = xml.attributes().value(QLatin1String("pattern")).toString();
                    if (mainPattern.isEmpty() && pattern.startsWith(QLatin1Char('*'))) {
                        mainPattern = pattern;
                    }
                    if (!data.globPatterns.contains(pattern))
                        data.globPatterns.append(pattern);
                }
                xml.skipCurrentElement();
            }
            Q_ASSERT(xml.name() == QLatin1String("mime-type"));
        }
    }

    // Let's assume that shared-mime-info is at least version 0.70
    // Otherwise we would need 1) a version check, and 2) code for parsing patterns from the globs file.
#if 1
    if (!mainPattern.isEmpty() && (data.globPatterns.isEmpty() || data.globPatterns.constFirst() != mainPattern)) {
        // ensure it's first in the list of patterns
        data.globPatterns.removeAll(mainPattern);
        data.globPatterns.prepend(mainPattern);
    }
#else
    const bool globsInXml = sharedMimeInfoVersion() >= QT_VERSION_CHECK(0, 70, 0);
    if (globsInXml) {
        if (!mainPattern.isEmpty() && data.globPatterns.constFirst() != mainPattern) {
            // ensure it's first in the list of patterns
            data.globPatterns.removeAll(mainPattern);
            data.globPatterns.prepend(mainPattern);
        }
    } else {
        // Fallback: get the patterns from the globs file
        // TODO: This would be the only way to support shared-mime-info < 0.70
        // But is this really worth the effort?
    }
#endif
#endif //QT_NO_XMLSTREAMREADER
}

// Binary search in the icons or generic-icons list
QLatin1String QMimeBinaryProvider::iconForMime(CacheFile *cacheFile, int posListOffset, const QByteArray &inputMime)
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
            return QLatin1String(cacheFile->getCharStar(iconOffset));
        }
    }
    return QLatin1String();
}

void QMimeBinaryProvider::loadIcon(QMimeTypePrivate &data)
{
    const QByteArray inputMime = data.name.toLatin1();
    const QLatin1String icon = iconForMime(m_cacheFile, PosIconsListOffset, inputMime);
    if (!icon.isEmpty()) {
        data.iconName = icon;
    }
}

void QMimeBinaryProvider::loadGenericIcon(QMimeTypePrivate &data)
{
    const QByteArray inputMime = data.name.toLatin1();
    const QLatin1String icon = iconForMime(m_cacheFile, PosGenericIconsListOffset, inputMime);
    if (!icon.isEmpty()) {
        data.genericIconName = icon;
    }
}

////

QMimeXMLProvider::QMimeXMLProvider(QMimeDatabasePrivate *db, const QString &directory)
    : QMimeProviderBase(db, directory)
{
    initResources();
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
    for (const QMimeMagicRuleMatcher &matcher : qAsConst(m_magicMatchers)) {
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
    allFiles.reserve(files.count());
    for (const QString &xmlFile : files)
        allFiles.append(packageDir + QLatin1Char('/') + xmlFile);

    if (m_allFiles == allFiles)
        return;
    m_allFiles = allFiles;

    m_nameMimeTypeMap.clear();
    m_aliases.clear();
    m_parents.clear();
    m_mimeTypeGlobs.clear();
    m_magicMatchers.clear();

    //qDebug() << "Loading" << m_allFiles;

    for (const QString &file : qAsConst(allFiles))
        load(file);
}

void QMimeXMLProvider::load(const QString &fileName)
{
    QString errorMessage;
    if (!load(fileName, &errorMessage))
        qWarning("QMimeDatabase: Error loading %s\n%s", qPrintable(fileName), qPrintable(errorMessage));
}

bool QMimeXMLProvider::load(const QString &fileName, QString *errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QLatin1String("Cannot open ") + fileName + QLatin1String(": ") + file.errorString();
        return false;
    }

    if (errorMessage)
        errorMessage->clear();

    QMimeTypeParser parser(*this);
    return parser.parse(&file, fileName, errorMessage);
}

void QMimeXMLProvider::addGlobPattern(const QMimeGlobPattern &glob)
{
    m_mimeTypeGlobs.addGlob(glob);
}

void QMimeXMLProvider::addMimeType(const QMimeType &mt)
{
    Q_ASSERT(!mt.d.data()->fromCache);
    m_nameMimeTypeMap.insert(mt.name(), mt);
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
    for (auto it = m_aliases.constBegin(), end = m_aliases.constEnd() ; it != end ; ++it) {
        if (it.value() == name) {
            if (!result.contains(it.key()))
                result.append(it.key());
        }
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
