/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

QT_BEGIN_NAMESPACE

class QTextStream;
class Driver;
class Uic;

namespace CPP {

struct WriteIncludes : public TreeWalker
{
    WriteIncludes(Uic *uic);

    void acceptUI(DomUI *node) Q_DECL_OVERRIDE;
    void acceptWidget(DomWidget *node) Q_DECL_OVERRIDE;
    void acceptLayout(DomLayout *node) Q_DECL_OVERRIDE;
    void acceptSpacer(DomSpacer *node) Q_DECL_OVERRIDE;
    void acceptProperty(DomProperty *node) Q_DECL_OVERRIDE;
    void acceptWidgetScripts(const DomScripts &, DomWidget *, const DomWidgets &) Q_DECL_OVERRIDE;

//
// custom widgets
//
    void acceptCustomWidgets(DomCustomWidgets *node) Q_DECL_OVERRIDE;
    void acceptCustomWidget(DomCustomWidget *node) Q_DECL_OVERRIDE;

//
// include hints
//
    void acceptIncludes(DomIncludes *node) Q_DECL_OVERRIDE;
    void acceptInclude(DomInclude *node) Q_DECL_OVERRIDE;

    bool scriptsActivated() const { return m_scriptsActivated; }

private:
    void add(const QString &className, bool determineHeader = true, const QString &header = QString(), bool global = false);

private:
    typedef QMap<QString, bool> OrderedSet;
    void insertIncludeForClass(const QString &className, QString header = QString(), bool global = false);
    void insertInclude(const QString &header, bool global);
    void writeHeaders(const OrderedSet &headers, bool global);
    QString headerForClassName(const QString &className) const;
    void activateScripts();

    const Uic *m_uic;
    QTextStream &m_output;

    OrderedSet m_localIncludes;
    OrderedSet m_globalIncludes;
    QSet<QString> m_includeBaseNames;

    QSet<QString> m_knownClasses;

    typedef QMap<QString, QString> StringMap;
    StringMap m_classToHeader;
    StringMap m_oldHeaderToNewHeader;

    bool m_scriptsActivated;
    bool m_laidOut;
};

} // namespace CPP

QT_END_NAMESPACE

#endif // CPPWRITEINCLUDES_H
