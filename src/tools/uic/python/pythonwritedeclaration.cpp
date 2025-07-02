// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonwritedeclaration.h"
#include <cppwriteinitialization.h>
#include <language.h>
#include <driver.h>
#include <ui4.h>
#include <uic.h>

#include <QtCore/qtextstream.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace Python {

WriteDeclaration::WriteDeclaration(Uic *uic)  :
    m_uic(uic),
    m_driver(uic->driver()),
    m_output(uic->output()),
    m_option(uic->option())
{
}

void WriteDeclaration::acceptUI(DomUI *node)
{
    // remove any left-over C++ namespaces
    const QString qualifiedClassName = "Ui_"_L1 + node->elementClass()
        + m_option.postfix;
    m_output << "class " << language::fixClassName(qualifiedClassName) << "(object):\n";

    TreeWalker::acceptWidget(node->elementWidget());
    if (const DomButtonGroups *domButtonGroups = node->elementButtonGroups())
        acceptButtonGroups(domButtonGroups);
    CPP::WriteInitialization(m_uic).acceptUI(node);
}

// Register button groups to prevent the on-the-fly creation legacy
// feature from triggering
void WriteDeclaration::acceptButtonGroup(const DomButtonGroup *buttonGroup)
{
    m_driver->findOrInsertButtonGroup(buttonGroup);
}

} // namespace Python

QT_END_NAMESPACE
