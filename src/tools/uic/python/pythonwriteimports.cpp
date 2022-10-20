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
#include <driver.h>

#include <ui4.h>

#include <QtCore/qtextstream.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

// Generate imports for Python. Note some things differ from C++:
// - qItemView->header()->setFoo() does not require QHeaderView to be imported
// - qLabel->setFrameShape(QFrame::Box) however requires QFrame to be imported
//   (see acceptProperty())

namespace Python {

// Classes required for properties
static WriteImports::ClassesPerModule defaultClasses()
{
    return {
        {QStringLiteral("QtCore"),
            {QStringLiteral("QCoreApplication"), QStringLiteral("QDate"),
             QStringLiteral("QDateTime"), QStringLiteral("QLocale"),
             QStringLiteral("QMetaObject"), QStringLiteral("QObject"),
             QStringLiteral("QPoint"), QStringLiteral("QRect"),
             QStringLiteral("QSize"), QStringLiteral("QTime"),
             QStringLiteral("QUrl"), QStringLiteral("Qt")},
        },
        {QStringLiteral("QtGui"),
            {QStringLiteral("QBrush"), QStringLiteral("QColor"),
             QStringLiteral("QConicalGradient"), QStringLiteral("QCursor"),
             QStringLiteral("QGradient"), QStringLiteral("QFont"),
             QStringLiteral("QFontDatabase"), QStringLiteral("QIcon"),
             QStringLiteral("QImage"), QStringLiteral("QKeySequence"),
             QStringLiteral("QLinearGradient"), QStringLiteral("QPalette"),
             QStringLiteral("QPainter"), QStringLiteral("QPixmap"),
             QStringLiteral("QTransform"), QStringLiteral("QRadialGradient")}
        },
        // Add QWidget for QWidget.setTabOrder()
        {QStringLiteral("QtWidgets"),
          {QStringLiteral("QSizePolicy"), QStringLiteral("QWidget")}
        }
    };
}

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

// Helpers for WriteImports::ClassesPerModule maps
static void insertClass(const QString &module, const QString &className,
                        WriteImports::ClassesPerModule *c)
{
    auto usedIt = c->find(module);
    if (usedIt == c->end())
        c->insert(module, {className});
    else if (!usedIt.value().contains(className))
        usedIt.value().append(className);
}

// Format a class list: "from A import (B, C)"
static void formatImportClasses(QTextStream &str, QStringList classList)
{
    std::sort(classList.begin(), classList.end());

    const qsizetype size = classList.size();
    if (size > 1)
        str << '(';
    for (qsizetype i = 0; i < size; ++i) {
        if (i > 0)
            str << (i % 4 == 0 ? ",\n    " : ", ");
        str << classList.at(i);
    }
    if (size > 1)
        str << ')';
}

static void formatClasses(QTextStream &str, const WriteImports::ClassesPerModule &c,
                          bool useStarImports = false,
                          const QByteArray &modulePrefix = {})
{
    for (auto it = c.cbegin(), end = c.cend(); it != end; ++it) {
        str << "from " << modulePrefix << it.key() << " import ";
        if (useStarImports)
            str << "*  # type: ignore";
        else
            formatImportClasses(str, it.value());
        str << '\n';
    }
}

WriteImports::WriteImports(Uic *uic) : WriteIncludesBase(uic),
    m_qtClasses(defaultClasses())
{
    for (const auto &e : classInfoEntries())
        m_classToModule.insert(QLatin1String(e.klass), QLatin1String(e.module));
}

void WriteImports::acceptUI(DomUI *node)
{
    WriteIncludesBase::acceptUI(node);

    auto &output = uic()->output();
    const bool useStarImports = uic()->driver()->option().useStarImports;

    const QByteArray qtPrefix = QByteArrayLiteral("PySide")
        + QByteArray::number(QT_VERSION_MAJOR) + '.';

    formatClasses(output, m_qtClasses, useStarImports, qtPrefix);

    if (!m_customWidgets.isEmpty() || !m_plainCustomWidgets.isEmpty()) {
        output << '\n';
        formatClasses(output, m_customWidgets, useStarImports);
        for (const auto &w : m_plainCustomWidgets)
            output << "import " << w << '\n';
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
    if (uic()->option().fromImports)
        uic()->output() << "from  . ";
    uic()->output() << "import " << module << '\n';
}

void WriteImports::doAdd(const QString &className, const DomCustomWidget *dcw)
{
    const CustomWidgetsInfo *cwi = uic()->customWidgetsInfo();
    if (cwi->extends(className, "QListWidget"))
        add(QStringLiteral("QListWidgetItem"));
    else if (cwi->extends(className, "QTreeWidget"))
        add(QStringLiteral("QTreeWidgetItem"));
    else if (cwi->extends(className, "QTableWidget"))
        add(QStringLiteral("QTableWidgetItem"));

    if (dcw != nullptr) {
        addPythonCustomWidget(className, dcw);
        return;
    }

    if (!addQtClass(className))
        qWarning("WriteImports::add(): Unknown Qt class %s", qPrintable(className));
}

bool WriteImports::addQtClass(const QString &className)
{
    // QVariant is not exposed in PySide
    if (className == u"QVariant" || className == u"Qt")
        return true;

    const auto moduleIt = m_classToModule.constFind(className);
    const bool result = moduleIt != m_classToModule.cend();
    if (result)
        insertClass(moduleIt.value(), className, &m_qtClasses);
    return result;
}

void WriteImports::addPythonCustomWidget(const QString &className, const DomCustomWidget *node)
{
    if (className.contains(QLatin1String("::")))
        return; // Exclude namespaced names (just to make tests pass).

    if (addQtClass(className))  // Qt custom widgets like QQuickWidget, QAxWidget, etc
        return;

    // When the elementHeader is not set, we know it's the continuation
    // of a Qt for Python import or a normal import of another module.
    if (!node->elementHeader() || node->elementHeader()->text().isEmpty()) {
        m_plainCustomWidgets.append(className);
    } else { // When we do have elementHeader, we know it's a relative import.
        QString modulePath = node->elementHeader()->text();
        // Replace the '/' by '.'
        modulePath.replace(QLatin1Char('/'), QLatin1Char('.'));
        // '.h' is added by default on headers for <customwidget>
        if (modulePath.endsWith(QLatin1String(".h")))
            modulePath.chop(2);
        insertClass(modulePath, className, &m_customWidgets);
    }
}

void WriteImports::acceptProperty(DomProperty *node)
{
    switch (node->kind()) {
    case DomProperty::Enum:
        addEnumBaseClass(node->elementEnum());
        break;
    case DomProperty::Set:
        addEnumBaseClass(node->elementSet());
        break;
    default:
        break;
    }

    WriteIncludesBase::acceptProperty(node);
}

void WriteImports::addEnumBaseClass(const QString &v)
{
    // Add base classes like QFrame for QLabel::frameShape()
    const auto colonPos = v.indexOf(u"::");
    if (colonPos > 0) {
        const QString base = v.left(colonPos);
        if (base.startsWith(u'Q') && base != u"Qt")
            addQtClass(base);
    }
}

} // namespace Python

QT_END_NAMESPACE
