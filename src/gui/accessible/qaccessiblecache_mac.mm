// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblecache_p.h"
#include <QtCore/private/qcore_mac_p.h>

// qcocoaaccessibilityelement.h in platform plugin
QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QMacAccessibilityElement, NSObject
- (void)invalidate;
)

QT_BEGIN_NAMESPACE

bool QAccessibleCache::insertElement(QAccessible::Id axid,
                                     QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *element) const
{
    auto *oldElement = accessibleElements.value(axid);
    if (oldElement == element)
        return false;

    // Update the map before invalidating and releasing the old element, as
    // that might re-enter the cache.
    [element retain];
    accessibleElements.insert(axid, element);
    if (oldElement) {
        [oldElement invalidate];
        [oldElement release];
    }
    return true;
}

void QAccessibleCache::removeAccessibleElement(QAccessible::Id axid)
{
    // Some QAccessibleInterface instances in the cache didn't get created in
    // response to a query, but when we emit events. So we might get called with
    // and axid for which we don't have any element (yet).
    // Remove the element from the map before invalidating and releasing it,
    // as that might re-enter the cache.
    if (QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *element = accessibleElements.take(axid)) {
        [element invalidate];
        [element release];
    }
}

QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *QAccessibleCache::elementForId(QAccessible::Id axid) const
{
    return accessibleElements.value(axid);
}

QT_END_NAMESPACE
