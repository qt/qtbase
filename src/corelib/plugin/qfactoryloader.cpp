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
#include "qmap.h"
#include <qdir.h>
#include <qdebug.h>
#include "qmutex.h"
#include "qplugin.h"
#include "qplugin_p.h"
#include "qpluginloader.h"
#include "private/qobject_p.h"
#include "private/qcoreapplication_p.h"
#include "qcbormap.h"
#include "qcborvalue.h"
#include "qjsondocument.h"
#include "qjsonvalue.h"
#include "qjsonobject.h"
#include "qjsonarray.h"

#include <qtcore_tracepoints_p.h>

QT_BEGIN_NAMESPACE

static inline int metaDataSignatureLength()
{
    return sizeof("QTMETADATA  ") - 1;
}

static QJsonDocument jsonFromCborMetaData(const char *raw, qsizetype size, QString *errMsg)
{
    // extract the keys not stored in CBOR
    int qt_metadataVersion = quint8(raw[0]);
    int qt_version = qFromBigEndian<quint16>(raw + 1);
    int qt_archRequirements = quint8(raw[3]);
    if (Q_UNLIKELY(raw[-1] != '!' || qt_metadataVersion != 0)) {
        *errMsg = QStringLiteral("Invalid metadata version");
        return QJsonDocument();
    }

    raw += 4;
    size -= 4;
    QByteArray ba = QByteArray::fromRawData(raw, int(size));
    QCborParserError err;
    QCborValue metadata = QCborValue::fromCbor(ba, &err);

    if (err.error != QCborError::NoError) {
        *errMsg = QLatin1String("Metadata parsing error: ") + err.error.toString();
        return QJsonDocument();
    }

    if (!metadata.isMap()) {
        *errMsg = QStringLiteral("Unexpected metadata contents");
        return QJsonDocument();
    }

    QJsonObject o;
    o.insert(QLatin1String("version"), qt_version << 8);
    o.insert(QLatin1String("debug"), bool(qt_archRequirements & 1));
    o.insert(QLatin1String("archreq"), qt_archRequirements);

    // convert the top-level map integer keys
    for (auto it : metadata.toMap()) {
        QString key;
        if (it.first.isInteger()) {
            switch (it.first.toInteger()) {
#define CONVERT_TO_STRING(IntKey, StringKey, Description) \
            case int(IntKey): key = QStringLiteral(StringKey); break;
                QT_PLUGIN_FOREACH_METADATA(CONVERT_TO_STRING)
#undef CONVERT_TO_STRING

            case int(QtPluginMetaDataKeys::Requirements):
                // special case: recreate the debug key
                o.insert(QLatin1String("debug"), bool(it.second.toInteger() & 1));
                key = QStringLiteral("archreq");
                break;
            }
        } else {
            key = it.first.toString();
        }

        if (!key.isEmpty())
            o.insert(key, it.second.toJsonValue());
    }
    return QJsonDocument(o);
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QJsonDocument qJsonFromRawLibraryMetaData(const char *raw, qsizetype sectionSize, QString *errMsg)
{
    raw += metaDataSignatureLength();
    sectionSize -= metaDataSignatureLength();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (Q_UNLIKELY(raw[-1] == ' ')) {
        // the size of the embedded JSON object can be found 8 bytes into the data (see qjson_p.h)
        uint size = qFromLittleEndian<uint>(raw + 8);
        // but the maximum size of binary JSON is 128 MB
        size = qMin(size, 128U * 1024 * 1024);
        // and it doesn't include the size of the header (8 bytes)
        size += 8;
        // finally, it can't be bigger than the file or section size
        size = qMin(sectionSize, qsizetype(size));

        QByteArray json(raw, size);
        return QJsonDocument::fromBinaryData(json);
    }
#endif

    return jsonFromCborMetaData(raw, sectionSize, errMsg);
}
QT_WARNING_POP

class QFactoryLoaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QFactoryLoader)
public:
    QFactoryLoaderPrivate(){}
    QByteArray iid;
#if QT_CONFIG(library)
    ~QFactoryLoaderPrivate();
    mutable QMutex mutex;
    QList<QLibraryPrivate*> libraryList;
    QMap<QString,QLibraryPrivate*> keyMap;
    QString suffix;
    Qt::CaseSensitivity cs;
    QStringList loadedPaths;
#endif
};

#if QT_CONFIG(library)

Q_GLOBAL_STATIC(QList<QFactoryLoader *>, qt_factory_loaders)

Q_GLOBAL_STATIC(QRecursiveMutex, qt_factoryloader_mutex)

QFactoryLoaderPrivate::~QFactoryLoaderPrivate()
{
    for (int i = 0; i < libraryList.count(); ++i) {
        QLibraryPrivate *library = libraryList.at(i);
        library->unload();
        library->release();
    }
}

void QFactoryLoader::update()
{
#ifdef QT_SHARED
    Q_D(QFactoryLoader);
    QStringList paths = QCoreApplication::libraryPaths();
    for (int i = 0; i < paths.count(); ++i) {
        const QString &pluginDir = paths.at(i);
        // Already loaded, skip it...
        if (d->loadedPaths.contains(pluginDir))
            continue;
        d->loadedPaths << pluginDir;

#ifdef Q_OS_ANDROID
        QString path = pluginDir;
#else
        QString path = pluginDir + d->suffix;
#endif

        if (qt_debug_component())
            qDebug() << "QFactoryLoader::QFactoryLoader() checking directory path" << path << "...";

        if (!QDir(path).exists(QLatin1String(".")))
            continue;

        QStringList plugins = QDir(path).entryList(
#if defined(Q_OS_WIN)
                    QStringList(QStringLiteral("*.dll")),
#elif defined(Q_OS_ANDROID)
                    QStringList(QLatin1String("libplugins_%1_*.so").arg(d->suffix)),
#endif
                    QDir::Files);
        QLibraryPrivate *library = nullptr;

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
            if (qt_debug_component()) {
                qDebug() << "QFactoryLoader::QFactoryLoader() looking at" << fileName;
            }

            Q_TRACE(QFactoryLoader_update, fileName);

            library = QLibraryPrivate::findOrCreate(QFileInfo(fileName).canonicalFilePath());
            if (!library->isPlugin()) {
                if (qt_debug_component()) {
                    qDebug() << library->errorString << Qt::endl
                             << "         not a plugin";
                }
                library->release();
                continue;
            }

            QStringList keys;
            bool metaDataOk = false;

            QString iid = library->metaData.value(QLatin1String("IID")).toString();
            if (iid == QLatin1String(d->iid.constData(), d->iid.size())) {
                QJsonObject object = library->metaData.value(QLatin1String("MetaData")).toObject();
                metaDataOk = true;

                QJsonArray k = object.value(QLatin1String("Keys")).toArray();
                for (int i = 0; i < k.size(); ++i)
                    keys += d->cs ? k.at(i).toString() : k.at(i).toString().toLower();
            }
            if (qt_debug_component())
                qDebug() << "Got keys from plugin meta data" << keys;


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
                const QString &key = keys.at(k);
                QLibraryPrivate *previous = d->keyMap.value(key);
                int prev_qt_version = 0;
                if (previous) {
                    prev_qt_version = (int)previous->metaData.value(QLatin1String("version")).toDouble();
                }
                int qt_version = (int)library->metaData.value(QLatin1String("version")).toDouble();
                if (!previous || (prev_qt_version > QT_VERSION && qt_version <= QT_VERSION)) {
                    d->keyMap[key] = library;
                    ++keyUsageCount;
                }
            }
            if (keyUsageCount || keys.isEmpty()) {
                library->setLoadHints(QLibrary::PreventUnloadHint); // once loaded, don't unload
                QMutexLocker locker(&d->mutex);
                d->libraryList += library;
            } else {
                library->release();
            }
        }
    }
#else
    Q_D(QFactoryLoader);
    if (qt_debug_component()) {
        qDebug() << "QFactoryLoader::QFactoryLoader() ignoring" << d->iid
                 << "since plugins are disabled in static builds";
    }
#endif
}

QFactoryLoader::~QFactoryLoader()
{
    QMutexLocker locker(qt_factoryloader_mutex());
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

QList<QJsonObject> QFactoryLoader::metaData() const
{
    Q_D(const QFactoryLoader);
    QList<QJsonObject> metaData;
#if QT_CONFIG(library)
    QMutexLocker locker(&d->mutex);
    for (int i = 0; i < d->libraryList.size(); ++i)
        metaData.append(d->libraryList.at(i)->metaData);
#endif

    const auto staticPlugins = QPluginLoader::staticPlugins();
    for (const QStaticPlugin &plugin : staticPlugins) {
        const QJsonObject object = plugin.metaData();
        if (object.value(QLatin1String("IID")) != QLatin1String(d->iid.constData(), d->iid.size()))
            continue;
        metaData.append(object);
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

    QVector<QStaticPlugin> staticPlugins = QPluginLoader::staticPlugins();
    for (int i = 0; i < staticPlugins.count(); ++i) {
        const QJsonObject object = staticPlugins.at(i).metaData();
        if (object.value(QLatin1String("IID")) != QLatin1String(d->iid.constData(), d->iid.size()))
            continue;

        if (index == 0)
            return staticPlugins.at(i).instance();
        --index;
    }

    return nullptr;
}

QMultiMap<int, QString> QFactoryLoader::keyMap() const
{
    QMultiMap<int, QString> result;
    const QList<QJsonObject> metaDataList = metaData();
    for (int i = 0; i < metaDataList.size(); ++i) {
        const QJsonObject metaData = metaDataList.at(i).value(QLatin1String("MetaData")).toObject();
        const QJsonArray keys = metaData.value(QLatin1String("Keys")).toArray();
        const int keyCount = keys.size();
        for (int k = 0; k < keyCount; ++k)
            result.insert(i, keys.at(k).toString());
    }
    return result;
}

int QFactoryLoader::indexOf(const QString &needle) const
{
    const QList<QJsonObject> metaDataList = metaData();
    for (int i = 0; i < metaDataList.size(); ++i) {
        const QJsonObject metaData = metaDataList.at(i).value(QLatin1String("MetaData")).toObject();
        const QJsonArray keys = metaData.value(QLatin1String("Keys")).toArray();
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
