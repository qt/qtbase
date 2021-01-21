/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
****************************************************************************/

#include "customwidgetsinfo.h"
#include "driver.h"
#include "ui4.h"
#include "utils.h"

#include <utility>

QT_BEGIN_NAMESPACE

CustomWidgetsInfo::CustomWidgetsInfo() = default;

void CustomWidgetsInfo::acceptUI(DomUI *node)
{
    m_customWidgets.clear();

    if (node->elementCustomWidgets())
        acceptCustomWidgets(node->elementCustomWidgets());
}

void CustomWidgetsInfo::acceptCustomWidgets(DomCustomWidgets *node)
{
    TreeWalker::acceptCustomWidgets(node);
}

void CustomWidgetsInfo::acceptCustomWidget(DomCustomWidget *node)
{
    if (node->elementClass().isEmpty())
        return;

    m_customWidgets.insert(node->elementClass(), node);
}

bool CustomWidgetsInfo::extends(const QString &classNameIn, QLatin1String baseClassName) const
{
    if (classNameIn == baseClassName)
        return true;

    QString className = classNameIn;
    while (const DomCustomWidget *c = customWidget(className)) {
        const QString extends = c->elementExtends();
        if (className == extends) // Faulty legacy custom widget entries exist.
            return false;
        if (extends == baseClassName)
            return true;
        className = extends;
    }
    return false;
}

bool CustomWidgetsInfo::extendsOneOf(const QString &classNameIn,
                                     const QStringList &baseClassNames) const
{
    if (baseClassNames.contains(classNameIn))
        return true;

    QString className = classNameIn;
    while (const DomCustomWidget *c = customWidget(className)) {
        const QString extends = c->elementExtends();
        if (className == extends) // Faulty legacy custom widget entries exist.
            return false;
        if (baseClassNames.contains(extends))
            return true;
        className = extends;
    }
    return false;
}

bool CustomWidgetsInfo::isCustomWidgetContainer(const QString &className) const
{
    if (const DomCustomWidget *dcw = m_customWidgets.value(className, nullptr))
        if (dcw->hasElementContainer())
            return dcw->elementContainer() != 0;
    return false;
}

QString CustomWidgetsInfo::realClassName(const QString &className) const
{
    if (className == QLatin1String("Line"))
        return QLatin1String("QFrame");

    return className;
}

QString CustomWidgetsInfo::customWidgetAddPageMethod(const QString &name) const
{
    if (DomCustomWidget *dcw = m_customWidgets.value(name, nullptr))
        return dcw->elementAddPageMethod();
    return QString();
}

// add page methods for simple containers taking only the widget parameter
QString CustomWidgetsInfo::simpleContainerAddPageMethod(const QString &name) const
{
    using AddPageMethod = std::pair<const char *, const char *>;

    static AddPageMethod addPageMethods[] = {
        {"QStackedWidget", "addWidget"},
        {"QToolBar", "addWidget"},
        {"QDockWidget", "setWidget"},
        {"QScrollArea", "setWidget"},
        {"QSplitter", "addWidget"},
        {"QMdiArea", "addSubWindow"}
    };
    for (const auto &m : addPageMethods) {
        if (extends(name, QLatin1String(m.first)))
            return QLatin1String(m.second);
    }
    return QString();
}

QT_END_NAMESPACE
