/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qaccessiblecache_p.h"

// qcocoaaccessibilityelement.h in Cocoa platform plugin
@interface QT_MANGLE_NAMESPACE(QMacAccessibilityElement)
- (void)invalidate;
@end

QT_BEGIN_NAMESPACE

void QAccessibleCache::insertElement(QAccessible::Id axid, QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *element) const
{
    cocoaElements[axid] = element;
}

void QAccessibleCache::removeCocoaElement(QAccessible::Id axid)
{
    QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *element = elementForId(axid);
    [element invalidate];
    cocoaElements.remove(axid);
}

QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *QAccessibleCache::elementForId(QAccessible::Id axid) const
{
    return cocoaElements.value(axid);
}

QT_END_NAMESPACE
