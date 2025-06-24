// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <qplatformdefs.h> // always first

#include "qmimedatabase.h"
#include "qmimedatabase_p.h"

#include "qmimeprovider_p.h"
#include "qmimetype_p.h"

#include <private/qduplicatetracker_p.h>
#include <private/qfilesystementry_p.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QBuffer>
#include <QtCore/QUrl>
#include <QtCore/QDebug>

#include <algorithm>
#include <functional>
#include <stack>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QString directoryMimeType()
{
    return QStringLiteral("inode/directory");
}
static QString plainTextMimeType()
{
    return QStringLiteral("text/plain");
}

Q_GLOBAL_STATIC(QMimeDatabasePrivate, staticQMimeDatabase)

QMimeDatabasePrivate *QMimeDatabasePrivate::instance()
{
    return staticQMimeDatabase();
}

QMimeDatabasePrivate::QMimeDatabasePrivate()
    : m_defaultMimeType(QStringLiteral("application/octet-stream"))
{
}

QMimeDatabasePrivate::~QMimeDatabasePrivate()
{
}

Q_CONSTINIT
#ifdef QT_BUILD_INTERNAL
Q_CORE_EXPORT
#else
static const
#endif
int qmime_secondsBetweenChecks = 5;

bool QMimeDatabasePrivate::shouldCheck()
{
    if (m_lastCheck.isValid() && m_lastCheck.elapsed() < qmime_secondsBetweenChecks * 1000)
        return false;
    m_lastCheck.start();
    return true;
}

static QStringList locateMimeDirectories()
{
    QStringList dirs =
            QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("mime"),
                                      QStandardPaths::LocateDirectory);
    dirs.append(u":/qt-project.org/qmime"_s);
    return dirs;
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_INTEGRITY)
#  define QT_USE_MMAP
#endif

void QMimeDatabasePrivate::loadProviders()
{
    // We use QStandardPaths every time to check if new files appeared
    const QStringList mimeDirs = locateMimeDirectories();
    const auto fdoIterator = std::find_if(mimeDirs.constBegin(), mimeDirs.constEnd(), [](const QString &mimeDir) -> bool {
        return QFileInfo::exists(mimeDir + "/packages/freedesktop.org.xml"_L1); }
    );
    const bool needInternalDB = QMimeXMLProvider::InternalDatabaseAvailable && fdoIterator == mimeDirs.constEnd();
    //qDebug() << "mime dirs:" << mimeDirs;

    Providers currentProviders;
    std::swap(m_providers, currentProviders);

    m_providers.reserve(mimeDirs.size() + (needInternalDB ? 1 : 0));

    for (const QString &mimeDir : mimeDirs) {
        const QString cacheFile = mimeDir + "/mime.cache"_L1;
        // Check if we already have a provider for this dir
        const auto predicate = [mimeDir](const std::unique_ptr<QMimeProviderBase> &prov)
        {
            return prov && prov->directory() == mimeDir;
        };
        const auto it = std::find_if(currentProviders.begin(), currentProviders.end(), predicate);
        if (it == currentProviders.end()) {
            std::unique_ptr<QMimeProviderBase> provider;
#if defined(QT_USE_MMAP)
            if (qEnvironmentVariableIsEmpty("QT_NO_MIME_CACHE") && QFileInfo::exists(cacheFile)) {
                provider.reset(new QMimeBinaryProvider(this, mimeDir));
                //qDebug() << "Created binary provider for" << mimeDir;
                if (!provider->isValid()) {
                    provider.reset();
                }
            }
#endif
            if (!provider) {
                provider.reset(new QMimeXMLProvider(this, mimeDir));
                //qDebug() << "Created XML provider for" << mimeDir;
            }
            m_providers.push_back(std::move(provider));
        } else {
            auto provider = std::move(*it); // take provider out of the vector
            provider->ensureLoaded();
            if (!provider->isValid()) {
                provider.reset(new QMimeXMLProvider(this, mimeDir));
                //qDebug() << "Created XML provider to replace binary provider for" << mimeDir;
            }
            m_providers.push_back(std::move(provider));
        }
    }
    // mimeDirs is sorted "most local first, most global last"
    // so the internal XML DB goes at the end
    if (needInternalDB) {
        // Check if we already have a provider for the InternalDatabase
        const auto isInternal = [](const std::unique_ptr<QMimeProviderBase> &prov)
        {
            return prov && prov->isInternalDatabase();
        };
        const auto it = std::find_if(currentProviders.begin(), currentProviders.end(), isInternal);
        if (it == currentProviders.end()) {
            m_providers.push_back(Providers::value_type(new QMimeXMLProvider(this, QMimeXMLProvider::InternalDatabase)));
        } else {
            m_providers.push_back(std::move(*it));
        }
    }

    auto it = m_providers.begin();
    (*it)->setOverrideProvider(nullptr);
    ++it;
    const auto end = m_providers.end();
    for (; it != end; ++it)
        (*it)->setOverrideProvider((it - 1)->get());
}

const QMimeDatabasePrivate::Providers &QMimeDatabasePrivate::providers()
{
#ifndef Q_OS_WASM // stub implementation always returns true
    Q_ASSERT(!mutex.tryLock()); // caller should have locked mutex
#endif
    if (m_providers.empty()) {
        loadProviders();
        m_lastCheck.start();
    } else {
        if (shouldCheck())
            loadProviders();
    }
    return m_providers;
}

QString QMimeDatabasePrivate::resolveAlias(const QString &nameOrAlias)
{
    for (const auto &provider : providers()) {
        const QString ret = provider->resolveAlias(nameOrAlias);
        if (!ret.isEmpty())
            return ret;
    }
    return nameOrAlias;
}

/*!
    \internal
    Returns a MIME type or an invalid one if none found
 */
QMimeType QMimeDatabasePrivate::mimeTypeForName(const QString &nameOrAlias)
{
    const QString mimeName = resolveAlias(nameOrAlias);
    for (const auto &provider : providers()) {
        if (provider->knowsMimeType(mimeName))
            return QMimeType(QMimeTypePrivate(mimeName));
    }
    return {};
}

QStringList QMimeDatabasePrivate::mimeTypeForFileName(const QString &fileName)
{
    if (fileName.endsWith(u'/'))
        return { directoryMimeType() };

    const QMimeGlobMatchResult result = findByFileName(fileName);
    QStringList matchingMimeTypes = result.m_matchingMimeTypes;
    matchingMimeTypes.sort(); // make it deterministic
    return matchingMimeTypes;
}

QMimeGlobMatchResult QMimeDatabasePrivate::findByFileName(const QString &fileName)
{
    QMimeGlobMatchResult result;
    const QString fileNameExcludingPath = QFileSystemEntry(fileName).fileName();
    for (const auto &provider : providers())
        provider->addFileNameMatches(fileNameExcludingPath, result);
    return result;
}

QMimeTypePrivate::LocaleHash QMimeDatabasePrivate::localeComments(const QString &name)
{
    QMutexLocker locker(&mutex);
    for (const auto &provider : providers()) {
        auto comments = provider->localeComments(name);
        if (!comments.isEmpty())
            return comments; // maybe we want to merge in comments from more global providers, in
                             // case of more translations?
    }
    return {};
}

QStringList QMimeDatabasePrivate::globPatterns(const QString &name)
{
    QMutexLocker locker(&mutex);
    QStringList patterns;
    const auto &providerList = providers();
    // reverse iteration because we start from most global, add up, clear if delete-all, and add up
    // again.
    for (auto rit = providerList.rbegin(); rit != providerList.rend(); ++rit) {
        auto *provider = rit->get();
        if (provider->hasGlobDeleteAll(name))
            patterns.clear();
        patterns += provider->globPatterns(name);
    }
    return patterns;
}

QString QMimeDatabasePrivate::genericIcon(const QString &name)
{
    QMutexLocker locker(&mutex);
    for (const auto &provider : providers()) {
        QString genericIconName = provider->genericIcon(name);
        if (!genericIconName.isEmpty())
            return genericIconName;
    }
    return {};
}

QString QMimeDatabasePrivate::icon(const QString &name)
{
    QMutexLocker locker(&mutex);
    for (const auto &provider : providers()) {
        QString iconName = provider->icon(name);
        if (!iconName.isEmpty())
            return iconName;
    }
    return {};
}

QString QMimeDatabasePrivate::fallbackParent(const QString &mimeTypeName) const
{
    const QStringView myGroup = QStringView{mimeTypeName}.left(mimeTypeName.indexOf(u'/'));
    // All text/* types are subclasses of text/plain.
    if (myGroup == "text"_L1 && mimeTypeName != plainTextMimeType())
        return plainTextMimeType();
    // All real-file mimetypes implicitly derive from application/octet-stream
    if (myGroup != "inode"_L1 &&
        // ignore non-file extensions
        myGroup != "all"_L1 && myGroup != "fonts"_L1 && myGroup != "print"_L1 && myGroup != "uri"_L1
        && mimeTypeName != defaultMimeType()) {
        return defaultMimeType();
    }
    return QString();
}

QStringList QMimeDatabasePrivate::mimeParents(const QString &mimeName)
{
    QMutexLocker locker(&mutex);
    return parents(mimeName);
}

QStringList QMimeDatabasePrivate::parents(const QString &mimeName)
{
    Q_ASSERT(!mutex.tryLock());
    QStringList result;
    for (const auto &provider : providers())
        provider->addParents(mimeName, result);
    if (result.isEmpty()) {
        const QString parent = fallbackParent(mimeName);
        if (!parent.isEmpty())
            result.append(parent);
    }
    return result;
}

QStringList QMimeDatabasePrivate::listAliases(const QString &mimeName)
{
    QMutexLocker locker(&mutex);
    QStringList result;
    for (const auto &provider : providers())
        provider->addAliases(mimeName, result);
    return result;
}

bool QMimeDatabasePrivate::mimeInherits(const QString &mime, const QString &parent)
{
    QMutexLocker locker(&mutex);
    return inherits(mime, parent);
}

static inline bool isTextFile(const QByteArray &data)
{
    // UTF16 byte order marks
    static const char bigEndianBOM[] = "\xFE\xFF";
    static const char littleEndianBOM[] = "\xFF\xFE";
    if (data.startsWith(bigEndianBOM) || data.startsWith(littleEndianBOM))
        return true;

    // Check the first 128 bytes (see shared-mime spec)
    const char *p = data.constData();
    const char *e = p + qMin(128, data.size());
    for ( ; p < e; ++p) {
        if (static_cast<unsigned char>(*p) < 32 && *p != 9 && *p !=10 && *p != 13)
            return false;
    }

    return true;
}

QMimeType QMimeDatabasePrivate::findByData(const QByteArray &data, int *accuracyPtr)
{
    if (data.isEmpty()) {
        *accuracyPtr = 100;
        return mimeTypeForName(QStringLiteral("application/x-zerosize"));
    }

    QMimeMagicResult result;
    for (const auto &provider : providers())
        provider->findByMagic(data, result);

    if (result.isValid()) {
        *accuracyPtr = result.accuracy;
        return QMimeType(QMimeTypePrivate(result.candidate));
    }

    if (isTextFile(data)) {
        *accuracyPtr = 5;
        return mimeTypeForName(plainTextMimeType());
    }

    return mimeTypeForName(defaultMimeType());
}

QMimeType QMimeDatabasePrivate::mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device)
{
    // First, glob patterns are evaluated. If there is a match with max weight,
    // this one is selected and we are done. Otherwise, the file contents are
    // evaluated and the match with the highest value (either a magic priority or
    // a glob pattern weight) is selected. Matching starts from max level (most
    // specific) in both cases, even when there is already a suffix matching candidate.

    // Pass 1) Try to match on the file name
    QMimeGlobMatchResult candidatesByName = findByFileName(fileName);
    if (candidatesByName.m_allMatchingMimeTypes.size() == 1) {
        const QMimeType mime = mimeTypeForName(candidatesByName.m_matchingMimeTypes.at(0));
        if (mime.isValid())
            return mime;
        candidatesByName = {};
    }

    // Extension is unknown, or matches multiple mimetypes.
    // Pass 2) Match on content, if we can read the data
    const auto matchOnContent = [this, &candidatesByName](QIODevice *device) {
        const bool openedByUs = !device->isOpen() && device->open(QIODevice::ReadOnly);
        if (device->isOpen()) {
            // Read 16K in one go (QIODEVICE_BUFFERSIZE in qiodevice_p.h).
            // This is much faster than seeking back and forth into QIODevice.
            const QByteArray data = device->peek(16384);

            if (openedByUs)
                device->close();

            int magicAccuracy = 0;
            QMimeType candidateByData(findByData(data, &magicAccuracy));

            // Disambiguate conflicting extensions (if magic matching found something)
            if (candidateByData.isValid() && magicAccuracy > 0) {
                const QString sniffedMime = candidateByData.name();
                // If the sniffedMime matches a highest-weight glob match, use it
                if (candidatesByName.m_matchingMimeTypes.contains(sniffedMime))
                    return candidateByData;

                for (const QString &m : std::as_const(candidatesByName.m_allMatchingMimeTypes)) {
                    if (inherits(m, sniffedMime)) {
                        // We have magic + pattern pointing to this, so it's a pretty good match
                        return mimeTypeForName(m);
                    }
                }
                if (candidatesByName.m_allMatchingMimeTypes.isEmpty()) {
                    // No glob, use magic
                    return candidateByData;
                }
            }
        }

        if (candidatesByName.m_allMatchingMimeTypes.size() > 1) {
            candidatesByName.m_matchingMimeTypes.sort(); // make it deterministic
            const QMimeType mime = mimeTypeForName(candidatesByName.m_matchingMimeTypes.at(0));
            if (mime.isValid())
                return mime;
        }

        return mimeTypeForName(defaultMimeType());
    };

    if (device)
        return matchOnContent(device);

    QFile fallbackFile(fileName);
    return matchOnContent(&fallbackFile);
}

QMimeType QMimeDatabasePrivate::mimeTypeForFileExtension(const QString &fileName)
{
    const QStringList matches = mimeTypeForFileName(fileName);
    if (matches.isEmpty()) {
        return mimeTypeForName(defaultMimeType());
    } else {
        // We have to pick one in case of multiple matches.
        return mimeTypeForName(matches.first());
    }
}

QMimeType QMimeDatabasePrivate::mimeTypeForData(QIODevice *device)
{
    int accuracy = 0;
    const bool openedByUs = !device->isOpen() && device->open(QIODevice::ReadOnly);
    if (device->isOpen()) {
        // Read 16K in one go (QIODEVICE_BUFFERSIZE in qiodevice_p.h).
        // This is much faster than seeking back and forth into QIODevice.
        const QByteArray data = device->peek(16384);
        QMimeType result = findByData(data, &accuracy);
        if (openedByUs)
            device->close();
        return result;
    }
    return mimeTypeForName(defaultMimeType());
}

QMimeType QMimeDatabasePrivate::mimeTypeForFile(const QString &fileName,
                                                const QFileInfo &fileInfo,
                                                QMimeDatabase::MatchMode mode)
{
    if (false) {
#ifdef Q_OS_UNIX
    } else if (fileInfo.isNativePath()) {
        // If this is a local file, we'll want to do a stat() ourselves so we can
        // detect additional inode types. In addition we want to follow symlinks.
        const QByteArray nativeFilePath = QFile::encodeName(fileName);
        QT_STATBUF statBuffer;
        if (QT_STAT(nativeFilePath.constData(), &statBuffer) == 0) {
            if (S_ISDIR(statBuffer.st_mode))
                return mimeTypeForName(directoryMimeType());
            if (S_ISCHR(statBuffer.st_mode))
                return mimeTypeForName(QStringLiteral("inode/chardevice"));
            if (S_ISBLK(statBuffer.st_mode))
                return mimeTypeForName(QStringLiteral("inode/blockdevice"));
            if (S_ISFIFO(statBuffer.st_mode))
                return mimeTypeForName(QStringLiteral("inode/fifo"));
            if (S_ISSOCK(statBuffer.st_mode))
                return mimeTypeForName(QStringLiteral("inode/socket"));
        }
#endif
    } else if (fileInfo.isDir()) {
        return mimeTypeForName(directoryMimeType());
    }

    switch (mode) {
    case QMimeDatabase::MatchDefault:
        break;
    case QMimeDatabase::MatchExtension:
        return mimeTypeForFileExtension(fileName);
    case QMimeDatabase::MatchContent: {
        QFile file(fileName);
        return mimeTypeForData(&file);
    }
    }
    // MatchDefault:
    return mimeTypeForFileNameAndData(fileName, nullptr);
}

QList<QMimeType> QMimeDatabasePrivate::allMimeTypes()
{
    QList<QMimeType> result;
    for (const auto &provider : providers())
        provider->addAllMimeTypes(result);
    return result;
}

bool QMimeDatabasePrivate::inherits(const QString &mime, const QString &parent)
{
    const QString resolvedParent = resolveAlias(parent);
    QDuplicateTracker<QString> seen;
    std::stack<QString, QStringList> toCheck;
    toCheck.push(mime);
    while (!toCheck.empty()) {
        if (toCheck.top() == resolvedParent)
            return true;
        const QString mimeName = toCheck.top();
        toCheck.pop();
        const auto parentList = parents(mimeName);
        for (const QString &par : parentList) {
            const QString resolvedPar = resolveAlias(par);
            if (!seen.hasSeen(resolvedPar))
                toCheck.push(resolvedPar);
        }
    }
    return false;
}

/*!
    \class QMimeDatabase
    \inmodule QtCore
    \brief The QMimeDatabase class maintains a database of MIME types.

    \since 5.0

    The MIME type database is provided by the freedesktop.org shared-mime-info
    project. If the MIME type database cannot be found on the system, as is the case
    on most Windows, \macos, and iOS systems, Qt will use its own copy of it.

    Applications which want to define custom MIME types need to install an
    XML file into the locations searched for MIME definitions.
    These locations can be queried with
    \snippet code/src_corelib_mimetype_qmimedatabase.cpp 1
    On a typical Unix system, this will be /usr/share/mime/packages/, but it is also
    possible to extend the list of directories by setting the environment variable
    \c XDG_DATA_DIRS. For instance adding /opt/myapp/share to \c XDG_DATA_DIRS will result
    in /opt/myapp/share/mime/packages/ being searched for MIME definitions.

    Here is an example of MIME XML:
    \snippet code/src_corelib_mimetype_qmimedatabase.cpp 2

    For more details about the syntax of XML MIME definitions, including defining
    "magic" in order to detect MIME types based on data as well, read the
    Shared Mime Info specification at
    http://standards.freedesktop.org/shared-mime-info-spec/shared-mime-info-spec-latest.html

    On Unix systems, a binary cache is used for more performance. This cache is generated
    by the command "update-mime-database path", where path would be /opt/myapp/share/mime
    in the above example. Make sure to run this command when installing the MIME type
    definition file.

    \threadsafe

    \snippet code/src_corelib_mimetype_qmimedatabase.cpp 0

    \sa QMimeType, {MIME Type Browser}
 */

/*!
    \fn QMimeDatabase::QMimeDatabase();
    Constructs a QMimeDatabase object.

    It is perfectly OK to create an instance of QMimeDatabase every time you need to
    perform a lookup.
    The parsing of mimetypes is done on demand (when shared-mime-info is installed)
    or when the very first instance is constructed (when parsing XML files directly).
 */
QMimeDatabase::QMimeDatabase() :
        d(staticQMimeDatabase())
{
}

/*!
    \fn QMimeDatabase::~QMimeDatabase();
    Destroys the QMimeDatabase object.
 */
QMimeDatabase::~QMimeDatabase()
{
    d = nullptr;
}

/*!
    \fn QMimeType QMimeDatabase::mimeTypeForName(const QString &nameOrAlias) const;
    Returns a MIME type for \a nameOrAlias or an invalid one if none found.
 */
QMimeType QMimeDatabase::mimeTypeForName(const QString &nameOrAlias) const
{
    QMutexLocker locker(&d->mutex);

    return d->mimeTypeForName(nameOrAlias);
}

/*!
    Returns a MIME type for \a fileInfo.

    A valid MIME type is always returned.

    The default matching algorithm looks at both the file name and the file
    contents, if necessary. The file extension has priority over the contents,
    but the contents will be used if the file extension is unknown, or
    matches multiple MIME types.
    If \a fileInfo is a Unix symbolic link, the file that it refers to
    will be used instead.
    If the file doesn't match any known pattern or data, the default MIME type
    (application/octet-stream) is returned.

    When \a mode is set to MatchExtension, only the file name is used, not
    the file contents. The file doesn't even have to exist. If the file name
    doesn't match any known pattern, the default MIME type (application/octet-stream)
    is returned.
    If multiple MIME types match this file, the first one (alphabetically) is returned.

    When \a mode is set to MatchContent, and the file is readable, only the
    file contents are used to determine the MIME type. This is equivalent to
    calling mimeTypeForData with a QFile as input device.

    \a fileInfo may refer to an absolute or relative path.

    \sa QMimeType::isDefault(), mimeTypeForData()
*/
QMimeType QMimeDatabase::mimeTypeForFile(const QFileInfo &fileInfo, MatchMode mode) const
{
    QMutexLocker locker(&d->mutex);

    return d->mimeTypeForFile(fileInfo.filePath(), fileInfo, mode);
}

/*!
    Returns a MIME type for the file named \a fileName using \a mode.

    \overload
*/
QMimeType QMimeDatabase::mimeTypeForFile(const QString &fileName, MatchMode mode) const
{
    QMutexLocker locker(&d->mutex);

    if (mode == MatchExtension) {
        return d->mimeTypeForFileExtension(fileName);
    } else {
        QFileInfo fileInfo(fileName);
        return d->mimeTypeForFile(fileName, fileInfo, mode);
    }
}

/*!
    Returns the MIME types for the file name \a fileName.

    If the file name doesn't match any known pattern, an empty list is returned.
    If multiple MIME types match this file, they are all returned.

    This function does not try to open the file. To also use the content
    when determining the MIME type, use mimeTypeForFile() or
    mimeTypeForFileNameAndData() instead.

    \sa mimeTypeForFile()
*/
QList<QMimeType> QMimeDatabase::mimeTypesForFileName(const QString &fileName) const
{
    QMutexLocker locker(&d->mutex);

    const QStringList matches = d->mimeTypeForFileName(fileName);
    QList<QMimeType> mimes;
    mimes.reserve(matches.size());
    for (const QString &mime : matches)
        mimes.append(d->mimeTypeForName(mime));
    return mimes;
}
/*!
    Returns the suffix for the file \a fileName, as known by the MIME database.

    This allows to pre-select "tar.bz2" for foo.tar.bz2, but still only
    "txt" for my.file.with.dots.txt.
*/
QString QMimeDatabase::suffixForFileName(const QString &fileName) const
{
    QMutexLocker locker(&d->mutex);
    const qsizetype suffixLength = d->findByFileName(fileName).m_knownSuffixLength;
    return fileName.right(suffixLength);
}

/*!
    Returns a MIME type for \a data.

    A valid MIME type is always returned. If \a data doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.
*/
QMimeType QMimeDatabase::mimeTypeForData(const QByteArray &data) const
{
    QMutexLocker locker(&d->mutex);

    int accuracy = 0;
    return d->findByData(data, &accuracy);
}

/*!
    Returns a MIME type for the data in \a device.

    A valid MIME type is always returned. If the data in \a device doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.
*/
QMimeType QMimeDatabase::mimeTypeForData(QIODevice *device) const
{
    QMutexLocker locker(&d->mutex);

    return d->mimeTypeForData(device);
}

/*!
    Returns a MIME type for \a url.

    If the URL is a local file, this calls mimeTypeForFile.

    Otherwise the matching is done based on the file name only,
    except for schemes where file names don't mean much, like HTTP.
    This method always returns the default mimetype for HTTP URLs,
    use QNetworkAccessManager to handle HTTP URLs properly.

    A valid MIME type is always returned. If \a url doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.
*/
QMimeType QMimeDatabase::mimeTypeForUrl(const QUrl &url) const
{
    if (url.isLocalFile())
        return mimeTypeForFile(url.toLocalFile());

    const QString scheme = url.scheme();
    if (scheme.startsWith("http"_L1) || scheme == "mailto"_L1)
        return mimeTypeForName(d->defaultMimeType());

    return mimeTypeForFile(url.path(), MatchExtension);
}

/*!
    Returns a MIME type for the given \a fileName and \a device data.

    This overload can be useful when the file is remote, and we started to
    download some of its data in a device. This allows to do full MIME type
    matching for remote files as well.

    If the device is not open, it will be opened by this function, and closed
    after the MIME type detection is completed.

    A valid MIME type is always returned. If \a device data doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.

    This method looks at both the file name and the file contents,
    if necessary. The file extension has priority over the contents,
    but the contents will be used if the file extension is unknown, or
    matches multiple MIME types.
*/
QMimeType QMimeDatabase::mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device) const
{
    QMutexLocker locker(&d->mutex);

    if (fileName.endsWith(u'/'))
        return d->mimeTypeForName(directoryMimeType());

    const QMimeType result = d->mimeTypeForFileNameAndData(fileName, device);
    return result;
}

/*!
    Returns a MIME type for the given \a fileName and device \a data.

    This overload can be useful when the file is remote, and we started to
    download some of its data. This allows to do full MIME type matching for
    remote files as well.

    A valid MIME type is always returned. If \a data doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.

    This method looks at both the file name and the file contents,
    if necessary. The file extension has priority over the contents,
    but the contents will be used if the file extension is unknown, or
    matches multiple MIME types.
*/
QMimeType QMimeDatabase::mimeTypeForFileNameAndData(const QString &fileName, const QByteArray &data) const
{
    QMutexLocker locker(&d->mutex);

    if (fileName.endsWith(u'/'))
        return d->mimeTypeForName(directoryMimeType());

    QBuffer buffer(const_cast<QByteArray *>(&data));
    buffer.open(QIODevice::ReadOnly);
    return d->mimeTypeForFileNameAndData(fileName, &buffer);
}

/*!
    Returns the list of all available MIME types.

    This can be useful for showing all MIME types to the user, for instance
    in a MIME type editor. Do not use unless really necessary in other cases
    though, prefer using the  \l {mimeTypeForData()}{mimeTypeForXxx()} methods for performance reasons.
*/
QList<QMimeType> QMimeDatabase::allMimeTypes() const
{
    QMutexLocker locker(&d->mutex);

    return d->allMimeTypes();
}

/*!
    \enum QMimeDatabase::MatchMode

    This enum specifies how matching a file to a MIME type is performed.

    \value MatchDefault Both the file name and content are used to look for a match

    \value MatchExtension Only the file name is used to look for a match

    \value MatchContent The file content is used to look for a match
*/

QT_END_NAMESPACE
