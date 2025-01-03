// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonwriteimports.h"

#include <customwidgetsinfo.h>
#include <option.h>
#include <uic.h>
#include <driver.h>

#include <ui4.h>

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qtextstream.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
        m_classToModule.insert(QLatin1StringView(e.klass), QLatin1StringView(e.module));
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
                writeResourceImport(include->attributeLocation());
        }
        output << '\n';
    }
}

QString WriteImports::resourceAbsolutePath(QString resource) const
{
    // If we know the project root, generate an absolute Python import
    // to the resource. options. pythonRoot is the Python path component
    // under which the UI file is.
    const auto &options = uic()->option();
    if (!options.inputFile.isEmpty() && !options.pythonRoot.isEmpty()) {
        resource = QDir::cleanPath(QFileInfo(options.inputFile).canonicalPath() + u'/' + resource);
        if (resource.size() > options.pythonRoot.size())
            resource.remove(0, options.pythonRoot.size() + 1);
    }
    // If nothing is known, we assume the directory pointed by "../" is the root
    while (resource.startsWith(u"../"))
        resource.remove(0, 3);
    resource.replace(u'/', u'.');
    return resource;
}

void WriteImports::writeResourceImport(const QString &module)
{
    const auto &options = uic()->option();
    auto &str = uic()->output();

    QString resource = QDir::cleanPath(module);
    if (resource.endsWith(u".qrc"))
        resource.chop(4);
    const qsizetype basePos = resource.lastIndexOf(u'/') + 1;
    // Change the name of a qrc file "dir/foo.qrc" file to the Python
    // module name "foo_rc" according to project conventions.
    if (options.rcPrefix)
        resource.insert(basePos, u"rc_");
    else
        resource.append(u"_rc");

    switch (options.pythonResourceImport) {
    case Option::PythonResourceImport::Default:
        str << "import " << QStringView{resource}.sliced(basePos) << '\n';
        break;
    case Option::PythonResourceImport::FromDot:
        str << "from . import " << QStringView{resource}.sliced(basePos) << '\n';
        break;
    case Option::PythonResourceImport::Absolute:
        str << "import " << resourceAbsolutePath(resource) << '\n';
        break;
    }
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
    if (className.contains("::"_L1))
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
        modulePath.replace(u'/', u'.');
        // '.h' is added by default on headers for <customwidget>
        if (modulePath.endsWith(".h"_L1))
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
