// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
