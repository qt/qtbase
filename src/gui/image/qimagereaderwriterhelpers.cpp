/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "private/qimagereaderwriterhelpers_p.h"

#include <qjsonarray.h>
#include <qmutex.h>
#include <private/qfactoryloader_p.h>

QT_BEGIN_NAMESPACE

namespace QImageReaderWriterHelpers {

#ifndef QT_NO_IMAGEFORMATPLUGIN

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QImageIOHandlerFactoryInterface_iid, QLatin1String("/imageformats")))
Q_GLOBAL_STATIC(QMutex, loaderMutex)

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
    QList<QJsonObject> metaDataList = loader->metaData();

    const int pluginCount = metaDataList.size();
    for (int i = 0; i < pluginCount; ++i) {
        const QJsonObject metaData = metaDataList.at(i).value(QLatin1String("MetaData")).toObject();
        const QJsonArray keys = metaData.value(QLatin1String("Keys")).toArray();
        const QJsonArray mimeTypes = metaData.value(QLatin1String("MimeTypes")).toArray();
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
    loaderMutex()->lock();
    return QSharedPointer<QFactoryLoader>(loader(), [](QFactoryLoader *) {
        loaderMutex()->unlock();
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
    appendImagePluginFormats(loader(), pluginCapability(cap), &formats);
#endif // QT_NO_IMAGEFORMATPLUGIN

    std::sort(formats.begin(), formats.end());
    formats.erase(std::unique(formats.begin(), formats.end()), formats.end());
    return formats;
}

QList<QByteArray> supportedMimeTypes(Capability cap)
{
    QList<QByteArray> mimeTypes;
    mimeTypes.reserve(_qt_NumFormats);
    for (const auto &fmt : _qt_BuiltInFormats)
        mimeTypes.append(QByteArrayLiteral("image/") + fmt.mimeType);

#ifndef QT_NO_IMAGEFORMATPLUGIN
    appendImagePluginMimeTypes(loader(), pluginCapability(cap), &mimeTypes);
#endif // QT_NO_IMAGEFORMATPLUGIN

    std::sort(mimeTypes.begin(), mimeTypes.end());
    mimeTypes.erase(std::unique(mimeTypes.begin(), mimeTypes.end()), mimeTypes.end());
    return mimeTypes;
}

QList<QByteArray> imageFormatsForMimeType(const QByteArray &mimeType, Capability cap)
{
    QList<QByteArray> formats;
    if (mimeType.startsWith("image/")) {
        const QByteArray type = mimeType.mid(sizeof("image/") - 1);
        for (const auto &fmt : _qt_BuiltInFormats) {
            if (fmt.mimeType == type && !formats.contains(fmt.extension))
                formats << fmt.extension;
        }
    }

#ifndef QT_NO_IMAGEFORMATPLUGIN
    QList<QByteArray> mimeTypes;
    QList<QByteArray> keys;
    appendImagePluginMimeTypes(loader(), pluginCapability(cap), &mimeTypes, &keys);
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
