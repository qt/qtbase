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

#include "pythonwriteimports.h"

#include <customwidgetsinfo.h>
#include <option.h>
#include <uic.h>

#include <ui4.h>

#include <QtCore/qtextstream.h>

QT_BEGIN_NAMESPACE

static const char *standardImports =
R"I(from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *
)I";

// Change the name of a qrc file "dir/foo.qrc" file to the Python
// module name "foo_rc" according to project conventions.
static QString pythonResource(QString resource)
{
    const int lastSlash = resource.lastIndexOf(QLatin1Char('/'));
    if (lastSlash != -1)
        resource.remove(0, lastSlash + 1);
    if (resource.endsWith(QLatin1String(".qrc"))) {
        resource.chop(4);
        resource.append(QLatin1String("_rc"));
    }
    return resource;
}

namespace Python {

WriteImports::WriteImports(Uic *uic) : m_uic(uic)
{
}

void WriteImports::acceptUI(DomUI *node)
{
    auto &output = m_uic->output();
    output << standardImports << '\n';
    if (auto customWidgets = node->elementCustomWidgets()) {
        TreeWalker::acceptCustomWidgets(customWidgets);
        output << '\n';
    }

    if (auto resources = node->elementResources()) {
        const auto includes = resources->elementInclude();
        for (auto include : includes) {
            if (include->hasAttributeLocation())
                writeImport(pythonResource(include->attributeLocation()));
        }
        output << '\n';
    }
}

void WriteImports::writeImport(const QString &module)
{

    if (m_uic->option().fromImports)
        m_uic->output() << "from  . ";
    m_uic->output() << "import " << module << '\n';
}

QString WriteImports::qtModuleOf(const DomCustomWidget *node) const
{
    if (m_uic->customWidgetsInfo()->extends(node->elementClass(), QLatin1String("QAxWidget")))
        return QStringLiteral("QtAxContainer");
    if (const auto headerElement = node->elementHeader()) {
        const auto &header = headerElement->text();
        if (header.startsWith(QLatin1String("Qt"))) {
            const int slash = header.indexOf(QLatin1Char('/'));
            if (slash != -1)
                return header.left(slash);
        }
    }
    return QString();
}

void WriteImports::acceptCustomWidget(DomCustomWidget *node)
{
    const auto &className = node->elementClass();
    if (className.contains(QLatin1String("::")))
        return; // Exclude namespaced names (just to make tests pass).
    const QString &importModule = qtModuleOf(node);
    auto &output = m_uic->output();
    // For starting importing PySide2 modules
    if (!importModule.isEmpty()) {
        output << "from ";
        if (importModule.startsWith(QLatin1String("Qt")))
            output << "PySide2.";
        output << importModule;
        if (!className.isEmpty())
            output << " import " << className << "\n\n";
    } else {
        // When the elementHeader is not set, we know it's the continuation
        // of a PySide2 import or a normal import of another module.
        if (!node->elementHeader() || node->elementHeader()->text().isEmpty()) {
            output << "import " << className << '\n';
        } else { // When we do have elementHeader, we know it's a relative import.
            QString modulePath = node->elementHeader()->text();
            // Replace the '/' by '.'
            modulePath.replace(QLatin1Char('/'), QLatin1Char('.'));
            // '.h' is added by default on headers for <customwidget>
            if (modulePath.endsWith(QLatin1String(".h")))
                modulePath.chop(2);
            output << "from " << modulePath << " import " << className << '\n';
        }
    }
}

} // namespace Python

QT_END_NAMESPACE
