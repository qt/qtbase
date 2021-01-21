/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiaprovidercache.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


// Private constructor
QWindowsUiaProviderCache::QWindowsUiaProviderCache()
{
}

// shared instance
QWindowsUiaProviderCache *QWindowsUiaProviderCache::instance()
{
    static QWindowsUiaProviderCache providerCache;
    return &providerCache;
}

// Returns the provider instance associated with the ID, or nullptr.
QWindowsUiaBaseProvider *QWindowsUiaProviderCache::providerForId(QAccessible::Id id) const
{
    return m_providerTable.value(id);
}

// Inserts a provider in the cache and associates it with an accessibility ID.
void QWindowsUiaProviderCache::insert(QAccessible::Id id, QWindowsUiaBaseProvider *provider)
{
    remove(id);
    if (provider) {
        m_providerTable[id] = provider;
        m_inverseTable[provider] = id;
        // Connects the destroyed signal to our slot, to remove deleted objects from the cache.
        QObject::connect(provider, &QObject::destroyed, this, &QWindowsUiaProviderCache::objectDestroyed);
    }
}

// Removes deleted provider objects from the cache.
void QWindowsUiaProviderCache::objectDestroyed(QObject *obj)
{
    // We have to use the inverse table to map the object address back to its ID,
    // since at this point (called from QObject destructor), it has already been
    // partially destroyed and we cannot treat it as a provider.
    auto it = m_inverseTable.find(obj);
    if (it != m_inverseTable.end()) {
        m_providerTable.remove(*it);
        m_inverseTable.remove(obj);
    }
}

// Removes a provider with a given id from the cache.
void QWindowsUiaProviderCache::remove(QAccessible::Id id)
{
    m_inverseTable.remove(m_providerTable.value(id));
    m_providerTable.remove(id);
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
