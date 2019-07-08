/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef CPPWRITEINCLUDES_H
#define CPPWRITEINCLUDES_H

#include "treewalker.h"

#include <qmap.h>
#include <qset.h>
#include <qstring.h>

#include <set>

QT_BEGIN_NAMESPACE

class QTextStream;
class CustomWidgetsInfo;
class Driver;
class Uic;

namespace CPP {

struct WriteIncludes : public TreeWalker
{
    WriteIncludes(Uic *uic);

    void acceptUI(DomUI *node) override;
    void acceptWidget(DomWidget *node) override;
    void acceptLayout(DomLayout *node) override;
    void acceptSpacer(DomSpacer *node) override;
    void acceptProperty(DomProperty *node) override;

//
// actions
//
    void acceptActionGroup(DomActionGroup *node) override;
    void acceptAction(DomAction *node) override;
    void acceptActionRef(DomActionRef *node) override;

//
// custom widgets
//
    void acceptCustomWidgets(DomCustomWidgets *node) override;
    void acceptCustomWidget(DomCustomWidget *node) override;

//
// include hints
//
    void acceptIncludes(DomIncludes *node) override;
    void acceptInclude(DomInclude *node) override;

protected:
     QTextStream &output() const { return m_output; }

private:
    void add(const QString &className, bool determineHeader = true, const QString &header = QString(), bool global = false);

private:
    using OrderedSet = std::set<QString>;
    void insertIncludeForClass(const QString &className, QString header = QString(), bool global = false);
    void insertInclude(const QString &header, bool global);
    void writeHeaders(const OrderedSet &headers, bool global);
    QString headerForClassName(const QString &className) const;

    const Uic *m_uic;
    QTextStream &m_output;

    OrderedSet m_localIncludes;
    OrderedSet m_globalIncludes;
    QSet<QString> m_includeBaseNames;

    QSet<QString> m_knownClasses;

    using StringMap = QMap<QString, QString>;
    StringMap m_classToHeader;
    StringMap m_oldHeaderToNewHeader;

    bool m_laidOut = false;
};

} // namespace CPP

QT_END_NAMESPACE

#endif // CPPWRITEINCLUDES_H
