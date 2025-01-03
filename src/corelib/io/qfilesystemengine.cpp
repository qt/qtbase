// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfilesystemengine_p.h"
#include <QtCore/qdir.h>
#include <QtCore/qset.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/private/qabstractfileengine_p.h>
#ifdef QT_BUILD_CORE_LIB
#include <QtCore/private/qresource_p.h>
#endif
#include <QtCore/private/qduplicatetracker_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal

    Returns the canonicalized form of \a path (i.e., with all symlinks
    resolved, and all redundant path elements removed.
*/
QString QFileSystemEngine::slowCanonicalized(const QString &path)
{
    if (path.isEmpty())
        return path;

    QFileInfo fi;
    const QChar slash(u'/');
    QString tmpPath = path;
    qsizetype separatorPos = 0;
    QSet<QString> nonSymlinks;
    QDuplicateTracker<QString> known;

    (void)known.hasSeen(path);
    do {
#ifdef Q_OS_WIN
        if (separatorPos == 0) {
            if (tmpPath.size() >= 2 && tmpPath.at(0) == slash && tmpPath.at(1) == slash) {
                // UNC, skip past the first two elements
                separatorPos = tmpPath.indexOf(slash, 2);
            } else if (tmpPath.size() >= 3 && tmpPath.at(1) == u':' && tmpPath.at(2) == slash) {
                // volume root, skip since it can not be a symlink
                separatorPos = 2;
            }
        }
        if (separatorPos != -1)
#endif
        separatorPos = tmpPath.indexOf(slash, separatorPos + 1);
        QString prefix = separatorPos == -1 ? tmpPath : tmpPath.left(separatorPos);
        if (!nonSymlinks.contains(prefix)) {
            fi.setFile(prefix);
            if (fi.isSymLink()) {
                QString target = fi.symLinkTarget();
                if (separatorPos != -1) {
                    if (fi.isDir() && !target.endsWith(slash))
                        target.append(slash);
                    target.append(QStringView{tmpPath}.mid(separatorPos));
                }
                tmpPath = QDir::cleanPath(target);
                separatorPos = 0;

                if (known.hasSeen(tmpPath))
                    return QString();
            } else {
                nonSymlinks.insert(prefix);
            }
        }
    } while (separatorPos != -1);

    return QDir::cleanPath(tmpPath);
}

static inline bool _q_checkEntry(QFileSystemEntry &entry, QFileSystemMetaData &data, bool resolvingEntry)
{
    if (resolvingEntry) {
        if (!QFileSystemEngine::fillMetaData(entry, data, QFileSystemMetaData::ExistsAttribute)
                || !data.exists()) {
            data.clear();
            return false;
        }
    }

    return true;
}

static inline bool _q_checkEntry(QAbstractFileEngine *&engine, bool resolvingEntry)
{
    if (resolvingEntry) {
        if (!(engine->fileFlags(QAbstractFileEngine::FlagsMask) & QAbstractFileEngine::ExistsFlag)) {
            delete engine;
            engine = nullptr;
            return false;
        }
    }

    return true;
}

static bool _q_resolveEntryAndCreateLegacyEngine_recursive(QFileSystemEntry &entry, QFileSystemMetaData &data,
        QAbstractFileEngine *&engine, bool resolvingEntry = false)
{
    QString const &filePath = entry.filePath();
    if ((engine = qt_custom_file_engine_handler_create(filePath)))
        return _q_checkEntry(engine, resolvingEntry);

#if defined(QT_BUILD_CORE_LIB)
    for (qsizetype prefixSeparator = 0; prefixSeparator < filePath.size(); ++prefixSeparator) {
        QChar const ch = filePath[prefixSeparator];
        if (ch == u'/')
            break;

        if (ch == u':') {
            if (prefixSeparator == 0) {
                engine = new QResourceFileEngine(filePath);
                return _q_checkEntry(engine, resolvingEntry);
            }

            if (prefixSeparator == 1)
                break;

            const QStringList &paths = QDir::searchPaths(filePath.left(prefixSeparator));
            for (int i = 0; i < paths.size(); i++) {
                entry = QFileSystemEntry(QDir::cleanPath(
                        paths.at(i) % u'/' % QStringView{filePath}.mid(prefixSeparator + 1)));
                // Recurse!
                if (_q_resolveEntryAndCreateLegacyEngine_recursive(entry, data, engine, true))
                    return true;
            }

            // entry may have been clobbered at this point.
            return false;
        }

        //  There's no need to fully validate the prefix here. Consulting the
        //  unicode tables could be expensive and validation is already
        //  performed in QDir::setSearchPaths.
        //
        //  if (!ch.isLetterOrNumber())
        //      break;
    }
#endif // defined(QT_BUILD_CORE_LIB)

    return _q_checkEntry(entry, data, resolvingEntry);
}

/*!
    \internal

    Resolves the \a entry (see QDir::searchPaths) and returns an engine for
    it, but never a QFSFileEngine.

    Returns a file engine that can be used to access the entry. Returns 0 if
    QFileSystemEngine API should be used to query and interact with the file
    system object.
*/
QAbstractFileEngine *QFileSystemEngine::resolveEntryAndCreateLegacyEngine(
        QFileSystemEntry &entry, QFileSystemMetaData &data) {
    QFileSystemEntry copy = entry;
    QAbstractFileEngine *engine = nullptr;

    if (_q_resolveEntryAndCreateLegacyEngine_recursive(copy, data, engine))
        // Reset entry to resolved copy.
        entry = copy;
    else
        data.clear();

    return engine;
}

//static
QString QFileSystemEngine::resolveUserName(const QFileSystemEntry &entry, QFileSystemMetaData &metaData)
{
#if defined(Q_OS_WIN)
    Q_UNUSED(metaData);
    return QFileSystemEngine::owner(entry, QAbstractFileEngine::OwnerUser);
#else //(Q_OS_UNIX)
    if (!metaData.hasFlags(QFileSystemMetaData::UserId))
        QFileSystemEngine::fillMetaData(entry, metaData, QFileSystemMetaData::UserId);
    if (!metaData.exists())
        return QString();
    return resolveUserName(metaData.userId());
#endif
}

//static
QString QFileSystemEngine::resolveGroupName(const QFileSystemEntry &entry, QFileSystemMetaData &metaData)
{
#if defined(Q_OS_WIN)
    Q_UNUSED(metaData);
    return QFileSystemEngine::owner(entry, QAbstractFileEngine::OwnerGroup);
#else //(Q_OS_UNIX)
    if (!metaData.hasFlags(QFileSystemMetaData::GroupId))
        QFileSystemEngine::fillMetaData(entry, metaData, QFileSystemMetaData::GroupId);
    if (!metaData.exists())
        return QString();
    return resolveGroupName(metaData.groupId());
#endif
}

//static
QFileSystemEntry QFileSystemEngine::getJunctionTarget(const QFileSystemEntry &link,
                                                      QFileSystemMetaData &data)
{
#if defined(Q_OS_WIN)
    return junctionTarget(link, data);
#else
    Q_UNUSED(link);
    Q_UNUSED(data);
    return {};
#endif
}

QT_END_NAMESPACE
