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
#include <qfileinfo.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static const char pythonPathVar[] = "PYTHONPATH";

// From the Python paths, find the component the UI file is under
static QString pythonRoot(const QString &pythonPath, const QString &uiFileIn)
{
#ifdef Q_OS_WIN
    static const Qt::CaseSensitivity fsSensitivity = Qt::CaseInsensitive;
#else
    static const Qt::CaseSensitivity fsSensitivity = Qt::CaseSensitive;
#endif

    if (pythonPath.isEmpty() || uiFileIn.isEmpty())
        return {};
    const QString uiFile = QFileInfo(uiFileIn).canonicalFilePath();
    if (uiFile.isEmpty())
        return {};
    const auto uiFileSize = uiFile.size();
    const auto paths = pythonPath.split(QDir::listSeparator(), Qt::SkipEmptyParts);
    for (const auto &path : paths) {
        const QString canonicalPath = QFileInfo(path).canonicalFilePath();
        const auto canonicalPathSize = canonicalPath.size();
        if (uiFileSize > canonicalPathSize
            && uiFile.at(canonicalPathSize) == u'/'
            && uiFile.startsWith(canonicalPath, fsSensitivity)) {
            return canonicalPath;
        }
    }
    return {};
}

int runUic(int argc, char *argv[])
{
    QHashSeed::setDeterministicGlobalSeed();

    QCoreApplication app(argc, argv);
    const QString version = QString::fromLatin1(qVersion());
    QCoreApplication::setApplicationVersion(version);

    Driver driver;

    // Note that uic isn't translated.
    // If you use this code as an example for a translated app, make sure to translate the strings.
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription(u"Qt User Interface Compiler version %1"_s.arg(version));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dependenciesOption(QStringList{u"d"_s, u"dependencies"_s});
    dependenciesOption.setDescription(u"Display the dependencies."_s);
    parser.addOption(dependenciesOption);

    QCommandLineOption outputOption(QStringList{u"o"_s, u"output"_s});
    outputOption.setDescription(u"Place the output into <file>"_s);
    outputOption.setValueName(u"file"_s);
    parser.addOption(outputOption);

    QCommandLineOption noAutoConnectionOption(QStringList{u"a"_s, u"no-autoconnection"_s});
    noAutoConnectionOption.setDescription(u"Do not generate a call to QObject::connectSlotsByName()."_s);
    parser.addOption(noAutoConnectionOption);

    QCommandLineOption noProtOption(QStringList{u"p"_s, u"no-protection"_s});
    noProtOption.setDescription(u"Disable header protection."_s);
    parser.addOption(noProtOption);

    QCommandLineOption noImplicitIncludesOption(QStringList{u"n"_s, u"no-implicit-includes"_s});
    noImplicitIncludesOption.setDescription(u"Disable generation of #include-directives."_s);
    parser.addOption(noImplicitIncludesOption);

    QCommandLineOption postfixOption(u"postfix"_s);
    postfixOption.setDescription(u"Postfix to add to all generated classnames."_s);
    postfixOption.setValueName(u"postfix"_s);
    parser.addOption(postfixOption);

    QCommandLineOption noQtNamespaceOption(u"no-qt-namespace"_s);
    noQtNamespaceOption.setDescription(
        u"Disable wrapping the definition of the generated class in QT_{BEGIN,END}_NAMESPACE."_s);
    parser.addOption(noQtNamespaceOption);

    QCommandLineOption translateOption(QStringList{u"tr"_s, u"translate"_s});
    translateOption.setDescription(u"Use <function> for i18n."_s);
    translateOption.setValueName(u"function"_s);
    parser.addOption(translateOption);

    QCommandLineOption includeOption(QStringList{u"include"_s});
    includeOption.setDescription(u"Add #include <include-file> to <file>."_s);
    includeOption.setValueName(u"include-file"_s);
    parser.addOption(includeOption);

    QCommandLineOption generatorOption(QStringList{u"g"_s, u"generator"_s});
    generatorOption.setDescription(u"Select generator."_s);
    generatorOption.setValueName(u"python|cpp"_s);
    parser.addOption(generatorOption);

    QCommandLineOption connectionsOption(QStringList{u"c"_s, u"connections"_s});
    connectionsOption.setDescription(u"Connection syntax."_s);
    connectionsOption.setValueName(u"pmf|string"_s);
    parser.addOption(connectionsOption);

    QCommandLineOption idBasedOption(u"idbased"_s);
    idBasedOption.setDescription(u"Use id based function for i18n"_s);
    parser.addOption(idBasedOption);

    QCommandLineOption fromImportsOption(u"from-imports"_s);
    fromImportsOption.setDescription(u"Python: generate imports relative to '.'"_s);
    parser.addOption(fromImportsOption);

    QCommandLineOption absoluteImportsOption(u"absolute-imports"_s);
    absoluteImportsOption.setDescription(u"Python: generate absolute imports"_s);
    parser.addOption(absoluteImportsOption);

    // FIXME Qt 7: Flip the default?
    QCommandLineOption rcPrefixOption(u"rc-prefix"_s);
    rcPrefixOption.setDescription(uR"(Python: Generate "rc_file" instead of "file_rc" import)"_s);
    parser.addOption(rcPrefixOption);

    // FIXME Qt 7: Remove?
    QCommandLineOption useStarImportsOption(u"star-imports"_s);
    useStarImportsOption.setDescription(u"Python: Use * imports"_s);
    parser.addOption(useStarImportsOption);

    QCommandLineOption pythonPathOption(u"python-paths"_s);
    pythonPathOption.setDescription(u"Python paths for --absolute-imports."_s);
    pythonPathOption.setValueName(u"pathlist"_s);
    parser.addOption(pythonPathOption);

    parser.addPositionalArgument(u"[uifile]"_s, u"Input file (*.ui), otherwise stdin."_s);

    parser.process(app);

    driver.option().dependencies = parser.isSet(dependenciesOption);
    driver.option().outputFile = parser.value(outputOption);
    driver.option().autoConnection = !parser.isSet(noAutoConnectionOption);
    driver.option().headerProtection = !parser.isSet(noProtOption);
    driver.option().implicitIncludes = !parser.isSet(noImplicitIncludesOption);
    driver.option().qtNamespace = !parser.isSet(noQtNamespaceOption);
    driver.option().idBased = parser.isSet(idBasedOption);
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

    const QString inputFile = parser.positionalArguments().value(0);

    Language language = Language::Cpp;
    if (parser.isSet(generatorOption)) {
        if (parser.value(generatorOption).compare("python"_L1) == 0)
            language = Language::Python;
    }
    language::setLanguage(language);
    if (language == Language::Python) {
        if (parser.isSet(fromImportsOption))
            driver.option().pythonResourceImport = Option::PythonResourceImport::FromDot;
        else if (parser.isSet(absoluteImportsOption))
            driver.option().pythonResourceImport = Option::PythonResourceImport::Absolute;
        driver.option().useStarImports = parser.isSet(useStarImportsOption);
        if (parser.isSet(rcPrefixOption))
            driver.option().rcPrefix = 1;
        QString pythonPaths;
        if (parser.isSet(pythonPathOption))
            pythonPaths = parser.value(pythonPathOption);
        else if (qEnvironmentVariableIsSet(pythonPathVar))
            pythonPaths = qEnvironmentVariable(pythonPathVar);
        driver.option().pythonRoot = pythonRoot(pythonPaths, inputFile);
    }

    if (inputFile.isEmpty()) // reading from stdin
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
