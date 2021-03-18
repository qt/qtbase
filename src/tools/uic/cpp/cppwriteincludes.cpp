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

#include "cppwriteincludes.h"
#include "driver.h"
#include "ui4.h"
#include "uic.h"
#include "databaseinfo.h"

#include <qdebug.h>
#include <qfileinfo.h>
#include <qtextstream.h>

#include <stdio.h>

QT_BEGIN_NAMESPACE

enum { debugWriteIncludes = 0 };
enum { warnHeaderGeneration = 0 };

struct ClassInfoEntry
{
    const char *klass;
    const char *module;
    const char *header;
};

static const ClassInfoEntry qclass_lib_map[] = {
#define QT_CLASS_LIB(klass, module, header) { #klass, #module, #header },
#include "qclass_lib_map.h"

#undef QT_CLASS_LIB
};

// Format a module header as 'QtCore/QObject'
static inline QString moduleHeader(const QString &module, const QString &header)
{
    QString rc = module;
    rc += QLatin1Char('/');
    rc += header;
    return rc;
}

namespace CPP {

WriteIncludes::WriteIncludes(Uic *uic)
    : m_uic(uic), m_output(uic->output())
{
    // When possible (no namespace) use the "QtModule/QClass" convention
    // and create a re-mapping of the old header "qclass.h" to it. Do not do this
    // for the "Phonon::Someclass" classes, however.
    const QString namespaceDelimiter = QLatin1String("::");
    const ClassInfoEntry *classLibEnd = qclass_lib_map + sizeof(qclass_lib_map)/sizeof(ClassInfoEntry);
    for (const ClassInfoEntry *it = qclass_lib_map; it < classLibEnd;  ++it) {
        const QString klass = QLatin1String(it->klass);
        const QString module = QLatin1String(it->module);
        QLatin1String header = QLatin1String(it->header);
        if (klass.contains(namespaceDelimiter)) {
            m_classToHeader.insert(klass, moduleHeader(module, header));
        } else {
            const QString newHeader = moduleHeader(module, klass);
            m_classToHeader.insert(klass, newHeader);
            m_oldHeaderToNewHeader.insert(header, newHeader);
        }
    }
}

void WriteIncludes::acceptUI(DomUI *node)
{
    m_laidOut = false;
    m_localIncludes.clear();
    m_globalIncludes.clear();
    m_knownClasses.clear();
    m_includeBaseNames.clear();

    if (node->elementIncludes())
        acceptIncludes(node->elementIncludes());

    if (node->elementCustomWidgets())
        TreeWalker::acceptCustomWidgets(node->elementCustomWidgets());

    add(QLatin1String("QApplication"));
    add(QLatin1String("QVariant"));

    if (node->elementButtonGroups())
        add(QLatin1String("QButtonGroup"));

    TreeWalker::acceptUI(node);

    const auto includeFile = m_uic->option().includeFile;
    if (!includeFile.isEmpty())
        m_globalIncludes.insert(includeFile);

    writeHeaders(m_globalIncludes, true);
    writeHeaders(m_localIncludes, false);

    m_output << '\n';
}

void WriteIncludes::acceptWidget(DomWidget *node)
{
    if (debugWriteIncludes)
        fprintf(stderr, "%s '%s'\n", Q_FUNC_INFO, qPrintable(node->attributeClass()));

    add(node->attributeClass());
    TreeWalker::acceptWidget(node);
}

void WriteIncludes::acceptLayout(DomLayout *node)
{
    add(node->attributeClass());
    m_laidOut = true;
    TreeWalker::acceptLayout(node);
}

void WriteIncludes::acceptSpacer(DomSpacer *node)
{
    add(QLatin1String("QSpacerItem"));
    TreeWalker::acceptSpacer(node);
}

void WriteIncludes::acceptProperty(DomProperty *node)
{
    if (node->kind() == DomProperty::Date)
        add(QLatin1String("QDate"));
    if (node->kind() == DomProperty::Locale)
        add(QLatin1String("QLocale"));
    if (node->kind() == DomProperty::IconSet)
        add(QLatin1String("QIcon"));
    TreeWalker::acceptProperty(node);
}

void WriteIncludes::insertIncludeForClass(const QString &className, QString header, bool global)
{
    if (debugWriteIncludes)
        fprintf(stderr, "%s %s '%s' %d\n", Q_FUNC_INFO, qPrintable(className), qPrintable(header), global);

    do {
        if (!header.isEmpty())
            break;

        // Known class
        const StringMap::const_iterator it = m_classToHeader.constFind(className);
        if (it != m_classToHeader.constEnd()) {
            header = it.value();
            global =  true;
            break;
        }

        // Quick check by class name to detect includehints provided for custom widgets.
        // Remove namespaces
        QString lowerClassName = className.toLower();
        static const QString namespaceSeparator = QLatin1String("::");
        const int namespaceIndex = lowerClassName.lastIndexOf(namespaceSeparator);
        if (namespaceIndex != -1)
            lowerClassName.remove(0, namespaceIndex + namespaceSeparator.size());
        if (m_includeBaseNames.contains(lowerClassName)) {
            header.clear();
            break;
        }

        // Last resort: Create default header
        if (!m_uic->option().implicitIncludes)
            break;
        header = lowerClassName;
        header += QLatin1String(".h");
        if (warnHeaderGeneration) {
            qWarning("%s: Warning: generated header '%s' for class '%s'.",
                     qPrintable(m_uic->option().messagePrefix()),
                     qPrintable(header), qPrintable(className));

        }

        global = true;
    } while (false);

    if (!header.isEmpty())
        insertInclude(header, global);
}

void WriteIncludes::add(const QString &className, bool determineHeader, const QString &header, bool global)
{
    if (debugWriteIncludes)
            fprintf(stderr, "%s %s '%s' %d\n", Q_FUNC_INFO, qPrintable(className), qPrintable(header), global);

    if (className.isEmpty() || m_knownClasses.contains(className))
        return;

    m_knownClasses.insert(className);

    const CustomWidgetsInfo *cwi = m_uic->customWidgetsInfo();
    static const QStringList treeViewsWithHeaders = {
        QLatin1String("QTreeView"), QLatin1String("QTreeWidget"),
        QLatin1String("QTableView"), QLatin1String("QTableWidget")
    };
    if (cwi->extendsOneOf(className, treeViewsWithHeaders))
        add(QLatin1String("QHeaderView"));

    if (!m_laidOut && cwi->extends(className, QLatin1String("QToolBox")))
        add(QLatin1String("QLayout")); // spacing property of QToolBox)

    if (className == QLatin1String("Line")) { // ### hmm, deprecate me!
        add(QLatin1String("QFrame"));
        return;
    }

    if (determineHeader)
        insertIncludeForClass(className, header, global);
}

void WriteIncludes::acceptCustomWidget(DomCustomWidget *node)
{
    const QString className = node->elementClass();
    if (className.isEmpty())
        return;

    if (!node->elementHeader() || node->elementHeader()->text().isEmpty()) {
        add(className, false); // no header specified
    } else {
        // custom header unless it is a built-in qt class
        QString header;
        bool global = false;
        if (!m_classToHeader.contains(className)) {
            global = node->elementHeader()->attributeLocation().toLower() == QLatin1String("global");
            header = node->elementHeader()->text();
        }
        add(className, true, header, global);
    }
}

void WriteIncludes::acceptActionGroup(DomActionGroup *node)
{
    add(QLatin1String("QAction"));
    TreeWalker::acceptActionGroup(node);
}

void WriteIncludes::acceptAction(DomAction *node)
{
    add(QLatin1String("QAction"));
    TreeWalker::acceptAction(node);
}

void WriteIncludes::acceptActionRef(DomActionRef *node)
{
    add(QLatin1String("QAction"));
    TreeWalker::acceptActionRef(node);
}

void WriteIncludes::acceptCustomWidgets(DomCustomWidgets *node)
{
    Q_UNUSED(node);
}

void WriteIncludes::acceptIncludes(DomIncludes *node)
{
    TreeWalker::acceptIncludes(node);
}

void WriteIncludes::acceptInclude(DomInclude *node)
{
    bool global = true;
    if (node->hasAttributeLocation())
        global = node->attributeLocation() == QLatin1String("global");
    insertInclude(node->text(), global);
}

void WriteIncludes::insertInclude(const QString &header, bool global)
{
    if (debugWriteIncludes)
        fprintf(stderr, "%s %s %d\n", Q_FUNC_INFO, qPrintable(header), global);

    OrderedSet &includes = global ?  m_globalIncludes : m_localIncludes;
    // Insert (if not already done).
    const bool isNewHeader = includes.insert(header).second;
    if (!isNewHeader)
        return;
    // Also remember base name for quick check of suspicious custom plugins
    const QString lowerBaseName = QFileInfo(header).completeBaseName ().toLower();
    m_includeBaseNames.insert(lowerBaseName);
}

void WriteIncludes::writeHeaders(const OrderedSet &headers, bool global)
{
    const QChar openingQuote = global ? QLatin1Char('<') : QLatin1Char('"');
    const QChar closingQuote = global ? QLatin1Char('>') : QLatin1Char('"');

    // Check for the old headers 'qslider.h' and replace by 'QtGui/QSlider'
    for (const QString &header : headers) {
        const QString value = m_oldHeaderToNewHeader.value(header, header);
        const auto trimmed = QStringRef(&value).trimmed();
        if (!trimmed.isEmpty())
            m_output << "#include " << openingQuote << trimmed << closingQuote << '\n';
    }
}

} // namespace CPP

QT_END_NAMESPACE
