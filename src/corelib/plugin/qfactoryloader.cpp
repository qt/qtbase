/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2018 Intel Corporation.
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

#include "qfactoryloader_p.h"

#ifndef QT_NO_QOBJECT
#include "qfactoryinterface.h"

#include "private/qcoreapplication_p.h"
#include "private/qduplicatetracker_p.h"
#include "private/qloggingregistry_p.h"
#include "private/qobject_p.h"
#include "qcborarray.h"
#include "qcbormap.h"
#include "qcborvalue.h"
#include "qcborvalue.h"
#include "qdir.h"
#include "qfileinfo.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "qmap.h"
#include "qmutex.h"
#include "qplugin.h"
#include "qplugin_p.h"
#include "qpluginloader.h"

#if QT_CONFIG(library)
#  include "qlibrary_p.h"
#endif

#include <qtcore_tracepoints_p.h>

QT_BEGIN_NAMESPACE

bool QPluginParsedMetaData::parse(QByteArrayView raw)
{
    QPluginMetaData::Header header;
    Q_ASSERT(raw.size() >= qsizetype(sizeof(header)));
    memcpy(&header, raw.data(), sizeof(header));
    if (Q_UNLIKELY(header.version > QPluginMetaData::CurrentMetaDataVersion))
        return setError(QFactoryLoader::tr("Invalid metadata version"));

    // use fromRawData to keep QCborStreamReader from copying
    raw = raw.sliced(sizeof(header));
    QByteArray ba = QByteArray::fromRawData(raw.data(), raw.size());
    QCborParserError err;
    QCborValue metadata = QCborValue::fromCbor(ba, &err);

    if (err.error != QCborError::NoError)
        return setError(QFactoryLoader::tr("Metadata parsing error: %1").arg(err.error.toString()));
    if (!metadata.isMap())
        return setError(QFactoryLoader::tr("Unexpected metadata contents"));
    QCborMap map = metadata.toMap();
    metadata = {};

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

class QFactoryLoaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QFactoryLoader)
public:
    QFactoryLoaderPrivate() { }
    QByteArray iid;
#if QT_CONFIG(library)
    ~QFactoryLoaderPrivate();
    mutable QMutex mutex;
    QDuplicateTracker<QString> loadedPaths;
    QList<QLibraryPrivate*> libraryList;
    QMap<QString,QLibraryPrivate*> keyMap;
    QString suffix;
    Qt::CaseSensitivity cs;

    void updateSinglePath(const QString &pluginDir);
#endif
};

#if QT_CONFIG(library)

static Q_LOGGING_CATEGORY_WITH_ENV_OVERRIDE(lcFactoryLoader, "QT_DEBUG_PLUGINS",
                                            "qt.core.plugin.factoryloader")

Q_GLOBAL_STATIC(QList<QFactoryLoader *>, qt_factory_loaders)

Q_GLOBAL_STATIC(QRecursiveMutex, qt_factoryloader_mutex)

QFactoryLoaderPrivate::~QFactoryLoaderPrivate()
{
    for (QLibraryPrivate *library : qAsConst(libraryList))
        library->release();
}

inline void QFactoryLoaderPrivate::updateSinglePath(const QString &path)
{
    // If we've already loaded, skip it...
    if (loadedPaths.hasSeen(path))
        return;

    qCDebug(lcFactoryLoader) << "checking directory path" << path << "...";

    if (!QDir(path).exists(QLatin1String(".")))
        return;

    QStringList plugins = QDir(path).entryList(
#if defined(Q_OS_WIN)
                QStringList(QStringLiteral("*.dll")),
#elif defined(Q_OS_ANDROID)
                QStringList(QLatin1String("libplugins_%1_*.so").arg(suffix)),
#endif
                QDir::Files);

    for (int j = 0; j < plugins.count(); ++j) {
        QString fileName = QDir::cleanPath(path + QLatin1Char('/') + plugins.at(j));
#ifdef Q_OS_MAC
        const bool isDebugPlugin = fileName.endsWith(QLatin1String("_debug.dylib"));
        const bool isDebugLibrary =
            #ifdef QT_DEBUG
                true;
            #else
                false;
            #endif

        // Skip mismatching plugins so that we don't end up loading both debug and release
        // versions of the same Qt libraries (due to the plugin's dependencies).
        if (isDebugPlugin != isDebugLibrary)
            continue;
#elif defined(Q_PROCESSOR_X86)
        if (fileName.endsWith(QLatin1String(".avx2")) || fileName.endsWith(QLatin1String(".avx512"))) {
            // ignore AVX2-optimized file, we'll do a bait-and-switch to it later
            continue;
        }
#endif
        qCDebug(lcFactoryLoader) << "looking at" << fileName;

        Q_TRACE(QFactoryLoader_update, fileName);

        QLibraryPrivate *library = QLibraryPrivate::findOrCreate(QFileInfo(fileName).canonicalFilePath());
        if (!library->isPlugin()) {
            qCDebug(lcFactoryLoader) << library->errorString << Qt::endl
                                     << "         not a plugin";
            library->release();
            continue;
        }

        QStringList keys;
        bool metaDataOk = false;

        QString iid = library->metaData.value(QtPluginMetaDataKeys::IID).toString();
        if (iid == QLatin1String(this->iid.constData(), this->iid.size())) {
            QCborMap object = library->metaData.value(QtPluginMetaDataKeys::MetaData).toMap();
            metaDataOk = true;

            QCborArray k = object.value(QLatin1String("Keys")).toArray();
            for (int i = 0; i < k.size(); ++i)
                keys += cs ? k.at(i).toString() : k.at(i).toString().toLower();
        }
        qCDebug(lcFactoryLoader) << "Got keys from plugin meta data" << keys;

        if (!metaDataOk) {
            library->release();
            continue;
        }

        int keyUsageCount = 0;
        for (int k = 0; k < keys.count(); ++k) {
            // first come first serve, unless the first
            // library was built with a future Qt version,
            // whereas the new one has a Qt version that fits
            // better
            constexpr int QtVersionNoPatch = QT_VERSION_CHECK(QT_VERSION_MAJOR, QT_VERSION_MINOR, 0);
            const QString &key = keys.at(k);
            QLibraryPrivate *previous = keyMap.value(key);
            int prev_qt_version = 0;
            if (previous)
                prev_qt_version = int(previous->metaData.value(QtPluginMetaDataKeys::QtVersion).toInteger());
            int qt_version = int(library->metaData.value(QtPluginMetaDataKeys::QtVersion).toInteger());
            if (!previous || (prev_qt_version > QtVersionNoPatch && qt_version <= QtVersionNoPatch)) {
                keyMap[key] = library;
                ++keyUsageCount;
            }
        }
        if (keyUsageCount || keys.isEmpty()) {
            library->setLoadHints(QLibrary::PreventUnloadHint); // once loaded, don't unload
            QMutexLocker locker(&mutex);
            libraryList += library;
        } else {
            library->release();
        }
    };
}

void QFactoryLoader::update()
{
#ifdef QT_SHARED
    Q_D(QFactoryLoader);

    QStringList paths = QCoreApplication::libraryPaths();
    for (int i = 0; i < paths.count(); ++i) {
        const QString &pluginDir = paths.at(i);
#ifdef Q_OS_ANDROID
        QString path = pluginDir;
#else
        QString path = pluginDir + d->suffix;
#endif

        d->updateSinglePath(path);
    }
#else
    Q_D(QFactoryLoader);
    qCDebug(lcFactoryLoader) << "ignoring" << d->iid
                             << "since plugins are disabled in static builds";
#endif
}

QFactoryLoader::~QFactoryLoader()
{
    QMutexLocker locker(qt_factoryloader_mutex());
    if (qt_factory_loaders.exists())
        qt_factory_loaders()->removeAll(this);
}

#if defined(Q_OS_UNIX) && !defined (Q_OS_MAC)
QLibraryPrivate *QFactoryLoader::library(const QString &key) const
{
    Q_D(const QFactoryLoader);
    return d->keyMap.value(d->cs ? key : key.toLower());
}
#endif

void QFactoryLoader::refreshAll()
{
    QMutexLocker locker(qt_factoryloader_mutex());
    QList<QFactoryLoader *> *loaders = qt_factory_loaders();
    for (QList<QFactoryLoader *>::const_iterator it = loaders->constBegin();
         it != loaders->constEnd(); ++it) {
        (*it)->update();
    }
}

#endif // QT_CONFIG(library)

QFactoryLoader::QFactoryLoader(const char *iid,
                               const QString &suffix,
                               Qt::CaseSensitivity cs)
    : QObject(*new QFactoryLoaderPrivate)
{
    moveToThread(QCoreApplicationPrivate::mainThread());
    Q_D(QFactoryLoader);
    d->iid = iid;
#if QT_CONFIG(library)
    d->cs = cs;
    d->suffix = suffix;
# ifdef Q_OS_ANDROID
    if (!d->suffix.isEmpty() && d->suffix.at(0) == QLatin1Char('/'))
        d->suffix.remove(0, 1);
# endif

    QMutexLocker locker(qt_factoryloader_mutex());
    update();
    qt_factory_loaders()->append(this);
#else
    Q_UNUSED(suffix);
    Q_UNUSED(cs);
#endif
}

QFactoryLoader::MetaDataList QFactoryLoader::metaData() const
{
    Q_D(const QFactoryLoader);
    QList<QPluginParsedMetaData> metaData;
#if QT_CONFIG(library)
    QMutexLocker locker(&d->mutex);
    for (int i = 0; i < d->libraryList.size(); ++i)
        metaData.append(d->libraryList.at(i)->metaData);
#endif

    QLatin1String iid(d->iid.constData(), d->iid.size());
    const auto staticPlugins = QPluginLoader::staticPlugins();
    for (const QStaticPlugin &plugin : staticPlugins) {
        QByteArrayView pluginData(static_cast<const char *>(plugin.rawMetaData), plugin.rawMetaDataSize);
        QPluginParsedMetaData parsed(pluginData);
        if (parsed.isError() || parsed.value(QtPluginMetaDataKeys::IID) != iid)
            continue;
        metaData.append(std::move(parsed));
    }
    return metaData;
}

QObject *QFactoryLoader::instance(int index) const
{
    Q_D(const QFactoryLoader);
    if (index < 0)
        return nullptr;

#if QT_CONFIG(library)
    QMutexLocker lock(&d->mutex);
    if (index < d->libraryList.size()) {
        QLibraryPrivate *library = d->libraryList.at(index);
        if (QObject *obj = library->pluginInstance()) {
            if (!obj->parent())
                obj->moveToThread(QCoreApplicationPrivate::mainThread());
            return obj;
        }
        return nullptr;
    }
    index -= d->libraryList.size();
    lock.unlock();
#endif

    QLatin1String iid(d->iid.constData(), d->iid.size());
    const QList<QStaticPlugin> staticPlugins = QPluginLoader::staticPlugins();
    for (QStaticPlugin plugin : staticPlugins) {
        QByteArrayView pluginData(static_cast<const char *>(plugin.rawMetaData), plugin.rawMetaDataSize);
        QPluginParsedMetaData parsed(pluginData);
        if (parsed.isError() || parsed.value(QtPluginMetaDataKeys::IID) != iid)
            continue;

        if (index == 0)
            return plugin.instance();
        --index;
    }

    return nullptr;
}

QMultiMap<int, QString> QFactoryLoader::keyMap() const
{
    QMultiMap<int, QString> result;
    const QList<QPluginParsedMetaData> metaDataList = metaData();
    for (int i = 0; i < metaDataList.size(); ++i) {
        const QCborMap metaData = metaDataList.at(i).value(QtPluginMetaDataKeys::MetaData).toMap();
        const QCborArray keys = metaData.value(QLatin1String("Keys")).toArray();
        const int keyCount = keys.size();
        for (int k = 0; k < keyCount; ++k)
            result.insert(i, keys.at(k).toString());
    }
    return result;
}

int QFactoryLoader::indexOf(const QString &needle) const
{
    const QList<QPluginParsedMetaData> metaDataList = metaData();
    for (int i = 0; i < metaDataList.size(); ++i) {
        const QCborMap metaData = metaDataList.at(i).value(QtPluginMetaDataKeys::MetaData).toMap();
        const QCborArray keys = metaData.value(QLatin1String("Keys")).toArray();
        const int keyCount = keys.size();
        for (int k = 0; k < keyCount; ++k) {
            if (!keys.at(k).toString().compare(needle, Qt::CaseInsensitive))
                return i;
        }
    }
    return -1;
}

QT_END_NAMESPACE

#include "moc_qfactoryloader_p.cpp"

#endif // QT_NO_QOBJECT
