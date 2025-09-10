// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppwritedeclaration.h"
#include "cppwriteinitialization.h"
#include "driver.h"
#include "ui4.h"
#include "uic.h"
#include "databaseinfo.h"
#include "customwidgetsinfo.h"

#include <qtextstream.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {
    void openNameSpaces(const QStringList &namespaceList, QTextStream &output)
    {
        for (const QString &n : namespaceList) {
            if (!n.isEmpty())
                output << "namespace " << n << " {\n";
        }
    }

    void closeNameSpaces(const QStringList &namespaceList, QTextStream &output) {
        for (auto it = namespaceList.rbegin(), end = namespaceList.rend(); it != end; ++it) {
            if (!it->isEmpty())
                output << "} // namespace " << *it << "\n";
        }
    }
}

namespace CPP {

WriteDeclaration::WriteDeclaration(Uic *uic)  :
    m_uic(uic),
    m_driver(uic->driver()),
    m_output(uic->output()),
    m_option(uic->option())
{
}

void WriteDeclaration::acceptUI(DomUI *node)
{
    QString qualifiedClassName = node->elementClass() + m_option.postfix;
    QString className = qualifiedClassName;

    m_driver->findOrInsertWidget(node->elementWidget());

    QString exportMacro = node->elementExportMacro();
    if (!exportMacro.isEmpty())
        exportMacro.append(u' ');

    QStringList namespaceList = qualifiedClassName.split("::"_L1);
    if (!namespaceList.isEmpty()) {
        className = namespaceList.last();
        namespaceList.removeLast();
    }

    // This is a bit of the hack but covers the cases Qt in/without namespaces
    // and User defined classes in/without namespaces. The "strange" case
    // is a User using Qt-in-namespace having his own classes not in a namespace.
    // In this case the generated Ui helper classes will also end up in
    // the Qt namespace (which is harmless, but not "pretty")
    const bool needsMacro = m_option.qtNamespace &&
            (namespaceList.isEmpty() || namespaceList.at(0) == "qdesigner_internal"_L1);

    if (needsMacro)
        m_output << "QT_BEGIN_NAMESPACE\n\n";

    openNameSpaces(namespaceList, m_output);

    if (!namespaceList.isEmpty())
        m_output << "\n";

    m_output << "class " << exportMacro << m_option.prefix << className << "\n"
           << "{\n"
           << "public:\n";

    const QStringList connections = m_uic->databaseInfo()->connections();
    for (const QString &connection : connections) {
        if (connection != "(default)"_L1)
            m_output << m_option.indent << "QSqlDatabase " << connection << "Connection;\n";
    }

    TreeWalker::acceptWidget(node->elementWidget());
    if (const DomButtonGroups *domButtonGroups = node->elementButtonGroups())
        acceptButtonGroups(domButtonGroups);

    m_output << "\n";

    WriteInitialization(m_uic).acceptUI(node);

    m_output << "};\n\n";

    closeNameSpaces(namespaceList, m_output);

    if (!namespaceList.isEmpty())
        m_output << "\n";

    if (m_option.generateNamespace && !m_option.prefix.isEmpty()) {
        namespaceList.append("Ui"_L1);

        openNameSpaces(namespaceList, m_output);

        m_output << m_option.indent << "class " << exportMacro << className << ": public " << m_option.prefix << className << " {};\n";

        closeNameSpaces(namespaceList, m_output);

        if (!namespaceList.isEmpty())
            m_output << "\n";
    }

    if (needsMacro)
        m_output << "QT_END_NAMESPACE\n\n";
}

void WriteDeclaration::acceptWidget(DomWidget *node)
{
    QString className = u"QWidget"_s;
    if (node->hasAttributeClass())
        className = node->attributeClass();

    m_output << m_option.indent << CustomWidgetsInfo::realClassName(className) << " *" << m_driver->findOrInsertWidget(node) << ";\n";

    TreeWalker::acceptWidget(node);
}

void WriteDeclaration::acceptSpacer(DomSpacer *node)
{
     m_output << m_option.indent << "QSpacerItem *" << m_driver->findOrInsertSpacer(node) << ";\n";
     TreeWalker::acceptSpacer(node);
}

void WriteDeclaration::acceptLayout(DomLayout *node)
{
    QString className = u"QLayout"_s;
    if (node->hasAttributeClass())
        className = node->attributeClass();

    m_output << m_option.indent << className << " *" << m_driver->findOrInsertLayout(node) << ";\n";

    TreeWalker::acceptLayout(node);
}

void WriteDeclaration::acceptActionGroup(DomActionGroup *node)
{
    m_output << m_option.indent << "QActionGroup *" << m_driver->findOrInsertActionGroup(node) << ";\n";

    TreeWalker::acceptActionGroup(node);
}

void WriteDeclaration::acceptAction(DomAction *node)
{
    m_output << m_option.indent << "QAction *" << m_driver->findOrInsertAction(node) << ";\n";

    TreeWalker::acceptAction(node);
}

void WriteDeclaration::acceptButtonGroup(const DomButtonGroup *buttonGroup)
{
    m_output << m_option.indent << "QButtonGroup *" << m_driver->findOrInsertButtonGroup(buttonGroup) << ";\n";
    TreeWalker::acceptButtonGroup(buttonGroup);
}

} // namespace CPP

QT_END_NAMESPACE
