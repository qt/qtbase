// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMIMEPROVIDER_P_H
#define QMIMEPROVIDER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmimedatabase_p.h"

QT_REQUIRE_CONFIG(mimetype);

#include "qmimeglobpattern_p.h"
#include <QtCore/qdatetime.h>
#include <QtCore/qset.h>
#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

class QMimeMagicRuleMatcher;

class QMimeProviderBase
{
public:
    QMimeProviderBase(QMimeDatabasePrivate *db, const QString &directory);
    virtual ~QMimeProviderBase() {}

    virtual bool isValid() = 0;
    virtual bool isInternalDatabase() const = 0;
    virtual QMimeType mimeTypeForName(const QString &name) = 0;
    virtual void addFileNameMatches(const QString &fileName, QMimeGlobMatchResult &result) = 0;
    virtual void addParents(const QString &mime, QStringList &result) = 0;
    virtual QString resolveAlias(const QString &name) = 0;
    virtual void addAliases(const QString &name, QStringList &result) = 0;
    virtual void findByMagic(const QByteArray &data, int *accuracyPtr, QMimeType &candidate) = 0;
    virtual void addAllMimeTypes(QList<QMimeType> &result) = 0;
    virtual bool loadMimeTypePrivate(QMimeTypePrivate &) { return false; }
    virtual void loadIcon(QMimeTypePrivate &) {}
    virtual void loadGenericIcon(QMimeTypePrivate &) {}
    virtual void ensureLoaded() {}
    virtual void excludeMimeTypeGlobs(const QStringList &) {}

    QString directory() const { return m_directory; }

    QMimeDatabasePrivate *m_db;
    QString m_directory;

    /*
        MimeTypes with "glob-deleteall" tags are handled differently by each provider
        sub-class:
        - QMimeBinaryProvider parses glob-deleteall tags lazily, i.e. only when loadMimeTypePrivate()
          is called, and clears the glob patterns associated with mimetypes that have this tag
        - QMimeXMLProvider parses glob-deleteall from the the start, i.e. when a XML file is
          parsed with QMimeTypeParser

        The two lists below are used to let both provider types (XML and Binary) communicate
        about mimetypes with glob-deleteall.
    */
    /*
        List of mimetypes in _this_ Provider that have a "glob-deleteall" tag,
        glob patterns for those mimetypes should be ignored in all _other_ lower
        precedence Providers.
    */
    QStringList m_mimeTypesWithDeletedGlobs;

    /*
        List of mimetypes with glob patterns that are "overwritten" in _this_ Provider,
        by a "glob-deleteall" tag in a mimetype definition in a _higher precedence_
        Provider. With QMimeBinaryProvider, we can't change the data in the binary mmap'ed
        file, hence the need for this list.
    */
    QStringList m_mimeTypesWithExcludedGlobs;
};

/*
   Parses the files 'mime.cache' and 'types' on demand
 */
class QMimeBinaryProvider final : public QMimeProviderBase
{
public:
    QMimeBinaryProvider(QMimeDatabasePrivate *db, const QString &directory);
    virtual ~QMimeBinaryProvider();

    bool isValid() override;
    bool isInternalDatabase() const override;
    QMimeType mimeTypeForName(const QString &name) override;
    void addFileNameMatches(const QString &fileName, QMimeGlobMatchResult &result) override;
    void addParents(const QString &mime, QStringList &result) override;
    QString resolveAlias(const QString &name) override;
    void addAliases(const QString &name, QStringList &result) override;
    void findByMagic(const QByteArray &data, int *accuracyPtr, QMimeType &candidate) override;
    void addAllMimeTypes(QList<QMimeType> &result) override;
    bool loadMimeTypePrivate(QMimeTypePrivate &) override;
    void loadIcon(QMimeTypePrivate &) override;
    void loadGenericIcon(QMimeTypePrivate &) override;
    void ensureLoaded() override;
    void excludeMimeTypeGlobs(const QStringList &toExclude) override;

private:
    struct CacheFile;

    void matchGlobList(QMimeGlobMatchResult &result, CacheFile *cacheFile, int offset, const QString &fileName);
    bool matchSuffixTree(QMimeGlobMatchResult &result, CacheFile *cacheFile, int numEntries,
                         int firstOffset, const QString &fileName, qsizetype charPos,
                         bool caseSensitiveCheck);
    bool matchMagicRule(CacheFile *cacheFile, int numMatchlets, int firstOffset, const QByteArray &data);
    bool isMimeTypeGlobsExcluded(const char *name);
    QLatin1StringView iconForMime(CacheFile *cacheFile, int posListOffset, const QByteArray &inputMime);
    void loadMimeTypeList();
    bool checkCacheChanged();

    CacheFile *m_cacheFile = nullptr;
    QStringList m_cacheFileNames;
    QSet<QString> m_mimetypeNames;
    bool m_mimetypeListLoaded;
    struct MimeTypeExtra
    {
        // Both retrieved on demand in loadMimeTypePrivate
        QHash<QString, QString> localeComments;
        QStringList globPatterns;
    };
    QMap<QString, MimeTypeExtra> m_mimetypeExtra;
};

/*
   Parses the raw XML files (slower)
 */
class QMimeXMLProvider final : public QMimeProviderBase
{
public:
    enum InternalDatabaseEnum { InternalDatabase };
#if QT_CONFIG(mimetype_database)
    enum : bool { InternalDatabaseAvailable = true };
#else
    enum : bool { InternalDatabaseAvailable = false };
#endif
    QMimeXMLProvider(QMimeDatabasePrivate *db, InternalDatabaseEnum);
    QMimeXMLProvider(QMimeDatabasePrivate *db, const QString &directory);
    ~QMimeXMLProvider();

    bool isValid() override;
    bool isInternalDatabase() const override;
    QMimeType mimeTypeForName(const QString &name) override;
    void addFileNameMatches(const QString &fileName, QMimeGlobMatchResult &result) override;
    void addParents(const QString &mime, QStringList &result) override;
    QString resolveAlias(const QString &name) override;
    void addAliases(const QString &name, QStringList &result) override;
    void findByMagic(const QByteArray &data, int *accuracyPtr, QMimeType &candidate) override;
    void addAllMimeTypes(QList<QMimeType> &result) override;
    void ensureLoaded() override;

    bool load(const QString &fileName, QString *errorMessage);

    // Called by the mimetype xml parser
    void addMimeType(const QMimeType &mt);
    void excludeMimeTypeGlobs(const QStringList &toExclude) override;
    void addGlobPattern(const QMimeGlobPattern &glob);
    void addParent(const QString &child, const QString &parent);
    void addAlias(const QString &alias, const QString &name);
    void addMagicMatcher(const QMimeMagicRuleMatcher &matcher);

private:
    void load(const QString &fileName);
    void load(const char *data, qsizetype len);

    typedef QHash<QString, QMimeType> NameMimeTypeMap;
    NameMimeTypeMap m_nameMimeTypeMap;

    typedef QHash<QString, QString> AliasHash;
    AliasHash m_aliases;

    typedef QHash<QString, QStringList> ParentsHash;
    ParentsHash m_parents;
    QMimeAllGlobPatterns m_mimeTypeGlobs;

    QList<QMimeMagicRuleMatcher> m_magicMatchers;
    QStringList m_allFiles;
};

QT_END_NAMESPACE

#endif // QMIMEPROVIDER_P_H
