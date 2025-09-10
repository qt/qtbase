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
    if (const auto it = accessibleElements.find(axid); it == accessibleElements.end()) {
        accessibleElements[axid] = element;
        [element retain];
        return true;
    } else if (it.value() != element) {
        auto *oldElement = it.value();
        // this might invalidate the iterator
        [oldElement invalidate];
        [oldElement release];

        accessibleElements[axid] = element;
        [element retain];
        return true;
    }
    return false;
}

void QAccessibleCache::removeAccessibleElement(QAccessible::Id axid)
{
    // Some QAccessibleInterface instances in the cache didn't get created in
    // response to a query, but when we emit events. So we might get called with
    // and axid for which we don't have any element (yet).
    if (QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *element = elementForId(axid)) {
        [element invalidate];
        accessibleElements.remove(axid);
        [element release];
    }
}

QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *QAccessibleCache::elementForId(QAccessible::Id axid) const
{
    return accessibleElements.value(axid);
}

QT_END_NAMESPACE
