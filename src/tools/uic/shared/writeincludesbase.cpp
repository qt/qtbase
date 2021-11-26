/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "writeincludesbase.h"
#include "ui4.h"
#include <uic.h>
#include <databaseinfo.h>

QT_BEGIN_NAMESPACE

static const ClassInfoEntry qclass_lib_map[] = {
#define QT_CLASS_LIB(klass, module, header) { #klass, #module, #header },
#include "qclass_lib_map.h"
#undef QT_CLASS_LIB
};

ClassInfoEntries classInfoEntries()
{
    const ClassInfoEntry *classLibEnd = qclass_lib_map + sizeof(qclass_lib_map)/sizeof(ClassInfoEntry);
    return {qclass_lib_map, classLibEnd};
}

// Base class for implementing a class that determines includes and equivalents
// in other languages by implementing doAdd(). It makes sure all dependent
// classes are known.
WriteIncludesBase::WriteIncludesBase(Uic *uic) : m_uic(uic)
{
}

void WriteIncludesBase::acceptUI(DomUI *node)
{
    m_knownClasses.clear();
    m_laidOut = false;

    if (node->elementIncludes())
        acceptIncludes(node->elementIncludes());

    // Populate known custom widgets first
    if (node->elementCustomWidgets())
        TreeWalker::acceptCustomWidgets(node->elementCustomWidgets());

    add(QStringLiteral("QApplication"));
    add(QStringLiteral("QVariant"));

    if (node->elementButtonGroups())
        add(QStringLiteral("QButtonGroup"));

    TreeWalker::acceptUI(node);
}

void WriteIncludesBase::acceptWidget(DomWidget *node)
{
    add(node->attributeClass());
    TreeWalker::acceptWidget(node);
}

void WriteIncludesBase::acceptLayout(DomLayout *node)
{
    add(node->attributeClass());
    m_laidOut = true;
    TreeWalker::acceptLayout(node);
}

void WriteIncludesBase::acceptSpacer(DomSpacer *node)
{
    add(QStringLiteral("QSpacerItem"));
    TreeWalker::acceptSpacer(node);
}

void WriteIncludesBase::acceptProperty(DomProperty *node)
{
    if (node->kind() == DomProperty::Date)
        add(QStringLiteral("QDate"));
    if (node->kind() == DomProperty::Locale)
        add(QStringLiteral("QLocale"));
    if (node->kind() == DomProperty::IconSet)
        add(QStringLiteral("QIcon"));
    TreeWalker::acceptProperty(node);
}

void WriteIncludesBase::add(const QString &className, const DomCustomWidget *dcw)
{
    if (className.isEmpty() || m_knownClasses.contains(className))
        return;

    m_knownClasses.insert(className);

    const CustomWidgetsInfo *cwi = m_uic->customWidgetsInfo();
    static const QStringList treeViewsWithHeaders = {
        QStringLiteral("QTreeView"), QStringLiteral("QTreeWidget"),
        QStringLiteral("QTableView"), QStringLiteral("QTableWidget")
    };
    if (cwi->extendsOneOf(className, treeViewsWithHeaders))
        add(QStringLiteral("QHeaderView"));

    if (!m_laidOut && cwi->extends(className, "QToolBox"))
        add(QStringLiteral("QLayout")); // spacing property of QToolBox)

    if (className == QStringLiteral("Line")) { // ### hmm, deprecate me!
        add(QStringLiteral("QFrame"));
        return;
    }

    if (cwi->extends(className, "QDialogButtonBox"))
        add(QStringLiteral("QAbstractButton")); // for signal "clicked(QAbstractButton*)"

    doAdd(className, dcw);
}

void WriteIncludesBase::acceptCustomWidget(DomCustomWidget *node)
{
    const QString className = node->elementClass();
    if (!className.isEmpty())
        add(className, node);
}

void WriteIncludesBase::acceptActionGroup(DomActionGroup *node)
{
    add(QStringLiteral("QActionGroup"));
    TreeWalker::acceptActionGroup(node);
}

void WriteIncludesBase::acceptAction(DomAction *node)
{
    add(QStringLiteral("QAction"));
    TreeWalker::acceptAction(node);
}

void WriteIncludesBase::acceptActionRef(DomActionRef *node)
{
    add(QStringLiteral("QAction"));
    TreeWalker::acceptActionRef(node);
}

QT_END_NAMESPACE
