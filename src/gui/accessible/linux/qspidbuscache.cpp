// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qspidbuscache_p.h"
#include "qspiaccessiblebridge_p.h"

#if QT_CONFIG(accessibility)
#include "cache_adaptor.h"

#define QSPI_OBJECT_PATH_CACHE "/org/a11y/atspi/cache"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QSpiDBusCache
    \internal
    \brief This class is responsible for the AT-SPI cache interface.

    The idea behind the cache is that starting an application would
    result in many dbus calls. The way GTK/Gail/ATK work is that
    they create accessibles for all objects on startup.
    In order to avoid querying all the objects individually via DBus
    they get sent by using the GetItems call of the cache.

    Additionally the AddAccessible and RemoveAccessible signals
    are responsible for adding/removing objects from the cache.

    Currently the Qt bridge chooses to ignore these.
*/

QSpiDBusCache::QSpiDBusCache(QDBusConnection c, QObject* parent)
    : QObject(parent)
{
    new CacheAdaptor(this);
    c.registerObject(QSPI_OBJECT_PATH_CACHE ""_L1, this, QDBusConnection::ExportAdaptors);
}

void QSpiDBusCache::emitAddAccessible(const QSpiAccessibleCacheItem& item)
{
    emit AddAccessible(item);
}

void QSpiDBusCache::emitRemoveAccessible(const QSpiObjectReference& item)
{
    emit RemoveAccessible(item);
}

QSpiAccessibleCacheArray QSpiDBusCache::GetItems()
{
    return QSpiAccessibleCacheArray();
}

QT_END_NAMESPACE

#include "moc_qspidbuscache_p.cpp"
#endif // QT_CONFIG(accessibility)
