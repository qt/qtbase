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
#ifndef QCOCOAACCESIBILITY_H
#define QCOCOAACCESIBILITY_H

#include <AppKit/AppKit.h>

#include <QtGui>
#include <qpa/qplatformaccessibility.h>

#include "qcocoaaccessibilityelement.h"

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

class QCocoaAccessibility : public QPlatformAccessibility
{
public:
    QCocoaAccessibility();
    ~QCocoaAccessibility();
    void notifyAccessibilityUpdate(QAccessibleEvent *event) override;
    void setRootObject(QObject *o) override;
    void initialize() override;
    void cleanup() override;
};

namespace QCocoaAccessible {

/*
    Qt Cocoa Accessibility Overview

    Cocoa accessibility is implemented in the following files:

    - qcocoaaccessibility (this file) : QCocoaAccessibility "plugin", conversion and helper functions.
    - qnsviewaccessibility            : Root accessibility implementation for QNSView
    - qcocoaaccessibilityelement      : Cocoa accessibility protocol wrapper for QAccessibleInterface

    The accessibility implementation wraps QAccessibleInterfaces in QCocoaAccessibleElements, which
    implements the cocoa accessibility protocol. The root QAccessibleInterface (the one returned
    by QWindow::accessibleRoot), is anchored to the QNSView in qnsviewaccessibility.mm.

    Cocoa explores the accessibility tree by walking the tree using the parent/child
    relationships or hit testing. When this happens we create QCocoaAccessibleElements on
    demand.
*/

NSString *macRole(QAccessibleInterface *interface);
NSString *macSubrole(QAccessibleInterface *interface);
bool shouldBeIgnored(QAccessibleInterface *interface);
NSArray<QMacAccessibilityElement *> *unignoredChildren(QAccessibleInterface *interface);
NSString *getTranslatedAction(const QString &qtAction);
QString translateAction(NSString *nsAction, QAccessibleInterface *interface);
bool hasValueAttribute(QAccessibleInterface *interface);
id getValueAttribute(QAccessibleInterface *interface);

}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY

#endif // QCOCOAACCESIBILITY_H
