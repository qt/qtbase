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

// Format a module header as 'QtCore/QObject'
static inline QString moduleHeader(const QString &module, const QString &header)
{
    QString rc = module;
    rc += QLatin1Char('/');
    rc += header;
    return rc;
}

namespace CPP {

WriteIncludes::WriteIncludes(Uic *uic) : WriteIncludesBase(uic),
    m_output(uic->output())
{
    // When possible (no namespace) use the "QtModule/QClass" convention
    // and create a re-mapping of the old header "qclass.h" to it. Do not do this
    // for the "Phonon::Someclass" classes, however.
    const QString namespaceDelimiter = QLatin1String("::");
    for (const auto &e : classInfoEntries()) {
        const QString klass = QLatin1String(e.klass);
        const QString module = QLatin1String(e.module);
        QLatin1String header = QLatin1String(e.header);
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
    m_localIncludes.clear();
    m_globalIncludes.clear();
    m_includeBaseNames.clear();

    WriteIncludesBase::acceptUI(node);

    const auto includeFile = uic()->option().includeFile;
    if (!includeFile.isEmpty())
        m_globalIncludes.insert(includeFile);

    writeHeaders(m_globalIncludes, true);
    writeHeaders(m_localIncludes, false);

    m_output << '\n';
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
        if (!uic()->option().implicitIncludes)
            break;
        header = lowerClassName;
        header += QLatin1String(".h");
        if (warnHeaderGeneration) {
            qWarning("%s: Warning: generated header '%s' for class '%s'.",
                     qPrintable(uic()->option().messagePrefix()),
                     qPrintable(header), qPrintable(className));

        }

        global = true;
    } while (false);

    if (!header.isEmpty())
        insertInclude(header, global);
}

void WriteIncludes::doAdd(const QString &className, const DomCustomWidget *dcw)
{
    if (dcw != nullptr)
        addCppCustomWidget(className, dcw);
    else
        insertIncludeForClass(className, {}, false);
}

void WriteIncludes::addCppCustomWidget(const QString &className, const DomCustomWidget *dcw)
{
    const DomHeader *domHeader = dcw->elementHeader();
    if (domHeader != nullptr && !domHeader->text().isEmpty()) {
        // custom header unless it is a built-in qt class
        QString header;
        bool global = false;
        if (!m_classToHeader.contains(className)) {
            global = domHeader->attributeLocation().toLower() == QLatin1String("global");
            header = domHeader->text();
        }
        insertIncludeForClass(className, header, global);
        return;
    }
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
        const auto trimmed = QStringView(value).trimmed();
        if (!trimmed.isEmpty())
            m_output << "#include " << openingQuote << trimmed << closingQuote << '\n';
    }
}

} // namespace CPP

QT_END_NAMESPACE
