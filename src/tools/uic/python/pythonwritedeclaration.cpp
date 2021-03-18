/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "pythonwritedeclaration.h"
#include <cppwriteinitialization.h>
#include <language.h>
#include <driver.h>
#include <ui4.h>
#include <uic.h>

#include <QtCore/qtextstream.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

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
    const QString qualifiedClassName = QLatin1String("Ui_") + node->elementClass()
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
