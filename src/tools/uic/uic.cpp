// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

using namespace Qt::StringLiterals;

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
    if (fileName.isEmpty()) {
        if (!f.open(stdin, QIODevice::ReadOnly))
            return false;
    } else {
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
    return  c == u'/' || c == u'*';
}

static qsizetype leadingCppCommentCharCount(QStringView s)
{
    qsizetype i = 0;
    for (const qsizetype size = s.size(); i < size && isCppCommentChar(s.at(i)); ++i) {
    }
    return i;
}

void Uic::writeCopyrightHeaderPython(const DomUI *ui) const
{
    QString comment = ui->elementComment();
    if (!comment.isEmpty()) {
        const auto lines = QStringView{comment}.split(u'\n');
        for (const auto &line : lines) {
            if (const auto leadingCommentChars = leadingCppCommentCharCount(line)) {
                 out << language::repeat(leadingCommentChars, '#')
                     << line.right(line.size() - leadingCommentChars);
            } else {
                if (!line.startsWith(u'#'))
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
    const auto versionAttribute = "version"_L1;
    if (!attributes.hasAttribute(versionAttribute))
        return 4.0;
    const QStringView version = attributes.value(versionAttribute);
    return version.toDouble();
}

DomUI *Uic::parseUiFile(QXmlStreamReader &reader)
{
    DomUI *ui = nullptr;

    const auto uiElement = "ui"_L1;
    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::StartElement) {
            if (reader.name().compare(uiElement, Qt::CaseInsensitive) == 0
                && !ui) {
                const double version = versionFromUiAttribute(reader);
                if (version < 4.0) {
                    const QString msg = QString::fromLatin1("uic: File generated with too old version of Qt Widgets Designer (%1)").arg(version);
                    fprintf(stderr, "%s\n", qPrintable(msg));
                    return nullptr;
                }

                ui = new DomUI();
                ui->read(reader);
            } else {
                reader.raiseError("Unexpected element "_L1 + reader.name().toString());
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
        fprintf(stderr, "uic: File generated with too old version of Qt Widgets Designer\n");
        return false;
    }

    const QString &language = ui->attributeLanguage();
    driver()->setUseIdBasedTranslations(ui->attributeIdbasedtr());

    if (!language.isEmpty() && language.compare("c++"_L1, Qt::CaseInsensitive) != 0) {
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
    if (pixFunction == "QPixmap::fromMimeSource"_L1 || pixFunction == "qPixmapFromMimeSource"_L1) {
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
        u"QRadioButton"_s, u"QToolButton"_s,
        u"QCheckBox"_s, u"QPushButton"_s,
        u"QCommandLinkButton"_s
    };
    return customWidgetsInfo()->extendsOneOf(className, buttons);
}

bool Uic::isContainer(const QString &className) const
{
    static const QStringList containers = {
        u"QStackedWidget"_s, u"QToolBox"_s,
        u"QTabWidget"_s, u"QScrollArea"_s,
        u"QMdiArea"_s, u"QWizard"_s,
        u"QDockWidget"_s
    };

    return customWidgetsInfo()->extendsOneOf(className, containers);
}

bool Uic::isMenu(const QString &className) const
{
    static const QStringList menus = {
        u"QMenu"_s, u"QPopupMenu"_s
    };
    return customWidgetsInfo()->extendsOneOf(className, menus);
}

QT_END_NAMESPACE
