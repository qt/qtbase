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

#ifndef QACCESSIBLECACHE_P
#define QACCESSIBLECACHE_P

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qobject.h>
#include <QtCore/qhash.h>

#include "qaccessible.h"

#ifndef QT_NO_ACCESSIBILITY

Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(QMacAccessibilityElement));

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QAccessibleCache  :public QObject
{
    Q_OBJECT

public:
    ~QAccessibleCache() override;
    static QAccessibleCache *instance();
    QAccessibleInterface *interfaceForId(QAccessible::Id id) const;
    QAccessible::Id idForInterface(QAccessibleInterface *iface) const;
    QAccessible::Id insert(QObject *object, QAccessibleInterface *iface) const;
    void deleteInterface(QAccessible::Id id, QObject *obj = nullptr);

#ifdef Q_OS_MAC
    QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *elementForId(QAccessible::Id axid) const;
    void insertElement(QAccessible::Id axid, QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *element) const;
#endif

private Q_SLOTS:
    void objectDestroyed(QObject *obj);

private:
    QAccessible::Id acquireId() const;

    mutable QHash<QAccessible::Id, QAccessibleInterface *> idToInterface;
    mutable QHash<QAccessibleInterface *, QAccessible::Id> interfaceToId;
    mutable QHash<QObject *, QAccessible::Id> objectToId;

#ifdef Q_OS_MAC
    void removeCocoaElement(QAccessible::Id axid);
    mutable QHash<QAccessible::Id, QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *> cocoaElements;
#endif

    friend class QAccessible;
    friend class QAccessibleInterface;
};

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY

#endif
