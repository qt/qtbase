// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFACTORYLOADER_P_H
#define QFACTORYLOADER_P_H

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

#include "QtCore/qglobal.h"
#ifndef QT_NO_QOBJECT

#include "QtCore/private/qplugin_p.h"
#include "QtCore/qcbormap.h"
#include "QtCore/qcborvalue.h"
#include "QtCore/qmap.h"
#include "QtCore/qobject.h"
#include "QtCore/qplugin.h"

QT_BEGIN_NAMESPACE

class QJsonObject;
class QLibraryPrivate;

class QPluginParsedMetaData
{
    QCborValue data;
    bool setError(const QString &errorString) Q_DECL_COLD_FUNCTION
    {
        data = errorString;
        return false;
    }
public:
    QPluginParsedMetaData() = default;
    QPluginParsedMetaData(QByteArrayView input)     { parse(input); }

    bool isError() const                            { return !data.isMap(); }
    QString errorString() const                     { return data.toString(); }

    bool parse(QByteArrayView input);
    bool parse(QPluginMetaData metaData)
    { return parse(QByteArrayView(reinterpret_cast<const char *>(metaData.data), metaData.size)); }

    QJsonObject toJson() const;     // only for QLibrary & QPluginLoader

    // if data is not a map, toMap() returns empty, so shall these functions
    QCborMap toCbor() const                         { return data.toMap(); }
    QCborValue value(QtPluginMetaDataKeys k) const  { return data[int(k)]; }
};

class QFactoryLoaderPrivate;
class Q_CORE_EXPORT QFactoryLoader : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QFactoryLoader)

public:
    explicit QFactoryLoader(const char *iid,
                   const QString &suffix = QString(),
                   Qt::CaseSensitivity = Qt::CaseSensitive);

#if QT_CONFIG(library)
    ~QFactoryLoader();

    void update();
    static void refreshAll();

#if defined(Q_OS_UNIX) && !defined (Q_OS_DARWIN)
    QLibraryPrivate *library(const QString &key) const;
#endif // Q_OS_UNIX && !Q_OS_DARWIN
#endif // QT_CONFIG(library)

    void setExtraSearchPath(const QString &path);
    QMultiMap<int, QString> keyMap() const;
    int indexOf(const QString &needle) const;

    using MetaDataList = QList<QPluginParsedMetaData>;

    MetaDataList metaData() const;
    QList<QCborArray> metaDataKeys() const;
    QObject *instance(int index) const;
};

template <class PluginInterface, class FactoryInterface, typename ...Args>
PluginInterface *qLoadPlugin(const QFactoryLoader *loader, const QString &key, Args &&...args)
{
    const int index = loader->indexOf(key);
    if (index != -1) {
        QObject *factoryObject = loader->instance(index);
        if (FactoryInterface *factory = qobject_cast<FactoryInterface *>(factoryObject))
            if (PluginInterface *result = factory->create(key, std::forward<Args>(args)...))
                return result;
    }
    return nullptr;
}

template <class PluginInterface, class FactoryInterface, typename Arg>
Q_DECL_DEPRECATED PluginInterface *qLoadPlugin1(const QFactoryLoader *loader, const QString &key, Arg &&arg)
{ return qLoadPlugin<PluginInterface, FactoryInterface>(loader, key, std::forward<Arg>(arg)); }

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QFACTORYLOADER_P_H
