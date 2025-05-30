// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customwidgetsinfo.h"
#include "driver.h"
#include "ui4.h"
#include "utils.h"

#include <utility>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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

bool CustomWidgetsInfo::extends(const QString &classNameIn, QAnyStringView baseClassName) const
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

// Is it ambiguous, resulting in different signals for Python
// "QAbstractButton::clicked(checked=false)"
bool CustomWidgetsInfo::isAmbiguousSignal(const QString &className,
                                          const QString &signalSignature) const
{
    if (signalSignature.startsWith(u"triggered") && extends(className, "QAction"))
        return true;
    if (signalSignature.startsWith(u"clicked(")
        && extendsOneOf(className, {u"QCommandLinkButton"_s, u"QCheckBox"_s,
                                    u"QPushButton"_s, u"QRadioButton"_s, u"QToolButton"_s})) {
        return true;
    }
    return false;
}

QString CustomWidgetsInfo::realClassName(const QString &className) const
{
    if (className == "Line"_L1)
        return u"QFrame"_s;

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
    using AddPageMethod = std::pair<QString, QString>;

    static const AddPageMethod addPageMethods[] = {
        {u"QStackedWidget"_s, u"addWidget"_s},
        {u"QToolBar"_s, u"addWidget"_s},
        {u"QDockWidget"_s, u"setWidget"_s},
        {u"QScrollArea"_s, u"setWidget"_s},
        {u"QSplitter"_s, u"addWidget"_s},
        {u"QMdiArea"_s, u"addSubWindow"_s}
    };
    for (const auto &m : addPageMethods) {
        if (extends(name, m.first))
            return m.second;
    }
    return QString();
}

QT_END_NAMESPACE
