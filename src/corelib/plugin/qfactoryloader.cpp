/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfactoryloader_p.h"

#ifndef QT_NO_LIBRARY
#include "qfactoryinterface.h"
#include "qmap.h"
#include <qdir.h>
#include <qdebug.h>
#include "qmutex.h"
#include "qplugin.h"
#include "qpluginloader.h"
#include "private/qobject_p.h"
#include "private/qcoreapplication_p.h"
#include "qjsondocument.h"
#include "qjsonvalue.h"
#include "qjsonobject.h"
#include "qjsonarray.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QList<QFactoryLoader *>, qt_factory_loaders)

Q_GLOBAL_STATIC_WITH_ARGS(QMutex, qt_factoryloader_mutex, (QMutex::Recursive))

class QFactoryLoaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QFactoryLoader)
public:
    QFactoryLoaderPrivate(){}
    ~QFactoryLoaderPrivate();
    mutable QMutex mutex;
    QByteArray iid;
    QList<QLibraryPrivate*> libraryList;
    QMap<QString,QLibraryPrivate*> keyMap;
    QStringList keyList;
    QString suffix;
    Qt::CaseSensitivity cs;
    QStringList loadedPaths;

    void unloadPath(const QString &path);
};

QFactoryLoaderPrivate::~QFactoryLoaderPrivate()
{
    for (int i = 0; i < libraryList.count(); ++i) {
        QLibraryPrivate *library = libraryList.at(i);
        library->unload();
        library->release();
    }
}

QFactoryLoader::QFactoryLoader(const char *iid,
                               const QString &suffix,
                               Qt::CaseSensitivity cs)
    : QObject(*new QFactoryLoaderPrivate)
{
    moveToThread(QCoreApplicationPrivate::mainThread());
    Q_D(QFactoryLoader);
    d->iid = iid;
    d->cs = cs;
    d->suffix = suffix;


    QMutexLocker locker(qt_factoryloader_mutex());
    update();
    qt_factory_loaders()->append(this);
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

        QString path = pluginDir + d->suffix;
        if (!QDir(path).exists(QLatin1String(".")))
            continue;

        QStringList plugins = QDir(path).entryList(QDir::Files);
        QLibraryPrivate *library = 0;
        for (int j = 0; j < plugins.count(); ++j) {
            QString fileName = QDir::cleanPath(path + QLatin1Char('/') + plugins.at(j));

            if (qt_debug_component()) {
                qDebug() << "QFactoryLoader::QFactoryLoader() looking at" << fileName;
            }
            library = QLibraryPrivate::findOrCreate(QFileInfo(fileName).canonicalFilePath());
            if (!library->isPlugin()) {
                if (qt_debug_component()) {
                    qDebug() << library->errorString;
                    qDebug() << "         not a plugin";
                }
                library->release();
                continue;
            }

            QStringList keys;
            bool metaDataOk = false;
            if (library->compatPlugin) {
                qWarning("Qt plugin loader: Compatibility plugin '%s', need to load for accessing meta data.",
                         qPrintable(QDir::toNativeSeparators(fileName)));
                if (!library->loadPlugin()) {
                    if (qt_debug_component()) {
                        qDebug() << library->errorString;
                        qDebug() << "           could not load";
                    }
                    library->release();
                    continue;
                }

                if (!library->inst)
                    library->inst = library->instance();
                QObject *instance = library->inst.data();
                if (!instance) {
                    library->release();
                    // ignore plugins that have a valid signature but cannot be loaded.
                    continue;
                }
                QFactoryInterface *factory = qobject_cast<QFactoryInterface*>(instance);
                if (instance && factory && instance->qt_metacast(d->iid))
                    keys = factory->keys();

                if (!keys.isEmpty())
                    metaDataOk = true;

            } else {
                QString iid = library->metaData.value(QLatin1String("IID")).toString();
                if (iid == QLatin1String(d->iid.constData(), d->iid.size())) {
                    QJsonObject object = library->metaData.value(QLatin1String("MetaData")).toObject();
                    metaDataOk = true;

                    QJsonArray k = object.value(QLatin1String("Keys")).toArray();
                    for (int i = 0; i < k.size(); ++i) {
                        QString s = k.at(i).toString();
                        keys += s;
                    }
                }
                if (qt_debug_component())
                    qDebug() << "Got keys from plugin meta data" << keys;
            }

            if (!metaDataOk) {
                if (library->compatPlugin)
                    library->unload();
                library->release();
                continue;
            }

            d->libraryList += library;
            for (int k = 0; k < keys.count(); ++k) {
                // first come first serve, unless the first
                // library was built with a future Qt version,
                // whereas the new one has a Qt version that fits
                // better
                QString key = keys.at(k);
                if (!d->cs)
                    key = key.toLower();
                QLibraryPrivate *previous = d->keyMap.value(key);
                int prev_qt_version = 0;
                if (previous) {
                    prev_qt_version = (int)previous->metaData.value(QLatin1String("version")).toDouble();
                }
                int qt_version = (int)library->metaData.value(QLatin1String("version")).toDouble();
                if (!previous || (prev_qt_version > QT_VERSION && qt_version <= QT_VERSION)) {
                    d->keyMap[key] = library;
                    d->keyList += keys.at(k);
                }
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

QStringList QFactoryLoader::keys() const
{
    Q_D(const QFactoryLoader);
    QMutexLocker locker(&d->mutex);
    QStringList keys = d->keyList;
    QVector<QStaticPlugin> staticPlugins = QLibraryPrivate::staticPlugins();
    for (int i = 0; i < staticPlugins.count(); ++i) {
        if (staticPlugins.at(i).metaData) {
            const char *rawMetaData = staticPlugins.at(i).metaData();
            QJsonObject object = QLibraryPrivate::fromRawMetaData(rawMetaData).object();
            if (object.value(QLatin1String("IID")) != QLatin1String(d->iid.constData(), d->iid.size()))
                continue;

            QJsonObject meta = object.value(QLatin1String("MetaData")).toObject();
            QJsonArray a = meta.value(QLatin1String("Keys")).toArray();
            for (int i = 0; i < a.size(); ++i) {
                QString s = a.at(i).toString();
                if (!s.isEmpty())
                    keys += s;
            }
        } else {
            // compat plugin
            QObject *instance = staticPlugins.at(i).instance();
            QFactoryInterface *factory = qobject_cast<QFactoryInterface*>(instance);
            if (instance && factory && instance->qt_metacast(d->iid))
                keys += factory->keys();
        }
    }
    return keys;
}

QList<QJsonObject> QFactoryLoader::metaData() const
{
    Q_D(const QFactoryLoader);
    QMutexLocker locker(&d->mutex);
    QList<QJsonObject> metaData;
    for (int i = 0; i < d->libraryList.size(); ++i)
        metaData.append(d->libraryList.at(i)->metaData);

    QVector<QStaticPlugin> staticPlugins = QLibraryPrivate::staticPlugins();
    for (int i = 0; i < staticPlugins.count(); ++i) {
        if (staticPlugins.at(i).metaData) {
            const char *rawMetaData = staticPlugins.at(i).metaData();
            QJsonObject object = QLibraryPrivate::fromRawMetaData(rawMetaData).object();
            if (object.value(QLatin1String("IID")) != QLatin1String(d->iid.constData(), d->iid.size()))
                continue;

            QJsonObject meta = object.value(QLatin1String("MetaData")).toObject();
            metaData.append(meta);
        } else {
            // compat plugins
            QObject *instance = staticPlugins.at(i).instance();
            QFactoryInterface *factory = qobject_cast<QFactoryInterface*>(instance);
            if (instance && factory && instance->qt_metacast(d->iid)) {
                QJsonObject meta;
                QJsonArray a = QJsonArray::fromStringList(factory->keys());
                meta.insert(QLatin1String("Keys"), a);
                metaData.append(meta);
            }
        }
    }
    return metaData;
}

QObject *QFactoryLoader::instance(const QString &key) const
{
    Q_D(const QFactoryLoader);
    QMutexLocker locker(&d->mutex);
    QVector<QStaticPlugin> staticPlugins = QLibraryPrivate::staticPlugins();
    for (int i = 0; i < staticPlugins.count(); ++i) {
        QObject *instance = staticPlugins.at(i).instance();
        if (QFactoryInterface *factory = qobject_cast<QFactoryInterface*>(instance))
            if (instance->qt_metacast(d->iid) && factory->keys().contains(key, Qt::CaseInsensitive))
                return instance;
    }

    QString lowered = d->cs ? key : key.toLower();
    if (QLibraryPrivate* library = d->keyMap.value(lowered)) {
        if (library->instance || library->loadPlugin()) {
            if (!library->inst)
                library->inst = library->instance();
            QObject *obj = library->inst.data();
            if (obj) {
                if (!obj->parent())
                    obj->moveToThread(QCoreApplicationPrivate::mainThread());
                return obj;
            }
        }
    }
    return 0;
}

QObject *QFactoryLoader::instance(int index) const
{
    Q_D(const QFactoryLoader);
    if (index < 0 || index >= d->libraryList.size())
        return 0;

    QLibraryPrivate *library = d->libraryList.at(index);
    if (library->instance || library->loadPlugin()) {
        if (!library->inst)
            library->inst = library->instance();
        QObject *obj = library->inst.data();
        if (obj) {
            if (!obj->parent())
                obj->moveToThread(QCoreApplicationPrivate::mainThread());
            return obj;
        }
    }
    return 0;
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

QT_END_NAMESPACE

#endif // QT_NO_LIBRARY
