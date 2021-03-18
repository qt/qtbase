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
#include "option.h"
#include "driver.h"
#include <language.h>

#include <qfile.h>
#include <qdir.h>
#include <qhashfunctions.h>
#include <qtextstream.h>
#include <qtextcodec.h>
#include <qcoreapplication.h>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>

QT_BEGIN_NAMESPACE

int runUic(int argc, char *argv[])
{
    qSetGlobalQHashSeed(0);    // set the hash seed to 0

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QString::fromLatin1(QT_VERSION_STR));

    Driver driver;

    // Note that uic isn't translated.
    // If you use this code as an example for a translated app, make sure to translate the strings.
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription(QStringLiteral("Qt User Interface Compiler version %1").arg(QString::fromLatin1(QT_VERSION_STR)));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dependenciesOption(QStringList() << QStringLiteral("d") << QStringLiteral("dependencies"));
    dependenciesOption.setDescription(QStringLiteral("Display the dependencies."));
    parser.addOption(dependenciesOption);

    QCommandLineOption outputOption(QStringList() << QStringLiteral("o") << QStringLiteral("output"));
    outputOption.setDescription(QStringLiteral("Place the output into <file>"));
    outputOption.setValueName(QStringLiteral("file"));
    parser.addOption(outputOption);

    QCommandLineOption noAutoConnectionOption(QStringList() << QStringLiteral("a") << QStringLiteral("no-autoconnection"));
    noAutoConnectionOption.setDescription(QStringLiteral("Do not generate a call to QObject::connectSlotsByName()."));
    parser.addOption(noAutoConnectionOption);

    QCommandLineOption noProtOption(QStringList() << QStringLiteral("p") << QStringLiteral("no-protection"));
    noProtOption.setDescription(QStringLiteral("Disable header protection."));
    parser.addOption(noProtOption);

    QCommandLineOption noImplicitIncludesOption(QStringList() << QStringLiteral("n") << QStringLiteral("no-implicit-includes"));
    noImplicitIncludesOption.setDescription(QStringLiteral("Disable generation of #include-directives."));
    parser.addOption(noImplicitIncludesOption);

    QCommandLineOption noStringLiteralOption(QStringList() << QStringLiteral("s") << QStringLiteral("no-stringliteral"));
    noStringLiteralOption.setDescription(QStringLiteral("Deprecated. The use of this option won't take any effect."));
    parser.addOption(noStringLiteralOption);

    QCommandLineOption postfixOption(QStringLiteral("postfix"));
    postfixOption.setDescription(QStringLiteral("Postfix to add to all generated classnames."));
    postfixOption.setValueName(QStringLiteral("postfix"));
    parser.addOption(postfixOption);

    QCommandLineOption translateOption(QStringList() << QStringLiteral("tr") << QStringLiteral("translate"));
    translateOption.setDescription(QStringLiteral("Use <function> for i18n."));
    translateOption.setValueName(QStringLiteral("function"));
    parser.addOption(translateOption);

    QCommandLineOption includeOption(QStringList() << QStringLiteral("include"));
    includeOption.setDescription(QStringLiteral("Add #include <include-file> to <file>."));
    includeOption.setValueName(QStringLiteral("include-file"));
    parser.addOption(includeOption);

    QCommandLineOption generatorOption(QStringList() << QStringLiteral("g") << QStringLiteral("generator"));
    generatorOption.setDescription(QStringLiteral("Select generator."));
    generatorOption.setValueName(QStringLiteral("python|cpp"));
    parser.addOption(generatorOption);

    QCommandLineOption idBasedOption(QStringLiteral("idbased"));
    idBasedOption.setDescription(QStringLiteral("Use id based function for i18n"));
    parser.addOption(idBasedOption);

    QCommandLineOption fromImportsOption(QStringLiteral("from-imports"));
    fromImportsOption.setDescription(QStringLiteral("Python: generate imports relative to '.'"));
    parser.addOption(fromImportsOption);

    parser.addPositionalArgument(QStringLiteral("[uifile]"), QStringLiteral("Input file (*.ui), otherwise stdin."));

    parser.process(app);

    driver.option().dependencies = parser.isSet(dependenciesOption);
    driver.option().outputFile = parser.value(outputOption);
    driver.option().autoConnection = !parser.isSet(noAutoConnectionOption);
    driver.option().headerProtection = !parser.isSet(noProtOption);
    driver.option().implicitIncludes = !parser.isSet(noImplicitIncludesOption);
    driver.option().idBased = parser.isSet(idBasedOption);
    driver.option().fromImports = parser.isSet(fromImportsOption);
    driver.option().postfix = parser.value(postfixOption);
    driver.option().translateFunction = parser.value(translateOption);
    driver.option().includeFile = parser.value(includeOption);

    Language language = Language::Cpp;
    if (parser.isSet(generatorOption)) {
        if (parser.value(generatorOption).compare(QLatin1String("python")) == 0)
            language = Language::Python;
    }
    language::setLanguage(language);

    if (parser.isSet(noStringLiteralOption))
        fprintf(stderr, "The -s, --no-stringliteral option is deprecated and it won't take any effect.\n");

    QString inputFile;
    if (!parser.positionalArguments().isEmpty())
        inputFile = parser.positionalArguments().at(0);
    else // reading from stdin
        driver.option().headerProtection = false;

    if (driver.option().dependencies) {
        return !driver.printDependencies(inputFile);
    }

    QTextStream *out = nullptr;
    QFile f;
    if (!driver.option().outputFile.isEmpty()) {
        f.setFileName(driver.option().outputFile);
        if (!f.open(QIODevice::WriteOnly | QFile::Text)) {
            fprintf(stderr, "Could not create output file\n");
            return 1;
        }
        out = new QTextStream(&f);
#if QT_CONFIG(textcodec)
        out->setCodec(QTextCodec::codecForName("UTF-8"));
#endif
    }

    bool rtn = driver.uic(inputFile, out);
    delete out;

    if (!rtn) {
        if (driver.option().outputFile.size()) {
            f.close();
            f.remove();
        }
        fprintf(stderr, "File '%s' is not valid\n", inputFile.isEmpty() ? "<stdin>" : inputFile.toLocal8Bit().constData());
    }

    return !rtn;
}

QT_END_NAMESPACE

int main(int argc, char *argv[])
{
    return QT_PREPEND_NAMESPACE(runUic)(argc, argv);
}
