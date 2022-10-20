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

#include <writeincludesbase.h>

#include <QtCore/qmap.h>

#include <set>

QT_BEGIN_NAMESPACE

class QTextStream;

namespace CPP {

class  WriteIncludes : public WriteIncludesBase
{
public:
    WriteIncludes(Uic *uic);

    void acceptUI(DomUI *node) override;
    void acceptInclude(DomInclude *node) override;

protected:
     QTextStream &output() const { return m_output; }
     void doAdd(const QString &className, const DomCustomWidget *dcw = nullptr) override;

private:
    using OrderedSet = std::set<QString>;
    void addCppCustomWidget(const QString &className, const DomCustomWidget *dcw);
    void insertIncludeForClass(const QString &className, QString header = QString(), bool global = false);
    void insertInclude(const QString &header, bool global);
    void writeHeaders(const OrderedSet &headers, bool global);
    QString headerForClassName(const QString &className) const;

    QTextStream &m_output;

    OrderedSet m_localIncludes;
    OrderedSet m_globalIncludes;
    QSet<QString> m_includeBaseNames;
    using StringMap = QMap<QString, QString>;
    StringMap m_classToHeader;
    StringMap m_oldHeaderToNewHeader;
};

} // namespace CPP

QT_END_NAMESPACE

#endif // CPPWRITEINCLUDES_H
