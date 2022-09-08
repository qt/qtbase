// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uic.h"
#include "option.h"
#include "driver.h"
#include <language.h>

#include <qfile.h>
#include <qdir.h>
#include <qhashfunctions.h>
#include <qtextstream.h>
#include <qcoreapplication.h>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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

    QCommandLineOption connectionsOption(QStringList{QStringLiteral("c"), QStringLiteral("connections")});
    connectionsOption.setDescription(QStringLiteral("Connection syntax."));
    connectionsOption.setValueName(QStringLiteral("pmf|string"));
    parser.addOption(connectionsOption);

    QCommandLineOption idBasedOption(QStringLiteral("idbased"));
    idBasedOption.setDescription(QStringLiteral("Use id based function for i18n"));
    parser.addOption(idBasedOption);

    QCommandLineOption fromImportsOption(QStringLiteral("from-imports"));
    fromImportsOption.setDescription(QStringLiteral("Python: generate imports relative to '.'"));
    parser.addOption(fromImportsOption);

    // FIXME Qt 7: Flip the default?
    QCommandLineOption rcPrefixOption(QStringLiteral("rc-prefix"));
    rcPrefixOption.setDescription(QStringLiteral("Python: Generate \"rc_file\" instead of \"file_rc\" import"));
    parser.addOption(rcPrefixOption);

    // FIXME Qt 7: Remove?
    QCommandLineOption useStarImportsOption(QStringLiteral("star-imports"));
    useStarImportsOption.setDescription(QStringLiteral("Python: Use * imports"));
    parser.addOption(useStarImportsOption);

    parser.addPositionalArgument(QStringLiteral("[uifile]"), QStringLiteral("Input file (*.ui), otherwise stdin."));

    parser.process(app);

    driver.option().dependencies = parser.isSet(dependenciesOption);
    driver.option().outputFile = parser.value(outputOption);
    driver.option().autoConnection = !parser.isSet(noAutoConnectionOption);
    driver.option().headerProtection = !parser.isSet(noProtOption);
    driver.option().implicitIncludes = !parser.isSet(noImplicitIncludesOption);
    driver.option().idBased = parser.isSet(idBasedOption);
    driver.option().fromImports = parser.isSet(fromImportsOption);
    driver.option().useStarImports = parser.isSet(useStarImportsOption);
    driver.option().postfix = parser.value(postfixOption);
    driver.option().translateFunction = parser.value(translateOption);
    driver.option().includeFile = parser.value(includeOption);
    if (parser.isSet(connectionsOption)) {
        const auto value = parser.value(connectionsOption);
        if (value == "pmf"_L1)
            driver.option().forceMemberFnPtrConnectionSyntax = 1;
        else if (value == "string"_L1)
            driver.option().forceStringConnectionSyntax = 1;
    }

    if (parser.isSet(rcPrefixOption))
        driver.option().rcPrefix = 1;

    Language language = Language::Cpp;
    if (parser.isSet(generatorOption)) {
        if (parser.value(generatorOption).compare("python"_L1) == 0)
            language = Language::Python;
    }
    language::setLanguage(language);

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
        out->setEncoding(QStringConverter::Utf8);
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
