// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qimagereaderwriterhelpers_p.h"

#include <qcborarray.h>
#include <qmutex.h>
#include <private/qfactoryloader_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QImageReaderWriterHelpers {

#ifndef QT_NO_IMAGEFORMATPLUGIN

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, irhLoader,
                          (QImageIOHandlerFactoryInterface_iid, "/imageformats"_L1))
Q_GLOBAL_STATIC(QMutex, irhLoaderMutex)

static void appendImagePluginFormats(QFactoryLoader *loader,
                                     QImageIOPlugin::Capability cap,
                                     QList<QByteArray> *result)
{
    typedef QMultiMap<int, QString> PluginKeyMap;
    typedef PluginKeyMap::const_iterator PluginKeyMapConstIterator;

    const PluginKeyMap keyMap = loader->keyMap();
    const PluginKeyMapConstIterator cend = keyMap.constEnd();
    int i = -1;
    QImageIOPlugin *plugin = nullptr;
    result->reserve(result->size() + keyMap.size());
    for (PluginKeyMapConstIterator it = keyMap.constBegin(); it != cend; ++it) {
        if (it.key() != i) {
            i = it.key();
            plugin = qobject_cast<QImageIOPlugin *>(loader->instance(i));
        }
        const QByteArray key = it.value().toLatin1();
        if (plugin && (plugin->capabilities(nullptr, key) & cap) != 0)
            result->append(key);
    }
}

static void appendImagePluginMimeTypes(QFactoryLoader *loader,
                                       QImageIOPlugin::Capability cap,
                                       QList<QByteArray> *result,
                                       QList<QByteArray> *resultKeys = nullptr)
{
    QList<QPluginParsedMetaData> metaDataList = loader->metaData();
    const int pluginCount = metaDataList.size();
    for (int i = 0; i < pluginCount; ++i) {
        const QCborMap metaData = metaDataList.at(i).value(QtPluginMetaDataKeys::MetaData).toMap();
        const QCborArray keys = metaData.value("Keys"_L1).toArray();
        const QCborArray mimeTypes = metaData.value("MimeTypes"_L1).toArray();
        QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(loader->instance(i));
        const int keyCount = keys.size();
        for (int k = 0; k < keyCount; ++k) {
            const QByteArray key = keys.at(k).toString().toLatin1();
            if (plugin && (plugin->capabilities(nullptr, key) & cap) != 0) {
                result->append(mimeTypes.at(k).toString().toLatin1());
                if (resultKeys)
                    resultKeys->append(key);
            }
        }
    }
}

QSharedPointer<QFactoryLoader> pluginLoader()
{
    irhLoaderMutex()->lock();
    return QSharedPointer<QFactoryLoader>(irhLoader(), [](QFactoryLoader *) {
        irhLoaderMutex()->unlock();
    });
}

static inline QImageIOPlugin::Capability pluginCapability(Capability cap)
{
    return cap == CanRead ? QImageIOPlugin::CanRead : QImageIOPlugin::CanWrite;
}

#endif // QT_NO_IMAGEFORMATPLUGIN

QList<QByteArray> supportedImageFormats(Capability cap)
{
    QList<QByteArray> formats;
    formats.reserve(_qt_NumFormats);
    for (int i = 0; i < _qt_NumFormats; ++i)
        formats << _qt_BuiltInFormats[i].extension;

#ifndef QT_NO_IMAGEFORMATPLUGIN
    appendImagePluginFormats(irhLoader(), pluginCapability(cap), &formats);
#endif // QT_NO_IMAGEFORMATPLUGIN

    std::sort(formats.begin(), formats.end());
    formats.erase(std::unique(formats.begin(), formats.end()), formats.end());
    return formats;
}

static constexpr QByteArrayView imagePrefix() noexcept { return "image/"; }

QList<QByteArray> supportedMimeTypes(Capability cap)
{
    QList<QByteArray> mimeTypes;
    mimeTypes.reserve(_qt_NumFormats);
    for (const auto &fmt : _qt_BuiltInFormats)
        mimeTypes.emplace_back(imagePrefix() + fmt.mimeType);

#ifndef QT_NO_IMAGEFORMATPLUGIN
    appendImagePluginMimeTypes(irhLoader(), pluginCapability(cap), &mimeTypes);
#endif // QT_NO_IMAGEFORMATPLUGIN

    std::sort(mimeTypes.begin(), mimeTypes.end());
    mimeTypes.erase(std::unique(mimeTypes.begin(), mimeTypes.end()), mimeTypes.end());
    return mimeTypes;
}

QList<QByteArray> imageFormatsForMimeType(QByteArrayView mimeType, Capability cap)
{
    QList<QByteArray> formats;
    if (mimeType.startsWith(imagePrefix())) {
        const QByteArrayView type = mimeType.mid(imagePrefix().size());
        for (const auto &fmt : _qt_BuiltInFormats) {
            if (fmt.mimeType == type && !formats.contains(fmt.extension))
                formats << fmt.extension;
        }
    }

#ifndef QT_NO_IMAGEFORMATPLUGIN
    QList<QByteArray> mimeTypes;
    QList<QByteArray> keys;
    appendImagePluginMimeTypes(irhLoader(), pluginCapability(cap), &mimeTypes, &keys);
    for (int i = 0; i < mimeTypes.size(); ++i) {
        if (mimeTypes.at(i) == mimeType) {
            const auto &key = keys.at(i);
            if (!formats.contains(key))
                formats << key;
        }
    }
#endif // QT_NO_IMAGEFORMATPLUGIN

    return formats;
}

} // QImageReaderWriterHelpers

QT_END_NAMESPACE
