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

#include "uic.h"
#include "ui4.h"
#include "driver.h"
#include "option.h"
#include "treewalker.h"
#include "validator.h"

#include "cppwriteincludes.h"
#include "cppwritedeclaration.h"
#include <pythonwritedeclaration.h>
#include <pythonwriteimports.h>

#include <language.h>

#include <qxmlstream.h>
#include <qfileinfo.h>
#include <qscopedpointer.h>
#include <qtextstream.h>

QT_BEGIN_NAMESPACE

Uic::Uic(Driver *d)
     : drv(d),
       out(d->output()),
       opt(d->option())
{
}

Uic::~Uic() = default;

bool Uic::printDependencies()
{
    QString fileName = opt.inputFile;

    QFile f;
    if (fileName.isEmpty())
        f.open(stdin, QIODevice::ReadOnly);
    else {
        f.setFileName(fileName);
        if (!f.open(QIODevice::ReadOnly))
            return false;
    }

    DomUI *ui = nullptr;
    {
        QXmlStreamReader reader;
        reader.setDevice(&f);
        ui = parseUiFile(reader);
        if (!ui)
            return false;
    }

    if (DomIncludes *includes = ui->elementIncludes()) {
        const auto incls = includes->elementInclude();
        for (DomInclude *incl : incls) {
            QString file = incl->text();
            if (file.isEmpty())
                continue;

            fprintf(stdout, "%s\n", file.toLocal8Bit().constData());
        }
    }

    if (DomCustomWidgets *customWidgets = ui->elementCustomWidgets()) {
        const auto elementCustomWidget = customWidgets->elementCustomWidget();
        for (DomCustomWidget *customWidget : elementCustomWidget) {
            if (DomHeader *header = customWidget->elementHeader()) {
                QString file = header->text();
                if (file.isEmpty())
                    continue;

                fprintf(stdout, "%s\n", file.toLocal8Bit().constData());
            }
        }
    }

    delete ui;

    return true;
}

void Uic::writeCopyrightHeaderCpp(const DomUI *ui) const
{
    QString comment = ui->elementComment();
    if (!comment.isEmpty())
        out << "/*\n" << comment << "\n*/\n\n";

    out << "/********************************************************************************\n";
    out << "** Form generated from reading UI file '" << QFileInfo(opt.inputFile).fileName() << "'\n";
    out << "**\n";
    out << "** Created by: Qt User Interface Compiler version " << QT_VERSION_STR << "\n";
    out << "**\n";
    out << "** WARNING! All changes made in this file will be lost when recompiling UI file!\n";
    out << "********************************************************************************/\n\n";
}

// Format existing UI file comments for Python with some smartness : Replace all
// leading C++ comment characters by '#' or prepend '#' if needed.

static inline bool isCppCommentChar(QChar c)
{
    return  c == QLatin1Char('/') || c == QLatin1Char('*');
}

static int leadingCppCommentCharCount(const QStringRef &s)
{
    int i = 0;
    for (const int size = s.size(); i < size && isCppCommentChar(s.at(i)); ++i) {
    }
    return i;
}

void Uic::writeCopyrightHeaderPython(const DomUI *ui) const
{
    QString comment = ui->elementComment();
    if (!comment.isEmpty()) {
        const auto lines = comment.splitRef(QLatin1Char('\n'));
        for (const auto &line : lines) {
            if (const int leadingCommentChars = leadingCppCommentCharCount(line)) {
                 out << language::repeat(leadingCommentChars, '#')
                     << line.right(line.size() - leadingCommentChars);
            } else {
                if (!line.startsWith(QLatin1Char('#')))
                    out << "# ";
                out << line;
            }
            out << '\n';
        }
        out << '\n';
    }

    out << language::repeat(80, '#') << "\n## Form generated from reading UI file '"
        << QFileInfo(opt.inputFile).fileName()
        << "'\n##\n## Created by: Qt User Interface Compiler version " << QT_VERSION_STR
        << "\n##\n## WARNING! All changes made in this file will be lost when recompiling UI file!\n"
        << language::repeat(80, '#') << "\n\n";
}

// Check the version with a stream reader at the <ui> element.

static double versionFromUiAttribute(QXmlStreamReader &reader)
{
    const QXmlStreamAttributes attributes = reader.attributes();
    const QString versionAttribute = QLatin1String("version");
    if (!attributes.hasAttribute(versionAttribute))
        return 4.0;
    const QStringRef version = attributes.value(versionAttribute);
    return version.toDouble();
}

DomUI *Uic::parseUiFile(QXmlStreamReader &reader)
{
    DomUI *ui = nullptr;

    const QString uiElement = QLatin1String("ui");
    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::StartElement) {
            if (reader.name().compare(uiElement, Qt::CaseInsensitive) == 0
                && !ui) {
                const double version = versionFromUiAttribute(reader);
                if (version < 4.0) {
                    const QString msg = QString::fromLatin1("uic: File generated with too old version of Qt Designer (%1)").arg(version);
                    fprintf(stderr, "%s\n", qPrintable(msg));
                    return nullptr;
                }

                ui = new DomUI();
                ui->read(reader);
            } else {
                reader.raiseError(QLatin1String("Unexpected element ") + reader.name().toString());
            }
        }
    }
    if (reader.hasError()) {
        delete ui;
        ui = nullptr;
        fprintf(stderr, "%s\n", qPrintable(QString::fromLatin1("uic: Error in line %1, column %2 : %3")
                                    .arg(reader.lineNumber()).arg(reader.columnNumber())
                                    .arg(reader.errorString())));
    }

    return ui;
}

bool Uic::write(QIODevice *in)
{
    QScopedPointer<DomUI> ui;
    {
        QXmlStreamReader reader;
        reader.setDevice(in);
        ui.reset(parseUiFile(reader));
    }

    if (ui.isNull())
        return false;

    double version = ui->attributeVersion().toDouble();
    if (version < 4.0) {
        fprintf(stderr, "uic: File generated with too old version of Qt Designer\n");
        return false;
    }

    const QString &language = ui->attributeLanguage();
    driver()->setUseIdBasedTranslations(ui->attributeIdbasedtr());

    if (!language.isEmpty() && language.compare(QLatin1String("c++"), Qt::CaseInsensitive) != 0) {
        fprintf(stderr, "uic: File is not a \"c++\" ui file, language=%s\n", qPrintable(language));
        return false;
    }

    return write(ui.data());
}

bool Uic::write(DomUI *ui)
{
    if (!ui || !ui->elementWidget())
        return false;

    const auto lang = language::language();

    if (lang == Language::Python)
       out << "# -*- coding: utf-8 -*-\n\n";

    if (opt.copyrightHeader) {
        switch (language::language()) {
        case Language::Cpp:
            writeCopyrightHeaderCpp(ui);
            break;
        case Language::Python:
            writeCopyrightHeaderPython(ui);
            break;
        }
    }

    if (opt.headerProtection && lang == Language::Cpp) {
        writeHeaderProtectionStart();
        out << "\n";
    }

    pixFunction = ui->elementPixmapFunction();
    if (pixFunction == QLatin1String("QPixmap::fromMimeSource")
        || pixFunction == QLatin1String("qPixmapFromMimeSource")) {
        fprintf(stderr, "%s: Warning: Obsolete pixmap function '%s' specified in the UI file.\n",
                qPrintable(opt.messagePrefix()), qPrintable(pixFunction));
        pixFunction.clear();
    }

    info.acceptUI(ui);
    cWidgetsInfo.acceptUI(ui);

    switch (language::language()) {
    case Language::Cpp: {
        CPP::WriteIncludes writeIncludes(this);
        writeIncludes.acceptUI(ui);
        Validator(this).acceptUI(ui);
        CPP::WriteDeclaration(this).acceptUI(ui);
    }
        break;
    case Language::Python: {
        Python::WriteImports writeImports(this);
        writeImports.acceptUI(ui);
        Validator(this).acceptUI(ui);
        Python::WriteDeclaration(this).acceptUI(ui);
    }
        break;
    }

    if (opt.headerProtection && lang == Language::Cpp)
        writeHeaderProtectionEnd();

    return true;
}

void Uic::writeHeaderProtectionStart()
{
    QString h = drv->headerFileName();
    out << "#ifndef " << h << "\n"
        << "#define " << h << "\n";
}

void Uic::writeHeaderProtectionEnd()
{
    QString h = drv->headerFileName();
    out << "#endif // " << h << "\n";
}

bool Uic::isButton(const QString &className) const
{
    static const QStringList buttons = {
        QLatin1String("QRadioButton"), QLatin1String("QToolButton"),
        QLatin1String("QCheckBox"), QLatin1String("QPushButton"),
        QLatin1String("QCommandLinkButton")
    };
    return customWidgetsInfo()->extendsOneOf(className, buttons);
}

bool Uic::isContainer(const QString &className) const
{
    static const QStringList containers = {
        QLatin1String("QStackedWidget"), QLatin1String("QToolBox"),
        QLatin1String("QTabWidget"), QLatin1String("QScrollArea"),
        QLatin1String("QMdiArea"), QLatin1String("QWizard"),
        QLatin1String("QDockWidget")
    };

    return customWidgetsInfo()->extendsOneOf(className, containers);
}

bool Uic::isMenu(const QString &className) const
{
    static const QStringList menus = {
        QLatin1String("QMenu"), QLatin1String("QPopupMenu")
    };
    return customWidgetsInfo()->extendsOneOf(className, menus);
}

QT_END_NAMESPACE
