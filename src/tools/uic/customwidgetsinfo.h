// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CUSTOMWIDGETSINFO_H
#define CUSTOMWIDGETSINFO_H

#include "treewalker.h"
#include <qstringlist.h>
#include <qmap.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

class Driver;
class DomScript;

class CustomWidgetsInfo : public TreeWalker
{
public:
    CustomWidgetsInfo();

    void acceptUI(DomUI *node) override;

    void acceptCustomWidgets(DomCustomWidgets *node) override;
    void acceptCustomWidget(DomCustomWidget *node) override;

    inline DomCustomWidget *customWidget(const QString &name) const
    { return m_customWidgets.value(name); }

    QString customWidgetAddPageMethod(const QString &name) const;
    QString simpleContainerAddPageMethod(const QString &name) const;

    static QString realClassName(const QString &className);

    bool extends(const QString &className, QAnyStringView baseClassName) const;
    bool extendsOneOf(const QString &className, const QStringList &baseClassNames) const;

    bool isCustomWidgetContainer(const QString &className) const;

    bool isAmbiguousSignal(const QString &className,
                           const QString &signalSignature) const;
    bool isAmbiguousSlot(const QString &className,
                         const QString &slotSignature) const;

private:
    using NameCustomWidgetMap = QMap<QString, DomCustomWidget*>;
    NameCustomWidgetMap m_customWidgets;
    bool isAmbiguous(const QString &className, const QString &signature,
                     QMetaMethod::MethodType type) const;
};

QT_END_NAMESPACE

#endif // CUSTOMWIDGETSINFO_H
