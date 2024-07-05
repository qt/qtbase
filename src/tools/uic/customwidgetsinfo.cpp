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

// FIXME in 7.0 - QTBUG-124241
// Remove isAmbiguous logic when widget slots have been disambiguated.
bool CustomWidgetsInfo::isAmbiguous(const QString &className, const QString &signature,
                                    QMetaMethod::MethodType type) const
{
    using TypeMap = QHash<QString, QMetaMethod::MethodType>;
    struct AmbiguousInClass {
        QLatin1StringView className;
        TypeMap methodMap;
    };

    static const QList<AmbiguousInClass> ambiguousList = {

        {"QAction"_L1, {{"triggered"_L1, QMetaMethod::Signal}}},
        {"QCommandLinkButton"_L1, {{"triggered"_L1, QMetaMethod::Signal},
                                   {"clicked"_L1, QMetaMethod::Signal}}},
        {"QPushButton"_L1, {{"triggered"_L1, QMetaMethod::Signal},
                            {"clicked"_L1, QMetaMethod::Signal}}},
        {"QCheckBox"_L1, {{"triggered"_L1, QMetaMethod::Signal},
                          {"clicked"_L1, QMetaMethod::Signal}}},
        {"QRadioButton"_L1, {{"triggered"_L1, QMetaMethod::Signal},
                             {"clicked"_L1, QMetaMethod::Signal}}},
        {"QToolButton"_L1,  {{"triggered"_L1, QMetaMethod::Signal},
                            {"clicked"_L1, QMetaMethod::Signal}}},
        {"QLabel"_L1, {{"setNum"_L1, QMetaMethod::Slot}}},
        {"QGraphicsView"_L1, {{"invalidateScene"_L1, QMetaMethod::Slot}}},
        {"QListView"_L1, {{"dataChanged"_L1, QMetaMethod::Slot}}},
        {"QColumnView"_L1, {{"dataChanged"_L1, QMetaMethod::Slot}}},
        {"QListWidget"_L1, {{"dataChanged"_L1, QMetaMethod::Slot},
                           {"scrollToItem"_L1, QMetaMethod::Slot}}},
        {"QTableView"_L1, {{"dataChanged"_L1, QMetaMethod::Slot}}},
        {"QTableWidget"_L1, {{"dataChanged"_L1, QMetaMethod::Slot},
                            {"scrollToItem"_L1, QMetaMethod::Slot}}},
        {"QTreeView"_L1, {{"dataChanged"_L1, QMetaMethod::Slot},
                         {"verticalScrollbarValueChanged"_L1, QMetaMethod::Slot},
                         {"expandRecursively"_L1, QMetaMethod::Slot}}},
        {"QTreeWidget"_L1, {{"dataChanged"_L1, QMetaMethod::Slot},
                           {"verticalScrollbarValueChanged"_L1, QMetaMethod::Slot}
                           ,{"expandRecursively"_L1, QMetaMethod::Slot}
                           ,{"scrollToItem"_L1, QMetaMethod::Slot}}},
        {"QUndoView"_L1, {{"dataChanged"_L1, QMetaMethod::Slot}}},
        {"QLCDNumber"_L1, {{"display"_L1, QMetaMethod::Slot}}},
        {"QMenuBar"_L1, {{"setVisible"_L1, QMetaMethod::Slot}}},
        {"QTextBrowser"_L1, {{"setSource"_L1, QMetaMethod::Slot}}},

        /*
        The following widgets with ambiguities are not used in the widget designer:

        {"QSplashScreen"_L1, {{"showMessage"_L1, QMetaMethod::Slot}}},
        {"QCompleter"_L1, {{"activated"_L1, QMetaMethod::Signal},
                          {"highlighted"_L1, QMetaMethod::Signal}}},
        {"QSystemTrayIcon"_L1, {{"showMessage"_L1, QMetaMethod::Slot}}},
        {"QStyledItemDelegate"_L1, {{"closeEditor"_L1, QMetaMethod::Signal}}},
        {"QErrorMessage"_L1, {{"showMessage"_L1, QMetaMethod::Slot}}},
        {"QGraphicsDropShadowEffect"_L1, {{"setOffset"_L1, QMetaMethod::Slot}}},
        {"QGraphicsScene"_L1, {{"invalidate"_L1, QMetaMethod::Slot}}},
        {"QItemDelegate"_L1, {{"closeEditor"_L1, QMetaMethod::Signal}}}
        */
    };

    for (auto it = ambiguousList.constBegin(); it != ambiguousList.constEnd(); ++it) {
        if (extends(className, it->className)) {
            const qsizetype pos = signature.indexOf(u'(');
            const QString method = signature.left(pos);
            const auto methodIterator = it->methodMap.find(method);
            return methodIterator != it->methodMap.constEnd() && type == methodIterator.value();
        }
    }
    return false;
}

// Is it ambiguous, resulting in different signals for Python
// "QAbstractButton::clicked(checked=false)"
bool CustomWidgetsInfo::isAmbiguousSignal(const QString &className,
                                          const QString &signalSignature) const
{
    return isAmbiguous(className, signalSignature, QMetaMethod::Signal);
}

bool CustomWidgetsInfo::isAmbiguousSlot(const QString &className,
                                        const QString &signalSignature) const
{
    return isAmbiguous(className, signalSignature, QMetaMethod::Slot);
}

QString CustomWidgetsInfo::realClassName(const QString &className)
{
    if (className == "Line"_L1)
        return u"QFrame"_s;

    return className;
}

QString CustomWidgetsInfo::customWidgetAddPageMethod(const QString &name) const
{
    if (DomCustomWidget *dcw = m_customWidgets.value(name, nullptr))
        return dcw->elementAddPageMethod();
    return {};
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
    return {};
}

QT_END_NAMESPACE
