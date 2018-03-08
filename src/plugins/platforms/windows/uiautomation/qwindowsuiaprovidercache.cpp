/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY

#include "qwindowsuiaprovidercache.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtCore/QDebug>

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
    return providerTable.value(id);
}

// Inserts a provider in the cache and associates it with an accessibility ID.
void QWindowsUiaProviderCache::insert(QAccessible::Id id, QWindowsUiaBaseProvider *provider)
{
    remove(id);
    if (provider) {
        providerTable[id] = provider;
        inverseTable[provider] = id;
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
    auto it = inverseTable.find(obj);
    if (it != inverseTable.end()) {
        providerTable.remove(*it);
        inverseTable.remove(obj);
    }
}

// Removes a provider with a given id from the cache.
void QWindowsUiaProviderCache::remove(QAccessible::Id id)
{
    inverseTable.remove(providerTable.value(id));
    providerTable.remove(id);
}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
