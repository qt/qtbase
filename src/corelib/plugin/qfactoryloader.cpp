// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfactoryloader_p.h"

#include "private/qcoreapplication_p.h"
#include "private/qloggingregistry_p.h"
#include "qcborarray.h"
#include "qcbormap.h"
#include "qcborstreamreader.h"
#include "qcborvalue.h"
#include "qdirlisting.h"
#include "qfileinfo.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "qplugin.h"
#include "qpluginloader.h"

#include <qtcore_tracepoints_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_TRACE_POINT(qtcore, QFactoryLoader_update, const QString &fileName);

namespace {
struct IterationResult
{
    enum Result {
        FinishedSearch = 0,
        ContinueSearch,

        // parse errors
        ParsingError = -1,
        InvalidMetaDataVersion = -2,
        InvalidTopLevelItem = -3,
        InvalidHeaderItem = -4,
    };
    Result result;
    QCborError error = { QCborError::NoError };

    Q_IMPLICIT IterationResult(Result r) : result(r) {}
    Q_IMPLICIT IterationResult(QCborError e) : result(ParsingError), error(e) {}
};

struct QFactoryLoaderIidSearch
{
    QLatin1StringView iid;
    bool matchesIid = false;
    QFactoryLoaderIidSearch(QLatin1StringView iid) : iid(iid)
    { Q_ASSERT(!iid.isEmpty()); }

    static IterationResult::Result skip(QCborStreamReader &reader)
    {
        // skip this, whatever it is
        reader.next();
        return IterationResult::ContinueSearch;
    }

    IterationResult::Result operator()(QtPluginMetaDataKeys key, QCborStreamReader &reader)
    {
        if (key != QtPluginMetaDataKeys::IID)
            return skip(reader);
        matchesIid = (reader.readAllString() == iid);
        return IterationResult::FinishedSearch;
    }
    IterationResult::Result operator()(QUtf8StringView, QCborStreamReader &reader)
    {
        return skip(reader);
    }
};

struct QFactoryLoaderMetaDataKeysExtractor : QFactoryLoaderIidSearch
{
    QCborArray keys;
    QFactoryLoaderMetaDataKeysExtractor(QLatin1StringView iid)
        : QFactoryLoaderIidSearch(iid)
    {}

    IterationResult::Result operator()(QtPluginMetaDataKeys key, QCborStreamReader &reader)
    {
        if (key == QtPluginMetaDataKeys::IID) {
            QFactoryLoaderIidSearch::operator()(key, reader);
            return IterationResult::ContinueSearch;
        }
        if (key != QtPluginMetaDataKeys::MetaData)
            return skip(reader);

        if (!matchesIid)
            return IterationResult::FinishedSearch;
        if (!reader.isMap() || !reader.isLengthKnown())
            return IterationResult::InvalidHeaderItem;
        if (!reader.enterContainer())
            return IterationResult::ParsingError;
        while (reader.isValid()) {
            // the metadata is JSON, so keys are all strings
            QByteArray key = reader.readAllUtf8String();
            if (key == "Keys") {
                if (!reader.isArray() || !reader.isLengthKnown())
                    return IterationResult::InvalidHeaderItem;
                keys = QCborValue::fromCbor(reader).toArray();
                break;
            }
            skip(reader);
        }
        // warning: we may not have finished iterating over the header
        return IterationResult::FinishedSearch;
    }
    using QFactoryLoaderIidSearch::operator();
};
} // unnamed namespace

template <typename F> static IterationResult iterateInPluginMetaData(QByteArrayView raw, F &&f)
{
    QPluginMetaData::Header header;
    Q_ASSERT(raw.size() >= qsizetype(sizeof(header)));
    memcpy(&header, raw.data(), sizeof(header));
    if (Q_UNLIKELY(header.version > QPluginMetaData::CurrentMetaDataVersion))
        return IterationResult::InvalidMetaDataVersion;

    // use fromRawData to keep QCborStreamReader from copying
    raw = raw.sliced(sizeof(header));
    QByteArray ba = QByteArray::fromRawData(raw.data(), raw.size());
    QCborStreamReader reader(ba);
    if (reader.isInvalid())
        return reader.lastError();
    if (!reader.isMap())
        return IterationResult::InvalidTopLevelItem;
    if (!reader.enterContainer())
        return reader.lastError();
    while (reader.isValid()) {
        IterationResult::Result r;
        if (reader.isInteger()) {
            // integer key, one of ours
            qint64 value = reader.toInteger();
            auto key = QtPluginMetaDataKeys(value);
            if (qint64(key) != value)
                return IterationResult::InvalidHeaderItem;
            if (!reader.next())
                return reader.lastError();
            r = f(key, reader);
        } else if (reader.isString()) {
            QByteArray key = reader.readAllUtf8String();
            if (key.isNull())
                return reader.lastError();
            r = f(QUtf8StringView(key), reader);
        } else {
            return IterationResult::InvalidTopLevelItem;
        }

        if (QCborError e = reader.lastError())
            return e;
        if (r != IterationResult::ContinueSearch)
            return r;
    }

    if (!reader.leaveContainer())
        return reader.lastError();
    return IterationResult::FinishedSearch;
}

static bool isIidMatch(QByteArrayView raw, QLatin1StringView iid)
{
    QFactoryLoaderIidSearch search(iid);
    iterateInPluginMetaData(raw, search);
    return search.matchesIid;
}

bool QPluginParsedMetaData::parse(QByteArrayView raw)
{
    QCborMap map;
    auto r = iterateInPluginMetaData(raw, [&](const auto &key, QCborStreamReader &reader) {
        QCborValue item = QCborValue::fromCbor(reader);
        if (item.isInvalid())
            return IterationResult::ParsingError;
        if constexpr (std::is_enum_v<std::decay_t<decltype(key)>>)
            map[int(key)] = item;
        else
            map[QString::fromUtf8(key)] = item;
        return IterationResult::ContinueSearch;
    });

    switch (r.result) {
    case IterationResult::FinishedSearch:
    case IterationResult::ContinueSearch:
        break;

    // parse errors
    case IterationResult::ParsingError:
        return setError(QFactoryLoader::tr("Metadata parsing error: %1").arg(r.error.toString()));
    case IterationResult::InvalidMetaDataVersion:
        return setError(QFactoryLoader::tr("Invalid metadata version"));
    case IterationResult::InvalidTopLevelItem:
    case IterationResult::InvalidHeaderItem:
        return setError(QFactoryLoader::tr("Unexpected metadata contents"));
    }

    // header was validated
    auto header = qFromUnaligned<QPluginMetaData::Header>(raw.data());

    DecodedArchRequirements archReq =
            header.version == 0 ? decodeVersion0ArchRequirements(header.plugin_arch_requirements)
                                : decodeVersion1ArchRequirements(header.plugin_arch_requirements);

    // insert the keys not stored in the top-level CBOR map
    map[int(QtPluginMetaDataKeys::QtVersion)] =
               QT_VERSION_CHECK(header.qt_major_version, header.qt_minor_version, 0);
    map[int(QtPluginMetaDataKeys::IsDebug)] = archReq.isDebug;
    map[int(QtPluginMetaDataKeys::Requirements)] = archReq.level;

    data = std::move(map);
    return true;
}

QJsonObject QPluginParsedMetaData::toJson() const
{
    // convert from the internal CBOR representation to an external JSON one
    QJsonObject o;
    for (auto it : data.toMap()) {
        QString key;
        if (it.first.isInteger()) {
            switch (it.first.toInteger()) {
#define CONVERT_TO_STRING(IntKey, StringKey, Description) \
            case int(IntKey): key = QStringLiteral(StringKey); break;
                QT_PLUGIN_FOREACH_METADATA(CONVERT_TO_STRING)
            }
        } else {
            key = it.first.toString();
        }

        if (!key.isEmpty())
            o.insert(key, it.second.toJsonValue());
    }
    return o;
}

#if QT_CONFIG(library)

Q_STATIC_LOGGING_CATEGORY_WITH_ENV_OVERRIDE(lcFactoryLoader, "QT_DEBUG_PLUGINS",
                                            "qt.core.plugin.factoryloader")

namespace {
struct QFactoryLoaderGlobals
{
    // needs to be recursive because loading one plugin could cause another
    // factory to be initialized
    QRecursiveMutex mutex;
    QList<QFactoryLoader *> loaders;
};
}

Q_GLOBAL_STATIC(QFactoryLoaderGlobals, qt_factoryloader_global)

inline void QFactoryLoader::Private::updateSinglePath(const QString &path)
{
    struct LibraryReleaser {
        void operator()(QLibraryPrivate *library)
        { if (library) library->release(); }
    };

    // If we've already loaded, skip it...
    if (loadedPaths.hasSeen(path))
        return;

    qCDebug(lcFactoryLoader) << "checking directory path" << path << "...";

    QDirListing plugins(path,
#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN)
                QStringList(QStringLiteral("*.dll")),
#elif defined(Q_OS_ANDROID)
                QStringList("libplugins_%1_*.so"_L1.arg(suffix)),
#endif
                QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::ResolveSymlinks);

    for (const auto &dirEntry : plugins) {
        const QString &fileName = dirEntry.fileName();
#if defined(Q_PROCESSOR_X86)
        if (fileName.endsWith(".avx2"_L1) || fileName.endsWith(".avx512"_L1)) {
            // ignore AVX2-optimized file, we'll do a bait-and-switch to it later
            continue;
        }
#endif
        qCDebug(lcFactoryLoader) << "looking at" << fileName;

        Q_TRACE(QFactoryLoader_update, fileName);

        QLibraryPrivate::UniquePtr library;
        library.reset(QLibraryPrivate::findOrCreate(dirEntry.canonicalFilePath()));
        if (!library->isPlugin()) {
            qCDebug(lcFactoryLoader) << library->errorString << Qt::endl
                                     << "         not a plugin";
            continue;
        }

        QStringList keys;
        bool metaDataOk = false;

        QString iid = library->metaData.value(QtPluginMetaDataKeys::IID).toString();
        if (iid == QLatin1StringView(this->iid.constData(), this->iid.size())) {
            QCborMap object = library->metaData.value(QtPluginMetaDataKeys::MetaData).toMap();
            metaDataOk = true;

            const QCborArray k = object.value("Keys"_L1).toArray();
            for (QCborValueConstRef v : k)
                keys += cs ? v.toString() : v.toString().toLower();
        }
        qCDebug(lcFactoryLoader) << "Got keys from plugin meta data" << keys;

        if (!metaDataOk)
            continue;

        static constexpr qint64 QtVersionNoPatch = QT_VERSION_CHECK(QT_VERSION_MAJOR, QT_VERSION_MINOR, 0);
        int thisVersion = library->metaData.value(QtPluginMetaDataKeys::QtVersion).toInteger();
        if (iid.startsWith(QStringLiteral("org.qt-project.Qt.QPA"))) {
            // QPA plugins must match Qt Major.Minor
            if (thisVersion != QtVersionNoPatch) {
                qCDebug(lcFactoryLoader) << "Ignoring QPA plugin due to mismatching Qt versions" << QtVersionNoPatch << thisVersion;
                continue;
            }
        }

        int keyUsageCount = 0;
        for (const QString &key : std::as_const(keys)) {
            QLibraryPrivate *&keyMapEntry = keyMap[key];
            if (QLibraryPrivate *existingLibrary = keyMapEntry) {
                static constexpr bool QtBuildIsDebug = QT_CONFIG(debug);
                bool existingIsDebug = existingLibrary->metaData.value(QtPluginMetaDataKeys::IsDebug).toBool();
                bool thisIsDebug = library->metaData.value(QtPluginMetaDataKeys::IsDebug).toBool();
                bool configsAreDifferent = thisIsDebug != existingIsDebug;
                bool thisConfigDoesNotMatchQt = thisIsDebug != QtBuildIsDebug;
                if (configsAreDifferent && thisConfigDoesNotMatchQt)
                    continue; // Existing library matches Qt's build config

                // If the existing library was built with a future Qt version,
                // whereas the one we're considering has a Qt version that fits
                // better, we prioritize the better match.
                int existingVersion = existingLibrary->metaData.value(QtPluginMetaDataKeys::QtVersion).toInteger();
                if (existingVersion == QtVersionNoPatch)
                    continue; // Prefer exact Qt version match
                if (existingVersion < QtVersionNoPatch && thisVersion > QtVersionNoPatch)
                    continue; // Better too old than too new
                if (existingVersion < QtVersionNoPatch && thisVersion < existingVersion)
                    continue; // Otherwise prefer newest
            }

            keyMapEntry = library.get();
            ++keyUsageCount;
        }
        if (keyUsageCount || keys.isEmpty()) {
            library->setLoadHints(QLibrary::PreventUnloadHint); // once loaded, don't unload
            QMutexLocker locker(&mutex);
            libraries.push_back(std::move(library));
        }
    };

    loadedLibraries.resize(libraries.size());
}

void QFactoryLoader::setLoadHints(QLibrary::LoadHints loadHints)
{
    d->loadHints = loadHints;
}

void QFactoryLoader::update()
{
#ifdef QT_SHARED
    if (!d->extraSearchPath.isEmpty())
        d->updateSinglePath(d->extraSearchPath);

    const QStringList paths = QCoreApplication::libraryPaths();
    for (const QString &pluginDir : paths) {
#ifdef Q_OS_ANDROID
        QString path = pluginDir;
#else
        QString path = pluginDir + d->suffix;
#endif
        d->updateSinglePath(path);
    }
#else
    qCDebug(lcFactoryLoader) << "ignoring" << d->iid
                             << "since plugins are disabled in static builds";
#endif
}

QFactoryLoader::~QFactoryLoader()
{
    if (!qt_factoryloader_global.isDestroyed()) {
        QMutexLocker locker(&qt_factoryloader_global->mutex);
        qt_factoryloader_global->loaders.removeOne(this);
    }

#if QT_CONFIG(library)
    for (qsizetype i = 0; i < d->loadedLibraries.size(); ++i) {
        if (d->loadedLibraries.at(i)) {
            auto &plugin = d->libraries[i];
            delete plugin->inst.data();
            plugin->unload();
        }
    }
#endif

    for (QtPluginInstanceFunction staticInstance : d->usedStaticInstances) {
        if (staticInstance)
            delete staticInstance();
    }
}

#if defined(Q_OS_UNIX) && !defined (Q_OS_DARWIN)
QLibraryPrivate *QFactoryLoader::library(const QString &key) const
{
    const auto it = d->keyMap.find(d->cs ? key : key.toLower());
    if (it == d->keyMap.cend())
        return nullptr;
    return it->second;
}
#endif

void QFactoryLoader::refreshAll()
{
    if (qt_factoryloader_global.exists()) {
        QMutexLocker locker(&qt_factoryloader_global->mutex);
        for (QFactoryLoader *loader : std::as_const(qt_factoryloader_global->loaders))
            loader->update();
    }
}

#endif // QT_CONFIG(library)

QFactoryLoader::QFactoryLoader(const char *iid,
                               const QString &suffix,
                               Qt::CaseSensitivity cs)
{
    Q_ASSERT_X(suffix.startsWith(u'/'), "QFactoryLoader",
               "For historical reasons, the suffix must start with '/' (and it can't be empty)");

    d->iid = iid;
#if QT_CONFIG(library)
    d->cs = cs;
    d->suffix = suffix;
# ifdef Q_OS_ANDROID
    if (!d->suffix.isEmpty() && d->suffix.at(0) == u'/')
        d->suffix.remove(0, 1);
# endif

    QMutexLocker locker(&qt_factoryloader_global->mutex);
    update();
    qt_factoryloader_global->loaders.append(this);
#else
    Q_UNUSED(suffix);
    Q_UNUSED(cs);
#endif
}

void QFactoryLoader::setExtraSearchPath(const QString &path)
{
#if QT_CONFIG(library)
    if (d->extraSearchPath == path)
        return;             // nothing to do

    QMutexLocker locker(&qt_factoryloader_global->mutex);
    QString oldPath = std::exchange(d->extraSearchPath, path);
    if (oldPath.isEmpty()) {
        // easy case, just update this directory
        d->updateSinglePath(d->extraSearchPath);
    } else {
        // must re-scan everything
        for (qsizetype i = 0; i < d->loadedLibraries.size(); ++i) {
            if (d->loadedLibraries.at(i)) {
                auto &plugin = d->libraries[i];
                delete plugin->inst.data();
            }
        }
        d->loadedLibraries.fill(false);
        d->loadedPaths.clear();
        d->libraries.clear();
        d->keyMap.clear();
        update();
    }
#else
    Q_UNUSED(path);
#endif
}

QFactoryLoader::MetaDataList QFactoryLoader::metaData() const
{
    QList<QPluginParsedMetaData> metaData;
#if QT_CONFIG(library)
    QMutexLocker locker(&d->mutex);
    metaData.reserve(qsizetype(d->libraries.size()));
    for (const auto &library : d->libraries)
        metaData.append(library->metaData);
    locker.unlock();
#endif

    QLatin1StringView iid(d->iid.constData(), d->iid.size());
    const auto staticPlugins = QPluginLoader::staticPlugins();
    for (const QStaticPlugin &plugin : staticPlugins) {
        QByteArrayView pluginData(static_cast<const char *>(plugin.rawMetaData), plugin.rawMetaDataSize);
        QPluginParsedMetaData parsed(pluginData);
        if (parsed.isError() || parsed.value(QtPluginMetaDataKeys::IID) != iid)
            continue;
        metaData.append(std::move(parsed));
    }

    // other portions of the code will cast to int (e.g., keyMap())
    Q_ASSERT(metaData.size() <= std::numeric_limits<int>::max());
    return metaData;
}

QList<QCborArray> QFactoryLoader::metaDataKeys() const
{
    QList<QCborArray> metaData;
#if QT_CONFIG(library)
    QMutexLocker locker(&d->mutex);
    metaData.reserve(qsizetype(d->libraries.size()));
    for (const auto &library : d->libraries) {
        const QCborValue md = library->metaData.value(QtPluginMetaDataKeys::MetaData);
        metaData.append(md["Keys"_L1].toArray());
    }
    locker.unlock();
#endif

    QLatin1StringView iid(d->iid.constData(), d->iid.size());
    const auto staticPlugins = QPluginLoader::staticPlugins();
    for (const QStaticPlugin &plugin : staticPlugins) {
        QByteArrayView pluginData(static_cast<const char *>(plugin.rawMetaData),
                                  plugin.rawMetaDataSize);
        QFactoryLoaderMetaDataKeysExtractor extractor{ iid };
        iterateInPluginMetaData(pluginData, extractor);
        if (extractor.matchesIid)
            metaData += std::move(extractor.keys);
    }

    // other portions of the code will cast to int (e.g., keyMap())
    Q_ASSERT(metaData.size() <= std::numeric_limits<int>::max());
    return metaData;
}

QObject *QFactoryLoader::instance(int index) const
{
    if (index < 0)
        return nullptr;

    QMutexLocker lock(&d->mutex);
    QObject *obj = instanceHelper_locked(index);

    if (obj && !obj->parent())
        obj->moveToThread(QCoreApplicationPrivate::mainThread());
    return obj;
}

inline QObject *QFactoryLoader::instanceHelper_locked(int index) const
{
#if QT_CONFIG(library)
    if (size_t(index) < d->libraries.size()) {
        QLibraryPrivate *library = d->libraries[index].get();
        d->loadedLibraries[index] = true;
        library->setLoadHints(d->loadHints);
        return library->pluginInstance();
    }
    // we know d->libraries.size() <= index <= numeric_limits<decltype(index)>::max() → no overflow
    index -= static_cast<int>(d->libraries.size());
#endif

    QLatin1StringView iid(d->iid.constData(), d->iid.size());
    const QList<QStaticPlugin> staticPlugins = QPluginLoader::staticPlugins();
    qsizetype i = 0;
    for (QStaticPlugin plugin : staticPlugins) {
        QByteArrayView pluginData(static_cast<const char *>(plugin.rawMetaData), plugin.rawMetaDataSize);
        if (!isIidMatch(pluginData, iid))
            continue;

        if (i == index) {
            if (d->usedStaticInstances.size() <= i)
                d->usedStaticInstances.resize(i + 1);
            d->usedStaticInstances[i] = plugin.instance;
            return plugin.instance();
        }
        ++i;
    }

    return nullptr;
}

QMultiMap<int, QString> QFactoryLoader::keyMap() const
{
    QMultiMap<int, QString> result;
    const QList<QCborArray> metaDataList = metaDataKeys();
    for (int i = 0; i < int(metaDataList.size()); ++i) {
        const QCborArray &keys = metaDataList[i];
        for (QCborValueConstRef key : keys)
            result.insert(i, key.toString());
    }
    return result;
}

int QFactoryLoader::indexOf(const QString &needle) const
{
    const QList<QCborArray> metaDataList = metaDataKeys();
    for (int i = 0; i < int(metaDataList.size()); ++i) {
        const QCborArray &keys = metaDataList[i];
        for (QCborValueConstRef key : keys) {
            if (key.toString().compare(needle, Qt::CaseInsensitive) == 0)
                return i;
        }
    }
    return -1;
}

QT_END_NAMESPACE
