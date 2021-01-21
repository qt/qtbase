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

#include "qwinrtuiametadatacache.h"
#include "qwinrtuiautils.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/private/qeventdispatcher_winrt_p.h>

QT_BEGIN_NAMESPACE

using namespace QWinRTUiAutomation;

// Private constructor
QWinRTUiaMetadataCache::QWinRTUiaMetadataCache()
{
}

// shared instance
QWinRTUiaMetadataCache *QWinRTUiaMetadataCache::instance()
{
    static QWinRTUiaMetadataCache metadataCache;
    return &metadataCache;
}

// Returns the cached metadata associated with the ID, or an instance with default values.
QSharedPointer<QWinRTUiaControlMetadata> QWinRTUiaMetadataCache::metadataForId(QAccessible::Id id)
{
    QSharedPointer<QWinRTUiaControlMetadata> metadata;

    m_mutex.lock();
    if (m_metadataTable.contains(id))
        metadata = m_metadataTable[id];
    else
        metadata = QSharedPointer<QWinRTUiaControlMetadata>(new QWinRTUiaControlMetadata);
    m_mutex.unlock();
    return metadata;
}

// Caches metadata from the accessibility framework within the main thread.
bool QWinRTUiaMetadataCache::load(QAccessible::Id id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([id]() {
        QWinRTUiaMetadataCache::instance()->insert(id, QSharedPointer<QWinRTUiaControlMetadata>(new QWinRTUiaControlMetadata(id)));
        return S_OK;
    }))) {
        return false;
    }
    return true;
}

// Inserts metadata in the cache and associates it with an accessibility ID.
void QWinRTUiaMetadataCache::insert(QAccessible::Id id, const QSharedPointer<QWinRTUiaControlMetadata> &metadata)
{
    m_mutex.lock();
    m_metadataTable[id] = metadata;
    m_mutex.unlock();
}

// Removes metadata with a given id from the cache.
void QWinRTUiaMetadataCache::remove(QAccessible::Id id)
{
    m_mutex.lock();
    m_metadataTable.remove(id);
    m_mutex.unlock();
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

